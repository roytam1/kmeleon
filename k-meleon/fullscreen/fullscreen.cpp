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
#include "..\KmeleonConst.h"

#define PLUGIN_NAME "Fullscreen Plugin"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
// #include <afxres.h>
#include <stdlib.h>

#include "resource.h"

#define _T(x) x

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"


int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

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
      else if (stricmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
      else return 0;

      return 1;
   }
   return 0;
}


BOOL bHideReBar, bHideStatusBar, bAutoFullscreen;

HINSTANCE ghInstance;

RECT rectFullScreenWindowRect;
INT id_fullscreen;

struct fullscreen {
  HWND hWnd;
  HWND hReBar, hStatusBar;
  BOOL bMaximized;
  BOOL bReBarVisible, bStatusBarVisible;

  WINDOWPLACEMENT wpOld;
  BOOL bFullScreen;

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

int Init(){
   kPlugin.kFuncs->GetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_rebar"), &bHideReBar, (void *)"1");
   kPlugin.kFuncs->GetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_statusbar"), &bHideStatusBar, (void *)"1");
   kPlugin.kFuncs->GetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.auto"), &bAutoFullscreen, (void *)&bAutoFullscreen);
	id_fullscreen = kPlugin.kFuncs->GetCommandIDs(1);
   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
	if (bAutoFullscreen)
	  PostMessage(hWndParent, WM_COMMAND, id_fullscreen, 0);
}

void Config(HWND hWndParent) {
	DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_PREFS), hWndParent, (DLGPROC)DlgProc, (LPARAM)NULL);
}

void Quit(){
  while (root)
    remove_FS(root->hWnd);
}

void DoMenu(HMENU menu, char *param) {
   if (*param)
         AppendMenu(menu, MF_STRING, id_fullscreen, param);
   else AppendMenu(menu, MF_STRING, id_fullscreen, "&Full Screen");
}

int DoAccel(char *param) {
   return id_fullscreen;
}

void DoRebar(HWND rebarWnd) {
}

void HideClutter(HWND hWndParent, FS *fs) {
   fs->hReBar = FindWindowEx(hWndParent, NULL, REBARCLASSNAME, NULL);
   fs->hStatusBar = FindWindowEx(hWndParent, NULL, STATUSCLASSNAME, NULL);

   if (fs->bFullScreen) {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(wp);
      GetWindowPlacement(hWndParent, &wp);
      fs->bMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
      if (fs->hReBar) {
         fs->bReBarVisible = IsWindowVisible(fs->hReBar);     // save intitial visibility state
         ShowWindow(fs->hReBar, !bHideReBar);             // hide/unhide rebar
      }
      if (fs->hStatusBar) {
         fs->bStatusBarVisible = IsWindowVisible(fs->hStatusBar);
         ShowWindow(fs->hStatusBar, !bHideStatusBar);
      }
   }
   else {
      if (fs->hReBar) ShowWindow(fs->hReBar, fs->bReBarVisible);
      if (fs->hStatusBar) ShowWindow(fs->hStatusBar, fs->bStatusBarVisible);
   }
}


// Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case WM_GETMINMAXINFO:

      FS *fs;
      fs = find_FS(hWnd);
      if (!fs)
         fs = create_FS(hWnd);

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

   case WM_CLOSE:

      if (find_FS(hWnd))
         remove_FS(hWnd);
      break;

   case WM_COMMAND:
      WORD command = LOWORD(wParam);
      if (command == id_fullscreen) {

         WINDOWPLACEMENT wpNew;

         FS *fs = find_FS(hWnd);
         if (!fs)
            fs = create_FS(hWnd);

         if (!fs->bFullScreen) {

            RECT rectDesktop;
            fs->bFullScreen=TRUE;

            fs->wpOld.length = sizeof (fs->wpOld);
            GetWindowPlacement(hWnd, &fs->wpOld);
  
            GetWindowRect(GetDesktopWindow(), &rectDesktop );
            AdjustWindowRectEx(&rectDesktop, GetWindowLong(hWnd, GWL_STYLE), (GetMenu(hWnd)?true:false), GetWindowLong(hWnd, GWL_EXSTYLE));

            rectFullScreenWindowRect = rectDesktop;
            wpNew = fs->wpOld;

            wpNew.showCmd = SW_SHOWNORMAL;
            wpNew.rcNormalPosition = rectDesktop;

            HideClutter(hWnd, fs);
         }
         else  {
            fs->bFullScreen=FALSE;
            wpNew = fs->wpOld;
            HideClutter(hWnd, fs);
            if (fs->bMaximized) ShowWindow(hWnd, SW_MAXIMIZE);
         }
         SetWindowPlacement (hWnd, &wpNew);
         return true;
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
         break;
      case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDOK:
                     bHideReBar = SendDlgItemMessage(hWnd, IDC_HIDEREBAR, BM_GETCHECK, 0, 0);
                     bHideStatusBar = SendDlgItemMessage(hWnd, IDC_HIDESTATUSBAR, BM_GETCHECK, 0, 0);
                     bAutoFullscreen = SendDlgItemMessage(hWnd, IDC_AUTOFULLSCREEN, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_rebar"), &bHideReBar);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_statusbar"), &bHideStatusBar);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.auto"), &bAutoFullscreen);
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
