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

#define START_ID 2000

CPlugins::CPlugins(){
}

CPlugins::~CPlugins(){
  UnLoadAll();
}

// returns a pointer to the char after the last \ or /
const char *FileNoPath(const char *filepath){
  char *p1 = strrchr(filepath, '\\');
  char *p2 = strrchr(filepath, '/');
  if (p1 > p2){
    return p1 + 1;
  }
  else if (p2 > p1){
    return p2 + 1;
  }
  else{
    return filepath;
  }
}

UINT currentID = START_ID;
UINT GetCommandIDs(int num){
  UINT freeID = currentID;
  currentID += num;
  return freeID;
}

int CPlugins::OnUpdate(UINT command){
  if (command >= 2000 && command <= currentID){
    return true;
  }
  return false;
}

#if 0
// The new way of doing things is for the plugin to subclass our window
LRESULT CPlugins::OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam){
  /*
  if (!OnUpdate(command)){
    return;
  }
  */
  LRESULT result = 0;
  LRESULT result2;
  POSITION pos = pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  while (pos){
    pluginList.GetNextAssoc( pos, s, kPlugin);
    if (kPlugin && kPlugin->OnMessage){
      result2 = kPlugin->OnMessage(wnd, message, wParam, lParam);
      if (result2)
        result = result2;
    }
  }
  return result;
}
#endif

void CPlugins::OnCreate(HWND wnd){
  POSITION pos = pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  while (pos){
    pluginList.GetNextAssoc( pos, s, kPlugin);
    if (kPlugin && kPlugin->pf->Create){
      kPlugin->pf->Create(wnd);
    }
  }
}

void NavigateTo(char *url, int newWindow){
  CBrowserFrame *mainFrame = (CBrowserFrame *)theApp.m_pMainWnd->GetActiveWindow();
  mainFrame->m_wndBrowserView.OpenURL(url);
}

static kmeleonDocInfo kDocInfo;
kmeleonDocInfo * GetDocInfo(HWND mainWnd){
  CBrowserFrame *frame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);

  if (!frame){
    return NULL;
  }

  CString title;
  frame->GetWindowText(title);
  title.Replace(" (K-Meleon)", "");

  CString url;
  frame->m_wndUrlBar.GetEnteredURL(url);

  strcpy(kDocInfo.title, title);
  strcpy(kDocInfo.url, url);

  return &kDocInfo;
}

void GetPreference(enum PREFTYPE type, char *preference, void *ret, void *defVal){
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

void SetPreference(enum PREFTYPE type, char *preference, void *val){
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

int GetMozillaSessionHistory (char **titles[], int *count, int *index) {

	nsresult result;

	CBrowserFrame	*mainFrame		= (CBrowserFrame *) theApp.m_pMainWnd->GetActiveWindow();
	if (!mainFrame)	return FALSE;

	nsCOMPtr<nsISHistory> h;
	result = mainFrame->m_wndBrowserView.mWebNav->GetSessionHistory(getter_AddRefs (h));
	if (!NS_SUCCEEDED (result) || (!h)) return FALSE;

	h->GetCount (count);
	h->GetIndex (index);

	char **t = (char **) new char[*count * sizeof(char *)];

	nsCOMPtr<nsISHEntry> he;
	PRUnichar *title;
	for (PRInt32 i = 0; i < *count; i++) {

		result = h->GetEntryAtIndex (i, PR_FALSE, getter_AddRefs (he));
		if (!NS_SUCCEEDED(result) || (!he)) return FALSE;

		result = he->GetTitle (&title);
		if (!NS_SUCCEEDED(result) || (!title)) return FALSE;

		// The title is in 16-bit unicode, this converts it to 8bit (UTF)

		int len;
		len = WideCharToMultiByte(CP_ACP, 0, title, -1, 0, 0, NULL, NULL);
		char *s = new(char[len+1]);
		len = WideCharToMultiByte(CP_ACP, 0, title, -1, s, len, NULL, NULL);
		s[len]=0;

		t[i] = s;

	}
	*titles = t;
	return TRUE;
}

void GotoHistoryIndex(UINT index) {
	CBrowserFrame	*mainFrame		= (CBrowserFrame *) theApp.m_pMainWnd->GetActiveWindow();
	if (mainFrame)
		mainFrame->m_wndBrowserView.mWebNav->GotoIndex(index);
}

HWND GetToolbarWnd() {
	CBrowserFrame	*mainFrame		= (CBrowserFrame *) theApp.m_pMainWnd->GetActiveWindow();
	if (mainFrame)
		return mainFrame->m_wndToolBar.m_hWnd;
	else return NULL;
}

kmeleonFunctions kmelFuncs = {
  GetCommandIDs,
  NavigateTo,
  GetDocInfo,
  GetPreference,
  SetPreference,
  GetMozillaSessionHistory,
  GotoHistoryIndex,
  GetToolbarWnd
};

kmeleonPlugin * CPlugins::Load(const char *file){
  kmeleonPlugin * kPlugin;
  if (pluginList.Lookup(FileNoPath(file), kPlugin)){
    return kPlugin; // it's already loaded
  }

  HINSTANCE plugin = LoadLibrary(file);
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

  pluginList.SetAt(FileNoPath(file), kPlugin);

  return kPlugin;
}

int CPlugins::FindAndLoad(char *pattern = "*.dll"){
  CString filepath;
  CFileFind finder;
  BOOL bWorking = finder.FindFile(pattern);
  int i = 0;
  while (bWorking)
  {
    bWorking = finder.FindNextFile();

    filepath = finder.GetFilePath();
    if ( Load(filepath) ){
      i++;
    }
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
  currentID = START_ID;
}

void CPlugins::DoRebars(HWND rebarWnd){
  POSITION pos = pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  while (pos){
    pluginList.GetNextAssoc( pos, s, kPlugin);
    if (kPlugin && kPlugin->pf->DoRebar){
      kPlugin->pf->DoRebar(rebarWnd);
    }
  }
}
