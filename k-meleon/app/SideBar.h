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

#include "KMeleonConst.h"
//#include "Kmeleon_plugin.h"
typedef  HWND(*SideBarInitProc)(HWND);

// CSideBar


#define MINSIDEBAR_SIZE 100

class CSideBar : public CWnd
{
	DECLARE_DYNAMIC(CSideBar)

public:
	CSideBar();
	virtual ~CSideBar();

	void Show(int index);
	void Hide();
	void DrawSideBarMenu(HMENU menu);
	void SetVisibility(int index, bool visible);
	void ToggleVisibility(int index);
	UINT GetCurrent() {return m_iCurrent + SIDEBAR_MENU_START_ID;}
	int RegisterSideBar(TCHAR* name, SideBarInitProc proc, UINT commandID, int visibleOnMenu);

	static LONG pOldProc;

	int m_wSplitter; // Width of the splitter
	int m_iWidth; // Width of the entire bar
	int m_iMaxWidth; // Max width possible (parent size)
	bool m_bTracking; // Tracking mouse for resizing
	bool m_bVisible;
	bool m_bTimer; // Timer for smooth resizing. 
	HMENU m_menu;
	int m_iTitleHeight;
	int m_wBorder; // Border size (only 0 or 2)

	CReBar m_wndRebar;

protected:
	void SetResizeCursor(BOOL b);
	BOOL IsonBar(POINT pt);

	CToolBar closeBar;

	struct stBars {
		UINT uID;
		UINT commandID;
		SideBarInitProc proc;
		TCHAR *name;

		HWND child;
		BOOL visibleOnMenu;
	} **m_iBars;
	int m_iCount;
	int m_iCurrent; // Current visible bar

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnCancelMode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg LRESULT OnSizeParent(WPARAM, LPARAM);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
};


