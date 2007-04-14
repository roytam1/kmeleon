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

#include "nsIPermission.h"
#include "nsIPermissionManager.h"

extern nsresult NewURI(nsIURI **result, const nsACString &spec);

class CPermission
{
public:
	nsEmbedCString m_host;
	UINT m_state;
	
	CPermission(char* host, UINT state)
	{
		m_host = host;
		m_state = state;
	}

	CPermission(nsIPermission* permission)
	{
		permission->GetHost(m_host);

		PRUint32 cap;
		permission->GetCapability(&cap);
		m_state = cap;
	}

	~CPermission()
	{
	}
};

typedef CList<CPermission*, CPermission*> CPermissionList;

class CPermissions 
{
private:
	
	nsCOMPtr<nsIPermissionManager> m_permissionManager;
	char* m_type;

public:
	
	CPermissions(char* const type)
	{
		m_permissionManager = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
		m_type = type;
	}

	~CPermissions()
	{}

	BOOL set(char* url, int state)
	{
		nsresult rv;
		
		nsEmbedCString nsurl;
		nsurl = url;
		
		// http is needed to make an nsIURI
		if (!strstr(url, "http://"))
			nsurl.Insert("http://",0);

		nsCOMPtr<nsIURI> uri;
		rv = NewURI(getter_AddRefs(uri), nsurl);
		NS_ENSURE_SUCCESS(rv, FALSE);
		
		rv = m_permissionManager->Add(uri, m_type, state);
		NS_ENSURE_SUCCESS(rv, FALSE);
		return TRUE;
	}

	BOOL del(CPermission* permission)
	{
		nsresult rv;

		rv = m_permissionManager->Remove(permission->m_host, m_type);
		NS_ENSURE_SUCCESS(rv, FALSE);
		return TRUE;
	}

	BOOL list(CPermissionList &pList)
	{
		nsresult rv;
				
		nsCOMPtr<nsISimpleEnumerator> enumPermission;
        rv = m_permissionManager->GetEnumerator(getter_AddRefs(enumPermission));
        if (NS_FAILED(rv)) return FALSE;

		PRBool ret;
		enumPermission->HasMoreElements(&ret);
		while (ret)
		{
			nsCOMPtr<nsIPermission> nsPermission;
			rv = enumPermission->GetNext(getter_AddRefs(nsPermission));
			if (NS_FAILED(rv)) break;
			
			nsEmbedCString nsType;
			nsPermission->GetType(nsType);
			if (nsType.Equals(nsDependentCString(m_type)))
				pList.AddHead(new CPermission(nsPermission));
		}
		
		return TRUE;
	}
};

// Boîte de dialogue CPermissionsDlg

class CPermissionsDlg : public CDialog
{
	//DECLARE_DYNAMIC(CPermissionsDlg)

public:
	CPermissionsDlg(char* type, CWnd* pParent = NULL);   // constructeur standard
	virtual ~CPermissionsDlg();

// Données de boîte de dialogue
	enum { IDD = IDD_PERMISSIONS };

protected:
	CPermissions* m_permissions;
	CPermissionList m_PermissionsList;
	char* m_type;

	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	void OnNewPermission(int state);
	void FillList();
	static int CALLBACK SortPermissionsList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_cPermissionsList;
	afx_msg void OnBnClickedBlock();
	afx_msg void OnBnClickedAllow();
	afx_msg void OnBnClickedDeletePermissions();
	afx_msg void OnBnClickedDeleteAllPermissions();
	afx_msg void OnEnChangeUrl();
	afx_msg void OnBnClickedAllowsession();
	afx_msg void OnNMClickListPermissions(NMHDR *pNMHDR, LRESULT *pResult);
};
