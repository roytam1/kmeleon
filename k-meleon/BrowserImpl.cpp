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
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3Document.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMWindow2.h"
#include "nsIScriptSecurityManager.h" 

#ifdef USE_WINDOW_PROVIDER
#include "nsIBrowserDOMWindow.h"
#endif
#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "BrowserFrm.h" // XXXXX
#include "BrowserView.h" 

CBrowserImpl::CBrowserImpl()
{
    m_pBrowserFrameGlue = NULL;
    mWebBrowser = nsnull;
	mChromeFlags = 0;
	mChromeLoaded = PR_FALSE;
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

NS_IMETHODIMP CBrowserImpl::ShowAsModal(void)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

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
	
   if (frame->IsZoomed())
	   frame->ShowWindow(SW_RESTORE);

	if ((aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER))
	{
		CWnd* view = (CWnd*)frame->GetActiveView();
		NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
		
		RECT frameRect, viewRect;
		frame->GetWindowRect(&frameRect);
		view->GetClientRect(&viewRect);

		int deltax = frameRect.right - frameRect.left - viewRect.right;
		int deltay = frameRect.bottom - frameRect.top - viewRect.bottom;

		frame->SetWindowPos(NULL, x, y, cx+deltax, cy+deltay, flags);
	}
	else if ((aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
	{
		frame->SetWindowPos(NULL, x, y, cx, cy, flags);
	}
	else
	{
		flags |= SWP_NOSIZE;
		frame->SetWindowPos(NULL, x, y, 0, 0, flags);
	}

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
   if (m_pBrowserFrameGlue) {
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

    nsEmbedString nsTitle;
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

NS_IMETHODIMP CBrowserImpl::GetVisibility(PRBool *aVisibility)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_ERROR_FAILURE);

    m_pBrowserFrameGlue->GetVisibility(aVisibility);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetVisibility(PRBool aVisibility)
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

    m_pBrowserFrameGlue->ShowTooltip(0, 0, nsnull);

    return NS_OK;
}

#ifdef USE_WINDOW_PROVIDER
NS_IMETHODIMP CBrowserImpl::ProvideWindow(nsIDOMWindow *aParent, PRUint32 aChromeFlags, PRBool aPositionSpecified, PRBool aSizeSpecified, nsIURI *aURI, const nsAString & aName, const nsACString & aFeatures, PRBool *aWindowIsNew, nsIDOMWindow **_retval)
{
	NS_ENSURE_ARG_POINTER(aParent);
	*_retval = nsnull;
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_OK);

	nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
	NS_ENSURE_TRUE(prefs, NS_OK);

	nsCOMPtr<nsIPrefBranch> branch;
	prefs->GetBranch("browser.link.", getter_AddRefs(branch));
	NS_ENSURE_TRUE(branch, NS_OK);

	PRInt32 containerPref;
	NS_ENSURE_SUCCESS(branch->GetIntPref("open_newwindow", &containerPref), NS_OK);

	if ( containerPref != nsIBrowserDOMWindow::OPEN_NEWTAB &&
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

NS_IMETHODIMP CBrowserImpl::HandleEvent(nsIDOMEvent *event)
{
	NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_OK);

	nsresult rv;
	nsEmbedString type;
	event->GetType(type);

	if (type.Equals(NS_LITERAL_STRING("mousedown")))
	{
		// XXXXXXXXXX: Quick fix for the gesture plugin 
		nsCOMPtr<nsIDOMEventTarget> target;
		event->GetTarget(getter_AddRefs(target));
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);

		HWND h = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
		CBrowserFrame* frame = (CBrowserFrame*)CWnd::FromHandle(h);
		CBrowserView* view = (CBrowserView*)frame->GetActiveView();
		view->m_contextNode = node;
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("click")))
	{
		//event->PreventDefault();

		nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(event));
		NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

		PRUint16 button;
		mouseEvent->GetButton(&button);

		PRBool altKey, shiftKey, ctrlKey;
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

		nsCOMPtr<nsIDOMEventTarget> target;
		event->GetTarget(getter_AddRefs(target));
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(target);
		
		if (m_pBrowserFrameGlue->MouseAction(node, flags))
			event->PreventDefault();

		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("DOMContentLoaded")))
	{
		// DOMContentLoaded is not send if the page is reloaded from cache
		// Nevertheless I'm using it to get the IE favicon without waiting
		// for all images of the page to be loaded
		m_pBrowserFrameGlue->SetFavIcon(nsnull);
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("DOMLinkAdded")))
	{
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

			m_pBrowserFrameGlue->SetFavIcon(favUri);
		}
		return NS_OK;
	}

	if (type.Equals(NS_LITERAL_STRING("DOMPopupBlocked")))
	{
		nsCOMPtr<nsIDOMPopupBlockedEvent> popupEvent = do_QueryInterface(event);
		NS_ENSURE_TRUE (popupEvent, NS_ERROR_FAILURE);

		nsCOMPtr<nsIURI> uri;
#if GECKO_VERSION > 18
		// FIXME
		popupEvent->GetPopupWindowURI(getter_AddRefs(uri));
#else
		popupEvent->GetRequestingWindowURI(getter_AddRefs(uri));
#endif
		NS_ENSURE_TRUE (uri, NS_ERROR_FAILURE);

		nsEmbedCString host;
		rv = uri->GetHost(host);
		NS_ENSURE_SUCCESS (rv, rv);
		NS_ENSURE_TRUE (host.Length(), NS_OK);
	
		m_pBrowserFrameGlue->PopupBlocked(host.get());
		return NS_OK;
	}

	return NS_ERROR_NOT_IMPLEMENTED;
}
