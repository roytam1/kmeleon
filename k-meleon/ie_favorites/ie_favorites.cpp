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

#include <vector>

using namespace std;

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define MAX_FAVORITES 512

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

pluginFunctions pFuncs = {
   Init,
   Create,
   Config,
   Quit,
   DoMenu,
   DoRebar
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   "IE Favorites Plugin",
   &pFuncs
};

UINT nConfigCommand;
UINT nAddCommand;
UINT nEditCommand;
UINT nFirstFavoriteCommand;
BOOL bRebarEnabled;
HINSTANCE ghInstance;

TCHAR gFavoritesPath[MAX_PATH];
int   gFavoritesPathLen;

int Init(){
   nConfigCommand = kPlugin.kf->GetCommandIDs(1);
   nAddCommand = kPlugin.kf->GetCommandIDs(1);
   nEditCommand = kPlugin.kf->GetCommandIDs(1);

   nFirstFavoriteCommand = kPlugin.kf->GetCommandIDs(MAX_FAVORITES);

   TCHAR           sz[MAX_PATH];
   HKEY            hKey;
   DWORD           dwSize;

   // find out from the registry where the favorites are located.
   if(RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"), &hKey) != ERROR_SUCCESS)
   {
      TRACE0("Favorites folder not found\n");
   }
   else {
      dwSize = sizeof(sz);
      RegQueryValueEx(hKey, _T("Favorites"), NULL, NULL, (LPBYTE)sz, &dwSize);
      ExpandEnvironmentStrings(sz, gFavoritesPath, MAX_PATH);
      RegCloseKey(hKey);

      strcat(gFavoritesPath, "\\");
      gFavoritesPathLen = strlen(gFavoritesPath);
   }

   // Get the rebar status
   int pref = 0;
   kPlugin.kf->GetPreference(PREF_BOOL, _T("kmeleon.plugins.favorites.rebar"), &bRebarEnabled, &pref);

   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND hWndParent) {
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, NULL);
}

static HMENU gFavoritesMenu;

void Quit(){
   DestroyMenu(gFavoritesMenu);
}

static CStringArray      gFavorites;
static CStringArray      gFavoritesFiles;
static CArray<UINT, int> gIcons;

static UINT gNumFavorites;

static int        gInternetShortcutIcon;
static int        gFolderIcon;
static HIMAGELIST gSystemImages;
static CSize      gSysImageSize;

int BuildFavoritesMenu(char * strPath, HMENU mainMenu){
   const int nStart = gNumFavorites;
   int nPos;

   vector<char *> dirsArray;

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

   do {
      if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         // ignore the current and parent directory entries
         if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
            continue;

         subPath = new char[pathLen + strlen(wfd.cFileName) + 2];
         strcpy(subPath, strPath);
         strcat(subPath, wfd.cFileName);
         strcat(subPath, "/");

         dirsArray.push_back(subPath);

      }else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file

         char *dot = strrchr(wfd.cFileName, '.');
         if(dot && stricmp(dot, ".url") == 0) {

            // scan through the array and perform an insertion sort
            // to make sure the menu ends up in alphabetic order
            for(nPos = nStart; nPos < gNumFavorites; nPos++)	{
               if(stricmp(wfd.cFileName, gFavorites[nPos]) < 0)
                  break;
            }

            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new char[pathLen + filenameLen + 1];
            strcpy(urlFile, strPath);
            strcat(urlFile, wfd.cFileName);

            gFavoritesFiles.InsertAt(nPos, urlFile);

            *dot = 0;
            gFavorites.InsertAt(nPos, wfd.cFileName);

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
         }
      }
   } while(FindNextFile(h, &wfd));
   FindClose(h);

   // Now add these items to the menu
   for (nPos = nStart; nPos < gNumFavorites; nPos++) {
      AppendMenu(mainMenu, MF_STRING | MF_ENABLED, nFirstFavoriteCommand + nPos, gFavorites[nPos]);
   }

   // then do the directories
   HMENU subMenu;

   int nSize = dirsArray.size();
   for (nPos = 0; nPos < nSize; nPos++){
      subMenu = CreatePopupMenu();

      subPath = dirsArray[nPos];

      // call this function recursively.
      if(BuildFavoritesMenu(subPath, subMenu))	{
         // only insert a submenu if there are in fact .URL files in the subdirectory
         subPath[strlen(subPath)-1] = 0; // chop off the trailing slash
         InsertMenu(mainMenu, nFirstFavoriteCommand, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT)subMenu, subPath);
      }else{
         DestroyMenu(subMenu);
      }

      delete [] subPath;
   }

   return gNumFavorites - nStart;
}


void DoMenu(HMENU menu, char *param){

   // there are no favorites
   if (!*gFavoritesPath)
      return;

   if (stricmp(param, _T("Config")) == 0){
      AppendMenu(menu, MF_STRING, nConfigCommand, "&Config");
      return;
   }
   if (stricmp(param, _T("Add")) == 0){
      AppendMenu(menu, MF_STRING, nAddCommand, "&Add Favorite");
      return;
   }
   if (stricmp(param, _T("Edit")) == 0){
      AppendMenu(menu, MF_STRING, nEditCommand, "&Edit Favorites");
      return;
   }
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

#define SUBMENU_OFFSET 5000 // this is here to distinguish between submenus and menu items, which may have the same id

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
    kPlugin.kf->RegisterBand(hwndTB, "Favorites");

    SetWindowText(hwndTB, "Links");

   //SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)gSystemImages);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;
   int index;
   
   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
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
        
        if (stricmp(mInfo.dwTypeData, "Links") == 0){
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

BOOL gbContinueMenu;
int giCurrentItem; 
HWND ghToolbarWnd;
HHOOK ghhookMsg;
LRESULT CALLBACK MsgHook(int code, WPARAM wParam, LPARAM lParam){
   if (code == MSGF_MENU){
      MSG *msg = (MSG *)lParam;
      if (msg->message == WM_MOUSEMOVE){
         POINT mouse;
         mouse.x = LOWORD(msg->lParam);
         mouse.y = HIWORD(msg->lParam);

         if (ghToolbarWnd){
            ScreenToClient(ghToolbarWnd, &mouse);
            int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

            if (ndx >= 0){
               TBBUTTON button;
               SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button);
               if (giCurrentItem != button.idCommand && IsMenu((HMENU)(button.idCommand-SUBMENU_OFFSET))){
                  SendMessage(msg->hwnd, WM_CANCELMODE, 0, 0);

                  // this basically tells the loop, "we would like to enter a new menu loop with this item:"
                  giCurrentItem = button.idCommand;
                  gbContinueMenu = true;

                  return true;
               }
            }
         }
      }
   }
   return CallNextHookEx(ghhookMsg, code, wParam, lParam);
}

char *GetURL(int index){
   static char url[MAX_PATH];
   static char path[MAX_PATH] = {0};
   if (!path[0]){
      strcpy(path, gFavoritesPath);
   }
   strcpy(path+gFavoritesPathLen, gFavoritesFiles.GetAt(index));
   // a .URL file is formatted just like an .INI file, so we can
   // use GetPrivateProfileString() to get the information we want
   GetPrivateProfileString(_T("InternetShortcut"), _T("URL"),
      _T(""), url, MAX_PATH,
      path);
   return url;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);
      if (command == nConfigCommand){
         Config(NULL);
         return true;
      }
      if (command == nAddCommand){
         kmeleonDocInfo *dInfo = kPlugin.kf->GetDocInfo(hWnd);

         CString filename = gFavoritesPath;
         filename += _T('\\');
         filename += dInfo->title;
         filename += _T(".url");
         ::WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), dInfo->url, filename);

         int nPos = gFavorites.GetSize();
         AppendMenu(gFavoritesMenu, MF_STRING, nFirstFavoriteCommand+nPos, dInfo->title);
         gFavorites.Add(dInfo->title);
         gFavoritesFiles.Add(filename);

         DrawMenuBar(hWnd);

         return true;
      }
      if (command == nEditCommand){
         ShellExecute(hWnd, "explore", gFavoritesPath, NULL, gFavoritesPath, SW_SHOWNORMAL);
         return true;
      }
      if (command >= nFirstFavoriteCommand && command < (nFirstFavoriteCommand + MAX_FAVORITES)){
         kPlugin.kf->NavigateTo(GetURL(command-nFirstFavoriteCommand), OPEN_NORMAL);
         return true;
      }
   }
   else if (message == WM_NOTIFY){
     NMHDR *hdr = (LPNMHDR)lParam;
     if (hdr->code == TBN_DROPDOWN){
       NMTOOLBAR *tbhdr = (LPNMTOOLBAR)lParam;
       if (IsMenu((HMENU)(tbhdr->iItem-SUBMENU_OFFSET))){
         ghToolbarWnd = tbhdr->hdr.hwndFrom;
         giCurrentItem = tbhdr->iItem;

         int lastItem;

         do {
            gbContinueMenu = false;

            SendMessage(ghToolbarWnd, TB_PRESSBUTTON, giCurrentItem, MAKELONG(true, 0));
            ghhookMsg = SetWindowsHookEx(WH_MSGFILTER, MsgHook, kPlugin.hDllInstance, GetCurrentThreadId());

            RECT rc;
            WPARAM index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
            SendMessage(ghToolbarWnd, TB_GETITEMRECT, index, (LPARAM) &rc);
            POINT pt = { rc.left, rc.bottom };
            ClientToScreen(ghToolbarWnd, &pt);

            // the hook may change this, so we need to save it for the TB_PRESSBUTTON
            lastItem = giCurrentItem; 

            TrackPopupMenu((HMENU)(giCurrentItem-SUBMENU_OFFSET), TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

            UnhookWindowsHookEx(ghhookMsg);
            SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(false, 0));
         } while (gbContinueMenu);

         return DefWindowProc(hWnd, message, wParam, lParam);
       }
     }
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
                     kPlugin.kf->SetPreference(PREF_BOOL, _T("kmeleon.plugins.favorites.rebar"), &bRebarEnabled);
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
