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
kmeleonDocInfo kDocInfo;

CPlugins::CPlugins() {
}

CPlugins::~CPlugins(){
   if (SessionSize) {
      for (int i=0; i<SessionSize; i++)
         delete pHistory[i];
   }
   delete pHistory;

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.title)
      delete kDocInfo.title;
   
   UnLoadAll();
}

// returns a pointer to the char after the last \ or /
// also strips the .dll
const char *FileNoPath(const char *filepath){
   char *d = strrchr(filepath, '.');
   if (d && strcmpi(d, ".dll") == 0){
      *d = 0;
   }

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
      if (kPlugin->loaded && kPlugin->pf->Create) 
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

kmeleonDocInfo * GetDocInfo(HWND mainWnd) {
   CBrowserFrame *frame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);

   if (!frame){
      return NULL;
   }

   char *url;
   int len = frame->m_wndBrowserView.GetCurrentURI(NULL);
   if (len) {
      char *url = new char[len+1];
      frame->m_wndBrowserView.GetCurrentURI(url);
   }
   else
      url = NULL;

   CString title;
   frame->GetWindowText(title);
   title.Replace(" (K-Meleon)", "");
   char *doctitle = new char[title.GetLength()+1];
   strcpy(doctitle, title);

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.title)
      delete kDocInfo.title;

   kDocInfo.title = doctitle;
   kDocInfo.url = url;

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

BOOL CPlugins::TestLoad(const char *file, const char *description) {

   char preference[128] = "kmeleon.plugins.";
   strcat(preference, file);
   strcat(preference, ".load");
   
   int load = theApp.preferences.GetBool(preference, -1);
   if (load == -1) {
      char buf[256] = "A new plugin was found:\n\n";
      strcat(buf, description);
      strcat(buf, "\n\nWould you like to load this plugin now?");

      if (MessageBox(NULL, buf, "Plugin found", MB_YESNO) == IDYES)
         load = 1;
      else load = 0;
      theApp.preferences.SetBool(preference, load);
   }
   return load;

}

kmeleonPlugin * CPlugins::Load(char *file){
  
   file = SkipWhiteSpace(file);
   TrimWhiteSpace(file);

   kmeleonPlugin * kPlugin;
   if (pluginList.Lookup(FileNoPath(file), kPlugin)){
      return kPlugin; // it's already loaded
   }

   HINSTANCE plugin;

   int x=strlen(file);
   while (x>0 && file[x] != '\\' && file[x] != '/') x--;

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

   // If the plugin is enabled, get its functions and call init
   const char *noPath = FileNoPath(file);
   if (( kPlugin->loaded=TestLoad(noPath, kPlugin->description))) {
      kPlugin->pf->Init();
   }
   // otherwise, make a copy of the descripion, and unload it
   else {
      kmeleonPlugin *temp = new kmeleonPlugin;
      char *sBuf = new char[strlen(kPlugin->description)+1];
      temp->description = sBuf;
      strcpy(temp->description, kPlugin->description);
      temp->loaded = false;
      FreeLibrary(plugin);
      kPlugin=temp;
   }

   // save the filename
   char *name = new char[strlen(noPath)+1];
   strcpy(name, noPath);
   kPlugin->dllname=name;

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
      if ( Load((char *)(const char *)filepath) )
         i++;
   }
   return i;
}

void CPlugins::UnLoadAll(){
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc(pos, s, kPlugin);
      if (kPlugin){
         delete kPlugin->dllname;
         if (kPlugin->loaded) {
            kPlugin->pf->Quit();
            FreeLibrary(kPlugin->hDllInstance);
         }
         else  { // the plugin was disabled, delete the copied description
            delete kPlugin->description;
            delete kPlugin;
         }
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
      if (kPlugin->loaded && kPlugin->pf->DoRebar)
         kPlugin->pf->DoRebar(rebarWnd);
   }
}
