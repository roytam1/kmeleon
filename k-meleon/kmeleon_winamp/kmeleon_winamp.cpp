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

HMENU mainMenu = NULL;

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
void OnCommand(UINT command);
void DoRebar(HWND rebarWnd);

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "Winamp Plugin",
  Init,
  Config,
  Quit,
  DoMenu,
  OnCommand,
  DoRebar
};

HBITMAP prevBmp;
HBITMAP nextBmp;

UINT commandIDs;

const numCommands = 5;

DWORD commandTable[numCommands] = {
  WINAMP_BUTTON1, /* prev */
  WINAMP_BUTTON2, /* play */
  WINAMP_BUTTON3, /* pause */
  WINAMP_BUTTON4, /* stop */
  WINAMP_BUTTON5  /* next */
};

int Init(){

  // allocate some ids
  commandIDs = kPlugin.GetCommandIDs(numCommands);

  mainMenu = CreateMenu();

  AppendMenu(mainMenu, MF_STRING, commandIDs + 1, "Play");
  AppendMenu(mainMenu, MF_STRING, commandIDs + 2, "Pause");
  AppendMenu(mainMenu, MF_STRING, commandIDs + 3, "Stop");
  AppendMenu(mainMenu, MF_SEPARATOR, 0, "-");
  AppendMenu(mainMenu, MF_STRING, commandIDs + 0, "Previous");
  AppendMenu(mainMenu, MF_STRING, commandIDs + 4, "Next");

  return true;
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter B", "Winamp-Kmeleon plugin", 0);
}

void Quit(){
  DestroyMenu(mainMenu);

  DeleteObject(prevBmp);
  DeleteObject(nextBmp);
}

void DoMenu(HMENU menu, char *param){
  AppendMenu(menu, MF_POPUP | MF_STRING, (UINT)mainMenu, param);
}

void DoRebar(HWND rebarWnd){
  TBBUTTON buttons[numCommands];
  int i;
  for (i=0; i<numCommands; i++){
    buttons[i].iBitmap = i;
    buttons[i].idCommand = commandIDs + i;

    buttons[i].dwData = 0;
    buttons[i].fsState = TBSTATE_ENABLED;
    buttons[i].fsStyle = TBSTYLE_BUTTON;
    buttons[i].iString = 0;
    buttons[i].bReserved[0] = 0;
  };

  DWORD dwStyle = 0x40 | CCS_NOPARENTALIGN | CCS_NORESIZE | /*CCS_NOMOVEY | */
    TBSTYLE_FLAT | /*TBSTYLE_AUTOSIZE | TBSTYLE_LIST |*/ TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT;

  // Create the toolbar control to be added.
  HWND hwndTB = CreateToolbarEx(rebarWnd, dwStyle,
    /*id*/ 200,
    /*nBitmaps*/ 2,
    /*hBMInst*/ kPlugin.hDllInstance,
    /*wBMID*/ IDB_TOOLBAR_BUTTONS,
    /*lpButtons*/ buttons,
    /*iNumButtons*/ numCommands,
    /*dxButton*/ 12,
    /*dyButton*/ 12,
    /*dxBitmap*/ 12,
    /*dyBitmap*/ 12,
    /*uStructSize*/ sizeof(TBBUTTON)
    );

  //SetWindowText(hwndTB, "Winamp");

  HIMAGELIST himlHot = ImageList_LoadImage(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_TOOLBAR_BUTTONS), 12, numCommands, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR );

  SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)himlHot);

  // Get the height of the toolbar.
  DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

  REBARBANDINFO rbBand;
  rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
  rbBand.fMask  = /*RBBIM_TEXT |*/
    RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
    RBBIM_SIZE | RBBIM_IDEALSIZE;

  rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
  rbBand.lpText     = "Winamp";
  rbBand.hwndChild  = hwndTB;
  rbBand.cxMinChild = 0;
  rbBand.cyMinChild = HIWORD(dwBtnSize);
  rbBand.cx         = rbBand.cxIdeal = LOWORD(dwBtnSize) * numCommands;

  // Add the band that has the toolbar.
  SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

  UINT bandPos = SendMessage(rebarWnd, RB_GETBANDCOUNT, (WPARAM)0, (LPARAM)0) - 1;
  SendMessage(rebarWnd, RB_MINIMIZEBAND, (WPARAM)bandPos, (LPARAM)0);
  SendMessage(rebarWnd, RB_MAXIMIZEBAND, (WPARAM)bandPos, (LPARAM)true);
}

void OnCommand(UINT command){
  if (command >= commandIDs && command < (commandIDs + numCommands)){
    HWND hwndWinamp = FindWindow("Winamp v1.x",NULL);
    SendMessage(hwndWinamp, WM_COMMAND, commandTable[command - commandIDs], 0);
  }
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
