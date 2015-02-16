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

#pragma once

#define DIB_WIDTHBYTES(bits) ((((bits) + 31)>>5)<<2)

class CBrowserFrame;
class CToolBarEx;
class KmImage;

class KmButton
{
public:
	KmButton() : mEnabled(true), mChecked(false), mID(0) {}
	CString mName;
	CString mLabel;
	CString mTooltip;
	CString mHotImage;
	CString mColdImage;
	CString mDeadImage;
	CString mAction;
	CString mMenuName;	
	UINT mID;
	int mImageIndex;
	bool mEnabled,mChecked;
};

class KmToolbar
{
public:
	KmToolbar(UINT w, UINT h):mWidth(w),mHeight(h) {
		
	};
	~KmToolbar(void) {
		POSITION pos = mButtons.GetHeadPosition();
		while (pos) {
			KmButton* kbutton = mButtons.GetNext(pos);
			delete kbutton;
		}
	};

	void AddItem(KmButton& button, int before = -1, UINT w = 0, UINT h = 0);	
	bool RemoveItem(UINT id);

	UINT GetButtonCount()
	{
		return mButtons.GetCount();
	}

	KmButton* GetButton(UINT id) {
		POSITION pos = mButtons.GetHeadPosition();
		while (pos) {
			KmButton* button = mButtons.GetNext(pos);
			if (button->mID == id)
				return button;
		}
		return nullptr;
	}

	bool Init(CToolBarEx* wToolbar);
	void Remove(CToolBarEx* hToolbar);
	void Refresh();

	bool LoadImage(LPCTSTR skinImg, KmImage& img, UINT w = 0, UINT h = 0);
	CImageList mHot;
	CImageList mCold;
	CImageList mDead;
	CString mTitle;	
	int mWidth, mHeight;

protected:
	TBBUTTON InitButton(KmButton* kbutton, CToolBarEx* hToolbar);
	CList<KmButton*, KmButton*> mButtons;
	CList<CToolBarEx*, CToolBarEx*> mToolbars;
};

class KmToolbarService {
public:

	KmToolbarService() : mDefWidth(0),mDefHeight(0) {}

	~KmToolbarService() {
		CString s;
		KmToolbar *ktoolbar;
		POSITION pos = mToolbars.GetStartPosition();
		while (pos) {
			mToolbars.GetNextAssoc( pos, s, ktoolbar);
			delete ktoolbar;
		}
		mToolbars.RemoveAll();
	}

	KmToolbar* CreateToolbar(LPCTSTR name, UINT width = 0, UINT height = 0) {
		KmToolbar* toolbar = new KmToolbar(width, height);
		mToolbars[name] = toolbar;
		return toolbar;
	}

	KmToolbar* GetKToolbar(LPCTSTR name) {
		KmToolbar* toolbar;
		if (!mToolbars.Lookup(name, toolbar))
			return NULL;
		return toolbar;
	}

	void SetToolbar(LPCTSTR name, KmButton &item, long before = -1)
	{
		KmToolbar* toolbar;
		if (!mToolbars.Lookup(name, toolbar))
			toolbar = CreateToolbar(name);

		toolbar->AddItem(item, before);
	}

	bool Init();
	bool InitWindow(CBrowserFrame* frame);
	void CloseWindow(CBrowserFrame* frame);
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	UINT GetDefaultWidth() { return mDefWidth; }
	UINT GetDefaultHeight() { return mDefHeight; }
	void Refresh();

protected:
	CMap<CString, LPCTSTR, KmToolbar*, KmToolbar*> mToolbars;
	UINT mDefWidth, mDefHeight;	
};

