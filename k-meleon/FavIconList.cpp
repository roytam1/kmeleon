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
#include "imgIContainer.h"


#include "mfcembed.h"
#include "kmeleon_plugin.h"


//#define PNG_SUPPORT

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

HBITMAP ResizeIcon32(HDC hDC, HBITMAP hBitmap, LONG w, LONG h)
{
	IWICImagingFactory *pImgFac;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImgFac));
	if (!SUCCEEDED(hr)) return NULL;

	IWICBitmap* NewBmp;
	hr = pImgFac->CreateBitmapFromHBITMAP(hBitmap,0,WICBitmapUseAlpha,&NewBmp);
	if (!SUCCEEDED(hr)) return NULL;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = 16;
	bmi.bmiHeader.biHeight = -16;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	BYTE *pBits;
	HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);

	IWICBitmapScaler* pIScaler;
    hr = pImgFac->CreateBitmapScaler(&pIScaler);
    hr = pIScaler->Initialize(NewBmp,16,16,WICBitmapInterpolationModeFant);

    WICRect rect = {0, 0, 16, 16};
    hr = pIScaler->CopyPixels(&rect, 16 * 4, 16 * 16 * 4, pBits);
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

	HGDIOBJ old = SelectObject(hDCs, hBitmap);
	if (!old) {
		DeleteDC(hDCs);
		return NULL;
	}
	
	HDC hdcScaled = CreateCompatibleDC(hDCs); 
	HBITMAP hbmSized = CreateCompatibleBitmap(hDCs, 16, 16); 
	HGDIOBJ old2 = SelectObject(hdcScaled, hbmSized);
	SetStretchBltMode(hdcScaled, HALFTONE);
	SetStretchBltMode(hDCs, HALFTONE);
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
	m_iOffset = 0;
	//mIconObserver = new IconObserver(this);
}

CFavIconList::~CFavIconList()
{
	WriteCache();
}

BOOL CFavIconList::LoadCache()
{
	CFile iconCache;
	CImageList imgList;
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
	}

	if (!imgList.m_hImageList) 
		return FALSE;
	
	for (int i=0; i < imgList.GetImageCount(); i++) {
		HICON tmp = imgList.ExtractIcon(i);
		int idx = Add(tmp);
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

	CImageList imgList;
	imgList.Create(this);

	for (int i=0; i < m_iOffset; i++)
		imgList.Remove(0);

	CFile iconCache;
	if (iconCache.Open(theApp.GetFolder(ProfileFolder) + _T("\\") FAVICON_CACHE_FILE, CFile::modeCreate | CFile::modeWrite))
	{
		CArchive ar(&iconCache, CArchive::store);
		if (imgList.Write(&ar))
			m_urlMap.Serialize(ar);
	}
	return TRUE;
}

void CFavIconList::LoadDefaultIcon()
{
	CString szFullPath;

	HICON defaultIcon = NULL;
	if (theApp.FindSkinFile(szFullPath, _T("default.ico")))
	{
		FILE *fp = _tfopen(szFullPath, _T("r"));
		if (fp) {
			fclose(fp);
			defaultIcon = (HICON)LoadImage(NULL, szFullPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
			if (defaultIcon) {
				m_iDefaultIcon = Add(defaultIcon);
			}
		}
	}

	// Add can return 0 even if it fails...
	if (GetImageCount()==0)
		m_iDefaultIcon = Add(theApp.GetDefaultIcon());

	if (theApp.FindSkinFile(szFullPath, _T("loading.ico")))
	{
		FILE *fp = _tfopen(szFullPath, _T("r"));
		if (fp) {
			fclose(fp);
			HICON loadingIcon = (HICON)LoadImage(NULL, szFullPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
			if (loadingIcon) {
				m_iLoadingIcon = Add(loadingIcon);
				DestroyIcon(loadingIcon);
			}
		}
	}
	
	if (GetImageCount()==1) {
		if (defaultIcon)
			m_iLoadingIcon = Add(defaultIcon);
		else
			m_iLoadingIcon = Add(theApp.GetDefaultIcon());
	}
	
	if (defaultIcon)
		DestroyIcon(defaultIcon);

/*
	if (theApp.FindSkinFile(szFullPath, _T("loader.bmp")))
	{
		HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if (hBitmap)
		{
			HDC hdcBitmap = CreateCompatibleDC(NULL);
			HGDIOBJ oldBmp = SelectObject(hdcBitmap, hBitmap);			
			HBRUSH hBrush = CreateSolidBrush(RGB(255,0,255));

			int cx, cy;
			ImageList_GetIconSize(m_hImageList, &cx, &cy);
			for (int i=0; i<8; i++)
			{
				CBitmap button;
				HDC hdcButton = CreateCompatibleDC(hdcBitmap);
				HBITMAP hButton = CreateCompatibleBitmap(hdcBitmap, cx, cy);
				HGDIOBJ oldBmp2 = SelectObject(hdcButton, hButton);

				// fill the button with the transparency
				HGDIOBJ oldBrush = SelectObject(hdcButton, hBrush);
				PatBlt(hdcButton, 0, 0, cx, cy, PATCOPY);

				// copy the button from the full bitmap
				BitBlt(hdcButton, 0, 0, cx, cy, hdcBitmap, cx*i, 0, SRCCOPY);
				
				SelectObject(hdcButton, oldBrush);
				SelectObject(hdcButton, oldBmp2);
				DeleteDC(hdcButton);

				CBitmap bmp;
				bmp.Attach(hButton);
				Add(&bmp, RGB(255,0,255));
			}
			
			SelectObject(hdcBitmap, oldBmp);
			DeleteObject(hBrush);
			DeleteDC(hdcBitmap);
			DeleteObject(hBitmap);
		}
	}
*/
	m_iOffset = GetImageCount();
}

BOOL CFavIconList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{
	if (!CImageList::Create(cx,cy,nFlags,nInitial,nGrow))
		return FALSE;

	LoadDefaultIcon();
	LoadCache();
	return TRUE;
}

extern nsresult NewURI(nsIURI **result, const nsAString &spec);
extern nsresult NewURI(nsIURI **result, const nsACString &spec);

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
	nsEmbedCString nsUri;
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
}

int CFavIconList::AddIcon(const char* uri, HICON icon, const char* pageUri)
{
	int index = Add(icon);
	AddMap(uri, index, pageUri);
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
	USES_CONVERSION;
	if (!m_urlMap.Lookup(A2CT(nsUri.get()), index))
		return index;

	return index + m_iOffset;
}

int CFavIconList::GetIcon(nsIURI *aURI, nsIURI* aPageURI, BOOL download)
{
	int index = GetDefaultIcon();

	if  (!m_hImageList || !aURI) return 0;

	nsEmbedCString nsUri;
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
	nsEmbedCString nsUri;
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

BOOL CFavIconList::DwnFavIcon(nsIURI* iconURI, nsIURI* pageURI)
{
#ifndef PNG_SUPPORT
	// Borked way to get the favicon.
	imgIRequest* request = nullptr;

	IconObserver* observer = new IconObserver(this);
	if (NS_FAILED(observer->LoadIcon(iconURI, pageURI))) {
		delete observer;
		return FALSE;
	}
	return TRUE;

	//if (NS_FAILED(mIconObserver->LoadIcon(iconURI, pageURI))) {
//		return FALSE;
//	}
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
	nsresult rv = persist->SaveURI(iconURI, nullptr, nullptr, nullptr, nullptr, file);
	if (NS_FAILED(rv)) {
		persist->SetProgressListener(nullptr);
		return FALSE;
	}
#endif

	return TRUE;
}

void CFavIconList::DwnCall(char* uri, TCHAR* file, nsresult status, void* param)
{
	((CFavIconList*)param)->AddDownloadedIcon(uri,file,status);
}


NS_IMPL_ISUPPORTS1(IconObserver, imgINotificationObserver)
NS_IMETHODIMP IconObserver::Notify(imgIRequest *aProxy, int32_t aType, const nsIntRect *aRect)
{
	if (aType == imgINotificationObserver::LOAD_COMPLETE)
	{		
		if (!NS_SUCCEEDED(aProxy->StartDecoding()))
			aProxy->Cancel(NS_OK);
	}
	else if (aType == imgINotificationObserver::DECODE_COMPLETE)
	{
		CreateDIB(aProxy);
		aProxy->Cancel(NS_OK);
		//mRequest = nullptr;
		//NS_RELEASE_THIS();
	}

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

static HBITMAP DataToBitmap(PRUint8* aImageData,
                            PRUint32 aWidth,
                            PRUint32 aHeight,
                            PRUint32 aDepth)
{
  HDC dc = ::GetDC(NULL);

  if (aDepth == 32) {
    // Alpha channel. We need the new header.
    BITMAPV4HEADER head = { 0 };
    head.bV4Size = sizeof(head);
    head.bV4Width = aWidth;
    head.bV4Height = aHeight;
    head.bV4Planes = 1;
    head.bV4BitCount = aDepth;
    head.bV4V4Compression = BI_BITFIELDS;
    head.bV4SizeImage = 0; // Uncompressed
    head.bV4XPelsPerMeter = 0;
    head.bV4YPelsPerMeter = 0;
    head.bV4ClrUsed = 0;
    head.bV4ClrImportant = 0;

    head.bV4RedMask   = 0x00FF0000;
    head.bV4GreenMask = 0x0000FF00;
    head.bV4BlueMask  = 0x000000FF;
    head.bV4AlphaMask = 0xFF000000;

    HBITMAP bmp = ::CreateDIBitmap(dc,
                                   reinterpret_cast<CONST BITMAPINFOHEADER*>(&head),
                                   CBM_INIT,
                                   aImageData,
                                   reinterpret_cast<CONST BITMAPINFO*>(&head),
                                   DIB_RGB_COLORS);
    ::ReleaseDC(NULL, dc);
    return bmp;
  }

  char reserved_space[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2];
  BITMAPINFOHEADER& head = *(BITMAPINFOHEADER*)reserved_space;

  head.biSize = sizeof(BITMAPINFOHEADER);
  head.biWidth = aWidth;
  head.biHeight = aHeight;
  head.biPlanes = 1;
  head.biBitCount = (WORD)aDepth;
  head.biCompression = BI_RGB;
  head.biSizeImage = 0; // Uncompressed
  head.biXPelsPerMeter = 0;
  head.biYPelsPerMeter = 0;
  head.biClrUsed = 0;
  head.biClrImportant = 0;
  
  BITMAPINFO& bi = *(BITMAPINFO*)reserved_space;

  if (aDepth == 1) {
    RGBQUAD black = { 0, 0, 0, 0 };
    RGBQUAD white = { 255, 255, 255, 0 };

    bi.bmiColors[0] = white;
    bi.bmiColors[1] = black;
  }

  HBITMAP bmp = ::CreateDIBitmap(dc, &head, CBM_INIT, aImageData, &bi, DIB_RGB_COLORS);
  ::ReleaseDC(NULL, dc);
  return bmp;

}

uint8_t* Data32BitTo1Bit(uint8_t* aImageData,
                                      uint32_t aWidth, uint32_t aHeight)
{
  // We need (aWidth + 7) / 8 bytes plus zero-padding up to a multiple of
  // 4 bytes for each row (HBITMAP requirement). Bug 353553.
  uint32_t outBpr = ((aWidth + 31) / 8) & ~3;

  // Allocate and clear mask buffer
  
  uint8_t* outData = (uint8_t*)calloc(outBpr, aHeight);
  if (!outData)
    return nullptr;

  int32_t *imageRow = (int32_t*)aImageData;
  for (uint32_t curRow = 0; curRow < aHeight; curRow++) {
    uint8_t *outRow = outData + curRow * outBpr;
    uint8_t mask = 0x80;
    for (uint32_t curCol = 0; curCol < aWidth; curCol++) {
      // Use sign bit to test for transparency, as alpha byte is highest byte
      if (*imageRow++ < 0)
        *outRow |= mask;

      mask >>= 1;
      if (!mask) {
        outRow ++;
        mask = 0x80;
      }
    }
  }

  return outData;
}

HBITMAP CreateIcon(imgIContainer *aContainer, gfxIntSize aScaledSize) {

  // Get the image data
  nsRefPtr<gfxASurface> surface;
  aContainer->GetFrame(imgIContainer::FRAME_CURRENT,
                       imgIContainer::FLAG_SYNC_DECODE,
                       getter_AddRefs(surface));
  NS_ENSURE_TRUE(surface, NULL);

  nsRefPtr<gfxImageSurface> frame(surface->GetAsReadableARGB32ImageSurface());
  NS_ENSURE_TRUE(frame, NULL);

  int32_t width = frame->Width();
  int32_t height = frame->Height();
  if (!width || !height)
    return NULL;

  uint8_t *data;
  nsRefPtr<gfxImageSurface> dest;
  data = frame->Data();
  /*
  if ((aScaledSize.width == 0 && aScaledSize.height == 0) ||
      (aScaledSize.width == width && aScaledSize.height == height)) {
    // We're not scaling the image. The data is simply what's in the frame.
    
  }
  else {
    NS_ENSURE_TRUE(aScaledSize.width > 0, NULL);
    NS_ENSURE_TRUE(aScaledSize.height > 0, NULL);
    // Draw a scaled version of the image to a temporary surface
    dest = new gfxImageSurface(aScaledSize, gfxImageFormatARGB32);
    if (!dest)
      return NULL;

    gfxContext ctx(dest);

    // Set scaling
    gfxFloat sw = (double) aScaledSize.width / width;
    gfxFloat sh = (double) aScaledSize.height / height;
    ctx.Scale(sw, sh);

    // Paint a scaled image
    ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx.SetSource(frame);
    ctx.Paint();

    data = dest->Data();
    width = aScaledSize.width;
    height = aScaledSize.height;
  }*/

  HBITMAP bmp = DataToBitmap(data, width, -height, 32);
  return bmp;/*
  uint8_t* a1data = Data32BitTo1Bit(data, width, height);
  if (!a1data) {
    return NS_ERROR_FAILURE;
  }

  HBITMAP mbmp = DataToBitmap(a1data, width, -height, 1);
  free(a1data);

  ICONINFO info = {0};
  info.fIcon = !aIsCursor;
  info.xHotspot = aHotspotX;
  info.yHotspot = aHotspotY;
  info.hbmMask = mbmp;
  info.hbmColor = bmp;

  HCURSOR icon = ::CreateIconIndirect(&info);
  ::DeleteObject(mbmp);
  ::DeleteObject(bmp);
  if (!icon)
    return NS_ERROR_FAILURE;
  *aIcon = icon;
  return NS_OK;*/
}



NS_IMETHODIMP IconObserver::CreateDIB(imgIRequest *aRequest)
{
	nsresult rv;
	nsCOMPtr<imgIContainer> image;
	rv = aRequest->GetImage(getter_AddRefs(image));
	NS_ENSURE_SUCCESS(rv, rv);

	gfxIntSize s(16,16);
	HBITMAP hBitmap = CreateIcon(image, s);
	
	int32_t w; 
    int32_t h; 
	image->GetWidth(&w);
	image->GetHeight(&h);
		
	if (w!=16 && h!=16) { 
		HDC hDC = GetDC(NULL);
		HBITMAP hbmSized = ResizeIcon32(hDC, hBitmap, w, h);
		if (!hbmSized) hbmSized = ResizeIcon(hDC, hBitmap, w, h);
		if (hbmSized) {
			DeleteObject(hBitmap);
			hBitmap = hbmSized;
		}
		ReleaseDC(NULL, hDC);
	}	
	
	CBitmap bitmap;
	bitmap.Attach(hBitmap);

	CString uri;
	nsCOMPtr<nsIURI> URI;
	nsEmbedCString nsuri;
	rv = aRequest->GetURI(getter_AddRefs(URI));
	NS_ENSURE_SUCCESS(rv, rv);
	URI->GetSpec(nsuri);

	nsEmbedCString nsPageuri;
	if (mPageUri) mPageUri->GetSpec(nsPageuri);
	mFavList->AddIcon(nsuri.get(),&bitmap, (CBitmap*)NULL, nsPageuri.get());
	return NS_OK;
}

NS_IMETHODIMP IconObserver::LoadIcon(nsIURI *iconUri, nsIURI* pageUri)
{
	nsresult rv;
	nsCOMPtr<imgILoader> loader = do_GetService("@mozilla.org/image/loader;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	mPageUri = pageUri;

	return loader->LoadImageXPCOM(iconUri, pageUri, nullptr, 
		nullptr, nullptr, this, this, nsIRequest::LOAD_BYPASS_CACHE, 
		nullptr, nullptr, getter_AddRefs(mRequest));
}


#endif // INTERNAL_SITEICONS