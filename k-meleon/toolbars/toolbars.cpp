/*
*  Copyright (C) 2001 Jeff Doozan
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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdlib.h>

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\utils.h"
#include "..\KMeleonConst.h"

#include <afxres.h>
#include "..\resource.h"

#define _T(x) x

/* Begin K-Meleon Plugin Header */

int  Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);
int  DoAccel(char *param);
int  GetConfigFiles(configFileType **configFiles);


pluginFunctions pFunc = {
   Init,
   NULL,
   Config,
   Quit,
   NULL,
   DoRebar,
   NULL,
   GetConfigFiles   
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   "Toolbar Control Plugin",
   &pFunc
};

/* End K-Meleon Plugin Header */

struct s_button {
   char *sName;
//   char *sToolTip;
   int iID;
   int width, height;
   s_button *next;
};

struct s_toolbar {

   HWND hWnd;

   char *sTitle;

   HIMAGELIST hot;
   HIMAGELIST cold;
   HIMAGELIST dead;

   int iButtonCount;
   int width, height;

   s_button *buttonHead;
   s_toolbar *next;
};
s_toolbar *toolbar_head = NULL;


void LoadToolbars(char *filename);
void AddImageToList(s_toolbar *toolbar, HIMAGELIST list, char *file, int index, int width, int height);
s_toolbar *AddToolbar(char *name, int width, int height);
s_button  *AddButton(s_toolbar *toolbar, char *name, int width, int height);
void EndButton(s_toolbar *toolbar, s_button *button, int state);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   return TRUE;
}

char gszConfigDir[MAX_PATH];

int Init() {
   char szConfigFile[MAX_PATH];
   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", gszConfigDir, "");
   if (! *gszConfigDir)
      return 0;
   strcpy(szConfigFile, gszConfigDir);
   strcat(szConfigFile, "toolbars.cfg");
   LoadToolbars(szConfigFile);

   return 1;
}

void Config(HWND hWndParent) {
   char cfgPath[MAX_PATH];
   kPlugin.kf->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), cfgPath, "");
   strcat(cfgPath, "toolbars.cfg");
   ShellExecute(NULL, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];
int GetConfigFiles(configFileType **configFiles)
{
   char cfgPath[MAX_PATH];
   kPlugin.kf->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), cfgPath, "");

   strcpy(g_configFiles[0].file, cfgPath);
   strcat(g_configFiles[0].file, "toolbars.cfg");

   strcpy(g_configFiles[0].label, "Toolbars");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}


void Quit() {
   s_toolbar *tempbar, *toolbar = toolbar_head;
   s_button *tempbtn, *button;

   while (toolbar) {
      if (toolbar->sTitle)
         delete toolbar->sTitle;

      if (toolbar->hot)
         ImageList_Destroy(toolbar->hot);
      if (toolbar->cold)
         ImageList_Destroy(toolbar->cold);
      if (toolbar->dead)
         ImageList_Destroy(toolbar->dead);

      button = toolbar->buttonHead;
      while (button) {
         if (button->sName)
            delete button->sName;
//         if (button->sToolTip)
//            delete button->sToolTip;

         tempbtn = button;
         button = button->next;
         delete tempbtn;
      }

      tempbar = toolbar;
      toolbar = toolbar->next;
      delete tempbar;
   }
}

void DoRebar(HWND rebarWnd) {

   s_toolbar   *toolbar = toolbar_head;
   s_button    *button;
   TBBUTTON     *buttons = NULL;

   while (toolbar) {
      if (toolbar->iButtonCount == 0) continue;

      // Create the toolbar control to be added.
      HWND hwndTB = kPlugin.kf->CreateToolbar(GetParent(rebarWnd), CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS);
      if (!hwndTB){
         MessageBox(NULL, "Failed to create toolbar", "K-Meleon Toolbar Plugin", MB_OK);
         return;
      }

      buttons = new TBBUTTON[toolbar->iButtonCount];
      button = toolbar->buttonHead;
      int i = 0;
      while (button) {
         buttons[i].iBitmap = i;
         buttons[i].idCommand = button->iID;
         buttons[i].iString = i;

         buttons[i].dwData = 0;
         buttons[i].fsState = TBSTATE_ENABLED;
         buttons[i].fsStyle = TBSTYLE_BUTTON;
         buttons[i].bReserved[0] = 0;
         i++;
         button = button->next;
      }
      SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
      SendMessage(hwndTB, TB_ADDBUTTONS, i, (LPARAM) buttons);

      // if no images were specified, then we'll be using text buttons
      if (!toolbar->hot) {
         char buf[1024]; // you'd have to be crazy to use more than 1K worth of toolbar text on a single toolbar       
         int buflen = 0;
         button = toolbar->buttonHead;
         while (button) {
            int len = strlen(button->sName);
            if (buflen + len > 1024) break;
            strcpy(buf+buflen, button->sName);
            buflen += len;
            buf[buflen] = 0;
            buflen++;
            button = button->next;
         }
         buf[buflen] = 0;
         SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM) buf);
         SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM) NULL);
      }
      else {
         // if only one image was specified, set that as the default image
         // instead of making it the "hot" image
         if (!toolbar->cold)
            SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM) toolbar->hot);
         else {
            SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM) toolbar->hot);
            SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM) toolbar->cold);
            if (toolbar->dead)
               SendMessage(hwndTB, TB_SETDISABLEDIMAGELIST, 0, (LPARAM) toolbar->dead);
         }
      }

      SetWindowPos(hwndTB, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED);


      // Register the band name and child hwnd
      kPlugin.kf->RegisterBand(hwndTB, toolbar->sTitle);

      REBARBANDINFO rbBand;
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;

      DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0); 
      rbBand.fStyle     = RBBS_FIXEDBMP;
      rbBand.lpText     = NULL;
      rbBand.hwndChild  = hwndTB;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cxIdeal    = LOWORD(dwBtnSize) * toolbar->iButtonCount;
      rbBand.cx         = rbBand.cxIdeal;

      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

      toolbar = toolbar->next;
      if (buttons) {
         delete buttons;
         buttons = NULL;
      }
   }
}

enum state {
   TOOLBAR = 0,// waiting for toolbar to be added
   BUTTON,     // expecting a button to be added
   ID,         // expecting a button id
//   DESC,       // expecting a tooltip/description
   HOT,        // expecting theh hot image
   COLD,       // expecting the cold image
   DEAD    // expecting the disabled image
};
    

/* Sample config 

Sample ToolBar(16,16) {                # (width, height) is optional, defaults to 16, 16
   Button1(16, 16) {                   # the (width, height) is optional, defaults to toolbar dimensions
      ID_NAV_STOP                      # command
      c:\tool1.bmp[0]                  # hot image
      c:\tool2.bmp[0]                  # cold image (optional)
   }
}

*/

void LoadToolbars(char *filename) {
   HANDLE configFile;

   configFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, NULL, NULL);
   if (configFile == INVALID_HANDLE_VALUE) {
      MessageBox(NULL, "Could not open file", filename, MB_OK);
      return;
   }
   DWORD length = GetFileSize(configFile, NULL);
   char *buf = new char[length + 1];
   ReadFile(configFile, buf, length, &length, NULL);
   buf[length] = 0;
   CloseHandle(configFile);  
   
   s_toolbar *curToolbar = NULL;
   s_button  *curButton = NULL;
   int iBuildState = TOOLBAR;

   char *p = strtok(buf, "\r\n");
   while (p) {

      p = SkipWhiteSpace(p);
      TrimWhiteSpace(p);

      if (p[0] == '#') {}

      // empty line
      else if (p[0] == 0) {}
      
      // end block
      else if (p[0] == '}') {
         if (iBuildState > BUTTON) {
            EndButton(curToolbar, curButton, iBuildState);
            iBuildState = BUTTON;
         }
         else
            iBuildState = TOOLBAR;
         
      }

      // "ToolBar Name(width, height) {"
      // "ToolButton(width, height) {"
      else if (iBuildState <= BUTTON) {
         // There can only be 2 things outside a toolbar definition
         //   comments, and the beginning of a toolbar block
         char *cb = strchr(p, '{');
         if (cb) {
            *cb = 0;
            p = SkipWhiteSpace(p);
            TrimWhiteSpace(p);


            int width, height;
            if (iBuildState == TOOLBAR) {
               width=16;
               height=16;
            }
            else {
               width = curToolbar->width;
               height = curToolbar->height;
            }
            char *pp, *val;
            if ((pp = strchr(p, '('))) {
               *pp = 0;
               pp++;               
               // get width from parameters
               while(*pp && (*pp==' ' || *pp=='\t')) pp++;
               if (*pp) {
                  val = pp;
                  if ( (pp = strchr(pp, ','))  ) {
                     *pp = 0;
                     pp++;
                     width = atoi(val);
                  }
               }               
               // get height from parameters
               while(*pp && (*pp==' ' || *pp=='\t')) pp++;
               if (*pp) {
                  val = pp;
                  height = atoi(val);
               }
            };

            if (iBuildState == TOOLBAR) {
               // add new toolbar
               if (curToolbar) {
                  curToolbar->next = AddToolbar(p, width, height);
                  curToolbar = curToolbar->next;
                  curButton = NULL;
               }
               else
                  toolbar_head = curToolbar = AddToolbar(p, width, height);
               iBuildState++;
            }
            else if (iBuildState == BUTTON) {
               // add new button
               if (curButton) {
                  curButton->next = AddButton(curToolbar, p, width, height); 
                  curButton = curButton->next;
               }
               else
                  curToolbar->buttonHead = curButton = AddButton(curToolbar, p, width, height);
               iBuildState++;
            }
            else {
               MessageBox(NULL, "Extra { found", NULL, MB_OK);
            }
         }
      }

      // button data
      else {
         
         switch (iBuildState) {
         case ID:
            
            // check for call to other plugin
            char *op;
            op = strchr(p, '(');
            if (op) {
               *op = 0;
               char *param = op+1;
               char *plugin = p;
               p = strchr(param, ')');
               if (p) *p =0;
               curButton->iID = kPlugin.kf->GetAccel(plugin, param);
            }
            
            // check for numeric value
            else if (p[0] >= '0' && p[0] <='9')
               curButton->iID = atoi(p);
            
            
            // check for defined value
#define CHECK(VAR) \
   else if (!strcmp(#VAR, p)) \
            curButton->iID = VAR;
            
            CHECK(ID_NAV_BACK)
               CHECK(ID_NAV_FORWARD)
               CHECK(ID_NAV_HOME)
               CHECK(ID_NAV_STOP)
               CHECK(ID_NAV_RELOAD)
               CHECK(ID_NAV_SEARCH)
               CHECK(ID_VIEW_SOURCE)
               CHECK(ID_VIEW_TOOLBAR)
               CHECK(ID_VIEW_STATUS_BAR)
               CHECK(ID_PREFERENCES)
               CHECK(ID_NEW_BROWSER)
               CHECK(ID_FILE_SAVE_AS)
               CHECK(ID_FILE_OPEN)
               CHECK(ID_FILE_CLOSE)
               CHECK(ID_FILE_PRINT)
               CHECK(ID_EDIT_CUT)
               CHECK(ID_EDIT_COPY)
               CHECK(ID_EDIT_PASTE)
               CHECK(ID_EDIT_UNDO)
               CHECK(ID_EDIT_CLEAR)
               CHECK(ID_EDIT_SELECT_ALL)
               CHECK(ID_EDIT_SELECT_NONE)
               CHECK(ID_EDIT_FIND)
               CHECK(ID_EDIT_FINDNEXT)
               CHECK(ID_EDIT_FINDPREV)
               CHECK(ID_VIEW_IMAGE)
               CHECK(ID_SAVE_IMAGE_AS)
               CHECK(ID_SAVE_LINK_AS)
               CHECK(ID_APP_EXIT)
               CHECK(ID_APP_ABOUT)
               CHECK(ID_OPEN_LINK_IN_NEW_WINDOW)
               CHECK(ID_COPY_LINK_LOCATION)
               CHECK(ID_COPY_IMAGE_LOCATION)
               CHECK(ID_MANAGE_PROFILES)
               CHECK(ID_LINK_KMELEON_HOME)
               CHECK(ID_LINK_KMELEON_FORUM)
               CHECK(ID_SELECT_URL)
               CHECK(ID_OPEN_LINK_IN_BACKGROUND)
               CHECK(ID_WINDOW_NEXT)
               CHECK(ID_WINDOW_PREV)
               
               iBuildState++;
            break;
            //            case DESC:
            //               curButton->sToolTip = new char[strlen(p) + 1];
            //               strcpy(curButton->sToolTip, p);
            //               iBuildState++;
            //               break;
         case HOT:
         case COLD:
         case DEAD:
            char *file=p, *pp;
            int index=-1;
            
            if ((pp = strchr(p, '['))) {
               *pp = 0;
               pp = SkipWhiteSpace(pp+1);
               index = atoi(pp);
            };
            
            if (index == -1) index = 0;   // set default image index
            if (iBuildState == HOT) {
               if (!curToolbar->hot)
                  curToolbar->hot = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curToolbar->hot, file, index, curButton->width, curButton->height);
            }
            else if (iBuildState == COLD) {
               if (!curToolbar->cold)
                  curToolbar->cold = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curToolbar->cold, file, index, curButton->width, curButton->height);
            }
            else {
               if (!curToolbar->dead)
                  curToolbar->dead = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curToolbar->dead, file, index, curButton->width, curButton->height);
            }
            
            iBuildState++;
            break;
         }
      }
      p = strtok(NULL, "\r\n");
   } // while
   delete buf;
}

s_toolbar *AddToolbar(char *name, int width, int height) {
   s_toolbar *newToolbar = new s_toolbar;
   if (name) {
      newToolbar->sTitle = new char[strlen(name) + 1];
      strcpy (newToolbar->sTitle, name);
   }
   else
      newToolbar->sTitle = NULL;

   newToolbar->width = width;
   newToolbar->height = height;
   newToolbar->buttonHead = NULL;
   newToolbar->iButtonCount = 0;
   newToolbar->next = NULL;

   newToolbar->hot  = NULL;
   newToolbar->cold = NULL;
   newToolbar->dead = NULL;
   
   return newToolbar;
}

s_button  *AddButton(s_toolbar *toolbar, char *name, int width, int height) {
   s_button *newButton = new s_button; 
   if (name) {
      newButton->sName = new char[strlen(name) + 1];
      strcpy (newButton->sName, name);
   }
   else
      newButton->sName = NULL;

   newButton->width = width;
   newButton->height = height;
//   newButton->sToolTip = NULL;
   newButton->iID = 0;
   newButton->next = NULL;

   toolbar->iButtonCount++;

   return newButton;
}


// if no cold or disabled button images have been defined,
// we'll just have to create some
void EndButton(s_toolbar *toolbar, s_button *button, int state) {
   if (state == COLD) {
      // the following is a very messed up way of creating a new
      // bitmap that is compatible with the image we want it the imagelist
      // and then copying the image into our new bitmap

      int index = ImageList_GetImageCount(toolbar->hot)-1;

      // get a handle to the bitmap in the imagelist
      IMAGEINFO ii;
      ImageList_GetImageInfo(toolbar->hot, index, &ii);

      // create a 1x1 compatbile bitmap from the image
      HBITMAP hbmpTemp = (HBITMAP) CopyImage(ii.hbmImage, IMAGE_BITMAP, 1, 1, LR_COPYRETURNORG);

      // select the 1x1 bitmap into a new DC
      HDC hdcTemp = CreateCompatibleDC(NULL);
      SelectObject(hdcTemp, hbmpTemp);

      // create a new bitmap and DC based on the one we just created
      HDC hdcNew  = CreateCompatibleDC(hdcTemp);
      HBITMAP hbmpNew = CreateCompatibleBitmap(hdcTemp, ii.rcImage.right - ii.rcImage.left, ii.rcImage.bottom - ii.rcImage.top);
      SelectObject(hdcNew, hbmpNew);
      
      // we don't need the 1x1 bitmap DC anymore
      DeleteDC(hdcTemp);
      DeleteObject(hbmpTemp);

      // draw the image into our compatible bitmap
      ImageList_DrawEx(toolbar->hot, index, hdcNew, 0, 0, 0, 0, RGB(255, 0, 255), NULL, ILD_NORMAL);

      DeleteDC(hdcNew);

      ImageList_AddMasked(toolbar->cold, hbmpNew, RGB(255, 0, 255));

      DeleteObject(hbmpNew);
   }
}

void AddImageToList(s_toolbar *toolbar, HIMAGELIST list, char *file, int index, int width, int height) {

   if (!file || !*file)
      return;

   int xstart, ystart;

   // center the bitmap if smaller, and crop it if it's too big
   if (height > toolbar->height) height = toolbar->height;
   if (width > toolbar->width) width = toolbar->width;

   xstart = (toolbar->width - width)/2;
   ystart = (toolbar->height - height)/2;

   HDC hdcButton, hdcBitmap;
   HBITMAP hButton, hBitmap;
   HBRUSH hBrush;
   
   if (strchr(file, '\\')) {
      hBitmap = (HBITMAP)LoadImage(NULL, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   }
   else {
      char fullpath[MAX_PATH];
      strcpy(fullpath, gszConfigDir);
      strcat(fullpath, file);
      hBitmap = (HBITMAP)LoadImage(NULL, fullpath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   }
   
   hdcBitmap = CreateCompatibleDC(NULL);
   SelectObject(hdcBitmap, hBitmap);
   
   hdcButton = CreateCompatibleDC(hdcBitmap);
   hButton = CreateCompatibleBitmap(hdcBitmap, toolbar->width, toolbar->height);
   SelectObject(hdcButton, hButton);

   // fill the button with the transparency
   hBrush = CreateSolidBrush(RGB(255,0,255));
   SelectObject(hdcButton, hBrush);
   PatBlt(hdcButton, 0, 0, toolbar->width, toolbar->height, PATCOPY);

   // copy the button from the full bitmap
   BitBlt(hdcButton, xstart, ystart, width, height, hdcBitmap, width*index, 0, SRCCOPY);
   DeleteDC(hdcBitmap);
   DeleteDC(hdcButton);
   
   ImageList_AddMasked(list, hButton, RGB(255,0,255));

   DeleteObject(hBrush);
   DeleteObject(hButton);
   DeleteObject(hBitmap);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}