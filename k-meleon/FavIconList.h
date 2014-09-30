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

#pragma once

#include "afxtempl.h"

#include "imgIRequest.h"
#include "imgINotificationObserver.h"

class CFavIconList;

class IconObserver : public imgINotificationObserver
					 

{
	NS_DECL_ISUPPORTS
	NS_DECL_IMGINOTIFICATIONOBSERVER

	IconObserver(CFavIconList* favlist) : mFavList(favlist) {}
	virtual ~IconObserver() { }

	NS_IMETHOD LoadIcon(nsIURI* iconUri, nsIURI* pageUri);

protected:
	CFavIconList* mFavList;
	nsCOMPtr<imgIRequest> mRequest;
	nsCOMPtr<nsIURI> mPageUri;
	NS_IMETHOD CreateDIB(imgIRequest *aRequest);
};

class CFavIconList : public CImageList
{
private:
	CMap<CString, LPCTSTR, int, int &> m_urlMap;
	int m_iDefaultIcon;
	int m_iLoadingIcon;
	int m_iOffset;
	//nsCOMPtr<IconObserver> mIconObserver;
	
	void AddMap(const char *uri, int index, const char* pageUri = nullptr);
	int AddDownloadedIcon(char* uri, TCHAR* file, nsresult aStatus);
	BOOL LoadCache();
	

public:
	CFavIconList();
	virtual ~CFavIconList();

	BOOL WriteCache();
	int AddIcon(const char* uri, CBitmap*, CBitmap*, const char* pageUri = nullptr);
	int AddIcon(const char* uri, CBitmap*, COLORREF, const char* pageUri = nullptr);
	int AddIcon(const char* uri, HICON icon, const char* pageUri = nullptr);

	int GetHostIcon(const TCHAR* aUri);
	int GetIcon(const TCHAR* uri);
	int GetIcon(nsIURI* aUri, nsIURI* aPageURI = NULL, BOOL download = FALSE);
	
	void RefreshIcon(nsIURI* aURI);
	void ResetCache();
	void LoadDefaultIcon();
	
	BOOL DwnFavIcon(nsIURI* iconURI, nsIURI* pageURI = NULL);
	static void DwnCall(char* , TCHAR* , nsresult, void* );

	inline int GetDefaultIcon() {return m_iDefaultIcon;}
	inline int GetLoadingIcon() {return m_iLoadingIcon;}

	BOOL Create(int, int, UINT, int, int);
};


