#include "stdafx.h"
#include "nsIWindowWatcher.h"
#include "nsIIOService.h"
#include "nsIDOMWindow2.h"

nsresult NewURI(nsIURI **result, const nsACString &spec)
{
  nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, NS_ERROR_UNEXPECTED);

  return ios->NewURI(spec, nsnull, nsnull, result);
}

nsresult NewURI(nsIURI **result, const nsAString &spec)
{
  nsEmbedCString specUtf8;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, specUtf8);
  return NewURI(result, specUtf8);
}


nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
{
	NS_ENSURE_ARG(aWebBrowser);
	nsCOMPtr<nsIDOMWindow> domWin;
	aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
	NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);

  	nsCOMPtr<nsIDOMWindow2> domWin2 (do_QueryInterface (domWin));
	NS_ENSURE_TRUE (domWin2, NS_ERROR_FAILURE);
	
	return domWin2->GetWindowRoot (aTarget);
}


CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsIWindowWatcher> mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  NS_ENSURE_TRUE (mWWatch, nsnull);

  nsCOMPtr<nsIWebBrowserChrome> chrome;
  CWnd *val = 0;

  if (mWWatch) {
    nsCOMPtr<nsIDOMWindow> fosterParent;
    if (!aWindow) { // it will be a dependent window. try to find a foster parent.
      mWWatch->GetActiveWindow(getter_AddRefs(fosterParent));
      aWindow = fosterParent;
    }
    mWWatch->GetChromeForWindow(aWindow, getter_AddRefs(chrome));
  }

  if (chrome) {
    nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
    if (site) {
      HWND w;
      site->GetSiteWindow(reinterpret_cast<void **>(&w));
      val = CWnd::FromHandle(w);
    }
  }

  return val;
}

