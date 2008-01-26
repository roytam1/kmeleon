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
#include "imgIDecoderObserver.h"
#include "imgIRequest.h"

class CFavIconList;

class IconObserver : public imgIDecoderObserver
					 

{
	NS_DECL_ISUPPORTS
	NS_DECL_IMGIDECODEROBSERVER
	NS_DECL_IMGICONTAINEROBSERVER

	IconObserver(CFavIconList* favlist) : mFavList(favlist) {}
	virtual ~IconObserver() { }

	NS_IMETHOD LoadIcon(nsIURI* iconUri, nsIURI* pageUri);

protected:
	CFavIconList* mFavList;
	nsCOMPtr<imgIRequest> mRequest;
	NS_IMETHOD CreateDIB(imgIRequest *aRequest);
};

class CFavIconList : public CImageList
{
private:
	CMap<CString, LPCTSTR, int, int &> m_urlMap;
	int m_iDefaultIcon;
	int m_iOffset;
	
	void AddMap(const char *uri, int index);
	int AddDownloadedIcon(char* uri, TCHAR* file, nsresult aStatus);
	BOOL LoadCache();
	BOOL WriteCache();

public:
	CFavIconList();
	virtual ~CFavIconList();

	int AddIcon(const char* uri, CBitmap*, CBitmap*);
	int AddIcon(const char* uri, CBitmap*, COLORREF);
	int AddIcon(const char* uri, HICON icon);

	int GetHostIcon(const TCHAR* aUri);
	int GetIcon(nsIURI* aUri, BOOL download = FALSE);
	
	void RefreshIcon(nsIURI* aURI);
	void ResetCache();
	void LoadDefaultIcon();
	
	BOOL DwnFavIcon(nsIURI* iconURI);
	static void DwnCall(char* , TCHAR* , nsresult, void* );

	inline int GetDefaultIcon() {return m_iDefaultIcon;}

	BOOL Create(int, int, UINT, int, int);
};


