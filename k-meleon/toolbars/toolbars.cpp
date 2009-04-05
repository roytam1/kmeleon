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
#include <shlwapi.h>

#define PLUGIN_NAME "Toolbar Control Plugin"

#define _Tr(x) kPlugin.kFuncs->Translate(x)

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\utils.h"
#include "..\KMeleonConst.h"


#include <afxres.h>
#include "..\resource.h"

/* Begin K-Meleon Plugin Header */

int  Setup();
void Create(HWND parent);
void Config(HWND parent);
void Destroy(HWND hWnd);
void Quit();
void DoRebar(HWND rebarWnd);
int GetToolbarID(char *sName);
void SetButtonImage(char *sParams);
void EnableButton(char *sParams);
int  IsButtonEnabled(char *sParams);
void CheckButton(char *sParams);
int  IsButtonChecked(char *sParams);
int AddToolbarMsg(char* sParam);
int AddButtonMsg(char* sParam);

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
         Setup();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
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
      else if (stricmp(subject, "AddToolbar") == 0) {
         *(int *)data2 = AddToolbarMsg((char *)data1);
      }
      else if (stricmp(subject, "AddButton") == 0) {
         *(int *)data2 = AddButtonMsg((char *)data1);
      }
      else return 0;

      return 1;
   }
   return 0;
}

struct s_button {
   TCHAR *sName;
   TCHAR *sToolTip;
   char *sImagePath;
   char *menu;
	//HMENU menu;
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
BOOL gbIsComCtl6 = FALSE;


void LoadToolbars(TCHAR *filename);
void AddImageToList(s_toolbar *pToolbar, s_button *pButton, HIMAGELIST list, char *file);
s_toolbar *AddToolbar(char *name, int width, int height);
s_button  *AddButton(s_toolbar *toolbar, char *name, int width, int height);
void EndButton(s_toolbar *toolbar, s_button *button, int state);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   return TRUE;
}


#include "../findskin.cpp"


int Setup()
{
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


   TCHAR szConfigFile[MAX_PATH];
   FindSkinFile(szConfigFile, _T("toolbars.cfg"));
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
   TCHAR cfgPath[MAX_PATH];
   FindSkinFile(cfgPath, _T("toolbars.cfg"));
   ShellExecute(NULL, NULL, _T("notepad.exe"), cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];
int GetConfigFiles(configFileType **configFiles)
{
   FindSkinFile(g_configFiles[0].file, _T("toolbars.cfg"));

   strcpy(g_configFiles[0].label, "Toolbars");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}

void Destroy(HWND hWnd) {
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

		 s_toolbar *bar;
		 while (tempbar)
		 {
			 bar = tempbar;
			 tempbar = tempbar->next;
			 delete bar;
		 }
         break;
      }
      else {
         prevToolbar = toolbar;
         toolbar = toolbar->nextWindow;
      }
   }
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
			if (button->menu)
				delete button->menu;
         if (button->sToolTip)
            delete button->sToolTip;
		 if (button->sImagePath)
			 delete button->sImagePath;

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
   int sep;

   while (toolbar) {
      sep = 0;
	  if (toolbar->iButtonCount == 0) {
         toolbar = toolbar->next;
		 continue;
	  }

      // Create the toolbar control to be added.
      toolbar->hWnd = kPlugin.kFuncs->CreateToolbar(GetParent(rebarWnd), CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS);
      if (!toolbar->hWnd){
         MessageBox(NULL, _Tr("Failed to create toolbar"), _T(PLUGIN_NAME), MB_OK);
         return;
      }

      buttons = new TBBUTTON[toolbar->iButtonCount];
      button = toolbar->pButtonHead;
      int i = 0;
      int j = 0;
      while (button) {
         if (button->sName!=NULL || button->width!=0 || button->height!=0) {
            buttons[i].iBitmap = j;
            buttons[i].idCommand = button->iID;
            buttons[i].iString = j;
            j++;

            buttons[i].dwData = 0;
            buttons[i].fsState = TBSTATE_ENABLED;
            buttons[i].fsStyle = TBSTYLE_BUTTON; // | (button->menu?TBSTYLE_DROPDOWN:0);
            buttons[i].bReserved[0] = 0;
         }
         else {
            buttons[i].iBitmap = 0;
            buttons[i].idCommand = 0;
            buttons[i].iString = 0;
	    sep++;

            buttons[i].dwData = 0;
            buttons[i].fsState = TBSTATE_ENABLED;
            buttons[i].fsStyle = TBSTYLE_SEP;
            buttons[i].bReserved[0] = 0;
         }

         i++;
         button = button->next;
      }
      SendMessage(toolbar->hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
      SendMessage(toolbar->hWnd, TB_ADDBUTTONS, i, (LPARAM) buttons);

      // if no images were specified, then we'll be using text buttons
      if (!toolbar->hot) {
         TCHAR buf[4096]; // you'd have to be crazy to use more than 1K worth of toolbar text on a single toolbar       
         int buflen = 0;
         button = toolbar->pButtonHead;
         while (button) {
            if (button->sName != NULL) {
               int len = _tcslen(button->sName);
               if (buflen + len > 4096) break;
               _tcscpy(buf+buflen, button->sName);
               buflen += len;
               buf[buflen] = 0;
               buflen++;
			}
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
      kPlugin.kFuncs->RegisterBand(toolbar->hWnd, toolbar->sTitle, true);

      REBARBANDINFO rbBand;
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;

      DWORD dwBtnSize = SendMessage(toolbar->hWnd, TB_GETBUTTONSIZE, 0,0); 
      rbBand.fStyle     = RBBS_FIXEDBMP;
      rbBand.lpText     = NULL;
      rbBand.hwndChild  = toolbar->hWnd;
      rbBand.cxMinChild = LOWORD(dwBtnSize) * (toolbar->iButtonCount-sep) + 8*sep;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cxIdeal    = LOWORD(dwBtnSize) * (toolbar->iButtonCount-sep) + 8*sep;
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


int ifplugin(char *p)
{
  char *q;

  if (!p || !*p)
    return 0;

  q = strchr(p, '&');
  if (q) {
    *q = 0;
    q++;
    while (*q && (*q=='&' || isspace(*q)))
      q++;
    return ifplugin(p) && ifplugin(q);
  }
  else {
    q = strchr(p, '|');
    if (q) {
      *q = 0;
      q++;
      while (*q && (*q=='|' || isspace(*q)))
	q++;
      return ifplugin(p) || ifplugin(q);
    }
    else {
      while ( *p && isspace(*p) )
	p++;
      int loaded = 1;
      if (*p=='!')
	loaded = 0;
      while ( *p && !isalpha(*p) )
	p++;
      char *plugin = p;
      while ( *p && isalpha(*p) )
	p++;
      *p = 0;
	  if (strcmp(plugin, "tabs") == 0)
	  {
         int notab = 0;
		 kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.notab", &notab, &notab);
		 return !notab;
	  }
      kmeleonPlugin * plug = kPlugin.kFuncs->Load(plugin);
      if (!plug || !plug->loaded)
	return !loaded;
      else
	return loaded;
    }
  }
  return 0;
}

void LoadToolbars(TCHAR *filename) {

   FILE* configFile = _tfopen(filename, "r");
   if (!configFile) {
      MessageBox(NULL, _Tr("K-Meleon was not able to find your toolbar settings. Your selected skin may be missing or corrupt. Please, check your skin settings in the GUI appearance section of k-meleon preferences."), _Tr(PLUGIN_NAME), MB_OK);
      return;
   }

   s_toolbar *curToolbar = NULL;
   s_button  *curButton = NULL;
   int iBuildState = TOOLBAR;

   int pauseParsing = 0;
   char buf[512];
   while (fgets(buf, 512, configFile)) {

      char* p = SkipWhiteSpace(buf);
      TrimWhiteSpace(buf);

      if (p[0] == '#') {}

      // empty line
      else if (p[0] == 0) {}

      else if (pauseParsing > 0){
	if (p[0] == '%'){
	  if (strnicmp(p+1, "ifplugin", 8) == 0) {
	    pauseParsing++;
          }
          else if (strnicmp(p+1, "else", 4) == 0) {
	    if (pauseParsing == 1) {
	      pauseParsing = 0;
	    }
          }
          else if (strnicmp(p+1, "endif", 5) == 0) {
             pauseParsing--;
          }
	}
      }
      else if (p[0] == '%'){
        if (strnicmp(p+1, "ifplugin", 8) == 0) {
            pauseParsing = !ifplugin(p+9);
         }
         else if (strnicmp(p+1, "else", 4) == 0) {
	   pauseParsing = 1;
         }
         else if (strnicmp(p+1, "endif", 5) == 0) {
         }
      }
      
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
               MessageBox(NULL, _Tr("Extra { found"), NULL, MB_OK);
            }
         }
         cb = strchr(p, '-');
         if (cb && iBuildState == BUTTON) {
            p = SkipWhiteSpace(p);
            TrimWhiteSpace(p);
            if (strcmp(p, "-") == 0) {
               // add new separator
               if (curButton) {
                  curButton->next = AddButton(curToolbar, NULL, 0, 0);
                  curButton = curButton->next;
               }
               else
                  curButton = AddButton(curToolbar, NULL, 0, 0);
            }
         }
      }
      
      // button data
      else {
         
         switch (iBuildState) {
         case ID:
            

            // ID_NAME|MENU
            
            // get the menu associated with the button
            curButton->menu = NULL;            
            char *pipe;
            pipe = strchr(p, '|');
            if (pipe) {
               *pipe = 0;
               TrimWhiteSpace(p);

               TrimWhiteSpace(pipe+1);
               curButton->menu = new char[strlen(pipe+1) + 1];
					strcpy(curButton->menu, pipe+1);
					//kPlugin.kFuncs->GetMenu(SkipWhiteSpace(pipe+1));
            }
            
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
	   if (strcmp(p, "\"\"") != 0) {
                  char* tooltip = p;
                  if (strlen(p)>1 && p[0] == '\"' && p[strlen(p)-1] == '\"') {
                    tooltip++;
                    tooltip[strlen(tooltip)-1] = 0;
                  }

						TrimWhiteSpace(tooltip);
				  const TCHAR* lpText = kPlugin.kFuncs->Translate(tooltip);
				  curButton->sToolTip = new TCHAR[_tcslen(lpText)+1];
				  _tcscpy(curButton->sToolTip, lpText);
		  }
               iBuildState++;
               break;
         case HOT:
         case COLD:
         case DEAD:
			 {
				 
				 HIMAGELIST* pImgList = NULL; 
				 switch (iBuildState) 
				 {
					case HOT: pImgList = &curToolbar->hot; break;
					case COLD: pImgList = &curToolbar->cold; break;
					default: pImgList = &curToolbar->dead; break;
				 }

				 if (!pImgList) break;
				 if (!*pImgList) 
					 *pImgList = ImageList_Create(curToolbar->width, curToolbar->height, ILC_MASK | (gbIsComCtl6 ? ILC_COLOR32 : ILC_COLORDDB), 0, 32);
				AddImageToList(curToolbar, curButton, *pImgList, p);
			 }

            
            iBuildState++;
            break;
         }
      }
   } // while
   fclose(configFile);
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
	newToolbar->nextWindow = NULL;

   newToolbar->iID = ++giToolbarCount;
   newToolbar->pButtonHead = NULL;
   newToolbar->pButtonTail = NULL;
   newToolbar->hWnd = NULL;
   
   return newToolbar;
}

s_button  *AddButton(s_toolbar *toolbar, char *name, int width, int height) {
   s_button *newButton = new s_button; 
   if (name) {
#ifdef _UNICODE
	  wchar_t q[256];
	  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, q, 256);
#else
	  char* q = name;
#endif
	  newButton->sName = new TCHAR[_tcslen(q) + 1];
	  _tcscpy (newButton->sName, q);
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

HBITMAP LoadButtonImage(s_toolbar *pToolbar, s_button *pButton, char *sFile, COLORREF* bgColor) {

   if (!sFile || !*sFile)
      return 0;

   if (bgColor) *bgColor =-1;

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
   
#ifdef _UNICODE            
   wchar_t _sFile[MAX_PATH];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sFile, -1, _sFile, MAX_PATH);
#else
   char* _sFile = sFile;
#endif
   UINT flag = LR_LOADFROMFILE | LR_CREATEDIBSECTION;

   if (strchr(sFile, '\\')) {
      hBitmap = (HBITMAP)LoadImage(NULL, _sFile, IMAGE_BITMAP, 0, 0, flag);
   }
   else {
      TCHAR fullpath[MAX_PATH];
      FindSkinFile(fullpath, _sFile);
      hBitmap = (HBITMAP)LoadImage(NULL, fullpath, IMAGE_BITMAP, 0, 0, flag);
   }

   if (!hBitmap) return NULL;
   hdcBitmap = CreateCompatibleDC(NULL);

	struct {
		BITMAPINFOHEADER header;
		COLORREF col[256];
	} bmpi = {0};

	bmpi.header.biSize = sizeof(BITMAPINFOHEADER);

	GetDIBits(hdcBitmap, hBitmap, 0, 0, NULL, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS);
	int nCol = ((width*index) % bmpi.header.biWidth) / width;
	int nLine = bmpi.header.biHeight / height - (width*index) / bmpi.header.biWidth - 1;

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

		/*if (!gbIsComCtl6) {
			bmpi.header.biBitCount = 24;
			bmpi.header.biCompression = 0;
			
			pDest = NULL;
			HBITMAP hBmp2 = CreateDIBSection(hdcBitmap, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS, (void**)&pDest, NULL, 0);
			
			if (!hBmp2) {
				DeleteObject(hBmp);
				DeleteDC(hdcBitmap);
				delete[] pData;
				return NULL;
			}			

			GetDIBits(hdcBitmap, hBmp, 0, nLineCnt, pData, (BITMAPINFO*)&bmpi, DIB_RGB_COLORS);
		
			memcpy(pDest, pData, width * height * 4);
			DeleteObject(hBmp);
			hBmp = hBmp2;
		}*/

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
   hButton = CreateCompatibleBitmap(hdcBitmap, pToolbar->width, pToolbar->height);
   HGDIOBJ oldBmp2 = SelectObject(hdcButton, hButton);

   // fill the button with the transparency
   hBrush = CreateSolidBrush(RGB(255,0,255));
   HGDIOBJ oldBrush = SelectObject(hdcButton, hBrush);
   PatBlt(hdcButton, 0, 0, pToolbar->width, pToolbar->height, PATCOPY);
   
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

void AddImageToList(s_toolbar *pToolbar, s_button *pButton, HIMAGELIST list, char *sFile) {


   COLORREF bgColor;
   HBITMAP hButton = LoadButtonImage(pToolbar, pButton, sFile, &bgColor);

   if (bgColor != -1)
      ImageList_AddMasked(list, hButton, bgColor);
   else
      ImageList_Add(list, hButton, 0);

   DeleteObject(hButton);

   free(pButton->sImagePath);
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

int AddToolbarMsg(char *sParams) {
	// sParams = ToolbarName, width, height
	char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
	if (!c) return 0;
   *c = 0;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	int width = atoi(p);
   int height = atoi(SkipWhiteSpace(c+1));

	if (toolbar_head) {
		s_toolbar *t = toolbar_head;
		while (t->next) t = t->next;
		t->next = AddToolbar(sParams, width, height);
	}
	else
		toolbar_head = AddToolbar(sParams, width, height);
  
	return 1;
}

int AddButtonMsg(char *sParams) {
	// sParams toolbarname, buttonname, command, menu, tooltip, width, height, hot, cold, head
	char *p = SkipWhiteSpace(sParams);
   char *c = strchr(sParams, ',');
	if (!c) return 0;
   *c = 0;

	s_toolbar *pToolbar = toolbar_head;
   while (pToolbar) {
		if (strcmp(pToolbar->sTitle, sParams) == 0)
			break;
      pToolbar = pToolbar->next;
   }

	if (!pToolbar) return 0;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
   *c = 0;
	char *buttonname = p;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	int command = kPlugin.kFuncs->GetID(p);
	if (!command) return 0;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	char* menu = p;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	char* tooltip = p;

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	int width = atoi(p);

	p = SkipWhiteSpace(c+1);
   c = strchr(p, ',');
	if (!c) return 0;
	*c = 0;
	int height = atoi(p);

	char* hot = 0, *cold = 0, *dead = 0;
	p = SkipWhiteSpace(c+1);
	c = strchr(p, ',');
	if (!c) {
		hot = p;
	}
	else {
		*c = 0;
		hot = p;
		p = SkipWhiteSpace(c+1);
		c = strchr(p, ',');
		if (!c) {
			cold = p;
		}
		else {
			*c = 0;
			cold = p;
			p = SkipWhiteSpace(c+1);
			dead = p;
		}
	}
	
	s_button* pButton = pToolbar->pButtonHead;
	while (pButton) {
		if (pButton->iID == command)
			return 0;
		pButton = pButton->next;
	}
	
	pButton = AddButton(pToolbar, buttonname, width, height);
	pButton->sToolTip = new char[strlen(tooltip) + 1];
	strcpy(pButton->sToolTip,  tooltip);
   pButton->menu = new char[strlen(menu) + 1];
	strcpy(pButton->menu, menu);
	pButton->iID = command;

	if (hot) {
		if (!pToolbar->hot)
			pToolbar->hot = ImageList_Create(pToolbar->width, pToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
		AddImageToList(pToolbar, pButton, pToolbar->hot, hot);
	}
	
	if (cold) {
		if (!pToolbar->cold)
			pToolbar->cold = ImageList_Create(pToolbar->width, pToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
		AddImageToList(pToolbar, pButton, pToolbar->cold, cold);
	}

	if (dead) {
		if (!pToolbar->dead)
			pToolbar->dead = ImageList_Create(pToolbar->width, pToolbar->height, ILC_MASK | ILC_COLORDDB, 0, 32);
		AddImageToList(pToolbar, pButton, pToolbar->dead, dead);
	}

	return 1;
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
   
   COLORREF bgColor;
   HBITMAP hButton = LoadButtonImage(pToolbar, pButton, sImage, &bgColor);

   HDC hdcButton = CreateCompatibleDC(NULL);
   SelectObject(hdcButton, hButton);
   
   HIMAGELIST hImgList = NULL;
   if (iImagelist == IMAGELIST_HOT)
      hImgList = pToolbar->hot;
   else if (iImagelist == IMAGELIST_COLD)
      hImgList = pToolbar->cold;
   else if (iImagelist == IMAGELIST_DEAD)
      hImgList = pToolbar->dead;

   if (bgColor != -1) {
      // Create the transparency mask
      HDC hdcMask = CreateCompatibleDC(hdcButton);
      HBITMAP hMask = CreateBitmap(pButton->width, pButton->height, 1, 1, NULL);
      SelectObject(hdcMask, hMask);   
      SetBkColor(hdcButton, bgColor);
      BitBlt(hdcMask, 0, 0, pButton->width, pButton->height, hdcButton, 0, 0, SRCCOPY);
      DeleteObject(hdcButton);
      DeleteObject(hdcMask);
	  ImageList_Replace(hImgList, index, hButton, hMask);
      DeleteObject(hMask);
   }
   else
      ImageList_Replace(hImgList, index, hButton, 0);

   DeleteObject(hButton);


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
   if (!iButton)
      iButton = kPlugin.kFuncs->GetID(p);
   
   
   
   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return NULL;

   s_button *pButton = FindButton(pToolbar, iButton);
   if (!pButton)
      return NULL;

   while (pToolbar) {
      if (pToolbar->hwndWindow == GetActiveWindow())
         return SendMessage(pToolbar->hWnd, TB_ISBUTTONENABLED, pButton->iID, 0) != 0;
      pToolbar = pToolbar->nextWindow;
   }
   return NULL;
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
   if (!iButton)
      iButton = kPlugin.kFuncs->GetID(p);
   
   
   
   s_toolbar *pToolbar = FindToolbar(iToolbar);
   if (!pToolbar)
      return NULL;

   s_button *pButton = FindButton(pToolbar, iButton);
   if (!pButton)
      return NULL;

   while (pToolbar) {
      if (pToolbar->hwndWindow == GetActiveWindow())
         return SendMessage(pToolbar->hWnd, TB_ISBUTTONCHECKED, pButton->iID, 0) != 0;
      pToolbar = pToolbar->nextWindow;
   }
   return NULL;
}



int ShowMenuUnderButton(HWND hWndParent, UINT uMouseButton, int iID) {
   
   s_toolbar   *pToolbar = toolbar_head;
   s_button    *pButton;
   HMENU       hMenu = NULL;

   // Find the window
   while (pToolbar) {
      if (pToolbar->hwndWindow == hWndParent) break;
	  pToolbar = pToolbar->nextWindow;
   }

   // Find the toolbar
   bool stop = false;
   while (pToolbar) {
      pButton = pToolbar->pButtonTail;
      while (pButton) {
          if (pButton->iID == iID && pButton->menu) {
			  hMenu = kPlugin.kFuncs->GetMenu(pButton->menu);
			  stop = true;
			  break;
		  }
		  pButton = pButton->prev;
	  }
	  if (stop) break;
	  pToolbar = pToolbar->next;
   }
			
   // Show the menu if any
   if (hMenu) {

	  RECT rc;
	  int ButtonID = SendMessage(pToolbar->hWnd, TB_COMMANDTOINDEX, iID, 0);
      if (ButtonID < 0) return 0;

	  SendMessage(pToolbar->hWnd, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(pToolbar->hWnd, &pt);
	  if (pt.x<0) pt.x = 0;
	  if (pt.y<0) pt.y = 0;
      DWORD SelectionMade = TrackPopupMenu(hMenu, TPM_LEFTALIGN | uMouseButton | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);

	  if (SelectionMade > 0) {
         SendMessage(pToolbar->hWnd, TB_SETSTATE, (WPARAM) iID, (LPARAM) MAKELONG (TBSTATE_ENABLED , 0));
         PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) iID, 0);
         PostMessage(hWndParent, WM_COMMAND, (WPARAM) SelectionMade, iID);
	  }

	  return 1;
   }
   return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
#ifdef _UNICODE
	static char* tip = NULL;
#else
	static WCHAR* tip = NULL;
#endif
	if (message == TB_RBUTTONDOWN || message == TB_LBUTTONHOLD) {
    	int command = LOWORD(wParam);
		UINT button = (message == TB_RBUTTONDOWN) ? TPM_RIGHTBUTTON : TPM_LEFTBUTTON;
		if (ShowMenuUnderButton(hWnd, button, command))
		   return 0;
	}

  else if (message == WM_NOTIFY){

      LPNMHDR lpNmhdr = (LPNMHDR) lParam;
      if (lpNmhdr->code == TTN_NEEDTEXTA || lpNmhdr->code == TTN_NEEDTEXTW) {

         s_toolbar   *toolbar = toolbar_head;
         s_button    *button;

         while (toolbar) {
            if (toolbar->iButtonCount == 0) {
				toolbar = toolbar->next;
				continue;
			}
            
            button = toolbar->pButtonTail;
            while (button) {
               if (button->iID == (int) wParam) {
                  if (lpNmhdr->code == TTN_NEEDTEXTA)
				  {
					  LPTOOLTIPTEXTA lpTiptext = (LPTOOLTIPTEXTA) lParam;                  
#ifdef _UNICODE
					  if (button->sToolTip) {
					     if (tip) free(tip);
					     unsigned lengthDst = lstrlen(button->sToolTip)*2 + 1;
                         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, button->sToolTip, -1, tip, lengthDst, "_", NULL);
					     lpTiptext->lpszText = tip;
					  }
#else
					  lpTiptext->lpszText = button->sToolTip;
#endif
				  }
				  else
				  {
					  LPTOOLTIPTEXTW lpTiptext = (LPTOOLTIPTEXTW) lParam;                  
#ifdef _UNICODE
					  lpTiptext->lpszText = button->sToolTip;
#else				  
					  if (button->sToolTip) {
					     if (tip) free(tip);
					     unsigned lengthDst = strlen(button->sToolTip) + 1;
					     tip = (WCHAR*)malloc(sizeof(WCHAR) * lengthDst);
					     MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, button->sToolTip, -1, tip, lengthDst);
					     lpTiptext->lpszText = tip;
					  }
#endif
				  }
                  return 0;
               }
               button = button->prev;
            }
            toolbar = toolbar->next;
         }

      }
   }
	else if (message == WM_CLOSE){
		if (tip) {
			free(tip);
			tip = NULL;
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
