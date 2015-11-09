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
*/

#pragma once

#include "resource.h"
#include "afxtempl.h"
#include "DialogEx.h"

#include "nsICookieManager.h"
#include "nsICookie.h"

class CCookie;

typedef CList<CCookie*, CCookie*> CCookieList; 

class CCookiesViewerDlg : public CDialog
{
	//DECLARE_DYNAMIC(CCookiesViewerDlg)

public:
	CCookiesViewerDlg(CWnd* pParent = NULL);   // constructeur standard
	virtual ~CCookiesViewerDlg();

	static int CALLBACK SortCookiesList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	enum { IDD = IDD_COOKIES_VIEWER };

protected:
	CCookieList m_CookiesList;
	nsCOMPtr<nsICookieManager> m_cookieManager;

	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_cCookiesList;
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedListCookies(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedDeleteAllCookies();
	afx_msg void OnBnClickedDeleteCookies();
	afx_msg void OnEnChangeCookieSearch();
	afx_msg void OnDestroy();
};
