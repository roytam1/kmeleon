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

BOOL gbContinueMenu;
int giCurrentItem; 
HWND ghToolbarWnd;
HHOOK ghhookMsg;
LRESULT CALLBACK MsgHook(int code, WPARAM wParam, LPARAM lParam){
   if (code == MSGF_MENU){
      MSG *msg = (MSG *)lParam;
      if (msg->message == WM_MOUSEMOVE){
         POINT mouse;
         mouse.x = LOWORD(msg->lParam);
         mouse.y = HIWORD(msg->lParam);

         if (ghToolbarWnd){
            ScreenToClient(ghToolbarWnd, &mouse);
            int ndx = SendMessage(ghToolbarWnd, TB_HITTEST, 0, (LPARAM)&mouse);

            if (ndx >= 0){
               TBBUTTON button;
               SendMessage(ghToolbarWnd, TB_GETBUTTON, ndx, (LPARAM)&button);
               if (giCurrentItem != button.idCommand && IsMenu((HMENU)(button.idCommand-SUBMENU_OFFSET))){
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

      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, giCurrentItem, MAKELONG(true, 0));
      ghhookMsg = SetWindowsHookEx(WH_MSGFILTER, MsgHook, hInstance, GetCurrentThreadId());

      RECT rc;
      WPARAM index = SendMessage(ghToolbarWnd, TB_COMMANDTOINDEX, giCurrentItem, 0);
      SendMessage(ghToolbarWnd, TB_GETITEMRECT, index, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(ghToolbarWnd, &pt);

      // the hook may change this, so we need to save it for the TB_PRESSBUTTON
      lastItem = giCurrentItem; 

      TrackPopupMenu((HMENU)(giCurrentItem-SUBMENU_OFFSET), TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

      UnhookWindowsHookEx(ghhookMsg);
      SendMessage(ghToolbarWnd, TB_PRESSBUTTON, lastItem, MAKELONG(false, 0));
   } while (gbContinueMenu);
}

