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

#include <map>
#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"
#include "../LocalesUtils.h"
Locale* gLoc;

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
UINT nAddBookmarkHereCommand;
UINT nFirstBookmarkPosition;
UINT wm_deferbringtotop;

TCHAR gBookmarkFile[MAX_PATH];
bool gBookmarkDefFile = false;
CHAR gToolbarFolder[MAX_PATH];

char gBookmarksTitle[BOOKMARKS_TITLE_LEN];

HIMAGELIST gImagelist = NULL; // the one and only imagelist...

HMENU gMenuBookmarks = NULL;
HWND ghWndTB = NULL;
HWND ghWndTBParent = NULL;
HWND hWndFront = NULL;
HWND ghWndEdit = NULL;
HANDLE ghMutex = NULL;
std::map<HWND, HWND> gToolbarList;

BOOL gBookmarksModified = FALSE;
BOOL gGeneratedByUs = FALSE;

BOOL gToolbarEnabled = FALSE;
BOOL bChevronEnabled = TRUE;

int gMaxMenuLength;
BOOL gMenuAutoDetect;
int gMaxTBSize;

WNDPROC KMeleonWndProc;

CBookmarkNode *gBookmarkRoot;

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
   gLoc = Locale::kmInit(&kPlugin);

   nConfigCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddToolbarCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddLinkCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nEditCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nDropdownCommand = kPlugin.kFuncs->GetCommandIDs(1);
   nAddBookmarkHereCommand = kPlugin.kFuncs->GetCommandIDs(1);
   wm_deferbringtotop = kPlugin.kFuncs->GetCommandIDs(1);

   gBookmarkRoot = new CBookmarkNode(0, _T(""), _T(""), _T(""), _T(""), "", BOOKMARK_FOLDER, 0);
   ghMutex = CreateMutex(NULL, FALSE, _T("BookmarksFileMutex"));

   TCHAR tmp[MAX_PATH];
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_BOOKMARK_FILE, tmp, (TCHAR*)_T(""));

   if (!tmp[0]) {
      kPlugin.kFuncs->GetFolder(UserSettingsFolder, tmp, MAX_PATH);
      _tcscat(tmp, "\\" BOOKMARKS_DEFAULT_FILENAME);
      gBookmarkDefFile = true;
   }
   _tcscpy(gBookmarkFile, tmp);
   
   FILE *bmFile = _tfopen(gBookmarkFile, _T("a"));
   if (bmFile)
      fclose(bmFile);
   else {
      if (MessageBox(NULL, gLoc->GetString(IDS_NOT_FOUND),
         _T(PLUGIN_NAME), MB_YESNO) == IDYES) {

         if (BrowseForBookmarks(gBookmarkFile)) {
            // if the file they chose doesn't exist, create it
            bmFile = _tfopen(gBookmarkFile, _T("r"));
            if (!bmFile) {
               bmFile = _tfopen(gBookmarkFile, _T("w"));
               gGeneratedByUs = true;
            }
			gBookmarkDefFile = false;
            fclose(bmFile);
         }
         else {
            // this means they hit "Cancel" in the file open dialog
            MessageBox(NULL, gLoc->GetString(IDS_CREATING_NEW), _T(PLUGIN_NAME), 0);
            bmFile = _tfopen(gBookmarkFile, _T("w"));
            if (bmFile) {
               fclose(bmFile);
               gGeneratedByUs = true;
            }
         }
      }
      // create the default bookmark file
      else {
         bmFile = _tfopen(gBookmarkFile, _T("w"));
         if (bmFile) {
            fclose(bmFile);
         }
         gGeneratedByUs = true;
      }
   }
   if (!gBookmarkDefFile)
	 kPlugin.kFuncs->SetPreference(PREF_TSTRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, false);
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, PREFERENCE_TOOLBAR_FOLDER, gToolbarFolder, (TCHAR*)_T(""));
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled, &gToolbarEnabled);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CHEVRON_ENABLED, &bChevronEnabled, &bChevronEnabled);
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_MENU_LENGTH, &gMaxMenuLength, &gMaxMenuLength);
   kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_MENU_AUTODETECT, &gMenuAutoDetect, &gMenuAutoDetect);
   if (gMaxMenuLength < 1) gMaxMenuLength = 20;
   kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_MAX_TB_SIZE, &gMaxTBSize, &gMaxTBSize);
   if (gMaxTBSize < 1) gMaxTBSize = 20;

   InitImageList(gImagelist);

   strcpy(gBookmarksTitle, gLoc->GetString(IDS_DEFAULT_TITLE));
	LoadBM(gBookmarkFile);

   return true;
}

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Close(HWND hWnd) { 
  
   if (gToolbarList.size() == 1) {
	   // All toolbars will be gone, we must remove the menus first.
	   // We can't just free them on quit because of the loader.
	   std::map<HWND, HWND>::iterator it = gToolbarList.begin();

	   // XXX: Should assert if (it->first != hWnd)
	   // We better not clear it if it happens
	   if (it->first == hWnd) {
	     HWND toolbar = it->second;
	     TBBUTTON button;
	     do {
	        if (SendMessage(toolbar, TB_GETBUTTON, (WPARAM)0, (LPARAM)&button))
			    if (button.dwData) {
				    kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)button.dwData, 0);
			        DestroyMenu((HMENU)button.dwData);
			    }
	     } while (SendMessage(toolbar, TB_DELETEBUTTON, 0 /*index*/, 0));
	   }
   }

  gToolbarList.erase(hWnd);
}

void Quit(){
   hWndFront = NULL;
   if (ghWndEdit)
      SendMessage(ghWndEdit, WM_CLOSE, 0, 0);
   ImageList_Destroy(gImagelist);

   if (gBookmarksModified) {
      SaveBM(gBookmarkFile);
   }
   
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)gMenuBookmarks, 0);
   while (RealDeleteMenu(gMenuBookmarks, nFirstBookmarkPosition));

   TCHAR tmp[MAX_PATH];
   kPlugin.kFuncs->GetFolder(UserSettingsFolder, tmp, MAX_PATH);
   _tcscat(tmp, "\\" BOOKMARKS_DEFAULT_FILENAME);
 
   if (gBookmarkDefFile || _tcscmp(tmp, gBookmarkFile) == 0)
	   kPlugin.kFuncs->DelPreference(PREFERENCE_BOOKMARK_FILE);

   CloseHandle(ghMutex);

   delete gBookmarkRoot;
   delete gLoc;
}

void Config(HWND hWndParent){
   gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_CONFIG), hWndParent, (DLGPROC)DlgProc, (LPARAM)NULL);
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

      nFirstBookmarkPosition = GetMenuItemCount(menu);

      BuildMenu(menu, gBookmarkRoot->FindSpecialNode(BOOKMARK_FLAG_BM), false);
   }
}

int DoAccel(char *param)
{
   if (stricmp(param, "Config") == 0)
      return nConfigCommand;
   if (stricmp(param, "Add") == 0)
      return nAddCommand;
   if (stricmp(param, "AddToolbar") == 0)
      return nAddToolbarCommand;
    if (stricmp(param, "Edit") == 0)
      return nEditCommand;
	if (stricmp(param, "AddLink") == 0)
      return nAddLinkCommand;
   return 0;
}

void DoRebar(HWND rebarWnd) {

   // disabled to fix "create new window pauses for several seconds" bug
   if (gToolbarEnabled){

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS;

/*
   if (!ghWndTB) {
     // Create the toolbar control to be added.
     ghWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, _T(""),
        WS_CHILD | dwStyle,
        0,0,0,0,
        rebarWnd, (HMENU)200,
        kPlugin.hDllInstance, NULL
        );
     ghWndTBParent = GetParent(rebarWnd);

     if (!ghWndTB){
       MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
       return;
     }

     BuildRebar(ghWndTB);
   }*/

   // Create the toolbar control to be added.
   HWND hWndTmp = CreateWindowEx(0, TOOLBARCLASSNAME, _T(""),
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)200,
      kPlugin.hDllInstance, NULL
      );
	
   //HWND hWndTmp = kPlugin.kFuncs->CreateToolbar(GetParent(rebarWnd), CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST );
   if (!hWndTmp){
     MessageBox(NULL, TOOLBAND_FAILED_TO_CREATE, NULL, 0);
     return;
   }

   std::map<HWND, HWND>::iterator it = gToolbarList.begin();
   if (it == gToolbarList.end())
	   // This is the first toolbar, build it.
	   BuildRebar(hWndTmp);
   else // Just copy an existing one.
	   CopyRebar(hWndTmp, it->second);
   gToolbarList[GetParent(rebarWnd)] = hWndTmp;

   // Register the band name and child hwnd
   kPlugin.kFuncs->RegisterBand(hWndTmp, _T(TOOLBAND_NAME), true);

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hWndTmp, TB_GETBUTTONSIZE, 0,0);

     // Compute the width needed for the toolbar
   int ideal = 0;
   int iCount, iButtonCount = SendMessage(hWndTmp, TB_BUTTONCOUNT, 0,0);
   for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
   {
	   RECT rectButton;
	   SendMessage(hWndTmp, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
	   ideal += rectButton.right - rectButton.left;
   }

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = //RBBIM_TEXT |
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_USECHEVRON | RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = _T(PLUGIN_NAME);
   rbBand.hwndChild  = hWndTmp;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cx         = rbBand.cxIdeal = ideal;

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
            ImageList_Draw(gImagelist, IMAGE_FOLDER_OPEN, dis->hDC, dis->rcItem.left+2, top, ILD_TRANSPARENT | ILD_FOCUS );
         }
         else{
            ImageList_Draw(gImagelist, IMAGE_FOLDER_CLOSED, dis->hDC, dis->rcItem.left+2, top, ILD_TRANSPARENT);
         }
         return 18;
      }
// FIXME - This is probably way too slow to be useful.
	  CBookmarkNode* node = gBookmarkRoot->FindNode(LOWORD(dis->itemID));
      if (node) {
		 UINT flags = ILD_NORMAL;
		 if (dis->itemState & ODS_SELECTED)
			flags |= ILD_FOCUS;

         HIMAGELIST hList = gImagelist;
         UINT idx = IMAGE_BOOKMARK;

		 UINT i = GetSiteIcon((char*)node->url.c_str());
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
