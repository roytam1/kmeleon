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
#include "kmeleonConst.h"
#include "UnknownContentTypeHandler.h"
#include "PromptService.h"
#include "Utils.h"
#include "Tooltips.h"
#include "nsCRT.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MENU_CONFIG_FILE "menus.cfg"
#define ACCEL_CONFIG_FILE "accel.cfg"

// this is for overriding the Mozilla default PromptService component
//#define kComponentsLibname "kmeleonComponents.dll"

#define NS_PROMPTSERVICE_CID \
{0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

// this is for overriding the Mozilla default HelperAppLauncherDialog
#define NS_HELPERAPPLAUNCHERDIALOG_CID \
{0xf68578eb, 0x6ec2, 0x4169, {0xae, 0x19, 0x8c, 0x62, 0x43, 0xf0, 0xab, 0xe1}}
static NS_DEFINE_CID(kHelperAppLauncherDialogCID, NS_HELPERAPPLAUNCHERDIALOG_CID);

#define NS_DOWNLOAD_CID \
{ 0xe3fa9d0a, 0x1dd1, 0x11b2, { 0xbd, 0xef, 0x8c, 0x72, 0x0b, 0x59, 0x74, 0x45 } }
static NS_DEFINE_CID(kDownloadCID, NS_DOWNLOAD_CID);

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
   /*
   Brian - 1/8/01
      Moved the prompt stuff back into the main exe since
      it's a pain in the ass having it outside the exe
   */

   nsCOMPtr<nsIFactory> promptFactory;
   rv = NS_NewPromptServiceFactory(getter_AddRefs(promptFactory));
   if (NS_SUCCEEDED(rv)) {
      nsComponentManager::RegisterFactory(kPromptServiceCID,
         "Prompt Service",
         "@mozilla.org/embedcomp/prompt-service;1",
         promptFactory,
         PR_TRUE);
   }

   nsCOMPtr<nsIFactory> helperAppDlgFactory;
   rv = NewUnknownContentHandlerFactory(getter_AddRefs(helperAppDlgFactory));
   if (NS_SUCCEEDED(rv)) {
      nsComponentManager::RegisterFactory(kHelperAppLauncherDialogCID,
         "Helper App Launcher Dialog",
         "@mozilla.org/helperapplauncherdialog;1",
         helperAppDlgFactory,
         PR_TRUE); // replace existing
   }

   nsCOMPtr<nsIFactory> dlFactory;
   rv = NewDownloadFactory(getter_AddRefs(dlFactory));
   if (NS_SUCCEEDED(rv)) {
      nsComponentManager::RegisterFactory(kDownloadCID,
         "Download",
         NS_DOWNLOAD_CONTRACTID,
         dlFactory,
         PR_TRUE); // replace existing
   }

   nsCOMPtr<nsIFactory> tooltipTextProviderFactory;
   rv = NewTooltipTextProviderFactory(getter_AddRefs(tooltipTextProviderFactory));
   if (NS_SUCCEEDED(rv)) {
      nsComponentManager::RegisterFactory(kTooltipTextProviderCID,
         "Tooltip Text Provider",
         "@mozilla.org/embedcomp/tooltiptextprovider;1",
         tooltipTextProviderFactory,
         PR_TRUE); // replace existing
   }
   
   return rv;
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
   // parse the command line
   cmdline.Initialize(m_lpCmdLine);
   
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
      if (HWND hwndPrev = FindWindowEx(NULL, NULL, HIDDEN_WINDOW_CLASS, NULL) ) {
         // Ignore all command-line options when already open
         cmdline.GetSwitch("-P", NULL, TRUE);  
         cmdline.GetSwitch("-chrome", NULL, TRUE);
         cmdline.GetSwitch("-profilesDir", NULL, TRUE);

         if(*m_lpCmdLine) {
            COPYDATASTRUCT copyData;
            copyData.cbData = strlen(m_lpCmdLine)+1;
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
   
   winEmbedFileLocProvider *provider = new winEmbedFileLocProvider();
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
   
   // These have to be done in this order!
   InitializeDefineMap();

   InitializePrefs();

   plugins.FindAndLoad();

   InitializeMenusAccels();
   

   
   
   
   //	Register the hidden window class
   WNDCLASS wc = { 0 };
   wc.lpfnWndProc =  AfxWndProc;
   wc.hInstance = AfxGetInstanceHandle();
   wc.lpszClassName = HIDDEN_WINDOW_CLASS;


   if (!(m_hMainIcon = (HICON)LoadImage( NULL, theApp.preferences.settingsDir + "main.ico", IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE )))
       if (!(m_hMainIcon = (HICON)LoadImage( NULL, theApp.preferences.skinsDir + theApp.preferences.skinsCurrent + "main.ico", IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE )))
           if (!(m_hMainIcon = (HICON)LoadImage( NULL, theApp.preferences.skinsDir + "default\\main.ico", IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE )))
               m_hMainIcon = LoadIcon( IDR_MAINFRAME );

   if (!(m_hSmallIcon = (HICON)LoadImage( NULL, theApp.preferences.settingsDir + "main.ico", IMAGE_ICON, 16,16, LR_LOADFROMFILE )))
       if (!(m_hSmallIcon = (HICON)LoadImage( NULL, theApp.preferences.skinsDir + theApp.preferences.skinsCurrent + "main.ico", IMAGE_ICON, 16,16, LR_LOADFROMFILE )))
           if (!(m_hSmallIcon = (HICON)LoadImage( NULL, theApp.preferences.skinsDir + "default\\main.ico", IMAGE_ICON, 16,16, LR_LOADFROMFILE )))
               m_hSmallIcon = LoadIcon( IDR_MAINFRAME );

   wc.hIcon=m_hMainIcon;
   AfxRegisterClass( &wc );
   

   //	Register the browser window class
   wc.lpszClassName = BROWSER_WINDOW_CLASS;
   AfxRegisterClass( &wc );
   
   
   
   
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
   if (m_pMostRecentBrowserFrame && 
       !(m_pMostRecentBrowserFrame->m_style & WS_POPUP))
      m_pMostRecentBrowserFrame->m_wndReBar.SaveBandSizes();
   
   int openedByGecko = 0;
   if (chromeMask == 0) {
       chromeMask = nsIWebBrowserChrome::CHROME_ALL;
   } 
   else if (x==-1 && y==-1 && cx==-1 && cy==-1)
       openedByGecko = 1;

   LONG style = WS_OVERLAPPEDWINDOW;

   if (chromeMask && (chromeMask != nsIWebBrowserChrome::CHROME_DEFAULT) && 
       (!(chromeMask & (nsIWebBrowserChrome::CHROME_WINDOW_BORDERS)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_WINDOW_CLOSE)) || 
        !(chromeMask & (nsIWebBrowserChrome::CHROME_WINDOW_RESIZE)) || 
        !(chromeMask & (nsIWebBrowserChrome::CHROME_MENUBAR)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_TOOLBAR)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_LOCATIONBAR)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_STATUSBAR)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_SCROLLBARS)) ||
        !(chromeMask & (nsIWebBrowserChrome::CHROME_TITLEBAR)) )) {
       style = WS_POPUPWINDOW | WS_CAPTION;
   }
   else if (preferences.bMaximized && (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
       style |= WS_MAXIMIZE;

   style &= ~WS_VISIBLE;
   
   RECT winSize;
   SystemParametersInfo(SPI_GETWORKAREA, NULL, &winSize, 0);

   if (x>0 && y>0 && cx>0 && cy>0) {
       if (style & WS_POPUP) {
           winSize.left = x;
           winSize.top = y;
           winSize.right = x + cx;
           winSize.bottom = y + cy;
           AdjustWindowRectEx(&winSize, style, chromeMask & (nsIWebBrowserChrome::CHROME_MENUBAR), 0);
           winSize.right = winSize.right - winSize.left;
           winSize.bottom = winSize.bottom - winSize.top;
       }
       else {
           winSize.left = x;
           winSize.top = y;
           winSize.right = cx;
           winSize.bottom = cy;
       }
   }
   else {
       if (!(style & WS_POPUP) && 
           preferences.windowHeight > 0 && preferences.windowWidth > 0) {
           winSize.right  = preferences.windowWidth;
           winSize.bottom = preferences.windowHeight;         

           RECT screen;
           SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);
           int screenWidth   = screen.right - screen.left;
           int screenHeight  = screen.bottom - screen.top;

           if (preferences.windowYPos > 0 && preferences.windowXPos > 0) {
               winSize.left = preferences.windowXPos;
               winSize.top  = preferences.windowYPos;

               if (m_pMostRecentBrowserFrame && 
                   !(m_pMostRecentBrowserFrame->m_style & WS_POPUP)) {

                   int offset        = GetSystemMetrics(SM_CYSIZE);
                   offset           += GetSystemMetrics(SM_CXBORDER);

                   WINDOWPLACEMENT wp;
                   wp.length = sizeof(WINDOWPLACEMENT);
                   m_pMostRecentBrowserFrame->GetWindowPlacement(&wp);

                   // if the window is not maximized, let's use use GetWindowRect, which works
                   if (wp.showCmd == SW_SHOWNORMAL)
                       m_pMostRecentBrowserFrame->GetWindowRect(&wp.rcNormalPosition);
      
                   winSize.left   = wp.rcNormalPosition.left + offset;
                   winSize.top    = wp.rcNormalPosition.top + offset;

                   // the Create function is overloaded to treat "right" and "bottom" as
                   // "width" and "height"
                   winSize.right  = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
                   winSize.bottom = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

                   // don't create windows larger than the screen
                   if (winSize.right > screenWidth)
                       winSize.right = screenWidth;
                   if (winSize.bottom > screenHeight)
                       winSize.bottom = screenHeight;

                   // make sure the window isn't going to run off the screen
                   if (screen.right - (winSize.left + winSize.right) < 0)
                       winSize.left = screen.right - winSize.right;
                   if (screen.bottom - (winSize.top + winSize.bottom) < 0) 
                       winSize.top = screen.bottom - winSize.bottom;
      
                   // if we're going to be positioned right on top of the current window,
                   // move to the top corner
                   if ( ((winSize.left - wp.rcNormalPosition.left) < offset/3) &&
                        ((winSize.top - wp.rcNormalPosition.top) < offset/3) ) {
                       winSize.left = screen.left;
                       winSize.top = screen.top;
                   }
               }
               else {
                   // don't create windows larger than the screen
                   if (winSize.right > screenWidth)
                       winSize.right = screenWidth;
                   if (winSize.bottom > screenHeight)
                       winSize.bottom = screenHeight;

                   // make sure the window isn't going to run off the screen
                   if (screen.right - (winSize.left + winSize.right) < 0)
                       winSize.left = screen.right - winSize.right;
                   if (screen.bottom - (winSize.top + winSize.bottom) < 0) 
                       winSize.top = screen.bottom - winSize.bottom;
               }
           }
           else {
               // don't create windows larger than the screen
               if (winSize.right > screenWidth)
                   winSize.right = screenWidth;
               if (winSize.bottom > screenHeight)
                   winSize.bottom = screenHeight;
           }
       }
   }

   // Now, create the browser frame
   CBrowserFrame* pFrame = new CBrowserFrame(chromeMask, style);


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


   if (!pFrame->Create(BROWSER_WINDOW_CLASS, strTitle, style, winSize, NULL, NULL, 0L, NULL))
      return NULL;
   
   pFrame->SetIcon(m_hMainIcon, true);
   pFrame->SetIcon(m_hSmallIcon, false);
   
   // load accelerator resource
   //pFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));
   pFrame->m_hAccelTable = accel.GetTable();
   
   // this only needs to be called once
   if (!m_bFirstWindowCreated) {
      pFrame->m_wndReBar.DrawToolBarMenu();
      m_bFirstWindowCreated = TRUE;
   }

   if (!preferences.bHideTaskBarButtons) {
      pFrame->ModifyStyleEx(0, WS_EX_APPWINDOW);
   }

   // Show the window...
   if(bShowWindow) {
       if (preferences.bMaximized && !(style & WS_POPUP) && 
           (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
           pFrame->ShowWindow(SW_SHOWMAXIMIZED);
       else pFrame->ShowWindow(SW_SHOW);
       pFrame->UpdateWindow();
   }

   // Add to the list of BrowserFrame windows
   m_FrameWndLst.AddHead(pFrame);
   
   pFrame->m_created = true;
   pFrame->m_ignoreMoveResize = (openedByGecko && !(style & WS_POPUP)) ? 2 : 0;
   
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
      case PREF_NEW_WINDOW_BLANK:
         pBrowserFrame->m_wndBrowserView.OpenURL("about:blank");
         break;
      case PREF_NEW_WINDOW_URL:
         if (preferences.newWindowURL.IsEmpty())
            pBrowserFrame->m_wndBrowserView.OpenURL("about:blank");
         else
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
   
   if (m_ProfileMgr) delete m_ProfileMgr;
   
   // unload the plugins before we terminate embedding,
   // this way plugins can still call the preference functions
   plugins.UnLoadAll();

   preferences.Save();
   preferences.SaveDlgPrefs();
   
   NS_TermEmbedding();
   
   return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
   CWinApp::OnIdle(lCount);
   
   //NS_DoIdleEmbeddingStuff();
   
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
      do_GetService("@mozilla.org/observer-service;1", &rv);
   
   observerService->AddObserver(this, "profile-approve-change", PR_TRUE);
   observerService->AddObserver(this, "profile-change-teardown", PR_TRUE);
   observerService->AddObserver(this, "profile-after-change", PR_TRUE);
   
   
   int len = cmdline.GetSwitch("-P", NULL, FALSE);
   
   // there are no parameters, load the most recent profile
   if (len == 0) {
      cmdline.GetSwitch("-P", NULL, TRUE);  // remove the flag from the command line
   }
   // try loading the profile specified
   else if (len > 0) {
      char *profile = new char[len+1];
      cmdline.GetSwitch("-P", profile, TRUE);
      
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
   CFrameWnd *hiddenWnd = new CHiddenWnd;
   if(!hiddenWnd)
      return FALSE;

   // do this before Create so the window spawned in Create will know who it's parent is
   m_pMainWnd = hiddenWnd;
   
   RECT bounds = { 0 };
   hiddenWnd->Create(HIDDEN_WINDOW_CLASS, "K-Meleon hidden window", 0, bounds, NULL, NULL, 0, NULL);

   return TRUE;

}

nsresult CMfcEmbedApp::InitializePrefs(){
   preferences.Load();
   preferences.Save();
   preferences.SaveDlgPrefs();

   return TRUE;
}

nsresult CMfcEmbedApp::InitializeMenusAccels(){
   nsresult nResult = TRUE;

   CString filename;

   filename = preferences.settingsDir + MENU_CONFIG_FILE;

   if (!menus.Load(filename)){
      MessageBox(NULL, "Could not find " MENU_CONFIG_FILE, NULL, 0);

      if (!fopen(filename, "r")) {
         // if it doesn't exist, create it
         FILE *f = fopen(filename, "w");
         fclose(f);
      }

      nResult = FALSE;
   }


   filename = preferences.settingsDir + ACCEL_CONFIG_FILE;
   if (!accel.Load(filename)){
      MessageBox(NULL, "Could not find " ACCEL_CONFIG_FILE, NULL, 0);

      if (!fopen(filename, "r")) {
         // if it doesn't exist, create it
         FILE *f = fopen(filename, "w");
         fclose(f);
      }

      nResult = FALSE;
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

NS_IMETHODIMP CMfcEmbedApp::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
   nsresult rv = NS_OK;
   
   if (nsCRT::strcmp(aTopic, "profile-approve-change") == 0)
   {
      // Ask the user if they want to
      int result = AfxMessageBox(IDS_PROFILE_SWITCH, MB_YESNO | MB_ICONQUESTION, 0);
      if (result != IDYES)
      {
         nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
         NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
         status->VetoChange();
      }
   }
   else if (nsCRT::strcmp(aTopic, "profile-change-teardown") == 0)
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
   else if (nsCRT::strcmp(aTopic, "profile-after-change") == 0)
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
   
   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags, -1, -1, -1, -1, PR_FALSE);
   if(pBrowserFrame) {
      *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *, pBrowserFrame->GetBrowserImpl());
      NS_ADDREF(*_retval);
   }
   return NS_OK;
   
}



// load the mapped values
void CMfcEmbedApp::InitializeDefineMap() {

#include "defineMap.cpp"

}

int CMfcEmbedApp::GetID(char *strID) {
   int val = 0;
   defineMap.Lookup(strID, val);
   return val;
}
