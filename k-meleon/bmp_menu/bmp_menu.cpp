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
// adds icons to the menus
//

#include "stdafx.h"
#include "../resource.h"
#include "../Utils.h"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

std::vector<HMENU> menus;
int refCount;

char szPath[MAX_PATH];

/*
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

void SetOwnerDrawn(HMENU menu);
void UnSetOwnerDrawn(HMENU menu);

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

pluginFunctions pFunc = {
  Init,
  Create,
  Config,
  Quit,
  DoMenu,
  DoRebar
};

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "Bitmapped Menus",
  &pFunc
};

/*
# sample config

filename1.bmp {
ID_BLAH1
ID_BLAH2
ID_BLAH3
}
filename2.bmp {
ID_BLARG1
ID_BLARG2
}
*/

class CBmpEntry{
public:
  HBITMAP m_bitmap;
  int m_index;
  int canDelete;

public:
  CBmpEntry(){
    m_bitmap = NULL;
    m_index = 0;
    canDelete = true;
  }
  CBmpEntry(HBITMAP bitmap, int index){
    m_bitmap = bitmap;
    m_index = index;
    canDelete = true;
  }
  CBmpEntry(const CBmpEntry &other){
    m_bitmap = other.m_bitmap;
    m_index = other.m_index;
    canDelete = true;
  }
  CBmpEntry &operator= (CBmpEntry &other){
    m_bitmap = other.m_bitmap;
    m_index = other.m_index;
    other.canDelete = false;
    // the copier is responsible for deleting the bitmap now

    return *this;
  }
  ~CBmpEntry(){
    if (m_index == 0 && canDelete && m_bitmap){
      // only delete the object once
      DeleteObject(m_bitmap);
    }
  }
};

typedef std::map<int, CBmpEntry> BmpMapT;
BmpMapT bmpMap;
// this maps command ids to the bitmap/index

typedef std::map<std::string, int> DefineMapT;

void ParseConfig(char *buffer){
  DefineMapT defineMap;
#define DEFINEMAP_ADD(entry) defineMap[std::string(#entry)] = entry;
#include "../definemap.cpp"

  DefineMapT::iterator defineMapIt;

  HBITMAP currentBitmap = NULL;
  int index = 0;

  char *p;
  while ((p = strtok(NULL, "\n")) != NULL){
    if (*p == '#'){
      continue;
    }
    else if (!currentBitmap){
      char *b = strchr(p, '{');
      if (b){
        *b = 0;
        TrimWhiteSpace(p);
        p = SkipWhiteSpace(p);

        if (strchr(p, ':') || *p == '/' || *p == '\\'){
          currentBitmap = (HBITMAP)LoadImage(NULL, p, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
        }else{
          char bmpPath[MAX_PATH];
          strcpy(bmpPath, szPath);
          strcat(bmpPath, p);
          currentBitmap = (HBITMAP)LoadImage(NULL, bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
        }
      }
    }else{
      if (strchr(p, '}')){
        currentBitmap = NULL;
        index = 0;
        continue;
      }
      TrimWhiteSpace(p);
      p = SkipWhiteSpace(p);

      int id;

      defineMapIt = defineMap.find(std::string(p));
      if (defineMapIt != defineMap.end()){
        id = defineMapIt->second;
      }else{
        id = 0;
      }
      bmpMap[id] = CBmpEntry(currentBitmap, index);
      index++;
    }
  }
}

int Init(){
  refCount = 0;

  kPlugin.kf->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), szPath, "");

  char cfgPath[MAX_PATH];
  strcpy(cfgPath, szPath);
  strcat(cfgPath, "menuicons.cfg");

  FILE *cfgFile = fopen(cfgPath, "r");
  if (cfgFile){
    fseek(cfgFile, 0, SEEK_END);
    long cfgFileSize = ftell(cfgFile);
    fseek(cfgFile, 0, SEEK_SET);

    char *cfgFileBuffer = new char[cfgFileSize];
    if (cfgFileBuffer){
      fread(cfgFileBuffer, sizeof(char), cfgFileSize, cfgFile);

      strtok(cfgFileBuffer, "\n");
      ParseConfig(cfgFileBuffer);
      
      delete [] cfgFileBuffer;
    }
    fclose(cfgFile);
  }

  return true;
}

typedef std::map<HWND, void *> WndProcMap;
WndProcMap KMeleonWndProcs;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
	KMeleonWndProcs[parent] = (void *) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

  refCount++;
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter U", "Bitmapped Menu plugin", 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){
  // only do this the first time
  if (refCount == 0){
    menus.push_back(menu);
    SetOwnerDrawn(menu);
  }
}

void DoRebar(HWND rebarWnd){
}

int GetTabWidth(HMENU menu){
  MENUITEMINFO mmi;
  mmi.cbSize = sizeof(mmi);

  int maxChars = 0;
  int state;
  char *tab;

  int count = ::GetMenuItemCount(menu);
  int i;
  for (i=0; i<count; i++){
    state = ::GetMenuState(menu, i, MF_BYPOSITION);
    if (state & MF_OWNERDRAW){
      mmi.fMask = MIIM_DATA;
      ::GetMenuItemInfo(menu, i, true, &mmi);
      tab = strrchr((char *)mmi.dwItemData, '\t');
      if (tab){
        if ((tab - (char *)mmi.dwItemData) > maxChars){
          maxChars = tab - (char *)mmi.dwItemData;
        }
      }
    }
  }
  return maxChars*2;
}

void DrawMenuItem(DRAWITEMSTRUCT *dis){
  HMENU menu = (HMENU)dis->hwndItem;

  MENUITEMINFO mmi;
  mmi.cbSize = sizeof(mmi);
  mmi.fMask = MIIM_DATA;

  ::GetMenuItemInfo(menu, dis->itemID, false, &mmi);

  BOOL hasBitmap = false;

  BmpMapT::iterator bmpMapIt;
  bmpMapIt = bmpMap.find(dis->itemID);
  if (bmpMapIt != bmpMap.end()){
    HBITMAP hBmp = bmpMapIt->second.m_bitmap;

    HDC bmpDC = CreateCompatibleDC(dis->hDC);
    SelectObject(bmpDC, hBmp);
    int width = 16;
    int height = GetSystemMetrics(SM_CYMENUSIZE);
    BitBlt(dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height, bmpDC, bmpMapIt->second.m_index * 16, 0, SRCCOPY);
    //TransparentBlt(dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height, bmpDC, 0, 0, width, height, GetPixel(bmpDC, 0,0));

    DeleteDC(bmpDC);

    hasBitmap = true;

    dis->rcItem.left += 18;
  }

  SetBkMode(dis->hDC, TRANSPARENT);
  if (dis->itemState & ODS_SELECTED){
    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
  }else{
    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
  }
  if (!hasBitmap){
    dis->rcItem.left += 10;
  }
  char *tab = strrchr((char *)mmi.dwItemData, '\t');
  int leftLen, rightLen;
  if (tab){
    leftLen = tab - ((char *)mmi.dwItemData);
    rightLen = strlen(tab);
  }else{
     leftLen = strlen((char *)mmi.dwItemData);
     rightLen = 0;
  }
  if (dis->itemState & ODS_DISABLED){
    if (dis->itemState & ODS_GRAYED){
      SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));

      dis->rcItem.left += 3;
      dis->rcItem.top += 1;
      DrawText(dis->hDC, (char *)mmi.dwItemData, leftLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (0<<8) /* 0 spaces/tab */ );
      if (tab){
        if (hasBitmap){
          dis->rcItem.left -= 8;
        }
        int tabWidth;
        tabWidth = GetTabWidth(menu);
        DrawText(dis->hDC, tab, rightLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (tabWidth<<8) );
        if (hasBitmap){
          dis->rcItem.left += 8;
        }
      }
      dis->rcItem.left -= 3;
      dis->rcItem.top -= 1;
      
      SetTextColor(dis->hDC, GetSysColor(COLOR_GRAYTEXT));
    }
  }
  dis->rcItem.left += 2;
  DrawText(dis->hDC, (char *)mmi.dwItemData, leftLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (0<<8) /* 0 spaces/tab */ );
  if (tab){
    if (hasBitmap){
      dis->rcItem.left -= 8;
    }
    int tabWidth;
    tabWidth = GetTabWidth(menu);
    DrawText(dis->hDC, tab, rightLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (tabWidth<<8) );
    if (hasBitmap){
      dis->rcItem.left += 8;
    }
  }
}

void UnSetOwnerDrawn(HMENU menu){
  MENUITEMINFO mmi;
  mmi.cbSize = sizeof(mmi);
  int count = ::GetMenuItemCount(menu);
  int i;
  int state;
  for (i=0; i<count; i++){
    state = ::GetMenuState(menu, i, MF_BYPOSITION);
    if (state & MF_POPUP){
      UnSetOwnerDrawn(GetSubMenu(menu, i));
    }
    else if (state & MF_OWNERDRAW){
      mmi.fMask = MIIM_DATA;
      ::GetMenuItemInfo(menu, i, true, &mmi);

      delete [] (char *)mmi.dwItemData;
    }
  }
}

void SetOwnerDrawn(HMENU menu){
  MENUITEMINFO mmi;
  mmi.cbSize = sizeof(mmi);
  int count = ::GetMenuItemCount(menu);
  int i;
  int state;
  for (i=0; i<count; i++){
    state = ::GetMenuState(menu, i, MF_BYPOSITION);
    if (state & MF_POPUP){
      SetOwnerDrawn(GetSubMenu(menu, i));
    }
    else if (state == 0){
      mmi.fMask = MIIM_TYPE;
      mmi.cch = 0;
      mmi.dwTypeData = NULL;
      ::GetMenuItemInfo(menu, i, true, &mmi);
      mmi.cch ++;
      mmi.dwTypeData = new char[mmi.cch];
      ::GetMenuItemInfo(menu, i, true, &mmi);

      ModifyMenu(menu, i, MF_BYPOSITION | MF_OWNERDRAW, ::GetMenuItemID(menu, i), mmi.dwTypeData);
    }
  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
  if (message == WM_MEASUREITEM){
    MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
    if (mis->CtlType == ODT_MENU){
      RECT rc = {0};
      HDC hDC = GetDC(hWnd);
      DrawText(hDC, (char *)mis->itemData, strlen((char *)mis->itemData), &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER);
      ReleaseDC(hWnd, hDC);
      mis->itemWidth = rc.right;
      mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE);
      return true;
    }
  }
  else if (message == WM_DRAWITEM){
    DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
    if (dis->CtlType == ODT_MENU){
      DrawMenuItem(dis);
      return true;
    }
  }
  else if (message == WM_DESTROY){
    refCount--;
    if (refCount == 0){
      while(menus.size()){
        UnSetOwnerDrawn(menus.front());
        menus.erase(menus.begin());
      }
    }
  }
  WndProcMap::iterator WndProcIterator;
  WndProcIterator = KMeleonWndProcs.find(hWnd);

  return CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
