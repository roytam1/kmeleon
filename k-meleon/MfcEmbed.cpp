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
#include "BrowserFrmTab.h"
#include "KmFileLocProvider.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"
#include "BrowserImpl.h"
#include "kmeleonConst.h"
#include "UnknownContentTypeHandler.h"
#include "MenuParser.h"
#include "kmeleon_plugin.h"
#include <io.h>
#include <fcntl.h>

#include "nsIIOService.h"
#include "nsIWindowWatcher.h"

static UINT WM_POSTEVENT = RegisterWindowMessage(_T("XPCOM_PostEvent"));
static UINT WM_FLASHRELAY = RegisterWindowMessage(_T("MozFlashUserRelay"));
static UINT WM_NSEVENTID = RegisterWindowMessage(_T("nsAppShell:EventID"));

XRE_InitEmbedding2Type XRE_InitEmbedding2 = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;
XRE_AddManifestLocationType XRE_AddManifestLocation = 0;
XRE_GetProcessTypeType XRE_GetProcessType = 0;

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
ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
ON_COMMAND(ID_NEW_BROWSER, OnNewBrowser)
ON_COMMAND(ID_MANAGE_PROFILES, OnManageProfiles)
ON_COMMAND(ID_PREFERENCES, OnPreferences)
ON_COMMAND(ID_OFFLINE, OnToggleOffline)
ON_UPDATE_COMMAND_UI(ID_OFFLINE, OnUpdateToggleOffline)
ON_UPDATE_COMMAND_UI_RANGE(WINDOW_MENU_START_ID, WINDOW_MENU_STOP_ID, OnUpdateWindows)
ON_COMMAND_RANGE(WINDOW_MENU_START_ID, WINDOW_MENU_STOP_ID, OnWindowSelect)

// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#include <Iads.h>
#include <activeds.h>
#include <comdef.h>
#include <initguid.h>
#include <accctrl.h>


BOOL RestartAsRestricted()
{
	HANDLE hProcessToken = NULL;
	HANDLE hRestrictedToken = NULL;
	PTOKEN_USER pstructUserToken = NULL;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_READ, &hProcessToken))
		return FALSE;

	DWORD dwLen = 0;
	DWORD dwInertFlag;
	if (!GetTokenInformation(hProcessToken, TokenSandBoxInert, &dwInertFlag, sizeof(dwInertFlag), &dwLen) || dwInertFlag!=0){
		CloseHandle(hProcessToken);
		return FALSE;
	}

	// XXX : Bad check using privilege count to be compatible with sandboxie
	BYTE buf[1024];
	if (!GetTokenInformation(hProcessToken, TokenPrivileges, &buf, 1024, &dwLen)) 
	{
		CloseHandle(hProcessToken);
		return FALSE;
	}
	TOKEN_PRIVILEGES *tp = (TOKEN_PRIVILEGES*)buf;
	if (tp->PrivilegeCount < 2) return FALSE;

	if(!CreateRestrictedToken(hProcessToken, DISABLE_MAX_PRIVILEGE | SANDBOX_INERT, 0, &pstructUserToken->User, 0, NULL, 0, NULL, &hRestrictedToken ) ){
		CloseHandle(hProcessToken);
		return FALSE;
	}

	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_NORMAL;

	HANDLE hMutexOneInstance = CreateMutex( NULL, TRUE, _T("K-Meleon Instance Mutex") );
	ReleaseMutex(hMutexOneInstance);

	BOOL res = CreateProcessAsUser(hRestrictedToken, NULL, ::GetCommandLine(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	
	HeapFree(GetProcessHeap(), 0, (LPVOID)pstructUserToken);
	pstructUserToken = NULL;
	CloseHandle(hRestrictedToken);
	CloseHandle(hProcessToken);
	if (!res) return FALSE;

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return TRUE;
}

CMfcEmbedApp::CMfcEmbedApp()
{
   mRefCnt = 1; // Start at one - nothing is going to addref this object

   m_ProfileMgr = NULL;
   m_bSwitchingProfiles = FALSE;

   m_bFirstWindowCreated = FALSE;
   m_pMostRecentBrowserFrame  = NULL;
   m_hResDll = NULL;
   m_MRUList = NULL;
   m_bRestart = FALSE;
}

CMfcEmbedApp theApp;

static NS_IMETHODIMP
RefreshPlugins(PRBool aReloadPages)
{
	return NS_OK;
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


bool CMfcEmbedApp::FindSkinFile( CString& szSkinFile, LPCTSTR filename, LPCTSTR skin, bool searchUser ) 
{
   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   CString file;

   if (!szSkinFile || !filename || !*filename)
      return false;
   
   if (searchUser) {
      file = GetFolder(UserSettingsFolder) + _T('\\') + filename;
      hFile = FindFirstFile(file, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
            FindClose(hFile);
            szSkinFile = file;
            return true;
      }   
   }

    CString tmp = skin ? skin : theApp.preferences.skinsCurrent;
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

    tmp = skin ? skin : theApp.preferences.skinsCurrent;
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

#define FIREFOX_CHROME

#ifdef FIREFOX_CHROME
BOOL CMfcEmbedApp::LoadLanguage()
{
	nsresult rv;
	CString locale = preferences.GetLocaleString("general.useragent.locale", _T(""));
	if (locale.IsEmpty())
		return FALSE;

   if (_tcsncmp(locale, _T("en"), 2) == 0) {
      if (m_hResDll) {
         FreeLibrary(m_hResDll);
		 m_hResDll = NULL;
	  }

	  lang.Reset();
	  AfxSetResourceHandle(m_hInstance);
#if _MSC_VER >= 1300 
      _AtlBaseModule.SetResourceInstance(m_hInstance);
#endif
	  return TRUE;
   }

   CString localeFolder = GetFolder(RootFolder) + CString("\\locales\\") + locale + CString("\\");
   CString resDll = localeFolder + CString("kmeleon.dll");

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

   CFile file;
   if (!file.Open(localeFolder + CString("kmeleon.kml"), CFile::modeRead, NULL)) {
      ::FreeLibrary(hInstResDll);
      return FALSE;
   }
   file.Close();
   
   CFileFind finder;
   CString langFile, pattern = localeFolder + CString("*.kml");
   BOOL bWorking = finder.FindFile(pattern);
   while (bWorking) {
      bWorking = finder.FindNextFile();
      langFile = finder.GetFilePath();
	  lang.Load(langFile);
   }

   AfxSetResourceHandle(hInstResDll);
#if _MSC_VER >= 1300 
   _AtlBaseModule.SetResourceInstance(hInstResDll);
#endif

   if (m_hResDll && m_hResDll!=hInstResDll)
      FreeLibrary(m_hResDll);
   m_hResDll = hInstResDll;

   return TRUE;
}

#else

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
#endif

// Initialize our MFC application and also init
// the Gecko embedding APIs
// Note that we're also init'ng the profile switching
// code here
// Then, create a new BrowserFrame and load our
// default homepage
//



BOOL CMfcEmbedApp::CheckInstance()
{
   HANDLE hMutexOneInstance = CreateMutex( NULL, FALSE, _T("K-Meleon Instance Mutex") );
   DWORD dwWaitResult = WaitForSingleObject( hMutexOneInstance, 0);
   if (dwWaitResult == WAIT_OBJECT_0)
     return TRUE;

   // if another instance is already running, pass it our command line paramaters,
   // and ask it to open a new window
   // eventually, we should handle this through DDE
  
      // find the hidden window
      if (HWND hwndPrev = ::FindWindowEx(NULL, NULL, HIDDEN_WINDOW_CLASS, NULL) ) {

         if(*m_lpCmdLine) {
            COPYDATASTRUCT copyData;
            copyData.dwData = 0;
			copyData.cbData = (strlen(cmdline.m_sCmdLine)+1)*sizeof(char);
            copyData.lpData = (void *) cmdline.m_sCmdLine;
	           SendMessage(hwndPrev, WM_COPYDATA, NULL, (LPARAM) &copyData);
         }
         else
            SendMessage(hwndPrev, UWM_NEWWINDOW, NULL, NULL);
      }
      return FALSE;   
}

#define PROCESS_DEP_ENABLE                          0x00000001
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION     0x00000002

BOOL CMfcEmbedApp::InitInstance()
{
   USES_CONVERSION;
   cmdline.Initialize(T2A(m_lpCmdLine));

   // check for prior instances
   m_bAlreadyRunning = FALSE;
   if (cmdline.GetSwitch("-new", NULL, TRUE)<0 && !CheckInstance())
	   return FALSE;

   // Security 
#ifdef _UNICODE
   if (cmdline.GetSwitch("-norestrict", NULL, TRUE)<0 && RestartAsRestricted())
   {
		m_bAlreadyRunning = TRUE;		
		return FALSE;
   }
#endif

#ifndef _DEBUG

   HMODULE hMod = GetModuleHandleW(L"Kernel32.dll");
   if (hMod)
   {
      typedef BOOL (WINAPI *PSETDEP)(DWORD);
      PSETDEP procSet = (PSETDEP)GetProcAddress(hMod,"SetProcessDEPPolicy");
      if (procSet) procSet(PROCESS_DEP_ENABLE);
   }
#endif

   OleInitialize(NULL);

   // Profile selection
   int len = cmdline.GetSwitch("-P", NULL, FALSE);
   char *profile = NULL;
   if (len == 0) {
      cmdline.GetSwitch("-P", NULL, TRUE);  // remove the flag from the command line
   }
   else if (len > 0) {
      profile = new char[len+1];
      cmdline.GetSwitch("-P", profile, TRUE);
   }

#ifndef FIREFOX_CHROME
   LoadLanguage();
#endif

#ifdef _DEBUG
	ShowDebugConsole();
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	CString xul = GetFolder(RootFolder)+_T("\\xul.dll");
	len = WideCharToMultiByte(CP_UTF8, 0, xul, -1, NULL, 0, NULL, NULL);
	char* gnah = new char[len+1];
	WideCharToMultiByte(CP_UTF8, 0, xul, -1, gnah, len, NULL, NULL);
	if (NS_FAILED(XPCOMGlueStartup(gnah))) {
		AfxMessageBox(IDS_START_FAILED, MB_OK | MB_ICONERROR);
		//MessageBox(NULL, _T("Could not initialize XPCOM. Perhaps the GRE\nis not installed or could not be found?"), _T("K-Meleon"), MB_OK | MB_ICONERROR);
        return FALSE;
    }
	delete [] gnah;

   m_hMutex = CreateMutex(NULL, FALSE, NULL);

	// load XUL functions
    nsDynamicFunctionLoad nsFuncs[] = {
            {"XRE_InitEmbedding2", (NSFuncPtr*)&XRE_InitEmbedding2},
            {"XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
            {"XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
            {"XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory},
			{"XRE_AddManifestLocation", (NSFuncPtr*)&XRE_AddManifestLocation},
			{"XRE_GetProcessType",  (NSFuncPtr*)&XRE_GetProcessType},
            {0, 0}
    };

	nsresult rv = XPCOMGlueLoadXULFunctions(nsFuncs);
	NS_ENSURE_SUCCESS(rv, FALSE);
   
	// Set provider
   CString strRes;
   strRes.LoadString(IDS_PROFILES_FOLDER_NAME);
   KmFileLocProvider *provider = new KmFileLocProvider(nsDependentString(T2W(strRes.GetBuffer(0))));
   if(!provider) return FALSE;
      
   // Set app directory
   nsCOMPtr<nsIFile> mreAppDir;
   rv = NS_NewLocalFile(nsDependentString(theApp.GetFolder(RootFolder)), TRUE, getter_AddRefs(mreAppDir));
   NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create mreAppDir file");

   nsCOMPtr<nsIFile> appSubdir;
   rv = NS_NewLocalFile(nsDependentString(theApp.GetFolder(AppFolder)), TRUE, getter_AddRefs(appSubdir));
   NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create appSubdir file");

   // Set Profile
   m_ProfileMgr = new CProfileMgr;
   if (!m_ProfileMgr) return FALSE;

   BOOL result;
   result = m_ProfileMgr->StartUp(provider, profile ? A2CT(profile) : nullptr);
   if (profile) delete [] profile;

   ASSERT(result);
   if (!result) return FALSE;   
   
   // Init XRE
   rv = XRE_InitEmbedding2(mreAppDir, appSubdir, provider);
   NS_ENSURE_SUCCESS(rv, FALSE);
      
   rv = InitializeWindowCreator();
   if (NS_FAILED(rv)) 
   {
      ASSERT(FALSE);
	  XRE_TermEmbedding();
      return FALSE;
   }

   // Minimal unicode support
   #ifndef _UNICODE
   OSVERSIONINFO osinfo;
   osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osinfo);
   m_bUnicode = (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
   #endif

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

   // Register chrome language
   CString localesFolder = GetFolder(RootFolder) + CString(_T("\\locales\\*"));
   WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(localesFolder.GetBuffer(0), &ffd);
	localesFolder.Truncate(localesFolder.GetLength()-1);
	if (hFind != INVALID_HANDLE_VALUE) {            
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
				ffd.cFileName[0]!='.') {
					nsCOMPtr<nsIFile> mFile;
					NS_NewLocalFile(CStringToNSString(localesFolder + ffd.cFileName + _T("\\") + ffd.cFileName + _T(".manifest")), false, getter_AddRefs(mFile));
					XRE_AddManifestLocation(NS_COMPONENT_LOCATION, mFile);
			}
		} while ( FindNextFile(hFind, &ffd) );
		FindClose(hFind);
	}   

   XRE_NotifyProfile();

   // These have to be done in this order!
   InitializeDefineMap();
   InitializePrefs();
   SetOffline(theApp.preferences.bOffline);

#ifdef FIREFOX_CHROME
   LoadLanguage();
#endif
   
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

#ifdef INTERNAL_SITEICONS
   // Create the favicon list
   if (preferences.bSiteIcons)
		favicons.Create(16,16,ILC_COLOR32|ILC_MASK,25,100);
#endif
   
   // Initialize plugins
   plugins.FindAndLoad();
   plugins.SendMessage("*", "* Plugin Manager", "Init2");
   InitializeMenusAccels();

   // the hidden window will take care of creating the first
   // browser window for us
   if(!CreateHiddenWindow()){
      ASSERT(FALSE);
	  XRE_TermEmbedding();
      //NS_TermEmbedding();
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

CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrameWithUrl(LPCTSTR pUrl, LPCTSTR refferer,
							BOOL bBackground, 
							CWnd* pParent)
{
	if (!pUrl) return NULL;

	CBrowserFrame* pFrame;
	const TCHAR* ext = _tcschr(pUrl, L'.');
	
	if (ext && (_tcsstr(ext, _T(".xul")) == ext) &&
		(_tcsncmp(pUrl, _T("chrome:"), 7) == 0)) {
	   pFrame = CreateNewChromeDialog(pUrl, pParent);
		}
	else {
		pFrame = CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, bBackground, pParent);
		pFrame->OpenURL(pUrl, refferer, FALSE);
		pFrame->ShowWindow(SW_SHOW);
	}
	return pFrame;
}
/*
	CDialog* diag = new CDialog();
	diag->CreateIndirect((LPCDLGTEMPLATE)tplGenericDlg, pParent);

	CBrowserView* view = new CBrowserView();
	
	if (!view->CreateEx(0, NULL, NULL, WS_CHILD|WS_VISIBLE,
        CRect(0, 0, 0, 0), diag, AFX_IDW_PANE_FIRST, NULL)) return 0;
	view->OpenURL(url);
	diag->ShowWindow(SW_SHOW);
	return diag;
	*/

CBrowserFrame* CMfcEmbedApp::CreateNewChromeDialog(LPCTSTR url, CWnd* pParent)
{
	// Check if it's not already open
	POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
	CBrowserFrame* pBrowserFrame = NULL;
	while( pos != NULL ) {
		pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
		if(pBrowserFrame->IsDialog() && pBrowserFrame->GetActiveView()->GetCurrentURI().Compare(url) == 0) {
			pBrowserFrame->ActivateFrame();
			return pBrowserFrame;
		}
	}

	//XXXX We have to make a real Dialog!
	PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                     nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                     nsIWebBrowserChrome::CHROME_TITLEBAR |
                     nsIWebBrowserChrome::CHROME_OPENAS_CHROME|
                     nsIWebBrowserChrome::CHROME_WINDOW_MIN;
	
	CBrowserFrame* pFrame = CreateNewBrowserFrame(chromeMask, FALSE, pParent);
	pFrame->OpenURL(url);
	return pFrame;
}

CBrowserFrame* CMfcEmbedApp::CreateNewBrowserFrame(PRUint32 chromeMask,
                                                   BOOL inBackground,
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

   BOOL isPopupOrDialog = FALSE;
   
   // XXX Chrome dialogs shouldn't have thoses.
   if (chromeMask & nsIWebBrowserChrome::CHROME_OPENAS_CHROME) {
      chromeMask &=  ~nsIWebBrowserChrome::CHROME_MENUBAR
	                &~nsIWebBrowserChrome::CHROME_TOOLBAR
					&~nsIWebBrowserChrome::CHROME_LOCATIONBAR
					&~nsIWebBrowserChrome::CHROME_STATUSBAR
					&~nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR;

	  isPopupOrDialog = TRUE;
   }
	 
   
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
		!(chromeMask & nsIWebBrowserChrome::CHROME_TOOLBAR)) ) {
			isPopupOrDialog = TRUE;  
			style |= WS_POPUP; // XXX
		}

   if (!isPopupOrDialog && (preferences.bMaximized || 
      (m_pMostRecentBrowserFrame && m_pMostRecentBrowserFrame->IsZoomed())) &&
      ((chromeMask & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) || (chromeMask & nsIWebBrowserChrome::CHROME_ALL)))
      style |= WS_MAXIMIZE;

   RECT screen, winSize;
   SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);
   int screenWidth   = screen.right - screen.left;
   int screenHeight  = screen.bottom - screen.top;
/*
   if (x>0 && y>0 && cx>0 && cy>0) {
      winSize.left = x;
      winSize.top = y;
      winSize.right = x + cx;
      winSize.bottom = y + cy;
      AdjustWindowRectEx(&winSize, style, chromeMask & (nsIWebBrowserChrome::CHROME_MENUBAR), 0);
   }
   else {*/

  
   // If the last active window is not a popup use cascading placement
   //if (m_pMostRecentBrowserFrame) { 

      CBrowserFrame* pCascadeWnd = NULL;
	  if (inBackground && !m_FrameWndLst.IsEmpty()) {
         pCascadeWnd = (CBrowserFrame*)m_FrameWndLst.GetHead();
		 if (pCascadeWnd->IsPopup() || pCascadeWnd->IsDialog())
			 pCascadeWnd = NULL;
	  }
		 
	  if (!pCascadeWnd && m_pMostRecentBrowserFrame) {
	     pCascadeWnd = m_pMostRecentBrowserFrame;
	     if (pCascadeWnd->IsPopup() || pCascadeWnd->IsDialog())
			 pCascadeWnd = NULL;
	  }
      
	  if (pCascadeWnd)
      {
         WINDOWPLACEMENT wp;
         wp.length = sizeof(WINDOWPLACEMENT);
         pCascadeWnd->GetWindowPlacement(&wp);

 	     // if the window is not maximized, let's use use GetWindowRect, which works
         if (wp.showCmd == SW_SHOWNORMAL)
            pCascadeWnd->GetWindowRect(&wp.rcNormalPosition);

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
   

   // don't create windows larger than the screen
   if ((winSize.right - winSize.left) > screenWidth)
      winSize.right = screenWidth - winSize.left;
   if ((winSize.bottom - winSize.top) > screenHeight)
      winSize.bottom = screenHeight - winSize.top;
           
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
   CBrowserFrame* pFrame = NULL;
   if (isPopupOrDialog || preferences.GetBool("kmeleon.notab", FALSE))
       pFrame = new CBrowserFrame(chromeMask, style);
   else
       pFrame = (CBrowserFrame*)new CBrowserFrmTab(chromeMask, style);

   // this backup is made as part of a bad workaround:
   // m_pMostRecentBrowserFrame needs to be this frame for the life of this function so that
   // things like plugins and the rebar sizes can access it, but
   // m_pMostRecentBrowserFrame should not stay set to this if this window
   // is hidden (usually meaning that it is created using the open link in background option)
   // so the backup is made, and m_pMostRecentBrowserFrame will be restored
   // at the end of this function
   CBrowserFrame* pOldRecentFrame = m_pMostRecentBrowserFrame;   
   m_pMostRecentBrowserFrame = pFrame;   
   if (!pOldRecentFrame)
      pOldRecentFrame = pFrame;

   CMenu *menu = theApp.menus.GetMenu(_T("Main"));

   if (!pFrame->CreateEx(styleEx, BROWSER_WINDOW_CLASS, strTitle, style,
      winSize, chromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT ? pParent : NULL,
	  (UINT)menu->GetSafeHmenu(), NULL))
   {
      TRACE0("Warning: failed to create CFrameWnd.\n");
      ReleaseMutex(theApp.m_hMutex);
	  m_pMostRecentBrowserFrame = pOldRecentFrame;
	  delete pFrame;
      return FALSE;
   }

   pFrame->SetIcon(m_hMainIcon, true);
   pFrame->SetIcon(m_hSmallIcon, false);
   
   // this only needs to be called once
   if (!m_bFirstWindowCreated) {
      KmMenu* menu = menus.GetKMenu(_T("@Toolbars"));
      if (menu) menu->Invalidate();
#ifdef INTERNAL_SIDEBAR
      menu = menus.GetKMenu(_T("@Sidebars"));
      if (menu) menu->Invalidate();
#endif
      m_bFirstWindowCreated = TRUE;
   }
	
   if (!preferences.bHideTaskBarButtons)
      pFrame->ModifyStyleEx(0, WS_EX_APPWINDOW);

   if (preferences.GetBool("kmeleon.display.hideTitleBar", FALSE) && !isPopupOrDialog) {
      pFrame->ModifyStyle(WS_CAPTION, 0 , SWP_DRAWFRAME);
	  if (pFrame->IsZoomed())
		  pFrame->ModifyStyle(WS_THICKFRAME, 0);
   }

   if (inBackground) {
      pFrame->SetWindowPos((CWnd*)theApp.m_FrameWndLst.GetHead(),
         0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
      theApp.m_pMostRecentBrowserFrame = pOldRecentFrame;
   } else if (!isPopupOrDialog) {
	   theApp.m_pMostRecentBrowserFrame = pOldRecentFrame;
   }
   
   // Add to the list of BrowserFrame windows
   m_FrameWndLst.AddHead(pFrame);
   ReleaseMutex(m_hMutex);
   return pFrame;
}

void CMfcEmbedApp::OnAppAbout()
{
	CBrowserFrame* pFrm = CreateNewBrowserFrame(
					 nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                     nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                     nsIWebBrowserChrome::CHROME_TITLEBAR |
					 nsIWebBrowserChrome::CHROME_SCROLLBARS, 
					 FALSE);
	
	if (!pFrm) return;
	pFrm->OpenURL(_T("about:"));
	pFrm->ShowWindow(SW_SHOW);
}

void CMfcEmbedApp::OnNewBrowser()
{/*
	 if (m_pMostRecentBrowserFrame) {
		  ((CBrowserFrmTab*)m_pMostRecentBrowserFrame)->OnNewTab();
		  return;
	  }*/

   m_pOpenNewBrowserFrame = m_pMostRecentBrowserFrame;
   BOOL urlFocus = theApp.preferences.GetBool("kmeleon.display.NewWindowHasUrlFocus", FALSE);

   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame();
   if(pBrowserFrame) {

      //Load the new window start page into the browser view
      switch (preferences.iNewWindowOpenAs) {
      case PREF_NEW_WINDOW_CURRENT:
         if (m_pOpenNewBrowserFrame) {
            pBrowserFrame->OpenURL(m_pOpenNewBrowserFrame->GetActiveView()->GetCurrentURI(), NULL, urlFocus);
            m_pOpenNewBrowserFrame->GetActiveView()->CloneBrowser(pBrowserFrame->GetActiveView());
		 }
         break;
      case PREF_NEW_WINDOW_HOME:
         pBrowserFrame->GetActiveView()->LoadHomePage();
         break;
      case PREF_NEW_WINDOW_BLANK:
         pBrowserFrame->OpenURL(_T("about:blank"), NULL, urlFocus);
         break;
	  case PREF_NEW_WINDOW_URL: {
	     CString newUrl = preferences.newWindowURL;
         if (newUrl.IsEmpty())
            pBrowserFrame->OpenURL(_T("about:blank"), NULL, urlFocus);
         else
			pBrowserFrame->OpenURL(newUrl, NULL, urlFocus);
         break;
      }
	  }

	  pBrowserFrame->ShowWindow(SW_SHOW);
	  
	  if (theApp.preferences.bNewWindowHasUrlFocus)
		pBrowserFrame->m_wndUrlBar.SetFocus();
   }
}

void CMfcEmbedApp::OnToggleOffline()
{
	BroadcastMessage(WM_COMMAND, ID_NAV_STOP, (LPARAM) 0);

    SetOffline(!theApp.preferences.bOffline);
    //theApp.menus.SetCheck(ID_OFFLINE, theApp.preferences.bOffline);

	CString status;
	status.LoadString( theApp.preferences.bOffline ? IDS_OFFLINE : IDS_ONLINE );

	m_pMostRecentBrowserFrame->UpdateStatus(status); 
}

void CMfcEmbedApp::OnUpdateToggleOffline(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(preferences.bOffline);
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
   UpdateWindowListMenu();

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

void CMfcEmbedApp::UpdateWindowListMenu()
{
   KmMenu* menu = menus.GetKMenu(_T("@WindowList"));
   if (menu) menu->Invalidate();
}

void CMfcEmbedApp::DrawWindowListMenu(HMENU menu)
{
   CBrowserFrame* pBrowserFrame = NULL;
   POSITION pos = m_FrameWndLst.GetHeadPosition();
   int i = 0;
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
      CBrowserWrapper* wrapper = pBrowserFrame->GetActiveView()->GetBrowserWrapper();
      if (!wrapper) continue;

      CString title = wrapper->GetTitle();
	  if (title.IsEmpty())
		  title = wrapper->GetURI();

	  AppendMenu(menu, MF_ENABLED | MF_STRING | ((pBrowserFrame == m_pMostRecentBrowserFrame) ? MF_CHECKED : 0), WINDOW_MENU_START_ID + i++ , title);
   }
}

void CMfcEmbedApp::OnUpdateWindows(CCmdUI *pCmd)
{
   POSITION pos = m_FrameWndLst.FindIndex(pCmd->m_nID - WINDOW_MENU_START_ID);
   if (pos == NULL) return;

   CBrowserFrame* pBrowserFrame = (CBrowserFrame *)m_FrameWndLst.GetAt(pos);
   if (m_pMostRecentBrowserFrame == pBrowserFrame) 
      pCmd->SetCheck(1);
   else
      pCmd->SetCheck(0);
}

void CMfcEmbedApp::OnWindowSelect(UINT id)
{
   POSITION pos = m_FrameWndLst.FindIndex(id - WINDOW_MENU_START_ID);
   if (pos == NULL) return;

   CBrowserFrame* pBrowserFrame = (CBrowserFrame *)m_FrameWndLst.GetAt(pos);
   if (pBrowserFrame) pBrowserFrame->BringWindowToTop();
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
		 pBrowserFrame->DestroyWindow();
   }
   m_FrameWndLst.RemoveAll();
   
   m_pMostRecentBrowserFrame = NULL; // In case plugins are weird
   
   if (m_pMainWnd)
      m_pMainWnd->DestroyWindow();

   // unload the plugins before we terminate embedding,
   // this way plugins can still call the preference functions
   plugins.SendMessage("*", "* Plugin Manager", "Quit");

   delete m_MRUList;
   DestroyIcon(m_hMainIcon);
   DestroyIcon(m_hSmallIcon);

   favicons.WriteCache();

   preferences.Flush();
   if (m_ProfileMgr) {
      m_ProfileMgr->ShutDownCurrentProfile( theApp.preferences.bGuestAccount );
      delete m_ProfileMgr;
   }
   
   if (XRE_TermEmbedding) XRE_TermEmbedding();

   // In case a plugin is a component, must be unloaded last.
   plugins.UnLoadAll();
   if (m_hResDll) FreeLibrary(m_hResDll);

   OleUninitialize();

   if (m_bRestart) {
		PROCESS_INFORMATION pi = {0};
		STARTUPINFO si = {0};
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_NORMAL;
		::CreateProcess(NULL, ::GetCommandLine(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
   }

   return 1;
}

BOOL CMfcEmbedApp::OnIdle(LONG lCount)
{
   CWinApp::OnIdle(lCount);
   
   //NS_DoIdleEmbeddingStuff();
   /*
   CBrowserFrame* pBrowserFrame = NULL;
   POSITION pos = m_FrameWndLst.GetHeadPosition();
   BOOL visible = FALSE;
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
	  visible = visible || pBrowserFrame->IsWindowVisible();
   }
   if (!visible) {
		ASSERT(FALSE);
		pBrowserFrame->ShowWindow(SW_SHOW);
   }*/
      
   return FALSE;
}

BOOL CMfcEmbedApp::IsIdleMessage( MSG* pMsg )
{
   if (!CWinApp::IsIdleMessage( pMsg ) ||      
      pMsg->message == WM_NSEVENTID || 
	  pMsg->message == WM_FLASHRELAY || 
	  pMsg->message == WM_USER + 1 || // Flash
	  pMsg->message == WM_USER + 12 || // Flash
	  pMsg->message == WM_USER + 112 || // Flash
      pMsg->message == WM_POSTEVENT ||
      pMsg->message == WM_TIMER) 

      return FALSE;
   else
      return TRUE;
}

void CMfcEmbedApp::OnPreferences () {
	CreateNewChromeDialog(_T("chrome://kmprefs/content/pref.xul"));
   //CPreferencesDlg prefDlg;
   //prefDlg.DoModal();
}

void CMfcEmbedApp::OnManageProfiles()
{
	CProfilesDlg dialog(m_ProfileMgr);
	dialog.DoModal();
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
	plugins.SendMessage("*", "* Plugin Manager", "Setup2");
   
   filename = GetFolder(UserSettingsFolder) + _T("\\") ACCEL_CONFIG_FILE;
   accel.Load(filename);

   filename = GetFolder(UserSettingsFolder) + _T("\\") MENU_CONFIG_FILE;
   menusParser.Load(filename);

	plugins.SendMessage("*", "* Plugin Manager", "Setup");
	plugins.SendMessage("*", "* Plugin Manager", "UserSetup");

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
   nsCOMPtr<nsIWindowCreator> windowCreator(static_cast<nsIWindowCreator *>(this));
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
   
   //USES_CONVERSION;

   //if (strcmp(aTopic, "profile-approve-change") == 0 &&
   //    (!someData ||
   //     strcmp(W2A(someData), "shutdown-cleanse") != 0 &&
   //     strcmp(W2A(someData), "shutdown-persist") != 0))
   //{
   //   // Ask the user if they want to
   //   int result = AfxMessageBox(IDS_PROFILE_SWITCH, MB_YESNO | MB_ICONQUESTION, 0);
   //   if (result != IDYES)
   //   {
   //      nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
   //      NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
   //      status->VetoChange();
   //   }
   //}
   //else if (strcmp(aTopic, "profile-change-teardown") == 0)
   //{
   //   // Close all open windows. Alternatively, we could just call CBrowserWrapper::Stop()
   //   // on each. Either way, we have to stop all network activity on this phase.
   //   
   //   POSITION pos = m_MiscWndLst.GetHeadPosition();
   //   while( pos != NULL )
   //   {
   //      CProgressDialog *pDlg = (CProgressDialog *) m_MiscWndLst.GetNext(pos);
   //      if (pDlg)
   //         pDlg->Cancel();
   //   }
   //   
   //   m_bSwitchingProfiles = TRUE;
   //   pos = m_FrameWndLst.GetHeadPosition();
   //   while( pos != NULL ) {
   //      CBrowserFrame *pBrowserFrame = (CBrowserFrame *) m_FrameWndLst.GetNext(pos);
   //      if(pBrowserFrame)
   //         pBrowserFrame->SendMessage(WM_CLOSE);
   //   }
   //   m_bSwitchingProfiles = FALSE;

   //   preferences.Flush();
   //}
   //else if (strcmp(aTopic, "profile-after-change") == 0)
   //{        
   //   // Only reinitialize everything if this is a profile switch, since this
   //   // called at start up and we already do evenything once already
   //   if (!wcscmp(someData, NS_LITERAL_STRING("switch").get())) {
   //      
   //      /* XXX Plugin that use global vars can't be unloaded/reloaded
   //         correctly.
   //      */
   //      
   //      plugins.SendMessage("*", "* Plugin Manager", "Quit");
   //      plugins.UnLoadAll();
   //      menus.Destroy();
   //      InitializePrefs();
   //      CheckProfileVersion();

   //      plugins.FindAndLoad();
   //      plugins.SendMessage("*", "* Plugin Manager", "Init");
   //      InitializeMenusAccels();
   //      plugins.SendMessage("*", "* Plugin Manager", "Setup");
   //      
   //      CBrowserFrame* browser;
   //      browser = CreateNewBrowserFrame();
   //      
   //      if (!browser) {
   //         MessageBox(NULL, _T("Could not create browser frame"), NULL, MB_OK);
   //         m_pMainWnd->PostMessage(WM_QUIT);
   //         return NS_ERROR_FAILURE;
   //      }
   //      
   //      browser->SetFocus();
   //      // browser->m_wndUrlBar.MaintainFocus();
   //      browser->GetActiveView()->LoadHomePage();
   //   }
   //}
   
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
/*
   if (!pParent) {
	   // This means a popup opened when closing a window. NEVER >-]
	   return NS_ERROR_FAILURE;
   }*/

  //nsCOMPtr<nsIWindowCreator> browserChrome(do_QueryInterface(parent));
  //return browserChrome->CreateChromeWindow(parent, chromeFlags, _retval);

   CBrowserFrame *pBrowserFrame = CreateNewBrowserFrame(chromeFlags, FALSE, pParent);
   if(pBrowserFrame) {//XXXX
      *_retval = static_cast<nsIWebBrowserChrome *>(pBrowserFrame->GetActiveView()->GetBrowserWrapper()->mpBrowserImpl);
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
      case AppFolder:
		  return GetFolder(RootFolder) + _T("\\") + _T("browser");

      case RootFolder:
         if (m_sRootFolder.IsEmpty()) {
            TCHAR path[_MAX_PATH+1];
            ::GetModuleFileName(0, path, _MAX_PATH);
            TCHAR* lastSlash = _tcsrchr(path, _T('\\'));
            if (!lastSlash) return FALSE;
            *lastSlash = _T('\0');
            m_sRootFolder = path;
         }
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
   }

   if (needClean) {

       CString toDelete = GetFolder(ProfileFolder) + _T("compreg.dat");
       DeleteFile(toDelete);
       toDelete = GetFolder(ProfileFolder) + _T("xpti.dat");
       DeleteFile(toDelete);
	   toDelete = GetMozDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR) + _T("\\") + _T("startupCache") + _T("\0\0");
	   TCHAR path[MAX_PATH] = {0};
	   _tcsncpy(path, toDelete, MAX_PATH-2);
	   SHFILEOPSTRUCT fop = {0};
	   fop.wFunc = FO_DELETE | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	   fop.pFrom = path;
	   SHFileOperation(&fop);

       toDelete = GetMozDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR) + _T("\\xul.mfl"); 
       DeleteFile(toDelete);
		
	   if (oldVersion < 0x01060002)
		   if (theApp.preferences.GetBool(PREFERENCE_REBAR_BOTTOM, FALSE))
			   theApp.preferences.SetString(PREFERENCE_REBAR_POSITION, _T("bottom"));

       if (oldVersion < 0x01050025) {
		   CString oldPref = theApp.preferences.GetString("kmeleon.general.homePage", _T(""));
		   if (!oldPref.IsEmpty()) theApp.preferences.SetString("browser.startup.homepage", oldPref);
	   }

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

int CMfcEmbedApp::Run()
{
	if (m_pMainWnd == NULL && AfxOleGetUserCtrl())
	{
		// Not launched /Embedding or /Automation, but has no main window!
		TRACE(traceAppMsg, 0, "Warning: m_pMainWnd is NULL in CWinApp::Run - quitting application.\n");
		AfxPostQuitMessage(0);
	}
		
	ASSERT_VALID(this);
	_AFX_THREAD_STATE* pState = AfxGetThreadState();

	// for tracking the idle time state
	BOOL bIdle = TRUE;
	LONG lIdleCount = 0;

	// acquire and dispatch messages until a WM_QUIT message is received.
	for (;;)
	{
		// phase1: check to see if we can do idle work
		while (bIdle &&
			!::PeekMessage(&(pState->m_msgCur), NULL, NULL, NULL, PM_NOREMOVE))
		{
			// call OnIdle while in bIdle state
			if (!OnIdle(lIdleCount++))
				bIdle = FALSE; // assume "no idle" state
		}

		// phase2: pump messages while available
		do
		{
			while (::PeekMessage(&(pState->m_msgCur), NULL, NULL, WM_USER, PM_NOREMOVE))
			{
				if (!PumpMessage2(WM_USER))
					return ExitInstance();

				// reset "no idle" state after pumping "normal" message
				//if (IsIdleMessage(&m_msgCur))
				if (IsIdleMessage(&(pState->m_msgCur)))
				{
					bIdle = TRUE;
					lIdleCount = 0;
				}

			}
			// pump message, but quit on WM_QUIT
			if (!PumpMessage2())
				return ExitInstance();
		} while (::PeekMessage(&(pState->m_msgCur), NULL, NULL, NULL, PM_NOREMOVE));

	}
}

BOOL CMfcEmbedApp::PumpMessage2(UINT filter)
{
	_AFX_THREAD_STATE *pState = AfxGetThreadState();

	if (!::GetMessage(&(pState->m_msgCur), NULL, NULL, filter))
	{
#ifdef _DEBUG
		TRACE(traceAppMsg, 1, "CWinThread::PumpMessage - Received WM_QUIT.\n");
			pState->m_nDisablePumpCount++; // application must die
#endif
		// Note: prevents calling message loop things in 'ExitInstance'
		// will never be decremented
		return FALSE;
	}

#ifdef _DEBUG
  if (pState->m_nDisablePumpCount != 0)
	{
	  TRACE(traceAppMsg, 0, "Error: CWinThread::PumpMessage called when not permitted.\n");
	  ASSERT(FALSE);
	}
#endif

  // process this message

	if (pState->m_msgCur.message != WM_KICKIDLE && !AfxPreTranslateMessage(&(pState->m_msgCur)))
	{
		::TranslateMessage(&(pState->m_msgCur));
		::DispatchMessage(&(pState->m_msgCur));
	}
  return TRUE;
}

