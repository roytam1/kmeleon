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
// ns_bookmarks.cpp : Plugin that supports netscape-style boomarks
//

#include "stdafx.h"
#include "resource.h"

#include "commctrl.h"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define MAX_BOOKMARKS 255

#define _T(blah) blah

/*
// MFC handles this for us (how nice)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  switch (ul_reason_for_call)
  {
		case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}
*/

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

pluginFunctions pFuncs = {
  Init,
  Create,
  Config,
  Quit,
  DoMenu,
  DoRebar
};

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "Netscape Bookmark Plugin",
  &pFuncs
};

HMENU m_menuBookmarks;

UINT nConfigCommand;
UINT nAddCommand;
UINT nEditCommand;
UINT nFirstBookmarkCommand;

TCHAR szPath[MAX_PATH];

int Init(){
  nConfigCommand = kPlugin.kf->GetCommandIDs(1);
  nAddCommand = kPlugin.kf->GetCommandIDs(1);
  nEditCommand = kPlugin.kf->GetCommandIDs(1);

  nFirstBookmarkCommand = kPlugin.kf->GetCommandIDs(MAX_BOOKMARKS);

  kPlugin.kf->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), szPath, "");
  strcat(szPath, "bookmarks.html");

  return true;
}

typedef std::map<HWND, void *> WndProcMap;
WndProcMap KMeleonWndProcs;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
	KMeleonWndProcs[parent] = (void *) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter N", "Netscape Bookmark plugin", 0);
}

void Quit(){
//  should be destroyed by kmeleon...
//  DestroyMenu(m_menuBookmarks);
}

std::vector<std::string> urlVector;
void ParseBookmarks(char *bmFileBuffer, HMENU menu){
  char *p;
  char *t;
  while ((p = strtok(NULL, "\n")) != NULL){
    if ((t = strstr(p, "<DT><H3 ")) != NULL){
      t+=8;
      char *start = strchr(t, '>') + 1;
      char *end = strrchr(t, '<');
      *end = 0;
      HMENU subMenu = CreatePopupMenu();
      ParseBookmarks(end, subMenu);
      AppendMenu(menu, MF_STRING | MF_POPUP, (UINT)subMenu, start);
    }else if ((t = strstr(p, "<DT><A HREF=\"")) != NULL){
      t+=13; // t now points to the url
      char *q = strchr(t, '\"');
      if (q) *q = 0;
      urlVector.push_back((std::string)t);
      int position = urlVector.size() - 1;
      t = strchr(q+1, '>') + 1;
      q = strchr(t, '<');
      *q = 0;
      AppendMenu(menu, MF_STRING, nFirstBookmarkCommand+position, t);
    }else if ((t = strstr(p, "</DL>")) != NULL){
      return;
    }else if ((t = strstr(p, "<hr>")) != NULL){
      AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    }
  }
  return;
}

void DoMenu(HMENU menu, char *param){
  if (stricmp(param, _T("Config")) == 0){
    AppendMenu(menu, MF_STRING, nConfigCommand, "&Config");
    return;
  }
  if (stricmp(param, _T("Add")) == 0){
    AppendMenu(menu, MF_STRING, nAddCommand, "&Add Bookmark");
    return;
  }
  if (stricmp(param, _T("Edit")) == 0){
    AppendMenu(menu, MF_STRING, nEditCommand, "&Edit Bookmarks");
    return;
  }
  if (*param == 0){
    urlVector.reserve(MAX_BOOKMARKS);

    FILE *bmFile = fopen(szPath, "r");
    fseek(bmFile, 0, SEEK_END);

    long bmFileSize = ftell(bmFile);
    fseek(bmFile, 0, SEEK_SET);

    char *bmFileBuffer = new char[bmFileSize];
    fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);

    strtok(bmFileBuffer, "\n");
    ParseBookmarks(bmFileBuffer, menu);
    m_menuBookmarks = menu;

    delete [] bmFileBuffer;
    fclose(bmFile);
  }
}

void DoRebar(HWND rebarWnd){
/*
  TODO: Add a Bookmarks ReBar Band
*/
}

CALLBACK EditProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
  WndProcMap::iterator WndProcIterator;
  if (message == WM_COMMAND){
    WORD command = LOWORD(wParam);
    if (command == nConfigCommand){
      Config(NULL);
      return true;
    }
    if (command == nAddCommand){
      kmeleonDocInfo *dInfo = kPlugin.kf->GetDocInfo(hWnd);
      urlVector.push_back((std::string)dInfo->url);
      AppendMenu(m_menuBookmarks, MF_STRING, nFirstBookmarkCommand+urlVector.size()-1, dInfo->title);
      DrawMenuBar(hWnd);
      return true;
    }
    if (command == nEditCommand){
      DialogBoxParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), hWnd, EditProc, 0);
      return true;
    }
    if (command >= nFirstBookmarkCommand && command < (nFirstBookmarkCommand + MAX_BOOKMARKS)){
      kPlugin.kf->NavigateTo((char *)urlVector[command-nFirstBookmarkCommand].c_str(), false);
      return true;
    }
  }else if (message == WM_NCDESTROY){
    WndProcIterator = KMeleonWndProcs.find(hWnd); 

    if (WndProcIterator != KMeleonWndProcs.end()){
      SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WndProcIterator->second);

      LRESULT result = CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);

      KMeleonWndProcs.erase(WndProcIterator);

      return result;
    }
  }
  WndProcIterator = KMeleonWndProcs.find(hWnd);

  if (WndProcIterator != KMeleonWndProcs.end()){
    return CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);
  }
  return 0;
}

void FillTree(HWND hTree){
  TVINSERTSTRUCT tvis;
  tvis.hParent = NULL;
  tvis.hInsertAfter = NULL;
  tvis.itemex.mask = TVIF_TEXT;
  tvis.itemex.pszText = "Blah!";
  TreeView_InsertItem(hTree, &tvis);
}

CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
  switch (uMsg){
  case WM_INITDIALOG:
    {
      HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
      FillTree(hTree);
    }
    return false;
  case WM_COMMAND:
    {
      WORD id = LOWORD(wParam);

      switch(id){
      case IDOK:
      case IDCANCEL:
        EndDialog(hDlg, 1);
        break;
      }
    }
  }
  return false;
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}