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
#include <afxres.h>

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define _T(x) x

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

void SetOwnerDrawn(HMENU menu, HINSTANCE plugin);
void UnSetOwnerDrawn(HMENU menu);

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
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

HIMAGELIST hImageList;

typedef std::map<short, int> BmpMapT;
BmpMapT bmpMap;
// this maps command ids to the bitmap/index

typedef std::map<std::string, int> DefineMapT;

void ParseConfig(char *buffer){
  hImageList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR8, 32, 256);

	DefineMapT defineMap;
	#define DEFINEMAP_ADD(entry) defineMap[std::string(#entry)] = entry;
	#include "../definemap.cpp"
	DefineMapT::iterator defineMapIt;
  
  BOOL currentBitmap = false;
	int index = 0;

	char *p;
	while ((p = strtok(NULL, "\n")) != NULL){
		if (*p == '#'){
			continue;
		}
		else if (!currentBitmap){
			char *b = strchr(p, '{');
			if (b) {
				*b = 0;
				TrimWhiteSpace(p);
				p = SkipWhiteSpace(p);

        HBITMAP bitmap;
				if (strchr(p, ':') || *p == '/' || *p == '\\') {
					bitmap = (HBITMAP)LoadImage(NULL, p, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
				}
				else {
					char bmpPath[MAX_PATH];
					strcpy(bmpPath, szPath);
					strcat(bmpPath, p);
					bitmap = (HBITMAP)LoadImage(NULL, bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
				}
        ImageList_AddMasked(hImageList, bitmap, RGB(192, 192, 192));

        DeleteObject(bitmap);

        currentBitmap = true;
			}
		}
		else {
			if ( strchr( p, '}' )) {
        currentBitmap = false;
        index = ImageList_GetImageCount(hImageList);
				continue;
			}

			TrimWhiteSpace(p);
			p = SkipWhiteSpace(p);

			int id;

			defineMapIt = defineMap.find(std::string(p));
			if ( defineMapIt != defineMap.end() ) {
				id = defineMapIt->second;
			} 
			else {
				id = 0;
			}
			bmpMap[id] = index;
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

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
	KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

  refCount++;
}

void Config(HWND parent){
  char cfgPath[MAX_PATH];
  strcpy(cfgPath, szPath);
  strcat(cfgPath, "menuicons.cfg");

  ShellExecute(parent, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);

  strcpy(cfgPath, szPath);
  strcat(cfgPath, "menus.cfg");
  ShellExecute(parent, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);
}

void Quit(){
  ImageList_Destroy(hImageList);
}

void DoMenu(HMENU menu, char *param){
  // only do this the first time
  if (refCount == 0){
    menus.push_back(menu);

    HINSTANCE plugin = LoadLibrary(param);
    SetOwnerDrawn(menu, plugin);
    if (plugin) FreeLibrary(plugin);
  }
}

void DoRebar(HWND rebarWnd){
}

#define BMP_MENU_VERSION 3333

typedef int (*DRAWBITMAPPROC)(DRAWITEMSTRUCT *dis);

typedef struct {
  long version; // this is just to differenciate between our ownerdraw menus and someone else's menus
  void *data;
  DRAWBITMAPPROC DrawBitmap;
} MenuDataT;

#define LEFT_SPACE 18

int DrawBitmap(DRAWITEMSTRUCT *dis) {
	BmpMapT::iterator bmpMapIt;
	bmpMapIt = bmpMap.find(dis->itemID);

	// Load the corresponding bitmap
	if (bmpMapIt != bmpMap.end()){
    int top = (dis->rcItem.bottom - dis->rcItem.top - 16) / 2;
    top += dis->rcItem.top;
    if (dis->itemState & ODS_GRAYED){
      ImageList_DrawEx(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+1, top, 0, 0, CLR_NONE, GetSysColor(COLOR_3DFACE), ILD_BLEND  | ILD_TRANSPARENT);
    }
    else if (dis->itemState & ODS_SELECTED){
      ImageList_Draw(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+1, top, ILD_TRANSPARENT);
    }
    else{
      ImageList_Draw(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+1, top, ILD_TRANSPARENT);
    }
		return LEFT_SPACE;
	}
  return 0;
}

int GetTabWidth(HMENU menu){
	MENUITEMINFO mmi;
	mmi.cbSize = sizeof(mmi);

	int maxChars = 0;
	int state;
	char *string;
	char *tab;

	int count = ::GetMenuItemCount(menu);
	int i;
	for (i=0; i<count; i++) {

		state = ::GetMenuState(menu, i, MF_BYPOSITION);

		if (state & MF_OWNERDRAW){
			mmi.fMask = MIIM_DATA;
			::GetMenuItemInfo(menu, i, true, &mmi);

			MenuDataT *mdt = (MenuDataT *)mmi.dwItemData;
			if (mdt){
				string = (char *)mdt->data;

				tab = strrchr(string, '\t');

				if (tab) {
					if ((tab - string) > maxChars) {
						maxChars = tab - string;
					}
				}
			}
		}
	}
	return maxChars+5;
}

void DrawMenuItem(DRAWITEMSTRUCT *dis) {
	HMENU menu = (HMENU)dis->hwndItem;

	// make sure itemID is a valid int
	//dis->itemID = LOWORD(dis->itemID);
	MenuDataT *mdt = (MenuDataT *)dis->itemData;
  char *string = (char *)mdt->data;

	BOOL hasBitmap = false;

	// Draw the highlight rectangle
	SetBkMode(dis->hDC, TRANSPARENT);
	if (dis->itemState & ODS_SELECTED) {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
	}

  if (mdt->DrawBitmap){
    int space = mdt->DrawBitmap(dis);
    if (space > 0){
      dis->rcItem.left += space;
      hasBitmap = true;
    }
  }

	if (!hasBitmap){
		dis->rcItem.left += LEFT_SPACE;
	}

  dis->rcItem.left += 2;

	char *tab = strrchr(string, '\t');
	int leftLen, rightLen;
	if (tab) {
		leftLen = tab - string;
		rightLen = strlen(tab);
	}
	else {
		leftLen = strlen(string);
		rightLen = 0;
	}
  short tabWidth = GetTabWidth(menu);

	if (dis->itemState & ODS_GRAYED) {

		// setup pen to draw selected, grayed text
		if (dis->itemState & ODS_SELECTED) {
			SetTextColor(dis->hDC, GetSysColor(COLOR_3DFACE));
		}

		// Draw shadow for unselected grayed items
		else {
			dis->rcItem.left += 1;
			dis->rcItem.top += 1;

			SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));

			DrawText(dis->hDC, string, leftLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (0<<8) /* 0 spaces/tab */ );
			if (tab) {
				//dis->rcItem.right -= 15;  //  16 - 1 for shadow
				DrawText(dis->hDC, tab, rightLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (tabWidth<<8));
				//dis->rcItem.right += 15;
			}

			dis->rcItem.left -= 1;
			dis->rcItem.top -= 1;
      
			// set pen to draw normal text colour
			SetTextColor(dis->hDC, GetSysColor(COLOR_GRAYTEXT));
		}
	}

	DrawText(dis->hDC, string, leftLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (0<<8) /* 0 spaces/tab */ );
	if (tab){
		//dis->rcItem.right -= 16;
		DrawText(dis->hDC, tab, rightLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS | DT_TABSTOP | (tabWidth<<8));
		//dis->rcItem.right += 16;
	}
}

void UnSetOwnerDrawn(HMENU menu){
  MENUITEMINFO mmi;
  mmi.cbSize = sizeof(mmi);
  int count = ::GetMenuItemCount(menu);
  int i;
  int state;
  MenuDataT *mdt;
  for (i=0; i<count; i++){
    state = ::GetMenuState(menu, i, MF_BYPOSITION);
    if (state & MF_POPUP){
      UnSetOwnerDrawn(GetSubMenu(menu, i));
    }
    else if (state & MF_OWNERDRAW){
      mmi.fMask = MIIM_DATA;
      ::GetMenuItemInfo(menu, i, true, &mmi);

      mdt = (MenuDataT *)mmi.dwItemData;
      if (mdt->version == BMP_MENU_VERSION){
        delete [] (char *)mdt->data;
        delete mdt;
      }
    }
  }
}

void SetOwnerDrawn(HMENU menu, HINSTANCE plugin){
   MENUITEMINFO mmi;
   mmi.cbSize = sizeof(mmi);
   int count = ::GetMenuItemCount(menu);
   int i;
   int state;

   DRAWBITMAPPROC DrawProc;
   if (plugin) {
      DrawProc = (DRAWBITMAPPROC)GetProcAddress(plugin, "DrawBitmap");
      if (!DrawProc){
         DrawProc = DrawBitmap;
      }
   }
   else
    DrawProc = DrawBitmap;

   for (i=0; i<count; i++){
      state = ::GetMenuState(menu, i, MF_BYPOSITION);
      if (state & MF_POPUP){
         SetOwnerDrawn(GetSubMenu(menu, i), plugin);
         if (plugin){
            mmi.fMask = MIIM_TYPE;
            mmi.cch = 0;
            mmi.dwTypeData = NULL;
            GetMenuItemInfo(menu, i, true, &mmi);
            mmi.cch ++;
            mmi.dwTypeData = new char[mmi.cch];
            GetMenuItemInfo(menu, i, true, &mmi);

            MenuDataT *mData = new MenuDataT;
            mData->version = BMP_MENU_VERSION;
            mData->data = mmi.dwTypeData;
            mData->DrawBitmap = DrawProc;
        
            ModifyMenu(menu, i, MF_BYPOSITION | MF_OWNERDRAW | MF_POPUP, (UINT)GetSubMenu(menu, i), (LPCTSTR)(void *)mData);
         }
      }
      else if (state == 0) {
         mmi.fMask = MIIM_TYPE;
         mmi.cch = 0;
         mmi.dwTypeData = NULL;
         GetMenuItemInfo(menu, i, true, &mmi);
         mmi.cch ++;
         mmi.dwTypeData = new char[mmi.cch];
         GetMenuItemInfo(menu, i, true, &mmi);

         MenuDataT *mData = new MenuDataT;
         mData->version = BMP_MENU_VERSION;
         mData->data = mmi.dwTypeData;
         mData->DrawBitmap = DrawProc;

         ModifyMenu(menu, i, MF_BYPOSITION | MF_OWNERDRAW, GetMenuItemID(menu, i), (LPCTSTR)(void *)mData);
      }
   }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   if (message == WM_MEASUREITEM) {

      MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
      MenuDataT * mdt = (MenuDataT *)mis->itemData;

      if (mis->CtlType == ODT_MENU && mdt->version == BMP_MENU_VERSION) {
         RECT rc = {0};
         HDC hDC = GetDC(hWnd);

         NONCLIENTMETRICS ncm = {0};
		   ncm.cbSize = sizeof(ncm);
		   SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,(PVOID)&ncm,FALSE);
		   HFONT font;
		   font = CreateFontIndirect(&ncm.lfMenuFont);
		   HFONT oldFont = (HFONT)SelectObject(hDC, font);
      
         char *string = (char *)((MenuDataT *)mis->itemData)->data;

         char *tab = strrchr(string, '\t');
         int tabWidth;
         if (tab) tabWidth = tab - string +8;      // +8 gives us a little extra edge on the right after the accelerator
         else tabWidth = 0;

         DWORD size = GetTabbedTextExtent(hDC, string, strlen(string), 1, &tabWidth);
         rc.top    = 0;
         rc.left   = 0;
         rc.bottom = HIWORD(size);
         rc.right  = LOWORD(size);

         SelectObject(hDC, oldFont);
         DeleteObject(font);
         ReleaseDC(hWnd, hDC);

         mis->itemWidth = rc.right + LEFT_SPACE;
         mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE);
         return true;
      }
   }
   else if (message == WM_DRAWITEM){
       DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
      MenuDataT * mdt = (MenuDataT *)dis->itemData;
      if (dis->CtlType == ODT_MENU && mdt->version == BMP_MENU_VERSION){
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
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
