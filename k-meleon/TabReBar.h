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

#define PREFERENCE_REBAR_TITLE		"kmeleon.tabs.title"
#define PREFERENCE_BUTTON_MINWIDTH  "kmeleon.tabs.minWidth"
#define PREFERENCE_BUTTON_MAXWIDTH  "kmeleon.tabs.maxWidth"
#define PREFERENCE_BUTTON_STYLE		"kmeleon.tabs.style"
#define PREFERENCE_REBAR_BOTTOM		"kmeleon.tabs.bottomBar"
#define PREFERENCE_REBAR_FIXED		"kmeleon.tabs.fixedBar"

// CTabReBar

#include "stdafx.h"
#include "ReBarEx.h"
#include "resource.h"

#define CFSTR_TABPT TEXT("KmeleonTabPointer")

class CTBOleDropTarget : public COleDropTarget
{
	UINT cfShellURL;
	UINT cfTabPt;

	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point) {
			if (pDataObject->IsDataAvailable(cfShellURL, NULL))
				return DROPEFFECT_LINK;
			return DROPEFFECT_NONE;
		}

	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);

	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
		DROPEFFECT dropEffect, CPoint point);

	virtual DROPEFFECT OnDropEx(CWnd* pWnd, COleDataObject* pDataObject,
		DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point) {
			return (DROPEFFECT)-1;
		}

	virtual void OnDragLeave(CWnd* pWnd) {
		}

	virtual DROPEFFECT OnDragScroll(CWnd* pWnd, DWORD dwKeyState,
		CPoint point) {
			return DROPEFFECT_NONE;
		}

public:
	CTBOleDropTarget() {
		cfShellURL = ::RegisterClipboardFormat(CFSTR_SHELLURL);
		cfTabPt = ::RegisterClipboardFormat(CFSTR_TABPT);
	}

};

class CTabReBar : public CToolBar
{
	DECLARE_DYNAMIC(CTabReBar)
	friend CTBOleDropTarget;
public:
	CTabReBar();
	virtual ~CTabReBar();
	BOOL Create(CReBarEx* rebar, UINT idwnd = AFX_IDW_TOOLBAR);
	BOOL Init(CReBarEx* rebar);
	LONG InsertItem(int nItem, int idCommand, LPCTSTR lpszItem, DWORD data, int nImage=0);
	//BOOL SetItemInfo(int nItem, LPTSTR lpszItem, int nImage=0);
	BOOL SetItemText(int idCommand, LPCTSTR lpszItem);
	BOOL SetItemImage(int idCommand, int nImage);
	int  GetItemImage(int idCommand);
	BOOL SetItemCommandID(int idCommand, int newidCommand);

	int GetNextItem(int idCommand, bool strict=true);
	int GetPreviousItem(int idCommand, bool strict=true);

	BOOL SelectItem(int idCommand);

	//UINT GetItemCommandID(int index);
	BOOL DeleteItem(int idCommand);
	int FindById(int idCommand);
	int FindByData(DWORD_PTR data);
	CMenu* m_Menu;

	BOOL GetItemText(int idCommand, CString& str);
	void RefreshFavIcon();
	int GetButtonIDFromPoint(POINT point); // ....
	void UpdateVisibility(BOOL canHide = TRUE);

protected:
	
	BOOL bRebarEnabled ;
	TCHAR szTitle[MAX_PATH > 256 ? MAX_PATH : 256];
	int  mButtonStyle;
	BOOL bButtonNumbers;
	int  mDragItem;
	POINT mDragPoint;
	CTBOleDropTarget mDropTarget;
	BOOL mBottomBar;
	BOOL mFixedBar;
	BOOL mChevron;
	
	int GetButtonIDFromData(DWORD data);
	void UpdateButtonsSize();
	
	void HandleMouseClick(int flag, CPoint point);
	
	CReBarEx* m_wndParent;
	CReBarEx* mTemp;
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnTbnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTbnEndDrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};



