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
//  Holds various prefernces for k-meleon. also has the getters/setters

#include "StdAfx.h"

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
   NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
   if (NS_SUCCEEDED(rv)) {	  

      PRBool inited;
      rv = prefs->GetBoolPref("kmeleon.prefs_inited", &inited);
      if (NS_FAILED(rv) || !inited) {
         // Set up prefs for first run
         rv = prefs->SetBoolPref(_T("kmeleon.prefs_inited"), PR_TRUE);
         rv = prefs->SavePrefFile();
      }

      _GetBool(_T("kmeleon.display.maximized"), bMaximized, true);
      _GetInt(_T("kmeleon.display.cx"), posCX, -1);
      _GetInt(_T("kmeleon.display.cy"), posCY, -1);

      _GetBool(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground, true);

      _GetString(_T("kmeleon.display.backgroundImage"), toolbarBackground, _T(""));

      _GetBool(_T("kmeleon.general.startHome"), bStartHome, true);
      _GetString(_T("kmeleon.general.homePage"), homePage, _T("http://www.kmeleon.org"));

      _GetString(_T("kmeleon.general.searchEngine"), searchEngine, _T("http://www.google.com/keyword/"));

      _GetBool(_T("kmeleon.general.sourceEnabled"), sourceEnabled, false);
      _GetString(_T("kmeleon.general.sourceCommand"), sourceCommand, _T(""));
    
      _GetString(_T("kmeleon.general.settingsDir"), settingsDir, _T(""));
    
      if (settingsDir.IsEmpty()) {
         nsCOMPtr<nsIFile> profileDir;
         rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDir));
         NS_ASSERTION(profileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");

         if (NS_SUCCEEDED(rv)){
            nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(profileDir));
            NS_ASSERTION(localFile, "Cannot get nsILocalFile from profile dir");

            nsXPIDLCString descriptorString;
            rv = localFile->GetPersistentDescriptor(getter_Copies(descriptorString));

            settingsDir = descriptorString;
         }
      }
      if (settingsDir[settingsDir.GetLength() - 1] != '\\')
         settingsDir += '\\';

      _GetString(_T("network.proxy.http"), proxyHttp, _T("proxy"));
      _GetInt(_T("network.proxy.http_port"), proxyHttpPort, 80);

      _GetString(_T("network.proxy.no_proxies_on"), proxyNoProxy, _T("localhost"));

      _GetInt(_T("network.proxy.type"), proxyType, 0);

      _GetBool(_T("javascript.enabled"), bJavascriptEnabled, true);
      _GetBool(_T("security.enable_java"), bJavaEnabled, true);
      _GetBool(_T("css.allow"), bCSSEnabled, true);

      _GetInt(_T("network.accept_cookies"), bCookiesEnabled, 0);
      if (bCookiesEnabled == 2)  // 0 = Always, 1 = warn, 2 = never
         bCookiesEnabled = false;
      else
         bCookiesEnabled = true;

      _GetBool(_T("advanced.always_load_images"), bImagesEnabled, true);

      CString animationMode;
      _GetString(_T("image.animation_mode"), animationMode, _T("normal"));
      if (animationMode == _T("normal"))  // "once" "none" "normal"
         bAnimationsEnabled = true;
      else
         bAnimationsEnabled = false;
   }
   else
      NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

void CPreferences::Save() {
   nsresult rv;
   NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
   if (NS_SUCCEEDED(rv)) {

      rv = prefs->SetBoolPref(_T("kmeleon.display.maximized"), bMaximized);
      rv = prefs->SetIntPref(_T("kmeleon.display.cx"), posCX);
      rv = prefs->SetIntPref(_T("kmeleon.display.cy"), posCY);
      rv = prefs->SetBoolPref(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground);
      rv = prefs->SetCharPref(_T("kmeleon.display.backgroundImage"), toolbarBackground);

    //

    rv = prefs->SetBoolPref(_T("kmeleon.general.startHome"), bStartHome);
    rv = prefs->SetCharPref(_T("kmeleon.general.homePage"), homePage);

    rv = prefs->SetCharPref(_T("kmeleon.general.settingsDir"), settingsDir);

    rv = prefs->SetBoolPref(_T("kmeleon.general.sourceEnabled"), sourceEnabled);
    rv = prefs->SetCharPref(_T("kmeleon.general.sourceCommand"), sourceCommand);

    //

    rv = prefs->SetCharPref(_T("network.proxy.http"), proxyHttp);
    rv = prefs->SetIntPref(_T("network.proxy.http_port"), proxyHttpPort);

    rv = prefs->SetCharPref(_T("network.proxy.no_proxies_on"), proxyNoProxy);

    rv = prefs->SetIntPref(_T("network.proxy.type"), proxyType);

    //

    rv = prefs->SetBoolPref(_T("javascript.enabled"), bJavascriptEnabled);
    rv = prefs->SetBoolPref(_T("security.enable_java"), bJavaEnabled);
    rv = prefs->SetBoolPref(_T("css.allow"), bCSSEnabled);
    if (bCookiesEnabled){ // 0 = Always, 1 = warn, 2 = never
      rv = prefs->SetIntPref(_T("network.accept_cookies"), 0);
    }else{
      rv = prefs->SetIntPref(_T("network.accept_cookies"), 2);
    }
    rv = prefs->SetBoolPref(_T("advanced.always_load_images"), bImagesEnabled);
    if (bAnimationsEnabled){ // "once" "none" "normal"
      rv = prefs->SetCharPref(_T("image.animation_mode"), _T("normal"));
    }else{
      rv = prefs->SetCharPref(_T("image.animation_mode"), _T("none"));
    }

    rv = prefs->SavePrefFile();
  }
  else
    NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

int CPreferences::GetBool(const char *preference, int defaultVal){
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
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
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    prefs->SetBoolPref(preference, value);
  }
}
int CPreferences::GetInt(const char *preference, int defaultVal){
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
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
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    prefs->SetIntPref(preference, value);
  }
}
void CPreferences::GetString(const char *preference, char *retVal, char *defaultVal){
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    CString string;
    _GetString(preference, string, defaultVal);
    strcpy(retVal, string);
  }
}
void CPreferences::SetString(const char *preference, char *value){
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    prefs->SetCharPref(preference, value);
  }
}

