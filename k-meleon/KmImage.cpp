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
#include "KmImage.h"
#include "MfcEmbed.h"

#include "imgILoader.h"
#include "imgIContainer.h"
#include "gfxUtils.h"

bool IsComCtl6() {
	static int isv6 = -1;
	if (isv6 != -1) return isv6;

	HMODULE hComCtlDll = LoadLibrary(_T("comctl32.dll"));
	if (!hComCtlDll) return isv6 = 0;

	typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO*);
	PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hComCtlDll, "DllGetVersion");
	if (!pfnDllGetVersion) 
		isv6 = 0;
	else {
		DLLVERSIONINFO dvi = {0};
		dvi.cbSize = sizeof(dvi);

		HRESULT hRes = (*pfnDllGetVersion)(&dvi);
		isv6 = SUCCEEDED(hRes) && dvi.dwMajorVersion >= 6;
	}

	FreeLibrary(hComCtlDll);
	return isv6;
}

using namespace mozilla::gfx;

NS_IMPL_ISUPPORTS(nsImageObserver, imgINotificationObserver, nsISupportsWeakReference)

NS_IMETHODIMP nsImageObserver::Notify(imgIRequest *aProxy, int32_t aType, const nsIntRect *aRect)
{
	if (aType == imgINotificationObserver::LOAD_COMPLETE)
	{		
		if (!NS_SUCCEEDED(aProxy->StartDecoding())) {
			aProxy->Cancel(NS_OK);	
		}
		NS_ADDREF_THIS();
		mNeedRelease = true;
	}
	else if (aType == imgINotificationObserver::DECODE_COMPLETE)
	{
		mObserver->ImageLoaded(CreateDIB(aProxy));
		aProxy->CancelAndForgetObserver(NS_OK);
		//mRequest = nullptr;
		delete mObserver;
		if (mNeedRelease) NS_RELEASE_THIS();		
		// Can't release here anymore else gecko crash 
		/*nsCOMPtr<nsIThreadManager> tm = do_GetService("@mozilla.org/thread-manager;1");
		if (!tm) return NS_OK;
		nsCOMPtr<nsIThread> thread;
		tm->GetCurrentThread(getter_AddRefs(thread));
		if (!thread) return NS_OK;
		thread->Dispatch(new favRelease(this), nsIThread::DISPATCH_NORMAL);*/
	}

    return NS_OK;
}

static void
ConvertBGRXToBGRA(uint8_t* aData, const IntSize &aSize, int32_t aStride)
{
  uint32_t* pixel = reinterpret_cast<uint32_t*>(aData);

  for (int row = 0; row < aSize.height; ++row) {
    for (int column = 0; column < aSize.width; ++column) {
#ifdef IS_BIG_ENDIAN
      pixel[column] |= 0x000000FF;
#else
      pixel[column] |= 0xFF000000;
#endif
    }
    pixel += (aStride/4);
  }
}

static void
CopySurfaceDataToPackedArray(uint8_t* aSrc, uint8_t* aDst, IntSize aSrcSize,
                             int32_t aSrcStride, int32_t aBytesPerPixel)
{
  MOZ_ASSERT(aBytesPerPixel > 0,
             "Negative stride for aDst not currently supported");

  int packedStride = aSrcSize.width * aBytesPerPixel;

  if (aSrcStride == packedStride) {
    // aSrc is already packed, so we can copy with a single memcpy.
    memcpy(aDst, aSrc, packedStride * aSrcSize.height);
  } else {
    // memcpy one row at a time.
    for (int row = 0; row < aSrcSize.height; ++row) {
      memcpy(aDst, aSrc, packedStride);
      aSrc += aSrcStride;
      aDst += packedStride;
    }
  }
}

static uint8_t* SurfaceToPackedBGRA(DataSourceSurface *aSurface)
{
  SurfaceFormat format = aSurface->GetFormat();
  if (format != SurfaceFormat::B8G8R8A8 && format != SurfaceFormat::B8G8R8X8) {
    return nullptr;
  }

  IntSize size = aSurface->GetSize();

  uint8_t* imageBuffer = new (std::nothrow) uint8_t[size.width * size.height * sizeof(uint32_t)];
  if (!imageBuffer) {
    return nullptr;
  }

  DataSourceSurface::MappedSurface map;
  if (!aSurface->Map(DataSourceSurface::READ, &map)) {
    delete [] imageBuffer;
    return nullptr;
  }

  CopySurfaceDataToPackedArray(map.mData, imageBuffer, size,
                               map.mStride, 4 * sizeof(uint8_t));

  aSurface->Unmap();

  if (format == SurfaceFormat::B8G8R8X8) {
    // Convert BGRX to BGRA by setting a to 255.
    ConvertBGRXToBGRA(reinterpret_cast<uint8_t *>(imageBuffer), size, size.width * sizeof(uint32_t));
  }

  return imageBuffer;
}

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

HBITMAP nsImageObserver::CreateDIB(imgIRequest *aRequest)
{
	nsresult rv;
	nsCOMPtr<imgIContainer> container;
	rv = aRequest->GetImage(getter_AddRefs(container));
	NS_ENSURE_SUCCESS(rv, NULL);

#ifdef _DEBUG
	// There is a problem with the linking in debug 
	// Use a dummy image instead
	return  (HBITMAP)::LoadImage(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_TOOLBAR_CLOSE), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	//return NULL;
#endif

	// Get the image data
	mozilla::RefPtr<SourceSurface> surface;
	
	surface = container->GetFrame(imgIContainer::FRAME_CURRENT,
		imgIContainer::FLAG_SYNC_DECODE | imgIContainer::FLAG_WANT_DATA_SURFACE );
	NS_ENSURE_TRUE(surface, NULL);
	surface->GetFormat(); // call GetType in debug ...

	mozilla::RefPtr<DataSourceSurface> dataSurface;
	DataSourceSurface::MappedSurface map;
	bool mappedOK;
	if (surface->GetFormat() != mozilla::gfx::SurfaceFormat::B8G8R8A8) {
		// Convert format to SurfaceFormat::B8G8R8A8
		//dataSurface = gfxUtils::CopySurfaceToDataSourceSurfaceWithFormat(surface, SurfaceFormat::B8G8R8A8);
		//NS_ENSURE_TRUE(dataSurface, NULL);
		//mappedOK = dataSurface->Map(mozilla::gfx::DataSourceSurface::MapType::READ, &map);
	} else {
		dataSurface = surface->GetDataSurface();
		NS_ENSURE_TRUE(dataSurface, NULL);
		mappedOK = dataSurface->Map(mozilla::gfx::DataSourceSurface::READ, &map);
	}
	NS_ENSURE_TRUE(dataSurface && mappedOK, NULL);
	//MOZ_ASSERT(dataSurface->GetFormat() == mozilla::gfx::SurfaceFormat::B8G8R8A8);
  
	IntSize frameSize = surface->GetSize();
	if (frameSize.IsEmpty()) {
	return NULL;
	}

	uint8_t* data = nullptr;
	nsAutoArrayPtr<uint8_t> autoDeleteArray;
	if (map.mStride == BytesPerPixel(dataSurface->GetFormat()) * frameSize.width) {
		// Mapped data is already packed
		data = map.mData;
	} else {
		// We can't use map.mData since the pixels are not packed (as required by
		// CreateDIBitmap, which is called under the DataToBitmap call below).
		//
		// We must unmap before calling SurfaceToPackedBGRA because it needs access
		// to the pixel data.
		dataSurface->Unmap();
		map.mData = nullptr;
 
		data = autoDeleteArray = SurfaceToPackedBGRA(dataSurface);
		NS_ENSURE_TRUE(data, NULL);
	}

	HBITMAP bmp = DataToBitmap(data, frameSize.width, -frameSize.height, 32);
	return bmp;
}

ULONG_PTR KmImage::mGdiToken = 0;
UINT KmImage::mGdiCount = 0;

bool KmImage::LoadIndexedFromSkin(LPCTSTR name, UINT w, UINT h) 
{
	ASSERT(w && h);
	CString imgPath(name);
	UINT index = 0;
	int pos = imgPath.Find(_T('['));
	if (pos != -1) {
		int pos2 = imgPath.Find(_T(']'));
		index = _ttoi(imgPath.Mid(pos+1, pos2-pos).GetBuffer());
		imgPath.Truncate(pos);
	}

	if (!LoadFromSkin(imgPath)) {
		ASSERT(0);
		return false;
	}

	Crop(w, h, index);
	return true;
}

bool KmImage::LoadFromSkin(LPCTSTR name, const LPRECT rect)
{
	CString imgPath(name);
	int index = -1;
	int pos = imgPath.Find(_T('['));
	if (pos != -1) {
		int pos2 = imgPath.Find(_T(']'));
		index = _ttoi(imgPath.Mid(pos+1, pos2-pos).GetBuffer());
		imgPath.Truncate(pos);
	}

	CString path;
	theApp.skin.FindSkinFile(path, imgPath);
	if (!Load(path)) {
		ASSERT(0);
		return false;
	}
	
	ASSERT(index==-1 || rect);

	if (index>=0 && rect) {
		Crop(rect->right-rect->left, GetHeight(), index);
	} else if (rect) {
		Clip(*rect);
	}

	return true;
}

bool KmImage::LoadFromBitmap(HBITMAP hbmp, bool reverse)
{
	ASSERT(hbmp);
	Clean();
	mBitmap.Attach(hbmp);

	BITMAP bm = {0};
    mBitmap.GetObject(sizeof(BITMAP), (void*)&bm);	
	
	if (bm.bmBitsPixel == 32) {
		void* bits = bm.bmBits;
		if (!bits) {
			bits = new BYTE[bm.bmWidth * bm.bmHeight * bm.bmBitsPixel];
			mBitmap.GetBitmapBits(bm.bmWidth * bm.bmHeight * bm.bmBitsPixel, bits);
		}
		mGdiBitmap = new Gdiplus::Bitmap(bm.bmWidth, bm.bmHeight, PixelFormat32bppPARGB);
		Gdiplus::BitmapData bmd;
		Gdiplus::Rect rect(0, 0, bm.bmWidth, bm.bmHeight);
		mGdiBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bmd);
		int lineSize = bm.bmWidth * 4;
		byte* destBytes = (byte*)(bmd.Scan0);
		for (int y = 0; y < bm.bmHeight; y++)
		{
			memcpy(destBytes + (y * lineSize), (byte*)bits + ((reverse?(bm.bmHeight - y - 1):y) * lineSize), lineSize);
		}
		mGdiBitmap->UnlockBits(&bmd);
		if (!bm.bmBits) delete bits;
	} else {
		mGdiBitmap = Gdiplus::Bitmap::FromHBITMAP(hbmp, NULL);
		MakeTransparent(mTrColor);
	}

	return mGdiBitmap != nullptr;
}

bool KmImage::MakeTransparent(COLORREF clr) 
{
	ASSERT(mGdiBitmap);
	Gdiplus::Color color;
	if (mGdiBitmap->GetPixel(0,0, &color) != Gdiplus::Ok);
		color.MakeARGB(255,255,0,255);
	
	Gdiplus::ImageAttributes imageAttributes;

	Gdiplus::ColorMap  colorMap[1];
	colorMap[0].oldColor = color;
	colorMap[0].newColor = Gdiplus::Color(0, 255, 0, 255);  
	imageAttributes.SetRemapTable(1, colorMap, Gdiplus::ColorAdjustTypeBitmap);

	UINT w = mGdiBitmap->GetWidth();
	UINT h = mGdiBitmap->GetHeight();
	Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
	Gdiplus::Graphics graphics(newBitmap);
	Gdiplus::Status s = graphics.DrawImage(mGdiBitmap, 
		Gdiplus::Rect(0, 0, w, h),
		0, 0, w, h,
		Gdiplus::UnitPixel, &imageAttributes);
	if (s != Gdiplus::Ok) {
		delete newBitmap;
		return false;
	}
	Clean();
	mGdiBitmap = newBitmap;
	return true;
}

bool KmImage::Load(LPCTSTR path)
{
	Clean();

	if (CString(path).Find(_T(".bmp"))) {
		// 32 bbp bitmap does not work well with gdi+
		HBITMAP bmp = (HBITMAP)LoadImage(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		if (bmp) return LoadFromBitmap(bmp, true);
	}

	mGdiBitmap = Gdiplus::Bitmap::FromFile(path);
	if (mGdiBitmap->GetLastStatus() != Gdiplus::Ok) {
		delete mGdiBitmap;
		mGdiBitmap = nullptr;
		return false;
	}
	/*
	OSVERSIONINFO osvi = {0};
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (osvi.dwMajorVersion < 6) {
		// Prevent XP to fail because of SetResolution.
		Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(GetWidth(), GetHeight(), mGdiBitmap->GetPixelFormat());
		Gdiplus::Graphics graphics(newBitmap);
		graphics.DrawImage(mGdiBitmap, 0, 0, GetWidth(), GetHeight());
		delete newBitmap;
	}

	HDC dc = ::GetDC(NULL);
	int resy = GetDeviceCaps(dc, LOGPIXELSY); 
	int resx = GetDeviceCaps(dc, LOGPIXELSX); 
	mGdiBitmap->SetResolution(Gdiplus::REAL(resx),Gdiplus::REAL(resy)); // Prevent gdi+ to scale the image
	*/
	if (!Gdiplus::IsAlphaPixelFormat(mGdiBitmap->GetPixelFormat() && IsComCtl6()))
		MakeTransparent(mTrColor);

	return mGdiBitmap != nullptr;
	//return SUCCEEDED(mImage.Load(path));
}

UINT KmImage::GetWidth() const
{
	ASSERT(mGdiBitmap || mBitmap.GetSafeHandle());
	if (!mGdiBitmap) {
		BITMAP bm = {0};
		mBitmap.GetObject(sizeof(BITMAP), (void*)&bm);
		return bm.bmWidth;
	}
	return mGdiBitmap->GetWidth();
}

UINT KmImage::GetHeight() const
{
	ASSERT(mGdiBitmap || mBitmap.GetSafeHandle());
	if (!mGdiBitmap) {
		BITMAP bm = {0};
		mBitmap.GetObject(sizeof(BITMAP), (void*)&bm);
		return bm.bmHeight;
	}

	return mGdiBitmap->GetHeight();
}

#define round(x) (UINT)((x)+0.5)
bool KmImage::Scale(float f)
{
	ASSERT(mGdiBitmap || mBitmap.GetSafeHandle());
	return Resize(round(GetWidth()*f), round(GetHeight()*f));
	//return Resize(round(mImage.GetWidth()*f), round(mImage.GetHeight()*f));
}

bool KmImage::Scale(float f, KmImage& img) const
{
	ASSERT(mGdiBitmap || mBitmap.GetSafeHandle());
	return Resize(round(GetWidth()*f), round(GetHeight()*f), img);
	//return Resize(round(mImage.GetWidth()*f), round(mImage.GetHeight()*f));
}

Gdiplus::Bitmap* KmImage::_Resize(UINT w, UINT h) const
{
	Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(w, h, mGdiBitmap->GetPixelFormat());
	Gdiplus::Graphics graphics(newBitmap);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
	if (graphics.DrawImage(mGdiBitmap, Gdiplus::Rect(0, 0, w, h), 0, 0, GetWidth(), GetHeight(), Gdiplus::UnitPixel) != Gdiplus::Ok) {
		delete newBitmap;
		return nullptr;
	}
	return newBitmap;
}

bool KmImage::Resize(UINT w, UINT h, KmImage& img) const
{
	ASSERT(mGdiBitmap);
	Gdiplus::Bitmap* newBitmap = _Resize(w, h);
	if (!newBitmap) return false;
	img.Clean();
	img.mGdiBitmap = newBitmap;
	return true;
}

bool KmImage::Resize(UINT w, UINT h)
{
	ASSERT(mGdiBitmap);
	Resize(w, h, *this);
	return true;
}

HBITMAP KmImage::_Crop(UINT w, UINT h, UINT index) 
{
	/*CImage img;
	img.Create(w, h, mImage.GetBPP());

	UINT sw, sh;
	sw = mImage.GetWidth();
	sh = mImage.GetHeight();
	UINT nCol = ((w*index) % sw) / w;
	UINT nLine = (w*index)/sw;

	if (!mImage.BitBlt(img.GetDC(), 0, 0, w, h, nCol*w, nLine*h))
		return NULL;

	img.ReleaseDC();
	return img.Detach();*/
	return NULL;
}

bool KmImage::CropLine(UINT h, UINT line, KmImage& kImg) const
{
	ASSERT(mGdiBitmap);
	UINT sw = GetWidth();
	UINT sh = GetHeight();
	if (sh<h*(line+1)) return false;

	if (!mGdiBitmap) {

	} else {		
		Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(sw, h, mGdiBitmap->GetPixelFormat());
		Gdiplus::Graphics graphics(newBitmap);
		if (graphics.DrawImage(mGdiBitmap, Gdiplus::Rect(0, 0, sw, h), 0, line*h, sw, h, Gdiplus::UnitPixel) != Gdiplus::Ok) {
			delete newBitmap;
			return false;
		}
		kImg.Clean(); // XXX
		kImg.mGdiBitmap = newBitmap;
	}
	/*
	CImage img;
	img.Create(sw, h, mImage.GetBPP());
	if (!mImage.BitBlt(img.GetDC(), 0, 0, sw, h, 0, line*h))
		return false;
	img.ReleaseDC();
	kImg.LoadFromBitmap(img.Detach());*/
	return true;
}

bool KmImage::Clip(const RECT& r)
{
	ASSERT(mGdiBitmap);
	ASSERT(GetWidth() >= r.right);
	ASSERT(GetHeight() >= r.bottom);
	LONG w = r.right-r.left;
	LONG h = r.bottom-r.top;
	Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(w, h, mGdiBitmap->GetPixelFormat());
	Gdiplus::Graphics graphics(newBitmap);
	if (graphics.DrawImage(mGdiBitmap, Gdiplus::Rect(0, 0, w, h), r.left, r.top, w, h, Gdiplus::UnitPixel) != Gdiplus::Ok) {
		delete newBitmap;
		return false;
	}
	Clean();
	mGdiBitmap = newBitmap;
	return true;
}

bool KmImage::Crop(UINT w, UINT h, UINT index) 
{
	ASSERT(mGdiBitmap);
	UINT sw = GetWidth();
	UINT sh = GetHeight();
	if ((sw == w && sh ==h) || h == 0 || w == 0) 
		return index == 0;

	UINT nCol = ((w*index) % sw) / w;
	UINT nLine = (w*index)/sw;
	
	if (!mGdiBitmap) {

	} else {		
		Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(w, h, mGdiBitmap->GetPixelFormat());
		Gdiplus::Graphics graphics(newBitmap);
		if (graphics.DrawImage(mGdiBitmap, Gdiplus::Rect(0, 0, w, h), nCol*w, nLine*h, w, h, Gdiplus::UnitPixel) != Gdiplus::Ok) {
			delete newBitmap;
			return false;
		}
		Clean();
		mGdiBitmap = newBitmap;
	}
	return true;
}

bool KmImage::DrawItem(HDC dc, POINT pt, UINT w, UINT h, UINT index, UINT line) 
{
	Gdiplus::Graphics gDest(dc);
	if (gDest.DrawImage(
			mGdiBitmap,  
			Gdiplus::Rect(pt.x, pt.y, w, h), 
			index*w, line*h, w, h, Gdiplus::UnitPixel
		) != Gdiplus::Ok)
		return false;	
	return true;
}

HBITMAP KmImage::GetHBitmap() 
{
	if (mBitmap.GetSafeHandle())
		return (HBITMAP)mBitmap;

	if (!mGdiBitmap)
		return NULL;
	
	void* bits;
	HBITMAP hbmp;
	UINT bpp = Gdiplus::GetPixelFormatSize( mGdiBitmap->GetPixelFormat() );

	if (bpp == 32) {
		BITMAPINFO bmi = {0};
		bmi.bmiHeader.biSize = sizeof( bmi.bmiHeader );
		bmi.bmiHeader.biWidth = GetWidth();
		bmi.bmiHeader.biHeight = -GetHeight();
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = USHORT( 32 );
		bmi.bmiHeader.biCompression = BI_RGB;
		hbmp = ::CreateDIBSection( NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0 );
		Gdiplus::Bitmap bmDest( GetWidth(), GetHeight(), GetWidth()*4, IsComCtl6()?PixelFormat32bppARGB:PixelFormat32bppRGB, static_cast< BYTE* >( bits ) );
		Gdiplus::Graphics gDest( &bmDest );
		if (gDest.DrawImage( mGdiBitmap, 0, 0 ) != Gdiplus::Ok) {
			DeleteObject(hbmp);
			return NULL;
		}
	}
	else {
		if (mGdiBitmap->GetHBITMAP(0, &hbmp) != Gdiplus::Ok)
			return NULL;
	}

	mBitmap.Attach(hbmp);
	return hbmp;
}

int KmImage::AddToImageList(CImageList& list, int index)
{
	UINT bpp;
	ASSERT(mGdiBitmap);

	bpp = Gdiplus::GetPixelFormatSize( mGdiBitmap->GetPixelFormat() );
	GetHBitmap();		
	
#ifdef _DEBUG
	int cx,cy;
	ImageList_GetIconSize(list.m_hImageList, &cx, &cy);
	ASSERT(GetWidth()%cx == 0 && cy == GetHeight());
#endif

	Gdiplus::Color color;
	mGdiBitmap->GetPixel(0,0,&color);

	if (!IsComCtl6()) 
		return list.Add(&mBitmap, color.ToCOLORREF());
	if (index == -1)
		return list.Add(&mBitmap, nullptr);
	return list.Replace(index, &mBitmap, nullptr) ? index : -1;
}