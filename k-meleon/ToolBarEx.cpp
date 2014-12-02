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
#include "ToolBarEx.h"
#include "KmToolbar.h"
#include "MfcEmbed.h" 
#include "VisualStylesXP.h"

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
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xffff, OnTbnGetDispInfo) 
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx message handlers

void CToolBarEx::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{	
	*pResult = CDRF_DODEFAULT;
	KmToolbar* kmtoolbar = (KmToolbar*)GetProp(GetSafeHwnd(), L"KmToolbar");
	if (!kmtoolbar) return;

	LPNMTBCUSTOMDRAW pNMCD = reinterpret_cast<LPNMTBCUSTOMDRAW>(pNMHDR);
	switch(pNMCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		break;
	
	case CDDS_POSTPAINT:
		break; 
		

	case CDDS_ITEMPOSTPAINT:
		break;

	case CDDS_ITEMPREPAINT: {
		HTHEME hTheme = NULL;
		if (g_xpStyle.IsThemeActive() && g_xpStyle.IsAppThemed())// && (SendMessage(0x0129, 0, 0) & 0x4))
			hTheme = g_xpStyle.OpenThemeData (m_hWnd, L"TOOLBAR");
		
		CDC *pDC = CDC::FromHandle(pNMCD->nmcd.hdc);
		pDC->SetBkMode(TRANSPARENT);
		pDC->SelectObject(GetFont());
		int index = CommandToIndex(pNMCD->nmcd.dwItemSpec);
		UINT style = GetButtonStyle(index);

		CRect contentRect(pNMCD->nmcd.rc); 
		int stateId = TS_NORMAL;

		if (hTheme) {

			if (pNMCD->nmcd.uItemState & CDIS_DISABLED)
				stateId = TS_DISABLED;
			else if (pNMCD->nmcd.uItemState & CDIS_SELECTED)
				stateId = TS_PRESSED;		
			else if (pNMCD->nmcd.uItemState & CDIS_HOT && pNMCD->nmcd.uItemState & CDIS_CHECKED)
				stateId = TS_HOTCHECKED; 
			else if (pNMCD->nmcd.uItemState & CDIS_CHECKED)
				stateId = TS_CHECKED;
			else if (pNMCD->nmcd.uItemState & CDIS_HOT)
				stateId = TS_HOT;

			g_xpStyle.DrawThemeBackground(hTheme, pDC->m_hDC, TP_BUTTON, stateId, &pNMCD->nmcd.rc, NULL);
			g_xpStyle.GetThemeBackgroundContentRect(hTheme, pDC->m_hDC, TP_BUTTON, stateId, &pNMCD->nmcd.rc, &contentRect);
		}
		else
		{	
			UINT nState = DFCS_FLAT;
			//CBrush brBackground(GetSysColor(COLOR_BTNFACE));
			//pDC->FillRect(&pNMCD->nmcd.rc, &brBackground);
			if (pNMCD->nmcd.uItemState & CDIS_DISABLED)
				nState = DFCS_INACTIVE;
			else if (pNMCD->nmcd.uItemState & CDIS_SELECTED)
				nState = DFCS_PUSHED;		
			else if(pNMCD->nmcd.uItemState & CDIS_HOT && pNMCD->nmcd.uItemState & CDIS_CHECKED)
				nState = DFCS_CHECKED | DFCS_HOT; 
			else if (pNMCD->nmcd.uItemState & CDIS_CHECKED)
				nState = DFCS_CHECKED;
			else if (pNMCD->nmcd.uItemState & CDIS_HOT)
				nState = DFCS_HOT;

			if (nState == DFCS_HOT) {
				COLORREF clr = ::GetSysColor(COLOR_BTNHIGHLIGHT);
				COLORREF clr2 = ::GetSysColor(COLOR_BTNSHADOW);
				pDC->Draw3dRect(&pNMCD->nmcd.rc, clr, clr2);
			} else if (nState == DFCS_PUSHED) {
				COLORREF clr = ::GetSysColor(COLOR_BTNHIGHLIGHT);
				COLORREF clr2 = ::GetSysColor(COLOR_BTNSHADOW);
				pDC->Draw3dRect(&pNMCD->nmcd.rc, clr2, clr);
			} else if (nState & DFCS_CHECKED) {
					nState |= DFCS_TRANSPARENT | DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;
				pDC->DrawFrameControl(&pNMCD->nmcd.rc, DFC_BUTTON, nState);
			}			
		}
		
		UINT textFlag = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX | DT_WORD_ELLIPSIS;
		
		CString text;
		KmButton* kmbutton = kmtoolbar->GetButton(pNMCD->nmcd.dwItemSpec);
		if (kmbutton) text = kmbutton->mLabel;		

		int image = I_IMAGENONE;
		if (!kmtoolbar->mCold.m_hImageList) {
			image = theApp.skin.GetIconIndex(pNMCD->nmcd.dwItemSpec);
		} else {
			TBBUTTONINFO bi;
			bi.cbSize = sizeof(TBBUTTONINFO);
			bi.dwMask = TBIF_IMAGE;
			GetToolBarCtrl().GetButtonInfo(pNMCD->nmcd.dwItemSpec, &bi);
			image = bi.iImage;
		}

		CImageList* imageList;
		if (pNMCD->nmcd.uItemState & CDIS_DISABLED)
			imageList = GetToolBarCtrl().GetDisabledImageList();
		else if (pNMCD->nmcd.uItemState & CDIS_SELECTED)
			imageList = GetToolBarCtrl().GetHotImageList();
		else if(pNMCD->nmcd.uItemState & CDIS_HOT && pNMCD->nmcd.uItemState & CDIS_CHECKED)
			imageList = GetToolBarCtrl().GetHotImageList();
		else if (pNMCD->nmcd.uItemState & CDIS_CHECKED)
			imageList = GetToolBarCtrl().GetHotImageList();
		else if (pNMCD->nmcd.uItemState & CDIS_HOT)
			imageList = GetToolBarCtrl().GetHotImageList();
		else
			imageList = GetToolBarCtrl().GetImageList();
	
		CPoint imagePoint;
		CRect textRect;

		int hp, vp;
		GetToolBarCtrl().GetPadding(hp, vp);
		hp /= 2;

		int btMargin = 0;
		int iconPadding = 4;

		if (pNMCD->nmcd.uItemState & CDIS_SELECTED || pNMCD->nmcd.uItemState & CDIS_CHECKED)
			contentRect.OffsetRect(1,1);

		IMAGEINFO ii;
		ii.rcImage = CRect(0, 0, 16, 16);
		if (imageList)
		{
			imageList->GetImageInfo(image, &ii);
			if (ii.hbmMask) DeleteObject(ii.hbmMask);
			if (ii.hbmImage) DeleteObject(ii.hbmImage);

			imagePoint.y = contentRect.top + (contentRect.Height() - ii.rcImage.bottom + ii.rcImage.top)/2;
			imagePoint.x = hp + contentRect.left;
			imageList->Draw(pDC, image, imagePoint, ILD_TRANSPARENT);
			contentRect.left += 2*hp + ii.rcImage.right - ii.rcImage.left;
		}		
		
		if (text.GetLength()) {
			if (hTheme) {
				USES_CONVERSION;
				g_xpStyle.DrawThemeText(hTheme, pDC->m_hDC, TP_BUTTON, stateId,
					T2CW(text), text.GetLength(), textFlag, 0, &contentRect);
			}
			else {
				pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
				pDC->SetBkColor(::GetSysColor(COLOR_BTNFACE));
				pDC->DrawText(text, -1, &contentRect, textFlag);
			}
		}

		g_xpStyle.CloseThemeData(hTheme);
		*pResult = CDRF_SKIPDEFAULT;
		break;
	}
	}
	if (theApp.preferences.GetBool("kmeleon.display.toolbars_alt", 0));
		*pResult |= CDRF_NOTIFYITEMDRAW;
}

BOOL CToolBarEx::OnTbnGetDispInfo(UINT, NMHDR * pNotifyStruct, LRESULT* pResult)
{
	static CString text;
	*pResult = 0;
	KmToolbar* toolbar = (KmToolbar*)GetProp(GetSafeHwnd(), _T("kmToolbar"));
	if (!toolbar) return FALSE;
	
	KmButton* button;
	if (!(button = toolbar->GetButton(pNotifyStruct->idFrom)))
		return FALSE;

	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNotifyStruct;
	if (button->mTooltip.GetLength()) {
		pTTT->lpszText = (LPTSTR)theApp.lang.Translate(button->mTooltip);
	}
	else {
		if (button->mMenuName) {
			if (!text.LoadString(pNotifyStruct->idFrom))
				return FALSE;
			text += L"\n";
			text.AppendFormat(IDS_MORE_OPTIONS);
			pTTT->lpszText = text.GetBuffer();
		} else
			pTTT->lpszText = MAKEINTRESOURCE(pNotifyStruct->idFrom);
	}

	return TRUE;
}

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
