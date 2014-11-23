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
#include "BrowserWindow.h"
#include "MozUtils.h"

#include "nsIDOMEvent.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsPIWindowRoot.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMLocation.h"
#include "nsPIDOMWindow.h"

#include "nsIScriptSecurityManager.h" 


#ifdef USE_WINDOW_PROVIDER
#include "nsIBrowserDOMWindow.h"
#endif
#include "jsapi.h"

#include "BrowserFrm.h" // XXXXX
#include "BrowserView.h" 
#include "BrowserWindow.h"

CBrowserImpl::CBrowserImpl()
{
    m_pBrowserFrameGlue = NULL;
    mWebBrowser = nullptr;
	mChromeFlags = 0;
	mChromeLoaded = false;
	mIsPrinting = false;
}


CBrowserImpl::~CBrowserImpl()
{
}

// It's very important that the creator of this CBrowserImpl object
// Call on this Init() method with proper values after creation
//
NS_METHOD CBrowserImpl::Init(PBROWSERGLUE pBrowserFrameGlue, 
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
  // NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   //NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener2)
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
	nsCString str;
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
   NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

#ifndef _UNICODE
	if (wcslen(aStatus) > 1024)
		return NS_OK;
#endif

	m_pBrowserFrameGlue->UpdateStatusBarText(PRUnicharToCString(aStatus));

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
	NS_ENSURE_ARG_POINTER(aChromeMask);
	*aChromeMask = mChromeFlags;
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetChromeFlags(PRUint32 aChromeMask)
{
	mChromeFlags = aChromeMask;
	return NS_OK;
}

// Will get called in response to JavaScript window.close()
//
NS_IMETHODIMP CBrowserImpl::DestroyBrowserWindow()
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

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
   NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);
   m_pBrowserFrameGlue->SetBrowserSize(aCX, aCY);
   return NS_OK;
}

#include "nsIXPConnect.h"
#include "nsIScriptGlobalObject.h"

NS_IMETHODIMP CBrowserImpl::ShowAsModal(void)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
	CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
	
	/*//https://bugzilla.mozilla.org/show_bug.cgi?id=865729
	nsresult rv;
	nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
	JSContext *cx = xpc->GetCurrentJSContext();
	nsCOMPtr<nsIScriptContext> scx =
    do_QueryInterface(static_cast<nsISupports *> (::JS_GetContextPrivate(cx)));


	nsCOMPtr<nsIJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
	if (stack && NS_SUCCEEDED(stack->Push(nullptr))) {*/
		
		frame->DoModal();

	/*	JSContext* cx;
		stack->Pop(&cx);
		NS_ASSERTION(cx == nullptr, "JSContextStack mismatch");
	}
	else
		return NS_ERROR_FAILURE;*/

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsWindowModal(bool *retval)
{
   NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

   HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
   CWnd* frame = CWnd::FromHandle(h);
   *retval = frame->m_nFlags & WF_CONTINUEMODAL;

  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ExitModalEventLoop(nsresult aStatus)
{
   NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

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
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

    if (!m_pBrowserFrameGlue->FocusNextElement())
		return NS_ERROR_FAILURE;
    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::FocusPrevElement()
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

    if (!m_pBrowserFrameGlue->FocusPrevElement())
		return NS_ERROR_FAILURE;
    return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIEmbeddingSiteWindow
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
	CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);

	UINT flags = SWP_NOACTIVATE | SWP_NOZORDER;

	if (!(aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)) 
		flags |= SWP_NOMOVE;
	
	WINDOWPLACEMENT wp;
	frame->GetWindowPlacement(&wp);

	if ((aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER))
	{
		CWnd* view = (CWnd*)frame->GetActiveView();
		NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
		
		RECT frameRect, viewRect;
		frame->GetWindowRect(&frameRect);
		view->GetClientRect(&viewRect);

		int deltax = frameRect.right - frameRect.left - viewRect.right;
		int deltay = frameRect.bottom - frameRect.top - viewRect.bottom;

		RECT rc = {x, y, x+cx+deltax, y+cy+deltay};
		wp.rcNormalPosition = rc;
	}
	else if ((aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
	{
		RECT rc = {x, y, x+cx, y+cy};
		wp.rcNormalPosition = rc;
	}
	else
	{
		flags |= SWP_NOSIZE;
		wp.rcNormalPosition.left = x;
		wp.rcNormalPosition.top = y;
	}

	if (!mChromeLoaded && (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
		wp.showCmd = SW_HIDE;
	frame->SetWindowPlacement(&wp);
    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	RECT wndRect;
	HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
	CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
    
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
		frame->GetWindowRect(&wndRect);
		*x = wndRect.left;
		*y = wndRect.top;
    }
    
	if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
    {
		CWnd* view = (CWnd*)frame->GetActiveView();
		NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

		view->GetClientRect(&wndRect);
		*cx = wndRect.right;
		*cy = wndRect.bottom;
    }
	else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
    	frame->GetWindowRect(&wndRect);
		*cx = wndRect.right - wndRect.left;
		*cy = wndRect.bottom - wndRect.top;
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetSiteWindow(void** aSiteWindow)
{
   if (!aSiteWindow)
      return NS_ERROR_NULL_POINTER;

   *aSiteWindow = 0;
   // Thanks to https://bugzilla.mozilla.org/show_bug.cgi?id=588735
   // If we don't want the windows destroyed during print we must return null
   if (m_pBrowserFrameGlue && !mIsPrinting) {
       HWND w = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
       *aSiteWindow = reinterpret_cast<void *>(w);
   }
   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetFocus()
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);
	
	m_pBrowserFrameGlue->SetFocus();

	nsresult rv;
	nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
	
//	rv = baseWindow->SetFocus();

	return rv;
}

NS_IMETHODIMP CBrowserImpl::GetTitle(PRUnichar** aTitle)
{
	NS_ENSURE_ARG_POINTER(aTitle);
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	CString title;
	m_pBrowserFrameGlue->GetBrowserTitle(title);

    nsString nsTitle;
    *aTitle = NS_StringCloneData(CStringToNSString(title));
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetTitle(const PRUnichar* aTitle)
{
	NS_ENSURE_ARG_POINTER(aTitle);
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

#ifndef _UNICODE
	if (wcslen(aTitle)>1024)
		return NS_OK;
#endif

	m_pBrowserFrameGlue->SetBrowserTitle(PRUnicharToCString(aTitle));
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetVisibility(bool *aVisibility)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

    m_pBrowserFrameGlue->GetVisibility(aVisibility);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetVisibility(bool aVisibility)
{
    NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	if (!mChromeLoaded && (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
		return NS_OK; // Not yet, waiting for resize in UpdateBusyState

    m_pBrowserFrameGlue->SetVisibility(aVisibility);

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
	NS_ENSURE_ARG_POINTER(aTipText);
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

    m_pBrowserFrameGlue->ShowTooltip(aXCoords, aYCoords, PRUnicharToCString(aTipText));

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHideTooltip()
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

	// WORKAROUND: Because the tooltip is not erased correctly when using the mouse wheel
	// XXXX
	nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
	if (baseWindow) baseWindow->Repaint(PR_FALSE);

    m_pBrowserFrameGlue->ShowTooltip(0, 0, nullptr);

    return NS_OK;
}

#ifdef USE_WINDOW_PROVIDER
NS_IMETHODIMP CBrowserImpl::ProvideWindow(nsIDOMWindow *aParent, uint32_t aChromeFlags, bool aCalledFromJS, bool aPositionSpecified, bool aSizeSpecified, nsIURI *aURI, const nsAString & aName, const nsACString & aFeatures, bool *aWindowIsNew, nsIDOMWindow * *_retval)
{
	NS_ENSURE_ARG_POINTER(aParent);
	*_retval = nullptr;
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_OK);

	nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
	NS_ENSURE_TRUE(prefs, NS_OK);

	nsCOMPtr<nsIPrefBranch> branch;
	prefs->GetBranch("browser.link.", getter_AddRefs(branch));
	NS_ENSURE_TRUE(branch, NS_OK);

	PRInt32 containerPref;
	NS_ENSURE_SUCCESS(branch->GetIntPref("open_newwindow", &containerPref), NS_OK);

	if ( containerPref != (nsIBrowserDOMWindow::OPEN_NEWTAB) &&
	    containerPref != (nsIBrowserDOMWindow::OPEN_CURRENTWINDOW))
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

	CBrowserWrapper* window = m_pBrowserFrameGlue->ReuseWindow(containerPref == nsIBrowserDOMWindow::OPEN_CURRENTWINDOW);
	if (!window) return NS_OK;

	nsCOMPtr<nsIWebBrowser> browser = window->GetWebBrowser();
	NS_ENSURE_TRUE(browser, NS_OK);

	nsCOMPtr<nsIDOMWindow> domWindow;
	browser->GetContentDOMWindow(getter_AddRefs(domWindow));
	NS_ENSURE_TRUE(domWindow, NS_OK);
	
	nsCOMPtr<nsIDOMWindow> currentWindow;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(currentWindow));
	NS_ENSURE_TRUE(currentWindow, NS_OK);
	
	//*aWindowIsNew = (containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW);
	*aWindowIsNew = (currentWindow != domWindow);

	NS_ADDREF(*_retval = domWindow);
	return NS_OK;
}
#endif

//*****************************************************************************
// CBrowserImpl::nsIDOMEventListener
//***************************************************************************** 
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMXULElement.h"

NS_IMETHODIMP CBrowserImpl::HandleEvent(nsIDOMEvent *aEvent)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_OK);

	nsresult rv;
	nsString type;
	aEvent->GetType(type);

	if (type.Equals(NS_LITERAL_STRING("mousedown")))
	{
		// XXXXXXXXXX: Quick fix for the gesture plugin 
		nsCOMPtr<nsIDOMEventTarget> target;
		aEvent->GetTarget(getter_AddRefs(target));
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);

		HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
		CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
		CBrowserView* view = (CBrowserView*)frame->GetActiveView();
		view->m_contextNode = node;

		view->GetBrowserWrapper()->EndTypeAheadFind();
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("click")))
	{
		//event->PreventDefault();
		nsCOMPtr<nsIDOMEventTarget> eventTarget;
		rv = aEvent->GetOriginalTarget(getter_AddRefs(eventTarget));
		NS_ENSURE_SUCCESS(rv, rv);
		
		nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aEvent));
		NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

		int16_t button;
		mouseEvent->GetButton(&button);

		nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(eventTarget);
		if (button == 0 && targetNode)
		{
			CString urlStr;
			nsCOMPtr<nsIDOMDocument> domDocument;
			targetNode->GetOwnerDocument(getter_AddRefs(domDocument));
			if (domDocument) {
				nsString uri;
				domDocument->GetDocumentURI(uri);
				urlStr = NSStringToCString(uri);
				if (urlStr.Left(6).Compare(_T("about:")) == 0) {
					nsCOMPtr<nsIDOMElement> element = do_QueryInterface(eventTarget, &rv);
					if (element) {
						nsString str;
						rv = element->GetAttribute(NS_LITERAL_STRING("id"), str);
						if (str.Length() > 0) {
							urlStr = GetUriForDocument(domDocument);
							m_pBrowserFrameGlue->performXULCommand(str.get(), urlStr);
						}
					}
				}				
			}
		}		

		// Well, the session plugin can destroy it !
		if (!m_pBrowserFrameGlue) return NS_OK;

		bool altKey, shiftKey, ctrlKey;
		mouseEvent->GetCtrlKey(&ctrlKey);
		mouseEvent->GetShiftKey(&shiftKey);
		mouseEvent->GetAltKey(&altKey);

		UINT flags = 0;

		switch (button) {
			case 0: flags = MK_LBUTTON; break;
			case 1: flags = MK_MBUTTON; break;
			case 2: flags = MK_RBUTTON; break;
			default : ;
		}

		if (altKey) flags |= FALT << 8;
		if (shiftKey) flags |= FSHIFT << 8;
		if (ctrlKey) flags |= FCONTROL << 8;

		if (m_pBrowserFrameGlue->MouseAction(targetNode, flags)) {
			aEvent->PreventDefault();
			aEvent->StopPropagation();
		}

		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("keypress")))
	{
		nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aEvent));
		NS_ENSURE_TRUE(keyEvent, NS_ERROR_FAILURE);

		if (!theApp.preferences.GetBool("accessibility.typeaheadfind", false))
			return NS_OK;

		bool defPrevented;
		keyEvent->GetDefaultPrevented(&defPrevented);
		if (defPrevented) return NS_OK;

		bool altKey, shiftKey, ctrlKey;
		keyEvent->GetCtrlKey(&ctrlKey);
		keyEvent->GetShiftKey(&shiftKey);
		keyEvent->GetAltKey(&altKey);

		if (altKey || ctrlKey)
			return NS_OK;

		HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
		CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
		CBrowserView* view = (CBrowserView*)frame->GetActiveView();
		CBrowserWrapper* browser = view->GetBrowserWrapper();
		if (browser->InputHasFocus(false))
			return NS_OK;

		nsCOMPtr<nsIDOMEventTarget> targetNode;
		keyEvent->GetTarget(getter_AddRefs(targetNode));
		NS_ENSURE_TRUE(targetNode, NS_ERROR_NULL_POINTER);
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(targetNode);
		if (!node) return NS_OK;		

		nsCOMPtr<nsIDOMDocument> domDoc;
		node->GetOwnerDocument (getter_AddRefs(domDoc));
		NS_ENSURE_TRUE (domDoc, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMElement> docElem;
		domDoc->GetDocumentElement(getter_AddRefs(docElem));		 
		if (docElem) {
			nsString value;
			docElem->GetAttribute(NS_LITERAL_STRING("disablefastfind"), value);
			if (value.Compare(NS_LITERAL_STRING("true")) == 0)
				return NS_OK;
		}

		uint32_t code;
		keyEvent->GetKeyCode(&code);
		if (code != nsIDOMKeyEvent::DOM_VK_BACK_SPACE && code != nsIDOMKeyEvent::DOM_VK_ESCAPE && code != nsIDOMKeyEvent::DOM_VK_RETURN)
			keyEvent->GetCharCode(&code);
		nsString str;
		keyEvent->GetKey(str);
				
		if (browser->TypeAheadFind(keyEvent))
			aEvent->PreventDefault();
		return NS_OK;
	}


	if (type.Equals(NS_LITERAL_STRING("DOMContentLoaded")))
	{
		// DOMContentLoaded is not send if the page is reloaded from cache
		// Nevertheless I'm using it to get the IE favicon without waiting
		// for all images of the page to be loaded
		m_pBrowserFrameGlue->SetFavIcon(nullptr);
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("DOMLinkAdded")))
	{
		nsCOMPtr<nsIDOMEventTarget> target;
		aEvent->GetTarget(getter_AddRefs(target));
		NS_ENSURE_TRUE (target, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(target);
		NS_ENSURE_TRUE (elem, NS_ERROR_FAILURE);

		nsString value;
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
			nsCOMPtr<nsIDOMWindow> domWin;
			domDoc->GetDefaultView(getter_AddRefs(domWin));
			NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);

			nsCOMPtr<nsIDOMWindow> topDomWin;
			domWin->GetTop (getter_AddRefs (topDomWin));

			nsCOMPtr<nsISupports> domWinAsISupports (do_QueryInterface (domWin));
			nsCOMPtr<nsISupports> topDomWinAsISupports (do_QueryInterface (topDomWin));
			/* disallow subframes to set favicon */
			if (domWinAsISupports != topDomWinAsISupports) return NS_OK;

			nsString spec;
			rv = domDoc->GetDocumentURI (spec);
			NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

			nsCOMPtr<nsIURI> docUri;
			NewURI (getter_AddRefs (docUri), spec);

			nsCString favicon;
			nsCString cvalue;
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

			// XXX
			//rv = secMan->CheckLoadURI(docUri, favUri,
			//	nsIScriptSecurityManager::STANDARD);
			/* failure means it didn't pass the security check */
			//if (NS_FAILED (rv)) return NS_OK;

			/* Hide password part */
			nsCString password;
			favUri->GetUsername(password);
			favUri->SetUserPass(password);

			m_pBrowserFrameGlue->SetFavIcon(favUri);
		}
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("DOMPopupBlocked")))
	{
		nsCOMPtr<nsIDOMPopupBlockedEvent> popupEvent = do_QueryInterface(aEvent);
		NS_ENSURE_TRUE (popupEvent, NS_ERROR_FAILURE);

		nsCOMPtr<nsIURI> uri;

		nsCOMPtr<nsIDOMWindow> dom;
		popupEvent->GetRequestingWindow(getter_AddRefs(dom));
		if (dom) {			
			nsCOMPtr<nsIDOMLocation> location;
			dom->GetLocation(getter_AddRefs(location));
			if (location) {
				nsString href;
				location->GetHref(href);
				NewURI(getter_AddRefs(uri), href);
			}
		}
		if (!uri)
			popupEvent->GetPopupWindowURI(getter_AddRefs(uri));
		NS_ENSURE_TRUE (uri, NS_ERROR_FAILURE);

		nsCString host;
		rv = uri->GetHost(host);
		NS_ENSURE_SUCCESS (rv, rv);
		NS_ENSURE_TRUE (host.Length(), NS_OK);
	
		m_pBrowserFrameGlue->PopupBlocked(host.get());
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("flashblockCheckLoad")))
	{
		if (m_pBrowserFrameGlue->AllowFlash())
			aEvent->PreventDefault();
		aEvent->StopPropagation();
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("command")))
	{
		bool trusted = PR_FALSE;
		aEvent->GetIsTrusted(&trusted);
		if (!trusted) return NS_OK;

		nsCOMPtr<nsIDOMEventTarget> eventTarget;
		rv = aEvent->GetOriginalTarget(getter_AddRefs(eventTarget));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIDOMElement> element = do_QueryInterface(eventTarget, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		nsString str;
		rv = element->GetAttribute(NS_LITERAL_STRING("id"), str);
		NS_ENSURE_SUCCESS(rv, rv);
		
		CString urlStr;
		nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(eventTarget);
		if (targetNode)
		{
			nsCOMPtr<nsIDOMDocument> domDocument;
			targetNode->GetOwnerDocument(getter_AddRefs(domDocument));
			if (domDocument)
				urlStr = GetUriForDocument(domDocument);
		}

		HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
		CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
		CBrowserView* view = (CBrowserView*)frame->GetActiveView();
		view->m_contextNode = targetNode;

		if (m_pBrowserFrameGlue->performXULCommand(str.get(), urlStr))
			aEvent->StopPropagation();
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("contextmenu")))
	{
		// No context menu for chrome
		if (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
			return NS_OK;
		
		// Check if there was a custom context menu.
		bool p;
		aEvent->GetDefaultPrevented(&p);
		if (p) return NS_OK;

		/*nsCOMPtr<nsIDOMEventTarget> eventTarget;
		rv = aEvent->GetOriginalTarget(getter_AddRefs(eventTarget));
		NS_ENSURE_SUCCESS(rv, rv);		
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(eventTarget);*/
		
		nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
		NS_ENSURE_TRUE(mouseEvent, NS_ERROR_UNEXPECTED);
		nsCOMPtr<nsIDOMEventTarget> targetNode;
		aEvent->GetTarget(getter_AddRefs(targetNode));
		NS_ENSURE_TRUE(targetNode, NS_ERROR_NULL_POINTER);
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(targetNode);
		if (!node) return NS_OK;		

		// Whatever I do we get here before xul can handle the context menu
		nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(node));
		if (xulElement) return NS_OK;

		UINT flags = 0;
		
		// Look for links and images
		nsString url, title, bgImgSrc;
		nsCString imgSrc;
		if (::GetLinkTitleAndHref(node, url, title))
			flags |= CONTEXT_LINK;
		if (::GetImageSrc(node, imgSrc))
			flags |= CONTEXT_IMAGE;		

		// always consume events for plugins and Java who may throw their
		// own context menus but not for image objects.  Document objects
		// will never be targets or ancestors of targets, so that's OK.
		nsCOMPtr<nsIDOMHTMLObjectElement> objectElement;
		if (!(flags & CONTEXT_IMAGE)) 
			objectElement = do_QueryInterface(node);
		nsCOMPtr<nsIDOMHTMLEmbedElement> embedElement(do_QueryInterface(node));
		nsCOMPtr<nsIDOMHTMLAppletElement> appletElement(do_QueryInterface(node));

		if (objectElement || embedElement || appletElement)
			return NS_OK;

		// Look for form elements
		nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(node));
		if (formControl) {
			if (formControl->GetType() == NS_FORM_TEXTAREA) {
				flags |= CONTEXT_TEXT;
			} else {
				nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(formControl));
				if (inputElement) {
					if (formControl->IsSingleLineTextControl(false)) {
						flags |= CONTEXT_TEXT;
					}
				}
			}
		}

		// Check frame
		if (::GetFrameURL(mWebBrowser, node, url))
			flags |= CONTEXT_FRAME;

		// Nothing found, check if it's html
		if (!flags) {
			nsCOMPtr<nsIDOMDocument> document;
			node = do_QueryInterface(node);
			node->GetOwnerDocument(getter_AddRefs(document));
			nsCOMPtr<nsIDOMHTMLDocument> htmlDocument(do_QueryInterface(document));
			if (htmlDocument) {
				flags |= CONTEXT_DOCUMENT;	
				if (::GetBackgroundImageSrc(node, bgImgSrc)) {
					flags |= CONTEXT_BACKGROUND_IMAGE;
				}		
			}
		}

		nsCOMPtr<nsIDOMWindow> win;
		rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(win));
		NS_ENSURE_SUCCESS(rv, rv);
		NS_ENSURE_TRUE(win, NS_ERROR_FAILURE);

		nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(win));
		NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
		nsCOMPtr<nsPIWindowRoot> root = window->GetTopWindowRoot();
		NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
		if (root) {
			// set the window root's popup node to the event target
			root->SetPopupNode(node);
		}

		// XXX set the context node
		HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
		CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
		CBrowserView* view = (CBrowserView*)frame->GetActiveView();
		view->m_contextNode = node;

		m_pBrowserFrameGlue->ShowContextMenu(flags);
		return NS_OK;		
	}

	if (type.Equals(NS_LITERAL_STRING("mozfullscreenchange")))
	{
		nsCOMPtr<nsIDOMWindow> dom;
		nsresult rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIDOMDocument> document;
		rv = dom->GetDocument(getter_AddRefs(document));
		NS_ENSURE_SUCCESS(rv, rv);		
		if (!document) return NS_OK;		

		bool fullscreen;
		document->GetMozFullScreen(&fullscreen);
		m_pBrowserFrameGlue->SetFullScreen(fullscreen);
	}

	return NS_ERROR_NOT_IMPLEMENTED;
}
