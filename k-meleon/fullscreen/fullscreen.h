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

/*
*  Mark Yen, Dec 2001:
*  - support for multiple windows
*    (moved backup window info into data struct)
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd, char *name);
int DoAccel(char *param);

// each window has a WindowInfo struct
struct WindowInfo {
   BOOL bMaximized;
   BOOL bReBarVisible, bStatusBarVisible;

   WINDOWPLACEMENT wpOld;
   BOOL bFullScreen;
};

void HideClutter(HWND hWndParent, WindowInfo *wndinfo);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
