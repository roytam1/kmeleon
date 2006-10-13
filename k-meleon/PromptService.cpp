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
#include "PromptService.h"
#include "nsIPromptService.h"
#include "nsIWindowWatcher.h"

extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

//*****************************************************************************
// CPromptService
//*****************************************************************************



//*****************************************************************************

NS_IMPL_ISUPPORTS2(CPromptService, nsIPromptService, nsINonBlockingAlertService)

CPromptService::CPromptService()
{
}

CPromptService::~CPromptService() {
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
                                         PRBool *checkValue)
{
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.AddButton(100, IDS_OK);
  dlg.SetDefaultButton(100);
  dlg.SetCancelButton(100);
  dlg.SetMsgIcon(AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION));
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  dlg.DoModal();

  if (checkValue)
	*checkValue = (checkResult == TRUE ? PR_TRUE : PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP CPromptService::Confirm(nsIDOMWindow *parent,
                                      const PRUnichar *dialogTitle,
                                      const PRUnichar *text,
                                      PRBool *_retval)
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
                                           PRBool *checkValue,
                                           PRBool *_retval)
{
	const int COMMAND_OFFSET = 100;

	USES_CONVERSION;
    CWnd *wnd = CWndForDOMWindow(parent);
    CGenericDlg dlg(wnd);
    dlg.SetTitle(W2CT(dialogTitle));
    dlg.SetMsg(W2CT(text));
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
                                     PRBool *checkValue,
                                     PRBool *_retval)
{
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);

  CString csValue;
  if (value && *value) csValue = W2CT(*value);
  dlg.AddEdit(&csValue, "", FALSE);
  
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
	 if (value) {
		if (*value) nsMemory::Free(*value);
      nsString nsPromptEditValue;
		nsPromptEditValue.Assign(T2CW(csValue));
		*value = NS_StringCloneData(nsPromptEditValue);
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
                                                        PRBool *checkValue,
                                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(username);
  NS_ENSURE_ARG_POINTER(password);

  USES_CONVERSION;
  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);
    
  CString csUsername, csPassword;
  if (username && *username) csUsername = W2CT(*username);
  if (password && *password) csPassword = W2CT(*password);
  dlg.AddEdit(&csUsername, "User Name:", FALSE);
  dlg.AddEdit(&csPassword, "Password:", TRUE);
  
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
	  if(*username) nsMemory::Free(*username);
	  *username = NS_StringCloneData(nsEmbedString(T2CW(csUsername)));

	  if (*password) nsMemory::Free(*password);
	  *password = NS_StringCloneData(nsEmbedString(T2CW(csPassword)));

     if (checkValue)
        *checkValue = (checkResult == TRUE ? PR_TRUE : PR_FALSE);

	  *_retval = PR_TRUE;
  }
  else
    *_retval = PR_FALSE;
 
  return NS_OK;
}

NS_IMETHODIMP CPromptService::PromptPassword(nsIDOMWindow *parent,
                                             const PRUnichar *dialogTitle,
                                             const PRUnichar *text,
                                             PRUnichar **password,
                                             const PRUnichar *checkboxMsg,
                                             PRBool *checkValue,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(password);
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CGenericDlg dlg(wnd);
  dlg.SetTitle(W2CT(dialogTitle));
  dlg.SetMsg(W2CT(text));
  dlg.AddButton(IDOK, IDS_OK);
  dlg.AddButton(IDCANCEL, IDS_CANCEL);
  dlg.SetDefaultButton(IDOK);
  dlg.SetCancelButton(IDCANCEL);

  CString csValue;
  if (*password) csValue = W2CT(*password);
  dlg.AddEdit(&csValue, "", TRUE);
  
  BOOL checkResult;
  if (checkboxMsg && checkValue) {
    checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
    dlg.AddCheckBox(&checkResult, W2CT(checkboxMsg));
  }

  if (dlg.DoModal() == IDOK) {
     if (*password) nsMemory::Free(*password);
	  *password = NS_StringCloneData(nsEmbedString(T2CW(csValue)));
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
                                     PRBool *_retval)
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
                                        PRBool *checkValue,
                                        PRInt32 *buttonPressed)
{
	const int COMMAND_OFFSET = 100;
    USES_CONVERSION;

	CWnd *wnd = CWndForDOMWindow(parent);
	CGenericDlg dlg(wnd);
	dlg.SetTitle(W2CT(dialogTitle));
	dlg.SetMsg(W2CT(text));

	//https://bugzilla.mozilla.org/show_bug.cgi?id=329414
	//Set the cancel button to 1, and the default one.
	dlg.SetDefaultButton(COMMAND_OFFSET + ((buttonFlags & 0x03000000) >> 24));
	dlg.SetCancelButton(COMMAND_OFFSET + 1);

	// Determine the button titles based on buttonFlags
    const PRUnichar* buttonStrings[] = { button0Title, button1Title, button2Title };

    for(int i=0; i<3; i++)
    {
        switch(buttonFlags & 0xff) {
            case BUTTON_TITLE_OK:
				dlg.AddButton(COMMAND_OFFSET+i, IDS_OK);
                break;
            case BUTTON_TITLE_CANCEL:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_CANCEL);
                break;
            case BUTTON_TITLE_YES:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_YES);
                break;
            case BUTTON_TITLE_NO:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_NO);
                break;
            case BUTTON_TITLE_SAVE:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_SAVE);
                break;
            case BUTTON_TITLE_DONT_SAVE:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_DONTSAVE);
                break;
            case BUTTON_TITLE_REVERT:
                dlg.AddButton(COMMAND_OFFSET+i, IDS_REVERT);
                break;
            case BUTTON_TITLE_IS_STRING:
                dlg.AddButton(COMMAND_OFFSET+i, W2CT(buttonStrings[i]));
                break;
        }
   
        buttonFlags >>= 8;    
    }

	BOOL checkResult;
	if (checkMsg && checkValue) {
		checkResult = (*checkValue == PR_TRUE ? TRUE : FALSE);
		dlg.AddCheckBox(&checkResult, W2CT(checkMsg));
	}
	
	*buttonPressed = dlg.DoModal() - COMMAND_OFFSET;
	if (checkValue)
		*checkValue = (checkResult == TRUE ? PR_TRUE : PR_FALSE);

    return NS_OK;    
}
 
NS_IMETHODIMP
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
  dlg->SetMsgIcon(AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION));
  dlg->AddButton(100, IDS_OK);
  dlg->SetDefaultButton(100);
  dlg->SetCancelButton(100);

  result = dlg->DoModeless();
  
  return result ? NS_OK : NS_ERROR_FAILURE;
}

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
  *aFactory = nsnull;
  
  CPromptServiceFactory *result = new CPromptServiceFactory;
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;
    
  NS_ADDREF(result);
  *aFactory = result;
  
  return NS_OK;
}
*/