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
 *   Conrad Carlen <ccarlen@netscape.com> 
 */

// File Overview....
//
// The typical MFC app, frame creation code
//
// NS_InitEmbedding() is called in InitInstance()
// 
// NS_TermEmbedding() is called in ExitInstance()
// ExitInstance() also takes care of cleaning up of
// multiple browser frame windows on app exit
//
// NS_DoIdleEmbeddingStuff(); is called in the overridden
// OnIdle() method
//
// Code to handle the creation of a new browser window

// Next suggested file to look at : BrowserFrm.cpp

// Local Includes
#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserFrm.h"
#include "winEmbedFileLocProvider.h"
#include "ProfileMgr.h"
#include "BrowserImpl.h"
#include "nsIWindowWatcher.h"
#include "kmeleonConst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MENU_CONFIG_FILE "menus.cfg"
#define ACCEL_CONFIG_FILE "accel.cfg"

// this is for overriding the Mozilla default PromptService component
#include "components\PromptService.h"
#define kComponentsLibname "kmeleonComponents.dll"
#define NS_PROMPTSERVICE_CID \
   {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

BEGIN_MESSAGE_MAP(CMfcEmbedApp, CWinApp)
	//{{AFX_MSG_MAP(CMfcEmbedApp)
	ON_COMMAND(ID_NEW_BROWSER, OnNewBrowser)
	ON_COMMAND(ID_MANAGE_PROFILES, OnManageProfiles)
   ON_COMMAND(ID_PREFERENCES, OnPreferences)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMfcEmbedApp::CMfcEmbedApp()
{
    m_ProfileMgr = NULL;

   
   mRefCnt = 1; // Start at one - nothing is going to addref this object
   m_pMostRecentBrowserFrame  = NULL;
   m_toolbarControlsMenu = NULL;

   m_sMainWindowClassName = _T("KMeleon");
}

CMfcEmbedApp theApp;


/* Some Gecko interfaces are implemented as components, automatically
   registered at application initialization. nsIPrompt is an example:
   the default implementation uses XUL, not native windows. Embedding
   apps can override the default implementation by implementing the
   nsIPromptService interface and registering a factory for it with
   the same CID and Contract ID as the default's.

   Note that this example implements the service in a separate DLL,
   replacing the default if the override DLL is present. This could
   also have been done in the same module, without a separate DLL.
   See the PowerPlant example for, well, an example.
*/
nsresult CMfcEmbedApp::OverrideComponents()
{
   nsresult rv = NS_OK;

   // replace Mozilla's default PromptService with our own, if the
   // expected override DLL is present
   HMODULE overlib = ::LoadLibrary(kComponentsLibname);
   if (overlib) {
      InitPromptServiceType InitLib;
      MakeFactoryType MakeFactory;
      InitLib = reinterpret_cast<InitPromptServiceType>(::GetProcAddress(overlib, kPromptServiceInitFuncName));
      MakeFactory = reinterpret_cast<MakeFactoryType>(::GetProcAddress(overlib, kPromptServiceFactoryFuncName));

      if (InitLib && MakeFactory) {
         InitLib(overlib);

         nsCOMPtr<nsIFactory> promptFactory;
         rv = MakeFactory(getter_AddRefs(promptFactory));
         if (NS_SUCCEEDED(rv))
            nsComponentManager::RegisterFactory(kPromptServiceCID,
                                                "Prompt Service",
                                                "@mozilla.org/embedcomp/prompt-service;1",
                                                promptFactory,
                                                PR_TRUE); // replace existing
      } else
       ::FreeLibrary(overlib);
   }
   return rv;
}

/*
void CMfcEmbedApp::ParseCmdLine()
{
}

BOOL CMfcEmbedApp::IsCmdLineSwitch(const char *pSwitch, BOOL bRemove)
{
    //  Search for the switch in the command line.
    //  Don't take it out of m_lpCmdLine by default
    char *pFound = strstr(m_lpCmdLine, pSwitch);
    if(pFound == NULL ||
        // Switch must be at beginning of command line
        // or have a space in front of it to avoid
        // mangling filenames
        ( (pFound != m_lpCmdLine) &&
          *(pFound-1) != ' ' ) ) 
    {
        return(FALSE);
    }

    if (bRemove) 
    {
        // remove the flag from the command line
        char *pTravEnd = pFound + strlen(pSwitch);
        char *pTraverse = pFound;

        *pTraverse = *pTravEnd;
        while(*pTraverse != '\0')   
        {
            pTraverse++;
            pTravEnd++;
            *pTraverse = *pTravEnd;
        }
    }

    return(TRUE);
}

*/


LPCTSTR CMfcEmbedApp::GetMainWindowClassName() {
	return   m_sMainWindowClassName;
}

// Initialize our MFC application and also init
// the Gecko embedding APIs
// Note that we're also init'ng the profile switching
// code here
// Then, create a new BrowserFrame and load our
// default homepage
//
BOOL CMfcEmbedApp::InitInstance()
{
   
   // Parse command line
//	ParseCmdLine();

   // check for prior instances
   HANDLE hMutexOneInstance = CreateMutex( NULL, TRUE, "K-Meleon Instance Mutex" );
	m_bAlreadyRunning = ( GetLastError() == ERROR_ALREADY_EXISTS );

   if ( hMutexOneInstance )
		ReleaseMutex( hMutexOneInstance );


   // if another instance is already running, pass it our command line paramaters,
   // and ask it to open a new window
   // eventually, we should handle this through DDE
   if (m_bAlreadyRunning) {
      // find the hidden window
      if (HWND hwndPrev = FindWindowEx(NULL, NULL, "KMeleon", NULL) ) {
         if(*m_lpCmdLine) {
            COPYDATASTRUCT copyData;
            copyData.cbData = strlen(m_lpCmdLine);
            copyData.lpData = (void *) m_lpCmdLine;
            SendMessage(hwndPrev, WM_COPYDATA, NULL, (LPARAM) &copyData);
         }
         else
            SendMessage(hwndPrev, UWM_NEWWINDOW, NULL, NULL);
      }
      return FALSE;
   }

	Enable3dControls();   
   
   // Take a look at 
	// http://www.mozilla.org/projects/xpcom/file_locations.html
	// for more info on File Locations

   winEmbedFileLocProvider *provider = new winEmbedFileLocProvider("KMeleon");
   if(!provider){
      ASSERT(FALSE);
      return FALSE;
   }

   nsresult rv;
   rv = NS_InitEmbedding(nsnull, provider);
   if(NS_FAILED(rv)){
      ASSERT(FALSE);
      return FALSE;
   }

   rv = OverrideComponents();
   if(NS_FAILED(rv)) {                                                                           
      ASSERT(FALSE);
      return FALSE;
   }

   rv = InitializeWindowCreator();
   if (NS_FAILED(rv)) {
      ASSERT(FALSE);
      return FALSE;
   }

   if(!InitializeProfiles()){
      ASSERT(FALSE);
      NS_TermEmbedding();
      return FALSE;
   }

   if(!CreateHiddenWindow()){
      ASSERT(FALSE);
      NS_TermEmbedding();
      return FALSE;
   }

   plugins.FindAndLoad("*.dll");

   // Check if the tray control is running...if it is, we'll
   // ask if we should stay resident between sessions
   HWND hwndLoader = FindWindowEx(NULL, NULL, "KMeleon Tray Control", NULL);
   if (hwndLoader)
      ((CHiddenWnd*) m_pMainWnd)->GetPersist();

   // see if we're staying resident
   if (((CHiddenWnd*) m_pMainWnd)->m_bStayResident) {
      if ( ((CHiddenWnd*) m_pMainWnd)->StayResident() ) {
         if (((CHiddenWnd*) m_pMainWnd)->m_bPreloadWindow)
            m_pMostRecentBrowserFrame->m_wndReBar.DrawToolBarMenu();
      }
      else
         return FALSE;
   }

   // otherwise we should create a browser and load the start page
   else {
      // Create the first browser frame window, hidden if the -persist flag is set
	   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();
      if(!pBrowserFrame)
         return FALSE;
      pBrowserFrame->m_wndReBar.DrawToolBarMenu();
      
      if ( *m_lpCmdLine )
         pBrowserFrame->m_wndBrowserView.OpenURL(m_lpCmdLine);
      else
         pBrowserFrame->m_wndBrowserView.LoadHomePage();
   }

   return TRUE;
}

CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
												   PRInt32 x, PRInt32 y,
												   PRInt32 cx, PRInt32 cy,
												   PRBool bShowWindow)
{
	// Load the window title from the string resource table
	CString strTitle;
	strTitle.LoadString(IDR_MAINFRAME);

   // save the current band sizes before creating new window
   // this way the new window will "inherit" the current settings
   if (m_pMostRecentBrowserFrame)
      m_pMostRecentBrowserFrame->m_wndReBar.SaveBandSizes();
   
   // Now, create the browser frame
	CBrowserFrame* pFrame = new CBrowserFrame(chromeMask);

   // restore the saved window size
   RECT winSize;
   winSize.top    = CW_USEDEFAULT;
   winSize.left   = CW_USEDEFAULT;
   if (preferences.width != -1 && preferences.height != -1) {
      // the Create function has been modified to take width, and height values
      // instead of right and bottom window placement variables
      winSize.right  = preferences.width;
      winSize.bottom = preferences.height;
   }
   else {
      winSize.right  = CW_USEDEFAULT;
      winSize.bottom = CW_USEDEFAULT;
   }

   LONG style = WS_OVERLAPPEDWINDOW;
   if (preferences.bMaximized) style |= WS_MAXIMIZE;

   // this calls our modified create function, the winSize rect uses CreateWindowEx style x, y, cx, cy
   // rather than the MFC style left, top, right, bottom
   if (!pFrame->Create(NULL, strTitle, style, winSize, NULL, NULL, 0L, NULL))
		return NULL;

   pFrame->SetIcon(LoadIcon(IDR_MAINFRAME), true);
   pFrame->SetIcon(LoadIcon(IDR_MAINFRAME), false);

	// load accelerator resource
   //pFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));
   pFrame->m_hAccelTable = accel.GetTable();

	// Show the window...
	if(bShowWindow) {
      if (preferences.bMaximized) pFrame->ShowWindow(SW_MAXIMIZE);
      else pFrame->ShowWindow(SW_SHOW);
		pFrame->UpdateWindow();
	}

	// Add to the list of BrowserFrame windows
	m_FrameWndLst.AddHead(pFrame);
   
   pFrame->m_created = true;

   return pFrame;
}

void CMfcEmbedApp::OnNewBrowser()
{
   char *sURI = NULL;
   if (preferences.iNewWindowOpenAs == PREF_NEW_WINDOW_CURRENT) {
      // check that the current window has a URI loaded
      int len = m_pMostRecentBrowserFrame->m_wndBrowserView.GetCurrentURI(NULL);
      if (len > 0) {
         sURI = new char[len+1];
         m_pMostRecentBrowserFrame->m_wndBrowserView.GetCurrentURI(sURI);
         // save the frame that called us, in case it calls again
         // before the newly created frame has had time to load the URI
         m_pOpenNewBrowserFrame = m_pMostRecentBrowserFrame;
      }
      else if (m_pOpenNewBrowserFrame) {
         len = m_pOpenNewBrowserFrame->m_wndBrowserView.GetCurrentURI(NULL);
         if (len > 0) {
            sURI = new char[len+1];
            m_pOpenNewBrowserFrame->m_wndBrowserView.GetCurrentURI(sURI);
         }
      }
   }

   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();

   if(pBrowserFrame) {
	   //Load the new window start page into the browser view
      switch (preferences.iNewWindowOpenAs) {
      case PREF_NEW_WINDOW_CURRENT:
         if (sURI) {
            pBrowserFrame->m_wndBrowserView.OpenURL(sURI);
            delete sURI;
         }
         break;
      case PREF_NEW_WINDOW_HOME:
   	   pBrowserFrame->m_wndBrowserView.LoadHomePage();
         break;
      case PREF_NEW_WINDOW_URL:
         pBrowserFrame->m_wndBrowserView.OpenURL(preferences.newWindowURL);
         break;
      }
      pBrowserFrame->m_setURLBarFocus = true;
   }
}

// This gets called anytime a BrowserFrameWindow is
// closed i.e. by choosing the "close" menu item from
// a window's system menu or by dbl clicking on the
// system menu box
// 
// Sends a WM_QUIT to the hidden window which results
// in ExitInstance() being called and the app is
// properly cleaned up and shutdown
//
void CMfcEmbedApp::RemoveFrameFromList(CBrowserFrame* pFrm, BOOL bCloseAppOnLastFrame/*= TRUE*/)
{
	POSITION pos = m_FrameWndLst.Find(pFrm);
   m_FrameWndLst.RemoveAt(pos);


   // Unless we are set to stay resident,
   // send a WM_QUIT msg. to the hidden window if we've
	// just closed the last browserframe window and
	// if the bCloseAppOnLastFrame is TRUE. This be FALSE
	// only in the case we're switching profiles
	// Without this the hidden window will stick around
	// i.e. the app will never die even after all the 
	// visible windows are gone.
   if(m_FrameWndLst.GetCount() == 0) {
      // if we're switching profiles, we don't need to do anything
      if (!bCloseAppOnLastFrame) {}
      // if we're staying resident, create the hidden browser window
      else if ( ((CHiddenWnd*) m_pMainWnd)->m_bStayResident ) {
         ((CHiddenWnd*) m_pMainWnd)->StayResident();
      }
      // otherwise we're exiting, close the Evil Hidden Window
      else
		   m_pMainWnd->PostMessage(WM_QUIT);
   }
}

int CMfcEmbedApp::ExitInstance()
{
	// When File/Exit is chosen and if the user
	// has opened multiple browser windows shut all
	// of them down properly before exiting the app


   // if we're exiting because an instance was already
   // running, then we haven't created anything that
   // needs to be destroyed
   if (m_bAlreadyRunning)
      return 1;
   
   CBrowserFrame* pBrowserFrame = NULL;

	POSITION pos = m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
		pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		if(pBrowserFrame)	{
			pBrowserFrame->ShowWindow(false);
			pBrowserFrame->DestroyWindow();
		}
	}
	m_FrameWndLst.RemoveAll();

   if (m_pMainWnd)
      m_pMainWnd->DestroyWindow();

   delete m_ProfileMgr;

   preferences.Save();

   NS_TermEmbedding();

	return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
	CWinApp::OnIdle(lCount);

	NS_DoIdleEmbeddingStuff();

	return FALSE;
}

void CMfcEmbedApp::OnPreferences () {
  CPreferencesDlg prefDlg;
  prefDlg.DoModal();
}

void CMfcEmbedApp::OnManageProfiles()
{
  m_ProfileMgr->DoManageProfilesDialog(PR_FALSE);
}

BOOL CMfcEmbedApp::InitializeProfiles() {
   m_ProfileMgr = new CProfileMgr;
   if (!m_ProfileMgr)
      return FALSE;

   nsresult rv;
   NS_WITH_SERVICE(nsIObserverService, observerService, NS_OBSERVERSERVICE_CONTRACTID, &rv);

   observerService->AddObserver(this, NS_LITERAL_STRING("profile-approve-change").get());
   observerService->AddObserver(this, NS_LITERAL_STRING("profile-change-teardown").get());
   observerService->AddObserver(this, NS_LITERAL_STRING("profile-after-change").get());

   m_ProfileMgr->StartUp();

   return TRUE;
}

// When the profile switch happens, all open browser windows need to be 
// closed. 
// In order for that not to kill off the app, we just make the MFC app's 
// mainframe be an invisible window which doesn't get closed on profile 
// switches
BOOL CMfcEmbedApp::CreateHiddenWindow()
{
   //	Register the main window class
   WNDCLASS wc = { 0 };
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc =  AfxWndProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = m_sMainWindowClassName;
	wc.hIcon = LoadIcon( IDR_MAINFRAME );
	AfxRegisterClass( &wc );

	CFrameWnd *hiddenWnd = new CHiddenWnd;
	if(!hiddenWnd)
      return FALSE;

   RECT bounds = { -10010, -10010, -10000, -10000 };
   hiddenWnd->Create(NULL, "K-Meleon hidden window", WS_DISABLED, bounds, NULL, NULL, 0, NULL);
   m_pMainWnd = hiddenWnd;

   return TRUE;
}

nsresult CMfcEmbedApp::InitializePrefs(){
   preferences.Load();
   preferences.Save();

  if (!menus.Load(preferences.settingsDir + MENU_CONFIG_FILE)){
    // we used to create the file if it didn't exist
    // but now it should be copied automagically from defaults when the profile is created
    MessageBox(NULL, "Could not find " MENU_CONFIG_FILE, NULL, 0);
  }
  if (!accel.Load(preferences.settingsDir + ACCEL_CONFIG_FILE)){
    MessageBox(NULL, "Could not find " ACCEL_CONFIG_FILE, NULL, 0);
  }

  return TRUE;
}

/* InitializeWindowCreator creates and hands off an object with a callback
   to a window creation function. This will be used by Gecko C++ code
   (never JS) to create new windows when no previous window is handy
   to begin with. This is done in a few exceptional cases, like PSM code.
   Failure to set this callback will only disable the ability to create
   new windows under these circumstances.
*/
nsresult CMfcEmbedApp::InitializeWindowCreator() {
   // give an nsIWindowCreator to the WindowWatcher service
   nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, this));
   if (windowCreator) {
      nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
      if (wwatch) {
         wwatch->SetWindowCreator(windowCreator);
         return NS_OK;
      }
   }
   return NS_ERROR_FAILURE;
}
// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsISupports
// ---------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(CMfcEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference);

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsIObserver
// ---------------------------------------------------------------------------

// Mainly needed to support "on the fly" profile switching

NS_IMETHODIMP CMfcEmbedApp::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    
    if (nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-approve-change").get()) == 0)
    {
        // Ask the user if they want to
        int result = MessageBox(NULL, "Do you want to close all windows in order to switch the profile?", "Confirm", MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->VetoChange();
        }
    }
    else if (nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-change-teardown").get()) == 0)
    {
        // Close all open windows. Alternatively, we could just call CBrowserWindow::Stop()
        // on each. Either way, we have to stop all network activity on this phase.
        
	    POSITION pos = m_FrameWndLst.GetHeadPosition();
	    while( pos != NULL )
	    {
		    CBrowserFrame *pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		    if(pBrowserFrame)
		    {
			    pBrowserFrame->ShowWindow(false);

				// Passing in FALSE below so that we do not
				// kill the main app during a profile switch
				RemoveFrameFromList(pBrowserFrame, FALSE);

				pBrowserFrame->DestroyWindow();
		    }
       }
    }
    else if (nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-after-change").get()) == 0)
    {
        plugins.UnLoadAll();
        InitializePrefs(); // In case we have just switched to a newly created profile.
        
         // Only make a new browser window on a switch. This also gets
         // called at start up and we already make a window then.
         if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("switch").get()))      
            OnNewBrowser();
    }

    return rv;
}

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsIWindowCreator
// ---------------------------------------------------------------------------
NS_IMETHODIMP CMfcEmbedApp::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                               PRUint32 chromeFlags,
                                               nsIWebBrowserChrome **_retval)
{
   // XXX we're ignoring the "parent" parameter
   NS_ENSURE_ARG_POINTER(_retval);
   *_retval = 0;

   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags);
   if(pBrowserFrame) {
      *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *, pBrowserFrame->GetBrowserImpl());
      NS_ADDREF(*_retval);
   }
   return NS_OK;

}


BEGIN_MESSAGE_MAP(CHiddenWnd, CFrameWnd)
	//{{AFX_MSG_MAP(CHiddenWnd)
   ON_WM_CREATE()
   ON_WM_CLOSE()
   ON_WM_COPYDATA()
	ON_MESSAGE(UWM_NEWWINDOW, OnNewWindow)
   ON_MESSAGE(UWM_PERSIST_SET, OnSetPersist)
   ON_MESSAGE(UWM_PERSIST_SHOW, ShowBrowser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




BOOL CHiddenWnd::PreCreateWindow(CREATESTRUCT& cs) {

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
   cs.lpszClass = theApp.GetMainWindowClassName();

   return TRUE;
}

void CHiddenWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
   m_bPersisting = FALSE;
   m_bStayResident = FALSE;
   m_bPreloadWindow = FALSE;
   m_bPreloadStartPage = FALSE;

   CFrameWnd::OnCreate(lpCreateStruct);
}

void CHiddenWnd::OnClose() {
   if (!m_bStayResident)
      CFrameWnd::OnClose();

   // the user has selected File|Exit, close all the browser windows
   else {
      CBrowserFrame* pBrowserFrame = NULL;

	   POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
      while( pos != NULL ) {
		   pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
		   if(pBrowserFrame)	{
			   pBrowserFrame->ShowWindow(false);
   			pBrowserFrame->DestroyWindow();
	   	}
	   }
	   theApp.m_FrameWndLst.RemoveAll();
      StayResident();
   }
}

void CHiddenWnd::OnSetPersist(DWORD flags) {
   BOOL bNewStayResident = (flags & PERSIST_BROWSER);
   BOOL bNewPreloadWindow = (flags & PERSIST_WINDOW);
   BOOL bNewPreloadStartPage =(flags & PERSIST_STARTPAGE);

   // a little sanity checking
   if (bNewPreloadStartPage && ! bNewPreloadWindow)
      bNewPreloadStartPage = FALSE;

   // update the hidden window with the new settings
   if (m_bPersisting) {

      if (!bNewStayResident)
         PostMessage(WM_QUIT);

      // preload the window
      if (bNewPreloadWindow && !m_bPreloadWindow) {
         m_pHiddenBrowser = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL,
                                                     -1, -1, -1, -1, PR_FALSE);
         if (bNewPreloadStartPage)
            m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
         else
            m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");
      }

      // don't preload the window
      if (!bNewPreloadWindow && m_bPreloadWindow) {
         m_pHiddenBrowser->DestroyWindow();
         POSITION pos = theApp.m_FrameWndLst.Find(m_pHiddenBrowser);
         theApp.m_FrameWndLst.RemoveAt(pos);
      }

      // preload the start page
      if (bNewPreloadStartPage && !m_bPreloadStartPage)
         m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();

      // don't preload the start page
      if (bNewPreloadWindow && !bNewPreloadStartPage && m_bPreloadStartPage)
         m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");
   }

   m_bStayResident = bNewStayResident;
   m_bPreloadWindow = bNewPreloadWindow;
   m_bPreloadStartPage = bNewPreloadStartPage;
}

void CHiddenWnd::GetPersist() {
   HWND hwndLoader = FindWindowEx(NULL, NULL, "KMeleon Tray Control", NULL);
   if (hwndLoader) {
      LRESULT flags = ::SendMessage(hwndLoader, UWM_PERSIST_GET, NULL, NULL);

      m_bStayResident = ((flags & PERSIST_BROWSER));
      m_bPreloadWindow = ((flags & PERSIST_WINDOW));
      m_bPreloadStartPage = ((flags & PERSIST_STARTPAGE));
      m_bShowNow =  ((flags & PERSIST_SHOWNOW));

      // a little sanity checking
      if (m_bPreloadStartPage && ! m_bPreloadWindow)
         m_bPreloadStartPage = FALSE;
   
   }
   else {
      m_bStayResident = FALSE;
      m_bPreloadWindow = FALSE;
      m_bPreloadStartPage = FALSE;
      m_bShowNow = FALSE;
   }
}

BOOL CHiddenWnd::StayResident() {

   // if the ShowNow flag is set, we're not really going to stay resident
   if (m_bShowNow) {
      m_bShowNow = FALSE;
      m_bPersisting = FALSE;

      m_pHiddenBrowser = theApp.CreateNewBrowserFrame();
      if (!m_pHiddenBrowser)
         return FALSE;
      m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
   }

   else {
      m_bPersisting = TRUE;

      if (m_bPreloadWindow) {
         m_pHiddenBrowser = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL,
                                                     -1, -1, -1, -1, PR_FALSE);
         if (!m_pHiddenBrowser)
            return FALSE;

         if (m_bPreloadStartPage)
            m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
         else
            m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");
      }
   }

   return TRUE;
}

void CHiddenWnd::ShowBrowser(char *URI) {

   if (m_bPreloadStartPage)
      ::MessageBox(NULL, "Start", NULL, MB_OK);
   else if (m_bPreloadWindow)
      ::MessageBox(NULL, "Window", NULL, MB_OK);
   else if (m_bStayResident)
      ::MessageBox(NULL, "Browser", NULL, MB_OK);


   // if we already have a browser, load home page (if necessary), and show the window
   if (m_bPersisting && m_bPreloadWindow) {
      if (URI)
         m_pHiddenBrowser->m_wndBrowserView.OpenURL(URI);
      else if (!m_bPreloadStartPage)
         m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();

      if (theApp.preferences.bMaximized) m_pHiddenBrowser->ShowWindow(SW_MAXIMIZE);
      else m_pHiddenBrowser->ShowWindow(SW_SHOW);
      m_pHiddenBrowser->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

      m_bPersisting = FALSE;
   }

   // otherwise, just create a new browser
   else {
      CBrowserFrame* browser;
      browser = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, PR_TRUE);
      if (URI)
         browser->m_wndBrowserView.OpenURL(URI);
      else
         browser->m_wndBrowserView.LoadHomePage();
   }
}

// This is called from another instance of Kmeleon (via the UWM_NEWWINDOW message),
// when no command line paramaters have been specified
void CHiddenWnd::OnNewWindow() {
   ShowBrowser();
}

// This is called from another instance of Kmeleon,
// and contains any command line parameters specified
void CHiddenWnd::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) {
   ShowBrowser((char *) pCopyDataStruct->lpData);
}
