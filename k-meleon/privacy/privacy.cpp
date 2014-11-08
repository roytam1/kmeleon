/*
*  Copyright (C) 2004 Romain Vallet
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define _WIN32_WINNT 0x0500

#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include "privacy_res.h"

#define KMELEON_PLUGIN_EXPORTS
#include "..\KMeleonConst.h"
#include "..\kmeleon_plugin.h"
#include "..\Utils.h"
#include "..\LocalesUtils.h"
Locale* gLoc = NULL;

#define PLUGIN_NAME "Privacy Plugin"

#define PREFERENCE_CACHE_PARENTDIR      "browser.cache.disk.parent_directory"
#define PREFERENCE_MRU_MAXURLS          "kmeleon.MRU.maxURLs"
#define PREFERENCE_MRU_URL              "kmeleon.MRU.URL"
#define PREFERENCE_SIGNON_FILE          "signon.SignonFileName"

#define PREFERENCE_CLEARCOOKIES  "kmeleon.plugins.privacy.clearCookies"
#define PREFERENCE_CLEARCACHE    "kmeleon.plugins.privacy.clearCache"
#define PREFERENCE_CLEARMRU      "kmeleon.plugins.privacy.clearMRU"
#define PREFERENCE_CLEARSIGNON   "kmeleon.plugins.privacy.clearSignOn"
#define PREFERENCE_CLEARHISTORY  "kmeleon.plugins.privacy.clearHistory"

#define PRV_ONSTARTUP 1
#define PRV_ONSHUTDOWN 2

char settingsDir[MAX_PATH];        // settings directory
TCHAR cacheParentDir[MAX_PATH];     // cache parent directory
TCHAR signonFileName[MAX_PATH];     // sign on file

// Commands IDs
UINT cmdClearCookies;
UINT cmdClearCache;
UINT cmdClearMRU;
UINT cmdClearSignon;
UINT cmdClearHistory;
UINT cmdConfig;

// Preferences
INT prefClearCookies = 0;
INT prefClearCache = 0;
INT prefClearMRU = 0;
INT prefClearSignon = 0;
INT prefClearHistory = 0;

HWND hMainWindow;

BOOL APIENTRY DllMain (
        HANDLE hModule,
        DWORD ul_reason_for_call,
        LPVOID lpReserved)
{
  return TRUE;
}

LRESULT CALLBACK WndProc (
        HWND hWnd, UINT message,
        WPARAM wParam,
        LPARAM lParam);
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void * KMeleonWndProc;

INT Load();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void Exit();
void DoMenu(HMENU menu, char *param);
LONG DoMessage(LPCSTR to, LPCSTR from, LPCSTR subject, LONG data1, LONG data2);
void DoRebar(HWND rebarWnd);
INT DoAccel(LPSTR param);

kmeleonPlugin kPlugin =
{
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};
kmeleonFunctions *kFuncs;

// Global variables initialization
void InitGlobals()
{
    cmdClearCookies = kFuncs->GetCommandIDs(1);
    cmdClearCache = kFuncs->GetCommandIDs(1);
    cmdClearMRU = kFuncs->GetCommandIDs(1);
    cmdClearSignon = kFuncs->GetCommandIDs(1);
    cmdClearHistory = kFuncs->GetCommandIDs(1);
    cmdConfig = kFuncs->GetCommandIDs(1);
    kFuncs->GetFolder(UserSettingsFolder, settingsDir, MAX_PATH);
    kFuncs->GetPreference(PREF_UNISTRING, PREFERENCE_CACHE_PARENTDIR, cacheParentDir, NULL);
    kFuncs->GetPreference(PREF_UNISTRING, PREFERENCE_SIGNON_FILE, signonFileName, NULL);
}

// Load the plugin preferences
void LoadPluginPreferences()
{
    kFuncs->GetPreference(PREF_INT, PREFERENCE_CLEARCOOKIES, &prefClearCookies, &prefClearCookies);
    kFuncs->GetPreference(PREF_INT, PREFERENCE_CLEARCACHE, &prefClearCache, &prefClearCache);
    kFuncs->GetPreference(PREF_INT, PREFERENCE_CLEARMRU, &prefClearMRU, &prefClearMRU);
    kFuncs->GetPreference(PREF_INT, PREFERENCE_CLEARSIGNON, &prefClearSignon, &prefClearSignon);
    kFuncs->GetPreference(PREF_INT, PREFERENCE_CLEARHISTORY, &prefClearHistory, &prefClearHistory);
}

// Save the plugin preferences
void SavePluginPreferences()
{
    kFuncs->SetPreference(PREF_INT, PREFERENCE_CLEARCOOKIES, &prefClearCookies, FALSE);
    kFuncs->SetPreference(PREF_INT, PREFERENCE_CLEARCACHE, &prefClearCache, FALSE);
    kFuncs->SetPreference(PREF_INT, PREFERENCE_CLEARMRU, &prefClearMRU, FALSE);
    kFuncs->SetPreference(PREF_INT, PREFERENCE_CLEARSIGNON, &prefClearSignon, FALSE);
    kFuncs->SetPreference(PREF_INT, PREFERENCE_CLEARHISTORY, &prefClearHistory, FALSE);
}

#define MOZILLA_STRICT_API

#include "nsCOMPtr.h"
#include "nsICookieManager.h"
#include "nsServiceManagerUtils.h"

// Clear the cookies
void ClearCookies()
{
	nsresult rv;

	//NS_COOKIEMANAGER_CONTRACTID
	nsCOMPtr<nsICookieManager> CookieManager(do_GetService("@mozilla.org/cookiemanager;1", &rv));
	if (NS_FAILED(rv)) return;

	CookieManager->RemoveAll();
}


#include "nsIBrowserHistory.h"

// Clear the history
void ClearHistory()
{
	nsresult rv;

	nsCOMPtr<nsIBrowserHistory> BrowserHistory(do_GetService("@mozilla.org/browser/nav-history-service;1", &rv));
	if (NS_FAILED(rv)) return;
	
	BrowserHistory->RemoveAllPages();
}

#include "nsILoginManager.h"


// Clear the Sign on data
void ClearSignon()
{
	nsresult rv;

	//NS_PASSWORDMANAGER_CONTRACTID
	nsCOMPtr<nsILoginManager> PasswordManager(do_GetService(NS_LOGINMANAGER_CONTRACTID, &rv));
	if (NS_FAILED(rv)) return;

	PasswordManager->RemoveAllLogins();
}

// Delete all the files in a directory
void EmptyDirectory(LPCTSTR sDirectory)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = NULL;
    TCHAR fileToDelete[MAX_PATH];
    TCHAR sDirMask[MAX_PATH];
        
    _tcscpy(sDirMask, sDirectory);
    _tcscat(sDirMask, _T("*"));
    hFind = FindFirstFile(sDirMask, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            _tcscpy(fileToDelete, sDirectory);
            _tcscat(fileToDelete, FindFileData.cFileName);
            DeleteFile(fileToDelete);
        }
        while (FindNextFile(hFind, &FindFileData) != 0)
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                _tcscpy(fileToDelete, sDirectory);
                _tcscat(fileToDelete, FindFileData.cFileName);
                DeleteFile(fileToDelete);
            }
        FindClose(hFind);
    }
}

// Clear the cache
void ClearCache()
{
/* from nsICache.h */
enum
{
	STORE_ANYWHERE	      = 0,
	STORE_IN_MEMORY	      = 1,
	STORE_ON_DISK	      = 2,
	STORE_ON_DISK_AS_FILE = 3
};

    kFuncs->ClearCache(STORE_ANYWHERE);
    kFuncs->ClearCache(STORE_IN_MEMORY);
    kFuncs->ClearCache(STORE_ON_DISK);
    kFuncs->ClearCache(STORE_ON_DISK_AS_FILE);
}

// Clear the cache
void RemoveCache()
{
    TCHAR cacheDir[MAX_PATH];
    _tcscpy(cacheDir, cacheParentDir);
    _tcscat(cacheDir, _T("Cache\\"));
    
    EmptyDirectory(cacheDir);

    _tcscpy(cacheDir, cacheParentDir);
    _tcscat(cacheDir, _T("Cache.Trash\\"));

    EmptyDirectory(cacheDir);
}

// Clear the MRUs
void ClearMRU()
{
    UINT i, NbMRUs = 16;
    char PrefName[256];
    
    kFuncs->GetPreference(PREF_INT, PREFERENCE_MRU_MAXURLS, &NbMRUs, &NbMRUs);
    for (i=0; i<NbMRUs; i++)
    {
        sprintf(PrefName, "%s%i", PREFERENCE_MRU_URL, i);
        kFuncs->SetPreference(PREF_STRING, PrefName, (void*)"", FALSE);
    }
    //kFuncs->BroadcastMessage(UWM_REFRESHMRULIST, 0, 0);
}

// Process the Shutdown tasks
void DoShutdownTasks()
{
    if (prefClearCookies & PRV_ONSHUTDOWN)
        ClearCookies();
    if (prefClearHistory & PRV_ONSHUTDOWN)
        ClearHistory();
    if (prefClearMRU & PRV_ONSHUTDOWN)
        ClearMRU();
    if (prefClearCache & PRV_ONSHUTDOWN)
    {
        ClearCache();
        RemoveCache();
    }
    if (prefClearSignon & PRV_ONSHUTDOWN)
        ClearSignon();
}

// Process the Startup tasks
void DoStartupTasks()
{
    if (prefClearCookies & PRV_ONSTARTUP)
        ClearCookies();
    if (prefClearMRU & PRV_ONSTARTUP)
        ClearMRU();
    if (prefClearHistory & PRV_ONSTARTUP)
        ClearHistory();
    if (prefClearCache & PRV_ONSTARTUP) 
    {
        ClearCache();
        RemoveCache();
    }
    if (prefClearSignon & PRV_ONSTARTUP)
        ClearSignon();
}

/*void DebugInt(INT i)
{
    TCHAR s[256];
    sprintf(s, "%i", i);
    MessageBox(hMainWindow, s, "", 0);
}*/

INT Load()
{
	gLoc = Locale::kmInit(&kPlugin);
    kFuncs = kPlugin.kFuncs;
    InitGlobals();
    LoadPluginPreferences();
    SavePluginPreferences();    // If the Preferences don't exist yet, this create them
    DoStartupTasks();
    return TRUE;
}

void Create(HWND parent)
{
   KMeleonWndProc = (void *) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
   hMainWindow = parent;
}

void Config(HWND parent)
{
   gLoc->DialogBoxParam(MAKEINTRESOURCE(DLG_CONFIG), parent, (DLGPROC)DlgProc, (LPARAM)NULL);
}

void Quit()
{
	if (gLoc) delete gLoc;
    DoShutdownTasks();
}

void DoMenu(HMENU menu, LPSTR param)
{
   if (*param != 0){
      LPSTR action = param;
      LPSTR string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;
      
      UINT command = 0;
      if (stricmp(action, "Config") == 0) {
         command = cmdConfig;
      }
      else if (stricmp(action, "ClearCookies") == 0) {
         command = cmdClearCookies;
      }
      else if (stricmp(action, "ClearHistory") == 0) {
         command = cmdClearHistory;
      }
      else if (stricmp(action, "ClearCache") == 0) {
         command = cmdClearCache;
      }
      else if (stricmp(action, "ClearMRU") == 0) {
         command = cmdClearMRU;
      }
      else if (stricmp(action, "ClearSignon") == 0) {
         command = cmdClearSignon;
      }
      if (command) {
         AppendMenuA(menu, MF_STRING, command, string);
      }
   }
}

INT DoAccel(LPSTR param)
{
    if (stricmp(param, "Config") == 0){
        return cmdConfig;
    }
    if (stricmp(param, "ClearCookies") == 0){
        return cmdClearCookies;
    }
    if (stricmp(param, "ClearHistory") == 0){
        return cmdClearHistory;
    }
    if (stricmp(param, "ClearCache") == 0){
        return cmdClearCache;
    }
    if (stricmp(param, "ClearMRU") == 0){
        return cmdClearMRU;
    }
    if (stricmp(param, "ClearSignon") == 0){
        return cmdClearSignon;
    }
    return 0;
}

void DoRebar(HWND rebarWnd)
{
}

LONG DoMessage(LPCSTR to, LPCSTR from, LPCSTR subject, LONG data1, LONG data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0)
   {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (LPSTR)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
          *(PINT)data2 = DoAccel((LPSTR)data1);
      }
	  else if (stricmp(subject, "DoLocale") == 0) {
         if (gLoc) delete gLoc;
		 gLoc = Locale::kmInit(&kPlugin);
	  }
      else return 0;

      return 1;
   }
   return 0;
}

// Main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COMMAND)
    {
        WORD command = LOWORD(wParam);
        if (command == cmdConfig)
        {
                Config(hWnd);
                return TRUE;
        }
        else if (command == cmdClearCookies)
        {
                ClearCookies();
                return TRUE;
        }
        else if (command == cmdClearHistory)
        {
                ClearHistory();
                return TRUE;
        }
        else if (command == cmdClearCache)
        {
                ClearCache();
                return TRUE;
        }
        else if (command == cmdClearMRU)
        {
                ClearMRU();
                return TRUE;
        }
        else if (command == cmdClearSignon)
        {
                ClearSignon();
                return TRUE;
        }
    }

    return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
}

// Preferences dialog procedure
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg)
    {
        case WM_INITDIALOG:
            if (prefClearHistory & PRV_ONSTARTUP)
                CheckDlgButton(hWnd, CHK_HISTORY_STARTUP, BST_CHECKED);
            if (prefClearHistory & PRV_ONSHUTDOWN)
                CheckDlgButton(hWnd, CHK_HISTORY_SHUTDOWN, BST_CHECKED);
                
            if (prefClearCookies & PRV_ONSTARTUP)
                CheckDlgButton(hWnd, CHK_COOKIES_STARTUP, BST_CHECKED);
            if (prefClearCookies & PRV_ONSHUTDOWN)
                CheckDlgButton(hWnd, CHK_COOKIES_SHUTDOWN, BST_CHECKED);
                
            if (prefClearCache & PRV_ONSTARTUP)
                CheckDlgButton(hWnd, CHK_CACHE_STARTUP, BST_CHECKED);
            if (prefClearCache & PRV_ONSHUTDOWN)
                CheckDlgButton(hWnd, CHK_CACHE_SHUTDOWN, BST_CHECKED);
                
            if (prefClearMRU & PRV_ONSTARTUP)
                CheckDlgButton(hWnd, CHK_MRU_STARTUP, BST_CHECKED);
            if (prefClearMRU & PRV_ONSHUTDOWN)
                CheckDlgButton(hWnd, CHK_MRU_SHUTDOWN, BST_CHECKED);
                
            if (prefClearSignon & PRV_ONSTARTUP)
                CheckDlgButton(hWnd, CHK_SIGNON_STARTUP, BST_CHECKED);
            if (prefClearSignon & PRV_ONSHUTDOWN)
                CheckDlgButton(hWnd, CHK_SIGNON_SHUTDOWN, BST_CHECKED);
                
            break;
        case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
                        case BTN_CLEAR_COOKIES:
                            ClearCookies();
                            break;
                        case BTN_CLEAR_HISTORY:
                            ClearHistory();
                            break;
                        case BTN_CLEAR_CACHE:
                            ClearCache();
                            break;
                        case BTN_CLEAR_MRU:
                            ClearMRU();
                            break;
                        case BTN_CLEAR_SIGNON:
                            ClearSignon();
                            break;
                        case IDOK:
                            // History checkboxes
                            if (IsDlgButtonChecked(hWnd, CHK_HISTORY_STARTUP) == BST_CHECKED)
                                prefClearHistory |= PRV_ONSTARTUP;
                            else
                                prefClearHistory &= ~PRV_ONSTARTUP;
                            if (IsDlgButtonChecked(hWnd, CHK_HISTORY_SHUTDOWN) == BST_CHECKED)
                                prefClearHistory |= PRV_ONSHUTDOWN;
                            else
                                prefClearHistory &= ~PRV_ONSHUTDOWN;

                            // Cookies checkboxes
                            if (IsDlgButtonChecked(hWnd, CHK_COOKIES_STARTUP) == BST_CHECKED)
                                prefClearCookies |= PRV_ONSTARTUP;
                            else
                                prefClearCookies &= ~PRV_ONSTARTUP;
                            if (IsDlgButtonChecked(hWnd, CHK_COOKIES_SHUTDOWN) == BST_CHECKED)
                                prefClearCookies |= PRV_ONSHUTDOWN;
                            else
                                prefClearCookies &= ~PRV_ONSHUTDOWN;
                                
                            // Cache checkboxes
                            if (IsDlgButtonChecked(hWnd, CHK_CACHE_STARTUP) == BST_CHECKED)
                                prefClearCache |= PRV_ONSTARTUP;
                            else
                                prefClearCache &= ~PRV_ONSTARTUP;
                            if (IsDlgButtonChecked(hWnd, CHK_CACHE_SHUTDOWN) == BST_CHECKED)
                                prefClearCache |= PRV_ONSHUTDOWN;
                            else
                                prefClearCache &= ~PRV_ONSHUTDOWN;
                                
                            // MRU checkboxes
                            if (IsDlgButtonChecked(hWnd, CHK_MRU_STARTUP) == BST_CHECKED)
                                prefClearMRU |= PRV_ONSTARTUP;
                            else
                                prefClearMRU &= ~PRV_ONSTARTUP;
                            if (IsDlgButtonChecked(hWnd, CHK_MRU_SHUTDOWN) == BST_CHECKED)
                                prefClearMRU |= PRV_ONSHUTDOWN;
                            else
                                prefClearMRU &= ~PRV_ONSHUTDOWN;
                                
                            // Sign on checkboxes
                            if (IsDlgButtonChecked(hWnd, CHK_SIGNON_STARTUP) == BST_CHECKED)
                                prefClearSignon |= PRV_ONSTARTUP;
                            else
                                prefClearSignon &= ~PRV_ONSTARTUP;
                            if (IsDlgButtonChecked(hWnd, CHK_SIGNON_SHUTDOWN) == BST_CHECKED)
                                prefClearSignon |= PRV_ONSHUTDOWN;
                            else
                                prefClearSignon &= ~PRV_ONSHUTDOWN;
                                
                            SavePluginPreferences();
                        case IDCANCEL:
                            SendMessage(hWnd, WM_CLOSE, 0, 0);
                    }
            }
            break;
        case WM_CLOSE:
			EndDialog(hWnd, (LPARAM)NULL);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}

extern "C"
{
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin()
   {
          return &kPlugin;
   }

   KMELEON_PLUGIN INT DrawBitmap(DRAWITEMSTRUCT *dis)
   {
          return 14; // 14 = icon width
   }
}
