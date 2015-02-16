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
#include "KmSkin.h"
#include "KmImage.h"
#include "MfcEmbed.h"
#include "kmeleon_plugin.h"
#include "lib/rapidjson/document.h"

char* KmSkin::skinImg[] = {"main", "default", "loading","popup", "secure", "insecure", "broken", "background"};


int KmIconList::AddIcon(KmImage& img, KmImage& hotImg, KmImage& deadImg, UINT id)
{
	UINT imgWidth = img.GetWidth();
	UINT imgHeight= img.GetHeight();
	if (imgWidth != mWidth) {
		img.Scale((1.0*mWidth)/imgWidth);
		hotImg.Scale((1.0*mWidth)/imgWidth);
		deadImg.Scale((1.0*mWidth)/imgWidth);
	}

	int index = -1;
	if (id) mCmdList.Lookup(id, index);

	int pos = img.AddToImageList(mCold, index);
	if (pos == -1) return -1;
	int idx = hotImg.AddToImageList(mHot, index);
	ASSERT(pos == idx);
	idx = deadImg.AddToImageList(mDead, index);
	ASSERT(pos == idx);
	if (id) mCmdList[id] = pos;
	return pos;
}

int KmIconList::AddIcon(KmImage& img, UINT id)
{
	UINT imgWidth = img.GetWidth();
	UINT imgHeight= img.GetHeight();
	int pos = AddIcons(img, imgWidth, imgHeight, id);
	return pos;
}

int KmIconList::AddIcons(KmImage& img, UINT imgWidth, UINT imgHeight, UINT id)
{
	int idx = -1;
	int index = -1;
	if (id) mCmdList.Lookup(id, index);

	KmImage tmpImg;
	if (mHasDifferentSize) {
		int cx, cy;
		ImageList_GetIconSize(mSized.GetSafeHandle(), &cx, &cy);
		if (imgWidth != cx) { 
			KmImage sizedImg;
			img.Scale((1.0*cx)/imgWidth, sizedImg);
			if (sizedImg.CropLine(cy, 0, tmpImg)) {
				idx = tmpImg.AddToImageList(mSized, index);
			}
		} else {
			if (img.CropLine(cy, 0, tmpImg)) {
				idx = tmpImg.AddToImageList(mSized, index);
			}
		}
	}

	if (imgWidth != mWidth)
		img.Scale((1.0*mWidth)/imgWidth);		
	
	if (!img.CropLine(mHeight, 0, tmpImg))
		return -1;

	int pos = tmpImg.AddToImageList(mCold, index);
	if (pos == -1) return -1;
	ASSERT(idx == -1 || idx == pos);

	img.CropLine(mHeight, 1, tmpImg);
	idx = tmpImg.AddToImageList(mHot, index);
	ASSERT(pos == idx);
	
	img.CropLine(mHeight, 2, tmpImg);
	idx = tmpImg.AddToImageList(mDead, index);
	ASSERT(pos == idx);
	
	/*
	if (img.CropLine(mImages->mHeight, 2, tmpImg)) {
		int idx = tmpImg.AddToImageList(mDead);
		ASSERT(pos == idx);
	}	
		
	while (mImages->mDead.GetImageCount() < mImages->mCold.GetImageCount())
		if (mImages->mDead.Add(&emptyBitmap, &emptyBitmap) == -1)
			break;*/
		
	ASSERT(mDead.GetImageCount() == mCold.GetImageCount());
	if (id) mCmdList[id] = pos;
	return pos;
}

KmSkin::KmSkin(void) : mImages(0),mDefWidth(0),mDefHeight(0)
{
}

KmSkin::~KmSkin(void)
{
	 if (mImages) delete mImages;
}

bool KmSkin::SetImageList(CToolBarCtrl& toolbar) const
{
	if (!mImages) return false;
	toolbar.SetImageList(&mImages->mCold);
	toolbar.SetHotImageList(&mImages->mHot);
	toolbar.SetDisabledImageList(&mImages->mDead);
	return true;
}

bool KmSkin::FindSkinFile( CString& szSkinFile, LPCTSTR filename, LPCTSTR skin, bool searchUser) 
{
	WIN32_FIND_DATA FindData;
	HANDLE hFile;
	CString file;

	if (!skin) skin = mSkinName;
	ASSERT(_tcslen(skin));
	ASSERT(filename);
	if (!filename || !*filename)
		return false;
   
	// Search in user profile
	if (searchUser) {
		file = theApp.GetFolder(UserSettingsFolder) + _T('\\') + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
	}

	// Search in user profile skin
	file = theApp.GetFolder(UserSkinsFolder) + _T("\\") + skin + _T("\\") + filename;
	hFile = FindFirstFile(file, &FindData);
	if(hFile != INVALID_HANDLE_VALUE) {   
		FindClose(hFile);
		szSkinFile = file;
		return true;
	}  

	// Search in skin
	file = theApp.GetFolder(SkinsFolder) + _T("\\") + skin + _T("\\") + filename;
	hFile = FindFirstFile(file, &FindData);
	if(hFile != INVALID_HANDLE_VALUE) {   
		FindClose(hFile);
		szSkinFile = file;
		return true;
	} 

	/*
	CString tmp = skin ? skin : mSkinName;
	ASSERT(tmp.GetLength());
	CString skinsDir = theApp.GetFolder(UserSkinsFolder) + _T("\\");
	while (tmp.GetLength()>0) {
		if (tmp.GetAt( tmp.GetLength()-1 ) != '\\')
			tmp = tmp + _T("\\");
		file = skinsDir + tmp + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
		tmp = tmp.Left( tmp.GetLength()-2 );
	}

	tmp = skin ? skin : theApp.preferences.skinsCurrent;
	skinsDir = theApp.GetFolder(SkinsFolder) + _T("\\");
	while (tmp.GetLength()>0) {
		if (tmp.GetAt( tmp.GetLength()-1 ) != '\\')
			tmp = tmp + _T("\\");
		file = skinsDir + tmp + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
		tmp = tmp.Left( tmp.GetLength()-2 );
	}*/

	// Fallback to shared
	if (searchUser) {
		file = theApp.GetFolder(SkinsFolder) + _T("\\shared\\") + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}  

		file = theApp.GetFolder(SkinsFolder) + _T("\\default\\") + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		} 
	}

	return false;
}


bool KmSkin::Init(LPCTSTR skinName)
{
	using namespace rapidjson;

	bool first = false;
	if (!mImages) {
		mDefWidth = GetDefWidth();
		mDefHeight = GetDefHeight();

		int userSize = theApp.preferences.GetInt("kmeleon.display.toolbars_size", 0);
		if (userSize > 0) {
			mDefWidth = (int)((mDefWidth * userSize / mDefHeight) + .5);
			mDefHeight = userSize;
		}

		mImages = new KmIconList(mDefWidth, mDefHeight);
		first = true;
	}

	CString filename;
	bool oldSkin = !FindSkinFile(filename, _T("skin.cfg"), skinName, false);

	if (!first) {
		if (mOldSkin || oldSkin) return false;
		mImages->Reset();
		mBackImg.DeleteObject();
	}

	mOldSkin = oldSkin;
	mSkinName = skinName;
	
	CFile file;
    if (!file.Open(filename, CFile::modeRead, NULL))
	   return false;

	char* buf = new char[file.GetLength()+1];
	UINT size = file.Read(buf, file.GetLength());
	buf[size] = 0;
	
	Document d;
	d.Parse(buf);
	if (d.HasParseError() || !d.IsObject()) return false;

	const char* imgFile = 0;
	int defWidth = d.HasMember("width") ? d["width"].GetInt() : 16;
	int defHeight = d.HasMember("height") ? d["height"].GetInt() : 16;	

	/*CBitmap emptyBitmap;
	size_t imgsize = mImages->mWidth*mImages->mHeight/8 == 0 ? 1 : mImages->mWidth*mImages->mHeight/8;
	BYTE* bits = new BYTE[imgsize];
	memset(bits,0xff,imgsize);
	emptyBitmap.CreateBitmap(mImages->mWidth,mImages->mHeight,1,1,bits);
	delete [] bits;*/
	
	Value& lists = d["lists"];
	for (SizeType i=0;i<lists.Size();i++) {
		Value& confImage = lists[i];
		if (!confImage.IsObject() || !confImage.HasMember("images")) continue;

		int imgWidth  = defWidth, imgHeight = defHeight;
		Value& images = confImage["images"];

		if (images.IsArray()) {

			for (unsigned i=0;i<images.Size();i++) {
				Value& image = images[i];
				if (image.IsString()) {
					imgFile = image.GetString();
					imgWidth  = defWidth;
					imgHeight = defHeight;
				} else {
					if (!image.HasMember("name")) continue;
					imgWidth = image["width"].GetInt();
					imgHeight = image["height"].GetInt();
					imgFile = image["name"].GetString();			
				}
				if (imgWidth >= mDefWidth && imgHeight>=mDefHeight)
					break;
			}

		} else {
			imgFile = images.GetString();
		}
		
		wchar_t uImgFile[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, imgFile, -1, uImgFile, MAX_PATH);

		KmImage img;
		CString path; 
		FindSkinFile(path, uImgFile, skinName);
		if (!img.Load(path)) 
			continue;
		
		int pos = mImages->AddIcons(img, imgWidth, imgHeight);
		if (pos == -1) continue;
		
		Value& confCmds = confImage["commands"];
		if (!confCmds.IsArray()) continue;

		for (SizeType j=0;j<confCmds.Size();j++) {
			Value& confCmd = confCmds[j];
			if (confCmd.IsArray()) {
				for (SizeType k=0;k<confCmd.Size();k++) {
					mImages->mCmdList[theApp.commands.GetId(confCmd[k].GetString())] = pos+j;
				}
			} else {
				if (confCmd.IsString())
					mImages->mCmdList[theApp.commands.GetId(confCmd.GetString())] = pos+j;
			}
		}		
	}
	if (!mImages->mCmdList.GetSize())
		return false;
	mSkinName = skinName;
	return true;
}

class iconSkinObserver: public IImageObserver {
	UINT mID;
	RECT mRegion;

public:
	iconSkinObserver(UINT id): mID (id), mRegion(CRect(0,0,0,0)) {}
	iconSkinObserver(UINT id, RECT r): mID (id), mRegion(r) {}
	~iconSkinObserver() {}
	void ImageLoaded(HBITMAP hBitmap) 
	{
		if (!hBitmap) return;
		UINT w = theApp.skin.GetUserWidth();
		UINT h = theApp.skin.GetUserHeight();

		KmImage img;
		img.LoadFromBitmap(hBitmap);
		if (mRegion.bottom != 0 || mRegion.right != 0)
			img.Clip(mRegion);
		img.Resize(w, h);
		theApp.skin.mImages->AddIcon(img, mID);
		DeleteObject(hBitmap);
		theApp.toolbars.Refresh();
	}

protected:
	KmButton* mButton;
};

#include "MozUtils.h"
int KmIconList::AddIcon(LPCTSTR coldImgPath, LPCTSTR hotImgPath, LPCTSTR deadImgPath, UINT id, const LPRECT region) 
{
	if (CString(coldImgPath).Left(6).Compare(L"chrome") == 0) {

		iconSkinObserver* io = new iconSkinObserver(id, region?*region:CRect(0,0,0,0));
		nsCOMPtr<nsIURI> uri;
		NewURI(getter_AddRefs(uri), CStringToNSString(coldImgPath));

		if (!nsImageObserver::LoadImage(io, uri)) {
			delete io;
			return -1;
		}

		return 0;
	} else {

		KmImage img, hotImg, deadImg;
		if (!img.LoadFromSkin(coldImgPath, region))
			return -1;

		LONG w = img.GetWidth();
		LONG h = img.GetHeight();

		// If hot image specified, then 1 image for each state
		if (hotImgPath && *hotImgPath) {
			if (!hotImg.LoadFromSkin(hotImgPath, region))
				hotImg.LoadFromSkin(coldImgPath, region);
			if (!deadImgPath || !deadImg.LoadFromSkin(deadImgPath, region))
				deadImg.LoadFromSkin(coldImgPath, region);
			return AddIcon(img, hotImg, deadImg, id);
		}

		// Single image with all states
		int pos = AddIcons(img, w, h, id);					
		return pos;
	}
}

int KmSkin::AddIcon(LPCTSTR coldImgPath, LPCTSTR hotImgPath, LPCTSTR deadImgPath, UINT id, const LPRECT region) 
{
	return mImages->AddIcon(coldImgPath, hotImgPath, deadImgPath, id, region);
}

