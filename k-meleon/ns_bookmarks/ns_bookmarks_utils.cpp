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
         break;
      case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
               case IDC_BROWSE:
                  {
                     char tempPath[MAX_PATH];
                     HWND hBookmarksFileWnd = GetDlgItem(hWnd, IDC_BOOKMARKS_FILE);

                     GetWindowText(hBookmarksFileWnd, tempPath, MAX_PATH);
                     if (BrowseForBookmarks(tempPath)) {
                        SetWindowText(hBookmarksFileWnd, tempPath);
                        SetFocus(hBookmarksFileWnd);
                     }
                  }
                  break;
               case IDOK:
                  gToolbarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                  kPlugin.kf->SetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled);

                  GetDlgItemText(hWnd, IDC_BOOKMARKS_FILE, gBookmarkFile, MAX_PATH);
                  kPlugin.kf->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile);

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
			return FALSE;
   }
   return TRUE;
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

// Save boomkarks

static void SaveBookmarks(FILE *bmFile, CBookmarkNode &node)
{
   fprintf(bmFile, "<DL><p>\n");

   int type;
   CBookmarkNode *child;
   for (child=node.child; child; child=child->next) {
      type = child->type;
      if (type == BOOKMARK_FOLDER){
         fprintf(bmFile, "<DT><H3 ADD_DATE=\"%d\">%s</H3>\n", child->addDate, child->text.c_str());

         SaveBookmarks(bmFile, *child);
      }
      else if (type == BOOKMARK_FOLDER_TB) {
         fprintf(bmFile, "<DT><H3 ADD_DATE=\"%d\" PERSONAL_TOOLBAR_FOLDER=\"true\">%s</H3>\n<DL><p>\n", child->addDate, child->text.c_str());

         SaveBookmarks(bmFile, *child);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         fprintf(bmFile, "<HR>\n");
      }
      else{
         fprintf(bmFile, "<DT><A HREF=\"%s\" ADD_DATE=\"%d\" LAST_VISIT=\"%d\" LAST_ACCESS=\"%d\">%s</A>\n", child->url.c_str(), child->addDate, child->lastModified, child->lastVisit, child->text.c_str());
      }
   }
   fprintf(bmFile, "</DL><p>\n");
}

void Save(const char *file)
{
   if (!gMenuBookmarks)
      return;

   if (!gGeneratedByUs) {
      if (MessageBox(NULL, BOOKMARKS_NOT_BY_US,
         PLUGIN_NAME, MB_YESNO) != IDYES) {
         return;
      }
   }

   FILE *bmFile = fopen(file, "w");
   if (bmFile){
      fprintf(bmFile, "%s\n", BOOKMARK_TAG);
      fprintf(bmFile, "%s\n", KMELEON_TAG);
      fprintf(bmFile, "%s\n", COMMENT_TAG);
      fprintf(bmFile, "%s\n", CONTENT_TYPE_TAG);
      fprintf(bmFile, "<TITLE>%s</TITLE>\n", gBookmarksTitle);
      fprintf(bmFile, "<H1>%s</H1>\n\n", gBookmarksTitle);

      SaveBookmarks(bmFile, gBookmarkRoot);

      fclose(bmFile);
   }
   /* this is to support both NS 4 and NS 6 style bookmarks */
   kPlugin.kf->SetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, (void *)gBookmarkRoot.FindToolbarNode()->text.c_str());
}

// Load bookmarks

void ParseBookmarks(char *bmFileBuffer, CBookmarkNode &node)
{
   char *p;
   char *t;
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

         /* this is to support both NS 4 and NS 6 style bookmarks */
         if ( (strcmp(name, gToolbarFolder) == 0) ||
              (strstr(t, _Q(PERSONAL_TOOLBAR_FOLDER="true"))) )
         {
            newNode->type = BOOKMARK_FOLDER_TB;
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
            *q = 0;
         }
         else {
            name = "";
         }

         // if we have no name, set our name to the url
         if (name[0] == 0)
            name = url;

         node.AddChild(new CBookmarkNode(gNumBookmarks, name, url, BOOKMARK_BOOKMARK, addDate, lastVisit, lastModified));
         gNumBookmarks++;
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
void BuildMenu(HMENU menu, CBookmarkNode &node)
{
   CBookmarkNode *child;
   for (child=node.child; child; child=child->next) {
      if (child->type == BOOKMARK_SEPARATOR) {
         AppendMenu(menu, MF_SEPARATOR, 0, "");
      }
      else if (child->type == BOOKMARK_FOLDER || child->type == BOOKMARK_FOLDER_TB) {
         HMENU childMenu = CreatePopupMenu();
         BuildMenu(childMenu, *child);
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, child->text.c_str());
         child->id = (UINT)childMenu; // we have to save off the HMENU for the rebar
      }
      else {
         char *pszTemp = _strdup(child->text.c_str());
         CondenseString(pszTemp, 40);
         AppendMenu(menu, MF_STRING, nFirstBookmarkCommand + child->id, pszTemp);
         delete pszTemp;
      }
   }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (message == WM_COMMAND){
      WORD command = LOWORD(wParam);

      if (command == nConfigCommand) {
         Config(NULL);
         return true;
      }
      else if (command == nAddCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kf->GetDocInfo(hWnd);
         if (dInfo) {
            gBookmarkRoot.AddChild(new CBookmarkNode(gNumBookmarks, dInfo->title, dInfo->url, BOOKMARK_BOOKMARK, time(NULL)));

            if (strlen(dInfo->title) > 40)
               CondenseString(dInfo->title, 40);

            AppendMenu(gMenuBookmarks, MF_STRING, nFirstBookmarkCommand+gNumBookmarks, dInfo->title);
            gNumBookmarks++;

            DrawMenuBar(hWnd);
         }
         gBookmarksModified = true;
         return true;
      }
      else if (command == nEditCommand) {
         DialogBoxParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), hWnd, EditProc, 0);
         return true;
      }
      else if ((command >= nFirstBookmarkCommand) && (command < (nFirstBookmarkCommand + MAX_BOOKMARKS))){
         int id = command-nFirstBookmarkCommand;
         CBookmarkNode *node = gBookmarkRoot.FindNode(id);

         node->lastVisit = time(NULL);

         kPlugin.kf->NavigateTo(node->url.c_str(), OPEN_NORMAL);

         return true;
      }
   }
   else if (message == WM_NOTIFY){
      NMHDR *hdr = (LPNMHDR)lParam;
      if (hdr->code == TBN_DROPDOWN){
         NMTOOLBAR *tbhdr = (LPNMTOOLBAR)lParam;

         // this is the little down arrow thing
         if (tbhdr->iItem == nDropdownCommand){
            RECT rc;
            WPARAM index = 0;
            SendMessage(tbhdr->hdr.hwndFrom, TB_GETITEMRECT, index, (LPARAM) &rc);
            POINT pt = { rc.left, rc.bottom };
            ClientToScreen(tbhdr->hdr.hwndFrom, &pt);
            TrackPopupMenu((HMENU)gMenuBookmarks, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
         }
         else if (IsMenu(COMMAND_TO_MENU(tbhdr->iItem))){
            char toolbarName[11];
            GetWindowText(tbhdr->hdr.hwndFrom, toolbarName, 10);
            if (strcmp(toolbarName, TOOLBAND_NAME) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }

            BeginHotTrack(tbhdr, kPlugin.hDllInstance, hWnd);

            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}
