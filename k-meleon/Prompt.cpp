/*
*  Copyright (C) 2000 Brian Harris
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
*/

#include "stdafx.h"
#include "MainFrm.h"

#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsIDocShellTreeItem.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsCWebBrowser.h"
#include "nsWidgetsCID.h"

#include "Prompt.h"

#include "resource.h"

NS_IMPL_ISUPPORTS1(CPrompt, nsIPrompt)

CPrompt::CPrompt(){
	NS_INIT_ISUPPORTS();
}

CPrompt::~CPrompt(){

}

NS_IMETHODIMP CPrompt::Alert(const PRUnichar *dialogTitle, const PRUnichar *text)
{
	nsString title(dialogTitle);
	nsString txt(text);
	char *strTitle=title.ToNewCString();
	char *strTxt=txt.ToNewCString();
	MessageBox(NULL,strTxt,strTitle,MB_ICONWARNING);
	Recycle(strTxt);
	Recycle(strTitle);
    return NS_OK;
}

NS_IMETHODIMP CPrompt::AlertCheck(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue)
{
  MessageBox(NULL,"AlertCheck called","hu?",0);
  return NS_OK;
}

NS_IMETHODIMP CPrompt::Confirm(const PRUnichar *dialogTitle, const PRUnichar *text, PRBool *_retval)
{
	nsString title(dialogTitle);
	nsString txt(text);
	char *strTitle=title.ToNewCString();
	char *strTxt=txt.ToNewCString();
	*_retval=(MessageBox(NULL,strTxt,strTitle,MB_OKCANCEL|MB_ICONQUESTION)==IDOK);
	Recycle(strTxt);
	Recycle(strTitle);

    return NS_OK;
}

NS_IMETHODIMP CPrompt::ConfirmCheck(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    return NS_OK;
}

typedef struct {
	// global
	const PRUnichar *dialogTitle;
	const PRUnichar *text;
	const PRUnichar *passwordRealm;
	PRUint32 savePassword;

	// only Prompt
	const PRUnichar *defaultText;
	PRUnichar **result;

	// only Prompt Username/Password
	PRUnichar **user;
	PRUnichar **pwd;

} prompt_info;

BOOL CALLBACK PromptFunc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
  )
{
	static prompt_info *pi;
	switch (uMsg){
	case WM_INITDIALOG:
		{
			pi = (prompt_info *)lParam;

			nsString title(pi->dialogTitle);
			nsString txt(pi->text);

			char *strTitle=title.ToNewCString();
			char *strTxt=txt.ToNewCString();

			if (strTitle[0]){
				SetWindowText(hwndDlg, strTitle);
			}else{
				SetWindowText(hwndDlg, "Popup! Huzzah!");
			}
			SetDlgItemText(hwndDlg, IDC_CAPTION, strTxt);

			Recycle(strTxt);
			Recycle(strTitle);

			if (pi->pwd && !pi->user){
				HWND userBox = GetDlgItem(hwndDlg, IDC_EDIT_USERNAME);
				LONG style = GetWindowLong(userBox, GWL_STYLE);
				SetWindowLong(userBox, GWL_STYLE, style | WS_DISABLED);
			}
		}

		// true = did not call SetFocus
		return true;

	case WM_COMMAND:
		switch(wParam){
		case IDOK:
			{
				char result[1024];

				if (pi->result){
					GetDlgItemText(hwndDlg, IDC_EDITBOX, result, 1023);

					nsString nsResult;
					nsResult.AssignWithConversion(result);

					*pi->result = nsResult.ToNewUnicode();
				}
				if (pi->user){
					GetDlgItemText(hwndDlg, IDC_EDIT_USERNAME, result, 1023);

					nsString nsResult;
					nsResult.AssignWithConversion(result);

					*pi->user = nsResult.ToNewUnicode();
				}
				if (pi->pwd){
					GetDlgItemText(hwndDlg, IDC_EDIT_PASSWORD, result, 1023);

					nsString nsResult;
					nsResult.AssignWithConversion(result);

					*pi->pwd = nsResult.ToNewUnicode();
				}

				EndDialog(hwndDlg, true);

			}

			break;

		case IDCANCEL:
			EndDialog(hwndDlg, false);

			break;
		}
	default:
		// false = did not process
		return false;
	}
	// true = processed message
	return true;
}

NS_IMETHODIMP CPrompt::Prompt(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, const PRUnichar *defaultText, PRUnichar **result, PRBool *_retval)
{
	prompt_info pi;

	pi.defaultText = defaultText;
	pi.dialogTitle = dialogTitle;
	pi.passwordRealm = passwordRealm;
	pi.result = result;
	pi.savePassword = savePassword;
	pi.text = text;

	pi.user = NULL;
	pi.pwd = NULL;

	*_retval = DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_PROMPT), AfxGetMainWnd()->GetSafeHwnd(), PromptFunc, (LPARAM)&pi);

    return NS_OK;
}

NS_IMETHODIMP CPrompt::PromptUsernameAndPassword(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, PRUnichar **user, PRUnichar **pwd, PRBool *_retval)
{
	prompt_info pi;

	pi.dialogTitle = dialogTitle;
	pi.passwordRealm = passwordRealm;
	pi.savePassword = savePassword;
	pi.text = text;
	pi.user = user;
	pi.pwd = pwd;

	pi.result = NULL;
	pi.defaultText = NULL;

	*_retval = DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_PROMPT_USERNAME_PASSWORD), AfxGetMainWnd()->GetSafeHwnd(), PromptFunc, (LPARAM)&pi);

    return NS_OK;
}

NS_IMETHODIMP CPrompt::PromptPassword(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, PRUnichar **pwd, PRBool *_retval)
{
	prompt_info pi;

	pi.dialogTitle = dialogTitle;
	pi.passwordRealm = passwordRealm;
	pi.savePassword = savePassword;
	pi.text = text;
	pi.pwd = pwd;

	pi.user = NULL;

	pi.result = NULL;
	pi.defaultText = NULL;

	*_retval = DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_PROMPT_USERNAME_PASSWORD), AfxGetMainWnd()->GetSafeHwnd(), PromptFunc, (LPARAM)&pi);

    return NS_OK;
}

NS_IMETHODIMP CPrompt::Select(const PRUnichar *dialogTitle, const PRUnichar *text, PRUint32 count, const PRUnichar **selectList, PRInt32 *outSelection, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CPrompt::UniversalDialog(const PRUnichar *titleMessage, const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkboxMsg, const PRUnichar *button0Text, const PRUnichar *button1Text, const PRUnichar *button2Text, const PRUnichar *button3Text, const PRUnichar *editfield1Msg, const PRUnichar *editfield2Msg, PRUnichar **editfield1Value, PRUnichar **editfield2Value, const PRUnichar *iconURL, PRBool *checkboxState, PRInt32 numberButtons, PRInt32 numberEditfields, PRInt32 editField1Password, PRInt32 *buttonPressed)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}