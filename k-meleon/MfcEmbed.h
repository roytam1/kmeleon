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

// mozembed.h : main header file for the MOZEMBED application
//

#define NIGHTLY

#ifndef _MFCEMBED_H
#define _MFCEMBED_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "Plugins.h"
#include "Preferences.h"
#include "LangParser.h"
#include "KmMenu.h"
#include "AccelParser.h"
#include "KmeleonConst.h"
#include "CmdLine.h"
#include "MostRecentUrls.h"
#ifdef INTERNAL_SITEICONS
#include "faviconlist.h"
#endif
#include "resource.h"       // main symbols

enum FolderType;

#define HIDDEN_WINDOW_CLASS  _T("KMeleon")
#define BROWSER_WINDOW_CLASS _T("KMeleon Browser Window")

class CBrowserFrame;
class CProfileMgr;

/////////////////////////////////////////////////////////////////////////////
// CMfcEmbedApp:
// See mozembed.cpp for the implementation of this class
//

class CMfcEmbedApp : public CWinApp,
                     public nsIObserver,
                     public nsIWindowCreator,
                     public nsSupportsWeakReference
{
public:
	CMfcEmbedApp();
	
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIWINDOWCREATOR

	CBrowserFrame* CreateNewBrowserFrame(PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_ALL, 
							PRBool inBackground = FALSE,
							CWnd* pParent = NULL);

	CBrowserFrame* CreateNewBrowserFrameWithUrl(LPCTSTR url, LPCTSTR refferer = NULL,
							BOOL bBackground = FALSE, 
							CWnd* pParent = NULL);

	CBrowserFrame* CreateNewChromeDialog(LPCTSTR url, CWnd* pParent = NULL);

	void RemoveFrameFromList(CBrowserFrame* pFrm);
   void RegisterWindow(CDialog *window);
   void UnregisterWindow(CDialog *window);
   void UpdateWindowListMenu();
   void DrawWindowListMenu(HMENU menu);
   void BroadcastMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	inline HICON GetDefaultIcon(BOOL large = FALSE) {HICON ret; large ? ret = m_hMainIcon : ret = m_hSmallIcon; return ret;}
	bool FindSkinFile( CString& szSkinFile, TCHAR *filename );
   CString GetFolder(FolderType folder);

   nsresult SetOffline(BOOL offline);
   nsresult OverrideComponents();
#ifdef _DEBUG
   void ShowDebugConsole();
#endif

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcEmbedApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual BOOL IsIdleMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

   // list of browser windows
   CObList m_FrameWndLst;

   // list of download windows
   CObList m_MiscWndLst;

   CPlugins      plugins;
   CPreferences  preferences;
   CLangParser   lang;
   KmMenuService menus;
   CAccelParser  accel;
   CCmdLine      cmdline;
#ifdef INTERNAL_SITEICONS
   CFavIconList  favicons;
#endif
   CString       m_sRootFolder;

   CBrowserFrame* m_pMostRecentBrowserFrame; // the most recently used frame
   CBrowserFrame* m_pOpenNewBrowserFrame; // used by OnNewBrowser to preserve an initilaized frame

   int GetID(const char *strID);

   // Implementation
public:
   HANDLE m_hMutex;
   CMostRecentUrls *m_MRUList;
#ifndef _UNICODE 
   BOOL m_bUnicode;
#endif
   BOOL        LoadLanguage();
   //{{AFX_MSG(CMfcEmbedApp)
   afx_msg void OnAppAbout();
   afx_msg void OnNewBrowser();
   afx_msg void OnManageProfiles();
   afx_msg void OnPreferences();
   afx_msg void OnToggleOffline();
   afx_msg void OnUpdateToggleOffline(CCmdUI* pCmdUI);
   afx_msg void OnUpdateWindows(CCmdUI* pCmdUI);
   afx_msg void OnWindowSelect(UINT id);

   // NOTE - the ClassWizard will add and remove member functions here.
   // DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   BOOL			InitializeProfiles();
   BOOL			CreateHiddenWindow();  
   BOOL        InitializePrefs();
   BOOL        InitializeMenusAccels();
   nsresult    InitializeWindowCreator();
   void        InitializeDefineMap();
   void        CheckProfileVersion();

   CProfileMgr *m_ProfileMgr;
   BOOL        m_bAlreadyRunning;
   BOOL        m_bFirstWindowCreated;
   BOOL        m_bSwitchingProfiles;
   HICON       m_hMainIcon;
   HICON       m_hSmallIcon;
   HINSTANCE   m_hResDll;

   // used to process the rebar DrawToolbarMenu function, which must only
   // be called once, but must be called after the first window has been created

   CMap<CString, LPCTSTR, int, int &> defineMap;

};

extern CMfcEmbedApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _MFCEMBED_H

