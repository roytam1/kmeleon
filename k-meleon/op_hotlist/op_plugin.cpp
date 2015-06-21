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
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"
#include "../LocalesUtils.h"
Locale *gLoc = NULL;

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

int Load();
void Create(HWND parent);
void Config(HWND parent);
void Destroy(HWND hWnd);
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

void Setup()
{
  if (!bIgnore) {
     HBITMAP bitmap;
     int ilc_bits = ILC_COLOR;
     
     wchar_t szFullPath[MAX_PATH];
     kPlugin.kFuncs->FindSkinFile(L"hotlist.bmp", szFullPath, MAX_PATH);
     FILE *fp = _wfopen(szFullPath, L"r");
     if (fp) {
        fclose(fp);
        bitmap = (HBITMAP)LoadImageW(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
     } else {
        bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
     }
     
     BITMAP bmp;
     GetObject(bitmap, sizeof(BITMAP), &bmp);

     ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
     gImagelist = ImageList_Create(bmp.bmWidth/6, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
     if (gImagelist && bitmap)
        ImageList_AddMasked(gImagelist, bitmap, RGB(255, 0, 255));
     if (bitmap)
        DeleteObject(bitmap);
   }
}

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (strcmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (strcmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (strcmp(subject, "Destroy") == 0) {
         Destroy((HWND)data1);
      }
      else if (strcmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (strcmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (strcmp(subject, "DoAccel") == 0) {
         *(int *)data2 = DoAccel((char *)data1);
      }
      else if (strcmp(subject, "AddLink") == 0) {
         addLink((char *)data1, (char *)data2);
      }
      else if (strcmp(subject, "FindNick") == 0) {
         if (data1 && data2)
            findNick((char *)data1, (char **)data2);
      }
	  else if (strcmp(subject, "DoLocale") == 0) {
         if (gLoc) delete gLoc;
		 gLoc = Locale::kmInit(&kPlugin);
	  }
	  else if (strcmp(subject, "Setup") == 0) {
         Setup();
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

// HotlistFile Dialog function
BOOL CALLBACK HotlistFileDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   static int ret;
   
   switch (uMsg) {
      case WM_INITDIALOG:
      {
         ret = 0;

         HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
         int nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
         int nVSize = GetDeviceCaps(hdcScreen, VERTSIZE);
		 DeleteDC(hdcScreen);

         WINDOWPLACEMENT wp;
         wp.length = sizeof (WINDOWPLACEMENT);
         GetWindowPlacement(hWnd, &wp);

         int width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
         int height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

         wp.showCmd = SW_RESTORE;
         wp.rcNormalPosition.left = nHSize;
         wp.rcNormalPosition.top = nVSize;
         wp.rcNormalPosition.right = nHSize + width;
         wp.rcNormalPosition.bottom = nVSize + height;
         SetWindowPlacement(hWnd, &wp);

         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, bRebarEnabled, 0);
         break;
      }
      case WM_COMMAND:
         switch(HIWORD(wParam)) {
            case BN_CLICKED:
               switch (LOWORD(wParam)) {
                  case ID_CREATE:
                     ret = ID_CREATE;
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
                     break;
                  case ID_SEARCH:
                     ret = ID_SEARCH;
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
                     break;
                  case IDCANCEL:
                     ret = IDCANCEL;
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
                     break;
               }
         }
         break;
      case WM_CLOSE:
         EndDialog(hWnd, ret);
         break;
      default:
         return FALSE;
   }
   return TRUE;
}

void getHotlistFile() {
   FILE *bmFile;
   bool defFile =  false;
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, gHotlistFile, (char*)"");
   
   if (!gHotlistFile[0]) {
      kPlugin.kFuncs->GetFolder(UserSettingsFolder, gHotlistFile, MAX_PATH);
      strcat(gHotlistFile, "\\" HOTLIST_DEFAULT_FILENAME);
      defFile = true;
   }
   
   bmFile = fopen(gHotlistFile, "a");
   while (!bmFile && !bIgnore) {
   retry:
      kPlugin.kFuncs->GetFolder(UserSettingsFolder, gHotlistFile, MAX_PATH);
      strcat(gHotlistFile, "\\" HOTLIST_DEFAULT_FILENAME);
      
      while (!bmFile && !bIgnore) {
         int ret = gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_INSTALL), NULL, (DLGPROC)HotlistFileDlgProc, 0);
         if (ret == IDCANCEL || ret == ID_CREATE) {
            bIgnore = (ret == IDCANCEL);
            break;
         }
         if (!BrowseForHotlist(gHotlistFile)) {
            goto retry;
         }
         bmFile = fopen(gHotlistFile, "r");
         if (!bmFile && errno == ENOENT) {
            if (MessageBox(NULL, gLoc->GetStringFormat(IDS_FILENOTFOUND, gHotlistFile), PLUGIN_NAME, 
                           MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
               break;
         }
      }
      
      if (!bmFile && !bIgnore) {
         bmFile = fopen(gHotlistFile, "wb");
         if (bmFile) {
            fprintf(bmFile, 
                    "Opera Hotlist version 2.0\r\n"
                    "Options:encoding=utf8,version=3\r\n\r\n");
            bDOS = 1;
            MessageBox(NULL, gLoc->GetStringFormat(IDS_FILECREATED, gHotlistFile), PLUGIN_NAME, MB_ICONINFORMATION | MB_OK);
         }
         else {
            MessageBox(NULL, gLoc->GetStringFormat(IDS_CANTCREATEFILE, gHotlistFile), PLUGIN_NAME, MB_ICONERROR | MB_OK);
         }
      }
   }
   if (bmFile)
      fclose(bmFile);
   
   if (!bIgnore && !defFile)
      kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, gHotlistFile, false);
}

int Load(){
   gLoc = Locale::kmInit(&kPlugin);
   HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
   nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
   nHRes = GetDeviceCaps(hdcScreen, HORZRES);
   DeleteDC(hdcScreen);

   wm_deferhottrack = kPlugin.kFuncs->GetCommandIDs(1);
   wm_deferbringtotop = kPlugin.kFuncs->GetCommandIDs(1);

   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddLinkCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nUpdateTB = kPlugin.kFuncs->GetCommandIDs(1);
   
   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &bRebarEnabled, &bRebarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CHEVRON_ENABLED, &bChevronEnabled, &bChevronEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_HOTLIST_RESYNCH, &bResynchHotlist, &bResynchHotlist);
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_MENU_FOLDER, gMenuFolder, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_NEWITEM_FOLDER, gNewitemFolder, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = 20;
   gMenuSortOrder = 3*8*8 + 2*8 + 1;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &gMenuSortOrder, &gMenuSortOrder);
   nButtonMinWidth = 10;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   nButtonMaxWidth = 35;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &nButtonMaxWidth);
   bButtonIcons = true;
   kPlugin.kFuncs->GetPreference(PREF_BOOL,  PREFERENCE_BUTTON_ICONS, &bButtonIcons, &bButtonIcons);

   ghMutex = CreateMutex(NULL, FALSE, "HotlistFileMutex");
   
   getHotlistFile();
   bEmpty = true;

	int ret = op_readFile(gHotlistFile);
	lpszHotlistFile = strdup(gHotlistFile);
   if (ret < 0) {
      if (!gHotlistFile || !*gHotlistFile)
         MessageBox(NULL, 
                    "No hotlist specified.",
                    "Hotlist Error", MB_ICONSTOP|MB_OK);
      else {
         if (!bCreate || errno != ENOENT) {
            char tmp[2048];
            strcpy(tmp, "Unable to open hotlist file\r\n'");
            strcat(tmp, gHotlistFile);
            strcat(tmp, "'.");
            if (errno == ENOENT) {
               strcat(tmp, "\r\nFile not found.");
            }
            MessageBox(NULL, tmp, "Hotlist Error", MB_ICONSTOP|MB_OK);
         }
      }
	}
	

   
   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (void *) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   if (!bIgnore)
      pNewTB = create_TB(parent);
}

// Preferences Dialog function
BOOL CALLBACK PrefDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   int nTmp;
   char szTmp[MAX_PATH+1];
   static int sorting;
   static int rebuild;
   
   switch (uMsg) {
      case WM_INITDIALOG:
         sorting = rebuild = 0;

         kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, szTmp, gHotlistFile);
         SetDlgItemText(hWnd, IDC_HOTLIST_FILE, szTmp);

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &nTmp, &gMaxMenuLength);
         if (nTmp < 1) nTmp = 20;
         SetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, nTmp, TRUE);

         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &nTmp, &gMenuAutoDetect);
         SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_SETCHECK, nTmp, 0);
         if (nTmp) {
            EnableWindow(GetDlgItem(hWnd, IDC_MAX_MENU_LENGTH), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &nTmp, &gMenuSortOrder);
         if (nTmp==3 || nTmp==7 || nTmp==3*8+2 || nTmp==7*8+2 || 
             nTmp==3*8+1 || nTmp==7*8+1 || 
             nTmp==3*8*8+2*8+1 || nTmp==7*8*8+2*8+1) {
            while (nTmp > 8) {
               if ((nTmp % 8) == 1)
                  SendDlgItemMessage(hWnd, IDC_FOLDERFIRST, BM_SETCHECK, 1, 0);
               if ((nTmp % 8) == 2)
                  SendDlgItemMessage(hWnd, IDC_USEORDER, BM_SETCHECK, 1, 0);
               nTmp = nTmp / 8;
            }
            CheckRadioButton(hWnd, IDC_SORTORDER_AZ, IDC_SORTORDER_ZA, 
                             nTmp != 7 ? IDC_SORTORDER_AZ : IDC_SORTORDER_ZA);
            sorting = 1;
         } else {
            sorting = 0;
            EnableWindow(GetDlgItem(hWnd, IDC_SORTORDER_AZ), false);
            EnableWindow(GetDlgItem(hWnd, IDC_SORTORDER_ZA), false);
            EnableWindow(GetDlgItem(hWnd, IDC_FOLDERFIRST), false);
            EnableWindow(GetDlgItem(hWnd, IDC_USEORDER), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &nTmp, &bRebarEnabled);
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, nTmp, 0);
         if (!nTmp) {
            EnableWindow(GetDlgItem(hWnd, IDC_MIN_TB_SIZE), false);
            EnableWindow(GetDlgItem(hWnd, IDC_MAX_TB_SIZE), false);
         }

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_MINWIDTH, &nTmp, &nButtonMinWidth);
         SetDlgItemInt(hWnd, IDC_MIN_TB_SIZE, nButtonMinWidth, TRUE);

         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_BUTTON_MAXWIDTH, &nTmp, &nButtonMaxWidth);
         SetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, nButtonMaxWidth, TRUE);
         
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
                     HWND hBookmarksFileWnd = GetDlgItem(hWnd, IDC_HOTLIST_FILE);
                     GetWindowText(hBookmarksFileWnd, szTmp, MAX_PATH);
                     nTmp = strlen(szTmp);
                     while (--nTmp >= 0) {
                        if (szTmp[nTmp] == '/')
                           szTmp[nTmp] = '\\';
                     }
                     if (BrowseForHotlist(szTmp)) {
                        SetWindowText(hBookmarksFileWnd, szTmp);
                        SetFocus(hBookmarksFileWnd);
                     }
                  }
                  break;
                  case IDOK:
                  {
                     GetDlgItemText(hWnd, IDC_HOTLIST_FILE, szTmp, MAX_PATH);
                     if (strcmp(gHotlistFile, szTmp) != 0) {
                        if (gHotlistModified) {
                           op_writeFile(lpszHotlistFile);
                        }
                        free(lpszHotlistFile);
                        lpszHotlistFile = strdup(szTmp);
                        if (lpszHotlistFile && *lpszHotlistFile) {
                           delete gHotlistRoot.child;
                           delete gHotlistRoot.next;
                           gHotlistRoot.child = NULL;
                           gHotlistRoot.next = NULL;
                           if (op_readFile(lpszHotlistFile)) {
                              strcpy(gHotlistFile, szTmp);
                              kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_HOTLIST_FILE, szTmp, FALSE);
                           } else {
                              free(lpszHotlistFile);
                              lpszHotlistFile = strdup(gHotlistFile);
                              if (lpszHotlistFile && *lpszHotlistFile) {
                                 delete gHotlistRoot.child;
                                 delete gHotlistRoot.next;
                                 gHotlistRoot.child = NULL;
                                 gHotlistRoot.next = NULL;
                                 op_readFile(lpszHotlistFile);
                              }
                           }
                        }
                        rebuild = 1;
                     }

                     nTmp = GetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, NULL, TRUE);
                     if (nTmp < 1) nTmp = 20;
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &nTmp, FALSE);
                     if (nTmp != gMaxMenuLength) {
                        gMaxMenuLength = nTmp;
                        rebuild = 1;
                     }

                     nTmp = SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &nTmp, FALSE);
                     if (nTmp != gMenuAutoDetect) {
                        gMenuAutoDetect = nTmp;
                        rebuild = 1;
                     }

                     if (sorting) {
                        sorting = 0;

                        nTmp = IsDlgButtonChecked(hWnd, IDC_SORTORDER_AZ);
                        sorting = sorting + 3*(nTmp==BST_CHECKED);

                        nTmp = IsDlgButtonChecked(hWnd, IDC_SORTORDER_ZA);
                        sorting = sorting + 7*(nTmp==BST_CHECKED);

                        nTmp = IsDlgButtonChecked(hWnd, IDC_USEORDER);
                        if (nTmp == BST_CHECKED)
                           sorting = 8*sorting + 2;

                        nTmp = IsDlgButtonChecked(hWnd, IDC_FOLDERFIRST);
                        if (nTmp == BST_CHECKED)
                           sorting = 8*sorting + 1;

                        kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &sorting, FALSE);
                        gMenuSortOrder = sorting;
                        rebuild = 1;
                     }

                     nTmp = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                     kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_REBAR_ENABLED, &nTmp, FALSE);

                     nTmp = GetDlgItemInt(hWnd, IDC_MIN_TB_SIZE, NULL, TRUE);
                     if (nTmp < 1) nTmp = 0;
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_BUTTON_MINWIDTH, &nTmp, FALSE);

                     nTmp = GetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, NULL, TRUE);
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_BUTTON_MAXWIDTH, &nTmp, FALSE);

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
   gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)PrefDlgProc, 0);
}

void Quit(){
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)gMenuHotlist, 0);
   hWndFront = NULL;
   if (ghWndEdit)
      SendMessage(ghWndEdit, WM_CLOSE, 0, 0);
   if (gImagelist)
      ImageList_Destroy(gImagelist);
   if (gHotlistModified) {
      op_writeFile(lpszHotlistFile);
   }
   if (lpszHotlistFile) free(lpszHotlistFile);

   while (root)
      remove_TB(root->hWnd);

   TCHAR tmp[MAX_PATH];

   kPlugin.kFuncs->GetFolder(UserSettingsFolder, tmp, MAX_PATH);
   strcat(tmp, HOTLIST_DEFAULT_FILENAME);
   if (_tcscmp(tmp, gHotlistFile) == 0)
	   kPlugin.kFuncs->DelPreference(PREFERENCE_HOTLIST_FILE);
   delete gLoc;
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
         kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "SetOwnerDrawn", (long)menu, (long)DrawBitmap);
         return;
      }
      return;
   }

   if (bIgnore)
      return;

   gMenuHotlist = menu;
   if (gMenuHotlist)
      nFirstHotlistPosition = GetMenuItemCount(gMenuHotlist);
   else
      return;
   
   if (gMenuSortOrder)
	   gHotlistRoot.sort(gMenuSortOrder);
   BuildMenu(menu, gHotlistRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);
   
}

int DoAccel(char *param) {
   if (stricmp(param, "Config") == 0)
      return nConfigCommand;
   if (stricmp(param, "Add") == 0)
      return nAddCommand;
   if (stricmp(param, "Edit") == 0)
      return nEditCommand;
	if (stricmp(param, "AddLink") == 0)
      return nAddLinkCommand;
   return 0;
}

void DoRebar(HWND rebarWnd) {
   if (bIgnore)
      return;
   
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
      
      if (nButtonMinWidth > 0)
         wpOrigTBWndProc = (WNDPROC) SetWindowLong(hWndTB, 
            GWL_WNDPROC, (LONG) WndTBSubclassProc);

      pNewTB = NULL;

      // Register the band name and child hwnd
      kPlugin.kFuncs->RegisterBand(hWndTB, TOOLBAND_NAME, true);
      
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
// FIXME - This is probably way too slow to be useful.
      CBookmarkNode* node = gHotlistRoot.FindNode(LOWORD(dis->itemID));
	  if (node) {

         UINT flags = ILD_NORMAL;
		 if (dis->itemState & ODS_SELECTED)
			flags |= ILD_FOCUS;

         HIMAGELIST hList = gImagelist;
         UINT idx = IMAGE_BOOKMARK;

		 UINT i = GetSiteIcon((char*)node->url.c_str());
		 if (i > 0) {
            idx = i;
            hList = kPlugin.kFuncs->GetDefSizeIconList();
		 }

         ImageList_Draw(hList, idx, dis->hDC, dis->rcItem.left+2, top, flags);

         return 18;
      }

      return 0;
   }
}
