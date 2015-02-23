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

#include "imgIRequest.h"
#include "imgINotificationObserver.h"
#include "imgILoader.h"

interface IImageObserver {
	virtual void ImageLoaded(HBITMAP) = 0;
};

class nsImageObserver : public imgINotificationObserver, public nsSupportsWeakReference
{
	NS_DECL_ISUPPORTS
	NS_DECL_IMGINOTIFICATIONOBSERVER

	nsImageObserver(IImageObserver* observer) : mObserver(observer), mNeedRelease(false) {}
	virtual ~nsImageObserver() { }

	static bool LoadImage(IImageObserver* observer, nsIURI* imgUri) 
	{
		nsresult rv;
		nsCOMPtr<imgILoader> loader = do_GetService("@mozilla.org/image/loader;1", &rv);
		NS_ENSURE_SUCCESS(rv, false);

		nsImageObserver* obs = new nsImageObserver(observer);
		return NS_SUCCEEDED(loader->LoadImageXPCOM(imgUri, nullptr, nullptr, 
			nullptr, nullptr, obs, nullptr, nsIRequest::LOAD_BYPASS_CACHE, 
			nullptr, nullptr, getter_AddRefs(obs->mRequest)));
	}

protected:
	IImageObserver* mObserver;
	nsCOMPtr<imgIRequest> mRequest;
	HBITMAP CreateDIB(imgIRequest *aRequest);
	bool mNeedRelease;
};

class KmImage
{
protected:
	COLORREF mTrColor;
	CBitmap mBitmap;
	Gdiplus::Bitmap* mGdiBitmap;
	HBITMAP _Crop(UINT w, UINT h, UINT index);
	
	static ULONG_PTR mGdiToken;
	static UINT mGdiCount;

	void Clean() {
		mBitmap.DeleteObject();
		if (mGdiBitmap) delete mGdiBitmap;
		mGdiBitmap = nullptr;
	}
	bool InitGdiplus() {
		if (mGdiToken) return true;
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartupOutput output;
		Gdiplus::Status status = Gdiplus::GdiplusStartup( &mGdiToken, &input, &output );
		return status == Gdiplus::Ok;
	}
	bool MakeTransparent(COLORREF);
	Gdiplus::Bitmap* _Resize(UINT w, UINT h) const;

public:
	KmImage() : mTrColor(RGB(255,0,255)), mGdiBitmap(0) {
		mGdiCount++;
		InitGdiplus();
	};

	bool Load(LPCTSTR path);
	bool Crop(UINT w, UINT h, UINT index);
	bool Clip(const RECT& r);
	bool CropLine(UINT h, UINT line, KmImage& img) const;
	bool Resize(UINT w, UINT h);
	bool Resize(UINT w, UINT h, KmImage& img) const;
	bool Scale(float f);
	bool Scale(float f, KmImage& img) const;
	UINT GetWidth() const;
	UINT GetHeight() const;
	HBITMAP GetHBitmap();
	bool DrawItem(HDC dc, POINT pt, UINT index, UINT line, UINT, UINT);

	bool LoadIndexedFromSkin(LPCTSTR name, UINT w, UINT h);
	bool LoadFromSkin(LPCTSTR name, LPRECT rect = nullptr, bool single = false);
	bool LoadFromBitmap(HBITMAP hbmp, bool reverse = false);
	int AddToImageList(CImageList& list, int index = -1);
	~KmImage() {
		Clean();
		mGdiCount--;
		if (mGdiToken && mGdiCount == 0) {
			Gdiplus::GdiplusShutdown(mGdiToken);
			mGdiToken = 0;
		}
	};
};
