/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* PromptService is intended to override the default Mozilla PromptService,
   giving nsIPrompt implementations of our own design, rather than using
   Mozilla's. Do this by building this into a component and registering the
   factory with the same CID/ContractID as Mozilla's (see MfcEmbed.cpp).
*/

#include "stdafx.h"
#include "resource.h"
#include "GenericDlg.h"
#include "Dialogs.h"
#include "mfcembed.h"
#include "PromptService.h"
#include "nsIPromptService.h"
#include "nsIWindowWatcher.h"
#include "nsIAuthInformation.h"

#include "nsILoginManager.h"
#include "MozUtils.h"
#include "nsIIOService.h"
#include "nsIProtocolHandler.h"
#include "nsILoginInfo.h"
#include "nsIStringBundle.h"

extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

//*****************************************************************************
// CPromptService
//*****************************************************************************



//*****************************************************************************
NS_IMPL_ISUPPORTS(CPromptService, nsIPromptFactory, nsIPrompt, nsIAuthPrompt, nsIPromptService)
//NS_IMPL_ISUPPORTS1(CPromptService, nsIPromptService/*, nsINonBlockingAlertService*/)

CPromptService::CPromptService()
{
}

CPromptService::~CPromptService() {
}

NS_IMETHODIMP CPromptService::GetPrompt(nsIDOMWindow *aParent, const nsIID & iid, void **result)
{
	mDomWindow = aParent;
	return QueryInterface(iid, result);
}

/* boolean prompt (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in uint32_t savePassword, in wstring defaultText, out wstring result); */
NS_IMETHODIMP CPromptService::Prompt(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, const PRUnichar * defaultText, PRUnichar * *result, bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean promptUsernameAndPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in uint32_t savePassword, inout wstring user, inout wstring pwd); */
NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, PRUnichar * *user, PRUnichar * *pwd, bool *_retval)
{
	nsString realm, url;
	url.Assign(passwordRealm);
	const wchar_t *start = wcsrchr(passwordRealm, '('), *stop;
	if (start && (stop = wcsrchr(start, ')')) && stop>start) {
			realm.Append(start+1, stop-start-1);
			url.Assign(passwordRealm, start-passwordRealm);
	}
	realm.get();
	url.get();

	nsCOMPtr<nsIURI> uri;
	NewURI(getter_AddRefs(uri), url);
	nsCString scheme, host, hostname;
	int32_t port = -1, dport;
	if (uri) {
		uri->GetHost(host);
		uri->GetScheme(hostname);
		uri->GetPort(&port);
	}
	hostname.Append(scheme);
	hostname.Append("://");
	hostname.Append(host);
	if (port != -1) {
		 nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
		 if (ios) {
			 nsCOMPtr<nsIProtocolHandler> handler;
			 ios->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
			 handler->GetDefaultPort(&dport);
			 if (dport != port) {
				 hostname.Append(":");
				 char p[20];
				_itoa_s(port, p, 10);
				 hostname.Append(p);
			 }
		 }
	}

	nsString checkMsg;
	nsString hostname2 = NS_ConvertUTF8toUTF16(hostname);	
	bool canSave = false;
	nsILoginInfo* selectedLogin = nullptr;
	nsCOMPtr<nsILoginManager> loginManager = do_GetService("@mozilla.org/login-manager;1");
	if (loginManager) {
		loginManager->GetLoginSavingEnabled(hostname2, &canSave);
		canSave &= (savePassword == nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY);
		if (canSave) {
			uint32_t count;
			nsILoginInfo** logins;
			loginManager->FindLogins(&count, hostname2, nsString(), realm, &logins);
			if (count>0) {
				selectedLogin = logins[0];
				nsString nuser,npass;
				selectedLogin->GetPassword(npass);
				selectedLogin->GetUsername(nuser);
				*pwd = NS_StringCloneData(npass);
				*user = NS_StringCloneData(nuser);
			}
			nsCOMPtr<nsIStringBundleService> bundleService = do_GetService("@mozilla.org/intl/stringbundle;1");
			if (bundleService) {
				nsCOMPtr<nsIStringBundle> bundle;
				bundleService->CreateBundle("chrome://passwordmgr/locale/passwordmgr.properties", getter_AddRefs(bundle));
				if (bundle) {
					bundle->GetStringFromName(L"rememberPassword", getter_Copies(checkMsg));
				}
			}			
		}
	}

	bool checkRet = true;
	PromptUsernameAndPassword(mDomWindow, dialogTitle, text, user, pwd, checkMsg.get(), &checkRet, _retval);

	if (*_retval && checkRet && canSave && *pwd) {
		nsString username, password;
		nsCOMPtr<nsILoginInfo> newLogin;
		if (selectedLogin) {
			selectedLogin->GetUsername(username);
			selectedLogin->GetPassword(password);			
			if (wcscmp(username.get(), *user) == 0 && wcscmp(password.get(), *pwd) == 0) {
			} else {
				selectedLogin->Clone(getter_AddRefs(newLogin));
				loginManager->RemoveLogin(selectedLogin);
			}
		} else {
			newLogin = do_CreateInstance("@mozilla.org/login-manager/loginInfo;1");	
			newLogin->SetHostname(hostname2);
			newLogin->SetHttpRealm(realm);
			newLogin->SetPasswordField(NS_LITERAL_STRING(""));
			newLogin->SetUsernameField(NS_LITERAL_STRING(""));
		}

		if (newLogin) {
			newLogin->SetPassword(nsDependentString(*pwd));
			newLogin->SetUsername(nsDependentString(*user));		
			loginManager->AddLogin(newLogin);
		}
	}

    return NS_OK;
}

/* boolean promptPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in uint32_t savePassword, inout wstring pwd); */
NS_IMETHODIMP CPromptService::PromptPassword(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, PRUnichar * *pwd, bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void alert (in wstring dialogTitle, in wstring text); */
NS_IMETHODIMP CPromptService::Alert(const PRUnichar * dialogTitle, const PRUnichar * text)
{
	return Alert(mDomWindow, dialogTitle, text);
}

/* void alertCheck (in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::AlertCheck(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * checkMsg, bool *checkValue)
{
	return AlertCheck(mDomWindow, dialogTitle, text, checkMsg, checkValue);
}

/* boolean confirm (in wstring dialogTitle, in wstring text); */
NS_IMETHODIMP CPromptService::Confirm(const PRUnichar * dialogTitle, const PRUnichar * text, bool *_retval)
{
    return Confirm(mDomWindow, dialogTitle, text, _retval);
}

/* boolean confirmCheck (in wstring dialogTitle, in wstring text, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::ConfirmCheck(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * checkMsg, bool *checkValue, bool *_retval)
{
	return ConfirmCheck(mDomWindow, dialogTitle, text, checkMsg, checkValue, _retval);
}

/* int32_t confirmEx (in wstring dialogTitle, in wstring text, in unsigned long buttonFlags, in wstring button0Title, in wstring button1Title, in wstring button2Title, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::ConfirmEx(const PRUnichar * dialogTitle, const PRUnichar * text, uint32_t buttonFlags, const PRUnichar * button0Title, const PRUnichar * button1Title, const PRUnichar * button2Title, const PRUnichar * checkMsg, bool *checkValue, int32_t *_retval)
{
    return ConfirmEx(mDomWindow, dialogTitle, text, buttonFlags, button0Title, button1Title, button2Title, checkMsg, checkValue, _retval);
}

/* boolean prompt (in wstring dialogTitle, in wstring text, inout wstring value, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::Prompt(const PRUnichar * dialogTitle, const PRUnichar * text, PRUnichar * *value, const PRUnichar * checkMsg, bool *checkValue, bool *_retval)
{
	return Prompt(mDomWindow, dialogTitle, text, value, checkMsg, checkValue, _retval);
}

/* boolean promptPassword (in wstring dialogTitle, in wstring text, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::PromptPassword(const PRUnichar * dialogTitle, const PRUnichar * text, PRUnichar * *password, const PRUnichar * checkMsg, bool *checkValue, bool *_retval)
{
    return PromptPassword(mDomWindow, dialogTitle, text, password, checkMsg, checkValue, _retval);
}

/* boolean promptUsernameAndPassword (in wstring dialogTitle, in wstring text, inout wstring username, inout wstring password, in wstring checkMsg, inout boolean checkValue); */
NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(const PRUnichar * dialogTitle, const PRUnichar * text, PRUnichar * *username, PRUnichar * *password, const PRUnichar * checkMsg, bool *checkValue, bool *_retval)
{
	return PromptUsernameAndPassword(mDomWindow, dialogTitle, text, username, password, checkMsg, checkValue, _retval);
}

/* boolean select (in wstring dialogTitle, in wstring text, in uint32_t count, [array, size_is (count)] in wstring selectList, out long outSelection); */
NS_IMETHODIMP CPromptService::Select(const PRUnichar * dialogTitle, const PRUnichar * text, uint32_t count, const PRUnichar * *selectList, int32_t *outSelection, bool *_retval)
{
	return Select(mDomWindow, dialogTitle, text, count, selectList, outSelection, _retval);
}


NS_IMETHODIMP CPromptService::Alert(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                    const PRUnichar *text)
{
  USES_CONVERSION;
  CWnd *wnd = CWndForDOMWindow(parent);
  if (wnd)
    wnd->MessageBox(W2CT(text), W2CT(dialogTitle), MB_OK | MB_ICONEXCLAMATION);
  else
    ::MessageBox(0, W2CT(text), W2CT(dialogTitle), MB_OK | MB_ICONEXCLAMATION);

  return NS_OK;
}

NS_IMETHODIMP CPromptService::AlertCheck(nsIDOMWindow *parent,
                                         const PRUnichar *dialogTitle,
                                         const PRUnichar *text,
                                         const PRUnichar *checkboxMsg,
                                         bool *checkValue)
{
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
  dlg.AddButton(100, IDS_OK);
  dlg.SetDefaultButton(100);
  dlg.SetCancelButton(100);
  dlg.SetMsgIcon(AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION));
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == true ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  dlg.DoModal();

  if (checkValue)
	*checkValue = (checkResult == TRUE ? true : false);

  return NS_OK;
}

NS_IMETHODIMP CPromptService::Confirm(nsIDOMWindow *parent,
                                      const PRUnichar *dialogTitle,
                                      const PRUnichar *text,
                                      bool *_retval)
{
  USES_CONVERSION;
  CWnd *wnd = CWndForDOMWindow(parent);
  int choice;

  if (wnd)
    choice = wnd->MessageBox(W2CT(text), W2CT(dialogTitle),
                      MB_OKCANCEL | MB_ICONEXCLAMATION);
  else
    choice = ::MessageBox(0, W2CT(text), W2CT(dialogTitle),
                      MB_OKCANCEL | MB_ICONEXCLAMATION);

  *_retval = choice == IDOK ? PR_TRUE : PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP CPromptService::ConfirmCheck(nsIDOMWindow *parent,
                                           const PRUnichar *dialogTitle,
                                           const PRUnichar *text,
                                           const PRUnichar *checkboxMsg,
                                           bool *checkValue,
                                           bool *_retval)
{
	const int COMMAND_OFFSET = 100;

	USES_CONVERSION;
    CWnd *wnd = CWndForDOMWindow(parent);
    CGenericDlg dlg(wnd);
    dlg.SetTitle(W2CT(dialogTitle));
    dlg.SetMsg(W2CT(text));
	 dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
    BOOL checkResult;
    if (checkboxMsg && checkValue) {
       checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
	   dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
    }

	dlg.AddButton(COMMAND_OFFSET, IDS_YES);
	dlg.AddButton(COMMAND_OFFSET+1, IDS_NO);
	dlg.SetDefaultButton(COMMAND_OFFSET);
	dlg.SetCancelButton(COMMAND_OFFSET+1);
	
	int iBtnClicked = dlg.DoModal();

	if (checkValue)
		*checkValue = (checkResult == TRUE ? PR_TRUE : PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP CPromptService::Prompt(nsIDOMWindow *parent,
                                     const PRUnichar *dialogTitle,
                                     const PRUnichar *text,
                                     PRUnichar **value,
                                     const PRUnichar *checkboxMsg,
                                     bool *checkValue,
                                     bool *_retval)
{
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);

  CString csValue;
  if (value && *value) csValue = W2CT(*value);
  dlg.AddEdit(&csValue, _T(""), FALSE);
  
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == true ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
 	if (value) {
      if (*value) nsMemory::Free(*value);
      nsString nsPromptEditValue;
      nsPromptEditValue.Assign(T2CW(csValue));
      *value = NS_StringCloneData(nsPromptEditValue);
	  if (checkboxMsg && checkValue) *checkValue = checkResult;
    }
    *_retval = PR_TRUE;
  }
  else
    *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(nsIDOMWindow *parent,
                                                        const PRUnichar *dialogTitle,
                                                        const PRUnichar *text,
                                                        PRUnichar **username,
                                                        PRUnichar **password,
                                                        const PRUnichar *checkboxMsg,
                                                        bool *checkValue,
                                                        bool *_retval)
{
  NS_ENSURE_ARG_POINTER(username);
  NS_ENSURE_ARG_POINTER(password);

  USES_CONVERSION;
  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);

  
    
  CString csUsername, csPassword;
  if (username && *username) csUsername = W2CT(*username);
  if (password && *password) csPassword = W2CT(*password);
  dlg.AddEdit(&csUsername, IDS_USERNAME, FALSE);
  dlg.AddEdit(&csPassword, IDS_PASSWORD, TRUE);
  
  BOOL checkResult = FALSE;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == true ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
	  if(*username) nsMemory::Free(*username);
	  *username = NS_StringCloneData(nsString(T2CW(csUsername)));

	  if (*password) nsMemory::Free(*password);
	  *password = NS_StringCloneData(nsString(T2CW(csPassword)));

     if (checkValue)
        *checkValue = (checkResult == TRUE ? true : false);

	  *_retval = true;
  }
  else
    *_retval = false;
 
  return NS_OK;
}

NS_IMETHODIMP CPromptService::PromptPassword(nsIDOMWindow *parent,
                                             const PRUnichar *dialogTitle,
                                             const PRUnichar *text,
                                             PRUnichar **password,
                                             const PRUnichar *checkboxMsg,
                                             bool *checkValue,
                                             bool *_retval)
{
  NS_ENSURE_ARG_POINTER(password);
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);

  CString csValue;
  if (*password) csValue = W2CT(*password);
  dlg.AddEdit(&csValue, _T(""), TRUE);
  
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
     if (*password) nsMemory::Free(*password);
	  *password = NS_StringCloneData(nsString(T2CW(csValue)));
     *_retval = PR_TRUE;
  }
  else
    *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP CPromptService::Select(nsIDOMWindow *parent,
                                     const PRUnichar *dialogTitle,
                                     const PRUnichar *text, PRUint32 count,
                                     const PRUnichar **selectList,
                                     PRInt32 *outSelection,
                                     bool *_retval)
{
	USES_CONVERSION;
	CWnd *wnd = CWndForDOMWindow(parent);
   CSelectDialog dlg(wnd, W2CT(dialogTitle), W2CT(text));

	for (PRUint32 i = 0; i<count; i++)
		dlg.AddChoice(W2CT(selectList[i]));

	*_retval = dlg.DoModal() == IDOK ? PR_TRUE : PR_FALSE;
	*outSelection = dlg.GetChoice();
  
	return NS_OK;
}

NS_IMETHODIMP CPromptService::ConfirmEx(nsIDOMWindow *parent,
                                        const PRUnichar *dialogTitle,
                                        const PRUnichar *text,
                                        PRUint32 buttonFlags,
                                        const PRUnichar *button0Title,
                                        const PRUnichar *button1Title,
                                        const PRUnichar *button2Title,
                                        const PRUnichar *checkMsg,
                                        bool *checkValue,
                                        PRInt32 *buttonPressed)
{
	const int COMMAND_OFFSET = 100;
    USES_CONVERSION;

	CWnd *wnd = CWndForDOMWindow(parent);
	CGenericDlg dlg(wnd);
	dlg.SetTitle(W2CT(dialogTitle));
	dlg.SetMsg(W2CT(text));
	dlg.SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));

	//https://bugzilla.mozilla.org/show_bug.cgi?id=329414
	//Set the cancel button to 1, and the default one.
	dlg.SetDefaultButton(COMMAND_OFFSET + ((buttonFlags & 0x03000000) >> 24));
	dlg.SetCancelButton(COMMAND_OFFSET + 1);

	// Determine the button titles based on buttonFlags
    const PRUnichar* buttonStrings[] = { button0Title, button1Title, button2Title };

    for(int i=0; i<3; i++)
    {
        switch(buttonFlags & 0xff) {
			case nsIPromptService::BUTTON_TITLE_OK:
				dlg.AddButton(COMMAND_OFFSET+i, IDS_OK);
                break;
            case nsIPromptService::BUTTON_TITLE_CANCEL:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_CANCEL);
                break;
            case nsIPromptService::BUTTON_TITLE_YES:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_YES);
                break;
            case nsIPromptService::BUTTON_TITLE_NO:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_NO);
                break;
            case nsIPromptService::BUTTON_TITLE_SAVE:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_SAVE);
                break;
            case nsIPromptService::BUTTON_TITLE_DONT_SAVE:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_DONTSAVE);
                break;
            case nsIPromptService::BUTTON_TITLE_REVERT:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_REVERT);
                break;
            case nsIPromptService::BUTTON_TITLE_IS_STRING:
                dlg.AddButton(COMMAND_OFFSET+i, W2CT(buttonStrings[i]));
                break;
        }
   
        buttonFlags >>= 8;    
    }

	BOOL checkResult = false;
	if (checkMsg && checkValue) {
		checkResult = (*checkValue == true ? TRUE : FALSE);
		dlg.AddCheckBox(&checkResult, W2CT(checkMsg));
	}
	
	*buttonPressed = dlg.DoModal() - COMMAND_OFFSET;
	if (checkValue)
		*checkValue = (checkResult == TRUE ? true : false);

    return NS_OK;    
}

#include "nsIChannel.h"
NS_IMETHODIMP CPromptService::PromptAuth(nsIChannel *aChannel, uint32_t level, nsIAuthInformation *authInfo, bool *_retval)
{
	/*nsString username, password;
	authInfo->GetUsername(username);
	authInfo->GetPassword(password);

	nsCOMPtr<nsIURI> uri;
	aChannel->GetURI(getter_AddRefs(uri));
	
	nsCString scheme, host;
	nsString realm;
	uri->GetScheme(scheme);
	uri->GetHostPort(host);
	authInfo->GetRealm(realm);
	scheme.Append("://");
	scheme.Append(host);
	host = scheme;		
	
	nsString target;
	NS_CStringToUTF16(host, NS_CSTRING_ENCODING_UTF8,target);
	target.Append(L"(");
	target.Append(realm);
	target.Append(L")");

	uint32_t flags;
	authInfo->GetFlags(&flags);

	nsString text;

	nsresult rv;
	bool _retval;
	if (flags & nsIAuthInformation::ONLY_PASSWORD)
		rv = PromptPassword(mDomWindow, nullptr, text.get(), password, nullptr, nullptr, &_retval);
	else 
		rv = PromptUsernameAndPassword(mDomWindow, nullptr, text.get(), username, password, nullptr, nullptr, _retval);
    authInfo->SetUsername(username);
	authInfo->SetPassword(password);
	return rv;*/
	return NS_ERROR_NOT_IMPLEMENTED;
}
/*
NS_IMETHODIMP CPromptService::Prompt(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, const PRUnichar * defaultText, PRUnichar * *result, bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

// boolean promptUsernameAndPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in uint32_t savePassword, inout wstring user, inout wstring pwd); 
NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, PRUnichar * *user, PRUnichar * *pwd, bool *_retval)
{
    return PromptUsernameAndPassword(mDomWindow, dialogTitle, text, user, pwd, nullptr, nullptr, _retval);
}

// boolean promptPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in uint32_t savePassword, inout wstring pwd); 
NS_IMETHODIMP CPromptService::PromptPassword(const PRUnichar * dialogTitle, const PRUnichar * text, const PRUnichar * passwordRealm, uint32_t savePassword, PRUnichar * *pwd, bool *_retval)
{
    return PromptPassword(mDomWindow, dialogTitle, text, pwd, nullptr, nullptr, _retval);
}*/

/* nsICancelable asyncPromptAuth (in nsIChannel aChannel, in nsIAuthPromptCallback aCallback, in nsISupports aContext, in uint32_t level, in nsIAuthInformation authInfo); */
NS_IMETHODIMP CPromptService::AsyncPromptAuth(nsIChannel *aChannel, nsIAuthPromptCallback *aCallback, nsISupports *aContext, uint32_t level, nsIAuthInformation *authInfo, nsICancelable * *_retval)
{
	nsString s;
	authInfo->GetRealm(s);
    return NS_ERROR_NOT_IMPLEMENTED;
}
 
/*NS_IMETHODIMP
CPromptService::ShowNonBlockingAlert(nsIDOMWindow *aParent,
                                      const PRUnichar *aDialogTitle,
                                      const PRUnichar *aText)
{
  BOOL result;
  CGenericDlg* dlg = new CGenericDlg();  
  CWnd *wnd = CWndForDOMWindow(aParent);
  
  USES_CONVERSION;
  dlg->SetTitle(W2CT(aDialogTitle));
  dlg->SetMsg(W2CT(aText));
  dlg->SetDlgIcon(((CMfcEmbedApp*)AfxGetApp())->GetDefaultIcon(TRUE));
  dlg->SetMsgIcon(AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION));
  dlg->AddButton(100, IDS_OK);
  dlg->SetDefaultButton(100);
  dlg->SetCancelButton(100);

  result = dlg->DoModeless();
  
  return result ? NS_OK : NS_ERROR_FAILURE;
}
*/
//*****************************************************************************
// CPromptServiceFactory
//*****************************************************************************   

class CPromptServiceFactory : public nsIFactory {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CPromptServiceFactory();
  virtual ~CPromptServiceFactory();
};
/*
//*****************************************************************************

NS_IMPL_ISUPPORTS1(CPromptServiceFactory, nsIFactory)

CPromptServiceFactory::CPromptServiceFactory() {

}

CPromptServiceFactory::~CPromptServiceFactory() {
}

NS_IMETHODIMP CPromptServiceFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CPromptService *inst = new CPromptService;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CPromptServiceFactory::LockFactory(PRBool lock)
{
  return NS_OK;
}

//*****************************************************************************

nsresult NS_NewPromptServiceFactory(nsIFactory** aFactory)
{
  NS_ENSURE_ARG_POINTER(aFactory);
  *aFactory = nullptr;
  
  CPromptServiceFactory *result = new CPromptServiceFactory;
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;
    
  NS_ADDREF(result);
  *aFactory = result;
  
  return NS_OK;
}
*/