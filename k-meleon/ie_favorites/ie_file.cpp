/*
 * Copyright (C) 2000 Brian Harris
 * Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#else
#  include <shlobj.h>
#endif


#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#include "ie_favorites.h"

#define REG_USER_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define REG_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define REG_FAVORITES_KEY _T("Favorites")

#define MAX_FAVORITES 512
int gNumFavorites = 0;


int GetFavoritesPath(void)
{
   gFavoritesPath[0] = 0;

   long rslt = ERROR_SUCCESS;

   TCHAR           sz[MAX_PATH];
   HKEY            hKey;
   DWORD           dwSize;

#ifndef __MINGW32__
   ITEMIDLIST *idl;

   // first try the correct way
   if (SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &idl) == NOERROR) {
      IMalloc *malloc;
      SHGetPathFromIDList(idl, sz);
      SHGetMalloc(&malloc);
      malloc->Free(idl);
      malloc->Release();
   }  
   else 
#endif
   {
      // if the correct way failed, find out from the registry where the favorites are located.
      if(RegOpenKey(HKEY_CURRENT_USER, REG_USER_SHELL_FOLDERS, &hKey) == ERROR_SUCCESS) {
         dwSize = MAX_PATH;
         rslt = RegQueryValueEx(hKey, REG_FAVORITES_KEY, NULL, NULL, (LPBYTE)sz, &dwSize);
         RegCloseKey(hKey);

         if (rslt != ERROR_SUCCESS) {
            if (RegOpenKey(HKEY_CURRENT_USER, REG_SHELL_FOLDERS, &hKey) == ERROR_SUCCESS) {
               rslt = RegQueryValueEx(hKey, REG_FAVORITES_KEY, NULL, NULL, (LPBYTE)sz, &dwSize);
               RegCloseKey(hKey);
            }
            else {
// FIXME - error messagebox here
//               TRACE0("Favorites folder not found\n");
//               rslt = -1;
            }
         }
      }
      else {
// FIXME - error messagebox here
//         TRACE0("Favorites folder not found\n");
//         rslt = -1;
      }
   }
   if (rslt == ERROR_SUCCESS) {
      ExpandEnvironmentStrings(sz, gFavoritesPath, MAX_PATH);

      strcat(gFavoritesPath, "\\");
   }
   else {
      gFavoritesPath[0] = 0;
   }


   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_FAVORITES_PATH, gFavoritesPath, gFavoritesPath);

   return 1;
}

static bool found_tb = false;
static bool found_bm = false;
static bool found_nb = false;

int ReadFavorites(char *szRoot, char *szPath, CBookmarkNode &newFavoritesNode)
{
   int numFavoritesInFolder = 0;

   int pathLen = strlen(szPath);
   int gFavoritesPathLen = strlen(szRoot);
   
   char * searchString = new char[gFavoritesPathLen + pathLen + 2];
   strcpy(searchString, szRoot);
   strcat(searchString, szPath);
   strcat(searchString, "*");
   
   char * urlFile;
   char * subPath;
   
   // now scan the directory, first for .URL files and then for subdirectories
   // that may also contain .URL files
   WIN32_FIND_DATA wfd;
   HANDLE h = FindFirstFile(searchString, &wfd);
   
   delete [] searchString;
   
   if(h == INVALID_HANDLE_VALUE) {
      return numFavoritesInFolder;
   }
   
   do {
      if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         // ignore the current and parent directory entries
         if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
            continue;
         
         subPath = new char[pathLen + strlen(wfd.cFileName) + 2];
         strcpy(subPath, szPath);
         strcat(subPath, wfd.cFileName);
         strcat(subPath, "/");
         
         // make new node for favorites (child off of our current node)
         CBookmarkNode *newFavoritesChildNode = new CBookmarkNode(0, wfd.cFileName, "", BOOKMARK_FOLDER, time(NULL));
         newFavoritesNode.AddChild(newFavoritesChildNode);
         
         // build the tree for this directory
         numFavoritesInFolder += 
            ReadFavorites(szRoot, subPath, *newFavoritesChildNode);

         if ( !found_tb && gToolbarFolder[0] != 0 &&
              (strncmp(subPath, gToolbarFolder, strlen(subPath)-1) == 0)) {
            newFavoritesChildNode->flags |= BOOKMARK_FLAG_TB;
            found_tb = true;
         }
         if ( !found_bm && gMenuFolder[0] != 0 && 
              (strncmp(subPath, gMenuFolder, strlen(subPath)-1) == 0)) {
               newFavoritesChildNode->flags |= BOOKMARK_FLAG_BM;
               found_bm = true;
            }
         if ( !found_nb && gNewitemFolder[0] != 0 && 
              (strncmp(subPath, gNewitemFolder, strlen(subPath)-1) == 0)) {
               newFavoritesChildNode->flags |= BOOKMARK_FLAG_NB;
               found_nb = true;
            }
            
         delete [] subPath;
         
      } else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file
         
         char *dot = strrchr(wfd.cFileName, '.');
         if(dot && (dot != wfd.cFileName) && stricmp(dot, ".url") == 0) {
            
            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new char[pathLen + filenameLen + 1];
            strcpy(urlFile, szPath);
            strcat(urlFile, wfd.cFileName);
            
            // format for display in the menu
            // chop off the .url
            *dot = 0;

            // condense the string and escape ampersands
            char *pszTemp = fixString(wfd.cFileName, 40);
            
            // insert node
            newFavoritesNode.AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), wfd.cFileName, urlFile, BOOKMARK_BOOKMARK, time(NULL)));
            
            delete pszTemp;
            szRoot[gFavoritesPathLen] = 0;
            delete [] urlFile;
            
            numFavoritesInFolder++;
         }
      }
   } while(FindNextFile(h, &wfd));
   
   FindClose(h);
   
   return numFavoritesInFolder;
}

int CreateFavorite(CBookmarkNode *newNode) 
{
   char name[INTERNET_MAX_URL_LENGTH];
   char filename[INTERNET_MAX_URL_LENGTH];

   strcpy(name, newNode->text.c_str());

   // remove any characters the filesystem won't like
   char *c, *d;
   c = d = name;
   while (*d) {
      if (strchr("\\:*?\"<>|", *d)) {
         d++;
      }
      else if(*d == '/') {
         *c++ = '-';
         d++;
      }
      else {
         *c++ = *d++;
      }
   }
   *c = 0;

   for (int i=0;; i++) {
      if (i) {
         char str[10];
         itoa(i, str, 10);
         strcpy(c, "[");
         strcat(c, str);
         strcat(c, "]");
      }
      strcpy(filename, gFavoritesPath);
      if (gNewitemFolder[0] && 
          gFavoritesRoot.FindSpecialNode(BOOKMARK_FLAG_NB)) {
         strcat(filename, gNewitemFolder);
         if (filename[strlen(filename)-1] != '\\')
            strcat(filename, "\\");
      }
      strcat(filename, name);
      strcat(filename, _T(".url"));
      FILE *bmFile = fopen(filename, "r");
      if (bmFile){
         fclose(bmFile);
      }
      else {
         break;
      }
   }

   WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), newNode->url.c_str(), filename);

   newNode->text = name;
   strcat(name, _T(".url"));
   newNode->path = name;

   return 0;
}
