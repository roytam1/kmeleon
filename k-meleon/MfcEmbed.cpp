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
#include "HiddenWnd.h"
#include "BrowserFrm.h"
#include "winEmbedFileLocProvider.h"
#include "ProfileMgr.h"
#include "BrowserImpl.h"
#include "nsIWindowWatcher.h"
#include "kmeleonConst.h"
#include "UnknownContentTypeHandler.h"
#include "Utils.h"

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
   m_bSwitchingProfiles = FALSE;
   m_bFirstWindowCreated = FALSE;
   
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

   // Override the UnknownContentHandler
   nsCOMPtr<nsIFactory> unkFactory;
   rv = NewUnknownContentHandlerFactory(getter_AddRefs(unkFactory));
   if (NS_FAILED(rv)) return rv;
   rv = nsComponentManager::RegisterFactory(kUnknownContentTypeHandlerCID,
	   NS_IUNKNOWNCONTENTTYPEHANDLER_CLASSNAME,
	   NS_IUNKNOWNCONTENTTYPEHANDLER_CONTRACTID ,
	   unkFactory,
	   PR_TRUE); // replace existing

   rv = nsComponentManager::RegisterFactory(kUnknownContentTypeHandlerCID,
	   NS_IHELPERAPPLAUNCHERDLG_CLASSNAME,
	   NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
	   unkFactory,
	   PR_TRUE); // replace existing

/*
   // Override the nsIHelperAppLauncherDialog
   nsCOMPtr<nsIFactory> helpFactory;
   rv = NewUnknownContentHandlerFactory(getter_AddRefs(helpFactory));
   if (NS_FAILED(rv)) return rv;
   rv = nsComponentManager::RegisterFactory(kHelperAppLauncherDialogCID,
	   NS_IHELPERAPPLAUNCHERDLG_CLASSNAME,
	   NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
	   helpFactory,
	   PR_TRUE); // replace existing
*/
   
   return rv;
}

// search for a switch in the command line
// return -1 if the switch is not found
// for separation purposes, we make the assumption that flags begin with a dash "-",
// but the "-" is  still required in the pSwitch parameter
// otherwise return the number of characters in the argument after the flag
// if pArgs is a valid pointer, copy the argument data into it
// if bRemove is set, the flag (and it's argument, if pArgs is valid) will be removed
int CMfcEmbedApp::GetCommandLineSwitch(const char *pSwitch, char *pArgs, BOOL bRemove)
{
   char *p, *c;
   char *pQuote;
   char *pSwitchPos;
   char *pCmdLine;
   char *pArgStart, *pArgEnd;
   int iQuoteCount;
   BOOL bIsValidSwitch;
   int iSwitchLen, iArgLen=0;

   if ( !pSwitch || !*pSwitch )
      return -1;
   else
      iSwitchLen = strlen(pSwitch);

   p = pCmdLine = SkipWhiteSpace(m_lpCmdLine);
   do {
      bIsValidSwitch = FALSE;
      
      if ( ((!p) || (!*p)) || !(pSwitchPos = strstr(p, pSwitch)) ) {
         if (pArgs) *pArgs = 0;
         return -1;
      }
      p = SkipWhiteSpace(pSwitchPos + iSwitchLen);

      // if this happens to be the first character on the command line,
      // then we don't need to do any error checking
      if (pSwitchPos == pCmdLine)
         bIsValidSwitch = TRUE;

      // make sure the flag is preceeded by whitespace
      else if ( (*(pSwitchPos-1) != ' ') && (*(pSwitchPos-1) != '\t') )
         continue;

      // make sure the character after the flag is whitespace or null
      else if ( (*(pSwitchPos+iSwitchLen) != ' ') && (*(pSwitchPos+iSwitchLen) != '\t') && (*(pSwitchPos+iSwitchLen) != 0)  )
         continue;

      // if the "argument" starts with a dash, it's another switch, and this
      // switch has no argument
      else if (*p == '-') {
         *pArgs = 0;
         return 0;
      }

      // make sure the switch we've found isn't inside quotation marks
      else {
         c = pCmdLine;
         iQuoteCount = 0;
         do {
            pQuote = strchr(c, '\"');
            if (pQuote) {
               if ( !( *(pQuote-1) == '\\') )
                  iQuoteCount++;
               c = pQuote+1;
            }
         } while ( pQuote && (pQuote < pSwitchPos) );      

         // there are 3 cases when the switch found will be valid
         // 1) if there are the no quotes found before the switch
         // 2) if there an odd number of quotes found, and the last quote is found after the switch
         // 3) if there are an even number of quotes found, and no quotes after the switch

         if (  (iQuoteCount == 0) || \
              ((pQuote) && (iQuoteCount%2)) || \
            ((!pQuote) && (!(iQuoteCount%2))) )
            bIsValidSwitch = TRUE;
      }
   } while (pSwitchPos && !bIsValidSwitch);

   if (pSwitchPos && bIsValidSwitch) {
      pArgStart = SkipWhiteSpace(p);
      
      // check if the argument is inside quotes
      if (*pArgStart == '\"') {
         pArgStart++;
         pArgEnd = strchr(pArgStart, '\"');
      }

      // find the first whitespace
      else {
         char *pTab   = strchr(pArgStart, '\t');
         char *pSpace = strchr(pArgStart, ' ');

         if (pTab && pSpace)
            pArgEnd = ( pTab > pSpace ? pSpace : pTab );
         else
            pArgEnd = ( pTab ? pTab : pSpace );
      }

      if (pArgEnd)
         iArgLen = pArgEnd-pArgStart;
      else
         iArgLen = strlen(pArgStart);

      if (pArgs) {
         if (iArgLen) {
            memcpy(pArgs, pArgStart, iArgLen);
            pArgs[iArgLen] = 0;
         }
         else
            *pArgs = 0;
      }
      
      if (bRemove) {
         char *pNewData;
         if (pArgs)
            pNewData = pArgEnd ? pArgEnd+1 : pArgStart + iArgLen;
         else
            pNewData = pSwitchPos + iSwitchLen;

         pNewData = SkipWhiteSpace(pNewData);
         while (*pNewData)
            *pSwitchPos++ = *pNewData++;
         *pSwitchPos = 0;
      }

   }

   return iArgLen;
}



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
   int len = GetCommandLineSwitch("-f", NULL, FALSE);
   if (len == 0) {
      MessageBox(NULL, m_lpCmdLine, NULL, MB_OK);
   }
   else if (len > 0) {
      char *arg = new char[len+1];
      GetCommandLineSwitch("-f", arg, TRUE);
      MessageBox(NULL, m_lpCmdLine, NULL, MB_OK);
      delete arg;
   }

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
      if (HWND hwndPrev = FindWindowEx(NULL, NULL, GetMainWindowClassName(), NULL) ) {
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

   InitializePrefs();
   plugins.FindAndLoad();

   // the hidden window will take care of creating the first
   // browser window for us
   if(!CreateHiddenWindow()){
      ASSERT(FALSE);
      NS_TermEmbedding();
      return FALSE;
   }

   return TRUE;
}

// add new download dialags to the internal window list so we won't exit
// the app while downloads are in progress
void CMfcEmbedApp::RegisterWindow(CDialog *window) {
   m_MiscWndLst.AddHead(window);
}

void CMfcEmbedApp::UnregisterWindow(CDialog *window) {
	POSITION pos = m_MiscWndLst.Find(window);
   m_MiscWndLst.RemoveAt(pos);


   // Unless we are set to stay resident,
   // send a WM_QUIT msg. to the hidden window if we've
	// just closed the last browserframe window and
	// if the bCloseAppOnLastFrame is TRUE. This be FALSE
	// only in the case we're switching profiles
	// Without this the hidden window will stick around
	// i.e. the app will never die even after all the 
	// visible windows are gone.
   if ((m_MiscWndLst.GetCount() == 0) && (m_FrameWndLst.GetCount() == 0)) {

      if (m_pMainWnd) {
         // if we're staying resident, create the hidden browser window
         if (((CHiddenWnd*) m_pMainWnd)->Persisting() == PERSIST_STATE_ENABLED)
            ((CHiddenWnd*) m_pMainWnd)->StayResident();

         // otherwise we're exiting, close the Evil Hidden Window
         else {
            m_pMainWnd->PostMessage(WM_QUIT);
            delete (CHiddenWnd *) m_pMainWnd;
            m_pMainWnd = NULL;
         }
      }
   }
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

   // this backup is made as part of a bad workaround:
   // m_pMostRecentBrowserFrame needs to be this frame for the life of this function so that
   // things like plugins and the rebar sizes can access it, but
   // m_pMostRecentBrowserFrame should not stay set to this if this window
   // is hidden (usually meaning that it is created using the open link in background option)
   // so the backup is made, and m_pMostRecentBrowserFrame will be restored
   // at the end of this function
   CBrowserFrame* pOldRecentFrame = m_pMostRecentBrowserFrame;   
   theApp.m_pMostRecentBrowserFrame = pFrame;

   if (!pOldRecentFrame)
      pOldRecentFrame = pFrame;
   
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

   if (preferences.bMaximized && (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
      style |= WS_MAXIMIZE;

   // this calls our modified create function, the winSize rect uses CreateWindowEx style x, y, cx, cy
   // rather than the MFC style left, top, right, bottom
   if (!pFrame->Create(NULL, strTitle, style, winSize, NULL, NULL, 0L, NULL))
		return NULL;

   pFrame->SetIcon(LoadIcon(IDR_MAINFRAME), true);
   pFrame->SetIcon(LoadIcon(IDR_MAINFRAME), false);

	// load accelerator resource
   //pFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));
   pFrame->m_hAccelTable = accel.GetTable();

   // this only needs to be called once
   if (!m_bFirstWindowCreated) {
      pFrame->m_wndReBar.DrawToolBarMenu();
      m_bFirstWindowCreated = TRUE;
   }

   // Show the window...
	if(bShowWindow) {
      if (preferences.bMaximized) pFrame->ShowWindow(SW_MAXIMIZE);
      else pFrame->ShowWindow(SW_SHOW);
		pFrame->UpdateWindow();
	}

	// Add to the list of BrowserFrame windows
	m_FrameWndLst.AddHead(pFrame);
   
   pFrame->m_created = true;

   theApp.m_pMostRecentBrowserFrame = pOldRecentFrame;

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
      pBrowserFrame->SetFocus();
      pBrowserFrame->m_wndUrlBar.MaintainFocus();
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
void CMfcEmbedApp::RemoveFrameFromList(CBrowserFrame* pFrm)
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
   if ((m_FrameWndLst.GetCount() == 0) && (m_MiscWndLst.GetCount() == 0)) {

      // if we're switching profiles, we don't need to do anything
      if (m_bSwitchingProfiles) {}

      else if (m_pMainWnd) {

         // if we're staying resident, create the hidden browser window
         if (((CHiddenWnd*) m_pMainWnd)->Persisting() == PERSIST_STATE_ENABLED)
            ((CHiddenWnd*) m_pMainWnd)->StayResident();

         // otherwise we're exiting, close the Evil Hidden Window
         else {
            m_pMainWnd->PostMessage(WM_QUIT);
            delete (CHiddenWnd *) m_pMainWnd;
            m_pMainWnd = NULL;
         }
      }
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

   POSITION pos = m_MiscWndLst.GetHeadPosition();
   while( pos != NULL )
   {
      CProgressDialog *pDlg = (CProgressDialog *) m_MiscWndLst.GetNext(pos);
      if (pDlg)
         pDlg->Cancel();
   }

   pos = m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
		pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		if(pBrowserFrame)
         pBrowserFrame->SendMessage(WM_CLOSE);
	}
	m_FrameWndLst.RemoveAll();

   if (m_pMainWnd) {
      m_pMainWnd->SendMessage(WM_CLOSE);
      delete (CHiddenWnd *) m_pMainWnd;
      m_pMainWnd = NULL;
   }

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
   nsCOMPtr<nsIObserverService> observerService = 
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

   observerService->AddObserver(this, NS_LITERAL_STRING("profile-approve-change").get());
   observerService->AddObserver(this, NS_LITERAL_STRING("profile-change-teardown").get());
   observerService->AddObserver(this, NS_LITERAL_STRING("profile-after-change").get());


   int len = GetCommandLineSwitch("-P", NULL, FALSE);

   // there are no parameters, load the most recent profile
   if (len == 0) {
      GetCommandLineSwitch("-P", NULL, TRUE);  // remove the flag from the command line
   }
   // try loading the profile specified
   else if (len > 0) {
      char *profile = new char[len+1];
      GetCommandLineSwitch("-P", profile, TRUE);

      BOOL bResult;
      if (!stricmp("mostrecent", profile))
         bResult = m_ProfileMgr->SetMostRecentProfile();
      else
         bResult = m_ProfileMgr->SetCurrentProfile(profile);
      delete profile;
      if (bResult)
         return TRUE;
   }

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
      int result = MessageBox(NULL, "Do you want to close all windows in order to switch the profile?\nThis will cancel any files being downloaded.", "Confirm", MB_YESNO | MB_ICONQUESTION);
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

      POSITION pos = m_MiscWndLst.GetHeadPosition();
      while( pos != NULL )
      {
         CProgressDialog *pDlg = (CProgressDialog *) m_MiscWndLst.GetNext(pos);
         if (pDlg)
            pDlg->Cancel();
      }

      m_bSwitchingProfiles = TRUE;
      pos = m_FrameWndLst.GetHeadPosition();
	   while( pos != NULL ) {
		   CBrowserFrame *pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
		   if(pBrowserFrame)
            pBrowserFrame->SendMessage(WM_CLOSE);
      }
      m_bSwitchingProfiles = FALSE;
   }
   else if (nsCRT::strcmp(aTopic, NS_LITERAL_STRING("profile-after-change").get()) == 0)
   {        
      // Only reinitialize everything if this is a profile switch, since this
      // called at start up and we already do evenything once already
      if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("switch").get())) {


/* Unloading a plugin that has subclassed a browser window crashes us, even if the
   subclassed window has been closed...I don't really know why, so I'll save this 
   as something to debug later, and just not unload plugins (which will cause problems
   when switching profiles, but since this feature caused kmeleon .4 to crash and *nobody*
   submitted a bug report, I have a feeling this isn't a commonly used feature.
*/
//         plugins.UnLoadAll();
         menus.Destroy();
         InitializePrefs();
         plugins.FindAndLoad();
         
         CBrowserFrame* browser;
         browser = CreateNewBrowserFrame();

         if (!browser) {
            MessageBox(NULL, "Could not create browser frame", NULL, MB_OK);
            m_pMainWnd->PostMessage(WM_QUIT);
            return NS_ERROR_FAILURE;
         }

         browser->SetFocus();
         browser->m_wndUrlBar.MaintainFocus();
         browser->m_wndBrowserView.LoadHomePage();
      }
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