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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxpriv.h>		// Needed for MFC MBCS/Unicode Conversion Macros
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>

#include <afxole.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
//}}AFX_INSERT_LOCATION


// Please don't change the line below, I have a perl script that depends on it being here :)
// - BEGIN MOZILLA INCLUDES -
// Additional include directories: 
// /projects/mozilla/mozilla/dist/include/docshell, /projects/mozilla/mozilla/dist/include/dom, /projects/mozilla/mozilla/dist/include/embed_base, /projects/mozilla/mozilla/dist/include/exthandler, /projects/mozilla/mozilla/dist/include/find, /projects/mozilla/mozilla/dist/include/gfx, /projects/mozilla/mozilla/dist/include/helperAppDlg, /projects/mozilla/mozilla/dist/include/intl, /projects/mozilla/mozilla/dist/include/layout, /projects/mozilla/mozilla/dist/include/necko, /projects/mozilla/mozilla/dist/include/pref, /projects/mozilla/mozilla/dist/include/profile, /projects/mozilla/mozilla/dist/include/shistory, /projects/mozilla/mozilla/dist/include/string, /projects/mozilla/mozilla/dist/include/uriloader, /projects/mozilla/mozilla/dist/include/wallet, /projects/mozilla/mozilla/dist/include/webBrowser_core, /projects/mozilla/mozilla/dist/include/webshell, /projects/mozilla/mozilla/dist/include/widget, /projects/mozilla/mozilla/dist/include/windowwatcher, /projects/mozilla/mozilla/dist/include/xpcom, 

// docshell: 
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"

// dom: 
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"

// embed_base: 
#include "nsIWindowCreator.h"
#include "nsEmbedAPI.h"

// exthandler: 
#include "nsIExternalHelperAppService.h"

// find: 
#include "nsIWebBrowserFind.h"

// gfx: 
#include "nsIPrintOptions.h"

// helperAppDlg: 
#include "nsIHelperAppLauncherDialog.h"

// intl: 
#include "nsIStringBundle.h"

// layout: 
#include "nsIPrintListener.h"

// necko: 
#include "nsIPrompt.h"
#include "nsIHTTPChannel.h"
#include "nsIURI.h"
#include "nsIRequestObserver.h"
#include "nsIChannel.h"
#include "nsNetUtil.h"

// pref: 
#include "nsIPref.h"

// profile: 
#include "nsIProfileChangeStatus.h"

// shistory: 
#include "nsISHistory.h"
#include "nsIHistoryEntry.h"
#include "nsISHEntry.h"

// string: 
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsString.h"

// uriloader: 
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"

// wallet: 
#include "nsIWalletService.h"

// webBrowser_core: 
#include "nsIWebBrowserChrome.h"
#include "nsITooltipListener.h"
#include "nsITooltipTextProvider.h"
#include "nsCTooltipTextProvider.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsCWebBrowser.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIContextMenuListener.h"
#include "nsIWebBrowserPrint.h"
#include "nsIEmbeddingSiteWindow.h"

// webshell: 
#include "nsIClipboardCommands.h"

// widget: 
#include "nsIBaseWindow.h"
#include "nsWidgetsCID.h"
#include "nsIFilePicker.h"
#include "nsIWidget.h"

// windowwatcher: 
#include "nsIWindowWatcher.h"

// xpcom: 
#include "nsAppDirectoryServiceDefs.h"
#include "nsWeakReference.h"
#include "nsIGenericFactory.h"
#include "nsIObserver.h"
#include "nsIInterfaceRequestor.h"
#include "nsVoidArray.h"
#include "nsError.h"
#include "nsIObserverService.h"
#include "nsCOMPtr.h"

// - END MOZILLA INCLUDES -
// Please don't change the line above, I have a perl script that depends on it being here :)

#endif //_STDAFX_H
