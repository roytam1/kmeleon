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

#define PLUGIN_NAME "Bitmapped Menus"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define BMP_PADDING_LEFT 3
#define BMP_PADDING_RIGHT 4
#define BMP_HEIGHT 16
#define BMP_WIDTH  16
int     SPACE_BETWEEN = 0; // the space between the text and the accelerator, set to the width of 'X'
#define MARGIN_LEFT BMP_WIDTH + BMP_PADDING_LEFT + BMP_PADDING_RIGHT
#define MARGIN_RIGHT 16

typedef int (*DRAWBITMAPPROC)(DRAWITEMSTRUCT *dis);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WNDPROC KMeleonWndProc;

int DrawBitmap(DRAWITEMSTRUCT *dis);

void DrawCheckMark(HDC pDC,int x,int y,COLORREF color);
void SetOwnerDrawn(HMENU menu, DRAWBITMAPPROC DrawProc, BOOL topLevel);
void UnSetOwnerDrawn(HMENU menu);

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
int GetConfigFiles(configFileType **configFiles);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
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

char settingsPath[MAX_PATH];

// this maps HMENU's to DrawProcs
typedef std::map<HMENU, DRAWBITMAPPROC> menuMap;
menuMap menus;

BOOL bFirstRun;

HIMAGELIST hImageList;

// this maps command ids to the bitmap/index
typedef std::map<short, int> BmpMapT;
BmpMapT bmpMap;

// maps "ID_BLAH" to ID_BLAH
typedef std::map<std::string, int> DefineMapT;




HMENU ghMenuLast;          // the last menu that we've drawn
int   giMaxAccelWidth;     // the longest accelerator (used by DrawMenuItem)
int   giMaxTextMeasured;    // the longest text (used by MeasureMenuItem)
int   giMaxAccelMeasured;   // the longest accelerator (used by MeasureMenuItem)
BOOL  gbDrawn = TRUE;      // after the menu is drawn, we should reset iMaxText and iMaxAccel
BOOL  gbMeasureAccel = TRUE; // win98 workaround - win98 automatically adjusts measuremenuitem for the width of the accel






long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Init") == 0) {
         Init();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "UnSetOwnerDrawn") == 0) {
         UnSetOwnerDrawn((HMENU)data1);
      }
      else if (stricmp(subject, "SetOwnerDrawn") == 0) {

         // if this menu has already been "ownerdrawn", then it's
         // probably being called again as a result of an update,
         // so we remove it from the mapping because the SetOwnerDrawn
         // function exits if it's listed in the mapping
         menuMap::iterator menuIter = menus.find((HMENU)data1);
         if (menuIter != menus.end())
            menus.erase(menuIter);

         SetOwnerDrawn((HMENU)data1, (DRAWBITMAPPROC)data2, FALSE);
      }
      else return 0;

      return 1;
   }
   return 0;
}


void ParseConfig(char *buffer) {
   hImageList = ImageList_Create(BMP_WIDTH, BMP_HEIGHT, ILC_MASK | ILC_COLOR8, 32, 256);

	DefineMapT defineMap;
   DefineMapT::iterator defineMapIt;
#define DEFINEMAP_ADD(entry) defineMap[std::string(#entry)] = entry;
#include "../definemap.cpp"

   BOOL currentBitmap = false;
	int index = 0;

	char *p;
	while ((p = strtok(NULL, "\n")) != NULL) {

      // ignore the comments
		if (*p == '#') {
			continue;
		}
		else if (!currentBitmap) {
			char *b = strchr(p, '{');
			if (b) {
				*b = 0;
				TrimWhiteSpace(p);
				p = SkipWhiteSpace(p);

            HBITMAP bitmap;
            // if it's relative to the root (ie: c:\blah or \blah or /blah
				if (*p == '/' || *p == '\\' || *(p+1) == ':') {
					bitmap = (HBITMAP)LoadImage(NULL, p, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
				}
            // else it's relative to the settings directory (just plain blah)
				else {
					char bmpPath[MAX_PATH];
					strcpy(bmpPath, settingsPath);
					strcat(bmpPath, p);
					bitmap = (HBITMAP)LoadImage(NULL, bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
				}
            ImageList_AddMasked(hImageList, bitmap, RGB(255, 0, 255));

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
			if ( defineMapIt != defineMap.end() )
				id = defineMapIt->second;
			else
				id = 0;
			bmpMap[id] = index;
			index++;
		}
	}
}

int Init() {
   bFirstRun = TRUE;

   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", settingsPath, "");

   // Check for Win98
   OSVERSIONINFO     osVersion;
   osVersion.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   GetVersionEx( &osVersion );
   if ( (osVersion.dwMajorVersion ==4) && (osVersion.dwMinorVersion > 0) && (osVersion.dwMinorVersion < 90) )
      gbMeasureAccel = FALSE;


   // calculate the width of the character "X"
   // this will be used as a measurement for space between the
   // text and the accelerator
   NONCLIENTMETRICS ncm = {0};
   ncm.cbSize = sizeof(ncm);
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,(PVOID)&ncm,FALSE);

   HDC hDC = CreateCompatibleDC(NULL);
   HFONT font = CreateFontIndirect(&ncm.lfMenuFont);
   SelectObject(hDC, font); 
   
   SIZE size;
   GetTextExtentPoint32(hDC, "X", 1, &size);

   SPACE_BETWEEN = size.cx;

   DeleteObject(font);
   DeleteDC(hDC);


   char cfgPath[MAX_PATH];
   strcpy(cfgPath, settingsPath);
   strcat(cfgPath, "menuicons.cfg");

   FILE *cfgFile = fopen(cfgPath, "r");
   if (cfgFile){
      long cfgFileSize = FileSize(cfgFile);

      char *cfgFileBuffer = new char[cfgFileSize];
      if (cfgFileBuffer) {
         fread(cfgFileBuffer, sizeof(char), cfgFileSize, cfgFile);

         strtok(cfgFileBuffer, "\n");
         ParseConfig(cfgFileBuffer);

         delete [] cfgFileBuffer;
      }
      fclose(cfgFile);
   }
   return true;
}

void Create(HWND parent){
	KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   bFirstRun=FALSE;
}

void Config(HWND parent)
{
   char cfgPath[MAX_PATH];
   strcpy(cfgPath, settingsPath);
   strcat(cfgPath, "menuicons.cfg");

   ShellExecute(parent, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];

int GetConfigFiles(configFileType **configFiles)
{
   strcpy(g_configFiles[0].file, settingsPath);
   strcat(g_configFiles[0].file, "menuicons.cfg");

   strcpy(g_configFiles[0].label, "Menu Icons");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}

void Quit(){
   if (hImageList)
      ImageList_Destroy(hImageList);

   while(menus.size()) {
      menus.erase(menus.begin());
   }
}

void DoMenu(HMENU menu, char *param){
   // only do this the first time
   if (bFirstRun) {

      // WinNT4 is unhappy when we subclass the top level menus, it's probably a bug in
      // the way we do it now (since it worked before the recent changes), this is a bad
      // workaround, and needs to be fixed better.
      
      // Don't ownerdraw the top level menu items (File, Edit, View, etc)
      // We can't actually be sure that this is going to be that menu.
      // Perhaps someone who only wants menuicons on the File menu will decide
      // to move the bmpmenu() call from the top level menu into the file menu...
      // In that circumstance, none of the entries under "File" will be ownerdrawn...oops

      BOOL topLevel = TRUE;

      DRAWBITMAPPROC DrawProc = NULL;
      if (*param) {
         HINSTANCE plugin = LoadLibrary(param);
         if (plugin) {
            DrawProc = (DRAWBITMAPPROC)GetProcAddress(plugin, "DrawBitmap");
            topLevel = FALSE;  // plugin menus probably won't be top level
         }
      }
      if (!DrawProc) {
         DrawProc = DrawBitmap;
      }
      SetOwnerDrawn(menu, DrawProc, topLevel);
   }
}

int DrawBitmap(DRAWITEMSTRUCT *dis) {
	BmpMapT::iterator bmpMapIt;
	bmpMapIt = bmpMap.find(dis->itemID);

	// Load the corresponding bitmap
	if (bmpMapIt != bmpMap.end()){
      int top = (dis->rcItem.bottom - dis->rcItem.top - BMP_HEIGHT) / 2;
      top += dis->rcItem.top;

      if (dis->itemState & ODS_GRAYED)
         ImageList_DrawEx(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, 0, 0, CLR_NONE, GetSysColor(COLOR_MENU), ILD_BLEND  | ILD_TRANSPARENT);

      else if (dis->itemState & ODS_SELECTED)
         ImageList_Draw(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, ILD_TRANSPARENT);

      else
         ImageList_Draw(hImageList, bmpMapIt->second, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, ILD_TRANSPARENT);

      return BMP_WIDTH;
	}
   else if (dis->itemState & ODS_CHECKED) {

      // the check mark is only 6x6, so we need to offset its drawing position
      if (dis->itemState & ODS_SELECTED)
         DrawCheckMark(dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT+4, dis->rcItem.top+5, GetSysColor(COLOR_HIGHLIGHTTEXT));
      else
         DrawCheckMark(dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT+4, dis->rcItem.top+5, GetSysColor(COLOR_MENUTEXT));
      return BMP_WIDTH;
   }

   return 0;
}

void DrawCheckMark(HDC pDC,int x,int y,COLORREF color) {
   SetPixel(pDC, x,   y+2, color);
   SetPixel(pDC, x,   y+3, color);
   SetPixel(pDC, x,   y+4, color);

   SetPixel(pDC, x+1, y+3, color);
   SetPixel(pDC, x+1, y+4, color);
   SetPixel(pDC, x+1, y+5, color);

   SetPixel(pDC, x+2, y+4, color);
   SetPixel(pDC, x+2, y+5, color);
   SetPixel(pDC, x+2, y+6, color);

   SetPixel(pDC, x+3, y+3, color);
   SetPixel(pDC, x+3, y+4, color);
   SetPixel(pDC, x+3, y+5, color);

   SetPixel(pDC, x+4, y+2, color);
   SetPixel(pDC, x+4, y+3, color);
   SetPixel(pDC, x+4, y+4, color);

   SetPixel(pDC, x+5, y+1, color);
   SetPixel(pDC, x+5, y+2, color);
   SetPixel(pDC, x+5, y+3, color);

   SetPixel(pDC, x+6, y,   color);
   SetPixel(pDC, x+6, y+1, color);
   SetPixel(pDC, x+6, y+2, color);
}

int GetMaxAccelWidth(HDC hDC, HMENU hMenu){
   MENUITEMINFO mmi;
   mmi.cbSize = sizeof(mmi);
   
   int iMaxAccel = 0;
   char *string;
   char *tab;
   SIZE size;
   
   int count = GetMenuItemCount(hMenu);
   int i;
   for (i=0; i<count; i++) {      
      UINT state = GetMenuState(hMenu, i, MF_BYPOSITION);
      if (state & MF_OWNERDRAW) {
         mmi.fMask = MIIM_DATA;
         GetMenuItemInfo(hMenu, i, true, &mmi);            
         string = (char *)mmi.dwItemData;

         if (string) {
      
            tab = strchr(string, '\t');
      
            if (tab) {
               GetTextExtentPoint32(hDC, tab+1, strlen(tab+1), &size);
               if (size.cx > iMaxAccel) iMaxAccel = size.cx;
            }
         }
      }
   }
   return iMaxAccel;
}

void DrawMenuItem(DRAWITEMSTRUCT *dis) {
   gbDrawn = TRUE;
   HMENU menu = (HMENU)dis->hwndItem;

   if ((HMENU) dis->hwndItem != ghMenuLast &&  GetMenuItemCount((HMENU) dis->hwndItem)) {
      int accelWidth = GetMaxAccelWidth(dis->hDC, (HMENU)dis->hwndItem);
      if (accelWidth) {
         giMaxAccelWidth = accelWidth;
         int width = dis->rcItem.right - dis->rcItem.left;
      }
   }

   char *string = (char *)dis->itemData;

   if (!*string) {
		RECT rc;
      rc.bottom   = dis->rcItem.bottom;
      rc.left     = dis->rcItem.left;
      rc.right    = dis->rcItem.right;
		rc.top      = dis->rcItem.top + ((rc.bottom-dis->rcItem.top)>>1); // vertical center
		DrawEdge(dis->hDC, &rc, EDGE_ETCHED, BF_TOP);   // draw separator line
      return;
   }

	// Draw the highlight rectangle
	SetBkMode(dis->hDC, TRANSPARENT);
	if (dis->itemState & ODS_SELECTED) {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
	}

   DRAWBITMAPPROC DrawProc;
   menuMap::iterator menuIter = menus.find(menu);
   if (menuIter != menus.end()){
      DrawProc = menuIter->second;
      int space = DrawProc(dis);
   }

   dis->rcItem.left = MARGIN_LEFT + 1; // start drawing text at the pixel after our margin

   RECT rcTab;

	char *tab = strrchr(string, '\t');
	int itemLen, tabLen;

   if (tab) {
		itemLen = tab - string;
      tab++;
		tabLen = strlen(tab);

      rcTab.top = dis->rcItem.top;
      rcTab.left = dis->rcItem.right - giMaxAccelWidth - MARGIN_RIGHT;
      rcTab.right = dis->rcItem.right;
      rcTab.bottom = dis->rcItem.bottom;
	}
	else {
		itemLen = strlen(string);
		tabLen = 0;
	}
	if (dis->itemState & ODS_GRAYED) {
		// setup pen to draw selected, grayed text
		if (dis->itemState & ODS_SELECTED) {
			SetTextColor(dis->hDC, GetSysColor(COLOR_MENU));
		}
		// Draw shadow for unselected grayed items
		else {
			dis->rcItem.left += 1;
			dis->rcItem.top += 1;

			SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));

			DrawText(dis->hDC, string, itemLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
			if (tab) {
				DrawText(dis->hDC, tab, tabLen, &rcTab, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
			}

			dis->rcItem.left -= 1;
			dis->rcItem.top -= 1;
      
			// set pen to draw normal text colour
			SetTextColor(dis->hDC, GetSysColor(COLOR_GRAYTEXT));
		}
	}

	DrawText(dis->hDC, string, itemLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
	if (tab) {
		DrawText(dis->hDC, tab, tabLen, &rcTab, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
	}
}

void MeasureMenuItem(MEASUREITEMSTRUCT *mis, HDC hDC) {
   
   NONCLIENTMETRICS ncm = {0};
   ncm.cbSize = sizeof(ncm);
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,(PVOID)&ncm,FALSE);

   HFONT font;
   font = CreateFontIndirect(&ncm.lfMenuFont);
   HFONT oldFont = (HFONT)SelectObject(hDC, font); 

   SIZE size;
   char *string = (char *)mis->itemData;
   if (!*string)  { // it's a separator
      mis->itemWidth = 0;
      mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE) >> 1;
      return;
   }

   if (gbDrawn) {
      giMaxTextMeasured = 0;
      giMaxAccelMeasured = 0;
      gbDrawn = FALSE;
   }

#if 1
   char *tab = strrchr(string, '\t');
   if (tab) {
      int len = tab-string-1;
      while (*(string+len) == ' ')  // trim trailing spaces (to compensate for sloppy menus and TranslateTabs())
         len--;
      GetTextExtentPoint32(hDC, string, len+1, &size);
      if (size.cx > giMaxTextMeasured) giMaxTextMeasured = size.cx;
      
      GetTextExtentPoint32(hDC, tab+1, strlen(tab+1), &size);
      if (size.cx > giMaxAccelMeasured) giMaxAccelMeasured = size.cx;
         
      mis->itemWidth = SPACE_BETWEEN;

   }
   else {
      GetTextExtentPoint32(hDC, string, strlen(string), &size);
      if (size.cx > giMaxTextMeasured) giMaxTextMeasured = size.cx;
      mis->itemWidth = 0;
   }

   mis->itemWidth += giMaxTextMeasured;
   if (gbMeasureAccel)
      mis->itemWidth += giMaxAccelMeasured;

#else
   RECT rcText;
   DrawText(hDC, string, -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_VCENTER|DT_CALCRECT);

   mis->itemWidth = rcText.right - rcText.left;
#endif

   mis->itemWidth += MARGIN_LEFT + MARGIN_RIGHT;
   // compensate for the width of a chekmark (minus one pixel!) that windows so graciously adds for us
   mis->itemWidth -= (GetSystemMetrics(SM_CXMENUCHECK)-1);
   mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE);
   if (mis->itemHeight < 18)
      mis->itemHeight = 18;

   SelectObject(hDC, oldFont);
   DeleteObject(font);
}

void UnSetOwnerDrawn(HMENU menu){
   MENUITEMINFO mmi;
   mmi.cbSize = sizeof(mmi);

   int state;

   int i;
   int count = GetMenuItemCount(menu);
   for (i=0; i<count; i++) {
      state = GetMenuState(menu, i, MF_BYPOSITION);
      if (state & MF_POPUP){
         UnSetOwnerDrawn(GetSubMenu(menu, i));
      }
      if (state & MF_OWNERDRAW) {
         mmi.fMask = MIIM_DATA | MIIM_ID;
         GetMenuItemInfo(menu, i, true, &mmi);

         if (!mmi.dwItemData ||  !*( (char *)mmi.dwItemData) )
            ModifyMenu(menu, i, MF_BYPOSITION | MF_SEPARATOR, mmi.wID, NULL);
         else
            ModifyMenu(menu, i, MF_BYPOSITION | MF_STRING, mmi.wID, mmi.dwTypeData);
      }
   }
}

// this oddly named function converts a menu of type MF_STRING to MF_OWNERDRAW
void StringToOwnerDrawn(HMENU menu, int i, UINT flags, UINT id){
   MENUITEMINFO mmi;
   mmi.cbSize = sizeof(mmi);

   mmi.fMask = MIIM_TYPE;
   mmi.cch = 0;
   mmi.dwTypeData = NULL;
   GetMenuItemInfo(menu, i, true, &mmi);
   mmi.cch ++;
   mmi.dwTypeData = new char[mmi.cch];
   GetMenuItemInfo(menu, i, true, &mmi);

   // at first glance it appears as though dwTypeData is never delete[]d, but that's not so.   in
   // UnSetOwnerDrawn, we change it back to a MF_STRING which causes windows to delete[] it for us

   ModifyMenu(menu, i, MF_BYPOSITION | MF_OWNERDRAW | flags, id, mmi.dwTypeData);
}

void SetOwnerDrawn(HMENU menu, DRAWBITMAPPROC DrawProc, BOOL topLevel){
   menuMap::iterator menuIter = menus.find(menu);
   if (menuIter != menus.end()){
      // if this menu already has a DrawProc, just exit now
      return;
   }
   menus[menu] = DrawProc;

   int state;

   int i;
   int count = GetMenuItemCount(menu);
   for (i=0; i<count; i++){
      state = GetMenuState(menu, i, MF_BYPOSITION);

      // no need to do anything if it's already ownerdrawn
      if (state & MF_OWNERDRAW) {}

      if (state & MF_POPUP) {
         HMENU subMenu = GetSubMenu(menu, i);
         SetOwnerDrawn(subMenu, DrawProc, FALSE);
         if (!topLevel)
            StringToOwnerDrawn(menu, i, MF_POPUP, (UINT)subMenu);
      }
      
      // subclassing the separator is the key, otherwise widows will do all sorts
      // of weird things with the numbers we tell it to use in measuremenuitem
      else if (state & MF_SEPARATOR)
         ModifyMenu(menu, i, MF_BYPOSITION  | MF_OWNERDRAW, i, "");
      else if (state == 0)
         StringToOwnerDrawn(menu, i, 0, GetMenuItemID(menu, i));
   }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   if (message == WM_MEASUREITEM) {
      MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
     if (mis->CtlType == ODT_MENU && mis->itemData) {
         HDC hDC = GetWindowDC(hWnd);
         MeasureMenuItem(mis, hDC);
         ReleaseDC(hWnd, hDC);
         return true;
      }
   }
   else if (message == WM_DRAWITEM){
      DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
      if (dis->CtlType == ODT_MENU && dis->itemData){
         DrawMenuItem(dis);
         return true;
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
