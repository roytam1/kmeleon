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
// ns_bookmarks.cpp : Plugin that supports netscape-style boomarks
//

#include "stdafx.h"
#include "resource.h"

#include "commctrl.h"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#define MAX_BOOKMARKS 1024

#define _T(blah) blah

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
HGLOBAL GetMenu();
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
      "Netscape Bookmark Plugin",
      &pFuncs
};

HIMAGELIST imagelist; // the one and only imagelist...

HMENU m_menuBookmarks;
HMENU m_toolbarMenu;

UINT nConfigCommand;
UINT nAddCommand;
UINT nEditCommand;
UINT nDropdownCommand;
UINT nFirstBookmarkCommand;

TCHAR szPath[MAX_PATH];

int Init(){
   nConfigCommand = kPlugin.kf->GetCommandIDs(1);
   nAddCommand = kPlugin.kf->GetCommandIDs(1);
   nEditCommand = kPlugin.kf->GetCommandIDs(1);
   nDropdownCommand = kPlugin.kf->GetCommandIDs(1);

   nFirstBookmarkCommand = kPlugin.kf->GetCommandIDs(MAX_BOOKMARKS);

   kPlugin.kf->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), szPath, "");
   strcat(szPath, "bookmarks.html");

   imagelist = ImageList_Create(16, 15, ILC_MASK, 4, 4);

   HBITMAP bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));

   ImageList_AddMasked(imagelist, bitmap, RGB(192, 192, 192));

   DeleteObject(bitmap);

   return true;
}

typedef std::map<HWND, void *> WndProcMap;
WndProcMap KMeleonWndProcs;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProcs[parent] = (void *) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, "This plugin brought to you by the letter N", "Netscape Bookmark plugin", 0);
}

void Quit(){
   //  the menu should be destroyed by kmeleon...
   ImageList_Destroy(imagelist);
}

#define _Q(x) #x

std::vector<std::string> urlVector;
void ParseBookmarks(char *bmFileBuffer, HMENU menu){
   char *p;
   char *t;
   while ((p = strtok(NULL, "\n")) != NULL){
      if ((t = strstr(p, "<DT><H3 ")) != NULL){
         t+=8;
         char *start = strchr(t, '>') + 1;
         char *end = strrchr(t, '<');
         *end = 0;
         HMENU subMenu = CreatePopupMenu();
         ParseBookmarks(end, subMenu);
         AppendMenu(menu, MF_STRING | MF_POPUP, (UINT)subMenu, start);

         if (strstr(t, _Q(PERSONAL_TOOLBAR_FOLDER="true"))){
            m_toolbarMenu = subMenu;
         }
      }else if ((t = strstr(p, "<DT><A HREF=\"")) != NULL){
         t+=13; // t now points to the url
         char *q = strchr(t, '\"');
         if (q) *q = 0;
         urlVector.push_back((std::string)t);
         int position = urlVector.size() - 1;
         t = strchr(q+1, '>') + 1;
         q = strchr(t, '<');
         *q = 0;

         if (strlen(t) > 40)
            CondenseString(t, 40);

         AppendMenu(menu, MF_STRING, nFirstBookmarkCommand+position, t);
      }else if ((t = strstr(p, "</DL>")) != NULL){
         return;
      }else if ((t = strstr(p, "<hr>")) != NULL){
         AppendMenu(menu, MF_SEPARATOR, 0, NULL);
      }
   }
   return;
}

void DoMenu(HMENU menu, char *param){
   if (stricmp(param, _T("Config")) == 0){
      AppendMenu(menu, MF_STRING, nConfigCommand, "&Config");
      return;
   }
   if (stricmp(param, _T("Add")) == 0){
      AppendMenu(menu, MF_STRING, nAddCommand, "&Add Bookmark");
      return;
   }
   if (stricmp(param, _T("Edit")) == 0){
      AppendMenu(menu, MF_STRING, nEditCommand, "&Edit Bookmarks");
      return;
   }
   if (*param == 0){
      urlVector.reserve(MAX_BOOKMARKS);

      FILE *bmFile = fopen(szPath, "r");
      if (bmFile){
         fseek(bmFile, 0, SEEK_END);

         long bmFileSize = ftell(bmFile);
         fseek(bmFile, 0, SEEK_SET);

         char *bmFileBuffer = new char[bmFileSize];
         if (bmFileBuffer){
            fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);

            strtok(bmFileBuffer, "\n");
            ParseBookmarks(bmFileBuffer, menu);
            m_menuBookmarks = menu;

            delete [] bmFileBuffer;
         }
         fclose(bmFile);
      }
   }
}

#define SUBMENU_OFFSET 5000 // this is here to distinguish between submenus and menu items, which may have the same id

void DoRebar(HWND rebarWnd){
   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   // Create the toolbar control to be added.
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "Links:",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );

   if (!hwndTB){
      MessageBox(NULL, "Failed to create bookmark toolbar", NULL, 0);
      return;
   }

   if (!m_toolbarMenu){
      m_toolbarMenu = m_menuBookmarks;
   }

   SetWindowText(hwndTB, "Netscape Bookbar");

   //SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)imagelist);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;
   int index;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(m_toolbarMenu);
   for (i=0; i<count; i++){
     if (GetMenuState(m_toolbarMenu, i, MF_BYPOSITION) & MF_POPUP){
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(m_toolbarMenu, i, MF_BYPOSITION, &mInfo);

        stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);

        TBBUTTON button;
        button.iBitmap = 0; //m_iFolderIcon;
        button.idCommand = (int)mInfo.hSubMenu + SUBMENU_OFFSET;
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
        GetMenuItemInfo(m_toolbarMenu, i, MF_BYPOSITION, &mInfo);

        if (mInfo.wID >= nFirstBookmarkCommand && mInfo.wID < nFirstBookmarkCommand + MAX_BOOKMARKS){
           index = mInfo.wID - nFirstBookmarkCommand;

           stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);//m_astrFavorites[index]);

           TBBUTTON button;
           button.iBitmap = 1; //m_URLIcons[index];
           button.idCommand = mInfo.wID;
           button.fsState = TBSTATE_ENABLED;
           button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
           //button.bReserved = NULL;
           button.iString = stringID;

           SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
        }
     }
   }

   TBBUTTON button;
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_SEP;
   //button.bReserved = NULL;
   SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   button.iBitmap = 2;
   button.idCommand = nDropdownCommand;
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
   //button.bReserved = NULL;
   button.iString = -1;
   SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = "Personal Toolbar Folder";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = 0;
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
}

CALLBACK EditProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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
   WndProcMap::iterator WndProcIterator;
   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);
      if (command == nConfigCommand){
         Config(NULL);
         return true;
      }
      if (command == nAddCommand){
         kmeleonDocInfo *dInfo = kPlugin.kf->GetDocInfo(hWnd);
         urlVector.push_back((std::string)dInfo->url);
         AppendMenu(m_menuBookmarks, MF_STRING, nFirstBookmarkCommand+urlVector.size()-1, dInfo->title);
         DrawMenuBar(hWnd);
         return true;
      }
      if (command == nEditCommand){
         DialogBoxParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), hWnd, EditProc, 0);
         return true;
      }
      if (command >= nFirstBookmarkCommand && command < (nFirstBookmarkCommand + MAX_BOOKMARKS)){
         kPlugin.kf->NavigateTo((char *)urlVector[command-nFirstBookmarkCommand].c_str(), false);
         return true;
      }
   }
   else if (message == WM_NCDESTROY){
      WndProcIterator = KMeleonWndProcs.find(hWnd); 

      if (WndProcIterator != KMeleonWndProcs.end()){
         SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WndProcIterator->second);

         LRESULT result = CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);

         KMeleonWndProcs.erase(WndProcIterator);

         return result;
      }
   }
   else if (message == WM_NOTIFY){
     NMHDR *hdr = (LPNMHDR)lParam;
     if (hdr->code == TBN_DROPDOWN){
       NMTOOLBAR *tbhdr = (LPNMTOOLBAR)lParam;
       if (tbhdr->iItem == nDropdownCommand){
         RECT rc;
         WPARAM index = 0;
         SendMessage(tbhdr->hdr.hwndFrom, TB_GETITEMRECT, index, (LPARAM) &rc);
         POINT pt = { rc.left, rc.bottom };
         ClientToScreen(tbhdr->hdr.hwndFrom, &pt);
         TrackPopupMenu((HMENU)m_menuBookmarks, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
       }
       else if (IsMenu((HMENU)(tbhdr->iItem-SUBMENU_OFFSET))){
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
   WndProcIterator = KMeleonWndProcs.find(hWnd);

   if (WndProcIterator != KMeleonWndProcs.end()){
      return CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);
   }
   return 0;
}

void FillTree(HWND hTree, HTREEITEM parent, HMENU menu){
   TVINSERTSTRUCT tvis;
   tvis.hParent = parent;
   tvis.hInsertAfter = NULL;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(menu);
   for (i=0; i<count; i++){
     if (GetMenuState(menu, i, MF_BYPOSITION) & MF_POPUP){
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(menu, i, MF_BYPOSITION, &mInfo);

        tvis.itemex.pszText = mInfo.dwTypeData;
        tvis.itemex.lParam = 0;
        HTREEITEM thisItem = TreeView_InsertItem(hTree, &tvis);
        FillTree(hTree, thisItem, (HMENU)mInfo.hSubMenu);
     }
     else{
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_ID;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(menu, i, MF_BYPOSITION, &mInfo);

        if (mInfo.wID >= nFirstBookmarkCommand && mInfo.wID < nFirstBookmarkCommand + MAX_BOOKMARKS){
           tvis.itemex.pszText = mInfo.dwTypeData;
           tvis.itemex.lParam = mInfo.wID;
           TreeView_InsertItem(hTree, &tvis);
        }
     }
   }
}

CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
   switch (uMsg){
   case WM_INITDIALOG:
      {
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         FillTree(hTree, NULL, m_menuBookmarks);
      }
      return false;
   case WM_COMMAND:
      {
         WORD id = LOWORD(wParam);

         switch(id){
         case IDOK:
         case IDCANCEL:
            EndDialog(hDlg, 1);
            break;
         }
      }
   }
   return false;
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
            ImageList_Draw(imagelist, 0, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS );
         }else{
            ImageList_Draw(imagelist, 0, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }
         return 18;
      }
      if (dis->itemID >= nFirstBookmarkCommand && dis->itemID < (nFirstBookmarkCommand + MAX_BOOKMARKS)){
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(imagelist, 1, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS);
         }else{
            ImageList_Draw(imagelist, 1, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }

         return 18;
      }
      return 0;
   }

}
