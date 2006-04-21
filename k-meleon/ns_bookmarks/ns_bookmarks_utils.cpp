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

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "stdafx.h"
#include "resource.h"
#include "wininet.h"    // for INTERNET_MAX_URL_LENGTH
#include <stdio.h>
#include <io.h>

#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
//#include "../resource.h"
#include "../Utils.h"
#include <tchar.h>
#include "..\\rebar_menu\\hot_tracking.h"

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

extern WNDPROC KMeleonWndProc;
extern BOOL bChevronEnabled;
char szContentType[BOOKMARKS_TITLE_LEN];

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
                     TCHAR tempPath[MAX_PATH];
                     HWND hBookmarksFileWnd = GetDlgItem(hWnd, IDC_BOOKMARKS_FILE);

                     GetWindowText(hBookmarksFileWnd, tempPath, MAX_PATH);
                     if (BrowseForBookmarks(tempPath)) {
                        // if the file they chose doesn't exist, create it
                        FILE *bmFile = _tfopen(gBookmarkFile, _T("r"));
                        if (!bmFile) {
                           bmFile = _tfopen(gBookmarkFile, _T("w"));
                           gGeneratedByUs = true;
                        }
                        fclose(bmFile);

                        SetWindowText(hBookmarksFileWnd, tempPath);
                        SetFocus(hBookmarksFileWnd);
                     }
                  }
                  break;
			   case IDOK: {
                  gToolbarEnabled = SendDlgItemMessage(hWnd, IDC_REBARENABLED, BM_GETCHECK, 0, 0);
                  kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_TOOLBAR_ENABLED, &gToolbarEnabled, FALSE);

				  TCHAR tempPath[MAX_PATH];
				  GetDlgItemText(hWnd, IDC_BOOKMARKS_FILE, tempPath, MAX_PATH); 
				  if (_tcsicmp(tempPath, gBookmarkFile) != 0) {
					 GetDlgItemText(hWnd, IDC_BOOKMARKS_FILE, gBookmarkFile, MAX_PATH);
					 kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, false);
					 gBookmarkDefFile = false;
					 delete gBookmarkRoot.child;
					 delete gBookmarkRoot.next;
					 gBookmarkRoot.child = NULL;
					 gBookmarkRoot.next = NULL;
					 FILE *bmFile = _tfopen(gBookmarkFile, _T("a"));
					 if (bmFile)
						fclose(bmFile);
					 else {
						 MessageBox(NULL, BOOKMARKS_CREATING_NEW, _T(PLUGIN_NAME), 0);
						 bmFile = _tfopen(gBookmarkFile, _T("w"));
						 if (bmFile) {
							fclose(bmFile);
							gGeneratedByUs = true;
						 }

					 }
					 LoadBM(gBookmarkFile);
				  }
                  gMaxMenuLength = GetDlgItemInt(hWnd, IDC_MAX_MENU_LENGTH, NULL, false);
                  if (gMaxMenuLength < 1) gMaxMenuLength = 20;
                  kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MAX_MENU_LENGTH, &gMaxMenuLength, FALSE);

                  gMenuAutoDetect = SendDlgItemMessage(hWnd, IDC_MENU_AUTODETECT, BM_GETCHECK, 0, 0);
                  kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_MENU_AUTODETECT, &gMenuAutoDetect, FALSE);

                  gMaxTBSize = GetDlgItemInt(hWnd, IDC_MAX_TB_SIZE, NULL, false);
                  if (gMaxTBSize < 1) gMaxTBSize = 20;
                  kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_MAX_TB_SIZE, &gMaxTBSize, FALSE);

                  // rebuild menu and toolbars to provide instant gratification to users
                  Rebuild();
				 }
                  // fall through...
               case IDCANCEL:
                  SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;
      case WM_CLOSE:
         EndDialog(hWnd, (INT_PTR) NULL);
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
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
   ofn.lpstrTitle = PLUGIN_NAME;

   if (GetOpenFileName(&ofn)) {
      return true;
   }
   else {
      return false;
   }
}


int GlobalReplace(char *str, char *a, char *b) {
  char *prev, *p, *q;
  int i = 0;

  p = q = prev = strdup(str);
  *str = 0;

  while (p && *p && (p = strstr(p, a)) != NULL) {
    i++;
    *p = 0;
    strcat(str, q);
    strcat(str, b);
    p += strlen(a);
    q = p;
  }

  if (q && *q)
    strcat(str, q);
  free(prev);

  return i;
}

char *EncodeQuotes(const char *str) {
  char *pszStr = (char *)malloc(strlen(str) + INTERNET_MAX_URL_LENGTH);
  if (pszStr) {
    strcpy(pszStr, str);
    GlobalReplace(pszStr, "\"", "%22");
  }
  return pszStr;
}

char *EncodeString(const char *str) {
  char *pszStr = (char *)malloc(strlen(str) + INTERNET_MAX_URL_LENGTH);
  if (pszStr) {
    strcpy(pszStr, str);
    GlobalReplace(pszStr, "&",  "&amp;");
    GlobalReplace(pszStr, "<",  "&lt;");
    GlobalReplace(pszStr, ">",  "&gt;");
    GlobalReplace(pszStr, "\"", "&quot;");
    GlobalReplace(pszStr, "\'", "&apos;");
  }
  return pszStr;
}

char *DecodeQuotes(const char *str) {
  char *pszStr = (char *)malloc(strlen(str) + INTERNET_MAX_URL_LENGTH);
  if (pszStr) {
    strcpy(pszStr, str);
    GlobalReplace(pszStr, "%22", "\"");
  }
  return pszStr;
}

char *DecodeString(const char *str) {
  char *pszStr = (char *)malloc(strlen(str) + INTERNET_MAX_URL_LENGTH);
  if (pszStr) {
    strcpy(pszStr, str);
    GlobalReplace(pszStr, "&amp;",  "&");
    GlobalReplace(pszStr, "&lt;",   "<");
    GlobalReplace(pszStr, "&gt;",   ">");
    GlobalReplace(pszStr, "&quot;", "\"");
    GlobalReplace(pszStr, "&apos;", "\'");
  }
  return pszStr;
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
         char *psz, *psz2;
         char szFolderFlags[1024] = {0};
         if (child->flags & BOOKMARK_FLAG_GRP)
            strcat(szFolderFlags, "FOLDER_GROUP=\"true\" ");
         if (child->nick.c_str() && *(child->nick.c_str())) {
	    strcat(szFolderFlags, "SHORTCUTURL=\"");
	    psz = kPlugin.kFuncs->EncodeUTF8(child->nick.c_str());
	    strcat(szFolderFlags, psz ? psz : "");
	    if (psz) free(psz);
	    strcat(szFolderFlags, "\" ");
	 }
         if (child->flags & BOOKMARK_FLAG_TB)
            strcat(szFolderFlags, "PERSONAL_TOOLBAR_FOLDER=\"true\" ID=\"NC:PersonalToolbarFolder\" ");
         if (child->flags & BOOKMARK_FLAG_NB)
            strcat(szFolderFlags, "NEWITEMHEADER ");
         if (child->flags & BOOKMARK_FLAG_BM)
            strcat(szFolderFlags, "MENUHEADER ");
         psz = EncodeString(child->text.c_str());
         psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
         fprintf(bmFile, "%s<DT><H3 %sADD_DATE=\"%d\">%s</H3>\n", szSpacer, szFolderFlags, child->addDate, psz2 ? psz2 : "");
         if (psz) free(psz);
         if (psz2) free(psz2);
         if (child->desc.c_str() != NULL && *(child->desc.c_str()) != 0) {
            psz = EncodeString(child->desc.c_str());
            psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
            fprintf(bmFile, "%s<DD>%s\n", szSpacer, psz2 ? psz2 : "");
            if (psz) free(psz);
            if (psz2) free(psz2);
         }

         if (strlen(szSpacer) < MAXSPACER) szSpacer[strlen(szSpacer)] = ' ';   // add a space
         SaveBookmarks(bmFile, child);
         szSpacer[strlen(szSpacer)-1] = 0;      // remove the space
      }
      else if (type == BOOKMARK_SEPARATOR) {
         fprintf(bmFile, "%s<HR>\n", szSpacer);
      }
      else if (type == BOOKMARK_BOOKMARK) {
         char *psz;
	 char *psz2;
         fprintf(bmFile, "%s<DT><A", szSpacer);
         psz = EncodeQuotes(child->url.c_str());
         psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
         fprintf(bmFile, " HREF=\"%s\"", psz2 ? psz2 : "");
         if (psz)  free(psz);
         if (psz2) free(psz2);
         fprintf(bmFile, " ADD_DATE=\"%d\"", child->addDate);
         fprintf(bmFile, " LAST_VISIT=\"%d\"", child->lastVisit);
         fprintf(bmFile, " LAST_MODIFIED=\"%d\"", child->lastModified);
         psz = (char *) child->nick.c_str();
         if (psz && *psz) {
            psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
            fprintf(bmFile, " SHORTCUTURL=\"%s\"", psz2 ? psz2 : "");
	    if (psz2) free(psz2);
	 }
         psz = (char *) child->charset.c_str();
         if (psz && *psz)
            fprintf(bmFile, " LAST_CHARSET=\"%s\"", psz);
	 psz = EncodeString(child->text.c_str());
	 psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
         fprintf(bmFile, ">%s</A>\n", psz2 ? psz2 : "");
	 if (psz) free(psz);
	 if (psz2) free(psz2);
         if (child->desc.c_str() != NULL && *(child->desc.c_str()) != 0) {
            psz = EncodeString(child->desc.c_str());
            psz2 = kPlugin.kFuncs->EncodeUTF8(psz ? psz : "");
            fprintf(bmFile, "%s<DD>%s\n", szSpacer, psz2 ? psz2 : "");
            if (psz) free(psz);
            if (psz2) free(psz2);
         }
      }
      // if it falls through, there's a problem, but we'll just ignore it for now.
   }
   szSpacer[strlen(szSpacer)-1] = 0;      // remove the space
   fprintf(bmFile, "%s</DL><p>\n", szSpacer);
}

static BOOL bBackedUp = 0;

static void create_backup(const char *file, int num=2)
{
   int i;
   char buf[MAX_PATH];
   char buf2[MAX_PATH];
   
   if (bBackedUp)
      return;

   /* rotate the old backups */
   for (i=num; i>=1; i--) {
      sprintf(buf, "%s.bak%d", file, i);
      sprintf(buf2, "%s.bak%d", file, i+1);
      unlink(buf2);
      rename(buf, buf2);
   }

   sprintf(buf, "%s.bak1", file);
   unlink(buf);
   if (num)
     rename(file, buf);
   else
     unlink(file);

   bBackedUp = 1;
}

void SaveBM(const char *file)
{
   if (!gBookmarkRoot.child && !gBookmarkRoot.next) {
      if (MessageBox(NULL, "The bookmarks tree is empty.\nDo you really want to erase all your bookmarks?", PLUGIN_NAME ": WARNING" , MB_YESNO|MB_ICONEXCLAMATION) != IDYES) {
         LoadBM(gBookmarkFile);
         if (!gBookmarkRoot.child && !gBookmarkRoot.next) {
            MessageBox(NULL, "Unable to recover old bookmarks.", PLUGIN_NAME ": BUG WARNING" , MB_OK|MB_ICONSTOP);
            return;
         }
      }
   }

   if (!gGeneratedByUs) {
      if (MessageBox(NULL, BOOKMARKS_NOT_BY_US, PLUGIN_NAME, MB_YESNO) != IDYES) {
         return;
      }
   }

   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
   if (dwWaitResult == WAIT_TIMEOUT) {
     MessageBox(NULL, "Unable to get MutEx for bookmarks file.\\nFile not saved.", PLUGIN_NAME ": WARNING" , MB_OK|MB_ICONSTOP);
     return;
   }

   char buf[MAX_PATH];
   strcpy(buf, file);
   char *p, *q;
   p = strrchr(buf, '/');
   q = strrchr(buf, '\\');
   if (!q || p>q) q = p;
   p = strrchr(buf, '.');
   if (!p || q>p)
     strcat(buf, "XXXXXX");
   else if (p)
     strcat(p, "XXXXXX");
   else
     strcat(buf, "XXXXXX");
   p = mktemp(buf);

   FILE *bmFile = fopen(buf, "w");
   if (bmFile){
      fprintf(bmFile, "%s\n", BOOKMARK_TAG);
      fprintf(bmFile, "%s\n", KMELEON_TAG);
      fprintf(bmFile, "%s\n", COMMENT_TAG);
// FIXME - figure out if this needs to be here, or if it should be different on non-US-English systems
//      fprintf(bmFile, "%s\n", CONTENT_TYPE_TAG);
      if (*szContentType)
         fprintf(bmFile, "%s%s>\n", CONTENT_TYPE_TAG, szContentType);
      fprintf(bmFile, "<TITLE>%s</TITLE>\n", gBookmarksTitle);
      fprintf(bmFile, "<H1>%s</H1>\n\n", gBookmarksTitle);

      SaveBookmarks(bmFile, &gBookmarkRoot);

      fclose(bmFile);

#if 0
   create_backup(file);
#endif

      unlink(file);
      rename(buf, file);
   }

   gGeneratedByUs = true;
   gBookmarksModified = false;

   /* this is to support both NS 4 and NS 6 style bookmarks */
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, (void *)gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_TB)->text.c_str(), FALSE);

   ReleaseMutex(ghMutex);
}


// Load bookmarks
void ParseBookmarks(char *bmFileBuffer, CBookmarkNode &node)
{
   char *p;
   char *t;
   bool found_tb = false;
   CBookmarkNode * lastNode = &node;

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

         char *nick = NULL;
         d = strstr(t, "SHORTCUTURL=\"");
         if (d) {
            d+=13;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               nick = kPlugin.kFuncs->DecodeUTF8(d);
               *q = '\"';
            }
         }

	 char *pszTxt;
	 char *psz, *psz2;
	 psz = kPlugin.kFuncs->DecodeUTF8(name);
	 psz2 = DecodeString(psz ? psz : "");
	 if (psz2 && *psz2)
	   pszTxt = psz2;
	 else
	   pszTxt = DecodeString(name);
	 if (psz) free(psz);
	 if (psz2 && psz2 != pszTxt) free(psz2);

         CBookmarkNode * newNode = new CBookmarkNode(0, pszTxt, "", nick, "", "", BOOKMARK_FOLDER, addDate);
         node.AddChild(newNode);
         if (pszTxt) free(pszTxt);
         if (nick)   free(nick);
         lastNode = newNode;

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

         if ( strstr(t, _Q(FOLDER_GROUP="true")) )
         {
            newNode->flags |= BOOKMARK_FLAG_GRP;
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
         char *nick = NULL;
         char *charset = NULL;

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
         d = strstr(t, "SHORTCUTURL=\"");
         if (d) {
            d+=13;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               nick = kPlugin.kFuncs->DecodeUTF8(d);
               *q = '\"';
            }
         }
         d = strstr(t, "LAST_CHARSET=\"");
         if (d) {
            d+=14;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               charset = strdup(d);
               *q = '\"';
            }
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

	 char *pszTxt, *pszUrl;
	 char *psz, *psz2;
	 psz = kPlugin.kFuncs->DecodeUTF8(url);
	 pszUrl = DecodeQuotes(psz ? psz : "");
	 if (psz) free(psz);

	 psz = kPlugin.kFuncs->DecodeUTF8(name);
	 psz2 = DecodeString(psz ? psz : "");
	 if (psz2 && *psz2)
	   pszTxt = psz2;
	 else
	   pszTxt = DecodeString(name);
	 if (psz) free(psz);
	 if (psz2 && psz2 != pszTxt) free(psz2);

         lastNode = new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), pszTxt, pszUrl, nick, NULL, charset, BOOKMARK_BOOKMARK, addDate, lastVisit, lastModified);
         node.AddChild(lastNode);
         if (pszUrl) free(pszUrl);
         if (pszTxt) free(pszTxt);
         
         if (nick)    free(nick);
         if (charset) free(charset);
      }
      else if ((t = strstr(p, "</DL>")) != NULL) {
         return;
      }
      else if ((t = strstr(p, "<HR>")) != NULL) {
         lastNode = new CBookmarkNode(0, "", "", "", "", "", BOOKMARK_SEPARATOR);
         node.AddChild(lastNode);
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
      else if ((t = strstr(p, "<DD>")) != NULL && lastNode != NULL) {
         char *e = t+4;
         while (*e && *e!='\n')
            e++;
         *e = 0;
         e = kPlugin.kFuncs->DecodeUTF8(t+4);
	 if (e && *e) {
	   char *tmp = DecodeString(e);
	   free(e);
	   e = tmp;
	 }
	 if (!e || !*e) {
	   if (e)
	     free(e);
	   e = DecodeString(t+4);
	 }
         lastNode->desc = e ? e : "";
         if (e)
            free(e);
      }
      else if (strstr(p, KMELEON_TAG) != NULL) {
         gGeneratedByUs = true;
      }
      else if ((t = strstr(p, CONTENT_TYPE_TAG)) != NULL) {
         t += strlen(CONTENT_TYPE_TAG);
         strncpy(szContentType, t, sizeof(szContentType)-1);
         szContentType[sizeof(szContentType)-1] = 0;
         t = strchr(szContentType, '>');
         if (t)
            *t = 0;
      }
   }
   return;
}

void LoadBM(const char *file) 
{
   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
   if (dwWaitResult == WAIT_TIMEOUT) {
     MessageBox(NULL, "Unable to get MutEx for bookmarks file.\\nFile not loaded.", PLUGIN_NAME ": WARNING" , MB_OK|MB_ICONSTOP);
     return;
   }

   FILE *bmFile = fopen(file, "r");
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

   ReleaseMutex(ghMutex);
}

void findNick(char *nick, char **url)
{
   CBookmarkNode *retNode = (*nick ? gBookmarkRoot.FindNick(nick) : NULL);
   
   if (retNode) {
      if (retNode->type == BOOKMARK_BOOKMARK) {
	if (*url)
	  free(*url);
         *url = (char *) malloc(INTERNET_MAX_URL_LENGTH+1);
	 strcpy(*url, (char*)retNode->url.c_str());
      }
      else if (retNode->type == BOOKMARK_FOLDER) {
         CBookmarkNode *c = retNode->child;
	 int len = 0;
         while (c) {
            if (c->type == BOOKMARK_BOOKMARK && c->url.c_str())
               len += strlen(c->url.c_str()) + 1;
            c = c->next;
         }

	 if (!len) return;
         char *pUrl = (char *)malloc(len+1);
	 *url = pUrl;

         c = retNode->child;
         while (c) {
            if (c->type == BOOKMARK_BOOKMARK && c->url.c_str()) {
               strcpy(pUrl, c->url.c_str());
               pUrl += strlen(pUrl);
               *pUrl++ = '\t';
            }
            c = c->next;
         }
         *pUrl = 0;
      }
   }
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

#if 1
         // condense the title and escape ampersands
         char *pszTemp = fixString(child->text.c_str(), 40);
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, pszTemp);
         delete pszTemp;
#else         
         char *szTitle = (char*) child->text.c_str();
         int len = strlen(szTitle)+1;

         int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                        szTitle, len,
                                        NULL, 0);
         WCHAR *wszTitle = new WCHAR[wlen+1];
         wlen = MultiByteToWideChar(CP_UTF8, 0,
                                    szTitle, len,
                                    wszTitle, wlen);
         AppendMenuW(menu, MF_STRING|MF_POPUP, (UINT)childMenu, wszTitle);
         delete wszTitle;
#endif

         BuildMenu(childMenu, child, false);
      }
      else if (child->type == BOOKMARK_BOOKMARK) {
#if 1
         // condense the title and escape ampersands
		 if (!child->text.empty()) // BUG #785
		 {
			char *pszTemp = fixString(child->text.c_str(), 40);
			AppendMenu(menu, MF_STRING, child->id, pszTemp);
			delete pszTemp;
		 }
#else
         char *szTitle = (char*) child->text.c_str();
         int len = strlen(szTitle)+1;

         int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                        szTitle, len,
                                        NULL, 0);
         WCHAR *wszTitle = new WCHAR[wlen+1];
         wlen = MultiByteToWideChar(CP_UTF8, 0,
                                    szTitle, len,
                                    wszTitle, wlen);
         AppendMenuW(menu, MF_STRING, child->id, wszTitle);
         delete wszTitle;
#endif
      }
   }
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "SetOwnerDrawn", (long)menu, (long)DrawBitmap);
}

void CopyRebar(HWND hWndNewTB, HWND hWndOldTB)
{
  int i = 0;
  TBBUTTON button;

  SetWindowText(hWndNewTB, TOOLBAND_NAME);
  SendMessage(hWndNewTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
  SendMessage(hWndNewTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

  while (SendMessage(hWndOldTB, TB_GETBUTTON, i, (LPARAM)&button)) {
    LONG lResult = SendMessage(hWndOldTB, TB_GETBUTTONTEXT, (WPARAM) button.idCommand, NULL);
    if (lResult >= 0) {
      char *szTmp = new char[lResult + 2];
      SendMessage(hWndOldTB, TB_GETBUTTONTEXT, (WPARAM) button.idCommand, (LPARAM) szTmp);
      szTmp[lResult+1] = 0;

      int stringID = SendMessage(hWndNewTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM) szTmp);
      button.iString = stringID;
      SendMessage(hWndNewTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);

      delete szTmp;
    }
    else
      SendMessage(hWndNewTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);

    ++i;
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

   if (bChevronEnabled) {
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
}

void Rebuild() {
   // delete the old bookmarks from the menu (FIXME - needs to be more robust than "delete everything after the first bookmark position" - there may be normal menu items there (if the user is weird))
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)gMenuBookmarks, 0);
   while (DeleteMenu(gMenuBookmarks, nFirstBookmarkPosition, MF_BYPOSITION));
   // and rebuild
   BuildMenu(gMenuBookmarks, gBookmarkRoot.FindSpecialNode(BOOKMARK_FLAG_BM), false);

#if 0
   // need to rebuild the rebar, too, in case it had submenus (whose ids are now invalid)
   if (ghWndTB) {
      // delete the old rebar
      while (SendMessage(ghWndTB, TB_DELETEBUTTON, 0 /*index*/, 0));
      // and rebuild
      BuildRebar(ghWndTB);
   }
#endif

// FIXME - Is this needed?  Hm, if anywhere, in WndProc, below, in the nAddCommand/nAddToolbarCommand/nEditCommand cases...  but then it's still only one window, and the others don't get the call, so...   heck, it works without it.  It will stay until someone complains.  :)
//   DrawMenuBar(hWnd);
}


int addLink(char *url, char *title, int flag)
{
   if (!url || !(*url))
      return false;

   CBookmarkNode *addNode = gBookmarkRoot.FindSpecialNode(flag);
   char *pszUrl = DecodeString(url);
   char *pszTxt = DecodeString(title ? (*title ? title : url) : url);
   addNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), pszTxt, pszUrl, "", "", "", BOOKMARK_BOOKMARK, time(NULL)));
   if (pszUrl)
      free(pszUrl);
   if (pszTxt)
      free(pszTxt);
   SaveBM(gBookmarkFile);
   
   Rebuild();

   return true;
}


static char szInput[256];
static char *pszTitle;
static char *pszPrompt;

BOOL CALLBACK
PromptDlgProc( HWND hwnd,
               UINT Message,
               WPARAM wParam,
               LPARAM lParam )
{
    switch (Message) {
      case WM_INITDIALOG:
         SetWindowText( hwnd, pszTitle ? pszTitle : "Smart bookmark" );
         SetDlgItemText(hwnd, IDC_SEARCHTEXT, pszPrompt ? pszPrompt : "");
        return TRUE;
      case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
           GetDlgItemText(hwnd, IDC_INPUT, szInput, 256);
           EndDialog( hwnd, IDOK );
           break;
        case IDCANCEL:
           EndDialog( hwnd, IDCANCEL );
           break;
        }
        break;

      default:
        return FALSE;
    }
    return TRUE;
}

void OpenURL(char *url)
{
    char szOpenURLcmd[80];
    
    kPlugin.kFuncs->GetPreference(PREF_STRING, PREFERENCE_BOOKMARKS_OPENURL, szOpenURLcmd, (char*)"");
    
    if (*szOpenURLcmd) {
        char *plugin = szOpenURLcmd;
        char *parameter = strchr(szOpenURLcmd, '(');
        if (parameter) {
            *parameter++ = 0;
            char *close = strchr(parameter, ')');
            if (close) {
                *close = 0;
                
                if (kPlugin.kFuncs->SendMessage(plugin, PLUGIN_NAME, parameter, (long)url, 0))
                    return;
            }
        }

        int idOpen = kPlugin.kFuncs->GetID(szOpenURLcmd);
		if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK")) {
            kPlugin.kFuncs->NavigateTo(url, OPEN_NORMAL, NULL);
            return;
		}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_BACKGROUND")) {
            kPlugin.kFuncs->NavigateTo(url, OPEN_BACKGROUND, NULL);
            return;
		}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_NEW_WINDOW")) {
            kPlugin.kFuncs->NavigateTo(url, OPEN_NEW, NULL);
            return;
        }
    }

    kPlugin.kFuncs->NavigateTo(url, OPEN_NORMAL, NULL);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;

   if (message == WM_SETFOCUS) {
      hWndFront = hWnd;
   }
   else if (message == WM_COMMAND) {
      WORD command = LOWORD(wParam);

      if (command == nConfigCommand) {
         Config(hWnd);
         return true;
      }
      else if (command == nAddCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
         if (dInfo) {
            addLink(dInfo->url, dInfo->title, BOOKMARK_FLAG_NB);
         }
         return true;
      }
      else if (command == nAddLinkCommand) {
         char *title = 0;
         char *url = 0;
         int retLen = kPlugin.kFuncs->GetGlobalVar(PREF_STRING, "LinkURL", NULL);
         if (retLen) {
            char *retVal = new char[retLen+1];
            kPlugin.kFuncs->GetGlobalVar(PREF_STRING, "LinkURL", retVal);
            url = title = retVal;
            addLink(url, title, BOOKMARK_FLAG_NB);
            delete retVal;
         }
         return true;
      }
      else if (command == nAddToolbarCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
         if (dInfo) {
            addLink(dInfo->url, dInfo->title, BOOKMARK_FLAG_TB);
         }
         return true;
      }
      else if (command == nEditCommand) {
         if (ghWndEdit) {
            ShowWindow(ghWndEdit, SW_RESTORE);
            BringWindowToTop(ghWndEdit);
         }
         else
            ghWndEdit = CreateDialogParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), NULL, EditProc, 0);
         return true;
      }
      else if (command == wm_deferbringtotop) {
         BringWindowToTop(hWnd);
         return true;
      }
      else if (CBookmarkNode *node = gBookmarkRoot.FindNode(command)) {
         node->lastVisit = time(NULL);
         gBookmarksModified = true; // this doesn't call for instant saving, it can wait until we add/edit/quit

         if (node->url.c_str() == NULL || *node->url.c_str() == 0)
            return true;

         char *str = strdup(node->url.c_str());
         char *ptr = strstr(str, "%s");
         if (ptr) {
            char buff[INTERNET_MAX_URL_LENGTH];
            *ptr = 0;
            strcpy(buff, str);
            ptr += 2;

            pszTitle = strdup( node->text.c_str() );
            pszPrompt = strdup( node->desc.c_str() );

            int ok = DialogBox(kPlugin.hDllInstance,
                               MAKEINTRESOURCE(IDD_SMARTBOOKMARK), hWnd, (DLGPROC)PromptDlgProc);
            PostMessage(hWnd, WM_NULL, 0, 0);
            if (ok == IDOK && *szInput) {
               strcat(buff, szInput);
               strcat(buff, ptr);
               OpenURL(buff);
            }

            if (pszTitle) 
               free(pszTitle);
            if (pszPrompt) 
               free(pszPrompt);

         }
         else {
            OpenURL(str);
         }
         free(str);

         return true;
      }
   }
   else if (message == WM_NOTIFY) {
      hdr = *((LPNMHDR)lParam);
      if (hdr.code == (UINT) TBN_DROPDOWN) {
         tbhdr = *((LPNMTOOLBAR)lParam);

         // this is the little down arrow thing
         if (tbhdr.iItem == (int) nDropdownCommand){
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
            PostMessage(hWnd, WM_DEFERHOTTRACK, (WPARAM) NULL, (LPARAM) NULL);

            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   else if (message == WM_DEFERHOTTRACK) {
      BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
      return true;
   }
   else if (message == WM_MENUSELECT) {
      UINT id = LOWORD(wParam);
      if (id >= nConfigCommand && id < nDropdownCommand) {
         if (id == nConfigCommand)
            kPlugin.kFuncs->SetStatusBarText("Configure the bookmarks plugin");
         else if (id == nAddCommand)
            kPlugin.kFuncs->SetStatusBarText("Add to bookmarks");
         else if (id == nAddLinkCommand)
            kPlugin.kFuncs->SetStatusBarText("Add link to bookmarks");
         else if (id == nEditCommand)
            kPlugin.kFuncs->SetStatusBarText("Edit the bookmarks");
         return true;
      } 
      else if (CBookmarkNode *node = gBookmarkRoot.FindNode(LOWORD(wParam))) {
         kPlugin.kFuncs->SetStatusBarText(node->url.c_str());
         return true;
      }
// this would be a hack to clean the status bar for separators, popups
// (they work (clear the status bar) for CMenu-added separators/popups,
// but not when they are added via standard win32 calls...)
      else {
         kPlugin.kFuncs->SetStatusBarText("");
      }
   }
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}
