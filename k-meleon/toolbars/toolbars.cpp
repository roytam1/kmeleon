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

#define PLUGIN_NAME "Toolbar Control Plugin"

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
void DoRebar(HWND rebarWnd);
int GetToolbarID(char *sName);
void SetButtonImage(char *sParams);
void EnableButton(char *sParams);
int  IsButtonEnabled(char *sParams);
void CheckButton(char *sParams);
int  IsButtonChecked(char *sParams);

int  GetConfigFiles(configFileType **configFiles);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};


/* End K-Meleon Plugin Header */

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
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "GetConfigFiles") == 0) {
         *(int *)data2 = GetConfigFiles((configFileType**)data1);
      }
      else if (stricmp(subject, "SetButtonImage") == 0) {
         SetButtonImage((char *)data1);
      }
      else if (stricmp(subject, "EnableButton") == 0) {
         EnableButton((char *)data1);
      }
      else if (stricmp(subject, "IsButtonEnabled") == 0) {
         *(int *)data2 = IsButtonEnabled((char *)data1);
      }
      else if (stricmp(subject, "CheckButton") == 0) {
         CheckButton((char *)data1);
      }
      else if (stricmp(subject, "IsButtonChecked") == 0) {
         *(int *)data2 = IsButtonChecked((char *)data1);
      }

      else return 0;

      return 1;
   }
   return 0;
}

struct s_button {
   char *sName;
   char *sToolTip;
   char *sImagePath;
   HMENU menu;
   int iID;
   int width, height;
   s_button *next, *prev;
};

struct s_toolbar {

   HWND hWnd;
   WORD iID;

   char *sTitle;

   HIMAGELIST hot;
   HIMAGELIST cold;
   HIMAGELIST dead;

   int iButtonCount;
   int width, height;

   s_button *pButtonHead;
   s_button *pButtonTail;
   s_toolbar *next;

   s_toolbar *nextWindow;
   HWND       hwndWindow;
};
s_toolbar *toolbar_head = NULL;
int giToolbarCount = 0;


void LoadToolbars(char *filename);
void AddImageToList(s_toolbar *pToolbar, s_button *pButton, HIMAGELIST list, char *file);
s_toolbar *AddToolbar(char *name, int width, int height);
s_button  *AddButton(s_toolbar *toolbar, char *name, int width, int height);
void EndButton(s_toolbar *toolbar, s_button *button, int state);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   return TRUE;
}

// look for filename first in the settingsDir,
// then skinsDir\CurrentSkin,
// then skinsDir\default
// if it's not anywhere, return settingsDir

void FindSkinFile( char *szSkinFile, char *filename ) {

   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   
   char szTmpSkinDir[MAX_PATH];
   char szTmpSkinName[MAX_PATH];
   char szTmpSkinFile[MAX_PATH] = "";

   if (!szSkinFile || !filename || !*filename)
      return;

   // check for the file in the settingsDir
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szTmpSkinFile, (char*)"");
   if (*szTmpSkinFile) {
      strcat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }
   

   // it wasn't in settingsDir, check the current skin   

   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsDir", szTmpSkinDir, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsCurrent", szTmpSkinName, (char*)"");

   if (*szTmpSkinDir && *szTmpSkinName) {

      int len = strlen(szTmpSkinDir);
      if (szTmpSkinDir[len-1] != '\\')
         strcat(szTmpSkinDir, "\\");

      len = strlen(szTmpSkinName);
      if (szTmpSkinName[len-1] != '\\')
         strcat(szTmpSkinName, "\\");

      strcpy(szTmpSkinFile, szTmpSkinDir);
      strcat(szTmpSkinFile, szTmpSkinName);
      strcat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }


      // it wasn't in the current skin directory, check the default

      strcpy(szTmpSkinFile, szTmpSkinDir);
      strcat(szTmpSkinFile, "default\\");
      strcat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }

   // it wasn't anywhere, return the path to the settingsDir, in case the file is being created
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szSkinFile, (char*)"");
   if (! *szSkinFile)      // no settingsDir, bad
      strcpy(szSkinFile, filename);
   else
      strcat(szSkinFile, filename);
}


int Init() {
   char szConfigFile[MAX_PATH];

   FindSkinFile(szConfigFile, "toolbars.cfg");
   LoadToolbars(szConfigFile);

   return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WNDPROC KMeleonWndProc;
s_button *FindButton(s_toolbar *pToolbar, int iButton);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   s_toolbar *firstToolbar = NULL;
   s_toolbar *prevToolbar = NULL;
   s_toolbar *toolbar = toolbar_head;
   while (toolbar) {
     s_toolbar *newToolbar = new s_toolbar;
     *newToolbar = *toolbar;
     newToolbar->hwndWindow = parent;
     newToolbar->nextWindow = toolbar;
     newToolbar->next = NULL;
     if (prevToolbar != NULL)
       prevToolbar->next = newToolbar;
     prevToolbar = newToolbar;
     if (firstToolbar == NULL)
       firstToolbar = newToolbar;
     toolbar = toolbar->next;
   }
   toolbar_head = firstToolbar;
}

void Config(HWND hWndParent) {
   char cfgPath[MAX_PATH];
   FindSkinFile(cfgPath, "toolbars.cfg");
   ShellExecute(NULL, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];
int GetConfigFiles(configFileType **configFiles)
{
   kPlugin.kFuncs->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), g_configFiles[0].file, (char*)"");
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

      button = toolbar->pButtonHead;
      while (button) {
         if (button->sName)
            delete button->sName;
         if (button->sToolTip)
            delete button->sToolTip;

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
      toolbar->hWnd = kPlugin.kFuncs->CreateToolbar(GetParent(rebarWnd), CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS);
      if (!toolbar->hWnd){
         MessageBox(NULL, "Failed to create toolbar", PLUGIN_NAME, MB_OK);
         return;
      }

      buttons = new TBBUTTON[toolbar->iButtonCount];
      button = toolbar->pButtonHead;
      int i = 0;
      while (button) {
         buttons[i].iBitmap = i;
         buttons[i].idCommand = button->iID;
         buttons[i].iString = i;

         buttons[i].dwData = 0;
         buttons[i].fsState = TBSTATE_ENABLED;
         buttons[i].fsStyle = TBSTYLE_BUTTON | (button->menu?TBSTYLE_DROPDOWN:0);
         buttons[i].bReserved[0] = 0;
         i++;
         button = button->next;
      }
      SendMessage(toolbar->hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
      SendMessage(toolbar->hWnd, TB_ADDBUTTONS, i, (LPARAM) buttons);

      // if no images were specified, then we'll be using text buttons
      if (!toolbar->hot) {
         char buf[1024]; // you'd have to be crazy to use more than 1K worth of toolbar text on a single toolbar       
         int buflen = 0;
         button = toolbar->pButtonHead;
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
         SendMessage(toolbar->hWnd, TB_ADDSTRING, 0, (LPARAM) buf);
         SendMessage(toolbar->hWnd, TB_SETIMAGELIST, 0, (LPARAM) NULL);
      }
      else {
         // if only one image was specified, set that as the default image
         // instead of making it the "hot" image
         if (!toolbar->cold)
            SendMessage(toolbar->hWnd, TB_SETIMAGELIST, 0, (LPARAM) toolbar->hot);
         else {
            SendMessage(toolbar->hWnd, TB_SETHOTIMAGELIST, 0, (LPARAM) toolbar->hot);
            SendMessage(toolbar->hWnd, TB_SETIMAGELIST, 0, (LPARAM) toolbar->cold);
            if (toolbar->dead)
               SendMessage(toolbar->hWnd, TB_SETDISABLEDIMAGELIST, 0, (LPARAM) toolbar->dead);
         }
      }

      SetWindowPos(toolbar->hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED);


      // Register the band name and child hwnd
      kPlugin.kFuncs->RegisterBand(toolbar->hWnd, toolbar->sTitle);

      REBARBANDINFO rbBand;
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;

      DWORD dwBtnSize = SendMessage(toolbar->hWnd, TB_GETBUTTONSIZE, 0,0); 
      rbBand.fStyle     = RBBS_FIXEDBMP;
      rbBand.lpText     = NULL;
      rbBand.hwndChild  = toolbar->hWnd;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cxIdeal    = LOWORD(dwBtnSize) * toolbar->iButtonCount;
      rbBand.cx         = rbBand.cxIdeal;

      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	  if (toolbar && toolbar->nextWindow) {
			s_toolbar   *oldToolbar = toolbar->nextWindow;
			int i = 1;
			s_button *pButton = FindButton(oldToolbar, i);
			while (pButton) {
			   if (SendMessage(oldToolbar->hWnd, TB_ISBUTTONENABLED, pButton->iID, 0) != 0) 
				  SendMessage(toolbar->hWnd, TB_ENABLEBUTTON, pButton->iID, MAKELONG(1, 0));
			   if (SendMessage(oldToolbar->hWnd, TB_ISBUTTONCHECKED, pButton->iID, 0) != 0) 
				  SendMessage(toolbar->hWnd, TB_CHECKBUTTON, pButton->iID, MAKELONG(1, 0));
			   
			   i++;
			   pButton = FindButton(oldToolbar, i);
			}
	  }

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
   DESC,       // expecting a tooltip/description
   HOT,        // expecting theh hot image
   COLD,       // expecting the cold image
   DEAD    // expecting the disabled image
};
    

/* Sample config 

Sample ToolBar(16,16) {                # (width, height) is optional, defaults to 16, 16
   Button1(16, 16) {                   # the (width, height) is optional, defaults to toolbar dimensions
      ID_NAV_STOP | MenuName           # command / Menu
      Tooltip Text                     # Tooltip popup text
      c:\tool1.bmp[0]                  # hot image
      c:\tool2.bmp[0]                  # cold image (optional)
      c:\tool3.bmp[0]                  # dead image (optional)
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
                  curButton = AddButton(curToolbar, p, width, height);
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
            

/*            
            // ID_NAME|MENU
            
            // get the menu associated with the button
            curButton->menu = NULL;            
            char *pipe;
            pipe = strchr(p, '|');
            if (pipe) {
               *pipe = 0;
               TrimWhiteSpace(p);

               TrimWhiteSpace(pipe+1);
               curButton->menu = kPlugin.kFuncs->GetMenu(SkipWhiteSpace(pipe+1));
            }
*/
            
            // check for call to other plugin
            char *op;
            op = strchr(p, '(');
            if (op) {
               *op = 0;
               char *param = op+1;
               char *plugin = p;
               p = strchr(param, ')');
               if (p) *p =0;
               kPlugin.kFuncs->SendMessage(plugin, PLUGIN_NAME, "DoAccel", (long)param, (long)&curButton->iID);
            }
            
            else {
               curButton->iID = kPlugin.kFuncs->GetID(p);
               if (!curButton->iID)
                  curButton->iID = atoi(p);
            }

/*
            // if a menu wasn't explicitly set, see if the command id has a registered menu
            if (!pipe)
               curButton->menu = kPlugin.kFuncs->GetMenu(curButton->iID);
*/

            iBuildState++;

            break;
         case DESC:
               curButton->sToolTip = new char[strlen(p) + 1];
               strcpy(curButton->sToolTip, p);
               iBuildState++;
               break;
         case HOT:
         case COLD:
         case DEAD:
            if (iBuildState == HOT) {
               if (!curToolbar->hot)
                  curToolbar->hot = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curButton, curToolbar->hot, p);
            }
            else if (iBuildState == COLD) {
               if (!curToolbar->cold)
                  curToolbar->cold = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curButton, curToolbar->cold, p);
            }
            else {
               if (!curToolbar->dead)
                  curToolbar->dead = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
               AddImageToList(curToolbar, curButton, curToolbar->dead, p);
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
   newToolbar->iButtonCount = 0;
   newToolbar->next = NULL;

   newToolbar->hot  = NULL;
   newToolbar->cold = NULL;
   newToolbar->dead = NULL;

   newToolbar->iID = ++giToolbarCount;
   newToolbar->pButtonHead = NULL;
   newToolbar->pButtonTail = NULL;
   newToolbar->hWnd = NULL;
   
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
   newButton->sToolTip = NULL;
   newButton->iID = 0;
   newButton->menu = NULL;
   newButton->next = NULL;
   newButton->prev = toolbar->pButtonTail;
   newButton->sImagePath = NULL;


   if (toolbar->pButtonHead == NULL)
      toolbar->pButtonHead = newButton;
   toolbar->pButtonTail = newButton;
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

HBITMAP LoadButtonImage(s_toolbar *pToolbar, s_button *pButton, char *sFile) {

   if (!sFile || !*sFile)
      return 0;

   int index;
   if (char *p = strchr(sFile, '[')) {
      *p = 0;
      p = SkipWhiteSpace(p+1);
      index = atoi(p);
   }
   else
      index = 0; // set default image index

   int xstart, ystart;
   int height = pButton->height;
   int width = pButton->width;

   // center the bitmap if smaller, and crop it if it's too big
   if (height > pToolbar->height) height = pToolbar->height;
   if (width > pToolbar->width) width = pToolbar->width;

   xstart = (pToolbar->width - width)/2;
   ystart = (pToolbar->height - height)/2;

   HDC hdcButton, hdcBitmap;
   HBITMAP hButton, hBitmap;
   HBRUSH hBrush;
   
   if (strchr(sFile, '\\')) {
      hBitmap = (HBITMAP)LoadImage(NULL, sFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   }
   else {
      char fullpath[MAX_PATH];
      FindSkinFile(fullpath, sFile);
      hBitmap = (HBITMAP)LoadImage(NULL, fullpath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   }
   
   hdcBitmap = CreateCompatibleDC(NULL);
   SelectObject(hdcBitmap, hBitmap);
   
   hdcButton = CreateCompatibleDC(hdcBitmap);
   hButton = CreateCompatibleBitmap(hdcBitmap, pToolbar->width, pToolbar->height);
   SelectObject(hdcButton, hButton);

   // fill the button with the transparency
   hBrush = CreateSolidBrush(RGB(255,0,255));
   SelectObject(hdcButton, hBrush);
   PatBlt(hdcButton, 0, 0, pToolbar->width, pToolbar->height, PATCOPY);

   // copy the button from the full bitmap
   BitBlt(hdcButton, xstart, ystart, width, height, hdcBitmap, width*index, 0, SRCCOPY);
   DeleteDC(hdcBitmap);
   DeleteDC(hdcButton);
   
   DeleteObject(hBrush);
   DeleteObject(hBitmap);

   return hButton;

}

void AddImageToList(s_toolbar *pToolbar, s_button *pButton, HIMAGELIST list, char *sFile) {

   HBITMAP hButton = LoadButtonImage(pToolbar, pButton, sFile);

   ImageList_AddMasked(list, hButton, RGB(255,0,255));

   DeleteObject(hButton);

   delete pButton->sImagePath;
   pButton->sImagePath = strdup(sFile);
}


/*
   We use the toolbar name to ID conversion scheme because it lets us later squeeze both the
   toolbar ID the button ID into the same LONG paramater in SendMessage calls.  Since we
   need one of the two SendMessage paramaters for a return value data pointer, the
   alternative to using numeric ids would be have to be passing a pointer to a structure
   containing the toolbar string name and the button ID.
*/

int GetToolbarID(char *sToolbar) {

   s_toolbar *pToolbar = toolbar_head;
   while (pToolbar) {
      if (pToolbar->sTitle && *pToolbar->sTitle && !strcmpi(sToolbar, pToolbar->sTitle))
         return pToolbar->iID;
      pToolbar = pToolbar->next;
   }

   return 0;   // return 0 if the toolbar does not exist, the first toolbar number is 1
}


s_toolbar *FindToolbar(WORD iToolbar) {
   // make sure the toolbar id is valid
   if (iToolbar == 0 || iToolbar > giToolbarCount)
      return NULL;

   s_toolbar *pToolbar = toolbar_head;
   while (pToolbar) {
      if (pToolbar->iID == iToolbar)
         return pToolbar;
      pToolbar = pToolbar->next;
   }

   return NULL;
}

s_button *FindButton(s_toolbar *pToolbar, int iButton) {
   
   if (!pToolbar)
      return NULL;
   
   s_button *pButton = pToolbar->pButtonHead;
   while (pButton) {
      if (pButton->iID == iButton)
         return pButton;
      pButton = pButton->next;
   }

   int i = 1;
   pButton = pToolbar->pButtonHead;
   while (pButton) {
      if (i++ == iButton)
         return pButton;
      pButton = pButton->next;
   }

   return NULL;
}


enum list { IMAGELIST_HOT = 1, IMAGELIST_COLD, IMAGELIST_DEAD };


void SetButtonImage(char *sParams) {//WORD iToolbar, WORD iButton, char *sImage) {

   // sParams = "ToolbarID, BUTTON_ID, [HOT|COLD|DEAD], Path\to\new\image.bmp[0]"

   // parse paramaters

   // Get ToolbarID param
   char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iToolbar = atoi(p);
   if (!iToolbar)
      iToolbar = GetToolbarID(p);
   *c = ','; // replace comma

   // Get BUTTON_ID param   
   p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iButton = atoi(p);
   if (!iButton)
      iButton = kPlugin.kFuncs->GetID(p);
   *c = ','; // replace comma


   // Get imagelist param (hot, cold, dead)
   p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iImagelist;
   if (!strcmpi(p, "hot"))
      iImagelist = IMAGELIST_HOT;
   else if (!strcmpi(p, "cold"))
      iImagelist = IMAGELIST_COLD;
   else if (!strcmpi(p, "dead"))
      iImagelist = IMAGELIST_DEAD;
   *c = ','; // replace comma
   
   
   // get image param
   char *sImage = SkipWhiteSpace(c+1);

   
   
   
   
   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return;

   s_button *pButton = FindButton(pToolbar, iButton);
   if (!pButton)
      return;

   if (pButton->sImagePath && *pButton->sImagePath && !strcmpi(pButton->sImagePath, sImage))
      return;

   int index = SendMessage(pToolbar->hWnd, TB_COMMANDTOINDEX, pButton->iID, 0);
   
   HBITMAP hButton = LoadButtonImage(pToolbar, pButton, sImage);

   HDC hdcButton = CreateCompatibleDC(NULL);
   SelectObject(hdcButton, hButton);
   

   // Create the transparency mask
   HDC hdcMask = CreateCompatibleDC(hdcButton);
   HBITMAP hMask = CreateBitmap(pButton->width, pButton->height, 1, 1, NULL);
   SelectObject(hdcMask, hMask);   
   SetBkColor(hdcButton, RGB(255,0,255));
   BitBlt(hdcMask, 0, 0, pButton->width, pButton->height, hdcButton, 0, 0, SRCCOPY);

   DeleteObject(hdcButton);
   DeleteObject(hdcMask);

   if (iImagelist == IMAGELIST_HOT)
      ImageList_Replace(pToolbar->hot, index, hButton, hMask);
   else if (iImagelist == IMAGELIST_COLD)
      ImageList_Replace(pToolbar->cold, index, hButton, hMask);
   else if (iImagelist == IMAGELIST_DEAD)
      ImageList_Replace(pToolbar->dead, index, hButton, hMask);

   DeleteObject(hButton);
   DeleteObject(hMask);

   // force the toolbar to reload the image from the imagelist
   SendMessage(pToolbar->hWnd, TB_CHANGEBITMAP, iButton, MAKELPARAM(index, 0));

   delete pButton->sImagePath;
   pButton->sImagePath = strdup(sImage);

}


void EnableButton(char *sParams) {

   // sParams = "ToolbarID, BUTTON_ID, STATE"

   // Get ToolbarID param
   char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iToolbar = atoi(p);
   if (!iToolbar)
      iToolbar = GetToolbarID(p);
   *c = ','; // replace comma

   // Get BUTTON_ID param   
   p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iButton = atoi(p);
   if (!iButton)
      iButton = kPlugin.kFuncs->GetID(p);
   *c = ','; // replace comma

   // Get STATE param   
   p = SkipWhiteSpace(c+1);
   int iState = atoi(p);



   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return;

   while (pToolbar) {
     if (pToolbar->hWnd == NULL || pToolbar->nextWindow == NULL)
       return;
     s_button *pButton = FindButton(pToolbar, iButton);
     if (!pButton)
       return;
     
	 SendMessage(pToolbar->hWnd, TB_ENABLEBUTTON, pButton->iID, MAKELONG(iState != 0, 0));
     pToolbar = pToolbar->nextWindow;
   }
}

int IsButtonEnabled(char *sParams) {

   // sParams = "ToolbarID, BUTTON_ID"

   // Get ToolbarID param
   char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
   if (!c) return NULL;
   *c = 0;  // terminate string
   int iToolbar = atoi(p);
   if (!iToolbar)
      iToolbar = GetToolbarID(p);
   *c = ','; // replace comma

   // Get BUTTON_ID param   
   p = SkipWhiteSpace(c+1);
   int iButton = atoi(p);
   
   
   
   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return NULL;

   s_button *pButton = FindButton(pToolbar, iButton);
   if (!pButton)
      return NULL;

   return SendMessage(pToolbar->hWnd, TB_ISBUTTONENABLED, pButton->iID, 0) != 0;
}


void CheckButton(char *sParams) {

   // sParams = "ToolbarID, BUTTON_ID, STATE"

   // Get ToolbarID param
   char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iToolbar = atoi(p);
   if (!iToolbar)
      iToolbar = GetToolbarID(p);
   *c = ','; // replace comma

   // Get BUTTON_ID param   
   p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
   if (!c) return;
   *c = 0;  // terminate string
   int iButton = atoi(p);
   if (!iButton)
      iButton = kPlugin.kFuncs->GetID(p);
   *c = ','; // replace comma

   // Get STATE param   
   p = SkipWhiteSpace(c+1);
   int iState = atoi(p);



   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return;

   while (pToolbar) {
     if (pToolbar->hWnd == NULL || pToolbar->nextWindow == NULL)
       return;
     s_button *pButton = FindButton(pToolbar, iButton);
     if (!pButton)
       return;
     
     SendMessage(pToolbar->hWnd, TB_CHECKBUTTON, pButton->iID, MAKELONG(iState != 0, 0));

     pToolbar = pToolbar->nextWindow;
   }
}

int  IsButtonChecked(char *sParams) {

   // sParams = "ToolbarID, BUTTON_ID"

   // Get ToolbarID param
   char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
   if (!c) return NULL;
   *c = 0;  // terminate string
   int iToolbar = atoi(p);
   if (!iToolbar)
      iToolbar = GetToolbarID(p);
   *c = ','; // replace comma

   // Get BUTTON_ID param   
   p = SkipWhiteSpace(c+1);
   int iButton = atoi(p);
   
   
   
   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return NULL;

   s_button *pButton = FindButton(pToolbar, iButton);
   if (!pButton)
      return NULL;

   return SendMessage(pToolbar->hWnd, TB_ISBUTTONCHECKED, pButton->iID, 0) != 0;
}





LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

/*
   if (message == TB_LBUTTONHOLD) {

   }

   else if (message == WM_NOTIFY){

      LPNMHDR lpNmhdr = (LPNMHDR) lParam;

      if (lpNmhdr->code == TBN_DROPDOWN) {

         LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR) lParam;
         
         s_toolbar   *toolbar = toolbar_head;
         s_button    *button;

         while (toolbar) {
            if (toolbar->iButtonCount == 0) continue;

            button = toolbar->pButtonTail;
            while (button) {
               if (button->iID == lpnmtb->iItem && button->menu) {

                  RECT rc;
                  WPARAM index = SendMessage(lpnmtb->hdr.hwndFrom, TB_COMMANDTOINDEX, lpnmtb->iItem, 0);
                  SendMessage(lpnmtb->hdr.hwndFrom, TB_GETITEMRECT, index, (LPARAM) &rc);
                  POINT pt = { rc.left, rc.bottom };
                  ClientToScreen(lpnmtb->hdr.hwndFrom, &pt);

                  int iSel = TrackPopupMenu(button->menu, TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                  if (iSel)
                     SendMessage(hWnd, WM_COMMAND, 0, iSel);

                  return TBDDRET_DEFAULT;
               }
               button = button->prev;
            }
            toolbar = toolbar->next;
         }
      }
*/

   
   if (message == WM_NOTIFY){

      LPNMHDR lpNmhdr = (LPNMHDR) lParam;
      if (lpNmhdr->code == TTN_NEEDTEXT) {

         s_toolbar   *toolbar = toolbar_head;
         s_button    *button;

         while (toolbar) {
            if (toolbar->iButtonCount == 0) continue;
            
            button = toolbar->pButtonTail;
            while (button) {
               if (button->iID == (int) wParam) {
                  
                  LPTOOLTIPTEXT lpTiptext = (LPTOOLTIPTEXT) lParam;                  
                  lpTiptext->lpszText = button->sToolTip;
                  return NULL;

               }
               button = button->prev;
            }
            toolbar = toolbar->next;
         }

      }
   }
   else if (message == WM_CLOSE) {

	  s_toolbar *toolbar = toolbar_head;
	  s_toolbar *prevToolbar = NULL;
	  while (toolbar) {
		 if (toolbar->hwndWindow == hWnd) {
			s_toolbar *tempbar = toolbar;
			if (prevToolbar) {
			   while (toolbar) {
				  prevToolbar->nextWindow = toolbar->nextWindow;
				  prevToolbar = prevToolbar->next;
				  toolbar = toolbar->next;
			   }
			}
			else {
			   toolbar_head = toolbar->nextWindow;
			}
			delete tempbar;
			break;
		 }
		 else {
			prevToolbar = toolbar;
			toolbar = toolbar->nextWindow;
		 }
	  }
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}



// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}
