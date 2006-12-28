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

#ifndef _BROWSERIMPL_H
#define _BROWSERIMPL_H
#define USE_WINDOW_PROVIDER

#include "IBrowserFrameGlue.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsICommandParams.h"
#include "nsIDOMEventListener.h"
#include "nsISHistoryListener.h"
#include "nsIObserver.h"
#include "nsIDOMMouseListener.h"
#ifdef USE_WINDOW_PROVIDER
#include "nsIWindowProvider.h"
#endif

class CBrowserImpl : public nsIInterfaceRequestor,
					 public nsIWebBrowserChrome,
					 public nsIWebBrowserChromeFocus,
					 public nsIEmbeddingSiteWindow2,
					 public nsIWebProgressListener,
					 public nsIContextMenuListener2,
					 public nsITooltipListener,
					 public nsSupportsWeakReference,
					 // public nsISHistoryListener,
					 // public nsIObserver,
#ifdef USE_WINDOW_PROVIDER
					 public nsIWindowProvider,
#endif
					 public nsIDOMEventListener

					 //public nsIDOMMouseListener
{
public:
   CBrowserImpl();
   ~CBrowserImpl();
   NS_METHOD Init(PBROWSERFRAMEGLUE pBrowserFrameGlue,
                  nsIWebBrowser* aWebBrowser);

   NS_DECL_ISUPPORTS
   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIWEBBROWSERCHROME
   NS_DECL_NSIWEBBROWSERCHROMEFOCUS
   NS_DECL_NSIEMBEDDINGSITEWINDOW
   NS_DECL_NSIEMBEDDINGSITEWINDOW2
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_DECL_NSICONTEXTMENULISTENER2
   NS_DECL_NSITOOLTIPLISTENER
   //NS_DECL_NSISHISTORYLISTENER
   //NS_DECL_NSIOBSERVER
   NS_DECL_NSIDOMEVENTLISTENER
#ifdef USE_WINDOW_PROVIDER
	NS_DECL_NSIWINDOWPROVIDER
#endif
   /*NS_IMETHOD MouseDown(nsIDOMEvent* aDOMEvent);
   NS_IMETHOD MouseUp(nsIDOMEvent* aDOMEvent);
   NS_IMETHOD MouseClick(nsIDOMEvent* aDOMEvent);
   NS_IMETHOD MouseDblClick(nsIDOMEvent* aDOMEvent);
   NS_IMETHOD MouseOver(nsIDOMEvent* aDOMEvent);
   NS_IMETHOD MouseOut(nsIDOMEvent* aDOMEvent);*/

protected:
   
   PBROWSERFRAMEGLUE m_pBrowserFrameGlue;
    
   nsCOMPtr<nsIWebBrowser> mWebBrowser;
};

class CFavIconListener : public nsIDOMEventListener
{
  public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	CFavIconListener();
	virtual ~CFavIconListener();

	//nsresult Init(CBrowserView* aBrowser);
	nsresult Init(PBROWSERFRAMEGLUE pBrowserFrameGlue);
	void LoadFavIcon(nsIURI* iconURI);
	void AddIcon(char* uri, TCHAR* file, nsresult astatus);
	void Reset(nsIURI*);

	static void DwnCall(char* , TCHAR* , nsresult, void* );
	

  protected:
    PBROWSERFRAMEGLUE m_pBrowserFrameGlue;
//	CBrowserView	*mBrowser;
	BOOL m_bIcon;
};

#endif //_BROWSERIMPL_H
