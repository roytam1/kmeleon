/*
 * Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
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

#define WHERE
#include "ie_favorites.h"

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
   PLUGIN_NAME,
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
      else return 0;

      return 1;
   }
   return 0;
}


// look for filename first in the skinsDir, then in the settingsDir
// check for the filename in skinsDir, and copy the path into szSkinFile
// if it's not there, just assume it's in settingsDir, and copy that path

void FindSkinFile( char *szSkinFile, char *filename ) {

   char szTmpSkinDir[MAX_PATH];
   char szTmpSkinName[MAX_PATH];
   char szTmpSkinFile[MAX_PATH] = "";

   if (!szSkinFile || !filename || !*filename)
      return;

   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsDir", szTmpSkinDir, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsCurrent", szTmpSkinName, (char*)"");

   if (*szTmpSkinDir && *szTmpSkinName) {
      strcpy(szTmpSkinFile, szTmpSkinDir);

      int len = strlen(szTmpSkinFile);
      if (szTmpSkinFile[len-1] != '\\')
         strcat(szTmpSkinFile, "\\");

      strcat(szTmpSkinFile, szTmpSkinName);
      len = strlen(szTmpSkinFile);
      if (szTmpSkinFile[len-1] != '\\')
         strcat(szTmpSkinFile, "\\");

      strcat(szTmpSkinFile, filename);

      WIN32_FIND_DATA FindData;

      HANDLE hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }

   // it wasn't in the skinsDir, assume settingsDir
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szSkinFile, (char*)"");
   if (! *szSkinFile)      // no settingsDir, bad
      strcpy(szSkinFile, filename);
   else
      strcat(szSkinFile, filename);
}


int Init(){
   HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
   nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
   nHRes = GetDeviceCaps(hdcScreen, HORZRES);

   wm_deferhottrack = kPlugin.kFuncs->GetCommandIDs(1);

   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddLinkCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nUpdateTB = kPlugin.kFuncs->GetCommandIDs(1);
   
   nFirstFavoriteCommand = kPlugin.kFuncs->GetCommandIDs(1);

   GetFavoritesPath();

   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, &bRebarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CHEVRON_ENABLED, &bChevronEnabled, &bChevronEnabled);
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, (char*)TOOLBAND_FOLDER);
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_MENU_FOLDER, gMenuFolder, (char*)MENU_FOLDER);
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_NEWITEM_FOLDER, gNewitemFolder, (char*)NEWITEM_FOLDER);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &gMenuAutoDetect, &gMenuAutoDetect);
   gMaxMenuLength = MAX_MENU_LENGTH;
   if (gMaxMenuLength < 1) gMaxMenuLength = MAX_MENU_LENGTH;
   gMenuSortOrder = 17;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &gMenuSortOrder, &gMenuSortOrder);
   nButtonMinWidth = 10;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   nButtonMaxWidth = -100;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &nButtonMaxWidth);
   bButtonIcons = true;
   kPlugin.kFuncs->GetPreference(PREF_BOOL,  PREFERENCE_BUTTON_ICONS, &bButtonIcons, &bButtonIcons);
   strcpy(szTitle, "");
   bTitleSet = true;
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_REBAR_TITLE, &szTitle, &szTitle);
   if (szTitle[0] == 0) {
      strcpy(szTitle, TOOLBAND_TITLE);
      kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_REBAR_TITLE, &szTitle, &szTitle);
      if (strcmp(szTitle, TOOLBAND_TITLE) == 0)
         bTitleSet = false;
   }

   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;

   char szFullPath[MAX_PATH];
   FindSkinFile(szFullPath, "favorites.bmp");
   FILE *fp = fopen(szFullPath, "r");
   if (fp) {
      fclose(fp);
      bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   } else {
      bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
      ilc_bits = ILC_COLOR24;
   }

   BITMAP bmp;
   GetObject(bitmap, sizeof(BITMAP), &bmp);

   ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
   gImagelist = ImageList_Create(bmp.bmWidth/6, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
   if (gImagelist && bitmap)
      ImageList_AddMasked(gImagelist, bitmap, RGB(255, 0, 255));
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
BOOL CALLBACK PrefDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   int nTmp;
   char szTmp[INTERNET_MAX_URL_LENGTH+1];
   static int sorting;
   static int rebuild;
   
   switch (uMsg) {
      case WM_INITDIALOG:
         sorting = rebuild = 0;

         kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_FAVORITES_PATH, szTmp, gFavoritesPath);
         SetDlgItemText(hWnd, IDC_FAVORITES_PATH, szTmp);

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &nTmp, &gMaxMenuLength);
         if (nTmp < 1) nTmp = MAX_MENU_LENGTH;
         SetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, nTmp, TRUE);

         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &nTmp, &gMenuAutoDetect);
         SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_SETCHECK, nTmp, 0);
         if (nTmp) {
            EnableWindow(GetDlgItem(hWnd, IDC_MAX_MENU_LENGTH), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &nTmp, &gMenuSortOrder);
         if (nTmp==2 || nTmp==6 || nTmp==2*8+1 || nTmp==6*8+1) {
            if (nTmp > 8) {
               SendDlgItemMessage(hWnd, IDC_FOLDERFIRST, BM_SETCHECK, nTmp%2, 0);
               nTmp = nTmp / 8;
            }
            CheckRadioButton(hWnd, IDC_SORTORDER_AZ, IDC_SORTORDER_ZA, 
                             nTmp != 6 ? IDC_SORTORDER_AZ : IDC_SORTORDER_ZA);
            sorting = 1;
         } else {
            sorting = 0;
            EnableWindow(GetDlgItem(hWnd, IDC_SORTORDER_AZ), false);
            EnableWindow(GetDlgItem(hWnd, IDC_SORTORDER_ZA), false);
            EnableWindow(GetDlgItem(hWnd, IDC_FOLDERFIRST), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &nTmp, &bRebarEnabled);
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, nTmp, 0);
         if (!nTmp) {
            EnableWindow(GetDlgItem(hWnd, IDC_MIN_TB_SIZE), false);
            EnableWindow(GetDlgItem(hWnd, IDC_MAX_TB_SIZE), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_MINWIDTH, &nTmp, &nButtonMinWidth);
         SetDlgItemInt(hWnd, IDC_MIN_TB_SIZE, nTmp, TRUE);

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_MAXWIDTH, &nTmp, &nButtonMaxWidth);
         SetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, nTmp, TRUE);
         
         break;
      case WM_COMMAND:
         switch(HIWORD(wParam)) {
            case BN_CLICKED:
               switch (LOWORD(wParam)) {
                  case IDC_MENU_AUTODETECT:
                     if (SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_MAX_MENU_LENGTH), false);
                     }
                     else {
                        EnableWindow(GetDlgItem(hWnd, IDC_MAX_MENU_LENGTH), true);
                     }
                     break;
                  case IDC_REBARENABLED:
                     if (SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0)) {
                        EnableWindow(GetDlgItem(hWnd, IDC_MIN_TB_SIZE), true);
                        EnableWindow(GetDlgItem(hWnd, IDC_MAX_TB_SIZE), true);
                     }
                     else {
                        EnableWindow(GetDlgItem(hWnd, IDC_MIN_TB_SIZE), false);
                        EnableWindow(GetDlgItem(hWnd, IDC_MAX_TB_SIZE), false);
                     }
                     break;
                  case IDC_BROWSE:
                  {
                     HWND hBookmarksFileWnd = GetDlgItem(hWnd, IDC_FAVORITES_PATH);
                     GetWindowText(hBookmarksFileWnd, szTmp, MAX_PATH);
                     nTmp = strlen(szTmp);
                     while (--nTmp >= 0) {
                        if (szTmp[nTmp] == '/')
                           szTmp[nTmp] = '\\';
                     }
                     int len = strlen(szTmp);
                     if (szTmp[len-1] != '\\')
                        strcat(szTmp, "\\");

                     delete gFavoritesRoot.child;
                     delete gFavoritesRoot.next;
                     gFavoritesRoot.child = NULL;
                     gFavoritesRoot.next = NULL;

                     if (ReadFavorites(szTmp, "", gFavoritesRoot) > 0) {
                        if (gMenuSortOrder) {
                           strcpy(gFavoritesPath, szTmp);
                           gFavoritesRoot.sort(gMenuSortOrder);
                        }
                     } 
                     else {
                        strcpy(szTmp, gFavoritesPath);
                        ReadFavorites(gFavoritesPath, "", gFavoritesRoot);
                     }
                     RebuildMenu();
                     
                     SetWindowText(hBookmarksFileWnd, szTmp);
                     SetFocus(hBookmarksFileWnd);
                  }
                  break;
                  case IDOK:
                  {
                     GetDlgItemText(hWnd, IDC_FAVORITES_PATH, szTmp, INTERNET_MAX_URL_LENGTH);
                     kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_FAVORITES_PATH, szTmp);

                     nTmp = GetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, NULL, TRUE);
                     if (nTmp < 1) nTmp = 20;
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &nTmp);
                     if (nTmp != gMaxMenuLength) {
                        gMaxMenuLength = nTmp;
                        rebuild = 1;
                     }

                     nTmp = SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &nTmp);
                     if (nTmp != gMenuAutoDetect) {
                        gMenuAutoDetect = nTmp;
                        rebuild = 1;
                     }

                     if (sorting) {
                        sorting = 0;

                        nTmp = IsDlgButtonChecked(hWnd, IDC_SORTORDER_AZ);
                        sorting = sorting + 2*(nTmp==BST_CHECKED);

                        nTmp = IsDlgButtonChecked(hWnd, IDC_SORTORDER_ZA);
                        sorting = sorting + 6*(nTmp==BST_CHECKED);

                        nTmp = IsDlgButtonChecked(hWnd, IDC_FOLDERFIRST);
                        if (nTmp == BST_CHECKED)
                           sorting = 8*sorting + 1;

                        kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &sorting);
                        gMenuSortOrder = sorting;
                        rebuild = 1;
                     }

                     nTmp = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &nTmp);

                     nTmp = GetDlgItemInt(hWnd, IDC_MIN_TB_SIZE, NULL, TRUE);
                     if (nTmp < 1) nTmp = 0;
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_BUTTON_MINWIDTH, &nTmp);

                     nTmp = GetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, NULL, TRUE);
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_BUTTON_MAXWIDTH, &nTmp);

                     if (rebuild)
                        RebuildMenu();

                     // fall through...
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
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)PrefDlgProc, 0);
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
      if (stricmp(param, "Edit") == 0)
         command = nEditCommand;
      if (command && string && *string && (!bIgnore || command == nConfigCommand)) {
         AppendMenu(menu, MF_STRING, command, string);
         return;
      }
      return;
   }

   gMenuFavorites = menu;
   if (gMenuFavorites && gFavoritesPath && *gFavoritesPath)
      nFirstFavoritesPosition = GetMenuItemCount(gMenuFavorites);
   else
      return;
   
   int ret = -1;
   if ((ret = ReadFavorites(gFavoritesPath, "", gFavoritesRoot)) > 0) {
     if (gMenuSortOrder) {
       gFavoritesRoot.sort(gMenuSortOrder);
     }
     BuildMenu(menu, gFavoritesRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);
   }
}

int DoAccel(char *param) {
   if (stricmp(param, "Config") == 0)
      return nConfigCommand;
   if (stricmp(param, "Add") == 0)
      return nAddCommand;
   if (stricmp(param, "Edit") == 0)
      return nEditCommand;
   return 0;
}

void DoRebar(HWND rebarWnd){
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

      SetWindowText(hWndTB, TOOLBAND_NAME);
   
      //SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
   
      if (pNewTB) {
         pNewTB->hWndTB = hWndTB;
         if (gImagelist && bButtonIcons)
            SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
         else
            SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);
      }
      
      BuildRebar(hWndTB);

      // Get the height of the toolbar.
      DWORD dwBtnSize = SendMessage(hWndTB, TB_GETBUTTONSIZE, 0,0);
      
      REBARBANDINFO rbBand = {0};
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = RBBIM_TEXT |
         RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;
      
      rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
      rbBand.lpText     = szTitle;
      rbBand.hwndChild  = hWndTB;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cxIdeal    = 100;
      rbBand.cx         = rbBand.cxIdeal;
      
      if (nButtonMinWidth > 0)
         wpOrigTBWndProc = (WNDPROC) SetWindowLong(hWndTB, 
            GWL_WNDPROC, (LONG) WndTBSubclassProc);

      pNewTB = NULL;

      // Register the band name and child hwnd
      if (bTitleSet && szTitle[0] != 0) {
         int len = strlen(szTitle);
         char c = szTitle[len-1];
         if (c == ':')
            szTitle[len-1] = 0;
         kPlugin.kFuncs->RegisterBand(hWndTB, szTitle);
         if (c == ':')
            strcat(szTitle, ":");
      }
      else {
         kPlugin.kFuncs->RegisterBand(hWndTB, TOOLBAND_NAME);
      }
      
      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   }
}


extern "C" {
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }
}
