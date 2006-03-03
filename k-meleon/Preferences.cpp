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

#define _GetBool(_pref, _value, _defaultValue) { \
    PRBool tempBool;                            \
    rv = prefs->GetBoolPref(_pref, &tempBool);  \
    if (NS_SUCCEEDED(rv))                       \
      _value = tempBool;                        \
    else                                        \
      _value = _defaultValue;                   \
  }

#define _GetInt(_pref, _value, _defaultValue) {  \
    PRInt32 tempInt;                            \
    rv = prefs->GetIntPref(_pref, &tempInt);    \
    if (NS_SUCCEEDED(rv))                       \
      _value = tempInt;                         \
    else                                        \
      _value = _defaultValue;                   \
  }
#ifdef _UNICODE

#define _GetString(_pref, _value, _defaultValue) { \
    nsEmbedString tempString;                    \
    rv = prefs->CopyUnicharPref(_pref, getter_Copies(tempString));  \
    if (NS_SUCCEEDED(rv))                         \
      _value = tempString.get();                        \
    else                                          \
      _value = _defaultValue;						\
}
#define _SetString(_pref, _value) \
	prefs->SetUnicharPref(_pref, _value);

#else



#define _SetString(_pref, _value) \
	prefs->SetCharPref(_pref, _value);

#define _GetString(_pref, _value, _defaultValue) { \
    nsEmbedCString tempString;                    \
    rv = prefs->CopyCharPref(_pref, getter_Copies(tempString));  \
    if (NS_SUCCEEDED(rv))                         \
      _value = tempString.get();                        \
    else                                          \
      _value = _defaultValue;                     \
  }

#endif

void CPreferences::Load() {

   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {	  

      PRBool inited;
      rv = prefs->GetBoolPref("kmeleon.prefs_inited", &inited);
      if (NS_FAILED(rv) || !inited) {
         // Set up prefs for first run
         rv = prefs->SetBoolPref("kmeleon.prefs_inited", PR_TRUE);
         rv = prefs->SavePrefFile(nsnull);
      }


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

	  _GetInt("font.minimum-size.x-western", iFontMinSize, 0);
      // -- Find settings
      
	   _GetBool("kmeleon.find.matchCase", bFindMatchCase, false);
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
      NS_ASSERTION(nsProfileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");
      
      if (NS_SUCCEEDED(rv)){         
#ifdef _UNICODE
		 nsEmbedString pathBuf;
         rv = nsProfileDir->GetPath(pathBuf);
#else
	     nsEmbedCString pathBuf;
         rv = nsProfileDir->GetNativePath(pathBuf);
#endif
         profileDir = pathBuf.get();

         if (profileDir.Right(1) != '\\')
            profileDir += '\\';
      }
      else
         profileDir = appDir;
      SetString("kmeleon.general.profileDir", profileDir.GetBuffer(0));


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
      if (cacheDir.IsEmpty())
         cacheDir = settingsDir;
      else if (cacheDir[cacheDir.GetLength() - 1] != '\\')
         cacheDir += '\\';

      _GetInt("browser.cache.disk.capacity", cacheDisk, 4096);
      _GetInt("browser.cache.memory.capacity", cacheMemory, 1024);
      _GetInt("browser.cache.check_doc_frequency", cacheCheckFrequency, 0);

      // -- Proxies      
      
      
      _GetString  ("network.proxy.http",          proxyHTTP,     "");
      _GetInt     ("network.proxy.http_port",     proxyHTTPPort, 0);
      _GetString  ("network.proxy.ftp",           proxyFTP,     "");
      _GetInt     ("network.proxy.ftp_port",      proxyFTPPort,  0);
      _GetString  ("network.proxy.ssl",           proxySSL,     "");
      _GetInt     ("network.proxy.ssl_port",      proxySSLPort, 0);
      _GetString  ("network.proxy.gopher",        proxyGopher,     "");
      _GetInt     ("network.proxy.gopher_port",   proxyGopherPort, 0);
      _GetString  ("network.proxy.socks",         proxySOCKS,     "");
      _GetInt     ("network.proxy.socks_port",    proxySOCKSPort, 0);
      _GetInt     ("network.proxy.socks_version", proxySOCKSVersion, 5);

      if (proxySOCKSVersion == 5) proxySOCKSRadio = 1;    // the radio button state
      else proxySOCKSRadio = 0;

      _GetString("network.proxy.autoconfig_url", proxyAutoURL, "");
      _GetString("network.proxy.no_proxies_on", proxyNoProxy, _T("localhost"));

      _GetInt("network.proxy.type", proxyType, 0);


      // -- Advanced     
      
      _GetBool("javascript.enabled", bJavascriptEnabled, true);
      _GetBool("security.enable_java", bJavaEnabled, true);
      _GetBool("signon.rememberSignons", bRememberSignons, true);

	  _GetInt("kmeleon.MRU.behavior", MRUbehavior, 2);
      // 0 = Always, 1 = site, 2 = never
      // 0 = Always, 1 = site, 2 = never, 3 = P3P
      _GetInt("network.cookie.cookieBehavior", iCookiesEnabled, 0);

      // 1 = Always, 2 = never, 3 = site
      _GetInt("permissions.default.image", iImagesEnabled, 0);
	  iImagesEnabled--;

      _GetString("network.http.version", httpVersion, "");

      CString animationMode;
      _GetString("image.animation_mode", animationMode, "");
      if (animationMode == _T("normal"))  // "once" "none" "normal"
         bAnimationsEnabled = true;
      else
         bAnimationsEnabled = false;


      // -- Privacy

      _GetString("general.useragent.override", userAgent, "");

      CString restrictPopups;
      _GetString("capability.policy.restrictedpopups.Window.open", restrictPopups, "");
      if (restrictPopups == _T("noAccess"))
         bRestrictPopups = TRUE;
      else
         bRestrictPopups = FALSE;
         
      _GetString("capability.policy.restrictedpopups.sites", restrictedPopupSites, "");

      _GetBool("dom.disable_open_during_load", bDisablePopupsOnLoad, false);



      // -- Printing


      _GetString("kmeleon.print.headerLeft", printHeaderLeft, "&T");
      _GetString("kmeleon.print.headerMiddle", printHeaderMiddle, "");
      _GetString("kmeleon.print.headerRight", printHeaderRight, "&U");

      _GetString("kmeleon.print.footerLeft", printFooterLeft, "&PT");
      _GetString("kmeleon.print.footerMiddle", printFooterMiddle, "");
      _GetString("kmeleon.print.footerRight", printFooterRight, "&D");
      
      _GetBool("kmeleon.print.BGColors", printBGColors, false);
      _GetBool("kmeleon.print.BGImages", printBGImages, false);

      _GetString("kmeleon.print.marginLeft", printMarginLeft, "0.5");
      _GetString("kmeleon.print.marginRight", printMarginRight, "0.5");
      _GetString("kmeleon.print.marginTop", printMarginTop, "0.5");
      _GetString("kmeleon.print.marginBottom", printMarginBottom, "0.5");
      
      _GetInt("kmeleon.print.scaling", printScaling, 100);
	  _GetBool("kmeleon.print.shrinkToFit", printShrinkToFit, true);
      _GetInt("kmeleon.print.paperUnit", printUnit, nsIPrintSettings::kPaperSizeInches);
      _GetString("kmeleon.print.paperWidth", printWidth, "8.5");
      _GetString("kmeleon.print.paperHeight", printHeight, "11");

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
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

void CPreferences::Flush() {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      rv = prefs->SavePrefFile(nsnull);      
   }
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}


void CPreferences::Save() {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {

      // -- General preferences
      rv = prefs->SetBoolPref("kmeleon.general.startHome", bStartHome);
      _SetString("kmeleon.general.homePage",homePage)
      rv = prefs->SetBoolPref("kmeleon.general.offline", bOffline);
      rv = prefs->SetBoolPref("kmeleon.general.guest_account", bGuestAccount);

      _SetString("kmeleon.general.searchEngine",searchEngine)

      _SetString("kmeleon.general.settingsDir",settingsDir)
      _SetString("kmeleon.general.pluginsDir",pluginsDir)

      _SetString("kmeleon.general.skinsDir",skinsDir)
      _SetString("kmeleon.general.skinsCurrent",skinsCurrent)

      rv = prefs->SetBoolPref("kmeleon.general.sourceEnabled", bSourceUseExternalCommand);
      _SetString("kmeleon.general.sourceCommand",sourceCommand)

      // -- Cache
      rv = prefs->SetCharPref("browser.cache.disk.parent_directory", cacheDir);
      rv = prefs->SetIntPref("browser.cache.disk.capacity", cacheDisk);

      rv = prefs->SetIntPref("browser.cache.memory.capacity", cacheMemory);
      rv = prefs->SetIntPref("browser.cache.check_doc_frequency", cacheCheckFrequency);

      // -- Proxies
      _SetString("network.proxy.http",       proxyHTTP)
      rv = prefs->SetIntPref ("network.proxy.http_port",   proxyHTTPPort);
      _SetString("network.proxy.ftp",        proxyFTP)
      rv = prefs->SetIntPref ("network.proxy.ftp_port",    proxyFTPPort);
      _SetString("network.proxy.ssl",        proxySSL)
      rv = prefs->SetIntPref ("network.proxy.ssl_port",    proxySSLPort);
      _SetString("network.proxy.gopher",     proxyGopher)
      rv = prefs->SetIntPref ("network.proxy.gopher_port", proxyGopherPort);
      _SetString("network.proxy.socks",      proxySOCKS)
      rv = prefs->SetIntPref ("network.proxy.socks_port",  proxySOCKSPort);

      if (proxySOCKSRadio == 1) proxySOCKSVersion = 5;    // the radio button state
      else proxySOCKSVersion = 4;
      rv = prefs->SetIntPref ("network.proxy.socks_version",  proxySOCKSVersion);


      _SetString("network.proxy.autoconfig_url",proxyAutoURL)
      _SetString("network.proxy.no_proxies_on",proxyNoProxy)

      rv = prefs->SetIntPref("network.proxy.type", proxyType);

      //  -- Advanced
      rv = prefs->SetBoolPref("javascript.enabled", bJavascriptEnabled);
      rv = prefs->SetBoolPref("security.enable_java", bJavaEnabled);
      rv = prefs->SetBoolPref("signon.rememberSignons", bRememberSignons);

      rv = prefs->SetIntPref("network.cookie.cookieBehavior", iCookiesEnabled);
      rv = prefs->SetIntPref("permissions.default.image", iImagesEnabled+1);
      _SetString("network.http.version",httpVersion)

      if (bAnimationsEnabled)    // "once" "none" "normal"
         _SetString("image.animation_mode", _T("normal"))
      else
         _SetString("image.animation_mode", _T("none"))


      // -- Privacy

      if (!userAgent.IsEmpty())
         _SetString("general.useragent.override",userAgent)
      else
         rv = prefs->ClearUserPref("general.useragent.override");

      if (bRestrictPopups)
         _SetString("capability.policy.restrictedpopups.Window.open",_T("noAccess"))
      else
         _SetString("capability.policy.restrictedpopups.Window.open",_T("allAccess"))
      _SetString("capability.policy.restrictedpopups.sites",restrictedPopupSites)

      rv = prefs->SetBoolPref("dom.disable_open_during_load", bDisablePopupsOnLoad);

     if (!theApp.m_pMostRecentBrowserFrame || !(theApp.m_pMostRecentBrowserFrame->m_style & WS_POPUP)) {
       // -- Display settings
       rv = prefs->SetBoolPref("kmeleon.display.maximized", bMaximized);
       rv = prefs->SetIntPref("kmeleon.display.width", windowWidth);
       rv = prefs->SetIntPref("kmeleon.display.height", windowHeight);
       rv = prefs->SetIntPref("kmeleon.display.XPos", windowXPos);
       rv = prefs->SetIntPref("kmeleon.display.YPos", windowYPos);
     }

       rv = prefs->SetIntPref("kmeleon.general.saveType", iSaveType);
       rv = prefs->SetBoolPref("kmeleon.display.backgroundImageEnabled", bToolbarBackground);
       _SetString("kmeleon.display.backgroundImage",toolbarBackground)

      rv = prefs->SetIntPref("kmeleon.display.newWindowOpenAs", iNewWindowOpenAs);
      _SetString("kmeleon.display.newWindowURL",newWindowURL)

      rv = prefs->SetBoolPref("kmeleon.display.disableResize", bDisableResize);

      rv = prefs->SetBoolPref("kmeleon.display.NewWindowHasUrlFocus", bNewWindowHasUrlFocus);
	  rv = prefs->SetIntPref("font.minimum-size.x-western", iFontMinSize);
	  rv = prefs->SetIntPref("font.minimum-size.x-unicode", iFontMinSize);

      // -- Find settings
	   rv = prefs->SetBoolPref("kmeleon.find.matchCase", bFindMatchCase);
// don't set this - it might confuse the users :)  (okay, we *should* remove all references to it, perhaps, but I'm being lazy - sorry)
//	   rv = prefs->SetBoolPref("kmeleon.find.matchWholeWord", bFindMatchWholeWord);
	   rv = prefs->SetBoolPref("kmeleon.find.wrapAround", bFindWrapAround);
	   rv = prefs->SetBoolPref("kmeleon.find.searchBackwards", bFindSearchBackwards);


      // -- Print Settings
      _SetString("kmeleon.print.headerLeft",printHeaderLeft)
      _SetString("kmeleon.print.headerMiddle",printHeaderMiddle)
      _SetString("kmeleon.print.headerRight",printHeaderRight)

      _SetString("kmeleon.print.footerLeft",printFooterLeft)
      _SetString("kmeleon.print.footerMiddle",printFooterMiddle)
      _SetString("kmeleon.print.footerRight",printFooterRight)
      
      rv = prefs->SetBoolPref("kmeleon.print.BGColors", printBGColors);
      rv = prefs->SetBoolPref("kmeleon.print.BGImages", printBGImages);

      _SetString("kmeleon.print.marginLeft",printMarginLeft)
      _SetString("kmeleon.print.marginRight",printMarginRight)
      _SetString("kmeleon.print.marginTop",printMarginTop)
      _SetString("kmeleon.print.marginBottom",printMarginBottom)
      
	  rv = prefs->SetBoolPref("kmeleon.print.shrinkToFit", printShrinkToFit);
      rv = prefs->SetIntPref("kmeleon.print.scaling", printScaling);
      rv = prefs->SetIntPref("kmeleon.print.paperUnit", printUnit);
      _SetString("kmeleon.print.paperWidth",printWidth)
      _SetString("kmeleon.print.paperHeight",printHeight)
      
      

	  	  // -- Download

	  prefs->SetIntPref("kmeleon.general.saveType", iSaveType);
      _SetString("kmeleon.download.saveDir", saveDir);   
	  _SetString("kmeleon.download.dir", downloadDir);
	  _SetString("kmeleon.download.lastDir", lastDownloadDir);

	  
	  prefs->SetBoolPref("kmeleon.download.useDownloadDir", bUseDownloadDir);
	  prefs->SetBoolPref("kmeleon.download.askOpenSave", bAskOpenSave);
	  prefs->SetBoolPref("kmeleon.download.showMinimizedDialog", bShowMinimized);
	  prefs->SetBoolPref("kmeleon.download.flashWhenCompleted", bFlashWhenCompleted);
	  prefs->SetBoolPref("kmeleon.download.closeDownloadDialog", bCloseDownloadDialog);
	  prefs->SetBoolPref("kmeleon.download.saveUseTitle", bSaveUseTitle);

      rv = prefs->SavePrefFile(nsnull);   
   
   }
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

int CPreferences::GetBool(const char *preference, int defaultVal){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      PRBool tempBool;
      rv = prefs->GetBoolPref(preference, &tempBool);
      if (NS_SUCCEEDED(rv))
         return tempBool;
      else
         return defaultVal;
   }
   else return defaultVal;
}

void CPreferences::SetBool(const char *preference, int value){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));

   if (NS_SUCCEEDED(rv))
      prefs->SetBoolPref(preference, value);

}

int CPreferences::GetInt(const char *preference, int defaultVal){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      PRInt32 tempInt;
      rv = prefs->GetIntPref(preference, &tempInt);
      if (NS_SUCCEEDED(rv))
         return tempInt;
      else
         return defaultVal;
   }
   else return defaultVal;
}

void CPreferences::SetInt(const char *preference, int value){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));

   if (NS_SUCCEEDED(rv))
      prefs->SetIntPref(preference, value);

}

int CPreferences::GetString(const char *preference, char *retVal, char *defaultVal){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      nsEmbedCString string;
	  rv = prefs->CopyCharPref(preference, getter_Copies(string));  
	  if (NS_FAILED(rv))                         
        string = defaultVal;	
      	  
      if (retVal)
         strcpy(retVal, string.get());
      return string.Length();
   }
   return 0;
}

int CPreferences::GetString(const char *preference, wchar_t *retVal, wchar_t *defaultVal){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      nsEmbedString string;
	  rv = prefs->CopyUnicharPref(preference, getter_Copies(string));  
	  if (NS_FAILED(rv))                         
        string = defaultVal;	
      if (retVal)
         wcscpy(retVal, string.get());
      return string.Length();
   }
   return 0;
}

void CPreferences::SetString(const char *preference, wchar_t *value){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      prefs->SetUnicharPref(preference, value);
   }
}

void CPreferences::SetString(const char *preference, char *value){
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      prefs->SetCharPref(preference, value);
   }
}

void CPreferences::Clear(const char *preference) {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      prefs->ClearUserPref(preference);
   }   
}

void CPreferences::DeleteBranch(const char *startingAt) {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      prefs->DeleteBranch(startingAt);
   }
}

