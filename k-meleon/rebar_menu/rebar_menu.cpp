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

#include "hot_tracking.h"

#define _T(blah) blah
#define _Q(x) #x

#define PLUGIN_NAME "Rebar Menu Plugin"
#define MENU_NAME "Menu"
#define NO_OPTIONS "This plugin has no user configurable options."
#define ERROR_NO_MENU "Error! Could not get the menu"
#define ERROR_FAILED_TO_CREATE "Error! Failed to create menu toolbar"

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoRebar(HWND rebarWnd);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

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
      else return 0;

      return 1;
   }
   return 0;
}

HMENU m_menu;

int Init(){
   m_menu = NULL;

   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, NO_OPTIONS, PLUGIN_NAME, 0);
}

void Quit(){
}

void DoRebar(HWND rebarWnd) {
   m_menu = GetMenu(GetParent(rebarWnd));
   if (!m_menu){
      MessageBox(NULL, ERROR_NO_MENU, PLUGIN_NAME, 0);
      return;
   }

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   // Create the toolbar control to be added.
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, MENU_NAME,
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,  // note, the id doesn't matter at all, because it will be overridden by kmeleon
      kPlugin.hDllInstance, NULL
      );

   if (!hwndTB){
      MessageBox(NULL, ERROR_FAILED_TO_CREATE, PLUGIN_NAME, 0);
      return;
   }

   SetWindowText(hwndTB, MENU_NAME);

   // Register the band name and child hwnd
   kPlugin.kFuncs->RegisterBand(hwndTB, MENU_NAME, false);

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);

   int stringID;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(m_menu);
   for (i=0; i<count; i++) {
      if ( GetMenuState(m_menu, i, MF_BYPOSITION) & MF_POPUP) {
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   if (message == WM_NOTIFY) {
      NMHDR *hdr = (LPNMHDR)lParam;
      if (hdr->code == TBN_DROPDOWN) {
         NMTOOLBAR *tbhdr = (LPNMTOOLBAR)lParam;
         if (IsMenu((HMENU)(tbhdr->iItem-SUBMENU_OFFSET))){
            char toolbarName[11];
            GetWindowText(tbhdr->hdr.hwndFrom, toolbarName, 10);
            if (strcmp(toolbarName, MENU_NAME) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }

            BeginHotTrack(tbhdr, kPlugin.hDllInstance, hWnd);

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
