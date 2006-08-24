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
 *   Rod Spears <rods@netscape.com>
 *
 * ***** END LICENSE BLOCK ***** */ 

// BrowserView.h : interface of the CBrowserView class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BROWSERVIEW_H
#define _BROWSERVIEW_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "IBrowserFrameGlue.h"
#include "nsIX509Cert.h"
/////////////////////////////////////////////////////////////////////////////
// CBrowserView window

class CBrowserFrame;
class CBrowserImpl;
class CFindDialog;
class CPrintProgressDialog; 
class nsIPrintSettings;                                                         
class CFavIconListener;


#define UTF16ToCString(source,dest) \
{\
	USES_CONVERSION;\
	strcpy(dest, W2CA(source.get()));\
}
class CSaveAsHandler : public nsIWebProgressListener					    
{
public:
   CSaveAsHandler(nsIWebBrowserPersist* aPersist, nsIFile* aFile, nsIURI* aURL, nsIDOMDocument* aDocument,nsISupports* aDescriptor, nsIURI* aReferrer, CBrowserFrame* aBrower );
   virtual ~CSaveAsHandler();
   NS_DECL_ISUPPORTS
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_IMETHOD Save(const char* contentType, const char* disposition = NULL);

protected:
	nsIWebBrowserPersist* mPersist; 
	nsCOMPtr<nsISupports> mDescriptor;
	nsCOMPtr<nsIFile> mFile;
	nsCOMPtr<nsIURI> mURL;
	nsCOMPtr<nsIURI> mRealURI;
	nsCOMPtr<nsIDOMDocument> mDocument;
	nsCOMPtr<nsIURI> mReferrer;
	nsEmbedCString mContentDisposition;
	CBrowserFrame* mBrowser;
};

class CBrowserView : public CWnd
{
public:
	CBrowserView();
	virtual ~CBrowserView();

	// Some helper methods
	HRESULT CreateBrowser();
	HRESULT DestroyBrowser();
	void OpenURL(const char* pUrl, nsIURI *refURI=nsnull);
	void OpenURL(const PRUnichar* pUrl, nsIURI *refURI=nsnull);
	CString CBrowserView::NicknameLookup(const CString& typedUrl);
    //void OpenSingleURL(char *urls);
    void OpenMultiURL(LPTSTR urls);
	CBrowserFrame* CreateNewBrowserFrame(PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_ALL, 
							PRInt32 x = -1, PRInt32 y = -1, 
							PRInt32 cx = -1, PRInt32 cy = -1,
							PRBool bShowWindow = PR_TRUE);
	CBrowserFrame* OpenURLInNewWindow(const char* pUrl, BOOL bBackground=FALSE, nsIURI *refURI=nsnull);
    CBrowserFrame* OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground=FALSE, nsIURI *refURI=nsnull);
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
#ifdef INTERNAL_SITEICONS
	nsCOMPtr<CFavIconListener> mFavIconListener;
	nsCOMPtr<nsIURI> m_IconUri;
#endif
	// Mozilla interfaces
	//
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIBaseWindow>  mBaseWindow;
	nsCOMPtr<nsIWebNavigation> mWebNav;
	nsCOMPtr<nsIDOMEventTarget> mEventTarget;
	nsCOMPtr<nsIWebBrowserFocus> mWebBrowserFocus;
	
	BOOL m_bUrlJustEntered;

	void UpdateBusyState(PRBool aBusy);
	PRBool mbDocumentLoading;
	BOOL mbDOMLoaded;

    nsIDOMNode *GetNodeAtPoint(int x, int y, BOOL bPrepareMenu);
    int m_iGetNodeHack;
    nsCOMPtr<nsIDOMNode> m_pGetNode;
	nsCOMPtr<nsIDOMNode> m_lastMouseActionNode;
   
    nsIDOMWindow *FindDOMWindow(nsIDOMWindow *window, nsIDOMDocument *document);

    nsIDocShell *CBrowserView::GetDocShell();
    BOOL GetCharset(char* aCharset);
    BOOL ForceCharset(char *aCharSet);

    void SetCtxMenuLinkUrl(nsEmbedString& strLinkUrl);
	nsEmbedString mCtxMenuLinkUrl;

	void SetCtxMenuImageSrc(nsEmbedString& strImgSrc);
	nsEmbedString mCtxMenuImgSrc;

    void SetCurrentFrameURL(nsEmbedString& strcCurrentFrameURL);
    nsEmbedString mCtxMenuCurrentFrameURL;

	void Activate(BOOL bActive);

	BOOL OpenViewSourceWindow(const PRUnichar* pUrl);
    BOOL OpenViewSourceWindow(const char* pUrl);  
    BOOL IsViewSourceUrl(CString& strUrl);

    enum _securityState {
        SECURITY_STATE_SECURE,
        SECURITY_STATE_INSECURE,
        SECURITY_STATE_BROKEN
    };

    int m_SecurityState;
    void ShowSecurityInfo();
	BOOL GetCertificate(nsIX509Cert** certificate);
	BOOL GetSecurityInfo(CString &sign);

    BOOL ViewContentContainsFrames();

    void StartPanning();
    void StopPanning();
    BOOL m_panning;
    BOOL m_panningQuick;
    CPoint m_panningPoint;
    nsCOMPtr<nsIDOMWindow> s;
  
	afx_msg LRESULT RefreshToolBarItem(WPARAM ItemID, LPARAM unused);

    TCHAR * GetTempFile();
    void DeleteTempFiles();

    BOOL GetCurrentURI(CString& uri);

    inline void ClearFindDialog() { m_pFindDlg = NULL; }

   // void GetBrowserWindowTitle(nsEmbedString& title);

    BOOL URISaveAs(nsIURI *aURI, int bDocument=0);
	BOOL URISaveAs(PRUnichar* aURI , int bDocument=0);

    void OnIncreaseFont();
    void OnDecreaseFont();
    void ChangeTextSize(PRInt32 change);
	int GetTextSize();

	BOOL GetSelection(CString&);
	BOOL GetUSelection(nsEmbedString&);
	BOOL InjectCSS(const wchar_t* userStyleSheet);
	BOOL InjectJS(const wchar_t* userJS, bool bTopWindow = true);
    BOOL GetPrintSettings();
	BOOL CloneSHistory(CBrowserView& newWebBrowser);

	CString m_csHostPopupBlocked;

protected:
	BOOL _GetSelection(nsIDOMWindow* dom, nsAString& aSelText);
	BOOL _InjectCSS(nsIDOMWindow* dom, const wchar_t* userStyleSheet);
	void _OnNavReload(BOOL force = FALSE);
    void Highlight(BOOL);

	nsEmbedString m_lastHighlightWord;
	BOOL m_refreshBackButton;
	BOOL m_refreshForwardButton; 
    BOOL m_InPrintPreview;
    TCHAR **m_tempFileList;
    int m_tempFileCount;
    int maccel_cmd;
    int maccel_key;
    int maccel_pan;
	int m_iIcon;
  
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
	afx_msg void OnUrlSelectedInUrlBarOk();
	afx_msg void OnUrlSelectedInUrlBarCancel();
	afx_msg void OnUrlBarDropDown();
	afx_msg void OnNewUrlEnteredInUrlBar();
    afx_msg void OnUrlKillFocus();
	afx_msg void OnUrlSetFocus();
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
    afx_msg void OnToggleOffline();
	afx_msg void OnCut();
	afx_msg void OnCopy();
	afx_msg void OnPaste();
	afx_msg void OnSelectAll();
	afx_msg void OnSelectNone();
	afx_msg void OnCopyLinkLocation();
	afx_msg void OnCopyImageLocation();
    afx_msg void OnCopyImageContent();
    afx_msg void OnOpenLink();
    afx_msg void OnOpenLinkInNewWindow();
	afx_msg void OnOpenLinkInBackground();
	afx_msg void OnViewImageInNewWindow();
	afx_msg void OnSaveLinkAs();
	afx_msg void OnSaveImageAs();
	afx_msg void OnViewPageInfo();
	afx_msg void OnViewFrameInfo();
    //afx_msg void OnShowFindDlg();
    afx_msg void OnFilePrint();
    afx_msg void OnFilePrintPreview();
    afx_msg void OnFilePrintSetup();
    afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);
    afx_msg void OnFindNext();
    afx_msg void OnFindPrev();
    //afx_msg LRESULT OnFindMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnKmeleonHome();
    afx_msg void OnKmeleonForum();
    afx_msg void OnKmeleonFAQ();
    afx_msg void OnKmeleonManual();
    afx_msg void OnAboutPlugins();
	afx_msg void OnUpdateNavBack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavForward(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNavStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewImage(CCmdUI* pCmdUI);
	afx_msg void OnAppAbout();
	afx_msg void OnWindowNext();
	afx_msg void OnWindowPrev();
    afx_msg void OnViewFrameSource();
    afx_msg void OnOpenFrame();
    afx_msg void OnOpenFrameInBackground();
    afx_msg void OnOpenFrameInNewWindow();   
    afx_msg void OnMouseAction();
	afx_msg void OnWrapAround();
	afx_msg void OnMatchCase();
	afx_msg void OnHighlight();
	afx_msg void OnSecurityStateIcon();
	afx_msg void OnPopupBlockedIcon();

    afx_msg void OnEditURL( NMHDR * pNotifyStruct, LRESULT * result );
    afx_msg void OnDragURL( NMHDR * pNotifyStruct, LRESULT * result );
    //afx_msg void OnDropFiles( HDROP );
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

#endif //_BROWSERVIEW_H
