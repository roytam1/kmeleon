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

#include "DialogEx.h"
#include "nsICertificateDialogs.h"
#include "nsIArray.h"
#include "nsIASN1Object.h"
#include "resource.h"

#ifndef XP_WIN
#define XP_WIN
#endif

//f5998be6-866c-4db9-8c7c-9858ec790088
#define NS_NSSDIALOGS_CID \
 {0xF5998BE6, 0x866C, 0x4DB9, {0x8c, 0x7c, 0x98, 0x58, 0xec, 0x79, 0x00, 0x88}}
static NS_DEFINE_CID(kNSSDialogsCID, NS_NSSDIALOGS_CID);


class CNSSDialogs:public nsICertificateDialogs
{
public:
                 CNSSDialogs();
  virtual       ~CNSSDialogs();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICERTIFICATEDIALOGS
};


// Boîte de dialogue CConfirmCertExpiredDialog

class CConfirmCertExpiredDialog : public CDialog
{
	//DECLARE_DYNAMIC(CConfirmCertExpiredDialog)

public:
	CConfirmCertExpiredDialog(CWnd* pParent, const TCHAR* title, 
		const TCHAR* msgn, const TCHAR* ctime,
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert);   
	virtual ~CConfirmCertExpiredDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_CONFIRM_EXPIRED_CERT };

	BOOL m_bTrustWeb, m_bTrustEmail, m_bTrustSoftdev;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	CString m_csDialogTitle;
    CString m_csMsg;
	CString m_csCorrectTime;
	nsIInterfaceRequestor* m_ctx;
	nsIX509Cert* m_cert;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedHelpCert();
	afx_msg void OnBnClickedViewCert();
};


// Boîte de dialogue CNewServerDialog

class CNewServerDialog : public CDialog
{
	//DECLARE_DYNAMIC(CNewServerDialog)

public:
	CNewServerDialog(CWnd* pParent, const TCHAR* intro, 
		const TCHAR *reason3, const TCHAR* question, 
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert);
	
	virtual ~CNewServerDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_NEWSERVER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	nsIInterfaceRequestor *m_ctx;
	nsIX509Cert *m_cert;
	CString m_csIntro;
    CString m_csQuestion;
	CString m_csReason3;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedViewCert();
	virtual BOOL OnInitDialog();
	int m_addType;
};

// Boîte de dialogue CDomainMismatchDialog

class CDomainMismatchDialog : public CDialog
{
	//DECLARE_DYNAMIC(CDomainMismatchDialog)

public:
	CDomainMismatchDialog(CWnd* pParent, 
		const TCHAR* msg1, const TCHAR* msg2,
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert);   // constructeur standard
	virtual ~CDomainMismatchDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_DOMAIN_MISMATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	nsIInterfaceRequestor *m_ctx;
	nsIX509Cert *m_cert;
	CString m_csMsg1;
	CString m_csMsg2;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedViewCert();
};



// Boîte de dialogue CServerCrlNextupdateDialog

class CServerCrlNextupdateDialog : public CDialog
{
	//DECLARE_DYNAMIC(CServerCrlNextupdateDialog)

public:
	CServerCrlNextupdateDialog(CWnd* pParent, const TCHAR* msg1, const TCHAR* msg2);   // constructeur standard
	virtual ~CServerCrlNextupdateDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_SERVERCRLNEXTUPDATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	CString m_csMsg1;
	CString m_csMsg2;

	DECLARE_MESSAGE_MAP()
};

// Boîte de dialogue CDownloadCertDialog

class CDownloadCertDialog : public CDialog
{
	//DECLARE_DYNAMIC(CDownloadCertDialog)

public:
	CDownloadCertDialog(CWnd* pParent, const TCHAR* msg2, 
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert);   // constructeur standard
	virtual ~CDownloadCertDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_DOWNLOAD_CERT };

	BOOL m_bTrustWeb, m_bTrustEmail, m_bTrustSoftdev;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	nsIInterfaceRequestor *m_ctx;
	nsIX509Cert *m_cert;
	CString m_csMsg2;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedViewCert();
};


// Boîte de dialogue CGetPKCS12FilePasswordDialog

class CGetPKCS12FilePasswordDialog : public CDialog
{
	//DECLARE_DYNAMIC(CGetPKCS12FilePasswordDialog)

public:
	CGetPKCS12FilePasswordDialog(CWnd* pParent = NULL);   // constructeur standard
	virtual ~CGetPKCS12FilePasswordDialog();
	CString m_csPwd;

// Données de boîte de dialogue
	enum { IDD = IDD_GETPKCS12FILEPASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
};

// Boîte de dialogue CSetPKCS12FilePasswordDialog

class CSetPKCS12FilePasswordDialog : public CDialog
{
	//DECLARE_DYNAMIC(CSetPKCS12FilePasswordDialog)

public:
	CSetPKCS12FilePasswordDialog(CWnd* pParent = NULL);   // constructeur standard
	virtual ~CSetPKCS12FilePasswordDialog();
	CString m_csPwd;

// Données de boîte de dialogue
	enum { IDD = IDD_SETPKCS12FILEPASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
};

// Boîte de dialogue CViewCertGeneralPage

class CViewCertGeneralPage : public CPropertyPage
{
	//DECLARE_DYNAMIC(CViewCertGeneralPage)

public:
	CViewCertGeneralPage();
	virtual ~CViewCertGeneralPage();

	CString m_csVerified,
			m_csUsage,
			m_csCN,
			m_csO,
			m_csOU,
			m_csSN,
			m_csCN2,
			m_csO2,
			m_csOU2,
			m_csIssuedDate,
			m_csExpiresDate,
			m_csSHA1,
			m_csMD5;

// Données de boîte de dialogue
	enum { IDD = IDD_VIEWCERT_GENERAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
};

// Boîte de dialogue CViewCertDetailsPage

class CViewCertDetailsPage : public CPropertyPage
{
	//DECLARE_DYNAMIC(CViewCertDetailsPage)
	nsCOMPtr<nsIArray> m_certChain;	

public:
	CList<nsIASN1Object*> m_objects;
	CViewCertDetailsPage(nsIArray* cert, CWnd* pParent = NULL);   // constructeur standard
	virtual ~CViewCertDetailsPage();

// Données de boîte de dialogue
	enum { IDD = IDD_VIEW_CERT_DETAILS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	void loadASN1Structure(CTreeCtrl* tree, nsIASN1Object* asn1Object, HTREEITEM parent);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnTvnSelchangedCertHierarchy(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedCertFields(NMHDR *pNMHDR, LRESULT *pResult);
};
