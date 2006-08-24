/*
*  Copyright (C) 2000 Brian Harris
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
//  Holds various preferences for k-meleon. also has the getters/setters

#include "StdAfx.h"
#include "kmeleonConst.h"

#include "MfcEmbed.h"
#include "BrowserFrm.h"
extern CMfcEmbedApp theApp;

#include "Preferences.h"
#include "nsDirectoryServiceUtils.h"

CPreferences::CPreferences() {
}

CPreferences::~CPreferences() {
}

void CPreferences::Load() {

   nsresult rv;
   m_prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
   if (NS_FAILED(rv)) {
      _ASSERTE(m_prefs && "Could not get preferences service");
      return;
   }

#ifndef USE_PROFILES
   prefs->ReadUserPrefs(nsnull);
#endif
   PRBool inited;
   rv = m_prefs->GetBoolPref("kmeleon.prefs_inited", &inited);
   if (NS_FAILED(rv) || !inited) {
      // Set up prefs for first run
      rv = m_prefs->SetBoolPref("kmeleon.prefs_inited", PR_TRUE);
      rv = m_prefs->SavePrefFile(nsnull);
   }

   USES_CONVERSION;

   // -- Display settings

   _GetBool("kmeleon.display.hideTaskBarButtons", bHideTaskBarButtons, false);

   if (!theApp.m_pMostRecentBrowserFrame || !(theApp.m_pMostRecentBrowserFrame->m_style & WS_POPUP)) {
      _GetBool("kmeleon.display.maximized", bMaximized, true);

      _GetInt("kmeleon.display.width", windowWidth, -1);
      _GetInt("kmeleon.display.height", windowHeight, -1);
      _GetInt("kmeleon.display.XPos", windowXPos, -1);
      _GetInt("kmeleon.display.YPos", windowYPos, -1);
   }

   _GetBool("kmeleon.display.backgroundImageEnabled", bToolbarBackground, true);

   _GetString("kmeleon.display.backgroundImage", toolbarBackground, _T(""));

   _GetInt("kmeleon.display.newWindowOpenAs", iNewWindowOpenAs, 0);
   _GetString("kmeleon.display.newWindowURL", newWindowURL, _T(""));

   _GetBool("kmeleon.display.disableResize", bDisableResize, false);

   _GetBool("kmeleon.display.NewWindowHasUrlFocus", bNewWindowHasUrlFocus, true);
   _GetBool("kmeleon.favicons.show", bSiteIcons, PR_TRUE);

   _GetInt("font.minimum-size.x-western", iFontMinSize, 0);

   // -- Find settings

   _GetBool("kmeleon.find.matchCase", bFindMatchCase, false);
   _GetBool("kmeleon.find.highlight", bFindHighlight, false);
   //	   _GetBool("kmeleon.find.matchWholeWord", bFindMatchWholeWord, false);
   _GetBool("kmeleon.find.searchBackwards", bFindSearchBackwards, false);
   _GetBool("kmeleon.find.wrapAround", bFindWrapAround, false);

   // -- General preferences

   _GetBool("kmeleon.general.offline", bOffline, false);

   _GetBool("kmeleon.general.guest_account", bGuestAccount, false);

   _GetBool("kmeleon.general.startHome", bStartHome, true);
   _GetString("kmeleon.general.homePage", homePage, _T("http://kmeleon.sourceforge.net/start"));

   _GetString("kmeleon.general.searchEngine", searchEngine, _T("http://www.google.com/search?q="));

   _GetBool("kmeleon.general.sourceEnabled", bSourceUseExternalCommand, false);
   _GetString("kmeleon.general.sourceCommand", sourceCommand, _T(""));

   _GetString("kmeleon.general.settingsDir", settingsDir, _T(""));
   _GetString("kmeleon.general.pluginsDir", pluginsDir, _T(""));

   _GetString("kmeleon.general.skinsDir", skinsDir, _T(""));
   _GetString("kmeleon.general.skinsCurrent", skinsCurrent, _T(""));


   TCHAR appDir[MAX_PATH];
   GetModuleFileName(NULL, appDir, MAX_PATH);
   int x=_tcslen(appDir)-1;
   while (x>0 && appDir[x] != _T('\\')) x--;
   if (x>0) appDir[x+1]=0;


   nsCOMPtr<nsIFile> nsProfileDir;
   rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(nsProfileDir));
   _ASSERTE(nsProfileDir && "NS_APP_USER_PROFILE_50_DIR is not defined");
   
   if (NS_SUCCEEDED(rv)){         
#ifdef _UNICODE
      nsEmbedString pathBuf;
      rv = nsProfileDir->GetPath(pathBuf);
#else
      nsEmbedCString pathBuf;
      rv = nsProfileDir->GetNativePath(pathBuf);
#endif
      _defProfileDir = pathBuf.get();

      if (_defProfileDir.Right(1) != '\\')
         _defProfileDir += '\\';
   }
   else
      _defProfileDir = appDir;

   profileDir = _defProfileDir;
   SetString("kmeleon.general.profileDir", (LPCTSTR)profileDir);


   if (settingsDir.IsEmpty())
      settingsDir = profileDir;
   else if (settingsDir.Right(1) != '\\')
      settingsDir += '\\';


   if (skinsDir.IsEmpty()) {
      skinsDir = appDir;
      skinsDir += "skins\\";
   }
   else if (skinsDir.Right(1) != "\\")
      skinsDir += "\\";

   if (skinsCurrent.IsEmpty())
      skinsCurrent = "Default\\";     
   else if (skinsCurrent.Right(1) != "\\")
      skinsCurrent += "\\";

   if (pluginsDir.IsEmpty()) {
      pluginsDir = appDir;
      pluginsDir += "kplugins\\";
   }
   else if (pluginsDir[pluginsDir.GetLength() - 1] != '\\')
      pluginsDir += '\\';


   // -- Cache
   _GetString("browser.cache.disk.parent_directory", cacheDir, _T(""));
   if (!cacheDir.IsEmpty() && cacheDir[cacheDir.GetLength() - 1] != '\\')
      cacheDir += '\\';

   _GetInt("browser.cache.disk.capacity", cacheDisk, 4096);
   _GetInt("browser.cache.memory.capacity", cacheMemory, 1024);
   _GetInt("browser.cache.check_doc_frequency", cacheCheckFrequency, 0);

   // -- Proxies      


   _GetString  ("network.proxy.http",          proxyHTTP,     _T(""));
   _GetInt     ("network.proxy.http_port",     proxyHTTPPort, 0);
   _GetString  ("network.proxy.ftp",           proxyFTP,     _T(""));
   _GetInt     ("network.proxy.ftp_port",      proxyFTPPort,  0);
   _GetString  ("network.proxy.ssl",           proxySSL,     _T(""));
   _GetInt     ("network.proxy.ssl_port",      proxySSLPort, 0);
   _GetString  ("network.proxy.gopher",        proxyGopher,     _T(""));
   _GetInt     ("network.proxy.gopher_port",   proxyGopherPort, 0);
   _GetString  ("network.proxy.socks",         proxySOCKS,     _T(""));
   _GetInt     ("network.proxy.socks_port",    proxySOCKSPort, 0);
   _GetInt     ("network.proxy.socks_version", proxySOCKSVersion, 5);

   if (proxySOCKSVersion == 5) proxySOCKSRadio = 1;    // the radio button state
   else proxySOCKSRadio = 0;

   _GetString("network.proxy.autoconfig_url", proxyAutoURL, _T(""));
   _GetString("network.proxy.no_proxies_on", proxyNoProxy, _T("localhost"));

   _GetInt("network.proxy.type", proxyType, 0);


   // -- Advanced     

   _GetBool("javascript.enabled", bJavascriptEnabled, true);
   _GetBool("security.enable_java", bJavaEnabled, true);
   _GetBool("signon.rememberSignons", bRememberSignons, true);

   _GetInt("kmeleon.MRU.behavior", MRUbehavior, 1);

   // 0 = Always, 1 = site, 2 = never, 3 = P3P
   _GetInt("network.cookie.cookieBehavior", iCookiesEnabled, 0);

   // 1 = Always, 2 = never, 3 = site
   _GetInt("permissions.default.image", iImagesEnabled, 0);
   iImagesEnabled--;

   _GetString("network.http.version", httpVersion, _T(""));
   /*
   CString animationMode;
   _GetString("image.animation_mode", animationMode, "");
   if (animationMode == _T("normal"))  // "once" "none" "normal"
   bAnimationsEnabled = true;
   else
   bAnimationsEnabled = false;
   */

   // -- Privacy

   _GetString("general.useragent.override", userAgent, _T(""));

   CString restrictPopups;
   _GetString("capability.policy.restrictedpopups.Window.open", restrictPopups, _T(""));
   if (restrictPopups == _T("noAccess"))
      bRestrictPopups = TRUE;
   else
      bRestrictPopups = FALSE;

   _GetString("capability.policy.restrictedpopups.sites", restrictedPopupSites, _T(""));

   _GetBool("dom.disable_open_during_load", bDisablePopupsOnLoad, false);

   _GetInt("browser.history_expire_days", historyExpire, 7);
   _GetBool("kmeleon.favicons.cached", bCacheFavicons, true);
   bCacheFavicons = !bCacheFavicons;


   // -- Printing


   _GetString("kmeleon.print.headerLeft", printHeaderLeft, _T("&T"));
   _GetString("kmeleon.print.headerMiddle", printHeaderMiddle, _T(""));
   _GetString("kmeleon.print.headerRight", printHeaderRight, _T("&U"));

   _GetString("kmeleon.print.footerLeft", printFooterLeft, _T("&PT"));
   _GetString("kmeleon.print.footerMiddle", printFooterMiddle, _T(""));
   _GetString("kmeleon.print.footerRight", printFooterRight, _T("&D"));

   _GetBool("kmeleon.print.BGColors", printBGColors, false);
   _GetBool("kmeleon.print.BGImages", printBGImages, false);

   CString def;
   def.Format(_T("%.2f"), 0.5);
   _GetString("kmeleon.print.marginLeft", printMarginLeft, def);
   _GetString("kmeleon.print.marginRight", printMarginRight, def);
   _GetString("kmeleon.print.marginTop", printMarginTop, def);
   _GetString("kmeleon.print.marginBottom", printMarginBottom, def);

   _GetInt("kmeleon.print.scaling", printScaling, 100);
   _GetBool("kmeleon.print.shrinkToFit", printShrinkToFit, true);
   _GetInt("kmeleon.print.paperUnit", printUnit, nsIPrintSettings::kPaperSizeInches);
   def.Format(_T("%.2f"), 8.5);
   _GetString("kmeleon.print.paperWidth", printWidth, def);
   def.Format(_T("%d"), 11);
   _GetString("kmeleon.print.paperHeight", printHeight, def);

   // -- Download

   _GetInt("kmeleon.general.saveType", iSaveType, 0);
   _GetString("kmeleon.download.saveDir", saveDir, _T(""));   
   _GetString("kmeleon.download.dir", downloadDir, _T(""));
   _GetString("kmeleon.download.lastDir", lastDownloadDir, _T(""));
   if (lastDownloadDir.IsEmpty())
      lastDownloadDir = downloadDir;
   if (downloadDir.IsEmpty())
      downloadDir = lastDownloadDir;
   _GetBool("kmeleon.download.useDownloadDir", bUseDownloadDir, false);
   _GetBool("kmeleon.download.askOpenSave", bAskOpenSave, true);
   _GetBool("kmeleon.download.showMinimizedDialog", bShowMinimized, false);
   _GetBool("kmeleon.download.flashWhenCompleted", bFlashWhenCompleted, false);
   _GetBool("kmeleon.download.closeDownloadDialog", bCloseDownloadDialog, false);
   _GetBool("kmeleon.download.saveUseTitle", bSaveUseTitle, true);
}

void CPreferences::Flush()
{
   if (!m_prefs) return;
   m_prefs->SavePrefFile(nsnull);      
}

void CPreferences::Save(bool clearPath)
{
   if (!m_prefs) return;

   nsresult rv;
   USES_CONVERSION;

   // If the skin was changed, replace skin.js in the 
   // default/pref folder
   CString oldSkin;
   _GetString("kmeleon.general.skinsCurrent", oldSkin, skinsCurrent);

   if  (skinsCurrent.CompareNoCase(oldSkin) != 0)
   {
      CString skinFile;
      theApp.FindSkinFile(skinFile, _T("skin.js"));
      if (!skinFile.IsEmpty()) 
      {

         nsCOMPtr<nsIFile> nsProfileDir;
         rv = NS_GetSpecialDirectory(NS_APP_PREF_DEFAULTS_50_DIR, getter_AddRefs(nsProfileDir));
         _ASSERTE(nsProfileDir && "NS_APP_USER_PROFILE_50_DIR is not defined");

         if (NS_SUCCEEDED(rv))
         {         
#ifdef _UNICODE
            nsEmbedString pathBuf;
            rv = nsProfileDir->GetPath(pathBuf);
#else
            nsEmbedCString pathBuf;
            rv = nsProfileDir->GetNativePath(pathBuf);
#endif
            CString defaultDir = pathBuf.get();
            if (defaultDir.Right(1) != '\\')
               defaultDir += '\\';

            CopyFile(skinFile, defaultDir + _T("skin.js"), FALSE);
         }
      }
   }

   // -- General preferences
   rv = m_prefs->SetBoolPref("kmeleon.general.startHome", bStartHome);
   _SetString("kmeleon.general.homePage",homePage);
   rv = m_prefs->SetBoolPref("kmeleon.general.offline", bOffline);
   rv = m_prefs->SetBoolPref("kmeleon.general.guest_account", bGuestAccount);

   //_SetString("kmeleon.general.searchEngine",searchEngine)

   _SetString("kmeleon.general.settingsDir",settingsDir);
   _SetString("kmeleon.general.pluginsDir",pluginsDir);

   _SetString("kmeleon.general.skinsDir",skinsDir);
   _SetString("kmeleon.general.skinsCurrent",skinsCurrent);

   rv = m_prefs->SetBoolPref("kmeleon.general.sourceEnabled", bSourceUseExternalCommand);
   _SetString("kmeleon.general.sourceCommand",sourceCommand);

   // -- Cache
   if (cacheDir.IsEmpty())
      m_prefs->ClearUserPref("browser.cache.disk.parent_directory");
   else
      _SetString("browser.cache.disk.parent_directory",cacheDir);
   rv = m_prefs->SetIntPref("browser.cache.disk.capacity", cacheDisk);

   rv = m_prefs->SetIntPref("browser.cache.memory.capacity", cacheMemory);
   rv = m_prefs->SetIntPref("browser.cache.check_doc_frequency", cacheCheckFrequency);

   // -- Proxies
   _SetString("network.proxy.http",       proxyHTTP);
   rv = m_prefs->SetIntPref ("network.proxy.http_port",   proxyHTTPPort);
   _SetString("network.proxy.ftp",        proxyFTP);
   rv = m_prefs->SetIntPref ("network.proxy.ftp_port",    proxyFTPPort);
   _SetString("network.proxy.ssl",        proxySSL);
   rv = m_prefs->SetIntPref ("network.proxy.ssl_port",    proxySSLPort);
   _SetString("network.proxy.gopher",     proxyGopher);
   rv = m_prefs->SetIntPref ("network.proxy.gopher_port", proxyGopherPort);
   _SetString("network.proxy.socks",      proxySOCKS);
   rv = m_prefs->SetIntPref ("network.proxy.socks_port",  proxySOCKSPort);

   if (proxySOCKSRadio == 1) proxySOCKSVersion = 5;    // the radio button state
   else proxySOCKSVersion = 4;
   rv = m_prefs->SetIntPref ("network.proxy.socks_version",  proxySOCKSVersion);


   _SetString("network.proxy.autoconfig_url",proxyAutoURL);
   _SetString("network.proxy.no_proxies_on",proxyNoProxy);

   rv = m_prefs->SetIntPref("network.proxy.type", proxyType);

   //  -- Advanced
   rv = m_prefs->SetBoolPref("javascript.enabled", bJavascriptEnabled);
   rv = m_prefs->SetBoolPref("security.enable_java", bJavaEnabled);
   rv = m_prefs->SetBoolPref("signon.rememberSignons", bRememberSignons);

   rv = m_prefs->SetIntPref("network.cookie.cookieBehavior", iCookiesEnabled);
   rv = m_prefs->SetIntPref("permissions.default.image", iImagesEnabled+1);
   _SetString("network.http.version",httpVersion);
   /*
   if (bAnimationsEnabled)    // "once" "none" "normal"
   _SetString("image.animation_mode", _T("normal"))
   else
   _SetString("image.animation_mode", _T("none"))
   //rv = m_prefs->SetCharPref("image.animation_mode", _T("none"));
   */

   // -- Privacy

   if (!userAgent.IsEmpty())
      _SetString("general.useragent.override",userAgent);
   else
      rv = m_prefs->ClearUserPref("general.useragent.override");

   if (bRestrictPopups)
      _SetString("capability.policy.restrictedpopups.Window.open",_T("noAccess"));
   else
      _SetString("capability.policy.restrictedpopups.Window.open",_T("allAccess"));
   _SetString("capability.policy.restrictedpopups.sites",restrictedPopupSites);

   rv = m_prefs->SetBoolPref("dom.disable_open_during_load", bDisablePopupsOnLoad);

   rv = m_prefs->SetIntPref("browser.history_expire_days", historyExpire);
   rv = m_prefs->SetBoolPref("kmeleon.favicons.cached", !bCacheFavicons);

   if (!theApp.m_pMostRecentBrowserFrame || !(theApp.m_pMostRecentBrowserFrame->m_style & WS_POPUP)) {
      // -- Display settings
      rv = m_prefs->SetBoolPref("kmeleon.display.maximized", bMaximized);
      rv = m_prefs->SetIntPref("kmeleon.display.width", windowWidth);
      rv = m_prefs->SetIntPref("kmeleon.display.height", windowHeight);
      rv = m_prefs->SetIntPref("kmeleon.display.XPos", windowXPos);
      rv = m_prefs->SetIntPref("kmeleon.display.YPos", windowYPos);
   }

   rv = m_prefs->SetIntPref("kmeleon.general.saveType", iSaveType);
   rv = m_prefs->SetBoolPref("kmeleon.display.backgroundImageEnabled", bToolbarBackground);
   _SetString("kmeleon.display.backgroundImage",toolbarBackground);

   rv = m_prefs->SetIntPref("kmeleon.display.newWindowOpenAs", iNewWindowOpenAs);
   _SetString("kmeleon.display.newWindowURL",newWindowURL);

   rv = m_prefs->SetBoolPref("kmeleon.display.disableResize", bDisableResize);

   rv = m_prefs->SetBoolPref("kmeleon.display.NewWindowHasUrlFocus", bNewWindowHasUrlFocus);
   rv = m_prefs->SetBoolPref("kmeleon.favicons.show", bSiteIcons);
   rv = m_prefs->SetIntPref("font.minimum-size.x-western", iFontMinSize);
   rv = m_prefs->SetIntPref("font.minimum-size.x-unicode", iFontMinSize);

   // -- Find settings
   rv = m_prefs->SetBoolPref("kmeleon.find.matchCase", bFindMatchCase);
   rv = m_prefs->SetBoolPref("kmeleon.find.highlight", bFindHighlight);
   // don't set this - it might confuse the users :)  (okay, we *should* remove all references to it, perhaps, but I'm being lazy - sorry)
   //	   rv = m_prefs->SetBoolPref("kmeleon.find.matchWholeWord", bFindMatchWholeWord);
   rv = m_prefs->SetBoolPref("kmeleon.find.wrapAround", bFindWrapAround);
   rv = m_prefs->SetBoolPref("kmeleon.find.searchBackwards", bFindSearchBackwards);


   // -- Print Settings
   _SetString("kmeleon.print.headerLeft",printHeaderLeft);
   _SetString("kmeleon.print.headerMiddle",printHeaderMiddle);
   _SetString("kmeleon.print.headerRight",printHeaderRight);

   _SetString("kmeleon.print.footerLeft",printFooterLeft);
   _SetString("kmeleon.print.footerMiddle",printFooterMiddle);
   _SetString("kmeleon.print.footerRight",printFooterRight);

   rv = m_prefs->SetBoolPref("kmeleon.print.BGColors", printBGColors);
   rv = m_prefs->SetBoolPref("kmeleon.print.BGImages", printBGImages);

   _SetString("kmeleon.print.marginLeft",printMarginLeft);
   _SetString("kmeleon.print.marginRight",printMarginRight);
   _SetString("kmeleon.print.marginTop",printMarginTop);
   _SetString("kmeleon.print.marginBottom",printMarginBottom);

   rv = m_prefs->SetBoolPref("kmeleon.print.shrinkToFit", printShrinkToFit);
   rv = m_prefs->SetIntPref("kmeleon.print.scaling", printScaling);
   rv = m_prefs->SetIntPref("kmeleon.print.paperUnit", printUnit);
   _SetString("kmeleon.print.paperWidth",printWidth);
   _SetString("kmeleon.print.paperHeight",printHeight);


   // -- Download

   m_prefs->SetIntPref("kmeleon.general.saveType", iSaveType);
   _SetString("kmeleon.download.saveDir", saveDir);   
   _SetString("kmeleon.download.dir", downloadDir);
   _SetString("kmeleon.download.lastDir", lastDownloadDir);


   m_prefs->SetBoolPref("kmeleon.download.useDownloadDir", bUseDownloadDir);
   m_prefs->SetBoolPref("kmeleon.download.askOpenSave", bAskOpenSave);
   m_prefs->SetBoolPref("kmeleon.download.showMinimizedDialog", bShowMinimized);
   m_prefs->SetBoolPref("kmeleon.download.flashWhenCompleted", bFlashWhenCompleted);
   m_prefs->SetBoolPref("kmeleon.download.closeDownloadDialog", bCloseDownloadDialog);
   m_prefs->SetBoolPref("kmeleon.download.saveUseTitle", bSaveUseTitle);

   if (clearPath) {
      // XXX: Removing path from profile when equal to default
      TCHAR _appDir[MAX_PATH];
      GetModuleFileName(NULL, _appDir, MAX_PATH);
      int x=_tcslen(_appDir)-1;
      while (x>0 && _appDir[x] != _T('\\')) x--;
      if (x>0) _appDir[x+1]=0;

      CString appDir = _appDir;

      if (pluginsDir.CompareNoCase(appDir + _T("kplugins\\")) == 0)
         m_prefs->ClearUserPref("kmeleon.general.pluginsDir");

      if (skinsDir.CompareNoCase(appDir + _T("skins\\")) == 0)
         m_prefs->ClearUserPref("kmeleon.general.skinsDir");

      if (profileDir.CompareNoCase(_defProfileDir) == 0)
         m_prefs->ClearUserPref("kmeleon.general.profileDir");

      if (settingsDir.CompareNoCase(_defProfileDir) == 0)
         m_prefs->ClearUserPref("kmeleon.general.settingsDir");

      if (cacheDir.CompareNoCase(profileDir) == 0)
         m_prefs->ClearUserPref("browser.cache.disk.parent_directory");
   }

   rv = m_prefs->SavePrefFile(nsnull);   
}

int CPreferences::GetBool(const char *preference, int defaultVal)
{
   if (!m_prefs) return defaultVal;
	PRBool tempBool;
   nsresult rv = m_prefs->GetBoolPref(preference, &tempBool);
   if (NS_SUCCEEDED(rv))
      return tempBool;
   else
      return defaultVal;
}

int CPreferences::GetInt(const char *preference, int defaultVal)
{
	if (!m_prefs) return defaultVal;
	PRInt32 tempInt;
   nsresult rv = m_prefs->GetIntPref(preference, &tempInt);
   if (NS_SUCCEEDED(rv))
      return tempInt;
   else
      return defaultVal;
}

int CPreferences::GetString(const char *preference, char *retVal, char *defaultVal)
{
	nsEmbedString string;
	nsEmbedCString stringNat;
   if (!m_prefs || !NS_SUCCEEDED(m_prefs->CopyUnicharPref(preference, getter_Copies(string))))
      stringNat = defaultVal;	
	else
		NS_UTF16ToCString(string, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, stringNat);
   if (retVal)
      strcpy(retVal, stringNat.get());
   return stringNat.Length();
}

int CPreferences::GetString(const char *preference, wchar_t *retVal, wchar_t *defaultVal)
{
   nsEmbedString string;
   if (!m_prefs || !NS_SUCCEEDED(m_prefs->CopyUnicharPref(preference, getter_Copies(string))))
      string = defaultVal;	
   if (retVal)
      wcscpy(retVal, string.get());
   return string.Length();
}

inline void CPreferences::SetString(const char *preference, const wchar_t *value)
{
   if (!m_prefs) return;
   m_prefs->SetUnicharPref(preference, value);
}

void CPreferences::SetString(const char *preference, const char *value)
{
   if (!m_prefs) return;
   USES_CONVERSION;
   SetString(preference, A2CW(value));
}

void CPreferences::Clear(const char *preference)
{
   if (!m_prefs) return;
   m_prefs->ClearUserPref(preference);
}

void CPreferences::DeleteBranch(const char *startingAt)
{
   if (!m_prefs) return;
   m_prefs->DeleteBranch(startingAt);
}

void CPreferences::_GetBool(const char *preference, int& var, int defaultVal)
{
   ASSERT(m_prefs);
   PRBool tempBool;
   nsresult rv = m_prefs->GetBoolPref(preference, &tempBool);
   if (NS_SUCCEEDED(rv))
      var = tempBool;
   else
      var = defaultVal;
}

void CPreferences::_GetInt(const char *preference, int& var, int defaultVal)
{
   ASSERT(m_prefs);
   PRInt32 tempInt;
   nsresult rv = m_prefs->GetIntPref(preference, &tempInt);
   if (NS_SUCCEEDED(rv))
      var = tempInt;
   else
      var = defaultVal;
}

void CPreferences::_GetString(const char *preference, CString& var, LPCTSTR defaultVal)
{
   ASSERT(m_prefs);
   nsEmbedString string;
   nsresult rv = m_prefs->CopyUnicharPref(preference, getter_Copies(string));
   if (NS_SUCCEEDED(rv) && defaultVal) {
		USES_CONVERSION;
      var = W2CT(string.get());	
	}
   else if (defaultVal)
      var = defaultVal;
}

void CPreferences::_SetString(const char *preference, LPCTSTR value)
{
   ASSERT(m_prefs);
   USES_CONVERSION;
   m_prefs->SetUnicharPref(preference, T2CW(value));
}
