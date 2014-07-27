/*
*  Copyright (C) 2005 Dorian Boissonnade
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
*
*/

#include "stdafx.h"
#include "BrowserFrmTab.h" // XXX
#include "TabReBar.h"
#include "mfcembed.h"
#include "VisualStylesXP.h"

DROPEFFECT CTBOleDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
					  DWORD dwKeyState, CPoint point)
{
	if (pDataObject->IsDataAvailable(cfTabPt, NULL)) {
		int destIndex = ((CTabReBar*)pWnd)->GetButtonIDFromPoint(point);
		if (destIndex>=0)
			return DROPEFFECT_MOVE;
		else
			return DROPEFFECT_LINK;
	}
	if (pDataObject->IsDataAvailable(cfShellURL, NULL))
		return DROPEFFECT_LINK;
	return DROPEFFECT_NONE;
}

BOOL CTBOleDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
									  DROPEFFECT dropEffect, CPoint point)
{
	if (!pWnd) return FALSE;

	// THIS CODE SUCKS ********
	
	CTabReBar* tabbar = (CTabReBar*)pWnd;
	int destIndex = tabbar->GetButtonIDFromPoint(point);
	
	CBrowserTab* tab = NULL;
	HGLOBAL hTab = pDataObject->GetGlobalData(cfTabPt, NULL);
	if (hTab) {
		tab = (CBrowserTab*)(*(DWORD*)GlobalLock(hTab));
		if (!tab->IsKindOf(RUNTIME_CLASS(CBrowserTab)) ||
			tab->GetParentFrame() != pWnd->GetParentFrame()) 
			tab = NULL;

		GlobalUnlock(hTab);
		GlobalFree(hTab);
	}

	if (destIndex>=0 && tab) {
		int index = tabbar->GetButtonIDFromData((DWORD)tab);

		TBBUTTON button = {0};
		tabbar->GetToolBarCtrl().GetButton(index, &button);
		button.fsState &= ~TBSTATE_PRESSED;

		if (destIndex == index)
			return TRUE;
		
		TBBUTTON buttonDest = {0};
		tabbar->GetToolBarCtrl().GetButton(destIndex, &buttonDest);
		tabbar->GetToolBarCtrl().DeleteButton(index);
		tabbar->GetToolBarCtrl().InsertButton(destIndex,&button);
		theApp.plugins.SendMessage("*", "*", "MoveTab", (long)tab->GetSafeHwnd(), (long)((CBrowserTab*)(buttonDest.dwData))->GetSafeHwnd());
		return TRUE;

	} else {
		HGLOBAL hUrl = pDataObject->GetGlobalData(cfShellURL, NULL);
		if (!hUrl) return FALSE;
		char* url = (char*)GlobalLock(hUrl);

		if (destIndex>=0) {
			TBBUTTON button;
			tabbar->GetToolBarCtrl().GetButton(destIndex, &button);
			CBrowserTab* tab = (CBrowserTab*)button.dwData;
			USES_CONVERSION;
			tab->OpenURL(A2CT(url));
		} else {
			pWnd->GetParentFrame()->SendMessage(WM_OPENTAB, (WPARAM)url, 1);
		}
		
		GlobalUnlock(hUrl);
		GlobalFree(hUrl);
		return TRUE;
	}
	return FALSE;
	
}


// CTabReBar

IMPLEMENT_DYNAMIC(CTabReBar, CToolBar)

CTabReBar::CTabReBar()
{
	bRebarEnabled  =   1;
	bButtonNumbers =   0;
	mDragItem = -1;
	m_wndParent = NULL;	
	mBottomBar = FALSE;

	CString prefPos = theApp.preferences.GetString(PREFERENCE_REBAR_POSITION, _T("band"));
	if (prefPos.CompareNoCase(_T("vtop")) == 0) mPosBar = POSITION_VTOP;
	else if (prefPos.CompareNoCase(_T("top")) == 0) mPosBar = POSITION_TOP;
	else if (prefPos.CompareNoCase(_T("left")) == 0) mPosBar = POSITION_LEFT;
	else if (prefPos.CompareNoCase(_T("right")) == 0) mPosBar = POSITION_RIGHT;
	else if (prefPos.CompareNoCase(_T("bottom")) == 0) mPosBar = POSITION_BOTTOM;
	else mPosBar = POSITION_BAND;
	
	if (mPosBar == POSITION_BOTTOM) mBottomBar = TRUE;

	//mFixedBar = theApp.preferences.GetBool(PREFERENCE_REBAR_FIXED, FALSE);
	mFixedBar = mPosBar != POSITION_BAND; 
	
	mChevron = FALSE;
	mTemp = NULL;

	theApp.preferences.GetString(PREFERENCE_REBAR_TITLE, szTitle, _T(""));
	mButtonStyle = theApp.preferences.GetInt(PREFERENCE_BUTTON_STYLE, 2);
}

CTabReBar::~CTabReBar()
{
	if (mTemp)
		delete mTemp;
}

BOOL CTabReBar::Create(CReBarEx* rebar, UINT idwnd)
{
		if (!CreateEx(rebar->GetParentFrame(), CCS_BOTTOM | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
			WS_CHILD|WS_VISIBLE|(mBottomBar ? CBRS_ALIGN_BOTTOM : CBRS_ALIGN_TOP) ,CRect(0,0,0,0),idwnd))
			return FALSE;
		//ModifyStyle(0, CCS_ADJUSTABLE);
		m_wndParent = rebar;

		if (!mFixedBar && !mPosBar) {
			rebar->RegisterBand(m_hWnd, _T("Tabs"), false);
			rebar->AddBar(this, szTitle, 0, RBBS_USECHEVRON | RBBS_FIXEDBMP );

			REBARBANDINFO rbBand = {0};
			rbBand.cbSize = sizeof(REBARBANDINFO);  
			rbBand.fMask  =  RBBIM_BACKGROUND;// RBBIM_SIZE |RBBIM_CHILDSIZE;
			rbBand.hbmBack = theApp.preferences.bToolbarBackground ? (HBITMAP)CBrowserFrame::m_bmpBack : NULL;

			// Get the width & height of the toolbar.
			// SIZE size;
			// GetToolBarCtrl().GetMaxSize(&size);

			//rbBand.cxMinChild = 0;
			//rbBand.cyMinChild = size.cy + 5;
			//rbBand.cyIntegral = 1;
			//rbBand.cyMaxChild = rbBand.cyMinChild * 2;

			int iband = rebar->FindByName(_T("Tabs"));
			rebar->GetReBarCtrl().SetBandInfo(iband, &rbBand);			
		} else {
			mTemp = new CReBarEx();
			mTemp->Create(GetParentFrame(), RBS_BANDBORDERS, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|(mBottomBar ? CBRS_ALIGN_BOTTOM  : CBRS_TOP));
			mTemp->AddBar(this, szTitle, NULL,  RBBS_USECHEVRON | RBBS_NOGRIPPER);
			mTemp->SetWindowText(_T("TabsBar"));
			
			if (mPosBar == POSITION_VTOP) {
				rebar->SetWindowPos(mTemp ,0,0,0,0,SWP_NOMOVE);
				//rebar->SetBarStyle(rebar->m_dwStyle | CBRS_BORDER_BOTTOM);
			}
			else  if (mPosBar == POSITION_TOP) {
				mTemp->SetNeedSeparator(true);
				if (mPosBar != POSITION_TOP || !theApp.preferences.bAutoHideTabControl)
					rebar->SetNeedSeparator(false);
			} else  if (mPosBar == POSITION_BOTTOM) {
				mTemp->SetNeedSeparator(true);
			}
		}

		mDropTarget.Register(this);
		return TRUE;
}

BOOL CTabReBar::Init(CReBarEx* rebar)
{
	REBARBANDINFO rbi;
	rbi.cbSize = sizeof(REBARBANDINFO);

	// Catch the tab buttons if fixed
	if (mFixedBar)
	{
		rbi.fMask  = RBBIM_CHILD;
		int idx = rebar->FindByName(_T("Tab/&Window Buttons"));
		if (idx>=0) {
			rebar->GetReBarCtrl().GetBandInfo(idx, &rbi);
			CToolBar* toolbar = (CToolBar*)CWnd::FromHandle(rbi.hwndChild);
			if (toolbar) {
				mTemp->AddBar(toolbar, _T(""), NULL, RBBS_NOGRIPPER);
				rebar->UnregisterBand(_T("Tab/&Window Buttons"));
				rebar->GetReBarCtrl().DeleteBand(idx);
				mTemp->GetReBarCtrl().ShowBand(1);
				KmMenu* menu = theApp.menus.GetKMenu(_T("@Toolbars"));
				if (menu) menu->Invalidate();
			}
		}

		CRect rectDesktop;
		::GetWindowRect(::GetDesktopWindow(), rectDesktop);
		rbi.fMask = RBBIM_SIZE;
		rbi.cx = rectDesktop.Width();
		mTemp->GetReBarCtrl().SetBandInfo(0, &rbi);

		if (!mBottomBar && theApp.preferences.bToolbarBackground) {
			rbi.hbmBack = CBrowserFrame::m_bmpBack;
			rbi.fMask = RBBIM_BACKGROUND;
			mTemp->GetReBarCtrl().SetBandInfo(0, &rbi);
			mTemp->GetReBarCtrl().SetBandInfo(1, &rbi);
		}
	}

	// Set the height of the bar

	SIZE size;
	GetToolBarCtrl().GetMaxSize(&size);

	if (mFixedBar) {
		SIZE sizeButtons;
		rbi.fMask = RBBIM_CHILD;
		mTemp->GetReBarCtrl().GetBandInfo(1, &rbi);
		if (::SendMessage(rbi.hwndChild, TB_GETMAXSIZE, 0, (LPARAM)&sizeButtons) != 0)
			size.cy = std::max(size.cy, sizeButtons.cy);
	}

	rbi.fMask  = RBBIM_CHILDSIZE; 
	rbi.cxMinChild = 0;
	rbi.cyMinChild = rbi.cyChild = rbi.cyMaxChild = size.cy;
	if (!mFixedBar && !mPosBar) {
		int iband = m_wndParent->FindByName(_T("Tabs"));
		m_wndParent->GetReBarCtrl().SetBandInfo(iband, &rbi);
	} 
	else
		mTemp->GetReBarCtrl().SetBandInfo(0, &rbi);

	if (mBottomBar)
		mTemp->SetWindowPos(&(((CBrowserFrame*)GetParentFrame())->m_wndStatusBar),0,0,0,0,SWP_NOMOVE);

	return TRUE;
}

void CTabReBar::UpdateButtonsSize(bool forceUpdate)
{
	int count = this->GetToolBarCtrl().GetButtonCount();
	if (!count) return;

	int nButtonMinWidth = theApp.preferences.GetInt(PREFERENCE_BUTTON_MINWIDTH, 10);
	int nButtonMaxWidth = theApp.preferences.GetInt(PREFERENCE_BUTTON_MAXWIDTH, 35);
	
	// If the sizes are 0 the toolbar get crazy so avoid it!
	if (nButtonMinWidth>nButtonMaxWidth) nButtonMaxWidth = nButtonMinWidth;
	if (nButtonMaxWidth < 10) nButtonMaxWidth = nButtonMinWidth = 10;

	CDC dc;
	dc.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	int nHSize = dc.GetDeviceCaps(HORZSIZE);
	int nHRes = dc.GetDeviceCaps(HORZRES);
	dc.DeleteDC();
	if (nHSize == 0) nHSize = nHRes;

	int nMinWidth = nButtonMinWidth * nHRes / nHSize;
	int nMaxWidth = nButtonMaxWidth * nHRes / nHSize;

	RECT rect;
	GetWindowRect(&rect);
	mChevron = FALSE;

	int width = rect.right - rect.left - 2;
	int buttonWidth = width / count;
	if (buttonWidth > nMaxWidth)
		buttonWidth = nMaxWidth;
	else if (buttonWidth < nMinWidth) {
		mChevron = TRUE;
		buttonWidth = nMinWidth;
		if (width>nMinWidth)
			buttonWidth = width / (width/nMinWidth);
	}
	
	int w = LOWORD(GetToolBarCtrl().GetButtonSize());
	if (!forceUpdate && w == buttonWidth) return;
	GetToolBarCtrl().SetButtonWidth(buttonWidth, buttonWidth);
	w = LOWORD(GetToolBarCtrl().GetButtonSize());
	if (w != buttonWidth)
		GetToolBarCtrl().SetButtonWidth(--buttonWidth, buttonWidth);

	SIZE size;
	GetToolBarCtrl().GetMaxSize(&size);

	// Set the ideal size for chevron
	REBARBANDINFO rb;
	rb.cbSize = sizeof(REBARBANDINFO);
	rb.fMask  = RBBIM_IDEALSIZE; 
	rb.cxIdeal = buttonWidth*count;//size.cx;

	/* Stupid Vista+ Fix*/
	/*int static ignoreSize = 1;
	if (ignoreSize>0) {
		ignoreSize--;
		return;
	}
	ignoreSize = 1;*/

	if (!mFixedBar && !mPosBar) {		
		int iband = m_wndParent->FindByName(_T("Tabs"));
		m_wndParent->GetReBarCtrl().SetBandInfo(iband, &rb);
	} 
	else {
		mTemp->GetReBarCtrl().SetBandInfo(0, &rb);
		mTemp->GetReBarCtrl().MaximizeBand(0);
	}
}

void CTabReBar::UpdateVisibility(BOOL canHide)
{
	if (mFixedBar || mPosBar)
	{

		if (GetToolBarCtrl().GetButtonCount()>1) {
			
			mTemp->ShowWindow(SW_SHOW);//GetReBarCtrl().ShowBand(0, TRUE);
			if (mPosBar == POSITION_TOP) m_wndParent->SetNeedSeparator(false);
			if (mBottomBar && !mTemp->IsVisible()) mTemp->SetWindowPos(&(((CBrowserFrame*)GetParentFrame())->m_wndStatusBar),0,0,0,0,SWP_NOMOVE);
		}
		else if (theApp.preferences.bAutoHideTabControl) {
			if (mPosBar == POSITION_TOP) m_wndParent->SetNeedSeparator(true);
			mTemp->ShowWindow(SW_HIDE);
		}

				//mTemp->GetReBarCtrl().ShowBand(0, FALSE);
		GetParentFrame()->RecalcLayout();
/*
		mTemp->GetReBarCtrl().ShowBand(0, TRUE);
		SIZE s;
		CToolBarCtrl& tc = GetToolBarCtrl();

		if (tc.GetButtonCount()>1)
		{
			if (!tc.IsWindowVisible()) tc.ShowWindow(SW_SHOW);
			tc.GetMaxSize(&s);
			tc.SetButtonSize(CSize(24, s.cy));
			tc.SetWindowPos(&(((CBrowserFrame*)GetParentFrame())->m_wndStatusBar),0,0,0,0,SWP_NOMOVE);
		}
		else if (theApp.preferences.bAutoHideTabControl)
			tc.ShowWindow(SW_HIDE);
		GetParentFrame()->RecalcLayout();*/
	}
	else
	{
		ASSERT(m_wndParent);
		int index = m_wndParent->FindByName(_T("Tabs"));

		if (GetToolBarCtrl().GetButtonCount()>1)
			m_wndParent->GetReBarCtrl().ShowBand(index, TRUE);
		else
			if (canHide && theApp.preferences.bAutoHideTabControl)
				m_wndParent->GetReBarCtrl().ShowBand(index, FALSE);
	}
}

LONG CTabReBar::InsertItem(int nItem, int idCommand, LPCTSTR lpszItem, DWORD data, int nImage)
{
	// Add the button to the toolbar.
	TBBUTTON button = {0};
	button.iBitmap = theApp.favicons.GetDefaultIcon();
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_CHECKGROUP;
	button.idCommand = idCommand;
	button.dwData = data;
	button.iString = -1;
	GetToolBarCtrl().InsertButton(nItem,&button);

	SetButtonText(nItem, lpszItem);

	// Set buttons size & ideal size
	UpdateButtonsSize(true);
	UpdateVisibility(nItem!=0);

	return 1;
}

void CTabReBar::RefreshFavIcon()
{

}

BOOL CTabReBar::SetItemText(int idCommand, LPCTSTR lpszItem)
{
	//XXX
	CString str;
	GetItemText(idCommand, str);
	if (str.Compare(lpszItem) == 0) 
		return TRUE;

	int i = FindById(idCommand);
	if (i==-1) return FALSE;
	
	return SetButtonText(i, lpszItem);
}

BOOL CTabReBar::GetItemText(int idCommand, CString& str)
{
	int i = FindById(idCommand);
	if (i==-1) return FALSE;

	GetButtonText(i, str);
	return TRUE;
}

BOOL CTabReBar::SetItemImage(int idCommand, int nImage)
{
	//XXX
	if (GetItemImage(idCommand) == nImage)
		return TRUE;

	TBBUTTONINFO bi;
	bi.cbSize = sizeof(TBBUTTONINFO);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = nImage;
	return GetToolBarCtrl().SetButtonInfo(idCommand, &bi);	
}

int CTabReBar::GetItemImage(int idCommand)
{
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(TBBUTTONINFO);
	bi.dwMask = TBIF_IMAGE;
	GetToolBarCtrl().GetButtonInfo(idCommand, &bi);
	return bi.iImage;
}

int CTabReBar::GetButtonIDFromData(DWORD data) {
	
	int count = GetToolBarCtrl().GetButtonCount();

	CRect buttonRect;
	TBBUTTON button;
	for (int n = 0; n < count; n++) {
		GetToolBarCtrl().GetButton(n, &button);
		if (button.dwData == data)
			return n;
	}
	return -1;
}

int CTabReBar::GetButtonIDFromPoint(POINT point) {
	int count = GetToolBarCtrl().GetButtonCount();

	CRect buttonRect;
	
	for (int n = 0; n < count; n++) {
		GetItemRect(n, &buttonRect);
		if (buttonRect.PtInRect(point)) {
			return n;
		}
	}
	return -1;
}

BOOL CTabReBar::SetItemCommandID(int idCommand, int newidCommand)
{
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(TBBUTTONINFO);
	bi.dwMask = TBIF_COMMAND;
	bi.idCommand = newidCommand;
	return GetToolBarCtrl().SetButtonInfo(idCommand, &bi);
}

int CTabReBar::GetNextItem(int idCommand, bool strict)
{
	int index = FindById(idCommand);
	if (index>=0)
	{
		if (index<GetToolBarCtrl().GetButtonCount()-1)
			index++;
		else
			if (strict)
				index = 0;
			else
				return GetPreviousItem(idCommand);
		return GetItemID(index);
	}
	return 0;
}

int CTabReBar::GetPreviousItem(int idCommand, bool strict)
{
	int index = FindById(idCommand);
	if (index>0){
		index--;
		return GetItemID(index);
	} else if (index==0) 
		if (strict)
			return GetItemID(GetToolBarCtrl().GetButtonCount()-1);
		else
			return GetNextItem(idCommand);
	return 0;
}
/*
UINT CTabReBar::GetItemCommandID(int index)
{
	UINT nID=0, nStyle;int iImage;
	GetButtonInfo(index,nID,nStyle,iImage);
	return nID;
}*/


BOOL CTabReBar::DeleteItem(int idCommand)
{
	int i = FindById(idCommand);
	if (i==-1) return FALSE;

	BOOL r = GetToolBarCtrl().DeleteButton(i);
	UpdateButtonsSize(true);
	UpdateVisibility();

	mDragItem = -1;

	return r;
}

int CTabReBar::FindByData(DWORD_PTR data)
{
	bool found = false;
	
	int iCount, iButtonCount = GetToolBarCtrl().GetButtonCount();
	for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
	{
		TBBUTTON button;
		GetToolBarCtrl().GetButton(iCount, &button);
		if ( button.dwData == data) {found=true; break;}
	}

	if (!found) return -1;
	return iCount;
}

int CTabReBar::FindById(int idCommand)
{
	bool found = false;
	int iCount, iButtonCount = GetToolBarCtrl().GetButtonCount();
	for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
	{
		if ( GetItemID(iCount) == idCommand) {found=true; break;}
	}

	if (!found) return -1;
	return iCount;
}

BOOL CTabReBar::SelectItem(int idCommand)
{
	// Manual selecting kill the GROUP style ...
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(TBBUTTONINFO);
	bi.dwMask = TBIF_STATE;
	bi.fsState = TBSTATE_ENABLED | TBSTATE_CHECKED ;
	return GetToolBarCtrl().SetButtonInfo(idCommand, &bi);
}
/*
BOOL CTabReBar::SetItemInfo(int idCommand, LPTSTR lpszItem, int nImage)
{
	bool found = false;
	int iCount, iButtonCount = m_wndToolBar.GetToolBarCtrl().GetButtonCount();
	for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
	{
		if ( m_wndToolBar.GetItemID(iCount) == idCommand) {found=true; break;}
	}

	if (!found) return FALSE;

	TBBUTTONINFO bi;
	bi.cbSize = sizeof(TBBUTTONINFO);
	bi.dwMask = TBIF_TEXT | TBIF_IMAGE;
	bi.cchText = _tcslen(lpszItem);
	bi.pszText = lpszItem;
	bi.iImage = nImage;
	return m_wndToolBar.GetToolBarCtrl().SetButtonInfo(iCount, &bi);
}*/


// Gestionnaires de messages CTabReBar

BEGIN_MESSAGE_MAP(CTabReBar, CToolBar)
	ON_WM_MBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	// Doesn't work otherwise ....
//	ON_NOTIFY_REFLECT(64819, OnTbnGetDispInfo)
	//ON_NOTIFY_REFLECT(TBN_GETDISPINFO, OnTbnGetDispInfo)
	
	ON_NOTIFY_REFLECT(TBN_BEGINDRAG, OnTbnBeginDrag)
	ON_NOTIFY_REFLECT(TBN_ENDDRAG, OnTbnEndDrag)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

void CTabReBar::HandleMouseClick(int flag, CPoint point)
{
	mDragItem = -1;
	int buttonID = GetButtonIDFromPoint(point);

	switch (flag) {
		case 4:
			((CBrowserFrame*)GetParentFrame())->ToggleWindow();
			break;
		case 3:
			if (buttonID<0) 
			{
				GetParentFrame()->PostMessage(WM_COMMAND, ID_NEW_TAB,0);
				break;
			} 
			// Fall through
		case 0: {
			if (buttonID<0) return;
			TBBUTTON button;
			this->GetToolBarCtrl().GetButton(buttonID, &button);
			GetParentFrame()->PostMessage(WM_CLOSETAB, button.idCommand, button.dwData);
			break;
			}
		case 1:
			GetParentFrame()->PostMessage(WM_COMMAND, ID_NEW_TAB,0);
			break;
		case 2: {
			CMenu* menu = theApp.menus.GetMenu(_T("TabButtonPopup"));
			if (!menu) return;
			CRect rect;
			CBrowserTab* tab = NULL;
			if (buttonID>=0)
			{				
				GetToolBarCtrl().GetItemRect(buttonID, &rect);	
				TBBUTTON button;
				GetToolBarCtrl().GetButton(buttonID, &button);
				tab = (CBrowserTab*)button.dwData;
				point.SetPoint(rect.left, rect.bottom);
			}
			
			ClientToScreen(&point);
			UINT cmd = menu->TrackPopupMenu( TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame(), rect);
			
			// XXX Change temporarily the active tab so that the menu 
			// action affect the focussed tab

			CBrowserFrmTab* frame =(CBrowserFrmTab*)GetParentFrame();
			ASSERT(frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)));
			
			CBrowserTab* pTab = frame->GetActiveTab();
			if (tab && pTab!= tab)
				frame->SetActiveBrowser(tab);

			frame->SendMessage(WM_COMMAND,  MAKELONG(cmd,0), 0);
			
			if (tab && pTab!= tab)
				frame->SafeSetActiveBrowser(pTab);

			break;
		}
	}
}

void CTabReBar::OnMButtonUp(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnMiddleClick, point);
	CToolBar::OnMButtonDown(nFlags, point);
}

void CTabReBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnDoubleClick, point);
	CToolBar::OnLButtonDblClk(nFlags, point);
}

void CTabReBar::OnRButtonUp(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnRightClick, point);
	CToolBar::OnRButtonDown(nFlags, point);
}

void CTabReBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	int closePref = theApp.preferences.GetInt("browser.tabs.closeButtons", 2);
	if (closePref == 1)
	{
		int buttonID = GetButtonIDFromPoint(point);
		
		CRect rcItem;
		GetItemRect(buttonID, rcItem);
		if (buttonID>=0) 
		{
			rcItem.right = rcItem.right - 2;
			rcItem.left = rcItem.right - 16;
			rcItem.top = 2;
			rcItem.bottom = rcItem.top + 16;

			if (rcItem.PtInRect(point)) {
				TBBUTTON button;
				this->GetToolBarCtrl().GetButton(buttonID, &button);
				GetParentFrame()->PostMessage(WM_CLOSETAB, button.idCommand, button.dwData);
				return;
			}
		}
	}

	CToolBar::OnLButtonUp(nFlags, point);
}

/*
void CTabReBar::OnTbnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTBDISPINFO pNMTBDispInfo = reinterpret_cast<LPNMTBDISPINFO>(pNMHDR);
	pNMTBDispInfo->iImage = GetParentFrame()->SendMessage(UWM_GETFAVICON, pNMTBDispInfo->idCommand, pNMTBDispInfo->lParam);
	*pResult = 0;
}*/

void CTabReBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int buttonID = GetButtonIDFromPoint(point);
	if (buttonID<0)
		GetParentFrame()->SendMessage(WM_SYSCOMMAND, SC_MOVE+1, MAKELPARAM(point.x,point.y));
	else
		CToolBar::OnLButtonDown(nFlags,point);
}

void CTabReBar::OnTbnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTOOLBAR pNMTB = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
	
	// Only allow left mouse button
	if (!GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON))
		return;

	//HCURSOR cursor = ::LoadCursor(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_MOVE));
	//SetTimer(100,250,NULL);
	//mDragItem = pNMHDR->idFrom;
	GetCursorPos(&mDragPoint);
	ScreenToClient(&mDragPoint);
	mDragItem = GetButtonIDFromPoint(mDragPoint);
	
	*pResult = 0;
}

void CTabReBar::OnTbnEndDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTOOLBAR pNMTB = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
	//KillTimer(100);
	*pResult = 0;	
	mDragItem = -1;
}

void CTabReBar::OnTimer(UINT nIDEvent)
{
	CToolBar::OnTimer(nIDEvent);
}

void CTabReBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (mDragItem>=0 && (abs(mDragPoint.x-point.x)+abs(mDragPoint.y-point.y)>=5))
	{
		TBBUTTON button;
		this->GetToolBarCtrl().GetButton(mDragItem, &button);

		UINT cfTabPt = ::RegisterClipboardFormat(CFSTR_TABPT);

		HGLOBAL hTab = GlobalAlloc(GHND, sizeof(button.dwData));
		DWORD* pTab = (DWORD*)GlobalLock(hTab);
		*pTab = button.dwData;
		GlobalUnlock(hTab);

		CBrowserTab* tab = (CBrowserTab*)button.dwData;

		COleDataSource datasource;
		datasource.CacheGlobalData(cfTabPt, hTab);
		tab->AddURLAndPerformDrag(datasource);
		mDragItem = -1;
		GlobalFree(hTab);
		return;
	}

	CToolBar::OnMouseMove(nFlags, point);
}

void CTabReBar::OnSize(UINT nType, int cx, int cy)
{
	CToolBar::OnSize(nType, cx, cy);
	UpdateButtonsSize();
}

void CTabReBar::OnDestroy()
{
	CToolBar::OnDestroy();
	mDropTarget.Revoke();
}

void CTabReBar::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	
	*pResult = CDRF_DODEFAULT;

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

			if (style & WS_DISABLED)
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
			if (style & WS_DISABLED)
				nState = DFCS_INACTIVE;
			else if (pNMCD->nmcd.uItemState & CDIS_SELECTED)
				nState = DFCS_PUSHED;		
			else if(pNMCD->nmcd.uItemState & CDIS_HOT && pNMCD->nmcd.uItemState & CDIS_CHECKED)
				nState = DFCS_PUSHED | 0x1000; //DFCS_HOT; 
			else if (pNMCD->nmcd.uItemState & CDIS_CHECKED)
				nState = DFCS_CHECKED;
			else if (pNMCD->nmcd.uItemState & CDIS_HOT)
				nState = 0x1000; // DFCS_HOT;

			if (nState != DFCS_FLAT) {
				nState |= 0x0800 | DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;
				pDC->DrawFrameControl(&pNMCD->nmcd.rc, DFC_BUTTON, nState);
			}			
		}
		
		UINT textFlag = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX | DT_WORD_ELLIPSIS;
		
		CString text = GetButtonText(index);
		int image = GetItemImage(pNMCD->nmcd.dwItemSpec);
		CImageList* imageList = GetToolBarCtrl().GetImageList();
		CPoint imagePoint;
		CRect textRect;

		int btMargin = 2;
		int btClose = 16;
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
			imagePoint.x = btMargin + contentRect.left;
			imageList->Draw(pDC, image, imagePoint, ILD_TRANSPARENT);
			contentRect.left += iconPadding + ii.rcImage.right - ii.rcImage.left;
		}		

		contentRect.left += btMargin;
		contentRect.right -= 2 * btMargin + btClose + 2;

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
		
		
		CRect closeButtonRect = contentRect;
		closeButtonRect.left = contentRect.right + btMargin;
		closeButtonRect.right = contentRect.right + btMargin + btClose;
		closeButtonRect.top = (contentRect.bottom - btClose) / 2;
		closeButtonRect.bottom = closeButtonRect.top + btClose;

		HTHEME hThemeC = g_xpStyle.OpenThemeData(m_hWnd, L"WINDOW");

		if (hThemeC) 
		{
			UINT nState = CBS_NORMAL;
			if (pNMCD->nmcd.uItemState & CDIS_HOT)
				nState = CBS_HOT;
			else if (!(pNMCD->nmcd.uItemState & CDIS_CHECKED))
				nState |= CBS_DISABLED;
			if (g_xpStyle.IsThemeBackgroundPartiallyTransparent(hTheme, WP_SMALLCLOSEBUTTON, nState))
				g_xpStyle.DrawThemeParentBackground(m_hWnd, *pDC, &closeButtonRect);
			g_xpStyle.DrawThemeBackground(hThemeC, *pDC, WP_SMALLCLOSEBUTTON, nState, closeButtonRect, NULL);
			g_xpStyle.CloseThemeData(hThemeC);
		}
		else
		{
			UINT nState = DFCS_CAPTIONCLOSE | DFCS_FLAT;
			if (!(pNMCD->nmcd.uItemState & CDIS_CHECKED || pNMCD->nmcd.uItemState & CDIS_HOT))
				nState |= DFCS_INACTIVE;
		
			DrawFrameControl(*pDC, &closeButtonRect, DFC_CAPTION, DFCS_CAPTIONCLOSE | DFCS_FLAT | nState);
		}
        
		g_xpStyle.CloseThemeData(hTheme);

		*pResult = CDRF_SKIPDEFAULT;
		break;
		}
	}
	
	int closePref = theApp.preferences.GetInt("browser.tabs.closeButtons", 2);
	if (closePref == 1) 
		*pResult |= CDRF_NOTIFYITEMDRAW;
}
