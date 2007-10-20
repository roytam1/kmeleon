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
#include ".\tabrebar.h"

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
		theApp.plugins.SendMessage("*", "*", "MoveTab", (long)tab->m_hWnd, (long)((CBrowserTab*)(buttonDest.dwData))->GetSafeHwnd());
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
	mDrag = FALSE;
	mDragItem = -1;

	theApp.preferences.GetString(PREFERENCE_REBAR_TITLE, szTitle, _T(""));
	nButtonStyle = theApp.preferences.GetInt(PREFERENCE_BUTTON_STYLE, 2);
	
   // theApp.preferences.GetInt(PREFERENCE_BUTTON_WIDTH, &tmpWidth, &tmpWidth);

	gImagelist = 0;
   /*char szFullPath[MAX_PATH];
   FindSkinFile(szFullPath, "layers.bmp");
   FILE *fp = fopen(szFullPath, "r");
   if (fp) {
      fclose(fp);
      bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

      BITMAP bmp;
      GetObject(bitmap, sizeof(BITMAP), &bmp);
      
      ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
      gImagelist = ImageList_Create(bmp.bmWidth, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
      if (gImagelist && bitmap)
         ImageList_AddMasked(gImagelist, bitmap, RGB(255, 0, 255));
      if (bitmap)
         DeleteObject(bitmap);
   } */

}

CTabReBar::~CTabReBar()
{
}

BOOL CTabReBar::Create(CReBarEx* rebar, UINT idwnd)
{
		if (!CreateEx(rebar->GetParentFrame(), TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
			WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP ,CRect(0,0,0,0),idwnd))
			return FALSE;
		//ModifyStyle(0, CCS_ADJUSTABLE);
		
		rebar->RegisterBand(m_hWnd, _T("Tabs"), false);
		rebar->AddBar(this, szTitle,0, RBBS_USECHEVRON | RBBS_FIXEDBMP );
		m_wndParent = rebar;

		// Get the width & height of the toolbar.
		// SIZE size;
		// GetToolBarCtrl().GetMaxSize(&size);
				
		REBARBANDINFO rbBand = {0};
        rbBand.cbSize = sizeof(REBARBANDINFO);  
        rbBand.fMask  =  0;// RBBIM_SIZE |RBBIM_CHILDSIZE;
   
		//rbBand.cxMinChild = 0;
		//rbBand.cyMinChild = size.cy + 5;
		//rbBand.cyIntegral = 1;
		//rbBand.cyMaxChild = rbBand.cyMinChild * 2;

		int iband = rebar->FindByName(_T("Tabs"));
		rebar->GetReBarCtrl().SetBandInfo(iband, &rbBand);

		m_wndParent = rebar;
		mDropTarget.Register(this);
		return TRUE;
}

void CTabReBar::UpdateButtonsSize()
{
	int count = this->GetToolBarCtrl().GetButtonCount();
	if (!count) return;

	nButtonMinWidth = theApp.preferences.GetInt(PREFERENCE_BUTTON_MINWIDTH, 10);
	nButtonMaxWidth = theApp.preferences.GetInt(PREFERENCE_BUTTON_MAXWIDTH, 35);
	
	// If the sizes are 0 the toolbar get crazy so avoid it!
	if (nButtonMinWidth>nButtonMaxWidth) nButtonMaxWidth = nButtonMinWidth;
	if (nButtonMaxWidth < 10) nButtonMaxWidth = nButtonMinWidth = 10;

	CDC dc;
	dc.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	int nHSize = dc.GetDeviceCaps(HORZSIZE);
	int nHRes = dc.GetDeviceCaps(HORZRES);
	dc.DeleteDC();

	int nMinWidth = nButtonMinWidth > 0 ? nButtonMinWidth * nHRes / nHSize : nButtonMinWidth;
	int nMaxWidth = nButtonMaxWidth > 0 ? nButtonMaxWidth * nHRes / nHSize : nButtonMaxWidth;

	RECT rect;
	GetWindowRect(&rect);
	int width = rect.right - rect.left;
	width = width / count;
	if (width > nMaxWidth) width = nMaxWidth;
	else if (width < nMinWidth) width = nMinWidth;

	GetToolBarCtrl().SetButtonWidth(width, width);
/*

		// Compute the width needed for the toolbar
		RECT rectButton;
		int ideal = 0;	
		int iCount, iButtonCount = GetToolBarCtrl().GetButtonCount();
		for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
		{

			GetToolBarCtrl().GetItemRect(iCount, &rectButton);
			ideal += rectButton.right - rectButton.left;
		}*/

		SIZE size;
		GetToolBarCtrl().GetMaxSize(&size);

		// Set the ideal size for chevron
        REBARBANDINFO rb;
        rb.cbSize = sizeof(REBARBANDINFO);
		rb.fMask  = RBBIM_IDEALSIZE | RBBIM_CHILDSIZE; 
		rb.cxIdeal = size.cx;
		rb.cxMinChild = 0;
		rb.cyMinChild = rb.cyChild = rb.cyMaxChild = size.cy;
		rb.cyIntegral = 1;

		int iband = m_wndParent->FindByName(_T("Tabs"));
		m_wndParent->GetReBarCtrl().SetBandInfo(iband, &rb);
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
		UpdateButtonsSize();

		int index = m_wndParent->FindByName(_T("Tabs"));

		if (GetToolBarCtrl().GetButtonCount()>1)
			m_wndParent->GetReBarCtrl().ShowBand(index, TRUE);
		else
			if (nItem!=0 && theApp.preferences.bAutoHideTabControl)
				m_wndParent->GetReBarCtrl().ShowBand(index, FALSE);

		return 1;
}

void CTabReBar::RefreshFavIcon()
{

}

BOOL CTabReBar::SetItemText(int idCommand, LPCTSTR lpszItem)
{
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
	//GetToolBarCtrl().SetImageList(&theApp.favicons);
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
	UpdateButtonsSize();

	int index = m_wndParent->FindByName(_T("Tabs"));

	if (GetToolBarCtrl().GetButtonCount()>1)
		m_wndParent->GetReBarCtrl().ShowBand(index, TRUE);
	else
		if (theApp.preferences.bAutoHideTabControl)
			m_wndParent->GetReBarCtrl().ShowBand(index, FALSE);

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
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	// Doesn't work otherwise ....
//	ON_NOTIFY_REFLECT(64819, OnTbnGetDispInfo)
	//ON_NOTIFY_REFLECT(TBN_GETDISPINFO, OnTbnGetDispInfo)
	
	ON_NOTIFY_REFLECT(TBN_BEGINDRAG, OnTbnBeginDrag)
	ON_NOTIFY_REFLECT(TBN_ENDDRAG, OnTbnEndDrag)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CTabReBar::HandleMouseClick(int flag, CPoint point)
{
	int buttonID = GetButtonIDFromPoint(point);

	switch (flag) {
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
			RECT* rect = 0;
			if (buttonID>0)
			{
				rect = new RECT;
				GetToolBarCtrl().GetItemRect(buttonID, rect);	
			}
			GetToolBarCtrl().GetItemRect(buttonID, rect);
			ClientToScreen(&point);
			menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame(), rect);
			if (rect) delete rect;
			break;
		}
	}
}

void CTabReBar::OnMButtonDown(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnMiddleClick, point);
	CToolBar::OnMButtonDown(nFlags, point);
}

void CTabReBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnDoubleClick, point);
	CToolBar::OnLButtonDblClk(nFlags, point);
}

void CTabReBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	HandleMouseClick(theApp.preferences.iTabOnRightClick, point);
	CToolBar::OnRButtonDown(nFlags, point);
}
/*
void CTabReBar::OnTbnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTBDISPINFO pNMTBDispInfo = reinterpret_cast<LPNMTBDISPINFO>(pNMHDR);
	pNMTBDispInfo->iImage = GetParentFrame()->SendMessage(UWM_GETFAVICON, pNMTBDispInfo->idCommand, pNMTBDispInfo->lParam);
	*pResult = 0;
}*/

void CTabReBar::OnTbnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTOOLBAR pNMTB = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
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
	KillTimer(100);
	
	mDragItem = -1;
	
	if (!mDrag) return;

	mDrag = FALSE;
	theApp.favicons.DragLeave(NULL);
	theApp.favicons.EndDrag();
	
	int index;
	index = FindById(pNMTB->iItem);
	ASSERT(index>=0);

	CPoint pos;
	GetCursorPos(&pos);
	ScreenToClient(&pos);
	int destIndex = GetButtonIDFromPoint(pos);
	
	TBBUTTON button = {0};
	GetToolBarCtrl().GetButton(index, &button);
	button.fsState &= ~TBSTATE_PRESSED;

	if (destIndex<0 || destIndex == index)
		return;
		
	GetToolBarCtrl().DeleteButton(index);
	GetToolBarCtrl().InsertButton(destIndex,&button);

	HCURSOR cursor = ::LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
	UpdateWindow();
	
	*pResult = 0;
}

void CTabReBar::OnTimer(UINT nIDEvent)
{
	CToolBar::OnTimer(nIDEvent);
}

void CTabReBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!mDrag && mDragItem>=0 && (abs(mDragPoint.x-point.x)+abs(mDragPoint.y-point.y)>=5))
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

		mDrag = TRUE;
		theApp.favicons.DragShowNolock(TRUE);
		theApp.favicons.BeginDrag(0, CPoint(16,16));
		theApp.favicons.SetDragCursorImage(GetItemImage(GetItemID(GetButtonIDFromPoint(mDragPoint))), CPoint(0,0));
		//theApp.favicons.DragMove(ptAction);
		theApp.favicons.DragEnter(this, point);	// NULL means desktop screen

		HCURSOR cursor = ::LoadCursor(NULL, IDC_SIZEALL);
		SetCursor(cursor);
	}

	if (!mDrag) return;
	//ClientToScreen(&point);
	theApp.favicons.DragMove(point);
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
	// TODO : ajoutez ici le code de votre gestionnaire de messages
}
