/*
*  Copyright (C) 2000 Christophe Thibault
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

#ifndef __WebBrowserChrome__
#define __WebBrowserChrome__

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsIWebBrowserChrome.h"

#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"

// when compiling with the new code, use this line instead
//#include "nsIWebBrowserSiteWindow.h"
#include "nsIBaseWindow.h"

#include "nsIWebNavigation.h"
#include "nsIWebProgressListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebBrowser.h"
#include "nsVoidArray.h"
#include "nsIContextMenuListener.h"
#include "nsiScrollableView.h"
#include "nsISHistory.h"
#include "nsIURIContentListener.h"

#include "Prompt.h"
//#include "UnknownFile.h"

class WebBrowserChrome   : public nsIWebBrowserChrome,
                           public nsIWebProgressListener,

                           // when compiling with the new code, use this line instead
                           //public nsIWebBrowserSiteWindow
                           public nsIBaseWindow,

                           public nsIContextMenuListener,
                           public nsIInterfaceRequestor,
                           public nsIURIContentListener,
                           public CPrompt //  BC -- implements all nsIPrompt stufs
//                           public CUnknownFile // BC -- Unknown file stuff
{
public:
    WebBrowserChrome(nativeWindow nWindow, CMainFrame *parent);
    virtual ~WebBrowserChrome();

    int working;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIURICONTENTLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER

    // when compiling with the new code, use this line instead
    //NS_DECL_NSIWEBBROWSERSITEWINDOW
    NS_DECL_NSIBASEWINDOW

    NS_DECL_NSICONTEXTMENULISTENER
    NS_DECL_NSIINTERFACEREQUESTOR

protected:

   nativeWindow mNativeWindow;

   nsCOMPtr<nsIWebBrowser> mWebBrowser;
   nsCOMPtr<nsIBaseWindow> mBaseWindow;
   nsCOMPtr<nsIWebBrowserChrome> mTopWindow;

   nsIURIContentListener*  mParentContentListener;  // Weak Reference

   static nsVoidArray sBrowserList;

	 CMainFrame *parentFrame;

	 int need_new_hwnd;
};

#endif /* __WebBrowserChrome__ */
