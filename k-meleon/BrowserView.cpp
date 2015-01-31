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
 *   Rod Spears <rods@netscape.com>
 *
 * ***** END LICENSE BLOCK ***** */

// File Overview....
//
// When the CBrowserFrm creates this View:
//   - CreateBrowser() is called in OnCreate() to create the
//     mozilla embeddable browser
//
// OnSize() method handles the window resizes and calls the approriate
// interface method to resize the embedded browser properly
//
// Command handlers to handle browser navigation - OnNavBack(), 
// OnNavForward() etc
//
// DestroyBrowser() called for cleaning up during object destruction
//
// Some important coding notes....
//
// 1. Make sure we do not have the CS_HREDRAW|CS_VREDRAW in the call
//    to AfxRegisterWndClass() inside of PreCreateWindow() below
//    If these flags are present then you'll see screen flicker when 
//    you resize the frame window
//
// Next suggested file to look at : BrowserImpl.cpp
//

#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserView.h"
#include "BrowserFrm.h"
#include "BrowserWindow.h"
#include "MozUtils.h"
#include "Permissions.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const TCHAR* KMELEON_HOMEPAGE_URL = _T("http://kmeleon.sourceforge.net/");
static const TCHAR* KMELEON_FORUM_URL = _T("http://kmeleon.sourceforge.net/forum/");
static const TCHAR* KMELEON_FAQ_URL = _T("http://kmeleon.sourceforge.net/docs/faq.php");
static const TCHAR* KMELEON_MANUAL_URL = _T("http://kmeleon.sourceforge.net/manual/");
static const TCHAR* ABOUT_PLUGINS_URL = _T("about:plugins");
static const TCHAR* ABOUT_KMELEON = _T("about:");

IMPLEMENT_DYNAMIC(CBrowserView, CWnd)

BEGIN_MESSAGE_MAP(CBrowserView, CWnd)
    //{{AFX_MSG_MAP(CBrowserView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_MOUSEACTIVATE()
	ON_WM_ACTIVATE()

    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
    ON_COMMAND(ID_VIEW_SOURCE, OnViewSource)
    ON_COMMAND(ID_VIEW_INFO, OnViewInfo)
    ON_COMMAND(ID_NAV_BACK, OnNavBack)
    ON_COMMAND(ID_NAV_FORWARD, OnNavForward)
    ON_COMMAND(ID_NAV_SEARCH, OnNavSearch)
    ON_COMMAND(ID_NAV_HOME, OnNavHome)
    ON_COMMAND(ID_NAV_RELOAD, OnNavReload)
    ON_COMMAND(ID_NAV_FORCE_RELOAD, OnNavForceReload)
    ON_COMMAND(ID_FILE_SAVE_FRAME_AS, OnFileSaveFrameAs)
    ON_COMMAND(ID_NAV_STOP, OnNavStop)
    ON_COMMAND(ID_NAV_GO, OnNewUrlEnteredInUrlBar)
    ON_COMMAND(ID_EDIT_CUT, OnCut)
    ON_COMMAND(ID_EDIT_COPY, OnCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnPaste)
	ON_COMMAND(ID_EDIT_UNDO, OnUndo)
	ON_COMMAND(ID_EDIT_REDO, OnRedo)
	ON_COMMAND(ID_EDIT_CLEAR, OnDelete)
	ON_COMMAND(ID_PAGE_ENABLE_JS, OnEnableJS)
	ON_COMMAND(ID_PAGE_DISABLE_JS, OnDisableJS)
	ON_COMMAND(ID_PAGE_TOGGLE_JS, OnToggleJS)

	ON_UPDATE_COMMAND_UI(ID_PAGE_TOGGLE_JS, OnUpdateToggleJS)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateDelete)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
    ON_COMMAND(ID_EDIT_SELECT_NONE, OnSelectNone)
    ON_COMMAND(ID_OPEN_LINK_IN_NEW_WINDOW, OnOpenLinkInNewWindow)
    ON_COMMAND(ID_OPEN_LINK_IN_BACKGROUND, OnOpenLinkInBackground)
    ON_COMMAND(ID_VIEW_IMAGE, OnViewImageInNewWindow)
    ON_COMMAND(ID_COPY_LINK_LOCATION, OnCopyLinkLocation)
    ON_COMMAND(ID_COPY_IMAGE_LOCATION, OnCopyImageLocation)
    ON_COMMAND(ID_COPY_IMAGE_CONTENT, OnCopyImageContent)
    ON_COMMAND(ID_OPEN_LINK, OnOpenLink)
    ON_COMMAND(ID_SAVE_LINK_AS, OnSaveLinkAs)
    ON_COMMAND(ID_SAVE_IMAGE_AS, OnSaveImageAs)
    //ON_COMMAND(ID_EDIT_FIND, OnShowFindDlg)
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint) 
    ON_COMMAND(ID_FILE_PRINTPREVIEW, OnFilePrintPreview)
    ON_COMMAND(ID_FILE_PRINTSETUP, OnFilePrintSetup)
    ON_COMMAND(ID_VIEW_FRAME_SOURCE, OnViewFrameSource)
    ON_COMMAND(ID_VIEW_PAGE_INFO, OnViewPageInfo)
    ON_COMMAND(ID_VIEW_FRAME_INFO, OnViewFrameInfo)
    ON_COMMAND(ID_OPEN_FRAME, OnOpenFrame)
    ON_COMMAND(ID_OPEN_FRAME_IN_NEW_WINDOW, OnOpenFrameInNewWindow)
    ON_COMMAND(ID_OPEN_FRAME_IN_BACKGROUND, OnOpenFrameInBackground)
    //ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg) 
    ON_COMMAND(ID_LINK_KMELEON_HOME, OnKmeleonHome)
    ON_COMMAND(ID_LINK_KMELEON_FORUM, OnKmeleonForum)
    ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavBack)
    ON_UPDATE_COMMAND_UI(ID_NAV_FORWARD, OnUpdateNavForward)
    ON_UPDATE_COMMAND_UI(ID_NAV_STOP, OnUpdateNavStop)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSave)
    // ON_UPDATE_COMMAND_UI(ID_FILE_PRINTSETUP, OnUpdatePrintSetup) 
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINTPREVIEW, OnUpdateFilePrintPreview)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IMAGE, OnUpdateViewImage)
	
	ON_COMMAND_RANGE(SHISTORYB_START_ID, SHISTORYB_END_ID, OnSHistoryBack)
	ON_COMMAND_RANGE(SHISTORYF_START_ID, SHISTORYF_END_ID, OnSHistoryForward)
	ON_UPDATE_COMMAND_UI_RANGE(SHISTORYB_START_ID, SHISTORYF_END_ID, OnUpdateSHistory)

    ON_COMMAND(ID_FONT_INCREASE, OnIncreaseFont)
    ON_COMMAND(ID_FONT_DECREASE, OnDecreaseFont)

    ON_COMMAND(ID_FULLZOOM_INCREASE, OnIncreaseFullZoom)
    ON_COMMAND(ID_FULLZOOM_DECREASE, OnDecreaseFullZoom)

    ON_COMMAND(ID_LINK_KMELEON_FAQ, OnKmeleonFAQ)
    ON_COMMAND(ID_LINK_KMELEON_MANUAL, OnKmeleonManual)
    ON_COMMAND(ID_LINK_ABOUT_PLUGINS, OnAboutPlugins)
    //ON_COMMAND(ID_MOUSE_ACTION, OnMouseAction)
	ON_COMMAND(ID_SECURITY_STATE_ICON, OnSecurityStateIcon)
	ON_COMMAND(ID_POPUP_BLOCKED_ICON, OnPopupBlockedIcon)
	
   
    //ON_MESSAGE(UWM_REFRESHTOOLBARITEM, RefreshToolBarItem)
	ON_NOTIFY(CBEN_DRAGBEGIN, ID_URL_BAR, OnDragURL)
    ON_NOTIFY(CBEN_BEGINEDIT, ID_URL_BAR, OnBeginEditURL)
	ON_COMMAND(IDOK, OnNewUrlEnteredInUrlBar)
    ON_CBN_CLOSEUP(ID_URL_BAR, OnUrlSelectedInUrlBar)

    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CBrowserView::CBrowserView()
{
	m_pWindow = NULL;
    m_pBrowserFrameGlue = NULL;
	m_pBrowserGlue = NULL;

//    m_pFindDlg = NULL;
//    m_pPrintProgressDlg = NULL; 
    m_bCurrentlyPrinting = FALSE;

    m_InPrintPreview = FALSE;

	m_tempFileCount = 0;

	m_contextNode = NULL;

//	m_refreshBackButton = FALSE;
//	m_refreshForwardButton = FALSE;

    m_panning = FALSE;
    maccel_pan = FALSE;
}

CBrowserView::~CBrowserView()
{
	if (m_pBrowserGlue) delete m_pBrowserGlue;
	ASSERT(m_pWindow == NULL);
}

//* This is a good place to create the embeddable browser
// instance
//

int CBrowserView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	m_pWindow = new CBrowserWrapper();
	if (!m_pWindow->CreateBrowser(this, mpBrowserFrame->m_chromeMask)) //XXX
		return -1;
    
	//m_pWindow->SetBrowserFrameGlue(m_pBrowserFrameGlue);
	m_pWindow->SetBrowserGlue(m_pBrowserGlue);

	return 0;
}

void CBrowserView::OnDestroy()
{
	DeleteTempFiles();   
	m_pWindow->SetBrowserGlue(NULL);
    m_pWindow->DestroyBrowser();
	delete m_pWindow;
	m_pWindow = NULL;
	if (m_pBrowserGlue) { 
		delete (CBrowserGlue*)m_pBrowserGlue;
		m_pBrowserGlue = NULL;
	}
}

BOOL CBrowserView::PreCreateWindow(CREATESTRUCT& cs) 
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    //cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.style |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, 
        ::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW+1), NULL);

    return TRUE;
}

// Adjust the size of the embedded browser
// in response to any container size changes
//
void CBrowserView::OnSize( UINT nType, int cx, int cy)
{
    m_pWindow->SetPositionAndSize(0, 0, cx, cy);    
}

// Called by this object's creator i.e. the CBrowserFrame object
// to pass its pointer to us
//
void CBrowserView::SetBrowserFrame(CBrowserFrame* pBrowserFrame)
{
    mpBrowserFrame = pBrowserFrame;
}

void CBrowserView::SetBrowserGlue(PBROWSERGLUE pBrowserGlue)
{
	if (m_pBrowserGlue) delete m_pBrowserGlue;
	m_pBrowserGlue = pBrowserGlue;
	if (m_pWindow) m_pWindow->SetBrowserGlue(m_pBrowserGlue);
}

void CBrowserView::SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue)
{
    m_pBrowserFrameGlue = pBrowserFrameGlue;
	if (m_pWindow) m_pWindow->SetBrowserFrameGlue(m_pBrowserFrameGlue);
}

void CBrowserView::OnBeginEditURL( NMHDR * pNotifyStruct, LRESULT * result )
{
	// Send when the url bar get the focus. Deactivate gecko so that it
	// can't steal the focus.
	m_pWindow->SetActive(FALSE);
   *result = 0;
}

void CBrowserView::OnEndEditURL( NMHDR * pNotifyStruct, LRESULT * result )
{
	//mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
   *result = 0;
}

// A new URL was entered in the URL bar
// Get the URL's text from the Urlbar's (ComboBox's) EditControl 
// and navigate to that URL
//
void CBrowserView::OnNewUrlEnteredInUrlBar()
{
	m_pWindow->SetActive(TRUE);

#ifdef INTERNAL_SITEICONS
   GetBrowserGlue()->mIconURI = NULL;
#endif

	// Ugly hack: needed because I can get an OnCbnEditchange 
	// and no OnCbenEndedit
   TRACE0("EditChanged FALSE in CBrowserView::OnNewUrlEnteredInUrlBar\n");
   mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);

   //mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
   
   // Get the currently entered URL
   CString strUrl = mpBrowserFrame->m_wndUrlBar.GetEnteredURL();
   

   // Add what was just entered into the MRI list
   if (theApp.preferences.MRUbehavior == 2)
	   theApp.m_MRUList->AddURL(strUrl);

   //if(IsViewSourceUrl(strUrl))
//      OpenViewSourceWindow(strUrl.GetBuffer(0));
   //else {
       CString urls = NicknameLookup(strUrl);
       OpenMultiURL(urls.GetBuffer(0), TRUE);
   //}
}

// A URL has  been selected from the UrlBar's dropdown list
void CBrowserView::OnUrlSelectedInUrlBar()
{
   CString strUrl;  

   //mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
   if (!mpBrowserFrame->m_wndUrlBar.GetSelectedURL(strUrl))
      return;

   //m_pWindow->SetActive(TRUE);
   
//   if(IsViewSourceUrl(strUrl))
//      OpenViewSourceWindow(strUrl.GetBuffer(0));
//   else {
       CString urls = NicknameLookup(strUrl);
       OpenMultiURL(urls.GetBuffer(0), TRUE);
//   }
}

void CBrowserView::OnViewSource()
{
	OpenViewSourceWindow();
}

void CBrowserView::OnViewFrameSource()
{
	USES_CONVERSION;
	OpenViewSourceWindow(TRUE);
}

void CBrowserView::OnViewInfo() 
{
	ShowSecurityInfo();
}

void CBrowserView::OnNavBack() 
{
	m_pWindow->GoBack();
}

void CBrowserView::OnUpdateNavBack(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanGoBack());
/*
    PRBool canGoBack = PR_FALSE;

    // Buttons get "stuck" down after selecting
    // a menu item, this fixes them
    if (m_refreshBackButton) {
        pCmdUI->Enable(FALSE);
        pCmdUI->Enable(TRUE);
        m_refreshBackButton = FALSE;
    }

    if (mWebNav)
        mWebNav->GetCanGoBack(&canGoBack);

    pCmdUI->Enable(canGoBack);*/
}

void CBrowserView::OnNavForward() 
{
	m_pWindow->GoForward();
}

void CBrowserView::OnUpdateNavForward(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanGoForward());
}

void CBrowserView::OnNavHome()
{
	CString homePage = theApp.preferences.GetString("browser.startup.homepage", _T(""));
    if (!homePage.IsEmpty())
        OpenURL(homePage);
    else
        OpenURL(_T("about:blank"));
}

void CBrowserView::_OnNavReload(BOOL force) 
{
	// If there is no URI to reload, load the address in the url bar
	CString url = m_pWindow->GetURI();
	if (url.IsEmpty() || url.Compare(_T("about:blank")) == 0)
	{
		CString url = mpBrowserFrame->m_wndUrlBar.GetEnteredURL();
		if (!url.IsEmpty())
			OpenURL(url);
	}
	else
		m_pWindow->Reload(force);
}

void CBrowserView::OnNavReload() 
{

	_OnNavReload(FALSE);
}

void CBrowserView::OnNavForceReload()
{
#ifdef INTERNAL_SITEICONS
	if (GetBrowserGlue()->mIconURI) {
		theApp.favicons.RefreshIcon(GetBrowserGlue()->mIconURI);
		GetBrowserGlue()->mIconURI = NULL;
	}
#endif
	_OnNavReload(TRUE);
}

void CBrowserView::OnNavStop() 
{   
	m_pWindow->Stop();
}

void CBrowserView::OnUpdateNavStop(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pWindow->IsBusy());//m_bDocumentLoading);
}

void CBrowserView::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanSave());
}

void CBrowserView::OnCut()
{
	if (::IsChild(m_hWnd, ::GetFocus()))   
		m_pWindow->Cut();
}

void CBrowserView::OnUpdateCut(CCmdUI* pCmdUI)
{
	if (::IsChild(m_hWnd, ::GetFocus()))   
		pCmdUI->Enable(m_pWindow->CanCut());
}

void CBrowserView::OnCopy()
{
	if (::IsChild(m_hWnd, ::GetFocus()))   
		m_pWindow->Copy();
}

void CBrowserView::OnUpdateCopy(CCmdUI* pCmdUI)
{
	if (::IsChild(m_hWnd, ::GetFocus()))   
		pCmdUI->Enable(m_pWindow->CanCopy());
}

void CBrowserView::OnPaste()
{
	CWnd* focus = GetFocus();
	if (focus && focus->IsKindOf(RUNTIME_CLASS(CEdit))) {
		((CEdit*)focus)->Paste();
		return;
	}
	
	if (::IsChild(m_hWnd, ::GetFocus()))   
		m_pWindow->Paste();
}

void CBrowserView::OnUpdatePaste(CCmdUI* pCmdUI)
{
	if (::IsChild(m_hWnd, ::GetFocus()))   
		pCmdUI->Enable(m_pWindow->CanPaste());
}

void CBrowserView::OnUndo()
{
	if (::IsChild(m_hWnd, ::GetFocus()))
		m_pWindow->Undo();
}

void CBrowserView::OnUpdateUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanUndo());
}

void CBrowserView::OnDelete()
{
	if (::IsChild(m_hWnd, ::GetFocus()))
		m_pWindow->Delete();
}

void CBrowserView::OnUpdateDelete(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanDelete());
}

void CBrowserView::OnRedo()
{
	if (::IsChild(m_hWnd, ::GetFocus()))
		m_pWindow->Redo();
}

void CBrowserView::OnUpdateRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pWindow->CanRedo());
}

void CBrowserView::OnSelectAll()
{
	m_pWindow->SelectAll();
}

void CBrowserView::OnSelectNone()
{
	m_pWindow->SelectNone();
}

void CBrowserView::OnFileOpen()
{
	TCHAR lpszFilter[] =
        _T("HTML Files Only (*.htm;*.html)|*.htm;*.html|")
        _T("All Files (*.*)|*.*||");

	CFileDialog fileDlg (TRUE, NULL, NULL, 0, lpszFilter, this);
	if (fileDlg.DoModal() == IDOK)
	{
		CString pathName = fileDlg.GetPathName();
		FILE *test = _tfopen(pathName, _T("r"));
		if (!test) {
          // if the file doesn't exist, they probably typed a url...
         // so chop off the path (for some reason GetFileName doesn't work for us...
			pathName = pathName.Mid(pathName.ReverseFind('\\')+1);
            pathName = NicknameLookup(pathName);
		} else {
			 fclose(test);
		}
		 OpenMultiURL(pathName);
	}
}

void CBrowserView::OnFileSaveAs()
{
	// Prevent the incomplete save of an html document
	if (!m_pWindow->CanSave()
		&& !theApp.preferences.GetBool("kmeleon.download.allowIncompleteSave", FALSE))
	{
		AfxMessageBox(IDS_NOT_FINISHED_LOADING, MB_OK|MB_ICONERROR);
	}
	
	if (!m_pWindow->SaveDocument(FALSE))
		AfxMessageBox(IDS_SAVE_FAILED, MB_OK|MB_ICONERROR);
}

void SetClipboardTextData(HWND hwnd, CString& str)
{
    if (!OpenClipboard(hwnd))
        return;

	size_t l = (str.GetLength() + 1) * sizeof(TCHAR);
    HGLOBAL hClipData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, l);

	if (hClipData) {
		TCHAR *pszClipData = (TCHAR*)::GlobalLock(hClipData);
		if (pszClipData) {
			USES_CONVERSION;
			memcpy(pszClipData, str.GetBuffer(0), l);
			::GlobalUnlock(hClipData);
			::EmptyClipboard();
			::SetClipboardData(CF_UNICODETEXT, hClipData);
		}
	}
    CloseClipboard();
}

void CBrowserView::OnCopyLinkLocation()
{
	CString href, title;
	if (!::GetLinkTitleAndHref(m_contextNode, href, title))
		return;

    if (!OpenClipboard())
        return;

	SetClipboardTextData(m_hWnd, href);
}

void CBrowserView::OnOpenLink()
{
	CString url, title;
	if (!GetLinkTitleAndHref(m_contextNode, url, title))
		return;
    
	OpenURL(url, GetCurrentURI());
	/*
	if (m_ctxData.linkUrl.IsEmpty())
		return;

    OpenURL(m_ctxData.linkUrl, TRUE);*/
}

void CBrowserView::OnOpenLinkInNewWindow()
{
	CString url, title;
	if (!GetLinkTitleAndHref(m_contextNode, url, title))
		return;
	
	OpenURLInNewWindow(url, m_pWindow->GetDocURL(m_contextNode), FALSE);
}

void CBrowserView::OnOpenLinkInBackground()
{
	CString url, title;
	if (!::GetLinkTitleAndHref(m_contextNode, url, title))
		return;
    
	OpenURLInNewWindow(url, m_pWindow->GetDocURL(m_contextNode), TRUE);
}

void CBrowserView::OnViewImageInNewWindow()
{
	CString imgSrc;
	if (!::GetImageSrc(m_contextNode, imgSrc))
		if (!::GetBackgroundImageSrc(m_contextNode, imgSrc))
			return;
    
    OpenURLInNewWindow(imgSrc, m_pWindow->GetDocURL(m_contextNode), FALSE);
}

void CBrowserView::OnUpdateViewImage(CCmdUI *pCmdUI)
{
	CString imgSrc;
	if (!::GetImageSrc(m_contextNode, imgSrc))
		::GetBackgroundImageSrc(m_contextNode, imgSrc);

    pCmdUI->Enable(!imgSrc.IsEmpty());
}

BOOL CBrowserView::SaveLink(LPCTSTR url)
{
	ASSERT(url);
	if (!url || !*url)
		return FALSE;

	if (!m_pWindow->SaveURL(url)) {
		AfxMessageBox(IDS_SAVE_FAILED, MB_OK|MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}

void CBrowserView::OnSaveLinkAs()
{
	CString url, title;
	if (!GetLinkTitleAndHref(m_contextNode, url, title))
		return;
	SaveLink(url);
}

void CBrowserView::OnSaveImageAs()
{
	CString imgUrl;
	if (!GetImageSrc(m_contextNode, imgUrl))
		if (!GetBackgroundImageSrc(m_contextNode, imgUrl))
			return;

	SaveLink(imgUrl);

	/*
	nsresult rv;
    nsCOMPtr<nsIURI> imageURI;
    rv = NewURI(getter_AddRefs(imageURI), mCtxMenuImgSrc);
    NS_ENSURE_SUCCESS(rv, );

	nsCOMPtr<nsIURI> referrer;
	mWebNav->GetCurrentURI(getter_AddRefs(referrer));

	CSaveAsHandler* handler = new CSaveAsHandler(nullptr, nullptr, imageURI, nullptr, nullptr, referrer, mpBrowserFrame);
	rv = handler->Save(mCtxImgType.get(), mCtxImgDisposition.get());
    if (NS_FAILED(rv) && rv != NS_ERROR_ABORT)
		AfxMessageBox(IDS_SAVE_FAILED, MB_OK|MB_ICONERROR);*/
}

void CBrowserView::OnOpenFrame()
{
	CString url = m_pWindow->GetFrameURL(m_contextNode);
	if (url.IsEmpty()) return;
	OpenURL(url, GetCurrentURI());
}

void CBrowserView::OnOpenFrameInNewWindow()
{
	CString url = m_pWindow->GetFrameURL(m_contextNode);
	if (url.IsEmpty()) return;
    OpenURLInNewWindow(url, GetCurrentURI(), FALSE);
}

void CBrowserView::OnOpenFrameInBackground()
{
	CString url = m_pWindow->GetFrameURL(m_contextNode);
	if (url.IsEmpty()) return;
    OpenURLInNewWindow(url, GetCurrentURI(), TRUE);
}

/* ** */

#if 0
void CBrowserView::OnDropFiles( HDROP drop )
{
   UINT size = DragQueryFile(drop, 0, NULL, 0) + 1;
   char *filename = new char[size];
   DragQueryFile(drop, 0, filename, size);

   if (stricmp(filename + (size-5), _T(".url")) == 0){
      char tempUrl[1024];
      ::GetPrivateProfileString(_T("InternetShortcut"), _T("Url"), filename, tempUrl, 1023, filename);
      OpenURL(tempUrl);
   }else{
      OpenURL(filename);
   }

   delete filename;
   DragFinish(drop);
}
#endif

void CBrowserView::AddURLAndPerformDrag(COleDataSource& datasource)
{
   USES_CONVERSION;

   CString currentURI = GetCurrentURI();
   DWORD currentURISize = currentURI.GetLength()+1;
   const DWORD extraFileSize = 26; // size of [InternetShortcut]\r\nURL=...\r\n

   HGLOBAL hURL = GlobalAlloc(GHND, currentURISize);
   char *url = (char *)GlobalLock(hURL);
   strcpy(url, T2CA(currentURI));
   GlobalUnlock(hURL);

   HGLOBAL hFileDescriptor = GlobalAlloc(GHND, sizeof(FILEGROUPDESCRIPTOR));
   FILEGROUPDESCRIPTOR *fgd = (FILEGROUPDESCRIPTOR *)GlobalLock(hFileDescriptor);
   fgd->cItems = 1;
   fgd->fgd[0].dwFlags = FD_FILESIZE | FD_LINKUI;
   fgd->fgd[0].nFileSizeLow = currentURISize+extraFileSize;

   CString title = GetPageTitle();
   _tcsncpy(fgd->fgd[0].cFileName, title, 250);
   _tcscat(fgd->fgd[0].cFileName, _T(".url"));
   MakeFilename(fgd->fgd[0].cFileName);
   GlobalUnlock(hFileDescriptor);

   HGLOBAL hFileContents = GlobalAlloc(GHND, currentURISize+extraFileSize);
   char *contents = (char *)GlobalLock(hFileContents);
   strcpy(contents, "[InternetShortcut]\r\nURL=");
   strcat(contents, url);
   strcat(contents, "\r\n");
   GlobalUnlock(hFileContents);

   UINT cfFileDescriptor = ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
   UINT cfFileContents = ::RegisterClipboardFormat(CFSTR_FILECONTENTS);
   UINT cfShellURL = ::RegisterClipboardFormat(CFSTR_SHELLURL);

   FORMATETC fmetc = { cfFileContents, NULL, DVASPECT_CONTENT, 0, TYMED_FILE };
   
   // Note: order is important here!
   datasource.CacheGlobalData(cfFileContents, hFileContents, &fmetc);
   datasource.CacheGlobalData(cfFileDescriptor, hFileDescriptor);
   datasource.CacheGlobalData(cfShellURL, hURL);
   datasource.CacheGlobalData(CF_TEXT, hURL);
   datasource.DoDragDrop();

   GlobalFree(hURL);
   GlobalFree(hFileDescriptor);
   GlobalFree(hFileContents);
}

void CBrowserView::OnDragURL( NMHDR * pNotifyStruct, LRESULT * result )
{
   USES_CONVERSION;
   *result = 0;

   COleDataSource datasource;
   AddURLAndPerformDrag(datasource);
}

// this should probably go in BrowserViewUtils.cpp, but I don't want to add a function prototype :)
BOOL CALLBACK SearchProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static CString *search = NULL;

   if (uMsg == WM_INITDIALOG) {
       if (search) {
           EndDialog(hwndDlg, false);
           return TRUE;
       }
      search = (CString *)lParam;
      ::SetFocus(::GetDlgItem(hwndDlg, IDC_SEARCH_QUERY));
      return false;
   }

   else if (uMsg == WM_COMMAND) {
      if (LOWORD(wParam) == IDOK){
         TCHAR buffer[256];
         ::GetDlgItemText(hwndDlg, IDC_SEARCH_QUERY, buffer, 255);
         *search = buffer;
         search = NULL;
         EndDialog(hwndDlg, true);
      }
      else if (LOWORD(wParam) == IDCANCEL) {
         search = NULL;
         EndDialog(hwndDlg, false);
      }
      return true;
   }

   return false;
}

void CBrowserView::OnNavSearch()
{
   CString search;
   if (DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_SEARCH_DIALOG), m_hWnd, SearchProc, (LPARAM)&search)) {
      search.Replace(_T("+"), _T("%2b"));
	  search = GetSearchURL(search);
	  if (search.IsEmpty())
         search = theApp.preferences.GetString("kmeleon.general.searchEngine", _T("http://www.google.com/search?q=")) + search;
      OpenURL(search);
   }
}

void CBrowserView::OnFileClose()
{
   mpBrowserFrame->PostMessage(WM_CLOSE);
}

void CBrowserView::OnFileSaveFrameAs()
{
	if (!m_pWindow->SaveDocument(TRUE))
		AfxMessageBox(IDS_SAVE_FAILED, MB_OK|MB_ICONERROR);
	//SaveLink(m_ctxData.frameUrl);
}	

void CBrowserView::OnViewPageInfo()
{
	CString uri = m_pWindow->GetURI();

	// Build the page info url
	if (uri.Find(_T("https://")) == -1)
		uri.Insert(0, _T("about:cache-entry?client=HTTP&sb=1&key="));
	else
		uri.Insert(0, _T("about:cache-entry?client=HTTP-memory-only&sb=1&key="));

	OpenURLInNewWindow(uri);
}
 
void CBrowserView::OnViewFrameInfo()
{
	CString viewFrameInfoUrl = m_pWindow->GetFrameURL(m_contextNode);
	if (viewFrameInfoUrl.IsEmpty()) return;


   if (viewFrameInfoUrl.Find(_T("https://")) == -1)
      viewFrameInfoUrl = _T("about:cache-entry?client=HTTP&sb=1&key=") + viewFrameInfoUrl;
   else
      viewFrameInfoUrl = _T("about:cache-entry?client=HTTP-memory-only&sb=1&key=") + viewFrameInfoUrl;

   OpenURLInNewWindow(viewFrameInfoUrl);
}

void CBrowserView::OnCopyImageLocation()
{
	CString imgSrc;
	if (!::GetImageSrc(m_contextNode, imgSrc))
		if (!::GetBackgroundImageSrc(m_contextNode, imgSrc))
			return;

	if (! OpenClipboard())
		return;

	SetClipboardTextData(m_hWnd, imgSrc);
}

void CBrowserView::OnCopyImageContent()
{
   m_pWindow->CopyImage();
}

 
void CBrowserView::OnUpdateFilePrintPreview(CCmdUI* pCmdUI)
{
   pCmdUI->SetCheck(m_InPrintPreview);
}
                                                                           
void CBrowserView::OnKmeleonHome()
{
   OpenURL(KMELEON_HOMEPAGE_URL);
}

void CBrowserView::OnKmeleonForum()
{
   OpenURL(KMELEON_FORUM_URL);
}

void CBrowserView::OnKmeleonFAQ()
{
   OpenURL(KMELEON_FAQ_URL);
}

void CBrowserView::OnKmeleonManual()
{
   OpenURL(KMELEON_MANUAL_URL);
}

void CBrowserView::OnAboutPlugins()
{
   OpenURLInNewWindow(ABOUT_PLUGINS_URL);
}

void CBrowserView::OnMouseAction()
{/*
	if (m_pWindow->GetURI().IsEmpty()) 
		return;

	POINT pt;
	::GetCursorPos(&pt);

	HWND hWnd;
	hWnd = ::GetFocus();

	if (hWnd && ::IsChild(m_hWnd, hWnd)) {

		::SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(pt.x, pt.y));
		if ( (maccel_key!=WM_MBUTTONDOWN && mCtxMenuImgSrc.Length()>0) || mCtxMenuLinkUrl.Length() > 0)
			::PostMessage(mpBrowserFrame->m_hWnd, WM_COMMAND, (WPARAM)maccel_cmd, (LPARAM)0);
		else if (!m_panning && maccel_key==WM_MBUTTONDOWN) {
			maccel_pan=1;
			StartPanning(TRUE);
		}
	}

	maccel_cmd = 0;
*/
}

void CBrowserView::OnIncreaseFullZoom()
{
   m_pWindow->ChangeFullZoom(1);
}

void CBrowserView::OnDecreaseFullZoom()
{
   m_pWindow->ChangeFullZoom(-1);
}

void CBrowserView::OnIncreaseFont()
{
   m_pWindow->ChangeTextSize(1);
}

void CBrowserView::OnDecreaseFont()
{
   m_pWindow->ChangeTextSize(-1);
}

void CBrowserView::OnSecurityStateIcon()
{
	ShowSecurityInfo();
}

void CBrowserView::OnPopupBlockedIcon()
{
	CString msg;
	int x;
	// XXXX	
	msg.Format(IDS_ALLOW_POPUP, ((CBrowserGlue*)m_pBrowserGlue)->mPopupBlockedHost);
	if ( (x = MessageBox(msg, 0, MB_YESNO|MB_ICONQUESTION)) == IDYES)
	{
		USES_CONVERSION;
		CPermissions permissions("popup");
		permissions.set(T2CA(((CBrowserGlue*)m_pBrowserGlue)->mPopupBlockedHost), 1);
	}
}


void CBrowserView::OnFilePrint()
{
	theApp.preferences.SetBool("print.use_native_print_dialog", TRUE);
	theApp.preferences.SetBool("print.show_print_progress", FALSE);
	m_bCurrentlyPrinting = TRUE;
	m_pWindow->Print(); 
	m_bCurrentlyPrinting = FALSE;
}  

void CBrowserView::OnFilePrintPreview()                                         
{
	if (m_pWindow->PrintPreview())
		;//m_InPrintPreview = ! m_InPrintPreview;
}

void CBrowserView::OnFilePrintSetup()
{
	m_pWindow->PrintSetup();
}

void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting);
}

void CBrowserView::Activate(BOOL bActive)
{
	m_pWindow->SetActive(bActive);
}

void CBrowserView::Highlight(const wchar_t* string, BOOL matchCase)
{
	CString str;
	str.LoadString(IDS_HIGHLIGHTING);
	mpBrowserFrame->UpdateStatus(str);

	//ASSERT(string || m_lastHighlightWord.Length());

	if (string && string[0])
	{
		m_pWindow->Highlight(L"Yellow", string, matchCase);
		m_lastHighlightWord = string;
	}
	else
	{
		m_pWindow->Highlight(NULL, m_lastHighlightWord.get(), matchCase);
		m_lastHighlightWord.SetLength(0);
	}

	str.LoadString(AFX_IDS_IDLEMESSAGE);
	mpBrowserFrame->UpdateStatus(str);
}

int CBrowserView::GetSiteIcon()
{
	return theApp.favicons.GetIcon(GetBrowserGlue()->mIconURI);
}

void CBrowserView::OnSHistoryBack(UINT nID)
{
	int index = 0, count;
	m_pWindow->GetSHistoryState(index, count);
	m_pWindow->GotoHistoryIndex(nID - SHISTORYB_START_ID + (index>MAX_SHMENU_NUMBER/2?index-MAX_SHMENU_NUMBER/2:0));
}

void CBrowserView::OnSHistoryForward(UINT nID)
{
	int index = 0, count;
	m_pWindow->GetSHistoryState(index, count);
	m_pWindow->GotoHistoryIndex(nID - SHISTORYF_START_ID + index + 1);
}

void CBrowserView::OnUpdateSHistory(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();	
}

bool CBrowserView::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	CString title, url;
	if (nItemID >= SHISTORYF_START_ID && nItemID <= SHISTORYF_END_ID) {
		int index,count;
		m_pWindow->GetSHistoryState(index, count);
		m_pWindow->GetSHistoryInfoAt(nItemID - SHISTORYF_START_ID + index + 1, title, url);
		mpBrowserFrame->UpdateStatus(url);
		return true;
	} else if (nItemID >= SHISTORYB_START_ID && nItemID <= SHISTORYB_END_ID) {
		m_pWindow->GetSHistoryInfoAt(nItemID - SHISTORYB_START_ID, title, url);
		mpBrowserFrame->UpdateStatus(url);
		return true;
	}

	CString desc;// = theApp.commands.GetDescription(nItemID); // TODO
	if (desc.GetLength()>0) {
		mpBrowserFrame->UpdateStatus(desc);
		return true;
	}

	return false;
}

void CBrowserView::OnToggleJS()
{
	m_pWindow->AllowJS(!m_pWindow->IsJSAllowed());
}

void CBrowserView::OnUpdateToggleJS(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(!m_pWindow->IsJSAllowed());
}

void CBrowserView::OnEnableJS()
{
	m_pWindow->AllowJS(TRUE);
}

void CBrowserView::OnDisableJS()
{
	m_pWindow->AllowJS(FALSE);
}