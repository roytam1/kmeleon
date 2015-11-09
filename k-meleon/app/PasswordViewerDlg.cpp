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
#include "PasswordViewerDlg.h"
#include ".\passwordviewerdlg.h"
#include "nsILoginManager.h"
#include "nsILoginInfo.h"



// Boû‘e de dialogue CPasswordViewerDlg

//IMPLEMENT_DYNAMIC(CPasswordViewerDlg, CDialog)
CPasswordViewerDlg::CPasswordViewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswordViewerDlg::IDD, pParent)
{
	m_passwordManager = do_GetService(NS_LOGINMANAGER_CONTRACTID);
	m_bShowPasswords = FALSE;
}

CPasswordViewerDlg::~CPasswordViewerDlg()
{
}

void CPasswordViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PASSWORDS, m_cPasswordsList);
}


BEGIN_MESSAGE_MAP(CPasswordViewerDlg, CDialog)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedRadio2)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_DELETE_PASSWORDS, OnBnClickedDeletePasswords)
	ON_BN_CLICKED(IDC_DELETE_ALL_PASSWORDS, OnBnClickedDeleteAllPasswords)
	ON_BN_CLICKED(IDC_DISPLAY_PASSWORDS, OnBnClickedDisplayPasswords)
END_MESSAGE_MAP()


// Gestionnaires de messages CPasswordViewerDlg

BOOL CPasswordViewerDlg::OnInitDialog()
{
	// XXX Must display an error message
	if (!m_passwordManager) {
		PostMessage(WM_CLOSE, 0, 0);
		return TRUE;
	}

	CDialog::OnInitDialog();
	CButton* radio = (CButton*)GetDlgItem(IDC_RADIO1);

	CString header;
	header.LoadString(IDS_HEADER_SITE);
	m_cPasswordsList.InsertColumn(0, header, LVCFMT_LEFT, 0, 0);
	m_cPasswordsList.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_reject = TRUE;
	OnBnClickedRadio1();
	radio->SetCheck(BST_CHECKED);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPasswordViewerDlg::EmptyList()
{
	while (!m_PasswordsList.IsEmpty())
	{
		//delete(m_PasswordsList.GetHead());
		m_PasswordsList.RemoveHead();
	}
}

void CPasswordViewerDlg::FillHosts(PRUnichar** logins, uint32_t count)
{	
	PRBool ret;

	LVITEM lvItem;
	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.state = 0;
	lvItem.stateMask = 0;
	lvItem.pszText = 0;
	lvItem.iItem	= 0xffff;


	while (count-->0)
	{
		//CPassword* password = new CPassword(nsPassword);
		PRUnichar *host = logins[count];
		POSITION p = m_HostsList.AddHead(host);

		lvItem.lParam = (LPARAM)p;
		int index = m_cPasswordsList.InsertItem(&lvItem);

		if (index==-1) continue;
		m_cPasswordsList.SetItemText(index, 0, host);
	}
	m_cPasswordsList.SortItems(SortHostsList, (LPARAM) &m_PasswordsList);
}

void CPasswordViewerDlg::FillPasswords(nsILoginInfo** logins, uint32_t count)
{
	PRBool ret;

	LVITEM lvItem;
	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.state = 0;
	lvItem.stateMask = 0;
	lvItem.pszText = 0;
	lvItem.iItem	= 0xffff;


	while (count-->0)
	{
		//CPassword* password = new CPassword(nsPassword);
		nsILoginInfo *password = logins[count];
		POSITION p = m_PasswordsList.AddHead(password);

		lvItem.lParam = (LPARAM)p;
		int index = m_cPasswordsList.InsertItem(&lvItem);

		if (index==-1) continue;

		nsString str;
		password->GetHostname(str);
		m_cPasswordsList.SetItemText(index, 0, str.get());

		password->GetUsername(str);
		m_cPasswordsList.SetItemText(index, 1, str.get());

		password->GetPassword(str);
		m_cPasswordsList.SetItemText(index, 2, str.get());
	}
	m_cPasswordsList.SortItems(SortPasswordsList, (LPARAM) &m_PasswordsList);
}

int CALLBACK CPasswordViewerDlg::SortHostsList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CHostList* pHostList = (CHostList*) lParamSort;

	PRUnichar* host1 = pHostList->GetAt((POSITION)lParam1);
	PRUnichar* host2 = pHostList->GetAt((POSITION)lParam2);
	return wcscmp(host1, host2);
}

int CALLBACK CPasswordViewerDlg::SortPasswordsList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CPasswordList* pPasswordList = (CPasswordList*) lParamSort;

	nsILoginInfo* password1 = pPasswordList->GetAt((POSITION)lParam1);
	nsILoginInfo* password2 = pPasswordList->GetAt((POSITION)lParam2);

	nsString host1, host2;
	password1->GetHostname(host1);
	password1->GetHostname(host2);
	return wcscmp(host1.get(), host2.get());
}

void CPasswordViewerDlg::OnBnClickedRadio1()
{
	if (m_reject == FALSE) return;
	m_reject = FALSE;

	nsCOMPtr<nsISimpleEnumerator> enumPassword;
	nsresult rv;

	m_cPasswordsList.DeleteAllItems();
	EmptyList();

	CString header;
	header.LoadString(IDS_HEADER_USERNAME);
	m_cPasswordsList.InsertColumn(1, header, LVCFMT_LEFT, 0, 1);
	header.LoadString(IDS_HEADER_PASSWORD);
	m_cPasswordsList.InsertColumn(2, header, LVCFMT_LEFT, 0, 1);

	nsILoginInfo** logins;
	uint32_t count;
	rv = m_passwordManager->GetAllLogins(&count, &logins);
	if (NS_FAILED(rv)) return;

	FillPasswords(logins, count);
	ResizeColumns();

	CWnd* button = GetDlgItem(IDC_DISPLAY_PASSWORDS);
	if (button)
		button->ShowWindow(SW_SHOW);
}

void CPasswordViewerDlg::OnBnClickedRadio2()
{
	if (m_reject == TRUE) return;
	m_reject = TRUE;

	nsCOMPtr<nsISimpleEnumerator> enumPassword;
	nsresult rv;

	m_cPasswordsList.DeleteAllItems();
	m_cPasswordsList.DeleteColumn(2);
	m_cPasswordsList.DeleteColumn(1);
	EmptyList();

	PRUnichar** hosts;
	uint32_t count;
	rv = m_passwordManager->GetAllDisabledHosts(&count, &hosts);
	if (NS_FAILED(rv)) return;

	FillHosts(hosts, count);

	RECT rect;
	m_cPasswordsList.GetClientRect(&rect);
	int width = (rect.right - rect.left);
	m_cPasswordsList.SetColumnWidth(0, width);
	CWnd* button = GetDlgItem(IDC_DISPLAY_PASSWORDS);
	if (button)
		button->ShowWindow(SW_HIDE);
}

void CPasswordViewerDlg::OnDestroy()
{
	CDialog::OnDestroy();
	EmptyList();
}

void CPasswordViewerDlg::OnBnClickedDeletePasswords()
{
	UINT uSelectedCount = m_cPasswordsList.GetSelectedCount();
	int nItem = -1;
	nsresult rv;

	if (uSelectedCount>0)
	{
		UINT i;
		for (i=0;i < uSelectedCount;i++)
		{
			nItem = m_cPasswordsList.GetNextItem(nItem, LVNI_SELECTED);
			ASSERT(nItem != -1);
			POSITION p = (POSITION)m_cPasswordsList.GetItemData(nItem);

			if (!m_reject)
			{
				nsILoginInfo* password = m_PasswordsList.GetAt(p);
				rv = m_passwordManager->RemoveLogin(password);
				m_PasswordsList.RemoveAt(p);
			}
			else 
			{
				PRUnichar* host = m_HostsList.GetAt(p);
				rv = m_passwordManager->SetLoginSavingEnabled(nsString(host), true);
				m_HostsList.RemoveAt(p);
			}

			if (NS_FAILED(rv))
			{
				AfxMessageBox(IDS_FAILED_DELETE_PASSWORD);
				return;
			}


		}

		for (i=0;i < uSelectedCount;i++)
		{
			nItem = m_cPasswordsList.GetNextItem(-1, LVNI_SELECTED);
			m_cPasswordsList.DeleteItem(nItem);
		}
	}
}

void CPasswordViewerDlg::OnBnClickedDeleteAllPasswords()
{
	if (AfxMessageBox(IDS_CONFIRM_DELETEALLPASSWORDS, MB_OKCANCEL | MB_ICONWARNING) == IDOK) 
	{
		nsresult rv;
		if (!m_reject) {
			rv = m_passwordManager->RemoveAllLogins();
			if (NS_SUCCEEDED(rv)) m_cPasswordsList.DeleteAllItems();
		}
		else while (m_cPasswordsList.GetItemCount())
		{
			UINT nItem = m_cPasswordsList.GetTopIndex();
			POSITION p = (POSITION)m_cPasswordsList.GetItemData(nItem);
			PRUnichar* host = m_HostsList.GetAt(p);
			rv = m_passwordManager->SetLoginSavingEnabled(nsString(host), true);
			m_cPasswordsList.DeleteItem(nItem);			
		}

	}
}

void CPasswordViewerDlg::OnBnClickedDisplayPasswords()
{
	if (m_reject) return;
	m_bShowPasswords = !m_bShowPasswords;

	UINT textId;
	if (!m_bShowPasswords) 
		textId = IDS_SHOW_PASSWORDS;
	else
		textId = IDS_HIDE_PASSWORDS;

	CString buttonText;
	buttonText.LoadString(textId);
	SetDlgItemText(IDC_DISPLAY_PASSWORDS, buttonText);
	ResizeColumns();
}

void CPasswordViewerDlg::ResizeColumns() 
{
	RECT rect;
	int width;
	m_cPasswordsList.GetClientRect(&rect);
	if (m_bShowPasswords) {
		width = (rect.right - rect.left)/3;
		m_cPasswordsList.SetColumnWidth(2, width);
	}
	else {
		width = (rect.right - rect.left)/2;
		m_cPasswordsList.SetColumnWidth(2, 0);
	}

	m_cPasswordsList.SetColumnWidth(0, width);
	m_cPasswordsList.SetColumnWidth(1, width);
}
