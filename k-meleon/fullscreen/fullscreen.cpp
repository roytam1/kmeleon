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

#include "fullscreen.h"
#include "resource.h"
#include "..\KmeleonConst.h"

#define PLUGIN_NAME "Fullscreen Plugin"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <afxres.h>

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


HWND hReBar, hStatusBar;
BOOL bHideReBar, bHideStatusBar, bMaximized;
BOOL bReBarVisible, bStatusBarVisible;

HINSTANCE ghInstance;

WINDOWPLACEMENT wpOld;
RECT rectFullScreenWindowRect;
BOOL bFullScreen=0;
INT id_fullscreen;

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
   kPlugin.kFuncs->GetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_rebar"), &bHideReBar, "1");
   kPlugin.kFuncs->GetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_statusbar"), &bHideStatusBar, "1");
	id_fullscreen = kPlugin.kFuncs->GetCommandIDs(1);
   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND hWndParent) {
	DialogBoxParam(ghInstance ,MAKEINTRESOURCE(IDD_PREFS), hWndParent, (DLGPROC)DlgProc, NULL);
}

void Quit(){
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

void HideClutter(HWND hWndParent) {
   hReBar = FindWindowEx(hWndParent, NULL, REBARCLASSNAME, NULL);
   hStatusBar = FindWindowEx(hWndParent, NULL, STATUSCLASSNAME, NULL);

   if (bFullScreen) {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(wp);
      GetWindowPlacement(hWndParent, &wp);
      bMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
      if (hReBar) {
         bReBarVisible = IsWindowVisible(hReBar);     // save intitial visibility state
         ShowWindow(hReBar, !bHideReBar);             // hide/unhide rebar
      }
      if (hStatusBar) {
         bStatusBarVisible = IsWindowVisible(hStatusBar);
         ShowWindow(hStatusBar, !bHideStatusBar);
      }
   }
   else {
      if (hReBar) ShowWindow(hReBar, bReBarVisible);
      if (hStatusBar) ShowWindow(hStatusBar, bStatusBarVisible);
   }
}


// Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case WM_GETMINMAXINFO:
      if (bFullScreen) {       
         MINMAXINFO *lpMMI;

         lpMMI = (MINMAXINFO FAR *) lParam;

         lpMMI->ptMaxSize.y       = rectFullScreenWindowRect.bottom - rectFullScreenWindowRect.top;
         lpMMI->ptMaxTrackSize.y  = lpMMI->ptMaxSize.y;
         lpMMI->ptMaxSize.x       = rectFullScreenWindowRect.right - rectFullScreenWindowRect.left;
         lpMMI->ptMaxTrackSize.x  = lpMMI->ptMaxSize.x;
         return false;
      }
   case WM_COMMAND:
      WORD command = LOWORD(wParam);
      if (command == id_fullscreen) {

         WINDOWPLACEMENT wpNew;

         if (!bFullScreen) {

            RECT rectDesktop;
            bFullScreen=TRUE;

            wpOld.length = sizeof (wpOld);
            GetWindowPlacement(hWnd, &wpOld);
  
            GetWindowRect(GetDesktopWindow(), &rectDesktop );
            AdjustWindowRectEx(&rectDesktop, GetWindowLong(hWnd, GWL_STYLE), (GetMenu(hWnd)?true:false), GetWindowLong(hWnd, GWL_EXSTYLE));

            rectFullScreenWindowRect = rectDesktop;
            wpNew = wpOld;

            wpNew.showCmd = SW_SHOWNORMAL;
            wpNew.rcNormalPosition = rectDesktop;

            HideClutter(hWnd);
         }
         else  {
            bFullScreen=FALSE;
            wpNew = wpOld;
            HideClutter(hWnd);
            if (bMaximized) ShowWindow(hWnd, SW_MAXIMIZE);
         }
         SetWindowPlacement (hWnd, &wpNew);
         return true;
      }
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
		case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_HIDEREBAR, BM_SETCHECK, bHideReBar, 0);
         SendDlgItemMessage(hWnd, IDC_HIDESTATUSBAR, BM_SETCHECK, bHideStatusBar, 0);
         break;
      case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDOK:
                     bHideReBar = SendDlgItemMessage(hWnd, IDC_HIDEREBAR, BM_GETCHECK, 0, 0);
                     bHideStatusBar = SendDlgItemMessage(hWnd, IDC_HIDESTATUSBAR, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_rebar"), &bHideReBar);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, _T("kmeleon.plugins.fullscreen.hide_statusbar"), &bHideStatusBar);
                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;
      case WM_CLOSE:
			EndDialog(hWnd, NULL);
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
