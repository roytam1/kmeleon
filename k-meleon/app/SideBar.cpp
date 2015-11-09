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

#include "stdafx.h"
#include "SideBar.h"
#include "BrowserFrm.h"

// CSideBar
#ifdef INTERNAL_SIDEBAR

IMPLEMENT_DYNAMIC(CSideBar, CWnd)

CSideBar::CSideBar()
{
	m_iMaxWidth = m_iWidth = theApp.preferences.GetInt("kmeleon.sidebar.width", 200);
	m_wSplitter = 5;
	m_bVisible = false;
	m_bTracking = false;
	m_bTimer = false;
	m_iCount = 0;
	m_iCurrent = -1;
	m_iBars = NULL;
	m_wBorder = 0; // Currently not putting border
}

CSideBar::~CSideBar()
{
	if (m_iCount) {
		for (int x=0; x<m_iCount; x++) {
			if (m_iBars[x]->name)
				delete m_iBars[x]->name;
			delete m_iBars[x];
		}
		delete m_iBars;
	}
}

void CSideBar::Hide()
{
	SetVisibility(m_iCurrent, false);
	ShowWindow(SW_HIDE);
	m_bVisible = false;
	theApp.preferences.SetBool("kmeleon.sidebar.visible", false);
	GetParentFrame()->RecalcLayout();
}


void CSideBar::Show(int index)
{
	ASSERT(index>=0 && index<m_iCount);

	//   theApp.plugins.SendMessage("*", "* SideBar", "DoSidebar", (long)m_wndSideBar.m_hWnd, &m_iBars[index]->child);

	// Initialise the child window bar only when the user ask for it
	if (!m_iBars[index]->child&&m_iBars[index]->proc)
		m_iBars[index]->child = m_iBars[index]->proc(m_hWnd);

	// No child to display, no sidebar
	if (!IsWindow(m_iBars[index]->child))
		return;

	if (!m_bVisible)
	{
		m_bVisible = true;
		ShowWindow(SW_SHOW);
		SetVisibility(index, true);
		GetParentFrame()->RecalcLayout();
	}
	else
	{
		SetVisibility(m_iCurrent, false);
		SetVisibility(index, true);
	}

	theApp.preferences.SetBool("kmeleon.sidebar.visible", true);
	theApp.preferences.SetString("kmeleon.sidebar.lastVisible", m_iBars[index]->name);
}

void CSideBar::DrawSideBarMenu(HMENU menu)
{
	ASSERT(::IsMenu(menu));
	m_menu = menu;

	if (m_iCount == 0) 
		InsertMenu(m_menu, 0, MF_BYPOSITION | MF_GRAYED    | MF_STRING, 0, _T("None"));
	else

		for (int x=0; x<m_iCount; x++) {
			if (m_iBars[x]->name && m_iBars[x]->visibleOnMenu) {
				InsertMenu(m_menu, 0, MF_BYPOSITION | MF_STRING, SIDEBAR_MENU_START_ID+x, theApp.lang.Translate(m_iBars[x]->name));
			}
		}
}

void CSideBar::SetVisibility(int index, bool visible)
{
	ASSERT(index>=0 && index<m_iCount);
	if (visible)
	{
		m_iCurrent = index;

		//Adjust the header size and set the name of the sidebar
		REBARBANDINFO bandinfo;
		bandinfo.cbSize = sizeof(REBARBANDINFO);
		bandinfo.fMask =   RBBIM_HEADERSIZE | RBBIM_TEXT;
		bandinfo.cxHeader = m_iWidth - m_wSplitter - 20;
		bandinfo.lpText = m_iBars[index]->name;
		m_wndRebar.GetReBarCtrl().SetBandInfo(0, &bandinfo);

		// Get the height of the title bar (can depend of the text)
		m_iTitleHeight = m_wndRebar.GetReBarCtrl().GetBarHeight();

		// Positionning the child windows here is needed
		RECT rect;
		GetClientRect(&rect);
		::SetWindowPos(m_iBars[m_iCurrent]->child, NULL, m_wBorder, m_iTitleHeight+m_wBorder, m_iWidth-m_wSplitter-m_wBorder*2, rect.bottom-rect.top - m_iTitleHeight -m_wBorder*2 ,  SWP_NOZORDER);
		::SetWindowPos(m_wndRebar.m_hWnd, NULL, m_wBorder, m_wBorder, m_iWidth-m_wSplitter-m_wBorder*2, m_iTitleHeight,  SWP_NOZORDER);


		/*	// Set the child window for this sidebar
		bandinfo.hwndChild = m_iBars[index]->child;
		bandinfo.fMask = RBBIM_CHILD;
		rebar.GetReBarCtrl().SetBandInfo(1, &bandinfo);*/

		//Make it visible
		::ShowWindow(m_iBars[index]->child, SW_SHOW);
	}
	else
	{
		// You want to hide only the active sidebar
		ASSERT(m_iCurrent==index);
		::ShowWindow(m_iBars[index]->child, SW_HIDE);
		m_iCurrent = -1;
	}

	//   theApp.preferences.SetString("kmeleon.sidebar.last", m_iBars[index]->name);
}

void CSideBar::ToggleVisibility(int index)
{
	if (m_bVisible && m_iCurrent == index)
		Hide();
	else
		Show(index);
}

int CSideBar::RegisterSideBar(TCHAR* name, SideBarInitProc proc, UINT commandID, int visibleOnMenu)
{
	stBars **newIndex = new stBars *[m_iCount+1];
	if (m_iCount) {
		memcpy(newIndex, m_iBars, ((m_iCount)*sizeof(stBars *)));
		delete m_iBars;
	}
	m_iBars = newIndex;

	m_iBars[m_iCount] = new stBars;
	m_iBars[m_iCount]->uID = SIDEBAR_MENU_START_ID + m_iCount;
	m_iBars[m_iCount]->commandID = commandID;
	m_iBars[m_iCount]->child = NULL;
	m_iBars[m_iCount]->proc = proc;
	m_iBars[m_iCount]->name = NULL;
	m_iBars[m_iCount]->visibleOnMenu = visibleOnMenu;

	if (*name) {
		m_iBars[m_iCount]->name = new TCHAR[lstrlen(name)+1];
		lstrcpy(m_iBars[m_iCount]->name, name);
	}

	m_iCount++;

	if (theApp.preferences.GetBool("kmeleon.sidebar.visible", false))
	{
		TCHAR lastName[128];		
		theApp.preferences.GetString("kmeleon.sidebar.lastVisible", lastName, _T(""));
		if (_tcscmp(name, lastName) == 0)
			Show(m_iCount-1);
	}

	return m_iCount-1;
}


BEGIN_MESSAGE_MAP(CSideBar, CWnd)
	ON_WM_CAPTURECHANGED()
	ON_WM_CANCELMODE()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_COMMAND(ID_CLOSE, Hide)
	ON_MESSAGE(WM_SIZEPARENT, OnSizeParent)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// Gestionnaires de messages CSideBar

void CSideBar::OnCaptureChanged(CWnd *pWnd)
{
	m_bTracking = false;
	SetResizeCursor(false);
	CWnd::OnCaptureChanged(pWnd);
}

void CSideBar::OnCancelMode()
{
	m_bTracking = false;
	SetResizeCursor(false);
	CWnd::OnCancelMode();
}

void CSideBar::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO : ajoutez ici le code de votre gestionnaire de messages et/ou les paramètres par défaut des appels

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

// I'm intercepting here WM_SIZEPARENT which is an internal MFC
// message for recalcLayout, so that the sidebar can take part
// in the layout of the frame browser and not rely on CControlBar
LRESULT CSideBar::OnSizeParent(WPARAM wParam, LPARAM lParam)
{
	AFX_SIZEPARENTPARAMS* layout = (AFX_SIZEPARENTPARAMS*)lParam;

	RECT rectBar;
	rectBar = layout->rect;

	// If we are visible update the layout 
	if (m_bVisible)
	{
		layout->rect.left += m_iWidth;
		layout->sizeTotal.cx += -m_iWidth;

		//int cx = rectBar.left-rectBar.right;
		//int cy = rectBar.bottom - rectBar.top;
	}
	if (layout->hDWP != NULL)
	{
		DeferWindowPos(layout->hDWP, m_hWnd, NULL, rectBar.left, rectBar.top, m_iWidth, rectBar.bottom - rectBar.top,  SWP_NOZORDER);			
		m_iMaxWidth = layout->rect.right-100; //Keep some space for the browser
	}

	return 0;
}

void CSideBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (!m_bVisible) return;

	//Adjust the header size of the bar title to
	//reposition the close button
	REBARBANDINFO bandinfo;
	bandinfo.cbSize = sizeof(REBARBANDINFO);
	bandinfo.fMask = RBBIM_HEADERSIZE;
	bandinfo.cxHeader = cx - m_wSplitter - 20;
	m_wndRebar.GetReBarCtrl().SetBandInfo(0, &bandinfo);

	// Position the child windows
	HDWP h = BeginDeferWindowPos(2);
	DeferWindowPos(h, m_wndRebar.m_hWnd, NULL, m_wBorder, m_wBorder, cx-m_wSplitter-m_wBorder*2, m_iTitleHeight,  SWP_NOZORDER);
	DeferWindowPos(h, m_iBars[m_iCurrent]->child, NULL, m_wBorder, m_iTitleHeight+m_wBorder, cx-m_wSplitter-m_wBorder*2, cy - m_iTitleHeight -m_wBorder*2 ,  SWP_NOZORDER);
	EndDeferWindowPos(h);

	theApp.preferences.SetInt("kmeleon.sidebar.width", m_iWidth);
	/*
	//Adjust the height of the band for the child
	bandinfo.fMask = RBBIM_CHILDSIZE;
	bandinfo.cyIntegral = 1;
	bandinfo.cyMaxChild = bandinfo.cyChild = bandinfo.cyMinChild = cy - m_iTitleHeight-2;	
	rebar.GetReBarCtrl().SetBandInfo(1, &bandinfo);*/
}

BOOL CSideBar::IsonBar(POINT pt)
{
	RECT rect;
	GetClientRect(&rect);

	return (pt.x<=rect.right && pt.x>=rect.right-m_wSplitter) ;
}

void CSideBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bTracking)
	{
		if (point.x<MINSIDEBAR_SIZE){
			point.x =  MINSIDEBAR_SIZE;
			ClientToScreen(&point);
			m_iWidth = MINSIDEBAR_SIZE;
			::SetCursorPos(point.x, point.y);
		}
		else if (point.x>m_iMaxWidth){
			point.x = m_iMaxWidth;
			ClientToScreen(&point);
			m_iWidth = m_iMaxWidth;
			::SetCursorPos(point.x, point.y);
		}
		else
			m_iWidth = point.x;

		if (!m_bTimer)
		{
			SetTimer(1500, 10, NULL);
			m_bTimer = true;
		}
	}
	else
	{
		SetResizeCursor(IsonBar(point));
	}
}

void CSideBar::SetResizeCursor(BOOL b)
{
	LPTSTR idc;
	HCURSOR cursor;
	if (b)
		idc = IDC_SIZEWE;
	else
		idc = IDC_ARROW;

	cursor = ::LoadCursor(NULL, idc);
	SetCursor(cursor);
}


void CSideBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (!m_bTracking) return;
	ReleaseCapture();
	SetResizeCursor(false);
	m_bTracking = false;
}

void CSideBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bTracking) return;

	if (IsonBar(point))
	{
		SetResizeCursor(true);
		SetCapture();
		m_bTracking = true;
	}
}


void CSideBar::OnPaint()
{
	CPaintDC dc(this); 

	CRect rect;
	GetClientRect(rect);

	// Draw an possible border
	if (m_wBorder)
	{
		rect.right = rect.right - m_wSplitter;
		dc.Draw3dRect(rect, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
		rect.InflateRect(-1, -1);
		dc.Draw3dRect(rect, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_BTNFACE));
	}

	// Draw the resizing bar
	GetClientRect(rect);
	rect.left = rect.right - m_wSplitter;
	/*
	pDC->Draw3dRect(rect, GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOWFRAME));
	rect.InflateRect(-1, -1);
	pDC->Draw3dRect(rect, GetSysColor(COLOR_BTNHIGHLIGHT), GetSysColor(COLOR_BTNSHADOW));	
	*/
	COLORREF clr = GetSysColor(COLOR_BTNFACE);
	dc.FillSolidRect(rect, clr);
}

BOOL CSideBar::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO : ajoutez ici votre code spécialisEet/ou l'appel de la classe de base

	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	//cs.dwExStyle |= WS_EX_CLIENTEDGE | WS_EX_TOOLWINDOW;
	return true;
}

int CSideBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//Create the rebar within the sidebar
	m_wndRebar.Create(this,  RBS_REGISTERDROP |  RBS_BANDBORDERS |
		RBS_DBLCLKTOGGLE| WS_CHILDWINDOW| WS_BORDER );

	// Create the top toolbar with a button to close the sidebar
	//CToolBar *toolbar = new CToolBar();
	closeBar.CreateEx(&m_wndRebar, TBSTYLE_FLAT|TBSTYLE_TRANSPARENT,
		WS_CHILD|WS_VISIBLE);
	closeBar.LoadToolBar(IDR_TOOLBAR_CLOSE);
	closeBar.GetToolBarCtrl().SetBitmapSize(CSize(10,10));

	// Add the bar for the top toolbar and the title
	m_wndRebar.AddBar(&closeBar, _T(""), NULL, RBBS_BREAK | RBBS_NOGRIPPER);

	/*// Add the band for the child window
	REBARBANDINFO band;
	band.cbSize = sizeof(REBARBANDINFO);
	band.fMask = RBBIM_STYLE;
	band.fStyle = RBBS_NOGRIPPER | RBBS_BREAK ;//| RBBS_CHILDEDGE ;//| RBBS_VARIABLEHEIGHT;
	rebar.GetReBarCtrl().InsertBand(1, &band);
	*/

	return 0;
}

void CSideBar::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO : ajoutez ici le code de votre gestionnaire de messages
}

void CSideBar::OnTimer(UINT nIDEvent)
{
	// TODO : ajoutez ici le code de votre gestionnaire de messages et/ou les paramètres par défaut des appels
	KillTimer(nIDEvent);
	GetParentFrame()->RecalcLayout();
	m_bTimer = false;
	CWnd::OnTimer(nIDEvent);	
}

#endif
