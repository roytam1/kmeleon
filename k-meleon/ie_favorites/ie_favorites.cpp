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

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define MAX_FAVORITES 512

/*
// MFC handles this for us (how nice)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  switch (ul_reason_for_call)
  {
		case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
      }

  return TRUE;
  }
*/

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

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

CMenu m_menuFavorites;

UINT nConfigCommand;
UINT nAddCommand;
UINT nEditCommand;
UINT nFirstFavoriteCommand;

TCHAR szPath[MAX_PATH];

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
      //return;
      strcpy(sz, _T("c:\\windows\\favorites"));
   }
   dwSize = sizeof(sz);
   RegQueryValueEx(hKey, _T("Favorites"), NULL, NULL, (LPBYTE)sz, &dwSize);
   ExpandEnvironmentStrings(sz, szPath, MAX_PATH);
   RegCloseKey(hKey);

   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, "This plugin brought to you by the letter M", "IE Favorites plugin", 0);
}

void Quit(){
   m_menuFavorites.DestroyMenu();
}

static CStringArray    m_astrFavorites;
static CStringArray m_astrFavoriteURLs;
static CArray<UINT, int> m_URLIcons;

static int        m_iInternetShortcutIcon;
static HIMAGELIST m_himSystem;
static CSize      m_SysImageSize;
static int        m_iFolderIcon;

int BuildFavoritesMenu(LPCTSTR pszPath, int nStartPos, CMenu* pMenu)
{
   CString         strPath(pszPath);
   CString         strPath2;
   CString         str;
   WIN32_FIND_DATA wfd;
   HANDLE          h;
   int             nPos;
   int             nEndPos;
   int             nNewEndPos;
   int             nLastDir;
   TCHAR           buf[MAX_PATH];
   CStringArray    astrDirs;
   CMenu*          pSubMenu;

   // make sure there's a trailing backslash
   if(strPath[strPath.GetLength() - 1] != _T('\\'))
      strPath += _T('\\');
   strPath2 = strPath;
   strPath += _T("*.*");

   // now scan the directory, first for .URL files and then for subdirectories
   // that may also contain .URL files
   h = FindFirstFile(strPath, &wfd);
   if(h != INVALID_HANDLE_VALUE)
   {
      nEndPos = nStartPos;
      do
      {
         if((wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0)
         {
            str = wfd.cFileName;
            if(str.Right(4).CompareNoCase(_T(".url")) == 0)
            {
               // an .URL file is formatted just like an .INI file, so we can
               // use GetPrivateProfileString() to get the information we want
               GetPrivateProfileString(_T("InternetShortcut"), _T("URL"),
                  _T(""), buf, MAX_PATH,
                  strPath2 + str);
               str = str.Left(str.GetLength() - 4);

               // scan through the array and perform an insertion sort
               // to make sure the menu ends up in alphabetic order

               for(nPos = nStartPos ; nPos < nEndPos ; ++nPos)	{
                  if(str.CompareNoCase(m_astrFavorites[nPos]) < 0)
                     break;
               }

               //nPos = m_astrFavoriteURLs.GetSize();

               m_astrFavorites.InsertAt(nPos, str);
               m_astrFavoriteURLs.InsertAt(nPos, buf);

               // Retrieve icon
               SHFILEINFO sfi;
               if (SHGetFileInfo (strPath2 + wfd.cFileName, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SMALLICON | SHGFI_SYSICONINDEX) &&
                  sfi.iIcon >= 0)
               {
                  m_URLIcons.InsertAt(nPos, sfi.iIcon);

                  if (m_iInternetShortcutIcon == -1) {
                     m_iInternetShortcutIcon = sfi.iIcon;
                  }
               }

               ++nEndPos;
            }
         }
      } while(FindNextFile(h, &wfd));
      FindClose(h);
      // Now add these items to the menu
      for(nPos = nStartPos ; nPos < nEndPos ; ++nPos)	{
         pMenu->AppendMenu(MF_STRING | MF_ENABLED, nFirstFavoriteCommand + nPos, m_astrFavorites[nPos]);
      }

      // now that we've got all the .URL files, check the subdirectories for more
      nLastDir = 0;
      h = FindFirstFile(strPath, &wfd);
      ASSERT(h != INVALID_HANDLE_VALUE);
      do
      {
         if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         {
            // ignore the current and parent directory entries
            if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
               continue;

            for(nPos = 0 ; nPos < nLastDir ; ++nPos)
            {
               if(astrDirs[nPos].CompareNoCase(wfd.cFileName) > 0)
                  break;
            }
            pSubMenu = new CMenu;
            pSubMenu->CreatePopupMenu();

            // call this function recursively.
            nNewEndPos = BuildFavoritesMenu(strPath2 + wfd.cFileName, nEndPos, pSubMenu);
            if(nNewEndPos != nEndPos)	{
               // only intert a submenu if there are in fact .URL files in the subdirectory
               nEndPos = nNewEndPos;
               //pMenu->AppendMenu(MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)pSubMenu->m_hMenu, wfd.cFileName);
               pMenu->InsertMenu(nFirstFavoriteCommand + nStartPos, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT)pSubMenu->m_hMenu, wfd.cFileName);
               pSubMenu->Detach();
               astrDirs.InsertAt(nPos, wfd.cFileName);
               ++nLastDir;
            }
            delete pSubMenu;
         }
      } while(FindNextFile(h, &wfd));
      FindClose(h);
   }
   return nEndPos;
}


void DoMenu(HMENU menu, char *param){

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
   m_iInternetShortcutIcon = -1;

   m_menuFavorites.Attach(menu);

   BuildFavoritesMenu(szPath, 0, &m_menuFavorites);

   SHFILEINFO sfi;
   m_himSystem = (HIMAGELIST)SHGetFileInfo( szPath,
      0,
      &sfi, 
      sizeof(SHFILEINFO), 
      SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

   if (m_himSystem != NULL) {
      int cx, cy;

      ::ImageList_GetIconSize (m_himSystem, &cx, &cy);
      m_SysImageSize = CSize (cx, cy);

      m_iFolderIcon = sfi.iIcon;
   }
}

#define SUBMENU_OFFSET 5000 // this is here to distinguish between submenus and menu items, which may have the same id

void DoRebar(HWND rebarWnd){
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

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)m_himSystem);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;
   int index;
   
   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(m_menuFavorites);
   HMENU hLinksMenu = NULL;
   for (i=0; i<count; i++){
     if (GetMenuState(m_menuFavorites, i, MF_BYPOSITION) & MF_POPUP){
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(m_menuFavorites, i, MF_BYPOSITION, &mInfo);
        
        if (stricmp(mInfo.dwTypeData, "Links") == 0){
           hLinksMenu = GetSubMenu(m_menuFavorites, i);
           break;
        }
     }
   }
   if (hLinksMenu == NULL)
      hLinksMenu = m_menuFavorites;
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
        button.iBitmap = m_iFolderIcon;
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
           button.iBitmap = m_URLIcons[index];
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);
      if (command == nConfigCommand){
         Config(NULL);
         return true;
      }
      if (command == nAddCommand){
         kmeleonDocInfo *dInfo = kPlugin.kf->GetDocInfo(hWnd);

         CString filename = szPath;
         filename += _T('\\');
         filename += dInfo->title;
         filename += _T(".url");
         ::WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), dInfo->url, filename);

         UINT nPos = m_astrFavoriteURLs.GetSize();
         m_astrFavoriteURLs.InsertAt(nPos, dInfo->url);

         m_menuFavorites.AppendMenu(MF_STRING, nFirstFavoriteCommand+nPos, dInfo->title);

         DrawMenuBar(hWnd);

         return true;
      }
      if (command == nEditCommand){
         ShellExecute(hWnd, "explore", szPath, NULL, szPath, SW_SHOWNORMAL);
         return true;
      }
      if (command >= nFirstFavoriteCommand && command < (nFirstFavoriteCommand + MAX_FAVORITES)){
         kPlugin.kf->NavigateTo((char *)(LPCTSTR)m_astrFavoriteURLs.GetAt(command - nFirstFavoriteCommand), OPEN_NORMAL);
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
            ImageList_Draw(m_himSystem, m_iFolderIcon, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS );
         }else{
            ImageList_Draw(m_himSystem, m_iFolderIcon, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }
         return 18;
      }
      if (dis->itemID >= nFirstFavoriteCommand && dis->itemID < (nFirstFavoriteCommand + MAX_FAVORITES)){
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(m_himSystem, m_URLIcons[dis->itemID - nFirstFavoriteCommand], dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS);
         }else{
            ImageList_Draw(m_himSystem, m_URLIcons[dis->itemID - nFirstFavoriteCommand], dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }

         return 18;
      }
      return 0;
   }

}
