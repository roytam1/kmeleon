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

#include "StdAfx.h"

#include "hot_tracking.h"
#include <tchar.h>
#define HwndMenuCur() FindWindow(szMenuClass, NULL)
const TCHAR szMenuClass[] = _T("#32768");

BOOL gbContinueMenu;
int giCurrentItem; 
HWND ghToolbarWnd;
HWND ghFrame;
HHOOK ghhookMsg;
UINT giCmd;
UINT giButton;
HMENU ghSelMenu;
BOOL gbSelPopup;

LRESULT CALLBACK MsgHook(int code, WPARAM wParam, LPARAM lParam){
   if (code == MSGF_MENU) {
      MSG *msg = (MSG *)lParam;
	  switch (msg->message)
	  {
	  case WM_MENUSELECT:
			ghSelMenu = (HMENU)msg->lParam;
			if (ghSelMenu) giCmd = LOWORD(msg->wParam);
			gbSelPopup = (HIWORD(msg->wParam) & MF_POPUP) != 0;
			break;

	  case WM_KEYDOWN:
		  switch (msg->wParam)
		  {
		  case VK_LEFT:
			  {
				  if (ghSelMenu != (HMENU)(giCurrentItem-SUBMENU_OFFSET))
					  break;

				  UINT index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
				  if (index == 0)
					  break;

				  TBBUTTON button;
				  if (!SendMessage(ghToolbarWnd, TB_GETBUTTON, index-1, (LPARAM)&button))
					  break;

				  SendMessage(ghFrame, WM_CANCELMODE, 0, 0);
				  gbContinueMenu = true;
				  giCurrentItem = button.idCommand;
				  return TRUE;
			  }
		  case VK_RIGHT:
			  {
				  if (gbSelPopup)
					  break;

				  UINT index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
				  UINT count = SendMessage(ghToolbarWnd, TB_BUTTONCOUNT, 0, 0);
				  if (index >= count)
					  break;

				  TBBUTTON button;
				  if (!SendMessage(ghToolbarWnd, TB_GETBUTTON, index+1, (LPARAM)&button))
					  break;

				  SendMessage(ghFrame, WM_CANCELMODE, 0, 0);
				  gbContinueMenu = true;
				  giCurrentItem = button.idCommand;
				  return TRUE;
			  }
			case VK_ESCAPE:
				break;
		  }
		  break;

	  case WM_MBUTTONUP:
	  case WM_RBUTTONUP:
		  if (!giCmd || gbSelPopup)
			  break;

		  giButton = (msg->message == WM_MBUTTONUP ? 1 : 2);
		  SendMessage(ghFrame, WM_CANCELMODE, 0, 0);
		  gbContinueMenu = false;
		  return TRUE;

	  case WM_LBUTTONDOWN:
		  {
			  if (!ghToolbarWnd)
				  break;

			  POINT mouse;
			  mouse.x = LOWORD(msg->lParam);
			  mouse.y = HIWORD(msg->lParam);
			  if (ScreenToClient(ghToolbarWnd, &mouse) == 0)
				  break;

			  int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

			  // if we clicked on the button for the current menu, we should close
			  TBBUTTON button;
			  if (SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button))
				  if (giCurrentItem == button.idCommand){
					  SendMessage(ghFrame, WM_CANCELMODE, 0, 0);
					  gbContinueMenu = false;
					  return TRUE;
				  }
		  }
		  break;

	  case WM_MOUSEMOVE:
		  {
			  static POINT lastMouse = {0};
			  if (gbContinueMenu || !ghToolbarWnd)
				  break;

			  POINT mouse;
			  mouse.x = LOWORD(msg->lParam);
			  mouse.y = HIWORD(msg->lParam);

			  if (mouse.x == lastMouse.x && mouse.y == lastMouse.y)
				  break;

			  lastMouse = mouse;

			  if (ScreenToClient(ghToolbarWnd, &mouse) == 0)
				  break;

			  int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

			  TBBUTTON button;
			  if (!SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button))
				  break;

			  if (giCurrentItem != button.idCommand && IsMenu((HMENU)(button.idCommand-SUBMENU_OFFSET))){

				  RECT rect = {0};
				  HWND hw = HwndMenuCur();
				  GetWindowRect(hw, &rect);

				  ClientToScreen(ghToolbarWnd, &mouse);

				  if (!(mouse.x >= rect.left && mouse.x <= rect.right &&
					  mouse.y >= rect.top  && mouse.y <= rect.bottom)) {

						  SendMessage(ghFrame, WM_CANCELMODE, 0, 0);

						  // this basically tells the loop, "we would like to enter a new menu loop with this item:"
						  giCurrentItem = button.idCommand;
						  gbContinueMenu = true;
						  return TRUE;
					  }
			  }
		  }
		  break;
	  }
   }
   return CallNextHookEx(ghhookMsg, code, wParam, lParam);
}

void BeginHotTrack(NMTOOLBAR *nmToolbar, HINSTANCE hInstance, HWND hWnd)
{
   ghToolbarWnd = nmToolbar->hdr.hwndFrom;
   giCurrentItem = nmToolbar->iItem;

   int lastItem;
   int cmd = 0;
   do {
      gbContinueMenu = false;
      giCmd = 0;
	  giButton = 0;
	  ghSelMenu = NULL;
	  gbSelPopup = FALSE;
	  ghSelMenu = (HMENU)(giCurrentItem-SUBMENU_OFFSET);
	  ghFrame = hWnd;
      
	  // setup an exclusion area so the menu doesn't overlap the button
	  RECT rc;
      TPMPARAMS tpm;
      UINT index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
      SendMessage(ghToolbarWnd, TB_GETITEMRECT, index, (LPARAM) &rc);
      MapWindowPoints(ghToolbarWnd, HWND_DESKTOP,  (LPPOINT)&rc, 2);
      tpm.cbSize = sizeof(TPMPARAMS);
      tpm.rcExclude = rc;

	  SendMessage(hWnd, WM_MENUSELECT, MAKELONG(index, MF_POPUP | MF_HILITE | MF_MOUSESELECT), (LPARAM)ghSelMenu);

      // the hook may change this, so we need to save it for the TB_PRESSBUTTON
      lastItem = giCurrentItem; 
      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(true, 0));
      ghhookMsg = SetWindowsHookEx(WH_MSGFILTER, MsgHook, hInstance, GetCurrentThreadId());
      cmd = TrackPopupMenuEx(ghSelMenu, TPM_RETURNCMD|TPM_LEFTALIGN|TPM_VERTICAL, rc.left, rc.bottom, hWnd, &tpm);
      UnhookWindowsHookEx(ghhookMsg);
      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(false, 0));
   } while (gbContinueMenu);
   
   if (giCmd && giButton) cmd = giCmd;
   SendMessage(hWnd, WM_COMMAND, MAKELONG(cmd, 0) , giButton);
}
