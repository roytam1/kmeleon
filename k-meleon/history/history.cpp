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


#include "stdafx.h"
#include "history.h"
#include "../ToolBarExMessages.h"
#include "../resource.h"
#include "../KmeleonMessages.h"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>

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
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

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
	ID_HISTORY_FLAG = kPlugin.kf->GetCommandIDs(21);
	ID_HISTORY = ID_HISTORY_FLAG + 1;
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
	MessageBox(parent, "This plugin brought to you by the letter H", "History plugin", 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){
	AppendMenu(menu, MF_SEPARATOR, ID_HISTORY_FLAG, "");

	/* This separator serves as the "flag" where the session
	history should be added */

}

void DoRebar(HWND rebarWnd){
}


void CreateBackMenu (UINT button) {
	int index, count, i;
	char **titles, buf[47];

	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

	HMENU menu, submenu;

	menu = CreateMenu();
	submenu = CreatePopupMenu();

	int x=0;
	for (i = index - 1; i >= 0; i--) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(submenu, MF_STRING, i+1, buf);
	}

	RECT rc;
	HWND tb = kPlugin.kf->GetToolbarWnd();
	WPARAM ButtonID = ::SendMessage(tb, TB_COMMANDTOINDEX, ID_NAV_BACK, 0);
	::SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
	POINT pt = { rc.left, rc.bottom };
	::ClientToScreen(tb, &pt);
	DWORD SelectionMade = TrackPopupMenu(submenu, TPM_LEFTALIGN | button | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, ::GetActiveWindow(), &rc);

	DestroyMenu(submenu);
	DestroyMenu(menu);

	PostMessage(GetActiveWindow(), WM_REFRESHTOOLBARITEM, (WPARAM) ID_NAV_BACK, 0);

	FreeStringArray (titles, count-1);

	if (SelectionMade > 0) {
		kPlugin.kf->GotoHistoryIndex(SelectionMade-1);
	}
}


void CreateForwardMenu (UINT button) {
	int index, count, i;
	char **titles, buf[47];

	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

	HMENU menu, submenu;

	menu = CreateMenu();
	submenu = CreatePopupMenu();

	int x=0;
	for (i = index + 1; i < count; i++) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(submenu, MF_STRING, i+1, buf);
	}

	RECT rc;
	HWND tb = kPlugin.kf->GetToolbarWnd();
	WPARAM ButtonID = ::SendMessage(tb, TB_COMMANDTOINDEX, ID_NAV_FORWARD, 0);
	::SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
	POINT pt = { rc.left, rc.bottom };
	::ClientToScreen(tb, &pt);
	DWORD SelectionMade = TrackPopupMenu(submenu, TPM_LEFTALIGN | button | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, ::GetActiveWindow(), &rc);

	DestroyMenu(submenu);
	DestroyMenu(menu);

	PostMessage(GetActiveWindow(), WM_REFRESHTOOLBARITEM, ID_NAV_FORWARD, 0);

	FreeStringArray (titles, count-1);

	if (SelectionMade > 0) {
		kPlugin.kf->GotoHistoryIndex(SelectionMade-1);
	}
}

void UpdateHistoryMenu () {
	int index, count, i;
	char **titles;
	char buf[47];  //  3 spaces for "&# " 20 for beginning of title 3 for "..." 20 for end of title


	HMENU hTopMenu = GetMenu(GetActiveWindow()),hHistoryMenu=0, hSubMenu;

	if (!hTopMenu)
		return;


	// Walk the top level menus for our flag
	for (i = 0; i < GetMenuItemCount(hTopMenu); i++) {
		if( (hSubMenu = GetSubMenu(hTopMenu, i-1)) ) {
			for (int x = 0; x < GetMenuItemCount(hSubMenu); x++) {
				MENUITEMINFO mi;
				mi.cbSize = sizeof(MENUITEMINFO);
				mi.fMask = MIIM_ID;

				if (::GetMenuItemInfo(hSubMenu, x, TRUE, &mi))
					if (mi.wID == ID_HISTORY_FLAG)
						hHistoryMenu = hSubMenu;
			}
		}
	}

	if (!hHistoryMenu)
		return;

	// Clear the existing history menu 
	for (i=ID_HISTORY;i<ID_HISTORY+20;i++)
		DeleteMenu(hHistoryMenu, i, MF_BYCOMMAND);

	// Add the local history to the menu
	if (!kPlugin.kf->GetMozillaSessionHistory (&titles, &count, &index)) return;
	if (count > 20) count = 20;

	for (i=count-1;i>=0;i--) {
		CondenseMenuText(buf, titles[i], (count-1 - i) );

		if (i == index)
			AppendMenu(hHistoryMenu, MF_ENABLED | MF_STRING | MF_CHECKED, ID_HISTORY+i, buf);
		else
			AppendMenu(hHistoryMenu, MF_ENABLED | MF_STRING, ID_HISTORY+i, buf);
	}

	FreeStringArray (titles, count-1);	
}

void FreeStringArray(char *array[], int size) {
	int i;
	for (i = 0; i < size; i++)
		delete (array[i]);
	delete (array);
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
	if (len > 43)  {
		memcpy(buf+3, title, 20);
		memcpy(buf+23, "...", 3);
		strcpy(buf+26, title+len-21);
	}
	else
		strcpy(buf+3, title);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

	switch (message) {

		case WM_UPDATESESSIONHISTORY:
			UpdateHistoryMenu();
			return true;
		case TB_LBUTTONHOLD:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(TPM_LEFTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(TPM_LEFTBUTTON);
					break;
			}
			return true;
		case TB_RBUTTONDOWN:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(TPM_RIGHTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(TPM_RIGHTBUTTON);
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
	
	WndProcMap::iterator WndProcIterator;
	WndProcIterator = KMeleonWndProcs.find(hWnd);

	return CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}
