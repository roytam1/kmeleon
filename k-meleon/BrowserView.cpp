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
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "PrintProgressDialog.h"
//#include "nsPrintSettingsImpl.h"
#include "PrintSetupDialog.h"
#include "ToolBarEx.h"
#include "Utils.h"
#include "KmeleonConst.h"
#include "kmeleon_plugin.h"
#include "nsIDOMEventTarget.h"
#include <wininet.h>

extern CMfcEmbedApp theApp;
extern nsresult NewURI(nsIURI **result, const nsAString &spec);
extern nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const char* KMELEON_HOMEPAGE_URL = "http://kmeleon.sourceforge.net/";
static const char* KMELEON_FORUM_URL = "http://kmeleon.sourceforge.net/forum/";
static const char* KMELEON_FAQ_URL = "http://kmeleon.sourceforge.net/docs/faq.php";
static const char* KMELEON_MANUAL_URL = "http://kmeleon.sourceforge.net/manual/";
static const char* ABOUT_PLUGINS_URL = "about:plugins";
static const char* ABOUT_KMELEON = "about:";

// Register message for FindDialog communication
static UINT WM_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CBrowserView, CWnd)
    //{{AFX_MSG_MAP(CBrowserView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_MOUSEACTIVATE()
    ON_CBN_DROPDOWN(ID_URL_BAR, OnUrlBarDropDown)
    ON_CBN_SELENDCANCEL(ID_URL_BAR, OnUrlSelectedInUrlBarCancel)
    ON_CBN_SELENDOK(ID_URL_BAR, OnUrlSelectedInUrlBarOk)
    ON_CBN_CLOSEUP(ID_URL_BAR, OnUrlSelectedInUrlBar)
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
    ON_COMMAND(ID_NAV_FORCE_RELOAD, OnNavForceReload)
    ON_COMMAND(ID_FILE_SAVE_FRAME_AS, OnFileSaveFrameAs)
    ON_COMMAND(ID_NAV_STOP, OnNavStop)
    ON_COMMAND(ID_NAV_GO, OnNewUrlEnteredInUrlBar)
    ON_COMMAND(ID_OFFLINE, OnToggleOffline)
    ON_COMMAND(ID_EDIT_CUT, OnCut)
    ON_COMMAND(ID_EDIT_COPY, OnCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnPaste)
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
    ON_COMMAND(ID_EDIT_FINDNEXT, OnFindNext)
    ON_COMMAND(ID_EDIT_FINDPREV, OnFindPrev)
    //ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg) 
    ON_COMMAND(ID_LINK_KMELEON_HOME, OnKmeleonHome)
    ON_COMMAND(ID_LINK_KMELEON_FORUM, OnKmeleonForum)
    ON_UPDATE_COMMAND_UI(ID_NAV_BACK, OnUpdateNavBack)
    ON_UPDATE_COMMAND_UI(ID_NAV_FORWARD, OnUpdateNavForward)
    ON_UPDATE_COMMAND_UI(ID_NAV_STOP, OnUpdateNavStop)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdatePaste)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
    // ON_UPDATE_COMMAND_UI(ID_FILE_PRINTSETUP, OnUpdatePrintSetup) 
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINTPREVIEW, OnUpdateFilePrintPreview)
    ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IMAGE, OnUpdateViewImage)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
    ON_COMMAND(ID_WINDOW_PREV, OnWindowPrev)
    ON_COMMAND(ID_FONT_INCREASE, OnIncreaseFont)
    ON_COMMAND(ID_FONT_DECREASE, OnDecreaseFont)
    ON_COMMAND(ID_LINK_KMELEON_FAQ, OnKmeleonFAQ)
    ON_COMMAND(ID_LINK_KMELEON_MANUAL, OnKmeleonManual)
    ON_COMMAND(ID_LINK_ABOUT_PLUGINS, OnAboutPlugins)
    ON_COMMAND(ID_MOUSE_ACTION, OnMouseAction)
	ON_COMMAND(IDC_WRAP_AROUND, OnWrapAround)
	ON_COMMAND(IDC_MATCH_CASE, OnMatchCase)
	ON_COMMAND(IDC_HIGHLIGHT, OnHighlight)
    ON_WM_ACTIVATE()
    ON_MESSAGE(UWM_REFRESHTOOLBARITEM, RefreshToolBarItem)

    ON_NOTIFY(CBEN_BEGINEDIT, ID_URL_BAR, OnEditURL)
    //ON_NOTIFY(CBEN_DRAGBEGIN, ID_URL_BAR, OnDragURL)
    //ON_WM_DROPFILES()
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
    m_bCurrentlyPrinting = FALSE;

    m_SecurityState = SECURITY_STATE_INSECURE;

    m_InPrintPreview = FALSE;

    m_tempFileCount = 0;

    m_iGetNodeHack = 0;
    m_pGetNode = NULL;
	m_lastMouseActionNode = nsnull;

    m_panning = FALSE;
    maccel_pan = FALSE;
    s = NULL;
}

CBrowserView::~CBrowserView()
{
}

// This is a good place to create the embeddable browser
// instance
//
int CBrowserView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    nsresult rv = CreateBrowser();
	if (NS_FAILED(rv))
		return -1;

    // DragAcceptFiles();
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
    nsWeakPtr weakling(
        do_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*, mpBrowserImpl)));
    (void)mWebBrowser->AddWebBrowserListener(weakling, 
                                NS_GET_IID(nsIWebProgressListener));
    

	// Get the focus object
	mWebBrowserFocus = do_QueryInterface(mWebBrowser);
	NS_ENSURE_TRUE(mWebBrowserFocus, NS_ERROR_FAILURE);

	rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
	if (NS_SUCCEEDED(rv) && !(mpBrowserFrame->m_chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME ))
	{
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("mousedown"),
			                        mpBrowserImpl, PR_FALSE);
	}
    // Finally, show the web browser window
    mBaseWindow->SetVisibility(PR_TRUE);

	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));	
    if(finder)
	{
		finder->SetWrapFind(theApp.preferences.bFindWrapAround ? PR_TRUE:PR_FALSE);
		finder->SetMatchCase(theApp.preferences.bFindMatchCase ? PR_TRUE:PR_FALSE);
	}
	
    return NS_OK;
}

HRESULT CBrowserView::DestroyBrowser()
{
    DeleteTempFiles();   

	if (mEventTarget && !(mpBrowserFrame->m_chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME ))
	{
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"),
				  mpBrowserImpl, PR_FALSE);
	}

    if(mBaseWindow)
    {
        mBaseWindow->Destroy();
        mBaseWindow = nsnull;
    }

    if(mpBrowserImpl)
    {
        mpBrowserImpl->Release();
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

void CBrowserView::OnEditURL( NMHDR * pNotifyStruct, LRESULT * result )
{
   *result = 0;
}


char* CBrowserView::NicknameLookup(const char* pUrl)
{
    char *p, *q, *r;
    char *nickUrl;
    char *retUrl;

    // Check for a nickname
    nickUrl = NULL;
    p = strdup(pUrl);                   // get entered URL
    q = SkipWhiteSpace(p);              // skip any leading spaces
    q = strchr(q, ' ');                 // look for a space
    
    if (q)                              // if more than one word
        *q = 0;                         // terminate first word
    
    theApp.plugins.SendMessage("*", "* FindNick", "FindNick", (long)SkipWhiteSpace(p), (long)&nickUrl);

    if (q)                              // if more than one word
        *q = ' ';                       // restore space

    if (nickUrl) {
        int len = strlen(nickUrl);
        r = strstr(nickUrl, "%s");       // look for %s
        if (r) {                         // if found
            *r = 0;                      // terminate string up to %s
            char *custUrl = (char*)malloc(len+INTERNET_MAX_URL_LENGTH);
            strcpy(custUrl, nickUrl);     // copy string before %s
            if (q)                        // if more than one word
                strcat(custUrl, q+1);     // copy second word
            strcat(custUrl, r+2);         // copy string after %s
            retUrl = custUrl;
            free(nickUrl);
        }
        else
            retUrl = nickUrl;
    }
    else 
        retUrl = strdup(pUrl);

    free(p);
    return retUrl;
} 


void CBrowserView::OnSelectUrl()
{
   mpBrowserFrame->m_wndUrlBar.SetFocus();
}

void CBrowserView::OnUrlKillFocus()
{
   if (mpBrowserFrame->m_wndUrlBar.CheckFocus())
      mpBrowserFrame->m_wndUrlBar.ReturnFocus(mbDocumentLoading);
   else
      mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
}

void CBrowserView::OnUrlEditChange()
{
   mpBrowserFrame->m_wndUrlBar.EditChanged(TRUE);
}

void CBrowserView::OpenSingleURL(char *url)
{
    char szOpenURLcmd[80];

    theApp.preferences.GetString("kmeleon.general.openurl", szOpenURLcmd, "");

    if (*szOpenURLcmd) {
        char *plugin = szOpenURLcmd;
        char *parameter = strchr(szOpenURLcmd, '(');
        if (parameter) {
            *parameter++ = 0;
            char *close = strchr(parameter, ')');
            if (close) {
                *close = 0;
                
                if (theApp.plugins.SendMessage(plugin, "* kmeleon.exe", parameter, (long)url, 0))
                    return;
            }
        }

        int idOpen = theApp.GetID(szOpenURLcmd);

        if (!idOpen) {
            char *plugin = szOpenURLcmd;
            char *parameter = strchr(szOpenURLcmd, '(');
            if (parameter) {
                *parameter++ = 0;
                char *close = strchr(parameter, ')');
                if (close)
                    *close = 0;
            }
      
            theApp.plugins.SendMessage(plugin, "* kmeleon.exe", "DoAccel", (long) parameter, (long)&idOpen);
        }


        switch (idOpen) {
        case ID_OPEN_LINK:
            OpenURL(url);
            return;
        case ID_OPEN_LINK_IN_BACKGROUND:
            OpenURLInNewWindow(url);
            return;
        case ID_OPEN_LINK_IN_NEW_WINDOW:
            OpenURLInNewWindow(url);
            return;
        }
    }

    OpenURL(url, OPEN_NORMAL);
}

void CBrowserView::OpenMultiURL(char *urls)
{
    char szOpenURLcmd[80];

    theApp.preferences.GetString("kmeleon.general.opengroup", szOpenURLcmd, "");

    if (*szOpenURLcmd) {
        char *plugin = szOpenURLcmd;
        char *parameter = strchr(szOpenURLcmd, '(');
        if (parameter) {
            *parameter++ = 0;
            char *close = strchr(parameter, ')');
            if (close) {
                *close = 0;
                
                if (theApp.plugins.SendMessage(plugin, "* kmeleon.exe", parameter, (long)urls, 0))
                    return;
            }
        }

        int idOpenX = -1;
        char *p = strchr(szOpenURLcmd, '|');
        if (p) {
            *p = 0;
            idOpenX = theApp.GetID(p+1);
        }
        int idOpen  = theApp.GetID(szOpenURLcmd);

        p = urls;
        while (p) {
            char *q = strchr(p, '\t');
            if (q) *q = 0;
            if (!*p) return;

            switch (idOpen) {
            case ID_OPEN_LINK:
                OpenURL(p);
                break;
            case ID_OPEN_LINK_IN_BACKGROUND:
                OpenURLInNewWindow(p, TRUE);
                break;
            case ID_OPEN_LINK_IN_NEW_WINDOW:
                OpenURLInNewWindow(p);
                break;
            default:
                OpenURL(urls);
                return;
            }

            idOpen = idOpenX==-1 ? idOpen : idOpenX;

            if (q)
                p = q+1;
            else
                return;
        }
    }

    OpenURL(urls);
}

// A new URL was entered in the URL bar
// Get the URL's text from the Urlbar's (ComboBox's) EditControl 
// and navigate to that URL
//
void CBrowserView::OnNewUrlEnteredInUrlBar()
{
   mWebBrowserFocus->Activate();

   mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
   
   // Get the currently entered URL
   CString strUrl;
   mpBrowserFrame->m_wndUrlBar.GetEnteredURL(strUrl);

   // Add what was just entered into the MRI list
   if (theApp.preferences.MRUbehavior == 2)
   mpBrowserFrame->m_wndUrlBar.AddURLToList(strUrl);

   if(IsViewSourceUrl(strUrl))
      OpenViewSourceWindow(strUrl.GetBuffer(0));
   else {
       char *pUrl = NicknameLookup(strUrl);

       // Navigate to that URL
       if (strchr(pUrl, '\t'))
           OpenMultiURL(pUrl);
       else
           OpenSingleURL(pUrl);

       free(pUrl);
   }
}

void CBrowserView::OnUrlBarDropDown()
{
}

void CBrowserView::OnUrlSelectedInUrlBarOk()
{
    mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
    mpBrowserFrame->m_wndUrlBar.SetSelected(FALSE);
}

void CBrowserView::OnUrlSelectedInUrlBarCancel()
{
    mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
    mpBrowserFrame->m_wndUrlBar.SetSelected(TRUE);
}

// A URL has  been selected from the UrlBar's dropdown list
void CBrowserView::OnUrlSelectedInUrlBar()
{
   CString strUrl;  

   if (!mpBrowserFrame->m_wndUrlBar.GetSelectedURL(strUrl))
      return;

   mWebBrowserFocus->Activate();

   mpBrowserFrame->m_wndUrlBar.RefreshMRUList();
   
   if(IsViewSourceUrl(strUrl))
      OpenViewSourceWindow(strUrl.GetBuffer(0));
   else {
       char *pUrl = NicknameLookup(strUrl);

       // Navigate to that URL
       if (strchr(pUrl, '\t'))
           OpenMultiURL(pUrl);
       else
           OpenSingleURL(pUrl);

       free(pUrl);
   }
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
    nsEmbedCString uriString;
    rv = currentURI->GetSpec(uriString);
    if(NS_FAILED(rv))
        return;

   // Build the view-source: url
   nsEmbedCString viewSrcUrl("view-source:");
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

void CBrowserView::OnNavForward() 
{
    if(mWebNav)
        mWebNav->GoForward();
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

void CBrowserView::_OnNavReload(BOOL force) 
{
	PRUint32 loadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;
	if (force)
		loadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE | 
                    nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;

	if (mWebNav)
	{
		// If there is no URI to reload, load the address in the url bar
		nsCOMPtr<nsIURI> currentURI;
		nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
		if(NS_FAILED(rv) || !currentURI)
		{
			CString url;
			mpBrowserFrame->m_wndUrlBar.GetEnteredURL(url);
			if (!url.IsEmpty())
				OpenURL(url);
		}
		else 
			mWebNav->Reload(loadFlags); 
	}
}

void CBrowserView::OnNavReload() 
{
	_OnNavReload(FALSE);
}

void CBrowserView::OnNavForceReload()
{
	_OnNavReload(TRUE);
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
    TCHAR *lpszFilter =
        _T("HTML Files Only (*.htm;*.html)\0*.htm;*.html\0")
        _T("All Files (*.*)\0*.*\0\0");

    OPENFILENAME ofn;
    TCHAR *szFileName = new TCHAR[INTERNET_MAX_URL_LENGTH];

    memset(&ofn, 0, sizeof(ofn));
    memset(szFileName, 0, INTERNET_MAX_URL_LENGTH);
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = lpszFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = INTERNET_MAX_URL_LENGTH;
    ofn.Flags = OFN_HIDEREADONLY | OFN_NOVALIDATE;

    if( ::GetOpenFileName(&ofn) ) {
        CString strFullPath = szFileName;

        FILE *test = _tfopen(strFullPath, _T("r"));

        USES_CONVERSION;
        if (!test) {
            // if the file doesn't exist, they probably typed a url...
            // so chop off the path (for some reason GetFileName doesn't work for us...
            strFullPath = strFullPath.Mid(strFullPath.ReverseFind('\\')+1);
            char *pUrl = NicknameLookup(strFullPath);
            strFullPath = A2T(pUrl);
            free(pUrl);
        }else{
            fclose(test);
        }
		
		char* path = T2A(strFullPath.GetBuffer(0));
        if (strchr(path, '\t'))
            OpenMultiURL(path);
        else
            OpenSingleURL(path);
    }

    delete szFileName;
}

void CBrowserView::GetBrowserWindowTitle(nsEmbedString& title)
{
    PRUnichar *idlStrTitle = nsnull;
    if(mBaseWindow)
        mBaseWindow->GetTitle(&idlStrTitle);
	
	USES_CONVERSION;
    //title.Assign(T2CW(idlStrTitle));
	title = idlStrTitle;
	CString csTitle;
	csTitle = W2CT(idlStrTitle);
	csTitle.Trim();

	int pos;
	while ( (pos=csTitle.FindOneOf(_T("\\*|:\"><?"))) != -1)
		csTitle.Delete(pos,1);

	csTitle.Replace(_T('.'),_T('_'));
	csTitle.Replace(_T('/'),_T('-'));

	title = T2CW(csTitle);
	nsMemory::Free(idlStrTitle);
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
 
    URISaveAs(currentURI, TRUE);
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

	UTF16ToCString(mCtxMenuLinkUrl,pszClipData);

    GlobalUnlock(hClipData);

    EmptyClipboard();
    SetClipboardData(CF_TEXT, hClipData);
    CloseClipboard();
}

void CBrowserView::OnOpenLink()
{
    if(!mCtxMenuLinkUrl.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURL(mCtxMenuLinkUrl.get(), currentURI);
}

void CBrowserView::OnOpenLinkInNewWindow()
{
    if(!mCtxMenuLinkUrl.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURLInNewWindow(mCtxMenuLinkUrl.get(), false, currentURI);
}

void CBrowserView::OnOpenLinkInBackground()
{
    if(!mCtxMenuLinkUrl.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURLInNewWindow(mCtxMenuLinkUrl.get(), true, currentURI);
}

void CBrowserView::OnViewImageInNewWindow()
{
    if(!mCtxMenuImgSrc.get())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURLInNewWindow(mCtxMenuImgSrc.get(), false, currentURI);
}

void CBrowserView::OnUpdateViewImage(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(mCtxMenuImgSrc.Length() > 0);
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
    rv = NewURI(getter_AddRefs(linkURI), mCtxMenuLinkUrl);
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
    rv = NewURI(getter_AddRefs(imageURI), mCtxMenuImgSrc);
    if (NS_FAILED(rv)) 
        return;

    URISaveAs(imageURI);
}

void CBrowserView::OnFilePrint()
{
    nsresult rv;
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
        prefs->SetBoolPref("print.use_native_print_dialog", PR_TRUE);
        prefs->SetBoolPref("print.show_print_progress", PR_FALSE);
    }
    else
        NS_ASSERTION(PR_FALSE, "Could not get preferences service");
   
    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    if(print){
      
        // Get the printer settings
        if (!m_PrintSettings) {
            if (!GetPrintSettings())
                return;
        }
         
        CPrintProgressDialog  dlg(mWebBrowser, m_PrintSettings);
      
        nsCOMPtr<nsIURI> currentURI;
        nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
        if(NS_SUCCEEDED(rv) || currentURI) 
        {
            nsEmbedCString path;
            currentURI->GetPath(path);
            dlg.SetURI(path.get());
        }
        m_bCurrentlyPrinting = TRUE;
        dlg.DoModal();
        m_bCurrentlyPrinting = FALSE;
    }
}

void CBrowserView::OnFilePrintPreview()                                         
{

    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
    if(print) {
        // Get the printer settings
        if (!m_PrintSettings) {
            if (!GetPrintSettings())
                return;
        }
        if (!m_InPrintPreview) {
            if (NS_SUCCEEDED(print->PrintPreview(m_PrintSettings, nsnull, nsnull)))
			{
				// WORKAROUND - FIX ME: Why the print preview doesn't use all the width?
				// So I'm forcing the window to reposition itself.
				CRect rect;
				GetClientRect(rect);
				mBaseWindow->SetPositionAndSize(0, 0, rect.right, rect.bottom, PR_TRUE);
                m_InPrintPreview = TRUE;
			}
        }
        else {
            if (NS_SUCCEEDED(print->ExitPrintPreview()))
                m_InPrintPreview = FALSE;
        }
    }
}

static PRUnichar* GetUnicodeFromCString(const CString& aStr)
{
#ifdef _UNICODE
    nsEmbedString str(aStr);
#else
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(aStr), NS_CSTRING_ENCODING_ASCII, str);
#endif
    return NS_StringCloneData(str);
}

static float GetFloatFromStr(const TCHAR* aStr)
{                                                                               
   float val;                                                                    
   _stscanf(aStr, _T("%f"), &val);                                                     
   return val;                                                                 
}

void CBrowserView::OnFilePrintSetup()
{
   nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
   if (!print)
      return;
   
   // Get the printer settings
   if (!m_PrintSettings) {
      if (!GetPrintSettings())
         return;
   }
   
   
   CPrintSetupDialog  dlg(m_PrintSettings, this);
   if (dlg.DoModal() == IDOK && m_PrintSettings != NULL) {

      theApp.preferences.printMarginTop = dlg.m_TopMargin;
      theApp.preferences.printMarginLeft = dlg.m_LeftMargin;
      theApp.preferences.printMarginRight = dlg.m_RightMargin;
      theApp.preferences.printMarginBottom = dlg.m_BottomMargin;

      m_PrintSettings->SetMarginTop(GetFloatFromStr(theApp.preferences.printMarginTop));
      m_PrintSettings->SetMarginLeft(GetFloatFromStr(theApp.preferences.printMarginLeft));
      m_PrintSettings->SetMarginRight(GetFloatFromStr(theApp.preferences.printMarginRight));
      m_PrintSettings->SetMarginBottom(GetFloatFromStr(theApp.preferences.printMarginBottom));

	  theApp.preferences.printShrinkToFit = dlg.m_ShrinkToFit;
      theApp.preferences.printScaling = dlg.m_Scaling;
      theApp.preferences.printBGColors = dlg.m_PrintBGColors;
      theApp.preferences.printBGImages = dlg.m_PrintBGImages;

	  m_PrintSettings->SetShrinkToFit(theApp.preferences.printShrinkToFit);
      m_PrintSettings->SetScaling(double(theApp.preferences.printScaling) / 100.0);
      m_PrintSettings->SetPrintBGColors(theApp.preferences.printBGColors);
      m_PrintSettings->SetPrintBGImages(theApp.preferences.printBGImages);
      
      short  unit;
      double width;
      double height;
      dlg.GetPaperSizeInfo(unit, width, height);

      theApp.preferences.printUnit = unit;
      theApp.preferences.printWidth.Format(_T("%f"),width);
      theApp.preferences.printHeight.Format(_T("%f"),height);

      m_PrintSettings->SetPaperSizeType(unit);
      m_PrintSettings->SetPaperWidth(width);
      m_PrintSettings->SetPaperHeight(height);

      PRUnichar* uStr;
      uStr = GetUnicodeFromCString(dlg.m_HeaderLeft);
      m_PrintSettings->SetHeaderStrLeft(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printHeaderLeft = dlg.m_HeaderLeft;
      
      uStr = GetUnicodeFromCString(dlg.m_HeaderMiddle);
      m_PrintSettings->SetHeaderStrCenter(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printHeaderMiddle = dlg.m_HeaderMiddle;
      
      uStr = GetUnicodeFromCString(dlg.m_HeaderRight);
      m_PrintSettings->SetHeaderStrRight(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printHeaderRight = dlg.m_HeaderRight;
      
      uStr = GetUnicodeFromCString(dlg.m_FooterLeft);
      m_PrintSettings->SetFooterStrLeft(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printFooterLeft = dlg.m_FooterLeft;
      
      uStr = GetUnicodeFromCString(dlg.m_FooterMiddle);
      m_PrintSettings->SetFooterStrCenter(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printFooterMiddle = dlg.m_FooterMiddle;
      
      uStr = GetUnicodeFromCString(dlg.m_FooterRight);
      m_PrintSettings->SetFooterStrRight(uStr);
      if (uStr != nsnull) nsMemory::Free(uStr);
      theApp.preferences.printFooterRight = dlg.m_FooterRight;
   }

   if (m_InPrintPreview) 
   {
	    nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
		if(print)
		{
			if (NS_SUCCEEDED(print->PrintPreview(m_PrintSettings, nsnull, nsnull)))
			{
				// WORKAROUND - FIX ME: Why the print preview doesn't use all the width?
				// So I'm forcing the window to reposition itself.
				CRect rect;
				GetClientRect(rect);
				mBaseWindow->SetPositionAndSize(0, 0, rect.right, rect.bottom, PR_TRUE);
			}
		}
   }
}

///////////////////////////////////////////////////////////////////////////// 
void CBrowserView::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting);
}

/////////////////////////////////////////////////////////////////////////////
/*
void CBrowserView::OnUpdatePrintSetup(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_bCurrentlyPrinting && !m_InPrintPreview);
} 
*/
void CBrowserView::Activate(BOOL bActive)
{
	if (bActive)
		mWebBrowserFocus->Activate();
	else
		mWebBrowserFocus->Deactivate();
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
    nsEmbedString viewSrcUrl;
    viewSrcUrl.Append(L"view-source:");
    viewSrcUrl.Append(mCtxMenuCurrentFrameURL.get());

    OpenViewSourceWindow(viewSrcUrl.get());
}


void CBrowserView::OnOpenFrame()
{
    if(!mCtxMenuCurrentFrameURL.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURL(mCtxMenuCurrentFrameURL.get(), currentURI);
}

void CBrowserView::OnOpenFrameInNewWindow()
{
    if(!mCtxMenuCurrentFrameURL.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURLInNewWindow(mCtxMenuCurrentFrameURL.get(), false, currentURI);
}

void CBrowserView::OnOpenFrameInBackground()
{
    if(!mCtxMenuCurrentFrameURL.Length())
        return;
    
    nsCOMPtr<nsIURI> currentURI;
    nsresult rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv))
        currentURI=NULL;

    OpenURLInNewWindow(mCtxMenuCurrentFrameURL.get(), true, currentURI);
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

#if 0
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
#endif

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
      OpenURL(theApp.preferences.searchEngine + search);
   }
}

void CBrowserView::OnFileClose()
{
   mpBrowserFrame->PostMessage(WM_CLOSE);
}

void CBrowserView::OnFileSaveFrameAs()
{
    if(! mCtxMenuCurrentFrameURL.Length())
        return;

    // Try to get the file name part from the URL
    // To do that we first construct an obj which supports
    // nsIRUI interface. Makes it easy to extract portions
    // of a URL like the filename, scheme etc. + We'll also
    // use it while saving this link to a file
    nsresult rv   = NS_OK;
    nsCOMPtr<nsIURI> frameURI;
    rv = NewURI(getter_AddRefs(frameURI), mCtxMenuCurrentFrameURL);
    if(NS_FAILED(rv))
        return;
    URISaveAs(frameURI, TRUE);
}

void CBrowserView::OnViewPageInfo()
{
   if(! mWebNav)
      return;

   // Get the URI object we want to view from the cache.
   nsresult rv = NS_OK;
   nsCOMPtr<nsIURI> currentURI;
   rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
   if(NS_FAILED(rv) || !currentURI) 
      return;

   // Get the uri string associated with the nsIURI object
   nsEmbedCString uriString;
   rv = currentURI->GetSpec(uriString);
   if(NS_FAILED(rv))
      return;
 
   // Build the page info url
   nsEmbedCString viewPageInfoUrl;
   if (strstr((char*)uriString.get(),"https://") == NULL)
      viewPageInfoUrl.Append("about:cache-entry?client=HTTP&sb=1&key=");
   else
      viewPageInfoUrl.Append("about:cache-entry?client=HTTP-memory-only&sb=1&key=");
 
   viewPageInfoUrl.Append(uriString.get());
 
   OpenURLInNewWindow(viewPageInfoUrl.get());
}
 
void CBrowserView::OnViewFrameInfo()
{
   if(! mWebNav) 
      return;
 
   // Get the URI object we want to view from the cache.
   nsresult rv = NS_OK;
   nsCOMPtr<nsIURI> frameURI;
   rv = NewURI(getter_AddRefs(frameURI), mCtxMenuCurrentFrameURL);
   if(NS_FAILED(rv))
      return;
 
   // Get the uri string associated with the nsIURI object
   nsEmbedCString uriString;
   rv = frameURI->GetSpec(uriString);
   if(NS_FAILED(rv))
     return;

   // Build the page info url
   nsEmbedCString viewFrameInfoUrl;

   if (strstr((char*)uriString.get(),"https://") == NULL)
      viewFrameInfoUrl.Append("about:cache-entry?client=HTTP&sb=1&key=");
   else
      viewFrameInfoUrl.Append("about:cache-entry?client=HTTP-memory-only&sb=1&key=");

   viewFrameInfoUrl.Append(uriString.get());

   OpenURLInNewWindow(viewFrameInfoUrl.get());
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

	UTF16ToCString(mCtxMenuImgSrc, pszClipData);

    GlobalUnlock(hClipData);

    EmptyClipboard();
    SetClipboardData(CF_TEXT, hClipData);
    CloseClipboard();
}

void CBrowserView::OnCopyImageContent()
{
   if(! mCtxMenuImgSrc.Length())
      return;
	PRBool ok = TRUE;
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
	clipCmds->CanCopyImageContents(&ok);
	if (ok != TRUE)
	   return;
	clipCmds->CopyImageContents();
}

void CBrowserView::OnUpdateViewStatusBar(CCmdUI* pCmdUI)
{
    HWND hWndStatus = ::FindWindowEx(GetParent()->GetSafeHwnd(), NULL, STATUSCLASSNAME, NULL);
    pCmdUI->SetCheck(::IsWindowVisible(hWndStatus));
}


 
void CBrowserView::OnUpdateFilePrintPreview(CCmdUI* pCmdUI)
{
   pCmdUI->SetCheck(m_InPrintPreview);
}

BOOL CBrowserView::GetPrintSettings() {

   nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
   if (!print)
      return FALSE; 
   
   if (NS_FAILED(print->GetGlobalPrintSettings(getter_AddRefs(m_PrintSettings))))
      return FALSE;

   m_PrintSettings->SetMarginTop(GetFloatFromStr(theApp.preferences.printMarginTop));
   m_PrintSettings->SetMarginLeft(GetFloatFromStr(theApp.preferences.printMarginLeft));
   m_PrintSettings->SetMarginRight(GetFloatFromStr(theApp.preferences.printMarginRight));
   m_PrintSettings->SetMarginBottom(GetFloatFromStr(theApp.preferences.printMarginBottom));

   m_PrintSettings->SetShrinkToFit(theApp.preferences.printShrinkToFit);
   m_PrintSettings->SetScaling(double(theApp.preferences.printScaling) / 100.0);
   m_PrintSettings->SetPrintBGColors(theApp.preferences.printBGColors);
   m_PrintSettings->SetPrintBGImages(theApp.preferences.printBGImages);
      
   m_PrintSettings->SetPaperSizeType(theApp.preferences.printUnit);

   m_PrintSettings->SetPaperWidth(GetFloatFromStr(theApp.preferences.printWidth));
   m_PrintSettings->SetPaperHeight(GetFloatFromStr(theApp.preferences.printHeight));
      

   PRUnichar* uStr;
   uStr = GetUnicodeFromCString(theApp.preferences.printHeaderLeft);
   m_PrintSettings->SetHeaderStrLeft(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
      
   uStr = GetUnicodeFromCString(theApp.preferences.printHeaderMiddle);
   m_PrintSettings->SetHeaderStrCenter(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
      
   uStr = GetUnicodeFromCString(theApp.preferences.printHeaderRight);
   m_PrintSettings->SetHeaderStrRight(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
   
   uStr = GetUnicodeFromCString(theApp.preferences.printFooterLeft);
   m_PrintSettings->SetFooterStrLeft(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
   
   uStr = GetUnicodeFromCString(theApp.preferences.printFooterMiddle);
   m_PrintSettings->SetFooterStrCenter(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
      
   uStr = GetUnicodeFromCString(theApp.preferences.printFooterRight);
   m_PrintSettings->SetFooterStrRight(uStr);
   if (uStr != nsnull) nsMemory::Free(uStr);
   
   
   return TRUE;
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
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIURI> currentURI;
    rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
    if(!NS_FAILED(rv) && currentURI) {
        POINT pt;
        ::GetCursorPos(&pt);
        
        HWND hWnd;
        hWnd = ::GetFocus();

        if (hWnd && ::IsChild(m_hWnd, hWnd)) {
            m_iGetNodeHack = 2;
            ::SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(pt.x, pt.y));
            if ( (maccel_key!=WM_MBUTTONDOWN && mCtxMenuImgSrc.Length()>0) || mCtxMenuLinkUrl.Length() > 0)
                ::PostMessage(mpBrowserFrame->m_hWnd, WM_COMMAND, (WPARAM)maccel_cmd, (LPARAM)0);
            else if (!m_panning && maccel_key==WM_MBUTTONDOWN) {
                maccel_pan=1;
                StartPanning();
            }
        }

        maccel_cmd = 0;
    }
}

void CBrowserView::OnAppAbout()
{
   OpenURLInNewWindow(ABOUT_KMELEON);
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



void CBrowserView::OnIncreaseFont() {
   ChangeTextSize(1);
}

void CBrowserView::OnDecreaseFont() {
   ChangeTextSize(-1);
}

void CBrowserView::ChangeTextSize(PRInt32 change)
{
   nsresult rv;
   class nsIDOMWindow *domWindow;
   rv = mWebBrowser->GetContentDOMWindow(&domWindow);
   if (NS_FAILED(rv)) return;

   float textzoom;
   domWindow->GetTextZoom(&textzoom);
   textzoom += ((float)change) / 10;
   if (textzoom > 0 && textzoom <= 4) {
      domWindow->SetTextZoom(textzoom);

	  CString status;
	  status.Format(IDS_TEXT_ZOOM, textzoom*10);
      mpBrowserFrame->m_wndStatusBar.SetPaneText(0, status);
   }
}

int CBrowserView::GetTextSize()
{
   nsresult rv;
   class nsIDOMWindow *domWindow;
   rv = mWebBrowser->GetContentDOMWindow(&domWindow);
   NS_ENSURE_SUCCESS(rv, -1);
   
   float textzoom;
   rv = domWindow->GetTextZoom(&textzoom);
   return NS_SUCCEEDED(rv) ? (int)(textzoom*10.0) : -1;
}

void CBrowserView::OnToggleOffline()
{
    theApp.BroadcastMessage(WM_COMMAND, ID_NAV_STOP, (LPARAM) 0);

    theApp.SetOffline(!theApp.preferences.bOffline);
    theApp.menus.SetCheck(ID_OFFLINE, theApp.preferences.bOffline);

	CString status;
	status.LoadString( theApp.preferences.bOffline ? IDS_OFFLINE : IDS_ONLINE );

    mpBrowserFrame->m_wndStatusBar.SetPaneText(0, status); 
}
