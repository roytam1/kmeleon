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
#endif

#include "op_hotlist.h"
#include "kmeleon_plugin.h"
#include "LocalesUtils.h"
extern Locale* gLoc;

#include "Utils.h"
#include "BookmarkNode.h"

#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <wininet.h>    // for INTERNET_MAX_URL_LENGTH
#include <errno.h>

#ifndef MAX
#  define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static bool found_tb = false;
static bool found_bm = false;
static bool found_nb = false;

int ParseHotlistFolder(char **p, CBookmarkNode &node) 
{
   char *q = NULL;
   int size = 0;
   char szName[HOTLIST_TITLE_LEN] = {0};
   char szNick[HOTLIST_TITLE_LEN] = {0};
   char szTmp[HOTLIST_STRING_LEN] = {0};
   char szDesc[HOTLIST_STRING_LEN] = {0};
   time_t addDate=0, lastVisit=0;
   long order=-1;
   char szInPanel[HOTLIST_STRING_LEN] = {0};
   char szPanelPos[HOTLIST_STRING_LEN] = {0};
   char szBarPos[HOTLIST_STRING_LEN] = {0};
   char *pszTmp;
   bool onpb = false, trash = false;
   int id_opera = 0;

   while (p && *p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (isspace(**p))
         (*p)++;
      
	  if (strnicmp(*p, "ID=", 3) == 0 ) {
         *p += 3;
         // while (*p && **p && isspace(**p))
         //   (*p)++;
		 id_opera = atoi(*p);
      }

      else if (strnicmp(*p, "NAME=", 5) == 0) {
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p+5);
         strncpy(szName, (pszTmp && *pszTmp) ? pszTmp : *p+5, HOTLIST_TITLE_LEN);
         szName[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }

      else if (strnicmp(*p, "CREATED=", 8) == 0) {
         strncpy(szTmp, *p+8, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         addDate = atol(szTmp);
      }

      else if (strnicmp(*p, "VISITED=", 8) == 0) {
         strncpy(szTmp, *p+8, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         lastVisit = atol(szTmp);
      }

      else if (strnicmp(*p, "ORDER=", 6) == 0) {
         strncpy(szTmp, *p+6, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         order = atol(szTmp);
      }

      else if (strnicmp(*p, "IN PANEL=", 9) == 0) {
         strncpy(szInPanel, *p+9, HOTLIST_STRING_LEN);
         szInPanel[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "PANEL_POS=", 10) == 0) {
         strncpy(szPanelPos, *p+9, HOTLIST_STRING_LEN);
         szPanelPos[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "PERSONALBAR_POS=", 16) == 0) {
         strncpy(szBarPos, *p+16, HOTLIST_STRING_LEN);
         szBarPos[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "DESCRIPTION=", 12) == 0) {
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p+12);
         strncpy(szDesc, (pszTmp && *pszTmp) ? pszTmp : *p+12, HOTLIST_STRING_LEN);
         szDesc[HOTLIST_STRING_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (strnicmp(*p, "NICKNAME=", 9) == 0 && *((*p)+9) != 0) {
         *p += 9;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szNick, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (strnicmp(*p, "SHORT NAME=", 11) == 0 && *((*p)+11) != 0) {
         *p += 11;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szNick, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }

	  else if (strnicmp(*p, "TRASH FOLDER=", 13) == 0 && *((*p)+13) != 0) {
         *p += 13;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
		 if (*p) {
		 if (strcmp(*p, "YES") == 0)
			 trash = true;
		 }
      }

	  else if (strnicmp(*p, "ON PERSONALBAR=", 15) == 0 && *((*p)+15) != 0) {
         *p += 15;
         //while (*p && **p && isspace(**p))
         //   (*p)++;

		 if (*p) {
		 if (strcmp(*p, "YES") == 0)
			 onpb = true;
		 }
      }

      else if (**p == 0 || **p == '#' || **p == '-') {
         if (**p) {
            *(q-1) = '\n';
            q = *p;
         }
         break;
      }
      *p = q;
   }
   *p = q;
   
   if (szName[0]) {
      CBookmarkNode * newNode = 
         new CBookmarkNode(0, szName, "", szNick, szDesc, BOOKMARK_FOLDER, addDate, lastVisit, 0, order, szInPanel, szPanelPos, szBarPos, onpb, trash, id_opera);
      if (newNode) {
         node.AddChild(newNode);
         
         size += ParseHotlist(p, *newNode);
         
         if ( !found_tb && gToolbarFolder[0] != 0 && 
              ((strcmp(szName, gToolbarFolder) == 0)))
            {
               newNode->flags |= BOOKMARK_FLAG_TB;
               found_tb = true;
            }
         if ( !found_bm && gMenuFolder[0] != 0 && 
              ((strcmp(szName, gMenuFolder) == 0)))
            {
               newNode->flags |= BOOKMARK_FLAG_BM;
               found_bm = true;
            }
         if ( !found_nb && gNewitemFolder[0] != 0 && 
              ((strcmp(szName, gNewitemFolder) == 0)))
            {
               newNode->flags |= BOOKMARK_FLAG_NB;
               found_nb = true;
            }
      }
   }
   
   return size;
}

int ParseHotlistUrl(char **p, CBookmarkNode &node) 
{
   char *q = NULL;
   char szName[HOTLIST_TITLE_LEN] = {0};
   char szNick[HOTLIST_TITLE_LEN] = {0};
   char szURL[INTERNET_MAX_URL_LENGTH] = {0};
   char szTmp[HOTLIST_STRING_LEN] = {0};
   char szDesc[HOTLIST_STRING_LEN] = {0};
   char szIcon[HOTLIST_STRING_LEN] = {0};
   time_t addDate=0, lastVisit=0;
   long order=-1;
   char szInPanel[HOTLIST_STRING_LEN] = {0};
   char szPanelPos[HOTLIST_STRING_LEN] = {0};
   char szBarPos[HOTLIST_STRING_LEN] = {0};
   char *pszTmp;
   bool onpb = false;
   int id_opera  = 0;

   while (*p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (*p && **p && isspace(**p))
         (*p)++;
      
	   if (strnicmp(*p, "ID=", 3) == 0) {
         *p += 3;
         // while (*p && **p && isspace(**p))
         //   (*p)++;
		 id_opera = atoi(*p);
      }

      else if (strnicmp(*p, "NAME=", 5) == 0 && szName[0] == 0) {
         *p += 5;
         // while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szName, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_TITLE_LEN);
         szName[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (strnicmp(*p, "URL=", 4) == 0) {
         strncpy(szURL, *p+4, INTERNET_MAX_URL_LENGTH);
         szURL[INTERNET_MAX_URL_LENGTH-1] = 0;
      }

      else if (strnicmp(*p, "CREATED=", 8) == 0) {
         strncpy(szTmp, *p+8, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         addDate = atol(szTmp);
      }

      else if (strnicmp(*p, "VISITED=", 8) == 0) {
         strncpy(szTmp, *p+8, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         lastVisit = atol(szTmp);
      }

      else if (strnicmp(*p, "ORDER=", 6) == 0) {
         strncpy(szTmp, *p+6, HOTLIST_STRING_LEN);
         szTmp[HOTLIST_STRING_LEN-1] = 0;
         order = atol(szTmp);
      }

      else if (strnicmp(*p, "IN PANEL=", 9) == 0) {
         strncpy(szInPanel, *p+9, HOTLIST_STRING_LEN);
         szInPanel[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "PANEL_POS=", 10) == 0) {
         strncpy(szPanelPos, *p+9, HOTLIST_STRING_LEN);
         szPanelPos[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "PERSONALBAR_POS=", 16) == 0) {
         strncpy(szBarPos, *p+16, HOTLIST_STRING_LEN);
         szBarPos[HOTLIST_STRING_LEN-1] = 0;
      }

      else if (strnicmp(*p, "DESCRIPTION=", 12) == 0) {
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p+12);
         strncpy(szDesc, (pszTmp && *pszTmp) ? pszTmp : *p+12, HOTLIST_STRING_LEN);
         szDesc[HOTLIST_STRING_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (strnicmp(*p, "NICKNAME=", 9) == 0 && *((*p)+9) != 0) {
         *p += 9;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szNick, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (strnicmp(*p, "SHORT NAME=", 11) == 0 && *((*p)+11) != 0) {
         *p += 11;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szNick, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }

	  else if (strnicmp(*p, "ON PERSONALBAR=", 15) == 0 && *((*p)+15) != 0) {
         *p += 15;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
		 if (*p) {
		 if (strcmp(*p, "YES") == 0)
			 onpb = true;
		 }
		 if (pszTmp) free(pszTmp);
      }

	  else if (strnicmp(*p, "ICONFILE=", 9) == 0 && *((*p)+9) != 0) {
         *p += 9;
         //while (*p && **p && isspace(**p))
         //   (*p)++;
         pszTmp = kPlugin.kFuncs->DecodeUTF8(*p);
         strncpy(szNick, (pszTmp && *pszTmp) ? pszTmp : *p, HOTLIST_STRING_LEN);
         szNick[HOTLIST_STRING_LEN-1] = 0;
         if (pszTmp)
            free(pszTmp);
      }
      
      else if (**p == '#' || **p == '-') {
         *(q-1) = '\n';
         q = *p;
         break;
      }
      *p = q;
   } 
   *p = q;
   
   if (szURL[0]) {
      int id = kPlugin.kFuncs->GetCommandIDs(1);
      CBookmarkNode * newNode = 
         new CBookmarkNode(id, szName[0] ? szName : szURL, szURL, szNick, szDesc, BOOKMARK_BOOKMARK, addDate, lastVisit, 0, order, szInPanel, szPanelPos, szBarPos, onpb, false, id_opera, szIcon);
      if (newNode) {
         node.AddChild(newNode);
         return 1;
      } 
      else
         return 0;
   }
   
   return 0;
}

int ParseHotlist(char **p, CBookmarkNode &node) 
{
   char *q = NULL;
   int size = 0;
   
   while (*p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (*p && **p && isspace(**p))
         (*p)++;
      
      if (strnicmp(*p, "#FOLDER", 7) == 0) {
         size += ParseHotlistFolder(&q, node);
      }
      else if (strnicmp(*p, "#URL", 4) == 0) {
         size += ParseHotlistUrl(&q, node);
      }
      else if (**p == '-') {
         break;
      }
      
      *p = q;
   }
   *p = q;
   return size;
}

static time_t timestamp;

int op_readFile(char *file) {
   int ret = -1;
   found_tb = false;
   found_bm = false;
   found_nb = false;
   
   DWORD dwWaitResult; 
   dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
   if (dwWaitResult == WAIT_TIMEOUT) {
      MessageBox(NULL, "Unable to get MutEx for hotlist file.\\nFile not loaded.", PLUGIN_NAME ": WARNING" , MB_OK|MB_ICONSTOP);
      return ret;
   }

   FILE *bmFile = fopen(file, "r");
   if (bmFile){
      long bmFileSize = FileSize(bmFile);
      
      char *bmFileBuffer = new char[bmFileSize+1];
      if (bmFileBuffer){
         struct stat st = {0};

         fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);
         bmFileBuffer[bmFileSize] = 0;
         
         // strtok(bmFileBuffer, "\n");
         char *szTmpBuf = bmFileBuffer;
         ret = ParseHotlist(&szTmpBuf, gHotlistRoot);
         
         delete [] bmFileBuffer;

         stat(file, &st);
         timestamp = MAX(time(NULL), st.st_mtime);
      }
      fclose(bmFile);
      gHotlistModified = false;
   }
   
   ReleaseMutex(ghMutex);
   return ret;
}

int ReloadHotlist() {
   int ret = 0;

#if 0
   struct stat st = {0};
   stat (lpszHotlistFile, &st);
   
   if (st.st_mtime > timestamp) {
      delete gHotlistRoot;
      ret = op_readFile(lpszHotlistFile);     
   }
#endif

   return ret;
}

#define EOL (bDOS ? "\r\n" : "\n")

int SaveHotlistEntry(FILE *bmFile, CBookmarkNode *node)
{
   char *pszTmp;
   fprintf(bmFile, "#URL%s", EOL);
   if (node->id_opera>0) {
      fprintf(bmFile, "\tID=%ld", node->id_opera);
	  fprintf(bmFile, EOL);
	}

   pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)node->text.c_str());
   fprintf(bmFile, "\tNAME=%s%s", (pszTmp && *pszTmp) ? pszTmp : (char*)node->text.c_str(), EOL);
   if (pszTmp)
      free(pszTmp);
   fprintf(bmFile, "\tURL=%s%s", (char*)node->url.c_str(), EOL);
   if (node->addDate>0) {
      fprintf(bmFile, "\tCREATED=%ld", node->addDate);
	  fprintf(bmFile, EOL);
   }
   if (node->lastVisit>0) {
      fprintf(bmFile, "\tVISITED=%ld", node->lastVisit); 
	  fprintf(bmFile, EOL);
   }
   if (node->order!=-1) {
      fprintf(bmFile, "\tORDER=%ld", node->order);
	  fprintf(bmFile, EOL);
   }
   
   		 if (node->desc.length()>0 && (pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)node->desc.c_str()))) {
            fprintf(bmFile, "\tDESCRIPTION=%s%s", pszTmp, EOL);
            free(pszTmp);
		 }

		 if (node->nick.length()>0 && (pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)node->nick.c_str()))) {
            fprintf(bmFile, "\tSHORT NAME=%s%s", pszTmp, EOL);
            free(pszTmp);
		 }

		 if (node->icon.length()>0 && (pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)node->icon.c_str()))) {
            fprintf(bmFile, "\tSHORT NAME=%s%s", pszTmp, EOL);
            free(pszTmp);
		 }
   
   if (node->bar_pos.length() > 0)
      fprintf(bmFile, "\tPERSONALBAR_POS=%s%s", (char*)node->bar_pos.c_str(), EOL);
   if (node->in_panel.length() > 0)
      fprintf(bmFile, "\tIN PANEL=%s%s", (char*)node->in_panel.c_str(), EOL);
   if (node->panel_pos.length() > 0)
      fprintf(bmFile, "\tPANEL_POS=%s%s", (char*)node->panel_pos.c_str(), EOL);
   if (node->on_personalbar)
      fprintf(bmFile, "\tON PERSONALBAR=YES%s", EOL);

   
   return 0;
}

int SaveHotlist(FILE *bmFile, CBookmarkNode *node)
{
   int type;
   CBookmarkNode *child;
   for (child=node->child; child; child=child->next) {
      type = child->type;
      if (type == BOOKMARK_FOLDER) {
         char *pszTmp;
         fprintf(bmFile, "#FOLDER%s", EOL);
		 if (child->id_opera>0) {
            fprintf(bmFile, "\tID=%ld", child->id_opera);
			fprintf(bmFile, EOL);
		 }
		 pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)child->text.c_str());
         fprintf(bmFile, "\tNAME=%s%s", (pszTmp && *pszTmp) ? pszTmp : (char*)child->text.c_str(), EOL);
         if (pszTmp)
            free(pszTmp);

         if (child->addDate>0) {
            fprintf(bmFile, "\tCREATED=%ld", child->addDate);
			fprintf(bmFile, EOL);
		 }
		 if (child->lastVisit>0) {
            fprintf(bmFile, "\tVISITED=%ld", child->lastVisit);
			fprintf(bmFile, EOL);
		 }
         if (child->order!=-1) {
		    fprintf(bmFile, "\tORDER=%ld", child->order);
            fprintf(bmFile, EOL);
		 }
         
		 if (child->desc.length()>0 && (pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)child->desc.c_str()))) {
            fprintf(bmFile, "\tDESCRIPTION=%s%s", pszTmp, EOL);
            free(pszTmp);
		 }

		 if (child->nick.length()>0 && (pszTmp = kPlugin.kFuncs->EncodeUTF8((char*)child->nick.c_str()))){
            fprintf(bmFile, "\tSHORT NAME=%s%s", pszTmp, EOL);
            free(pszTmp);
		 }

         if (child->bar_pos.length() > 0)
            fprintf(bmFile, "\tPERSONALBAR_POS=%s%s", (char*)child->bar_pos.c_str(), EOL);
         if (child->in_panel.length() > 0)
            fprintf(bmFile, "\tIN PANEL=%s%s", (char*)child->in_panel.c_str(), EOL);
         if (child->panel_pos.length() > 0)
            fprintf(bmFile, "\tPANEL_POS=%s%s", (char*)child->panel_pos.c_str(), EOL);
         if (child->on_personalbar)
            fprintf(bmFile, "\tON PERSONALBAR=YES%s", EOL);
		 if (child->trash)
            fprintf(bmFile, "\tTRASH FOLDER=YES%s", EOL);

         fprintf(bmFile, "%s", EOL);
         SaveHotlist(bmFile, child);
         fprintf(bmFile, "-%s", EOL);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         // fprintf(bmFile, "%s", EOL);
      }
      else if (type == BOOKMARK_BOOKMARK) {
         SaveHotlistEntry(bmFile, child);
         fprintf(bmFile, "%s", EOL);
      }
   }
   return 0;
}

static BOOL bHotlistBak = 0;

static void backup_hotlist(TCHAR *file, int num=2)
{
   char buf[MAX_PATH];
	_tcscpy(buf, file);
	_tcscat(buf, "_backup");
   
	struct _stat s = {0};
	if (_tstat(buf, &s)!=-1 && s.st_mtime > time(NULL) - 172800)
		return;

   _tunlink(buf);
   _trename(file, buf);
   bHotlistBak = 1;
}

int op_addEntry(char *file, CBookmarkNode *node)
{
   if (file && *file) {
      int bNewFile = 0;
      struct stat sb;
      if (stat (lpszHotlistFile, &sb) == -1 && errno == ENOENT)
         bNewFile = 1;
      
      if (bNewFile) {
         FILE *bmFile = fopen(file, "w+");
         if (bmFile)
            fclose(bmFile);
      }
      FILE *bmFile = fopen(file, "rb+");
      if (bmFile) {
         if (bNewFile) {
            fprintf(bmFile, 
                    "Opera Hotlist version 2.0\r\n"
                    "Options:encoding=utf8,version=3\r\n\r\n");
            bDOS = 1;
         }
         else {
            long bmFileSize = FileSize(bmFile);
#define LEN 16
            char *bmFileBuffer = new char[LEN+1];
            if (bmFileBuffer){
               if (bmFileSize > LEN-1)
                  fseek(bmFile, bmFileSize-(LEN-1), SEEK_SET);
               int bufsize = fread(bmFileBuffer, sizeof(char), LEN, bmFile);
               bmFileBuffer[LEN] = 0;
               char *p = strstr(bmFileBuffer, "\r\n");
               bDOS = (p != NULL);
               p = strrchr(bmFileBuffer, '-');
               if (p) {
                  int offset = (bmFileBuffer + bufsize) - p;
                  fseek(bmFile, bmFileSize-offset, SEEK_SET);
               }
               delete [] bmFileBuffer;
            }
         }
         SaveHotlistEntry(bmFile, node);
         if (bDOS) 
            fprintf(bmFile, "\r");
         fprintf(bmFile, "\n");
         fprintf(bmFile, "-");
         if (bDOS) 
            fprintf(bmFile, "\r");
         fprintf(bmFile, "\n");
         fclose(bmFile);
      }
      else {
         MessageBox(NULL, "Unable to save hotlist", "Error", MB_ICONSTOP|MB_OK);
         return -1;
      }
   }
   else {
      MessageBox(NULL, "Unable to save hotlist", "Error", MB_ICONSTOP|MB_OK);
      return -1;
   }

   return 0;
}

int op_writeFile(char *file) {
   int ret = -1;

   if (file && *file) {
      DWORD dwWaitResult; 
      dwWaitResult = WaitForSingleObject( ghMutex, 1000L);
      if (dwWaitResult == WAIT_TIMEOUT) {
         MessageBox(NULL, "Unable to get MutEx for hotlist file.\\nFile not saved.", PLUGIN_NAME ": WARNING" , MB_OK|MB_ICONSTOP);
         return ret;
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
      
      FILE *bmFile = fopen(buf, "wb+");
      if (bmFile) {
         fprintf(bmFile, 
                 "Opera Hotlist version 2.0\r\n"
                 "Options:encoding=utf8,version=3\r\n\r\n");
         bDOS = 1;
         ret = SaveHotlist(bmFile, &gHotlistRoot);
         fprintf(bmFile, "-\r\n");
         if (fclose(bmFile) == EOF) {
		    _tunlink(buf);
            MessageBox(NULL, gLoc->GetString(IDS_SAVE_FAILED), gLoc->GetString(IDS_ERROR), MB_OK|MB_ICONERROR);
            ReleaseMutex(ghMutex);
            return -1;
	     }
         gHotlistModified = false;

         if (!bHotlistBak)
            backup_hotlist(file);

         unlink(file);
         rename(buf, file);
      }

      ReleaseMutex(ghMutex);
   }

   return ret;
}
