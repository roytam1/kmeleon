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

#include "mfcembed.h"
extern CMfcEmbedApp theApp;
#include "atlimage.h"

#define PNG_SUPPORT

#ifdef PNG_SUPPORT
#define PNGDIB_NO_D2P
#define PNGDIB_S
#include "../pngdib-3.0.1/pngdib.h"
#ifdef _UNICODE
#pragma comment(lib,"../pngdib-3.0.1/lib/pngdib_u_s_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/libpng_u_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/zlib/zlib_u_md.lib")
#else
#pragma comment(lib,"../pngdib-3.0.1/lib/pngdib_s_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/libpng_md.lib")
#pragma comment(lib,"../lpng128/projects/visualc71/lib/zlib/zlib_md.lib")
#endif
#endif

#define FAVICON_CACHE_FILE _T("IconCache.dat")

CFavIconList::CFavIconList()
{
	m_iDefaultIcon = 0;
}

CFavIconList::~CFavIconList()
{
	if  (!m_hImageList) return;

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
	/*else if (theApp.FindSkinFile(szFullPath, "layers.bmp"))
	{
		FILE *fp = fopen(szFullPath, "r");
		if (fp) {
			fclose(fp);
		    HBITMAP bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 16, 16, LR_LOADFROMFILE);
			
			if (bitmap)
			{
				CBitmap cbitmap;
				cbitmap.Attach(bitmap);
				m_iDefaultIcon = Add(&cbitmap, RGB(255, 0, 255));
				cbitmap.DeleteObject();
			}
		} 
	}*/

	// Add can return 0 even if it fails...
	if (GetImageCount()==0)
		m_iDefaultIcon = Add(theApp.GetDefaultIcon());
}

BOOL CFavIconList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{

	// Try to load the icon cache
	CFile   iconCache;
	if (iconCache.Open(theApp.preferences.settingsDir + FAVICON_CACHE_FILE, CFile::osSequentialScan | CFile::modeRead))
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

int CFavIconList::AddIcon(char* uri, TCHAR* file, nsresult aStatus)
{
	int index = GetDefaultIcon();

	if (NS_SUCCEEDED(aStatus))
	{
		HICON favicon = (HICON)LoadImage(NULL, file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		if (favicon){
			index = Add(favicon);
			DeleteObject(favicon);
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
						index = Add(&bitmap,RGB(pr,pg,pb));
					else
						index = Add(&bitmap, (CBitmap*)NULL);
					
					bitmap.DeleteObject();

					pngdib_p2d_free_dib(pngdib,NULL);
				}
				pngdib_done(pngdib);
			}

		}
		#endif
	
	}
	
	DeleteFile(file);
	m_urlMap[uri] = index;

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
		m_urlMap[nsUri.get()] = index;
	}

	// Even if it's not really efficient, at least I don't have
	// to mess with synchronisation.
	theApp.BroadcastMessage(UWM_NEWSITEICON, (WPARAM)uri, index);
	return index;
}

#ifdef _UNICODE
#define UTF8ToCString(src, dest) \
	nsEmbedString _str;\
	NS_UTF16ToCString(nsDependentString(src), NS_CSTRING_ENCODING_UTF8, _str);\
	url = _str.get();
#else
#define UTF8ToCString(src, dest) \
	nsEmbedCString _str;\
	NS_UTF16ToCString(nsDependentString(src), NS_CSTRING_ENCODING_UTF8, _str);\
	url = _str.get();
#endif

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
	m_urlMap.Lookup(nsUri.get(), index);

	return index;
}

int CFavIconList::GetIcon(nsIURI *aURI, BOOL download)
{
	int index = GetDefaultIcon();

	if  (!m_hImageList || !aURI) return 0;

	nsEmbedCString nsUri;
	aURI->GetSpec(nsUri);
	if (!m_urlMap.Lookup(nsUri.get(), index))
	{
		if (download) DwnFavIcon(aURI);
		return -1;
	}

	return index;
}

void CFavIconList::RefreshIcon(nsIURI* aURI)
{
	if (!m_hImageList || !aURI) return;
	int index = 0;
	nsEmbedCString nsUri;
	aURI->GetSpec(nsUri);
	if (m_urlMap.Lookup(nsUri.get(), index))
	{
		m_urlMap.RemoveKey(nsUri.get());
		if (index!=0) 
		{
			Remove(index);
			/*aURI->GetHost(nsUri);
			nsUri.Insert("http://", 0, 7);
			m_urlMap.RemoveKey(nsUri.get());*/

			CMap<CStringA, LPCSTR, int, int &>::CPair* pos = m_urlMap.PGetFirstAssoc();
			while(pos!=NULL)
			{
				if (pos->value == index)
				{
					CStringA tmpKey = pos->key;
					pos = m_urlMap.PGetNextAssoc(pos);
					m_urlMap.RemoveKey(tmpKey);
				}
				else
				{
					if (pos->value > index) pos->value--;
					pos = m_urlMap.PGetNextAssoc(pos);
				}
			}

			theApp.BroadcastMessage(UWM_NEWSITEICON, 0, -1);
		}
	}
}

void CFavIconList::ResetCache()
{
	while (GetImageCount())	Remove(0);
	m_urlMap.RemoveAll();
	LoadDefaultIcon();
	theApp.BroadcastMessage(UWM_NEWSITEICON, 0, -1);
}

void CFavIconList::DwnFavIcon(nsIURI* iconURI)
{
	/*int index;
	
	// Look if we have it already in cache
	nsEmbedCString nsUri;
	iconURI->GetSpec(nsUri);
	if (m_urlMap.Lookup(nsUri.get(), index))
	{
		m_pBrowserFrameGlue->SetFavIcon(iconURI);
		return;
	}*/

	// Currently the favicon is downloaded like any other file 
	// which is bad. A nsStreamListener have to be 
	// implemented.
	nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
	if(!persist) return;

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
	nsresult rv = persist->SaveURI(iconURI, nsnull, nsnull, nsnull, nsnull, file);
	if (NS_FAILED(rv))
		persist->SetProgressListener(nsnull);
}

void CFavIconList::DwnCall(char* uri, TCHAR* file, nsresult status, void* param)
{
	((CFavIconList*)param)->AddIcon(uri,file,status);
}

#endif // INTERNAL_SITEICONS