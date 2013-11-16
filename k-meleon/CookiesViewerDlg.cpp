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

#include "stdafx.h"
#include "CookiesViewerDlg.h"
#include "Cookies.h"
#include "nsISimpleEnumerator.h"


// Boîte de dialogue CCookiesViewerDlg

//IMPLEMENT_DYNAMIC(CCookiesViewerDlg, CDialog)
CCookiesViewerDlg::CCookiesViewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCookiesViewerDlg::IDD, pParent)
{
	m_cookieManager = do_GetService(NS_COOKIEMANAGER_CONTRACTID);
}

CCookiesViewerDlg::~CCookiesViewerDlg()
{
}

void CCookiesViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_COOKIES, m_cCookiesList);
}


BEGIN_MESSAGE_MAP(CCookiesViewerDlg, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_COOKIES, OnLvnItemchangedListCookies)
	ON_BN_CLICKED(IDC_DELETE_ALL_COOKIES, OnBnClickedDeleteAllCookies)
	ON_BN_CLICKED(IDC_DELETE_COOKIES, OnBnClickedDeleteCookies)
	ON_EN_CHANGE(IDC_COOKIE_SEARCH, OnEnChangeCookieSearch)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// Gestionnaires de messages CCookiesViewerDlg
int CALLBACK CCookiesViewerDlg::SortCookiesList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   //CListCtrl* pListCtrl = (CListCtrl*) lParamSort;
   CCookieList* pCookieList = (CCookieList*) lParamSort;

   CCookie* cookie1 = pCookieList->GetAt((POSITION)lParam1);
   CCookie* cookie2 = pCookieList->GetAt((POSITION)lParam2);

   return cookie1->m_csHost.Compare(cookie2->m_csHost);
}

BOOL CCookiesViewerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	nsresult rv;
    
	nsCOMPtr<nsISimpleEnumerator> enumCookie;
	rv = m_cookieManager->GetEnumerator(getter_AddRefs(enumCookie));
	if (NS_FAILED(rv))
	{
		AfxMessageBox(_T("Failed to get the cookie manager"));
		return TRUE;
	}

	CString header;
	header.LoadString(IDS_HEADER_SITE);
	m_cCookiesList.InsertColumn(0, header, LVCFMT_LEFT, 0, 0);
	header.LoadString(IDS_HEADER_NAME);
	m_cCookiesList.InsertColumn(1, header, LVCFMT_LEFT, 0, 1);

	m_cCookiesList.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	bool ret;
	LVITEM lvItem;
	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.state = 0;
	lvItem.stateMask = 0;
	lvItem.pszText = 0;
	lvItem.iItem	= 0xffff;
	
	enumCookie->HasMoreElements(&ret);
	while (ret)
    {
		nsCOMPtr<nsICookie> nsCookie;
		rv = enumCookie->GetNext(getter_AddRefs(nsCookie));
		if (NS_FAILED(rv)) break;

		CCookie* cookie = new CCookie(nsCookie);
		POSITION p = m_CookiesList.AddHead(cookie);

		lvItem.lParam = (LPARAM)p;
		int index = m_cCookiesList.InsertItem(&lvItem);

		if (index==-1) continue;

		ListView_SetItemText(m_cCookiesList.GetSafeHwnd(), index, 0, cookie->m_csHost.GetBuffer(0))
		ListView_SetItemText(m_cCookiesList.GetSafeHwnd(), index, 1, cookie->m_csName.GetBuffer(0))
		
		enumCookie->HasMoreElements(&ret);
    }     
	
	// Set the column width, I'm doing it after inserting cookies in the list
	// because of the scrollbar
	RECT rect;
	m_cCookiesList.GetClientRect(&rect);
	int width = (rect.right - rect.left)/2;
	m_cCookiesList.SetColumnWidth(0, width);
	m_cCookiesList.SetColumnWidth(1, width);

	m_cCookiesList.SortItems(SortCookiesList, (LPARAM) &m_CookiesList);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CCookiesViewerDlg::OnLvnItemchangedListCookies(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (m_cCookiesList.GetSelectedCount()!=1)
	{
		SetDlgItemText(IDC_COOKIE_TEXT, _T(""));
		SetDlgItemText(IDC_COOKIE_NAME, _T(""));
		SetDlgItemText(IDC_COOKIE_HOST, _T(""));
		SetDlgItemText(IDC_COOKIE_PATH, _T(""));
		SetDlgItemText(IDC_COOKIE_SENDFOR, _T(""));
		SetDlgItemText(IDC_COOKIE_EXPIRES, _T(""));
		SetDlgItemText(IDC_COOKIE_VALUE, _T(""));
	}
	else
	{
		int nItem = m_cCookiesList.GetNextItem(-1, LVNI_SELECTED);
		LPARAM lp = m_cCookiesList.GetItemData(nItem);
		if (lp == -1) return;
		
		POSITION p = (POSITION)lp;
		CCookie* cookie = m_CookiesList.GetAt(p);
		
		SetDlgItemText(IDC_COOKIE_NAME, cookie->m_csName);
		SetDlgItemText(IDC_COOKIE_HOST, cookie->m_csHost);
		SetDlgItemText(IDC_COOKIE_PATH, cookie->m_csPath);
		SetDlgItemText(IDC_COOKIE_EXPIRES, cookie->m_csExpire);
		SetDlgItemText(IDC_COOKIE_VALUE, cookie->m_csValue);

		CString str;
		if (cookie->m_secure)
			str.LoadString(IDS_FOR_SECURE);
		else
			str.LoadString(IDS_FOR_ANY);

		SetDlgItemText(IDC_COOKIE_SENDFOR, str);
	}

	// TODO : ajoutez ici le code de votre gestionnaire de notification de contrôle
	*pResult = 0;
}

void CCookiesViewerDlg::OnBnClickedDeleteAllCookies()
{
	if (AfxMessageBox(IDS_CONFIRM_DELETEALLCOOKIES, MB_OKCANCEL | MB_ICONWARNING) == IDOK) 
	{
		nsresult rv = m_cookieManager->RemoveAll();
		if (NS_FAILED(rv)) 
		{
			AfxMessageBox(IDS_FAILED_DELETE_COOKIE,MB_OK|MB_ICONERROR);
			return;
		}

		m_cCookiesList.DeleteAllItems();

		while (!m_CookiesList.IsEmpty())
		{
			delete(m_CookiesList.GetHead());
			m_CookiesList.RemoveHead();
		}
	}
}

void CCookiesViewerDlg::OnBnClickedDeleteCookies()
{
	UINT uSelectedCount = m_cCookiesList.GetSelectedCount();
	int nItem = -1;
	nsresult rv;

	if (uSelectedCount>0)
	{
		UINT i;
		for (i=0;i < uSelectedCount;i++)
		{
			nItem = m_cCookiesList.GetNextItem(nItem, LVNI_SELECTED);
			ASSERT(nItem != -1);
			POSITION p = (POSITION)m_cCookiesList.GetItemData(nItem);
			CCookie* cookie = m_CookiesList.GetAt(p);
			
			PRBool blocked = IsDlgButtonChecked(IDC_ALLOW_DELETED) ? PR_TRUE : PR_FALSE;
			rv = m_cookieManager->Remove(cookie->m_host, cookie->m_name, cookie->m_path, blocked);
			if (NS_FAILED(rv)) 
			{
				AfxMessageBox(IDS_FAILED_DELETE_COOKIE,MB_OK|MB_ICONERROR);
				return;
			}

			m_CookiesList.RemoveAt(p);
			delete cookie;
		}

		for (i=0;i < uSelectedCount;i++)
		{
			nItem = m_cCookiesList.GetNextItem(-1, LVNI_SELECTED);
			m_cCookiesList.DeleteItem(nItem);
		}
	}
}

void CCookiesViewerDlg::OnEnChangeCookieSearch()
{
	LVFINDINFO info = {0};
	info.flags = LVFI_PARTIAL;
	
	CString csSearch;
	GetDlgItemText(IDC_COOKIE_SEARCH, csSearch);
	info.psz = csSearch.GetBuffer(0);

	int nIndex = m_cCookiesList.FindItem(&info, -1);
	m_cCookiesList.EnsureVisible(nIndex, FALSE);
}

void CCookiesViewerDlg::OnDestroy()
{
	CDialog::OnDestroy();
	while (!m_CookiesList.IsEmpty())
	{
		delete(m_CookiesList.GetHead());
		m_CookiesList.RemoveHead();
	}
}
