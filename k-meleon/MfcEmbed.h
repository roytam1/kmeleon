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
#include "MenuParser.h"
#include "AccelParser.h"
#include "KmeleonConst.h"
#include "CmdLine.h"

#include "resource.h"       // main symbols


#define HIDDEN_WINDOW_CLASS  "KMeleon"
#define BROWSER_WINDOW_CLASS "KMeleon Browser Window"

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
							PRInt32 x = -1, PRInt32 y = -1, 
							PRInt32 cx = -1, PRInt32 cy = -1,
							PRBool bShowWindow = PR_TRUE);
	void RemoveFrameFromList(CBrowserFrame* pFrm);
   void RegisterWindow(CDialog *window);
   void UnregisterWindow(CDialog *window);

   nsresult OverrideComponents();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcEmbedApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

   // list of browser windows
   CObList m_FrameWndLst;

   // list of download windows
   CObList m_MiscWndLst;


   CPlugins      plugins;
   CPreferences  preferences;
   CMenuParser   menus;
   CAccelParser  accel;
   CCmdLine      cmdline;

   HMENU          m_toolbarControlsMenu;
   CBrowserFrame* m_pMostRecentBrowserFrame; // the most recently used frame
   CBrowserFrame* m_pOpenNewBrowserFrame; // used by OnNewBrowser to preserve an initilaized frame

   int GetID(char *strID);

   // Implementation
public:
   //{{AFX_MSG(CMfcEmbedApp)
   afx_msg void OnNewBrowser();
   afx_msg void OnManageProfiles();
   afx_msg void OnPreferences();
   // NOTE - the ClassWizard will add and remove member functions here.
   // DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL			InitializeProfiles();
	BOOL			CreateHiddenWindow();  
   nsresult    InitializePrefs();
   nsresult    InitializeMenusAccels();
   nsresult    InitializeWindowCreator();
   void        InitializeDefineMap();


private:
   CProfileMgr *m_ProfileMgr;
   BOOL        m_bAlreadyRunning;
   BOOL        m_bFirstWindowCreated;
   BOOL        m_bSwitchingProfiles;
   // used to process the rebar DrawToolbarMenu function, which must only
   // be called once, but must be called after the first window has been created

   CMap<CString, LPCSTR, int, int &> defineMap;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _MFCEMBED_H

