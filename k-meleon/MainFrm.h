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

#if !defined(AFX_MAINFRM_H__F83C8E0F_F33E_11D2_A713_0090274409AC__INCLUDED_)
#define AFX_MAINFRM_H__F83C8E0F_F33E_11D2_A713_0090274409AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CFrameWnd CBCGFrameWnd

#include "MainToolBar.h"
#include "LinksBar.h"

#include "Preferences.h"
#include "MenuParser.h"

class CMainFrame : public CFrameWnd
{
public:
  CMainFrame();

protected: // create from serialization only
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	HIMAGELIST          m_himSystem;
	CSize               m_SysImageSize;

protected:
	int                 m_iFolderIcon;
	int                 m_iInternetShortcutIcon;

	CBitmap             m_bmpBack;

	int                 m_iAnimationStarted;

	//  BHarris - save old status for onMouseOut
	CString             oldStatus;

  CMenuParser         m_menuParse;

public:
	int setupBookmarks();
	int BuildFavoritesMenu(LPCTSTR pszPath, int nStartPos, CMenu* pMenu);

	void SetAddress(LPCTSTR lpszUrl);
	void StartAnimation();
	void StopAnimation();
	BOOL IsFavoritesMenu (const CBCGToolbarMenuButton* pMenuButton);

	virtual BOOL OnShowPopupMenu (CBCGPopupMenu* pMenuPopup);
	virtual BOOL OnDrawMenuImage (CDC* pDC,
									const CBCGToolbarMenuButton* pMenuButton, 
									const CRect& rectImage);

	virtual BOOL OnMenuButtonToolHitTest (CBCGToolbarButton* pButton, TOOLINFO* pTI);
	virtual BOOL GetToolbarButtonToolTipText (CBCGToolbarButton* pButton, CString& strTTText);

	void LoadBackImage ();
	void SetBackImage ();

	void onPageTitleChange(char *title);
	void onOverLink(char *link);
	void onJSStatus(char *status);
	void onJSDefaultStatus(char *status);
	void onSizeBrowserTo(int cx, int cy);
	void onProgressChange(int percentage);
  void onPopup(int flags);

	void *createNewBrowser();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CBCGMenuBar		m_wndMenuBar;
	CStatusBar		m_wndStatusBar;
	CMainToolBar	m_wndToolBar;
	CLinksBar		m_wndLinksBar;
	CReBar			m_wndReBar;
	CAnimateCtrl	m_wndAnimate;
	CComboBoxEx		m_wndAddress;

	BOOL			m_bMainToolbarMenu;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLink1();
	afx_msg void OnViewAddressBar();
	afx_msg void OnUpdateViewAddressBar(CCmdUI* pCmdUI);
	afx_msg void OnViewLinksBar();
	afx_msg void OnUpdateViewLinksBar(CCmdUI* pCmdUI);
	afx_msg void OnViewTextlabels();
	afx_msg void OnUpdateViewTextlabels(CCmdUI* pCmdUI);
	afx_msg void OnViewBackground();
	afx_msg void OnUpdateViewBackground(CCmdUI* pCmdUI);
	afx_msg void OnSysColorChange();
	afx_msg void OnHelpWebKmeleonHome();
	afx_msg void OnHelpWebKmeleonForum();
	afx_msg void OnGoStartPage();
	afx_msg void OnGoSearchTheWeb();
	afx_msg void OnFileNewwindow();
	//}}AFX_MSG
	afx_msg void OnViewCustomize();
  afx_msg void OnViewPreferences();
	afx_msg LRESULT OnToolbarReset(WPARAM,LPARAM);
	afx_msg LRESULT OnToolbarContextMenu(WPARAM,LPARAM);
	afx_msg LRESULT OnHelpCustomizeToolbars(WPARAM wp, LPARAM lp);
	afx_msg void OnNewAddress();
	afx_msg void OnNewAddressEnter();
	afx_msg void OnFavorite(UINT nID);
	afx_msg void OnHistory(UINT nID);

  afx_msg void OnDestroy();

  afx_msg void OnGenericCommand(UINT nID);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__F83C8E0F_F33E_11D2_A713_0090274409AC__INCLUDED_)
