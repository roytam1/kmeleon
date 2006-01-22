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
#pragma once

#include "nsICookiePromptService.h"
#include "resource.h"

#ifndef XP_WIN
#define XP_WIN
#endif

#define NS_COOKIEPROMPTSERVICE_CID \
 {0xCE002B28, 0x92B7, 0x4701, {0x86, 0x21, 0xCC, 0x92, 0x58, 0x66, 0xFB, 0x87}}
static NS_DEFINE_CID(kPromptCookieServiceCID, NS_COOKIEPROMPTSERVICE_CID);


class CCookiePromptService: public nsICookiePromptService

{
public:
                 CCookiePromptService();
  virtual       ~CCookiePromptService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIEPROMPTSERVICE

 };


// Boîte de dialogue CConfirmCookieDialog

class CConfirmCookieDialog : public CDialog
{
	DECLARE_DYNAMIC(CConfirmCookieDialog)

public:
	CConfirmCookieDialog::CConfirmCookieDialog(CWnd* pParent, const TCHAR* pText, bool bAcceptSite);
	virtual ~CConfirmCookieDialog();
	int m_bCheckBoxValue;
	CString m_csMsg,
		    m_csName,
			m_csValue,
			m_csHost,
			m_csPath,
			m_csExpires,
			m_csSendFor;
	CStatic m_bIcon;
	
// Données de boîte de dialogue
	enum { IDD = IDD_CONFIRM_COOKIE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	CString m_csText;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAllowsession();
	afx_msg void OnBnClickedDeny();
	afx_msg void OnBnClickedAllow();
	afx_msg void OnClose();
};
