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
         nsCOMPtr<nsIScrollable> s(do_QueryInterface(mWebBrowser));
         
         if(!s) return;

         POINT p;
         GetCursorPos(&p);

         int scroll_x,scroll_y;

         s->GetCurScrollPos(s->ScrollOrientation_X,&scroll_x);
         s->GetCurScrollPos(s->ScrollOrientation_Y,&scroll_y);

         if(abs(p.y-m_panningPoint.y)>4){
            if(p.y>m_panningPoint.y)	scroll_y+=(p.y-m_panningPoint.y)*2;
            else scroll_y-=(m_panningPoint.y-p.y)*2;
         }

         if(abs(p.x-m_panningPoint.x)>4){
            if(p.x>m_panningPoint.x)	scroll_x+=(p.x-m_panningPoint.x)*2;
            else scroll_x-=(m_panningPoint.x-p.x)*2;
         }

         s->SetCurScrollPosEx(scroll_x,scroll_y);

         int cursorx=p.x-m_panningPoint.x,cursory=p.y-m_panningPoint.y;
         int cursor=IDC_PAN;
         if(cursory<-4) cursor=IDC_PAN_UP;
         if(cursory>4) cursor=IDC_PAN_DOWN;
         if(cursorx<-4)
         {
            if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPLEFT;
            else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNLEFT;
            else cursor=IDC_PAN_LEFT;
         }
         if(cursorx>4)
         {
            if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPRIGHT;
            else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNRIGHT;
            else cursor=IDC_PAN_RIGHT;
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
	SetTimer(0x1,10,NULL);

	SetFocus();
}

void CBrowserView::StopPanning()
{
	ReleaseCapture();
	KillTimer(0x1);
	m_panning = 0;
}

BOOL CBrowserView::PreTranslateMessage(MSG* pMsg)
{
  if(m_panning && (pMsg->message==WM_SETCURSOR || pMsg->message==WM_MOUSEMOVE))
    return TRUE;

  if(m_panning && (pMsg->message==WM_LBUTTONDOWN || pMsg->message==WM_RBUTTONDOWN))
    StopPanning();
	
	return CWnd::PreTranslateMessage(pMsg);
}
