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
*  Extended CToolBar class, processes WM_LBUTTONDOWN and WM_LBUTTONUP messages
*  if a button is pressed for longer than 300ms, the event WM_LBUTTONHOLD is
*  sent to the BrowserView window.
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

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx

CToolBarEx::CToolBarEx()
{
}

CToolBarEx::~CToolBarEx()
{
}


BEGIN_MESSAGE_MAP(CToolBarEx, CToolBar)
	//{{AFX_MSG_MAP(CToolBarEx)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx message handlers


void CToolBarEx::OnLButtonDown(UINT nFlags, CPoint point) {

	// determine which button is being clicked

	int count = GetToolBarCtrl().GetButtonCount();

	CRect buttonRect;

	for (int n = 0; n < count; n++) {
		GetItemRect(n, &buttonRect);
		if (buttonRect.PtInRect(point)) {
			SetTimer(n+1,300,NULL);
			break;
		}
	}
	
	CToolBar::OnLButtonDown(nFlags, point);
}

void CToolBarEx::OnLButtonUp(UINT nFlags, CPoint point) {

	int count = GetToolBarCtrl().GetButtonCount();

	CRect buttonRect;
	
	for (int n = 0; n < count; n++) {
		GetItemRect(n, &buttonRect);
		if (buttonRect.PtInRect(point)) {
			KillTimer(n+1);
			break;
		}
	}

	CToolBar::OnLButtonUp(nFlags, point);
}

void CToolBarEx::OnTimer(UINT nIDEvent) {

	if (nIDEvent <= (UINT) GetToolBarCtrl().GetButtonCount()) {
		KillTimer(nIDEvent);

		CBrowserFrame *mainFrame = (CBrowserFrame *)theApp.m_pMainWnd->GetActiveWindow();
		mainFrame->m_wndBrowserView.PostMessage(WM_LBUTTONHOLD, GetItemID(nIDEvent-1), 0);
	}

	CToolBar::OnTimer(nIDEvent);
}
