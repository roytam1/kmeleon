/*
*  Copyright (C) 2000 Brian Harris
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

void BeginHotTrack(NMTOOLBAR *nmToolbar, HINSTANCE hInstance, HWND hWnd);

#define SUBMENU_OFFSET 6000 // this is here to distinguish between submenus and menu items, which may have the same id otherwise
#define MENU_TO_COMMAND(x) (x+SUBMENU_OFFSET)
#define COMMAND_TO_MENU(x) (HMENU)(x-SUBMENU_OFFSET)