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
#include "Utils.h"

#define PLUGIN_NAME "Bitmapped Menus"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>
#include <string>
#include <vector>

#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"

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

bool gbIsComCtl6 = false;

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

BOOL bFirstRun;

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || strcmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Init") == 0) {
         Init();
      }
      else if (strcmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (strcmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (strcmp(subject, "Quit") == 0) {
         Quit();
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
   BOOL currentBitmap = false;
   int index = 0;
   char* image = 0;
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
			image = strdup(p);
            currentBitmap = true;
         }
      }
      else {
         if ( strchr( p, '}' )) {
            currentBitmap = false;
			delete image;
			image = 0;
			index = 0;
            p = strtok(NULL, "\n");	     
            continue;
         }
         
         TrimWhiteSpace(p);
         p = SkipWhiteSpace(p);
         
		 RECT r = {index*gBmpWidth,0,(index+1)*gBmpWidth,gBmpHeight};
		 kPlugin.kFuncs->SetCmdIcon(p, image, &r, 0, 0, 0, 0);
         index++;
      }
	  p = strtok(NULL, "\n");
   }
   if (image) delete(image);
}

int Init() {
   bFirstRun = TRUE;

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
   bFirstRun=FALSE;
}

void Config(HWND parent)
{
   TCHAR cfgPath[MAX_PATH];
   kPlugin.kFuncs->FindSkinFile(CONFIG_FILE, cfgPath, MAX_PATH);
   ShellExecute(parent, NULL, _T("notepad.exe"), cfgPath, NULL, SW_SHOW);
}

void Quit(){
}

// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
