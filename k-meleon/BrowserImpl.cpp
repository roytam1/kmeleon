/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this Mozilla sample software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Contributor(s):
 *   Chak Nanga <chak@netscape.com> 
 *
 * ***** END LICENSE BLOCK ***** */

// File Overview....
// This is the class which implements all the interfaces
// required(and optional) by the mozilla embeddable browser engine
//
// Note that this obj gets passed in the IBrowserFrameGlue* using the 
// Init() method. Many of the interface implementations use this
// to get the actual task done. Ex: to update the status bar
//
// Look at the INTERFACE_MAP_ENTRY's below for the list of 
// the currently implemented interfaces
//
// This file currently has the implementation for all the interfaces
// which are required of an app embedding Gecko
// Implementation of other optional interfaces are in separate files
// to avoid cluttering this one and to demonstrate other embedding
// principles. For example, nsIPrompt is implemented in a separate DLL.  
// 
//	nsIWebBrowserChrome	- This is a required interface to be implemented
//						  by embeddors
//
//	nsIEmbeddingSiteWindow - This is a required interface to be implemented
//						 by embedders			
//					   - SetTitle() gets called after a document
//						 load giving us the chance to update our
//						 titlebar
//
// Some points to note...
//
// nsIWebBrowserChrome interface's SetStatus() gets called when a user 
// mouses over the links on a page
//
// nsIWebProgressListener interface's OnStatusChange() gets called
// to indicate progress when a page is being loaded
//
// Next suggested file(s) to look at : 
//			Any of the BrowserImpl*.cpp files for other interface
//			implementation details
//

#include "stdafx.h"
#include "BrowserImpl.h"

#include "nsIDOMWindow.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMMouseListener.h"
#ifdef USE_WINDOW_PROVIDER
#include "nsIBrowserDOMWindow.h"
#endif
#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "BrowserFrm.h"

CBrowserImpl::CBrowserImpl()
{
    m_pBrowserFrameGlue = NULL;
    mWebBrowser = nsnull;
}


CBrowserImpl::~CBrowserImpl()
{
}

// It's very important that the creator of this CBrowserImpl object
// Call on this Init() method with proper values after creation
//
NS_METHOD CBrowserImpl::Init(PBROWSERFRAMEGLUE pBrowserFrameGlue, 
							  nsIWebBrowser* aWebBrowser)
{
	  m_pBrowserFrameGlue = pBrowserFrameGlue;
	  
	  SetWebBrowser(aWebBrowser);

	  return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(CBrowserImpl)
NS_IMPL_RELEASE(CBrowserImpl)

NS_INTERFACE_MAP_BEGIN(CBrowserImpl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
#ifdef USE_WINDOW_PROVIDER
   NS_INTERFACE_MAP_ENTRY(nsIWindowProvider)
#endif
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow2)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener2)
   NS_INTERFACE_MAP_ENTRY(nsITooltipListener)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  // NS_INTERFACE_MAP_ENTRY(nsISHistoryListener)
   //NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
   //NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
NS_INTERFACE_MAP_END

/*
NS_IMETHODIMP CBrowserImpl::MouseDown(nsIDOMEvent* aMouseEvent) {return NS_OK;}
NS_IMETHODIMP CBrowserImpl::MouseUp(nsIDOMEvent* aMouseEvent) {return NS_OK;}
NS_IMETHODIMP CBrowserImpl::MouseClick(nsIDOMEvent* aMouseEvent){return NS_OK;}
NS_IMETHODIMP CBrowserImpl::MouseDblClick(nsIDOMEvent* aMouseEvent) {return NS_OK;}
NS_IMETHODIMP CBrowserImpl::MouseOver(nsIDOMEvent* aMouseEvent) {return NS_OK;} 
NS_IMETHODIMP CBrowserImpl::MouseOut(nsIDOMEvent* aMouseEvent) {return NS_OK;}
*/
/*
NS_IMETHODIMP CBrowserImpl::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
	return NS_ERROR_NOT_IMPLEMENTED;
	
	//"nsWebBrowserFind_FindAgain"
	//return m_pBrowserFrameGlue->OnFindNext(aSubject);
}

NS_IMETHODIMP CBrowserImpl::OnHistoryNewEntry(nsIURI *aNewURI)
{
	nsEmbedCString str;
	aNewURI->GetSpec(str);
	CString cStr = str.get();
	m_pBrowserFrameGlue->UpdateMRU(str.get());
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHistoryGoBack(nsIURI *aBackURI, PRBool *_retval)
{
		return NS_ERROR_NOT_IMPLEMENTED;					
}
NS_IMETHODIMP CBrowserImpl::OnHistoryGoForward(nsIURI *aForwardURI, PRBool *_retval)
{
		return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CBrowserImpl::OnHistoryReload(nsIURI *aReloadURI, PRUint32 aReloadFlags, PRBool *_retval)
{
		return NS_ERROR_NOT_IMPLEMENTED;

}
NS_IMETHODIMP CBrowserImpl::OnHistoryGotoIndex(PRInt32 aIndex, nsIURI *aGotoURI, PRBool *_retval)
{
		return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CBrowserImpl::OnHistoryPurge(PRInt32 aNumEntries, PRBool *_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}
*/
//*****************************************************************************
// CBrowserImpl::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
	// Note that we're wrapping our nsIPrompt impl. with the 
	// nsISingleSignOnPrompt - if USE_SINGLE_SIGN_ON is defined
	// (and if single sign-on support dll's are present and enabled) 
	// This allows the embedding app to use the password save
	// feature. When signing on to a host which needs authentication
	// the Single sign-on service will check to see if the auth. info
	// is already saved and if so uses it to pre-fill the sign-on form
	// If not, our nsIPrompt impl will be called
	// to present the auth UI and return the required auth info.
	// If we do not compile with single sign-on support or if that
	// service is missing we just fall back to the regular 
	// implementation of nsIPrompt

	if(aIID.Equals(NS_GET_IID(nsIDOMWindow)))
	{
		if (mWebBrowser)
			return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
		return NS_ERROR_NOT_INITIALIZED;
	}

	return QueryInterface(aIID, aInstancePtr);
}

//*****************************************************************************
// CBrowserImpl::nsIWebBrowserChrome
//*****************************************************************************   

//Gets called when you mouseover links etc. in a web page
//
NS_IMETHODIMP CBrowserImpl::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->UpdateStatusBarText(aStatus);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);

   *aWebBrowser = mWebBrowser;

   NS_IF_ADDREF(*aWebBrowser);

   return NS_OK;
}

// Currently called from Init(). I'm not sure who else
// calls this
//
NS_IMETHODIMP CBrowserImpl::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);

   mWebBrowser = aWebBrowser;

   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetChromeFlags(PRUint32* aChromeMask)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CBrowserImpl::SetChromeFlags(PRUint32 aChromeMask)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

// Will get called in response to JavaScript window.close()
//
NS_IMETHODIMP CBrowserImpl::DestroyBrowserWindow()
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->DestroyBrowserFrame();

	return NS_OK;
}

// Gets called in response to set the size of a window
// Ex: In response to a JavaScript Window.Open() call of
// the form 
//
//		window.open("http://www.mozilla.org", "theWin", "width=200, height=400");
//
// First the CreateBrowserWindow() call will be made followed by a 
// call to this method to resize the window
//
NS_IMETHODIMP CBrowserImpl::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
   if(! m_pBrowserFrameGlue)
      return NS_ERROR_FAILURE;

   m_pBrowserFrameGlue->SetBrowserSize(aCX, aCY);

   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ShowAsModal(void)
{
	HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
	CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);

	nsCOMPtr<nsIJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
	if (stack && NS_SUCCEEDED(stack->Push(nsnull))) {
		
		frame->DoModal();

		JSContext* cx;
		stack->Pop(&cx);
		NS_ASSERTION(cx == nsnull, "JSContextStack mismatch");
	}
	else
		return NS_ERROR_FAILURE;

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsWindowModal(PRBool *retval)
{
   HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
   CWnd* frame = CWnd::FromHandle(h);
   *retval = frame->m_nFlags & WF_CONTINUEMODAL;

  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ExitModalEventLoop(nsresult aStatus)
{
   HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
   CWnd* frame = CWnd::FromHandle(h);
   frame->PostMessage(WM_CLOSE, 0, 0);
   //frame->EndModalLoop(aStatus);
   return NS_OK;
}

#if 0

NS_IMETHODIMP
CBrowserImpl::SetPersistence(PRBool aPersistX, PRBool aPersistY,
                             PRBool aPersistCX, PRBool aPersistCY,
                             PRBool aPersistSizeMode)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
CBrowserImpl::GetPersistence(PRBool* aPersistX, PRBool* aPersistY,
                             PRBool* aPersistCX, PRBool* aPersistCY,
                             PRBool* aPersistSizeMode)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

#endif

//*****************************************************************************
// CBrowserImpl::nsIWebBrowserChromeFocus
//*****************************************************************************

NS_IMETHODIMP CBrowserImpl::FocusNextElement()
{
    m_pBrowserFrameGlue->FocusNextElement();
    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::FocusPrevElement()
{
    m_pBrowserFrameGlue->FocusPrevElement();
    return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIEmbeddingSiteWindow
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION){
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER){
            m_pBrowserFrameGlue->SetBrowserFramePositionAndSize(x, y, cx, cy, PR_TRUE);
        }
        else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER){
            m_pBrowserFrameGlue->SetBrowserPositionAndSize(x, y, cx, cy, PR_TRUE);
        }
        else {
            m_pBrowserFrameGlue->SetBrowserFramePosition(x, y);
        }
    }
    else{
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER){
            m_pBrowserFrameGlue->SetBrowserFrameSize(cx, cy);
        }else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER){
            m_pBrowserFrameGlue->SetBrowserSize(cx, cy);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;
    
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
    	m_pBrowserFrameGlue->GetBrowserFramePosition(x, y);
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
    {
    	m_pBrowserFrameGlue->GetBrowserSize(cx, cy);
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
    	m_pBrowserFrameGlue->GetBrowserFrameSize(cx, cy);
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetSiteWindow(void** aSiteWindow)
{
   if (!aSiteWindow)
      return NS_ERROR_NULL_POINTER;

   *aSiteWindow = 0;
   if (m_pBrowserFrameGlue) {
       HWND w = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
       *aSiteWindow = NS_REINTERPRET_CAST(void *, w);
   }
   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetFocus()
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->SetFocus();

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetTitle(PRUnichar** aTitle)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->GetBrowserFrameTitle(aTitle);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetTitle(const PRUnichar* aTitle)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetBrowserFrameTitle(aTitle);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetVisibility(PRBool *aVisibility)
{
	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->GetBrowserFrameVisibility(aVisibility);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetVisibility(PRBool aVisibility)
{
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->ShowBrowserFrame(aVisibility);

    return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIEmbeddingSiteWindow2
//*****************************************************************************

NS_IMETHODIMP CBrowserImpl::Blur()
{
    return NS_OK;
} 


//*****************************************************************************
// CBrowserImpl::nsITooltipListener
//*****************************************************************************

/* void onShowTooltip (in long aXCoords, in long aYCoords, in wstring aTipText); */
NS_IMETHODIMP CBrowserImpl::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
    USES_CONVERSION;
   
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    const TCHAR *text = W2CT(aTipText);

    m_pBrowserFrameGlue->ShowTooltip(aXCoords, aYCoords, text);

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHideTooltip()
{
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->ShowTooltip(0, 0, nsnull);

    return NS_OK;
}

#ifdef USE_WINDOW_PROVIDER
NS_IMETHODIMP CBrowserImpl::ProvideWindow(nsIDOMWindow *aParent, PRUint32 aChromeFlags, PRBool aPositionSpecified, PRBool aSizeSpecified, nsIURI *aURI, const nsAString & aName, const nsACString & aFeatures, PRBool *aWindowIsNew, nsIDOMWindow **_retval)
{
	NS_ENSURE_ARG_POINTER(aParent);
	*_retval = nsnull;

	if(! m_pBrowserFrameGlue)
		return NS_OK;

	nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
	NS_ENSURE_TRUE(prefs, NS_OK);

	nsCOMPtr<nsIPrefBranch> branch;
	prefs->GetBranch("browser.link.", getter_AddRefs(branch));
	NS_ENSURE_TRUE(branch, NS_OK);

	PRInt32 containerPref;
	NS_ENSURE_SUCCESS(branch->GetIntPref("open_newwindow", &containerPref), NS_OK);

	if ( /*containerPref != nsIBrowserDOMWindow::OPEN_NEWTAB &&*/
	    containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW)
		return NS_OK;

	/* Now check our restriction pref.  The restriction pref is a power-user's
	   fine-tuning pref. values:     
	   770      0: no restrictions - divert everything
	   771      1: don't divert window.open at all
	   772      2: don't divert window.open with features
	   773   */

	PRInt32 restrictionPref;
	if (NS_FAILED(branch->GetIntPref("open_newwindow.restriction",
	                                 &restrictionPref)) ||
	    restrictionPref < 0 ||
	    restrictionPref > 2) 
		restrictionPref = 2; // Sane default behavior
	
	if (restrictionPref == 1) return NS_OK;

	if (restrictionPref == 2 && 
	   // Only continue if there are no size/position features and no special
	   // chrome flags.
	    (aChromeFlags != nsIWebBrowserChrome::CHROME_ALL ||
		  aPositionSpecified || aSizeSpecified))
		 return NS_OK;

	nsCOMPtr<nsIDOMWindow> domWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	NS_ENSURE_TRUE(domWindow, NS_OK);
	
	*aWindowIsNew = (containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW);

	NS_ADDREF(*_retval = domWindow);
	return NS_OK;
}
#endif

//*****************************************************************************
// CBrowserImpl::nsIDOMEventListener
//***************************************************************************** 

#include "nsIDOMContextMenuListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Document.h"
#include "nsIDOMWindow2.h"
#include "nsIScriptSecurityManager.h" 

extern nsresult NewURI(nsIURI **result, const nsAString &spec);
extern nsresult NewURI(nsIURI **result, const nsACString &spec);

NS_IMETHODIMP CBrowserImpl::HandleEvent(nsIDOMEvent *event)
{
	nsresult rv;

	nsEmbedString type;
	event->GetType(type);
	if (type.Equals(NS_LITERAL_STRING("DOMPopupBlocked"))) {
		nsCOMPtr<nsIDOMPopupBlockedEvent> popupEvent = do_QueryInterface(event);
		NS_ENSURE_TRUE (popupEvent, NS_ERROR_FAILURE);

		nsCOMPtr<nsIURI> uri;
		popupEvent->GetRequestingWindowURI(getter_AddRefs(uri));
		NS_ENSURE_TRUE (uri, NS_ERROR_FAILURE);

		nsEmbedCString host;
		rv = uri->GetHost(host);
		NS_ENSURE_SUCCESS (rv, rv);
		NS_ENSURE_TRUE (host.Length(), NS_OK);
	
		m_pBrowserFrameGlue->PopupBlocked(host.get());
		return NS_OK;
	}
	
	if (type.Equals(NS_LITERAL_STRING("mousedown"))) {
        nsCOMPtr<nsIDOMEventTarget> target;
		event->GetTarget(getter_AddRefs(target));

		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);
		m_pBrowserFrameGlue->MouseAction(node);
		/*
		nsCOMPtr<nsIBaseWindow>  mBaseWindow;
		mBaseWindow = do_QueryInterface(mWebBrowser, &rv);
	    if(NS_FAILED(rv))
			return rv;
	
		nsCOMPtr<nsIDocShellTreeItem> tree = do_QueryInterface(mBaseWindow, &rv);
		if(NS_FAILED(rv))
			return rv;

		nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
		tree->GetTreeOwner(getter_AddRefs(treeOwner));

		ChromeContextMenuListener* ccml = new ChromeContextMenuListener(mWebBrowser, webBrowserChrome);
		*/
		return NS_OK;
	}

	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMPL_ISUPPORTS1(CFavIconListener, nsIDOMEventListener)

CFavIconListener::CFavIconListener() : m_pBrowserFrameGlue(NULL)// mBrowser(NULL)
{
}

CFavIconListener::~CFavIconListener()
{
}

/***********************************************************/
/*! Part of the following code is from The Galeon Browser !*/

nsresult CFavIconListener::Init(PBROWSERFRAMEGLUE pBrowserFrameGlue)
{
	//mBrowser = aBrowser;
	m_pBrowserFrameGlue = pBrowserFrameGlue;
	m_bIcon = FALSE;
	return NS_OK;
}

NS_IMETHODIMP CFavIconListener::HandleEvent (nsIDOMEvent* event)
{
	nsEmbedString type;
	event->GetType(type);

	if (type.Equals(NS_LITERAL_STRING("DOMContentLoaded")))
	{
		// DOMContentLoaded is not send if the page is reloaded from cache
		// Nevertheless I'm using it to get the IE favicon without waiting
		// for all images of the page to be loaded
		m_pBrowserFrameGlue->SetFavIcon(nsnull);
		return NS_OK;
	}

	nsresult rv;

	nsCOMPtr<nsIDOMEventTarget> target;
	event->GetTarget(getter_AddRefs(target));
	NS_ENSURE_TRUE (target, NS_ERROR_FAILURE);

	nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(target);
	NS_ENSURE_TRUE (elem, NS_ERROR_FAILURE);

	nsEmbedString value;
	rv = elem->GetTagName(value);

	rv = elem->GetAttribute(NS_LITERAL_STRING("rel"), value);
	NS_ENSURE_SUCCESS (rv, rv);
		
	if ( wcsicmp(value.get(), L"SHORTCUT ICON") == 0 || 
		wcsicmp(value.get(), L"ICON") == 0 )
	{

/*		rv = elem->GetAttribute(NS_LITERAL_STRING("type"), value);
		NS_ENSURE_SUCCESS (rv, rv);
		if (value.Equals(NS_LITERAL_STRING("image/png")))
			return NS_OK;*/
		
		rv = elem->GetAttribute (NS_LITERAL_STRING("href"), value);
		NS_ENSURE_SUCCESS (rv, rv);
		NS_ENSURE_FALSE (value.IsEmpty(), NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMDocument> domDoc;
		elem->GetOwnerDocument (getter_AddRefs(domDoc));
		NS_ENSURE_TRUE (domDoc, NS_ERROR_FAILURE);

		/* See if this is from the toplevel frame */
		nsCOMPtr<nsIDOMDocumentView> docView (do_QueryInterface (domDoc));
		NS_ENSURE_TRUE (docView, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMAbstractView> abstractView;
		docView->GetDefaultView (getter_AddRefs (abstractView));

		nsCOMPtr<nsIDOMWindow> domWin (do_QueryInterface (abstractView));
		NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMWindow> topDomWin;
		domWin->GetTop (getter_AddRefs (topDomWin));

		nsCOMPtr<nsISupports> domWinAsISupports (do_QueryInterface (domWin));
		nsCOMPtr<nsISupports> topDomWinAsISupports (do_QueryInterface (topDomWin));
		/* disallow subframes to set favicon */
		if (domWinAsISupports != topDomWinAsISupports) return NS_OK;
		
		nsCOMPtr<nsIDOM3Document> doc = do_QueryInterface (domDoc);
		NS_ENSURE_TRUE (doc, NS_ERROR_FAILURE);

		nsEmbedString spec;
		rv = doc->GetDocumentURI (spec);
		NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);
		
		nsCOMPtr<nsIURI> docUri;
		NewURI (getter_AddRefs (docUri), spec);

		nsEmbedCString favicon;
		nsEmbedCString cvalue;
		NS_UTF16ToCString(value, NS_CSTRING_ENCODING_UTF8, cvalue);
		rv = docUri->Resolve (cvalue, favicon);
		NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

		nsCOMPtr<nsIURI> favUri;
		NewURI (getter_AddRefs (favUri), favicon);
		NS_ENSURE_TRUE (favUri, NS_ERROR_FAILURE);

		/* check if load is allowed */
		nsCOMPtr<nsIScriptSecurityManager> secMan
			(do_GetService("@mozilla.org/scriptsecuritymanager;1"));
		/* refuse if we can't check */
		NS_ENSURE_TRUE (secMan, NS_ERROR_FAILURE);

		rv = secMan->CheckLoadURI(docUri, favUri,
					  nsIScriptSecurityManager::STANDARD);
		/* failure means it didn't pass the security check */
		if (NS_FAILED (rv)) return NS_OK;
		
		/* Hide password part */
		nsEmbedCString password;
		favUri->GetUsername(password);
		favUri->SetUserPass(password);
		
		m_bIcon = TRUE;
		m_pBrowserFrameGlue->SetFavIcon(favUri);
		//LoadFavIcon(favUri);
	}
	return NS_OK;
}
/***************************************************/
