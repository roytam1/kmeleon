/*
*  Copyright (C) 2000 Christophe Thibault
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#if !defined(AFX_MOZILLA_H__D1EE282F_F28C_4C33_AFA5_30F9DB478339__INCLUDED_)
#define AFX_MOZILLA_H__D1EE282F_F28C_4C33_AFA5_30F9DB478339__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WebBrowserChrome.h"

// Mozilla.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMozilla window

class CMozilla : public CView
{
// Construction
public:
	CMozilla();
	DECLARE_DYNCREATE(CMozilla)

// Attributes
public:

protected:
	nsIWebBrowserChrome *chrome;
  nsIWebBrowser *mBrowser;
	nsIWebNavigation *mWebNav;

	int panning;
	POINT panning_point;
  char last_link[2048];

// Operations
public:
	void *createBrowser();
	int ResizeEmbedding();

  void SetLastLink(char *link);

  void StartPanning();
  void StopPanning();

  void OnPopup(int flags);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMozilla)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view

// Implementation
public:
	virtual ~CMozilla();

	int Navigate(CString *url);

	// Generated message map functions
protected:
	//{{AFX_MSG(CMozilla)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGoBack();
	afx_msg void OnUpdateGoBack(CCmdUI* pCmdUI);
	afx_msg void OnGoForward();
	afx_msg void OnUpdateGoForward(CCmdUI* pCmdUI);
	afx_msg void OnViewStop();
	afx_msg void OnUpdateViewStop(CCmdUI* pCmdUI);
	afx_msg void OnViewRefresh();
	afx_msg void OnUpdateViewRefresh(CCmdUI* pCmdUI);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLinkOpen();
	afx_msg void OnLinkOpenNewWindow();
	afx_msg void OnPopupCopyShortcut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOZILLA_H__D1EE282F_F28C_4C33_AFA5_30F9DB478339__INCLUDED_)
