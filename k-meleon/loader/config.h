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

#define TEXT_START_TITLE "Extreme"
#define TEXT_START_INFO "Preload Start Page\nPrecreate Window\nPreload Browser Engine\n\nThis is only recommended if you use a simple, text based, start page"

#define TEXT_WINDOW_TITLE "Faster"
#define TEXT_WINDOW_INFO "Precreate Window\nPreload Browser Engine"

#define TEXT_BROWSER_TITLE "Fast (Default)"
#define TEXT_BROWSER_INFO "Preload Browser Engine"

#define TEXT_NONE_TITLE "None"
#define TEXT_NONE_INFO  "Don't preload\nany components"

void ShowDialog(HINSTANCE hInstance, HWND parent);
BOOL CALLBACK ConfigDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateDialog(HWND hWnd);