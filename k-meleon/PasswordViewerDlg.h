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
#include "afxtempl.h"
#include "resource.h"
#include "DialogEx.h"
#include "nsILoginManager.h"

class CPassword {


public:
	CString m_csHost;
	CString m_csUsername;
	CString m_csPassword;
	nsCString m_host;
	nsString m_username;

	CPassword(nsILoginInfo* login) 
	{
	}


	~CPassword()
	{
	}

};

typedef CList<nsILoginInfo*, nsILoginInfo*> CPasswordList;

// Boîte de dialogue CPasswordViewerDlg

class CPasswordViewerDlg : public CDialog
{
	//DECLARE_DYNAMIC(CPasswordViewerDlg)

public:
	CPasswordViewerDlg(CWnd* pParent = NULL);   // constructeur standard
	virtual ~CPasswordViewerDlg();
	static int CALLBACK SortPasswordsList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// Données de boîte de dialogue
	enum { IDD = IDD_PASSWORDS_VIEWER };

protected:

	CPasswordList m_PasswordsList;
	nsCOMPtr<nsILoginManager> m_passwordManager;
	BOOL m_bShowPasswords;

    BOOL m_reject;

	void FillList(nsILoginInfo** logins, uint32_t count);
	void EmptyList();
	void ResizeColumns();
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_cPasswordsList;
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedDeletePasswords();
	afx_msg void OnBnClickedDeleteAllPasswords();
	afx_msg void OnBnClickedDisplayPasswords();
};
