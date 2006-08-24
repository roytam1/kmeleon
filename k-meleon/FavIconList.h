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

class CFavIconList : public CImageList
{
private:
	CMap<CStringA, LPCSTR, int, int &> m_urlMap;
	int m_iDefaultIcon;

public:
	CFavIconList();
	virtual ~CFavIconList();
	
	int GetHostIcon(const TCHAR* aUri);
	int GetIcon(nsIURI* aUri, BOOL download = FALSE);
	
	void RefreshIcon(nsIURI* aURI);
	void ResetCache();
	void LoadDefaultIcon();
	
	BOOL DwnFavIcon(nsIURI* iconURI);
	static void DwnCall(char* , TCHAR* , nsresult, void* );

	inline int GetDefaultIcon() {return m_iDefaultIcon;}
	int AddIcon(char* uri, TCHAR* file, nsresult aStatus);
	//void AddIcon(nsIURI* aURI, TCHAR* file);
	BOOL Create(int, int, UINT, int, int);
};


