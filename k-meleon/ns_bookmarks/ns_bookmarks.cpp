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
UINT nEditCommand;
UINT nDropdownCommand;
UINT nFirstBookmarkCommand;

CHAR gBookmarkFile[MAX_PATH];
CHAR gToolbarFolder[MAX_PATH];

char gBookmarksTitle[BOOKMARKS_TITLE_LEN];

HIMAGELIST gImagelist; // the one and only imagelist...

HMENU gMenuBookmarks;

BOOL gBookmarksModified = false;
BOOL gGeneratedByUs = false;

BOOL gToolbarEnabled;

WNDPROC KMeleonWndProc;

CBookmarkNode gBookmarkRoot(0, "", "", BOOKMARK_FOLDER);
int gNumBookmarks;

pluginFunctions pFuncs = {
   Init,
   Create,
   Config,
   Quit,
   DoMenu,
   DoRebar,
   DoAccel
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   &pFuncs
};

int Init(){
   nConfigCommand = kPlugin.kf->GetCommandIDs(1);
   nAddCommand = kPlugin.kf->GetCommandIDs(1);
   nEditCommand = kPlugin.kf->GetCommandIDs(1);
   nDropdownCommand = kPlugin.kf->GetCommandIDs(1);

   nFirstBookmarkCommand = kPlugin.kf->GetCommandIDs(MAX_BOOKMARKS);

   kPlugin.kf->GetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, "");

   if (!gBookmarkFile[0]) {
      kPlugin.kf->GetPreference(PREF_STRING, PREFERENCE_SETTINGS_DIR, gBookmarkFile, "");
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
            if (!bmFile)
               bmFile = fopen(gBookmarkFile, "w");
            fclose(bmFile);
         }
         else {
            // this means they hit "Cancel" in the file open dialog
            MessageBox(NULL, BOOKMARKS_CREATING_NEW, PLUGIN_NAME, 0);
            bmFile = fopen(gBookmarkFile, "w");
            if (bmFile) {
               fclose(bmFile);
            }
         }
      }
      // create the default bookmark file
      else {
         bmFile = fopen(gBookmarkFile, "w");
         if (bmFile) {
            fclose(bmFile);
         }
      }
   }
   kPlugin.kf->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile);

   kPlugin.kf->GetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, "");
   kPlugin.kf->GetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled, &gToolbarEnabled);

   gImagelist = ImageList_Create(16, 15, ILC_MASK, 4, 4);
   HBITMAP bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
   ImageList_AddMasked(gImagelist, bitmap, RGB(192, 192, 192));
   DeleteObject(bitmap);

   strcpy(gBookmarksTitle, BOOKMARKS_DEFAULT_TITLE);
   gNumBookmarks = 0;

   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Quit(){
   //  the menu should have been destroyed by kmeleon...
   ImageList_Destroy(gImagelist);
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

      BuildMenu(menu, gBookmarkRoot);
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
   if (stricmp(param, "Edit") == 0){
      return nEditCommand;
   }
   return nAddCommand;
}

void DoRebar(HWND rebarWnd) {

   // disabled to fix "create new window pauses for several seconds" bug
   if (gToolbarEnabled){

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

   // Create the toolbar control to be added.
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );


   // Register the band name and child hwnd
   kPlugin.kf->RegisterBand(hwndTB, TOOLBAND_NAME);

   if (!hwndTB){
      MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
      return;
   }

   CBookmarkNode *toolbarNode = gBookmarkRoot.FindToolbarNode();

   SetWindowText(hwndTB, TOOLBAND_NAME);

   //SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;

   CBookmarkNode *child;

   for (child=toolbarNode->child; child; child=child->next) {
      if (child->type == BOOKMARK_SEPARATOR) {
         continue;
      }

      stringID = SendMessage(hwndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)child->text.c_str());

      TBBUTTON button = {0};
      button.iString = stringID;
      button.fsState = TBSTATE_ENABLED;
      button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

      if (child->type == BOOKMARK_FOLDER){
         button.idCommand = MENU_TO_COMMAND(child->id);  // for folders, id is the HMENU
         button.iBitmap = IMAGE_FOLDER_CLOSED;
         button.fsStyle |= TBSTYLE_DROPDOWN;
      }
      else {
         button.idCommand = child->id + nFirstBookmarkCommand;
         button.iBitmap = IMAGE_BOOKMARK;
      }

      SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
   }

   TBBUTTON button = {0};
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_SEP;
   SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   button.iBitmap = IMAGE_CHEVRON;
   button.idCommand = nDropdownCommand;
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
   button.iString = -1;
   SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = PLUGIN_NAME;
   rbBand.hwndChild  = hwndTB;
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
      if (dis->itemID >= nFirstBookmarkCommand && dis->itemID < (nFirstBookmarkCommand + MAX_BOOKMARKS)){
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
