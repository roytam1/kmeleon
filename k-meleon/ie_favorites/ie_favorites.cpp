/*
*  Copyright (C) 2000 Brian Harris
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
// ie_favorites.cpp : Plugin that supports ie-style boomarks
//

#include "stdafx.h"
#include "resource.h"
#include "wininet.h"    // for INTERNET_MAX_URL_LENGTH

#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#include "../rebar_menu/hot_tracking.h"

#define PLUGIN_NAME "IE Favorites Plugin"
#define TOOLBAND_LABEL "Links"

#define PREFERENCE_REBAR_ENABLED _T("kmeleon.plugins.favorites.rebar")

#define REG_USER_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define REG_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define REG_FAVORITES_KEY _T("Favorites")

#define MAX_FAVORITES 4096

#define ERROR_TOO_MANY_FAVORITES "Error! You have too many favorites!\n\n" \
                                 "Right now, only 4096 favorites are supported"

static CStringArray      gFavorites;         // this one contains the display filename (Really...vorite)
static CStringArray      gFavoritesFiles;    // this one contains the full filename (SomeFolder\Stuff\Really Long Favorite)
static CArray<UINT, int> gIcons;

static UINT gNumFavorites;

static int        gInternetShortcutIcon;
static int        gFolderIcon;
static HIMAGELIST gSystemImages;
static CSize      gSysImageSize;

static HMENU gFavoritesMenu;

static UINT nConfigCommand;
static UINT nAddCommand;
static UINT nEditCommand;
static UINT nFirstFavoriteCommand;
static BOOL bRebarEnabled;
static HINSTANCE ghInstance;

static TCHAR gFavoritesPath[MAX_PATH];
static int   gFavoritesPathLen;

static WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int BuildFavoritesMenu(char * strPath, HMENU mainMenu);

BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
int DoAccel(char *param);
void DoRebar(HWND rebarWnd);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

/* End K-Meleon Plugin Header */

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Init") == 0) {
         Init();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
      else return 0;

      return 1;
   }
   return 0;
}


int Init()
{
   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);

   nFirstFavoriteCommand = kPlugin.kFuncs->GetCommandIDs(MAX_FAVORITES);

   TCHAR           sz[MAX_PATH];
   HKEY            hKey;
   DWORD           dwSize;
   ITEMIDLIST *idl;

   long rslt = ERROR_SUCCESS;

   // first try the correct way
   if (SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &idl) == NOERROR) {
      IMalloc *malloc;
      SHGetPathFromIDList(idl, sz);
      SHGetMalloc(&malloc);
      malloc->Free(idl);
      malloc->Release();
   }  
   else {
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
               TRACE0("Favorites folder not found\n");
               rslt = -1;
            }
         }
      }
      else {
         TRACE0("Favorites folder not found\n");
         rslt = -1;
      }
   }
   if (rslt == ERROR_SUCCESS) {
      ExpandEnvironmentStrings(sz, gFavoritesPath, MAX_PATH);

      strcat(gFavoritesPath, "\\");
      gFavoritesPathLen = strlen(gFavoritesPath);
   }
   else {
      gFavoritesPath[0] = 0;
      gFavoritesPathLen = 0;
   }


   // Get the rebar status
   int pref = 0;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, &pref);

   return true;
}

void Create(HWND parent)
{
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND hWndParent)
{
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, NULL);
}

void Quit()
{
   DestroyMenu(gFavoritesMenu);
}

void DoMenu(HMENU menu, char *param)
{
   // there are no favorites
   if (!*gFavoritesPath)
      return;

   if (*param != 0)  {

      char *comma = strchr(param, ',');
      char *action = param;
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;

      int command = 0;
      if (stricmp(action, "Config") == 0){
         command = nConfigCommand;
      }
      if (stricmp(action, "Add") == 0){
         command = nAddCommand;
      }
      if (stricmp(action, "Edit") == 0){
         command = nEditCommand;
      }
      if (command) {
         AppendMenu(menu, MF_STRING, command, string);
      }
   }
   else {
      gInternetShortcutIcon = -1;
      
      gFavoritesMenu = menu;
      BuildFavoritesMenu("", gFavoritesMenu);
      
      SHFILEINFO sfi;
      gSystemImages = (HIMAGELIST)SHGetFileInfo( gFavoritesPath,
         0,
         &sfi, 
         sizeof(SHFILEINFO), 
         SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
      
      if (gSystemImages != NULL) {
         int cx, cy;
         
         ImageList_GetIconSize (gSystemImages, &cx, &cy);
         gSysImageSize = CSize (cx, cy);
         
         gFolderIcon = sfi.iIcon;
      }
   }
}

int DoAccel(char *param)
{
   if (stricmp(param, "Config") == 0){
      return nConfigCommand;
   }
   if (stricmp(param, "Add") == 0){
      return nAddCommand;
   }
   if (stricmp(param, "Edit") == 0){
      return nEditCommand;
   }
   return nAddCommand;
}


void DoRebar(HWND rebarWnd){

   if (!bRebarEnabled)
      return;

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   // Create the toolbar control to be added.
#if 1
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "Links:",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );
#else
   HWND hwndTB = CreateToolbarEx(rebarWnd, dwStyle,
      /*id*/ 200,
      /*nBitmaps*/ 0,
      /*hBMInst*/ kPlugin.hDllInstance,
      /*wBMID*/ 0,
      /*lpButtons*/ NULL,
      /*iNumButtons*/ 0,
      /*dxButton*/ 16,
      /*dyButton*/ 16,
      /*dxBitmap*/ 16,
      /*dyBitmap*/ 16,
      /*uStructSize*/ sizeof(TBBUTTON)
      );
#endif

   if (!hwndTB){
      MessageBox(NULL, "Failed to create ie toolbar", NULL, 0);
      return;
   }

   // Register the band name and child hwnd
   kPlugin.kFuncs->RegisterBand(hwndTB, "Favorites");

   SetWindowText(hwndTB, TOOLBAND_LABEL);

   //SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)gSystemImages);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;
   int index;
   
   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(MENUITEMINFO);
   int i;
   int count = GetMenuItemCount(gFavoritesMenu);
   HMENU hLinksMenu = NULL;
   for (i=0; i<count; i++){
      if (GetMenuState(gFavoritesMenu, i, MF_BYPOSITION) & MF_POPUP){
         char temp[128];
         mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
         mInfo.cch = 127;
         mInfo.dwTypeData = temp;
         GetMenuItemInfo(gFavoritesMenu, i, MF_BYPOSITION, &mInfo);
        
         if (mInfo.dwTypeData && stricmp(mInfo.dwTypeData, "Links") == 0){
            hLinksMenu = GetSubMenu(gFavoritesMenu, i);
            break;
         }
      }
   }
   if (hLinksMenu == NULL)
      hLinksMenu = gFavoritesMenu;
   count = GetMenuItemCount(hLinksMenu);
   for (i=0; i<count; i++){
      if (GetMenuState(hLinksMenu, i, MF_BYPOSITION) & MF_POPUP){
         char temp[128];
         mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
         mInfo.cch = 127;
         mInfo.dwTypeData = temp;
         GetMenuItemInfo(hLinksMenu, i, MF_BYPOSITION, &mInfo);

         stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);

         TBBUTTON button;
         button.iBitmap = gFolderIcon;
         button.idCommand = (int)mInfo.hSubMenu+SUBMENU_OFFSET;
         button.fsState = TBSTATE_ENABLED;
         button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
         //button.bReserved = NULL;
         button.iString = stringID;

         SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
      }
      else{
         char temp[128];
         mInfo.fMask = MIIM_TYPE | MIIM_ID;
         mInfo.cch = 127;
         mInfo.dwTypeData = temp;
         GetMenuItemInfo(hLinksMenu, i, MF_BYPOSITION, &mInfo);

         if (mInfo.wID >= nFirstFavoriteCommand && mInfo.wID < nFirstFavoriteCommand + MAX_FAVORITES){
            index = mInfo.wID - nFirstFavoriteCommand;
            stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);//m_astrFavorites[index]);

            TBBUTTON button;
            button.iBitmap = gIcons[index];
            button.idCommand = mInfo.wID;
            button.fsState = TBSTATE_ENABLED;
            button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
            //button.bReserved = NULL;
            button.iString = stringID;

            SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
         }
      }
   }

   // Get the width & height of the toolbar.
   SIZE size;
   SendMessage(hwndTB, TB_GETMAXSIZE, 0, (LPARAM)&size);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = "Links";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = size.cy;
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = size.cx + 16;
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
}

int BuildFavoritesMenu(char * strPath, HMENU mainMenu)
{
   const int nStart = gNumFavorites;
   int nPos;

   int pathLen = strlen(strPath);

   char * searchString = new char[gFavoritesPathLen + pathLen + 2];
   strcpy(searchString, gFavoritesPath);
   strcat(searchString, strPath);
   strcat(searchString, "*");

   char * urlFile;
   char * subPath;

   // now scan the directory, first for .URL files and then for subdirectories
   // that may also contain .URL files
   WIN32_FIND_DATA wfd;
   HANDLE h = FindFirstFile(searchString, &wfd);

   delete [] searchString;

   if(h == INVALID_HANDLE_VALUE) {
      return gNumFavorites;
   }

   int tooMany = false;

   do {
      if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         // ignore the current and parent directory entries
         if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
            continue;

         subPath = new char[pathLen + strlen(wfd.cFileName) + 2];
         strcpy(subPath, strPath);
         strcat(subPath, wfd.cFileName);
         strcat(subPath, "/");

         HMENU subMenu = CreatePopupMenu();

         // call this function recursively.
         if(gNumFavorites < MAX_FAVORITES && BuildFavoritesMenu(subPath, subMenu)) {
            // only insert a submenu if there are in fact .URL files in the subdirectory
            // condense the name and escape ampersands
            char *pszTemp = fixString(wfd.cFileName, 40);
            AppendMenu(mainMenu, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT)subMenu, pszTemp);
            delete pszTemp;
         } else{
            DestroyMenu(subMenu);
         }

         delete [] subPath;

      } else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file

         char *dot = strrchr(wfd.cFileName, '.');
         if(dot && stricmp(dot, ".url") == 0) {

            // scan through the array and perform an insertion sort
            // to make sure the menu ends up in alphabetic order
//            for(nPos = nStart; nPos < gNumFavorites; nPos++)	{
//               if(stricmp(wfd.cFileName, gFavorites[nPos]) < 0)
//                  break;
//            }
            nPos = gNumFavorites;   // no alphabetization for now - isn't the order in which they were inserted better anyway?

            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new char[pathLen + filenameLen + 1];
            strcpy(urlFile, strPath);
            strcat(urlFile, wfd.cFileName);

            gFavoritesFiles.InsertAt(nPos, urlFile);

            // format for display in the menu
            // chop off the .url
            *dot = 0;
            // condense the string and escape ampersands
            char *pszTemp = fixString(wfd.cFileName, 40);

            gFavorites.InsertAt(nPos, pszTemp);
            AppendMenu(mainMenu, MF_STRING | MF_ENABLED, nFirstFavoriteCommand + nPos, pszTemp);
            delete pszTemp;

            /*
            HACK HACK HACK
            We append the filename to gFavoritesPath, get the icon, then chop the file off again
            this is a really lame way of doing this...
            */
            strcpy(gFavoritesPath + gFavoritesPathLen, urlFile);
            // Retrieve icon
            SHFILEINFO sfi;
            if (SHGetFileInfo (gFavoritesPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SMALLICON | SHGFI_SYSICONINDEX) && sfi.iIcon >= 0) {
               gIcons.InsertAt(nPos, sfi.iIcon);

               if (gInternetShortcutIcon == -1) {
                  gInternetShortcutIcon = sfi.iIcon;
               }
            }else{
               gIcons.InsertAt(nPos, 0);
            }
            gFavoritesPath[gFavoritesPathLen] = 0;

            delete [] urlFile;

            gNumFavorites++;
            if (gNumFavorites >= MAX_FAVORITES) {
               tooMany = true;
               break;
            }
         }
      }
   } while(FindNextFile(h, &wfd));

   FindClose(h);

   if (tooMany) MessageBox(NULL, ERROR_TOO_MANY_FAVORITES, PLUGIN_NAME, 0);

   return gNumFavorites - nStart;
}

char *GetURL(int index){
   // this is static so that we can return it
   static char url[INTERNET_MAX_URL_LENGTH];

   char path[MAX_PATH];
   strcpy(path, gFavoritesPath);
   strcpy(path+gFavoritesPathLen, gFavoritesFiles.GetAt(index));

   // a .URL file is formatted just like an .INI file, so we can
   // use GetPrivateProfileString() to get the information we want
   GetPrivateProfileString(_T("InternetShortcut"), _T("URL"), _T(""), url, INTERNET_MAX_URL_LENGTH, path);

   return url;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;

   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);
      if (command == nConfigCommand){
         Config(hWnd);
         return true;
      }
      if (command == nAddCommand){
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);

         // remove any characters the filesystem won't like
         char *c;
         while ((c = strpbrk(dInfo->title, "\\/:*?\"<>|")) != NULL) *c = ' ';

         CString filename = dInfo->title;
         filename += _T(".url");
         gFavoritesFiles.Add(filename);
         filename = gFavoritesPath + filename;
         ::WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), dInfo->url, filename);

         int nPos = gFavorites.GetSize();
         
         char *pszTemp = fixString(dInfo->title, 40);
         AppendMenu(gFavoritesMenu, MF_STRING, nFirstFavoriteCommand+nPos, pszTemp);
         gFavorites.Add(pszTemp);
         gIcons.Add(0);
         delete pszTemp;

         DrawMenuBar(hWnd);

         return true;
      }
      if (command == nEditCommand){
         ShellExecute(hWnd, "explore", gFavoritesPath, NULL, gFavoritesPath, SW_SHOWNORMAL);
         return true;
      }
      if (command >= nFirstFavoriteCommand && command < (nFirstFavoriteCommand + MAX_FAVORITES)){
         kPlugin.kFuncs->NavigateTo(GetURL(command-nFirstFavoriteCommand), OPEN_NORMAL);
         return true;
      }
   }
   else if (message == WM_NOTIFY) {
      hdr = *((LPNMHDR)lParam);
      if (hdr.code == TBN_DROPDOWN) {
         tbhdr = *((LPNMTOOLBAR)lParam);
         if (IsMenu((HMENU)(tbhdr.iItem-SUBMENU_OFFSET))){
            char toolbarName[11];
            GetWindowText(tbhdr.hdr.hwndFrom, toolbarName, 10);
            if (strcmp(toolbarName, TOOLBAND_LABEL) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }

            // post a message to defer exceution of BeginHotTrack
            PostMessage(hWnd, WM_DEFERHOTTRACK, NULL, NULL);

            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   else if (message == WM_DEFERHOTTRACK) {
      BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
      return true;
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
		case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, bRebarEnabled, 0);
         break;
      case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDOK:
                     bRebarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.favorites.rebar"), &bRebarEnabled);
                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;
      case WM_CLOSE:
			EndDialog(hWnd, NULL);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}

// so it doesn't munge the function name
extern "C" {

   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }

   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
      int top = (dis->rcItem.bottom - dis->rcItem.top - 16) / 2;
      top += dis->rcItem.top;

      if (GetMenuState((HMENU)dis->hwndItem, dis->itemID, MF_BYCOMMAND) & MF_POPUP){
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(gSystemImages, gFolderIcon, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS );
         }else{
            ImageList_Draw(gSystemImages, gFolderIcon, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }
         return 18;
      }
      if (dis->itemID >= nFirstFavoriteCommand && dis->itemID < (nFirstFavoriteCommand + MAX_FAVORITES)){
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(gSystemImages, gIcons[dis->itemID - nFirstFavoriteCommand], dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS);
         }else{
            ImageList_Draw(gSystemImages, gIcons[dis->itemID - nFirstFavoriteCommand], dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }

         return 18;
      }
      return 0;
   }
}


/*

From: "Nathan Fredrickson" <8nrf@qlink.queensu.ca>
To: <kmeleon-dev@lists.sourceforge.net>
Subject: [Kmeleon-dev] favorites plugin
Date: Fri, 2 Nov 2001 11:17:40 -0500

Hello,

I like how favorites can be shared between IE and Kmeleon, but I miss the
ability to custom order my favorites in IE.  While I would ideally like to
re-order favorites in Kmeleon, a good first step would be just using the IE
ordering when displaying the favorites.

I have done some investigating of how IE stores the menu order of favorites.
The favorites directory and each subdirectory has a corresponding key in the
registry:

HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites

HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites\\subdir

These keys contain a binary value named "Order".  If the key does not exist
for a specified directory, alphabetical order should be used.  I have not
found the format of the binary data specified anywhere, so this is what I
have discovered so far:

Order = {
	4	0x08 0x00 0x00 0x00
	4	0x02 0x00 0x00 0x00
	4	(total size of data) - (first 8 bytes) - (last byte)
	4	0x01 0x00 0x00 0x00
	4	N (number of records)
	record 1
	record 2
	...
	record N
	1	terminating byte 0x00
}

record = {
	4	size of record in bytes
	4	position of item in menu
		range: [0, recordcount-1] or negative if no order is defined
	14	unknown ???
	X	null-terminated string of full filename: "Yahoo!.url"
	X	null-terminated string of 8.3 filename: "YAHOO!.URL__"
	6	unknown ???
}

The above is enough info to extract the positions and the filenames (I have
written bits of code to do that while experimenting).  If I have time, I may
try to add this to the favorites plugin.  Not sure what is the best approach
to correlate this data with the URL files read from the directory...

*/

/*

   Brian's Notes:

  Implementation:
   Build a list of menu items in order from the registry keys (ordered favorites)
   Build a list of menu items in alphabetical order from the directories (actual favorites)
  
   Loop through ordered favorites.
      If the ordered favorite is in actual favorites
         Remove it from actual favorites
         Add it to merged favorites
      If the ordered favorite is not found in actual favorites
         Remove it from ordered favorites
   Append all remaining actual favorites to merged favorites

  We should store the favorites lists as a linked tree list thing (like the bookmarks)

*/
