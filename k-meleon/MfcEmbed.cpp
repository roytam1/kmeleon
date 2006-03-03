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
 *   Conrad Carlen <ccarlen@netscape.com> 
 *
 * ***** END LICENSE BLOCK ***** */

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
#include "nsXPCOM.h"
#include "nsXPCOMGlue.h"
#include "MfcEmbed.h"
#include "HiddenWnd.h"
#include "BrowserFrm.h"
#include "winEmbedFileLocProvider.h"
#include "ProfileMgr.h"
#include "BrowserImpl.h"
#include "kmeleonConst.h"
#include "UnknownContentTypeHandler.h"
#include <io.h>
#include <fcntl.h>

#include "nsIPluginManager.h"
#include "nsIIOService.h"
#include "nsIWindowWatcher.h"
static UINT WM_POSTEVENT = RegisterWindowMessage(_T("XPCOM_PostEvent"));

#ifdef MOZ_PROFILESHARING
#include "nsIProfileSharingSetup.h"
#endif

#ifdef _BUILD_STATIC_BIN
#include "nsStaticComponent.h"
nsresult PR_CALLBACK
app_getModuleInfo(nsStaticModuleInfo **info, PRUint32 *count);
#endif


#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MENU_CONFIG_FILE "menus.cfg"
#define ACCEL_CONFIG_FILE "accel.cfg"

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
   mRefCnt = 1; // Start at one - nothing is going to addref this object

   m_ProfileMgr = NULL;
   m_bSwitchingProfiles = FALSE;

   m_bFirstWindowCreated = FALSE;
   m_pMostRecentBrowserFrame  = NULL;
   m_toolbarControlsMenu = NULL;     
}

CMfcEmbedApp theApp;

static NS_IMETHODIMP
RefreshPlugins(PRBool aReloadPages)
{
    NS_DEFINE_CID(pluginManagerCID,NS_PLUGINMANAGER_CID);

    nsCOMPtr<nsIPluginManager> plugins(do_GetService(pluginManagerCID));

    if (!plugins)
        return NS_ERROR_FAILURE;

    return plugins->ReloadPlugins(aReloadPages);
}

nsresult CMfcEmbedApp::SetOffline(BOOL offline)
{
    nsresult result;
    nsCOMPtr<nsIIOService> io = do_GetService(NS_IOSERVICE_CONTRACTID, &result);
    if (NS_SUCCEEDED(result)) {
        result = io->SetOffline(offline);
        if (NS_SUCCEEDED(result))
            theApp.preferences.bOffline = offline;
    }

    return result;
}


bool CMfcEmbedApp::FindSkinFile( CString& szSkinFile, TCHAR *filename ) 
{
   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   
   if (!szSkinFile || !filename || !*filename)
      return false;

   CString file = theApp.preferences.settingsDir + filename;
   hFile = FindFirstFile(file, &FindData);
   if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         szSkinFile = file;
         return true;
      }   

    CString tmp = theApp.preferences.skinsCurrent;
	while (tmp.GetLength()>0) {
		if (tmp.GetAt( tmp.GetLength()-1 ) != '\\')
			tmp = tmp + "\\";
		file = theApp.preferences.skinsDir + tmp + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
		tmp = tmp.Left( tmp.GetLength()-2 );
	}

	file = theApp.preferences.skinsDir + CString("default\\") + filename;
	hFile = FindFirstFile(file, &FindData);
	if(hFile != INVALID_HANDLE_VALUE) {   
		FindClose(hFile);
		szSkinFile = file;
		return true;
	}  

	return false;
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
#ifdef _BUILD_STATIC_BIN
    // Initialize XPCOM's module info table
    NSGetStaticModuleInfo = app_getModuleInfo;
#endif
#ifdef XPCOM_GLUE
    if (NS_FAILED(XPCOMGlueStartup(GRE_GetXPCOMPath()))) {
        MessageBox(NULL, _T("Could not initialize XPCOM. Perhaps the GRE\nis not installed or could not be found?"), _T("Kmeleon"), MB_OK | MB_ICONERROR);
        return FALSE;
    }
#endif
   m_hMutex = CreateMutex(NULL, FALSE, NULL);

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
      if (HWND hwndPrev = ::FindWindowEx(NULL, NULL, HIDDEN_WINDOW_CLASS, NULL) ) {
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
   //
    // 1. Determine the name of the dir from which the GRE based app is being run
    // from [It's OK to do this even if you're not running in an GRE env]
    //
    // 2. Create an nsILocalFile out of it which will passed in to NS_InitEmbedding()
    //
    // Please see http://www.mozilla.org/projects/embedding/GRE.html
    // for more info. on GRE

    TCHAR path[_MAX_PATH+1];
    ::GetModuleFileName(0, path, _MAX_PATH);
    TCHAR* lastSlash = _tcsrchr(path, _T('\\'));
    if (!lastSlash) {
        NS_ERROR("No slash in module file name... something is wrong.");
        return FALSE;
    }
    *lastSlash = _T('\0');

    
    nsresult rv;
    nsCOMPtr<nsILocalFile> mreAppDir;
    rv = NS_NewNativeLocalFile(nsEmbedCString(T2A(path)), TRUE, getter_AddRefs(mreAppDir));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create mreAppDir localfile");
   // Take a look at 
   // http://www.mozilla.org/projects/xpcom/file_locations.html
   // for more info on File Locations
   
   CString strRes;
   strRes.LoadString(IDS_PROFILES_FOLDER_NAME);
   winEmbedFileLocProvider *provider = new winEmbedFileLocProvider(nsEmbedCString(T2A(strRes.GetBuffer(0))));
   if(!provider){
      ASSERT(FALSE);
      return FALSE;
   }
   
   rv = NS_InitEmbedding(mreAppDir, provider);
   if(NS_FAILED(rv))
   { 
      ASSERT(FALSE);
      return FALSE;
   }
   
   rv = OverrideComponents();
   if(NS_FAILED(rv)) 
   {
      ASSERT(FALSE);
      return FALSE;
   }
   
   rv = InitializeWindowCreator();
   if (NS_FAILED(rv)) 
   {
      ASSERT(FALSE);
      return FALSE;
   }
   
   if(!InitializeProfiles())
   {
      ASSERT(FALSE);
      NS_TermEmbedding();
      return FALSE;
   }
   
   // These have to be done in this order!
   InitializeDefineMap();

   InitializePrefs();
   SetOffline(theApp.preferences.bOffline);

   m_MRUList = new CMostRecentUrls();

   // Retrieve the default icon
   CString sSkinIcon;
   if (FindSkinFile(sSkinIcon, _T("main.ico")))
   {
	   m_hMainIcon = (HICON)LoadImage( NULL, sSkinIcon, IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE );
	   m_hSmallIcon = (HICON)LoadImage( NULL, sSkinIcon, IMAGE_ICON, 16,16, LR_LOADFROMFILE );
   }
   else
   {
	   m_hMainIcon = LoadIcon( IDR_MAINFRAME );
	   m_hSmallIcon = LoadIcon( IDR_MAINFRAME );
   }

   plugins.FindAndLoad();
   plugins.SendMessage("*", "* Plugin Manager", "Init");
   InitializeMenusAccels();
   plugins.SendMessage("*", "* Plugin Manager", "Setup");


   RefreshPlugins(PR_FALSE);
   
   
   // Register the hidden window class
   WNDCLASS wc = { 0 };
   wc.lpfnWndProc =  AfxWndProc;
   wc.hInstance = AfxGetInstanceHandle();
   wc.lpszClassName = HIDDEN_WINDOW_CLASS;


   wc.hIcon=m_hMainIcon;
   AfxRegisterClass( &wc );
   

   // Register the browser window class
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
   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( m_hMutex, 1000L);
   if (dwWaitResult != WAIT_OBJECT_0) {
     return NULL;
   }

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
   else if (x==-1 && y==-1 && cx==-1 && cy==-1 && !bShowWindow)
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
   
   RECT screen;
   SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);

   int screenWidth   = screen.right - screen.left;
   int screenHeight  = screen.bottom - screen.top;

   RECT winSize = CFrameWnd::rectDefault;

   winSize.left = screen.left + screenWidth / 40;
   winSize.top = screen.top + screenHeight / 40;
   winSize.right = 15*screenWidth / 20;
   winSize.bottom = 18*screenHeight/20;
           
   if (x>0 && y>0 && cx>0 && cy>0) {
       if (style & WS_POPUP) {
           winSize.left = x;
           winSize.top = y;
           winSize.right = x + cx;
           winSize.bottom = y + cy;
           AdjustWindowRectEx(&winSize, style, chromeMask & (nsIWebBrowserChrome::CHROME_MENUBAR), 0);
           winSize.right = winSize.right - winSize.left;
           winSize.bottom = winSize.bottom - winSize.top;

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
       else {
           winSize.left = x;
           winSize.top = y;
           winSize.right = cx;
           winSize.bottom = cy;

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
       if (!(style & WS_POPUP)) {
         if (preferences.windowHeight > 0 && preferences.windowWidth > 0) {
           winSize.right  = preferences.windowWidth;
           winSize.bottom = preferences.windowHeight;         

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
                   if ( (abs(winSize.left - wp.rcNormalPosition.left) < offset/3) &&
                        (abs(winSize.top - wp.rcNormalPosition.top) < offset/3) ) {
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
         else {
           winSize.left = screen.left + screenWidth / 20;
           winSize.top = screen.top + screenHeight / 20;
           winSize.right = 15*screenWidth / 20;
           winSize.bottom = 18*screenHeight/20;

           preferences.windowXPos   = winSize.left;
           preferences.windowYPos   = winSize.top;
           preferences.windowWidth  = winSize.right;
           preferences.windowHeight = winSize.bottom;
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


   if (!pFrame->Create(BROWSER_WINDOW_CLASS, strTitle, style, winSize, NULL, NULL, 0L, NULL)) {
       ReleaseMutex(m_hMutex);
      return NULL;
   }

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
   if(bShowWindow) 
   {
       if (preferences.bMaximized && !(style & WS_POPUP) && 
           (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
           pFrame->ShowWindow(SW_SHOWMAXIMIZED);
       else 
           pFrame->ShowWindow(SW_SHOW);
       pFrame->BringWindowToTop();
       pFrame->SetActiveWindow();
       pFrame->SetFocus();
       pFrame->UpdateWindow();
       pFrame->m_created = true;
   }

   // Add to the list of BrowserFrame windows
   m_FrameWndLst.AddHead(pFrame);
   
   pFrame->m_ignoreMoveResize = (openedByGecko && !(style & WS_POPUP)) ? 2 : 0;
   
   theApp.m_pMostRecentBrowserFrame = pOldRecentFrame;
   
   ReleaseMutex(m_hMutex);
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
      // pBrowserFrame->m_wndUrlBar.MaintainFocus();
      switch (preferences.iNewWindowOpenAs) {
      case PREF_NEW_WINDOW_CURRENT:
         if (sURI) {
            m_pOpenNewBrowserFrame->m_wndBrowserView.CloneSHistory(pBrowserFrame->m_wndBrowserView);
			//pBrowserFrame->m_wndBrowserView.OpenURL(sURI);
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
   
   if(IsClipboardFormatAvailable(CF_TEXT)) {
       if(OpenClipboard(NULL)) {
           HANDLE hcb = GetClipboardData(CF_TEXT);
           if (hcb) {
               LPVOID pData = GlobalLock(hcb);
               if (pData) {
                   char *pszData = (char*)malloc(strlen((char*)pData) + 1);
                   if (pszData) {
                       strcpy(pszData, (LPSTR)pData);
                       GlobalUnlock(hcb);

                       EmptyClipboard();

                       HGLOBAL hData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,strlen(pszData) + 1);
                       if (hData) {
                           pData = GlobalLock(hData);
                           if (pData) {
                               strcpy((LPSTR)pData, pszData);
                               GlobalUnlock(hData);
                               SetClipboardData(CF_TEXT, hData);
                           }
                       }
				   }
				   free(pszData);
               }
		   }
		   CloseClipboard();
       }
   }
   
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

void CMfcEmbedApp::BroadcastMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
   CBrowserFrame* pBrowserFrame = NULL;
   POSITION pos = m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
      if(pBrowserFrame)
         pBrowserFrame->SendMessage(Msg, wParam, lParam);
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

   delete m_MRUList;
   
   // unload the plugins before we terminate embedding,
   // this way plugins can still call the preference functions
   plugins.SendMessage("*", "* Plugin Manager", "Quit");

   preferences.Save();
   
   m_ProfileMgr->ShutDownCurrentProfile( theApp.preferences.bGuestAccount );
   if (m_ProfileMgr) delete m_ProfileMgr;
   
   NS_TermEmbedding();
#ifdef XPCOM_GLUE
    XPCOMGlueShutdown();
#endif
   plugins.UnLoadAll();
   
   return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
   CWinApp::OnIdle(lCount);
   
   //NS_DoIdleEmbeddingStuff();
   
   return FALSE;
}
BOOL CMfcEmbedApp::IsIdleMessage( MSG* pMsg )
{
   if (!CWinApp::IsIdleMessage( pMsg ) || 
       pMsg->message == WM_USER+1 || // WM_CALLMETHOD
	   pMsg->message == WM_POSTEVENT ||
	   pMsg->message == WM_TIMER) 

      return FALSE;
   else
      return TRUE;
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
   if (!observerService) return FALSE;
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
   return hiddenWnd->Create(HIDDEN_WINDOW_CLASS, _T("K-Meleon hidden window"), 0, bounds, NULL, NULL, 0, NULL);
}

nsresult CMfcEmbedApp::InitializePrefs(){
   preferences.Load();
   preferences.Save();

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

#ifdef XPCOM_GLUE
NS_IMPL_ISUPPORTS3(CMfcEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference);
#else
NS_IMPL_THREADSAFE_ISUPPORTS3(CMfcEmbedApp, nsIObserver, nsIWindowCreator, nsISupportsWeakReference);
#endif

// ---------------------------------------------------------------------------
//  CMfcEmbedApp : nsIObserver
// ---------------------------------------------------------------------------

// Mainly needed to support "on the fly" profile switching

NS_IMETHODIMP CMfcEmbedApp::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
   nsresult rv = NS_OK;
   
   USES_CONVERSION;

   if (strcmp(aTopic, "profile-approve-change") == 0 &&
       (!someData ||
        strcmp(W2A(someData), "shutdown-cleanse") != 0 &&
        strcmp(W2A(someData), "shutdown-persist") != 0))
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
   else if (strcmp(aTopic, "profile-change-teardown") == 0)
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
   else if (strcmp(aTopic, "profile-after-change") == 0)
   {        
      // Only reinitialize everything if this is a profile switch, since this
      // called at start up and we already do evenything once already
      if (!wcscmp(someData, NS_LITERAL_STRING("switch").get())) {
         
         
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
         // browser->m_wndUrlBar.MaintainFocus();
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
    if (theApp.preferences.GetBool("kmeleon.general.BlockAllChromeWindows", false))
        return NS_ERROR_FAILURE;

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
