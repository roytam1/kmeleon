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

#include <windows.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void OnCreate(HWND hWnd);
void UpdateBrowser();
BOOL LoadBrowser();
void ShowBrowser();
void LoadSettings();
void SaveSettings();
void OnClose (HWND hWnd);
LRESULT OnGetPersist();
void TrayLButton(HWND hWnd);
void TrayRButton(HWND hWnd);

void SetPersistBrowser(BOOL persist);
void SetPersistWindow(BOOL persist);
void SetPersistStartPage(BOOL persist);
BOOL GetPersistBrowser();
BOOL GetPersistWindow();
BOOL GetPersistStartPage();
