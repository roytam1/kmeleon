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

// this handles plugin loading/unloading

#include "StdAfx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserView.h"
#include "BrowserFrm.h"

#include "kmeleon_plugin.h"
#include "Plugins.h"
#include "Utils.h"

int SessionSize=0;
char **pHistory;

CPlugins::CPlugins() {
   configBuffer = NULL;
   loadLine = NULL;
   dontLoadLine = NULL;
}

CPlugins::~CPlugins(){
   if (configBuffer){
      delete [] configBuffer;
   }
   if (SessionSize) {
      for (int i=0; i<SessionSize; i++)
         delete pHistory[i];
   }
   delete pHistory;
   UnLoadAll();
}

// returns a pointer to the char after the last \ or /
const char *FileNoPath(const char *filepath){
   char *p1 = strrchr(filepath, '\\');
   char *p2 = strrchr(filepath, '/');
   if (p1 > p2) {
      return p1 + 1;
   }
   else if (p2 > p1) {
      return p2 + 1;
   }
   else {
      return filepath;
   }
}


UINT currentCmdID = PLUGIN_COMMAND_START_ID;
UINT GetCommandIDs(int num) {
   UINT freeID = currentCmdID;
   currentCmdID += num;
   return freeID;
}

int CPlugins::OnUpdate(UINT command){
   if (command >= 2000 && command <= currentCmdID)
      return true;
   return false;
}

void CPlugins::OnCreate(HWND wnd){
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos){
      pluginList.GetNextAssoc( pos, s, kPlugin);
      if (kPlugin && kPlugin->pf->Create) 
         kPlugin->pf->Create(wnd);
   }
}

void NavigateTo(char *url, int windowState){
   CBrowserFrame *mainFrame = theApp.m_pMostRecentBrowserFrame;

   switch(windowState) {
   case OPEN_NORMAL:
      mainFrame->m_wndBrowserView.OpenURL(url);
      break;
   case OPEN_NEW:
      mainFrame->m_wndBrowserView.OpenURLInNewWindow(NS_ConvertASCIItoUCS2(url).GetUnicode());
      break;
   case OPEN_BACKGROUND:
      mainFrame->m_wndBrowserView.OpenURLInNewWindow(NS_ConvertASCIItoUCS2(url).GetUnicode(), true);
      break;
   }
}

static kmeleonDocInfo kDocInfo;
kmeleonDocInfo * GetDocInfo(HWND mainWnd) {
   CBrowserFrame *frame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);

   if (!frame){
      return NULL;
   }

   CString url;
   frame->m_wndUrlBar.GetEnteredURL(url);
   if (url.GetLength() >= MAX_URL)
      return NULL;

   CString title;
   frame->GetWindowText(title);
   title.Replace(" (K-Meleon)", "");

   strcpy(kDocInfo.title, title);
   strcpy(kDocInfo.url, url);

   return &kDocInfo;
}

void GetPreference(enum PREFTYPE type, char *preference, void *ret, void *defVal) {
   switch (type){
   case PREF_BOOL:
      *(int *)ret = theApp.preferences.GetBool(preference, *(int *)defVal);
      break;
   case PREF_INT:
      *(int *)ret = theApp.preferences.GetInt(preference, *(int *)defVal);
      break;
   case PREF_STRING:
      theApp.preferences.GetString(preference, (char *)ret, (char *)defVal);
      break;
   }
}

void SetPreference(enum PREFTYPE type, char *preference, void *val) {
   switch (type){
      case PREF_BOOL:
         theApp.preferences.SetBool(preference, *(int *)val);
         break;
      case PREF_INT:
         theApp.preferences.SetInt(preference, *(int *)val);
         break;
      case PREF_STRING:
         theApp.preferences.SetString(preference, (char *)val);
         break;
   }
}


int GetMozillaSessionHistory (char ***titles, int *count, int *index) {

   nsresult result;
   int i;

	if (!theApp.m_pMostRecentBrowserFrame)	return FALSE;

	nsCOMPtr<nsISHistory> h;

   result = theApp.m_pMostRecentBrowserFrame->m_wndBrowserView.mWebNav->GetSessionHistory(getter_AddRefs (h));

   if (!NS_SUCCEEDED (result) || (!h)) return FALSE;

   h->GetCount (count);
	h->GetIndex (index);

   // Clear the previous table
   if (SessionSize) {
      for (i=0; i<SessionSize; i++)
         delete pHistory[i];
   }

   SessionSize = *count;

   delete pHistory;
   pHistory = new char *[SessionSize];

	nsCOMPtr<nsIHistoryEntry> he;
	PRUnichar *title;
	for (i=0; i < SessionSize; i++) {
		result = h->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs (he));
		if (!NS_SUCCEEDED(result) || (!he)) return FALSE;

		result = he->GetTitle (&title);
		if (!NS_SUCCEEDED(result) || (!title)) return FALSE;

		// The title is in 16-bit unicode, this converts it to 8bit (UTF)
		int len;
		len = WideCharToMultiByte(CP_ACP, 0, title, -1, 0, 0, NULL, NULL);
		char *s = new char[len+1];
		len = WideCharToMultiByte(CP_ACP, 0, title, -1, s, len, NULL, NULL);
      s[len] = 0;
      pHistory[i] = s;
	}

   *titles = pHistory;
	return TRUE;
}

void GotoHistoryIndex(UINT index) {
	CBrowserFrame	*mainFrame = theApp.m_pMostRecentBrowserFrame;
	if (mainFrame)
		mainFrame->m_wndBrowserView.mWebNav->GotoIndex(index);
}

void RegisterBand(HWND hWnd, char *name) {
   theApp.m_pMostRecentBrowserFrame->m_wndReBar.RegisterBand(hWnd, name);
}

kmeleonFunctions kmelFuncs = {
   GetCommandIDs,
   NavigateTo,
   GetDocInfo,
   GetPreference,
   SetPreference,
   GetMozillaSessionHistory,
   GotoHistoryIndex,
   RegisterBand
};

int CPlugins::TestLoad(const char *file){
   if (!configBuffer){
      FILE *configFile = fopen(theApp.preferences.settingsDir + "plugins.cfg", "r");
      if (configFile){
         fseek(configFile, 0, SEEK_END);
         int length = ftell(configFile);
         fseek(configFile, 0, SEEK_SET);
         configBuffer = new char[length];

         fread(configBuffer, sizeof(char), length, configFile);

         loadLine = strstr(configBuffer, "[load]");
         dontLoadLine = strstr(configBuffer, "[dontload]");
      }
   }
   if (configBuffer){
      char *pluginLine = strstr(configBuffer, file);
      if (pluginLine){
         if (loadLine > dontLoadLine){
            // [dontload]
            // [load]
            if (pluginLine > loadLine){
               return 1;
            }else{
               return 0;
            }
         }else{
            // [load]
            // [dontload]
            if (pluginLine > dontLoadLine){
               return 0;
            }else{
               return 1;
            }
         }
      }else{
         return -1;
      }
   }else{
      return -1;
   }
}

kmeleonPlugin * CPlugins::Load(const char *file){
   kmeleonPlugin * kPlugin;
   if (pluginList.Lookup(FileNoPath(file), kPlugin)){
      return kPlugin; // it's already loaded
   }

   HINSTANCE plugin;

   int x=strlen(file);
   while (x>0 && file[x] != '\\' && file[x] != '/') x--;

   const char *noPath = FileNoPath(file);
   int testLoad = TestLoad(noPath); // 1 load, 0 don't load, -1 not found
   if (testLoad == 0){
      return NULL;
   }

   if (x==0) {       // if pattern does not contain \ or / we need to prepend pluginsDir
      char buf[MAX_PATH];
      strcpy(buf, theApp.preferences.pluginsDir);
      strcat(buf, file);
      plugin = LoadLibrary(buf);    // load the full path
   }
   else plugin = LoadLibrary(file);

   KmeleonPluginGetter kpg = (KmeleonPluginGetter)GetProcAddress(plugin, "GetKmeleonPlugin");

   if (!kpg){
      FreeLibrary(plugin);
      return 0;
   }

   kPlugin = kpg();

   if (!kPlugin){
      FreeLibrary(plugin);
      return 0;
   }

   kPlugin->hParentInstance = AfxGetInstanceHandle();
   kPlugin->hDllInstance = plugin;
  
   kPlugin->kf = &kmelFuncs;

   kPlugin->pf->Init();

   pluginList.SetAt(noPath, kPlugin);

   return kPlugin;
}

int CPlugins::FindAndLoad(char *pattern = "*.dll"){
   CString filepath;
   CFileFind finder;
   BOOL bWorking;

   int x=strlen(pattern);
   while (x>0 && pattern[x] != '\\' && pattern[x] != '/') x--;

   if (x==0) {       // if pattern does not contain \ or / we need to prepend pluginsDir
      CString search = theApp.preferences.pluginsDir + pattern;
      bWorking = finder.FindFile(search);
   }
   else bWorking = finder.FindFile(pattern);

   int i = 0;
   while (bWorking) {
      bWorking = finder.FindNextFile();

      filepath = finder.GetFilePath();
      if ( Load(filepath) )
         i++;
   }
   return i;
}

void CPlugins::UnLoadAll(){
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos){
      pluginList.GetNextAssoc( pos, s, kPlugin);
      if (kPlugin){
         kPlugin->pf->Quit();
         FreeLibrary(kPlugin->hDllInstance);
      }
   }
   pluginList.RemoveAll();
   currentCmdID = PLUGIN_COMMAND_START_ID;
}

void CPlugins::DoRebars(HWND rebarWnd){
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc( pos, s, kPlugin);
      if (kPlugin && kPlugin->pf->DoRebar)
         kPlugin->pf->DoRebar(rebarWnd);
   }
}
