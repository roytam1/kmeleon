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

#define HwndMenuCur() FindWindow(szMenuClass, NULL)
const char szMenuClass[] = "#32768";

BOOL gbContinueMenu;
int giCurrentItem; 
HWND ghToolbarWnd;
HHOOK ghhookMsg;
LRESULT CALLBACK MsgHook(int code, WPARAM wParam, LPARAM lParam){
   if (code == MSGF_MENU){
      MSG *msg = (MSG *)lParam;
      if (msg->message == WM_LBUTTONDOWN && ghToolbarWnd) {
         POINT mouse;
         mouse.x = LOWORD(msg->lParam);
         mouse.y = HIWORD(msg->lParam);
         if (ScreenToClient(ghToolbarWnd, &mouse) != 0) {
            int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

            // if we clicked on the button for the current menu, we should close
            TBBUTTON button;
            SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button);
            if (giCurrentItem == button.idCommand){
               SendMessage(msg->hwnd, WM_CANCELMODE, 0, 0);
               gbContinueMenu = false;
               return true;
            }
         }
      }
      if (msg->message == WM_MOUSEMOVE && !gbContinueMenu && ghToolbarWnd) {
         POINT mouse;
         mouse.x = LOWORD(msg->lParam);
         mouse.y = HIWORD(msg->lParam);

         if (ScreenToClient(ghToolbarWnd, &mouse) != 0) {
            int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

            TBBUTTON button;
            SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button);
            if (giCurrentItem != button.idCommand && IsMenu((HMENU)(button.idCommand-SUBMENU_OFFSET))){

               RECT rect = {0};
               HWND hw = HwndMenuCur();
               GetWindowRect(hw, &rect);

               ClientToScreen(ghToolbarWnd, &mouse);

               if (!(mouse.x >= rect.left && mouse.x <= rect.right &&
                     mouse.y >= rect.top  && mouse.y <= rect.bottom)) {

                  SendMessage(msg->hwnd, WM_CANCELMODE, 0, 0);
                  
                  // this basically tells the loop, "we would like to enter a new menu loop with this item:"
                  giCurrentItem = button.idCommand;
                  gbContinueMenu = true;
                  
                  return true;
               }
            }
         }
      }
   }
   return CallNextHookEx(ghhookMsg, code, wParam, lParam);
}

void BeginHotTrack(NMTOOLBAR *nmToolbar, HINSTANCE hInstance, HWND hWnd)
{
   ghToolbarWnd = nmToolbar->hdr.hwndFrom;
   giCurrentItem = nmToolbar->iItem;

   int lastItem;

   do {
      gbContinueMenu = false;

      RECT rc;
      TPMPARAMS tpm;
      WPARAM index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
      SendMessage(ghToolbarWnd, TB_GETITEMRECT, index, (LPARAM) &rc);
      MapWindowPoints(ghToolbarWnd, HWND_DESKTOP,  (LPPOINT)&rc, 2);
      // setup an exclusion area so the menu doesn't overlap the button
      tpm.cbSize = sizeof(TPMPARAMS);
      tpm.rcExclude = rc;

      // the hook may change this, so we need to save it for the TB_PRESSBUTTON
      lastItem = giCurrentItem; 
      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(true, 0));
      ghhookMsg = SetWindowsHookEx(WH_MSGFILTER, MsgHook, hInstance, GetCurrentThreadId());
      TrackPopupMenuEx((HMENU)(giCurrentItem-SUBMENU_OFFSET), TPM_LEFTALIGN|TPM_VERTICAL, rc.left, rc.bottom, hWnd, &tpm);
      UnhookWindowsHookEx(ghhookMsg);
      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(false, 0));
   } while (gbContinueMenu);
}
