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
#include "Fontpackagehandler.h"
#include "nsIFontPackageService.h"
#include "BrowserFrm.h"
#include "MfcEmbed.h"

NS_IMPL_ISUPPORTS1(CFontPackageHandler, nsIFontPackageHandler);

CFontPackageHandler::CFontPackageHandler(void)
{
}

CFontPackageHandler::~CFontPackageHandler(void)
{
}


NS_IMETHODIMP CFontPackageHandler::NeedFontPackage(const char *aFontPackID)
{
  // no FontPackage is passed, return
  NS_ENSURE_ARG_POINTER(aFontPackID);

  if (!strlen(aFontPackID))
    return NS_ERROR_UNEXPECTED;

  CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
  if (!pApp->preferences.GetBool("font.askWhenNeeded", true))
	  return NS_OK;

  nsresult rv;

  // Should get that from chrome
  CString cshandledLanguages;
  cshandledLanguages.LoadString(IDS_HANDLED_LANGUAGES);
  USES_CONVERSION;
  const char* handledLanguages = T2CA(cshandledLanguages);
  
  // aFontPackID is of the form lang:xx or lang:xx-YY
  char *langCode = strchr(aFontPackID,':');
  if (!langCode || !*(langCode + 1))
    return NS_ERROR_UNEXPECTED;
  
  langCode = strdup(langCode + 1);
  strlwr(langCode);
  
  // check for xx or xx-yy in handled_languages
  // if not handled, return now, don't show the font dialog
  if (!strstr(handledLanguages, langCode))
    return NS_OK; // XXX should be error?

  HWND hWndTop;
  HWND hWnd = CWnd::GetSafeOwner_(NULL, &hWndTop);
  if (hWnd != hWndTop)
		EnableWindow(hWnd, TRUE);

  CWnd* parent = CWnd::FromHandle(hWnd);

  // check windows version
 OSVERSIONINFO vi;
 vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 GetVersionEx(&vi);
 if (vi.dwMajorVersion>4)
 {
	 CFontNeededDialog* dlg = new CFontNeededDialog(langCode, parent);
	 rv = dlg->DoModal();
 }
 else
 {
	 CDownloadFontDialog* dlg = new CDownloadFontDialog(langCode, parent);
	 rv = dlg->DoModal();
 }	
 free(langCode);

  nsCOMPtr<nsIFontPackageService> fontService(do_GetService(NS_FONTPACKAGESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(fontService, NS_ERROR_FAILURE);

  fontService->FontPackageHandled(NS_SUCCEEDED(rv), PR_FALSE, aFontPackID);
    return rv;
  }


// Boîte de dialogue CDownloadFontDialog

//IMPLEMENT_DYNAMIC(CDownloadFontDialog, CDialog)
CDownloadFontDialog::CDownloadFontDialog(const char* langcode, CWnd* pParent /*=NULL*/)
	: CDialog(CDownloadFontDialog::IDD, pParent)
{
	USES_CONVERSION;
	m_csFontName.LoadString(IDS_NAME_UNKNOW);
	CString cshandledLanguages;
	cshandledLanguages.LoadString(IDS_HANDLED_LANGUAGES);
	char* handledLanguages = T2A(cshandledLanguages.GetBuffer(0));

	int i=0;
	char *token = strtok(handledLanguages, ", " );
	while( token != NULL )
    {
	   if (strcmp(token, langcode)==0)
	   {
		   m_csFontName.LoadString(IDS_NAME_FIRST + i);
		   m_csFontSize.LoadString(IDS_SIZE_FIRST + i);
		   break;
	   }
	   i++;
	   token = strtok(NULL, ", ");
    }

	m_LangCode = langcode;
}

CDownloadFontDialog::~CDownloadFontDialog()
{
}

void CDownloadFontDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FONTNAME, m_csFontName);
	DDX_Text(pDX, IDC_FONTSIZE, m_csFontSize);
}


BEGIN_MESSAGE_MAP(CDownloadFontDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// Gestionnaires de messages CDownloadFontDialog

// Boîte de dialogue CFontNeededDialog

//IMPLEMENT_DYNAMIC(CFontNeededDialog, CDialog)
CFontNeededDialog::CFontNeededDialog(const char* langcode, CWnd* pParent /*=NULL*/)
	: CDialog(CFontNeededDialog::IDD, pParent)
{
	USES_CONVERSION;
	m_csFontName.LoadString(IDS_NAME_UNKNOW);
	CString cshandledLanguages;
	cshandledLanguages.LoadString(IDS_HANDLED_LANGUAGES);
	char* handledLanguages = T2A(cshandledLanguages.GetBuffer(0));

	int i=0;
	char *token = strtok(handledLanguages, ", " );
	while( token != NULL )
    {
	   if (strcmp(token, langcode)==0)
	   {
		   m_csFontName.LoadString(IDS_NAME_FIRST + i);
		   break;
	   }
	   i++;
	   token = strtok(NULL, ", ");
    }
	
	m_LangCode = langcode;
}

CFontNeededDialog::~CFontNeededDialog()
{
}

void CFontNeededDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FONTNAME, m_csFontName);
}


BEGIN_MESSAGE_MAP(CFontNeededDialog, CDialog)
END_MESSAGE_MAP()


// Gestionnaires de messages CFontNeededDialog


void CDownloadFontDialog::OnBnClickedOk()
{
	CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
    if(!pApp)
		return;

    CBrowserFrame* pFrm = pApp->CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, PR_FALSE);
    if(!pFrm)
		return;

    pFrm->m_wndBrowserView.OpenURL(
		_T("http://www.mozilla.org/projects/intl/fonts/win/redirect/package_") + m_LangCode + _T(".html"), nsnull);

	pFrm->ShowWindow(SW_SHOW);
    pFrm->SetFocus();
	OnOK();
}
