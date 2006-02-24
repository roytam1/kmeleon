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

// Boîte de dialogue CPasswordViewerDlg

//IMPLEMENT_DYNAMIC(CPasswordViewerDlg, CDialog)
CPasswordViewerDlg::CPasswordViewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswordViewerDlg::IDD, pParent)
{
	m_passwordManager = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
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
END_MESSAGE_MAP()


// Gestionnaires de messages CPasswordViewerDlg

BOOL CPasswordViewerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CButton* radio = (CButton*)GetDlgItem(IDC_RADIO1);
		
	m_cPasswordsList.InsertColumn(0, _T("Site"), LVCFMT_LEFT, 0, 0);
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
		delete(m_PasswordsList.GetHead());
		m_PasswordsList.RemoveHead();
	}
}

void CPasswordViewerDlg::FillList(nsISimpleEnumerator* enumPassword)
{
	PRBool ret;
	nsresult rv;

	LVITEM lvItem;
	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.state = 0;
	lvItem.stateMask = 0;
	lvItem.pszText = 0;
	lvItem.iItem	= 0xffff;
	
	enumPassword->HasMoreElements(&ret);
	while (ret)
    {
		nsCOMPtr<nsIPassword> nsPassword;
        rv = enumPassword->GetNext(getter_AddRefs(nsPassword));
		if (NS_FAILED(rv)) break;

		CPassword* password = new CPassword(nsPassword);
		POSITION p = m_PasswordsList.AddHead(password);

		lvItem.lParam = (LPARAM)p;
		int index = m_cPasswordsList.InsertItem(&lvItem);

		if (index==-1) continue;

		ListView_SetItemText(m_cPasswordsList.GetSafeHwnd(), index, 0, password->m_csHost.GetBuffer(0))
		ListView_SetItemText(m_cPasswordsList.GetSafeHwnd(), index, 1, password->m_csUsername.GetBuffer(0))
		
		enumPassword->HasMoreElements(&ret);
	}
	m_cPasswordsList.SortItems(SortPasswordsList, (LPARAM) &m_PasswordsList);
}

int CALLBACK CPasswordViewerDlg::SortPasswordsList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CList<CPassword*>* pPasswordList = (CList<CPassword*>*) lParamSort;

   CPassword* password1 = pPasswordList->GetAt((POSITION)lParam1);
   CPassword* password2 = pPasswordList->GetAt((POSITION)lParam2);

   return strcmp(password1->m_host.get(), password2->m_host.get());
}

void CPasswordViewerDlg::OnBnClickedRadio1()
{
	if (m_reject == FALSE) return;
	m_reject = FALSE;
		
	nsCOMPtr<nsISimpleEnumerator> enumPassword;
	nsresult rv;

	m_cPasswordsList.DeleteAllItems();
	EmptyList();

	m_cPasswordsList.InsertColumn(1, _T("Username"), LVCFMT_LEFT, 0, 1);

	rv = m_passwordManager->GetEnumerator(getter_AddRefs(enumPassword));
	if (NS_FAILED(rv)) return;

	FillList(enumPassword);

	RECT rect;
	m_cPasswordsList.GetClientRect(&rect);
	int width = (rect.right - rect.left)/2;
	m_cPasswordsList.SetColumnWidth(0, width);
	m_cPasswordsList.SetColumnWidth(1, width);

	// TODO : ajoutez ici le code de votre gestionnaire de notification de contrôle
}

void CPasswordViewerDlg::OnBnClickedRadio2()
{
	if (m_reject == TRUE) return;
	m_reject = TRUE;

    nsCOMPtr<nsISimpleEnumerator> enumPassword;
	nsresult rv;

	m_cPasswordsList.DeleteAllItems();
	m_cPasswordsList.DeleteColumn(1);
	EmptyList();

	rv = m_passwordManager->GetRejectEnumerator(getter_AddRefs(enumPassword));
	if (NS_FAILED(rv)) return;

	FillList(enumPassword);

	RECT rect;
	m_cPasswordsList.GetClientRect(&rect);
	int width = (rect.right - rect.left);
	m_cPasswordsList.SetColumnWidth(0, width);
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
			CPassword* password = m_PasswordsList.GetAt(p);
				
			if (!m_reject)
				rv = m_passwordManager->RemoveUser(password->m_host, password->m_username);
			else
				rv = m_passwordManager->RemoveReject(password->m_host);

			if (NS_FAILED(rv))
			{
				AfxMessageBox(IDS_FAILED_DELETE_PASSWORD);
				return;
			}

			m_PasswordsList.RemoveAt(p);
			delete password;
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
		while (!m_PasswordsList.IsEmpty())
		{
			CPassword* password = m_PasswordsList.GetHead();
			if (!m_reject)
				m_passwordManager->RemoveUser(password->m_host, password->m_username);
			else
				m_passwordManager->RemoveReject(password->m_host);
			delete(password);
			m_PasswordsList.RemoveHead();
		}
		m_cPasswordsList.DeleteAllItems();
	}
}
