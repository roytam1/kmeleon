/*
 *  Copyright (C) 2006 Dorian Boissonnade
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
#include "BrowserWindow.h"
#include "BrowserImpl.h"
#include "MozUtils.h"
#include "PrintSetupDialog.h"
#include "PrintProgressDialog.h"
#include "SaveAsHandler.h"
#include "mfcembed.h"
#include "Utils.h"

#include "nsIWidget.h"
#include "nsISHistory.h"
#include "nsISHEntry.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDocShellTreeItem.h"

#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h" 
#include "nsIDocCharset.h"
#include "nsISelection.h"
#include "nsISHistoryInternal.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMWindowCollection.h"
#include "nsIWebPageDescriptor.h"
#include "nsIDocShell.h"

#include "nsIDOMEventTarget.h"
#include "nsIScriptGlobalObject.h"

#include "nsISecureBrowserUI.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsICertificateDialogs.h"
#include "nsIScriptSecurityManager.h"

#include "nsIDOMHTMLTextAreaElement.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsITypeAheadFind.h"
//#include "nsIFocusController.h"
//#include "nsIDOMNSHTMLDocument.h"

#include "nsIPrintSettingsService.h"
#include "nsCWebBrowserPersist.h"
#include "nsIDomLocation.h"

#include "nsIX509Cert.h"

/* For highlight only x_x */
#include "nsIFind.h"
#include "nsIDOMText.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsIDOMNodeList.h"
//#include "nsIDOMDocumentRange.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMWindowCollection.h"
#include "nsIFocusManager.h"
#include "nsIScriptContext.h"

static const PRUnichar kSpan[] = L"span";
static const PRUnichar kStyle[] = L"style";
static const PRUnichar kClass[] = L"class";
static const PRUnichar kHighlighClassName[] = L"km_hightlight_class";
static const PRUnichar kDefaultHighlightStyle[] = L"display: inline;font-size: inherit;padding: 0;color: black;background-color: yellow;";
/* */

CBrowserWrapper::CBrowserWrapper(void)
{
	//mDomEventListener = nullptr;
	mpBrowserImpl = nullptr;
	mLastMouseActionNode = nullptr;

	mpBrowserImpl = NULL;
	mpBrowserFrameGlue = NULL;
	mpBrowserGlue = NULL;
	mChromeContent = FALSE;
	mWndOwner = NULL;
	BOOL mIsDocumentLoading = FALSE;
	BOOL mIsDOMLoaded = FALSE;
}

CBrowserWrapper::~CBrowserWrapper(void)
{
}

void CBrowserWrapper::SetBrowserGlue(PBROWSERGLUE pBrowserGlue)
{
    mpBrowserGlue = pBrowserGlue;
	if (mpBrowserImpl)
		mpBrowserImpl->Init(mpBrowserGlue, mWebBrowser);
}

void CBrowserWrapper::SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue)
{
    mpBrowserFrameGlue = pBrowserFrameGlue;
//	if (mpBrowserImpl)
//		mpBrowserImpl->Init(pBrowserFrameGlue, mWebBrowser);
}

BOOL CBrowserWrapper::CreateBrowser(CWnd* parent, uint32_t chromeFlags)
{
	mChromeContent = chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME;
	mWndOwner = parent;

	// Create web shell
	nsresult rv;
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	NS_ENSURE_SUCCESS(rv, FALSE);

	// Save off the nsIWebNavigation interface pointer 
	// in the mWebNav member variable which we'll use 
	// later for web page navigation
	mWebNav = do_QueryInterface(mWebBrowser, &rv);
	NS_ENSURE_SUCCESS(rv, FALSE);

	// Get the focus object
	mWebBrowserFocus = do_QueryInterface(mWebBrowser, &rv);
	NS_ENSURE_SUCCESS(rv, FALSE);

	// Create the CBrowserImpl object - this is the object
	// which implements the interfaces which are required
	// by an app embedding mozilla i.e. these are the interfaces
	// thru' which the *embedded* browser communicates with the
	// *embedding* app
	//
	// The CBrowserImpl object will be passed in to the 
	// SetContainerWindow() call below
	//
	// Also note that we're passing the BrowserFrameGlue pointer 
	// and also the mWebBrowser interface pointer via CBrowserImpl::Init()
	// of CBrowserImpl object. 
	// These pointers will be saved by the CBrowserImpl object.
	// The CBrowserImpl object uses the BrowserFrameGlue pointer to 
	// call the methods on that interface to update the status/progress bars
	// etc.
	mpBrowserImpl = new CBrowserImpl();
	NS_ENSURE_TRUE(mpBrowserImpl, FALSE);

	// Pass along the mpBrowserFrameGlue pointer to the BrowserImpl object
	// This is the interface thru' which the XP BrowserImpl code communicates
	// with the platform specific code to update status bars etc.
	mpBrowserImpl->Init(mpBrowserGlue, mWebBrowser);
	mpBrowserImpl->SetChromeFlags(chromeFlags);
	mpBrowserImpl->AddRef();
	mWebBrowser->SetContainerWindow(static_cast<nsIWebBrowserChrome*>(mpBrowserImpl));


	nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser, &rv);
	NS_ENSURE_SUCCESS(rv, FALSE);

	// XXX Content and chrome dialog are currently the same type of window.
	// Set the correct type depending if this window will host chrome or content.
	dsti->SetItemType( chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME ?
			nsIDocShellTreeItem::typeChromeWrapper :
			nsIDocShellTreeItem::typeContentWrapper);

	// Create the real webbrowser window

	// Note that we're passing the m_hWnd in the call below to InitWindow()
	// (CBrowserView inherits the m_hWnd from CWnd)
	// This m_hWnd will be used as the parent window by the embeddable browser

	mBaseWindow = do_QueryInterface(mWebBrowser, &rv);
	if (NS_FAILED(rv)) {
		DestroyBrowser();
		return FALSE;
	}
		
	CRect rect;
	parent->GetClientRect(rect);
	rv = mBaseWindow->InitWindow(nsNativeWidget(parent->GetSafeHwnd()), nullptr, 0, 0, rect.Width(), rect.Height());
	rv = mBaseWindow->Create();
	
	if (mChromeContent) {
	// Eagerly create an about:blank content viewer with the right principal here,
  // rather than letting it happening in the upcoming call to
  // SetInitialPrincipalToSubject. This avoids creating the about:blank document
  // and then blowing it away with a second one, which can cause problems for the
  // top-level chrome window case. See bug 789773.
	  nsCOMPtr<nsIScriptSecurityManager> ssm =
		do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
	  if (ssm) { // Sometimes this happens really early  See bug 793370.
		nsCOMPtr<nsIPrincipal> principal;
		ssm->GetSubjectPrincipal(getter_AddRefs(principal));
		if (!principal) {
		  ssm->GetSystemPrincipal(getter_AddRefs(principal));
		}
		rv = GetDocShell()->CreateAboutBlankContentViewer(principal);
		NS_ENSURE_SUCCESS(rv, rv);
		//nsCOMPtr<nsIDocument> doc = do_GetInterface(GetDocShell());
		//NS_ENSURE_TRUE(!!doc, NS_ERROR_FAILURE);
		//doc->SetIsInitialDocument(true);
	  }
	}

	// Register the BrowserImpl object to receive progress messages
	// These callbacks will be used to update the status/progress bars
    nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(static_cast<nsIWebProgressListener*>(mpBrowserImpl));
    mWebBrowser->AddWebBrowserListener(weak, NS_GET_IID(nsIWebProgressListener));

	// Add listeners for various events like popup-blocking, link added ... 
	AddListeners();

	// Find again observer to know when a new search was triggered
	/*	nsCOMPtr<nsIObserverService> observerService = 
	do_GetService("@mozilla.org/observer-service;1", &rv);
	if (NS_SUCCEEDED(rv))
	observerService->AddObserver(mpBrowserImpl, "nsWebBrowserFind_FindAgain", PR_TRUE);*/

	//History listener
	/*	nsWeakPtr weakling2(
	do_GetWeakReference(NS_STATIC_CAST(nsISHistoryListener*, mpBrowserImpl)));
	(void)mWebBrowser->AddWebBrowserListener(weakling2, 
	NS_GET_IID(nsISHistoryListener));*/

	/*
	nsCOMPtr<nsPIDOMWindow> piWin;

	// get the content DOM window for that web browser
	nsCOMPtr<nsIDOMWindow> domWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	if (!domWindow)
	return NS_ERROR_FAILURE;

	// get the private DOM window
	nsCOMPtr<nsPIDOMWindow> domWindowPrivate = do_QueryInterface(domWindow);
	// and the root window for that DOM window
	piWin = domWindowPrivate->GetPrivateRoot();

	nsCOMPtr<nsIDOMEventReceiver> mEventReceiver = do_QueryInterface(piWin->GetChromeEventHandler());
	rv = mEventReceiver->AddEventListenerByIID(mpBrowserImpl,
	NS_GET_IID(nsIDOMMouseListener));
	*/
	
    // Finally, show the web browser window
    mBaseWindow->SetVisibility(PR_TRUE);
	return TRUE;
}

void CBrowserWrapper::ShowScrollbars(BOOL visible)
{
	nsCOMPtr<nsIScrollable> scrollable(do_QueryInterface(GetDocShell()));
	NS_ENSURE_TRUE(scrollable, );

	if (!visible) {
		scrollable->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_X, nsIScrollable::Scrollbar_Never);
		scrollable->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_Y, nsIScrollable::Scrollbar_Never);
	}
}

BOOL CBrowserWrapper::DestroyBrowser()
{
/*	nsresult rv;
	nsCOMPtr<nsIObserverService> observerService = 
        do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv))
      observerService->RemoveObserver(mpBrowserImpl, "nsWebBrowserFind_FindAgain");*/

	/*nsCOMPtr<nsIWebBrowserPrint> print;
	nsIDocShell* shell = GetDocShell();	
	shell->GetPrintPreview(getter_AddRefs(print));
	if (print) {
		bool isPreview = false;
		print->GetDoingPrintPreview(&isPreview);
		if (isPreview) 
			print->ExitPrintPreview();
	}*/

	RemoveListeners();

	if (mWebNav)
	{
		mWebNav->Stop(nsIWebNavigation::STOP_ALL);
		mWebNav = nullptr;
	}

    if (mBaseWindow)
    {
        mBaseWindow->Destroy();
        mBaseWindow = nullptr;
    }

    if (mpBrowserImpl)
    {
		mpBrowserImpl->Init(NULL, mWebBrowser);
        mpBrowserImpl->Release();
        mpBrowserImpl = nullptr;
    }

	return TRUE;
}



BOOL CBrowserWrapper::AddListeners(void)
{
	nsresult rv;
	rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
	NS_ENSURE_TRUE(mEventTarget, FALSE);

	mWebNav = do_QueryInterface(mWebBrowser);
	NS_ENSURE_TRUE(mWebNav, FALSE);
    
	mWebBrowserFocus = do_QueryInterface(mWebBrowser);
	NS_ENSURE_TRUE(mWebBrowserFocus, FALSE);

	/*mDomEventListener = new CDomEventListener();
	if(mDomEventListener == nullptr) return FALSE;
	mDomEventListener->Init(mpBrowserFrameGlue);
*/
	if (!mChromeContent) {
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("click"),
			mpBrowserImpl, true);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("mousedown"),
			mpBrowserImpl, true);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("keypress"),
			mpBrowserImpl, true);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("DOMPopupBlocked"),
			mpBrowserImpl, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("DOMLinkAdded"),
			mpBrowserImpl, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("DOMContentLoaded"),
			mpBrowserImpl, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("flashblockCheckLoad"),
			mpBrowserImpl, true, true, 2);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("contextmenu"), 
			mpBrowserImpl, false, false);
		/*rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("mozfullscreenchange"), 
			mpBrowserImpl, false);		
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("MozEnteredDomFullscreen"), 
			mpBrowserImpl, false);		*/
	}

	rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("command"),
	mpBrowserImpl, false);

	return TRUE;
}

void CBrowserWrapper::RemoveListeners(void)
{
	NS_ENSURE_TRUE(mEventTarget, );
	if (!mChromeContent) {
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("click"),
			mpBrowserImpl, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"),
			mpBrowserImpl, true);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"),
			mpBrowserImpl, true);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("DOMPopupBlocked"),
			mpBrowserImpl, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("DOMLinkAdded"),
			mpBrowserImpl, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("DOMContentLoaded"),
			mpBrowserImpl, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("contextmenu"), 
			mpBrowserImpl, false);
	}
	mEventTarget->RemoveEventListener(NS_LITERAL_STRING("command"),
		mpBrowserImpl, false);
}

BOOL CBrowserWrapper::LoadURL(LPCTSTR url, LPCTSTR referrer, BOOL allowFixup)
{
	//ASSERT(url);
	ASSERT(mWebNav);
	NS_ENSURE_TRUE(url, FALSE);
	NS_ENSURE_TRUE(mWebNav, FALSE);

	nsCOMPtr<nsIURI> referrerURI;
	if (referrer)
#ifdef _UNICODE
		NewURI(getter_AddRefs(referrerURI), nsDependentString(referrer));
#else
		NewURI(getter_AddRefs(referrerURI), nsDependentCString(referrer));
#endif

	nsresult rv = mWebNav->LoadURI(CStringToPRUnichar(url), 
		                 allowFixup ?            
							nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP : 
							nsIWebNavigation::LOAD_FLAGS_NONE, 
		                 referrerURI,                 
		                 nullptr,                 
		                 nullptr);        
	return NS_SUCCEEDED(rv);
}

BOOL CBrowserWrapper::LoadURL(LPCTSTR url, BOOL currentForRef, BOOL allowFixup)
{
	ASSERT(url);
	ASSERT(mWebNav);
	NS_ENSURE_TRUE(url, FALSE);
	NS_ENSURE_TRUE(mWebNav, FALSE);

    nsCOMPtr<nsIURI> referrerURI;
	if (currentForRef) mWebNav->GetCurrentURI(getter_AddRefs(referrerURI));

	nsresult rv = mWebNav->LoadURI(CStringToPRUnichar(url), 
		                 allowFixup ?            
							nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP : 
							nsIWebNavigation::LOAD_FLAGS_NONE, 
		                 referrerURI,                 
		                 nullptr,                 
		                 nullptr);
	return NS_SUCCEEDED(rv);
}

CString CBrowserWrapper::GetTitle()
{
	NS_ENSURE_TRUE(mBaseWindow, _T(""));
	PRUnichar *idlStrTitle = nullptr;

	mBaseWindow->GetTitle(&idlStrTitle);
	NS_ENSURE_TRUE(idlStrTitle, _T(""));

	CString title = PRUnicharToCString(idlStrTitle);
	nsMemory::Free(idlStrTitle);
	return title;
}

CString CBrowserWrapper::GetURI(BOOL unescape)
{
	NS_ENSURE_TRUE(mWebNav, _T(""));
	nsCOMPtr<nsIURI> currentURI;
	nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
		return _T("");

	// Get the uri string associated with the nsIURI object
	nsEmbedCString uriString;
	rv = currentURI->GetSpec(uriString);
	NS_ENSURE_SUCCESS(rv, _T(""));

	if (unescape) {
		char* temp = strdup(uriString.get());
		uriString.Assign(nsUnescape(temp));
		free(temp);
	}

	return NSUTF8StringToCString(uriString);
}

BOOL CBrowserWrapper::GetCharset(char* aCharset)
{
	NS_ENSURE_TRUE(mWebBrowser, FALSE);

	// Look for the forced charset
	nsresult result;
	nsCOMPtr<nsIDocShell> DocShell = GetDocShell();
	NS_ENSURE_TRUE(DocShell, FALSE);

	nsCOMPtr<nsIContentViewer> contentViewer;
	result = DocShell->GetContentViewer (getter_AddRefs(contentViewer));
	if (NS_FAILED(result) || !contentViewer) 
		return FALSE;

	nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(contentViewer,&result);
	if (NS_FAILED(result) || !mdv) 
		return FALSE;

	nsCString mCharset;
	result = mdv->GetForceCharacterSet(mCharset);

	if (NS_FAILED(result) || mCharset.IsEmpty() )
	{
		
		// If no forced charset look for the document charset
		char *charset;
		DocShell->GetCharset(&charset);
		if (!charset)
		{
			// If no document charset use default
			result = mdv->GetDefaultCharacterSet(mCharset);
		}
		else
		{
			mCharset = charset;
			nsMemory::Free (charset);
		}
	}

	strncpy(aCharset, mCharset.get(), 63);
	aCharset[63] = 0;
	return NS_SUCCEEDED(result);
}

BOOL CBrowserWrapper::ForceCharset(const char *aCharSet)
{
	nsresult result;
	nsCOMPtr<nsIDocShell> DocShell = GetDocShell();
	NS_ENSURE_TRUE(DocShell, FALSE);

	nsCOMPtr<nsIContentViewer> contentViewer;
	result = DocShell->GetContentViewer (getter_AddRefs(contentViewer));
	if (NS_FAILED(result) || !contentViewer) 
		return FALSE;

	nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(contentViewer,&result);
	if (NS_FAILED(result) || !mdv) 
		return FALSE;

	nsCString mCharset;
	mCharset = aCharSet;
	result = mdv->SetForceCharacterSet(mCharset);

	if (NS_SUCCEEDED(result))
		mWebNav->Reload(nsIWebNavigation::LOAD_FLAGS_CHARSET_CHANGE);

	return NS_SUCCEEDED(result);
} 

BOOL CBrowserWrapper::ScrollBy(int32_t dx, int32_t dy)
{
	nsCOMPtr<nsIDOMWindow> dom;
	mWebBrowserFocus->GetFocusedWindow(getter_AddRefs(dom));
	if (!dom) {
		nsCOMPtr<nsIDOMElement> element;
		mWebBrowserFocus->GetFocusedElement(getter_AddRefs(element));
		if (element) {
			nsCOMPtr<nsIDOMNode> node(do_QueryInterface(element));
		}
		mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	}
	NS_ENSURE_TRUE(dom, FALSE);

	return dom->ScrollBy (dx, dy);
}

already_AddRefed<nsIMarkupDocumentViewer> CBrowserWrapper::GetMarkupViewer()
{
	nsresult rv;
	nsCOMPtr<nsIDocShell> docShell = GetDocShell();
	NS_ENSURE_TRUE(docShell, NULL);

	nsCOMPtr<nsIContentViewer> contentViewer;
	rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
	NS_ENSURE_SUCCESS(rv, NULL);
		
	nsCOMPtr<nsIMarkupDocumentViewer> _markupViewer = do_QueryInterface(contentViewer, &rv);
	NS_ENSURE_SUCCESS(rv, NULL);

	return _markupViewer.forget();	
}

BOOL CBrowserWrapper::SetFullZoom(float textzoom)
{
	nsCOMPtr<nsIMarkupDocumentViewer> markupViewer = GetMarkupViewer();
	NS_ENSURE_TRUE(markupViewer, FALSE);
	nsresult rv = markupViewer->SetFullZoom(textzoom);
	return NS_SUCCEEDED(rv);
}

float CBrowserWrapper::GetFullZoom()
{
	nsCOMPtr<nsIMarkupDocumentViewer> markupViewer = GetMarkupViewer();
	NS_ENSURE_TRUE(markupViewer, FALSE);
	float textzoom;
	nsresult rv = markupViewer->GetFullZoom(&textzoom);
	return NS_SUCCEEDED(rv) ? textzoom : -1;
}

BOOL CBrowserWrapper::ChangeFullZoom(int change)
{
	float textzoom = GetFullZoom();
	if (textzoom == -1)
		return FALSE;

	textzoom = (textzoom*10 + (float)change) / 10;
	if (textzoom <0.1 || textzoom > 4)
		return FALSE;

	SetFullZoom(textzoom);

	CString status;
	status.Format(IDS_FULL_ZOOM, textzoom*10);
	mpBrowserImpl->SetStatus(0, CStringToPRUnichar(status));
	return TRUE;
}

BOOL CBrowserWrapper::SetTextSize(float textzoom)
{
   nsresult rv;
   
   nsCOMPtr<nsIDOMWindow> domWindow;
   rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
   NS_ENSURE_SUCCESS(rv, FALSE);

   rv = domWindow->SetTextZoom(textzoom);
   return NS_SUCCEEDED(rv);
}

float CBrowserWrapper::GetTextSize()
{
   nsresult rv;
   
   nsCOMPtr<nsIDOMWindow> domWindow;
   rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
   NS_ENSURE_SUCCESS(rv, -1);

   float textzoom;
   rv = domWindow->GetTextZoom(&textzoom);
   return NS_SUCCEEDED(rv) ? textzoom : -1;
}

BOOL CBrowserWrapper::ChangeTextSize(int change)
{
	float textzoom = GetTextSize();
	if (textzoom == -1)
		return FALSE;

	textzoom = (textzoom*10 + (float)change) / 10;
	if (textzoom <=0 || textzoom > 4)
		return FALSE;

	SetTextSize(textzoom);

	CString status;
	status.Format(IDS_TEXT_ZOOM, textzoom*10);
	mpBrowserImpl->SetStatus(0, CStringToPRUnichar(status));
	return TRUE;
}

BOOL CBrowserWrapper::GetSHistory(nsISHistory **aSHistory)
{
	// XXX: Prevent crash with message send when browser is closing
	NS_ENSURE_TRUE(mWebNav, FALSE);
	nsCOMPtr<nsISHistory> sHistory;
	mWebNav->GetSessionHistory(getter_AddRefs(sHistory));
	NS_ENSURE_TRUE(sHistory, FALSE);

	*aSHistory = sHistory.get();
	NS_IF_ADDREF(*aSHistory);

	return TRUE;
}

BOOL CBrowserWrapper::CloneSHistory(CBrowserWrapper* newWebBrowser)
{
	nsCOMPtr<nsISHistory> oldSH;	
	GetSHistory(getter_AddRefs(oldSH));
	NS_ENSURE_TRUE(oldSH, FALSE);

	PRInt32 count;
	oldSH->GetCount(&count);
	if (!count) {
		newWebBrowser->LoadURL(_T("about:blank"));
		return TRUE;
	}

	nsCOMPtr<nsISHistory> newSH;
	newWebBrowser->GetSHistory(getter_AddRefs(newSH));
	NS_ENSURE_TRUE(newSH, FALSE);
	
	nsCOMPtr<nsISHistoryInternal> newSHInternal(do_QueryInterface(newSH));
	NS_ENSURE_TRUE(newSHInternal, FALSE);

	nsCOMPtr<nsIHistoryEntry> she;

	for (int i=0;i<count;i++)
	{
		oldSH->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs(she));
		if (she) {
			nsCOMPtr<nsISHEntry> newSHEntry;
			nsCOMPtr<nsISHEntry> shee(do_QueryInterface(she));
			shee->Clone(getter_AddRefs(newSHEntry));
			if (newSHEntry) newSHInternal->AddEntry(newSHEntry, PR_TRUE);
		}
	}

	PRInt32 index;
	oldSH->GetIndex(&index);
	newWebBrowser->GotoHistoryIndex(index);


	return TRUE;
}

BOOL CBrowserWrapper::GotoHistoryIndex(UINT index)
{
	NS_ENSURE_TRUE(mWebNav, FALSE);
	return mWebNav->GotoIndex (index);
}

BOOL CBrowserWrapper::GetSHistoryState(int& index, int& count)
{
	NS_ENSURE_TRUE (mWebBrowser, FALSE);

	nsCOMPtr<nsISHistory> sHistory;
	mWebNav->GetSessionHistory(getter_AddRefs(sHistory));
	NS_ENSURE_TRUE(sHistory, FALSE);

	sHistory->GetCount(&count);
	sHistory->GetIndex(&index);	

	return TRUE;
}

BOOL CBrowserWrapper::GetSHistoryInfoAt(PRInt32 index, CString& title, CString& url)
{
	NS_ENSURE_TRUE(mWebBrowser, FALSE);

	nsCOMPtr<nsISHistory> sHistory;
	mWebNav->GetSessionHistory(getter_AddRefs(sHistory));
	NS_ENSURE_TRUE(sHistory, FALSE);

	nsCOMPtr<nsIHistoryEntry> he;
	sHistory->GetEntryAtIndex(index, PR_FALSE, getter_AddRefs(he));
	if (!he) return false;

	nsresult rv;
	nsEmbedCString nsUrl;
	PRUnichar* nsTitle;
	
	rv = he->GetTitle(&nsTitle);
	NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && title, FALSE);

	title = PRUnicharToCString(nsTitle);
	nsMemory::Free(nsTitle);
	
	nsCOMPtr<nsIURI> uri;
	he->GetURI(getter_AddRefs(uri));
	NS_ENSURE_TRUE(uri, FALSE);
	
	rv = uri->GetSpec(nsUrl);
	url = NSUTF8StringToCString(nsUrl);

	NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && url.GetLength(), FALSE);

	return TRUE;
}

void CBrowserWrapper::PrintSetup()
{
	if (!mPrintSettings) InitPrintSettings();

	CPrintSetupDialog dlg(mWndOwner);

	dlg.SetPrintSettings(mPrintSettings);
	if (dlg.DoModal() != IDOK) return;
	dlg.GetPrintSettings(mPrintSettings);

	nsCOMPtr<nsIPrintSettingsService> psService =
	do_GetService("@mozilla.org/gfx/printsettings-service;1");
	NS_ENSURE_TRUE(psService, );

	psService->SavePrintSettingsToPrefs(mPrintSettings, PR_FALSE, nsIPrintSettings::kInitSaveAll);

	nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	NS_ENSURE_TRUE(print, );

	bool isPreview = PR_FALSE;
	nsresult rv = print->GetDoingPrintPreview(&isPreview);
	NS_ENSURE_SUCCESS(rv, );
	
	if (isPreview) 
	{
		if (NS_SUCCEEDED(print->PrintPreview(mPrintSettings, nullptr, nullptr)))
		{
			// WORKAROUND - FIX ME: Why the print preview doesn't use all the width?
			// So I'm forcing the window to reposition itself.
			CRect rect;
			mWndOwner->GetClientRect(rect);
			mBaseWindow->SetPositionAndSize(0, 0, rect.right, rect.bottom, PR_TRUE);
		}
	}

}

BOOL CBrowserWrapper::Print()
{
	NS_ENSURE_TRUE(mWebBrowser, FALSE);

	nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	NS_ENSURE_TRUE(print, FALSE);

	if (!mPrintSettings) InitPrintSettings();
	
	mpBrowserImpl->mIsPrinting = true;
	CPrintProgressDialog dlg(mWndOwner);
	nsresult rv = print->Print(mPrintSettings, static_cast<nsIWebProgressListener*>(dlg.m_PrintListener.get()));
	NS_ENSURE_SUCCESS(rv, FALSE);
	mpBrowserImpl->mIsPrinting = false;

	if (dlg.DoModal() != IDOK) {
		print->Cancel();
		return FALSE;
	}
	return TRUE;
}

BOOL CBrowserWrapper::InitPrintSettings()
{
	nsCOMPtr<nsIPrintSettingsService> psService =
		do_GetService("@mozilla.org/gfx/printsettings-service;1");
	NS_ENSURE_TRUE(psService, FALSE);

	psService->GetGlobalPrintSettings(getter_AddRefs(mPrintSettings));
	NS_ENSURE_TRUE(mPrintSettings, FALSE);

	nsresult rv = psService->InitPrintSettingsFromPrefs(mPrintSettings, PR_FALSE, nsIPrintSettings::kInitSaveAll);
	return NS_SUCCEEDED(rv);
}
//#include "BrowserFrm.h"
//#include "BrowserView.h"
BOOL CBrowserWrapper::PrintPreview()
{
	nsCOMPtr<nsIWebBrowserPrint> print;
	nsIDocShell* shell = GetDocShell();	
	shell->GetPrintPreview(getter_AddRefs(print));
	if (!print) return FALSE;

	if (!mPrintSettings) InitPrintSettings();

	bool isPreview = false;
	nsresult rv = print->GetDoingPrintPreview(&isPreview);
	NS_ENSURE_SUCCESS(rv, FALSE);

	if (isPreview) 
		rv = print->ExitPrintPreview();
	else {
		nsCOMPtr<nsIDOMWindow> dom;	
		rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		NS_ENSURE_SUCCESS(rv, FALSE);

	/*	CBrowserFrame* frm = theApp.CreateNewBrowserFrameWithUrl(L"about:blank");//nsIWebBrowserChrome::CHROME_OPENAS_CHROME | nsIWebBrowserChrome::CHROME_DEFAULT);
		shell = frm->GetActiveView()->GetBrowserWrapper()->GetDocShell();
		shell->GetPrintPreview(getter_AddRefs(print));
		if (!print) return FALSE;
		frm->ShowWindow(SW_SHOW);
				
		uint32_t flags;
		mpBrowserImpl->GetChromeFlags(&flags);
		this->CreateBrowser(mWndOwner, flags);*/
		
		rv = print->PrintPreview(mPrintSettings, dom, nullptr);

		// WORKAROUND - FIX ME: Why the print preview doesn't use all the width?
		// So I'm forcing the window to reposition itself.
		//CRect rect;
		//mWndOwner->GetClientRect(rect);
		//mBaseWindow->SetPositionAndSize(0, 0, rect.right, rect.bottom, PR_TRUE);
	}

	return NS_SUCCEEDED(rv);
}

BOOL CBrowserWrapper::_InjectCSS(nsIDOMWindow* dom, const wchar_t* userStyleSheet)
{
	if (!dom) return FALSE;
	
	nsresult rv;

	nsCOMPtr<nsIDOMDocument> document;
	rv = dom->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMElement> styleElement;
	rv = document->CreateElement(nsDependentString(L"style"), getter_AddRefs(styleElement));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMText> textStyle;
	rv = document->CreateTextNode(nsDependentString(userStyleSheet), getter_AddRefs(textStyle));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMNode> notused;
	rv = styleElement->AppendChild(textStyle, getter_AddRefs(notused));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMNodeList> headList;
	rv = document->GetElementsByTagName(nsDependentString(L"head"), getter_AddRefs(headList));
	if (headList)
	{
		nsCOMPtr<nsIDOMNode> headNode;
		rv = headList->Item(0, getter_AddRefs(headNode));
		NS_ENSURE_TRUE(headNode, FALSE);

		rv = headNode->AppendChild(styleElement, getter_AddRefs(notused));
	}
	else
	{
		nsCOMPtr<nsIDOMElement> documentElement;
		document->GetDocumentElement(getter_AddRefs(documentElement));
		rv = documentElement->AppendChild(styleElement, getter_AddRefs(notused));
	}

	BOOL b = TRUE;
	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		frames->GetLength(&nbframes);
		if (nbframes>0)
		{
			nsCOMPtr<nsIDOMWindow> frame;
			for (PRUint32 i = 0; i<nbframes; i++)
			{
				rv = frames->Item(i, getter_AddRefs(frame));
				if (NS_FAILED(rv)) return FALSE;
				b = b && _InjectCSS(frame, userStyleSheet);
			}
		}
	}

	return b && NS_SUCCEEDED(rv);
}

BOOL CBrowserWrapper::InjectCSS(const wchar_t* userStyleSheet)
{
	nsresult rv;
	nsCOMPtr<nsIDOMWindow> dom;
	
	rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	// Use a recursive function to inject the CSS in all frames.
	return _InjectCSS(dom, userStyleSheet);
}

BOOL CBrowserWrapper::InjectJS(const wchar_t* userScript, CString& result, bool bTopWindow)
{
	/*nsresult rv;
	nsCOMPtr<nsIDOMDocument> document;

	if (!bTopWindow)
	{
		if (mLastMouseActionNode)
			rv = mLastMouseActionNode->GetOwnerDocument(getter_AddRefs(document));
		else
		{
			nsCOMPtr<nsIDOMWindow> dom;
			rv = mWebBrowserFocus->GetFocusedWindow(getter_AddRefs(dom));
			if (dom) rv = dom->GetDocument(getter_AddRefs(document));
		}
		
	}

	if (!document)
	{
		nsCOMPtr<nsIDOMWindow> dom;
		rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = dom->GetDocument(getter_AddRefs(document));
	}

	NS_ENSURE_SUCCESS(rv, FALSE);*/
	
	PRBool jsEnabled = PR_TRUE;
	jsEnabled = theApp.preferences.GetBool("javascript.enabled", jsEnabled);
	theApp.preferences.SetBool("javascript.enabled", true);
	
	nsCOMPtr<nsIScriptGlobalObject> sgo = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIScriptContext> ctx = sgo->GetContext();

	JSContext* cx = ctx->GetNativeContext();
	//mozilla::AutoPushJSContext cx(ctx->GetNativeContext());
	JS::Rooted<JSObject*> global(cx, sgo->GetGlobalJSObject());

	JS::Rooted<JS::Value> retval(cx, JS::UndefinedValue());
	JS::CompileOptions options(cx);	
	//options.setFileAndLine("kmeleon",0).setVersion(JSVERSION_DEFAULT);

    nsCOMPtr<nsIScriptObjectPrincipal> sgoPrincipal = do_QueryInterface(sgo);
	ctx->EvaluateString(nsEmbedString(userScript), global,
						options, false, nullptr/*retval.address()*/);
	
	//result = NSStringToCString(nsString(JS_GetStringCharsZ(ctx->GetNativeContext(), retval.toString())));
	theApp.preferences.SetBool("javascript.enabled", jsEnabled);

	return TRUE;
}

BOOL CBrowserWrapper::_GetSelection(nsIDOMWindow* dom, nsAString &aSelText)
{
	nsCOMPtr<nsISelection> sel;
	dom->GetSelection(getter_AddRefs(sel));
	if (sel) {
		nsString selText;
		nsresult rv = sel->ToString(selText);
		NS_ENSURE_SUCCESS(rv, FALSE);

		aSelText = selText;
		
		if (aSelText.Length()>0)
			return TRUE;
	}

	BOOL ret = FALSE;

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	NS_ENSURE_TRUE(frames, FALSE);

	PRUint32 nbframes;
	frames->GetLength(&nbframes);
	if (nbframes>0)
	{
		nsCOMPtr<nsIDOMWindow> frame;
		for (PRUint32 i = 0; i<nbframes; i++)
		{
			frames->Item(i, getter_AddRefs(frame));
			ret = ret || _GetSelection(frame, aSelText);
		}
	}
	return ret;
}

BOOL CBrowserWrapper::GetSelectionInsideForm(nsIDOMElement *element, nsEmbedString &aSelText)
{
	nsCOMPtr<nsIDOMHTMLInputElement> domnsinput = do_QueryInterface(element);
	if (domnsinput)
	{
		PRInt32 start, end;
		nsresult rv;
		rv = domnsinput->GetSelectionStart(&start);
		NS_ENSURE_SUCCESS(rv, FALSE);
		rv = domnsinput->GetSelectionEnd(&end);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (start >= end) return FALSE;

		nsCOMPtr<nsIDOMHTMLInputElement> dominput = do_QueryInterface(element);
		if (!dominput) return FALSE;

		nsEmbedString value;
		dominput->GetValue(value);
		value.Cut(end,-1);
		if (start>value.Length())
			return FALSE;

		aSelText = value.get()+start;
		return TRUE;
	}

	nsCOMPtr<nsIDOMHTMLTextAreaElement> tansinput = do_QueryInterface(element);
	if (tansinput)
	{
		PRInt32 start, end;
		nsresult rv;
		rv = tansinput->GetSelectionStart(&start);
		NS_ENSURE_SUCCESS(rv, FALSE);
		rv = tansinput->GetSelectionEnd(&end);
		NS_ENSURE_SUCCESS(rv, FALSE);
		if (start >= end) return FALSE;

		nsCOMPtr<nsIDOMHTMLTextAreaElement> tainput = do_QueryInterface(element);
		if (!tansinput) return FALSE;

		nsEmbedString value;
		tainput->GetValue(value);
		value.Cut(end,-1);
		if (start>value.Length())
			return FALSE;

		aSelText = value.get()+start;
		return TRUE;
	}

	return FALSE;
}

BOOL CBrowserWrapper::GetUSelection(nsEmbedString& aSelText)
{
	nsCOMPtr<nsIDOMWindow> dom(do_GetInterface(mWebBrowser));
	NS_ENSURE_TRUE(dom, FALSE);

	// Check selection inside the focused element if it's an 
	// input text or a textarea element.
	nsCOMPtr<nsIDOMElement> element;
	mWebBrowserFocus->GetFocusedElement(getter_AddRefs(element));
	if (element)
	{
		if (GetSelectionInsideForm(element, aSelText))
			return TRUE;
	}

	// Check normal selection inside the document
	if (_GetSelection(dom, aSelText))
		return TRUE;

	return FALSE;
}

BOOL CBrowserWrapper::GetSelection(CString& aSelText)
{
	nsEmbedString selText;
	if (!GetUSelection(selText))
		return FALSE;

	aSelText = NSStringToCString(selText);
	return TRUE;
}

//#include "nsIWidgetListener.h"
void CBrowserWrapper::SetActive(BOOL aActive)
{	
	/*nsCOMPtr<nsIDOMWindow> dom;
	nsresult rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_SUCCESS(rv, );
	mWebBrowser->SetIsActive(aActive);
	nsCOMPtr<nsIFocusManager> fm = do_GetService("@mozilla.org/focus-manager;1");
	if (!fm) return;
	if (aActive) fm->WindowShown(dom, true); else fm->WindowHidden(dom);*/


	NS_ENSURE_TRUE(mWebBrowserFocus, );
	TRACE2("Set Active Browser %u for window %s\n", aActive, (LPCTSTR)GetTitle());
	if (aActive) mWebBrowserFocus->Activate(); else mWebBrowserFocus->Deactivate();
}

BOOL CBrowserWrapper::GetCertificate(nsIX509Cert** certificate)
{
	nsresult rv;
	
	nsCOMPtr<nsIDocShell> docShell = GetDocShell();
	NS_ENSURE_TRUE(docShell, FALSE);
		
	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	rv = docShell->GetSecurityUI(getter_AddRefs(securityInfo));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsISSLStatusProvider> SSLProvider = do_QueryInterface(securityInfo,&rv);
	if (!SSLProvider) return FALSE;

	nsCOMPtr<nsISSLStatus> SSLStatus;
	SSLProvider->GetSSLStatus(getter_AddRefs(SSLStatus));
	if (!SSLStatus) return FALSE;

	nsCOMPtr<nsIX509Cert> cert;
	SSLStatus->GetServerCert(getter_AddRefs (cert));
	if (!cert) return FALSE;
	
	*certificate = cert;
	NS_ADDREF(*certificate);
	
	return TRUE;
}

BOOL CBrowserWrapper::GetSecurityInfo(CString &sign)
{
	nsresult rv;

	nsCOMPtr<nsIDocShell> docShell = GetDocShell();
	NS_ENSURE_TRUE(docShell, FALSE);
		
	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	rv = docShell->GetSecurityUI (getter_AddRefs (securityInfo));
	NS_ENSURE_SUCCESS(rv, FALSE);

	/*nsEmbedString tooltip;
	rv = securityInfo->GetTooltipText (tooltip);
	NS_ENSURE_SUCCESS(rv, FALSE);

	sign = NSStringToCString(tooltip);*/
    
	return TRUE;
}

int CBrowserWrapper::GetSecurityState()
{
	nsCOMPtr<nsIDocShell> docShell = GetDocShell();
	NS_ENSURE_TRUE(docShell, nsIWebProgressListener::STATE_IS_INSECURE);

	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	docShell->GetSecurityUI(getter_AddRefs(securityInfo));
	NS_ENSURE_TRUE(securityInfo, nsIWebProgressListener::STATE_IS_INSECURE);

	PRUint32 s;
	securityInfo->GetState(&s);
	return s & 0xffff;
}

BOOL CBrowserWrapper::ShowCertificate()
{
	nsresult rv;
	nsCOMPtr<nsIDocShell> docShell = GetDocShell();
	NS_ENSURE_TRUE(docShell, FALSE);

	nsCOMPtr<nsIX509Cert> cert;
	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	rv = docShell->GetSecurityUI (getter_AddRefs (securityInfo));
	if (NS_FAILED(rv) || !(GetCertificate(getter_AddRefs(cert))))
		return FALSE;
	
	nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID, &rv);
	NS_ENSURE_TRUE(certDialogs, FALSE);

	rv = certDialogs->ViewCert(NULL, cert);
	return NS_SUCCEEDED(rv);
}

BOOL CBrowserWrapper::ViewContentContainsFrames()
{
    nsresult rv = NS_OK;

    // Get nsIDOMDocument from nsIWebNavigation
    nsCOMPtr<nsIDOMDocument> domDoc;
    rv = mWebNav->GetDocument(getter_AddRefs(domDoc));
    if(NS_FAILED(rv))
       return FALSE;

    // QI nsIDOMDocument for nsIDOMHTMLDocument
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
    if (!htmlDoc)
        return FALSE;
   
    // Get the <body> element of the doc
    nsCOMPtr<nsIDOMHTMLElement> body;
    rv = htmlDoc->GetBody(getter_AddRefs(body));
    if(NS_FAILED(rv))
       return FALSE;

    // Is it of type nsIDOMHTMLFrameSetElement?
    nsCOMPtr<nsIDOMHTMLFrameSetElement> frameset = do_QueryInterface(body);

    return (frameset != nullptr);
}


bool CBrowserWrapper::CheckNode(nsIDOMElement* elem)
{
	if (elem) 
	{
		nsEmbedString className;
		elem->GetAttribute(nsEmbedString(kClass), className);
		if (className.Equals(nsEmbedString(kHighlighClassName)))
			return PR_TRUE;
	}
	return PR_FALSE;
}

BOOL CBrowserWrapper::Highlight(const PRUnichar* backcolor, const PRUnichar* word, BOOL matchCase)
{
	nsCOMPtr<nsIDOMWindow> dom;
	nsresult rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_SUCCESS(rv, FALSE);

	return _Highlight(dom, backcolor, word, matchCase);
}

BOOL CBrowserWrapper::_Highlight(nsIDOMWindow* dom, const PRUnichar* backcolor, const PRUnichar* word, BOOL matchCase)
{
	nsCOMPtr<nsIDOMDocument> document;
	nsresult rv = dom->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		nsCOMPtr<nsIDOMWindow> frame;
		frames->GetLength(&nbframes);
		for (PRUint32 i = 0; i<nbframes; i++)
		{
			frames->Item(i, getter_AddRefs(frame));
			_Highlight(frame, backcolor, word, matchCase);
		}
	}

	nsCOMPtr<nsIFind> find = do_CreateInstance("@mozilla.org/embedcomp/rangefind;1", &rv);
	if (NS_FAILED(rv)) return FALSE;

	find->SetCaseSensitive(matchCase);
    find->SetFindBackwards(PR_FALSE);

	nsCOMPtr<nsIDOMRange> searchRange, startPt, endPt;
    document->CreateRange(getter_AddRefs(searchRange));
	document->CreateRange(getter_AddRefs(startPt));
	document->CreateRange(getter_AddRefs(endPt));

	nsCOMPtr<nsIDOMHTMLElement> body;
	nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(document));
	NS_ENSURE_TRUE(htmlDoc, FALSE);
	
	htmlDoc->GetBody(getter_AddRefs(body));
	NS_ENSURE_TRUE(body, FALSE);
	
	nsCOMPtr<nsIDOMNodeList> nodelist;
	rv = body->GetChildNodes(getter_AddRefs(nodelist));
	NS_ENSURE_SUCCESS(rv, FALSE);

	PRUint32 count;
	nodelist->GetLength(&count);

	searchRange->SetStart(body, 0);
	searchRange->SetEnd(body, count);
	startPt->SetStart(body, 0);
	startPt->SetEnd(body, 0);
	endPt->SetStart(body, count);
	endPt->SetEnd(body, count);

	if(!backcolor)
	{
		// To optimize

		while (1)
		{
			nsCOMPtr<nsIDOMRange> retRange;
			rv = find->Find(word, searchRange, startPt, endPt, getter_AddRefs(retRange));
			if (NS_FAILED(rv) || !retRange)	break;
			
			nsCOMPtr<nsIDOMNode> startContainer;
			retRange->GetStartContainer(getter_AddRefs(startContainer));
			if (!startContainer) break;

			nsCOMPtr<nsIDOMNode> node;
			rv = startContainer->GetParentNode(getter_AddRefs(node));
			nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(node);
			
			bool bFound = CheckNode(elem);
			
			if (!bFound && elem)
			{ 
				nsCOMPtr<nsIDOMNode> node;
				retRange->GetEndContainer(getter_AddRefs(node));
				if (!node) break;
				
				while (1)
				{
					rv = elem->GetParentNode(getter_AddRefs(node));
					elem = do_QueryInterface(node);
					if (NS_FAILED(rv) || !elem) break;
					
					if (CheckNode(elem)) {
						bFound = PR_TRUE;
						break;
					}
				}
			}

			if (bFound)
			{
				nsCOMPtr<nsIDOMDocumentFragment> fragment;
				document->CreateDocumentFragment(getter_AddRefs(fragment));
				nsCOMPtr<nsIDOMNode> next, parent, child, notused;
				rv = elem->GetNextSibling(getter_AddRefs(next));
				if (NS_FAILED(rv) || !retRange)	break;
				rv = elem->GetParentNode(getter_AddRefs(parent));
				if (NS_FAILED(rv) || !retRange)	break;
				while(1)
				{
					elem->GetFirstChild(getter_AddRefs(child));
					if (NS_FAILED(rv) || !child) break;
					fragment->AppendChild(child, getter_AddRefs(notused));
				}
				document->CreateRange(getter_AddRefs(startPt));
				startPt->SetStartAfter(elem);
				parent->RemoveChild(elem, getter_AddRefs(notused));
				parent->InsertBefore(fragment, next, getter_AddRefs(notused));
				parent->Normalize();
			}
            else
			{
				nsCOMPtr<nsIDOMNode> ec;
				retRange->GetEndContainer(getter_AddRefs(ec));
				if (!ec) break;

				PRInt32 startOffset, endOffset;
				retRange->GetStartOffset(&startOffset);
				retRange->GetEndOffset(&endOffset);

				document->CreateRange(getter_AddRefs(startPt));
				startPt->SetStart(ec, endOffset);	
			}
			startPt->Collapse(PR_TRUE);
		}
		return TRUE;
	}

	nsCOMPtr<nsIDOMElement> baseElement;
	rv = document->CreateElement(nsEmbedString(kSpan), getter_AddRefs(baseElement));
	NS_ENSURE_SUCCESS(rv, FALSE);

	baseElement->SetAttribute(nsEmbedString(kStyle), nsEmbedString(kDefaultHighlightStyle));
	baseElement->SetAttribute(nsEmbedString(kClass), nsEmbedString(kHighlighClassName));

	while (1)
	{
		nsCOMPtr<nsIDOMRange> retRange;
		rv = find->Find(word, searchRange, startPt, endPt, getter_AddRefs(retRange));
		if (NS_FAILED(rv) || !retRange)	break;

		nsCOMPtr<nsIDOMNode> tNode;
		baseElement->CloneNode(false, 0, getter_AddRefs(tNode));
		nsCOMPtr<nsIDOMElement> hNode = do_QueryInterface(tNode);
		if (!hNode) break;

		nsCOMPtr<nsIDOMNode> sc;
		retRange->GetStartContainer(getter_AddRefs(sc));

		nsCOMPtr<nsIDOMNode> ec;
		retRange->GetEndContainer(getter_AddRefs(ec));

        PRInt32 startOffset, endOffset;
		retRange->GetStartOffset(&startOffset);
		retRange->GetEndOffset(&endOffset);

		nsCOMPtr<nsIDOMDocumentFragment> fragment;
		rv = retRange->ExtractContents(getter_AddRefs(fragment));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNode> fchild,lchild;
		fragment->GetFirstChild(getter_AddRefs(fchild));
		fragment->GetLastChild(getter_AddRefs(lchild));
		if (!fchild || !lchild) break;

		PRUint16 ftype, ltype;
		fchild->GetNodeType(&ftype);
		lchild->GetNodeType(&ltype);
		
		nsCOMPtr<nsIDOMNode> before;
		if (ftype==nsIDOMNode::ELEMENT_NODE && ltype==nsIDOMNode::ELEMENT_NODE)
		{
			rv = ec->GetParentNode(getter_AddRefs(before));
			if (NS_FAILED(rv)||!before) break;
		}
		else 
		{
			nsCOMPtr<nsIDOMText> container = do_QueryInterface(sc);
			PRInt32 offset = startOffset;
			if (ftype==nsIDOMNode::ELEMENT_NODE)
			{
				container = do_QueryInterface(ec);
				offset = 0;
			}

			if (!container) break;

			nsCOMPtr<nsIDOMText> beforeText;
			rv = container->SplitText(offset, getter_AddRefs(beforeText));
			before = do_QueryInterface(beforeText);
			if (NS_FAILED(rv)||!before) break;
		}
			
		nsCOMPtr<nsIDOMNode> parent;
		rv = before->GetParentNode(getter_AddRefs(parent));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNode> notused;
		rv = hNode->AppendChild(fragment, getter_AddRefs(notused));
		if (NS_FAILED(rv)) break;

		rv = parent->InsertBefore(hNode, before, getter_AddRefs(notused));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMDocument> docowner;
		rv = hNode->GetOwnerDocument(getter_AddRefs(docowner));
		if (NS_FAILED(rv)) break;

		rv = document->CreateRange(getter_AddRefs(startPt));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNodeList> nodelist;
		rv = hNode->GetChildNodes(getter_AddRefs(nodelist));
		if (NS_FAILED(rv) || !nodelist) break;

		PRUint32 count;
		nodelist->GetLength(&count);

		startPt->SetStart(hNode, count);
		startPt->SetEnd(hNode, count);
	}

	return TRUE;
}

already_AddRefed<nsISupports> CBrowserWrapper::GetPageDescriptor(BOOL focus)
{
	nsCOMPtr<nsIDocShell> docShell;
	if (!focus) {
		docShell = GetDocShell();
		if (!docShell) return NULL;
	} else {
		nsCOMPtr<nsIDOMWindow> domWindow;
		mWebBrowserFocus->GetFocusedWindow(getter_AddRefs(domWindow));

		if (!domWindow)
			mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
		
		if (!domWindow)	return NULL;

		nsCOMPtr<nsPIDOMWindow> privWin(do_QueryInterface(domWindow));
		if (!domWindow)	return NULL;
		docShell = privWin->GetDocShell();
	}

	nsCOMPtr<nsIWebPageDescriptor> wpd = do_QueryInterface(docShell);
	if (!wpd) return NULL;

	nsCOMPtr<nsISupports> descriptor;
	wpd->GetCurrentDescriptor(getter_AddRefs(descriptor));
	return descriptor.forget();
}

BOOL CBrowserWrapper::CanSave()
{
	nsEmbedCString contentType;
	nsCOMPtr<nsIDOMDocument> document;
	nsCOMPtr<nsIDOMWindow> dom;
	
	nsresult rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_TRUE(dom, FALSE);

	rv = dom->GetDocument(getter_AddRefs(document));

	nsEmbedString type;
	rv = document->GetContentType(type);
	NS_UTF16ToCString(type, NS_CSTRING_ENCODING_ASCII, contentType);

	const char* ctype = contentType.get();
	PRBool isHTML = 
	(strcmp(ctype, "text/html") == 0) ||
	(strcmp(ctype, "text/xml") == 0) ||
	(strcmp(ctype, "application/xhtml+xml") == 0) ;

	return (!IsBusy() || mIsDOMLoaded || !isHTML);
}

BOOL CBrowserWrapper::SaveDocument(BOOL frame, LPCTSTR filename)
{
	nsCOMPtr<nsIDOMWindow> dom;
	if (frame) mWebBrowserFocus->GetFocusedWindow(getter_AddRefs(dom));
	if (!dom) mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_TRUE(dom, FALSE);

	nsCOMPtr<nsIDOMDocument> document;
    nsresult rv = dom->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIURI> nsURI;
	nsCOMPtr<nsISupports> cacheDescriptor;
	if (!frame) 
	{
		rv = mWebNav->GetCurrentURI(getter_AddRefs(nsURI));
		NS_ENSURE_SUCCESS(rv, FALSE);
		
		nsCOMPtr<nsIDocShell> docShell = GetDocShell();
		NS_ENSURE_TRUE(docShell, FALSE);

		nsCOMPtr<nsIWebPageDescriptor> descriptor;
		descriptor = do_QueryInterface(docShell);
		if (descriptor)
			descriptor->GetCurrentDescriptor(getter_AddRefs(cacheDescriptor));
	}
	else
	{
        nsCOMPtr<nsIDOMLocation> location;
		document->GetLocation(getter_AddRefs(location));
		NS_ENSURE_TRUE(location, FALSE);

		nsEmbedString nsURL;
		location->GetHref(nsURL);
  		nsresult rv = NewURI(getter_AddRefs(nsURI), nsURL);
		NS_ENSURE_SUCCESS(rv, FALSE);
	}

	nsCOMPtr<nsIURI> referrer;
	mWebNav->GetReferringURI(getter_AddRefs(referrer));

	return _Save(nsURI, document, filename, referrer, cacheDescriptor);
}

BOOL CBrowserWrapper::SaveURL(LPCTSTR url, LPCTSTR filename)
{
	ASSERT(url);
	NS_ENSURE_TRUE(url, FALSE);

	nsCOMPtr<nsIURI> nsURI;
#ifdef _UNICODE
	nsresult rv = NewURI(getter_AddRefs(nsURI), nsEmbedString((WCHAR*)url));
#else
	nsresult rv = NewURI(getter_AddRefs(nsURI), nsEmbedCString((char*)url));
#endif
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIURI> referrer;
	mWebNav->GetCurrentURI(getter_AddRefs(referrer));

	return _Save(nsURI, nullptr, filename, referrer, nullptr);
}

BOOL CBrowserWrapper::_Save(nsIURI* aURI, 
						   nsIDOMDocument* aDocument, 
						   LPCTSTR filename,
						   nsIURI* aReferrer,
						   nsISupports* aDescriptor)
{
	// NEED better error handling 
	nsresult rv;

	nsEmbedCString contentType;
	BOOL isHTML = FALSE;

	if (aDocument) 
	{
		nsEmbedString type;
		rv = aDocument->GetContentType(type);
		NS_UTF16ToCString(type, NS_CSTRING_ENCODING_ASCII, contentType);

		const char* ctype = contentType.get();
		isHTML = 
		  (strcmp(ctype, "text/html") == 0) ||
		  (strcmp(ctype, "text/xml") == 0) ||
		  (strcmp(ctype, "application/xhtml+xml") == 0) ;
	}

	nsCOMPtr<nsIWebBrowserPersist> persist = do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID);
	if (!persist) return FALSE;

	CSaveAsHandler* handler = new CSaveAsHandler(persist, aURI, aDocument, aDescriptor, aReferrer);
	
	if (filename)
	{
		// We have a filename. No need to ask the user for one.
		rv = handler->DownloadTo(CStringToNSString(filename), FALSE);
	}
	else if (isHTML || 
		theApp.preferences.GetBool("kmeleon.download.disableContentSniffingOnSave", false)) 
	{
		// No need to check the content type for an html document.
		// The user may want to skip this step too.
		rv = handler->Save(contentType.get());
		delete handler;
	}
	else
	{
		// Initiate a download only to get the content type.
		// XXX Need to merge this with the standard download so that we can download
		// in the background and ask for the location as soon as we have the content type.

		TCHAR tempFile[MAX_PATH];
		::GetTempPath(MAX_PATH, tempFile);
		::GetTempFileName(tempFile, _T("kme"), 0, tempFile); 

		nsCOMPtr<nsIFile> file;
#ifdef _UNICODE
		NS_NewLocalFile(nsDependentString(tempFile), TRUE, getter_AddRefs(file));
#else
		NS_NewNativeLocalFile(nsDependentCString(tempFile), TRUE, getter_AddRefs(file));
#endif

		handler->SetTempFile(file);
		persist->SetProgressListener(handler);
		rv = persist->SaveURI(aURI, aDescriptor, aReferrer, nullptr, nullptr, file, nullptr);
		if (NS_FAILED(rv))
			persist->SetProgressListener(nullptr);
	}
	
	// The user may have cancelled the download, in that case
	// don't show an error.
	return (NS_SUCCEEDED(rv) || rv == NS_ERROR_ABORT);
}

#include "nsIDOMHTMLCollection.h"

BOOL CBrowserWrapper::InputHasFocus(bool typeAhead)
{
	nsCOMPtr<nsIDOMElement> element;
	mWebBrowserFocus->GetFocusedElement(getter_AddRefs(element));
	if (IsInputOrObject(element))
		return TRUE;

	if (typeAhead && mTypeAhead) {
		nsString str;
		mTypeAhead->GetSearchString(str);
		if (str.Length()>0) return TRUE;
	}


	nsCOMPtr<nsIDOMWindow> domWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	nsCOMPtr<nsPIDOMWindow> piWindow = do_QueryInterface(domWindow);
	if (!piWindow) return FALSE;

	nsCOMPtr<nsIFocusManager> fm = do_GetService("@mozilla.org/focus-manager;1");
	if (!fm) return FALSE;
	nsCOMPtr<nsIDOMWindow> focusedWindow;
	fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
	if (!focusedWindow) return FALSE;

	nsCOMPtr<nsIDOMDocument> domDoc;
	focusedWindow->GetDocument(getter_AddRefs(domDoc));
	nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(domDoc));
	if (!htmlDoc) return FALSE;

	nsEmbedString designMode;
	htmlDoc->GetDesignMode(designMode);
	if (designMode.Equals(NS_LITERAL_STRING("on")))
		return TRUE;

	nsCOMPtr<nsIDOMHTMLCollection> embeds;
	htmlDoc->GetEmbeds(getter_AddRefs(embeds));
	if (!embeds) return FALSE;

	PRUint32 length;
	embeds->GetLength(&length);
	if (length > 0) return TRUE;
	
	return FALSE;
}

BOOL CBrowserWrapper::IsClickable(nsIDOMElement* element)
{
	nsCOMPtr<nsIDOMHTMLInputElement> domnsinput = do_QueryInterface(element);
	if (domnsinput) return TRUE;

	nsCOMPtr<nsIDOMHTMLEmbedElement> embed = do_QueryInterface(element);
	if (embed) return TRUE;
	
	nsCOMPtr<nsIDOMHTMLObjectElement> object = do_QueryInterface(element);
	if (object) return TRUE;

	nsCOMPtr<nsIDOMHTMLButtonElement> button = do_QueryInterface(element);
	if (button) return TRUE;

	return FALSE;
}

BOOL CBrowserWrapper::IsInputOrObject(nsIDOMElement* element)
{
	NS_ENSURE_TRUE(element, FALSE);

	nsCOMPtr<nsIDOMHTMLInputElement> domnsinput = do_QueryInterface(element);
	if (domnsinput) return TRUE;
	
	nsCOMPtr<nsIDOMHTMLTextAreaElement> tansinput = do_QueryInterface(element);
	if (tansinput) return TRUE;

	nsCOMPtr<nsIDOMHTMLEmbedElement> embed = do_QueryInterface(element);
	if (embed) return TRUE;
	
	nsCOMPtr<nsIDOMHTMLObjectElement> object = do_QueryInterface(element);
	if (object) return TRUE;

	nsCOMPtr<nsIDOMHTMLSelectElement> select = do_QueryInterface(element);
	if (select) return TRUE;

	nsString attr;
	element->GetAttribute(NS_LITERAL_STRING("contenteditable"), attr);
	if (attr.Compare(NS_LITERAL_STRING("true")) == 0)
		return TRUE;

	return FALSE;			
}

CString CBrowserWrapper::GetDocURL(nsIDOMNode* aNode)
{
	CString frame = GetFrameURL(aNode);
	if (frame.IsEmpty())
		return GetURI();
	return frame;
}

CString CBrowserWrapper::GetFrameURL(nsIDOMNode* aNode)
{
	nsString url;
	if (!::GetFrameURL(mWebBrowser, aNode, url))
		return _T("");
	return NSStringToCString(url);
}

#ifndef FINDBAR_USE_TYPEAHEAD

// This function collapse the current selection in
// the window and in frames. See below for its purpose.
// Have to look if we have frames. It's a little violent
// currently. The observer is also passing the root and 
// not the frame so it's useless.

void CBrowserWrapper::CollapseSelToStartInFrame(nsIDOMWindow* dom)
{
	NS_ENSURE_TRUE(dom, );

	nsCOMPtr<nsISelection> sel;
	dom->GetSelection(getter_AddRefs(sel));
	if (sel) sel->CollapseToStart();

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		frames->GetLength(&nbframes);
		if (nbframes>0)
		{
			nsCOMPtr<nsIDOMWindow> frame;
			for (PRUint32 i = 0; i<nbframes; i++)
			{
				frames->Item(i, getter_AddRefs(frame));
				CollapseSelToStartInFrame(frame);
			}
		}
	}
}

BOOL CBrowserWrapper::Find(const wchar_t* searchString, 
						   BOOL matchCase,
						   BOOL wrapAround,
						   BOOL backwards,
						   BOOL ahead)
{/*
	if (!mTypeAhead) {
		mTypeAhead = do_GetService("@mozilla.org/typeaheadfind;1");
		NS_ENSURE_TRUE(mTypeAhead, FALSE);
		mTypeAhead->Init(GetDocShell());
	}

	uint16 result;
	mTypeAhead->SetCaseSensitive(matchCase);
	nsString oldSearch;
	mTypeAhead->GetSearchString(oldSearch);
	if (!searchString || wcscmp(oldSearch.get(), searchString)==0)
		mTypeAhead->FindAgain(backwards,false, &result);
	else
		mTypeAhead->Find(nsDependentString(searchString), false, &result);
	return !(result == nsITypeAheadFind::FIND_NOTFOUND);*/

	nsCOMPtr<nsIWebBrowserFind> finder = do_GetInterface(mWebBrowser);
	NS_ENSURE_TRUE(finder, FALSE);

	// Not setting the search parameter when no searchString so that
	// if a typeahead search is active, it will be used.
	if (searchString) {
		finder->SetSearchString(searchString);
		finder->SetWrapFind(wrapAround);
		finder->SetMatchCase(matchCase);
	}
	
	finder->SetFindBackwards(backwards);
	
	// HACK because not use typeahead
	// The problem with the autosearch feature is that 
	// webbrowserfind start to search at the end of the 
	// current selection. But with autosearch it should
	// start at the beginning. So I collapse the selection.
	if (ahead) {
		nsCOMPtr<nsIDOMWindow> dom;
		mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		CollapseSelToStartInFrame(dom);
	}

	bool didFind = false;
	finder->FindNext(&didFind);
	return didFind;
}

#include "nsITypeAheadFind.h"
#include "nsIDOMKeyEvent.h"
#include "nsISelectionController.h"
void CBrowserWrapper::EndTypeAheadFind()
{
	if (!mTypeAhead) return;
	uint16_t result;
	mTypeAhead->SetSelectionModeAndRepaint(nsISelectionController::SELECTION_ON);
	mSearchString.Truncate();
	mTypeAhead = nullptr;
}

bool CBrowserWrapper::TypeAheadFind(nsIDOMKeyEvent* keyEvent)
{
	static bool linkOnly = theApp.preferences.GetBool("accessibility.typeaheadfind.linksonly", true);
	if (!mTypeAhead) {
		mTypeAhead = do_GetService("@mozilla.org/typeaheadfind;1");
		NS_ENSURE_TRUE(mTypeAhead, FALSE);
		mTypeAhead->Init(GetDocShell());
	}

	uint32_t c;
	keyEvent->GetKeyCode(&c);
	nsString str;
	keyEvent->GetKey(str);	
	str.get();
	uint16_t result;
	bool res = true;
	if (c == nsIDOMKeyEvent::DOM_VK_ESCAPE) {
		mSearchString.Truncate();
	}
	else if (c == nsIDOMKeyEvent::DOM_VK_RETURN) {
		mSearchString.Truncate();
		res = false;
	}
	else if (c == nsIDOMKeyEvent::DOM_VK_TAB) {
		if (mSearchString.IsEmpty()) {
			return false;
		}
		
		bool shiftKey;
		keyEvent->GetShiftKey(&shiftKey);
		mTypeAhead->FindAgain(shiftKey, linkOnly, &result);
		return true;
	}
	else if (c == nsIDOMKeyEvent::DOM_VK_BACK_SPACE) {
		mSearchString.Cut(mSearchString.Length()-1,1);
	} else {
		keyEvent->GetCharCode(&c);
		if (c==0) {
			if (mSearchString.IsEmpty()) 
				return false;
			mSearchString.Truncate();
			res = false;
		}
		else {
			if (mSearchString.IsEmpty()) {
				if (c == '/') {
					linkOnly = false;
					return TRUE;
				}
				if (c == '\'') {
					linkOnly = true;
					return TRUE;
				}
			}
		
			wchar_t key[2] = {c, 0};		
			mSearchString.Append(nsDependentString(key));
		}
	}

	
	mpBrowserImpl->SetStatus(0, mSearchString.get());
	mTypeAhead->Find(mSearchString, linkOnly, &result);
	return res;
	//USES_CONVERSION;
	//frame->m_wndFindBar->StartSearch(A2CT(key));
}


nsIDocShell* CBrowserWrapper::GetDocShell()
{
	if (!mWebBrowser) return NULL;
	nsCOMPtr<nsIDOMWindow> domWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWindow));
	if (!piWin) return NULL;
	return piWin->GetDocShell();
}


/*
BOOL CBrowserWrapper::Find(const wchar_t* searchString, BOOL ahead)
{
	nsCOMPTtr<nsIWebBrowserFind> finder = do_GetInterface(mWebBrowser);
	NS_ENSURE_TRUE(finder, FALSE);
	
	finder->SetSearchString(searchString);

	// HACK because not use typeahead
	// The problem with the autosearch feature is that 
	// webbrowserfind start to search at the end of the 
	// current selection. But with autosearch it should
	// start at the beginning. So I collapse the selection.
	if (ahead) {
		nsCOMPtr<nsIDOMWindow> dom;
		mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		CollapseSelToStartInFrame(dom);
	}

	return FindNext(FALSE);
}

#else

BOOL CBrowserWrapper::Find(const wchar_t* searchString, BOOL ahead)
{
	NS_ENSURE_TRUE(mFinder, FALSE);
	mFinder->SetSearchString(searchString);
	return FindNext(FALSE);
}
*/
#endif
/*
BOOL CBrowserWrapper::FindNext(BOOL backward)
{
	NS_ENSURE_TRUE(mFinder, FALSE);

	PRBool didFind;
	if (!backward) 
		mFinder->FindNext(&didFind);
	else
	{
		mFinder->SetFindBackwards(PR_TRUE);
		mFinder->FindNext(&didFind);
		mFinder->SetFindBackwards(PR_FALSE);
	}

	return didFind;
}

void CBrowserWrapper::SetMatchCase(BOOL match)
{
	NS_ENSURE_TRUE(mFinder, );
	mFinder->SetMatchCase(match ? PR_TRUE : PR_FALSE);
}

void CBrowserWrapper::SetWrapAround(BOOL wrap)
{
	NS_ENSURE_TRUE(mFinder, );
	mFinder->SetWrapFind(wrap ? PR_TRUE : PR_FALSE);
}

// XXX
wchar_t* CBrowserWrapper::GetSearchString()
{
	NS_ENSURE_TRUE(mFinder, NULL);
	nsEmbedString stringBuf;
	mFinder->GetSearchString(getter_Copies(stringBuf));
	return wcsdup(stringBuf.get());
}
*/