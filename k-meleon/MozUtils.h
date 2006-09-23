
#ifndef __MOZUTILS_H__
#define __MOZUTILS_H__

#include "stdafx.h"

nsresult NewURI(nsIURI **result, const nsACString &spec);
nsresult NewURI(nsIURI **result, const nsAString &spec);
nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget);
CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);
CString GetMozDirectory(char* dirName);

#endif