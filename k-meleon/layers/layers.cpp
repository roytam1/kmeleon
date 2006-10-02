/*
 * Copyright (C) 2002-2003 Ulf Erikson <ulferikson@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#endif

#include <windows.h>
#include <commctrl.h>

#ifdef __MINGW32__
#  include "../missing.h"
#else
#  include <afxres.h>     // for ID_APP_EXIT
#endif

#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <tchar.h>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../KMeleonConst.h"
#include "../resource.h"
#include "layers.h"
#include "../Utils.h"
#include "../macros/macros.h"

#define MAX_LAYERS 32
#define BUF_SIZE 20
#define IMAGE_LAYER_BUTTON 0

#define PLUGIN_NAME  "Layered Windows Plugin"
#define PLUGIN_JUNK  953

#define _Tr(x) kPlugin.kFuncs->Translate(_T(x))

static BOOL bRebarEnabled  =   1;
static BOOL bRebarBottom   =   0;
static TCHAR szTitle[MAX_PATH > 256 ? MAX_PATH : 256] = _T("Layers:");
static int  nButtonMinWidth   =  10;
static int  nButtonMaxWidth;  // 35;
static BOOL bButtonNumbers =   0;
static BOOL nButtonStyle   =   2;
static BOOL bCloseWindow   =   0;
static BOOL bCatchOpenWindow   =   0;
static BOOL bCatchCloseWindow   =   0;
static long lastTime;

#define BS_GRAYED  1
#define BS_PRESSED 2
#define BS_3D      4
#define BS_BOLD    8

#define PREFERENCE_REBAR_ENABLED "kmeleon.plugins.layers.rebar"
#define PREFERENCE_REBAR_BOTTOM "kmeleon.plugins.layers.rebarBottom"
#define PREFERENCE_REBAR_TITLE   "kmeleon.plugins.layers.title"
#define PREFERENCE_BUTTON_WIDTH  "kmeleon.plugins.layers.width"
#define PREFERENCE_BUTTON_MINWIDTH  "kmeleon.plugins.layers.minWidth"
#define PREFERENCE_BUTTON_MAXWIDTH  "kmeleon.plugins.layers.maxWidth"
#define PREFERENCE_BUTTON_NUMBER "kmeleon.plugins.layers.numbers"
#define PREFERENCE_BUTTON_STYLE  "kmeleon.plugins.layers.style"
#define PREFERENCE_CLOSE_WINDOW  "kmeleon.plugins.layers.close"
#define PREFERENCE_CATCH_WINDOW  "kmeleon.plugins.layers.catch"
#define PREFERENCE_CATCHOPEN_WINDOW  "kmeleon.plugins.layers.catchOpen"
#define PREFERENCE_CATCHCLOSE_WINDOW "kmeleon.plugins.layers.catchClose"
#define PREFERENCE_ONOPENOPTION  "kmeleon.plugins.layers.onOpenOption"
#define PREFERENCE_ONCLOSEOPTION "kmeleon.plugins.layers.onCloseOption"
#define PREFERENCE_CONFIRMCLOSE  "kmeleon.plugins.layers.confirmClose"

BOOL APIENTRY DllMain (
        HANDLE hModule,
        DWORD ul_reason_for_call,
        LPVOID lpReserved) { 

   return TRUE;
}

LRESULT CALLBACK WndProc (
        HWND hWnd, UINT message, 
        WPARAM wParam, 
        LPARAM lParam);

void * KMeleonWndProc;

int Load();
void Create(HWND parent, LPCREATESTRUCT pCS);
void Config(HWND parent);
void Destroy(HWND hWnd);
void Quit();
void DoMenu(HMENU menu, char *param);
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

enum {New, Busy, Done, Closed};

struct layer {
   HWND hWnd;
   HWND hWndTB;
   BOOL popup;
   INT  state;
   struct layer *next;
};

struct frame {
   HWND hWndFront, hWndLast;
   struct layer *layer;
   struct frame *next;
};

struct frame *find_frame(HWND hWnd);

struct frame *root = NULL;
BOOL    bIgnore = 0;
BOOL    bDoClose = 0;
BOOL    bCaught = 0;

INT id_layer;
INT id_open_new_layer, id_open_link_front, id_open_link_back;
INT id_close_layer, id_close_all, id_close_others;
INT id_next_layer, id_prev_layer, id_last_layer;
INT id_open_frame, id_close_frame;
INT id_config;
INT id_resize;

struct menulist {
   HMENU hMenu;
   struct menulist *next;
};
typedef struct menulist MenuList;
MenuList *gMenuList = NULL;
HWND ghCurHwnd;
int curLayer;

HIMAGELIST gImagelist;
HWND ghParent;
int bLayer;
int bBack;
int bFront;
WINDOWPLACEMENT gwpOld;
int numLayers;

struct frame *getFrameByString(char *sz) {
   struct frame *pFrame = NULL;

   if (sz && *sz) {
      int frame = atoi(sz);
      if (frame > 0) {
         pFrame = root;
         while (pFrame && frame > 1) {
            pFrame = pFrame->next;
            frame--;
         }
      }
      else {
         pFrame = find_frame(ghCurHwnd);
      }
   }
   else {
      pFrame = find_frame(ghCurHwnd);
   }
   if (!pFrame)
      pFrame = root;

   return pFrame;
}

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1, (LPCREATESTRUCT)data2);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Destroy") == 0) {
         Destroy((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
         *(int *)data2 = DoAccel((char *)data1);
      }
      else if (stricmp(subject, "OpenURL") == 0) {
         ghParent = ghCurHwnd;
         bBack = 0;
         bLayer = 1;

		 kmeleonDocInfo* docinfo = kPlugin.kFuncs->GetDocInfo(NULL);
		 if (docinfo && (!docinfo->url || _tcscmp(docinfo->url, _T("about:blank"))) == 0)
		   kPlugin.kFuncs->NavigateTo((char *)data1, OPEN_NORMAL, NULL);
		 else
		   kPlugin.kFuncs->NavigateTo((char *)data1, OPEN_BACKGROUND, NULL);
      }
      else if (stricmp(subject, "OpenURLBg") == 0) {
         ghParent = ghCurHwnd;
         bBack = 1;
         bLayer = 1;
         kPlugin.kFuncs->NavigateTo((char *)data1, OPEN_BACKGROUND, NULL);
      }
      else if (stricmp(subject, "NumberOfWindows") == 0) {
         int frames = 0;
         struct frame *pFrame = root;
         while (pFrame) {
            pFrame = pFrame->next;
            frames++;
         }
         *(int *)data2 = frames;
      }
      else if (stricmp(subject, "NumberOfLayersInWindow") == 0) {
         int layers = 0;
         struct frame *pFrame;
         pFrame = getFrameByString((char *)data1);
         if (pFrame) {
            struct layer *pLayer = pFrame->layer;
            while (pLayer) {
               pLayer = pLayer->next;
               layers++;
            }
         }
         *(int *)data2 = layers;
      }
      else if (stricmp(subject, "GetLayersInWindow") == 0) {
         int len = 0;

         struct frame *pFrame;
         pFrame = getFrameByString((char *)data1);
         if (pFrame) {
            struct layer *pLayer = pFrame->layer;
            while (pLayer) {
               kmeleonDocInfo *dInfo;
               dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
               if (dInfo && dInfo->url) {
                  len += strlen(dInfo->url) + 1;
               }
               pLayer = pLayer->next;
            }

            char *cPtr = (char *)malloc(len+1);
            *(char **)data2 = cPtr;

            pLayer = pFrame->layer;
            while (pLayer) {
               kmeleonDocInfo *dInfo;
               dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
               if (dInfo && dInfo->url) {
                  strcpy(cPtr, dInfo->url);
                  cPtr += strlen(cPtr);
                  *cPtr++ = '\t';
               }
               pLayer = pLayer->next;
            }
            *cPtr = 0;
         }
      }
      else if (stricmp(subject, "AllLayersInWindow") == 0) {
         struct frame *pFrame = find_frame(ghCurHwnd);
         char *cPtr = (char *)data1;

         if (cPtr && pFrame) {
            struct layer *pLayer = pFrame->layer;
	    int id = kPlugin.kFuncs->GetID(cPtr);
	    while (pLayer) {
	      PostMessage(pLayer->hWnd, WM_COMMAND, id, 0);
	      pLayer = pLayer->next;
	    }
	 }
      }
      else if (stricmp(subject, "ReplaceLayersInWindow") == 0 ||
               stricmp(subject, "AddLayersToWindow") == 0) {
         struct frame *pFrame = find_frame(ghCurHwnd);
         char *cPtr = (char *)data1;
         
         gwpOld.length = sizeof (WINDOWPLACEMENT);
         GetWindowPlacement(ghCurHwnd, &gwpOld);
         if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
           gwpOld.showCmd = SW_RESTORE;

         if (data1 && data2 && *(char*)data2) {
            pFrame = getFrameByString((char *)data1);
            cPtr = (char *)data2;
         }

         if (cPtr && pFrame) {
            struct layer *pLayer = pFrame->layer;
            int iNextLayer = 0;
            if (pLayer)
              ghCurHwnd = pLayer->hWnd;
            gwpOld.length = sizeof (WINDOWPLACEMENT);
            GetWindowPlacement(ghCurHwnd, &gwpOld);
            
            if (stricmp(subject, "AddLayersToWindow") == 0) {
               while (pLayer) {
                  iNextLayer++;
                  pLayer = pLayer->next;
               }
            }
            
            while (pLayer && cPtr && *cPtr) {
               char *p = strchr(cPtr, '\t');
               if (p)
                  *p = 0;
               if (*cPtr) 
                  kPlugin.kFuncs->NavigateTo(cPtr, OPEN_NORMAL, pLayer->hWnd);
               if (p)
                  cPtr = p+1;
               else
                  cPtr = NULL;
               pLayer = pLayer->next;
            }

            while (cPtr && *cPtr) {
               char *p = strchr(cPtr, '\t');
               if (p)
                  *p = 0;

               if (*cPtr) {
                  ghParent = ghCurHwnd;
                  bBack = 1;
                  bLayer = 1;
                  numLayers++;
                  kPlugin.kFuncs->NavigateTo(cPtr, OPEN_BACKGROUND, NULL);
               }

               if (p)
                  cPtr = p+1;
               else
                  cPtr = NULL;
            }

            while (pLayer) {
               PostMessage(pLayer->hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,-1));
               pLayer = pLayer->next;
            }
            if (pFrame)
              pFrame->hWndFront = ghCurHwnd;
            curLayer = -1;
            PostMessage(ghCurHwnd, WM_COMMAND, id_layer+iNextLayer, 0);
         }
      }
      else return 0;
      
      return 1;
   }
   return 0;
}

struct frame *find_frame(HWND hWnd) {
   struct frame *pFrame = root;
   while (pFrame) {
      struct layer *pLayer = pFrame->layer;
      while (pLayer) {
         if (pLayer->hWnd == hWnd)
            return pFrame;
         pLayer = pLayer->next;
      }
      pFrame = pFrame->next;
   }
   return NULL;
}

struct frame *add_frame(HWND hWnd) {
   struct frame *newframe = (struct frame*)calloc(1, sizeof(struct frame));
   newframe->layer = (struct layer*)calloc(1, sizeof(struct layer));
   newframe->layer->hWnd = hWnd;
   newframe->layer->next = NULL;
   newframe->hWndFront = hWnd;
   newframe->hWndLast = NULL;
   newframe->next = NULL;
   if (root == NULL)
      root = newframe;
   else {
      struct frame *pFrame = root;
      while (pFrame->next) {
         pFrame = pFrame->next;
      }
      pFrame->next = newframe;
   }
   return newframe;
}

int add_layer(HWND hWnd, HWND parent, int flag=0) {
   struct frame *pFrame = find_frame(parent);
   if (pFrame) {
      struct layer *newlayer = (struct layer*)calloc(1, sizeof(struct layer));
      newlayer->hWnd = hWnd;
      newlayer->next = NULL;
      struct layer *pLayer = pFrame->layer;
      while (pLayer->next && 
             (flag==0 ||
              flag==1 && pLayer->hWnd!=parent)) {
         pLayer = pLayer->next;
      }
      if (pLayer->next)
         newlayer->next = pLayer->next;
      pLayer->next = newlayer;
      return 1;
   }
   else 
      add_frame(hWnd);
   return 0;
}

struct layer *find_layer(HWND hWnd) {
   struct frame *pFrame;
   struct layer *pLayer;
   
   pFrame = find_frame(hWnd);
   if (pFrame) {
      pLayer = pFrame->layer;
      while (pLayer && pLayer->hWnd != hWnd)
         pLayer = pLayer->next;
   }
   else 
      return NULL;
   return pLayer;
}

struct layer *find_next_layer(HWND hWnd) {
   struct frame *pFrame;
   struct layer *pLayer;
   
   pFrame = find_frame(hWnd);
   if (pFrame) {
      pLayer = pFrame->layer;
      while (pLayer->hWnd != hWnd)
         pLayer = pLayer->next;
      pLayer = pLayer->next;
      if (pLayer == NULL)
         pLayer = pFrame->layer;
   }
   else 
      return NULL;
   return pLayer;
}

struct layer *find_prev_layer(HWND hWnd) {
   struct frame *pFrame;
   struct layer *pLayer;
   
   pFrame = find_frame(hWnd);
   if (pFrame) {
      pLayer = pFrame->layer;
      while (pLayer->next && pLayer->next->hWnd != hWnd)
         pLayer = pLayer->next;
      return pLayer;
   }
   return NULL;
}

struct frame *del_layer(HWND hWnd) {
   struct frame *pFrame = find_frame(hWnd);
   if (pFrame == NULL)
      return NULL;
   
   struct layer *pLayer = pFrame->layer;
   if (pLayer->hWnd == hWnd) {
      pFrame->layer = pLayer->next;
      
      if (pFrame->layer == NULL) {
         if (root == pFrame) {
            root = pFrame->next;
            free(pFrame);
         }
         else {
            struct frame *prev = root;
            struct frame *frame = root->next;
            while (frame) {
               if (frame == pFrame)
                  break;
               prev = frame;
               frame = frame->next;
            }
            prev->next = frame->next;
            free(frame);
         }
         pFrame = NULL;
      }
      
   }
   else {
      struct layer *prev = pFrame->layer;
      while (pLayer) {
         if (pLayer->hWnd == hWnd)
            break;
         prev = pLayer;
         pLayer = pLayer->next;
      }
      prev->next = pLayer->next;
   }
   free(pLayer);
   return pFrame;
}

void CondenseMenuText(TCHAR *buf, int len, TCHAR *title, int index) {
   if ( (index >= 0) && (index <10) ) {
      buf[0] = _T('&');
      buf[1] = index + _T('0');
      buf[2] = _T(' ');
   }
   else
      memcpy(buf, _T("   "), 3*sizeof(TCHAR));
   
   TCHAR *p = fixString(title, 0);
   _tcsncpy(buf+3, p, len-4);
   buf[len-1] = 0;
   delete p;
}


#include "../findskin.cpp"


int nHSize, nHRes;

int Load(){
   HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL); 
   nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
   nHRes = GetDeviceCaps(hdcScreen, HORZRES);
   DeleteDC(hdcScreen);

   id_layer = kPlugin.kFuncs->GetCommandIDs(MAX_LAYERS);
   
   id_open_new_layer = kPlugin.kFuncs->GetCommandIDs(1);
   id_open_link_front = kPlugin.kFuncs->GetCommandIDs(1);
   id_open_link_back = kPlugin.kFuncs->GetCommandIDs(1);
   id_next_layer = kPlugin.kFuncs->GetCommandIDs(1);
   id_prev_layer = kPlugin.kFuncs->GetCommandIDs(1);
   id_last_layer = kPlugin.kFuncs->GetCommandIDs(1);
   id_config = kPlugin.kFuncs->GetCommandIDs(1);
   id_close_layer = kPlugin.kFuncs->GetCommandIDs(1);
   id_close_all = kPlugin.kFuncs->GetCommandIDs(1);
   id_open_frame = kPlugin.kFuncs->GetCommandIDs(1);
   id_close_frame = kPlugin.kFuncs->GetCommandIDs(1);
   id_close_others = kPlugin.kFuncs->GetCommandIDs(1);
   
   id_resize = kPlugin.kFuncs->GetCommandIDs(1);
   
   // Get the rebar status
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, &bRebarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_BOTTOM, &bRebarBottom, &bRebarBottom);
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_REBAR_TITLE, &szTitle, &szTitle);
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   int tmpWidth = 0;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &tmpWidth);
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_WIDTH, &tmpWidth, &tmpWidth);
   if (tmpWidth && !nButtonMaxWidth)
     nButtonMaxWidth = tmpWidth > 0 ? tmpWidth * nHSize / nHRes : tmpWidth;
   else if (!nButtonMaxWidth)
     nButtonMaxWidth = 35;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_BUTTON_NUMBER, &bButtonNumbers, &bButtonNumbers);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_STYLE, &nButtonStyle, &nButtonStyle);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CLOSE_WINDOW, &bCloseWindow, &bCloseWindow);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCH_WINDOW, &bCatchOpenWindow, &bCatchOpenWindow);
   bCatchCloseWindow = bCatchOpenWindow;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHOPEN_WINDOW, &bCatchOpenWindow, &bCatchOpenWindow);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHCLOSE_WINDOW, &bCatchCloseWindow, &bCatchCloseWindow);
   

   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;

   gImagelist = kPlugin.kFuncs->GetIconList();
   if (!gImagelist)
   {
	   TCHAR szFullPath[MAX_PATH];
	   FindSkinFile(szFullPath, _T("layers.bmp"));
	   FILE *fp = _tfopen(szFullPath, _T("r"));
	   if (fp) {
		   fclose(fp);
		   bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		   BITMAP bmp;
		   GetObject(bitmap, sizeof(BITMAP), &bmp);

		   ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
		   gImagelist = ImageList_Create(bmp.bmWidth, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
		   if (gImagelist && bitmap)
			   ImageList_AddMasked(gImagelist, bitmap, RGB(255, 0, 255));
		   if (bitmap)
			   DeleteObject(bitmap);
	   }
   }

   return true;
}

void Create(HWND parent, LPCREATESTRUCT pCS){
   KMeleonWndProc = (void *) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   HWND prevHwnd = NULL;
   int popup = (pCS->style & WS_POPUP) != 0;
   if (!popup && ghParent) {
     struct layer *pParentLayer = find_layer( ghParent );
     if (pParentLayer && !pParentLayer->popup)
       prevHwnd = ghParent;
   }
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHOPEN_WINDOW, &bCatchOpenWindow, &bCatchOpenWindow);
   if (!prevHwnd && bCatchOpenWindow && !popup && ghCurHwnd) {
     struct layer *pParentLayer = find_layer( ghCurHwnd );
     if (pParentLayer && !pParentLayer->popup) {
       struct frame *pFrame = find_frame(ghCurHwnd);
       if (pFrame)
         ghCurHwnd = pFrame->hWndFront;
       prevHwnd = ghCurHwnd;
       ghParent = ghCurHwnd;
     }
   }
   enum OnOpenOptions { OpenLast, OpenNext };
   int iOnOpenOption = OpenLast;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_ONOPENOPTION, &iOnOpenOption, &iOnOpenOption);

   int found = add_layer(parent, prevHwnd, numLayers > 0 ? OpenLast : iOnOpenOption);
   struct layer *pNewLayer = find_layer( parent );
   pNewLayer->state = New;
   if (pNewLayer && popup)
     pNewLayer->popup = 1;

   if (found && bLayer) {
      if (ghParent) {
         gwpOld.length = sizeof (WINDOWPLACEMENT);
         GetWindowPlacement(ghParent, &gwpOld);
         if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
           gwpOld.showCmd = SW_RESTORE;
         find_frame(parent)->hWndLast = ghParent;
         if (!bBack) {
            struct frame *pFrame = find_frame(parent);
            ghParent = pFrame->hWndFront;
            pFrame->hWndLast = pFrame->hWndFront;
            pFrame->hWndFront = parent;
            MoveWindow(parent, 
                       gwpOld.rcNormalPosition.left, gwpOld.rcNormalPosition.top, 
                       gwpOld.rcNormalPosition.right - gwpOld.rcNormalPosition.left, 
                       gwpOld.rcNormalPosition.bottom - gwpOld.rcNormalPosition.top, 
                       FALSE);
            PostMessage(parent, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
            ShowWindowAsync( ghParent, SW_HIDE );
         }
         else {
            find_frame(parent)->hWndFront = ghParent;
            PostMessage(ghParent, UWM_UPDATESESSIONHISTORY, (WPARAM)0, (LPARAM)0);
            PostMessage(ghParent, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
            ShowWindowAsync(parent, SW_HIDE);
         } 
      }
      else {
         ShowWindowAsync( parent, SW_HIDE );
      }
   }
   if (bFront) {
      PostMessage(parent, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
   }
   
   if (numLayers > 0)
      numLayers--;

   if (numLayers == 0) {
      if (bCatchOpenWindow) {
         if (!bBack || !ghParent || !found)
            ghParent = parent;
      }
      else
         ghParent = NULL;
      bBack = bFront = 0;
      bLayer = bCatchOpenWindow;
   }
}

// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   BOOL b;
   switch (uMsg) {
      case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, bRebarEnabled, 0);
		 b = bRebarBottom;
		 kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_BOTTOM, &b, &b);
		 SendDlgItemMessage(hWnd, IDC_REBARBOTTOM, BM_SETCHECK, b, 0);
         break;
      case WM_COMMAND:
         switch(HIWORD(wParam)) {
            case BN_CLICKED:
               switch (LOWORD(wParam)) {
                  case IDOK:
                     bRebarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, FALSE);
					 b = SendDlgItemMessage(hWnd, IDC_REBARBOTTOM, BM_GETCHECK, 0, 0);
					 kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_REBAR_BOTTOM, &b, false);
					 // Changing bRebarBottom now would only bring utter confusion
                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
					 break;
				  case IDC_REBARENABLED:
					  GetDlgItem(hWnd, IDC_REBARENABLED);
					  if (IsDlgButtonChecked(hWnd, IDC_REBARENABLED))
						  EnableWindow(GetDlgItem(hWnd, IDC_REBARBOTTOM), TRUE);
					  else
						  EnableWindow(GetDlgItem(hWnd, IDC_REBARBOTTOM), FALSE);						  
               }
         }
         break;
      case WM_CLOSE:
         EndDialog(hWnd, 0);
         break;
      default:
         return FALSE;
   }
   return TRUE;
}

void Config(HWND hWndParent){
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, 0);
}

void Destroy(HWND hWnd){
  struct frame *pFrame;
  struct layer *pLayer;

  pFrame = find_frame(hWnd);
  if (pFrame) {
    HWND hWndTmp = NULL;
    pLayer = pFrame->layer;
    if (pLayer->hWnd == hWnd)
      pLayer = pLayer->next;
    if (pLayer)
      hWndTmp = pLayer->hWnd;
    del_layer(hWnd);
    if (!ghCurHwnd || ghCurHwnd==hWnd)
      ghCurHwnd = hWndTmp;
	//if (pLayer) {
    gwpOld.length = sizeof (WINDOWPLACEMENT);
    GetWindowPlacement(hWnd, &gwpOld);
    if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
      gwpOld.showCmd = SW_RESTORE;
    PostMessage(ghCurHwnd, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
	//}
  }
}

void Quit(){
   if (gImagelist && gImagelist!=kPlugin.kFuncs->GetIconList())
      ImageList_Destroy(gImagelist);
   
   while (gMenuList) {
	   MenuList *tmp;
	   tmp = gMenuList;
	   gMenuList = gMenuList->next;
	   free(tmp);
   }

}

void DoMenu(HMENU menu, char *param){
   if (*param != 0){
      char *action = param;
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;
      
      int command = 0;
      if (stricmp(action, "Open") == 0) {
         command = id_open_new_layer;
      }
      else if (stricmp(action, "OpenLink") == 0) {
         command = id_open_link_front;
      }
      else if (stricmp(action, "OpenLinkBg") == 0) {
         command = id_open_link_back;
      }
      else if (stricmp(action, "Next") == 0) {
         command = id_next_layer;
      }
      else if (stricmp(action, "Prev") == 0) {
         command = id_prev_layer;
      }
      else if (stricmp(action, "Last") == 0) {
         command = id_last_layer;
      }
      else if (stricmp(action, "Config") == 0){
         command = id_config;
      }
      else if (stricmp(action, "Close") == 0){
         command = id_close_layer;
      }
      else if (stricmp(action, "OpenWindow") == 0){
         command = id_open_frame;
      }
      else if (stricmp(action, "CloseWindow") == 0){
         command = id_close_frame;
      }
      else if (stricmp(action, "CloseAll") == 0){
         command = id_close_all;
      }
      else if (stricmp(action, "CloseAllOther") == 0){
         command = id_close_others;
      }
      if (command) {
         AppendMenu(menu, MF_STRING, command, string);
      }
   }
   else {
      MenuList *tmp;
      tmp = (MenuList *) calloc(1, sizeof(struct menulist));
      tmp->hMenu = menu;
      tmp->next = gMenuList;
      gMenuList = tmp;
   }
}

int DoAccel(char *param) {
   if (stricmp(param, "Open") == 0)
      return id_open_new_layer;
   if (stricmp(param, "OpenLink") == 0)
      return id_open_link_front;
   if (stricmp(param, "OpenLinkBg") == 0)
      return id_open_link_back;
   if (stricmp(param, "Next") == 0)
      return id_next_layer;
   if (stricmp(param, "Prev") == 0)
      return id_prev_layer;
   if (stricmp(param, "Last") == 0)
      return id_last_layer;
   if (stricmp(param, "Config") == 0)
      return id_config;   
   if (stricmp(param, "OpenWindow") == 0)
      return id_open_frame;   
   if (stricmp(param, "CloseWindow") == 0)
      return id_close_frame;   
   if (stricmp(param, "Close") == 0)
      return id_close_layer;   
   if (stricmp(param, "CloseAll") == 0)
      return id_close_all;   
   if (stricmp(param, "CloseAllOther") == 0)
      return id_close_others;   
   
   if (*param && isdigit(*param)) {
      int num = atoi(param);
      if (num < MAX_LAYERS)
         return id_layer + num;
      
      return 0;
   }
   
   return id_open_new_layer;
}


void addRebarButton(HWND hWndTB, TCHAR *text, int num, int active, int image=0)
{
   int stringID;
   if (!text || *text==0)
      return;
   if ((nButtonStyle & BS_BOLD) && active) {
      TCHAR *lpszText = (TCHAR *)calloc(sizeof(TCHAR), _tcslen(text)+4 );
      _tcscpy(lpszText, _T("*"));
      _tcscat(lpszText, text);
      _tcscat(lpszText, _T("*"));
	  lpszText[_tcslen(lpszText) +1 ] = 0;
      stringID = SendMessage(hWndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)lpszText);
      free(lpszText);
   }
   else {
      stringID = SendMessage(hWndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)text);
   }
   
   TBBUTTON button = {0};
   button.iString = stringID;
   button.fsState = 0 |
      (((nButtonStyle & BS_GRAYED) && active) ? 0 : TBSTATE_ENABLED) |
      (((nButtonStyle & BS_PRESSED) && active) ? TBSTATE_CHECKED : 0);
   button.fsStyle = TBSTYLE_BUTTON | 
      ((nButtonMaxWidth < 0) ? TBSTYLE_AUTOSIZE : 0) | TBSTYLE_GROUP | TBSTYLE_ALTDRAG | TBSTYLE_WRAPABLE |
      (((nButtonStyle & BS_PRESSED) && active) ? TBSTYLE_CHECK : 0);
   button.idCommand = id_layer + num;
   if (gImagelist)
      button.iBitmap = image; //MAKELONG(IMAGE_LAYER_BUTTON, 0);
   else
      button.iBitmap = 0;
   
   SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);

}

// Build Rebar
void BuildRebar(HWND hWndTB, HWND hWndParent)
{
   if (!bRebarEnabled || !hWndTB || bIgnore)
      return;
   
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &nButtonMaxWidth);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_BUTTON_NUMBER, &bButtonNumbers, &bButtonNumbers);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_STYLE, &nButtonStyle, &nButtonStyle);
   
   int nMinWidth = nButtonMinWidth > 0 ? nButtonMinWidth * nHRes / nHSize : nButtonMinWidth;
   int nMaxWidth = nButtonMaxWidth > 0 ? nButtonMaxWidth * nHRes / nHSize : nButtonMaxWidth;

   SetWindowText(hWndTB, _T(""));
   
   // SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
   if (gImagelist)
      SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
   else
      SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);
   struct frame *pFrame = find_frame(hWndParent);
   if (!pFrame) {
     SendMessage(hWndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(nMaxWidth >= 0 ? nMaxWidth : 0, nMaxWidth >= 0 ? nMaxWidth : 0));
      addRebarButton(hWndTB, _T("K-Meleon"), 0, 0);
      return;
   }

   int width = 0;
   if (nMaxWidth > 0) {
     RECT rect;
     GetWindowRect(hWndTB,&rect);
     width = rect.right - rect.left;
     
     int i = 0;
     struct layer *pLayer = pFrame->layer;
     while (pLayer && i<MAX_LAYERS) {
       i++;
       pLayer = pLayer->next;
     }
     
     if (i)
       width = width / i;
     if (width > nMaxWidth) width = nMaxWidth;
     if (width < nMinWidth) width = nMinWidth;
   }

   SendMessage(hWndTB, TB_SETBUTTONWIDTH, 0, 
               MAKELONG(nMaxWidth >= 0 ? width : 0, 
                        nMaxWidth >= 0 ? width : 0));
   
   if (pFrame) {
      struct layer *pLayer = pFrame->layer;
      int i = 0;
      while (pLayer && i<MAX_LAYERS) {
         while (pLayer && pLayer->state==Closed)
            pLayer = pLayer->next;
         if (!pLayer)
            break;
         
         int nTextLength = nMaxWidth < 0 ? -nMaxWidth : 256;
         nTextLength = nTextLength > 4 ? nTextLength : 4;
         TCHAR *buf = new TCHAR[nTextLength + 1 + 1];
         buf[0] = 0;
         int len = 0;
		 int icon = 0;
         
         if (bButtonNumbers) {
            _itot(i, buf, 10);
            len = _tcslen(buf);
         }
         
         if (nMaxWidth) {
            kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
            if (dInfo) {
			   TCHAR *p;
			   if (dInfo->title && dInfo->title[0])
				 p = fixString(dInfo->title, 0);
			   else if (dInfo->url)
				 p = fixString(dInfo->url, 0);
			   else
			     p = _tcsdup(_T(""));

               _tcscat(buf, _T(" "));
               len++;
               if (nMaxWidth<0 && 
                  _tcslen(p) < (UINT)((-nMaxWidth) < 3 ? 3 : (-nMaxWidth))) {
                  _tcscat(buf, p);
               }
               else {
                  _tcsncat(buf, p, nTextLength - len);
                  if (nMaxWidth < 0) {
                     buf[nTextLength - 2] = 0;
                     _tcscat(buf, _T("..."));
                  }
               }
               delete p;
			   icon = dInfo->idxIcon;
			}
         }
		 buf[_tcslen(buf)+1] = 0; // for TB_ADDSTRING
         addRebarButton(hWndTB, buf, i, pLayer->hWnd == pFrame->hWndFront, icon);

         delete buf;
         
         pLayer = pLayer->next;
         i++;
      }
   }
   else {
      addRebarButton(hWndTB, _T("K-Meleon"), 0, 0);
   }
}

HWND ghWndTB;
HMENU ghMenu;
int bReadMenu;

char *getLine(char *text, char **line) {
   if (!text)
      return NULL;
   char *p = SkipWhiteSpace(text);
   
   if (line)
      *line = p;
   
   while (*p && *p != '\n')
      p++;
   
   if (*p == 0)
      return NULL;
   
   *p = 0;
   return (p+1);
}

void readMenu() {
   if (bReadMenu != 0)
      return;
   bReadMenu = 1;

   ghMenu = kPlugin.kFuncs->GetMenu("LayerButtonPopup");
}

void DoRebar(HWND rebarWnd){
   
   if (!bRebarEnabled)
      return;

   struct layer *pNewLayer = find_layer( GetParent(rebarWnd) );
   if (!pNewLayer)
     return;
   if (pNewLayer->popup)
     return;

   readMenu();
   
   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CS_DBLCLKS | CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      ((nButtonStyle & BS_3D) ? TBSTYLE_TRANSPARENT : (TBSTYLE_FLAT | TBSTYLE_TRANSPARENT)) | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;
   
   if (bRebarBottom) dwStyle |= CCS_BOTTOM;
   // Create the toolbar control to be added.
   //ghWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
   //                         WS_CHILD | dwStyle,
   //                         0,0,0,0,
   //                         rebarWnd, (HMENU)/*id*/200,
   //                         kPlugin.hDllInstance, NULL
   //                         );

   ghWndTB = kPlugin.kFuncs->CreateToolbar(GetParent(rebarWnd), dwStyle);
   
   if (!ghWndTB){
      MessageBox(NULL, _Tr("Failed to create layers toolbar"), NULL, 0);
      return;
   }
   
   pNewLayer->hWndTB = ghWndTB;
   
   // Register the band name and child hwnd
   if (szTitle && *szTitle) {
      int len = _tcslen(szTitle);
      TCHAR c = szTitle[len-1];
      if (c == ':')
         szTitle[len-1] = 0;
      kPlugin.kFuncs->RegisterBand(ghWndTB, szTitle, true);
      if (c == ':')
         _tcscat(szTitle, _T(":"));
   }
   else
      kPlugin.kFuncs->RegisterBand(ghWndTB, _T("Layers"), true);
   
   BuildRebar(ghWndTB, NULL);
   
   // Get the width & height of the toolbar.
   SIZE size;
   SendMessage(ghWndTB, TB_GETMAXSIZE, 0, (LPARAM)&size);
   
   if (!bRebarBottom)
   {
   REBARBANDINFO rbBand = {0};
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;
   
   rbBand.fStyle = RBBS_USECHEVRON | RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = szTitle;
   rbBand.hwndChild  = ghWndTB;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = size.cy + ((nButtonStyle & BS_3D) ? 4 : 0);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = size.cx + 16;
   rbBand.cx         = rbBand.cxIdeal;
   
   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   }
   else
	   SendMessage(ghWndTB, TB_SETBUTTONSIZE, 0, 
               MAKELONG(24, size.cy + ((nButtonStyle & BS_3D) ? 4 : 0)));

}

void UpdateLayersMenu (HWND hWndParent) {
   
   if (bIgnore)
      return;
   
   MenuList *tmpMenu = gMenuList;
   while (tmpMenu) {
      
      HMENU ghMenu = tmpMenu->hMenu;
      if (!ghMenu)
         break;
      
      for (int i=id_layer; i<id_layer+MAX_LAYERS; i++)
         DeleteMenu(ghMenu, i, MF_BYCOMMAND);
      
      struct frame *pFrame = find_frame(hWndParent);
      if (pFrame) {
         struct layer *pLayer = pFrame->layer;
         int i = 0;
         curLayer = -1;
         while (pLayer && i<MAX_LAYERS) {
#define TOT_SIZE 3 + BUF_SIZE + 3 + BUF_SIZE + 1
            TCHAR buf[TOT_SIZE];
            for (int j=0; j<TOT_SIZE; j++)
               buf[j] = 0;
            kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
            if (pLayer->hWnd == pFrame->hWndFront)
               curLayer = i;
            CondenseMenuText(buf, TOT_SIZE, dInfo ? dInfo->title : (TCHAR*)_T(""), i );
            AppendMenu(ghMenu, MF_ENABLED | MF_STRING | 
                       (i == curLayer ? MF_CHECKED : 0), 
                       id_layer+i , buf);
            pLayer = pLayer->next;
            i++;
         }
      }
      tmpMenu = tmpMenu->next;
   }
}


void UpdateRebarMenu(struct layer *pLayer)
{
   if (bIgnore)
      return;
   
   if (bRebarEnabled && pLayer && pLayer->hWndTB) {
      while (SendMessage(pLayer->hWndTB, TB_DELETEBUTTON, 0 /*index*/, 0));
      BuildRebar(pLayer->hWndTB, pLayer->hWnd);

   // Compute the width needed for the toolbar
   int ideal = 0;
   int iCount, iButtonCount = SendMessage(pLayer->hWndTB, TB_BUTTONCOUNT, 0,0);
   for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
   {
	   RECT rectButton;
	   SendMessage(pLayer->hWndTB, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
	   ideal += rectButton.right - rectButton.left;
   }
   HWND hReBar = FindWindowEx(pLayer->hWnd, NULL, REBARCLASSNAME, NULL);
   int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  

   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;
   for(int x=0;x < uBandCount;x++) {

	   if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x, (LPARAM) &rb))
         continue;

	  if (rb.hwndChild == pLayer->hWndTB)
	  {
		  if (iButtonCount<2)
		      rb.fStyle = RBBS_HIDDEN;
		  else
			  rb.fStyle = RBBS_USECHEVRON | RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;

		  rb.fMask  = RBBIM_IDEALSIZE ;//| RBBIM_STYLE;
		  rb.cxIdeal = ideal;

		  SendMessage(hReBar, RB_SETBANDINFO, x, (LPARAM)&rb);
		  break;
	  }
   }
   }
}

void ShowMenuUnderButton(HWND hWndParent, HMENU hMenu, UINT uMouseButton, int iID) {
   // Find the toolbar
   if (!bRebarBottom) {
   HWND hReBar = FindWindowEx(GetForegroundWindow(), NULL, REBARCLASSNAME, NULL);
   int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  
   int x = 0;
   BOOL bFound = FALSE;
   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;
   while (x < uBandCount && !bFound) {
         
      if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x++, (LPARAM) &rb))
         continue;
                  
      // toolbar hwnd
      HWND tb = rb.hwndChild;
      RECT rc;
      
      int ButtonID = SendMessage(tb, TB_COMMANDTOINDEX, iID, 0);
      if (ButtonID < 0)
         continue;
      if (ButtonID == 0) {
         TBBUTTON button;
         SendMessage(tb, TB_GETBUTTON, 0, (LPARAM) &button);
         if (button.idCommand != iID)
            continue;
      }

      SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(tb, &pt);
      DWORD SelectionMade = TrackPopupMenu(hMenu, TPM_LEFTALIGN | uMouseButton | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);
      
      PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) iID, 0);
      
      if (SelectionMade > 0) {
         PostMessage(hWndParent, WM_COMMAND, (WPARAM) SelectionMade, iID);
      }

      bFound = TRUE;
   }
   }
   else
   { // FIXME
	   POINT pt;
	   GetCursorPos(&pt);

	   DWORD SelectionMade = TrackPopupMenu(hMenu, TPM_LEFTALIGN | uMouseButton | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, 0);
      
      PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) iID, 0);
      
      if (SelectionMade > 0) {
         PostMessage(hWndParent, WM_COMMAND, (WPARAM) SelectionMade, iID);
      }

   }

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   struct frame *pFrame;
   struct layer *pLayer;
   static TCHAR *tip = NULL;

   if (!bIgnore)
      
      switch (message) {

		 case UWM_UPDATEBUSYSTATE:
			 if (wParam != 0) break;

		 case UWM_NEWSITEICON:
			 pFrame = find_frame(hWnd);
			 if (!pFrame) break;
			 if ( (pLayer = find_layer(pFrame->hWndFront)) == NULL) break;
			 UpdateRebarMenu(pLayer);
			 break;
			 
         case UWM_UPDATESESSIONHISTORY:
         case WM_SETFOCUS:
            if (bIgnore)
               break;
            pFrame = find_frame(hWnd);
            if (!pFrame)
               break;
            if (message == WM_SETFOCUS) {
               if (hWnd != pFrame->hWndFront) {
                     ShowWindowAsync(hWnd, SW_HIDE);
                     PostMessage(pFrame->hWndFront, WM_COMMAND, id_resize, (LPARAM)0);
                     break;
               }
               ghCurHwnd = hWnd;
            }
            if (message == UWM_UPDATESESSIONHISTORY ||
                hWnd == pFrame->hWndFront) {
               UpdateLayersMenu(pFrame->hWndFront);
               if ( (pLayer = find_layer(pFrame->hWndFront)) == NULL)
                  break;
               UpdateRebarMenu(pLayer);
            }
            else {
               int i = id_layer;
               pLayer = pFrame->layer;
               while (pLayer && pLayer->hWnd != hWnd) {
                  pLayer = pLayer->next;
                  i++;
               }
               if (pLayer)
                  PostMessage(hWnd, WM_COMMAND, (WPARAM)i, (LPARAM)NULL);
            }
            break;
            
         case WM_QUIT:
            bIgnore = true;
            break;
            
         case WM_CLOSE:
			 if (tip) free(tip);
			 tip = NULL;
            if (bIgnore)
               break;
            pLayer = find_layer(hWnd);
            if (!pLayer || pLayer->state == Closed)
               break;
            pFrame = find_frame(hWnd);
            if (pFrame) {
               bIgnore = true;
               kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHCLOSE_WINDOW, &bCatchCloseWindow, &bCatchCloseWindow);
               if (bCatchCloseWindow || (wParam==-1 && lParam==-1)) {
                 bDoClose = 1;
                 bCaught = 1;
                 bIgnore = false;
                 PostMessage(hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,-1));
                 return 0;
               }
               else {
                  int layers = 0;

                  pLayer = pFrame->layer;
                  while (pLayer) {
                     layers++;
                     pLayer = pLayer->next;
                  }
                  
                  if (layers > 1) {
                     BOOL bConfirmClose = FALSE;
                     kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CONFIRMCLOSE, &bConfirmClose, &bConfirmClose);
                     
                     if (bConfirmClose) {
                        TCHAR *szMessage = new TCHAR[1024];
                        _stprintf(szMessage, _Tr("There are %d layers open in this window."), layers);
						_tcscat(szMessage, "\r\n");
				        _tcscat(szMessage, _Tr("Do you want to close all of them?"));
						int ret = MessageBox(hWnd, szMessage, _Tr("Confirm Close"), MB_YESNOCANCEL | MB_ICONWARNING);
                        delete szMessage;
                        if (ret == IDCANCEL) {
                           bIgnore = false;
                           return 0;
                        }
                        if (ret == IDNO) {
                           pLayer = find_layer(hWnd);
                           if (pLayer)
                              pLayer->state = Closed;
                           PostMessage(hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,-1));
                           bIgnore = false;
                           return 0;
                        }
                     }

                     kPlugin.kFuncs->SendMessage("macros", PLUGIN_NAME, "CloseFrame", (long)hWnd, (long)layers);

                  }
                  
                  pLayer = pFrame->layer;
                  while (pLayer) {
                     HWND hWndTmp = pLayer->hWnd;
                     pLayer->state = Closed;
                     pLayer = pLayer->next;
                     if (hWndTmp != hWnd)
                        CallWindowProc((WNDPROC)KMeleonWndProc, hWndTmp, WM_CLOSE, 0, 0);
                  }
               }
               bIgnore = false;
            }
            break;
            
         case TB_RBUTTONDOWN:
         {
            WORD command = wParam;
            if ((command >= id_layer) && 
                (command < id_layer+MAX_LAYERS)) {
               
               if (ghMenu)
                  ShowMenuUnderButton(hWnd, ghMenu, TPM_RIGHTBUTTON, command);
               break;
            }
            break;
         }
            
		 case TB_LBUTTONDBLCLK:
         case TB_MBUTTONDOWN:
         {
            WORD command = wParam;
            if ((command >= id_layer) && 
                (command < id_layer+MAX_LAYERS)) {
               
               PostMessage(hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,command));
               break;
            }
            /* Fall through! */
         }

         //case TB_LBUTTONDBLCLK:
         {
            WORD command = wParam;
            if (!command) {
               HWND hWndTB = (HWND) lParam;
               pLayer = find_layer(hWnd);
               if (pLayer->hWndTB == hWndTB) {
                  PostMessage(hWnd, WM_COMMAND, id_open_new_layer, (LPARAM) 0);
               }
            }
            break;
         }

         case WM_COMMAND:
         {
            WORD command = LOWORD(wParam);
            
            if ((command == id_open_link_front) || 
                (command == id_open_link_back)) {
               
               SetActiveWindow(hWnd);
               BringWindowToTop(hWnd);

               ghParent = hWnd;
               bBack = (command == id_open_link_back);
               bLayer = 1;
               
               PostMessage(hWnd, WM_COMMAND, ID_OPEN_LINK_IN_BACKGROUND, 0);
               
               return 1;
            }
            
            else if (command == id_config){
               Config(hWnd);
               return 1;
            }

            else if (command == ID_VIEW_SOURCE) {
               BOOL bExternalViewer = FALSE;
               kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.general.sourceEnabled", &bExternalViewer, &bExternalViewer);
               kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHOPEN_WINDOW, &bCatchOpenWindow, &bCatchOpenWindow);
               
               if (bCatchOpenWindow && !bExternalViewer) {
                  ghParent = hWnd;
                  bBack = 0;
                  bLayer = 1;
                  
                  if (command == ID_VIEW_SOURCE) {
                     kmeleonDocInfo *dInfo;
                     dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
                     if (dInfo && dInfo->url) {
                        char *cTmp = new char[12 + strlen(dInfo->url)+1];
                        strcpy(cTmp, "view-source:");
                        strcat(cTmp, dInfo->url);
                        kPlugin.kFuncs->NavigateTo(cTmp, OPEN_BACKGROUND, NULL);
                        delete cTmp;
                        return 0;
                     }
                  }
               }
            }
            
            else if (command == ID_APP_EXIT) {
               bIgnore = true;
            }

            else if ((command == ID_WINDOW_NEXT) ||
                     (command == ID_WINDOW_PREV)) {
               
               struct frame *newframe = NULL;
               pFrame = find_frame(hWnd);
               newframe = root;
               if (command == ID_WINDOW_NEXT) {
                  while (newframe != pFrame)
                     newframe = newframe->next;
                  newframe = newframe->next;
                  if (newframe == NULL)
                     newframe = root;
               }
               else {
                  while (newframe->next && newframe->next != pFrame)
                     newframe = newframe->next;
               }
               if (newframe != pFrame) {
                  gwpOld.length = sizeof (WINDOWPLACEMENT);
                  GetWindowPlacement(newframe->hWndFront, &gwpOld);
                  if (gwpOld.showCmd == SW_HIDE || gwpOld.showCmd == SW_SHOWMINIMIZED) {
                    gwpOld.showCmd = SW_RESTORE;
                     PostMessage(newframe->hWndFront, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
                  } 
                  else {
                     WINDOWPLACEMENT wpTmp;
                     wpTmp.length = sizeof (WINDOWPLACEMENT);
                     GetWindowPlacement(newframe->hWndFront, &wpTmp);
                     SetWindowPlacement(newframe->hWndFront, &wpTmp);
                     BringWindowToTop(newframe->hWndFront);
                  }
               }
               
               return 1;
            }
            
            else if ((command == id_next_layer) ||
                     (command == id_prev_layer)) {
               
               pFrame = find_frame(hWnd);
               if (!pFrame)
                  break;
               
               if (command == id_next_layer)
                  pLayer = find_next_layer(hWnd);
               else 
                  pLayer = find_prev_layer(hWnd);
               if (!pLayer)
                  break;
               
               if (pLayer->hWnd != hWnd && hWnd == pFrame->hWndFront) {
                  gwpOld.length = sizeof (WINDOWPLACEMENT);
                  GetWindowPlacement(hWnd, &gwpOld);
                  if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                    gwpOld.showCmd = SW_RESTORE;
                  pFrame = find_frame(pLayer->hWnd);
                  pFrame->hWndLast = pFrame->hWndFront;
                  pFrame->hWndFront = pLayer->hWnd;
                  UpdateRebarMenu( find_layer(pFrame->hWndFront) );
                  UpdateRebarMenu( find_layer(pFrame->hWndLast) );
                  ghCurHwnd = pFrame->hWndFront;
                  PostMessage(pFrame->hWndFront, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
                  ShowWindowAsync(pFrame->hWndLast, SW_HIDE);
               }
               return 1;
            }
            
            else if (command == id_last_layer) {
               pFrame = find_frame(hWnd);
               if (!pFrame)
                  break;
               
               if (pFrame->hWndLast && pFrame->hWndLast != hWnd && hWnd == pFrame->hWndFront) {
                  gwpOld.length = sizeof (WINDOWPLACEMENT);
                  GetWindowPlacement(hWnd, &gwpOld);
                  if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                    gwpOld.showCmd = SW_RESTORE;
                  pFrame->hWndFront = pFrame->hWndLast;
                  pFrame->hWndLast = hWnd;
                  UpdateRebarMenu( find_layer(pFrame->hWndFront) );
                  UpdateRebarMenu( find_layer(pFrame->hWndLast) );
                  ghCurHwnd = pFrame->hWndFront;
                  PostMessage(pFrame->hWndFront, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
                  ShowWindowAsync(pFrame->hWndLast, SW_HIDE);
               }
               return 1;
            }
            
            else if (command == id_open_new_layer ||
                     command == id_open_frame) {
               
               if (command == id_open_new_layer) {
                  ghParent = hWnd;
                  bBack = 0;
                  bLayer = 1;
               }
               else {
                  ghParent = NULL;
                  bBack = 0;
                  bLayer = 0;
               }

               int iNewWindowOpenAs = 0;
               kPlugin.kFuncs->GetPreference(PREF_INT, "kmeleon.display.newWindowOpenAs",
                                             &iNewWindowOpenAs, &iNewWindowOpenAs);
               
               char cRetval[256 > MAX_PATH ? 256 : MAX_PATH];
               kmeleonDocInfo *dInfo;
               
               switch (iNewWindowOpenAs) {
                  
                  case PREF_NEW_WINDOW_CURRENT:
                     dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
                     kPlugin.kFuncs->NavigateTo((dInfo && dInfo->url) ? dInfo->url : "", OPEN_CLONE | (command == id_open_new_layer ? OPEN_BACKGROUND : OPEN_NEW), NULL);
                     break;
                     
                  case PREF_NEW_WINDOW_HOME:
                     kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.homePage", 
                                                   &cRetval, &cRetval);
                     kPlugin.kFuncs->NavigateTo(cRetval, command == id_open_new_layer ? OPEN_BACKGROUND : OPEN_NEW, NULL);
                     break;
                     
                  case PREF_NEW_WINDOW_BLANK:
                     kPlugin.kFuncs->NavigateTo("about:blank", command == id_open_new_layer ? OPEN_BACKGROUND : OPEN_NEW, NULL);
                     break;
                     
                  case PREF_NEW_WINDOW_URL:
                     kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.display.newWindowURL", 
                                                   &cRetval, &cRetval);
                     kPlugin.kFuncs->NavigateTo(cRetval, command == id_open_new_layer ? OPEN_BACKGROUND : OPEN_NEW, NULL);
                     break;
               }
               
               return 1;
            }
            
            else if (command == id_close_all ||
                     command == id_close_others ||
                     command == id_close_frame) {

               if (lParam >= 0) {
                  if (hWnd != GetForegroundWindow()) // || hWnd != GetActiveWindow())
                     // if (GetForegroundWindow() == GetActiveWindow())
                     if (find_frame(GetForegroundWindow()))
                        hWnd = GetForegroundWindow();
               }
               
               if (lParam > 0) {
                  pFrame = find_frame(hWnd);
                  if (pFrame) {
                     int i = lParam - id_layer;
                     pLayer = pFrame->layer;
                     while (i>0 && pLayer) {
                        pLayer = pLayer->next;
                        i--;
                     }
                     if (pLayer) {
                        SendMessage(hWnd, WM_COMMAND, lParam, -1);
                        PostMessage(pLayer->hWnd, WM_COMMAND, command, -1);
                        return 0;
                     }
                  }
               }

               pFrame = find_frame(hWnd);
               
               if (pFrame) {
                  
                  if (command == id_close_all) {
                     gwpOld.length = sizeof (WINDOWPLACEMENT);
                     if (find_frame(GetForegroundWindow()))
                       GetWindowPlacement(GetForegroundWindow(), &gwpOld);
                     else
                       GetWindowPlacement(hWnd, &gwpOld);
                     if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                       gwpOld.showCmd = SW_RESTORE;
                     bFront = 1;
                     bLayer = 0;
                     ghParent = NULL;
                     ghCurHwnd = NULL;
                     kPlugin.kFuncs->NavigateTo((char*)"", OPEN_BACKGROUND, NULL);
                  }
                  
                  bIgnore = true;
                  pLayer = pFrame->layer;
                  while (pLayer) {
                     if (pLayer->hWnd == hWnd) {
                        if (command != id_close_others)
                           pLayer->state = Closed;
                        pLayer = pLayer->next;
                        if (!pLayer)
                           break;
                     }
                     pLayer->state = Closed;
                     HWND hWndTmp = pLayer->hWnd;
                     if (hWndTmp != hWnd) {
                        CallWindowProc((WNDPROC)KMeleonWndProc, hWndTmp, WM_CLOSE, 0, 0);
                     }
                     pLayer = pFrame ? pFrame->layer : NULL;
                  }
                  bIgnore = false;
                  if (command == id_close_others) {
                     WINDOWPLACEMENT wpTmp;
                     wpTmp.length = sizeof (WINDOWPLACEMENT);
                     if (find_frame(GetForegroundWindow()))
                       GetWindowPlacement(GetForegroundWindow(), &wpTmp);
                     else
                       GetWindowPlacement(hWnd, &wpTmp);
                     SetWindowPlacement(hWnd, &wpTmp);
                     ghCurHwnd = hWnd;
                     PostMessage(hWnd, WM_COMMAND, id_resize, (LPARAM)0);
                     if (pFrame && pFrame->hWndFront)
                        UpdateLayersMenu(pFrame->hWndFront);
                     if (pFrame && pFrame->layer)
                        UpdateRebarMenu(pFrame->layer);
                  }
                  else if (command != id_close_others) {
                     PostMessage(hWnd, WM_CLOSE, 0, 0);
                  }
               }
            }
            
            else if (command == id_close_layer) {

               if (LOWORD(lParam)==PLUGIN_JUNK) {
                 WORD minone = -1;
                 lParam = HIWORD(lParam);
                 if (lParam==minone) lParam = -1;
               } else {
                 lParam = 0;
               }

               if (lParam >= 0) {
                  if (hWnd != GetForegroundWindow()) // || hWnd != GetActiveWindow())
                     // if (GetForegroundWindow() == GetActiveWindow())
                     if (find_frame(GetForegroundWindow()))
                        hWnd = GetForegroundWindow();
               }
               
               if (lParam > 0) {
                  pFrame = find_frame(hWnd);
                  if (pFrame) {
                     int i = lParam - id_layer;
                     pLayer = pFrame->layer;
                     while (i>0 && pLayer) {
                        pLayer = pLayer->next;
                        i--;
                     }
                     if (pLayer) {
                        pLayer->state = Closed;
                        PostMessage(pLayer->hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,-1));
                     }
                  }
                  return 0;
               }
               
               pLayer = find_layer(hWnd);
               if (pLayer)
                  pLayer->state = Closed;
               
               int newLayer = 0;
               pLayer = find_prev_layer(hWnd);
               
               char* url = (kPlugin.kFuncs->GetDocInfo(hWnd))->url;
               kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CLOSE_WINDOW, &bCloseWindow, &bCloseWindow);
               if ((!pLayer || pLayer->hWnd==hWnd) && !bCloseWindow && !bDoClose
                     && ( !url || !(strstr(url, "view-source:") == url))) {
                  gwpOld.length = sizeof (WINDOWPLACEMENT);
                  if (find_frame(GetForegroundWindow()))
                     GetWindowPlacement(GetForegroundWindow(), &gwpOld);
                  else
                     GetWindowPlacement(hWnd, &gwpOld);
                  if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                    gwpOld.showCmd = SW_RESTORE;
                  bFront = 1;
                  bLayer = 0;
                  ghParent = NULL;
                  ghCurHwnd = NULL;
                  newLayer = 1;
                  bDoClose = 0;
                  if (pLayer)
                     pLayer->state = Closed;
				  kPlugin.kFuncs->NavigateTo((char*)"about:blank", OPEN_BACKGROUND, NULL);
                  if (bCaught)
                     CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, WM_CLOSE, 0, 0);
                  else
                     PostMessage(hWnd, WM_CLOSE, 0, 0);
                  bCaught = 0;
                  return 1;
               }
               bDoClose = 0;
               
               pFrame = find_frame(hWnd);
               
               HWND hWndNext = NULL;
               enum OnCloseOptions { FocusLast, FocusPrevious, FocusNext };
               int iOnCloseOption = FocusLast;
               kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_ONCLOSEOPTION, &iOnCloseOption, &iOnCloseOption);
               if (iOnCloseOption == FocusPrevious) {
                  if (pFrame->layer->hWnd == hWnd && pFrame->layer->next)
                     hWndNext = pFrame->layer->next->hWnd;
                  else
                     hWndNext = pLayer->hWnd;
               }
               else if (iOnCloseOption == FocusNext && find_next_layer(hWnd)) {
                  hWndNext = find_next_layer(hWnd)->hWnd;
                  if (hWndNext == pFrame->layer->hWnd)
                     hWndNext = find_prev_layer(hWnd)->hWnd;
               }
               else {
                  if (pFrame)
                     hWndNext = (pFrame->hWndLast && pFrame->hWndLast != hWnd) ? 
                       pFrame->hWndLast : pLayer->hWnd;
               }

               if (pFrame && pLayer && pLayer->hWnd!=hWnd) {
                  int closebg = (pFrame->hWndFront != hWnd);
                  gwpOld.length = sizeof (WINDOWPLACEMENT);
                  if (find_frame(GetForegroundWindow()))
                     GetWindowPlacement(GetForegroundWindow(), &gwpOld);
                  else
                     GetWindowPlacement(hWnd, &gwpOld);
                  if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                    gwpOld.showCmd = SW_RESTORE;
                  pFrame->hWndFront = closebg ? pFrame->hWndFront : hWndNext;
                  pFrame->hWndLast = closebg ? 
                     (pFrame->hWndLast != hWnd ? pFrame->hWndLast : NULL) :
                     NULL;
                  kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CATCHOPEN_WINDOW, &bCatchOpenWindow, &bCatchOpenWindow);
                  if (!newLayer && bCatchOpenWindow)
                     ghParent = pFrame->hWndFront;
                  UpdateRebarMenu( find_layer(pFrame->hWndFront) );
                  UpdateRebarMenu( find_layer(hWnd) );
                  ghCurHwnd = pFrame->hWndFront;                  
                  PostMessage(pFrame->hWndFront, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
                  if (bCaught)
                     CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, WM_CLOSE, 0, 0);
                  else
                     PostMessage(hWnd, WM_CLOSE, 0, 0);
               }
               else {
                  if (bCaught)
                     CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, WM_CLOSE, 0, 0);
                  else
                     PostMessage(hWnd, WM_CLOSE, 0, 0);
               }
               bCaught = 0;
               return 1;
            }
            
            else if ((command >= id_layer) && 
                     (command < id_layer+MAX_LAYERS)) {
               int layer = command - id_layer;
               if (layer != curLayer) {
                  pFrame = find_frame(hWnd);
                  if (!pFrame) break;
                  pLayer = pFrame->layer;
                  int i = 0;
                  while (pLayer && i<layer) {
                     pLayer = pLayer->next;
                     i++;
                  }
                  if (pLayer && pLayer->hWnd != hWnd && hWnd == pFrame->hWndFront) {
                     kPlugin.kFuncs->SetStatusBarText("");
                     gwpOld.length = sizeof (WINDOWPLACEMENT);
                     GetWindowPlacement(hWnd, &gwpOld);
                     if (gwpOld.showCmd != SW_SHOWMAXIMIZED)
                       gwpOld.showCmd = SW_RESTORE;
                     pFrame->hWndLast = pFrame->hWndFront;
                     pFrame->hWndFront = pLayer->hWnd;
                     UpdateRebarMenu( find_layer(pFrame->hWndFront) );
                     UpdateRebarMenu( find_layer(pFrame->hWndLast) );
                     PostMessage(pFrame->hWndFront, WM_COMMAND, id_resize, (LPARAM)&gwpOld);
                     ShowWindowAsync(pFrame->hWndLast, SW_HIDE);
                  }
               }
               /*else {
                 if (clock()-lastTime < 250) {
                   PostMessage(hWnd, WM_COMMAND, id_close_layer, MAKELPARAM(PLUGIN_JUNK,command));
                 }
               }
               lastTime = clock();*/
               return 1;
            }
            
            else if (command == id_resize) {
               WINDOWPLACEMENT *wpNew = (WINDOWPLACEMENT*)lParam;
               if (wpNew)
                  SetWindowPlacement(hWnd, wpNew);

               if (!wpNew || (wpNew->showCmd == SW_RESTORE || 
                              wpNew->showCmd == SW_SHOWMAXIMIZED)) {

                 pFrame = find_frame(hWnd);
                 if (pFrame) {
                   if (pFrame->hWndFront!=hWnd) {
                     pFrame->hWndFront=hWnd;
                   }
                 }

                  BringWindowToTop(hWnd);
                  ghCurHwnd = hWnd;
               }
               
               pLayer = find_layer(hWnd);
               if (pLayer) {
                 if (pLayer->state==New)
                   pLayer->state=Done;
               }
               return 1;
            }

            break;
         }
         
         case WM_MENUSELECT:
         {
            int id = LOWORD(wParam);
            if (id >= id_open_new_layer && id < id_resize) {
               if (id == id_open_new_layer) 
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Open a new layer in current window"));
               else if (id == id_close_layer)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Close the active layer"));
               else if (id == id_open_link_front)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Open link as a new layer"));
               else if (id == id_open_link_back)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Open link as a new layer in the background"));
               else if (id == id_next_layer)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Switch to next layer"));
               else if (id == id_prev_layer)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Switch to previous layer"));
               else if (id == id_last_layer)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Return to the last active layer of current window"));
               else if (id == id_close_all)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Close all layers"));
               else if (id == id_open_frame)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Open a new browser window"));
               else if (id == id_close_frame)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Close window"));
               else if (id == id_close_others)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Close all but the active layer"));
               else if (id == id_config)
                  kPlugin.kFuncs->SetStatusBarText(_Tr("Configure the layers plugin"));
               return 1;
            }
            if ((id >= id_layer) && (id < id_layer+MAX_LAYERS)) {
               pFrame = find_frame(hWnd);
               pLayer = pFrame->layer;
               for (int i = id-id_layer; i && pLayer; i--) 
                  pLayer = pLayer->next;
               if (pLayer) {
                  kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
                  kPlugin.kFuncs->SetStatusBarText((dInfo && dInfo->url) ? dInfo->url : (char*)"");
                  return 0;
               }
            }
            
            break;
         }
         case WM_NOTIFY:
         {
            LPNMHDR lpNmhdr = (LPNMHDR) lParam;
            if (lpNmhdr->code == (UINT)TTN_NEEDTEXT) {
               int id = LOWORD(wParam);
               if ((id >= id_layer) && (id < id_layer+MAX_LAYERS)) {
                  pFrame = find_frame(hWnd);
                  pLayer = pFrame->layer;
                  for (int i = id-id_layer; i && pLayer; i--) 
                     pLayer = pLayer->next;
                  if (pLayer) {
                     kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(pLayer->hWnd);
                     if (dInfo && (dInfo->title || dInfo->url)) {
                        if (tip) 
                           free(tip);
                        tip = (TCHAR*)calloc(1, ((dInfo->title ? _tcslen(dInfo->title) : 0) + 
                                            (dInfo->url ? _tcslen(dInfo->url) : 0) + 3)*sizeof(TCHAR));
						tip[0] = 0;
                        if (dInfo->title && dInfo->title[0]) {
                           _tcscpy(tip, dInfo->title);
                           if (dInfo->url)
                              _tcscat(tip, _T("\n"));
                        }
                        if (dInfo->url)
                           _tcscat(tip, dInfo->url);
                        LPTOOLTIPTEXT lpTiptext = (LPTOOLTIPTEXT) lParam;
                        lpTiptext->lpszText = tip;
                        return 0;
                     }
                  }
               }
            }
            break;
         }
      }
   
   return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
          return &kPlugin;
   }
}
