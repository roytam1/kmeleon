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
// kmeleon_winamp.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"
#include "frontend.h"

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

HMENU mainMenu = NULL;

int Init();
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void OnCommand(UINT command);

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "Winamp Plugin",
  Init,
  Config,
  Quit,
  GetMenu,
  OnCommand
};

int Init(){
  // we have to do it this crazy assed way because you apparently can't pass HMENUs between processes
  mainMenu = (HMENU)LoadResource(kPlugin.hDllInstance, FindResource(kPlugin.hDllInstance, MAKEINTRESOURCE(201), RT_MENU));
  return true;
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter B", "Winamp-Kmeleon plugin", 0);
}

void Quit(){
  DestroyMenu(mainMenu);
}

HGLOBAL GetMenu(){
  return LockResource(mainMenu);
}

/*
		play
			SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON2,0);
		pause
			SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON3,0);
		stop
			SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON4,0);
		next
			SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON5,0);
		prev
			SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON1,0);
*/

void OnCommand(UINT command){
  if (command == ID_WINAMP_PREV){
    HWND hwndWinamp = FindWindow("Winamp v1.x",NULL);
    SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON1,0);
  }
  if (command == 2502){
    HWND hwndWinamp = FindWindow("Winamp v1.x",NULL);
    SendMessage(hwndWinamp, WM_COMMAND,WINAMP_BUTTON5,0);
  }
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}