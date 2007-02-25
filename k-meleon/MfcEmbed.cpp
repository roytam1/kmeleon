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
#include "MenuParser.h"
#include <io.h>
#include <fcntl.h>

#include "nsIPluginManager.h"
#include "nsIIOService.h"
#include "nsIWindowWatcher.h"
#include "nsIChromeRegistrySea.h"
#include "nsIProfileInternal.h"

static UINT WM_POSTEVENT = RegisterWindowMessage(_T("XPCOM_PostEvent"));

#ifdef MOZ_PROFILESHARING
#include "nsIProfileSharingSetup.h"
#endif

#ifdef _BUILD_STATIC_BIN
#include "nsStaticComponents.h"
nsresult PR_CALLBACK
app_getModuleInfo(nsStaticModuleInfo **info, PRUint32 *count);
#endif


#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif

#ifdef _DEBUG
/*#include "StackWalker.h"

static struct _test
{
  _test() { InitAllocCheck(); }
  ~_test(){ DeInitAllocCheck(); }

} _myLeakFinder;*/
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LANG_CONFIG_FILE _T("language.cfg")
#define MENU_CONFIG_FILE _T("menus.cfg")
#define ACCEL_CONFIG_FILE _T("accel.cfg")

extern CString GetMozDirectory(char* dirName);

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

#ifdef _DEBUG
void CMfcEmbedApp::ShowDebugConsole()
{

    // Show console only in debug mode

    if(! AllocConsole())
        return;

    // Redirect stdout to the console
    int hCrtOut = _open_osfhandle(
                (long) GetStdHandle(STD_OUTPUT_HANDLE),
                _O_TEXT);
    if(hCrtOut == -1)
        return;

    FILE *hfOut = _fdopen(hCrtOut, "w");
    if(hfOut != NULL)
    {
        // Setup for unbuffered I/O so the console 
        // output shows up right away
        *stdout = *hfOut;
        setvbuf(stdout, NULL, _IONBF, 0); 
    }

    // Redirect stderr to the console
    int hCrtErr = _open_osfhandle(
                (long) GetStdHandle(STD_ERROR_HANDLE),
                _O_TEXT);
    if(hCrtErr == -1)
        return;

    FILE *hfErr = _fdopen(hCrtErr, "w");
    if(hfErr != NULL)
    {
        // Setup for unbuffered I/O so the console 
        // output shows up right away
        *stderr = *hfErr;
        setvbuf(stderr, NULL, _IONBF, 0); 
    }
}
#endif

bool CMfcEmbedApp::FindSkinFile( CString& szSkinFile, TCHAR *filename ) 
{
   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   
   if (!szSkinFile || !filename || !*filename)
      return false;

   CString file = GetFolder(UserSettingsFolder) + _T('\\') + filename;
   hFile = FindFirstFile(file, &FindData);
   if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         szSkinFile = file;
         return true;
      }   

    CString tmp = theApp.preferences.skinsCurrent;
    CString skinsDir = GetFolder(UserSkinsFolder) + _T("\\");
	while (tmp.GetLength()>0) {
		if (tmp.GetAt( tmp.GetLength()-1 ) != '\\')
			tmp = tmp + _T("\\");
		file = skinsDir + tmp + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
		tmp = tmp.Left( tmp.GetLength()-2 );
	}

    tmp = theApp.preferences.skinsCurrent;
    skinsDir = GetFolder(SkinsFolder) + _T("\\");
	while (tmp.GetLength()>0) {
		if (tmp.GetAt( tmp.GetLength()-1 ) != '\\')
			tmp = tmp + _T("\\");
		file = skinsDir + tmp + filename;
		hFile = FindFirstFile(file, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			szSkinFile = file;
			return true;
		}   
		tmp = tmp.Left( tmp.GetLength()-2 );
	}

	file = GetFolder(SkinsFolder) + _T("\\default\\") + filename;
	hFile = FindFirstFile(file, &FindData);
	if(hFile != INVALID_HANDLE_VALUE) {   
		FindClose(hFile);
		szSkinFile = file;
		return true;
	}  

	return false;
}

#include <shlwapi.h>

BOOL CMfcEmbedApp::LoadLanguage()
{
   TCHAR resDll[MAX_PATH], *extension;
	
   // Look for dll resources
	::GetModuleFileName(m_hInstance, resDll, MAX_PATH);
   extension = _tcsrchr(resDll, _T('.'));//::PathFindExtension(resDll);

   if ((extension + 7) - resDll  > MAX_PATH)
      return FALSE;

   lstrcpy(extension, _T("loc.dll"));
   HINSTANCE hInstResDll = ::LoadLibrary(resDll);
   if (!hInstResDll) return FALSE;

   // Check dll version
   CString version, versiondll;
   version.LoadString(IDS_LANG_VERSION);
#if _MSC_VER >= 1300 
   versiondll.LoadString(hInstResDll, IDS_LANG_VERSION);
#else
   LoadString(hInstResDll, IDS_LANG_VERSION, versiondll.GetBuffer(10), 10);
   versiondll.ReleaseBuffer();
#endif
   if (version.Compare(versiondll) != 0) {
      ::FreeLibrary(hInstResDll);
      return FALSE;
   }
   
   // Look for language file
   TCHAR* langFile = resDll;
   TCHAR* lastSlash = _tcsrchr(resDll, _T('\\'));
   if (!lastSlash) {
      ::FreeLibrary(hInstResDll);
      ASSERT(FALSE);
      return FALSE;
   }
   lastSlash++;

   TCHAR locale[10];
   ::LoadString(hInstResDll, IDS_LOCALE_ID, locale, 10);
   
   lstrcpy(lastSlash, _T("kmeleon."));
   lstrcat(lastSlash, locale);
   lstrcat(lastSlash, _T(".kml"));

   if (!lang.Load(langFile)) {
      lstrcpy(lastSlash, LANG_CONFIG_FILE);
      if (!lang.Load(langFile)) {
         ::FreeLibrary(hInstResDll);
         return FALSE;
      }
   }

   AfxSetResourceHandle(hInstResDll);
#if _MSC_VER >= 1300 
   _AtlBaseModule.SetResourceInstance(hInstResDll);
#endif
   return TRUE;
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
	LoadLanguage();
#ifdef _DEBUG
	ShowDebugConsole();
#endif

#ifdef XPCOM_GLUE
    if (NS_FAILED(XPCOMGlueStartup(GRE_GetXPCOMPath()))) {
        MessageBox(NULL, _T("Could not initialize XPCOM. Perhaps the GRE\nis not installed or could not be found?"), _T("Kmeleon"), MB_OK | MB_ICONERROR);
        return FALSE;
    }
#endif
   m_hMutex = CreateMutex(NULL, FALSE, NULL);

   // parse the command line
   // XXX 
   USES_CONVERSION;
   cmdline.Initialize(T2A(m_lpCmdLine));
   
   // check for prior instances
   HANDLE hMutexOneInstance = CreateMutex( NULL, TRUE, _T("K-Meleon Instance Mutex") );
   m_bAlreadyRunning = ( GetLastError() == ERROR_ALREADY_EXISTS );
   
   if ( hMutexOneInstance )
      ReleaseMutex( hMutexOneInstance );

   // if another instance is already running, pass it our command line paramaters,
   // and ask it to open a new window
   // eventually, we should handle this through DDE
   if (cmdline.GetSwitch("-new", NULL, TRUE)<0 && m_bAlreadyRunning) {
      // find the hidden window
      if (HWND hwndPrev = ::FindWindowEx(NULL, NULL, HIDDEN_WINDOW_CLASS, NULL) ) {
         // Ignore all command-line options when already open
         cmdline.GetSwitch("-P", NULL, TRUE);  
         cmdline.GetSwitch("-chrome", NULL, TRUE);
         cmdline.GetSwitch("-profilesDir", NULL, TRUE);

         if(*m_lpCmdLine) {
            COPYDATASTRUCT copyData;
            copyData.dwData = 0;
            copyData.cbData = (_tcsclen(m_lpCmdLine)+1)*sizeof(TCHAR);
            copyData.lpData = (void *) m_lpCmdLine;
            SendMessage(hwndPrev, WM_COPYDATA, NULL, (LPARAM) &copyData);
         }
         else
            SendMessage(hwndPrev, UWM_NEWWINDOW, NULL, NULL);
      }
      return FALSE;
   }
   else 
      m_bAlreadyRunning = FALSE;

    //Enable3dControls();   
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
    m_sRootFolder = path;

    nsresult rv;
    nsCOMPtr<nsILocalFile> mreAppDir;
#ifdef _UNICODE
    rv = NS_NewLocalFile(nsEmbedString(path), TRUE, getter_AddRefs(mreAppDir));
#else
    rv = NS_NewNativeLocalFile(nsEmbedCString(path), TRUE, getter_AddRefs(mreAppDir));
#endif
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
   
#ifdef _BUILD_STATIC_BIN
	rv = NS_InitEmbedding(mreAppDir, provider, kPStaticModules, kStaticModuleCount);
#else
	rv = NS_InitEmbedding(mreAppDir, provider);
#endif
   
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

   CheckProfileVersion();

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

   // Minimal unicode support
   #ifndef _UNICODE
   OSVERSIONINFO osinfo;
   osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osinfo);
   m_bUnicode = (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
   #endif

#ifdef INTERNAL_SITEICONS
   // Create the favicon list
   if (preferences.bSiteIcons)
		favicons.Create(16,16,ILC_COLOR32|ILC_MASK,25,100);
#endif

   plugins.FindAndLoad();
   InitializeMenusAccels();
   


   RefreshPlugins(PR_FALSE);
   
   
   // Register the hidden window class
   WNDCLASS wc = { 0 };
#ifdef _UNICODE
   wc.lpfnWndProc =  DefWindowProc; // MSLU incompatibility
#else
   wc.lpfnWndProc =  AfxWndProc;
#endif
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

// add new download dialogs to the internal window list so we won't exit
// the app while downloads are in progress
void CMfcEmbedApp::RegisterWindow(CDialog *window) {
   m_MiscWndLst.AddHead(window);
}

void CMfcEmbedApp::UnregisterWindow(CDialog *window) {
   POSITION pos = m_MiscWndLst.Find(window);
   m_MiscWndLst.RemoveAt(pos);
   
   
   // See comment in RemoveFrameFromList()
   if ((m_MiscWndLst.GetCount() == 0) && (m_FrameWndLst.GetCount() == 0)) {
      
      if (m_pMainWnd) {
         // if we're staying resident, create the hidden browser window
         if (((CHiddenWnd*) m_pMainWnd)->Persisting() == PERSIST_STATE_ENABLED)
            ((CHiddenWnd*) m_pMainWnd)->StayResident();
         
         // otherwise we're exiting, close the Evil Hidden Window
         else
             m_pMainWnd->DestroyWindow();
      }
   }
}



CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
                                                   PRInt32 x, PRInt32 y,
                                                   PRInt32 cx, PRInt32 cy,
                                                   PRBool bShowWindow,
												   CWnd* pParent)
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

   // XXX Chrome dialogs shouldn't have thoses.
   if (chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
      chromeMask &=  ~nsIWebBrowserChrome::CHROME_MENUBAR
	                &~nsIWebBrowserChrome::CHROME_TOOLBAR
					&~nsIWebBrowserChrome::CHROME_LOCATIONBAR
					&~nsIWebBrowserChrome::CHROME_STATUSBAR
					&~nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR;
   
   LONG style, styleEx = 0L;

   if (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_POPUP) {
      style = WS_POPUPWINDOW | WS_CAPTION;
      styleEx = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
   }
   else if (chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_DIALOG) {
	   style =  WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_DLGFRAME;
	         //| DS_3DLOOK | DS_MODALFRAME;
	   styleEx = WS_EX_DLGMODALFRAME;
   }
   else {
	   style = WS_OVERLAPPEDWINDOW;
	   if (pParent && (chromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT))
          style |= WS_POPUP;
   }
   
   if ( !(chromeMask & nsIWebBrowserChrome::CHROME_DEFAULT) &&
	    ((chromeMask & nsIWebBrowserChrome::CHROME_ALL) != nsIWebBrowserChrome::CHROME_ALL)) {
   
      if( !(chromeMask & nsIWebBrowserChrome::CHROME_TITLEBAR) )
         style &= ~WS_CAPTION; // No caption      

      if (!theApp.preferences.bDisableResize) {   
         if( !(chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) ) {
             // Can't resize this window
             style &= ~WS_THICKFRAME;
             style &= ~WS_MAXIMIZEBOX;
         }
      }

      if ( !(chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_CLOSE) &&
	       !(chromeMask & nsIWebBrowserChrome::CHROME_MENUBAR) )
         style &= ~WS_SYSMENU;

      if (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_MIN)  
         style |= WS_MINIMIZEBOX;
   }
      
   if ( (chromeMask & (nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) ||
        (!(chromeMask & nsIWebBrowserChrome::CHROME_DEFAULT) &&
        !(chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR)) )
      style |= WS_POPUP; // For the sake of layers....

   if (preferences.bMaximized && !(style & WS_POPUP) &&
      !(chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME) &&
      (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
      style |= WS_MAXIMIZE;

   RECT screen, winSize;
   SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);
   int screenWidth   = screen.right - screen.left;
   int screenHeight  = screen.bottom - screen.top;

   if (x>0 && y>0 && cx>0 && cy>0) {
      winSize.left = x;
      winSize.top = y;
      winSize.right = x + cx;
      winSize.bottom = y + cy;
      AdjustWindowRectEx(&winSize, style, chromeMask & (nsIWebBrowserChrome::CHROME_MENUBAR), 0);
   }
   else {

      // If the last active window is not a popup use cascading placement
      if (m_pMostRecentBrowserFrame && 
         !(m_pMostRecentBrowserFrame->m_style & WS_POPUP))
      {
         WINDOWPLACEMENT wp;
         wp.length = sizeof(WINDOWPLACEMENT);
         m_pMostRecentBrowserFrame->GetWindowPlacement(&wp);

         // if the window is not maximized, let's use use GetWindowRect, which works
         if (wp.showCmd == SW_SHOWNORMAL)
            m_pMostRecentBrowserFrame->GetWindowRect(&wp.rcNormalPosition);

         int offset = GetSystemMetrics(SM_CYSIZE) + GetSystemMetrics(SM_CXBORDER);
         winSize.left   = wp.rcNormalPosition.left + offset;
         winSize.top    = wp.rcNormalPosition.top + offset;
         winSize.right  = wp.rcNormalPosition.right + offset;
         winSize.bottom = wp.rcNormalPosition.bottom + offset;

         // Put the window to the top corner if we're too far in 
         // the bottom left.
         if ( (screen.right - winSize.right) < offset
            && (screen.bottom - winSize.bottom) < offset)
         {
            winSize.left = screen.left;
            winSize.top = screen.top;
            winSize.right = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
            winSize.bottom = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
         }
      }
      else {
         
         // Use default position 
         winSize.left = screen.left + screenWidth / 20;
         winSize.top = screen.top + screenHeight / 20;
         winSize.right = winSize.left + 15*screenWidth / 20;
         winSize.bottom = winSize.top + 18*screenHeight/20;

         // Use user preference position if any
         if (preferences.windowXPos >= 0) 
            winSize.left = preferences.windowXPos;
         if (preferences.windowYPos >=0)
            winSize.top  = preferences.windowYPos;
         if (preferences.windowWidth > 0)
            winSize.right  = winSize.left + preferences.windowWidth;
         if (preferences.windowHeight > 0)
            winSize.bottom = winSize.top + preferences.windowHeight;
         
      }
   }

   // don't create windows larger than the screen
   if ((winSize.right - winSize.left) > screenWidth)
      winSize.right = screenWidth - winSize.left;
   if ((winSize.bottom - winSize.top) > screenHeight)
      winSize.bottom = screenHeight - winSize.top;

   
   // Center the window if needed, useless at this point.
   //if (chromeMask & nsIWebBrowserChrome::CHROME_CENTER_SCREEN)
   //{
   //   int height = winSize.bottom - winSize.top;
   //   int width = winSize.right - winSize.left;
   //	winSize.top = screen.top + (screenHeight - height)/2;
   //   winSize.bottom = winSize.top + height;
   //   winSize.left = screen.left + (screenWidth - width)/2;
   //   winSize.right = winSize.left + width;
   //}
           
   // make sure the window isn't going to run off the screen
   if ((screen.right - winSize.right) < 0) {
      winSize.left = screen.right - (winSize.right - winSize.left);
      winSize.right = screen.right;
   }
   if ((screen.bottom - winSize.bottom) < 0) {
      winSize.top = screen.bottom - (winSize.bottom - winSize.top);
      winSize.bottom = screen.bottom;
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

   CMenu *menu = theApp.menus.GetMenu(_T("Main"));
   theApp.menus.SetCheck(ID_OFFLINE, theApp.preferences.bOffline);

   if (!pFrame->CreateEx(styleEx, BROWSER_WINDOW_CLASS, strTitle, style,
      winSize, chromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT ? pParent : NULL,
	  (UINT)menu->GetSafeHmenu(), NULL))
   {
      TRACE0("Warning: failed to create CFrameWnd.\n");
      ReleaseMutex(theApp.m_hMutex);
      return FALSE;
   }

   pFrame->SetIcon(m_hMainIcon, true);
   pFrame->SetIcon(m_hSmallIcon, false);
   
   // Set accelerator only if it's not a chrome window.
   if (!(chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
      pFrame->m_hAccelTable = accel.GetTable();
   
   // this only needs to be called once
   if (!m_bFirstWindowCreated) {
      pFrame->m_wndReBar.DrawToolBarMenu();
#ifdef INTERNAL_SIDEBAR
      pFrame->m_wndSideBar.DrawSideBarMenu();
#endif
      m_bFirstWindowCreated = TRUE;
   }

   if (!preferences.bHideTaskBarButtons) {
      pFrame->ModifyStyleEx(0, WS_EX_APPWINDOW);
   }

   // XUL Dialog must be resized after loading.
   BOOL canResize = !theApp.preferences.GetBool("kmeleon.display.dontResizeXul", FALSE);
   BOOL sizeOnLoad = FALSE;
   if (canResize && chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
      sizeOnLoad = TRUE;

   pFrame->m_bSizeOnLoad = sizeOnLoad;
   // Show the window...
   if(bShowWindow) 
   {
       if (preferences.bMaximized && !(style & WS_POPUP) && 
           (chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE))
           pFrame->ShowWindow(SW_SHOWMAXIMIZED);
       else 
           pFrame->ShowWindow(SW_SHOW);
    
	   pFrame->SetForegroundWindow();
       pFrame->m_created = true;
   }
   else 
	   theApp.m_pMostRecentBrowserFrame = pOldRecentFrame;
   // Add to the list of BrowserFrame windows
   m_FrameWndLst.AddHead(pFrame);
   
   pFrame->m_ignoreMoveResize = 
      (theApp.preferences.GetBool("kmeleon.display.dontResizeNewWindow", FALSE)) && 
      (!sizeOnLoad && !(style & WS_POPUP)) ? 2 : 0;
   
   
   ReleaseMutex(m_hMutex);
   return pFrame;
}

void CMfcEmbedApp::OnNewBrowser()
{
   m_pOpenNewBrowserFrame = m_pMostRecentBrowserFrame;
   
   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();
   if(pBrowserFrame) {

      //Load the new window start page into the browser view
      pBrowserFrame->SetFocus();
      // pBrowserFrame->m_wndUrlBar.MaintainFocus();
      switch (preferences.iNewWindowOpenAs) {
      case PREF_NEW_WINDOW_CURRENT:
	     if (m_pOpenNewBrowserFrame)
			m_pOpenNewBrowserFrame->m_wndBrowserView.CloneSHistory(pBrowserFrame->m_wndBrowserView);
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

	  theApp.preferences.bNewWindowHasUrlFocus = theApp.preferences.GetBool("kmeleon.display.NewWindowHasUrlFocus", FALSE); 
	  if (theApp.preferences.bNewWindowHasUrlFocus)
		pBrowserFrame->m_wndUrlBar.MaintainFocus();
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
   // destroy the hidden window (which will post a WM_QUIT msg
   // since this is the main frame window) if we've
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
         else
            m_pMainWnd->DestroyWindow();
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
         pBrowserFrame->PostMessage(Msg, wParam, lParam);
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
   
   m_pMostRecentBrowserFrame = NULL; // In case plugins are weird
   
   if (m_pMainWnd)
      m_pMainWnd->DestroyWindow();

   delete m_MRUList;
   DestroyIcon(m_hMainIcon);
   DestroyIcon(m_hSmallIcon);

   // unload the plugins before we terminate embedding,
   // this way plugins can still call the preference functions
   plugins.SendMessage("*", "* Plugin Manager", "Quit");

   preferences.Save(true);
   
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

BOOL CMfcEmbedApp::InitializePrefs(){
   preferences.Load();
   //preferences.Save();

   return TRUE;
}

BOOL CMfcEmbedApp::InitializeMenusAccels(){
   CString filename;
	CMenuParser menusParser;

   filename = GetFolder(DefSettingsFolder) + _T("\\") ACCEL_CONFIG_FILE;
   accel.Load(filename);

   filename = GetFolder(DefSettingsFolder) + _T("\\") MENU_CONFIG_FILE;
   menusParser.Load(filename);
   
	plugins.SendMessage("*", "* Plugin Manager", "Init");
   
   filename = GetFolder(UserSettingsFolder) + _T("\\") ACCEL_CONFIG_FILE;
   accel.Load(filename);

   filename = GetFolder(UserSettingsFolder) + _T("\\") MENU_CONFIG_FILE;
   menusParser.Load(filename);

	plugins.SendMessage("*", "* Plugin Manager", "Setup");

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

      preferences.Save(true);
   }
   else if (strcmp(aTopic, "profile-after-change") == 0)
   {        
      // Only reinitialize everything if this is a profile switch, since this
      // called at start up and we already do evenything once already
      if (!wcscmp(someData, NS_LITERAL_STRING("switch").get())) {
         
         /* XXX Plugin that use global vars can't be unloaded/reloaded
            correctly.
         */
         
         plugins.SendMessage("*", "* Plugin Manager", "Quit");
         plugins.UnLoadAll();
         menus.Destroy();
         InitializePrefs();
         CheckProfileVersion();

         plugins.FindAndLoad();
         plugins.SendMessage("*", "* Plugin Manager", "Init");
         InitializeMenusAccels();
         plugins.SendMessage("*", "* Plugin Manager", "Setup");
         
         CBrowserFrame* browser;
         browser = CreateNewBrowserFrame();
         
         if (!browser) {
            MessageBox(NULL, _T("Could not create browser frame"), NULL, MB_OK);
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

   NS_ENSURE_ARG_POINTER(_retval);
   *_retval = 0;

      CWnd* pParent = NULL;
   if (parent) {
      HWND w;
      nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(parent));
      if (site) {
         site->GetSiteWindow(reinterpret_cast<void **>(&w));
         pParent = CWnd::FromHandle(w);
      }
   }

   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags, -1, -1, -1, -1, PR_FALSE, pParent);
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

int CMfcEmbedApp::GetID(const char *strID) {
   int val = 0;
   USES_CONVERSION;
   defineMap.Lookup(A2CT(strID), val);
   return val;
}

CString CMfcEmbedApp::GetFolder(FolderType folder)
{
   // You can call me lazy.
   switch (folder) {
      case RootFolder:
         return m_sRootFolder;

      case DefSettingsFolder:
         return preferences.settingsFolder;
      
      case UserSettingsFolder:
         return preferences.userSettingsFolder;
  
      case ProfileFolder:
         return preferences.profileFolder;

      case PluginsFolder:
         return preferences.pluginsFolder;
        
      case UserPluginsFolder:
         return preferences.userPluginsFolder; 

      case SkinsFolder:
         return preferences.skinsFolder;

      case UserSkinsFolder:
         return preferences.userSkinsFolder; 

      case ResFolder:
         return preferences.resFolder;

      case CurrentSkinFolder:
         return preferences.currentSkinFolder;
   }
   return _T("");
}

void CMfcEmbedApp::CheckProfileVersion()
{
   CString fileVersion = GetFolder(ProfileFolder) + _T("\\version.ini");
   BOOL needClean = FALSE;

	CString locale;
   locale.LoadString(IDS_LOCALE_ID);

   TCHAR version[34];
   _itot(KMELEON_VERSION, version, 10);
   
   int oldVersion = GetPrivateProfileInt(_T("Version"), _T("LastVersion"), 0, fileVersion);
   if (!oldVersion)
   {
      needClean = TRUE; // XXX This will be done even with a new profile
      WritePrivateProfileString(_T("Version"), _T("LastVersion"), version, fileVersion);
      WritePrivateProfileString(_T("Version"), _T("LastLocale"), locale, fileVersion);
   }
   else {
      if (oldVersion != KMELEON_VERSION) {
         needClean = TRUE;
         WritePrivateProfileString(_T("Version"), _T("LastVersion"), version, fileVersion);
      }

      if (locale.GetLength()) {
         TCHAR buf[10];
         GetPrivateProfileString(_T("Version"), _T("LastLocale"), version, buf, 10, fileVersion);
         if (_tcscmp(buf, locale) != 0) {
            nsresult rv = NS_OK;
            nsCOMPtr<nsIChromeRegistrySea> chromeRegistry =
               do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv)) {
               USES_CONVERSION;
               // XXX The first call to SelectLocale shouldn't be here. The current 
               // global locale should be stored elsewhere
               // rv = chromeRegistry->SelectLocale(nsEmbedCString(T2CA(locale)), PR_FALSE);
               rv |= chromeRegistry->SelectLocale(nsEmbedCString(T2CA(locale)), PR_TRUE);
               if (NS_SUCCEEDED(rv))
                  WritePrivateProfileString(_T("Version"), _T("LastLocale"), locale, fileVersion);
            }
         }
      }
   }

   if (needClean) {

       CString toDelete = GetFolder(ProfileFolder) + _T("compreg.dat");
       DeleteFile(toDelete);
       toDelete = GetFolder(ProfileFolder) + _T("xpti.dat");
       DeleteFile(toDelete);

       toDelete = GetMozDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR) + _T("\\xul.mfl"); 
       DeleteFile(toDelete);

       if (oldVersion < 0x01010001) {

           if (!theApp.preferences.skinsCurrent.IsEmpty() && 
		      theApp.preferences.skinsCurrent[theApp.preferences.skinsCurrent.GetLength() - 1] == '\\')
           {
              theApp.preferences.skinsCurrent.GetBuffer(0);
              theApp.preferences.skinsCurrent.ReleaseBuffer(theApp.preferences.skinsCurrent.GetLength()-1);
           }

           CString accelFile = GetFolder(UserSettingsFolder) + _T("\\") ACCEL_CONFIG_FILE;
           CString menuFile = GetFolder(UserSettingsFolder) + _T("\\") MENU_CONFIG_FILE;
           CString macroFile = GetFolder(UserSettingsFolder) + _T("\\") _T("macros.cfg");

           CFile file;
           if (file.Open(macroFile, CFile::modeRead, NULL)) {
               file.Close();

               CString title,msg;
               title.LoadString(IDS_KMELEON_UPDATE);
               msg.LoadString(IDS_UPDATE_11);

               int choice = MessageBox(NULL, msg, title, MB_ICONQUESTION|MB_YESNO);

               if (choice == IDNO) {
                   DeleteFile(menuFile);
                   DeleteFile(accelFile);
                   DeleteFile(macroFile);
               } 
               else {
                   CString backup = GetFolder(UserSettingsFolder) + _T("\\Backup\\");
                   BOOL backupSucceed = FALSE;
                   CreateDirectory(backup, NULL);
                   backupSucceed = MoveFile(menuFile, backup + MENU_CONFIG_FILE);
                   backupSucceed &= MoveFile(accelFile, backup + ACCEL_CONFIG_FILE);
                   backupSucceed &= MoveFile(macroFile, backup + _T("macros.cfg"));

                   if (backupSucceed) 
                       AfxMessageBox(IDS_UPDATE11_SUCCESS, MB_ICONINFORMATION|MB_OK);
                   else
                       AfxMessageBox(IDS_UPDATE11_FAIL, MB_ICONEXCLAMATION|MB_OK);

               }
           }
       }
   }
}
