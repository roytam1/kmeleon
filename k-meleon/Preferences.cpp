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
extern CMfcEmbedApp theApp;

#include "Preferences.h"

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

#define _GetString(_pref, _value, _defaultValue) { \
    nsXPIDLCString tempString;                    \
    rv = prefs->CopyCharPref(_pref, getter_Copies(tempString));  \
    if (NS_SUCCEEDED(rv))                         \
      _value = tempString;                        \
    else                                          \
      _value = _defaultValue;                     \
  }

void CPreferences::Load() {

   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {	  

      PRBool inited;
      rv = prefs->GetBoolPref("kmeleon.prefs_inited", &inited);
      if (NS_FAILED(rv) || !inited) {
         // Set up prefs for first run
         rv = prefs->SetBoolPref(_T("kmeleon.prefs_inited"), PR_TRUE);
         rv = prefs->SavePrefFile(nsnull);
      }


      // -- Display settings

      _GetBool(_T("kmeleon.display.hideTaskBarButtons"), bHideTaskBarButtons, false);
      
      _GetBool(_T("kmeleon.display.maximized"), bMaximized, true);

      _GetInt(_T("kmeleon.display.width"), windowWidth, -1);
      _GetInt(_T("kmeleon.display.height"), windowHeight, -1);
      _GetInt(_T("kmeleon.display.XPos"), windowXPos, -1);
      _GetInt(_T("kmeleon.display.YPos"), windowYPos, -1);

      _GetBool(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground, true);

      _GetString(_T("kmeleon.display.backgroundImage"), toolbarBackground, _T(""));

      _GetInt(_T("kmeleon.display.newWindowOpenAs"), iNewWindowOpenAs, 0);
      _GetString(_T("kmeleon.display.newWindowURL"), newWindowURL, _T(""));

      _GetBool(_T("kmeleon.display.disableResize"), bDisableResize, false);
      
      _GetBool(_T("kmeleon.display.NewWindowHasUrlFocus"), bNewWindowHasUrlFocus, true);

      // -- Find settings
      
	   _GetBool(_T("kmeleon.find.matchCase"), bFindMatchCase, false);
//	   _GetBool(_T("kmeleon.find.matchWholeWord"), bFindMatchWholeWord, false);
	   _GetBool(_T("kmeleon.find.searchBackwards"), bFindSearchBackwards, false);
	   _GetBool(_T("kmeleon.find.wrapAround"), bFindWrapAround, false);
      
      // -- General preferences

      _GetBool(_T("kmeleon.general.startHome"), bStartHome, true);
      _GetString(_T("kmeleon.general.homePage"), homePage, _T("http://kmeleon.sourceforge.net/start"));

      _GetString(_T("kmeleon.general.searchEngine"), searchEngine, _T("http://www.google.com/search?q="));

      _GetBool(_T("kmeleon.general.sourceEnabled"), bSourceUseExternalCommand, false);
      _GetString(_T("kmeleon.general.sourceCommand"), sourceCommand, _T(""));

      _GetString(_T("kmeleon.general.saveDir"), saveDir, _T(""));   
      _GetString(_T("kmeleon.general.settingsDir"), settingsDir, _T(""));
      _GetString(_T("kmeleon.general.pluginsDir"), pluginsDir, _T(""));

      _GetString(_T("kmeleon.general.skinsDir"), skinsDir, _T(""));
      _GetString(_T("kmeleon.general.skinsCurrent"), skinsCurrent, _T(""));

      
      char appDir[MAX_PATH];
      GetModuleFileName(NULL, appDir, MAX_PATH);
      int x=strlen(appDir)-1;
      while (x>0 && appDir[x] != '\\') x--;
      if (x>0) appDir[x+1]=0;

      

      nsCOMPtr<nsIFile> nsProfileDir;
      rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(nsProfileDir));
      NS_ASSERTION(nsProfileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");
      
      if (NS_SUCCEEDED(rv)){         
         nsCAutoString pathBuf;
         rv = nsProfileDir->GetNativePath(pathBuf);
         profileDir = pathBuf.get();

         if (profileDir.Right(1) != '\\')
            profileDir += '\\';
      }
      else
         profileDir = appDir;



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
      _GetString(_T("browser.cache.disk.parent_directory"), cacheDir, _T(""));
      if (cacheDir.IsEmpty())
         cacheDir = settingsDir;
      else if (cacheDir[cacheDir.GetLength() - 1] != '\\')
         cacheDir += '\\';

      _GetInt(_T("browser.cache.disk.capacity"), cacheDisk, 4096);
      _GetInt(_T("browser.cache.memory.capacity"), cacheMemory, 1024);
      _GetInt(_T("browser.cache.check_doc_frequency"), cacheCheckFrequency, 0);

      // -- Proxies      
      
      
      _GetString  (_T("network.proxy.http"),          proxyHTTP,     "");
      _GetInt     (_T("network.proxy.http_port"),     proxyHTTPPort, 0);
      _GetString  (_T("network.proxy.ftp"),           proxyFTP,     "");
      _GetInt     (_T("network.proxy.ftp_port"),      proxyFTPPort,  0);
      _GetString  (_T("network.proxy.ssl"),           proxySSL,     "");
      _GetInt     (_T("network.proxy.ssl_port"),      proxySSLPort, 0);
      _GetString  (_T("network.proxy.gopher"),        proxyGopher,     "");
      _GetInt     (_T("network.proxy.gopher_port"),   proxyGopherPort, 0);
      _GetString  (_T("network.proxy.socks"),         proxySOCKS,     "");
      _GetInt     (_T("network.proxy.socks_port"),    proxySOCKSPort, 0);
      _GetInt     (_T("network.proxy.socks_version"), proxySOCKSVersion, 5);

      if (proxySOCKSVersion == 5) proxySOCKSRadio = 1;    // the radio button state
      else proxySOCKSRadio = 0;

      _GetString(_T("network.proxy.autoconfig_url"), proxyAutoURL, "");
      _GetString(_T("network.proxy.no_proxies_on"), proxyNoProxy, _T("localhost"));

      _GetInt(_T("network.proxy.type"), proxyType, 0);


      // -- Advanced     
      
      _GetBool(_T("javascript.enabled"), bJavascriptEnabled, true);
      _GetBool(_T("security.enable_java"), bJavaEnabled, true);
      _GetBool(_T("signon.rememberSignons"), bRememberSignons, true);

      // 0 = Always, 1 = site, 2 = never
      _GetInt(_T("network.cookie.cookieBehavior"), iCookiesEnabled, 0);

      // 0 = Always, 1 = site, 2 = never
      _GetInt(_T("network.image.imageBehavior"), iImagesEnabled, 0);

      _GetString(_T("network.http.version"), httpVersion, "");

      CString animationMode;
      _GetString(_T("image.animation_mode"), animationMode, "");
      if (animationMode == _T("normal"))  // "once" "none" "normal"
         bAnimationsEnabled = true;
      else
         bAnimationsEnabled = false;


      // -- Privacy

      _GetString(_T("general.useragent.override"), userAgent, "");

      CString restrictPopups;
      _GetString(_T("capability.policy.restrictedpopups.Window.open"), restrictPopups, "");
      if (restrictPopups == _T("noAccess"))
         bRestrictPopups = TRUE;
      else
         bRestrictPopups = FALSE;
         
      _GetString(_T("capability.policy.restrictedpopups.sites"), restrictedPopupSites, "");

      _GetBool(_T("dom.disable_open_during_load"), bDisablePopupsOnLoad, false);



      // -- Printing


      _GetString(_T("kmeleon.print.headerLeft"), printHeaderLeft, "&T");
      _GetString(_T("kmeleon.print.headerMiddle"), printHeaderMiddle, "");
      _GetString(_T("kmeleon.print.headerRight"), printHeaderRight, "&U");

      _GetString(_T("kmeleon.print.footerLeft"), printFooterLeft, "&PT");
      _GetString(_T("kmeleon.print.footerMiddle"), printFooterMiddle, "");
      _GetString(_T("kmeleon.print.footerRight"), printFooterRight, "&D");
      
      _GetBool(_T("kmeleon.print.BGColors"), printBGColors, false);
      _GetBool(_T("kmeleon.print.BGImages"), printBGImages, false);

      _GetString(_T("kmeleon.print.marginLeft"), printMarginLeft, "0.5");
      _GetString(_T("kmeleon.print.marginRight"), printMarginRight, "0.5");
      _GetString(_T("kmeleon.print.marginTop"), printMarginTop, "0.5");
      _GetString(_T("kmeleon.print.marginBottom"), printMarginBottom, "0.5");
      
      _GetInt(_T("kmeleon.print.scaling"), printScaling, 100);
      _GetInt(_T("kmeleon.print.paperUnit"), printUnit, nsIPrintSettings::kPaperSizeInches);
      _GetString(_T("kmeleon.print.paperWidth"), printWidth, "8.5");
      _GetString(_T("kmeleon.print.paperHeight"), printHeight, "11");

   }
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

void CPreferences::SaveDlgPrefs() {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      // -- General preferences
      rv = prefs->SetBoolPref(_T("kmeleon.general.startHome"), bStartHome);
      rv = prefs->SetCharPref(_T("kmeleon.general.homePage"), homePage);

      rv = prefs->SetCharPref(_T("kmeleon.general.saveDir"), saveDir);
      rv = prefs->SetCharPref(_T("kmeleon.general.settingsDir"), settingsDir);
      rv = prefs->SetCharPref(_T("kmeleon.general.pluginsDir"), pluginsDir);

      rv = prefs->SetCharPref(_T("kmeleon.general.skinsDir"), skinsDir);
      rv = prefs->SetCharPref(_T("kmeleon.general.skinsCurrent"), skinsCurrent);

      rv = prefs->SetBoolPref(_T("kmeleon.general.sourceEnabled"), bSourceUseExternalCommand);
      rv = prefs->SetCharPref(_T("kmeleon.general.sourceCommand"), sourceCommand);

      // -- Cache
      rv = prefs->SetCharPref(_T("browser.cache.disk.parent_directory"), cacheDir);
      rv = prefs->SetIntPref(_T("browser.cache.disk.capacity"), cacheDisk);

      rv = prefs->SetIntPref(_T("browser.cache.memory.capacity"), cacheMemory);
      rv = prefs->SetIntPref(_T("browser.cache.check_doc_frequency"), cacheCheckFrequency);

      // -- Proxies
      rv = prefs->SetCharPref(_T("network.proxy.http"),        proxyHTTP);
      rv = prefs->SetIntPref (_T("network.proxy.http_port"),   proxyHTTPPort);
      rv = prefs->SetCharPref(_T("network.proxy.ftp"),         proxyFTP);
      rv = prefs->SetIntPref (_T("network.proxy.ftp_port"),    proxyFTPPort);
      rv = prefs->SetCharPref(_T("network.proxy.ssl"),         proxySSL);
      rv = prefs->SetIntPref (_T("network.proxy.ssl_port"),    proxySSLPort);
      rv = prefs->SetCharPref(_T("network.proxy.gopher"),      proxyGopher);
      rv = prefs->SetIntPref (_T("network.proxy.gopher_port"), proxyGopherPort);
      rv = prefs->SetCharPref(_T("network.proxy.socks"),       proxySOCKS);
      rv = prefs->SetIntPref (_T("network.proxy.socks_port"),  proxySOCKSPort);

      if (proxySOCKSRadio == 1) proxySOCKSVersion = 5;    // the radio button state
      else proxySOCKSVersion = 4;
      rv = prefs->SetIntPref (_T("network.proxy.socks_version"),  proxySOCKSVersion);


      rv = prefs->SetCharPref(_T("network.proxy.autoconfig_url"), proxyAutoURL);
      rv = prefs->SetCharPref(_T("network.proxy.no_proxies_on"), proxyNoProxy);

      rv = prefs->SetIntPref(_T("network.proxy.type"), proxyType);

      //  -- Advanced
      rv = prefs->SetBoolPref(_T("javascript.enabled"), bJavascriptEnabled);
      rv = prefs->SetBoolPref(_T("security.enable_java"), bJavaEnabled);
      rv = prefs->SetBoolPref(_T("signon.rememberSignons"), bRememberSignons);

      rv = prefs->SetIntPref(_T("network.cookie.cookieBehavior"), iCookiesEnabled);
      rv = prefs->SetIntPref(_T("network.image.imageBehavior"), iImagesEnabled);
      rv = prefs->SetCharPref(_T("network.http.version"), httpVersion);

      if (bAnimationsEnabled)    // "once" "none" "normal"
         rv = prefs->SetCharPref(_T("image.animation_mode"), _T("normal"));
      else
         rv = prefs->SetCharPref(_T("image.animation_mode"), _T("none"));


      // -- Privacy

      if (!userAgent.IsEmpty())
         rv = prefs->SetCharPref(_T("general.useragent.override"), userAgent);
      else
         rv = prefs->ClearUserPref(_T("general.useragent.override"));

      if (bRestrictPopups)
         rv = prefs->SetCharPref(_T("capability.policy.restrictedpopups.Window.open"), "noAccess");
      else
         rv = prefs->SetCharPref(_T("capability.policy.restrictedpopups.Window.open"), "allAccess");
      rv = prefs->SetCharPref(_T("capability.policy.restrictedpopups.sites"), restrictedPopupSites);

      rv = prefs->SetBoolPref(_T("dom.disable_open_during_load"), bDisablePopupsOnLoad);

      Save();
   }
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

void CPreferences::Save() {
   nsresult rv;
   nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv));
   if (NS_SUCCEEDED(rv)) {
      // NOTE: The prefs set in this function will not be settable in a macro!
      //       This is why most were moved into SaveDlgPrefs(), so they wouldn't
      //        be overwritten when trying to save them from a macro.
      //       Those left here can be changed outside of the prefs dialog, so
      //        they're left in here for now.

      // -- Display settings
      rv = prefs->SetBoolPref(_T("kmeleon.display.maximized"), bMaximized);
      rv = prefs->SetIntPref(_T("kmeleon.display.width"), windowWidth);
      rv = prefs->SetIntPref(_T("kmeleon.display.height"), windowHeight);
      rv = prefs->SetIntPref(_T("kmeleon.display.XPos"), windowXPos);
      rv = prefs->SetIntPref(_T("kmeleon.display.YPos"), windowYPos);
      rv = prefs->SetBoolPref(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground);
      rv = prefs->SetCharPref(_T("kmeleon.display.backgroundImage"), toolbarBackground);

      rv = prefs->SetIntPref(_T("kmeleon.display.newWindowOpenAs"), iNewWindowOpenAs);
      rv = prefs->SetCharPref(_T("kmeleon.display.newWindowURL"), newWindowURL);

      rv = prefs->SetBoolPref(_T("kmeleon.display.disableResize"), bDisableResize);

      rv = prefs->SetBoolPref(_T("kmeleon.display.NewWindowHasUrlFocus"), bNewWindowHasUrlFocus);

      // -- Find settings
	   rv = prefs->SetBoolPref(_T("kmeleon.find.matchCase"), bFindMatchCase);
// don't set this - it might confuse the users :)  (okay, we *should* remove all references to it, perhaps, but I'm being lazy - sorry)
//	   rv = prefs->SetBoolPref(_T("kmeleon.find.matchWholeWord"), bFindMatchWholeWord);
	   rv = prefs->SetBoolPref(_T("kmeleon.find.wrapAround"), bFindWrapAround);
	   rv = prefs->SetBoolPref(_T("kmeleon.find.searchBackwards"), bFindSearchBackwards);


      // -- Print Settings
      rv = prefs->SetCharPref(_T("kmeleon.print.headerLeft"), printHeaderLeft);
      rv = prefs->SetCharPref(_T("kmeleon.print.headerMiddle"), printHeaderMiddle);
      rv = prefs->SetCharPref(_T("kmeleon.print.headerRight"), printHeaderRight);

      rv = prefs->SetCharPref(_T("kmeleon.print.footerLeft"), printFooterLeft);
      rv = prefs->SetCharPref(_T("kmeleon.print.footerMiddle"), printFooterMiddle);
      rv = prefs->SetCharPref(_T("kmeleon.print.footerRight"), printFooterRight);
      
      rv = prefs->SetBoolPref(_T("kmeleon.print.BGColors"), printBGColors);
      rv = prefs->SetBoolPref(_T("kmeleon.print.BGImages"), printBGImages);

      rv = prefs->SetCharPref(_T("kmeleon.print.marginLeft"), printMarginLeft);
      rv = prefs->SetCharPref(_T("kmeleon.print.marginRight"), printMarginRight);
      rv = prefs->SetCharPref(_T("kmeleon.print.marginTop"), printMarginTop);
      rv = prefs->SetCharPref(_T("kmeleon.print.marginBottom"), printMarginBottom);
      
      rv = prefs->SetIntPref(_T("kmeleon.print.scaling"), printScaling);
      rv = prefs->SetIntPref(_T("kmeleon.print.paperUnit"), printUnit);
      rv = prefs->SetCharPref(_T("kmeleon.print.paperWidth"), printWidth);
      rv = prefs->SetCharPref(_T("kmeleon.print.paperHeight"), printHeight);
      
      
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
      CString string;
      _GetString(preference, string, defaultVal);
      if (retVal)
         strcpy(retVal, string);
      return string.GetLength();
   }
   return 0;
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

