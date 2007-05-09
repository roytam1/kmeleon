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

#include "nsIDOMWindowInternal.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMEventTarget.h"
/*
** Middle button panning shit
*/
int CBrowserView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// Give the focus to the browser if it don't have it.
	if (!::IsChild(m_hWnd, ::GetFocus())) {
		m_pWindow->SetActive(TRUE);
		mpBrowserFrame->m_wndLastFocused = NULL;
	}

/*
   int id;
   id = theApp.accel.CheckMouse(message);

   if (message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN)
      mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);

   if (m_panning) {
	  if (message==WM_LBUTTONDOWN ||
		  message==WM_RBUTTONDOWN ||
		  message==WM_MBUTTONDOWN) {
		 StopPanning();
		 return MA_ACTIVATE;
	  }
   }
   else {
	  if (id) {
		 maccel_cmd = id;
		 maccel_key = message;
		 PostMessage(WM_COMMAND, (WPARAM)ID_MOUSE_ACTION, 0);
		 return MA_ACTIVATE;
	  }
	  else {
		 if (message==WM_MBUTTONDOWN) {
			StartPanning();
			return MA_ACTIVATE;
		 }
	  }
   }
*/
   return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}
/*
nsIDOMWindow *CBrowserView::FindDOMWindow(nsIDOMWindow *window, nsIDOMDocument *document) {
  nsCOMPtr<nsIDOMDocument> windoc;
  window->GetDocument(getter_AddRefs(windoc));
  if (windoc == document)
    return window;

  nsCOMPtr<nsIDOMWindowCollection> frameset;
  window->GetFrames(getter_AddRefs(frameset));

  if (frameset) {
    PRUint32 length;
    frameset->GetLength(&length);
    
    if (length) {
      for (unsigned i=0; i<length; i++) {
	nsCOMPtr<nsIDOMWindow> tmpwin;
	frameset->Item(i, getter_AddRefs(tmpwin));
	tmpwin = FindDOMWindow(tmpwin, document);
	if (tmpwin)
	  return tmpwin;
      }
    }
  }

  return NULL;
}
*/
void CBrowserView::OnTimer(UINT nIDEvent)
{
   switch(nIDEvent){
   case 0x1:
      if(m_panning) {
         //if (!s) return;

         POINT p;
         GetCursorPos(&p);

         //int scroll_x,scroll_y;

         //s->GetScrollX(&scroll_x);
         //s->GetScrollY(&scroll_y);

         int dx = (p.x-m_panningPoint.x)/10;
         int dy = (p.y-m_panningPoint.y)/10;
		 m_pWindow->ScrollBy(dx, dy);
/*
         if(dy!=0) {
            if(dy>0)	scroll_y += dy*dy; 
            else scroll_y -= dy*dy;
            s->ScrollTo(scroll_x,scroll_y);
         }

         if(dx!=0) {
            if(dx>0)	scroll_x += dx*dx;
            else scroll_x -= dx*dx;
            s->ScrollTo(scroll_x,scroll_y);
         }*/
         /* cursor flickering : https://bugzilla.mozilla.org/show_bug.cgi?id=316352 */
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

void CBrowserView::StartPanning(BOOL accel)
{
   if (accel) maccel_pan = 1;

   m_panning = 1;
   m_panningQuick = theApp.preferences.GetBool("kmeleon.general.quickAutoscroll", FALSE);
   GetCursorPos(&m_panningPoint);
/*
   nsCOMPtr<nsIWebBrowser> browser = m_pWindow->GetWebBrowser();
   browser->GetContentDOMWindow(getter_AddRefs(s)); 
   if(!s) return;

   nsCOMPtr<nsIDOMWindowCollection> frameset;
   s->GetFrames(getter_AddRefs(frameset));

   if (frameset) {
	   PRUint32 length;
	   frameset->GetLength(&length);

	   if (length) {
		   // get the DOMNode at the point
		 //  nsCOMPtr<nsIDOMNode> aNode;
		 //  GetNodeAtPoint(m_panningPoint.x, m_panningPoint.y, TRUE);
		   if (m_contextNode) {
			   nsCOMPtr<nsIDOMDocument> ownerdoc;
			   m_contextNode->GetOwnerDocument(getter_AddRefs(ownerdoc));

			   if (ownerdoc)
				   s = FindDOMWindow(s, ownerdoc);
		   }

		   if(!s) return;
	   }
   }
*/
   SetCapture();
   SetTimer(0x1,20,NULL);
   //SetFocus();
}

void CBrowserView::StopPanning()
{
   SetCursor(LoadCursor(NULL,IDC_ARROW));   // return to the normal cursor
   ReleaseCapture();
   KillTimer(0x1);
   m_panning = 0;
   maccel_pan = 0;
}

BOOL CBrowserView::PreTranslateMessage(MSG* pMsg)
{/*
  if(m_panning && (pMsg->message==WM_SETCURSOR || pMsg->message==WM_MOUSEMOVE))
    return 1;
*/
  if(m_panning && (pMsg->message==WM_LBUTTONDOWN || (pMsg->message==WM_MBUTTONDOWN && maccel_pan) || (pMsg->message==WM_MBUTTONUP && m_panningQuick) || pMsg->message==WM_RBUTTONDOWN || pMsg->message==WM_MOUSEWHEEL))
    StopPanning();

	return CWnd::PreTranslateMessage(pMsg);
}
