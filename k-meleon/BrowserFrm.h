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

#include "BrowserView.h"
#include "IBrowserFrameGlue.h"
#include "MostRecentUrls.h"
#include "ToolBarEx.h"
#include "KmeleonConst.h"
#include "ReBarEx.h"
#include "Tooltips.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;


// A simple UrlBar class...
class CUrlBar : public CComboBoxEx 
{
public:

    HWND m_hwndEdit;
   
   
    int Create(DWORD style, RECT &rect, CWnd *parentWnd, UINT id) {
        int ret = CComboBoxEx::Create(style | CBS_AUTOHSCROLL, rect, parentWnd, id);
        SetExtendedStyle(CBES_EX_CASESENSITIVE, CBES_EX_CASESENSITIVE);
      
        COMBOBOXEXITEM ci;
        ci.mask = CBEIF_IMAGE;
        ci.iItem = -1;
        ci.iImage = 15;
        SetItem(&ci);
      
        m_bFocusEnabled = theApp.preferences.bNewWindowHasUrlFocus;
        m_preserveUrlBarFocus = FALSE;
        m_changed = FALSE;
        m_bSelected = FALSE;
        m_iFocusCount = 0;

        CEdit *edit = GetEditCtrl();
        if (edit)
            m_hwndEdit = edit->m_hWnd;
      
        return ret;
    }
    inline void GetEnteredURL(CString& url) {
        GetEditCtrl()->GetWindowText(url);
    }
    inline void GetSelectedURL(CString& url) {
        GetLBText(GetCurSel(), url);
        m_bSelected = TRUE;
    }   
    inline void SetCurrentURL(LPCTSTR pUrl) {
        if (!m_changed) {
            SetWindowText(pUrl);
            m_changed = FALSE;
        }
    }   
    inline void AddURLToList(CString& url, bool bAddToMRUList = true) {
        USES_CONVERSION;
        COMBOBOXEXITEM ci;
        ci.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        ci.iItem = 0;
        ci.iImage = 15;
        ci.iSelectedImage = 15;
        ci.pszText = const_cast<TCHAR *>((LPCTSTR)url);
        InsertItem(&ci);
      
        if(bAddToMRUList) {
            m_MRUList.AddURL((LPTSTR)(LPCTSTR)url);
            ResetContent();
            LoadMRUList();
        }
    }
    inline void LoadMRUList() {
        for (int i=m_MRUList.GetURLCount()-1;i>=0;i--) {
            USES_CONVERSION;
            CString urlStr(A2CT(m_MRUList.GetURL(i)));
            AddURLToList(urlStr, false);
        }
    }   
    void MaintainFocus() {
        if (m_bFocusEnabled) {
            // SetFocus();
            m_preserveUrlBarFocus = TRUE;
            m_iFocusCount = 0;
        }
    }   
    BOOL CheckFocus() {
        return m_preserveUrlBarFocus;
    }   
    void ReturnFocus(BOOL bDocumentLoading) {
        // SetFocus();
        if (m_changed)
            GetEditCtrl()->SetSel(-1, 0);

        // mozilla always tries to steal focus twice
        if (++m_iFocusCount >= 2)
            m_preserveUrlBarFocus = FALSE;
    }
    void EndFocus() {
        m_preserveUrlBarFocus = FALSE;
    }
    inline void EditChanged(BOOL state) {
        if (m_bSelected) {
            m_changed = FALSE;
            m_bSelected = FALSE;
        }
        else m_changed = state;
    }

protected:
    CMostRecentUrls m_MRUList;
    BOOL m_preserveUrlBarFocus;
    BOOL m_changed;
    BOOL m_bSelected;
    BOOL m_bFocusEnabled;
    int  m_iFocusCount;
};

// CMyStatusBar class
class CMyStatusBar : public CStatusBar
{
public:
    CMyStatusBar();
    virtual ~CMyStatusBar();

protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()
};

class CToolBarList;
class CToolBarItem {
friend class CToolBarList;
public:
    CToolBarItem(CWnd *wnd, UINT style)
    {
        m_tb = new CToolBarEx;
        m_tb->CreateEx (wnd, style);
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
public:
    CBrowserFrame(PRUint32 chromeMask, LONG style);

protected: 
    DECLARE_DYNAMIC(CBrowserFrame)

public:
    inline CBrowserImpl *GetBrowserImpl() { return m_wndBrowserView.mpBrowserImpl; }

    CMyStatusBar    m_wndStatusBar;
    CProgressCtrl   m_wndProgressBar;
    CUrlBar         m_wndUrlBar;
    CReBarEx        m_wndReBar;
    CAnimateCtrl     m_wndAnimate;

    CBitmap         m_bmpBack;

    // The view inside which the embedded browser will
    // be displayed in
    CBrowserView   m_wndBrowserView;

    void UpdateSecurityStatus(PRInt32 aState);
    void ShowSecurityInfo();

    // note: right now it's just a CStatic, but eventually it will become something better
    CKmToolTip     m_wndToolTip;

    // This specifies what UI elements this frame will sport
    // w.r.t. toolbar, statusbar, urlbar etc.
    PRUint32 m_chromeMask;
    LONG m_style;
    int m_ignoreMoveResize;

    BOOL m_created; // set after we are created
    INT m_ignoreFocus;

    CToolBarList m_tbList;
    HWND CreateToolbar(UINT style);


    BOOL Create(LPCTSTR lpszClassName,     LPCTSTR lpszWindowName,
       DWORD dwStyle,      const RECT& rect,       CWnd* pParentWnd,
       LPCTSTR lpszMenuName,       DWORD dwExStyle,    CCreateContext* pContext);

protected:
    //
    // This nested class implements the IBrowserFramGlue interface
    // The Gecko embedding interfaces call on this interface
    // to update the status bars etc.
    //
    class BrowserFrameGlueObj : public IBrowserFrameGlue 
    {
        //
        // NS_DECL_BROWSERFRAMEGLUE below is a macro which expands
        // to the function prototypes of methods in IBrowserFrameGlue
        // Take a look at IBrowserFrameGlue.h for this macro define
        //

        NS_DECL_BROWSERFRAMEGLUE

    } m_xBrowserFrameGlueObj;
    friend class BrowserFrameGlueObj;

public:
    void SetupFrameChrome();

    void LoadBackImage ();
    void SetBackImage ();

    void SaveWindowPos();
    void RestoreWindowPos(PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBrowserFrame)
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CBrowserFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
    //{{AFX_MSG(CBrowserFrame)
    afx_msg void OnClose();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    // afx_msg void OnMove(int x, int y);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnSysColorChange();
    afx_msg void RefreshToolBarItem(WPARAM ItemID, LPARAM unused);
    afx_msg void ToggleToolBar(UINT uID);
    afx_msg void ToggleToolbarLock();
    afx_msg void OnUpdateToggleToolbarLock(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //_IBROWSERFRM_H
