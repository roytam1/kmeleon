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

// BrowserView.h : interface of the CBrowserView class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BROWSERVIEW_H
#define _BROWSERVIEW_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "IBrowserFrameGlue.h"

/////////////////////////////////////////////////////////////////////////////
// CBrowserView window

class CBrowserFrame;
class CBrowserImpl;
class CFindDialog;
class CPrintProgressDialog; 
class nsIPrintSettings;                                                         


class CBrowserView : public CWnd
{
public:
	CBrowserView();
	virtual ~CBrowserView();

	// Some helper methods
	HRESULT CreateBrowser();
	HRESULT DestroyBrowser();
	void OpenURL(const char* pUrl);
	void OpenURL(const PRUnichar* pUrl);
	CBrowserFrame* CreateNewBrowserFrame(PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_ALL, 
							PRInt32 x = -1, PRInt32 y = -1, 
							PRInt32 cx = -1, PRInt32 cy = -1,
							PRBool bShowWindow = PR_TRUE);
	void OpenURLInNewWindow(const char* pUrl, BOOL bBackground=FALSE);
   void OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground=FALSE);
	void LoadHomePage();

	void GetPageTitle(CString& title);
	
	// Called by the CBrowserFrame after it creates the view
	// Essentially a back pointer to the BrowserFrame
	void SetBrowserFrame(CBrowserFrame* pBrowserFrame);
	CBrowserFrame* mpBrowserFrame;

	// Called by the CBrowserFrame after it creates the view
	// The view passes this on to the embedded Browser's Impl
	// obj
	void SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue);
	PBROWSERFRAMEGLUE mpBrowserFrameGlue;

	// Pointer to the object which implements
	// the inerfaces required by Mozilla embedders
	//
	CBrowserImpl* mpBrowserImpl;

	// Mozilla interfaces
	//
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIBaseWindow>  mBaseWindow;
	nsCOMPtr<nsIWebNavigation> mWebNav;

	void UpdateBusyState(PRBool aBusy);
	PRBool mbDocumentLoading;


   nsIDOMNode *GetNodeAtPoint(int x, int y, BOOL bPrepareMenu);
   int m_iGetNodeHack;
   nsCOMPtr<nsIDOMNode> m_pGetNode;
   
   void SetCtxMenuLinkUrl(nsAutoString& strLinkUrl);
	nsAutoString mCtxMenuLinkUrl;

	void SetCtxMenuImageSrc(nsAutoString& strImgSrc);
	nsAutoString mCtxMenuImgSrc;

   void SetCurrentFrameURL(nsAutoString& strcCurrentFrameURL);
   nsString mCtxMenuCurrentFrameURL;

   void Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

   BOOL OpenViewSourceWindow(const char* pUrl);  
   BOOL IsViewSourceUrl(CString& strUrl);

   enum _securityState {
      SECURITY_STATE_SECURE,
      SECURITY_STATE_INSECURE,
      SECURITY_STATE_BROKEN
   };

   int m_SecurityState;
   void ShowSecurityInfo();

   BOOL ViewContentContainsFrames();

   void StartPanning();
   void StopPanning();
   BOOL m_panning;
   CPoint m_panningPoint;
  
	void RefreshToolBarItem(WPARAM ItemID, LPARAM unused);

   char * GetTempFile();
   void DeleteTempFiles();

   int GetCurrentURI(char *sURI);

   inline void ClearFindDialog() { m_pFindDlg = NULL; }

   void GetBrowserWindowTitle(nsCString& title);

   NS_IMETHODIMP URISaveAs(nsIURI *aURI, bool bDocument=FALSE);

   void OnIncreaseFont();
   void OnDecreaseFont();
   void ChangeTextSize(PRInt32 change);

   BOOL GetPrintSettings();


protected:
	BOOL m_refreshBackButton;
	BOOL m_refreshForwardButton; 
   BOOL m_InPrintPreview;
   char **m_tempFileList;
   int m_tempFileCount;
  
   CFindDialog* m_pFindDlg;
   CPrintProgressDialog* m_pPrintProgressDlg;

   // Indicates whether we are currently printing      
   BOOL m_bCurrentlyPrinting;   

   // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowserView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
   virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL  
  
  // Generated message map functions
protected:
	nsCOMPtr<nsIPrintSettings> m_PrintSettings;

	//{{AFX_MSG(CBrowserView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize( UINT, int, int );
   afx_msg void OnTimer(UINT nIDEvent);
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnUrlSelectedInUrlBar();
	afx_msg void OnNewUrlEnteredInUrlBar();
   afx_msg void OnUrlKillFocus();
   afx_msg void OnUrlEditChange();
   afx_msg void OnSelectUrl();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSaveFrameAs();
   afx_msg void OnFileClose();
	afx_msg void OnViewSource();
	afx_msg void OnViewInfo();
	afx_msg void OnNavBack();
	afx_msg void OnNavForward();
	afx_msg void OnNavSearch();
	afx_msg void OnNavHome();
	afx_msg void OnNavReload();
	afx_msg void OnNavForceReload(); 
	afx_msg void OnNavStop();
	afx_msg void OnCut();
	afx_msg void OnCopy();
	afx_msg void OnPaste();
	afx_msg void OnSelectAll();
	afx_msg void OnSelectNone();
	afx_msg void OnCopyLinkLocation();
	afx_msg void OnCopyImageLocation();
   afx_msg void OnOpenLink();
   afx_msg void OnOpenLinkInNewWindow();
	afx_msg void OnOpenLinkInBackground();
	afx_msg void OnViewImageInNewWindow();
	afx_msg void OnSaveLinkAs();
	afx_msg void OnSaveImageAs();
	afx_msg void OnViewPageInfo();
	afx_msg void OnViewFrameInfo();
   afx_msg void OnShowFindDlg();
   afx_msg void OnFilePrint();
   afx_msg void OnFilePrintPreview();
   afx_msg void OnFilePrintSetup();
   afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
   afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);
   afx_msg void OnFindNext();
   afx_msg void OnFindPrev();
   afx_msg LRESULT OnFindMsg(WPARAM wParam, LPARAM lParam);
   afx_msg void OnKmeleonHome();
   afx_msg void OnKmeleonForum();
   afx_msg void OnKmeleonFAQ();
   afx_msg void OnKmeleonManual();
   afx_msg void OnAboutPlugins();
	afx_msg void OnUpdateNavBack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavForward(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
	afx_msg void OnAppAbout();
	afx_msg void OnWindowNext();
	afx_msg void OnWindowPrev();
   afx_msg void OnViewFrameSource();
   afx_msg void OnOpenFrame();
   afx_msg void OnOpenFrameInBackground();
   afx_msg void OnOpenFrameInNewWindow();   

   afx_msg void OnEditURL( NMHDR * pNotifyStruct, LRESULT * result );
   afx_msg void OnDragURL( NMHDR * pNotifyStruct, LRESULT * result );
   afx_msg void OnDropFiles( HDROP );
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

#endif //_BROWSERVIEW_H
