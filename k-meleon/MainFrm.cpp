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

#include "stdafx.h"
#include <wininet.h>
#include "KMeleon.h"
#include "KMeleonDoc.h"
#include "MainFrm.h"
#include "LinkButton.h"
#include "Mozilla.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static nsVoidArray sMainFrameList;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
  ON_WM_CLOSE()
  ON_WM_ACTIVATE()
  ON_COMMAND(ID_LINK_1, OnLink1)
	ON_COMMAND(ID_VIEW_ADDRESS_BAR, OnViewAddressBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ADDRESS_BAR, OnUpdateViewAddressBar)
	ON_COMMAND(ID_VIEW_LINKS_BAR, OnViewLinksBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINKS_BAR, OnUpdateViewLinksBar)
	ON_COMMAND(ID_VIEW_TEXTLABELS, OnViewTextlabels)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TEXTLABELS, OnUpdateViewTextlabels)
	ON_WM_SYSCOLORCHANGE()
	ON_COMMAND(ID_HELP_WEB_KMELEON_HOME, OnHelpWebKmeleonHome)
	ON_COMMAND(ID_HELP_WEB_KMELEON_FORUM, OnHelpWebKmeleonForum)
	ON_COMMAND(ID_GO_START_PAGE, OnGoStartPage)
	ON_COMMAND(ID_GO_SEARCH_THE_WEB, OnGoSearchTheWeb)
	ON_COMMAND(ID_FILE_NEWWINDOW, OnFileNewwindow)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_VIEW_CUSTOMIZE, OnViewCustomize)
	ON_COMMAND(ID_VIEW_PREFERENCES, OnViewPreferences)
	ON_REGISTERED_MESSAGE(BCGM_RESETTOOLBAR, OnToolbarReset)
	ON_REGISTERED_MESSAGE(BCGM_TOOLBARMENU, OnToolbarContextMenu)
	ON_REGISTERED_MESSAGE(BCGM_CUSTOMIZEHELP, OnHelpCustomizeToolbars)
	ON_CBN_SELENDOK(AFX_IDW_TOOLBAR + 1,OnNewAddress)
	ON_COMMAND_RANGE(FIRST_HISTORY_COMMAND, FIRST_HISTORY_COMMAND + HISTORY_LEN - 1, OnHistory)
	ON_COMMAND(IDOK, OnNewAddressEnter)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bMainToolbarMenu = FALSE;
	m_iFolderIcon = 0;
	m_iInternetShortcutIcon = -1;
	m_iAnimationStarted = 0;
  m_browserCreated = 0;

	sMainFrameList.AppendElement(this);
}

CMainFrame::~CMainFrame() {
	sMainFrameList.RemoveElement(this);

  // if we were the last window, close the application
  if (sMainFrameList.Count() == 0){
    theApp.m_pMainWnd->PostMessage(WM_QUIT);
    return;
  }
}

void CMainFrame::OnClose(){
  // save the toolbar state

  // don't quit if we are loading an document or else mozilla crashes
  if (m_iAnimationStarted){
    CMozilla *moz = (CMozilla *)GetActiveView();
    moz->SendMessage(WM_COMMAND, ID_STOP);
    Sleep(100);
    PostMessage(WM_CLOSE, 0, 0);
    return;
  }

  CFrameWnd::OnClose();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct){
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//CBCGToolBar::EnableQuickCustomization ();

	LoadBackImage ();

	//---------------------------------
	// Set toolbar and menu image size:
	//---------------------------------
	//CBCGToolBar::SetSizes (CSize (36, 28), CSize (22, 20));  // big buttons
	CBCGToolBar::SetSizes (CSize (22, 20), CSize (16, 16));  // small buttons
	CBCGToolBar::SetMenuSizes (CSize (22, 21), CSize (16, 15));

	CBCGToolBar::SetHotTextColor (::GetSysColor (COLOR_HIGHLIGHT));

	//--------------------
	// Create the menubar:
	//--------------------
	if (!m_wndMenuBar.CreateEx (this, TBSTYLE_TRANSPARENT))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetBarStyle(m_wndMenuBar.GetBarStyle() | CBRS_SIZE_DYNAMIC);
	m_wndMenuBar.EnableCustomizeButton (TRUE, (UINT)-1, _T(""));

	//-----------------------
	// Disable menu shaddows:
	//-----------------------
	CBCGMenuBar::EnableMenuShadows (FALSE);

	//------------------------------------
	// Remove menubar gripper and borders:
	//------------------------------------
	m_wndMenuBar.SetBarStyle (m_wndMenuBar.GetBarStyle() &
		~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));


	//------------------------------
	// Create the animation control:
	//------------------------------
	if (!m_wndAnimate.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), this, AFX_IDW_TOOLBAR + 2) ||
		!m_wndAnimate.Open(IDR_MFCAVI))
	{
		TRACE0("Failed to create aimation\n");
		return -1;      // fail to create
	}

	if (!m_wndToolBar.CreateEx (this, TBSTYLE_TRANSPARENT) ||
//		!m_wndToolBar.LoadBitmap (IDB_HOTTOOLBAR, IDB_COLDTOOLBAR, IDB_MENU_IMAGES) ||
		!m_wndToolBar.LoadBitmap (IDB_HOTTOOLBAR_SMALL, IDB_COLDTOOLBAR_SMALL, IDB_MENU_IMAGES) ||
		!m_wndToolBar.RestoreOriginalstate ())
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	//------------------------------------
	// Remove toolbar gripper and borders:
	//------------------------------------
	m_wndToolBar.SetBarStyle (m_wndToolBar.GetBarStyle() &
		~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.EnableCustomizeButton (TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));

	//----------------------------------------
	// Create a combo box for the address bar:
	//----------------------------------------
	CString strAddressLabel;
	strAddressLabel.LoadString(IDS_ADDRESS);

	if (!m_wndAddress.Create (CBS_DROPDOWN | WS_CHILD, CRect(0, 0, 200, 120), this, AFX_IDW_TOOLBAR + 1))
	{
		TRACE0("Failed to create combobox\n");
		return -1;      // fail to create
	}

	//------------------
	// Create links bar:
	//------------------
	CString strLinksLabel;
	strLinksLabel.LoadString (IDS_LINKS);

	if (!m_wndLinksBar.CreateEx (this, TBSTYLE_TRANSPARENT,
		dwDefaultToolbarStyle, CRect(1, 1, 1, 1), AFX_IDW_TOOLBAR + 3))
	{
		TRACE0("Failed to create links bar\n");
	}

	m_wndLinksBar.EnableCustomizeButton (TRUE, -1, _T(""));

	m_wndLinksBar.SetWindowText (strLinksLabel);
	m_wndLinksBar.SetBarStyle (m_wndLinksBar.GetBarStyle() &
		~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	//--------------
	// Create rebar:
	//--------------
	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndMenuBar) ||
		!m_wndReBar.AddBar(&m_wndToolBar, NULL, NULL,
				RBBS_GRIPPERALWAYS | RBBS_FIXEDBMP | RBBS_BREAK) ||
		!m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP) ||
		!m_wndReBar.AddBar(&m_wndAddress, strAddressLabel, NULL,
				RBBS_GRIPPERALWAYS | RBBS_FIXEDBMP | RBBS_BREAK) ||
		!m_wndReBar.AddBar(&m_wndLinksBar, strLinksLabel, NULL,
				RBBS_GRIPPERALWAYS | RBBS_FIXEDBMP | RBBS_BREAK))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.AdjustLayout ();
	m_wndToolBar.AdjustLayout ();
	m_wndLinksBar.AdjustLayout ();

	//--------------------------------------------------------------
	// Set up min/max sizes and ideal sizes for pieces of the rebar:
	//--------------------------------------------------------------
	REBARBANDINFO rbbi;

	CRect rectToolBar;
	m_wndToolBar.GetItemRect(0, &rectToolBar);

	rbbi.cbSize = sizeof(rbbi);
	rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_SIZE;
	rbbi.cxMinChild = rectToolBar.Width();
	rbbi.cyMinChild = rectToolBar.Height();
	rbbi.cx = rbbi.cxIdeal = rectToolBar.Width() * m_wndToolBar.GetCount ();
	m_wndReBar.GetReBarCtrl().SetBandInfo (1, &rbbi);

	CRect rectAddress;
	m_wndAddress.GetEditCtrl()->GetWindowRect(&rectAddress);

	rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
	rbbi.cyMinChild = rectAddress.Height() + 10;
  rbbi.cxMinChild = 0;
	rbbi.cxIdeal = 200;
	m_wndReBar.GetReBarCtrl().SetBandInfo (3, &rbbi);

  theApp.plugins.DoRebars(m_wndReBar.GetReBarCtrl().m_hWnd);

	//-------------------
	// Create status bar:
	//-------------------
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	CString strMainToolbarTitle;
	strMainToolbarTitle.LoadString (IDS_MAIN_TOOLBAR);
	m_wndToolBar.SetWindowText (strMainToolbarTitle);
	m_wndMenuBar.SetBarStyle(m_wndMenuBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndLinksBar.SetBarStyle(m_wndLinksBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

  // this function is actually called twice per window.
  // the first time hInstance is 0
  if (cs.hInstance){
    if (theApp.menus.GetMenu("Main")){
      cs.hMenu = theApp.menus.GetMenu("Main")->m_hMenu;
    }
  }

	return TRUE;
}

void CMainFrame::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized ){
  // We can't do this in OnCreate because it's possible the active view hasn't been created yet
  if (!m_browserCreated){
    CMozilla *p=(CMozilla *)GetActiveView();

    p->createBrowser();

    if (theApp.preferences.bStartHome){
      p->Navigate(&theApp.preferences.homePage);
    }else{
      p->Navigate(&(CString)_T("about:blank"));
    }

    m_browserCreated = 1;
  }
  CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) {
  if (nCode == CN_COMMAND){
    if (!pHandlerInfo){
      theApp.plugins.OnCommand(nID);
    }else if (theApp.plugins.OnUpdate(nID)){
      return true;
    }
  }

  return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnViewCustomize() {
	//------------------------------------
	// Create a customize toolbars dialog:
	//------------------------------------
	CBCGToolbarCustomize* pDlgCust = new CBCGToolbarCustomize (this,
		TRUE, /* Automatic menus scaning */
		BCGCUSTOMIZE_MENU_ANIMATIONS | BCGCUSTOMIZE_TEXT_LABELS);

	pDlgCust->Create ();
}

void CMainFrame::OnViewPreferences(){
  CPreferencesDlg prefDlg;
  prefDlg.DoModal();

  LoadBackImage();
  SetBackImage();
}

LRESULT CMainFrame::OnToolbarContextMenu(WPARAM wp,LPARAM lp) {
	m_bMainToolbarMenu = 
		DYNAMIC_DOWNCAST (	CMainToolBar, 
							CWnd::FromHandlePermanent ((HWND) wp)) != NULL;

	CPoint point (LOWORD (lp), HIWORD(lp));

	CMenu menu;
	VERIFY(menu.LoadMenu (IDR_POPUP_TOOLBAR));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CBCGPopupMenu* pPopupMenu = new CBCGPopupMenu;
	pPopupMenu->Create (this, point.x, point.y, pPopup->Detach ());

	return 0;
}

afx_msg LRESULT CMainFrame::OnToolbarReset(WPARAM /*wp*/,LPARAM) {
	// TODO: reset toolbar with id = (UINT) wp to its initial state:
	//
	// UINT uiToolBarId = (UINT) wp;
	// if (uiToolBarId == IDR_MAINFRAME)
	// {
	//		do something with m_wndToolBar
	// }

	return 0;
}

LRESULT CMainFrame::OnHelpCustomizeToolbars(WPARAM wp, LPARAM lp) {
	int iPageNum = (int) wp;

	CBCGToolbarCustomize* pDlg = (CBCGToolbarCustomize*) lp;
	ASSERT_VALID (pDlg);

	// TODO: show help about page number iPageNum


	return 0;
}

BOOL CMainFrame::OnShowPopupMenu (CBCGPopupMenu* pMenuPopup) {
  CBCGFrameWnd::OnShowPopupMenu (pMenuPopup);

	if (pMenuPopup == NULL) {
    return TRUE;
  }

	CBCGPopupMenuBar* pMenuBar = pMenuPopup->GetMenuBar ();
	ASSERT_VALID (pMenuBar);

	for (int i = 0; i < pMenuBar->GetCount (); i ++) {
		CBCGToolbarButton* pButton = pMenuBar->GetButton (i);
		ASSERT_VALID (pButton);
	}

	CBCGToolbarMenuButton* pParentButton = pMenuPopup->GetParentButton ();
	if (pParentButton == NULL) {
		return TRUE;
	}

	switch (pParentButton->m_nID) {
	case ID_GO_BACK:
	case ID_GO_FORWARD:
		{
			if (CBCGToolBar::IsCustomizeMode ()) {
				return FALSE;
			}

      /*
			CBCGIEDemoView* pView = ((CBCGIEDemoView*)GetActiveView());
			ASSERT_VALID (pView);

			CBCGIEDemoDoc* pDoc = pView->GetDocument();
			ASSERT_VALID(pDoc);

			_T_HistotyList lst;

			if (pParentButton->m_nID == ID_GO_BACK)
			{
				pDoc->GetBackList (lst);
			}
			else
			{
				pDoc->GetFrwdList (lst);
			}
			
			if (!lst.IsEmpty ())
			{
				pMenuPopup->RemoveAllItems ();

				for (POSITION pos = lst.GetHeadPosition (); pos != NULL;)
				{
					CHistoryObj* pObj = lst.GetNext (pos);
					ASSERT (pObj != NULL);

					pMenuPopup->InsertItem (
						CBCGToolbarMenuButton (pObj->GetCommand (), NULL, -1, 
												pObj->GetTitle ()));
				}
			}
      */
		}
	}

	return TRUE;
}

BOOL CMainFrame::OnDrawMenuImage (CDC* pDC,
								const CBCGToolbarMenuButton* pMenuButton,
								const CRect& rectImage)
{
	if (m_himSystem == NULL) {
		return FALSE;
	}

	ASSERT_VALID (pDC);
	ASSERT_VALID (pMenuButton);

	int iIcon = -1;

  /*
	if (pMenuButton->m_nID >= FIRST_FAVORITE_COMMAND &&
		pMenuButton->m_nID <= LAST_FAVORITE_COMMAND)
	{
		TCHAR ext[_MAX_EXT];
		_tsplitpath (m_astrFavoriteURLs [pMenuButton->m_nID - FIRST_FAVORITE_COMMAND], NULL, NULL, NULL, ext);

    m_URLIcons.Lookup (ext, iIcon);
  }
  else if (IsFavoritesMenu (pMenuButton))	// Maybe, favorits folder?
  {
    iIcon = m_iFolderIcon;
  }
  */

	if (iIcon == -1)
	{
		return FALSE;	// Don't draw it!
	}

	::ImageList_Draw (m_himSystem, iIcon, pDC->GetSafeHdc (), 
		rectImage.left + (rectImage.Width () - m_SysImageSize.cx) / 2, 
		rectImage.top + (rectImage.Height () - m_SysImageSize.cy) / 2, ILD_TRANSPARENT);

	return TRUE;
}

void CMainFrame::OnHistory(UINT nID){
  /*
	CBCGIEDemoView* pView = ((CBCGIEDemoView*)GetActiveView());
	ASSERT_VALID (pView);

	CBCGIEDemoDoc* pDoc = pView->GetDocument();
	ASSERT_VALID(pDoc);

	CHistoryObj* pObj = pDoc->Go (nID);
	ASSERT (pObj != NULL);

	pView->Navigate2 (pObj->GetURL (), 0, NULL);
  */
}

void CMainFrame::SetAddress(LPCTSTR lpszUrl){
	m_wndAddress.SetWindowText(lpszUrl);
}

void CMainFrame::StartAnimation(){
	m_wndAnimate.Play(0, -1, -1);
	m_iAnimationStarted = 1;
}

void CMainFrame::StopAnimation() {
	if(m_iAnimationStarted)	{
		m_wndAnimate.Stop();
		m_wndAnimate.Seek(0);
	}
	m_iAnimationStarted = 0;
}

void CMainFrame::OnLink1() {
	
}

void CMainFrame::OnNewAddress() {
	// gets called when an item in the Address combo box is selected
	// just navigate to the newly selected location.
	CString str;

	m_wndAddress.GetLBText(m_wndAddress.GetCurSel(), str);
	((CMozilla *)GetActiveView())->Navigate(&str);
}

void CMainFrame::OnNewAddressEnter() {
	// gets called when an item is entered manually into the edit box portion
	// of the Address combo box.
	// navigate to the newly selected location and also add this address to the
	// list of addresses in the combo box.
	CString url;

  // stop current document first or bad stuff happens
  ((CMozilla *)GetActiveView())->SendMessage(WM_COMMAND, ID_STOP);

	m_wndAddress.GetEditCtrl()->GetWindowText(url);
	((CMozilla *)GetActiveView())->Navigate(&url);

	COMBOBOXEXITEM item;

	item.mask = CBEIF_TEXT;
	item.iItem = 0;
	item.pszText = (LPTSTR)(LPCTSTR)url;
	m_wndAddress.InsertItem(&item);

  m_wndAddress.GetEditCtrl()->SetSel(0,4096,TRUE);

  // set focus to document so scrolling works as expected
  ((CMozilla *)GetActiveView())->SetFocus();
}

void CMainFrame::OnViewAddressBar() {
	m_wndReBar.GetReBarCtrl().ShowBand (3, !m_wndAddress.IsWindowVisible ());
}

void CMainFrame::OnUpdateViewAddressBar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck (m_wndAddress.IsWindowVisible ());
}

void CMainFrame::OnViewLinksBar() 
{
	ShowControlBar (&m_wndLinksBar, (m_wndLinksBar.GetStyle () & WS_VISIBLE) == 0, FALSE);
}

void CMainFrame::OnUpdateViewLinksBar(CCmdUI* pCmdUI){
	pCmdUI->SetCheck (m_wndLinksBar.IsWindowVisible ());
}

BOOL CMainFrame::OnMenuButtonToolHitTest (CBCGToolbarButton* pButton, TOOLINFO* pTI){
	ASSERT_VALID (pButton);
	ASSERT (pTI != NULL);

	return TRUE;
}
//***************************************************************************************
void CMainFrame::OnViewTextlabels() 
{
	m_wndToolBar.EnableTextLabels (!m_wndToolBar.AreTextLabels ());
}
//***************************************************************************************
void CMainFrame::OnUpdateViewTextlabels(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck (m_bMainToolbarMenu && m_wndToolBar.AreTextLabels ());
	pCmdUI->Enable (m_bMainToolbarMenu);
}
//***************************************************************************************
BOOL CMainFrame::GetToolbarButtonToolTipText (CBCGToolbarButton* pButton, CString& strTTText)
{
	CLinkButton* pLinkBtn = NULL;

	ASSERT_VALID (pButton);

	if (pButton->m_nID == ID_LINK_1 &&
		(pLinkBtn = DYNAMIC_DOWNCAST (CLinkButton, pButton)) != NULL)
	{
		strTTText = pLinkBtn->GetURL ();
		return TRUE;
	}

	return FALSE;	// Default tooltip text
}

//*******************************************************************************************
void CMainFrame::OnSysColorChange() 
{
	CFrameWnd::OnSysColorChange();
	
	//-------------------------
	// Reload background image:
	//-------------------------
	LoadBackImage ();
	SetBackImage ();
}

//***************************************************************************************
void CMainFrame::SetBackImage ()
{
	CReBarCtrl& rc = m_wndReBar.GetReBarCtrl ();

	for (UINT i = 0; i < rc.GetBandCount(); i++)
	{
		REBARBANDINFO info;
		memset (&info, 0, sizeof (REBARBANDINFO));
		info.cbSize = sizeof (info);
		info.fMask = RBBIM_BACKGROUND;
		info.hbmBack = theApp.preferences.bToolbarBackground ? (HBITMAP)m_bmpBack : NULL;
		rc.SetBandInfo (i, &info);

		CRect rectBand;
		rc.GetRect (i, rectBand);

		rc.InvalidateRect (rectBand);
		rc.UpdateWindow ();

		info.fMask = RBBIM_CHILD;
		rc.GetBandInfo (i, &info);

		if (info.hwndChild != NULL)
		{
			::InvalidateRect (info.hwndChild, NULL, TRUE);
			::UpdateWindow (info.hwndChild);
		}
	}
}

//********************************************************************************************
void CMainFrame::LoadBackImage ()
{
	//------------------------------------
	// Load control bars background image:
	//------------------------------------

	if (m_bmpBack.GetSafeHandle () != NULL)
	{
		m_bmpBack.DeleteObject ();
	}

  HBITMAP hbmp;
  if (theApp.preferences.toolbarBackground.IsEmpty()){
    hbmp = (HBITMAP) ::LoadImage (AfxGetResourceHandle (),
			MAKEINTRESOURCE (IDB_BACK),
			IMAGE_BITMAP,
			0, 0,
			LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
	}else{
    hbmp = (HBITMAP) ::LoadImage (AfxGetResourceHandle (),
			theApp.preferences.toolbarBackground,
			IMAGE_BITMAP,
			0, 0,
			LR_LOADMAP3DCOLORS | LR_LOADFROMFILE);
	}

	m_bmpBack.Attach (hbmp);
}
//********************************************************************************************
BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	if (!CFrameWnd::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	SetBackImage ();
	return TRUE;
}

void CMainFrame::onPageTitleChange(char *title) {
	char tmp[512+64];
	strncpy(tmp,title,512);
	tmp[512]=0;
	strcat(tmp," - K-Meleon");
	SetWindowText(tmp);
}

void CMainFrame::onOverLink(char *link) {
	CMozilla *m=(CMozilla *)GetActiveView();
	// BHarris - oldStatus stuff
  if(link && link[0]){

		m_wndStatusBar.GetWindowText(oldStatus);

		m->SetLastLink(link);
		SetMessageText(link);
	}else{
		SetMessageText(oldStatus);
	}
	// BH end
}

void CMainFrame::onJSStatus(char *status) {
	SetMessageText(status);
}

void CMainFrame::onJSDefaultStatus(char *status) {
	SetMessageText(status);
}

void CMainFrame::onSizeBrowserTo(int cx,int cy) {
	// FUCKO:There is probably a better way of doing it
	CMozilla *m=(CMozilla *)GetActiveView();
	RECT rC,rM;
	m->GetClientRect(&rC);
	GetWindowRect(&rM);
	rM.right-=rM.left;
	rM.bottom-=rM.top;
	SetWindowPos(NULL,0,0,(rM.right-rC.right)+cx,(rM.bottom-rC.bottom)+cy,SWP_NOMOVE|SWP_NOZORDER);
}

void CMainFrame::onProgressChange(int percentage) {
	char tmp[512];
	wsprintf(tmp,"Done: %i %%",percentage);
	SetMessageText(tmp);
}

void CMainFrame::onPopup(int flags) {
	CMozilla *m=(CMozilla *)GetActiveView();
  m->OnPopup(flags);
}

void *CMainFrame::createNewBrowser() {
  // FIXME: DEFINITELY have to find a better (and less buggy) way for doing this
  /*
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CKMeleonDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMozilla)
    );
	CKMeleonDoc *d=(CKMeleonDoc *)pDocTemplate->OpenDocumentFile(NULL,TRUE);
  */
  theApp.OnFileNew();
  CMainFrame *pFrame=(CMainFrame *)sMainFrameList.ElementAt(sMainFrameList.Count()-1);
  CMozilla *m=(CMozilla *)pFrame->GetActiveView();
  return(m->createBrowser());
}


void CMainFrame::OnHelpWebKmeleonHome() {
  CString c="http://kmeleon.org/";
  ((CMozilla *)GetActiveView())->Navigate(&c);
}

void CMainFrame::OnHelpWebKmeleonForum() {
  CString c="http://kmeleon.org/forum/";
  ((CMozilla *)GetActiveView())->Navigate(&c);
}

void CMainFrame::OnGoStartPage() {
	((CMozilla *)GetActiveView())->Navigate(&theApp.preferences.homePage);
}

void CMainFrame::OnGoSearchTheWeb() {
  CString c="http://google.com/";
  ((CMozilla *)GetActiveView())->Navigate(&c);
}

void CMainFrame::OnFileNewwindow() {
  nsIWebBrowser *mNewBrowser=(nsIWebBrowser *)createNewBrowser();

  CString str;
  m_wndAddress.GetEditCtrl()->GetWindowText(str);

	nsCOMPtr<nsIWebNavigation> mNewWebNav(do_QueryInterface(mNewBrowser));
  mNewWebNav->LoadURI(NS_ConvertASCIItoUCS2(str.GetBuffer(1024)).GetUnicode(),nsIWebNavigation::LOAD_FLAGS_NONE);
}
