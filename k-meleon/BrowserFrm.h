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

// BrowserFrm.h : interface of the CBrowserFrame class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _IBROWSERFRM_H
#define _IBROWSERFRM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IBrowserFrameGlue.h"
#include "ToolBarEx.h"
#include "KmeleonConst.h"
#include "ReBarEx.h"
#include "Tooltips.h"
#include "urlbar.h"
#include "sidebar.h"
#include "Dialogs.h"

class CBrowserView;

class CBrowserGlue : public IBrowserGlue
{
public:
	CString mTitle;
	CString mLocation;
	CString mStatusText;
	CString mPopupBlockedHost;
	CString mPendingLocation;
	int     mIcon;
	int     mSecurityState;
	BOOL    mLoading;
	BOOL    mDOMLoaded;
	int     mProgressCurrent;
	int     mProgressMax;

	nsCOMPtr<nsIURI> mIconURI;
	nsCOMPtr<nsIDOMNode> mContextNode;

	CBrowserGlue(CBrowserFrame* frame, CBrowserView* view) : mIcon(0),
		mSecurityState(nsIWebProgressListener::STATE_IS_INSECURE),
		mProgressCurrent(0),
		mProgressMax(100),
		mLoading(FALSE),
		mDOMLoaded(FALSE),
		mpBrowserFrame(frame),
		mpBrowserView(view)
	{
	}

	virtual ~CBrowserGlue();

	//SetFrame(CBrowserFrame* frame)  {mpBrowserFrame = }

	NS_DECL_BROWSERGLUE;

protected:
	CBrowserFrame* mpBrowserFrame;
	CBrowserView* mpBrowserView;
};

// CMyStatusBar class
class CMyStatusBar : public CStatusBar
{
public:
    CMyStatusBar();
    virtual ~CMyStatusBar();

	BOOL RemoveIcon(UINT nID);
	BOOL AddIcon(UINT nID);
	BOOL SetIconInfo(UINT nID, HICON hIcon, LPCTSTR tpText = NULL);
	void GetItemRect(UINT i, LPRECT r);

protected:
	struct icon_info 
	{
		UINT    nID; 
		HICON	hIcon;
		LONG	lWidth;
		CString csTpText;
	};

    CArray<struct icon_info, struct icon_info> arrIcons;
 	CFont m_statusFont;

	void RefreshPanes();
   int HitTest(POINT point);
	
    DECLARE_MESSAGE_MAP()

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
   afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
   afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
   afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
};

class CToolBarList;
class CToolBarItem {
friend class CToolBarList;
public:
    CToolBarItem(CWnd *wnd, UINT style)
    {
        m_tb = new CToolBarEx;
		int sstyle = WS_CHILD|WS_VISIBLE| (style&CCS_BOTTOM ? CBRS_ALIGN_BOTTOM : CBRS_ALIGN_TOP);
        m_tb->CreateEx(wnd, style, sstyle);
        m_next = NULL;
    }
    ~CToolBarItem() {
        delete m_tb;
    }
private:
    CToolBarEx    *m_tb;
    CToolBarItem  *m_next;
};

class CToolBarList {
public:
    CToolBarList() {
        m_head = NULL;
        m_tail = NULL;
    };
    ~CToolBarList() {
        CToolBarItem *cur=m_head, *temp;
        while (cur) {
            temp = cur;
            cur = cur->m_next;
            delete temp;
        }
    }
    HWND Add(CWnd *wnd, UINT style)
    {
        CToolBarItem *newTB = new CToolBarItem(wnd, style);

        if (!m_head)
            m_head = m_tail = newTB;
        else {
            m_tail->m_next = newTB;
            m_tail = newTB;
        }
        return newTB->m_tb->m_hWnd;
    }
private:
    CToolBarItem  *m_head;
    CToolBarItem  *m_tail;
};



class CBrowserFrame : public CFrameWnd
{   
protected:
    // The view inside which the embedded browser will
    // be displayed in
	CBrowserView*	m_wndBrowserView;	

	CAnimateCtrl    m_wndAnimate;
	CFindRebar*     m_wndFindBar;
	wchar_t*        m_searchString;

    HICON           m_hSecurityIcon;
    
	static CBitmap  m_bmpBack;

	// This specifies what UI elements this frame will support
    // w.r.t. toolbar, statusbar, urlbar etc.
    PRUint32 m_chromeMask;
    LONG m_style;

    BOOL m_created; // set after we are created
    CToolBarList m_tbList;

	int m_cx;
	int m_cy;

public:
	CUrlBar         m_wndUrlBar;
	// note: right now it's just a CStatic, but eventually it will become something better
	CKmToolTip      m_wndToolTip;
    CReBarEx        m_wndReBar;
    CMyStatusBar    m_wndStatusBar;
	CProgressCtrl   m_wndProgressBar;
	HWND            m_wndLastFocused;
    
#ifdef INTERNAL_SIDEBAR
    CSideBar        m_wndSideBar;
#endif

	friend CBrowserGlue;
    DECLARE_DYNAMIC(CBrowserFrame);

	CBrowserFrame(PRUint32 chromeMask, LONG style);
	virtual ~CBrowserFrame();
	
	void OpenURL(LPCTSTR url, LPCTSTR refferer = NULL, BOOL focusUrl = FALSE, BOOL allowFixup = TRUE);

	virtual CBrowserView* GetActiveView() { return m_wndBrowserView; }
	BOOL IsDialog() { return (m_chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME); }
	BOOL IsPopup() { return (m_style & WS_POPUP); } 

    void UpdateSecurityStatus(PRInt32 aState);
	void UpdateStatus(LPCTSTR aStatus);
	void UpdateSiteIcon(int aIcon);
	void UpdateLocation(LPCTSTR aLocation, BOOL aIgnoreTyping = FALSE);
	void UpdateProgress(int aCurrent, int aMax); 
	void UpdateLoading(BOOL aLoading);
	void UpdateTitle(LPCTSTR aTitle);
	void UpdatePopupNotification(LPCTSTR uri);

	void ClearFindBar();
	void CloseNothing(){}
	INT_PTR DoModal();
	
    HWND CreateToolbar(UINT style);

	

protected:
	int InitLayout();
	void SetupFrameChrome();

	void LoadBackImage ();
	void SetBackImage ();
	void SaveWindowPos();
#ifdef INTERNAL_SITEICONS
	void SetFavIcon(int iIcon);
#endif

	//
    // This nested class implements the IBrowserFramGlue interface
    // The Gecko embedding interfaces call on this interface
    // to update the status bars etc.
    //
   /* class BrowserFrameGlueObj : public IBrowserFrameGlue 
    {
        //
        // NS_DECL_BROWSERFRAMEGLUE below is a macro which expands
        // to the function prototypes of methods in IBrowserFrameGlue
        // Take a look at IBrowserFrameGlue.h for this macro define
        //

        NS_DECL_BROWSERFRAMEGLUE

    } m_xBrowserFrameGlueObj;
    friend class BrowserFrameGlueObj;*/

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBrowserFrame)
	virtual HACCEL GetDefaultAccelerator();
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL
   
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

	afx_msg void OnShowFindBar();

protected:
// Generated message map functions
    //{{AFX_MSG(CBrowserFrame)
    afx_msg void OnClose();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    // afx_msg void OnMove(int x, int y);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnSysColorChange();
	
	afx_msg void OnSelectUrl();
	afx_msg void OnWindowNext();
	afx_msg void OnWindowPrev();
    afx_msg LRESULT RefreshToolBarItem(WPARAM ItemID, LPARAM unused);
    afx_msg LRESULT RefreshMRUList(WPARAM ItemID, LPARAM unused);
    afx_msg void ToggleToolBar(UINT uID);
    afx_msg void ToggleToolbarLock();
    afx_msg void OnUpdateToggleToolbarLock(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolBarMenu(CCmdUI*);
#ifdef INTERNAL_SIDEBAR
	afx_msg void ToggleSideBar(UINT uID);
	afx_msg void OnUpdateSideBarMenu(CCmdUI*);
#endif
#ifdef INTERNAL_SITEICONS
	afx_msg LRESULT OnNewSiteIcon(WPARAM url, LPARAM index);
#endif
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg LRESULT OnEnterSizeMove(WPARAM, LPARAM); 
   afx_msg void OnRbnLayoutChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);

    afx_msg void OnFindNext();
    afx_msg void OnFindPrev();
	afx_msg void OnWrapAround();
	afx_msg void OnMatchCase();
	afx_msg void OnHighlight();

   afx_msg void OnCookiesViewer();
   afx_msg void OnPasswordsViewer();
   afx_msg void OnCookiePermissions();
   afx_msg void OnImagePermissions();
   afx_msg void OnPopupPermissions();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //_IBROWSERFRM_H
