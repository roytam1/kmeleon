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

*  Patch to add multiple window support thanks to Ulf Erikson

*/



#include "fullscreen.h"
#include "resource.h"
#include "..\..\app\KmeleonConst.h"
#include "utils.h"
#define PLUGIN_NAME "Fullscreen Plugin"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
// #include <afxres.h>
#include <stdlib.h>
#include <shellapi.h>

#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"
#include "LocalesUtils.h"
#include "mozilla.h"
Locale* gLoc;

int Load();
void Create(HWND parent);
void CreateTab(HWND parent, HWND tab);
void DestroyTab(HWND parent, HWND tab);
void SwitchTab(HWND parent, HWND tab);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);
void Destroy(HWND hWnd);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

kmeleonFunctions *kFuncs;

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (strcmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
	  else if (strcmp(subject, "CreateTab") == 0) {
         CreateTab((HWND)data1, (HWND)data2);
      }
	  else if (strcmp(subject, "DestroyTab") == 0) {
         DestroyTab((HWND)data1, (HWND)data2);
      }
	  else if (strcmp(subject, "SwitchTab") == 0) {
         SwitchTab((HWND)data1, (HWND)data2);
      }
      else if (strcmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (strcmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (strcmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (strcmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (strcmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
      else if (strcmp(subject, "Destroy") == 0) {
         Destroy((HWND)data1);
      }
	  else if (strcmp(subject, "DoLocale") == 0) {
         if (gLoc) delete gLoc;
		 gLoc = Locale::kmInit(&kPlugin);
	  }
      else return 0;

      return 1;
   }
   return 0;
}


BOOL bHideReBar=1, bHideStatusBar=1, bAutoFullscreen=0, bHideTaskbar=1, bHideTabsBar = 1;

HINSTANCE ghInstance;

RECT rectFullScreenWindowRect;
INT id_fullscreen;

struct fullscreen {
  HWND hWnd;
  HWND hReBar, hStatusBar, hTabsBar;
  BOOL bMaximized;
  BOOL bReBarVisible, bStatusBarVisible, bTabsBarVisible;

  WINDOWPLACEMENT wpOld;
  BOOL bFullScreen;
  BOOL bDomFullScreen;
  CDomEventListener* listener;

  struct fullscreen *next;
};
typedef struct fullscreen FS;

static FS *root;

static FS *create_FS(HWND hWnd) {
  FS *ptr = (FS*)calloc(1, sizeof(struct fullscreen));
  if (ptr) {
    ptr->hWnd = hWnd;
    ptr->next = root;
    root = ptr;
  }
  ptr->listener = new CDomEventListener(hWnd);
  return ptr;
}

static FS *find_FS(HWND hWnd) {
  FS *ptr = root;
  while (ptr && ptr->hWnd != hWnd)
    ptr = ptr->next;
  return ptr;
}

static void remove_FS(HWND hWnd) {

   FS *prev = NULL, *ptr = root;
   while (ptr && (ptr->hWnd != hWnd)) {
      prev = ptr;
      ptr = ptr->next;
   }

   if (ptr) {
      if (prev)
         prev->next = ptr->next;
      else
         root = ptr->next;

      free(ptr);
   }
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         ghInstance = (HINSTANCE) hModule;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}


void HideClutter(HWND hWndParent, FS *fs) {
   fs->hReBar = FindWindowEx(hWndParent, NULL, REBARCLASSNAME, NULL);
   fs->hStatusBar = FindWindowEx(hWndParent, NULL, STATUSCLASSNAME, NULL);
   fs->hTabsBar = FindWindowEx(hWndParent, NULL, REBARCLASSNAME, _T("TabsBar"));

   if (fs->bFullScreen) {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(wp);
      GetWindowPlacement(hWndParent, &wp);
      fs->bMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
      if (fs->hReBar) {
         fs->bReBarVisible = IsWindowVisible(fs->hReBar);     // save intitial visibility state
		 ShowWindow(fs->hReBar, !fs->bDomFullScreen && !bHideReBar);             // hide/unhide rebar
      }
      if (fs->hStatusBar) {
         fs->bStatusBarVisible = IsWindowVisible(fs->hStatusBar);
         ShowWindow(fs->hStatusBar, !fs->bDomFullScreen && !bHideStatusBar);
      }
	  if (fs->hTabsBar) {
         fs->bTabsBarVisible = IsWindowVisible(fs->hTabsBar);
         ShowWindow(fs->hTabsBar, !fs->bDomFullScreen && !bHideTabsBar);
	  }
   }
   else {
      if (fs->hReBar) ShowWindow(fs->hReBar, fs->bReBarVisible);
      if (fs->hStatusBar) ShowWindow(fs->hStatusBar, fs->bStatusBarVisible);
	  if (fs->hTabsBar) ShowWindow(fs->hTabsBar, fs->bTabsBarVisible);
   }
}

void SetFullScreen(HWND hWnd, int fullscreen = -1, bool domFullscreen = false)
{
    WINDOWPLACEMENT wpNew;

    FS *fs = find_FS(hWnd);
    if (!fs) fs = create_FS(hWnd);
	
	if (fullscreen != -1 && (fs->bFullScreen == fullscreen))
		return;

	if (!fs->bFullScreen) {
		fs->bDomFullScreen = domFullscreen;
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_rebar", &bHideReBar, (void *)&bHideReBar);
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_statusbar", &bHideStatusBar, (void *)&bHideStatusBar);
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.auto", &bAutoFullscreen, (void *)&bAutoFullscreen);
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_taskbar", &bHideTaskbar, (void *)&bHideTaskbar);
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_tabsbar", &bHideTabsBar, (void *)&bHideTabsBar);

		RECT rectWindow;
		RECT rectDesktop;

		fs->bFullScreen=TRUE;

		fs->wpOld.length = sizeof (fs->wpOld);
		GetWindowPlacement(hWnd, &fs->wpOld);
  
		GetWindowRect(GetDesktopWindow(), &rectDesktop );
		rectWindow = rectDesktop;

		APPBARDATA abd;
		abd.cbSize = sizeof(abd);
		UINT uState = (UINT) SHAppBarMessage(ABM_GETSTATE, &abd); 

		if ((uState & ABS_ALWAYSONTOP) && !(uState & ABS_AUTOHIDE))
		{
			BOOL fResult = (BOOL) SHAppBarMessage(ABM_GETTASKBARPOS, &abd); 
			if (!bHideTaskbar)
			{
				// Idiotic try to get the taskbar position
				if (abd.rc.top - rectDesktop.top > 10)
					rectWindow.bottom = abd.rc.top;
				else if (rectDesktop.right - abd.rc.right > 10)
					rectWindow.right -= abd.rc.right - abd.rc.left;
					//rectWindow.left = abd.rc.right;
				else if (rectDesktop.bottom - abd.rc.bottom > 10)
					//rectWindow.top = abd.rc.bottom;
					rectWindow.bottom -= abd.rc.bottom - abd.rc.top;
				else
					rectWindow.right = abd.rc.left;
			}

			else if (abd.rc.left <= 1 && abd.rc.top <= 1)
			{
				if (abd.rc.right >= rectDesktop.right) {
					rectWindow.top -= (abd.rc.bottom-abd.rc.top);
					rectWindow.bottom -= abd.rc.bottom - abd.rc.top ;
				}
				else if (abd.rc.bottom >= rectDesktop.bottom) {
					rectWindow.left -= (abd.rc.right-abd.rc.left);
					rectWindow.right -= abd.rc.right - abd.rc.left ;
				}
			}
		}

		AdjustWindowRectEx(&rectWindow, GetWindowLong(hWnd, GWL_STYLE), (GetMenu(hWnd)?true:false), GetWindowLong(hWnd, GWL_EXSTYLE)); 
    	    
		/* rectWindow.top    -= 1;
		rectWindow.left   -= 1;
		rectWindow.bottom += 1;
		rectWindow.right  += 1;*/

		rectFullScreenWindowRect = rectWindow;
		wpNew = fs->wpOld;

		if (fs->wpOld.showCmd!=SW_SHOWMINIMIZED)
			wpNew.showCmd = SW_SHOWNORMAL;
		wpNew.rcNormalPosition = rectWindow;

		HideClutter(hWnd, fs);
    }
    else  {
		fs->bDomFullScreen = false;
		fs->bFullScreen=FALSE;
		wpNew = fs->wpOld;
		HideClutter(hWnd, fs);
		if (fs->bMaximized) ShowWindow(hWnd, SW_MAXIMIZE);
    }
	if (!domFullscreen)
		kPlugin.kFuncs->SetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.last", &fs->bFullScreen, false);
    SetWindowPlacement (hWnd, &wpNew);
    return;
}


int Load(){
	gLoc = Locale::kmInit(&kPlugin);
	id_fullscreen = kPlugin.kFuncs->GetCommandIDs(1);
   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
	BOOL bLast = FALSE;
	kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.last", &bLast, (void *)&bLast);
	if (bAutoFullscreen || bLast)
	  PostMessage(hWndParent, WM_COMMAND, id_fullscreen, 0);
}

void CreateTab(HWND hWndParent, HWND hTab) {
	FS *fs = find_FS(hWndParent);
    if (!fs) fs = create_FS(hWndParent);
	fs->listener->Init(hTab);
	fs->listener->CancelFullScreen();
}

void DestroyTab(HWND hWndParent, HWND hTab) {
	FS *fs = find_FS(hWndParent);
	if (!fs) return;	
	//fs->listener->CancelFullScreen();
	fs->listener->Done(hTab);
	if (fs->bDomFullScreen)
		SetFullScreen(hWndParent, 0);
}

void SwitchTab(HWND hWndOldTab, HWND hTab) {
	HWND hWndParent = ::GetParent(hTab);
	FS *fs = find_FS(hWndParent);
	if (!fs) return;
	fs->listener->CancelFullScreen();
}


void Config(HWND hWndParent) {
	gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_PREFS), hWndParent, (DLGPROC)DlgProc, (LPARAM)NULL);
}

void Quit(){
  while (root)
    remove_FS(root->hWnd);
  delete gLoc;
}

void DoMenu(HMENU menu, char *param) {
	if (*param) {
		char *action = param;
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;
	  AppendMenuA(menu, MF_STRING, id_fullscreen, string);
	}
   else AppendMenuA(menu, MF_STRING, id_fullscreen, kPlugin.kFuncs->Translate("&Full Screen"));
}

int DoAccel(char *param) {
   return id_fullscreen;
}

void DoRebar(HWND rebarWnd) {
}

void Destroy(HWND hWnd) {
  if (find_FS(hWnd))
    remove_FS(hWnd);
}

// Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case WM_GETMINMAXINFO:

      FS *fs;
      fs = find_FS(hWnd);
      if (!fs) break;

      if (fs->bFullScreen) {       
         MINMAXINFO *lpMMI;

         lpMMI = (MINMAXINFO FAR *) lParam;

         lpMMI->ptMaxSize.y       = rectFullScreenWindowRect.bottom - rectFullScreenWindowRect.top;
         lpMMI->ptMaxTrackSize.y  = lpMMI->ptMaxSize.y;
         lpMMI->ptMaxSize.x       = rectFullScreenWindowRect.right - rectFullScreenWindowRect.left;
         lpMMI->ptMaxTrackSize.x  = lpMMI->ptMaxSize.x;
         return false;
      }
      break;

   case WM_COMMAND:
      WORD command = LOWORD(wParam);
      if (command == id_fullscreen) {
		  SetFullScreen(hWnd);
		  return 0;
      }
      break;
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
		case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_HIDEREBAR, BM_SETCHECK, bHideReBar, 0);
         SendDlgItemMessage(hWnd, IDC_HIDESTATUSBAR, BM_SETCHECK, bHideStatusBar, 0);
         SendDlgItemMessage(hWnd, IDC_AUTOFULLSCREEN, BM_SETCHECK, bAutoFullscreen, 0);
		 SendDlgItemMessage(hWnd, IDC_HIDETASKBAR, BM_SETCHECK, bHideTaskbar, 0);
         break;
      case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDOK:
                     bHideReBar = SendDlgItemMessage(hWnd, IDC_HIDEREBAR, BM_GETCHECK, 0, 0);
                     bHideStatusBar = SendDlgItemMessage(hWnd, IDC_HIDESTATUSBAR, BM_GETCHECK, 0, 0);
                     bAutoFullscreen = SendDlgItemMessage(hWnd, IDC_AUTOFULLSCREEN, BM_GETCHECK, 0, 0);
					 bHideTaskbar = SendDlgItemMessage(hWnd, IDC_HIDETASKBAR, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_rebar", &bHideReBar, false);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_statusbar", &bHideStatusBar, false);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.auto", &bAutoFullscreen, false);
					 kPlugin.kFuncs->SetPreference(PREF_BOOL, "kmeleon.plugins.fullscreen.hide_taskbar", &bHideTaskbar, false);
                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;
      case WM_CLOSE:
			EndDialog(hWnd, (LPARAM)NULL);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}

// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

//KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
//}

}
