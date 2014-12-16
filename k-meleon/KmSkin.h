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
#include "resource.h"

class KmSkin;
class KmImage;

class KmIconList
{
	friend KmSkin;

	typedef CMap<UINT, UINT, int, int> TypeImageCmdList;
	TypeImageCmdList mCmdList;

	CImageList mSized;

	UINT mWidth, mHeight;
	bool mHasDifferentSize;

	bool EmptyImageList(CImageList& list)
	{
		while (list.GetImageCount())
			if (!list.Remove(0))
				return false;
		return true;
	}

public:

	CImageList mHot;
	CImageList mCold;
	CImageList mDead;

	KmIconList(UINT w, UINT h) : mWidth(w), mHeight(h)
	{
		int width = ::GetSystemMetrics(SM_CXSMICON);
		int height = ::GetSystemMetrics(SM_CYSMICON);
		if (width != w || height != h) {
			mHasDifferentSize = true;
			mSized.Create(width, height, ILC_COLOR32, 0, 10);
		} else 
			mHasDifferentSize = false;
		mHot.Create(w, h, ILC_COLOR32, 0, 10);
		mCold.Create(w, h, ILC_COLOR32, 0, 10);
		mDead.Create(w, h, ILC_COLOR32, 0, 10);
	}	

	int AddIcons(KmImage& img, UINT w, UINT h, UINT id = 0);
	int AddIcon(KmImage& img, UINT id = 0);
	int AddIcon(KmImage&  img, KmImage&  hotImg, KmImage&  deadImg, UINT id = 0);
	int AddIcon(LPCTSTR coldImgPath, LPCTSTR hotImgPath, LPCTSTR deadImgPath, UINT id = 0, UINT w = 0, UINT h = 0);
	
	HIMAGELIST GetIconList() {
		return mHasDifferentSize ? mSized.GetSafeHandle() : mCold.GetSafeHandle();
	}

	int GetImg(UINT id)
	{
		int index;
		if (mCmdList.Lookup(id, index))
			return index;
		return I_IMAGENONE;
	}

	void Reset()
	{
		if (mHasDifferentSize) 
			EmptyImageList(mSized);
		EmptyImageList(mHot);
		EmptyImageList(mCold);
		EmptyImageList(mDead);
	}
};

class KmSkin
{
protected:	
	CBitmap mBackImg;
	UINT mDefWidth, mDefHeight;
	CString mSkinName;

	static char* skinImg[];
	CGdiObject miSecure;
	CGdiObject miInsecure;
	CGdiObject miBroken;
	CGdiObject miPopup;
	CGdiObject miLoading;
	CGdiObject miDefault;
	CGdiObject miMain;

	HICON LoadSkinIcon(LPCTSTR aSkinFile, UINT resID, CGdiObject& obj, UINT w = 16, UINT h = 16)
	{
		if (!obj.m_hObject)
			obj.Attach(LoadSkinIcon(aSkinFile, resID, w, h));
		return (HICON)(HGDIOBJ)obj;
	}

public:
	KmIconList* mImages;

	KmSkin(void);
	~KmSkin(void);

	bool Init(LPCTSTR skinName);	
	bool SetImageList(CToolBarCtrl& toolbar) const;

	UINT GetDefWidth() {
		return ::GetSystemMetrics(SM_CXSMICON);
	}

	UINT GetDefHeight() {
		return ::GetSystemMetrics(SM_CYSMICON);
	}

	UINT GetUserWidth() {
		return mDefWidth;
	}

	UINT GetUserHeight() {
		return mDefHeight;
	}

	HIMAGELIST GetIconList() {
		return mImages ? mImages->GetIconList() : NULL;
	}

	int GetIconIndex(UINT id) {
		return mImages ? mImages->GetImg(id) : I_IMAGENONE;
	}

	HICON GetIconMain()
	{
		return LoadSkinIcon(_T("main.ico"), 0, miMain);
	}

	HICON GetIconDefault()
	{
		return LoadSkinIcon(_T("default.ico"), 0, miDefault, GetUserWidth(), GetUserHeight());
	}

	HICON GetIconLoading()
	{
		return LoadSkinIcon(_T("loading.ico"), 0, miLoading, GetUserWidth(), GetUserHeight());
	}

	HICON GetIconInsecure()
	{
		return LoadSkinIcon(_T("sinsecur.ico"), IDI_SECURITY_UNLOCK, miInsecure);
	}

	HICON GetIconSecure()
	{
		return LoadSkinIcon(_T("ssecur.ico"), IDI_SECURITY_LOCK, miSecure);
	}

	HICON GetIconBroken()
	{
		return LoadSkinIcon(_T("sbroken.ico"), IDI_SECURITY_BROKEN, miBroken);
	}

	HICON GetIconPopupBlock()
	{
		return LoadSkinIcon(_T("popupblock.ico"), IDI_POPUP_BLOCKED, miPopup);
	}

	HBITMAP GetBackImage()
	{
		if (!mBackImg.m_hObject) {
			CString skinFile;
			if (FindSkinFile(skinFile, _T("Back.bmp")))
			{
				mBackImg.Attach( (HBITMAP) ::LoadImage (NULL,
					skinFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE) );
			}
		}
		return mBackImg;
	}	

	HICON LoadSkinIcon(LPCTSTR aSkinFile, UINT resID, UINT w = 16, UINT h = 16)
	{
		HICON hIcon = NULL;
		CString skinFile(aSkinFile);
		if (FindSkinFile(skinFile, aSkinFile))
			hIcon = (HICON)::LoadImage(NULL,
				skinFile, IMAGE_ICON, w, h, LR_LOADFROMFILE);
		if (!hIcon && resID)
			hIcon = (HICON)::LoadImage(AfxGetResourceHandle(),
				MAKEINTRESOURCE(resID), IMAGE_ICON, w, h, LR_LOADMAP3DCOLORS);
		return hIcon;
	}

	bool FindSkinFile( CString& szSkinFile, LPCTSTR filename, LPCTSTR skin = NULL, bool searchUser = true);
	int AddIcon(LPCTSTR coldImgPath, LPCTSTR hotImgPath, LPCTSTR deadImgPath, UINT id = 0, UINT w = 0, UINT h = 0);
};

