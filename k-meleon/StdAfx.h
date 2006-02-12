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

#ifndef NEW_H
#define NEW_H <new>
#endif

#include "mozilla-config.h"

//
// These headers are very evil, as they will define DEBUG if _DEBUG is
//  defined, which is lame and not what we want for things like
//  MOZ_TRACE_MALLOC and other tools.
// If we do not detect this, various MOZ/NS debug symbols are undefined
//  and we can not build.
// /MDd defines _DEBUG automagically to have the right debug C LIB
//  functions get called (so we can get symbols and hook into malloc).
//
#if !defined(DEBUG)
#define THERECANBENODEBUG
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxpriv.h>		// Needed for MFC MBCS/Unicode Conversion Macros
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
//#include <afxtempl.h>

//#include <afxole.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
//}}AFX_INSERT_LOCATION

#if defined(THERECANBENODEBUG) && defined(DEBUG)
#undef DEBUG
#endif

#define MOZILLA_STRICT_API

// Please don't change the line below, I have a perl script that depends on it being here :)
// - BEGIN MOZILLA INCLUDES -
// Additional include directories: 
// ../mozilla/mozilla/dist/include/docshell, ../mozilla/mozilla/dist/include/dom, ../mozilla/mozilla/dist/include/embed_base, ../mozilla/mozilla/dist/include/exthandler, ../mozilla/mozilla/dist/include/find, ../mozilla/mozilla/dist/include/gfx, ../mozilla/mozilla/dist/include/helperAppDlg, ../mozilla/mozilla/dist/include/intl, ../mozilla/mozilla/dist/include/necko, ../mozilla/mozilla/dist/include/nkcache, ../mozilla/mozilla/dist/include/pref, ../mozilla/mozilla/dist/include/profile, ../mozilla/mozilla/dist/include/shistory, ../mozilla/mozilla/dist/include/string, ../mozilla/mozilla/dist/include/uriloader, ../mozilla/mozilla/dist/include/wallet, ../mozilla/mozilla/dist/include/webBrowser_core, ../mozilla/mozilla/dist/include/webbrowserpersist, ../mozilla/mozilla/dist/include/webshell, ../mozilla/mozilla/dist/include/widget, ../mozilla/mozilla/dist/include/windowwatcher, ../mozilla/mozilla/dist/include/xpcom, ../mozilla/mozilla/dist/include/nspr, 

// docshell: 
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeItem.h"

// dom: 
//#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLFrameSetElement.h"

// embed_base: 
#include "nsEmbedAPI.h"
#include "nsIWindowCreator.h"

// exthandler: 
#include "nsIExternalHelperAppService.h"

// find: 
#include "nsIWebBrowserFind.h"

// gfx: 

// helperAppDlg: 
#include "nsIHelperAppLauncherDialog.h"

// intl: 

// necko: 
#include "nsIPrompt.h"
#include "nsIURI.h"

// nkcache: 
#include "nsICacheService.h"

// pref: 
#include "nsIPref.h"

// profile: 
#include "nsIProfile.h"
#include "nsIProfileChangeStatus.h"

// shistory: 
#include "nsISHistory.h"
#include "nsISHEntry.h"

// string: 
#include "nsEmbedString.h"

// uriloader: 
#include "nsIWebProgress.h"
#include "nsIWebProgressListener2.h"

// wallet: 
#include "nsIWalletService.h"

// webBrowser_core: 
#include "nsIWebBrowser.h"
#include "nsIContextMenuListener2.h"
#include "nsIWebBrowserPrint.h"
#include "nsIWebBrowserChrome.h"
#include "nsITooltipListener.h"
#include "nsIWebBrowserFocus.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsCWebBrowser.h"
#include "nsIWebBrowserChromeFocus.h"

// webbrowserpersist: 
#include "nsIWebBrowserPersist.h"

// webshell: 
#include "nsIClipboardCommands.h"

// widget: 
#include "nsIBaseWindow.h"
#include "nsWidgetsCID.h"
#include "nsIFilePicker.h"
#include "nsIWidget.h"

// xpcom:
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsIGenericFactory.h" 
#include "nsIInterfaceRequestor.h"
#include "nsIServiceManager.h"
#include "nsError.h"
//#include "imgIContainer.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsMemory.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsNetCID.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIPrintSettings.h"
// nspr: 

// Not Found in ../mozilla/mozilla/dist/include: 
//#include "nsIPrintListener.h"

// - END MOZILLA INCLUDES -
// Please don't change the line above, I have a perl script that depends on it being here :)




// MfcEmbed #defines

// USE_PROFILES - If defined, nsIProfile will be used which allows for
// multiple profiles. If not defined, a standalone directory service provider
// will be used to provide "profile" locations to one specified directory.
// In the case, the mozilla profile DLL is not needed.

#define USE_PROFILES 1

#endif //_STDAFX_H
