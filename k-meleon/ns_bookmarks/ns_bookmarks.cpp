/*
*  Copyright (C) 2000 Brian Harris, Mark Liffiton
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
// ns_bookmarks.cpp : Plugin that supports netscape-style boomarks
//

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "stdafx.h"
#include "resource.h"

#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#include "../rebar_menu/hot_tracking.h"

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

// The globals

UINT nConfigCommand;
UINT nAddCommand;
UINT nAddLinkCommand;
UINT nAddToolbarCommand;
UINT nEditCommand;
UINT nDropdownCommand;
UINT nFirstBookmarkPosition;
UINT wm_deferbringtotop;

CHAR gBookmarkFile[MAX_PATH];
CHAR gToolbarFolder[MAX_PATH];

char gBookmarksTitle[BOOKMARKS_TITLE_LEN];

HIMAGELIST gImagelist; // the one and only imagelist...

HMENU gMenuBookmarks;
HWND ghWndTB;
HWND ghWndTBParent;
HWND hWndFront;
HWND ghWndEdit;
HANDLE ghMutex;

BOOL gBookmarksModified = false;
BOOL gGeneratedByUs = false;

BOOL gToolbarEnabled;
BOOL bChevronEnabled = true;

int gMaxMenuLength;
BOOL gMenuAutoDetect;
int gMaxTBSize;

WNDPROC KMeleonWndProc;

CBookmarkNode gBookmarkRoot(0, "", "", "", "", "", BOOKMARK_FOLDER, 0);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Close") == 0) {
         Close((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
          *(int *)data2 = DoAccel((char *)data1);
      }
      else if (stricmp(subject, "AddLink") == 0) {
         addLink((char *)data1, (char *)data2, BOOKMARK_FLAG_NB);
      }
      else if (stricmp(subject, "FindNick") == 0) {
         findNick((char *)data1, (char **)data2);
      }
      else return 0;

      return 1;
   }
   return 0;
}


#include "../findskin.cpp"


int Load(){
   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddToolbarCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddLinkCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);
   wm_deferbringtotop = kPlugin.kFuncs->GetCommandIDs(1);

   ghMutex = CreateMutex(NULL, FALSE, "BookmarksFileMutex");

   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, (char*)"");

   if (!gBookmarkFile[0]) {
      kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_SETTINGS_DIR, gBookmarkFile, (char*)"");
      strcat(gBookmarkFile, BOOKMARKS_DEFAULT_FILENAME);
   }

   FILE *bmFile = fopen(gBookmarkFile, "a");
   if (bmFile)
      fclose(bmFile);
   else {
      if (MessageBox(NULL, BOOKMARKS_NOT_FOUND,
         PLUGIN_NAME, MB_YESNO) == IDYES) {

         if (BrowseForBookmarks(gBookmarkFile)) {
            // if the file they chose doesn't exist, create it
            bmFile = fopen(gBookmarkFile, "r");
            if (!bmFile) {
               bmFile = fopen(gBookmarkFile, "w");
               gGeneratedByUs = true;
            }
            fclose(bmFile);
         }
         else {
            // this means they hit "Cancel" in the file open dialog
            MessageBox(NULL, BOOKMARKS_CREATING_NEW, PLUGIN_NAME, 0);
            bmFile = fopen(gBookmarkFile, "w");
            if (bmFile) {
               fclose(bmFile);
               gGeneratedByUs = true;
            }
         }
      }
      // create the default bookmark file
      else {
         bmFile = fopen(gBookmarkFile, "w");
         if (bmFile) {
            fclose(bmFile);
         }
         gGeneratedByUs = true;
      }
   }
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, FALSE);

   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled, &gToolbarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CHEVRON_ENABLED, &bChevronEnabled, &bChevronEnabled);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_MENU_LENGTH, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTODETECT, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = 20;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_TB_SIZE, &gMaxTBSize, &gMaxTBSize);
   if (gMaxTBSize < 1) gMaxTBSize = 20;

   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;
   COLORREF bgCol = RGB(255, 0, 255);

   char szFullPath[MAX_PATH];
   FindSkinFile(szFullPath, "bookmarks.bmp");
   FILE *fp = fopen(szFullPath, "r");
   if (fp) {
      fclose(fp);
      bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   } else {
      bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
      bgCol = RGB(192, 192, 192);
   }

   BITMAP bmp;
   GetObject(bitmap, sizeof(BITMAP), &bmp);

   ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
   gImagelist = ImageList_Create(bmp.bmWidth/6, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
   if (gImagelist && bitmap)
      ImageList_AddMasked(gImagelist, bitmap, bgCol);
   if (bitmap)
      DeleteObject(bitmap);

   strcpy(gBookmarksTitle, BOOKMARKS_DEFAULT_TITLE);

   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Close(HWND hWnd) {
  if (ghWndTB && ghWndTBParent==hWnd) {
    ghWndTB = NULL;
  }
}

void Quit(){
   hWndFront = NULL;
   if (ghWndEdit)
      SendMessage(ghWndEdit, WM_CLOSE, 0, 0);
   ImageList_Destroy(gImagelist);

   if (gBookmarksModified) {
      SaveBM(gBookmarkFile);
   }
}

void Config(HWND hWndParent){
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, (LPARAM)NULL);
}

// param format <action>, String
// example, add, &Add Bookmark
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
      if (stricmp(action, "Config") == 0){
         command = nConfigCommand;
      }
      if (stricmp(action, "Add") == 0){
         command = nAddCommand;
      }
      if (stricmp(action, "AddToolbar") == 0){
         command = nAddToolbarCommand;
      }
      if (stricmp(action, "AddLink") == 0){
         command = nAddLinkCommand;
      }
      if (stricmp(action, "Edit") == 0){
         command = nEditCommand;
      }
      if (command) {
         AppendMenu(menu, MF_STRING, command, string);
         kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "SetOwnerDrawn", (long)menu, (long)DrawBitmap);
      }
   }
   else {
      gMenuBookmarks = menu;

      LoadBM(gBookmarkFile);

      nFirstBookmarkPosition = GetMenuItemCount(menu);

      BuildMenu(menu, gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);
   }
}

int DoAccel(char *param)
{
   if (stricmp(param, "Config") == 0){
      return nConfigCommand;
   }
   if (stricmp(param, "Add") == 0){
      return nAddCommand;
   }
   if (stricmp(param, "AddToolbar") == 0){
      return nAddToolbarCommand;
   }
   if (stricmp(param, "Edit") == 0){
      return nEditCommand;
   }
   return nConfigCommand;
}

void DoRebar(HWND rebarWnd) {

   // disabled to fix "create new window pauses for several seconds" bug
   if (gToolbarEnabled){

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   if (!ghWndTB) {
     // Create the toolbar control to be added.
     ghWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
        WS_CHILD | dwStyle,
        0,0,0,0,
        rebarWnd, (HMENU)/*id*/200,
        kPlugin.hDllInstance, NULL
        );
     ghWndTBParent = GetParent(rebarWnd);

     if (!ghWndTB){
       MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
       return;
     }

     BuildRebar(ghWndTB);
   }

   // Create the toolbar control to be added.
   HWND hWndTmp = CreateWindowEx(0, TOOLBARCLASSNAME, "",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );

   if (!hWndTmp){
     MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
     return;
   }

   CopyRebar(hWndTmp, ghWndTB);

   // Register the band name and child hwnd
   kPlugin.kFuncs->RegisterBand(hWndTmp, TOOLBAND_NAME, true);

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hWndTmp, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = PLUGIN_NAME;
   rbBand.hwndChild  = hWndTmp;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = 0;
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

   }
}


// so it doesn't munge the function name
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
      if (gBookmarkRoot.FindNode(LOWORD(dis->itemID))) {
         if (dis->itemState & ODS_SELECTED){
            ImageList_Draw(gImagelist, IMAGE_BOOKMARK, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS);
         }
         else{
            ImageList_Draw(gImagelist, IMAGE_BOOKMARK, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
         }

         return 18;
      }
      return 0;
   }
}
