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
#include <tchar.h>
#include "../utf.h"
#include "../LocalesUtils.h"
#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"
#include "../KMeleonConst.h"
#include "../Utils.h"
#include "../DialogUtils.h"
#include "../rebar_menu/hot_tracking.h"
#define WM_DEFERHOTTRACK WM_USER+11

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

extern WNDPROC KMeleonWndProc;
extern BOOL bChevronEnabled;
char szContentType[BOOKMARKS_TITLE_LEN];
BOOL gLoaded = FALSE;

UINT GetSiteIcon(CBookmarkNode *node)
{
   if (node->iconurl.length()) {
      UINT idx = kPlugin.kFuncs->GetIconIdx(node->iconurl.c_str());
	  if (idx>0) return idx;
   }

   char* url = (char*)node->url.c_str();
   char* begin = strstr(url, "://");
   if (!begin) return 0;
   char* end = strchr(begin+3, '/');
   if (end) *end = 0;
   UINT i = kPlugin.kFuncs->GetIconIdx(url);
   if (end) *end = '/';
   return i;
}

void InitImageList(HIMAGELIST& imageList)
{
   if (imageList)
      ImageList_Destroy(imageList);

   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;
   COLORREF bgCol = RGB(255, 0, 255);

   TCHAR szFullPath[MAX_PATH];
   FindSkinFile(szFullPath, _T("bookmarks.bmp"));
   FILE *fp = _tfopen(szFullPath, _T("r"));
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
   imageList = ImageList_Create(bmp.bmWidth/6, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
   if (imageList && bitmap)
      ImageList_AddMasked(imageList, bitmap, bgCol);
   if (bitmap)
      DeleteObject(bitmap);
}

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
					 kPlugin.kFuncs->SetPreference(PREF_TSTRING, PREFERENCE_BOOKMARK_FILE, gBookmarkFile, false);
					 gBookmarkDefFile = false;
					 delete gBookmarkRoot->child;
					 delete gBookmarkRoot->next;
					 gBookmarkRoot->child = NULL;
					 gBookmarkRoot->next = NULL;
					 FILE *bmFile = _tfopen(gBookmarkFile, _T("a"));
					 if (bmFile)
						fclose(bmFile);
					 else {
						 MessageBox(NULL, gLoc->GetString(IDS_CREATING_NEW), _T(PLUGIN_NAME), 0);
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

BOOL BrowseForBookmarks(TCHAR *file)
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
   TCHAR* filter = _tcsdup(gLoc->GetString(IDS_FILTER));
   for (TCHAR* p = filter;*p;p++)
	   if (*p == '|') *p=0;
   ofn.lpstrFilter = filter;
   ofn.lpstrFile = file;
   ofn.nMaxFile = MAX_PATH;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
   ofn.lpstrTitle = _T(PLUGIN_NAME);

   if (GetOpenFileName(&ofn)) {
      free(filter);
      return true;
   }
   else {
	  free(filter);
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
  char *pszStr = (char *)malloc(strlen(str)*3+1);
  if (pszStr) {
    strcpy(pszStr, str);
    GlobalReplace(pszStr, "\"", "%22");
  }
  return pszStr;
}

char *EncodeString(const char *str) {
  char *pszStr = (char *)malloc(strlen(str)*6+1);
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
	    strcat(szFolderFlags, child->nick.c_str());
	    strcat(szFolderFlags, "\" ");
	 }
		 if (child->m_id.length()) {
			strcat(szFolderFlags, "ID=\"");
			strcat(szFolderFlags, child->m_id.c_str());
			strcat(szFolderFlags, "\" ");
		 }
		
         if (child->flags & BOOKMARK_FLAG_TB)
            strcat(szFolderFlags, "PERSONAL_TOOLBAR_FOLDER=\"true\" "); //ID=\"NC:PersonalToolbarFolder\" ");
         if (child->flags & BOOKMARK_FLAG_NB)
            strcat(szFolderFlags, "NEWITEMHEADER ");
         if (child->flags & BOOKMARK_FLAG_BM)
            strcat(szFolderFlags, "MENUHEADER ");
         psz = EncodeString(child->text.c_str());
	     fprintf(bmFile, "%s<DT><H3 %sADD_DATE=\"%ld\">", szSpacer, szFolderFlags, child->addDate);
	     fprintf(bmFile, "%s</H3>\n", psz);		 
         if (psz) free(psz);
         
         if (child->desc.c_str() != NULL && *(child->desc.c_str()) != 0) {
            psz = EncodeString(child->desc.c_str());
            fprintf(bmFile, "%s<DD>%s\n", szSpacer, psz ? psz : "");
            if (psz) free(psz);
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
	 
         fprintf(bmFile, "%s<DT><A", szSpacer);
         psz = EncodeQuotes(child->url.c_str());
         fprintf(bmFile, " HREF=\"%s\"", psz ? psz : "");
         if (psz)  free(psz);
        
         fprintf(bmFile, " ADD_DATE=\"%ld\"", child->addDate);
         fprintf(bmFile, " LAST_VISIT=\"%ld\"", child->lastVisit);
         fprintf(bmFile, " LAST_MODIFIED=\"%ld\"", child->lastModified);
		 if (child->m_id.length())
			fprintf(bmFile, " ID=\"%s\"", child->m_id.c_str());
		 if (child->icon.length())
			fprintf(bmFile, " ICON=\"%s\"", child->icon.c_str());
		 if (child->iconurl.length())
			fprintf(bmFile, " ICONURL=\"%s\"", child->iconurl.c_str());
		 if (child->feedurl.length())
			fprintf(bmFile, " FEEDURL=\"%s\"", child->feedurl.c_str());
         psz = (char *) child->nick.c_str();
         if (psz && *psz) {
            fprintf(bmFile, " SHORTCUTURL=\"%s\"", psz);

         }
         psz = (char *) child->charset.c_str();
         if (psz && *psz)
            fprintf(bmFile, " LAST_CHARSET=\"%s\"", psz);
         psz = EncodeString(child->text.c_str());
         fprintf(bmFile, ">%s</A>\n", psz ? psz : "");
         if (psz) free(psz);

         if (child->desc.c_str() != NULL && *(child->desc.c_str()) != 0) {
            psz = EncodeString(child->desc.c_str());
            fprintf(bmFile, "%s<DD>%s\n", szSpacer, psz ? psz : "");
            if (psz) free(psz);
         }
      }
      // if it falls through, there's a problem, but we'll just ignore it for now.
   }
   szSpacer[strlen(szSpacer)-1] = 0;      // remove the space
   fprintf(bmFile, "%s</DL><p>\n", szSpacer);
}

#include <sys/stat.h>
static void create_backup(const TCHAR *file, int num=2)
{
	static BOOL bBackedUp = 0;

	if (bBackedUp)
      return;

	TCHAR buf[MAX_PATH];
	_tcscpy(buf, file);
	TCHAR* dot = _tcsrchr(buf, '.');
	if (dot)
		_tcscpy(dot, _T("_backup.html"));
	else
		_tcscat(buf, _T("_backup"));
   
	struct _stat s = {0};
	if (_tstat(buf, &s)!=-1 && s.st_mtime > time(NULL) - 172800)
		return;

   _tunlink(buf);
   _trename(file, buf);
   
   bBackedUp = 1;
}

void SaveBM(const TCHAR *file)
{
	if (!gLoaded)
		return;

   if (!gBookmarkRoot->child && !gBookmarkRoot->next) {
      if (MessageBox(NULL, _T("The bookmarks tree is empty.\nDo you really want to erase all your bookmarks?"), _T(PLUGIN_NAME) _T(": WARNING") , MB_YESNO|MB_ICONEXCLAMATION) != IDYES) {
         LoadBM(gBookmarkFile);
         if (!gBookmarkRoot->child && !gBookmarkRoot->next) {
            MessageBox(NULL, _T("Unable to recover old bookmarks."), _T(PLUGIN_NAME) _T(": BUG WARNING") , MB_OK|MB_ICONSTOP);
            return;
         }
      }
   }

   BOOL warn = TRUE;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.bookmarks.warnOnSave", &warn, &warn);
   if (!gGeneratedByUs && warn) {
      if (MessageBox(NULL, gLoc->GetString(IDS_NOT_BY_US), _T(PLUGIN_NAME), MB_YESNO) != IDYES) {
         return;
      }
   }

   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
   if (dwWaitResult == WAIT_TIMEOUT) {
     MessageBox(NULL, _T("Unable to get MutEx for bookmarks file.\\nFile not saved."), _T(PLUGIN_NAME) _T(": WARNING") , MB_OK|MB_ICONSTOP);
     return;
   }

   TCHAR buf[MAX_PATH];
   _tcscpy(buf, file);
   TCHAR *p, *q;
   p = _tcsrchr(buf, _T('/'));
   q = _tcsrchr(buf, _T('\\'));
   if (!q || p>q) q = p;
   p = _tcsrchr(buf, _T('.'));
   if (!p || q>p)
     _tcscat(buf, _T("XXXXXX"));
   else if (p)
     _tcscat(p, _T("XXXXXX"));
   else
     _tcscat(buf, _T("XXXXXX"));
   p = _tmktemp(buf);

   FILE *bmFile = _tfopen(buf, _T("w"));
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

      SaveBookmarks(bmFile, gBookmarkRoot);

      if (fclose(bmFile) == EOF) {
		 _tunlink(buf);
         MessageBox(NULL, gLoc->GetString(IDS_FAILED_SAVE), gLoc->GetString(IDS_ERROR), MB_OK|MB_ICONERROR);
         ReleaseMutex(ghMutex);
         return;
	  }

   create_backup(file);

      _tunlink(file);
      _trename(buf, file);
   }

   gGeneratedByUs = true;
   gBookmarksModified = false;

   /* this is to support both NS 4 and NS 6 style bookmarks */
   kPlugin.kFuncs->SetPreference(PREF_STRING, PREFERENCE_TOOLBAR_FOLDER, (void*)gBookmarkRoot->FindSpecialNode(BOOKMARK_FLAG_TB)->text.c_str(), false);

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
         char *_name = strchr(t, '>');
         if (_name) {
            *_name = 0; _name++;
         }
         else {
            _name = "";
         }

         char *end = strrchr(_name, '<');
         if (end) *end = 0;

         time_t addDate=0;
		 char *id = NULL;

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
               nick = strdup(d);
               *q = '\"';
            }
         }

		 d = strstr(t, "ID=\"");
         if (d) {
            d+=4;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               id = strdup(d);
               *q = '\"';
            }
         }

	     char* name = DecodeString(_name ? _name : "");	 

         CBookmarkNode * newNode = new CBookmarkNode(0, name, "", nick, "", "", BOOKMARK_FOLDER, addDate, 0, 0, id);
         node.AddChild(newNode);
         
         if (nick)   free(nick);
		 if (id)     free(id);
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

		 if (name) free(name);

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
		 char *id = NULL;
		 char *icon = NULL;
		 char *iconurl = NULL;
		 char *feedurl = NULL;
         
         char *d;
         d = strstr(t, "ID=\"");
         if (d) {
            d+=4;
            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               id = strdup(d);
               *q = '\"';
            }
         }
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
               nick = strdup(d);
               *q = '\"';
            }
         }
		 d = strstr(t, "ICON=\"");
         if (d) {
            d+=6;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               icon = strdup(d);
               *q = '\"';
            }
         }
		 d = strstr(t, "ICONURL=\"");
         if (d) {
            d+=9;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               iconurl = strdup(d);
               *q = '\"';
            }
         }

		 d = strstr(t, "FEEDURL=\"");
         if (d) {
            d+=9;

            char *q = strchr(d, '\"');
            if (q) {
               *q = 0;
               feedurl = strdup(d);
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
         pszUrl = DecodeQuotes(url ? url : "");
         pszTxt = DecodeString(name ? name : "");

	     lastNode = new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), pszTxt, pszUrl, nick, NULL, charset, BOOKMARK_BOOKMARK, addDate, lastVisit, lastModified, id, feedurl, icon, iconurl);
         node.AddChild(lastNode);
         if (pszUrl) free(pszUrl);
         if (pszTxt) free(pszTxt);
         if (id)      free(id);
         if (nick)    free(nick);
         if (charset) free(charset);
		 if (feedurl) free(feedurl);
		 if (icon)    free(icon);
		 if (iconurl)    free(iconurl);
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
         e = strdup(t+4);
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

void LoadBM(const TCHAR *file) 
{
   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
   if (dwWaitResult == WAIT_TIMEOUT) {
     MessageBox(NULL, _T("Unable to get MutEx for bookmarks file.\\nFile not loaded."), _T(PLUGIN_NAME) _T(": WARNING") , MB_OK|MB_ICONSTOP);
     return;
   }

   FILE *bmFile = _tfopen(file, _T("r"));
   if (bmFile){
      long bmFileSize = FileSize(bmFile);
      if (bmFileSize) {
         char *bmFileBuffer = new char[bmFileSize];
         if (bmFileBuffer){
            size_t s = fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);
            bmFileBuffer[s] = 0;
            strtok(bmFileBuffer, "\n");
            ParseBookmarks(bmFileBuffer, *gBookmarkRoot);
            gLoaded = TRUE;
            delete [] bmFileBuffer;
         }
      fclose(bmFile);
	  }
   }

   ReleaseMutex(ghMutex);
}

void findNick(char *nick, char **url)
{
   static char* sUrl = 0;
   CBookmarkNode *retNode = (*nick ? gBookmarkRoot->FindNick(nick) : NULL);
   if (sUrl) { 
	   free(sUrl);
	   sUrl = 0;
   }
   if (retNode) {
      if (retNode->type == BOOKMARK_BOOKMARK) {
         
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
	  sUrl = *url;
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
int cmenu = max(18,GetSystemMetrics(SM_CYMENU));

// space to allow above menu for title bar, menu bar (assuming maximized window), and extra frame junk
#define MENUPADDING 50

   int maxLength = (gMenuAutoDetect) ? (int)((cy-MENUPADDING)/cmenu) : gMaxMenuLength;

   CBookmarkNode *child;
   int count = GetMenuItemCount(menu);

   for (child = (isContinuation) ? node : node->child ; child ; child = child->next , count++) {
      if (count == maxLength) {
         HMENU childMenu = CreatePopupMenu();
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, _T("[more]"));
         BuildMenu(childMenu, child, true);
         break;
      }
      else if (child->type == BOOKMARK_SEPARATOR) {
         AppendMenu(menu, MF_SEPARATOR, 0, _T(""));
      }
      else if (child->type == BOOKMARK_FOLDER) {
         HMENU childMenu = CreatePopupMenu();
         child->id = (UINT)childMenu; // we have to save off the HMENU for the rebar

#if 1
         // condense the title and escape ampersands
         TCHAR *pszTemp = fixString(CUTF8_to_T(child->text.c_str()), 40);
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
		 BOOL addBookmark = TRUE;
		 kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_BOOKMARK_MENUADD, &addBookmark, &addBookmark);
		 if (addBookmark) {
			AppendMenu(childMenu, MF_SEPARATOR, 0, _T(""));
			AppendMenu(childMenu, MF_STRING, nAddBookmarkHereCommand, gLoc->GetString(IDS_ADD_BOOKMARK_HERE));
		 }
      }
      else if (child->type == BOOKMARK_BOOKMARK) {
         // condense the title and escape ampersands
		 if (!child->text.empty()) // BUG #785
		 {
			TCHAR *pszTemp = fixString(CUTF8_to_T(child->text.c_str()), 40);
			AppendMenu(menu, MF_STRING, child->id, pszTemp);
			free(pszTemp);
		 }

      }
   }
   kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "SetOwnerDrawn", (long)menu, (long)DrawBitmap);
}

void CopyRebar(HWND hWndNewTB, HWND hWndOldTB)
{
  int i = 0;
  TBBUTTON button;

  SetWindowText(hWndNewTB, _T(TOOLBAND_NAME));
  SendMessage(hWndNewTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
  SendMessage(hWndNewTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

  while (SendMessage(hWndOldTB, TB_GETBUTTON, i, (LPARAM)&button)) {
    LONG lResult = SendMessage(hWndOldTB, TB_GETBUTTONTEXT, (WPARAM) button.idCommand, NULL);
    if (lResult >= 0) {
      TCHAR *szTmp = new TCHAR[lResult + 2];
      SendMessage(hWndOldTB, TB_GETBUTTONTEXT, (WPARAM) button.idCommand, (LPARAM) szTmp);
      szTmp[lResult+1] = 0;

	  button.iString = (INT_PTR)szTmp;
      //int stringID = SendMessage(hWndNewTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM) szTmp);
      //button.iString = stringID;
      SendMessage(hWndNewTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);

      delete szTmp;
    }
    else
      SendMessage(hWndNewTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);

    ++i;
  }
}

BOOL RealDeleteMenu(HMENU menu, UINT pos)
{
    if (HMENU m = GetSubMenu(menu, pos))
		DestroyMenu(m);
	
	return DeleteMenu(menu, pos, MF_BYPOSITION);
}

// Build Rebar
void BuildRebar(HWND hWndTB)
{
   InitImageList(gImagelist);

   HIMAGELIST siteIcons = kPlugin.kFuncs->GetIconList();
   
   BOOL useSiteicon = TRUE;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.bookmarks.displaySiteicon", &useSiteicon, &useSiteicon);
   useSiteicon &= (siteIcons != NULL); 

   CBookmarkNode *toolbarNode = gBookmarkRoot->FindSpecialNode(BOOKMARK_FLAG_TB);

   SetWindowText(hWndTB, _T(TOOLBAND_NAME));

   //SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

   SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);

   SendMessage(hWndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   //int stringID;
   CBookmarkNode *child;
   int curButton = 0;

   for (child=toolbarNode->child; child; child=child->next) {
      if (++curButton > TOOLBAND_MAX_BUTTONS) {
         break;      // Simple fix to prevent massive toolbar folders from bringing KM to its knees
      }

      if (child->type == BOOKMARK_SEPARATOR) {
         TBBUTTON button;
         button.iBitmap = 0;
         button.idCommand = 0;
         button.fsState = TBSTATE_ENABLED;
         button.fsStyle = TBSTYLE_SEP; 
         button.dwData = 0;
         button.iString = 0;
         SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
         continue;
      }

      // condense the title and escape ampersands
      TCHAR *buttonString = fixString(CUTF8_to_T(child->text.c_str()), gMaxTBSize);

      //stringID = SendMessage(hWndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)buttonString);
      

      TBBUTTON button = {0};
      //button.iString = stringID;
	  button.iString = (int)buttonString;
      button.fsState = TBSTATE_ENABLED;
      button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

      if (child->type == BOOKMARK_FOLDER){
         // toolbar may not be contained in bookmark menu, so we can't just use its submenus
         HMENU childMenu = CreatePopupMenu();
         BuildMenu(childMenu, child, false);
         button.idCommand = MENU_TO_COMMAND((UINT)childMenu);
         button.iBitmap = IMAGE_FOLDER_CLOSED;
         button.fsStyle |= TBSTYLE_DROPDOWN;
		 button.dwData = (DWORD_PTR)childMenu;
      }
      else {
         button.idCommand = child->id;
		 button.iBitmap = IMAGE_BOOKMARK;
         if (useSiteicon) {
            UINT idx = GetSiteIcon(child);
            if (idx>0) {
               HICON icon = ImageList_GetIcon(siteIcons, idx, ILD_NORMAL);
			   if (icon) {
                  idx = ImageList_AddIcon(gImagelist, icon);
                  if (idx != -1) button.iBitmap = idx;
                  DestroyIcon(icon);
			   }
            }
		 }
		 button.dwData = NULL;
      }

      SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
	  free(buttonString);
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
   while (RealDeleteMenu(gMenuBookmarks, nFirstBookmarkPosition));
   // and rebuild
   BuildMenu(gMenuBookmarks, gBookmarkRoot->FindSpecialNode(BOOKMARK_FLAG_BM), false);

   int ideal = 0;

   std::map<HWND, HWND>::iterator it = gToolbarList.begin();
   // It's possible to edit bookmark, without having a toolbar (loader)
   if (it == gToolbarList.end()) 
	   return;
   
   HWND refToolbar = it->second;

      // delete the old rebar and the menus.
      TBBUTTON button; 
      do
	  {
		  if (SendMessage(refToolbar, TB_GETBUTTON, (WPARAM)0, (LPARAM)&button))
			  if (button.dwData) {
				  kPlugin.kFuncs->SendMessage("bmpmenu", PLUGIN_NAME, "UnSetOwnerDrawn", (long)button.dwData, 0);
				  DestroyMenu((HMENU)button.dwData);
			  }
	  } while (SendMessage(refToolbar, TB_DELETEBUTTON, 0, 0));

	  // and rebuild
      BuildRebar(refToolbar);

	  // Compute the width needed for the toolbar
      int iCount, iButtonCount = SendMessage(refToolbar, TB_BUTTONCOUNT, 0,0);
      for ( iCount = 0 ; iCount < iButtonCount ; iCount++ )
      { 
	      RECT rectButton;
	      SendMessage(refToolbar, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
	      ideal += rectButton.right - rectButton.left;
      }


   // Copy the new toolbar to the other windows.
   while (it != gToolbarList.end())
   {
	   HWND toolbar = it->second;

	   if (toolbar != refToolbar)
	   {
			while (SendMessage(toolbar, TB_DELETEBUTTON, 0 /*index*/, 0)) ;
			CopyRebar(toolbar, refToolbar);
	   }

	   HWND hReBar = FindWindowEx(it->first, NULL, REBARCLASSNAME, NULL);
	   int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  
	   REBARBANDINFO rb;
	   rb.cbSize = sizeof(REBARBANDINFO);
	   rb.fMask = RBBIM_CHILD;

	   // Update the ideal size 
	   for(int x=0;x < uBandCount;x++)
	   {
		   if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x, (LPARAM) &rb))
			   continue;

		   if (rb.hwndChild == toolbar)
		   {
			   rb.fMask  = RBBIM_IDEALSIZE;
			   rb.cxIdeal = ideal;
			   SendMessage(hReBar, RB_SETBANDINFO, x, (LPARAM)&rb);
			   break;
		   }
	   }
	   ++it;
   }

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

void Fill_Combo(HWND hWnd, CBookmarkNode* node, unsigned short depth=0)
{
	for (CBookmarkNode* child=node->child; child; child=child->next) {
		if (child->type == BOOKMARK_FOLDER) {
			std::basic_string<TCHAR> title = CUTF8_to_T(child->text.c_str());
			title.insert(0, depth*2, ' ');
			int index = SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)title.c_str());
			SendMessage(hWnd, CB_SETITEMDATA, index, (LPARAM)child);
			Fill_Combo(hWnd, child, depth+1);
		}
	}
}

void addLink(const char *url, const char *title, const char* nick, const char* iconurl, CBookmarkNode* node)
{
	if (!node || node->type!=BOOKMARK_FOLDER) return;

	node->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), title, url, nick, "", "", BOOKMARK_BOOKMARK, time(NULL), 0, 0, "", "", "", iconurl));
	SaveBM(gBookmarkFile);
	Rebuild();
}

int CALLBACK AddProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static char *iconurl = 0;
	switch (uMsg) {
		case WM_INITDIALOG: {
			kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo((HWND)lParam);
			if (dInfo && dInfo->url) {
				if (dInfo->title) {
					SetDlgItemText(hDlg, IDC_TITLE, CUTF8_to_T(dInfo->title));
				}else{
					SetDlgItemText(hDlg, IDC_TITLE, CUTF8_to_T(dInfo->url));
				}
				SetDlgItemText(hDlg, IDC_URL, CUTF8_to_T(dInfo->url));
				if (iconurl) {
					delete iconurl;
					iconurl = NULL;
				}
				if (dInfo->iconurl) iconurl = strdup(dInfo->iconurl);
			}
			
			HWND combo = GetDlgItem(hDlg, IDC_FOLDER);
			int index = SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)_T("Bookmarks"));
			SendMessage(combo, CB_SETITEMDATA, index, (LPARAM)gBookmarkRoot);
			Fill_Combo(combo, gBookmarkRoot, 1);

            CBookmarkNode *defaultNode = gBookmarkRoot->FindSpecialNode(BOOKMARK_FLAG_NB);
            int count = SendMessage(combo, CB_GETCOUNT, 0, 0);
            for (index=0;index<count;index++) {
               CBookmarkNode* node = (CBookmarkNode*)SendMessage(combo, CB_GETITEMDATA, index, 0);
               if (defaultNode == node) {
                  SendMessage(combo, CB_SETCURSEL, index, 0);
                  break;
               }
            }

			// Set the height
			RECT rect;
			GetClientRect(combo, &rect);
			int height = rect.bottom + 4 + SendMessage(combo, CB_GETITEMHEIGHT, 0, 0) * 10;
			SetWindowPos(combo, 0,0,0,rect.right,height,SWP_NOMOVE|SWP_NOZORDER);

			return TRUE;
			}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCHAR title[1024], url[4096], nick[128];
					GetDlgItemText(hDlg, IDC_TITLE, title, sizeof(title));
					GetDlgItemText(hDlg, IDC_URL, url, sizeof(url));
					GetDlgItemText(hDlg, IDC_NICK, nick, sizeof(nick));

					HWND combo = GetDlgItem(hDlg, IDC_FOLDER);
					int index = SendMessage(combo, CB_GETCURSEL, 0, 0);
					CBookmarkNode* node = (CBookmarkNode*)SendMessage(combo, CB_GETITEMDATA, index, 0);

					addLink(CT_to_UTF8(url), CT_to_UTF8(title), CT_to_UTF8(nick), iconurl, node);
					EndDialog( hDlg, IDOK );
					break;
					}
				case IDCANCEL:
					EndDialog( hDlg, IDCANCEL );
					break;
			}
	}
	return FALSE;
}

int addLink(const char *url, const char *title, int flag, const char* iconurl)
{
   if (!url || !(*url))
      return false;

   CBookmarkNode *addNode = gBookmarkRoot->FindSpecialNode(flag);
   char *pszUrl = DecodeString(url);
   char *pszTxt = DecodeString(title ? (*title ? title : url) : url);
   addNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), pszTxt, pszUrl, "", "", "", BOOKMARK_BOOKMARK, time(NULL),0,0,"","","",iconurl));
   if (pszUrl)
      free(pszUrl);
   if (pszTxt)
      free(pszTxt);
   SaveBM(gBookmarkFile);
   
   Rebuild();

   return true;
}


static TCHAR szInput[256];
static TCHAR *pszTitle = 0;
static TCHAR *pszPrompt = 0;

BOOL CALLBACK
PromptDlgProc( HWND hwnd,
               UINT Message,
               WPARAM wParam,
               LPARAM lParam )
{
    switch (Message) {
      case WM_INITDIALOG:
         SetWindowText( hwnd, pszTitle ? pszTitle : _T("Smart bookmark") );
         SetDlgItemText(hwnd, IDC_SEARCHTEXT, pszPrompt ? pszPrompt : _T(""));
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

void OpenURL(char *url, int mode = 0)
{
	char szOpenURLcmd[80];
	const char* pref;
	switch (mode) {

		case 1: pref = PREFERENCE_BOOKMARKS_OPENURLM; break;
		case 2: pref = PREFERENCE_BOOKMARKS_OPENURLR; break;
		case 0:
		default:
			pref = PREFERENCE_BOOKMARKS_OPENURL; break;
	}

	kPlugin.kFuncs->GetPreference(PREF_STRING, pref, szOpenURLcmd, (char*)"");

	if (!*szOpenURLcmd) {
		if (mode != 0) return;
		kPlugin.kFuncs->NavigateTo(url, OPEN_NORMAL, NULL);
		return;
	}

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
	}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_BACKGROUND")) {
		kPlugin.kFuncs->NavigateTo(url, OPEN_BACKGROUND, NULL);
	}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_NEW_WINDOW")) {
		kPlugin.kFuncs->NavigateTo(url, OPEN_NEW, NULL);
	}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_NEW_TAB")) {
		kPlugin.kFuncs->NavigateTo(url, OPEN_NEWTAB, NULL);
	}else if (idOpen == kPlugin.kFuncs->GetID("ID_OPEN_LINK_IN_BACKGROUNDTAB")) {
		kPlugin.kFuncs->NavigateTo(url, OPEN_BACKGROUNDTAB, NULL);
	}
}

void OpenBookmark(CBookmarkNode* node, HWND hWnd = NULL, int mode = 0)
{
	if (!node) return;

	node->lastVisit = time(NULL);
	gBookmarksModified = true; // this doesn't call for instant saving, it can wait until we add/edit/quit

	if (node->url.c_str() == NULL || *node->url.c_str() == 0)
		return;

	char *str = strdup(node->url.c_str());
	char *ptr = strstr(str, "%s");
	if (ptr) {
		char buff[INTERNET_MAX_URL_LENGTH];
		*ptr = 0;
		strcpy(buff, str);
		ptr += 2;

		if (pszTitle) free(pszTitle);
		if (pszPrompt) free(pszPrompt);
		pszTitle = t_from_utf8( node->text.c_str() );
		pszPrompt = t_from_utf8( node->desc.c_str() );

		int ok = gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_SMARTBOOKMARK), 
			hWnd, (DLGPROC)PromptDlgProc, NULL);
		PostMessage(hWnd, WM_NULL, 0, 0);
		if (ok == IDOK && *szInput) {
			strcat(buff, CT_to_UTF8(szInput));
			strcat(buff, ptr);
			OpenURL(buff, mode);
		}
	}
	else {
		OpenURL(str, mode);
	}
	free(str);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;
   static HMENU lastMenu = NULL;

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
         if (!dInfo || !dInfo->url || strncmp(dInfo->url, "wyciwyg", 7) == 0)
			 ::MessageBox(NULL, gLoc->GetString(IDS_CANT_BOOKMARK), gLoc->GetString(IDS_CANT_BOOKMARK_TITLE), MB_OK);

		 BOOL ask = TRUE;
		 kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_BOOKMARK_ASKFOLDER, &ask, &ask);
		 
		 if (ask) {
            gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_ADDBOOKMARK), NULL, AddProc, (LPARAM)hWnd);  
		 }
		 else {
			addLink(dInfo->url, dInfo->title, BOOKMARK_FLAG_NB, dInfo->iconurl);
		 }
         return true;
      }
      else if (command == nAddLinkCommand) {
         
         int retLen = kPlugin.kFuncs->GetGlobalVar(PREF_STRING, "LinkURL", NULL);
         if (retLen) {
			char *title = 0;
            char *url = new char[retLen+1];
            kPlugin.kFuncs->GetGlobalVar(PREF_STRING, "LinkURL", url);
			
			retLen = kPlugin.kFuncs->GetWindowVar(hWnd, Window_LinkTitle, NULL);
			if (retLen > 0) {
				title = new char[retLen+1];
				kPlugin.kFuncs->GetWindowVar(hWnd, Window_LinkTitle, title);
			}
            else title = url;

            addLink(url, title, BOOKMARK_FLAG_NB);
            delete url;
			if (title!=url) delete title;
         }
         return true;
      }
      else if (command == nAddToolbarCommand) {
         kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
         if (dInfo) {
			addLink(dInfo->url, dInfo->title, BOOKMARK_FLAG_TB, dInfo->iconurl);
         }
         return true;
      }
      else if (command == nEditCommand) {
         if (ghWndEdit) {
            ShowWindow(ghWndEdit, SW_RESTORE);
            BringWindowToTop(ghWndEdit);
         }
		 else {
            ghWndEdit = gLoc->CreateDialogParam(MAKEINTRESOURCE(IDD_EDIT_BOOKMARKS), NULL, EditProc, 0);
		 }
         return true;
      }
      else if (command == wm_deferbringtotop) {
         BringWindowToTop(hWnd);
         return true;
      }
	  else if (command == nAddBookmarkHereCommand) {
          CBookmarkNode* node = gBookmarkRoot->FindFolder(lastMenu);
          if (node) {
             kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd); 
             if (dInfo) addLink(dInfo->url, dInfo->title, "", dInfo->iconurl, node);
          }
          return true;
	  }
	  else if (CBookmarkNode *node = gBookmarkRoot->FindNode(command)) {
	     OpenBookmark(node, hWnd, lParam);
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
            TCHAR toolbarName[11];
            GetWindowText(tbhdr.hdr.hwndFrom, toolbarName, 10);
            if (_tcscmp(toolbarName, _T(TOOLBAND_NAME)) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
            }

            // post a message to defer exceution of BeginHotTrack
            PostMessage(hWnd, WM_DEFERHOTTRACK, (WPARAM) NULL, (LPARAM) NULL);

            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   else if (message == WM_INITMENUPOPUP) {
		lastMenu = (HMENU)wParam;
   }
   else if (message == WM_DEFERHOTTRACK) {
      BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
      return true;
   }
   else if (message == WM_MENUSELECT) {
      UINT id = LOWORD(wParam);
	  
      if (id >= nConfigCommand && id < nDropdownCommand) {
         if (id == nConfigCommand)
            kPlugin.kFuncs->SetStatusBarText(CT_to_UTF8(gLoc->GetString(IDS_CONFIGURE)));
         else if (id == nAddCommand)
            kPlugin.kFuncs->SetStatusBarText(CT_to_UTF8(gLoc->GetString(IDS_ADD)));
         else if (id == nAddLinkCommand)
            kPlugin.kFuncs->SetStatusBarText(CT_to_UTF8(gLoc->GetString(IDS_ADDLINK)));
         else if (id == nEditCommand)
            kPlugin.kFuncs->SetStatusBarText(CT_to_UTF8(gLoc->GetString(IDS_EDIT)));
         return true;
      } 
	  else if (id > 0) {
         CBookmarkNode *node = gBookmarkRoot->FindNode(id);
		 if (node) {
			kPlugin.kFuncs->SetStatusBarText(node->url.c_str());
			return true;
		}
      }
// this would be a hack to clean the status bar for separators, popups
// (they work (clear the status bar) for CMenu-added separators/popups,
// but not when they are added via standard win32 calls...)
      else {
         kPlugin.kFuncs->SetStatusBarText("");
      }
   }
   else if (message == TB_MBUTTONDOWN || message == TB_RBUTTONDOWN) {
      int command = LOWORD(wParam);
	  if (CBookmarkNode *node = gBookmarkRoot->FindNode(command)) {
		  OpenBookmark(node, hWnd, message-(TB_MBUTTONDOWN) + 1);
	      return true;
	  }
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}
