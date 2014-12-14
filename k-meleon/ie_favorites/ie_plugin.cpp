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

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

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
Locale* gLoc;

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
   KMEL_PLUGIN_VER_UTF8,
   PLUGIN_NAME,
   DoMessage
};

void Setup()
{
   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;

   wchar_t szFullPath[MAX_PATH];
   kPlugin.kFuncs->FindSkinFile(L"favorites.bmp", szFullPath, MAX_PATH);
   FILE *fp = _tfopen(szFullPath, _T("r"));
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
      else if (strcmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (strcmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
      else if (strcmp(subject, "AddLink") == 0) {
         addLink((char *)data1, (char *)data2);
      }
      else if (strcmp(subject, "FindNick") == 0) {
         findNick((char *)data1, (char **)data2);
      }
	  else if (strcmp(subject, "DoLocale") == 0) {
         if (gLoc) delete gLoc;
		 gLoc = Locale::kmInit(&kPlugin);
	  }
	  else if (stricmp(subject, "Setup") == 0) {
         Setup();
	  }
      else return 0;

      return 1;
   }
   return 0;
}

int Load(){
   gLoc = Locale::kmInit(&kPlugin);
   HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL); 
   nHSize = GetDeviceCaps(hdcScreen, HORZSIZE);
   nHRes = GetDeviceCaps(hdcScreen, HORZRES);
   DeleteDC(hdcScreen);

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

   char toolbandFolder[MAX_PATH] = {0};
   DWORD type, size = MAX_PATH;
   HKEY key;
   if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Internet Explorer\\Toolbar"), 0, KEY_READ, &key) == ERROR_SUCCESS) {
	  RegQueryValueEx(key, _T("LinksFolderName"), NULL, &type, (LPBYTE)toolbandFolder, &size);
      RegCloseKey(key);			
   }

   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, toolbandFolder);
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_MENU_FOLDER, gMenuFolder, MENU_FOLDER);
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_NEWITEM_FOLDER, gNewitemFolder, NEWITEM_FOLDER);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_MAXLEN, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTOLEN, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = MAX_MENU_LENGTH;
   gMenuSortOrder = 17;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MENU_SORTORDER, &gMenuSortOrder, &gMenuSortOrder);
   nButtonMinWidth = 10;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MINWIDTH, &nButtonMinWidth, &nButtonMinWidth);
   nButtonMaxWidth = -100;
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_BUTTON_MAXWIDTH, &nButtonMaxWidth, &nButtonMaxWidth);
   bButtonIcons = true;
   kPlugin.kFuncs->GetPreference(PREF_BOOL,  PREFERENCE_BUTTON_ICONS, &bButtonIcons, &bButtonIcons);
   _tcscpy(szTitle, _T(""));
   bTitleSet = true;
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_REBAR_TITLE, &szTitle, &szTitle);
   if (szTitle[0] == 0) {
      _tcscpy(szTitle, CUTF8_to_T(TOOLBAND_TITLE));
      kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_REBAR_TITLE, &szTitle, &szTitle);
      if (_tcscmp(szTitle, CUTF8_to_T(TOOLBAND_TITLE)) == 0)
         bTitleSet = false;
   }

	ReadFavorites(gFavoritesPath, _T(""), gFavoritesRoot);

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
   TCHAR szTmp[INTERNET_MAX_URL_LENGTH+1];
   static int sorting;
   static int rebuild;
   
   switch (uMsg) {
      case WM_INITDIALOG:
         sorting = rebuild = 0;

         kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_FAVORITES_PATH, szTmp, gFavoritesPath);
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
                     nTmp = _tcslen(szTmp);
                     while (--nTmp >= 0) {
                        if (szTmp[nTmp] == _T('/'))
                           szTmp[nTmp] = _T('\\');
                     }
                     int len = _tcslen(szTmp);
                     if (szTmp[len-1] != _T('\\'))
                        _tcscat(szTmp, _T("\\"));

                     delete gFavoritesRoot.child;
                     delete gFavoritesRoot.next;
                     gFavoritesRoot.child = NULL;
                     gFavoritesRoot.next = NULL;

                     if (ReadFavorites(szTmp, _T(""), gFavoritesRoot) > 0) {
                        if (gMenuSortOrder) {
                           _tcscpy(gFavoritesPath, szTmp);
                           gFavoritesRoot.sort(gMenuSortOrder);
                        }
                     } 
                     else {
                        _tcscpy(szTmp, gFavoritesPath);
                        ReadFavorites(gFavoritesPath, _T(""), gFavoritesRoot);
                     }
                     RebuildMenu();
                     
                     SetWindowText(hBookmarksFileWnd, szTmp);
                     SetFocus(hBookmarksFileWnd);
                  }
                  break;
                  case IDOK:
                  {
                     GetDlgItemText(hWnd, IDC_FAVORITES_PATH, szTmp, INTERNET_MAX_URL_LENGTH);
                     kPlugin.kFuncs->SetPreference(PREF_TSTRING, PREFERENCE_FAVORITES_PATH, szTmp, FALSE);

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
                        sorting = sorting + 2*(nTmp==BST_CHECKED);

                        nTmp = IsDlgButtonChecked(hWnd, IDC_SORTORDER_ZA);
                        sorting = sorting + 6*(nTmp==BST_CHECKED);

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
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)gMenuFavorites, 0);
   if (gImagelist)
      ImageList_Destroy(gImagelist);
   while (root)
      remove_TB(root->hWnd);
   delete gFavoritesRoot.child;
   delete gFavoritesRoot.next;
   gFavoritesRoot.child = NULL;
   gFavoritesRoot.next = NULL;
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
         AppendMenu(menu, MF_STRING, command, CUTF8_to_T(string));
         kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "SetOwnerDrawn", (long)menu, (long)DrawBitmap);
         return;
      }
      return;
   }

   gMenuFavorites = menu;
   if (gMenuFavorites && gFavoritesPath && *gFavoritesPath)
      nFirstFavoritesPosition = GetMenuItemCount(gMenuFavorites);
   else
      return;
   
   if (gMenuSortOrder)
      gFavoritesRoot.sort(gMenuSortOrder);
   BuildMenu(menu, gFavoritesRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);
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

void DoRebar(HWND rebarWnd){
   if (bRebarEnabled) {
      
      DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
         CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_ADJUSTABLE | TBSTYLE_ALTDRAG |
         TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;
      
      // Create the toolbar control to be added.
      HWND hWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, _T(""),
                               WS_CHILD | dwStyle,
                               0,0,0,0,
                               rebarWnd, (HMENU)/*id*/200,
                               kPlugin.hDllInstance, NULL
                               );
      
      if (!hWndTB){
         MessageBox(NULL, gLoc->GetString(IDS_TOOLBAND_FAILED), NULL, 0);
         return;
      }

      SetWindowText(hWndTB, _T(TOOLBAND_NAME));
   
      //SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
   
      if (pNewTB) {
         pNewTB->hWndTB = hWndTB;
         if (gImagelist && bButtonIcons)
            SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
         else
            SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)NULL);
      }
      
      BuildRebar(hWndTB);

		// Compute the width needed for the toolbar
		int ideal = 0;
		int iCount, iButtonCount = SendMessage(hWndTB, TB_BUTTONCOUNT, 0,0);
		for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
		{	
		   RECT rectButton;
			SendMessage(hWndTB, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
			ideal += rectButton.right - rectButton.left;
		}

      // Get the height of the toolbar.
      DWORD dwBtnSize = SendMessage(hWndTB, TB_GETBUTTONSIZE, 0,0);
      
      REBARBANDINFO rbBand = {0};
      rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
      rbBand.fMask  = RBBIM_TEXT |
         RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
         RBBIM_SIZE | RBBIM_IDEALSIZE;
      
      rbBand.fStyle = RBBS_USECHEVRON | RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
      rbBand.lpText     = szTitle;
      rbBand.hwndChild  = hWndTB;
      rbBand.cxMinChild = 0;
      rbBand.cyMinChild = HIWORD(dwBtnSize);
      rbBand.cyIntegral = 1;
      rbBand.cyMaxChild = rbBand.cyMinChild;
      rbBand.cx         = rbBand.cxIdeal = ideal;
      
      if (nButtonMinWidth > 0)
         wpOrigTBWndProc = (WNDPROC) SetWindowLong(hWndTB, 
            GWL_WNDPROC, (LONG) WndTBSubclassProc);

      pNewTB = NULL;

      // Register the band name and child hwnd
      if (bTitleSet && szTitle[0] != 0) {
         int len = _tcslen(szTitle);
         TCHAR c = szTitle[len-1];
         if (c == ':')
            szTitle[len-1] = 0;
         kPlugin.kFuncs->RegisterBand(hWndTB, (char*)(const char*)CT_to_UTF8(szTitle), TRUE);
         if (c == ':')
            _tcscat(szTitle, _T(":"));
      }
      else {
         kPlugin.kFuncs->RegisterBand(hWndTB, TOOLBAND_NAME, TRUE);
      }
      
      // Add the band that has the toolbar.
      SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   }
}


extern "C" {
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }

   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
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
      CBookmarkNode* node = gFavoritesRoot.FindNode(LOWORD(dis->itemID));
	  if (node) {

         UINT flags = ILD_NORMAL;
		 if (dis->itemState & ODS_SELECTED)
			flags |= ILD_FOCUS;

         HIMAGELIST hList = gImagelist;
         UINT idx = IMAGE_BOOKMARK;
		
		 TCHAR *ptr = (TCHAR *)node->url.c_str();
         if (!node->url.length()) {
            TCHAR url[INTERNET_MAX_URL_LENGTH];
            TCHAR path[INTERNET_MAX_URL_LENGTH];
            _tcscpy(path, gFavoritesPath);
            _tcscat(path, node->path.c_str());
            GetPrivateProfileString(_T("InternetShortcut"), _T("URL"), _T(""), url, INTERNET_MAX_URL_LENGTH, path);
			node->url = url;
			GetPrivateProfileString(_T("InternetShortcut"), _T("IconFile"), _T(""), url, INTERNET_MAX_URL_LENGTH, path);
			node->iconurl = url;
		 }

		 UINT i = GetSiteIcon(node);

		 if (i > 0) {
            idx = i;
            hList = kPlugin.kFuncs->GetIconList();
		 }

         ImageList_Draw(hList, idx, dis->hDC, dis->rcItem.left+2, top, flags);

         return 18;
      }
      return 0;
   }
}
