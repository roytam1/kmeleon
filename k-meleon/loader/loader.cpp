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

#define WIN32_LEAN_AND_MEAN

#include "../KmeleonConst.h"

#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>

#include "loader.h"
#include "config.h"
#include "tray.h"
#include "resource.h"


char *gszAppName = "KMeleon Tray Control";
HINSTANCE ghInstance;
HMENU popup;

BOOL bPersistBrowser;
BOOL bPersistWindow;
BOOL bPersistStartPage;
BOOL bPersistShowNow;

void SetPersistBrowser(BOOL persist) {
   bPersistBrowser = persist;
}
void SetPersistWindow(BOOL persist) {
   bPersistWindow = persist;
}
void SetPersistStartPage(BOOL persist) {
   bPersistStartPage = persist;
}
BOOL GetPersistBrowser() {
   return bPersistBrowser;
}
BOOL GetPersistWindow() {
   return bPersistWindow;
}
BOOL GetPersistStartPage() {
   return bPersistStartPage;
}

void OnCreate(HWND hWnd) {

   CreateTrayIcon(hWnd, 1, ghInstance, ICON_KMELEON, "K-Meleon");

   // create the popup menu
   popup = CreatePopupMenu();
   AppendMenu(popup, MF_STRING, 1, "&About");
   AppendMenu(popup, MF_SEPARATOR, NULL, "");
   AppendMenu(popup, MF_STRING, 2, "&Config");
   AppendMenu(popup, MF_SEPARATOR, NULL, "");
   AppendMenu(popup, MF_STRING, 3, "E&xit");

   LoadSettings();

   // load the browser, it will automatically ask us for settings
   if (bPersistBrowser)
      LoadBrowser();
}

void UpdateBrowser() {
   // see if kmeleon is already running...
   HWND hwndKmeleon = FindWindowEx(NULL, NULL, "KMeleon", NULL);

   // if kmeleon is already running, tell it to update the stay resident flags
   if (hwndKmeleon) {
      WPARAM flags = bPersistBrowser * PERSIST_BROWSER | \
                     bPersistWindow * PERSIST_WINDOW |   \
                     bPersistStartPage * PERSIST_STARTPAGE;

      PostMessage(hwndKmeleon, UWM_PERSIST_SET, flags, NULL);
   }
   else if (bPersistBrowser)
      LoadBrowser();
}

BOOL LoadBrowser() {

   // check if kmeleon is already running...
   HWND hwndKmeleon = FindWindowEx(NULL, NULL, "KMeleon", NULL);

   // if kmeleon is already running, return true
   if (hwndKmeleon)
      return TRUE;

   // get the current path
   char path[MAX_PATH];
   GetModuleFileName(NULL, path, MAX_PATH);
   int x;
   x = strlen(path)-1;
   while (x>0 && path[x] != '\\') x--;
   if (x>0) path[x+1] = 0; 
      
   if (ShellExecute(NULL, NULL, "k-meleon.exe", NULL, path, SW_SHOW) > (HINSTANCE) 32)
      return FALSE;
   else
      return TRUE;
}

void ShowBrowser() {
   // see if kmeleon is already running...
   HWND hwndKmeleon = FindWindowEx(NULL, NULL, "KMeleon", NULL);

   // if kmeleon is already running, tell it to display a window
   if (hwndKmeleon)
      PostMessage(hwndKmeleon, UWM_PERSIST_SHOW, NULL, NULL);

   // if it's not, try to load it
   // it will detect us, and automatically stay resident
   else {
      bPersistShowNow=TRUE;
      LoadBrowser();
   }
}

void LoadSettings() {
   
   // load default settings
   bPersistBrowser = TRUE;
   bPersistWindow = FALSE;
   bPersistStartPage = FALSE;

   // find out from the registry where the favorites are located.
   HKEY hKey;

   if(RegOpenKey(HKEY_CURRENT_USER, "Software\\K-Meleon\\Loader", &hKey) == ERROR_SUCCESS) {
      DWORD dwTemp;
      DWORD dwSize = sizeof(dwTemp);

      if (RegQueryValueEx(hKey, "Preload Browser", NULL, NULL, (LPBYTE) &dwTemp, &dwSize) == ERROR_SUCCESS)
         bPersistBrowser = (dwTemp);

      if (RegQueryValueEx(hKey, "Preload Window", NULL, NULL, (LPBYTE) &dwTemp, &dwSize) == ERROR_SUCCESS)
         bPersistWindow = (dwTemp);

      if (RegQueryValueEx(hKey, "Preload StartPage", NULL, NULL, (LPBYTE) &dwTemp, &dwSize) == ERROR_SUCCESS)
         bPersistStartPage = (dwTemp);

      RegCloseKey(hKey);
   }
}

void SaveSettings() {

   HKEY hKey;
	if(RegCreateKeyEx (HKEY_CURRENT_USER, "Software\\K-Meleon\\Loader", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {

      DWORD dwTemp;
      DWORD dwSize = sizeof(dwTemp);

      dwTemp = bPersistBrowser;
      RegSetValueEx(hKey, "Preload Browser", NULL, REG_DWORD, (LPBYTE) &dwTemp, dwSize);

      dwTemp = bPersistWindow;
      RegSetValueEx(hKey, "Preload Window", NULL, REG_DWORD, (LPBYTE) &dwTemp, dwSize);

      dwTemp = bPersistStartPage;
      RegSetValueEx(hKey, "Preload StartPage", NULL, REG_DWORD, (LPBYTE) &dwTemp, dwSize);
   }
}

void OnClose (HWND hWnd) {
   RemoveTrayIcon(hWnd, 1);

   // if kmeleon is running, tell it to clear all persist flags
   HWND hwndKmeleon = FindWindowEx(NULL, NULL, "KMeleon", NULL);
   if (hwndKmeleon)
      PostMessage(hwndKmeleon, UWM_PERSIST_SET, NULL, NULL);

   DestroyMenu(popup);

   PostQuitMessage(0);
}

LRESULT OnGetPersist() {

   LRESULT flags = bPersistBrowser * PERSIST_BROWSER | \
                   bPersistWindow * PERSIST_WINDOW |   \
                   bPersistStartPage * PERSIST_STARTPAGE | \
                   bPersistShowNow * PERSIST_SHOWNOW;

   if (bPersistShowNow)
      bPersistShowNow = FALSE;
   return flags;
}

void TrayLButton(HWND hWnd) {
   ShowBrowser();
}

void TrayRButton(HWND hWnd) {
   POINT CurPos;
   GetCursorPos(&CurPos);

   SetForegroundWindow(hWnd);
   TrackPopupMenu(popup, TPM_RIGHTBUTTON, CurPos.x, CurPos.y, 0, hWnd, NULL);
   PostMessage(hWnd, WM_NULL, 0, 0);
}

WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
 
   // check for prior instances
   HANDLE hMutexOneInstance = CreateMutex( NULL, TRUE, "K-Meleon Loader Instance Mutex" );
	BOOL bAlreadyRunning ( GetLastError() == ERROR_ALREADY_EXISTS );

   if ( hMutexOneInstance )
		ReleaseMutex( hMutexOneInstance );   
   
   if (bAlreadyRunning) {
      if (MessageBox(NULL, "The tray loader is already running.\nWould you like to close it?", "Previous Instance Detected", MB_YESNO) == IDYES) {
         if (HWND hwndPrev = FindWindowEx(NULL, NULL, gszAppName, NULL) ) {
            SendMessage(hwndPrev, WM_CLOSE, NULL, NULL);
         }
      }
      return 0;
   }

   MSG msg;
   HWND hWnd;
	ghInstance = hInstance;

   WNDCLASS wc = {0};
   wc.lpfnWndProc    = WndProc;
   wc.hInstance      = hInstance;
   wc.lpszClassName  = gszAppName;
   if(!RegisterClass(&wc))
      return 0;

   // create window
   hWnd = CreateWindow(gszAppName,"Hidden",NULL,0,0,0,0,NULL,NULL,hInstance,NULL);

   // hide window, this shouldn't be necessary, but it doesn't hurt...
   ShowWindow(hWnd, SW_HIDE);

   while (GetMessage (&msg, NULL, 0, 0)) {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
   }

   UnregisterClass(gszAppName, hInstance);

   return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {

   case WM_CREATE:
      OnCreate(hWnd);
      break;

   case WM_ENDSESSION:
	  if (wParam == FALSE) break;

   case WM_CLOSE:
      OnClose(hWnd);
      break;
  
   case UWM_PERSIST_GET:
      return OnGetPersist();

   case UWM_NOTIFY_ICON:
      switch(LOWORD(lParam)) {
      case WM_LBUTTONDOWN:
         TrayLButton(hWnd);
         break;
      case WM_RBUTTONDOWN:
         TrayRButton(hWnd);
         break;
      }
      break;

   case WM_COMMAND:
      switch(HIWORD(wParam)) {
         case BN_CLICKED:
            switch (LOWORD(wParam)) {
            case 1: // about
               MessageBox(NULL, "K-Meleon Loader .2\n\nCopyright 2001 - 2003, Jeff Doozan\nPart of the K-Meleon project.", "K-Meleon Loader", MB_OK);
               break;
            case 2: // config
               ShowDialog(ghInstance, hWnd);
               break;
            case 3: // exit
               SendMessage(hWnd, WM_CLOSE, 0, 0);
               break;
         }
         break;
      } 
      break;

   default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   return FALSE;
}
