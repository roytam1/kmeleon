/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chak Nanga <chak@netscape.com>    
 */

// File Overview....
//
// When the CBrowserFrm creates this View:
//   - CreateBrowser() is called in OnCreate() to create the
//	   mozilla embeddable browser
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
//	  to AfxRegisterWndClass() inside of PreCreateWindow() below
//	  If these flags are present then you'll see screen flicker when 
//	  you resize the frame window
//
// Next suggested file to look at : BrowserImpl.cpp
//

#include "stdafx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserView.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "PrintProgressDialog.h"
//#include "nsPrintSettingsImpl.h"
#include "PrintSetupDialog.h"
#include "ToolBarEx.h"
#include "Utils.h"
#include "KmeleonConst.h"
#include "About.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const char* KMELEON_HOMEPAGE_URL = "http://kmeleon.sourceforge.net";
static const char* KMELEON_FORUM_URL = "http://kmeleon.sourceforge.net/forum";

// Register message for FindDialog communication                                                                                
static UINT WM_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CBrowserView, CWnd)
	//{{AFX_MSG_MAP(CBrowserView)
   ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
   ON_WM_TIMER()
   ON_WM_MOUSEACTIVATE()
	ON_CBN_SELENDOK(ID_URL_BAR, OnUrlSelectedInUrlBar)
   ON_CBN_KILLFOCUS(ID_URL_BAR, OnUrlKillFocus)
   ON_CBN_EDITCHANGE(ID_URL_BAR, OnUrlEditChange)
   ON_COMMAND(IDOK, OnNewUrlEnteredInUrlBar)
   ON_COMMAND(ID_SELECT_URL, OnSelectUrl)
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
	ON_COMMAND(ID_NAV_STOP, OnNavStop)
	ON_COMMAND(ID_EDIT_CUT, OnCut)
	ON_COMMAND(ID_EDIT_COPY, OnCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
	ON_COMMAND(ID_EDIT_SELECT_NONE, OnSelectNone)
	ON_COMMAND(ID_COPY_LINK_LOCATION, OnCopyLinkLocation)
	ON_COMMAND(ID_COPY_IMAGE_LOCATION, OnCopyImageLocation)
	ON_COMMAND(ID_OPEN_LINK, OnOpenLink)
	ON_COMMAND(ID_OPEN_LINK_IN_NEW_WINDOW, OnOpenLinkInNewWindow)
	ON_COMMAND(ID_OPEN_LINK_IN_BACKGROUND, OnOpenLinkInBackground)
	ON_COMMAND(ID_VIEW_IMAGE, OnViewImageInNewWindow)
	ON_COMMAND(ID_SAVE_LINK_AS, OnSaveLinkAs)
	ON_COMMAND(ID_SAVE_IMAGE_AS, OnSaveImageAs)
   ON_COMMAND(ID_EDIT_FIND, OnShowFindDlg)
   ON_COMMAND(ID_FILE_PRINT, OnFilePrint) 
   ON_COMMAND(ID_FILE_PRINTPREVIEW, OnFilePrintPreview)
   ON_COMMAND(ID_FILE_PRINTSETUP, OnFilePrintSetup)
   ON_COMMAND(ID_VIEW_FRAME_SOURCE, OnViewFrameSource)
   ON_COMMAND(ID_OPEN_FRAME, OnOpenFrame)
   ON_COMMAND(ID_OPEN_FRAME_IN_NEW_WINDOW, OnOpenFrameInNewWindow)
   ON_COMMAND(ID_OPEN_FRAME_IN_BACKGROUND, OnOpenFrameInBackground)
   ON_COMMAND(ID_EDIT_FINDNEXT, OnFindNext)
   ON_COMMAND(ID_EDIT_FINDPREV, OnFindPrev)
   ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg) 
   ON_COMMAND(ID_LINK_KMELEON_HOME, OnKmeleonHome)
   ON_COMMAND(ID_LINK_KMELEON_FORUM, OnKmeleonForum)
   ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavBack)
	ON_UPDATE_COMMAND_UI(ID_NAV_FORWARD, OnUpdateNavForward)
	ON_UPDATE_COMMAND_UI(ID_NAV_STOP, OnUpdateNavStop)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)
   ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
   ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
	ON_COMMAND(ID_WINDOW_PREV, OnWindowPrev)
	ON_WM_ACTIVATE()
	ON_MESSAGE(UWM_REFRESHTOOLBARITEM, RefreshToolBarItem)

   ON_NOTIFY(CBEN_BEGINEDIT, ID_URL_BAR, OnEditURL)
   ON_NOTIFY(CBEN_DRAGBEGIN, ID_URL_BAR, OnDragURL)
   ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CBrowserView::CBrowserView()
{
	mWebBrowser = nsnull;
	mBaseWindow = nsnull;
	mWebNav = nsnull;

	mpBrowserImpl = nsnull;
	mpBrowserFrame = nsnull;
	mpBrowserFrameGlue = nsnull;

	mbDocumentLoading = PR_FALSE;

   m_pFindDlg = NULL;
   m_pPrintProgressDlg = NULL; 
   m_bCurrentlyPrinting = NULL;
   m_SecurityState = SECURITY_STATE_INSECURE;

   m_tempFileCount = 0;

   m_iGetNodeHack = 0;
   m_pGetNode = NULL;

   m_panning = FALSE;
}

CBrowserView::~CBrowserView()
{
}

// This is a good place to create the embeddable browser
// instance
//
int CBrowserView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CreateBrowser();

   DragAcceptFiles();
	return 0;
}

void CBrowserView::OnDestroy()
{
	DestroyBrowser();
}

// Create an instance of the Mozilla embeddable browser
//
HRESULT CBrowserView::CreateBrowser() 
{	   
	// Create web shell
	nsresult rv;
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	if(NS_FAILED(rv))
		return rv;

	// Save off the nsIWebNavigation interface pointer 
	// in the mWebNav member variable which we'll use 
	// later for web page navigation
	//
	rv = NS_OK;
	mWebNav = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;

	// Create the CBrowserImpl object - this is the object
	// which implements the interfaces which are required
	// by an app embedding mozilla i.e. these are the interfaces
	// thru' which the *embedded* browser communicates with the
	// *embedding* app
	//
	// The CBrowserImpl object will be passed in to the 
	// SetContainerWindow() call below
	//
	// Also note that we're passing the BrowserFrameGlue pointer 
	// and also the mWebBrowser interface pointer via CBrowserImpl::Init()
	// of CBrowserImpl object. 
	// These pointers will be saved by the CBrowserImpl object.
	// The CBrowserImpl object uses the BrowserFrameGlue pointer to 
	// call the methods on that interface to update the status/progress bars
	// etc.
	mpBrowserImpl = new CBrowserImpl();
	if(mpBrowserImpl == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	// Pass along the mpBrowserFrameGlue pointer to the BrowserImpl object
	// This is the interface thru' which the XP BrowserImpl code communicates
	// with the platform specific code to update status bars etc.
	mpBrowserImpl->Init(mpBrowserFrameGlue, mWebBrowser);
	mpBrowserImpl->AddRef();

   mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, mpBrowserImpl));

	rv = NS_OK;
   nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;


   // If the browser window hosting chrome or content?
   dsti->SetItemType( theApp.cmdline.m_bChrome ?
      nsIDocShellTreeItem::typeChromeWrapper :
      nsIDocShellTreeItem::typeContentWrapper);


    // Create the real webbrowser window
  
	// Note that we're passing the m_hWnd in the call below to InitWindow()
	// (CBrowserView inherits the m_hWnd from CWnd)
	// This m_hWnd will be used as the parent window by the embeddable browser
	//
	rv = NS_OK;
	mBaseWindow = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;

	// Get the view's ClientRect which to be passed in to the InitWindow()
	// call below
	RECT rcLocation;
	GetClientRect(&rcLocation);
	if(IsRectEmpty(&rcLocation))
	{
		rcLocation.bottom++;
		rcLocation.top++;
	}

   rv = mBaseWindow->InitWindow(nsNativeWidget(m_hWnd), nsnull,
		0, 0, rcLocation.right - rcLocation.left, rcLocation.bottom - rcLocation.top);
   rv = mBaseWindow->Create();

   // Register the BrowserImpl object to receive progress messages
   // These callbacks will be used to update the status/progress bars

   nsWeakPtr weakling( dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*, mpBrowserImpl))));
   (void)mWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));
   
   // Finally, show the web browser window
	mBaseWindow->SetVisibility(PR_TRUE);

   return S_OK;
}

HRESULT CBrowserView::DestroyBrowser()
{

   DeleteTempFiles();   

         if(mBaseWindow)
	{
		mBaseWindow->Destroy();
      mBaseWindow = nsnull;
	}

	if(mpBrowserImpl)
	{
		delete mpBrowserImpl;
		mpBrowserImpl = nsnull;
	}

	return NS_OK;
}

BOOL CBrowserView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW+1), NULL);

	return TRUE;
}

// Adjust the size of the embedded browser
// in response to any container size changes
//
void CBrowserView::OnSize( UINT nType, int cx, int cy)
{
    mBaseWindow->SetPositionAndSize(0, 0, cx, cy, PR_TRUE);    
}

// Called by this object's creator i.e. the CBrowserFrame object
// to pass its pointer to us
//
void CBrowserView::SetBrowserFrame(CBrowserFrame* pBrowserFrame)
{
	mpBrowserFrame = pBrowserFrame;
}

void CBrowserView::SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue)
{
	mpBrowserFrameGlue = pBrowserFrameGlue;
}

void CBrowserView::Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{

   nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(mWebBrowser));
   if(!focus)
      return;

   switch(nState) {
   case WA_ACTIVE:
   case WA_CLICKACTIVE:
      // don't activate it if the window is minimized
      // this was sending a WM_SIZE message, which lost the bMaximized state
      if(!bMinimized)
         focus->Activate();
      break;
   case WA_INACTIVE:
      focus->Deactivate();
      break;
   default:
      break;
   }
}

// Determintes if the currently loaded document
// contains frames
//
BOOL CBrowserView::ViewContentContainsFrames()
{
    nsresult rv = NS_OK;

    // Get nsIDOMDocument from nsIWebNavigation
    nsCOMPtr<nsIDOMDocument> domDoc;
    rv = mWebNav->GetDocument(getter_AddRefs(domDoc));
    if(NS_FAILED(rv))
       return FALSE;

    // QI nsIDOMDocument for nsIDOMHTMLDocument
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
    if (!htmlDoc)
        return FALSE;
   
    // Get the <body> element of the doc
    nsCOMPtr<nsIDOMHTMLElement> body;
    rv = htmlDoc->GetBody(getter_AddRefs(body));
    if(NS_FAILED(rv))
       return FALSE;

    // Is it of type nsIDOMHTMLFrameSetElement?
    nsCOMPtr<nsIDOMHTMLFrameSetElement> frameset = do_QueryInterface(body);

    return (frameset != nsnull);
}

void CBrowserView::OnViewFrameSource()
{
    USES_CONVERSION;

    // Build the view-source: url
    //
    nsCAutoString viewSrcUrl;
    viewSrcUrl.Append("view-source:");
    viewSrcUrl.Append(W2T(mCtxMenuCurrentFrameURL.get()));

    OpenViewSourceWindow(viewSrcUrl.get());
}

void CBrowserView::OnOpenFrame()
{
	if(mCtxMenuCurrentFrameURL.Length())
		OpenURL(mCtxMenuCurrentFrameURL.get());
}

void CBrowserView::OnOpenFrameInNewWindow()
{
	if(mCtxMenuCurrentFrameURL.Length())
		OpenURLInNewWindow(mCtxMenuCurrentFrameURL.get());
}

void CBrowserView::OnOpenFrameInBackground()
{
	if(mCtxMenuCurrentFrameURL.Length())
		OpenURLInNewWindow(mCtxMenuCurrentFrameURL.get(), true);
}

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

void CBrowserView::OnDragURL( NMHDR * pNotifyStruct, LRESULT * result )
{
   USES_CONVERSION;
   *result = 0;

   DWORD currentURISize = GetCurrentURI(NULL);
   const DWORD extraFileSize = 26; // size of [InternetShortcut]\r\nURL=...\r\n

   HGLOBAL hURL = GlobalAlloc(GHND, currentURISize);
   char *url = (char *)GlobalLock(hURL);
   GetCurrentURI(url);
   GlobalUnlock(hURL);

   HGLOBAL hFileDescriptor = GlobalAlloc(GHND, sizeof(FILEGROUPDESCRIPTOR));
   FILEGROUPDESCRIPTOR *fgd = (FILEGROUPDESCRIPTOR *)GlobalLock(hFileDescriptor);
   fgd->cItems = 1;
   fgd->fgd[0].dwFlags = FD_FILESIZE | FD_LINKUI;
   fgd->fgd[0].nFileSizeLow = currentURISize+extraFileSize;

   CString title;
   GetPageTitle(title);
   strncpy(fgd->fgd[0].cFileName, title, 250);
   strcat(fgd->fgd[0].cFileName, ".url");
   
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

   COleDataSource datasource;
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

void CBrowserView::OnEditURL( NMHDR * pNotifyStruct, LRESULT * result )
{
   *result = 0;
}
// A new URL was entered in the URL bar
// Get the URL's text from the Urlbar's (ComboBox's) EditControl 
// and navigate to that URL
//
void CBrowserView::OnNewUrlEnteredInUrlBar()
{

   mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
   SetFocus();
   
   // Get the currently entered URL
	CString strUrl;
	mpBrowserFrame->m_wndUrlBar.GetEnteredURL(strUrl);

   if(IsViewSourceUrl(strUrl))
      OpenViewSourceWindow(strUrl.GetBuffer(0));
   else 
      // Navigate to that URL
	   OpenURL(strUrl.GetBuffer(0));

	// Add what was just entered into the UrlBar
	mpBrowserFrame->m_wndUrlBar.AddURLToList(strUrl);
}

// A URL has  been selected from the UrlBar's dropdown list
//
void CBrowserView::OnUrlSelectedInUrlBar()
{
   CString strUrl;	
   mpBrowserFrame->m_wndUrlBar.GetSelectedURL(strUrl);   
   
   if(IsViewSourceUrl(strUrl))
      OpenViewSourceWindow(strUrl.GetBuffer(0));
   else 
   	OpenURL(strUrl.GetBuffer(0));
}

void CBrowserView::OnSelectUrl()
{
   mpBrowserFrame->m_wndUrlBar.SetFocus();
}

void CBrowserView::OnUrlKillFocus()
{
   if (mpBrowserFrame->m_wndUrlBar.CheckFocus())
      mpBrowserFrame->m_wndUrlBar.ReturnFocus();
   else
      mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
}

void CBrowserView::OnUrlEditChange()
{
   mpBrowserFrame->m_wndUrlBar.EditChanged(TRUE);
}
void CBrowserView::OnViewSource()
{
   if(! mWebNav)
      return;

	// Get the URI object whose source we want to view.
   nsresult rv = NS_OK;
   nsCOMPtr<nsIURI> currentURI;
   rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
   if(NS_FAILED(rv) || !currentURI)
      return;

   // Get the uri string associated with the nsIURI object
   nsCAutoString uriString;
   rv = currentURI->GetSpec(uriString);
   if(NS_FAILED(rv))
      return;

   // Build the view-source: url
   nsCAutoString viewSrcUrl;
   viewSrcUrl.Append("view-source:");
   viewSrcUrl.Append(uriString);

   OpenViewSourceWindow(viewSrcUrl.get());
}

void CBrowserView::OnViewInfo() 
{
	ShowSecurityInfo();
}

void CBrowserView::OnNavBack() 
{
	if(mWebNav)
		mWebNav->GoBack();
}

void CBrowserView::OnNavForward() 
{
	if(mWebNav)
		mWebNav->GoForward();
}

void CBrowserView::OnUpdateNavBack(CCmdUI* pCmdUI)
{
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

	pCmdUI->Enable(canGoBack);
}

void CBrowserView::OnUpdateNavForward(CCmdUI* pCmdUI)
{
	PRBool canGoFwd = PR_FALSE;

	// Buttons get "stuck" down after selecting
	// a menu item, this fixes thim
	if (m_refreshForwardButton) {
		pCmdUI->Enable(FALSE);
		pCmdUI->Enable(TRUE);
		m_refreshForwardButton = FALSE;
	}

    if (mWebNav)
        mWebNav->GetCanGoForward(&canGoFwd);

	pCmdUI->Enable(canGoFwd);
}

void CBrowserView::OnNavHome()
{
   if (!theApp.preferences.homePage.IsEmpty())
      OpenURL(theApp.preferences.homePage);
   else
      OpenURL("about:blank");
}

// this should probably go in BrowserViewUtils.cpp, but I don't want to add a function prototype :)
BOOL CALLBACK SearchProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static CString *search;

   if (uMsg == WM_INITDIALOG) {
      search = (CString *)lParam;
      ::SetFocus(::GetDlgItem(hwndDlg, IDC_SEARCH_QUERY));
      return false;
   }

   else if (uMsg == WM_COMMAND) {
      if (LOWORD(wParam) == IDOK){
         char buffer[256];
         ::GetDlgItemText(hwndDlg, IDC_SEARCH_QUERY, buffer, 255);
         *search = buffer;
         EndDialog(hwndDlg, true);
      }
      else if (LOWORD(wParam) == IDCANCEL)
         EndDialog(hwndDlg, false);
      return true;
   }

   return false;
}

void CBrowserView::OnNavSearch()
{
   CString search;
   if (DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_SEARCH_DIALOG), m_hWnd, SearchProc, (LPARAM)&search))
      OpenURL(theApp.preferences.searchEngine + search);
}

void CBrowserView::OnNavReload() 
{
	if(mWebNav)
		mWebNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
}

void CBrowserView::OnNavStop() 
{	
	if(mWebNav)
		mWebNav->Stop(nsIWebNavigation::STOP_ALL);
}

void CBrowserView::OnUpdateNavStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(mbDocumentLoading);
}

void CBrowserView::OnCut()
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      mpBrowserFrame->m_wndUrlBar.Cut();
      return;
   }

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->CutSelection();
}

void CBrowserView::OnUpdateCut(CCmdUI* pCmdUI)
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      DWORD selPosition = mpBrowserFrame->m_wndUrlBar.GetEditSel();
      pCmdUI->Enable(LOWORD(selPosition) != HIWORD(selPosition));
      return;
   }
   PRBool canCutSelection = PR_FALSE;

   nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
	if (clipCmds)
      clipCmds->CanCutSelection(&canCutSelection);

	pCmdUI->Enable(canCutSelection);
}

void CBrowserView::OnCopy()
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      mpBrowserFrame->m_wndUrlBar.Copy();
      return;
   }

   nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

   if(clipCmds)
      clipCmds->CopySelection();
}

void CBrowserView::OnUpdateCopy(CCmdUI* pCmdUI)
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      DWORD selPosition = mpBrowserFrame->m_wndUrlBar.GetEditSel();
      pCmdUI->Enable(LOWORD(selPosition) != HIWORD(selPosition));
      return;
   }
   PRBool canCopySelection = PR_FALSE;

   nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
   if (clipCmds)
      clipCmds->CanCopySelection(&canCopySelection);

   pCmdUI->Enable(canCopySelection);
}

void CBrowserView::OnPaste()
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      mpBrowserFrame->m_wndUrlBar.Paste();
      return;
   }
   
   nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

   if(clipCmds)
      clipCmds->Paste();
}

void CBrowserView::OnUpdatePaste(CCmdUI* pCmdUI)
{
   if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
      pCmdUI->Enable();
      return;
   }
   PRBool canPaste = PR_FALSE;

   nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
   if (clipCmds)
      clipCmds->CanPaste(&canPaste);

   pCmdUI->Enable(canPaste);
}

void CBrowserView::OnSelectAll()
{
	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->SelectAll();
}

void CBrowserView::OnSelectNone()
{
	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->SelectNone();
}

void CBrowserView::OnFileOpen()
{
	char *lpszFilter =
        "HTML Files Only (*.htm;*.html)|*.htm;*.html|"
        "All Files (*.*)|*.*||";

	CFileDialog cf(TRUE, NULL, NULL, OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,	lpszFilter, this);
   cf.m_ofn.lpstrTitle = "Select a file or type in a URL";

   if(cf.DoModal() == IDOK) {
		CString strFullPath = cf.GetPathName(); // Will be like: c:\tmp\junk.htm

      FILE *test = fopen(strFullPath, "r");

      if (!test) {
         // if the file doesn't exist, they probably typed a url...
         // so chop off the path (for some reason GetFileName doesn't work for us...
         strFullPath = strFullPath.Mid(strFullPath.ReverseFind('\\')+1);
      }else{
         fclose(test);
      }
		OpenURL(strFullPath);
	}
}

void CBrowserView::OnFileClose()
{
   mpBrowserFrame->PostMessage(WM_CLOSE);
}

void CBrowserView::OnFileSaveAs()
{
	// Try to get the file name part from the URL
	// To do that we first construct an obj which supports 
	// nsIRUI interface. Makes it easy to extract portions
	// of a URL like the filename, scheme etc. + We'll also
	// use it while saving this link to a file

   nsresult rv   = NS_OK;

   nsCOMPtr<nsIURI> currentURI;
	rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
      return;

   URISaveAs(currentURI, true);
}

void CBrowserView::OnCopyImageLocation()
{
	if(! mCtxMenuImgSrc.Length())
		return;

	if (! OpenClipboard())
		return;

	HGLOBAL hClipData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, mCtxMenuImgSrc.Length() + 1);
	if(! hClipData)
		return;

	char *pszClipData = (char*)::GlobalLock(hClipData);
	if(!pszClipData)
		return;

	mCtxMenuImgSrc.ToCString(pszClipData, mCtxMenuImgSrc.Length() + 1);

	GlobalUnlock(hClipData);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hClipData);
	CloseClipboard();
}

void CBrowserView::OnCopyLinkLocation()
{
	if(! mCtxMenuLinkUrl.Length())
		return;

	if (! OpenClipboard())
		return;

	HGLOBAL hClipData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, mCtxMenuLinkUrl.Length() + 1);
	if(! hClipData)
		return;

	char *pszClipData = (char*)::GlobalLock(hClipData);
	if(!pszClipData)
		return;

	mCtxMenuLinkUrl.ToCString(pszClipData, mCtxMenuLinkUrl.Length() + 1);

	GlobalUnlock(hClipData);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hClipData);
	CloseClipboard();
}

void CBrowserView::OnOpenLink()
{
	if(mCtxMenuLinkUrl.Length())
		OpenURL(mCtxMenuLinkUrl.get());
}

void CBrowserView::OnOpenLinkInNewWindow()
{
	if(mCtxMenuLinkUrl.Length())
		OpenURLInNewWindow(mCtxMenuLinkUrl.get());
}

void CBrowserView::OnOpenLinkInBackground()
{
	if(mCtxMenuLinkUrl.Length())
		OpenURLInNewWindow(mCtxMenuLinkUrl.get(), true);
}

void CBrowserView::OnViewImageInNewWindow()
{
	if(mCtxMenuImgSrc.Length())
		OpenURLInNewWindow(mCtxMenuImgSrc.get());
}

void CBrowserView::OnSaveLinkAs()
{
   
   if(! mCtxMenuLinkUrl.Length())
		return;

	// Try to get the file name part from the URL
	// To do that we first construct an obj which supports 
	// nsIRUI interface. Makes it easy to extract portions
	// of a URL like the filename, scheme etc. + We'll also
	// use it while saving this link to a file
	nsresult rv   = NS_OK;
	nsCOMPtr<nsIURI> linkURI;
	rv = NS_NewURI(getter_AddRefs(linkURI), mCtxMenuLinkUrl);
	if (NS_FAILED(rv)) 
		return;

   URISaveAs(linkURI);
}

void CBrowserView::OnSaveImageAs()
{
	if(! mCtxMenuImgSrc.Length())
		return;

	// Try to get the file name part from the URL
	// To do that we first construct an obj which supports 
	// nsIRUI interface. Makes it easy to extract portions
	// of a URL like the filename, scheme etc. + We'll also
	// use it while saving this link to a file
	nsresult rv   = NS_OK;
	nsCOMPtr<nsIURI> imageURI;
	rv = NS_NewURI(getter_AddRefs(imageURI), mCtxMenuImgSrc);
	if (NS_FAILED(rv)) 
		return;

   URISaveAs(imageURI);
}

void CBrowserView::OnFilePrint()
{
  nsresult rv;
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) 
  {
    prefs->SetBoolPref("print.use_native_print_dialog", PR_TRUE);
    prefs->SetBoolPref("print.show_print_progress", PR_FALSE);
  }
  else
	NS_ASSERTION(PR_FALSE, "Could not get preferences service");

  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	if(print)
	{

   // Get the printer settings
   if (!m_PrintSettings)
      print->GetNewPrintSettings(getter_AddRefs(m_PrintSettings));

    CPrintProgressDialog  dlg(mWebBrowser, m_PrintSettings);

	  nsCOMPtr<nsIURI> currentURI;
	  nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
    if(NS_SUCCEEDED(rv) || currentURI) 
    {
	    nsCAutoString path;
	    currentURI->GetPath(path);
      dlg.SetURI(path.get());
    }
    m_bCurrentlyPrinting = TRUE;
    dlg.DoModal();
    m_bCurrentlyPrinting = FALSE;
  }
}

void CBrowserView::OnUpdateViewStatusBar(CCmdUI* pCmdUI)
{
   HWND hWndStatus = FindWindowEx(this->GetParent()->GetSafeHwnd(), NULL, STATUSCLASSNAME, NULL);
   BOOL bVis = ::IsWindowVisible(hWndStatus);

   pCmdUI->SetCheck(bVis);
}


 

void CBrowserView::OnFilePrintPreview()                                         
{

   nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
   if(print) {
      // Get the printer settings
      if (!m_PrintSettings)
         print->GetNewPrintSettings(getter_AddRefs(m_PrintSettings));

      print->PrintPreview(m_PrintSettings, nsnull);
   
   }
}

static float GetFloatFromStr(const char* aStr, float aMaxVal = 1.0)             
{                                                                               
   float val;                                                                    
   sscanf(aStr, "%f", &val);                                                     
   if (val <= aMaxVal) {                                                         
      return val;                                                                 
   } else {                                                                      
      return 0.5;                                                                 
   }                                                                             
}

static PRUnichar* GetUnicodeFromCString(const CString& aStr)
{
  nsString str;
  str.AssignWithConversion(LPCSTR(aStr));
  return ToNewUnicode(str);
}


void CBrowserView::OnFilePrintSetup()
{

   // Get the printer settings
   nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	if(print && !m_PrintSettings)
      print->GetNewPrintSettings(getter_AddRefs(m_PrintSettings));
   
   CPrintSetupDialog  dlg(m_PrintSettings);
  if (dlg.DoModal() == IDOK && m_PrintSettings != NULL) {
    m_PrintSettings->SetMarginTop(GetFloatFromStr(dlg.m_TopMargin));
    m_PrintSettings->SetMarginLeft(GetFloatFromStr(dlg.m_LeftMargin));
    m_PrintSettings->SetMarginRight(GetFloatFromStr(dlg.m_RightMargin));
    m_PrintSettings->SetMarginBottom(GetFloatFromStr(dlg.m_BottomMargin));

    m_PrintSettings->SetScaling(double(dlg.m_Scaling) / 100.0);
    m_PrintSettings->SetPrintBGColors(dlg.m_PrintBGColors);
    m_PrintSettings->SetPrintBGColors(dlg.m_PrintBGImages);

    short  type;
    double width;
    double height;
    dlg.GetPaperSizeInfo(type, width, height);
    m_PrintSettings->SetPaperSizeType(type);
    m_PrintSettings->SetPaperWidth(width);
    m_PrintSettings->SetPaperHeight(height);

    PRUnichar* uStr;
    uStr = GetUnicodeFromCString(dlg.m_HeaderLeft);
    m_PrintSettings->SetHeaderStrLeft(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

    uStr = GetUnicodeFromCString(dlg.m_HeaderMiddle);
    m_PrintSettings->SetHeaderStrCenter(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

    uStr = GetUnicodeFromCString(dlg.m_HeaderRight);
    m_PrintSettings->SetHeaderStrRight(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

    uStr = GetUnicodeFromCString(dlg.m_FooterLeft);
    m_PrintSettings->SetFooterStrLeft(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

    uStr = GetUnicodeFromCString(dlg.m_FooterMiddle);
    m_PrintSettings->SetFooterStrCenter(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

    uStr = GetUnicodeFromCString(dlg.m_FooterRight);
    m_PrintSettings->SetFooterStrRight(uStr);
    if (uStr != nsnull) nsMemory::Free(uStr);

  }
}
                                                                           


void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting);
}


void CBrowserView::OnKmeleonHome()
{
   OpenURL(KMELEON_HOMEPAGE_URL);
}

void CBrowserView::OnKmeleonForum()
{
   OpenURL(KMELEON_FORUM_URL);
}

void CBrowserView::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CBrowserView::OnWindowNext()
{
   CBrowserFrame* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(mpBrowserFrame);
   theApp.m_FrameWndLst.GetNext(pos);
   if (pos)  pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
   else      pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetHead();
   
   pFrame->ActivateFrame();
}

void CBrowserView::OnWindowPrev()
{
   CBrowserFrame* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(mpBrowserFrame);
   theApp.m_FrameWndLst.GetPrev(pos);
   if (pos)  pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetPrev(pos);
   else      pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetTail();
   
   pFrame->ActivateFrame();
}
