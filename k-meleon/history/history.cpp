/*
*  Copyright (C) 2001 Jeff Doozan
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


//#include "stdafx.h"
#include "history.h"
#include "..\Utils.h"
#include "..\KmeleonConst.h"
#include "..\resource.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"


/*
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
	switch (ul_reason_for_call) {
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

HMENU ghMenu;

pluginFunctions pFunc = {
	Init,
	Create,
	Config,
	Quit,
	DoMenu,
	DoRebar
};

kmeleonPlugin kPlugin = {
	KMEL_PLUGIN_VER,
	"History Plugin",
	&pFunc
};

int Init(){
	ID_HISTORY = kPlugin.kf->GetCommandIDs(20);
	return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
	KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
	MessageBox(parent, "This plugin has no user configurable options", "History plugin", 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){
   ghMenu = menu;
   AppendMenu(menu, MF_SEPARATOR, 0, "");
}

void DoRebar(HWND rebarWnd) {
}

void CreateBackMenu (HWND hWndParent, UINT button) {
	int index, count, i, limit;
	char **titles, buf[47];

	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

   if (index>20)
      limit = index-20;
   else limit=0;

   HMENU menu, submenu;

	menu = CreateMenu();
	submenu = CreatePopupMenu();

	int x=0;
	for (i = index - 1; i >= limit; i--) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(submenu, MF_STRING, i+1, buf);
	}

   // Find the toolbar
   HWND hReBar = FindWindowEx(GetActiveWindow(), NULL, REBARCLASSNAME, NULL);

   UINT uBandIndex = SendMessage(hReBar, RB_IDTOINDEX, 200, 0);     // 200 = Toolbar ID

   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;

   if (SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) uBandIndex, (LPARAM) &rb)) {

      // toolbar hwnd
      HWND tb = rb.hwndChild;
      RECT rc;

      WPARAM ButtonID = SendMessage(tb, TB_COMMANDTOINDEX, ID_NAV_BACK, 0);
	   SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
	   POINT pt = { rc.left, rc.bottom };
	   ClientToScreen(tb, &pt);
      DWORD SelectionMade = TrackPopupMenu(submenu, TPM_LEFTALIGN | button | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);

	   DestroyMenu(submenu);
	   DestroyMenu(menu);

	   PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) ID_NAV_BACK, 0);

      if (SelectionMade > 0) {
		   kPlugin.kf->GotoHistoryIndex(SelectionMade-1);
      }
   }
}


void CreateForwardMenu (HWND hWndParent, UINT button) {

   int index, count, i, limit;
	char **titles, buf[47];

	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

	HMENU menu, submenu;

	menu = CreateMenu();
	submenu = CreatePopupMenu();

   if (count-index > 20)
      limit = index+20;
   else limit=count;

   int x=0;
	for (i = index + 1; i < limit; i++) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(submenu, MF_STRING, i+1, buf);
	}

   // Find the toolbar
   HWND hReBar = FindWindowEx(GetActiveWindow(), NULL, REBARCLASSNAME, NULL);

   UINT uBandIndex = SendMessage(hReBar, RB_IDTOINDEX, 200, 0);     // 200 = Toolbar ID
   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;

   if (SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) uBandIndex, (LPARAM) &rb)) {

      // toolbar hwnd
      HWND tb = rb.hwndChild;
      RECT rc;

   	WPARAM ButtonID = SendMessage(tb, TB_COMMANDTOINDEX, ID_NAV_FORWARD, 0);
	   SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
	   POINT pt = { rc.left, rc.bottom };
	   ClientToScreen(tb, &pt);
	   DWORD SelectionMade = TrackPopupMenu(submenu, TPM_LEFTALIGN | button | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);

	   DestroyMenu(submenu);
	   DestroyMenu(menu);

	   PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) ID_NAV_FORWARD, 0);

      if (SelectionMade > 0) {
		   kPlugin.kf->GotoHistoryIndex(SelectionMade-1);
	   }

   }
}

void UpdateHistoryMenu (HWND hWndParent) {
	int index, count, i;
	char **titles;
	char buf[47];  //  3 spaces for "&# " 20 for beginning of title 3 for "..." 20 for end of title

	if (!ghMenu)
		return;

	// Clear the existing history menu 
	for (i=ID_HISTORY;i<ID_HISTORY+20;i++)
		DeleteMenu(ghMenu, i, MF_BYCOMMAND);

	// Add the local history to the menu
	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) return;
	if (count > 20) count = 20;

	for (i=count-1;i>=0;i--) {
		CondenseMenuText(buf, titles[i], (count-1 - i) );

		if (i == index)
			AppendMenu(ghMenu, MF_ENABLED | MF_STRING | MF_CHECKED, ID_HISTORY+i, buf);
		else
			AppendMenu(ghMenu, MF_ENABLED | MF_STRING, ID_HISTORY+i, buf);
	}
}


void CondenseMenuText(char *buf, char *title, int index) {
	int len;

	if ( (index >= 0) && (index <10) ) {
		buf[0] = '&';
		buf[1] = index +48; // convert int to ascii
		buf[2] = ' ';
	}
	else
		memcpy(buf, "   ", 3);

	len = strlen(title);
	if (len > 43)
      CondenseString(title, 43);

   strcpy(buf+3, title);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

	switch (message) {

		case UWM_UPDATESESSIONHISTORY:
			UpdateHistoryMenu(hWnd);
			return true;
		case TB_LBUTTONHOLD:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(hWnd, TPM_LEFTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(hWnd, TPM_LEFTBUTTON);
					break;
			}
			return true;
		case TB_RBUTTONDOWN:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(hWnd, TPM_RIGHTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(hWnd, TPM_RIGHTBUTTON);
					break;
			}
			return true;

		case WM_COMMAND:
			WORD command;
			command = LOWORD(wParam);
			if ((command >= ID_HISTORY) && (command <= ID_HISTORY+20)) {
				kPlugin.kf->GotoHistoryIndex(command-ID_HISTORY);
				return true;
			}
			break;
	}
	
	return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}
