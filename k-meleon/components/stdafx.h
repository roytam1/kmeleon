/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chak Nanga <chak@netscape.com> 
 */

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef _STDAFX_H
#define _STDAFX_H

#define VC_EXTRALEAN	// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxpriv.h>		// Needed for MFC MBCS/Unicode Conversion Macros
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
//}}AFX_INSERT_LOCATION}}

// Please don't change the line below, I have a perl script that depends on it being here :)
// - BEGIN MOZILLA INCLUDES -
// Additional include directories: 
// /projects/mozilla/mozilla/dist/include/dom, /projects/mozilla/mozilla/dist/include/string, /projects/mozilla/mozilla/dist/include/webBrowser_core, /projects/mozilla/mozilla/dist/include/windowwatcher, /projects/mozilla/mozilla/dist/include/xpcom, 

// dom: 
#include "nsIDOMWindow.h"

// string: 
#include "nsString.h"
#include "nsReadableUtils.h"

// webBrowser_core: 
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"

// windowwatcher: 
#include "nsIPromptService.h"
#include "nsIWindowWatcher.h"

// xpcom: 
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIFactory.h"
#include "nsError.h"
#include "nsMemory.h"

// - END MOZILLA INCLUDES -
// Please don't change the line above, I have a perl script that depends on it being here :)

#endif //_STDAFX_H
