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
// ns_bookmarks.cpp : Plugin that supports netscape-style boomarks
//

#include "stdafx.h"
//#include "resource.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  switch (ul_reason_for_call)
  {
		case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}

int Init();
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "Netscape Bookmark Plugin",
  Init,
  Config,
  Quit,
  DoMenu,
  NULL,
  OnMessage
};

HMENU mainMenu = NULL;

int Init(){
  mainMenu = NULL;
  return true;
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter N", "Netscape Bookmark plugin", 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){

  AppendMenu(menu, MF_STRING, 2000, param);

}

void OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam){
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
