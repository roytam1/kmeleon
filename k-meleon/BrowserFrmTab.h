/*
*  Copyright (C) 2005 Dorian Boissonnade
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
*
*
*/

#pragma once

#define MAX_TABS_NUMBER				   100
#define	TABS_START_ID				   2300
#define TABS_STOP_ID				   TABS_START_ID + MAX_TABS_NUMBER - 1

#define WM_CLOSETAB						WM_APP + 150
#define WM_OPENTAB					    WM_APP + 151
#define WM_SWITCHTAB					WM_APP + 152

#define IDTOTABINDEX(ID) (ID - TABS_START_ID) 
#define TABINDEXTOID(index) (TABS_START_ID + index)

#include "BrowserView.h"
#include "BrowserFrm.h"
#include "TabRebar.h"

class CBrowserFrmTab;

class CBrowserTab : public CBrowserView
{
public:
	DECLARE_DYNAMIC(CBrowserTab)

	CBrowserTab();
	~CBrowserTab();
	BOOL Create(CBrowserFrmTab* aFrame, int index);
	void Destroy();

	inline bool IsActive() {return m_bActive;}
	inline bool SetActive(bool state, bool haveFocus=true);

	CBrowserTab*   OpenURLInNewTab(LPCTSTR url, LPCTSTR refferer = NULL, BOOL bBackground = FALSE, BOOL allowFixup = FALSE);
	CBrowserFrame* OpenURLInNewWindow(LPCTSTR url, LPCTSTR refferer = NULL, BOOL bBackground = FALSE, BOOL allowFixup = FALSE);
	virtual void OpenURLWithCommand(UINT idCommand, LPCTSTR url, LPCTSTR refferer = NULL, BOOL allowFixup = FALSE);
/*	void SetFrameGlue(PBROWSERFRAMEGLUE glue){
		m_wndView.SetBrowserFrameGlue(glue);
	}*/

	void SetIndex(int i);
	
	CString GetLocation()
	{
		if (m_typedLocation.GetLength())
			return m_typedLocation;
		return GetBrowserGlue()->mLocation;
	}

	void SetTypedLocation(CString location)
	{
		m_typedLocation = location;
	}
	
	//CBrowserView m_wndView;
	int m_iIndex;

protected:
/*
	class BrowserFrameTabGlueObj : public IBrowserFrameGlue 
    {
        //
        // NS_DECL_BROWSERFRAMEGLUE below is a macro which expands
        // to the function prototypes of methods in IBrowserFrameGlue
        // Take a look at IBrowserFrameGlue.h for this macro define
        //

        NS_DECL_BROWSERFRAMEGLUE

    } m_xBrowserFrameTabGlueObj;
    friend class BrowserFrameTabGlueObj;
*/
	PBROWSERFRAMEGLUE mpBrowserFrameTabGlue;

	bool m_bActive;
	CBrowserFrmTab* mpFrameTab;
	CString m_typedLocation;

	afx_msg void OnOpenFrameInNewTab();
	afx_msg void OnOpenFrameInBackgroundTab();
	afx_msg void OnOpenLinkInNewTab();
	afx_msg void OnOpenLinkInBackgroundTab();

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnCloseTab();
};

class CBrowserFrmTab : public CBrowserFrame
{

public:
	DECLARE_DYNAMIC(CBrowserFrmTab)

	CBrowserFrmTab(PRUint32 chromeMask, LONG style);
	~CBrowserFrmTab();

	void DrawTabListMenu(HMENU menu);
	void UpdateTabListMenu();

	CBrowserTab* CreateBrowserTab(bool first=false);
	void SetActiveBrowser(CBrowserTab*);
	BOOL SafeSetActiveBrowser(CBrowserTab*);
	BOOL CloseTab(CBrowserTab*);
	virtual CBrowserView* GetActiveView() { return (CBrowserView*)m_wndCBrowserTab; }
	CBrowserTab* GetActiveTab() { return m_wndCBrowserTab; }

	BOOL SetTabIcon(CBrowserTab* tab, int icon) {
		return m_wndTabs->SetItemImage(TABINDEXTOID(tab->m_iIndex), icon);
	}

	BOOL SetTabTitle(CBrowserTab* tab, CString title){
		UpdateTabListMenu();
		title.Replace(_T("&"), _T("&&"));
		return m_wndTabs->SetItemText(TABINDEXTOID(tab->m_iIndex), title);
	}

	BOOL GetTabTitle(CBrowserTab* tab, CString& title){
		return m_wndTabs->GetItemText(TABINDEXTOID(tab->m_iIndex), title);
	}
	
	int GetTabCount() {
		return m_iBrowserCount;
	}

	CBrowserTab* GetTabIndex(int index) {
		if (index<0||index>=m_iBrowserCount)
			return NULL;
		return m_Tabs[index];
	}

	virtual void RecalcLayout(BOOL bNotify = TRUE);

protected:

	CBrowserTab* m_wndCBrowserTab;
	CBrowserTab* m_Tabs[MAX_TABS_NUMBER];
	CBrowserTab* m_pPreviousSelectedTab;
	
	int m_iBrowserCount; // Number of tab
	int m_iCBrowserView; // Index of the current active tab
	CTabReBar*	m_wndTabs; // Toolbar with the tabs selector
	CString m_tabBarTip;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnCloseTab();
	afx_msg void OnCloseAllTab();
	afx_msg void OnCloseAllOtherTab();
	afx_msg LRESULT OnCloseTab(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenTab(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateTabs(CCmdUI*);
	afx_msg void OnNextTab();
	afx_msg void OnPrevTab();
	afx_msg void OnLastTab();
	afx_msg void OnTabSelect(UINT id);
	afx_msg LRESULT OnGetFavIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewSiteIcon(WPARAM url, LPARAM index);
	afx_msg void OnTbnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnShowFindBar();
public: // Temporary
	afx_msg void OnNewTab();
};


