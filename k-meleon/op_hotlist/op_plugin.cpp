/*
 * Copyright (C) 2002 Ulf Erikson <ulferikson@fastmail.fm>
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
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"
#include <errno.h>

#define WHERE
#include "op_hotlist.h"

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

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   "Opera Hotlist Plugin",
   DoMessage
};

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
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
         *(int *)data2 = DoAccel((char *)data1);
      }
      else if (stricmp(subject, "AddLink") == 0) {
         addLink((char *)data1, (char *)data2);
      }
      else if (stricmp(subject, "FindNick") == 0) {
         findNick((char *)data1, (char *)data2);
      }
      else return 0;
      
      return 1;
   }
   return 0;
}

BOOL BrowseForHotlist(char *file)
{
   OPENFILENAME ofn;
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hInstance = kPlugin.hDllInstance;
   ofn.hwndOwner = NULL;
   ofn.lpstrCustomFilter = NULL;
   ofn.nMaxCustFilter = 0;
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = MAX_PATH;
   ofn.lpstrInitialDir = NULL;
   ofn.nFileOffset = 0;
   ofn.nFileExtension = 0;
   ofn.lpstrDefExt = NULL;
   ofn.lCustData = 0;
   ofn.lpfnHook = NULL;
   ofn.lpTemplateName = NULL;
   ofn.lpstrFilter = HOTLIST_FILTER;
   ofn.lpstrFile = file;
   ofn.nMaxFile = MAX_PATH;
   ofn.Flags = OFN_PATHMUSTEXIST |  OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
   ofn.lpstrTitle = PLUGIN_NAME;
   
   if (file && GetOpenFileName(&ofn)) {
      return true;
   }
   else {
      return false;
   }
}

void getHotlistFile() {
   FILE *bmFile;
   char tmp[2048];
   
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, gHotlistFile, (char*)"");
   
   if (!gHotlistFile[0]) {
      kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_SETTINGS_DIR, gHotlistFile, (char*)"");
      strcat(gHotlistFile, HOTLIST_DEFAULT_FILENAME);
   }
   
   bmFile = fopen(gHotlistFile, "r");
   while (!bmFile) {
   retry:
      kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_SETTINGS_DIR, gHotlistFile, (char*)"");
      strcat(gHotlistFile, HOTLIST_DEFAULT_FILENAME);
      
      strcpy(tmp, "Would you like to set your hotlist file to the default value?\n\n'");
      strcat(tmp, gHotlistFile);
      strcat(tmp, "'\n");
      
      while (!bmFile && 
             MessageBox(NULL, tmp, PLUGIN_NAME, 
                        MB_ICONQUESTION | MB_YESNO) == IDNO) {
         if (!BrowseForHotlist(gHotlistFile)) {
            goto retry;
         }
         bmFile = fopen(gHotlistFile, "r");
         if (!bmFile && errno == ENOENT) {
            strcpy(tmp, "File not found:\n'");
            strcat(tmp, gHotlistFile);
            strcat(tmp, "'\n\nCreate it?\n");
         }
      }
      
      if (!bmFile) {
         bmFile = fopen(gHotlistFile, "wb");
         if (bmFile) {
            fprintf(bmFile, 
                    "Opera Hotlist version 2.0\r\n"
                    "Options:encoding=utf8,version=3\r\n\r\n");
            bDOS = 1;
            strcpy(tmp, "Hotlist file '");
            strcat(tmp, gHotlistFile);
            strcat(tmp, "' created.\n");
            MessageBox(NULL, tmp, PLUGIN_NAME, MB_OK);
         }
         else {
            strcpy(tmp, "Unable to create file '");
            strcat(tmp, gHotlistFile);
            strcat(tmp, "'.\n");
            MessageBox(NULL, tmp, PLUGIN_NAME, MB_ICONEXCLAMATION | MB_OK);
         }
      }
   }
   if (bmFile)
      fclose(bmFile);
   
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, gHotlistFile);
}

int Init(){
   HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
   nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
   nHRes = GetDeviceCaps(hdcScreen, HORZRES);

   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddLinkCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nUpdateTB = kPlugin.kFuncs->GetCommandIDs(1);
   
   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, &bRebarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_HOTLIST_RESYNCH, &bResynchHotlist, &bResynchHotlist);
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = 20;
   gMenuSortOrder = 209;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &gMenuSortOrder, &gMenuSortOrder);
   nButtonMinWidth = 10;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   nButtonMaxWidth = 35;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &nButtonMaxWidth);
   bButtonIcons = true;
   kPlugin.kFuncs->GetPreference(PREF_BOOL,  PREFERENCE_BUTTON_ICONS, &bButtonIcons, &bButtonIcons);
   
   getHotlistFile();
   bEmpty = true;
   
   gImagelist = ImageList_Create(16, 15, ILC_MASK, 4, 4);
   HBITMAP bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
   if (gImagelist && bitmap)
      ImageList_AddMasked(gImagelist, bitmap, RGB(192, 192, 192));
   if (bitmap)
      DeleteObject(bitmap);
   
   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (void *) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   pNewTB = create_TB(parent);
}

// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   
   switch (uMsg) {
      case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, bRebarEnabled, 0);
         break;
      case WM_COMMAND:
         switch(HIWORD(wParam)) {
            case BN_CLICKED:
               switch (LOWORD(wParam)) {
                  case IDOK:
                  {
                     int bTmpRebarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bTmpRebarEnabled);
                  }
                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
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

void Quit(){
   if (gImagelist)
      ImageList_Destroy(gImagelist);
   while (root)
      remove_TB(root->hWnd);
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
      if (stricmp(action, "Add") == 0){
         command = nAddCommand;
      }
      if (stricmp(action, "AddLink") == 0){
         command = nAddLinkCommand;
      }
      if (stricmp(param, "Config") == 0)
         command = nConfigCommand;
      if (command && string && *string) {
         AppendMenu(menu, MF_STRING, command, string);
         return;
      }
      return;
   }
   
   gMenuHotlist = menu;
   if (gMenuHotlist)
      nFirstHotlistPosition = GetMenuItemCount(gMenuHotlist);
   else
      return;
   
   int ret = -1;
   lpszHotlistFile = strdup(gHotlistFile);
   if (lpszHotlistFile && *lpszHotlistFile) {
      if ((ret = op_readFile(lpszHotlistFile)) > 0) {
         if (gMenuSortOrder)
            gHotlistRoot.sort(gMenuSortOrder);
         BuildMenu(menu, &gHotlistRoot, false);
      }
   }
   
   if (ret < 0) {
      if (!lpszHotlistFile || !*lpszHotlistFile)
         MessageBox(NULL, 
                    "No hotlist specified.",
                    "Hotlist Error", MB_ICONSTOP|MB_OK);
      else {
         if (!bCreate || errno != ENOENT) {
            char tmp[2048];
            strcpy(tmp, "Unable to open hotlist file\r\n'");
            strcat(tmp, lpszHotlistFile);
            strcat(tmp, "'.");
            if (errno == ENOENT) {
               strcat(tmp, "\r\nFile not found.");
            }
            MessageBox(NULL, tmp, "Hotlist Error", MB_ICONSTOP|MB_OK);
         }
      }
   }
}

int DoAccel(char *param) {
   if (stricmp(param, "Config") == 0)
      return nConfigCommand;
   return 0;
}

void DoRebar(HWND rebarWnd) {
   
   if (bRebarEnabled) {
      
      DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
         CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_ADJUSTABLE | TBSTYLE_ALTDRAG |
         TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;
      
      // Create the toolbar control to be added.
      HWND hWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
                               WS_CHILD | dwStyle,
                               0,0,0,0,
                               rebarWnd, (HMENU)/*id*/200,
                               kPlugin.hDllInstance, NULL
                               );
      
      if (!hWndTB){
         MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
         return;
      }
      
      wpOrigTBWndProc = (WNDPROC) SetWindowLong(hWndTB, 
         GWL_WNDPROC, (LONG) WndTBSubclassProc);

      if (pNewTB) {
         pNewTB->hWndTB = hWndTB;
         pNewTB = NULL;
      }

      // Register the band name and child hwnd
      kPlugin.kFuncs->RegisterBand(hWndTB, TOOLBAND_NAME);
      
      BuildRebar(hWndTB);

      // Get the height of the toolbar.
      DWORD dwBtnSize = SendMessage(hWndTB, TB_GETBUTTONSIZE, 0,0);
      
      REBARBANDINFO rbBand = {0};
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = //RBBIM_TEXT |
         RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;
      
      rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
      rbBand.lpText     = PLUGIN_NAME;
      rbBand.hwndChild  = hWndTB;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cxIdeal    = 100;
      rbBand.cx         = rbBand.cxIdeal;
      
      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   }
}

extern "C" {
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }
   
   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
      if (dis==NULL || gImagelist==NULL) return 0;
      int top = (dis->rcItem.bottom - dis->rcItem.top - 16) / 2;
      top += dis->rcItem.top;
      
      if (GetMenuState((HMENU)dis->hwndItem, dis->itemID, 0) & MF_POPUP){
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(gImagelist, IMAGE_FOLDER_OPEN, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS );
         }
         else{
            ImageList_Draw(gImagelist, IMAGE_FOLDER_CLOSED, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }
         return 18;
      }
      return 0;
   }
}
