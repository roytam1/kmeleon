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
#include "BrowserWindow.h"
#include "MozUtils.h"

/////////////////////////////////////////////////////////////////////////////
// CBrowserView window

class CBrowserFrame;
class CBrowserGlue;
class CBrowserImpl;
class CFavIconListener;
class CBrowserWrapper;
class CFindDialog;
class CPrintProgressDialog;

typedef enum
{
	CONTEXT_NONE		= 0,
    CONTEXT_LINK		= 1, 
    CONTEXT_IMAGE		= 2,
    CONTEXT_DOCUMENT	= 4,
    CONTEXT_TEXT		= 8,
    CONTEXT_INPUT		= 16,
	CONTEXT_BACKGROUND_IMAGE = 32,
	CONTEXT_FRAME		= 64,
	CONTEXT_SELECTION   = 128
} ContextFlags;

	struct SContextData
	{
		PRInt32 flags;
		nsCOMPtr<nsIDOMNode> node;
		//nsCOMPtr<nsIDOMEvent> event;

		BOOL contextMenu;
		CString linkUrl;
		CString imageUrl;
		CString frameUrl;

		SContextData() : node(nsnull), /*event(nsnull),*/ contextMenu(FALSE), flags(0) {}
	};


class CBrowserView : public CWnd
{

public:

	DECLARE_DYNAMIC(CBrowserView)

	/*** XXXXXXXXXXXXXXXXX  */

	CString GetCurrentURI() { return m_pWindow->GetURI(); }
	CString GetPageTitle() { return m_pWindow->GetTitle(); }

	nsCOMPtr<nsIDOMNode> m_contextNode;
	CString GetContextLinkUrl() {
		CString url, title;
		::GetLinkTitleAndHref(m_contextNode, url, title);
		return url;
	}
		
	CString GetContextLinkTitle() {
		CString url, title;
		::GetLinkTitleAndHref(m_contextNode, url, title);
		return title;
	}

	CString GetContextImageUrl() {
		CString imgSrc;
		if (!::GetImageSrc(m_contextNode, imgSrc))
			::GetBackgroundImageSrc(m_contextNode, imgSrc);
		return imgSrc;
	}

	CString GetContextFrameUrl() {
		return m_pWindow->GetFrameURL(m_contextNode);
	}

	//nsCOMPtr<nsIURI> m_IconUri;
	int GetSiteIcon(); 

	void Highlight(const wchar_t* string, BOOL matchCase);
	/**************************/

protected:
	PBROWSERGLUE m_pBrowserGlue;
	CBrowserFrame* mpBrowserFrame;

public:
	PBROWSERFRAMEGLUE m_pBrowserFrameGlue;
	
	CBrowserView();
	virtual ~CBrowserView();

	CString NicknameLookup(const CString& typedUrl);
	//void OpenURL(LPCTSTR url, BOOL sendRef = FALSE, BOOL allowFixup = FALSE);
	virtual void OpenURL(LPCTSTR url, LPCTSTR refferer = NULL, BOOL allowFixup = FALSE);
	virtual void OpenMultiURL(LPCTSTR urls, BOOL allowFixup = FALSE);
	virtual void OpenURLWithCommand(UINT idCommand, LPCTSTR url, LPCTSTR refferer = NULL, BOOL allowFixup = FALSE);

	//CBrowserFrame* OpenURLInNewWindow(LPCTSTR url, BOOL bBackground=FALSE, BOOL sendRef = FALSE, BOOL allowFixup = FALSE);
	virtual CBrowserFrame* OpenURLInNewWindow(LPCTSTR url, LPCTSTR refferer = NULL, BOOL bBackground=FALSE, BOOL allowFixup = FALSE);
	void LoadHomePage();

	CBrowserWrapper* GetBrowserWrapper() { return this == NULL ? NULL : m_pWindow; }
	CBrowserGlue*    GetBrowserGlue() { ASSERT(this!=NULL); return (CBrowserGlue*)m_pBrowserGlue; }

	BOOL CloneBrowser(CBrowserView* browserView) { return m_pWindow->CloneSHistory(browserView->m_pWindow); }

	// Called by the CBrowserFrame after it creates the view
	// Essentially a back pointer to the BrowserFrame
	void SetBrowserFrame(CBrowserFrame* pBrowserFrame);

	// Called by the CBrowserFrame after it creates the view
	// The view passes this on to the embedded Browser's Impl
	// obj
	void SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue);
	void SetBrowserGlue(PBROWSERGLUE pBrowserGlue);
	
	

//    nsIDOMWindow *FindDOMWindow(nsIDOMWindow *window, nsIDOMDocument *document);

/*    void SetCtxMenuLinkUrl(nsEmbedString& strLinkUrl);
	nsEmbedString mCtxMenuLinkUrl;

	void SetCtxMenuImageSrc(nsEmbedString& strImgSrc);
	nsEmbedString mCtxMenuImgSrc;

    void SetCurrentFrameURL(nsEmbedString& strcCurrentFrameURL);
    nsEmbedString mCtxMenuCurrentFrameURL;*/

	void Activate(BOOL bActive);

	//BOOL OpenViewSourceWindow(const PRUnichar* pUrl);
    BOOL OpenViewSourceWindow(BOOL frame = FALSE);  
    BOOL IsViewSourceUrl(CString& strUrl);
	BOOL SaveLink(LPCTSTR url);

/*    enum _securityState {
        SECURITY_STATE_SECURE,
        SECURITY_STATE_INSECURE,
        SECURITY_STATE_BROKEN
    };*/
    
    void ShowSecurityInfo();
    void StartPanning(BOOL accel);
    void StopPanning();

	//afx_msg LRESULT RefreshToolBarItem(WPARAM ItemID, LPARAM unused);

    TCHAR * GetTempFile();
    void DeleteTempFiles();

  //  inline void ClearFindDialog() { m_pFindDlg = NULL; }
	void AddURLAndPerformDrag(COleDataSource& datasource);

protected:
	void _OnNavReload(BOOL force = FALSE);
	
	virtual void PostNcDestroy() {
		delete this;
	}
   

	CBrowserWrapper* m_pWindow;
	nsEmbedString m_lastHighlightWord;
//	BOOL m_refreshBackButton;
//	BOOL m_refreshForwardButton; 
    BOOL m_InPrintPreview;
    TCHAR **m_tempFileList;
    int m_tempFileCount;
    
    BOOL maccel_pan;
	BOOL m_panning;
    BOOL m_panningQuick;
    CPoint m_panningPoint;
	
	
  
   // CFindDialog* m_pFindDlg;
   // CPrintProgressDialog* m_pPrintProgressDlg;

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
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnDelete();
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
	afx_msg void OnUpdateUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRedo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewImage(CCmdUI* pCmdUI);
    afx_msg void OnViewFrameSource();
    afx_msg void OnOpenFrame();
    afx_msg void OnOpenFrameInBackground();
    afx_msg void OnOpenFrameInNewWindow();   
    afx_msg void OnMouseAction();
	afx_msg void OnSecurityStateIcon();
	afx_msg void OnPopupBlockedIcon();
	afx_msg void OnIncreaseFont();
    afx_msg void OnDecreaseFont();
	afx_msg void OnIncreaseFullZoom();
	afx_msg void OnDecreaseFullZoom();
    afx_msg void OnBeginEditURL( NMHDR * pNotifyStruct, LRESULT * result );
	afx_msg void OnEndEditURL( NMHDR * pNotifyStruct, LRESULT * result );
    afx_msg void OnDragURL( NMHDR * pNotifyStruct, LRESULT * result );
    //afx_msg void OnDropFiles( HDROP );
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

#endif //_BROWSERVIEW_H
