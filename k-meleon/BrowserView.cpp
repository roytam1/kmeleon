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
#include "Dialogs.h"
#include "PrintProgressDialog.h"    
#include "ToolBarEx.h"
#include "Utils.h"
#include "KmeleonConst.h"
#include "About.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//static const char* g_HomeURL = "http://www.mozilla.org/projects/embedding";
static const char* KMELEON_HOMEPAGE_URL = "http://www.kmeleon.org";
static const char* KMELEON_FORUM_URL = "http://www.kmeleon.org/forum/";

// Register message for FindDialog communication                                                                                
static UINT WM_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CBrowserView, CWnd)
	//{{AFX_MSG_MAP(CBrowserView)
   ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
   ON_WM_TIMER()
   ON_WM_MOUSEACTIVATE()
   ON_WM_DROPFILES()
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
	ON_COMMAND(ID_OPEN_LINK_IN_NEW_WINDOW, OnOpenLinkInNewWindow)
	ON_COMMAND(ID_OPEN_LINK_IN_BACKGROUND, OnOpenLinkInBackground)
	ON_COMMAND(ID_VIEW_IMAGE, OnViewImageInNewWindow)
	ON_COMMAND(ID_SAVE_LINK_AS, OnSaveLinkAs)
	ON_COMMAND(ID_SAVE_IMAGE_AS, OnSaveImageAs)
   ON_COMMAND(ID_EDIT_FIND, OnShowFindDlg)
   ON_COMMAND(ID_FILE_PRINT, OnFilePrint) 
   ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
   ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
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
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
	ON_COMMAND(ID_WINDOW_PREV, OnWindowPrev)
	ON_WM_ACTIVATE()
	ON_MESSAGE(UWM_REFRESHTOOLBARITEM, RefreshToolBarItem)
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
   //dsti->SetItemType(nsIDocShellTreeItem::typeChromeWrapper);
   dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

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

  /*
  nsCOMPtr<nsIWebNavigation> mWebNav(do_QueryInterface(mBaseWindow));
  mWebNav->LoadURI(NS_ConvertASCIItoUCS2("chrome://embed/content/simple-shell.xul"), nsIWebNavigation::LOAD_FLAGS_NONE);
  */


   /*
	// Set up the content listeners
   nsCOMPtr<nsIURIContentListener> uriListener;
   uriListener = do_QueryInterface(NS_STATIC_CAST(nsIURIContentListener*, mpBrowserImpl));
   NS_ENSURE_TRUE(uriListener, NS_ERROR_FAILURE);
	mWebBrowser->SetParentURIContentListener(uriListener);
*/

   // Register the BrowserImpl object to receive progress messages
   // These callbacks will be used to update the status/progress bars

   nsWeakPtr weakling( dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*, mpBrowserImpl))));
   (void)mWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));

	// Finally, show the web browser window
	mBaseWindow->SetVisibility(PR_TRUE);

	return S_OK;
}

HRESULT CBrowserView::DestroyBrowser() {

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

void CBrowserView::Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized) {
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

void CBrowserView::OnDropFiles( HDROP drop ){
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

// A new URL was entered in the URL bar
// Get the URL's text from the Urlbar's (ComboBox's) EditControl 
// and navigate to that URL
//
void CBrowserView::OnNewUrlEnteredInUrlBar()
{
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

void CBrowserView::OnSelectUrl(){
   mpBrowserFrame->m_wndUrlBar.SetFocus();
}

void CBrowserView::OnUrlKillFocus() {
   if (mpBrowserFrame->m_wndUrlBar.CheckFocus())
      mpBrowserFrame->m_wndUrlBar.ReturnFocus();
}

void CBrowserView::OnUrlEditChange() {
   mpBrowserFrame->m_wndUrlBar.EditChanged(TRUE);
}

BOOL CBrowserView::IsViewSourceUrl(CString& strUrl) {
   return (strUrl.Find("view-source:", 0) != -1) ? TRUE : FALSE;
}

BOOL CBrowserView::OpenViewSourceWindow(const char* pUrl) {

   // Use external viewer
   if (theApp.preferences.bSourceUseExternalCommand) {
      if (theApp.preferences.sourceCommand) {

         char *tempfile = GetTempFile();

         nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
         if(persist) {
            persist->SaveDocument(nsnull, tempfile, 0);

            char *command = new char[theApp.preferences.sourceCommand.GetLength() + strlen(tempfile) +2];
            
            strcpy(command, theApp.preferences.sourceCommand);
            strcat(command, " ");                              //append " filename" to the viewer command
            strcat(command, tempfile);
            
            STARTUPINFO si = { 0 };
            PROCESS_INFORMATION pi;
            si.cb = sizeof STARTUPINFO;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOW;

            CreateProcess(0,command,0,0,0,0,0,0,&si,&pi);      // launch external viewer
         }
         return TRUE;
      }
   }
   
   // use the internal viewer

	// Create a new browser frame in which we'll show the document source
	// Note that we're getting rid of the toolbars etc. by specifying
	// the appropriate chromeFlags
	PRUint32 chromeFlags =  nsIWebBrowserChrome::CHROME_WINDOW_BORDERS |
							nsIWebBrowserChrome::CHROME_TITLEBAR |
							nsIWebBrowserChrome::CHROME_WINDOW_RESIZE;
	CBrowserFrame* pFrm = CreateNewBrowserFrame(chromeFlags);
	if(!pFrm)
		return FALSE;

	// Finally, load this URI into the newly created frame
	pFrm->m_wndBrowserView.OpenURL(pUrl);

   pFrm->BringWindowToTop();

   return TRUE;
}                                                                               

void CBrowserView::OnViewSource() {
	if(! mWebNav)
		return;

	// Get the URI object whose source we want to view.
   nsresult rv = NS_OK;
   nsCOMPtr<nsIURI> currentURI;
   rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
   if(NS_FAILED(rv) || !currentURI)
      return;

   // Get the uri string associated with the nsIURI object
   nsXPIDLCString uriString;
   rv = currentURI->GetSpec(getter_Copies(uriString));
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
	AfxMessageBox("To Be Done...");
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


void CBrowserView::RefreshToolBarItem(WPARAM ItemID, LPARAM unused) {
	switch (ItemID) {
		case ID_NAV_BACK:
			m_refreshBackButton = TRUE;
			break;
		case ID_NAV_FORWARD:
			m_refreshForwardButton = TRUE;
			break;
	}
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

void CBrowserView::OnNavHome(){
  OpenURL(theApp.preferences.homePage);
}

BOOL CALLBACK SearchProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
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

void CBrowserView::OnNavSearch(){
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

void CBrowserView::OnCut(){
  if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
    mpBrowserFrame->m_wndUrlBar.Cut();
    return;
  }

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->CutSelection();
}

void CBrowserView::OnUpdateCut(CCmdUI* pCmdUI){
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

void CBrowserView::OnCopy(){
  if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
    mpBrowserFrame->m_wndUrlBar.Copy();
    return;
  }

	nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->CopySelection();
}

void CBrowserView::OnUpdateCopy(CCmdUI* pCmdUI){
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

void CBrowserView::OnPaste(){
  if (mpBrowserFrame->m_wndUrlBar.IsChild(GetFocus())){
    mpBrowserFrame->m_wndUrlBar.Paste();
    return;
  }
  
  nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);

	if(clipCmds)
		clipCmds->Paste();
}

void CBrowserView::OnUpdatePaste(CCmdUI* pCmdUI){
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

	CFileDialog cf(TRUE, NULL, NULL, OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					lpszFilter, this);
   cf.m_ofn.lpstrTitle = "Select a file or type in a URL";
	if(cf.DoModal() == IDOK)
	{
		CString strFullPath = cf.GetPathName(); // Will be like: c:\tmp\junk.htm
      FILE *test = fopen(strFullPath, "r");
      if (!test){
         // if the file doesn't exist, they probably typed a url...
         // so chop off the path (for some reason GetFileName doesn't work for us...
         strFullPath = strFullPath.Mid(strFullPath.ReverseFind('\\')+1);
      }else{
         fclose(test);
      }
		OpenURL(strFullPath);
	}
}

void CBrowserView::OnFileClose() {
   mpBrowserFrame->PostMessage(WM_CLOSE);
}

void CBrowserView::GetBrowserWindowTitle(nsCString& title)
{
   nsXPIDLString idlStrTitle;
	if(mBaseWindow)
		mBaseWindow->GetTitle(getter_Copies(idlStrTitle));

	title.AssignWithConversion(idlStrTitle);

	// Sanitize the title of all illegal characters
   title.CompressWhitespace();     // Remove whitespace from the ends
   title.StripChars("\\*|:\"><?"); // Strip illegal characters
   title.ReplaceChar('.', L'_');   // Dots become underscores
   title.ReplaceChar('/', L'-');   // Forward slashes become hyphens
}

void CBrowserView::OnFileSaveAs() {

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

NS_IMETHODIMP CBrowserView::URISaveAs(nsIURI* aURI, bool bDocument) {

   NS_ENSURE_ARG_POINTER(aURI);

	// Get the "path" portion (see nsIURI.h for more info
	// on various parts of a URI)
	nsXPIDLCString path;
	aURI->GetPath(getter_Copies(path));

   char sDefault[] = "default.htm";
   char *pBuf = NULL, *pFileName = sDefault;

   if (strlen(path.get()) > 1) {
	   // The path may have the "/" char in it - strip those
	   pBuf = new char[strlen(path.get()) + 5];      // +5 for ".htm" to be safely appended, if necessary
      strcpy(pBuf, path.get());
	   char* slash = strrchr(pBuf, '/');
      if (strlen(slash) > 1)
         pFileName = slash+1;                                  // filename = file.ext
      else {
         while ((slash > pBuf) && (strlen(slash) <= 1)) {      // strip off extra /es
            *slash = 0;
            slash--;
   	      slash = strrchr(pBuf, '/');
         }
         if (slash && (strlen(slash) > 0)) {
            pFileName=slash+1;                                 // filename = directory.htm
            strcat(pFileName, ".htm");
         }
      }
   }
   else {
   	aURI->GetHost(getter_Copies(path));
      if (strlen(path.get()) >= 1) {
   	   pBuf = new char[strlen(path.get()) + 5];  // +5 for ".htm" to be safely appended, if necessary
         strcpy(pBuf, path.get());
         pFileName = pBuf;
         for (int x=strlen(pBuf)-1; x>=0; x--)
            if (pBuf[x] == '.') pBuf[x] = '_';
         strcat(pBuf, ".htm");                     // filename = www_host_com.htm
      }
   }

   char *extension = strrchr(pFileName, '.');
   if (!extension) {
      extension = strrchr(sDefault, '.');
      strcat(pFileName, extension);
   }
   extension++;

   char lpszFilter[256];
      strcpy(lpszFilter, extension);
      strcat(lpszFilter, " Files (*.");
      strcat(lpszFilter, extension);
      strcat(lpszFilter, ")|*.");
      strcat(lpszFilter, extension);
      strcat(lpszFilter, "|");
      strcat(lpszFilter, "Web Page, HTML Only (*.htm;*.html)|*.htm;*.html|");
      if (bDocument)
        strcat(lpszFilter, "Web Page, Complete (*.htm;*.html)|*.htm;*.html|");
      strcat(lpszFilter,"Text File (*.txt)|*.txt|"
        "All Files (*.*)|*.*||");
  
   CFileDialog cf(FALSE, extension, (const char *)pFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					lpszFilter, this);

   cf.m_ofn.lpstrInitialDir = theApp.preferences.saveDir;

   if(cf.DoModal() == IDOK) {
		CString strFullPath = cf.GetPathName();            // Will be like: c:\tmp\junk.htm
      theApp.preferences.saveDir = cf.GetPathName();
      int idxSlash;
      idxSlash = theApp.preferences.saveDir.ReverseFind('\\');
      theApp.preferences.saveDir = theApp.preferences.saveDir.Mid(0, idxSlash+1);
		char *pStrFullPath = strFullPath.GetBuffer(0);     // Get char * for later use
		
		CString strDataPath; 
		char *pStrDataPath = NULL;
      if (bDocument && (cf.m_ofn.nFilterIndex == 3)) {

         // cf.m_ofn.nFilterIndex == 3 indicates that the
			// user wants to save the complete document including
			// all frames, images, scripts, stylesheets etc.

			int idxPeriod = strFullPath.ReverseFind('.');
			strDataPath = strFullPath.Mid(0, idxPeriod);
			strDataPath += "_files";

			// At this stage strDataPath will be of the form
			// c:\tmp\junk_files - assuming we're saving to a file
			// named junk.htm
			// Any images etc in the doc will be saved to a dir
			// with the name c:\tmp\junk_files

			pStrDataPath = strDataPath.GetBuffer(0); //Get char * for later use
		}

      // Save the file
      nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
      if(persist) {
         if (bDocument)
            persist->SaveDocument(nsnull, pStrFullPath, pStrDataPath);
         else
            persist->SaveURI(aURI, nsnull, strFullPath);
      }
	}

   if (pBuf)
      delete pBuf;

   return NS_OK;
}

void CBrowserView::OpenURL(const char* pUrl){
   if(mWebNav)
      mWebNav->LoadURI(NS_ConvertASCIItoUCS2(pUrl).get(), nsIWebNavigation::LOAD_FLAGS_NONE);
}

void CBrowserView::OpenURL(const PRUnichar* pUrl) {
   if(mWebNav)
      mWebNav->LoadURI(pUrl, nsIWebNavigation::LOAD_FLAGS_NONE);
}

CBrowserFrame* CBrowserView::CreateNewBrowserFrame(PRUint32 chromeMask, 
									PRInt32 x, PRInt32 y, 
									PRInt32 cx, PRInt32 cy,
									PRBool bShowWindow)
{  
   CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
	if(!pApp)
		return NULL;

  return pApp->CreateNewBrowserFrame(chromeMask, x, y, cx, cy, bShowWindow);
}

void CBrowserView::OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground){
	if(!pUrl)
		return;

   // create hidden window
   CBrowserFrame* pFrm = CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, false);
	if(!pFrm)
		return;

   // show the window
   if (bBackground)
         pFrm->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
   else
      pFrm->ShowWindow(SW_SHOW);

   pFrm->UpdateWindow();
   
   // Load the URL into it...

	// Note that OpenURL() is overloaded - one takes a "char *"
	// and the other a "PRUniChar *". We're using the "PRUnichar *"
	// version here

	pFrm->m_wndBrowserView.OpenURL(pUrl);
}


void CBrowserView::LoadHomePage()
{
  if (theApp.preferences.bStartHome)
    OnNavHome();
  else
    OpenURL("about:blank");
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

void CBrowserView::OnSaveLinkAs() {
   
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

void CBrowserView::OnKmeleonHome()
{
   OpenURL(KMELEON_HOMEPAGE_URL);
}

void CBrowserView::OnKmeleonForum()
{
   OpenURL(KMELEON_FORUM_URL);
}

void CBrowserView::OnShowFindDlg() {

   // When the the user chooses the Find menu item
	// and if a Find dlg. is already being shown
	// just set focus to the existing dlg instead of
	// creating a new one
	if(m_pFindDlg)	{
		m_pFindDlg->SetFocus();
		return;
	}

	CString csSearchStr;
	PRBool bMatchCase = PR_FALSE;
	PRBool bMatchWholeWord = PR_FALSE;
	PRBool bWrapAround = PR_TRUE;
	PRBool bSearchBackwards = PR_FALSE;

	// See if we can get and initialize the dlg box with
	// the values/settings the user specified in the previous search
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(finder) {
		nsXPIDLString stringBuf;
		finder->GetSearchString(getter_Copies(stringBuf));
		csSearchStr = stringBuf.get();

      if (csSearchStr != "") {
		   finder->GetMatchCase(&bMatchCase);
		   finder->GetEntireWord(&bMatchWholeWord);
		   finder->GetWrapFind(&bWrapAround);
		   finder->GetFindBackwards(&bSearchBackwards);		
      }
      else {
         csSearchStr = theApp.preferences.findSearchStr;
	      bMatchCase = theApp.preferences.bFindMatchCase;
	      bMatchWholeWord = theApp.preferences.bFindMatchWholeWord;
	      bWrapAround = theApp.preferences.bFindWrapAround;
	      bSearchBackwards = theApp.preferences.bFindSearchBackwards;
      }
	}

	m_pFindDlg = new CFindDialog(csSearchStr, bMatchCase, bMatchWholeWord, bWrapAround, bSearchBackwards, this);
	m_pFindDlg->Create(TRUE, NULL, NULL, 0, this);
}

void CBrowserView::OnFindNext() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));

   if(!finder) return;

	nsXPIDLString stringBuf;
   finder->GetSearchString(getter_Copies(stringBuf));
   if (stringBuf.get()[0]) {

      PRBool didFind;
	   finder->FindNext(&didFind);
   }
   else {
      OnShowFindDlg();
   }
}

void CBrowserView::OnFindPrev() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));

   if(!finder) return;

   PRBool rv;

   finder->GetFindBackwards(&rv);
   finder->SetFindBackwards(rv^2);  // reverse the find direction

	nsXPIDLString stringBuf;
   finder->GetSearchString(getter_Copies(stringBuf));
   if (stringBuf.get()[0]) {
      PRBool didFind;
	   finder->FindNext(&didFind);
   }
   else {
      OnShowFindDlg();
   }
      
   finder->GetFindBackwards(&rv);
   finder->SetFindBackwards(rv^2);  // reset the initial find direction
}

// This will be called whenever the user pushes the Find
// button in the Find dialog box
// This method gets bound to the WM_FINDMSG windows msg via the
//
// ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg)
//
//  message map entry.
//
// WM_FINDMSG (which is registered towards the beginning of this file)
// is the message via which the FindDialog communicates with this view
//
LRESULT CBrowserView::OnFindMsg(WPARAM wParam, LPARAM lParam) {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));

	if(!finder) return NULL;

	// Get the pointer to the current Find dialog box
	CFindDialog* dlg = (CFindDialog *) CFindReplaceDialog::GetNotifier(lParam);
	if(!dlg) return NULL;

	// Has the user decided to terminate the dialog box?
	if(dlg->IsTerminating()) return NULL;

	if(dlg->FindNext()) {

      // save the find settings
      theApp.preferences.findSearchStr = dlg->GetFindString().GetBuffer(0);
	   theApp.preferences.bFindMatchCase = dlg->MatchCase();
	   theApp.preferences.bFindMatchWholeWord = dlg->MatchWholeWord();
	   theApp.preferences.bFindWrapAround = dlg->WrapAround();
	   theApp.preferences.bFindSearchBackwards = dlg->SearchBackwards();


      // create the find query
      nsString searchString;
		searchString.AssignWithConversion(theApp.preferences.findSearchStr.GetBuffer(0));
		finder->SetSearchString(searchString.get());
      
      finder->SetMatchCase((theApp.preferences.bFindMatchCase) ? PR_TRUE : PR_FALSE);
		finder->SetEntireWord((theApp.preferences.bFindMatchWholeWord) ? PR_TRUE : PR_FALSE);
		finder->SetWrapFind((theApp.preferences.bFindWrapAround) ? PR_TRUE : PR_FALSE);
		finder->SetFindBackwards((theApp.preferences.bFindSearchBackwards) ? PR_TRUE : PR_FALSE);

		PRBool didFind;
		nsresult rv = finder->FindNext(&didFind);
      
      return (NS_SUCCEEDED(rv) && didFind);
	}
	return 0;
}

void CBrowserView::OnFilePrint()
{
   nsCOMPtr<nsIDOMWindow> domWindow;
   mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
   if(domWindow) {
      nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
      if(print) {
         CPrintProgressDialog  dlg(mWebBrowser, domWindow);

         nsCOMPtr<nsIURI> currentURI;
         nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));

         if(rv || currentURI) {
            nsXPIDLCString path;
            currentURI->GetPath(getter_Copies(path));
            dlg.SetURI(path.get());
         }
         m_bCurrentlyPrinting = TRUE;
         dlg.DoModal();
         m_bCurrentlyPrinting = FALSE;
      }
   }  
}

void CBrowserView::OnUpdateViewStatusBar(CCmdUI* pCmdUI) {
   HWND hWndStatus = FindWindowEx(this->GetParent()->GetSafeHwnd(), NULL, STATUSCLASSNAME, NULL);
   BOOL bVis = ::IsWindowVisible(hWndStatus);

   pCmdUI->SetCheck(bVis);
}

/////////////////////////////////////////////////////////////////////////////
void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting);
}

// Called from the busy state related methods in the 
// BrowserFrameGlue object
//
// When aBusy is TRUE it means that browser is busy loading a URL
// When aBusy is FALSE, it's done loading
// We use this to update our STOP tool bar button
//
// We basically note this state into a member variable
// The actual toolbar state will be updated in response to the
// ON_UPDATE_COMMAND_UI method - OnUpdateNavStop() being called
//
void CBrowserView::UpdateBusyState(PRBool aBusy) {
	mbDocumentLoading = aBusy;

	if (mbDocumentLoading)
		mpBrowserFrame->m_wndAnimate.Play(0, -1, -1);   

   else {
      mpBrowserFrame->m_wndAnimate.Stop();
		mpBrowserFrame->m_wndAnimate.Seek(0);
  }
}

void CBrowserView::SetCtxMenuLinkUrl(nsAutoString& strLinkUrl)
{
	mCtxMenuLinkUrl = strLinkUrl;
}

void CBrowserView::SetCtxMenuImageSrc(nsAutoString& strImgSrc)
{
	mCtxMenuImgSrc = strImgSrc;
}


/*
** Middle button panning shit
*/
int CBrowserView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) {
	switch (message) {
		case WM_MBUTTONDOWN:
			if(m_panning) StopPanning();
			else StartPanning();
			return TRUE;
		case WM_MBUTTONUP:
			{
			}
			return TRUE;
	}
	
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CBrowserView::OnTimer(UINT nIDEvent){
	switch(nIDEvent){
		case 0x1:
			if(m_panning) {
				nsCOMPtr<nsIScrollable> s(do_QueryInterface(mWebBrowser));
        
        if(!s) return;

				POINT p;
				GetCursorPos(&p);

				int scroll_x,scroll_y;

				s->GetCurScrollPos(s->ScrollOrientation_X,&scroll_x);
				s->GetCurScrollPos(s->ScrollOrientation_Y,&scroll_y);

				if(abs(p.y-m_panningPoint.y)>4)
					if(p.y>m_panningPoint.y)	scroll_y+=(p.y-m_panningPoint.y)*2;
					else scroll_y-=(m_panningPoint.y-p.y)*2;

				if(abs(p.x-m_panningPoint.x)>4)
					if(p.x>m_panningPoint.x)	scroll_x+=(p.x-m_panningPoint.x)*2;
					else scroll_x-=(m_panningPoint.x-p.x)*2;

				s->SetCurScrollPosEx(scroll_x,scroll_y);

        int cursorx=p.x-m_panningPoint.x,cursory=p.y-m_panningPoint.y;
        int cursor=IDC_PAN;
        if(cursory<-4) cursor=IDC_PAN_UP;
        if(cursory>4) cursor=IDC_PAN_DOWN;
        if(cursorx<-4)
        {
          if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPLEFT;
          else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNLEFT;
          else cursor=IDC_PAN_LEFT;
        }
        if(cursorx>4)
        {
          if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPRIGHT;
          else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNRIGHT;
          else cursor=IDC_PAN_RIGHT;
        }

        HCURSOR c=LoadCursor(theApp.m_hInstance,MAKEINTRESOURCE(cursor));
        SetCursor(c);
			}
			return;
	}
	
	CWnd::OnTimer(nIDEvent);
}

void CBrowserView::StartPanning(){
	m_panning = 1;
	GetCursorPos(&m_panningPoint);
	SetCapture();
	SetTimer(0x1,10,NULL);

	SetFocus();
}

void CBrowserView::StopPanning(){
	ReleaseCapture();
	KillTimer(0x1);
	m_panning = 0;
}

BOOL CBrowserView::PreTranslateMessage(MSG* pMsg) {
  if(m_panning && (pMsg->message==WM_SETCURSOR || pMsg->message==WM_MOUSEMOVE))
    return TRUE;

  if(m_panning && (pMsg->message==WM_LBUTTONDOWN || pMsg->message==WM_RBUTTONDOWN))
    StopPanning();
	
	return CWnd::PreTranslateMessage(pMsg);
}

char * CBrowserView::GetTempFile() {

   m_tempFileCount++;
   
   char ** newFileList = new char*[m_tempFileCount];                             // create new index

   memcpy(newFileList, m_tempFileList, ((m_tempFileCount-1)*sizeof(char**)) );   // copy old index

   if (m_tempFileCount>1) delete m_tempFileList;                                 // delete old index
   m_tempFileList = newFileList;

   char *newFile = new char[MAX_PATH];
 
   char temppath[MAX_PATH];
   GetTempPath(MAX_PATH, temppath);
   GetTempFileName(temppath, "kme", 0, newFile);                                 // create tempfile name
   
   m_tempFileList[m_tempFileCount-1] = newFile;

   return newFile;
}

void CBrowserView::DeleteTempFiles() {

   for (int x=0;x<m_tempFileCount;x++) {
      DeleteFile(m_tempFileList[x]);
      delete m_tempFileList[x];
   }
   if (m_tempFileCount > 0) delete m_tempFileList;

}

// Show the AboutDlg
void CBrowserView::OnAppAbout() {
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CBrowserView::OnWindowNext() {
   CBrowserFrame* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(mpBrowserFrame);
   theApp.m_FrameWndLst.GetNext(pos);
   if (pos)  pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
   else      pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetHead();
   
   pFrame->ActivateFrame();
}

void CBrowserView::OnWindowPrev() {
   CBrowserFrame* pFrame;
	POSITION pos = theApp.m_FrameWndLst.Find(mpBrowserFrame);
   theApp.m_FrameWndLst.GetPrev(pos);
   if (pos)  pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetPrev(pos);
   else      pFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetTail();
   
   pFrame->ActivateFrame();
}

int CBrowserView::GetCurrentURI(char *sURI) {
   if(! mWebNav)
		return 0;

	nsresult rv = NS_OK;
   
   nsCOMPtr<nsIURI> currentURI;
	rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
      return 0;

   // Get the uri string associated with the nsIURI object
	nsXPIDLCString uriString;
	rv = currentURI->GetSpec(getter_Copies(uriString));
	if(NS_FAILED(rv))
		return 0;

   int len = strlen(uriString.get());
   if (sURI)
      strcpy(sURI, uriString.get());
   return len;
}

 

void CBrowserView::ShowSecurityInfo()                                           
{
   HWND hParent = mpBrowserFrame->m_hWnd;

   if(m_SecurityState == SECURITY_STATE_INSECURE) { 
      CString csMsg;
      csMsg.LoadString(IDS_NOSECURITY_INFO);

      ::MessageBox(hParent, csMsg, "K-Meleon", MB_OK);

      return;
   }                                                                           
   ::MessageBox(hParent, "This page is secure.", "K-Meleon", MB_OK);
}
