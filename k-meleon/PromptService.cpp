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
#include "Dialogs.h"
#include "PromptService.h"
#include "nsIPromptService.h"
#include "nsIWindowWatcher.h"

extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

//*****************************************************************************
// CPromptService
//*****************************************************************************



//*****************************************************************************

NS_IMPL_ISUPPORTS1(CPromptService, nsIPromptService)

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
  CAlertCheckDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
                    W2CT(checkboxMsg), checkValue ? *checkValue : 0);

  dlg.DoModal();

  *checkValue = dlg.m_bCheckBoxValue;

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
    USES_CONVERSION;

    CWnd *wnd = CWndForDOMWindow(parent);
	CString sYes, sNo;
	sYes.LoadString(IDS_YES);
	sNo.LoadString(IDS_NO);
    CConfirmCheckDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
                    W2CT(checkboxMsg), checkValue ? *checkValue : 0,
                    sYes, sNo, NULL, 1);

    int iBtnClicked = dlg.DoModal();

    *checkValue = dlg.m_bCheckBoxValue;

    *_retval = iBtnClicked == 0 ? PR_TRUE : PR_FALSE;

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
  CPromptDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
					*value ? W2CT(*value) : NULL,
                    checkValue && checkboxMsg, W2CT(checkboxMsg),
                    checkValue ? *checkValue : 0);
  if(dlg.DoModal() == IDOK) {
    // Get the value entered in the editbox of the PromptDlg
    if (value && *value) {
      nsMemory::Free(*value);
      *value = nsnull;
    }
    nsString csPromptEditValue;
    csPromptEditValue.Assign(T2CW(dlg.m_csPromptAnswer.GetBuffer(0)));

    *value = NS_StringCloneData(csPromptEditValue);

    *_retval = PR_TRUE;
  } else
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
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CPromptUsernamePasswordDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
                    username && *username ? W2CT(*username) : 0,
                    password && *password ? W2CT(*password) : 0, 
                    checkValue != nsnull, W2CT(checkboxMsg),
                    checkValue ? *checkValue : 0);

  if (dlg.DoModal() == IDOK) {
    // Get the username entered
    if (username && *username) {
        nsMemory::Free(*username);
        *username = nsnull;
    }
	
	{USES_CONVERSION;
    nsString csUserName;
    csUserName.Assign(T2CW(dlg.m_csUserName.GetBuffer(0)));
	*username = NS_StringCloneData(csUserName);}

    // Get the password entered
    if (password && *password) {
      nsMemory::Free(*password);
      *password = nsnull;
    }
	{USES_CONVERSION;
    nsString csPassword;
    csPassword.Assign(T2CW(dlg.m_csPassword.GetBuffer(0)));
	*password = NS_StringCloneData(csPassword);}

    if(checkValue)		
      *checkValue = dlg.m_bCheckBoxValue;

    *_retval = PR_TRUE;
  } else
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
  USES_CONVERSION;

  CWnd *wnd = CWndForDOMWindow(parent);
  CPromptPasswordDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
                            password && *password ? W2CT(*password) : 0,
                            checkValue != nsnull, W2CT(checkboxMsg),
                            checkValue ? *checkValue : 0);
  if(dlg.DoModal() == IDOK) {
    // Get the password entered
    if (password && *password) {
        nsMemory::Free(*password);
        *password = nsnull;
    }
	
	USES_CONVERSION;
    nsString csPassword;
    csPassword.Assign(T2CW(dlg.m_csPassword.GetBuffer(0)));
    *password = NS_StringCloneData(csPassword);

    if(checkValue)
      *checkValue = dlg.m_bCheckBoxValue;

    *_retval = PR_TRUE;
  } else
    *_retval = PR_FALSE;

  return NS_OK;
}

// Used to add boolean value in about:config
// Maybe for something else too?
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
    USES_CONVERSION;

    // First, determine the button titles based on buttonFlags
    const PRUnichar* buttonStrings[] = { button0Title, button1Title, button2Title };
    CString csBtnTitles[3];

	//Set the cancel button to 1, and the default one.
	int defButton = 256 + ((buttonFlags & 0x03000000) >> 24);
    for(int i=0; i<3; i++)
    {
        switch(buttonFlags & 0xff) {
            case BUTTON_TITLE_OK:
                csBtnTitles[i].LoadString(IDS_OK);
                break;
            case BUTTON_TITLE_CANCEL:
                csBtnTitles[i].LoadString(IDS_CANCEL);
                break;
            case BUTTON_TITLE_YES:
                csBtnTitles[i].LoadString(IDS_YES);
                break;
            case BUTTON_TITLE_NO:
                csBtnTitles[i].LoadString(IDS_NO);
                break;
            case BUTTON_TITLE_SAVE:
                csBtnTitles[i].LoadString(IDS_SAVE);
                break;
            case BUTTON_TITLE_DONT_SAVE:
                csBtnTitles[i].LoadString(IDS_DONTSAVE);
                break;
            case BUTTON_TITLE_REVERT:
                csBtnTitles[i].LoadString(IDS_REVERT);
                break;
            case BUTTON_TITLE_IS_STRING:
                csBtnTitles[i] = W2CT(buttonStrings[i]);
                break;
        }
   
        buttonFlags >>= 8;    
    }

    CWnd *wnd = CWndForDOMWindow(parent);
    CConfirmCheckDialog dlg(wnd, W2CT(dialogTitle), W2CT(text),
        checkMsg ? W2CT(checkMsg) : NULL, checkValue ? *checkValue : 0,
                    csBtnTitles[0].IsEmpty() ? NULL : (LPCTSTR)csBtnTitles[0], 
                    csBtnTitles[1].IsEmpty() ? NULL : (LPCTSTR)csBtnTitles[1], 
                    csBtnTitles[2].IsEmpty() ? NULL : (LPCTSTR)csBtnTitles[2],
					defButton);

    *buttonPressed = dlg.DoModal();

    if(checkValue)
        *checkValue = dlg.m_bCheckBoxValue;

    return NS_OK;    
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