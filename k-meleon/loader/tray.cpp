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
#include <windows.h>
#include <shellapi.h>

#include "tray.h"

void CreateTrayIcon(HWND hWnd, UINT uId, HINSTANCE hInstance, WORD icon, char *pszTip) {
	NOTIFYICONDATA IconData;

	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hWnd;
	IconData.uID = uId;
	IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	// Send WM_NOTIFY_ICON message when the mouse pointer is over the icon in the sys tray.
	IconData.uCallbackMessage = UWM_NOTIFY_ICON;
	IconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(icon));
	strcpy(IconData.szTip, pszTip);
	Shell_NotifyIcon(NIM_ADD, &IconData);
}

void ModifyTrayIcon(HWND hWnd, UINT uId, HINSTANCE hInstance, WORD icon, char *pszTip) {
	NOTIFYICONDATA IconData;

	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hWnd;
	IconData.uID= uId;
	IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	// Send WM_NOTIFY_ICON message when the mouse pointer is over the icon in the sys tray.
	IconData.uCallbackMessage = UWM_NOTIFY_ICON;
	IconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(icon));
	strcpy(IconData.szTip, pszTip);
	Shell_NotifyIcon(NIM_MODIFY, &IconData);
}

void RemoveTrayIcon(HWND hWnd, UINT uId) {
	NOTIFYICONDATA IconData;

	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hWnd;
	IconData.uID = uId;

	Shell_NotifyIcon(NIM_DELETE, &IconData);	
}