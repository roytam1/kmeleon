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

#include "stdafx.h"
#include "resource.h"

#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../Utils.h"

#include "..\\rebar_menu\\hot_tracking.h"

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

extern WNDPROC KMeleonWndProc;

// Preferences Dialog function
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
      case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_SETCHECK, gToolbarEnabled, 0);
         SetDlgItemText(hWnd, IDC_BOOKMARKS_FILE, gBookmarkFile);
         SetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, gMaxMenuLength, false);
         SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_SETCHECK, gMenuAutoDetect, 0);
         if (gMenuAutoDetect) {
            EnableWindow(GetDlgItem(hWnd, IDC_MAX_MENU_LENGTH), false);
         }
         SetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, gMaxTBSize, false);
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
               case IDC_BROWSE:
                  {
                     char tempPath[MAX_PATH];
                     HWND hBookmarksFileWnd = GetDlgItem(hWnd, IDC_BOOKMARKS_FILE);

                     GetWindowText(hBookmarksFileWnd, tempPath, MAX_PATH);
                     if (BrowseForBookmarks(tempPath)) {
                        // if the file they chose doesn't exist, create it
                        FILE *bmFile = fopen(gBookmarkFile, "r");
                        if (!bmFile) {
                           bmFile = fopen(gBookmarkFile, "w");
                           gGeneratedByUs = true;
                        }
                        fclose(bmFile);

                        SetWindowText(hBookmarksFileWnd, tempPath);
                        SetFocus(hBookmarksFileWnd);
                     }
                  }
                  break;
               case IDOK:
                  gToolbarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                  kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled);

                  GetDlgItemText(hWnd, IDC_BOOKMARKS_FILE, gBookmarkFile, MAX_PATH);
                  kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile);

                  gMaxMenuLength = GetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, NULL, false);
                  if (gMaxMenuLength < 1) gMaxMenuLength = 20;
                  kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MAX_MENU_LENGTH, &gMaxMenuLength);

                  gMenuAutoDetect = SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_GETCHECK, 0, 0);
                  kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_MENU_AUTODETECT, &gMenuAutoDetect);

                  gMaxTBSize = GetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, NULL, false);
                  if (gMaxTBSize < 1) gMaxTBSize = 20;
                  kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MAX_TB_SIZE, &gMaxTBSize);

                  // rebuild menu and toolbars to provide instant gratification to users
                  Rebuild();

                  // fall through...
               case IDCANCEL:
                  SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;
      case WM_CLOSE:
         EndDialog(hWnd, NULL);
         break;
      default:
         return false;
   }
   return true;
}

BOOL BrowseForBookmarks(char *file)
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
   ofn.lpstrFilter = BOOKMARKS_FILTER;
   ofn.lpstrFile = file;
   ofn.nMaxFile = MAX_PATH;
   ofn.Flags = OFN_PATHMUSTEXIST | 	OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
   ofn.lpstrTitle = PLUGIN_NAME;

   if (GetOpenFileName(&ofn)) {
      return true;
   }
   else {
      return false;
   }
}

// Save bookmarks
static void SaveBookmarks(FILE *bmFile, CBookmarkNode *node)
{
#define MAXSPACER 50
   int type;
   CBookmarkNode *child;
   static char szSpacer[MAXSPACER+1] = {0};

   fprintf(bmFile, "%s<DL><p>\n", szSpacer);
   if (strlen(szSpacer) < MAXSPACER) szSpacer[strlen(szSpacer)] = ' ';   // add a space

   for (child=node->child; child; child=child->next) {
      type = child->type;
      if (type == BOOKMARK_FOLDER){
         char szFolderFlags[256] = {0};
         if (child->flags & BOOKMARK_FLAG_TB)
            strcat(szFolderFlags, "PERSONAL_TOOLBAR_FOLDER=\"true\" ID=\"NC:PersonalToolbarFolder\" ");
         if (child->flags & BOOKMARK_FLAG_NB)
            strcat(szFolderFlags, "NEWITEMHEADER ");
         if (child->flags & BOOKMARK_FLAG_BM)
            strcat(szFolderFlags, "MENUHEADER ");
         fprintf(bmFile, "%s<DT><H3 %sADD_DATE=\"%d\">%s</H3>\n", szSpacer, szFolderFlags, child->addDate, child->text.c_str());

         if (strlen(szSpacer) < MAXSPACER) szSpacer[strlen(szSpacer)] = ' ';   // add a space
         SaveBookmarks(bmFile, child);
         szSpacer[strlen(szSpacer)-1] = 0;      // remove the space
      }
      else if (type == BOOKMARK_SEPARATOR) {
         fprintf(bmFile, "%s<HR>\n", szSpacer);
      }
      else if (type == BOOKMARK_BOOKMARK) {
         fprintf(bmFile, "%s<DT><A HREF=\"%s\" ADD_DATE=\"%d\" LAST_VISIT=\"%d\" LAST_MODIFIED=\"%d\">%s</A>\n", szSpacer, child->url.c_str(), child->addDate, child->lastVisit, child->lastModified, child->text.c_str());
      }
      // if it falls through, there's a problem, but we'll just ignore it for now.
   }
   szSpacer[strlen(szSpacer)-1] = 0;      // remove the space
   fprintf(bmFile, "%s</DL><p>\n", szSpacer);
}

void Save(const char *file)
{
   if (!gGeneratedByUs) {
      if (MessageBox(NULL, BOOKMARKS_NOT_BY_US, PLUGIN_NAME, MB_YESNO) != IDYES) {
         return;
      }
   }

   FILE *bmFile = fopen(file, "w");
   if (bmFile){
      fprintf(bmFile, "%s\n", BOOKMARK_TAG);
      fprintf(bmFile, "%s\n", KMELEON_TAG);
      fprintf(bmFile, "%s\n", COMMENT_TAG);
// FIXME - figure out if this needs to be here, or if it should be different on non-US-English systems
//      fprintf(bmFile, "%s\n", CONTENT_TYPE_TAG);
      fprintf(bmFile, "<TITLE>%s</TITLE>\n", gBookmarksTitle);
      fprintf(bmFile, "<H1>%s</H1>\n\n", gBookmarksTitle);

      SaveBookmarks(bmFile, &gBookmarkRoot);

      fclose(bmFile);
   }

   gGeneratedByUs = true;
   gBookmarksModified = false;

   /* this is to support both NS 4 and NS 6 style bookmarks */
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, (void *)gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_TB)->text.c_str());
}

// Load bookmarks
void ParseBookmarks(char *bmFileBuffer, CBookmarkNode &node)
{
   char *p;
   char *t;
   bool found_tb = false;

   while ((p = strtok(NULL, "\n")) != NULL){
      if ((t = strstr(p, "<DT><H3")) != NULL){
         t+=7;
         char *name = strchr(t, '>');
         if (name) {
            *name = 0; name++;
         }
         else {
            name = "";
         }

         char *end = strrchr(name, '<');
         if (end) *end = 0;

         time_t addDate=0;
         char *d;
         d = strstr(t, "ADD_DATE=\"");
         if (d) {
            d+=10;
            addDate = atol(d);
         }

         CBookmarkNode * newNode = new CBookmarkNode(0, name, "", BOOKMARK_FOLDER, addDate);
         node.AddChild(newNode);

         ParseBookmarks(end, *newNode);

         // this is to support both NS 4 and NS 6 style bookmarks
         // FIXME - We should only allow one toolbar folder, eh?
         //   Example: Two folders, same name, set first one as toolbar folder.
         //             Pref saved w/ name only, though only one gets PERSONAL_TOOL...
         //             Both given FLAG_TB when next parsed, because both match gToolbarFolder
         //   Note: NS 4 has a bug (tee-hee!) - It just sets the first that matches to the toolbar.
         //         So I'll just do the same for now.  First folder that matches gets it, no others.
         //   But this should still be fixed (probably by just removing NS 4 compatibility...)
         if ( !found_tb &&
              ((strcmp(name, gToolbarFolder) == 0) ||
              (strstr(t, _Q(PERSONAL_TOOLBAR_FOLDER="true")))) )
         {
            newNode->flags |= BOOKMARK_FLAG_TB;
            found_tb = true;
         }

         // These are both NS 4 style - I don't know how Mozilla does it
         if ( strstr(t, "NEWITEMHEADER") ) {
            newNode->flags |= BOOKMARK_FLAG_NB;
         }
         if ( strstr(t, "MENUHEADER") ) {
            newNode->flags |= BOOKMARK_FLAG_BM;
         }

         newNode->id = 0;  // later, in buildmenu, this will be set to the hmenu
      }
      else if ((t = strstr(p, "<DT><A HREF=\"")) != NULL) {
         t+=13; // t now points to the url

         char *name = t;
         char *url = t;

         // if there's a quote, chop it
         char *q = strchr(url, '\"');
         if (q) {
            *q = 0;
            t = q+1;
         }

         time_t addDate=0;
         time_t lastVisit=0;
         time_t lastModified=0;

         char *d;
         d = strstr(t, "ADD_DATE=\"");
         if (d) {
            d+=10;
            addDate = atol(d);
         }
         d = strstr(t, "LAST_VISIT=\"");
         if (d) {
            d+=12;
            lastVisit = atol(d);
         }
         d = strstr(t, "LAST_MODIFIED=\"");
         if (d) {
            d+=15;
            lastModified = atol(d);
         }

         t = strchr(t, '>');
         if (t) {
            name = t+1;
            q = strchr(name, '<');
            if (q) {
               *q = 0;
            }
         }
         else {
            name = "";
         }

         // if we have no name, set our name to the url
         if (name[0] == 0)
            name = url;

         node.AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), name, url, BOOKMARK_BOOKMARK, addDate, lastVisit, lastModified));
      }
      else if ((t = strstr(p, "</DL>")) != NULL) {
         return;
      }
      else if ((t = strstr(p, "<HR>")) != NULL) {
         node.AddChild(new CBookmarkNode(0, "", "", BOOKMARK_SEPARATOR));
      }
      else if ((t = strstr(p, "<TITLE>")) != NULL) {
         t+=7; // t now points to the title

         char *end = strrchr(t, '<');
         if (end) {
            if (end > t+BOOKMARKS_TITLE_LEN) end = t+BOOKMARKS_TITLE_LEN;
            *end = 0;
         }

         strcpy(gBookmarksTitle, t);
      }
      else if (strstr(p, KMELEON_TAG) != NULL) {
         gGeneratedByUs = true;
      }
   }
   return;
}

// Build Menu
void BuildMenu(HMENU menu, CBookmarkNode *node, BOOL isContinuation)
{
   // screen height
   int cy = GetSystemMetrics(SM_CYSCREEN);
   // menu line height
//   int cmenu = GetSystemMetrics(SM_CYMENU);
   // if bmp_menu is enabled, the menu items will actually be at least 18 pixels... but this system call won't reflect that
   // in any case, SM_CYMENU gets the height of the menu bar, not a menu item
   // for now we'll just assume bmp_menu is enabled and they're 18 pixels...
#define cmenu 18

// space to allow above menu for title bar, menu bar (assuming maximized window), and extra frame junk
#define MENUPADDING 50

   int maxLength = (gMenuAutoDetect) ? (int)((cy-MENUPADDING)/cmenu) : gMaxMenuLength;

   CBookmarkNode *child;
   int count = GetMenuItemCount(menu);

   for (child = (isContinuation) ? node : node->child ; child ; child = child->next , count++) {
      if (count == maxLength) {
         HMENU childMenu = CreatePopupMenu();
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, "[more]");
         BuildMenu(childMenu, child, true);
         break;
      }
      else if (child->type == BOOKMARK_SEPARATOR) {
         AppendMenu(menu, MF_SEPARATOR, 0, "");
      }
      else if (child->type == BOOKMARK_FOLDER) {
         HMENU childMenu = CreatePopupMenu();
         child->id = (UINT)childMenu; // we have to save off the HMENU for the rebar
         // condense the title and escape ampersands
         char *pszTemp = fixString(child->text.c_str(), 40);
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, pszTemp);
         delete pszTemp;
         BuildMenu(childMenu, child, false);
      }
      else if (child->type == BOOKMARK_BOOKMARK) {
         // condense the title and escape ampersands
         char *pszTemp = fixString(child->text.c_str(), 40);
         AppendMenu(menu, MF_STRING, child->id, pszTemp);
         delete pszTemp;
      }
   }
}

// Build Rebar
void BuildRebar(HWND hWndTB)
{
   CBookmarkNode *toolbarNode = gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_TB);

   SetWindowText(hWndTB, TOOLBAND_NAME);

   //SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);

   SendMessage(hWndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   int stringID;
   CBookmarkNode *child;
   int curButton = 0;

   for (child=toolbarNode->child; child; child=child->next) {
      if (++curButton > TOOLBAND_MAX_BUTTONS) {
         break;      // Simple fix to prevent massive toolbar folders from bringing KM to its knees
      }

      if (child->type == BOOKMARK_SEPARATOR) {
         continue;
      }

      // condense the title and escape ampersands
      char *buttonString = fixString(child->text.c_str(), gMaxTBSize);
      stringID = SendMessage(hWndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)buttonString);
      delete buttonString;

      TBBUTTON button = {0};
      button.iString = stringID;
      button.fsState = TBSTATE_ENABLED;
      button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

      if (child->type == BOOKMARK_FOLDER){
         // toolbar may not be contained in bookmark menu, so we can't just use its submenus
         HMENU childMenu = CreatePopupMenu();
         BuildMenu(childMenu, child, false);
         button.idCommand = MENU_TO_COMMAND((UINT)childMenu);
         button.iBitmap = IMAGE_FOLDER_CLOSED;
         button.fsStyle |= TBSTYLE_DROPDOWN;
      }
      else {
         button.idCommand = child->id;
         button.iBitmap = IMAGE_BOOKMARK;
      }

      SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
   }

   TBBUTTON button = {0};
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_SEP;
   SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   button.iBitmap = IMAGE_CHEVRON;
   button.idCommand = nDropdownCommand;
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
   button.iString = -1;
   SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);
}

void Rebuild() {
   // delete the old bookmarks from the menu (FIXME - needs to be more robust than "delete everything after the first bookmark position" - there may be normal menu items there (if the user is weird))
   while (DeleteMenu(gMenuBookmarks, nFirstBookmarkPosition, MF_BYPOSITION));
   // and rebuild
   BuildMenu(gMenuBookmarks, gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);

   // need to rebuild the rebar, too, in case it had submenus (whose ids are now invalid)
   if (ghWndTB) {
      // delete the old rebar
      while (SendMessage(ghWndTB, TB_DELETEBUTTON, 0 /*index*/, 0));
      // and rebuild
      BuildRebar(ghWndTB);
   }

// FIXME - Is this needed?  Hm, if anywhere, in WndProc, below, in the nAddCommand/nAddToolbarCommand/nEditCommand cases...  but then it's still only one window, and the others don't get the call, so...   heck, it works without it.  It will stay until someone complains.  :)
//   DrawMenuBar(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;

   if (message == WM_COMMAND) {
      WORD command = LOWORD(wParam);

      if (command == nConfigCommand) {
         Config(hWnd);
         return true;
      }
      else if (command == nAddCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
         if (dInfo) {
            CBookmarkNode *addNode = gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_NB);
            addNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), dInfo->title, dInfo->url, BOOKMARK_BOOKMARK, time(NULL)));

            Save(gBookmarkFile);

            Rebuild();
         }
         return true;
      }
      else if (command == nAddToolbarCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
         if (dInfo) {
            CBookmarkNode *addNode = gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_TB);
            addNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), dInfo->title, dInfo->url, BOOKMARK_BOOKMARK, time(NULL)));

            Save(gBookmarkFile);

            Rebuild();
         }
         return true;
      }
      else if (command == nEditCommand) {
         DialogBoxParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), hWnd, EditProc, 0);
         return true;
      }
      else if (CBookmarkNode *node = gBookmarkRoot.FindNode(command)) {
         node->lastVisit = time(NULL);
         gBookmarksModified = true;	// this doesn't call for instant saving, it can wait until we add/edit/quit
         kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL);

         return true;
      }
   }
   else if (message == WM_NOTIFY) {
      hdr = *((LPNMHDR)lParam);
      if (hdr.code == TBN_DROPDOWN) {
         tbhdr = *((LPNMTOOLBAR)lParam);

         // this is the little down arrow thing
         if (tbhdr.iItem == nDropdownCommand){
            RECT rc;
            WPARAM index = 0;
            SendMessage(tbhdr.hdr.hwndFrom, TB_GETITEMRECT, index, (LPARAM) &rc);
            POINT pt = { rc.left, rc.bottom };
            ClientToScreen(tbhdr.hdr.hwndFrom, &pt);
            TrackPopupMenu((HMENU)gMenuBookmarks, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
         }
         else if (IsMenu((HMENU)(tbhdr.iItem-SUBMENU_OFFSET))){
            char toolbarName[11];
            GetWindowText(tbhdr.hdr.hwndFrom, toolbarName, 10);
            if (strcmp(toolbarName, TOOLBAND_NAME) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }

            // post a message to defer exceution of BeginHotTrack
            PostMessage(hWnd, WM_DEFERHOTTRACK, NULL, NULL);

            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   else if (message == WM_DEFERHOTTRACK) {
      BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
      return true;
   }
   else if (message == WM_MENUSELECT) {
      if (CBookmarkNode *node = gBookmarkRoot.FindNode(LOWORD(wParam))) {
         kPlugin.kFuncs->SetStatusBarText(node->url.c_str());
         return true;
      }
// this would be a hack to clean the status bar for separators, popups
// (they work (clear the status bar) for CMenu-added separators/popups,
// but not when they are added via standard win32 calls...)
//      else {
//         kPlugin.kFuncs->SetStatusBarText("");
//      }
   }
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}
