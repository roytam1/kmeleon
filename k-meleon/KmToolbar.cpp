/*
*  Copyright (C) 2014 Dorian Boissonnade
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
#include "KmToolbar.h"
#include "KmImage.h"
#include "MfcEmbed.h"
#include "ReBarEx.h"
#include "ToolBarEx.h"
#include "BrowserFrm.h"

bool KmToolbar::RemoveItem(UINT id)
{
	bool res = false;
	POSITION pos = mButtons.GetHeadPosition();
	while (pos) {
		KmButton* kbutton = mButtons.GetAt(pos);		
		if (kbutton->mID == id) {
			mButtons.RemoveAt(pos);
			res = true;
			break;
		}
		mButtons.GetNext(pos);
	}

	pos = mToolbars.GetHeadPosition();
	while (pos) {
		CToolBarEx* toolbar = mToolbars.GetNext(pos);	
		for (int i=0;i<toolbar->GetToolBarCtrl().GetButtonCount();i++) {
			UINT bid, style;
			int iimage;
			toolbar->GetButtonInfo(i, bid, style, iimage);
			if (id == bid) {
				toolbar->GetToolBarCtrl().DeleteButton(i);
				CBrowserFrame* frm = DYNAMIC_DOWNCAST(CBrowserFrame,toolbar->GetParentFrame());
				frm->m_wndReBar.RecalcMinSize(toolbar);
			}
		}
	}
	return res;
}

KmButton* KmToolbar::AddItem(LPCTSTR name, UINT id, int before)
{
	KmButton* pbutton = new KmButton();
	pbutton->mName = name;
	return pbutton;
}

int KmToolbar::AddImage(LPCTSTR cold, UINT w, UINT h, LPCTSTR hot, LPCTSTR dead, int oldIndex)
{
	KmImage img;
	int index = -1;
	RECT r = {0,0,w,h};
	if (cold && img.LoadFromSkin(cold, &r)) {	

		if (!mHot.m_hImageList) {
			mHot.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
			mCold.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
			mDead.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
		}

		img.Scale((1.0*mWidth)/img.GetWidth());

		if (hot && _tcslen(hot)) {
			index = img.AddToImageList(mCold, oldIndex);
					
			img.LoadFromSkin(hot, &r);
			img.Scale((1.0*mWidth)/img.GetWidth());
			int i = img.AddToImageList(mHot, oldIndex);
			ASSERT(i == index);

			if (dead && _tcslen(dead)) {
				img.LoadFromSkin(dead, &r);
				img.Scale((1.0*mWidth)/img.GetWidth());
			}
			i = img.AddToImageList(mDead, oldIndex);
			ASSERT(i == index);
					
		} else {
			KmImage tmpImg;
			if (!img.CropLine(mHeight, 0, tmpImg))
				return index;	
			index = tmpImg.AddToImageList(mCold, oldIndex);

			img.CropLine(mHeight, 1, tmpImg);
			int i = tmpImg.AddToImageList(mHot, oldIndex);
			ASSERT(index == i);

			img.CropLine(mHeight, 2, tmpImg);
			i = tmpImg.AddToImageList(mDead, oldIndex);
			ASSERT(index == i);
		}
	} else {
		ASSERT(oldIndex>=0);
		if (oldIndex < 0) return -1;
		if (hot && _tcslen(hot)) {
			img.LoadFromSkin(hot, &r);
			img.Scale((1.0*mWidth)/img.GetWidth());
			index = img.AddToImageList(mHot, oldIndex);
			ASSERT(index == oldIndex);
		}

		if (dead && _tcslen(dead)) {
			img.LoadFromSkin(dead, &r);
			img.Scale((1.0*mWidth)/img.GetWidth());
			index = img.AddToImageList(mHot, oldIndex);
			ASSERT(index == oldIndex);
		}
	}
	return index;
}

int KmToolbar::SetImage(UINT id, LPCTSTR cold, LPCTSTR hot, LPCTSTR dead)
{
	KmButton* b = GetButton(id);
	if (!b) return -1;
	int w = mWidth ? mWidth : theApp.skin.GetUserWidth();
	int h = mHeight ? mHeight : theApp.skin.GetUserHeight();
	b->mImageIndex = AddImage(cold, w, h, hot, dead, b->mImageIndex);
	Refresh();
	return b->mImageIndex;
}

void KmToolbar::AddItem(KmButton& button, int before, UINT w, UINT h)
{
	KmButton* pbutton = new KmButton();
	*pbutton = button;
	if (pbutton->mAction.GetLength()) {
		pbutton->mID = theApp.commands.GetId(pbutton->mAction);
		if (!pbutton->mID) return;
	}
	int imageIndex = theApp.skin.GetIconIndex(pbutton->mID);
	if (imageIndex == I_IMAGENONE && button.mColdImage.GetLength()) {

		/*if (button.mColdImage.Left(6).Compare(L"chrome") == 0) {

			iconTbObserver* io = new iconTbObserver(pbutton);
			nsCOMPtr<nsIURI> uri;
			NewURI(getter_AddRefs(uri), CStringToNSString(button.mColdImage));

			if (!nsImageObserver::LoadImage(io, uri)) {
				delete io;
				return;
			}

		} else {*/

		if (!w) w = mWidth ? mWidth : theApp.skin.GetUserWidth();
		if (!h) h = mHeight ? mHeight : theApp.skin.GetUserHeight();

		if (!mWidth) {
			mWidth = w;
			mHeight = h;
		}

		pbutton->mImageIndex = AddImage(
			button.mColdImage, w, h,
			button.mHotImage,
			button.mDeadImage
		);
		
		// If possible add the icon to the shared list
		/*if (theApp.skin.mImages && (!mWidth || (
			mWidth == theApp.skin.GetUserWidth() &&
			mHeight == theApp.skin.GetUserHeight()))) {
			
			theApp.skin.mImages->AddIcon(button.mColdImage, button.mHotImage, button.mDeadImage, pbutton->mID);
				
		} else */{

		}
		//}
	} else
		pbutton->mImageIndex = I_IMAGECALLBACK;

	mButtons.AddTail(pbutton);
	POSITION pos = mToolbars.GetHeadPosition();
	while (pos) {
		CToolBarEx* toolbar = mToolbars.GetNext(pos);	
		TBBUTTON b = InitButton(pbutton, toolbar);
		toolbar->GetToolBarCtrl().InsertButton(-1, &b);
		CBrowserFrame* frm = DYNAMIC_DOWNCAST(CBrowserFrame,toolbar->GetParentFrame());
		frm->m_wndReBar.RecalcMinSize(toolbar);
	}
}

void KmToolbar::Refresh()
{
	POSITION pos = mToolbars.GetHeadPosition();
	while (pos) {
		CToolBarEx* toolbar = mToolbars.GetNext(pos);
		toolbar->Invalidate();
	}
}

TBBUTTON KmToolbar::InitButton(KmButton* kbutton, CToolBarEx* hToolbar)
{
	TBBUTTON res;
	if (kbutton->mID || kbutton->mName.GetLength()>0) {
		res.iString = -1;
		if (kbutton->mLabel.GetLength() > 0) {
			CString strTemp(theApp.lang.Translate(kbutton->mLabel), lstrlen(kbutton->mLabel)+1);
			
			res.iString = (INT_PTR)hToolbar->GetToolBarCtrl().SendMessage(TB_ADDSTRING, 0, (LPARAM)strTemp.GetBuffer());
		}
		res.iBitmap = kbutton->mImageIndex;
		res.idCommand = kbutton->mID;
		//res.iString = kbutton->mLabel.GetLength() ? (INT_PTR)(LPCTSTR)kbutton->mLabel : 0;
		res.dwData = (DWORD_PTR)&kbutton;
		res.fsState = TBSTATE_ENABLED;
		res.fsStyle = BTNS_BUTTON; 
		res.bReserved[0] = 0;
	} else {
		// Separator
		res.iBitmap = 0;
		res.idCommand = 0;
		res.iString = -1;
		res.dwData = 0;
		res.fsState = TBSTATE_ENABLED;
		res.fsStyle = BTNS_SEP;
		res.bReserved[0] = 0;
	}
	return res;
}

void KmToolbar::Remove(CToolBarEx* hToolbar)
{
	POSITION pos = mToolbars.GetHeadPosition();
	while (pos) {
		CToolBarEx* toolbar = mToolbars.GetAt(pos);	
		if (hToolbar == toolbar) {
			mToolbars.RemoveAt(pos);
			break;
		}
		mToolbars.GetNext(pos);	
	}
	ASSERT(pos != mToolbars.GetTailPosition());
}

bool KmToolbar::Init(CToolBarEx* hToolbar)
{
	if (!GetButtonCount()) return NULL;
	TBBUTTON* hButtons = new TBBUTTON[GetButtonCount()];
	bool hasString = false;
	POSITION pos = mButtons.GetHeadPosition();
	int i = 0, j = 0;
	while (pos) {
		KmButton* kbutton = mButtons.GetNext(pos);		
		hButtons[i] = InitButton(kbutton, hToolbar);
		if (hButtons[i].iString != -1) hasString = true;
		i++;
	}
	if (hasString)
		hToolbar->ModifyStyle(0, TBSTYLE_LIST);

	hToolbar->GetToolBarCtrl().AddButtons(GetButtonCount(), hButtons);
	delete hButtons;

	CSize btnSize, btnImgSize(0,0);
	if (mCold.m_hImageList) {
		hToolbar->GetToolBarCtrl().SetImageList(&mCold);
		hToolbar->GetToolBarCtrl().SetHotImageList(&mHot);
		hToolbar->GetToolBarCtrl().SetDisabledImageList(&mDead);
		btnImgSize.SetSize(mWidth, mHeight);
	} else if (theApp.skin.SetImageList(hToolbar->GetToolBarCtrl())) {		
		btnImgSize.SetSize(theApp.skin.GetUserWidth(), theApp.skin.GetUserHeight());
	}

	if (!hasString) {
		int hp, vp;
		hToolbar->GetToolBarCtrl().GetPadding(hp, vp);	
		btnSize.SetSize(btnImgSize.cx+hp, btnImgSize.cy+vp);
	} else {		
		hToolbar->GetToolBarCtrl().SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED);
		DWORD dwBtnSize = hToolbar->GetToolBarCtrl().GetButtonSize();	
		btnSize.SetSize(LOWORD(dwBtnSize), HIWORD(dwBtnSize));
	}
	hToolbar->SetSizes(btnSize, btnImgSize);
	mToolbars.AddTail(hToolbar);
	return true;
}

void KmToolbarService::CloseWindow(CBrowserFrame* frame) 
{
	CString s;
	KmToolbar *ktoolbar;		
	POSITION pos = mToolbars.GetStartPosition();
	while (pos) {
		mToolbars.GetNextAssoc( pos, s, ktoolbar);
		CToolBarEx* hToolbar = (CToolBarEx*)CWnd::FromHandle(frame->m_wndReBar.GetChildByName(s));
		ASSERT(hToolbar);
		ktoolbar->Remove(hToolbar);
	}
}

bool KmToolbarService::InitWindow(CBrowserFrame* frame) 
{	
	CString s;
	CReBarEx* rebar = &frame->m_wndReBar;
	KmToolbar *ktoolbar;		
	POSITION pos = mToolbars.GetStartPosition();
	while (pos) {
		mToolbars.GetNextAssoc( pos, s, ktoolbar);

		//CToolBarEx* hToolbar = new CToolBarEx();
		int style = CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS;
		//int sstyle = WS_CHILD|WS_VISIBLE| (style&CCS_BOTTOM ? CBRS_ALIGN_BOTTOM : CBRS_ALIGN_TOP);
        //hToolbar->CreateEx(rebar->GetParentFrame(), style, sstyle);
		CToolBarEx* hToolbar = frame->CreateToolbar(style);
		SetProp(hToolbar->GetSafeHwnd(), _T("kmToolbar"), (HANDLE)ktoolbar);
		ktoolbar->Init(hToolbar);
		
		rebar->AddBar(hToolbar);
		//rebar->GetReBarCtrl().InsertBand();
		rebar->RegisterBand(hToolbar->GetSafeHwnd(), s, true);		
	}
	return true;
}

BOOL KmToolbarService::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (nCode != CN_UPDATE_COMMAND_UI || !pExtra)
		return FALSE;

	((CCmdUI*)pExtra)->m_bEnableChanged = TRUE;
	KmToolbar* kmtoolbar = (KmToolbar*)GetProp(((CCmdUI*)pExtra)->m_pOther->GetSafeHwnd(), _T("kmToolbar"));
	if (!kmtoolbar)
		return FALSE;

	KmButton* button = kmtoolbar->GetButton(nID);
	if (!button) 
		return FALSE;

	unsigned ret;
	if (theApp.plugins.SendMessageUntilSuccess("*", "KmToolbar", "GetState", nID, (long)&ret))
	{
		((CCmdUI*)pExtra)->Enable((ret & 0x1)==0);
		((CCmdUI*)pExtra)->SetCheck((ret & 0x2)!=0);
	} else {
		((CCmdUI*)pExtra)->Enable(button->mEnabled);
		((CCmdUI*)pExtra)->SetCheck(button->mChecked);
	}

	((CCmdUI*)pExtra)->m_bEnableChanged = TRUE;
	return TRUE;
}

void KmToolbarService::Refresh()
{
	CBrowserFrame* pBrowserFrame = NULL;
	POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
	while( pos != NULL ) {
		pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
		pBrowserFrame->m_wndReBar.RedrawWindow(0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	}
}