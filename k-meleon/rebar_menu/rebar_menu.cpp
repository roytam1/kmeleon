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
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#define WM_DEFERHOTTRACK WM_USER+10 
#include "hot_tracking.h"
#include "../strconv.h"

#define _Q(x) #x

#define _Tr(x) kPlugin.kFuncs->Translate(x)

#define PLUGIN_NAME "Rebar Menu Plugin"
#define MENU_NAME "Menu"
#define NO_OPTIONS _Tr("This plugin has no user configurable options.")
#define ERROR_NO_MENU _Tr("Error! Could not get the menu")
#define ERROR_FAILED_TO_CREATE _Tr("Error! Failed to create menu toolbar")

#define PREFERENCE_MENUTYPE "kmeleon.plugins.rebarmenu.menutype"

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int Load();
void Setup();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoLocale();
void DoRebar(HWND rebarWnd);
int DoAccel(char *param, BOOL bSubmenu=FALSE);

#define MAX_KEYBOARD_MENUS 32
int id_accel=0;
HMENU accels[MAX_KEYBOARD_MENUS];
int accels_used=0;
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
int  nMenuType = 2;

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (strcmp(subject, "Load") == 0) {
         Load();
      }
      if (strcmp(subject, "Setup") == 0) {
         Setup();
      }
      else if (strcmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (strcmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (strcmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (strcmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
	  else if (strcmp(subject, "DoLocale") == 0) {
         DoLocale();
      }

      else return 0;

      return 1;
   }
   return 0;
}

HMENU m_menu;

int Load(){
   m_menu = NULL;
   id_accel = kPlugin.kFuncs->GetCommandIDs(MAX_KEYBOARD_MENUS);

   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_MENUTYPE, &nMenuType, &nMenuType);

   return true;
}

void Setup(){
   HMENU hMenu = kPlugin.kFuncs->GetMenu("Main");

   if (!hMenu || nMenuType==1)
     return;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(hMenu);
   for (i=0; i<count; i++) {
      if ( GetMenuState(hMenu, i, MF_BYPOSITION) & MF_POPUP) {
         TCHAR temp[128] = {0};
         mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
         mInfo.cch = 127;
         mInfo.dwTypeData = temp;
         GetMenuItemInfo(hMenu, i, MF_BYPOSITION, &mInfo);
		 if (mInfo.fType != MFT_STRING) continue;
#ifdef _UNICODE
         char q[256];
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, mInfo.dwTypeData, -1, q, 256, "_", NULL);
#else
         char *q = mInfo.dwTypeData;
#endif
         char *p = strchr(q, '&');
         if (p && *(p+1)) {
	       char szBuf[16] = {0};
	       char szId[6] = {0};
	       strcpy(szBuf, "ALT ? = ");
	       szBuf[4] = toupper(*(p+1));
	       int id = DoAccel(q, TRUE);
	       itoa(id, szId, 10);
	       strcat(szBuf, szId);
	       kPlugin.kFuncs->ParseAccel(szBuf);
	     }
      }
   }
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
//   MessageBox(parent, NO_OPTIONS, _T(PLUGIN_NAME), 0);
}

void Quit(){
}
 
int DoAccel(char *param, BOOL bSubmenu) {
   HMENU hMenu = kPlugin.kFuncs->GetMenu("Main");

   if (!hMenu)
     return 0;

   MENUITEMINFO mInfo;
   mInfo.cbSize = sizeof(mInfo);
   int i;
   int count = GetMenuItemCount(hMenu);
   for (i=0; i<count; i++) {
     TCHAR temp[128];
     mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
     mInfo.cch = 127;
     mInfo.dwTypeData = temp;
     GetMenuItemInfo(hMenu, i, MF_BYPOSITION, &mInfo);
#ifdef _UNICODE
     char q[128];
     WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, temp, -1, q, 128, "_", NULL);
#else
	 char *q = temp;
#endif
	 if (stricmp(param, q) == 0)
       break;
   }

   int id = atoi(param);
   if (i<count) {
     id = i;
   }
   else {
     HMENU hMenu = kPlugin.kFuncs->GetMenu(param);
     if (hMenu) {
       accels_used++;
       id = MAX_KEYBOARD_MENUS-accels_used;
       accels[id] = hMenu;
     }
   }

   if (id>=0 && id<=MAX_KEYBOARD_MENUS) {
     return id_accel + id;
   }

   return 0;
}


void AddButtons(HWND hwndTB)
{
	int stringID;

      MENUITEMINFO mInfo;
      mInfo.cbSize = sizeof(mInfo);
      int i;
      int count = GetMenuItemCount(m_menu);
      for (i=0; i<count; i++) {
	 if ( GetMenuState(m_menu, i, MF_BYPOSITION) & MF_POPUP) {
	    TCHAR temp[128];
	    mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
	    mInfo.cch = 126;
	    mInfo.dwTypeData = temp;
	    GetMenuItemInfo(m_menu, i, MF_BYPOSITION, &mInfo);
	    temp[_tcslen(temp)+1] = '\0';

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
	    TCHAR temp[128];
	    mInfo.fMask = MIIM_TYPE | MIIM_ID;
	    mInfo.cch = 126;
	    mInfo.dwTypeData = temp;
	    GetMenuItemInfo(m_menu, i, MF_BYPOSITION, &mInfo);
	    temp[_tcslen(temp)+1] = '\0';

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
}

void DoRebar(HWND rebarWnd) {
   m_menu = GetMenu(GetParent(rebarWnd));
   if (!m_menu) {
     // MessageBox(NULL, ERROR_NO_MENU, PLUGIN_NAME, 0);
     return;
   }

   if (nMenuType==2) {
      DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
	 CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
	 TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

      // Create the toolbar control to be added.
      HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, _T(MENU_NAME),
	 WS_CHILD | dwStyle,
	 0,0,0,0,
	 rebarWnd, (HMENU)/*id*/200,  // note, the id doesn't matter at all, because it will be overridden by kmeleon
	 kPlugin.hDllInstance, NULL
	 );

      if (!hwndTB){
	 MessageBox(NULL, CUTF8_to_T(ERROR_FAILED_TO_CREATE), _T(PLUGIN_NAME), 0);
	 return;
      }

      SetWindowText(hwndTB, _T(MENU_NAME));

      // Register the band name and child hwnd
      kPlugin.kFuncs->RegisterBand(hwndTB, MENU_NAME, true);

      SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);

	  AddButtons(hwndTB);     

      // Get the height of the toolbar.
      DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

     // Compute the width needed for the toolbar
      int ideal = 0;
      int iCount, iButtonCount = SendMessage(hwndTB, TB_BUTTONCOUNT, 0,0);
      for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
      {
         RECT rectButton;
         SendMessage(hwndTB, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
         ideal += rectButton.right - rectButton.left;
      }

      REBARBANDINFO rbBand;
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = //RBBIM_TEXT |
        RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
        RBBIM_SIZE | RBBIM_IDEALSIZE;

      rbBand.fStyle = RBBS_USECHEVRON | RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
      rbBand.lpText     = _T("");
      rbBand.hwndChild  = hwndTB;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cx         = rbBand.cxIdeal = ideal;

      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   }

   if (nMenuType==0 || nMenuType==2) {
     // then hide the menu
     SetMenu(GetParent(rebarWnd), NULL);
   }
}

HWND FindMenuBar(HWND hWndParent)
{
	HWND hReBar = FindWindowEx(hWndParent, NULL, REBARCLASSNAME, NULL);

	int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  
	int x = 0;
	BOOL bFound = FALSE;
	REBARBANDINFO rb;
	rb.cbSize = sizeof(REBARBANDINFO);
	rb.fMask = RBBIM_CHILD;
	while (x < uBandCount && !bFound)
	{
		if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x++, (LPARAM) &rb))
			continue;

		if (!rb.hwndChild) 
			continue;

		TCHAR toolbarName[11];
		GetWindowText(rb.hwndChild, toolbarName, 10);
		if (_tcscmp(toolbarName, _T(MENU_NAME)) == 0)
			return rb.hwndChild;
	}

	return NULL;
}


void DoLocale() {	
	if (!m_menu) return;
	int count = kPlugin.kFuncs->GetWindowsList(nullptr, 0);
	if (!(count>0)) return;

	HWND* hwnd = new HWND[count];
	kPlugin.kFuncs->GetWindowsList(hwnd, count);

	MENUITEMINFO mInfo;
    mInfo.cbSize = sizeof(mInfo);
	for (int j=0; j<count; j++) {
		HWND menuBar = FindMenuBar(hwnd[j]);
		if (!menuBar) continue;
		while (SendMessage(menuBar, TB_DELETEBUTTON, 0, 0) == TRUE) ;
		AddButtons(menuBar);
	}

	delete [] hwnd;
}

void ShowMenuUnderButton(HWND hWndParent, HMENU hMenu, int iID) {
   // Find the toolbar
   HWND hReBar = FindWindowEx(GetActiveWindow(), NULL, REBARCLASSNAME, NULL);
   int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  
   int x = 0;
   BOOL bFound = FALSE;
   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;
   while (x < uBandCount && !bFound) {
         
      if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x++, (LPARAM) &rb))
         continue;
                  
      // toolbar hwnd
      HWND tb = rb.hwndChild;
      RECT rc;
      
      if (!tb) 
	continue;

      TCHAR toolbarName[11];
      GetWindowText(tb, toolbarName, 10);
      if (_tcscmp(toolbarName, _T(MENU_NAME)) != 0) {
	// oops, this isn't our toolbar
         continue;
      }

      TBBUTTON button;
      SendMessage(tb, TB_GETBUTTON, iID, (LPARAM)&button);
      int ButtonID = button.idCommand;

      SendMessage(tb, TB_GETITEMRECT, iID, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(tb, &pt);
      
      if (nMenuType!=2)
	GetCursorPos(&pt);

      MENUITEMINFO mInfo;
      mInfo.cbSize = sizeof(mInfo);

      TCHAR temp[128];
      mInfo.fMask = MIIM_TYPE | MIIM_SUBMENU;
      mInfo.cch = 127;
      mInfo.dwTypeData = temp;
      GetMenuItemInfo(m_menu, iID, MF_BYPOSITION, &mInfo);

      SendMessage(tb, TB_PRESSBUTTON, ButtonID, MAKELONG(true, 0));

      DWORD SelectionMade = TrackPopupMenu( mInfo.hSubMenu,
                               TPM_TOPALIGN | TPM_LEFTALIGN | TPM_NONOTIFY |
                               TPM_LEFTBUTTON | TPM_RETURNCMD,
                               pt.x, pt.y, 0, hWndParent, NULL);

      SendMessage(tb, TB_PRESSBUTTON, ButtonID, MAKELONG(false, 0));

      if (SelectionMade > 0)
         PostMessage(hWndParent, WM_COMMAND, SelectionMade, 0);

      bFound = TRUE;
   }
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;

   if (message == WM_NOTIFY) {
      hdr = *((LPNMHDR)lParam);
      if (hdr.code == TBN_DROPDOWN) {
         tbhdr = *((LPNMTOOLBAR)lParam);
         if (IsMenu((HMENU)(tbhdr.iItem-SUBMENU_OFFSET))){
            TCHAR toolbarName[11];
            GetWindowText(tbhdr.hdr.hwndFrom, toolbarName, 10);
            if (_tcscmp(toolbarName, _T(MENU_NAME)) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }
			//BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
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
   else if (message == WM_COMMAND) {
     WORD command = LOWORD(wParam);
         
     if ((command >= id_accel) && (command < id_accel + MAX_KEYBOARD_MENUS)) {
       int i = command - id_accel;
       int count = -1;
       if (m_menu)
	 count = GetMenuItemCount(m_menu);

       if (m_menu && i<=count && GetMenuState(m_menu, i, MF_BYPOSITION) & MF_POPUP) {
         tbhdr.hdr.hwndFrom = FindMenuBar(hWnd);
         TBBUTTON button;
         SendMessage(tbhdr.hdr.hwndFrom, TB_GETBUTTON, i, (LPARAM)&button);
         tbhdr.iItem = button.idCommand;
		 //BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
		 PostMessage(hWnd, WM_DEFERHOTTRACK, NULL, NULL);
         //ShowMenuUnderButton(hWnd, m_menu, i);
       }
       else {
	 if (MAX_KEYBOARD_MENUS - i <= accels_used) {
	   POINT pt;
	   GetCursorPos(&pt);
	   DWORD SelectionMade = TrackPopupMenu( accels[i],
                               TPM_TOPALIGN | TPM_LEFTALIGN | TPM_NONOTIFY |
                               TPM_LEFTBUTTON | TPM_RETURNCMD,
                               pt.x, pt.y, 0, hWnd, NULL);
	   if (SelectionMade > 0)
	     PostMessage(hWnd, WM_COMMAND, SelectionMade, 0);
	 }
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
