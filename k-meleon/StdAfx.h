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
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

//unicows.lib  xpcom.lib xpcomglue_s.lib nspr4.lib embed_base_s.lib plc4.lib plds4.lib kernel32.lib user32.lib gdi32.lib wsock32.lib advapi32.lib comdlg32.lib shell32.lib version.lib winspool.lib oleacc.lib oledlg.lib uafxcwd.lib msimg32.lib shlwapi.lib comctl32.lib atl.lib ..\pngdib-3.0.1\lib\pngdib_u_s.lib "..\lpng128\projects\visualc71\lib\libpng_u.lib" "..\lpng128\projects\visualc71\lib\zlib\zlib_u.lib"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef NEW_H
#define NEW_H <new>
#endif

#ifndef WINVER				// Autorise l'utilisation des fonctionnalités spécifiques à Windows 95 et Windows NT 4 ou version ultérieure.
#ifdef _UNICODE
#define WINVER 0x0500
#else
#define WINVER 0x0400		// Attribuez la valeur appropriée à cet élément pour cibler Windows 98 et Windows 2000 ou version ultérieure.
#endif
#endif
#define _WIN32_WINNT 0x0400

#ifdef XPCOM_GLUE
	#pragma comment(lib, "xpcomglue.lib")
#else
#ifdef _BUILD_STATIC_BIN
	// Lot of shit to put here.
#else
	#pragma comment(lib, "xpcomglue_s.lib")
	#pragma comment(lib, "xpcom.lib")
#endif
#endif
#pragma comment(lib, "nspr4.lib")
#pragma comment(lib, "embed_base_s.lib")
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit 

// turns off MFC's hiding of some common and often safely ignored warning messages 
#define _AFX_ALL_WARNINGS

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
#include <afxtempl.h>
#include <afxole.h>

#if defined(THERECANBENODEBUG) && defined(DEBUG)
#undef DEBUG
#endif

#define MOZILLA_STRICT_API
#define INTERNAL_SIDEBAR
#define INTERNAL_SITEICONS

// Please don't change the line below, I have a perl script that depends on it being here :)
// - BEGIN MOZILLA INCLUDES -
// Additional include directories: 
// ../mozilla/mozilla/dist/include/docshell, ../mozilla/mozilla/dist/include/dom, ../mozilla/mozilla/dist/include/embed_base, ../mozilla/mozilla/dist/include/exthandler, ../mozilla/mozilla/dist/include/find, ../mozilla/mozilla/dist/include/gfx, ../mozilla/mozilla/dist/include/helperAppDlg, ../mozilla/mozilla/dist/include/intl, ../mozilla/mozilla/dist/include/necko, ../mozilla/mozilla/dist/include/nkcache, ../mozilla/mozilla/dist/include/pref, ../mozilla/mozilla/dist/include/profile, ../mozilla/mozilla/dist/include/shistory, ../mozilla/mozilla/dist/include/string, ../mozilla/mozilla/dist/include/uriloader, ../mozilla/mozilla/dist/include/wallet, ../mozilla/mozilla/dist/include/webBrowser_core, ../mozilla/mozilla/dist/include/webbrowserpersist, ../mozilla/mozilla/dist/include/webshell, ../mozilla/mozilla/dist/include/widget, ../mozilla/mozilla/dist/include/windowwatcher, ../mozilla/mozilla/dist/include/xpcom, ../mozilla/mozilla/dist/include/nspr, 

// docshell: 
#include "nsIDocShell.h"

// dom:
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLDocument.h"

/*
#include "nsIDOMWindowCollection.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIWebNavigation.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMHTMLFrameSetElement.h"
*/

// embed_base: 
#include "nsEmbedAPI.h"
#include "nsIWindowCreator.h"

// necko: 
//#include "nsIPrompt.h"
#include "nsIURI.h"

// nkcache:
#include "nsICacheService.h"

// pref:
#include "nsIPref.h"

// profile:
#include "nsIProfile.h"
#include "nsIProfileChangeStatus.h"

// string:
#include "nsEmbedString.h"

// uriloader:
#include "nsIWebProgress.h"
#include "nsIWebProgressListener2.h"

// wallet: 
#include "nsIWalletService.h"

// webBrowser_core: 
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIContextMenuListener2.h"
#include "nsIWebBrowserPrint.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsCWebBrowser.h"

// webbrowserpersist: 
#include "nsIWebBrowserPersist.h"

// webshell: 
#include "nsIClipboardCommands.h"

// widget:
/*
#include "nsIBaseWindow.h"
#include "nsWidgetsCID.h"
#include "nsIFilePicker.h"
#include "nsIWidget.h"*/

// xpcom:
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsIGenericFactory.h" 
#include "nsIInterfaceRequestor.h"
#include "nsIServiceManager.h"
#include "nsError.h"
#include "imgIContainer.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsMemory.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsNetCID.h"
#include "nsIInterfaceRequestorUtils.h"

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

#ifndef USE_PROFILES
	#pragma comment(lib, "profdirserviceprovidersa_s.lib")
#endif

