/*
* 
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
#include "CookiePromptService.h"
#include "nsICookieAcceptDialog.h"
#include "nsICookie.h"
#include "Cookies.h"
#include "MozUtils.h"


extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

NS_IMPL_ISUPPORTS1(CCookiePromptService, nsICookiePromptService)

CCookiePromptService::CCookiePromptService(){
}

CCookiePromptService::~CCookiePromptService() {
}

NS_IMETHODIMP CCookiePromptService::CookieDialog(nsIDOMWindow *parent, nsICookie *aCookie, const nsACString & hostname, PRInt32 cookiesFromHost, PRBool changingCookie, PRBool *rememberDecision, PRInt32 *_retval)
{
	NS_ENSURE_ARG_POINTER(aCookie);

	CString q;
	static BOOL accept = TRUE;
	CString host = NSUTF8StringToCString(nsEmbedCString(hostname));

	if (changingCookie)
		q.Format(IDS_COOKIE_MODIFY, host);
	else if (cookiesFromHost == 0)
		q.Format(IDS_COOKIE_SET, host);
	else if (cookiesFromHost == 1)
		q.Format(IDS_COOKIE_SET2, host);
	else
		q.Format(IDS_COOKIE_ANOTHER, host, cookiesFromHost);

	HWND activeWnd = GetActiveWindow();
	CWnd *wnd = CWndForDOMWindow(parent);
	CConfirmCookieDialog dlg(wnd, q, accept);

	CCookie* cookie = new CCookie(aCookie);
	dlg.m_csName = cookie->m_csName;
	dlg.m_csValue = cookie->m_csValue;
	dlg.m_csHost = cookie->m_csHost;
	dlg.m_csPath = cookie->m_csPath;
	dlg.m_csExpires = cookie->m_csExpire;

	if (cookie->m_secure)
		dlg.m_csSendFor.LoadString(IDS_FOR_SECURE);
	else
		dlg.m_csSendFor.LoadString(IDS_FOR_ANY);

	*_retval = dlg.DoModal();
	accept = *rememberDecision = dlg.m_bCheckBoxValue;
	delete cookie;	  

	SetForegroundWindow(activeWnd);
	return NS_OK;
}

// Boîte de dialogue CConfimCookieDialog

//IMPLEMENT_DYNAMIC(CConfirmCookieDialog, CDialog)
CConfirmCookieDialog::CConfirmCookieDialog(CWnd* pParent, const TCHAR* pText, BOOL bAcceptSite)
: CDialog(CConfirmCookieDialog::IDD, pParent)
{
	if(pText)
		m_csText = pText;
	m_bCheckBoxValue = bAcceptSite;
}

CConfirmCookieDialog::~CConfirmCookieDialog()
{
}

void CConfirmCookieDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAlertCheckDialog)
	DDX_Check(pDX, IDC_CHECK_REMEMBER, m_bCheckBoxValue);
	DDX_Text(pDX, IDC_COOKIE_TEXT, m_csText);
	DDX_Text(pDX, IDC_COOKIE_NAME, m_csName);
	DDX_Text(pDX, IDC_COOKIE_HOST, m_csHost);
	DDX_Text(pDX, IDC_COOKIE_PATH, m_csPath);
	DDX_Text(pDX, IDC_COOKIE_SENDFOR, m_csSendFor);
	DDX_Text(pDX, IDC_COOKIE_EXPIRES, m_csExpires);
	DDX_Text(pDX, IDC_COOKIE_VALUE, m_csValue);
	DDX_Control(pDX, IDC_COOKIE_ICON, m_bIcon);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfirmCookieDialog, CDialog)
	ON_BN_CLICKED(IDALLOWSESSION, OnBnClickedAllowsession)
	ON_BN_CLICKED(IDDENY, OnBnClickedDeny)
	ON_BN_CLICKED(IDALLOW, OnBnClickedAllow)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CConfirmCookieDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	HICON icon = (HICON)LoadImage(0,MAKEINTRESOURCE(32514),IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_SHARED);
	m_bIcon.SetIcon(icon);
	return TRUE;
}

void CConfirmCookieDialog::OnBnClickedAllowsession()
{
	UpdateData();
	EndDialog(nsICookiePromptService::ACCEPT_SESSION_COOKIE);
}

void CConfirmCookieDialog::OnBnClickedDeny()
{
	UpdateData();
	EndDialog(nsICookiePromptService::DENY_COOKIE);
}

void CConfirmCookieDialog::OnBnClickedAllow()
{
	UpdateData();
	EndDialog(nsICookiePromptService::ACCEPT_COOKIE);
}

void CConfirmCookieDialog::OnClose()
{
	m_bCheckBoxValue = false;
	EndDialog(nsICookiePromptService::DENY_COOKIE);
}
