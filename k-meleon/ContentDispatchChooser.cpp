#include "stdafx.h"
#include "MozUtils.h"
#include "resource.h"
#include "GenericDlg.h"
#include "ContentDispatchChooser.h"

#include "nsIHandlerService.h"
#include "nsIMIMEInfo.h"

NS_IMPL_ISUPPORTS1(ContentDispatchChooser, nsIContentDispatchChooser);

ContentDispatchChooser::ContentDispatchChooser(void)
{
}

ContentDispatchChooser::~ContentDispatchChooser(void)
{
}

NS_IMETHODIMP ContentDispatchChooser::Ask(nsIHandlerInfo *aHandler,	nsIInterfaceRequestor *aWindowContext, nsIURI *aURI, PRUint32 aReason)
{
	nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(aWindowContext));
	CWnd *wnd = CWndForDOMWindow(domWindow);
	
	nsEmbedCString scheme;
	aURI->GetScheme(scheme);
	CString csScheme = NSCStringToCString(scheme);
	
	PRBool hasDefault = PR_FALSE;
	aHandler->GetHasDefaultHandler(&hasDefault);
	if (!hasDefault)
	{
		CString caption;
		caption.Format(IDS_EXTERNAL_HANDLER_CAPTION);
		CString text;
		text.Format(IDS_EXTERNAL_HANDLER_REQUIRED, csScheme);
		wnd->MessageBox(text, caption, MB_ICONEXCLAMATION);
		return NS_OK;
	}
	
	CGenericDlg dlg;
	dlg.SetMsgIcon(AfxGetApp()->LoadStandardIcon(IDI_QUESTION));
	dlg.SetDefaultButton(IDOK);
	dlg.SetCancelButton(IDCANCEL);

	nsEmbedString defaultDesc;
	nsresult rv = aHandler->GetDefaultDescription(defaultDesc);
	CString csDesc = NSStringToCString(defaultDesc);
	
	CString str;
	str.Format(IDS_EXTERNAL_HANDLER_CAPTION);
	dlg.SetTitle(str);

	dlg.AddButton(IDOK, IDS_YES);
	dlg.AddButton(IDCANCEL, IDS_CANCEL);
	
	str.Format(IDS_EXTERNAL_HANDLER_REMEMBER, csScheme, csDesc);
	BOOL remember = FALSE;
	dlg.AddCheckBox(&remember, str);

	str.Format(IDS_EXTERNAL_HANDLER_QUESTION, csScheme, csDesc);
	dlg.SetMsg(str);

	int answer = dlg.DoModal();
	
	if (answer == IDOK)
	{
		if (remember)
		{
			aHandler->SetPreferredAction(nsIHandlerInfo::useSystemDefault);
			aHandler->SetAlwaysAskBeforeHandling(PR_FALSE);
			nsCOMPtr<nsIHandlerService> handlerService(do_GetService("@mozilla.org/uriloader/handler-service;1"));
			if (handlerService) handlerService->Store(aHandler);
		}
		aHandler->LaunchWithURI(aURI, aWindowContext);
	}
	
	return NS_OK;
}


