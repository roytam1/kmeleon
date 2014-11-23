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

bool KmToolbar::LoadImage(LPCTSTR skinImg, KmImage& img, UINT w, UINT h) 
{
	if (!w) w = mWidth;
	if (!h) h = mHeight;
	ASSERT(w && h);
	CString imgPath(skinImg);
	UINT index = 0;
	int pos = imgPath.Find(_T('['));
	if (pos != -1) {
		int pos2 = imgPath.Find(_T(']'));
		index = _ttoi(imgPath.Mid(pos+1, pos2-pos).GetBuffer());
		imgPath.Truncate(pos);
	}

	if (!img.LoadFromSkin(imgPath)) {
		ASSERT(0);
		return false;
	}

	img.Crop(w, h, index);
	return true;
}

void KmToolbar::AddItem(KmButton& button, int before, UINT w, UINT h)
{
	KmButton* pbutton = new KmButton();
	*pbutton = button;
	if (pbutton->mAction.GetLength())
		pbutton->mID = theApp.commands.GetId(pbutton->mAction);
	if (!pbutton->mID && pbutton->mName.GetLength())
		return;
	pbutton->mImageIndex = theApp.skin.GetIconIndex(pbutton->mID);
	if (pbutton->mImageIndex == I_IMAGENONE && button.mColdImage) {
		if (!w) w = theApp.skin.GetUserWidth();
		if (!h) h = theApp.skin.GetUserHeight();
		KmImage img;
		if (LoadImage(button.mColdImage, img, w, h)) {			

			// If possible add the icon to the shared list
			if (theApp.skin.mImages && (!mWidth || (
				mWidth == theApp.skin.GetUserWidth() &&
				mHeight == theApp.skin.GetUserHeight()))) {
			
				KmImage hotImg, deadImg;

				// If hot image specified, then 1 image for each state
				if (button.mHotImage.GetLength()) {
					if (!LoadImage(button.mHotImage, hotImg, w, h))
						LoadImage(button.mColdImage, hotImg, w, h);
					if (!button.mDeadImage.GetLength() || !LoadImage(button.mDeadImage, deadImg, w, h))
						LoadImage(button.mColdImage, deadImg, w, h);
					pbutton->mImageIndex = theApp.skin.mImages->AddIcon(img, hotImg, deadImg, pbutton->mID);
				}
				else {
					// Single image with all states
					pbutton->mImageIndex = theApp.skin.mImages->AddIcon(img, pbutton->mID);					
				}
				
			} else {

				int w = img.GetWidth();
				int h = img.GetHeight();
				if (!mHot.m_hImageList) {
					mHot.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
					mCold.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
					mDead.Create(mWidth, mHeight, ILC_MASK|ILC_COLOR32, 0, 10);
				}
		
				pbutton->mImageIndex = img.AddToImageList(mCold);

				if (button.mHotImage.GetLength()) {
					LoadImage(button.mHotImage, img, w, h);
					int i = img.AddToImageList(mHot);
					ASSERT(i == pbutton->mImageIndex);
				}

				if (button.mDeadImage.GetLength()) {
					if (LoadImage(button.mDeadImage, img, w, h)) {
						int i = img.AddToImageList(mDead);
						ASSERT(i == pbutton->mImageIndex);
					}
				}
			}
		}
	}
	mButtons.AddTail(pbutton);
}

bool KmToolbar::Init(CToolBar* hToolbar)
{
	if (!GetButtonCount()) return NULL;
	TBBUTTON* hButtons = new TBBUTTON[GetButtonCount()];
	bool hasString = false;
	POSITION pos = mButtons.GetHeadPosition();
	int i = 0, j = 0;
	while (pos) {
		KmButton* kbutton = mButtons.GetNext(pos);		
		if (kbutton->mName.GetLength()>0) {
			hButtons[i].iString = -1;
			if (kbutton->mLabel.GetLength() > 0) {
				CString strTemp(theApp.lang.Translate(kbutton->mLabel), lstrlen(kbutton->mLabel)+1);
				
				hButtons[i].iString = (INT_PTR)hToolbar->GetToolBarCtrl().SendMessage(TB_ADDSTRING, 0, (LPARAM)strTemp.GetBuffer());
				hasString = true;
			}
			hButtons[i].iBitmap = kbutton->mImageIndex;
			hButtons[i].idCommand = kbutton->mID;
			//hButtons[i].iString = kbutton->mLabel.GetLength() ? (INT_PTR)(LPCTSTR)kbutton->mLabel : 0;
			hButtons[i].dwData = (DWORD_PTR)&kbutton;
			hButtons[i].fsState = TBSTATE_ENABLED;
			hButtons[i].fsStyle = BTNS_BUTTON; 
			hButtons[i].bReserved[0] = 0;
		} else {
			// Separator
			hButtons[i].iBitmap = 0;
			hButtons[i].idCommand = 0;
			hButtons[i].iString = 0;
			hButtons[i].dwData = 0;
			hButtons[i].fsState = TBSTATE_ENABLED;
			hButtons[i].fsStyle = BTNS_SEP;
			hButtons[i].bReserved[0] = 0;
		}
		i++;
	}
	if (hasString)
		hToolbar->ModifyStyle(0, TBSTYLE_LIST);

	hToolbar->GetToolBarCtrl().AddButtons(GetButtonCount(), hButtons);

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
	return true;
}

bool KmToolbarService::InitWindows(CReBarEx* rebar) 
{	
	CString s;
	KmToolbar *ktoolbar;		
	POSITION pos = mToolbars.GetStartPosition();
	while (pos) {
		mToolbars.GetNextAssoc( pos, s, ktoolbar);

		CToolBarEx* hToolbar = new CToolBarEx();
		int style = CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS;
		int sstyle = WS_CHILD|WS_VISIBLE| (style&CCS_BOTTOM ? CBRS_ALIGN_BOTTOM : CBRS_ALIGN_TOP);
        hToolbar->CreateEx(rebar->GetParentFrame(), style, sstyle);
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