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

/*

Bug:	Changing skin need to delete the iconcache.

*/

#include "stdafx.h"

#include "FavIconList.h"

#include "mfcembed.h"
#include "kmeleon_plugin.h"
#include "MozUtils.h"
#include "KmImage.h"

using namespace mozilla::gfx;

#define FAVICON_CACHE_FILE _T("IconCache.dat")

HBITMAP ResizeIcon32(HDC hDC, HBITMAP hBitmap, int w, int h)
{
	IWICImagingFactory *pImgFac;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImgFac));
	if (!SUCCEEDED(hr)) return NULL;

	IWICBitmap* NewBmp;
	hr = pImgFac->CreateBitmapFromHBITMAP(hBitmap,0,WICBitmapUseAlpha,&NewBmp);
	if (!SUCCEEDED(hr)) return NULL;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	BYTE *pBits;
	HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	
	IWICBitmapScaler* pIScaler;
    hr = pImgFac->CreateBitmapScaler(&pIScaler);
    hr = pIScaler->Initialize(NewBmp,w,h,WICBitmapInterpolationModeFant);

    WICRect rect = {0, 0, w, h};
    hr = pIScaler->CopyPixels(&rect, w * 4, w * h * 4, pBits);
	if (!SUCCEEDED(hr)) return NULL;

	pIScaler->Release();
	NewBmp->Release();
	pImgFac->Release();
	return hbmp;
}

// Used to resize the icon if it's not 16x16
HBITMAP ResizeIcon(HDC hDC, HBITMAP hBitmap, LONG w, LONG h)
{
	HDC hDCs = CreateCompatibleDC(hDC);
	BITMAP info;
	::GetObject(hBitmap, sizeof(BITMAP), &info);
	HGDIOBJ old = SelectObject(hDCs, hBitmap);
	if (!old) {
		DeleteDC(hDCs);
		return NULL;
	}
	
	HDC hdcScaled = CreateCompatibleDC(hDCs); 
	HBITMAP hbmSized = CreateCompatibleBitmap(hDCs, w, h); 
	HGDIOBJ old2 = SelectObject(hdcScaled, hbmSized);
	SetStretchBltMode(hdcScaled, HALFTONE);
	SetStretchBltMode(hDCs, HALFTONE);
	StretchBlt(hdcScaled,0,0,w,h,hDCs,0,0,info.bmWidth,info.bmHeight,SRCCOPY);
	SelectObject(hdcScaled, old2);
	DeleteDC(hdcScaled);
	SelectObject(hDCs, old);
	DeleteDC(hDCs);
	return hbmSized;
}

CFavIconList::CFavIconList()
{
	m_iDefaultIcon = 0;
	m_iOffset = 0;
	int width = ::GetSystemMetrics(SM_CXSMICON);
	int height = ::GetSystemMetrics(SM_CYSMICON);
	mSized.Create(width, height, ILC_COLOR32, 25, 10);
	//mIconObserver = new IconObserver(this);
}

CFavIconList::~CFavIconList()
{
	//WriteCache();
}

BOOL CFavIconList::LoadCache()
{
	CFile iconCache;
	CImageList imgList;
	CImageList imgListSized;
	if (!iconCache.Open(theApp.GetFolder(ProfileFolder) + _T("\\") FAVICON_CACHE_FILE, CFile::modeRead))
		return FALSE;

	if (!theApp.preferences.GetBool("kmeleon.favicons.cached", TRUE)) {
		iconCache.Close();
		DeleteFile(theApp.GetFolder(ProfileFolder) + _T("\\") FAVICON_CACHE_FILE);
	}
	else {
		CArchive ar(&iconCache, CArchive::load );
		TRY
			if (imgList.Read(&ar))
				m_urlMap.Serialize(ar);
		CATCH (CArchiveException, e) {
			if (imgList.m_hImageList) imgList.DeleteImageList();
			return FALSE;
		}
		END_CATCH

		TRY
			imgListSized.Read(&ar);
		CATCH (CArchiveException, e) {
	
		}
		END_CATCH
	}

	if (!imgList.m_hImageList) 
		return FALSE;
	
	for (int i=0; i < imgList.GetImageCount(); i++) {
		HICON tmp = imgList.ExtractIcon(i);
		Add(tmp);		
		if (imgListSized.GetSafeHandle()) {
			HICON stmp = imgListSized.ExtractIcon(i);
			mSized.Add(stmp);
			DestroyIcon(stmp);
		} else
			mSized.Add(tmp);
		DestroyIcon(tmp);
	}

	return TRUE;
}

BOOL CFavIconList::WriteCache()
{
	if (!m_hImageList) 
		return FALSE;

	if (!theApp.preferences.GetBool("kmeleon.favicons.cached", TRUE))
		return FALSE;

	CImageList imgList, imgListSized;
	imgList.Create(this);
	imgListSized.Create(&mSized);

	for (int i=0; i < m_iOffset; i++) {
		imgList.Remove(0);
		imgListSized.Remove(0);
	}

	CFile iconCache;
	if (iconCache.Open(theApp.GetFolder(ProfileFolder) + _T("\\") FAVICON_CACHE_FILE, CFile::modeCreate | CFile::modeWrite))
	{
		CArchive ar(&iconCache, CArchive::store);
		if (imgList.Write(&ar)) {
			m_urlMap.Serialize(ar);
			imgListSized.Write(&ar);
		}
	}
	return TRUE;
}

void CFavIconList::LoadDefaultIcon()
{
	CString szFullPath;

	HICON defaultIcon = theApp.skin.GetIconDefault();
	if (defaultIcon) m_iDefaultIcon = Add(defaultIcon);
	
	// Add can return 0 even if it fails...
	if (GetImageCount()==0)
		m_iDefaultIcon = Add(theApp.GetDefaultIcon());
	
	HICON loadingIcon = theApp.skin.GetIconLoading();
	if (loadingIcon) {
		m_iLoadingIcon = Add(loadingIcon);
		DestroyIcon(loadingIcon);
	}
	
	if (GetImageCount()==1) {
		if (defaultIcon)
			m_iLoadingIcon = Add(defaultIcon);
		else
			m_iLoadingIcon = Add(theApp.GetDefaultIcon());
	}
	
	if (defaultIcon)
		DestroyIcon(defaultIcon);

	mSized.Add(theApp.GetDefaultIcon());
	mSized.Add(theApp.GetDefaultIcon());
	m_iOffset = GetImageCount();
}

BOOL CFavIconList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{
	if (!CImageList::Create(cx,cy,nFlags,nInitial,nGrow))
		return FALSE;

	mWidth = cx,
	mHeight = cy;
	LoadDefaultIcon();
	LoadCache();
	return TRUE;
}

void CFavIconList::AddMap(const char *uri, int index, const char* pageUri)
{
	// This function is called from another thread
	// if the moz image loader is used.
	
	if (index == -1) return;

	USES_CONVERSION;
	m_urlMap[A2CT(uri)] = index - m_iOffset;

	// Add an entry for the hostname only. This is for the mru list.
	// It's not really good to do it like that, because a site may
	// have several different icons.
	nsCOMPtr<nsIURI> URI;
	nsCString nsUri;
	nsUri.Assign(pageUri && pageUri[0] ? pageUri : uri);
	nsresult rv = NewURI(getter_AddRefs(URI), nsUri);
	if (NS_SUCCEEDED(rv)) {
		bool ishttps, ishttp;
		URI->SchemeIs("http", &ishttp);
	    URI->SchemeIs("https", &ishttps);
		if (ishttp || ishttps) {
			URI->GetHost(nsUri);
			nsUri.Insert("http://", 0, 7);
			m_urlMap[A2CT(nsUri.get())] = index - m_iOffset;
		}
	}

	// If it's not really efficient, at least I don't have
	// to mess with synchronisation.
	theApp.BroadcastMessage(UWM_NEWSITEICON, (WPARAM)0, index);
}
/*
int CFavIconList::AddIcon(const char* uri, CBitmap* icon, COLORREF cr, const char* pageUri)
{
	int index = Add(icon, cr);
	AddMap(uri, index, pageUri);
	return index;
}

int CFavIconList::AddIcon(const char* uri, CBitmap* icon, CBitmap* mask, const char* pageUri)
{
	int index = Add(icon, mask);
	AddMap(uri, index, pageUri);
	return index;
}*/

int CFavIconList::AddIcon(const char* uri, HBITMAP hBitmap, const char* pageUri)
{
	BITMAP info;
	::GetObject(hBitmap, sizeof(BITMAP), &info);
	int w,h;
	ImageList_GetIconSize(GetSafeHandle(), &w, &h);
	HBITMAP hbmSized = NULL;
	if (info.bmWidth!=w && info.bmHeight!=h) { 
		HDC hDC = GetDC(NULL);
		hbmSized = ResizeIcon32(hDC, hBitmap, w, h);
		if (!hbmSized) hbmSized = ResizeIcon(hDC, hBitmap, w, h);		
		ReleaseDC(NULL, hDC);
	}
	if (!hbmSized) hbmSized = hBitmap;

	CBitmap bitmap;
	bitmap.Attach(hbmSized);
	int index = Add(&bitmap, (CBitmap*)NULL);
	AddMap(uri, index, pageUri);
	
	HBITMAP hbmSized2 = NULL;
	ImageList_GetIconSize(mSized.GetSafeHandle(), &w, &h);
	if (info.bmWidth!=w && info.bmHeight!=h) { 
		HDC hDC = GetDC(NULL);
		hbmSized2 = ResizeIcon32(hDC, hBitmap, w, h);
		if (!hbmSized) hbmSized2 = ResizeIcon(hDC, hBitmap, w, h);
		ReleaseDC(NULL, hDC);
	}
	if (!hbmSized) hbmSized = hBitmap;

	bitmap.Attach(hbmSized);
	int index2 = mSized.Add(&bitmap, (CBitmap*)NULL);
	ASSERT(index == index2);

	if (hbmSized != hBitmap) DeleteObject(hbmSized);
	if (hbmSized2 != hBitmap) DeleteObject(hbmSized2);
	DeleteObject(hBitmap);
	
	return index;
}

int CFavIconList::GetIcon(const TCHAR* aUrl)
{
	int index = GetDefaultIcon();
	if (m_urlMap.Lookup(aUrl, index))
		return index + m_iOffset;
	return index;
}

int CFavIconList::GetHostIcon(const TCHAR* aUrl)
{
	int index = GetDefaultIcon();

	const char* url;
#ifdef _UNICODE
	nsCString _str;
	NS_UTF16ToCString(nsDependentString(aUrl), NS_CSTRING_ENCODING_UTF8, _str);
	url = _str.get();
#else
	url = aUrl;
#endif

	nsCOMPtr<nsIURI> URI;
	nsCString nsUri;
	nsUri.Assign(url);
	nsresult rv = NewURI(getter_AddRefs(URI), nsUri);
	if (NS_FAILED(rv)||!URI) return index;

	URI->GetHost(nsUri);
	nsUri.Insert("http://", 0, 7);
	USES_CONVERSION;
	if (!m_urlMap.Lookup(A2CT(nsUri.get()), index))
		return index;

	return index + m_iOffset;
}

int CFavIconList::GetIcon(nsIURI *aURI, nsIURI* aPageURI, BOOL download)
{
	int index = GetDefaultIcon();

	if  (!m_hImageList || !aURI) return 0;

	nsCString nsUri;
	aURI->GetSpec(nsUri);
	USES_CONVERSION;
	if (m_urlMap.Lookup(A2CT(nsUri.get()), index))
		return index + m_iOffset;
	
	if (download) 
		DwnFavIcon(aURI, aPageURI);
	return GetDefaultIcon();
}

void CFavIconList::RefreshIcon(nsIURI* aURI)
{
	if (!m_hImageList || !aURI) 
		return;
	
	int index = 0;
	nsCString nsUri;
	aURI->GetSpec(nsUri);
	
   USES_CONVERSION;
   const TCHAR* url = A2CT(nsUri.get());
	if (!m_urlMap.Lookup(url, index))
		return;
	
	m_urlMap.RemoveKey(url);
	
	// Don't remove the default icon
	if (index==GetDefaultIcon()) 
		return;

	Remove(index + m_iOffset);
	mSized.Remove(index + m_iOffset);
	
   // We have to remove all url with this icon.
   POSITION pos = m_urlMap.GetStartPosition();
   while (pos!=NULL)
	{
		CString key;
		int value;
		m_urlMap.GetNextAssoc(pos, key, value);
		if (value==index) 
			m_urlMap.RemoveKey(key);
		else
			if (value>index)
				m_urlMap[key] = value-1;

	}
	theApp.BroadcastMessage(UWM_NEWSITEICON, 0, -1);
}

void CFavIconList::ResetCache()
{
	while (GetImageCount())	Remove(0);
	while (mSized.GetImageCount())	mSized.Remove(0);
	m_urlMap.RemoveAll();
	LoadDefaultIcon();
	theApp.BroadcastMessage(UWM_NEWSITEICON, 0, -1);
}


class iconObserver: public IImageObserver {
public:
	iconObserver(CFavIconList* favList, nsIURI* icon, nsIURI* page) : 
		mFavList(favList), mIconURI(icon), mPageURI(page) {}

	void ImageLoaded(HBITMAP hBitmap) 
	{
		nsCString nsuri;
		mIconURI->GetSpec(nsuri);
		nsCString nsPageuri;
		if (mPageURI) mPageURI->GetSpec(nsPageuri);
		mFavList->AddIcon(nsuri.get(), hBitmap, nsPageuri.get());
	}

protected:
	nsCOMPtr<nsIURI> mIconURI;
	nsCOMPtr<nsIURI> mPageURI;
	CFavIconList* mFavList;
};

BOOL CFavIconList::DwnFavIcon(nsIURI* iconURI, nsIURI* pageURI)
{
	iconObserver* io = new iconObserver(this, iconURI, pageURI);

	// Add fragment for wanted size
	nsCString spec;
	iconURI->GetSpec(spec);
	spec.Append("#-moz-resolution=");
	char tmp[34];
	itoa(mWidth, tmp, 10);
	spec.Append(tmp);
	spec.Append(",");
	itoa(mHeight, tmp, 10);
	spec.Append(tmp);
	
	nsCOMPtr<nsIURI> uri;
	NewURI(getter_AddRefs(uri), spec);

	if (!nsImageObserver::LoadImage(io, uri)) {
		delete io;
		return FALSE;
	}
	return TRUE;
}