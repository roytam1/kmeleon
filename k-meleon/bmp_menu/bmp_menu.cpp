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
#include <tchar.h>
#include <malloc.h>
#include "../resource.h"
#include "../Utils.h"

#define PLUGIN_NAME "Bitmapped Menus"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define CONFIG_FILE _T("menuicons.cfg")

//#define BMP_PADDING_LEFT 3
//#define BMP_PADDING_RIGHT 4
#define BMP_PADDING_LEFT 2
#define BMP_PADDING_RIGHT 2

int     SPACE_BETWEEN = 0; // the space between the text and the accelerator, set to the width of 'X'
//LONG MARGIN_LEFT = max(GetSystemMetrics(SM_CXMENUCHECK)+ BMP_PADDING_LEFT + BMP_PADDING_RIGHT, BMP_WIDTH + BMP_PADDING_LEFT + BMP_PADDING_RIGHT);
#define MARGIN_RIGHT 16

int gBmpHeight = 16;
int gBmpWidth = 16;
int gMarginLeft = 16 + BMP_PADDING_LEFT + BMP_PADDING_RIGHT;

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
//int GetConfigFiles(configFileType **configFiles);

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
BOOL gbIsComCtl6 = FALSE;



HMENU ghMenuLast;          // the last menu that we've drawn
int   giMaxAccelWidth;     // the longest accelerator (used by DrawMenuItem)
int   giMaxTextMeasured;    // the longest text (used by MeasureMenuItem)
int   giMaxAccelMeasured;   // the longest accelerator (used by MeasureMenuItem)
BOOL  gbDrawn = TRUE;      // after the menu is drawn, we should reset iMaxText and iMaxAccel
BOOL  gbMeasureAccel = TRUE; // win98 workaround - win98 automatically adjusts measuremenuitem for the width of the accel
HFONT gMenuFont = NULL;


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

HBITMAP LoadImage24(const char* sFile, COLORREF* bgColor, int width = -1, int height = -1, int index = -1, int xstart = 0, int ystart = 0) 
{
   if (!sFile || !*sFile)
      return 0;

   if (bgColor) *bgColor =-1;

   HDC hdcButton, hdcBitmap;
   HBITMAP hButton, hBitmap;
   HBRUSH hBrush;
   
#ifdef _UNICODE            
   wchar_t _sFile[MAX_PATH];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sFile, -1, _sFile, MAX_PATH);
#else
   const char* _sFile = sFile;
#endif
   UINT flag = LR_LOADFROMFILE | LR_CREATEDIBSECTION;

   if (strchr(sFile, '\\')) {
      hBitmap = (HBITMAP)LoadImage(NULL, _sFile, IMAGE_BITMAP, 0, 0, flag);
   }
   else {
      wchar_t fullpath[MAX_PATH];
      kPlugin.kFuncs->FindSkinFile(_sFile, fullpath, MAX_PATH);
      hBitmap = (HBITMAP)LoadImage(NULL, fullpath, IMAGE_BITMAP, 0, 0, flag);
   }

   if (!hBitmap) return NULL;
   hdcBitmap = CreateCompatibleDC(NULL);

	struct {
		BITMAPINFOHEADER header;
		COLORREF col[256];
	} bmpi = {0};

	bmpi.header.biSize = sizeof(BITMAPINFOHEADER);

	int nCol = 0;
	int nLine = 0;
	GetDIBits(hdcBitmap, hBitmap, 0, 0, NULL, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS);
	if (width != -1) {
		nCol = ((width*index) % bmpi.header.biWidth) / width;
		nLine = bmpi.header.biHeight / height - (width*index) / bmpi.header.biWidth - 1;
	} else {
		width = bmpi.header.biWidth;
		height = bmpi.header.biHeight;
	}


	if (bmpi.header.biBitCount == 32) {


		int srcWidth = bmpi.header.biWidth * 4;
		int srcHeight = bmpi.header.biHeight;

		int dstWidth = width * 4;
		int offset = nCol * dstWidth + nLine * srcWidth * height;

		if (offset + (height-1) * srcWidth + dstWidth > srcWidth * srcHeight)
			return NULL;

		BYTE* srcBits = new BYTE[srcWidth * srcHeight];
		GetDIBits(hdcBitmap, hBitmap, 0, srcHeight, srcBits, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS);
		
		bmpi.header.biWidth = width;
		bmpi.header.biHeight = height;

		BYTE* dstBits = NULL;
		HBITMAP hBmp = CreateDIBSection(hdcBitmap, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS, (void**)&dstBits, NULL, 0);

		if (!hBmp) {
			DeleteObject(hBitmap);
			DeleteDC(hdcBitmap);
			delete[] srcBits;
			return NULL;
		}

		for (int i = 0; i< height; i++)
			memcpy(&dstBits[dstWidth*i], &srcBits[i * srcWidth + offset], dstWidth);
		
		DeleteObject(hBitmap);

		if (!gbIsComCtl6 && bgColor) {
			// Pseudo background color check
			*bgColor = RGB(255, 0, 255);
			for (int i=0;i<width*height*4;i+=4)
				if (dstBits[i] == 0) {
					*bgColor = RGB(dstBits[i+1], dstBits[i+2], dstBits[i+3]);
					break;
				}
		}

		delete[] srcBits;
		DeleteDC(hdcBitmap);
		return hBmp;
	}
   
   HGDIOBJ oldBmp = SelectObject(hdcBitmap, hBitmap);
   
   hdcButton = CreateCompatibleDC(hdcBitmap);
   hButton = CreateCompatibleBitmap(hdcBitmap, width, height);
   HGDIOBJ oldBmp2 = SelectObject(hdcButton, hButton);

   // fill the button with the transparency
   hBrush = CreateSolidBrush(RGB(255,0,255));
   HGDIOBJ oldBrush = SelectObject(hdcButton, hBrush);
   PatBlt(hdcButton, 0, 0, width, height, PATCOPY);
   
   // copy the button from the full bitmap
   BitBlt(hdcButton, xstart, ystart, width, height, hdcBitmap, width*nCol + height*nLine, 0, SRCCOPY);
   SelectObject(hdcButton, oldBrush);
   SelectObject(hdcButton, oldBmp2);
   SelectObject(hdcBitmap, oldBmp);
   DeleteDC(hdcBitmap);
   DeleteDC(hdcButton);
   
   DeleteObject(hBrush);
   DeleteObject(hBitmap);

   if (bgColor) *bgColor = RGB(255,0,255);
   return hButton;
}


void ParseConfig(char *buffer) {
   hImageList = ImageList_Create(gBmpWidth, gBmpHeight, ILC_MASK | (gbIsComCtl6 ? ILC_COLOR32 : ILC_COLORDDB), 32, 256);

	DefineMapT defineMap;
   DefineMapT::iterator defineMapIt;
#define DEFINEMAP_ADD(entry) defineMap[std::string(#entry)] = entry;
#include "../definemap.cpp"

   BOOL currentBitmap = false;
   int index = 0;
   
   char *p = strtok(buffer, "\n");
   while (p != NULL) {
      
      // ignore the comments
      if (*p == '#') {
         p = strtok(NULL, "\n");
         continue;
      }
      else if (!currentBitmap) {
         char *b = strchr(p, '{');
         if (b) {
            *b = 0;
            TrimWhiteSpace(p);
            p = SkipWhiteSpace(p);

			COLORREF bgColor;
			HBITMAP bitmap;
			bitmap = LoadImage24(p, &bgColor);
            if (bgColor != -1)
			  ImageList_AddMasked(hImageList, bitmap, bgColor);
			else
			   ImageList_Add(hImageList, bitmap, 0);
            
            DeleteObject(bitmap);
            
            currentBitmap = true;
         }
      }
      else {
         if ( strchr( p, '}' )) {
            currentBitmap = false;
            index = ImageList_GetImageCount(hImageList);
            p = strtok(NULL, "\n");	     
            continue;
         }
         
         TrimWhiteSpace(p);
         p = SkipWhiteSpace(p);
         
         int id;
         defineMapIt = defineMap.find(std::string(p));
         if ( defineMapIt != defineMap.end() )
            id = defineMapIt->second;
         else {
            // check for call to other plugin
            char *op;
            op = strchr(p, '(');
            if (op) {
               *op = 0;
               char *param = op+1;
               char *plugin = p;
               p = strchr(param, ')');
               if (p) *p =0;
               p = strchr(param, ',');
               if (p) *p =0;
               kPlugin.kFuncs->SendMessage(plugin, PLUGIN_NAME, "DoAccel", (long)param, (long)&id);
            }
            else {
               id = kPlugin.kFuncs->GetID(p);
               if (!id)
                  id = atoi(p);
            }
            if (id < 0)
               id = 0;
         }
         bmpMap[id] = index;
         index++;
      }
	  p = strtok(NULL, "\n");
   }
}

int Init() {
   bFirstRun = TRUE;
   
   HIMAGELIST hList = kPlugin.kFuncs->GetCmdIconList();
   if (hList) {
	   ImageList_GetIconSize(hList, &gBmpWidth, &gBmpHeight);
	   gMarginLeft = gBmpWidth + BMP_PADDING_LEFT + BMP_PADDING_RIGHT;
   }

   gMarginLeft = max(gMarginLeft, GetSystemMetrics(SM_CXMENUCHECK)+ BMP_PADDING_LEFT + BMP_PADDING_RIGHT);

	HMODULE hComCtlDll = LoadLibrary(_T("comctl32.dll"));
	if (hComCtlDll)
	{
		typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO*);

		PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hComCtlDll, "DllGetVersion");

		if (pfnDllGetVersion)
		{
			DLLVERSIONINFO dvi = {0};
			dvi.cbSize = sizeof(dvi);

			HRESULT hRes = (*pfnDllGetVersion)(&dvi);
			if (SUCCEEDED(hRes) && dvi.dwMajorVersion >= 6)
				gbIsComCtl6 = TRUE;
		}

		FreeLibrary(hComCtlDll);
	}

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
   gMenuFont = CreateFontIndirect(&ncm.lfMenuFont);
   HGDIOBJ oldObj = SelectObject(hDC, gMenuFont); 
   
   SIZE size;
   GetTextExtentPoint32(hDC, _T("X"), 1, &size);

   SPACE_BETWEEN = size.cx;

   SelectObject(hDC, oldObj);
   DeleteDC(hDC);


   TCHAR cfgPath[MAX_PATH];
   kPlugin.kFuncs->FindSkinFile(CONFIG_FILE, cfgPath, MAX_PATH);

   FILE *cfgFile = _tfopen(cfgPath, _T("r"));
   if (cfgFile){
      long cfgFileSize = FileSize(cfgFile);

      char *cfgFileBuffer = new char[cfgFileSize+1];
      if (cfgFileBuffer) {
         fread(cfgFileBuffer, sizeof(char), cfgFileSize, cfgFile);
         cfgFileBuffer[cfgFileSize] = 0;
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
   TCHAR cfgPath[MAX_PATH];
   kPlugin.kFuncs->FindSkinFile(CONFIG_FILE, cfgPath, MAX_PATH);

   ShellExecute(parent, NULL, _T("notepad.exe"), cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];

/*int GetConfigFiles(configFileType **configFiles)
{
   FindSkinFile(g_configFiles[0].file, CONFIG_FILE);

   strcpy(g_configFiles[0].label, "Menu Icons");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}*/

void Quit(){
   if (hImageList)
      ImageList_Destroy(hImageList);
   
   DeleteObject(gMenuFont);
   
   while(menus.size()) {
	  UnSetOwnerDrawn(menus.begin()->first);
      //menus.erase(menus.begin());
   }
   bmpMap.clear();
}

void DoMenu(HMENU menu, char *param){
   // only do this the first time
//   if (bFirstRun) {

      // WinNT4 is unhappy when we subclass the top level menus, it's probably a bug in
      // the way we do it now (since it worked before the recent changes), this is a bad
      // workaround, and needs to be fixed better.
      
      // Don't ownerdraw the top level menu items (File, Edit, View, etc)
      // We can't actually be sure that this is going to be that menu.
      // To identify it, the user MUST put bmpmenu(top) for this menu
	  BOOL topLevel;
	  DRAWBITMAPPROC DrawProc = NULL;
	  param = SkipWhiteSpace(param);
	  TrimWhiteSpace(param);
	  if (strcmp(param, "top") == 0)
		topLevel = TRUE;
	  else {
	    topLevel = FALSE;

        if (*param) {
		   // plugin menus probably won't be top level
           HINSTANCE plugin = LoadLibraryA(param);
           if (plugin) {
             DrawProc = (DRAWBITMAPPROC)GetProcAddress(plugin, "DrawBitmap");
           }
		   FreeLibrary(plugin);
        }
	  }
      if (!DrawProc) {
         DrawProc = DrawBitmap;
      }
      SetOwnerDrawn(menu, DrawProc, topLevel);
 //  }
}

int DrawBitmap(DRAWITEMSTRUCT *dis) {
   BmpMapT::iterator bmpMapIt;
   bmpMapIt = bmpMap.find(dis->itemID);
   int bmpIdx = bmpMapIt != bmpMap.end() ? bmpMapIt->second : -1;
   HIMAGELIST hImgList = kPlugin.kFuncs->GetCmdIconList();
   if (!hImgList) 
      hImgList = hImageList;
   else 
      bmpIdx = kPlugin.kFuncs->GetCmdIcon(dis->itemID);

   // Load the corresponding bitmap
   if (bmpIdx >= 0){
      int top = (dis->rcItem.bottom - dis->rcItem.top - gBmpHeight) / 2;
      top += dis->rcItem.top;

      if (dis->itemState & ODS_GRAYED)
         ImageList_DrawEx(hImgList, bmpIdx, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, 0, 0, CLR_NONE, GetSysColor(COLOR_MENU), ILD_BLEND  | ILD_TRANSPARENT);

      else if (dis->itemState & ODS_SELECTED)
         ImageList_Draw(hImgList, bmpIdx, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, ILD_TRANSPARENT);

      else
         ImageList_Draw(hImgList, bmpIdx, dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT, top, ILD_TRANSPARENT);

      return gBmpWidth;	
   }
   else if (dis->itemState & ODS_CHECKED) {

	   // The checkmark doesn't have a fixed size !!!
	   HDC hdcMem = CreateCompatibleDC(dis->hDC);
	   if (hdcMem) {

		   int cxCheck = GetSystemMetrics(SM_CXMENUCHECK);
		   int cyCheck = GetSystemMetrics(SM_CYMENUCHECK);
		   HBITMAP hbmMono = CreateBitmap(cxCheck, cyCheck, 1, 1, NULL);
		   if (hbmMono) {
			   HBITMAP hbmPrev = (HBITMAP)SelectObject(hdcMem, (HGDIOBJ)hbmMono);
			   if (hbmPrev) {
				   RECT rc = { 0, 0, cxCheck, cyCheck };
				   DrawFrameControl(hdcMem, &rc, DFC_MENU, DFCS_MENUCHECK);

				   /*COLORREF text, bgk;
				   if (dis->itemState & ODS_SELECTED) {
					   text = GetSysColor(COLOR_HIGHLIGHTTEXT);
					   bgk = GetSysColor(COLOR_HIGHLIGHT);
				   } else {
					   text = GetSysColor(COLOR_MENUTEXT);
					   bgk = GetSysColor(COLOR_MENU);
				   }

				   if (dis->itemState & ODS_GRAYED) 
					   text = GetSysColor(COLOR_GRAYTEXT);
				*/
				// COLORREF clrTextPrev = SetTextColor(dis->hDC, text );
				 //COLORREF clrBkPrev = SetBkColor(dis->hDC, bgk );
				   BitBlt(dis->hDC, dis->rcItem.left + BMP_PADDING_LEFT, 
					   dis->rcItem.top + (dis->rcItem.bottom - dis->rcItem.top - cyCheck)/2, // Seems like it need some margin
					   cxCheck, cyCheck, hdcMem, 0, 0, SRCCOPY);
				 //SetBkColor(dis->hDC, clrBkPrev);
				 //SetTextColor(dis->hDC, clrTextPrev);
				   SelectObject(hdcMem, (HGDIOBJ)hbmPrev);
			   }
			   DeleteObject(hbmMono);
		   }
		   DeleteDC(hdcMem);
	   }

    /*  // the check mark is only 6x6, so we need to offset its drawing position
      if (dis->itemState & ODS_SELECTED)
         DrawCheckMark(dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT+4, dis->rcItem.top+5, GetSysColor(COLOR_HIGHLIGHTTEXT));
      else
         DrawCheckMark(dis->hDC, dis->rcItem.left+BMP_PADDING_LEFT+4, dis->rcItem.top+5, GetSysColor(COLOR_MENUTEXT));
      return BMP_WIDTH;*/
   }
	
   return 0;
}
/*
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
*/
int GetMaxAccelWidth(HDC hDC, HMENU hMenu){
	MENUITEMINFO mmi = {0};
   mmi.cbSize = sizeof(mmi);
   
   int iMaxAccel = 0;
   TCHAR *string;
   TCHAR *tab;
   SIZE size;
   
   int count = GetMenuItemCount(hMenu);
   int i;
   for (i=0; i<count; i++) {      
      UINT state = GetMenuState(hMenu, i, MF_BYPOSITION);
      if (state & MF_OWNERDRAW) {
         mmi.fMask = MIIM_DATA;
         GetMenuItemInfo(hMenu, i, true, &mmi);            
         string = (TCHAR *)mmi.dwItemData;

         if (string) {
      
            tab = _tcschr(string, L'\t');
      
            if (tab) {
               GetTextExtentPoint32(hDC, tab+1, _tcslen(tab+1), &size);
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

   TCHAR *string = (TCHAR *)dis->itemData;

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
	  SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
   }
   else {
      FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
	  SetTextColor(dis->hDC, GetSysColor(COLOR_MENUTEXT));
	  SetBkColor(dis->hDC, GetSysColor(COLOR_MENU));
   }

   if (dis->itemState & ODS_GRAYED)
	  if (dis->itemState & ODS_SELECTED)
		 SetTextColor(dis->hDC, GetSysColor(COLOR_MENU));
      else
	     SetTextColor(dis->hDC, GetSysColor(COLOR_GRAYTEXT));
		
   
   DRAWBITMAPPROC DrawProc;
   menuMap::iterator menuIter = menus.find(menu);
   if (menuIter != menus.end()){
      DrawProc = menuIter->second;
      int space = DrawProc(dis);
   }

   dis->rcItem.left = gMarginLeft + 1; // start drawing text at the pixel after our margin

   RECT rcTab;
   
   TCHAR *tab = _tcsrchr(string, _T('\t'));
   int itemLen, tabLen;
   
   if (tab) {
      itemLen = tab - string;
      tab++;
      tabLen = lstrlen(tab);
      
      rcTab.top = dis->rcItem.top;
      rcTab.left = dis->rcItem.right - giMaxAccelWidth - MARGIN_RIGHT;
      rcTab.right = dis->rcItem.right;
      rcTab.bottom = dis->rcItem.bottom;
   }
   else {
      itemLen = lstrlen(string);
      tabLen = 0;
   }
   /*
   if (dis->itemState & ODS_GRAYED) {
      // setup pen to draw selected, grayed text
      if (dis->itemState & ODS_SELECTED) {
         SetTextColor(dis->hDC, GetSysColor(COLOR_MENU));
      }
      // Draw shadow for unselected grayed items
      else {
		  // Problematic on XP
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
   }*/
   
   DrawText(dis->hDC, string, itemLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
   if (tab) {
      DrawText(dis->hDC, tab, tabLen, &rcTab, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP );
   }
}

void MeasureMenuItem(MEASUREITEMSTRUCT *mis, HDC hDC) {
   
   SIZE size;
   TCHAR *string = (TCHAR *)mis->itemData;
   if (!string || !*string)  { // it's a separator
      mis->itemWidth = 0;
      mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE) >> 1;
      return;
   }

   HFONT oldFont = (HFONT)SelectObject(hDC, gMenuFont); 

   if (gbDrawn) {
      giMaxTextMeasured = 0;
      giMaxAccelMeasured = 0;
      gbDrawn = FALSE;
   }

#if 1
   TCHAR *tab = _tcsrchr(string, _T('\t'));
   if (tab) {
      int len = tab-string-1;
      while (*(string+len) == _T(' '))  // trim trailing spaces (to compensate for sloppy menus and TranslateTabs())
         len--;
      GetTextExtentPoint32(hDC, string, len+1, &size);
      if (size.cx > giMaxTextMeasured) giMaxTextMeasured = size.cx;
      
      GetTextExtentPoint32(hDC, tab+1, lstrlen(tab+1), &size);
      if (size.cx > giMaxAccelMeasured) giMaxAccelMeasured = size.cx;
         
      mis->itemWidth = SPACE_BETWEEN;

   }
   else {
      GetTextExtentPoint32(hDC, string, lstrlen(string), &size);
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

   mis->itemWidth += gMarginLeft + MARGIN_RIGHT;
   // compensate for the width of a chekmark (minus one pixel!) that windows so graciously adds for us
   mis->itemWidth -= (GetSystemMetrics(SM_CXMENUCHECK)-1);
   mis->itemHeight = GetSystemMetrics(SM_CYMENUSIZE);
   if (mis->itemHeight < gBmpHeight+2)
      mis->itemHeight = gBmpHeight+2;

   SelectObject(hDC, oldFont);
}

void UnSetOwnerDrawn(HMENU menu){
   menuMap::iterator menuIter = menus.find(menu);
   if (menuIter == menus.end())
      return;
   menus.erase(menu);

   MENUITEMINFO mmi = {0};
   mmi.cbSize = sizeof(mmi);

   int state;

   OSVERSIONINFO osinfo;
   osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osinfo);

   // Windows 95 works differently
   bool w95 = (osinfo.dwMajorVersion == 4) && (osinfo.dwMinorVersion == 0);

   int i;
   int count = GetMenuItemCount(menu);
   for (i=0; i<count; i++)
   {
	  //mmi.fMask =  MIIM_DATA | MIIM_ID | (w95 ? MIIM_TYPE : MIIM_FTYPE);
	   mmi.fMask =  MIIM_DATA | MIIM_ID | MIIM_TYPE;
	  GetMenuItemInfo(menu, i, true, &mmi);
	  
      state = GetMenuState(menu, i, MF_BYPOSITION);
      if (state & MF_POPUP)
         UnSetOwnerDrawn(GetSubMenu(menu, i));
      
	  if (mmi.fType & MFT_OWNERDRAW) {
         if (!mmi.dwItemData ||  !*( (TCHAR *)mmi.dwItemData) )
            ModifyMenu(menu, i, MF_BYPOSITION | MF_SEPARATOR, mmi.wID, NULL);
		 else {
            ModifyMenu(menu, i, MF_BYPOSITION | MF_STRING, mmi.wID, (TCHAR*)mmi.dwItemData);
			delete[] (TCHAR*)mmi.dwItemData; 
		 }
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
   mmi.dwTypeData = new TCHAR[mmi.cch];
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
         ModifyMenu(menu, i, MF_BYPOSITION  | MF_OWNERDRAW, i, _T(""));
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
