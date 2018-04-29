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
#include "stdafx.h"
#include <time.h>
#include "NSSDialogs.h"
#include "MozUtils.h"

//#include "nsIPKIParamBlock.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsIX509CertValidity.h"
#include "nsICertificateDialogs.h"
#include "nsIArray.h"
#include "nsIASN1Object.h"
#include "nsIASN1Sequence.h"

#include "nsIDialogParamBlock.h"
#include "nsIWindowWatcher.h"
#include "nsIMutableArray.h"


extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

NS_DEFINE_CID (kX509CertCID, NS_IX509CERT_IID);
NS_DEFINE_CID (kASN1ObjectCID, NS_IASN1OBJECT_IID);

NS_IMPL_ISUPPORTS1(CNSSDialogs, nsICertificateDialogs)				   


CNSSDialogs::CNSSDialogs()
{
  /* member initializers and constructor code */
}

CNSSDialogs::~CNSSDialogs()
{
  /* destructor code */
}

//////////////////////////////////////////////////////////////

/* boolean confirmDownloadCACert (in nsIInterfaceRequestor ctx, in nsIX509Cert cert, out unsigned long trust); */
NS_IMETHODIMP CNSSDialogs::ConfirmDownloadCACert(nsIInterfaceRequestor *ctx, nsIX509Cert *cert, uint32_t *trust, bool *_retval)
{
	// chrome://pippki/content/downloadcert.xul

	CString msg2;
	LPCTSTR sCName;
	USES_CONVERSION;

	nsEmbedString commonName;
	cert->GetCommonName (commonName);
	sCName = W2CT(commonName.get());

	msg2.Format(IDS_newCAMessage1, sCName);

	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
    CDownloadCertDialog dlg(CWndForDOMWindow(parent), msg2, ctx, cert);
	
	*_retval = (dlg.DoModal() == IDOK);

	*trust = nsIX509CertDB::UNTRUSTED;
	*trust |= (dlg.m_bTrustWeb) ? nsIX509CertDB::TRUSTED_SSL : 0;
	*trust |= (dlg.m_bTrustEmail) ? nsIX509CertDB::TRUSTED_EMAIL : 0;
	*trust |= (dlg.m_bTrustSoftdev) ? nsIX509CertDB::TRUSTED_OBJSIGN : 0;

    return NS_OK;
}

/* void notifyCACertExists (in nsIInterfaceRequestor ctx); */
NS_IMETHODIMP CNSSDialogs::NotifyCACertExists(nsIInterfaceRequestor *ctx)
{
	// chrome://pippki/content/cacertexists.xul
/*
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
	CString msg;

	
	msg.LoadString(IDS_CERTEXISTS);
	CWnd* wnd = CWndForDOMWindow(parent);
	
	if (wnd)
		wnd->MessageBox(msg, "", MB_OK | MB_ICONEXCLAMATION);
	else
		::MessageBox(NULL, msg, "", MB_OK | MB_ICONEXCLAMATION);
	*/

	AfxMessageBox(IDS_CERTEXISTS, MB_OK | MB_ICONEXCLAMATION, 0);
    return NS_OK;
}

/* boolean setPKCS12FilePassword (in nsIInterfaceRequestor ctx, out AString password); */
NS_IMETHODIMP CNSSDialogs::SetPKCS12FilePassword(nsIInterfaceRequestor *ctx, nsAString & password, bool *_retval)
{
	//chrome://pippki/content/setp12password.xul
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
	CSetPKCS12FilePasswordDialog dlg(CWndForDOMWindow(parent));
	if (dlg.DoModal() == IDOK)
	{
		*_retval = PR_TRUE;
		USES_CONVERSION;
		password = T2CW(dlg.m_csPwd);
	}
	else *_retval = PR_FALSE;


    return NS_OK;
}

/* boolean getPKCS12FilePassword (in nsIInterfaceRequestor ctx, out AString password); */
NS_IMETHODIMP CNSSDialogs::GetPKCS12FilePassword(nsIInterfaceRequestor *ctx, nsAString & password, bool *_retval)
{
	//chrome://pippki/content/getp12password.xul

	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
	CGetPKCS12FilePasswordDialog dlg(CWndForDOMWindow(parent));
	*_retval = (dlg.DoModal() == IDOK);
	if (*_retval)
	{
		USES_CONVERSION;
		NS_StringCopy(password, nsEmbedString(T2W(dlg.m_csPwd.GetBuffer(0))));
	}

    return NS_OK;
}

class CViewCertDialog: public CPropertySheet 
{
public:
	CViewCertDialog(UINT nIDCaption, CWnd* pParentWnd = NULL,
		UINT iSelectPage = 0) : CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
	{
	}

	//DECLARE_MESSAGE_MAP()
	BOOL CViewCertDialog::OnInitDialog()
	{
		BOOL bResult = CPropertySheet::OnInitDialog();
		CWnd* wnd;
		if (wnd = GetDlgItem(IDOK))
			wnd->ShowWindow(SW_HIDE);

		if (wnd=GetDlgItem(IDCANCEL)) {
			CString text;
			text.LoadString(IDS_CLOSE);
			wnd->SetWindowText(text);
		}

		return bResult;
	}
	//virtual BOOL OnInitDialog();
};
/* void viewCert (in nsIInterfaceRequestor ctx, in nsIX509Cert cert); */
NS_IMETHODIMP CNSSDialogs::ViewCert(nsIInterfaceRequestor *ctx, nsIX509Cert *cert)
{
	// chrome://pippki/content/certViewer.xul
	USES_CONVERSION;

	PRUint32 verifyState, count;
	PRUnichar ** usageList;
	CString state;
	nsEmbedString value;

	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
	CWnd* wnd = CWndForDOMWindow(parent);
	if (wnd)
		wnd = wnd->GetLastActivePopup();
	
	CViewCertDialog viewCert(IDS_CERT_VIEWER_TITLE, wnd);
	CViewCertGeneralPage viewCertGeneral;
	
	nsCOMPtr<nsIArray> certChain;
	cert->GetChain (getter_AddRefs(certChain));
	CViewCertDetailsPage viewCertDetails(certChain);

	nsresult rv = cert->GetUsagesArray (FALSE, &verifyState, &count, &usageList);
	if (NS_FAILED(rv)) return rv;

	switch (verifyState)
	{
		case nsIX509Cert::VERIFIED_OK:
			state.LoadString(IDS_CERT_VERIFIED_OK);
			break;
		case nsIX509Cert::CERT_REVOKED:
			state.LoadString(IDS_CERT_REVOKED);
			break;
		case nsIX509Cert::CERT_EXPIRED:
			state.LoadString(IDS_CERT_EXPIRED);
			break;
		case nsIX509Cert::CERT_NOT_TRUSTED:
			state.LoadString(IDS_CERT_NOT_TRUSTED);
			break;
		case nsIX509Cert::ISSUER_NOT_TRUSTED:
			state.LoadString(IDS_CERT_ISSUER_NOT_TRUSTED);
			break;
		case nsIX509Cert::ISSUER_UNKNOWN:
			state.LoadString(IDS_CERT_ISSUER_UNKNOWN);
			break;
		case nsIX509Cert::INVALID_CA:
			state.LoadString(IDS_CERT_INVALID_CA);
			break;
		default:
			state.LoadString(IDS_CERT_NOT_VERIFIED_UNKNOWN);
	}

	viewCertGeneral.m_csVerified = state;

	if (count>0)
		for (UINT i=0; i<count; i++)
			viewCertGeneral.m_csUsage += W2CT(usageList[i]) + CString(_T("\r\n"));
	
	cert->GetCommonName(value);
	viewCertGeneral.m_csCN = W2CT(value.get());

	cert->GetOrganization(value);
	viewCertGeneral.m_csO = W2CT(value.get());

	cert->GetOrganizationalUnit(value);
	viewCertGeneral.m_csOU = W2CT(value.get());

	cert->GetSerialNumber(value);
	viewCertGeneral.m_csSN = W2CT(value.get());

	cert->GetIssuerCommonName(value);
	viewCertGeneral.m_csCN2 = W2CT(value.get());

	cert->GetIssuerOrganization(value);
	viewCertGeneral.m_csCN2 = W2CT(value.get());

	cert->GetIssuerOrganizationUnit(value);
	viewCertGeneral.m_csOU2 = W2CT(value.get());

	nsCOMPtr<nsIX509CertValidity> validity;
	cert->GetValidity(getter_AddRefs(validity));
		
	validity->GetNotBeforeLocalDay (value);
	viewCertGeneral.m_csIssuedDate = W2CT(value.get());

	validity->GetNotAfterLocalDay(value);
	viewCertGeneral.m_csExpiresDate = W2CT(value.get());
		
	cert->GetSha1Fingerprint (value);
	viewCertGeneral.m_csSHA1 = W2CT(value.get());

	/*cert->GetMd5Fingerprint (value);
	viewCertGeneral.m_csMD5 = W2CT(value.get());*/
	
	viewCert.m_psh.dwFlags |= PSH_NOAPPLYNOW;
	viewCert.AddPage(&viewCertGeneral);
	viewCert.AddPage(&viewCertDetails);
	//viewCert.Create(CWndForDOMWindow(parent));
	viewCert.DoModal();

    return NS_OK;
}

//////////////////////////////////////////////////////////////

#if GECKO_VERSION == 18

/* boolean confirmUnknownIssuer (in nsIInterfaceRequestor socketInfo, in nsIX509Cert cert, out short certAddType); */
NS_IMETHODIMP CNSSDialogs::ConfirmUnknownIssuer(nsIInterfaceRequestor *socketInfo, nsIX509Cert *cert, PRInt16 *certAddType, PRBool *_retval)
{
	// chrome://pippki/content/newserver.xul
	CString csIntro,csQuestion,csReason3;
	LPCTSTR sCName;
	USES_CONVERSION;

	nsEmbedString commonName;
	cert->GetCommonName (commonName);
	sCName = W2CT(commonName.get());

	csIntro.Format(IDS_NEWSERVER_INTRO, sCName);
	csQuestion.Format(IDS_NEWSERVER_QUESTION, sCName);
	csReason3.Format(IDS_NEWSERVER_REASON3, sCName);
	
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (socketInfo);
    CNewServerDialog dlg(CWndForDOMWindow(parent), csIntro, csReason3, csQuestion, socketInfo, cert);
	
	if (dlg.DoModal() == IDOK)
	{
		switch (dlg.m_addType) {
			case 0:
				*certAddType = ADD_TRUSTED_PERMANENTLY;
				*_retval    = PR_TRUE;
				break;
			case 1:
				*certAddType = ADD_TRUSTED_FOR_SESSION;
				*_retval    = PR_TRUE;
			break;
			default:
				*certAddType = UNINIT_ADD_FLAG;
				*_retval    = PR_FALSE;
			break;
		}
	}
	else
		*_retval = PR_FALSE;

	return NS_OK;
}

/* boolean confirmMismatchDomain (in nsIInterfaceRequestor socketInfo, in AUTF8String targetURL, in nsIX509Cert cert); */
NS_IMETHODIMP CNSSDialogs::ConfirmMismatchDomain(nsIInterfaceRequestor *socketInfo, const nsACString & targetURL, nsIX509Cert *cert, PRBool *_retval)
{
	// chrome://pippki/content/domainMismatch.xul

	CString msg1,msg2;
	CString sTUrl =  NSUTF8StringToCString(nsEmbedCString(targetURL));

	nsEmbedString commonName;
	cert->GetCommonName (commonName);
	
	msg1.Format(IDS_mismatchDomainMsg1, sTUrl, NSStringToCString(commonName));
	msg2.Format(IDS_mismatchDomainMsg2, sTUrl);
	
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (socketInfo);
	CDomainMismatchDialog dlg(CWndForDOMWindow(parent), msg1, msg2, socketInfo, cert);

	*_retval = (dlg.DoModal() == IDOK);
	
    return NS_OK;
}

/* boolean confirmCertExpired (in nsIInterfaceRequestor socketInfo, in nsIX509Cert cert); */
NS_IMETHODIMP CNSSDialogs::ConfirmCertExpired(nsIInterfaceRequestor *socketInfo, nsIX509Cert *cert, PRBool *_retval)
{
  nsresult rv;
  PRTime now = PR_Now();
  PRTime notAfter, notBefore;
  nsCOMPtr<nsIX509CertValidity> validity;
  CString msg, title, ctime;
  TCHAR fDate[128];
  __time64_t t;

  *_retval = PR_FALSE;

  rv = cert->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotAfter(&notAfter);
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotBefore(&notBefore);
  if (NS_FAILED(rv))
    return rv;
  
  nsEmbedString commonName;
  cert->GetCommonName (commonName);

  USES_CONVERSION;
  CString cName(W2CT(commonName.get()));

  if (LL_CMP(now, >, notAfter)) {
    msg.LoadString(IDS_CERTEXPIREDTITLE); 
	LL_DIV(t, notAfter, PR_USEC_PER_SEC);
	_tcsftime (fDate, sizeof(fDate), _T("%a %d %b %Y"), _localtime64 (&t));
	title.Format(IDS_CERTEXPIREDMSG, cName, fDate);
  } else {
    msg.LoadString(IDS_CERTNOTVALIDTITLE);
	LL_DIV(t, notBefore, PR_USEC_PER_SEC);
    _tcsftime (fDate, sizeof(fDate), _T("%a %d %b %Y"), _localtime64 (&t));
	title.Format(IDS_CERTNOTVALIDMSG, cName, fDate);
  }

  _time64(&t);
  _tcsftime (fDate, sizeof(fDate), _T("%a %d %b %Y"), _localtime64 (&t));
  ctime.Format(IDS_CORRECT_COMPUTER_TIME, fDate);
  
  nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (socketInfo);
  CConfirmCertExpiredDialog dlg(CWndForDOMWindow(parent), msg, title, ctime, socketInfo, cert);
  
  *_retval = (dlg.DoModal() == IDOK);

  return NS_OK;
}

/* void notifyCrlNextupdate (in nsIInterfaceRequestor socketInfo, in AUTF8String targetURL, in nsIX509Cert cert); */
NS_IMETHODIMP CNSSDialogs::NotifyCrlNextupdate(nsIInterfaceRequestor *socketInfo, const nsACString & targetURL, nsIX509Cert *cert)
{
	// chrome://pippki/content/serverCrlNextupdate.xul
	CString msg1,msg2;
	LPCTSTR sCName;
	CString sTUrl =  NSUTF8StringToCString(nsEmbedCString(targetURL));
	nsEmbedString commonName;

	USES_CONVERSION;
	
	cert->GetCommonName (commonName);
	sCName = W2CT(commonName.get());

	msg1.Format(IDS_crlNextUpdateMsg1, sTUrl);
	msg2.Format(IDS_crlNextUpdateMsg2, sCName);
	
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (socketInfo);
/*	CServerCrlNextupdateDialog dlg(CWndForDOMWindow(parent), msg1, msg2);
	dlg.DoModal();*/
	
	CString msg3;
	msg3.LoadString(IDS_crlNextUpdateMsg3);
	CString msg = msg1 + _T(" ") + msg2 + _T(" ") + msg3;
	CWnd* wnd = CWndForDOMWindow(parent);
	if (wnd)
		wnd->MessageBox(msg, _T(""), MB_OK | MB_ICONEXCLAMATION);
	else
		::MessageBox(NULL, msg, _T(""), MB_OK | MB_ICONEXCLAMATION);
	

	return NS_OK;
}

#endif // GECKO_VERSION

// Boîte de dialogue CConfirmCertExpiredDialog

//IMPLEMENT_DYNAMIC(CConfirmCertExpiredDialog, CDialog)
CConfirmCertExpiredDialog::CConfirmCertExpiredDialog(CWnd* pParent, const TCHAR* title, 
		const TCHAR* msg, const TCHAR* ctime,
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert)
		: CDialog(CConfirmCertExpiredDialog::IDD, pParent)
{
	m_csDialogTitle = title;
	m_csCorrectTime = ctime;
	m_csMsg = msg;
	m_cert = cert;
	m_ctx = ctx;
}

CConfirmCertExpiredDialog::~CConfirmCertExpiredDialog()
{
}

void CConfirmCertExpiredDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfirmCertExpiredDialog, CDialog)
	ON_BN_CLICKED(IDC_HELP_CERT, OnBnClickedHelpCert)
	ON_BN_CLICKED(IDC_VIEW_CERT, OnBnClickedViewCert)
END_MESSAGE_MAP()


// Gestionnaires de messages CConfirmCertExpiredDialog

BOOL CConfirmCertExpiredDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_csDialogTitle);
	CWnd *pWnd = GetDlgItem(IDC_STATIC_MSG);
    if(pWnd)
        pWnd->SetWindowText(m_csMsg);

	pWnd = GetDlgItem(IDC_STATIC_CORRECTIME);
    if(pWnd)
        pWnd->SetWindowText(m_csCorrectTime);

	return TRUE;  
}

void CConfirmCertExpiredDialog::OnBnClickedHelpCert()
{
	/*nsCOMPtr<nsIDialogParamBlock> params = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID);
	nsIDOMWindow* dm;
	
	params->SetNumberStrings(2);	
    params->SetString(0, L"chrome://help/locale/mozillahelp.rdf");
    params->SetString(1, L"exp_web_cert");
    nsCOMPtr<nsIWindowWatcher> mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (mWWatch)
	{
		mWWatch->OpenWindow(nullptr, "chrome://help/content/help.xul", "_blank", "chrome,all,alwaysRaised,dialog=no", params, &dm);
		CWndForDOMWindow(dm)->SetFocus();
	}*/
}

void CConfirmCertExpiredDialog::OnBnClickedViewCert()
{
	nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID);

	if (certDialogs)
		certDialogs->ViewCert (m_ctx, m_cert);
}

// Boîte de dialogue CNewServerDialog

//IMPLEMENT_DYNAMIC(CNewServerDialog, CDialog)
CNewServerDialog::CNewServerDialog(CWnd* pParent, const TCHAR* intro, 
		const TCHAR *reason3, const TCHAR* question, 
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert)
	: CDialog(CNewServerDialog::IDD, pParent)
{
	m_csIntro = intro;
	m_csQuestion = question;
	m_csReason3 = reason3;
	m_ctx = ctx;
	m_cert = cert;
	m_addType = 0;
}

CNewServerDialog::~CNewServerDialog()
{
}

void CNewServerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewServerDialog)
    DDX_Text(pDX, IDC_INTRO, m_csIntro);
	DDX_Text(pDX, IDC_QUESTION, m_csQuestion);
	DDX_Text(pDX, IDC_REASON3, m_csReason3);
	DDX_Radio(pDX, IDC_REMEMBER, m_addType);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewServerDialog, CDialog)
	ON_BN_CLICKED(IDC_VIEW_CERT, OnBnClickedViewCert)
END_MESSAGE_MAP()


// Gestionnaires de messages CNewServerDialog

void CNewServerDialog::OnBnClickedViewCert()
{
	nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID);

	if (certDialogs)
		certDialogs->ViewCert (m_ctx, m_cert);
}

BOOL CNewServerDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CheckDlgButton(IDC_REMEMBER, FALSE);
	CheckDlgButton(IDC_SESSION, TRUE);
	return TRUE;  
}


// Boîte de dialogue CDomainMismatchDialog

//IMPLEMENT_DYNAMIC(CDomainMismatchDialog, CDialog)
CDomainMismatchDialog::CDomainMismatchDialog(CWnd* pParent, 
		const TCHAR* msg1, const TCHAR* msg2,
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert)
	: CDialog(CDomainMismatchDialog::IDD, pParent)
{
	m_ctx = ctx;
	m_cert = cert;
	m_csMsg1 = msg1;
	m_csMsg2 = msg2;
}

CDomainMismatchDialog::~CDomainMismatchDialog()
{
}

void CDomainMismatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewServerDialog)
    DDX_Text(pDX, IDC_MSG1, m_csMsg1);
	DDX_Text(pDX, IDC_MSG2, m_csMsg2);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainMismatchDialog, CDialog)
	ON_BN_CLICKED(IDC_VIEW_CERT, OnBnClickedViewCert)
END_MESSAGE_MAP()


// Gestionnaires de messages CDomainMismatchDialog

void CDomainMismatchDialog::OnBnClickedViewCert()
{
	nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID);

	if (certDialogs)
		certDialogs->ViewCert (m_ctx, m_cert);
}


// Boîte de dialogue CServerCrlNextupdateDialog

//IMPLEMENT_DYNAMIC(CServerCrlNextupdateDialog, CDialog)
CServerCrlNextupdateDialog::CServerCrlNextupdateDialog(CWnd* pParent, const TCHAR* msg1, const TCHAR* msg2)
	: CDialog(CServerCrlNextupdateDialog::IDD, pParent)
{
	m_csMsg1 = msg1;
	m_csMsg2 = msg2;
}

CServerCrlNextupdateDialog::~CServerCrlNextupdateDialog()
{
}

void CServerCrlNextupdateDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewServerDialog)
    DDX_Text(pDX, IDC_MSG1, m_csMsg1);
	DDX_Text(pDX, IDC_MSG2, m_csMsg2);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServerCrlNextupdateDialog, CDialog)
END_MESSAGE_MAP()

// Boîte de dialogue CDownloadCertDialog

//IMPLEMENT_DYNAMIC(CDownloadCertDialog, CDialog)
CDownloadCertDialog::CDownloadCertDialog(CWnd* pParent, const TCHAR* msg2, 
		nsIInterfaceRequestor *ctx,	nsIX509Cert *cert)
	: CDialog(CDownloadCertDialog::IDD, pParent)
{
	m_csMsg2 = msg2;
	m_cert = cert;
	m_ctx = ctx;
	m_bTrustWeb = FALSE;
	m_bTrustEmail = FALSE;
	m_bTrustSoftdev = FALSE;

}

CDownloadCertDialog::~CDownloadCertDialog()
{
}

void CDownloadCertDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewServerDialog)
    DDX_Text(pDX, IDC_MSG2, m_csMsg2);
	DDX_Check(pDX, IDC_CHECKSSL, m_bTrustWeb);
	DDX_Check(pDX, IDC_CHECKEMAIL, m_bTrustEmail);
	DDX_Check(pDX, IDC_CHECKOBJSIGN, m_bTrustSoftdev);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDownloadCertDialog, CDialog)
	ON_BN_CLICKED(IDC_VIEW_CERT, OnBnClickedViewCert)
END_MESSAGE_MAP()


// Gestionnaires de messages CDownloadCertDialog

void CDownloadCertDialog::OnBnClickedViewCert()
{
	nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID);

	if (certDialogs)
		certDialogs->ViewCert (m_ctx, m_cert);
}

// Boîte de dialogue CGetPKCS12FilePasswordDialog

//IMPLEMENT_DYNAMIC(CGetPKCS12FilePasswordDialog, CDialog)
CGetPKCS12FilePasswordDialog::CGetPKCS12FilePasswordDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CGetPKCS12FilePasswordDialog::IDD, pParent)
{
	m_csPwd = "";
}

CGetPKCS12FilePasswordDialog::~CGetPKCS12FilePasswordDialog()
{
}

void CGetPKCS12FilePasswordDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PWD1, m_csPwd);
}


BEGIN_MESSAGE_MAP(CGetPKCS12FilePasswordDialog, CDialog)
END_MESSAGE_MAP()

// Boîte de dialogue CSetPKCS12FilePasswordDialog

//IMPLEMENT_DYNAMIC(CSetPKCS12FilePasswordDialog, CDialog)
CSetPKCS12FilePasswordDialog::CSetPKCS12FilePasswordDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSetPKCS12FilePasswordDialog::IDD, pParent)
{
}

CSetPKCS12FilePasswordDialog::~CSetPKCS12FilePasswordDialog()
{
}

void CSetPKCS12FilePasswordDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


void CSetPKCS12FilePasswordDialog::OnOK()
{
	CString pwd1,pwd2;
	GetDlgItemText(IDC_PWD1, pwd1);
	GetDlgItemText(IDC_PWD2, pwd2);
	if (pwd1.Compare(pwd2) == 0)
	{
		m_csPwd = pwd1;
		CDialog::OnOK();
	}
	else
		AfxMessageBox(IDS_PASSWORD_MISMATCH, MB_OK|MB_ICONERROR);
}

BEGIN_MESSAGE_MAP(CSetPKCS12FilePasswordDialog, CDialog)
END_MESSAGE_MAP()

// Boîte de dialogue CViewCertGeneralPage

//IMPLEMENT_DYNAMIC(CViewCertGeneralPage, CPropertyPage)
CViewCertGeneralPage::CViewCertGeneralPage()
	: CPropertyPage(CViewCertGeneralPage::IDD)
{
}

CViewCertGeneralPage::~CViewCertGeneralPage()
{
}

void CViewCertGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewServerDialog)
	DDX_Text(pDX, IDC_VERIFIED, m_csVerified);
	DDX_Text(pDX, IDC_USAGE, m_csUsage);
	DDX_Text(pDX, IDC_EDIT_OU, m_csOU);
	DDX_Text(pDX, IDC_EDIT_OU2, m_csOU2);
	DDX_Text(pDX, IDC_EDIT_O, m_csO);
	DDX_Text(pDX, IDC_EDIT_O2, m_csO2);
	DDX_Text(pDX, IDC_EDIT_CN, m_csCN);
	DDX_Text(pDX, IDC_EDIT_CN2, m_csCN2);
	DDX_Text(pDX, IDC_EDIT_SN, m_csSN);
	DDX_Text(pDX, IDC_ISSUED_DATE, m_csIssuedDate);
	DDX_Text(pDX, IDC_EXPIRES_DATE, m_csExpiresDate);
	DDX_Text(pDX, IDC_SHA1, m_csSHA1);
	DDX_Text(pDX, IDC_MD5, m_csMD5);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewCertGeneralPage, CPropertyPage)
END_MESSAGE_MAP()

// Boîte de dialogue CViewCertDetailsPage

//IMPLEMENT_DYNAMIC(CViewCertDetailsPage, CPropertyPage)
CViewCertDetailsPage::CViewCertDetailsPage(nsIArray* certChain, CWnd* pParent /*=NULL*/)
	: CPropertyPage(CViewCertDetailsPage::IDD)
{
	m_certChain = certChain;
}

CViewCertDetailsPage::~CViewCertDetailsPage()
{
}

void CViewCertDetailsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CViewCertDetailsPage, CPropertyPage)
	ON_NOTIFY(TVN_SELCHANGED, IDC_CERT_HIERARCHY, OnTvnSelchangedCertHierarchy)
	ON_NOTIFY(TVN_SELCHANGED, IDC_CERT_FIELDS, OnTvnSelchangedCertFields)
END_MESSAGE_MAP()


// Gestionnaires de messages CViewCertDetailsPage

BOOL CViewCertDetailsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	PRUint32 numCerts;
	m_certChain->GetLength(&numCerts);
	CTreeCtrl* treeH = (CTreeCtrl*)GetDlgItem(IDC_CERT_HIERARCHY);
	HTREEITEM current =  TVI_ROOT;
	
	
	// I'm not sure if what I'm doing here is right
	USES_CONVERSION;
	for (PRInt32 i=numCerts-1;i>=0;i--)
	{
		nsCOMPtr<nsIX509Cert> cert;
		m_certChain->QueryElementAt(i, kX509CertCID, getter_AddRefs(cert));

		nsEmbedString displayVal;
		cert->GetCommonName(displayVal);

		if (displayVal.IsEmpty())
		{
			char *title;
			cert->GetWindowTitle(&title);
			displayVal = A2W(title);
			nsMemory::Free(title);
		}
		
		nsIX509Cert *pCert = cert;
		current = treeH->InsertItem(TVIF_TEXT|TVIF_PARAM, W2CT(displayVal.get()), 
			0, 0, 0, 0, (LPARAM)(void*)pCert, current, TVI_LAST );
	}

	treeH->SelectItem(current);
	treeH->Expand(treeH->GetRootItem(), TVE_EXPAND);

	return TRUE;  // return TRUE unless you set the focus to a control
}

	void CViewCertDetailsPage::loadASN1Structure(CTreeCtrl* tree, nsIASN1Object* asn1Object, HTREEITEM parent)
	{
		if (!asn1Object) return;

		nsEmbedString displayVal;
		asn1Object->GetDisplayName(displayVal);
		
		USES_CONVERSION;
		HTREEITEM current = tree->InsertItem(TVIF_TEXT|TVIF_PARAM, W2CT(displayVal.get()), 
			0, 0, 0, 0, (LPARAM)(void*)asn1Object, parent, TVI_LAST );

		nsCOMPtr<nsIASN1Sequence> sequence(do_QueryInterface(asn1Object));
		if (!sequence) return;

		nsCOMPtr<nsIMutableArray> asn1Objects;
		sequence->GetASN1Objects(getter_AddRefs(asn1Objects));

		PRUint32 count;
		asn1Objects->GetLength(&count);

		for (PRUint32 i=0;i<count;i++)
		{
			nsCOMPtr<nsIASN1Object> currObject;
			asn1Objects->QueryElementAt(i, kASN1ObjectCID, getter_AddRefs (currObject));
			loadASN1Structure(tree, currObject, current);
		}
		tree->Expand(current, TVE_EXPAND);
	}

	void CViewCertDetailsPage::OnTvnSelchangedCertHierarchy(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
		
		CTreeCtrl* treeH = (CTreeCtrl*)GetDlgItem(IDC_CERT_HIERARCHY);
		if (HTREEITEM selected = treeH->GetSelectedItem())
		{
			nsIX509Cert *cert;
			cert = (nsIX509Cert*)treeH->GetItemData(selected);
	
			nsCOMPtr<nsIASN1Object> asn1Object;
			cert->GetASN1Structure(getter_AddRefs(asn1Object));
				
			CTreeCtrl* treeC = (CTreeCtrl*)GetDlgItem(IDC_CERT_FIELDS);
			treeC->DeleteAllItems();
			loadASN1Structure(treeC, asn1Object, TVI_ROOT);
		}
		
		*pResult = 0;
	}

	void CViewCertDetailsPage::OnTvnSelchangedCertFields(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

		CTreeCtrl* treeC = (CTreeCtrl*)GetDlgItem(IDC_CERT_FIELDS);
		if (HTREEITEM selected = treeC->GetSelectedItem())
		{
			USES_CONVERSION;
			nsIASN1Object* asn1Object;
			asn1Object = (nsIASN1Object*)treeC->GetItemData(selected);
			nsEmbedString displayVal;
			asn1Object->GetDisplayValue(displayVal);

			// Have replace \n by \r\n for proper line break in the edit box
			CString csVal(W2CT(displayVal.get()));
			csVal.Replace(_T("\n"),_T("\r\n"));
			
			SetDlgItemText(IDC_FIELD_VALUE, csVal);

		}
		
		*pResult = 0;
	}
