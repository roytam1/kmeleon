/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *OnAppExitwm_
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

// File Overview....
//
// The typical MFC View, toolbar, statusbar creation done 
// in CBrowserFrame::OnCreate()
//
// Code to update the Status/Tool bars in response to the
// Web page loading progress(called from methods in CBrowserImpl)
//
// SetupFrameChrome() determines what, if any, UI elements this Frame
// will sport based on the current "chromeMask" 
//
// Also take a look at OnClose() which gets used when you close a browser
// window. This needs to be overrided mainly to handle supporting multiple
// browser frame windows via the "New Browser Window" menu item
// Without this being overridden the MFC framework handles the OnClose and
// shutsdown the complete application when a frame window is closed.
// In our case, we want the app to shutdown when the File/Exit menu is chosen
//
// Another key functionality this object implements is the IBrowserFrameGlue
// interface - that's the interface the Gecko embedding interfaces call
// upong to update the status bar etc.
// (Take a look at IBrowserFrameGlue.h for the interface definition and
// the BrowserFrm.h to see how we implement this interface - as a nested
// class)
// We pass this Glue object pointer to the CBrowserView object via the 
// SetBrowserFrameGlue() method. The CBrowserView passes this on to the
// embedding interface implementaion
//
// Please note the use of the macro METHOD_PROLOGUE in the implementation
// of the nested BrowserFrameGlue object. Essentially what this macro does
// is to get you access to the outer (or the object which is containing the
// nested object) object via the pThis pointer.
// Refer to the AFXDISP.H file in VC++ include dirs
//
// Next suggested file to look at : BrowserView.cpp

#include "stdafx.h"

#include "BrowserFrm.h"
#include "BrowserView.h"
#include "ToolBarEx.h"
#include "KmeleonConst.h"
#include "PasswordViewerDlg.h"
#include "CookiesViewerDlg.h"
#include "Permissions.h"
#include "rebar_menu/hot_tracking.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowserFrame

IMPLEMENT_DYNAMIC(CBrowserFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CBrowserFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CBrowserFrame)
    ON_WM_CREATE()
    ON_WM_SETFOCUS()
    ON_WM_SIZE()
    ON_WM_MOVE()
    ON_WM_CLOSE()
    ON_WM_ACTIVATE()
    ON_WM_SYSCOLORCHANGE()
	ON_WM_SYSCOMMAND()
	ON_WM_INITMENUPOPUP()
	ON_WM_MENUSELECT()

	ON_COMMAND(ID_SELECT_URL, OnSelectUrl)
	ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
    ON_COMMAND(ID_WINDOW_PREV, OnWindowPrev)

    ON_MESSAGE(UWM_REFRESHMRULIST, RefreshMRUList)
#ifdef INTERNAL_SITEICONS
	ON_MESSAGE(UWM_NEWSITEICON, OnNewSiteIcon)
#endif
	ON_COMMAND(ID_CLOSE, OnClose)
    ON_COMMAND_RANGE(TOOLBAR_MENU_START_ID, TOOLBAR_MENU_END_ID, ToggleToolBar)
    ON_COMMAND(ID_TOOLBARS_LOCK, ToggleToolbarLock)
    ON_COMMAND(ID_COOKIES_VIEWER, OnCookiesViewer)
    ON_COMMAND(ID_PASSWORDS_VIEWER, OnPasswordsViewer)
	ON_COMMAND(ID_COOKIE_PERM, OnCookiePermissions)
	ON_COMMAND(ID_IMAGE_PERM, OnImagePermissions)
	ON_COMMAND(ID_POPUP_PERM, OnPopupPermissions)
	
    ON_UPDATE_COMMAND_UI(ID_TOOLBARS_LOCK, OnUpdateToggleToolbarLock)
#ifdef INTERNAL_SIDEBAR
	ON_COMMAND_RANGE(SIDEBAR_MENU_START_ID, SIDEBAR_MENU_END_ID, ToggleSideBar)
	ON_UPDATE_COMMAND_UI_RANGE(SIDEBAR_MENU_START_ID, SIDEBAR_MENU_END_ID, OnUpdateSideBarMenu)
#endif
	ON_UPDATE_COMMAND_UI_RANGE(TOOLBAR_MENU_START_ID, TOOLBAR_MENU_END_ID, OnUpdateToolBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
	ON_UPDATE_COMMAND_UI(ID_CLOSE_FINDBAR, OnUpdateNothing)
	ON_COMMAND(ID_EDIT_FIND, OnShowFindBar)
	ON_COMMAND(ID_VIEW_STATUS_BAR, OnViewStatusBar)
    ON_NOTIFY(RBN_LAYOUTCHANGED, AFX_IDW_REBAR, OnRbnLayoutChanged)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xffff, OnToolTipText)

    ON_COMMAND(ID_EDIT_FINDNEXT, OnFindNext)
	ON_COMMAND(ID_EDIT_FINDPREV, OnFindPrev)
	ON_COMMAND(ID_WRAP_AROUND, OnWrapAround)
	ON_COMMAND(ID_MATCH_CASE, OnMatchCase)
	ON_COMMAND(ID_HIGHLIGHT, OnHighlight)

	ON_COMMAND(ID_MAXIMIZE_WINDOW,OnMaximizeWindow)
	ON_COMMAND(ID_MINIMIZE_WINDOW,OnMinimizeWindow)
	ON_COMMAND(ID_RESTORE_WINDOW, OnRestoreWindow)
	ON_COMMAND(ID_TOGGLE_WINDOW, OnToggleWindow)
	
	ON_MESSAGE(TB_LBUTTONHOLD, OnToolbarContextMenu)
	ON_MESSAGE(TB_RBUTTONDOWN, OnToolbarContextMenu)
	ON_MESSAGE(TB_LBUTTONDOWN, OnToolbarCommand)

//	ON_MESSAGE(WM_ENTERSIZEMOVE, OnEnterSizeMove)
//	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	//}}AFX_MSG_MAP

//	ON_WM_MOVING()
	ON_WM_DESTROY()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

#define PREF_TOOLBAND_LOCKED "kmeleon.general.toolbars_locked"

/////////////////////////////////////////////////////////////////////////////
// CBrowserFrame construction/destruction

CBrowserFrame::CBrowserFrame(PRUint32 chromeMask, LONG style = 0)
{
    // Save the chromeMask off. It'll be used
    // later to determine whether this browser frame
    // will have menubar, toolbar, statusbar etc.
	
    m_chromeMask = chromeMask;
    m_style = style;
    m_created = FALSE;
    m_hSecurityIcon = NULL;
	m_wndFindBar = NULL;
	m_searchString = NULL;
	m_wndLastFocused = NULL;
	m_wndBrowserView = NULL;
}

CBrowserFrame::~CBrowserFrame()
{
    if (m_hSecurityIcon)
        DestroyIcon(m_hSecurityIcon);
	if (m_wndFindBar)
		delete m_wndFindBar;
	if (m_searchString)
		free(m_searchString);
	::UnregisterHotKey(m_hWnd,1);
}

BOOL CBrowserFrame::PreTranslateMessage(MSG* pMsg)
{
   ::RegisterHotKey(m_hWnd,1,MOD_ALT,192);
   if (pMsg->message==WM_KEYDOWN)
   {
	  if ( pMsg->wParam == VK_TAB && !(GetKeyState(VK_CONTROL)  & 0x8000)) {
            if (pMsg->hwnd == m_wndUrlBar.m_hwndEdit) {
               if (GetKeyState(VK_SHIFT)  & 0x8000)
                  if (m_wndFindBar) 
                     m_wndFindBar->SetFocus();
				  else {
					  CBrowserView* view = GetActiveView();
					  if (view) view->GetBrowserWrapper()->FocusLastElement();
                  }
               else {
					CBrowserView* view = GetActiveView();
					if (view) view->GetBrowserWrapper()->FocusFirstElement();
               }
               return 1;
            }
            else if ( m_wndFindBar && (pMsg->hwnd == m_wndFindBar->m_cEdit.m_hWnd)) { 

               if (GetKeyState(VK_SHIFT)  & 0x8000) {
					  CBrowserView* view = GetActiveView();
					  if (view) view->GetBrowserWrapper()->FocusLastElement();
               }
               else if (m_wndUrlBar.IsWindowVisible())
                  ::SetFocus(m_wndUrlBar.m_hwndEdit);
               else {
					  CBrowserView* view = GetActiveView();
					  if (view) view->GetBrowserWrapper()->FocusFirstElement();
               }

               return 1;
            }
         }
 
	  // Prevent accels to interfere with input controls
 	  else if (pMsg->wParam >= VK_END && pMsg->wParam <= VK_DOWN) {
         if ( GetActiveView()->IsChild(GetFocus()) && GetActiveView()->GetBrowserWrapper()->InputHasFocus() )
            return 0;
      }
	  else if (MapVirtualKey(pMsg->wParam, 2  /*MAPVK_VK_TO_CHAR*/) != 0) {
         if (!(GetKeyState(VK_CONTROL) & 0x8000) && (!GetActiveView()->IsChild(GetFocus()) || GetActiveView()->GetBrowserWrapper()->InputHasFocus()))
            return 0;
	  }

	 /* else if ( pMsg->wParam == VK_BACK ) {
	     if (m_wndBrowserView.InputHasFocus())
		   return 0;
	  }*/

   }  else if ( pMsg->wParam == 0xff ) {
	   if  ( (pMsg->lParam & 0x00ff0000) == 0x000b0000) {
           	GetActiveView()->GetBrowserWrapper()->ChangeTextSize(1);		
			return 1;
	   }
	   else if ( (pMsg->lParam  & 0x00ff0000) == 0x00110000) {
           GetActiveView()->GetBrowserWrapper()->ChangeTextSize(-1);
		   return 1;
	   }
   }
   else if ( pMsg->message == WM_XBUTTONUP ) {
      if (HIWORD(pMsg->wParam) == XBUTTON1) 
         GetActiveView()->GetBrowserWrapper()->GoBack();
      else if (HIWORD(pMsg->wParam) == XBUTTON2) 
         GetActiveView()->GetBrowserWrapper()->GoForward();
   }

   return CFrameWnd::PreTranslateMessage(pMsg);
}

void CBrowserFrame::OnClose()
{
   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( theApp.m_hMutex, 1000L);
   if (dwWaitResult != WAIT_OBJECT_0) {
     return;
   }

   // tell all our plugins that we are closing
   if (!IsDialog())
   theApp.plugins.SendMessage("*", "* OnClose", "Close", (long)m_hWnd);

   // the browserframeglue will be deleted soon, so we set it to null so it won't try to access it after it's deleted.
   //GetActiveView()->SetBrowserFrameGlue(NULL);
   //GetActiveView()->SetBrowserGlue(NULL);

   if (theApp.m_pMostRecentBrowserFrame == this) {
      CBrowserFrame* pFrame;

      POSITION pos = theApp.m_FrameWndLst.Find(this);
      if (pos) theApp.m_FrameWndLst.GetPrev(pos);

      pFrame = pos ? 
         (CBrowserFrame *) theApp.m_FrameWndLst.GetPrev(pos) : // previous frame
         (CBrowserFrame *) theApp.m_FrameWndLst.GetTail();     // last frame

      // if no other browser views exist, nullify the pointer
      theApp.m_pMostRecentBrowserFrame = (pFrame != this) ? pFrame : NULL;
   }

	SaveWindowPos();


   // XXX: Destroying the window with the findbar open, will activate
   // it during destruction because I'm setting back the focus to the
   // view, and m_pMostRecentBrowserFrame will point to an deleted 
   // object.
   CBrowserFrame* pTemp = theApp.m_pMostRecentBrowserFrame;
   theApp.RemoveFrameFromList(this);
   
   if (m_nFlags & (WF_MODALLOOP|WF_CONTINUEMODAL))
      EndModalLoop(IDCANCEL);
   else
      DestroyWindow();

   if (theApp.m_pMostRecentBrowserFrame == this) {
	 theApp.m_pMostRecentBrowserFrame = pTemp;
   }
   
   ReleaseMutex(theApp.m_hMutex);
}

void CBrowserFrame::OnDestroy()
{
	// if we don't do this, then our menu will be destroyed when the first window is.
    // that's bad because our menu is shared between all windows
    SetMenu(NULL);

	if (!IsDialog()) {
		theApp.plugins.SendMessage("*", "* OnClose", "Destroy", (long)m_hWnd);
		theApp.toolbars.CloseWindow(this);
	}
	m_wndStatusBar.RemoveIcon(ID_SECURITY_STATE_ICON);
	CFrameWnd::OnDestroy();
}


// This is where the UrlBar, ToolBar, StatusBar, ProgressBar
// get created
// 
int CBrowserFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

	// Will be deleted in CBrowserView::PostNcDestroy()
	m_wndBrowserView = new CBrowserView();

    // tell all our plugins that we were created
	if (!IsDialog())
    theApp.plugins.SendMessage("*", "* OnCreate", "Create", (long)this->m_hWnd, IsPopup()?2:0);

    // Pass "this" to the View for later callbacks
    // and/or access to any public data members, if needed
    //
    m_wndBrowserView->SetBrowserFrame(this);

    // Pass on the BrowserFrameGlue also to the View which
    // it will use during the Init() process after creation
    // of the BrowserImpl obj. Essentially, the View object
    // hooks up the Embedded browser's callbacks to the BrowserFrame
    // via this BrowserFrameGlue object
    //m_wndBrowserView.SetBrowserFrameGlue((PBROWSERFRAMEGLUE)&m_xBrowserFrameGlueObj);
	m_wndBrowserView->SetBrowserGlue(new CBrowserGlue(this, m_wndBrowserView));

    // create a view to occupy the client area of the frame
    // This will be the view in which the embedded browser will
    // be displayed in
    if (!m_wndBrowserView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
        CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
    {
        TRACE0("Failed to create view window\n");
        return -1;
    }

	// Create a ReBar window to which the toolbar and UrlBar 
    // will be added
    BOOL hasLine = theApp.preferences.GetBool("kmeleon.display.toolbars_line", TRUE);
    if (!m_wndReBar.Create(this, RBS_DBLCLKTOGGLE | RBS_VARHEIGHT | (hasLine ? RBS_BANDBORDERS:0)))
    {
        TRACE0("Failed to create ReBar\n");
        return -1;      // fail to create
    }
	m_wndReBar.SetNeedSeparator(true);

	if (InitLayout() == -1)
		return -1;

	//m_wndReBar.RestoreBandSizes();
	return 0;
}

int CBrowserFrame::InitLayout()
{
	// create the URL bar (essentially a ComboBoxEx object)
    if (!m_wndUrlBar.Create(CBS_DROPDOWN | WS_CHILD, CRect(0, 0, 200, 150), this, ID_URL_BAR))
    {
        TRACE0("Failed to create URL Bar\n");
        return -1;      // fail to create
    }
	
#ifdef INTERNAL_SITEICONS
	m_wndUrlBar.SetImageList(&theApp.favicons);
#endif

	// Load the Most Recently Used(MRU) Urls into the UrlBar
    m_wndUrlBar.LoadMRUList();  

    // Create the animation control..
	BOOL bThrobber = TRUE;
    if (!m_wndAnimate.Create(WS_CHILD | WS_VISIBLE | ACS_TRANSPARENT, CRect(0, 0, 10, 10), this, ID_THROBBER))
    {
        TRACE0("Failed to create animation\n");
        bThrobber = FALSE;
    }

	CString throbberPath;
	theApp.skin.FindSkinFile(throbberPath, _T("Throbber.avi"));
	
	if (!m_wndAnimate.Open(throbberPath))
		bThrobber = FALSE;

    //Add the UrlBar and Throbber windows to the rebar
	CString urlTitle;
	urlTitle.LoadString(IDS_TOOLBAR_URL);
    m_wndReBar.AddBar(&m_wndUrlBar, theApp.preferences.GetString("kmeleon.display.URLbarTitle", _T("")));
	m_wndReBar.RegisterBand(m_wndUrlBar.m_hWnd, _T("URL Bar"), true);

	if (bThrobber) {
        m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP);
		m_wndReBar.RegisterBand(m_wndAnimate.m_hWnd, _T("Throbber"), true);
	}
    
    if (!m_wndStatusBar.CreateEx(this))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }
   
    // Create the progress bar as a child of the status bar.
    // Note that the ItemRect which we'll get at this stage
    // is bogus since the status bar panes are not fully
    // positioned yet i.e. we'll be passing in an invalid rect
    // to the Create function below
    // The actual positioning of the progress bar will be done
    // in response to OnSize()
    RECT rc;
    m_wndStatusBar.GetItemRect (m_wndStatusBar.CommandToIndex(ID_PROG_BAR), &rc);
    if (!m_wndProgressBar.Create(WS_CHILD|PBS_SMOOTH, rc, &m_wndStatusBar, ID_PROG_BAR))
    {
        TRACE0("Failed to create progress bar\n");
        return -1;      // fail to create
    }

	m_wndStatusBar.AddIcon(ID_SECURITY_STATE_ICON);

	if (!theApp.preferences.GetBool("kmeleon.display.statusbar", TRUE))
		m_wndStatusBar.ShowWindow(SW_HIDE);

	if (!IsDialog()) {
		theApp.plugins.SendMessage("*", "* OnCreate", "DoRebar", (long)m_wndReBar.GetReBarCtrl().m_hWnd);
		theApp.toolbars.InitWindow(this);
	}

    m_wndReBar.RestoreBandSizes();
    //m_wndReBar.LockBars(theApp.preferences.GetBool(PREF_TOOLBAND_LOCKED, false));

    // Create the tooltip window
    m_wndToolTip.Create(this);

    // Also, set the padlock icon to be the insecure icon to begin with
    UpdateSecurityStatus(nsIWebProgressListener::STATE_IS_INSECURE);

#ifdef INTERNAL_SIDEBAR
	// Create the side bar. Must be created last for proper layout calc
	if (!m_wndSideBar.CreateEx(WS_EX_WINDOWEDGE, NULL, NULL, WS_CLIPSIBLINGS | WS_CHILD, CRect(0, 0, 0, 0), this, ID_SIDE_BAR , NULL))
    {
        TRACE0("Failed to create SideBar\n");
        return -1;      // fail to create
    }
	
	theApp.plugins.SendMessage("*", "* OnCreate", "DoSidebar", (long)m_wndSideBar.m_hWnd);
#endif

    SetBackImage();

    // Based on the "chromeMask" we were supplied during construction
    // hide any requested UI elements - statusbar, menubar etc...
    // Note that the window styles (WM_RESIZE etc) are set inside
    // of PreCreateWindow()

    SetupFrameChrome();
	m_created = TRUE;
	return 0;
}

void CBrowserFrame::SetupFrameChrome()
{
    if(m_chromeMask == nsIWebBrowserChrome::CHROME_ALL)
        return;

    if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR) &&  
       ! (m_chromeMask & nsIWebBrowserChrome::CHROME_MENUBAR) && 
       ! (m_chromeMask & nsIWebBrowserChrome::CHROME_LOCATIONBAR) ) {
        m_wndReBar.ShowWindow(SW_HIDE); // Hide the Rebar
		  SetMenu(NULL);
    }
    else {
       if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR) ) {
          int i = m_wndReBar.count();
          int iMenubar = m_wndReBar.FindByName(_T("Menu"));
          int iUrlbar  = m_wndReBar.FindByName(_T("URL Bar"));
          while (i-- > 0) {
             if (i != iMenubar && i != iUrlbar)
                m_wndReBar.ShowBand(i, false);
          }
          m_wndReBar.lineup();
          m_wndReBar.LockBars(true);
       }

       if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_MENUBAR) ) {
          SetMenu(NULL); // Hide the MenuBar
          int iMenubar  = m_wndReBar.FindByName(_T("Menu"));
          if (iMenubar >= 0)
             m_wndReBar.ShowBand(iMenubar, false);
       }

       if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_LOCATIONBAR) ) {
          int iUrlbar  = m_wndReBar.FindByName(_T("URL Bar"));
          if (iUrlbar >= 0)
             m_wndReBar.ShowBand(iUrlbar, false);
       }
    }

    if(! (m_chromeMask & nsIWebBrowserChrome::CHROME_STATUSBAR))
        m_wndStatusBar.ShowWindow(SW_HIDE); // Hide the StatusBar

	if (!(m_chromeMask & nsIWebBrowserChrome::CHROME_DEFAULT) &&
	    !(m_chromeMask & nsIWebBrowserChrome::CHROME_SCROLLBARS))
		GetActiveView()->GetBrowserWrapper()->ShowScrollbars(FALSE);
}

BOOL CBrowserFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
 
    //cs.lpszClass = BROWSER_WINDOW_CLASS;
    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;


    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBrowserFrame message handlers
void CBrowserFrame::OnSetFocus(CWnd* pOldWnd)
{
    if (theApp.m_pMostRecentBrowserFrame != this) {
        theApp.m_pMostRecentBrowserFrame = this;

        // update session history for the current window
        PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);
    }

    // forward focus to the browser window
    if (GetActiveView()) GetActiveView()->SetFocus();
}

BOOL CBrowserFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    if (nCode == CN_COMMAND){
        if (pHandlerInfo && theApp.plugins.OnUpdate(nID)){
            return true;
        }
    }

	if (theApp.commands.IsPluginCommand(nID) && theApp.toolbars.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // Don't let MFC mess with plugin command
    if (nCode == CN_UPDATE_COMMAND_UI && (theApp.commands.IsPluginCommand(nID)
	   || IsMenu(COMMAND_TO_MENU(nID)))) {
        if (pExtra) ((CCmdUI*)pExtra)->m_bEnableChanged = TRUE;
        return TRUE;
    }

    // let the view have first crack at the command
	CBrowserView* pView = GetActiveView();
    if (pView && pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // otherwise, do default handling
    return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CBrowserFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// Intercept menu item click
	if (HIWORD(wParam) == 0 && lParam == 0)
		if (theApp.menus.MenuCommand(LOWORD(wParam)))
			return TRUE;

	return CFrameWnd::OnCommand(wParam, lParam);
}

// Needed to properly position/resize the progress bar
//
void CBrowserFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
	
    // Get the ItemRect of the status bar's Pane 0
    // That's where the progress bar will be located
    RECT rc;
    if (m_wndStatusBar.m_hWnd)
	{
        m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_PROG_BAR), &rc);

		// Move the progress bar into it's correct location
		if (m_wndProgressBar.m_hWnd)
			m_wndProgressBar.MoveWindow(&rc);
	}

	if (m_created) SaveWindowPos();
	
	// Fix rebar redrawing bug when xp themes are enabled 
	// and the background image is disabled and the the tabbar is fixed.
	m_wndReBar.RedrawWindow(0, 0, RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
}

void CBrowserFrame::SaveWindowPos()
{
	// only record the window size for non-popup windows
	if (IsPopup() || IsDialog())
		return;

	WINDOWPLACEMENT wp;
    wp.length = sizeof (WINDOWPLACEMENT);
    GetWindowPlacement(&wp);
	
	if (wp.showCmd == SW_SHOWMAXIMIZED)
		// record the maximized state
		theApp.preferences.bMaximized = true;
	else if (wp.showCmd == SW_SHOWNORMAL) {
		// record the window size/pos
		theApp.preferences.bMaximized = false;

		RECT rc; // = wp.rcNormalPosition;
		GetWindowRect(&rc);
		if (rc.left >= 0 && rc.top >= 0 ) {
			theApp.preferences.windowWidth = rc.right - rc.left;
			theApp.preferences.windowHeight = rc.bottom - rc.top;
			theApp.preferences.windowXPos = rc.left;
			theApp.preferences.windowYPos = rc.top;
		}
	}

}

void CBrowserFrame::UpdateSHistoryMenu()
{
	theApp.UpdateWindowListMenu();
	KmMenu* menu = theApp.menus.GetKMenu(_T("@SHistoryBack"));
	if (menu) menu->Invalidate();
	menu = theApp.menus.GetKMenu(_T("@SHistoryForward"));
	if (menu) menu->Invalidate();
	menu = theApp.menus.GetKMenu(_T("@SHistory"));
	if (menu) menu->Invalidate();
}

CString PrepareSHMenuText(LPCTSTR title, UINT keyNumber)
{
	CString menuText;
	if (keyNumber < 10)
	{
		TCHAR key[34];
		_itot(keyNumber++, key, 10);
		menuText = CString(_T("&")) + key + _T(" ") + title;
	}
	else
		menuText = CString(_T("   ")) + title;
	if (menuText.GetLength() > 50)
	{
		menuText.Truncate(47);
		menuText.Append(_T("..."));
	}
	return menuText;
}

void CBrowserFrame::DrawSHForwardMenu(HMENU menu)
{
	CBrowserWrapper* pWindows = GetActiveView()->GetBrowserWrapper();
	if (!pWindows) return;

	int index, count;
	pWindows->GetSHistoryState(index, count);

	int limit = std::min(MAX_SHMENU_NUMBER, theApp.preferences.GetInt("kmeleon.plugins.history.length", 25));
	limit =  count - index > limit ? index + limit : count;
	
	CString title, url;
	UINT keyNumber = 0;
	for (int i=index+1;i<limit;i++)
	{
		pWindows->GetSHistoryInfoAt(i, title, url);
		if (!title.GetLength()) title = url;
		AppendMenu(menu, MF_STRING, SHISTORYF_START_ID+i-index-1, PrepareSHMenuText(title, keyNumber++));
	}
}

void CBrowserFrame::DrawSHBackMenu(HMENU menu)
{
	CBrowserWrapper* pWindows = GetActiveView()->GetBrowserWrapper();
	if (!pWindows) return;

	int index, count;
	pWindows->GetSHistoryState(index, count);

	int limit = std::min(MAX_SHMENU_NUMBER, theApp.preferences.GetInt("kmeleon.plugins.history.length", 25));
	limit =  index > limit ? index - limit : 0;

	CString title, url;
	UINT keyNumber = 0;
	for (int i=index-1;i>=limit;i--)
	{
		pWindows->GetSHistoryInfoAt(i, title, url);
		if (!title.GetLength()) title = url;
		AppendMenu(menu, MF_STRING, SHISTORYB_START_ID+i-limit, PrepareSHMenuText(title, keyNumber++));
	}
}

void CBrowserFrame::DrawSHMenu(HMENU menu)
{
	CBrowserWrapper* pWindows = GetActiveView()->GetBrowserWrapper();
	if (!pWindows) return;

	int index, count;
	pWindows->GetSHistoryState(index, count);
	
	CString title, url;
	pWindows->GetSHistoryInfoAt(index, title, url);
	if (!title.GetLength()) title = url;
	if (!title.GetLength()) title = _T("Blank page");

	DrawSHForwardMenu(menu);
	AppendMenu(menu, MF_CHECKED | MF_STRING, 0, PrepareSHMenuText(title, -1));
	DrawSHBackMenu(menu);
}

#if 0
void CBrowserFrame::OnMove(int x, int y)
{ 
    CFrameWnd::OnMove(x, y);

    WINDOWPLACEMENT wp;
    wp.length = sizeof (WINDOWPLACEMENT);
    GetWindowPlacement(&wp);

    // only record the window position for non-popup windows
    if (!(m_style & WS_POPUP) && (wp.showCmd != SW_SHOWMAXIMIZED)){
        RECT rc;

        GetWindowRect(&rc);
        if (rc.left > 0 && rc.top > 0 ) {
            theApp.preferences.windowWidth = rc.right - rc.left;
            theApp.preferences.windowHeight = rc.bottom - rc.top;
            theApp.preferences.windowXPos = rc.left;
            theApp.preferences.windowYPos = rc.top;
        }
    }
}
#endif

#ifdef _DEBUG
void CBrowserFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CBrowserFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

void CBrowserFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	if (nState != WA_INACTIVE && theApp.m_pMostRecentBrowserFrame != this) {
        theApp.m_pMostRecentBrowserFrame = this;
		
        // update session history for the current window
		UpdateSHistoryMenu();
        PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);
    }

   CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
   if (pWndOther == this)
	   return;

   	if(bMinimized) // This isn't an activate event that Gecko cares about
	{
		// When we get there, the focus is already lost !!
		// So CBrowserFrame::OnSysCommand take care of it
		return;
	}

	CBrowserView* view = GetActiveView();
	if (!view) 
		return;

    switch(nState) {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:

			// The window dirrectly inside the view always change o_o breaking
			// this code so I'm testing if the windows still exist
			if (!IsWindow(m_wndLastFocused) || !m_wndLastFocused || 
				m_wndLastFocused == view->GetSafeHwnd() ||
				::IsChild(view->m_hWnd, m_wndLastFocused)) {
				view->Activate(TRUE);
			}
			else
				::SetFocus(m_wndLastFocused);
			break;
		case WA_INACTIVE:
			m_wndLastFocused = ::GetFocus();
			// When flash go fullscreen, the browser window become
			// inactive, but gecko doesn't like being deactivated
			// at this point, so I have to detect that flash is taking
			// the focus in a way or another ...
			if (pWndOther) {
				CString title;
				pWndOther->GetWindowText(title);
				if (title.Compare(L"Adobe Flash Player") == 0)
					break;
			}

			if ( ::IsChild(view->m_hWnd, m_wndLastFocused) 
				|| m_wndLastFocused == view->GetSafeHwnd())
				view->Activate(FALSE);

			break;		
		default:
            break;
	}
  // m_wndBrowserView.Activate(nState, pWndOther, bMinimized);
}

// CMyStatusBar Class
// The current implementation is bad but enough for what I want to do now.
CMyStatusBar::CMyStatusBar() 
{
}

CMyStatusBar::~CMyStatusBar() 
{
	int count = arrIcons.GetSize();
	for (int i=0; i<arrIcons.GetSize(); i++)
	{
		struct icon_info* ii = &arrIcons[i];
		if (ii->hIcon)	DestroyIcon(ii->hIcon);
	}
}

void CMyStatusBar::RefreshPanes()
{
	if (!m_hWnd) return;
	int count = arrIcons.GetSize(), i;
	
	CString statusText = GetPaneText(0);
	UINT* indicators = new UINT[2+count];
	indicators[0] = ID_SEPARATOR;
	indicators[1] = ID_PROG_BAR;
	
	for (i=2;i<count+2;i++)
		indicators[i] = arrIcons[i-2].nID;
	
	SetIndicators(indicators, count+2);
	SetPaneStyle(0, SBPS_STRETCH);
	SetPaneText(0, statusText);

	for (i=0;i<count;i++)
	{
		SetPaneInfo(i+2, indicators[i+2], SBPS_NORMAL|SBPS_NOBORDERS, arrIcons[i].lWidth);
		GetStatusBarCtrl().SetTipText(i+2, arrIcons[i].csTpText);
		if (arrIcons[i].hIcon) GetStatusBarCtrl().SetIcon(i+2, arrIcons[i].hIcon);
	}
	
	RedrawWindow();
	
	RECT rc;
	GetItemRect(CommandToIndex(ID_PROG_BAR), &rc);

    // Move the progress bar into it's correct location
	CBrowserFrame *pFrame = (CBrowserFrame *)GetParentFrame();
	if( pFrame!=NULL )
        pFrame->m_wndProgressBar.MoveWindow(&rc);

	delete [] indicators;
}

BOOL CMyStatusBar::RemoveIcon(UINT nID)
{
	BOOL done = FALSE;
	
	for (int i=0; i<arrIcons.GetSize(); i++)
	{
		if (arrIcons[i].nID == nID)	{
			struct icon_info* ii = &arrIcons[i];
			//DestroyIcon(ii->hIcon); // Let the owner handle it
			arrIcons.RemoveAt(i);
			done = TRUE;
			break;
		}
	}
	if (done) 
		RefreshPanes();
	
	return done;
}

BOOL CMyStatusBar::SetIconInfo(UINT nID, HICON hIcon, LPCTSTR tpText)
{
	for (int i=0; i<arrIcons.GetSize(); i++)
	{
		if (arrIcons[i].nID == nID)	{
			icon_info* ii = &arrIcons[i];
			if (hIcon) {
				ii->csTpText = tpText;
				if (ii->hIcon == hIcon) {
					int index = CommandToIndex(nID);
					if (tpText) GetStatusBarCtrl().SetTipText(index, tpText);
				}
				else {
					//if (ii->hIcon) DestroyIcon(ii->hIcon); // Let the owner handle it
					ii->hIcon = hIcon;
		
					// Better way to get the width ?
					BITMAP bm;	
					ICONINFO iconinfo;
					if (GetIconInfo(hIcon, &iconinfo)) {
						GetObject(iconinfo.hbmMask, sizeof(BITMAP), &bm);
						ii->lWidth = bm.bmWidth;
						DeleteObject(iconinfo.hbmColor);
						DeleteObject(iconinfo.hbmMask);
					}
					else ii->lWidth = 16;
				
				/*int index = CommandToIndex(nID);
				SetPaneInfo(index, nID, SBPS_NORMAL|SBPS_NOBORDERS, ii->lWidth);
				if (hIcon) GetStatusBarCtrl().SetIcon(index, hIcon);
				if (tpText) GetStatusBarCtrl().SetTipText(index, tpText);
				RedrawWindow();*/

					// Why do I have to refresh the panes ?
					RefreshPanes();
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL CMyStatusBar::AddIcon(UINT nID)
{
	BOOL idOk = TRUE;
	int count = arrIcons.GetSize();
	for (int i=0; i<count; i++)
		if (arrIcons[i].nID == nID) {
			idOk = FALSE;
			break;
		}

	if (idOk)
	{
		struct icon_info ii;
		ii.nID = nID;
		ii.csTpText = "";
		ii.hIcon = NULL;
		ii.lWidth = 0;
		arrIcons.Add(ii);
		RefreshPanes();
	}

	return (idOk);
}

BEGIN_MESSAGE_MAP(CMyStatusBar, CStatusBar)
   //{{AFX_MSG_MAP(CMyStatusBar)
   ON_WM_CREATE()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONDBLCLK()
   ON_WM_MBUTTONDOWN()
   ON_WM_MBUTTONDBLCLK()
   ON_WM_RBUTTONDBLCLK()
   ON_WM_RBUTTONDOWN()
   //}}AFX_MSG_MAP
   ON_WM_LBUTTONUP()
   ON_WM_RBUTTONUP()
   ON_WM_MBUTTONUP()
END_MESSAGE_MAP()

void CMyStatusBar::GetItemRect(UINT idx, LPRECT r)
{
	//Stupid fix for a stupid bug
	
		ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));

	CStatusBar::GetItemRect(idx, r);

	int width = 16;
	UINT nID = GetItemID(idx);
	int count = arrIcons.GetSize();
    for (int i=0;i<count;i++)
		if (arrIcons[i].nID == nID) {
			width = arrIcons[i].lWidth;
			break;
		}

	if ( idx>1 && r->right-r->left < width) r->right = r->left+width+GetSystemMetrics(SM_CXEDGE);
}

void CMyStatusBar::OnLButtonDown(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(WM_COMMAND, id, 0);
      GetParentFrame()->SendMessage(SB_LBUTTONDOWN, id, 0);
      return;
   }

   CStatusBar::OnLButtonDown(nFlags, point);
}

void CMyStatusBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_LBUTTONDBLCLK, id, 0);
      return;
   }

   CStatusBar::OnLButtonDblClk(nFlags, point);
}

void CMyStatusBar::OnMButtonDown(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_MBUTTONDOWN, id, 0);
      return;
   }

   CStatusBar::OnMButtonDown(nFlags, point);
}

void CMyStatusBar::OnMButtonDblClk(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_MBUTTONDBLCLK, id, 0);
      return;
   }

   CStatusBar::OnMButtonDblClk(nFlags, point);
}

void CMyStatusBar::OnRButtonDblClk(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_RBUTTONDBLCLK, id, 0);
      return;
   }

   CStatusBar::OnRButtonDblClk(nFlags, point);
}

void CMyStatusBar::OnRButtonDown(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_RBUTTONDOWN, id, 0);
      return;
   }

   CStatusBar::OnRButtonDown(nFlags, point);
}

void CMyStatusBar::OnLButtonUp(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_LBUTTONUP, id, 0);
      return;
   }
   
   CStatusBar::OnLButtonUp(nFlags, point);
}

void CMyStatusBar::OnRButtonUp(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_RBUTTONUP, id, 0);
      return;
   }

   CStatusBar::OnRButtonUp(nFlags, point);
}

void CMyStatusBar::OnMButtonUp(UINT nFlags, CPoint point)
{
   int id = HitTest(point);
   if (id != -1 ) {
      GetParentFrame()->SendMessage(SB_MBUTTONUP, id, 0);
      return;
   }
   
   CStatusBar::OnMButtonUp(nFlags, point);
}

int CMyStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatusBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Set the default status bar font, I wonder why windows doesn't
	// set it automatically
	NONCLIENTMETRICS ncm ;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0))
		return FALSE;

	if (m_statusFont.CreateFontIndirect(&ncm.lfStatusFont))
		SetFont(&m_statusFont);

	UINT indicators[2] = {ID_SEPARATOR, ID_PROG_BAR};
	SetIndicators(indicators, 2);
	SetPaneStyle(0, SBPS_STRETCH);
	return 0;
}

int CMyStatusBar::HitTest(POINT point) 
{
   // Check to see if the mouse click was within one of the icon
   RECT rc;
   int count = arrIcons.GetSize();
   for (int i=0;i<count;i++)
   {
      GetItemRect(i+2, &rc );
      if(PtInRect(&rc, point))
         return arrIcons[i].nID;
   }
   return -1;
}
    

/////////////////////////////////////////////////////////////////////////////
void CBrowserFrame::OnSysColorChange() 
{
    CFrameWnd::OnSysColorChange();
    
    //-------------------------
    // Reload background image:
    //-------------------------
	//m_bmpBack.DeleteObject();
    //SetBackImage ();
}

/////////////////////////////////////////////////////////////////////////////
void CBrowserFrame::SetBackImage()
{
	HBITMAP bmpBack = theApp.skin.GetBackImage();
	if (!bmpBack) return;
    CReBarCtrl& rc = m_wndReBar.GetReBarCtrl ();

    REBARBANDINFO info;
    memset (&info, 0, sizeof (REBARBANDINFO));
    info.cbSize = sizeof (info);
    info.hbmBack = theApp.preferences.bToolbarBackground ? (HBITMAP)bmpBack : NULL;

    int count = rc.GetBandCount();
    for (int i = 0; i < count; i++)  {
        info.fMask = RBBIM_BACKGROUND;
        BOOL blah = rc.SetBandInfo (i, &info);

      /*  CRect rectBand;
        rc.GetRect (i, rectBand);

        rc.InvalidateRect (rectBand);
        rc.UpdateWindow ();

        info.fMask = RBBIM_CHILD;
        rc.GetBandInfo (i, &info);

        if (info.hwndChild != NULL)
        {
            ::InvalidateRect (info.hwndChild, NULL, TRUE);
            ::UpdateWindow (info.hwndChild);
        }*/
    }

	m_wndReBar.RedrawWindow(0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
#ifdef INTERNAL_SIDEBAR	
	// Set the backimage to the sidebar title too
	CReBarCtrl& rc2 = m_wndSideBar.m_wndRebar.GetReBarCtrl();
	info.fMask = RBBIM_BACKGROUND;
	rc2.SetBandInfo (0, &info);
#endif
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CBrowserFrame::RefreshMRUList(WPARAM ItemID, LPARAM unused)
{
    m_wndUrlBar.LoadMRUList();
    return 0;
}

void CBrowserFrame::ToggleToolBar(UINT uID)
{
   CBrowserFrame* pBrowserFrame;
   POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
      pBrowserFrame->m_wndReBar.ToggleVisibility(uID - TOOLBAR_MENU_START_ID);
   }
}

void CBrowserFrame::ToggleToolbarLock()
{
    BOOL locked = theApp.preferences.GetBool(PREF_TOOLBAND_LOCKED, false);
    locked = !locked;
    theApp.preferences.SetBool(PREF_TOOLBAND_LOCKED, locked);
    CBrowserFrame* pBrowserFrame;
    POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
    while( pos != NULL ) {
       pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
       pBrowserFrame->m_wndReBar.LockBars(locked);
    }
}

#ifdef INTERNAL_SIDEBAR
void CBrowserFrame::ToggleSideBar(UINT uID)
{
    m_wndSideBar.ToggleVisibility(uID - SIDEBAR_MENU_START_ID);
}

void CBrowserFrame::OnUpdateSideBarMenu(CCmdUI* pCmdUI)
{
	if (m_wndSideBar.GetCurrent() == pCmdUI->m_nID)
        pCmdUI->SetCheck(true);
	else
		pCmdUI->SetCheck(false);
}
#endif

void CBrowserFrame::OnUpdateToolBarMenu(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(
		m_wndReBar.GetVisibility(pCmdUI->m_nID - TOOLBAR_MENU_START_ID)
	);
}

void CBrowserFrame::OnUpdateToggleToolbarLock(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(theApp.preferences.GetBool(PREF_TOOLBAND_LOCKED, false));
}


BOOL CALLBACK EnumToolbar(HWND hwnd, LPARAM lParam)
{
	TCHAR pszName[20];
	GetClassName(hwnd, pszName, 20);
	if ( GetParent(hwnd) == GetParent((HWND)lParam) && _tcscmp(TOOLBARCLASSNAME, pszName) == 0
		&& ( (GetWindowLong(hwnd, GWL_STYLE) & CCS_BOTTOM) == CCS_BOTTOM))
	{
		SetWindowPos((HWND)lParam, hwnd ,0,0,0,0,SWP_NOMOVE);
		//return FALSE;
	}
	return TRUE;
}

// create a linked list of CToolBarEx controls, return the hwnd
CToolBarEx* CBrowserFrame::CreateToolbar(UINT style) {
    return m_tbList.Add(this, style);
}

#include "nsITypeAheadFind.h"
#include ".\browserfrmtab.h"

void CBrowserFrame::OnShowFindBar()
{
	GetActiveView()->Activate(FALSE);
   // When the the user chooses the Find menu item
    // and if a Find dlg. is already being shown
    // just set focus to the existing dlg instead of
    // creating a new one
    if(m_wndFindBar)
    {
		m_wndFindBar->SetFocus();
		return;
    }

#ifdef FINDBAR_USE_TYPEAHEAD
	nsresult rv;
    nsCOMPtr<nsITypeAheadFind> tafinder = do_GetService(NS_TYPEAHEADFIND_CONTRACTID, &rv);
	if (NS_SUCCEEDED(rv))
	{
		PRBool b;
		tafinder->GetIsActive(&b);
		if (b == PR_FALSE)
		{
			nsCOMPtr<nsIDOMWindow> dom(do_GetInterface(GetActiveView()->mWebBrowser));
			tafinder->StartNewFind(dom, PR_FALSE);
		}
	}
#endif
    
	// Create the find bar
	m_wndFindBar = new CFindRebar(m_searchString, 
		theApp.preferences.bFindMatchCase, 
		FALSE, 
		theApp.preferences.bFindWrapAround, 
		theApp.preferences.bFindHighlight, this);

	m_wndFindBar->Create(this, CBRS_ALIGN_BOTTOM);
	
	// It must stay above the sidebar 
	//m_wndFindBar->SetWindowPos(&m_wndStatusBar ,0,0,0,0,SWP_NOMOVE);
	
	// And above any bottom toolbar ?
	EnumChildWindows(m_hWnd, EnumToolbar, (LPARAM)m_wndFindBar->m_hWnd); 

	// Update the layout
	RecalcLayout();
}

void CBrowserFrame::ClearFindBar()
{
	delete m_wndFindBar;
	m_wndFindBar = NULL;
	RecalcLayout();
	// WORKAROUND: Setting back the focus to gecko
	//m_wndBrowserView.mBaseWindow->SetFocus(); // WHY IT DOES NOTHING ?
	if (GetActiveView())
		GetActiveView()->Activate(TRUE);
}

#ifdef INTERNAL_SITEICONS
void CBrowserFrame::SetFavIcon(int iIcon)
{
	COMBOBOXEXITEM cb;
	cb.mask = CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
	cb.iSelectedImage = cb.iImage = iIcon;
	cb.iItem = -1;
	m_wndUrlBar.SetItem(&cb);

	if (theApp.preferences.GetBool("kmeleon.favicons.titleBar", FALSE))
	{
		HICON icon = GetIcon(FALSE);
		if (icon!=theApp.GetDefaultIcon(FALSE))
			DestroyIcon(icon);

		if (iIcon == theApp.favicons.GetDefaultIcon())
		{
			SetIcon(theApp.GetDefaultIcon(TRUE), TRUE);
			SetIcon(theApp.GetDefaultIcon(FALSE), FALSE);
		} else {
			icon = theApp.favicons.ExtractIcon(iIcon);
			SetIcon(icon, FALSE);
			SetIcon(icon, TRUE);
		}
	}
/*	cb.iSelectedImage = cb.iImage = I_IMAGECALLBACK;
	cb.iItem = 0;
	mpBrowserFrame->m_wndUrlBar.SetItem(&cb);
	cb.iItem = 1;
	mpBrowserFrame->m_wndUrlBar.SetItem(&cb);
	cb.iItem = 2;
	mpBrowserFrame->m_wndUrlBar.SetItem(&cb);*/
}

LRESULT CBrowserFrame::OnNewSiteIcon(WPARAM url, LPARAM index)
{
	int i = theApp.favicons.GetIcon(GetActiveView()->GetBrowserGlue()->mIconURI);
	
	if (i==-1) {// The icon doesn't exist anymore, was deleted
		GetActiveView()->GetBrowserGlue()->mIconURI = nullptr;
		SetFavIcon(theApp.favicons.GetDefaultIcon());
	}
	else
		if (index==-1 || index == i) SetFavIcon(i);

	return 0;
}
#endif

void CBrowserFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_MINIMIZE) {
		// We're taking care of the focus here, because it will
		// be lost after that and not correctly memorized.
        m_wndLastFocused = ::GetFocus();
		if (::IsChild(GetActiveView()->m_hWnd, m_wndLastFocused))
		   GetActiveView()->Activate(FALSE);
	} else if (nID == SC_RESTORE) {
		if (!(GetStyle() & WS_DLGFRAME) && IsZoomed())
			ModifyStyle(0, WS_THICKFRAME);
	}
	else if (nID == SC_MAXIMIZE) {
		if (!(GetStyle() & WS_DLGFRAME))
			ModifyStyle(WS_THICKFRAME, 0);
	}
	CFrameWnd::OnSysCommand(nID, lParam);
}

INT_PTR CBrowserFrame::DoModal()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp != NULL)
		pApp->EnableModeless(FALSE);
	/*
	HWND hWndTop = NULL;
	HWND hWndParent = CWnd::GetSafeOwner_(GetParent()->GetSafeHwnd(), &hWndTop);

	BOOL bEnableParent = FALSE;
	if (hWndParent && hWndParent != ::GetDesktopWindow() && ::IsWindowEnabled(hWndParent))
	{
		::EnableWindow(hWndParent, FALSE);
		::EnableWindow(m_hWnd, TRUE);
		bEnableParent = TRUE;
	}*/

	m_nFlags |= WF_CONTINUEMODAL;
	RunModalLoop(0);
	/*
	if (bEnableParent)
		::EnableWindow(hWndParent, TRUE);
	if (hWndParent != NULL && ::GetActiveWindow() == m_hWnd)
		::SetActiveWindow(hWndParent);

	if (::IsWindow(hWndTop))
		::EnableWindow(hWndTop, TRUE);
		*/
	if (pApp != NULL)
		pApp->EnableModeless(TRUE);

	INT_PTR res = m_nModalResult;
	DestroyWindow();
	return res;
}

void CBrowserFrame::OnRbnLayoutChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   *pResult = 0;

   if ( !(m_chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR) ||
        !(m_chromeMask & nsIWebBrowserChrome::CHROME_MENUBAR) ||
        !(m_chromeMask & nsIWebBrowserChrome::CHROME_LOCATIONBAR) )
      return;

   m_wndReBar.SaveBandSizes();
   CBrowserFrame* pBrowserFrame;
   POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
      pBrowserFrame->m_wndReBar.RestoreBandSizes();
   }
}

HACCEL CBrowserFrame::GetDefaultAccelerator()
{
	if (!(m_chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
		return theApp.accel.GetTable();
	return NULL;
}

void CBrowserFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	// This will rebuild the menu if needed
	theApp.menus.Activate(pPopupMenu); 
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}

void CBrowserFrame::OnCookiesViewer()
{
	CCookiesViewerDlg dlg(this);
	dlg.DoModal();
}

void CBrowserFrame::OnPasswordsViewer()
{
	CPasswordViewerDlg dlg(this);
	dlg.DoModal();
}

void CBrowserFrame::OnCookiePermissions() {
	CPermissionsDlg dlg("cookie", this);
	dlg.DoModal();
}

void CBrowserFrame::OnImagePermissions() {
	CPermissionsDlg dlg("image", this);
	dlg.DoModal();
}

void CBrowserFrame::OnPopupPermissions() {
	CPermissionsDlg dlg("popup", this);
	dlg.DoModal();
}

void CBrowserFrame::OnWindowPrev()
{
	CFrameWnd* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(this);
	theApp.m_FrameWndLst.GetNext(pos);
	if (pos)  pFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetNext(pos);
	else      pFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetHead();

	pFrame->ActivateFrame();
}

void CBrowserFrame::OnWindowNext()
{
	CFrameWnd* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(this);
	theApp.m_FrameWndLst.GetPrev(pos);
	if (pos)  pFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetPrev(pos);
	else      pFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetTail();

	pFrame->ActivateFrame();
}

void CBrowserFrame::OnSelectUrl()
{
	m_wndUrlBar.SetFocus();
}

void CBrowserFrame::OnUpdateViewStatusBar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndStatusBar.IsWindowVisible());
}

void CBrowserFrame::OnFind(BOOL backward)
{
	CBrowserView* view = GetActiveView();
	if (!view) return;

	CBrowserWrapper* wrapper = view->GetBrowserWrapper();
	
	const wchar_t* searchString = NULL;
	BOOL ahead = FALSE;
	if (m_wndFindBar) {
		searchString = m_wndFindBar->GetUFindString();
		ahead = m_wndFindBar->StartSel();
		if (m_searchString) free(m_searchString);
		m_searchString = wcsdup(searchString);
	}

	BOOL didFind = wrapper->Find(searchString, 
				theApp.preferences.bFindMatchCase, 
				theApp.preferences.bFindWrapAround, 
				backward, ahead);

	if (!didFind) {
		if (!m_wndFindBar) OnShowFindBar();
		if (m_wndFindBar && m_searchString && *m_searchString) 
			m_wndFindBar->OnNotFound();
	} else if (m_wndFindBar)
		m_wndFindBar->OnFound();
}

void CBrowserFrame::OnFindNext()
{
	OnFind(FALSE);
}

void CBrowserFrame::OnFindPrev()
{
	OnFind(TRUE);
}

void CBrowserFrame::OnWrapAround()
{
	if (!m_wndFindBar)
		theApp.preferences.bFindWrapAround = !theApp.preferences.bFindWrapAround;
	else
		theApp.preferences.bFindWrapAround = m_wndFindBar->WrapAround();
}

void CBrowserFrame::OnMatchCase()
{
	if (theApp.preferences.bFindHighlight) 
		GetActiveView()->Highlight(NULL, theApp.preferences.bFindMatchCase);

	if (!m_wndFindBar) 
		theApp.preferences.bFindMatchCase = !theApp.preferences.bFindMatchCase;
	else
		theApp.preferences.bFindMatchCase = m_wndFindBar->MatchCase();

	if (theApp.preferences.bFindHighlight) 
		GetActiveView()->Highlight(m_searchString, theApp.preferences.bFindMatchCase);
}

void CBrowserFrame::OnHighlight()
{
	theApp.preferences.bFindHighlight = m_wndFindBar->Highlight();
	GetActiveView()->Highlight(NULL, theApp.preferences.bFindMatchCase);
	if (theApp.preferences.bFindHighlight)
		GetActiveView()->Highlight(m_searchString, theApp.preferences.bFindMatchCase);
}

void CBrowserFrame::OpenURL(LPCTSTR url, LPCTSTR refferer, BOOL focusUrl, BOOL allowFixup)
{
	GetActiveView()->OpenURL(url, refferer, allowFixup);
	if (focusUrl) m_wndUrlBar.SetFocus();
}

void CBrowserFrame::OnViewStatusBar()
{
	if (m_wndStatusBar.IsVisible()) {
		theApp.preferences.SetBool("kmeleon.display.statusbar", FALSE);
		m_wndStatusBar.ShowWindow(SW_HIDE);
	} else {
		theApp.preferences.SetBool("kmeleon.display.statusbar", TRUE);
		m_wndStatusBar.ShowWindow(SW_SHOW);
	}
	RecalcLayout();
}

/****************************************************************************/

void CBrowserFrame::UpdateSecurityStatus(PRInt32 aState)
{
	UINT tpTextId, iconResID;
	TCHAR* icoFile;

	HICON securityIcon = NULL;
	if(aState & nsIWebProgressListener::STATE_IS_INSECURE) {
		securityIcon = theApp.skin.GetIconInsecure();
		m_wndUrlBar.Highlight(0);
		tpTextId = IDS_SECURITY_UNLOCK;
	}
	else if(aState & nsIWebProgressListener::STATE_IS_BROKEN) {
		securityIcon = theApp.skin.GetIconBroken();
		m_wndUrlBar.Highlight(2);
		tpTextId = IDS_SECURITY_BROKEN;
	}  
	else if(aState & nsIWebProgressListener::STATE_IS_SECURE) {
		securityIcon = theApp.skin.GetIconSecure();
		m_wndUrlBar.Highlight(1);
		tpTextId = IDS_SECURITY_LOCK;
	}
	else 
	{
		ASSERT(FALSE);
		return;
	}

   CString tpText;
   tpText.LoadString(tpTextId);
   m_wndStatusBar.SetIconInfo(ID_SECURITY_STATE_ICON, securityIcon, tpText);
}

void CBrowserFrame::UpdateStatus(LPCTSTR aStatus)
{
	if (!m_wndStatusBar.m_hWnd) return;
	m_wndStatusBar.SetPaneText(0, aStatus);
}

void CBrowserFrame::UpdateSiteIcon(int aIcon)
{
	SetFavIcon(aIcon);
}

void CBrowserFrame::UpdateLocation(LPCTSTR aLocation, BOOL aIgnoreTyping)
{
	if (!aLocation) return;
	// XXX Since Mozilla 1.8.0.2 about:blank is always passed here
	// before anything else, broking stuffs, so ignore it!
	//if ( _tcscmp(aLocation, _T("about:blank")) == 0 &&
//		m_wndUrlBar.GetEnteredURL().GetLength())
		//return;

	m_wndUrlBar.SetCurrentURL(aLocation, aIgnoreTyping);
}

void CBrowserFrame::UpdateProgress(int aCurrent, int aMax)
{
   m_wndProgressBar.SetRange32(0, aMax);
   m_wndProgressBar.SetPos(aCurrent);
}

void CBrowserFrame::UpdateLoading(BOOL aLoading)
{
	if (!aLoading) {
		m_wndAnimate.Stop();
		m_wndAnimate.Seek(0);
		m_wndProgressBar.ShowWindow(SW_HIDE);

		CString szUrl = m_wndUrlBar.GetEnteredURL();
		
		if (szUrl.CompareNoCase(_T("about:blank"))==0)
			m_wndUrlBar.SetFocus();
	}
	else {
		m_wndAnimate.Play(0, -1, -1);
		m_wndProgressBar.ShowWindow(SW_SHOW);
	}
}


void CBrowserFrame::UpdateTitle(LPCTSTR aTitle)
{
	CString title = aTitle;
	if (title.IsEmpty())
		title = GetActiveView()->GetBrowserGlue()->mLocation;

	if (!IsDialog()) {
		CString appTitle;
		appTitle.LoadString(AFX_IDS_APP_TITLE);
		appTitle = theApp.preferences.GetString("kmeleon.display.title", appTitle);
		if (!appTitle.IsEmpty())
			title += _T(" - ") + appTitle;
	}

	SetWindowText(title);
	UpdateSHistoryMenu();
}

void CBrowserFrame::UpdatePopupNotification(LPCTSTR uri)
{
	if (uri && *uri) {
		if (m_wndStatusBar.AddIcon(ID_POPUP_BLOCKED_ICON)) {
			HICON popupIcon = theApp.skin.GetIconPopupBlock();
			CString tpText;
			tpText.Format(IDS_POPUP_BLOCKED, uri);
			m_wndStatusBar.SetIconInfo(ID_POPUP_BLOCKED_ICON, popupIcon, tpText);
		}
	}
	else
		m_wndStatusBar.RemoveIcon(ID_POPUP_BLOCKED_ICON);
}

void CBrowserFrame::OnMaximizeWindow()
{
	if (!(GetStyle() & WS_DLGFRAME))
		ModifyStyle(WS_THICKFRAME, 0);		
	ShowWindow(SW_MAXIMIZE);
}

void CBrowserFrame::OnMinimizeWindow()
{
	ShowWindow(SW_MINIMIZE);		 
} 

void CBrowserFrame::OnRestoreWindow()
{
	if (!(GetStyle() & WS_DLGFRAME))
		ModifyStyle(0, WS_THICKFRAME);
	ShowWindow(SW_RESTORE);	
}

void CBrowserFrame::ToggleWindow()
{
	if (IsZoomed())
		OnRestoreWindow();
	else
		OnMaximizeWindow();
}

void CBrowserFrame::OnToggleWindow()
{
	ToggleWindow();
}

void CBrowserFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if (this->GetStyle() & WS_CAPTION)
		CFrameWnd::OnGetMinMaxInfo(lpMMI);
	else
	{
		CRect workArea;
		if (::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea,0)) {
			lpMMI->ptMaxPosition.x = workArea.left;
			lpMMI->ptMaxPosition.y = workArea.top;
			lpMMI->ptMaxSize.x = workArea.Width();
			lpMMI->ptMaxSize.y = workArea.Height();
			return;
		}

		CRect rectDesktop;
		::GetWindowRect(::GetDesktopWindow(), &rectDesktop);

		APPBARDATA abd;
		abd.cbSize = sizeof(abd);
		UINT uState = (UINT) SHAppBarMessage(ABM_GETSTATE, &abd); 
		if ((uState & ABS_ALWAYSONTOP) && !(uState & ABS_AUTOHIDE))
		{
			BOOL fResult = (BOOL) SHAppBarMessage(ABM_GETTASKBARPOS, &abd); 
			rectDesktop.SubtractRect(rectDesktop, &abd.rc);
		}

		AdjustWindowRectEx(rectDesktop, 
			GetWindowLong(m_hWnd, GWL_STYLE), 
			GetMenu() ? TRUE : FALSE, 
			GetWindowLong(m_hWnd, GWL_EXSTYLE)); 

		lpMMI->ptMaxPosition.x = rectDesktop.left;
		lpMMI->ptMaxPosition.y = rectDesktop.top;
		lpMMI->ptMaxSize.x = rectDesktop.Width();
		lpMMI->ptMaxSize.y = rectDesktop.Height();
	}
}

LRESULT CBrowserFrame::OnToolbarCommand(WPARAM wParam, LPARAM lParam)
{
	CToolBar* toolbar = (CToolBar*)CWnd::FromHandle((HWND)lParam);
	if (!toolbar) return CWnd::Default();
	KmToolbar* ktoolbar = (KmToolbar*)::GetProp(toolbar->GetSafeHwnd(), _T("kmToolbar"));
	if (!ktoolbar) return CWnd::Default();
	KmButton* button = ktoolbar->GetButton(wParam);
	if (!button) {
		CPoint p;
		::GetCursorPos(&p);
		if (!IsZoomed()) {
			SendMessage(WM_SYSCOMMAND, SC_MOVE+1, MAKELPARAM(p.x,p.y));
			return 1;
		}
		return CWnd::Default();
	}
	if (button->mAction[0] == _T('@')) {
		CMenu* menu = theApp.menus.GetMenu(button->mAction.Mid(1));
		if (menu) {				
			CRect rc;
			toolbar->GetItemRect(toolbar->CommandToIndex(wParam), &rc);
			CPoint pt(rc.left, rc.bottom);
			toolbar->ClientToScreen(&pt);
			menu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);
		}
	}

	return CWnd::Default();
}

LRESULT CBrowserFrame::OnToolbarContextMenu(WPARAM wParam, LPARAM lParam)
{
	CToolBar* toolbar = (CToolBar*)CWnd::FromHandle((HWND)lParam);
	if (toolbar) {
		
		CPoint pt;
		if (wParam) {
			CRect rc;
			toolbar->GetItemRect(toolbar->CommandToIndex(wParam), &rc);
			pt.SetPoint(rc.left, rc.bottom);
			toolbar->ClientToScreen(&pt);
		} else 
			GetCursorPos(&pt);

		if (wParam == ID_NAV_BACK || wParam == ID_NAV_FORWARD) {
			CMenu menu;
			menu.CreatePopupMenu();
			wParam == ID_NAV_BACK ? DrawSHBackMenu(menu.GetSafeHmenu()) : DrawSHForwardMenu(menu.GetSafeHmenu());
			menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);
		} else {
			KmToolbar* ktoolbar = (KmToolbar*)::GetProp(toolbar->GetSafeHwnd(), _T("kmToolbar"));
			if (ktoolbar) {
				KmButton* button = ktoolbar->GetButton(wParam);
				CMenu* menu = button && button->mMenuName.GetLength() ? theApp.menus.GetMenu(button->mMenuName) : theApp.menus.GetMenu(_T("Toolbars"));
				if (menu) menu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);				
			}
		}
	}
	return CWnd::Default();
}

void CBrowserFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	if (GetActiveView()->OnMenuSelect(nItemID, nFlags, hSysMenu))
		return;

	CFrameWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);
}

BOOL CBrowserFrame::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	return CFrameWnd::OnToolTipText(id, pNMHDR, pResult);
}

void CBrowserFrame::AllowJS(BOOL allow)
{
	GetActiveView()->GetBrowserWrapper()->AllowJS(allow);
}
