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

#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"

using namespace mozilla::gfx;

#define FAVICON_CACHE_FILE _T("IconCache.dat")

class iconCallback: public nsIFaviconDataCallback {

public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIFAVICONDATACALLBACK

	iconCallback(CFavIconList* favList, nsIURI* icon, nsIURI* page) : 
		mFavList(favList), mIconURI(icon), mPageURI(page) {}

private:
	~iconCallback() {};

protected:
	nsCOMPtr<nsIURI> mIconURI;
	nsCOMPtr<nsIURI> mPageURI;
	CFavIconList* mFavList;
};

NS_IMPL_ISUPPORTS(iconCallback, nsIFaviconDataCallback)

NS_IMETHODIMP iconCallback::OnComplete(nsIURI *aFaviconURI, uint32_t aDataLen, const uint8_t *aData, const nsACString & aMimeType)
{
	if (mPageURI) {
		nsCOMPtr<nsIURI> hostUri;
		mPageURI->Clone(getter_AddRefs(hostUri));
		hostUri->SetPath(NS_LITERAL_CSTRING(""));
		nsCOMPtr<mozIAsyncFavicons> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
		fis->SetAndFetchFaviconForPage(hostUri, aFaviconURI, false, nsIFaviconService::FAVICON_LOAD_NON_PRIVATE, nullptr);
	}

	/*
	nsCString nsuri;
	mIconURI->GetSpec(nsuri);
	nsCString nsPageuri;
	if (mPageURI) mPageURI->GetSpec(nsPageuri);

	if (!aDataLen) {
		if (mPageURI) {
			mFavList->AddIcon(nsuri.get(), NULL, nsPageuri.get());
		}
		return NS_OK;
	}

	CComPtr<IStream> stream;
	stream.Attach(SHCreateMemStream(aData, aDataLen));
	KmImage img;
	img.Load(stream);
	mFavList->AddIcon(nsuri.get(), &img, nsPageuri.get());*/
	return NS_OK;
}

CFavIconList::CFavIconList()
{
	m_iDefaultIcon = 0;
	m_iOffset = 0;
	int width = ::GetSystemMetrics(SM_CXSMICON);
	int height = ::GetSystemMetrics(SM_CYSMICON);
	mSized.Create(width, height, ILC_COLOR32|ILC_MASK, 25, 10);
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
	mIconService = nullptr;
	return TRUE;

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

	HICON icon = theApp.skin.LoadSkinIcon(_T("default.ico"), 0, theApp.skin.GetDefWidth(), theApp.skin.GetDefHeight());
	if (icon) {
		mSized.Add(icon);
		mSized.Add(icon);
		DestroyIcon(icon);
	}
	else {
		mSized.Add(theApp.GetDefaultIcon());
		mSized.Add(theApp.GetDefaultIcon());
	}
	m_iOffset = GetImageCount();
}

BOOL CFavIconList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{
	if (!CImageList::Create(cx,cy,nFlags,nInitial,nGrow))
		return FALSE;

	mWidth = cx,
		mHeight = cy;
	LoadDefaultIcon();
	//LoadCache();
	return TRUE;
}

void CFavIconList::AddMap(const char *uri, int index, const char* pageUri)
{
	// This function is called from another thread
	// if the moz image loader is used.

	if (index == -1) return;

	USES_CONVERSION;
	m_urlMap[A2CT(uri)] = index - m_iOffset;

	// If it's not really efficient, at least I don't have
	// to mess with synchronisation.
	theApp.BroadcastMessage(UWM_NEWSITEICON, (WPARAM)0, index);
}

int CFavIconList::AddIcon(const char* uri, KmImage* img, const char* pageUri)
{
	if (!img || !img->IsValid()) {
		AddMap(uri, GetDefaultIcon(), pageUri);
		return GetDefaultIcon();
	}

	HBITMAP hBitmap = NULL;
	int w,h;
	ImageList_GetIconSize(GetSafeHandle(), &w, &h);
	KmImage imgSized;	
	if (img->GetWidth()!=w || img->GetHeight()!=h) { 
		img->Resize(w, h, imgSized);
		hBitmap = imgSized.GetHBitmap();
	} else 
		hBitmap = img->GetHBitmap();

	CBitmap bitmap;
	bitmap.Attach(hBitmap);
	int index = Add(&bitmap, (CBitmap*)NULL);
	if (index == -1) return -m_iOffset;
	AddMap(uri, index, pageUri);

	ImageList_GetIconSize(mSized.GetSafeHandle(), &w, &h);
	if (img->GetWidth()!=w || img->GetHeight()!=h) { 
		img->Resize(w, h, imgSized);
		hBitmap = imgSized.GetHBitmap();
	} else 
		hBitmap = img->GetHBitmap();

	bitmap.Detach();
	bitmap.Attach(hBitmap);
	int index2 = mSized.Add(&bitmap, (CBitmap*)NULL);
	ASSERT(index == index2);
	return index;
}

bool CFavIconList::GetFaviconForPage(nsIURI* aPageURI, nsIURI** _retval)
{
	nsCOMPtr<nsIFaviconService> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
	if (!fis) return false;

	nsCOMPtr<nsIURI> faviconURI;
	nsresult rv = fis->GetFaviconForPage(aPageURI, getter_AddRefs(faviconURI));
	if (NS_FAILED(rv)) faviconURI = aPageURI;

	NS_IF_ADDREF(*_retval = faviconURI);
	return true;
}

int CFavIconList::GetIconForPage(const TCHAR* aUrl)
{
	int index = GetDefaultIcon();

	nsCOMPtr<nsIFaviconService> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
	if (!fis) return index;

	nsCOMPtr<nsIURI> URI;
	nsCString spec = CStringToNSCString(aUrl);
	nsresult rv = NewURI(getter_AddRefs(URI), spec);
	if (NS_FAILED(rv)||!URI) return index;

	nsCOMPtr<nsIURI> faviconURI;
	rv = fis->GetFaviconForPage(URI, getter_AddRefs(faviconURI));
	if (NS_FAILED(rv)) faviconURI = URI;
	//NS_ENSURE_SUCCESS(rv, index);

	faviconURI->GetSpec(spec);
	if (m_urlMap.Lookup(NSCStringToCString(spec), index))
		return index + m_iOffset;

	uint32_t dataLen = 0;
	uint8_t* data = 0;
	nsCString mime;
	rv = fis->GetFaviconData(faviconURI, mime, &dataLen, &data);
	if (!NS_SUCCEEDED(rv) || !dataLen)
		return index;//AddIcon(spec.get(), NULL, nullptr);	

	CComPtr<IStream> stream;
	stream.Attach(SHCreateMemStream(data, dataLen));
	KmImage img;
	img.Load(stream);
	return AddIcon(spec.get(), &img, nullptr);	
}

int CFavIconList::GetIcon(nsIURI *aURI, nsIURI* aPageURI, BOOL download)
{
	int index = GetDefaultIcon();

	if  (!m_hImageList || !aURI) return 0;

	nsCString nsUri;
	aURI->GetSpec(nsUri);
	USES_CONVERSION;

	if (download) {
		nsCOMPtr<iconCallback> ic = new iconCallback(this, aURI, aPageURI);
		nsCOMPtr<mozIAsyncFavicons> afis = GetIconService();
		if (!afis) return FALSE;
		nsresult rv = afis->SetAndFetchFaviconForPage(aPageURI, aURI, false, nsIFaviconService::FAVICON_LOAD_NON_PRIVATE, ic);
	}

	if (m_urlMap.Lookup(A2CT(nsUri.get()), index)) {
		if (index != GetDefaultIcon())
			return index + m_iOffset;
	}

	nsCOMPtr<nsIFaviconService> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
	if (fis) {
		bool failed = false;
		fis->IsFailedFavicon(aURI, &failed);
		if (failed) return GetDefaultIcon();
	}

	// Fetch the icon on our own. SetAndFetchFaviconForPage often
	// dont call the callback ...
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

	  void ImageLoaded(KmImage &bmp) 
	  {
		  nsCString nsuri;
		  mIconURI->GetSpec(nsuri);
		  nsCString nsPageuri;
		  if (mPageURI) mPageURI->GetSpec(nsPageuri);
		  mFavList->AddIcon(nsuri.get(), &bmp, nsPageuri.get());
	  }

protected:
	nsCOMPtr<nsIURI> mIconURI;
	nsCOMPtr<nsIURI> mPageURI;
	CFavIconList* mFavList;
};



int CFavIconList::GetFavIcon(nsIURI* iconURI)
{
	nsCOMPtr<nsIFaviconService> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
	if (!fis) return 0;

	nsCOMPtr<nsIURI> faviconURI;
	if (NS_SUCCEEDED(fis->GetFaviconForPage(iconURI, getter_AddRefs(faviconURI))))
		iconURI = faviconURI;

	uint32_t dataLen = 0;
	uint8_t* data = 0;
	nsCString mime;
	nsCString spec;
	iconURI->GetSpec(spec);
	nsresult rv = fis->GetFaviconData(iconURI, mime, &dataLen, &data);
	if (!NS_SUCCEEDED(rv) || !dataLen)
		return GetDefaultIcon();//AddIcon(spec.get(), NULL, nullptr);	

	CComPtr<IStream> stream;
	stream.Attach(SHCreateMemStream(data, dataLen));
	KmImage img;
	img.Load(stream);
	return AddIcon(spec.get(), &img, nullptr);

	//iconCallback* ic = new iconCallback(this, iconURI, nullptr);
	nsCOMPtr<mozIAsyncFavicons> afis = GetIconService();
	if (!afis) return FALSE;
	rv = afis->GetFaviconDataForPage(iconURI, nullptr);
	return NS_SUCCEEDED(rv);
}

bool CFavIconList::DwnFavIcon(nsIURI* iconURI, nsIURI* pageURI, bool reload)
{
	nsCString scheme;
	iconURI->GetScheme(scheme);

	iconObserver* io = new iconObserver(this, iconURI, pageURI);
	return kImageObserver::LoadImage(io, iconURI);
}

mozIAsyncFavicons* CFavIconList::GetIconService() 
{
	if (!mIconService) {
		nsCOMPtr<nsIFaviconService> fis = do_GetService("@mozilla.org/browser/favicon-service;1");
		mIconService = do_QueryInterface(fis);
	}
	return mIconService;
}