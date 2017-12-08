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
 *
 * ***** END LICENSE BLOCK ***** */

#include "mozilla-config.h"
#define NS_ARRAY_LENGTH(array_) \
  (sizeof(array_)/sizeof(array_[0]))


#include <mozilla/Char16.h>
#define XPCOM_GLUE
#define INTERNAL_SIDEBAR
#define INTERNAL_SITEICONS

#include "js-config.h"
#include "nsCOMPtr.h"

#include "nsIURI.h"
#include "nsIFile.h"
#include "nsMemory.h"
#include "nsNetCID.h"
#include "nsEmbedCID.h"
#include "nsIObserver.h"
#include "nsEmbedString.h"
#include "nsWeakReference.h"
#include "nsIWindowCreator.h"
#include "nsIInterfaceRequestor.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAppDirectoryServiceDefs.h"

#if defined(THERECANBENODEBUG) 
#define DEBUG
#endif

#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"

#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebProgressListener.h"

#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"
#include "nsXULAppAPI.h"

#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"

class nsIWebBrowserChrome;

namespace AppCallbacks {
  nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
             nsIWebBrowserChrome *aParent,
             nsIWebBrowserChrome **aNewWindow);

  void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, PRBool aEnabled);

  PRUint32 RunEventLoop(PRBool &aRunCondition);
}
