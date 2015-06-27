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

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

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

      _tcscat(gFavoritesPath, _T("\\"));
   }
   else {
      gFavoritesPath[0] = 0;
   }


   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_FAVORITES_PATH, gFavoritesPath, gFavoritesPath);

   return 1;
}

static bool found_tb = false;
static bool found_bm = false;
static bool found_nb = false;

int ReadFavorites(TCHAR *szRoot, TCHAR *szPath, CBookmarkNode &newFavoritesNode)
{
   int numFavoritesInFolder = 0;
   
   if (!szPath[0]) {
      found_tb = false;
      found_bm = false;
      found_nb = false;
   }

   int pathLen = _tcslen(szPath);
   int gFavoritesPathLen = _tcslen(szRoot);
   
   TCHAR * searchString = new TCHAR[gFavoritesPathLen + pathLen + 2];
   _tcscpy(searchString, szRoot);
   _tcscat(searchString, szPath);
   _tcscat(searchString, _T("*"));
   
   TCHAR * urlFile;
   TCHAR * subPath;
   
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
         
         subPath = new TCHAR[pathLen + _tcslen(wfd.cFileName) + 2];
         _tcscpy(subPath, szPath);
         _tcscat(subPath, wfd.cFileName);
         _tcscat(subPath, _T("/"));
         
         // make new node for favorites (child off of our current node)
         CBookmarkNode *newFavoritesChildNode = new CBookmarkNode(0, wfd.cFileName, _T(""), BOOKMARK_FOLDER, time(NULL));
         newFavoritesNode.AddChild(newFavoritesChildNode);
         
         // build the tree for this directory
         numFavoritesInFolder += 
            ReadFavorites(szRoot, subPath, *newFavoritesChildNode);

         if ( !found_tb && gToolbarFolder[0] != 0 &&
              (_tcsncmp(subPath, gToolbarFolder, _tcslen(subPath)-1) == 0)) {
            newFavoritesChildNode->flags |= BOOKMARK_FLAG_TB;
            found_tb = true;
         }
         if ( !found_bm && gMenuFolder[0] != 0 && 
              (_tcsncmp(subPath, gMenuFolder, _tcslen(subPath)-1) == 0)) {
               newFavoritesChildNode->flags |= BOOKMARK_FLAG_BM;
               found_bm = true;
            }
         if ( !found_nb && gNewitemFolder[0] != 0 && 
              (_tcsncmp(subPath, gNewitemFolder, _tcslen(subPath)-1) == 0)) {
               newFavoritesChildNode->flags |= BOOKMARK_FLAG_NB;
               found_nb = true;
            }
            
         delete [] subPath;
         
      } else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file
         
         TCHAR *dot = _tcsrchr(wfd.cFileName, _T('.'));
         if(dot && (dot != wfd.cFileName) && _tcsicmp(dot, _T(".url")) == 0) {
            
            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new TCHAR[pathLen + filenameLen + 1];
            _tcscpy(urlFile, szPath);
            _tcscat(urlFile, wfd.cFileName);
            
            // format for display in the menu
            // chop off the .url
            *dot = 0;

            // condense the string and escape ampersands
            TCHAR *pszTemp = fixString(wfd.cFileName, 40);
            
            // insert node
            newFavoritesNode.AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), wfd.cFileName, urlFile, BOOKMARK_BOOKMARK, time(NULL)));
            
            free(pszTemp);
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
   TCHAR name[INTERNET_MAX_URL_LENGTH];
   TCHAR filename[INTERNET_MAX_URL_LENGTH];

   _tcscpy(name, newNode->text.c_str());

   // remove any characters the filesystem won't like
   TCHAR *c, *d;
   c = d = name;
   while (*d) {
      if (_tcschr(_T("\\:*?\"<>|"), *d)) {
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
         TCHAR str[10];
         _itot(i, str, 10);
         _tcscpy(c, _T("["));
         _tcscat(c, str);
         _tcscat(c, _T("]"));
      }
      _tcscpy(filename, gFavoritesPath);
      if (gNewitemFolder[0] && 
          gFavoritesRoot.FindSpecialNode(BOOKMARK_FLAG_NB)) {
         _tcscat(filename, gNewitemFolder);
         if (filename[_tcslen(filename)-1] != _T('\\'))
            _tcscat(filename, _T("\\"));
      }
      _tcscat(filename, name);
      _tcscat(filename, _T(".url"));
      FILE *bmFile = _tfopen(filename, _T("r"));
      if (bmFile){
         fclose(bmFile);
      }
      else {
         break;
      }
   }

   WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), newNode->url.c_str(), filename);

   newNode->text = name;
   _tcscat(name, _T(".url"));
   newNode->path = name;

   return 0;
}
