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

#ifdef INTERNAL_SITEICONS

#include "FavIconList.h"

#include "UnknownContentTypeHandler.h"
#include "nscWebBrowserPersist.h"

#include "imgILoader.h"
#include "gfxIImageFrame.h"
#include "imgIContainer.h"
#include "nsIImage.h"

#include "mfcembed.h"
extern CMfcEmbedApp theApp;

#ifdef PNG_SUPPORT
#define PNGDIB_NO_D2P
#define PNGDIB_S
#include "../pngdib-3.0.1/pngdib.h"

#ifdef _DEBUG
#pragma comment(lib,"../pngdib-3.0.1/lib/pngdib.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/libpng.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/zlib/zlib.lib")
#else
#ifdef _UNICODE
#pragma comment(lib,"../pngdib-3.0.1/lib/pngdib_u_s_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/libpng_u_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/zlib/zlib_u_md.lib")
#else
#pragma comment(lib,"../pngdib-3.0.1/lib/pngdib_s_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/libpng_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/zlib/zlib_md.lib")
#endif
#endif // _DEBUG
#endif 

#define FAVICON_CACHE_FILE _T("IconCache.dat")

// Used to resize the icon if it's not 16x16
HBITMAP ResizeIcon(HDC hDC, HBITMAP hBitmap, LONG w, LONG h)
{
	HDC hDCs = CreateCompatibleDC(hDC);

	HGDIOBJ old = SelectObject(hDCs, hBitmap);
	if (!old) {
		DeleteDC(hDCs);
		return NULL;
	}
	
	HDC hdcScaled = CreateCompatibleDC(hDCs); 
	HBITMAP hbmSized = CreateCompatibleBitmap(hDCs, 16, 16); 
	HGDIOBJ old2 = SelectObject(hdcScaled, hbmSized);
	
	StretchBlt(hdcScaled,0,0,16,16,hDCs,0,0,w,h,SRCCOPY);
	SelectObject(hdcScaled, old2);
	DeleteDC(hdcScaled);
	SelectObject(hDCs, old);
	DeleteDC(hDCs);
	return hbmSized;
}

CFavIconList::CFavIconList()
{
	m_iDefaultIcon = 0;
}

CFavIconList::~CFavIconList()
{
	if (!m_hImageList) 
		return;

	if (theApp.preferences.GetBool("kmeleon.favicons.cached", true))
	{
		// Save the icons
		CFile iconCache;
		if (iconCache.Open(theApp.preferences.settingsDir + FAVICON_CACHE_FILE, CFile::modeCreate | CFile::modeWrite))
		{
			CArchive ar(&iconCache, CArchive::store);
			if (Write(&ar))
				m_urlMap.Serialize(ar);
		}
	}
}

void CFavIconList::LoadDefaultIcon()
{
	CString szFullPath;

	if (theApp.FindSkinFile(szFullPath, _T("default.ico")))
	{
		FILE *fp = _tfopen(szFullPath, _T("r"));
		if (fp) {
			fclose(fp);
			HICON defaultIcon = (HICON)LoadImage(NULL, szFullPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
			if (defaultIcon) {
				m_iDefaultIcon = Add(defaultIcon);
				DestroyIcon(defaultIcon);
			}
		}

	}

	// Add can return 0 even if it fails...
	if (GetImageCount()==0)
		m_iDefaultIcon = Add(theApp.GetDefaultIcon());
}

BOOL CFavIconList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{

	// Try to load the icon cache
	CFile   iconCache;
	if (iconCache.Open(theApp.preferences.settingsDir + FAVICON_CACHE_FILE, CFile::modeRead))
	{
		if (!theApp.preferences.GetBool("kmeleon.favicons.cached", true)) {
			iconCache.Close();
			DeleteFile(theApp.preferences.settingsDir + FAVICON_CACHE_FILE);
		}
		else
		{
			CArchive ar(&iconCache, CArchive::load );
			if (Read(&ar)) {
				m_urlMap.Serialize(ar);
			}
		}
	}

	if (m_hImageList) return TRUE;
	
	// If no cache, create the list
	if (!CImageList::Create(cx,cy,nFlags,nInitial,nGrow))
		return FALSE;

	// Load the default icon
	LoadDefaultIcon();

	return TRUE;
}

extern nsresult NewURI(nsIURI **result, const nsAString &spec);
extern nsresult NewURI(nsIURI **result, const nsACString &spec);

void CFavIconList::AddMap(const char *uri, int index)
{
	// This function is called from another thread
	// if the moz image loader is used.

	m_urlMap[A2CT(uri)] = index;

	// Add an entry for the hostname only. This is for the mru list.
	// It's not really good to do it like that, because a site may
	// have several different icons.
	nsCOMPtr<nsIURI> URI;
	nsEmbedCString nsUri;
	nsUri.Assign(uri);
	nsresult rv = NewURI(getter_AddRefs(URI), nsUri);
	if (NS_SUCCEEDED(rv)) {
		URI->GetHost(nsUri);
		nsUri.Insert("http://", 0, 7);
		m_urlMap[A2CT(nsUri.get())] = index;
	}

	// If it's not really efficient, at least I don't have
	// to mess with synchronisation.
	theApp.BroadcastMessage(UWM_NEWSITEICON, (WPARAM)uri, index);
}

int CFavIconList::AddIcon(const char* uri, CBitmap* icon, COLORREF cr)
{
	int index = Add(icon, cr);
	AddMap(uri, index);
	return index;
}

int CFavIconList::AddIcon(const char* uri, CBitmap* icon, CBitmap* mask)
{
	int index = Add(icon, mask);
	AddMap(uri, index);
	return index;
}

int CFavIconList::AddIcon(const char* uri, HICON icon)
{
	int index = Add(icon);
	AddMap(uri, index);
	return index;
}

int CFavIconList::AddDownloadedIcon(char* uri, TCHAR* file, nsresult aStatus)
{
	int index = GetDefaultIcon();

	if (NS_SUCCEEDED(aStatus))
	{
		HICON favicon = (HICON)LoadImage(NULL, file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		if (favicon){
			index = AddIcon(uri, favicon);
			DestroyIcon(favicon);
		}
		#ifdef PNG_SUPPORT
		else
		{
			PNGDIB *pngdib = pngdib_p2d_init();
			if (pngdib)
			{
				int errcode;
				LPBITMAPINFOHEADER pdib;
				int dib_size;
				void *pdib_bits;
				int bits_offs;

				pngdib_p2d_set_png_filename(pngdib, file);
				pngdib_p2d_set_custom_bg(pngdib, 192, 192, 192);
				
				//pngdib_set_dibalpha32(pngdib, 1); // Can be used with Comctl32 Ver. 6 only

				pngdib_p2d_set_use_file_bg(pngdib, 1);
				errcode=pngdib_p2d_run(pngdib);
				if(!errcode) {
					pngdib_p2d_get_dib(pngdib,&pdib,&dib_size);
					pngdib_p2d_get_dibbits(pngdib,&pdib_bits,&bits_offs,NULL);

					HDC hDC = GetDC(NULL);
					HBITMAP hBitmap = ::CreateDIBitmap(hDC,pdib,CBM_INIT,pdib_bits,(BITMAPINFO*)pdib,DIB_RGB_COLORS);
					ReleaseDC(NULL,hDC);
					
					CBitmap bitmap;
					bitmap.Attach(hBitmap);

					unsigned char pr,pg,pb;
					int r = pngdib_p2d_get_bgcolor(pngdib, &pr, &pg, &pb);
					if (r)
						index = AddIcon(uri, &bitmap,RGB(pr,pg,pb));
					else
						index = AddIcon(uri, &bitmap, (CBitmap*)NULL);
					
					bitmap.DeleteObject();

					pngdib_p2d_free_dib(pngdib,NULL);
				}
				pngdib_done(pngdib);
			}

		}
		#endif
	}
	DeleteFile(file);
	return index;
}

int CFavIconList::GetHostIcon(const TCHAR* aUrl)
{
	int index = GetDefaultIcon();
	const char* url;
#ifdef _UNICODE
	nsEmbedCString _str;
	NS_UTF16ToCString(nsDependentString(aUrl), NS_CSTRING_ENCODING_UTF8, _str);
	url = _str.get();
#else
	url = aUrl;
#endif

	nsCOMPtr<nsIURI> URI;
	nsEmbedCString nsUri;
	nsUri.Assign(url);
	nsresult rv = NewURI(getter_AddRefs(URI), nsUri);
	if (NS_FAILED(rv)||!URI) return index;

	URI->GetHost(nsUri);
	nsUri.Insert("http://", 0, 7);
	m_urlMap.Lookup(A2CT(nsUri.get()), index);

	return index;
}

int CFavIconList::GetIcon(nsIURI *aURI, BOOL download)
{
	int index = GetDefaultIcon();

	if  (!m_hImageList || !aURI) return 0;

	nsEmbedCString nsUri;
	aURI->GetSpec(nsUri);
	if (!m_urlMap.Lookup(A2CT(nsUri.get()), index))
	{
		if (download) 
			DwnFavIcon(aURI);
		return GetDefaultIcon();
	}

	return index;
}

void CFavIconList::RefreshIcon(nsIURI* aURI)
{
	if (!m_hImageList || !aURI) 
		return;
	
	int index = 0;
	nsEmbedCString nsUri;
	aURI->GetSpec(nsUri);
	
	if (!m_urlMap.Lookup(A2CT(nsUri.get()), index))
		return;
	
	m_urlMap.RemoveKey(A2CT(nsUri.get()));
	
	// Don't remove the default icon
	if (index==GetDefaultIcon()) 
		return;

	Remove(index);
	
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
	m_urlMap.RemoveAll();
	LoadDefaultIcon();
	theApp.BroadcastMessage(UWM_NEWSITEICON, 0, -1);
}

BOOL CFavIconList::DwnFavIcon(nsIURI* iconURI)
{
	nsresult rv;
#ifndef PNG_SUPPORT

	// Borked way to get the favicon.
	imgIRequest* request = nsnull;
	IconObserver* observer = new IconObserver(this);
	NS_ADDREF(observer);

	if (NS_FAILED(observer->LoadIcon(iconURI, nsnull))) {
		NS_RELEASE(observer);
		return FALSE;
	}
#else

	// Currently the favicon is downloaded like any other file 
	// which is bad. A nsStreamListener have to be 
	// implemented.
	nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
	if(!persist) return FALSE;

	TCHAR tempPath[MAX_PATH];
	GetTempPath(MAX_PATH, tempPath);
	GetTempFileName(tempPath, _T("kme"), 0, tempPath);  

	nsCOMPtr<nsILocalFile> file;
#ifdef _UNICODE
	NS_NewLocalFile(nsDependentString(tempPath), TRUE, getter_AddRefs(file));
#else
	NS_NewNativeLocalFile(nsDependentCString(tempPath), TRUE, getter_AddRefs(file));
#endif

	CProgressDialog *progress = new CProgressDialog(FALSE);
	//persist->SetProgressListener(progress);
	progress->InitPersist(iconURI, file, persist, FALSE); 
	progress->SetCallBack(CFavIconList::DwnCall, this);
	persist->SetPersistFlags(
		nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
		nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);
	rv = persist->SaveURI(iconURI, nsnull, nsnull, nsnull, nsnull, file);
	if (NS_FAILED(rv)) {
		persist->SetProgressListener(nsnull);
		return FALSE;
	}
#endif

	return TRUE;
}

void CFavIconList::DwnCall(char* uri, TCHAR* file, nsresult status, void* param)
{
	((CFavIconList*)param)->AddDownloadedIcon(uri,file,status);
}

NS_IMPL_ISUPPORTS2(IconObserver, imgIDecoderObserver, imgIContainerObserver)

/* void onStartDecode (in imgIRequest aRequest); */
NS_IMETHODIMP IconObserver::OnStartDecode(imgIRequest *aRequest)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStartContainer (in imgIRequest aRequest, in imgIContainer aContainer); */
NS_IMETHODIMP IconObserver::OnStartContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStartFrame (in imgIRequest aRequest, in gfxIImageFrame aFrame); */
NS_IMETHODIMP IconObserver::OnStartFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void onDataAvailable (in imgIRequest aRequest, in gfxIImageFrame aFrame, [const] in nsIntRect aRect); */
NS_IMETHODIMP IconObserver::OnDataAvailable(imgIRequest *aRequest, gfxIImageFrame *aFrame, const nsIntRect * aRect)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStopFrame (in imgIRequest aRequest, in gfxIImageFrame aFrame); */
NS_IMETHODIMP IconObserver::OnStopFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStopContainer (in imgIRequest aRequest, in imgIContainer aContainer); */
NS_IMETHODIMP IconObserver::OnStopContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onStopDecode (in imgIRequest aRequest, in nsresult status, in wstring statusArg); */
NS_IMETHODIMP IconObserver::OnStopDecode(imgIRequest *aRequest, nsresult status, const PRUnichar *statusArg)
{
	if (NS_SUCCEEDED(status))
		CreateDIB(aRequest);
			
	aRequest->Cancel(status);
	mRequest = nsnull;
	NS_RELEASE_THIS();
    return NS_OK;
}

struct ALPHABITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;
  RGBQUAD           bmiColors[256];


  ALPHABITMAPINFO(LONG aWidth, LONG aHeight, WORD depth)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = depth;

	
    /* fill in gray scale palette */
     int i, npal=1<<depth; // 2^depth
	 BYTE fact = 255/(npal-1); 
     for(i=0; i < npal; i++){
	  BYTE color = 255-i*fact;
      bmiColors[i].rgbBlue = color;
      bmiColors[i].rgbGreen = color;
      bmiColors[i].rgbRed = color;
      bmiColors[i].rgbReserved = 0;
     }
  }
};

NS_IMETHODIMP IconObserver::CreateDIB(imgIRequest *aRequest)
{
	nsresult rv;
	nsCOMPtr<imgIContainer> image;
	rv = aRequest->GetImage(getter_AddRefs(image));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<gfxIImageFrame> frame;
	rv = image->GetFrameAt(0, getter_AddRefs(frame));
	NS_ENSURE_SUCCESS(rv, rv);

	PRUint32 length;
	PRUint8 *bits;
	rv = frame->GetImageData(&bits, &length);
	NS_ENSURE_SUCCESS(rv, rv);

	PRInt32 w,h;
	frame->GetWidth(&w);
	frame->GetHeight(&h);

	PRUint32 bpr;
	frame->GetImageBytesPerRow(&bpr);

	/*	PRUint32 alphaLength;
	PRUint8 *alphaBits;

	frame->GetAlphaData(&alphaBits, &alphaLength);

	PRUint32 alphaBpr;
	frame->GetAlphaBytesPerRow(&alphaBpr);

	PRUint32* fBits = new PRUint32[w*h];
	PRUint32 offset = 0; 
	for (int y = 0; y < w*h; y++) {
	fBits[y] = alphaBits[y];
	fBits[y] = (fBits[y]<<8) + bits[offset+2];
	fBits[y] = (fBits[y]<<8) + bits[offset+1];
	fBits[y] = (fBits[y]<<8) + bits[offset];
	offset +=3;
	}*/

	CBitmap bitmap;
	BITMAPINFO binfo;
	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo.bmiHeader.biWidth = w;
	binfo.bmiHeader.biHeight = h;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biBitCount = (bpr/w)*8;
	binfo.bmiHeader.biCompression = BI_RGB;
	binfo.bmiHeader.biSizeImage = length;     
	binfo.bmiHeader.biXPelsPerMeter = 0;
	binfo.bmiHeader.biYPelsPerMeter = 0;
	binfo.bmiHeader.biClrUsed = 0;
	binfo.bmiHeader.biClrImportant = 0;

	HDC hDC = GetDC(NULL);
	HBITMAP hBitmap = ::CreateDIBitmap(hDC,&binfo.bmiHeader,CBM_INIT,bits,&binfo,DIB_RGB_COLORS);

	if (w!=16 && h!=16) {
		if (HBITMAP hbmSized = ResizeIcon(hDC, hBitmap, w, h)) {
			DeleteObject(hBitmap);
			hBitmap = hbmSized;
		}
	}

	ReleaseDC(NULL,hDC);
	bitmap.Attach(hBitmap);

	CString uri;
	nsCOMPtr<nsIURI> URI;
	nsEmbedCString nsuri;
	rv = aRequest->GetURI(getter_AddRefs(URI));
	NS_ENSURE_SUCCESS(rv, rv);
	URI->GetSpec(nsuri);

	gfx_color bkgColor = 0;
	rv = frame->GetBackgroundColor(&bkgColor);
	if (NS_FAILED(frame->GetBackgroundColor(&bkgColor)))
	{
		CBitmap maskbitmap;
		PRUint32 alphaLength;
		PRUint8 *alphaBits;

		if (NS_SUCCEEDED(frame->GetAlphaData(&alphaBits, &alphaLength)))
		{
			PRUint32 alphaBpr;
			frame->GetAlphaBytesPerRow(&alphaBpr); // Return a false value??
			alphaBpr = (8 * alphaLength) / (w*h);

			if (alphaBpr<=8) {

				// XXX: if alphaBpr == 2, must convert to 4
				ALPHABITMAPINFO binfo(w, h, alphaBpr);

				HDC hDC = GetDC(NULL);
				HBITMAP hBitmap = ::CreateDIBitmap(hDC,&binfo.bmiHeader,CBM_INIT,alphaBits,(BITMAPINFO*)&binfo,DIB_RGB_COLORS);
				if (w!=16 && h!=16) {
					if (HBITMAP hbmSized = ResizeIcon(hDC, hBitmap, w, h)) {
						DeleteObject(hBitmap);
						hBitmap = hbmSized;
					}
				}
				ReleaseDC(NULL,hDC);

				maskbitmap.Attach(hBitmap);
			}
		}

		mFavList->AddIcon(nsuri.get(),&bitmap, &maskbitmap);
		maskbitmap.DeleteObject();
	} else {
		mFavList->AddIcon(nsuri.get(),&bitmap, bkgColor);
	}

	bitmap.DeleteObject();
	return NS_OK;
}

NS_IMETHODIMP IconObserver::LoadIcon(nsIURI *iconUri, nsIURI* pageUri)
{
	nsresult rv;
	nsCOMPtr<imgILoader> loader = do_GetService("@mozilla.org/image/loader;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	
	return loader->LoadImage(iconUri, pageUri, nsnull, 
		nsnull, this, this, nsIRequest::LOAD_BYPASS_CACHE, 
		nsnull, nsnull, getter_AddRefs(mRequest));
}

NS_IMETHODIMP IconObserver::FrameChanged(imgIContainer *aContainer, gfxIImageFrame *aFrame, nsIntRect * aDirtyRect)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

#endif // INTERNAL_SITEICONS