/*
*  Copyright (C) 2000 Christophe Thibault, Brian Harris, Jeff Doozan
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

#include "stdafx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserView.h"
#include "BrowserFrm.h"

/*
** Middle button panning shit
*/
int CBrowserView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
   int id;
   id = theApp.accel.CheckMouse(message);
   if (id) {
     PostMessage(WM_COMMAND, (WPARAM)ID_MOUSE_ACTION, (LPARAM)id);
     id_mouse = id;
     return TRUE;
   }
   switch (message) {
   case WM_LBUTTONDOWN:
   case WM_RBUTTONDOWN:
      mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
      break;
   case WM_MBUTTONDOWN:
      if(m_panning) StopPanning();
      else StartPanning();
      return TRUE;
   case WM_MBUTTONUP:
      {
      }
      return TRUE;
   }

   return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CBrowserView::OnTimer(UINT nIDEvent)
{
   switch(nIDEvent){
   case 0x1:
      if(m_panning) {
         nsCOMPtr<nsIDOMWindow> s;
	 mWebBrowser->GetContentDOMWindow(getter_AddRefs(s)); 

         if(!s) return;

         POINT p;
         GetCursorPos(&p);

         int scroll_x,scroll_y;

         s->GetScrollX(&scroll_x);
         s->GetScrollY(&scroll_y);

         int dx = (p.x-m_panningPoint.x)/10;
         int dy = (p.y-m_panningPoint.y)/10;

         if(dy!=0) {
            if(dy>0)	scroll_y += dy*dy; 
            else scroll_y -= dy*dy;
            s->ScrollTo(scroll_x,scroll_y);
         }

         if(dx!=0) {
            if(dx>0)	scroll_x += dx*dx;
            else scroll_x -= dx*dx;
            s->ScrollTo(scroll_x,scroll_y);
         }

         int cursor;
         if (dx < 0) {
            if (dy < 0) cursor = IDC_PAN_UPLEFT;
            else if (dy > 0) cursor = IDC_PAN_DOWNLEFT;
            else cursor = IDC_PAN_LEFT;
         }
         else if (dx > 0) {
            if (dy < 0) cursor = IDC_PAN_UPRIGHT;
            else if (dy > 0) cursor = IDC_PAN_DOWNRIGHT;
            else cursor = IDC_PAN_RIGHT;
         }
         else {
            if (dy < 0) cursor = IDC_PAN_UP;
            else if (dy > 0) cursor = IDC_PAN_DOWN;
            else cursor = IDC_PAN;
         }

         HCURSOR c=LoadCursor(theApp.m_hInstance,MAKEINTRESOURCE(cursor));
         SetCursor(c);
      }
      return;
   }
   
   CWnd::OnTimer(nIDEvent);
}

void CBrowserView::StartPanning()
{
	m_panning = 1;
	GetCursorPos(&m_panningPoint);
	SetCapture();
	SetTimer(0x1,20,NULL);

	SetFocus();
}

void CBrowserView::StopPanning()
{
   SetCursor(LoadCursor(NULL,IDC_ARROW));   // return to the normal cursor
	ReleaseCapture();
	KillTimer(0x1);
	m_panning = 0;
}

BOOL CBrowserView::PreTranslateMessage(MSG* pMsg)
{
  if(m_panning && (pMsg->message==WM_SETCURSOR || pMsg->message==WM_MOUSEMOVE))
    return TRUE;

  if(m_panning && (pMsg->message==WM_LBUTTONDOWN || pMsg->message==WM_RBUTTONDOWN || pMsg->message==WM_MOUSEWHEEL))
    StopPanning();

	return CWnd::PreTranslateMessage(pMsg);
}
