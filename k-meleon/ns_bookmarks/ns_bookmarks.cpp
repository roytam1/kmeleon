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
// ns_bookmarks.cpp : Plugin that supports netscape-style boomarks
//

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
UINT nAddToolbarCommand;
UINT nEditCommand;
UINT nDropdownCommand;
UINT nFirstBookmarkPosition;

CHAR gBookmarkFile[MAX_PATH];
CHAR gToolbarFolder[MAX_PATH];

char gBookmarksTitle[BOOKMARKS_TITLE_LEN];

HIMAGELIST gImagelist; // the one and only imagelist...

HMENU gMenuBookmarks;
HWND ghWndTB;

BOOL gBookmarksModified = false;
BOOL gGeneratedByUs = false;

BOOL gToolbarEnabled;

int gMaxMenuLength;
BOOL gMenuAutoDetect;
int gMaxTBSize;

WNDPROC KMeleonWndProc;

CBookmarkNode gBookmarkRoot(0, "", "", BOOKMARK_FOLDER, 0);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

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
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
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
      else return 0;

      return 1;
   }
   return 0;
}

int Init(){
   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddToolbarCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);

   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, "");

   if (!gBookmarkFile[0]) {
      kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_SETTINGS_DIR, gBookmarkFile, "");
      strcat(gBookmarkFile, BOOKMARKS_DEFAULT_FILENAME);
   }

   FILE *bmFile = fopen(gBookmarkFile, "r");
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
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile);

   kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, "");
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled, &gToolbarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_MENU_LENGTH, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTODETECT, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = 20;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_TB_SIZE, &gMaxTBSize, &gMaxTBSize);
   if (gMaxTBSize < 1) gMaxTBSize = 20;

   gImagelist = ImageList_Create(16, 15, ILC_MASK, 4, 4);
   HBITMAP bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
   ImageList_AddMasked(gImagelist, bitmap, RGB(192, 192, 192));
   DeleteObject(bitmap);

   strcpy(gBookmarksTitle, BOOKMARKS_DEFAULT_TITLE);

   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Quit(){
   ImageList_Destroy(gImagelist);

   if (gBookmarksModified) {
      Save(gBookmarkFile);
   }
}

void Config(HWND hWndParent){
   DialogBoxParam(kPlugin.hDllInstance ,MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, NULL);
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
      if (stricmp(action, "Edit") == 0){
         command = nEditCommand;
      }
      if (command) {
         AppendMenu(menu, MF_STRING, command, string);
      }
   }
   else {
      gMenuBookmarks = menu;

      FILE *bmFile = fopen(gBookmarkFile, "r");
      if (bmFile){
         long bmFileSize = FileSize(bmFile);

         char *bmFileBuffer = new char[bmFileSize];
         if (bmFileBuffer){
            fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);

            strtok(bmFileBuffer, "\n");
            ParseBookmarks(bmFileBuffer, gBookmarkRoot);

            delete [] bmFileBuffer;
         }
         fclose(bmFile);
      }

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

   // Create the toolbar control to be added.
   ghWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );


   // Register the band name and child hwnd
   kPlugin.kFuncs->RegisterBand(ghWndTB, TOOLBAND_NAME);

   if (!ghWndTB){
      MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
      return;
   }

   BuildRebar(ghWndTB);

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(ghWndTB, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = PLUGIN_NAME;
   rbBand.hwndChild  = ghWndTB;
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
// FIXME - make this work without nFirstBookmarkCommand and MAX_BOOKMARKS
//      if (dis->itemID >= nFirstBookmarkCommand && dis->itemID < (nFirstBookmarkCommand + MAX_BOOKMARKS)){
//         if (dis->itemState & ODS_SELECTED){
//            ImageList_Draw(gImagelist, IMAGE_BOOKMARK, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT | ILD_FOCUS);
//         }
//         else{
//            ImageList_Draw(gImagelist, IMAGE_BOOKMARK, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);
//         }
//
//         return 18;
//      }
      return 0;
   }
}
