/*
*  Copyright (C) 2001 Jeff Doozan
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

*
*  Extended CToolBar class, sends WM_[L|W|R]BUTTONUP, WM_[L|W|R]BUTTONDOWN
*  and WM_[L|W|R]BUTTONHELD messages to the active window
*  The WM_[L|W|R]BUTTONHELD message is sent when a button is pressed for longer than 300ms
*

*/

#include "stdafx.h"
#include "mfcembed.h"
extern CMfcEmbedApp theApp;

#include "BrowserFrm.h"
#include "ToolBarEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LBUTTON_TIMER 100
#define MBUTTON_TIMER 200
#define RBUTTON_TIMER 300


CToolBarEx::CToolBarEx() {
}

CToolBarEx::~CToolBarEx() {
}


BEGIN_MESSAGE_MAP(CToolBarEx, CToolBar)
	//{{AFX_MSG_MAP(CToolBarEx)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
        ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
        ON_WM_MBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
        ON_WM_RBUTTONDBLCLK()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx message handlers



int CToolBarEx::GetButtonIDFromPoint(CPoint *point) {
	int count = GetToolBarCtrl().GetButtonCount();

	CRect buttonRect;
	
	for (int n = 0; n < count; n++) {
		GetItemRect(n, &buttonRect);
		if (buttonRect.PtInRect(*point)) {
			return n;
		}
	}
	return -1;
}


void CToolBarEx::OnLButtonDown(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		SetTimer(LBUTTON_TIMER + buttonID,300,NULL);

		GetParentFrame()->PostMessage(TB_LBUTTONDOWN, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_LBUTTONDOWN, 0, (LPARAM) m_hWnd);
	}
	
	CToolBar::OnLButtonDown(nFlags, point);
}

void CToolBarEx::OnLButtonUp(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		KillTimer(LBUTTON_TIMER + buttonID);

		GetParentFrame()->PostMessage(TB_LBUTTONUP, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_LBUTTONUP, 0, (LPARAM) m_hWnd);
	}

	CToolBar::OnLButtonUp(nFlags, point);
}

void CToolBarEx::OnLButtonDblClk(UINT nFlags, CPoint point) {

       int buttonID = GetButtonIDFromPoint(&point);
       if (buttonID > -1) {
               GetParentFrame()->PostMessage(TB_LBUTTONDBLCLK, GetItemID(buttonID), (LPARAM) m_hWnd);
       }
	else {
	  GetParentFrame()->PostMessage(TB_LBUTTONDBLCLK, 0, (LPARAM) m_hWnd);
	}
       
       CToolBar::OnLButtonDblClk(nFlags, point);
}

void CToolBarEx::OnMButtonDown(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		SetTimer(MBUTTON_TIMER + buttonID,300,NULL);

		GetParentFrame()->PostMessage(TB_MBUTTONDOWN, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_MBUTTONDOWN, 0, (LPARAM) m_hWnd);
	}
	
	CToolBar::OnMButtonDown(nFlags, point);
}

void CToolBarEx::OnMButtonUp(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		KillTimer(MBUTTON_TIMER + buttonID);

		GetParentFrame()->PostMessage(TB_MBUTTONUP, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_MBUTTONUP, 0, (LPARAM) m_hWnd);
	}

	CToolBar::OnMButtonUp(nFlags, point);
}

void CToolBarEx::OnMButtonDblClk(UINT nFlags, CPoint point) {

       int buttonID = GetButtonIDFromPoint(&point);
       if (buttonID > -1) {
               GetParentFrame()->PostMessage(TB_MBUTTONDBLCLK, GetItemID(buttonID), (LPARAM) m_hWnd);
       }
	else {
	  GetParentFrame()->PostMessage(TB_MBUTTONDBLCLK, 0, (LPARAM) m_hWnd);
	}
       
       CToolBar::OnMButtonDblClk(nFlags, point);
}

void CToolBarEx::OnRButtonDown(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		SetTimer(RBUTTON_TIMER + buttonID,300,NULL);

		GetParentFrame()->PostMessage(TB_RBUTTONDOWN, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_RBUTTONDOWN, 0, (LPARAM) m_hWnd);
	}
	
	CToolBar::OnRButtonDown(nFlags, point);
}

void CToolBarEx::OnRButtonUp(UINT nFlags, CPoint point) {

	int buttonID = GetButtonIDFromPoint(&point);
	if (buttonID > -1) {
		KillTimer(RBUTTON_TIMER + buttonID);

		GetParentFrame()->PostMessage(TB_RBUTTONUP, GetItemID(buttonID), (LPARAM) m_hWnd);
	}
	else {
	  GetParentFrame()->PostMessage(TB_RBUTTONUP, 0, (LPARAM) m_hWnd);
	}

	CToolBar::OnRButtonUp(nFlags, point);
}
 
void CToolBarEx::OnRButtonDblClk(UINT nFlags, CPoint point) {

       int buttonID = GetButtonIDFromPoint(&point);
       if (buttonID > -1) {
               GetParentFrame()->PostMessage(TB_RBUTTONDBLCLK, GetItemID(buttonID), (LPARAM) m_hWnd);
       }
	else {
	  GetParentFrame()->PostMessage(TB_RBUTTONDBLCLK, 0, (LPARAM) m_hWnd);
	}
       
       CToolBar::OnRButtonDblClk(nFlags, point);
}

void CToolBarEx::OnTimer(UINT nIDEvent) {

	// check that the timer is one of ours, it would be silly and inefficient
	// to initialize mainFrame and count on other timer ewents
	if (nIDEvent < 1000) {
		UINT count = GetToolBarCtrl().GetButtonCount();

		// Left button timer
		if ((nIDEvent >= LBUTTON_TIMER) && (nIDEvent < LBUTTON_TIMER + count)) {
			KillTimer(nIDEvent);
			GetParentFrame()->PostMessage(TB_LBUTTONHOLD, GetItemID(nIDEvent-LBUTTON_TIMER), 0);
		}

		// Middle button timer
		else if ((nIDEvent >= MBUTTON_TIMER) && (nIDEvent < MBUTTON_TIMER + count)) {
			KillTimer(nIDEvent);
			GetParentFrame()->PostMessage(TB_MBUTTONHOLD, GetItemID(nIDEvent-MBUTTON_TIMER), 0);
		}

		// Right button timer
		else if ((nIDEvent >= RBUTTON_TIMER) && (nIDEvent < RBUTTON_TIMER + count)) {
			KillTimer(nIDEvent);
			GetParentFrame()->PostMessage(TB_RBUTTONHOLD, GetItemID(nIDEvent-RBUTTON_TIMER), 0);
		}
	}
		
	CToolBar::OnTimer(nIDEvent);
}
