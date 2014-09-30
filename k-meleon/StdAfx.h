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

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef NEW_H
#define NEW_H <new>
#endif

#ifndef WINVER				
#ifdef _UNICODE
#define WINVER 0x0500
#else
#define WINVER 0x0400		
#endif
#endif

#ifndef _WIN32_WINNT
#ifdef _UNICODE
#define _WIN32_WINNT 0x0500
#else
#define _WIN32_WINNT 0x0400
#endif
#endif

#pragma comment(lib, "mozjs.lib")
#ifdef _AFXDLL
	#pragma comment(lib, "xpcomglue.lib")
#else
	#pragma comment(lib, "xpcomglue_staticruntime.lib")
#endif

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

#ifdef _DEBUG
#include <vld.h> 
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // Classes MFC Automation
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxpriv.h>		// Needed for MFC MBCS/Unicode Conversion Macros
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
//#include <afxcontrolbars.h>     // prise en charge des MFC pour les rubans et les barres de contrôles
#include <afxtempl.h>
#include <afxole.h>
#include <afxtoolbar.h>
#include <afxframewndex.h>

#undef min
#undef max

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#if defined(THERECANBENODEBUG) && defined(DEBUG)
#undef DEBUG
#endif

#define XP_WIN
#define XPCOM_GLUE
#define MOZILLA_STRICT_API
#define INTERNAL_SIDEBAR
#define INTERNAL_SITEICONS


#include "mozilla-config.h"

// docshell: 
#include "nsIDocShell.h"

// dom:
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsPIDOMWindow.h"
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
#include "nsIWindowCreator.h"

// necko: 
//#include "nsIPrompt.h"
#include "nsIURI.h"

// nkcache:
#include "nsICacheService.h"

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

// string:
#include "nsEmbedString.h"

// uriloader:
#include "nsIWebProgress.h"
#include "nsIWebProgressListener2.h"

// wallet: 
//#include "nsIWalletService.h"

// webBrowser_core: 
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIContextMenuListener2.h"
#include "nsIWebBrowserPrint.h"

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

