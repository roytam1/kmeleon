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
#include <stdlib.h>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"


#define PLUGIN_NAME "History Plugin"

#define _T(x) x
#define PREFERENCE_HISTORY_LENGTH _T("kmeleon.plugins.history.length")
static INT nHistoryLength = 20;

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
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);


struct menulist {
   HMENU hMenu;
   struct menulist *next;
};
typedef struct menulist MenuList;
MenuList *gMenuList = NULL;

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
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else return 0;

      return 1;
   }
   return 0;
}



int Init(){
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_HISTORY_LENGTH, &nHistoryLength, &nHistoryLength);
	if (nHistoryLength<0)
		nHistoryLength = 0;

	ID_HISTORY = kPlugin.kFuncs->GetCommandIDs(nHistoryLength);
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
   MenuList *tmp;
   tmp = (MenuList *) calloc(1, sizeof(struct menulist));
   tmp->hMenu = menu;
   tmp->next = gMenuList;
   gMenuList = tmp;
   AppendMenu(menu, MF_SEPARATOR, 0, "");
}

void DoRebar(HWND rebarWnd) {
}

void ShowMenuUnderButton(HWND hWndParent, HMENU hMenu, UINT uMouseButton, int iID) {
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
      
      int ButtonID = SendMessage(tb, TB_COMMANDTOINDEX, iID, 0);
      if (ButtonID < 0)
         continue;
      if (ButtonID == 0) {
         TBBUTTON button;
         SendMessage(tb, TB_GETBUTTON, 0, (LPARAM) &button);
         if (button.idCommand != iID)
            continue;
      }

      SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(tb, &pt);
      DWORD SelectionMade = TrackPopupMenu(hMenu, TPM_LEFTALIGN | uMouseButton | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);
      
      PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) iID, 0);
      
      if (SelectionMade > 0)
         kPlugin.kFuncs->GotoHistoryIndex(SelectionMade-1);

      bFound = TRUE;
   }
}

void CreateBackMenu (HWND hWndParent, UINT button) {
	int index, count, i, limit;
	char **titles, buf[47];

	if (!kPlugin.kFuncs->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

   if (index>nHistoryLength)
      limit = index-nHistoryLength;
   else limit=0;

   HMENU menu = CreatePopupMenu();

	int x=0;
	for (i = index - 1; i >= limit; i--) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(menu, MF_STRING, i+1, buf);
	}

   ShowMenuUnderButton(hWndParent, menu, button, ID_NAV_BACK);

   DestroyMenu(menu);
}


void CreateForwardMenu (HWND hWndParent, UINT button) {

   int index, count, i, limit;
	char **titles, buf[47];

	if (!kPlugin.kFuncs->GetMozillaSessionHistory (&titles, &count, &index)) {
		return;
	}

	HMENU menu = CreatePopupMenu();

   if (count-index > nHistoryLength)
      limit = index+nHistoryLength;
   else limit=count;

   int x=0;
	for (i = index + 1; i < limit; i++) {
		CondenseMenuText(buf, titles[i], x++);
		AppendMenu(menu, MF_STRING, i+1, buf);
	}

   ShowMenuUnderButton(hWndParent, menu, button, ID_NAV_FORWARD);

   DestroyMenu(menu);
}
 

void UpdateHistoryMenu (HWND hWndParent) {
	int index, count, i;
	char **titles;
	char buf[47];  //  3 spaces for "&# " 20 for beginning of title 3 for "..." 20 for end of title

   MenuList *tmpMenu = gMenuList;
   while (tmpMenu) {
      HMENU ghMenu = tmpMenu->hMenu;

	if (!ghMenu)
		return;

	// Clear the existing history menu 
	for (i=ID_HISTORY;i<ID_HISTORY+nHistoryLength;i++)
		DeleteMenu(ghMenu, i, MF_BYCOMMAND);

	// Add the local history to the menu
	if (!kPlugin.kFuncs->GetMozillaSessionHistory (&titles, &count, &index)) return;
	if (count > nHistoryLength) count = nHistoryLength;

	for (i=count-1;i>=0;i--) {
		CondenseMenuText(buf, titles[i], (count-1 - i) );

		if (i == index)
			AppendMenu(ghMenu, MF_ENABLED | MF_STRING | MF_CHECKED, ID_HISTORY+i, buf);
		else
			AppendMenu(ghMenu, MF_ENABLED | MF_STRING, ID_HISTORY+i, buf);
	}
	tmpMenu = tmpMenu->next;
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


		case WM_SETFOCUS:
		case UWM_UPDATESESSIONHISTORY:
			UpdateHistoryMenu(hWnd);
			break;
		case TB_LBUTTONHOLD:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(hWnd, TPM_LEFTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(hWnd, TPM_LEFTBUTTON);
					break;
			}
			break;
		case TB_RBUTTONDOWN:
			switch (wParam) {
				case ID_NAV_BACK:
					CreateBackMenu(hWnd, TPM_RIGHTBUTTON);
					break;
				case ID_NAV_FORWARD:
					CreateForwardMenu(hWnd, TPM_RIGHTBUTTON);
					break;
			}
			break;

		case WM_COMMAND:
			WORD command;
			command = LOWORD(wParam);
			if ((command >= ID_HISTORY) && (command < ID_HISTORY+nHistoryLength)) {
				kPlugin.kFuncs->GotoHistoryIndex(command-ID_HISTORY);
				return true;
			}
			break;
	    case WM_MENUSELECT:
		{
			int command = LOWORD(wParam);

			if ((command >= ID_HISTORY) && (command < ID_HISTORY+nHistoryLength)) {
				char **titles;
				int count, index;
				if (kPlugin.kFuncs->GetMozillaSessionHistory (&titles, &count, &index)) {
					kPlugin.kFuncs->SetStatusBarText(titles[command - ID_HISTORY]);
					return true;
				}
			}

			break;
		}
	}
	
	return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}
