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

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>

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
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

pluginFunctions pFunc = {
   Init,
      Create,
      Config,
      Quit,
      DoMenu,
      DoRebar
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
      "Winamp Plugin",
      &pFunc
};

HIMAGELIST himlHot;

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
   commandIDs = kPlugin.kf->GetCommandIDs(numCommands);

   himlHot = ImageList_LoadImage(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_TOOLBAR_BUTTONS), 12, numCommands, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR );

   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, "This plugin brought to you by the letter B", "Winamp-Kmeleon plugin", 0);
}

void Quit(){
   ImageList_Destroy(himlHot);
}

void DoMenu(HMENU menu, char *param){
   AppendMenu(menu, MF_STRING, commandIDs + 1, "Play");
   AppendMenu(menu, MF_STRING, commandIDs + 2, "Pause");
   AppendMenu(menu, MF_STRING, commandIDs + 3, "Stop");
   AppendMenu(menu, MF_SEPARATOR, 0, "-");
   AppendMenu(menu, MF_STRING, commandIDs + 0, "Previous");
   AppendMenu(menu, MF_STRING, commandIDs + 4, "Next");
}

void DoRebar(HWND rebarWnd){
   TBBUTTON buttons[numCommands];
   int i;
   for (i=0; i<numCommands; i++){
      buttons[i].iBitmap = i;
      buttons[i].idCommand = commandIDs + i;
      buttons[i].iString = i + 1;

      buttons[i].dwData = 0;
      buttons[i].fsState = TBSTATE_ENABLED;
      buttons[i].fsStyle = TBSTYLE_BUTTON;
      buttons[i].bReserved[0] = 0;
   };

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT /* | TBSTYLE_AUTOSIZE | TBSTYLE_LIST | TBSTYLE_TOOLTIPS */;

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

   if (!hwndTB){
      MessageBox(NULL, "Failed to create winamp toolbar", NULL, 0);
      return;
   }

   kPlugin.kf->RegisterToolBar(hwndTB, "Winamp");

   //SetWindowText(hwndTB, "Winamp");

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
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = LOWORD(dwBtnSize) * numCommands;
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

   /*
   UINT bandPos = SendMessage(rebarWnd, RB_GETBANDCOUNT, (WPARAM)0, (LPARAM)0) - 1;
   SendMessage(rebarWnd, RB_MINIMIZEBAND, (WPARAM)bandPos, (LPARAM)0);
   SendMessage(rebarWnd, RB_MAXIMIZEBAND, (WPARAM)bandPos, (LPARAM)true);
   */
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);
      if (command >= commandIDs && command < (commandIDs + numCommands)){
         HWND hwndWinamp = FindWindow("Winamp v1.x",NULL);
         SendMessage(hwndWinamp, WM_COMMAND, commandTable[command - commandIDs], 0);
         return true;
      }
   }
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// so it doesn't munge the function name
extern "C" {

   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }

   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
      short position = dis->itemID - commandIDs;
      int top = (dis->rcItem.bottom - dis->rcItem.top - 12) / 2;
      top += dis->rcItem.top;

      ImageList_Draw(himlHot, position, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
      return 14;
   }

}
