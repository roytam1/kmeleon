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
// Plugin that turns the menu into a rebar toolbar thing
//

#include "stdafx.h"

#include "commctrl.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

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
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

pluginFunctions pFuncs = {
   Init,
   Create,
   Config,
   Quit,
   NULL,
   DoRebar
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   "Rebar Menu Plugin",
   &pFuncs
};

HMENU m_menu;

int Init(){
   m_menu = NULL;

   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, "This plugin brought to you by the letter R", "Rebar Menu plugin", 0);
}

void Quit(){
}

#define _Q(x) #x

#define SUBMENU_OFFSET 5000 // this is here to distinguish between submenus and menu items, which may have the same id

void DoRebar(HWND rebarWnd) {
   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   // Create the toolbar control to be added.
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "Menu",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,  // note, the id doesn't matter at all, because it will be overridden by kmeleon
      kPlugin.hDllInstance, NULL
      );

   // Register the band name and child hwnd
   kPlugin.kf->RegisterBand(hwndTB, "Menu Bar");

   if (!hwndTB){
      MessageBox(NULL, "Failed to create menu toolbar", NULL, 0);
      return;
   }

   m_menu = GetMenu(GetParent(rebarWnd));
   if (!m_menu){
      MessageBox(NULL, "Could not get menu", NULL, 0);
   }

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);

   int stringID;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(m_menu);
   for (i=0; i<count; i++){
     if (GetMenuState(m_menu, i, MF_BYPOSITION) & MF_POPUP){
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(m_menu, i, MF_BYPOSITION, &mInfo);

        stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);

        TBBUTTON button;
        button.iBitmap = 0;
        button.idCommand = (int)mInfo.hSubMenu + SUBMENU_OFFSET;
        button.fsState = TBSTATE_ENABLED;
        button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
        button.iString = stringID;

        SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
     }
     else{
        char temp[128];
        mInfo.fMask = MIIM_TYPE | MIIM_ID;
        mInfo.cch = 127;
        mInfo.dwTypeData = temp;
        GetMenuItemInfo(m_menu, i, MF_BYPOSITION, &mInfo);

        stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPCTSTR)mInfo.dwTypeData);

        TBBUTTON button;
        button.iBitmap = 0;
        button.idCommand = mInfo.wID;
        button.fsState = TBSTATE_ENABLED;
        button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
        button.iString = stringID;

        SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
     }
   }

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = "";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = 0;
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

   // then hide the menu
   SetMenu(GetParent(rebarWnd), NULL);
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
   if (message == WM_NOTIFY){
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
}
