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
#include "../kmeleon_plugin.h"

#include "../Utils.h"
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
   
   while (p && *p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (isspace(**p))
         (*p)++;
      
      if (strnicmp(*p, "NAME=", 5) == 0) {
         strncpy(szName, *p+5, HOTLIST_TITLE_LEN);
         szName[HOTLIST_TITLE_LEN-1] = 0;
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
         strncpy(szDesc, *p+12, HOTLIST_STRING_LEN);
         szDesc[HOTLIST_STRING_LEN-1] = 0;
      }
      
      else if (strnicmp(*p, "NICKNAME=", 9) == 0 && *((*p)+9) != 0) {
         *p += 9;
         while (*p && **p && isspace(**p))
            (*p)++;
         strncpy(szNick, *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
      }
      
      else if (strnicmp(*p, "SHORT NAME=", 11) == 0 && *((*p)+11) != 0) {
         *p += 11;
         while (*p && **p && isspace(**p))
            (*p)++;
         strncpy(szNick, *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
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
         new CBookmarkNode(0, szName, "", szNick, szDesc, BOOKMARK_FOLDER, addDate, lastVisit, 0, order, szInPanel, szPanelPos, szBarPos);
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
   time_t addDate=0, lastVisit=0;
   long order=-1;
   char szInPanel[HOTLIST_STRING_LEN] = {0};
   char szPanelPos[HOTLIST_STRING_LEN] = {0};
   char szBarPos[HOTLIST_STRING_LEN] = {0};
   
   while (*p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (*p && **p && isspace(**p))
         (*p)++;
      
      if (strnicmp(*p, "NAME=", 5) == 0 && szName[0] == 0) {
         *p += 5;
         while (*p && **p && isspace(**p))
            (*p)++;
         strncpy(szName, *p, HOTLIST_TITLE_LEN);
         szName[HOTLIST_TITLE_LEN-1] = 0;
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
         strncpy(szDesc, *p+12, HOTLIST_STRING_LEN);
         szDesc[HOTLIST_STRING_LEN-1] = 0;
      }
      
      else if (strnicmp(*p, "NICKNAME=", 9) == 0 && *((*p)+9) != 0) {
         *p += 9;
         while (*p && **p && isspace(**p))
            (*p)++;
         strncpy(szNick, *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
      }
      
      else if (strnicmp(*p, "SHORT NAME=", 11) == 0 && *((*p)+11) != 0) {
         *p += 11;
         while (*p && **p && isspace(**p))
            (*p)++;
         strncpy(szNick, *p, HOTLIST_TITLE_LEN);
         szNick[HOTLIST_TITLE_LEN-1] = 0;
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
         new CBookmarkNode(id, szName[0] ? szName : szURL, szURL, szNick, szDesc, BOOKMARK_BOOKMARK, addDate, lastVisit, 0, order, szInPanel, szPanelPos, szBarPos);
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
   fprintf(bmFile, "#URL%s", EOL);
   fprintf(bmFile, "\tNAME=%s%s", (char*)node->text.c_str(), EOL);
   fprintf(bmFile, "\tURL=%s%s", (char*)node->url.c_str(), EOL);
   fprintf(bmFile, "\tCREATED=%ld%s", node->addDate, EOL);
   fprintf(bmFile, "\tVISITED=%ld%s", node->lastVisit, EOL);
   fprintf(bmFile, "\tORDER=%ld%s", node->order, EOL);
   fprintf(bmFile, "\tDESCRIPTION=%s%s", (char*)node->desc.c_str(), EOL);
   fprintf(bmFile, "\tSHORT NAME=%s%s", (char*)node->nick.c_str(), EOL);
   if (node->bar_pos.length() > 0)
      fprintf(bmFile, "\tPERSONALBAR_POS=%s%s", (char*)node->bar_pos.c_str(), EOL);
   if (node->in_panel.length() > 0)
      fprintf(bmFile, "\tIN PANEL=%s%s", (char*)node->in_panel.c_str(), EOL);
   if (node->panel_pos.length() > 0)
      fprintf(bmFile, "\tPANEL_POS=%s%s", (char*)node->panel_pos.c_str(), EOL);
   
   return 0;
}

int SaveHotlist(FILE *bmFile, CBookmarkNode *node)
{
   int type;
   CBookmarkNode *child;
   for (child=node->child; child; child=child->next) {
      type = child->type;
      if (type == BOOKMARK_FOLDER) {
         fprintf(bmFile, "#FOLDER%s", EOL);
         fprintf(bmFile, "\tNAME=%s%s", child->text.c_str(), EOL);
         fprintf(bmFile, "\tCREATED=%ld%s", child->addDate, EOL);
         fprintf(bmFile, "\tVISITED=%ld%s", child->lastVisit, EOL);
         fprintf(bmFile, "\tORDER=%ld%s", child->order, EOL);
         fprintf(bmFile, "\tDESCRIPTION=%s%s", (char*)child->desc.c_str(), EOL);
         fprintf(bmFile, "\tSHORT NAME=%s%s", (char*)child->nick.c_str(), EOL);
         if (node->bar_pos.length() > 0)
            fprintf(bmFile, "\tPERSONALBAR_POS=%s%s", (char*)node->bar_pos.c_str(), EOL);
         if (node->in_panel.length() > 0)
            fprintf(bmFile, "\tIN PANEL=%s%s", (char*)node->in_panel.c_str(), EOL);
         if (node->panel_pos.length() > 0)
            fprintf(bmFile, "\tPANEL_POS=%s%s", (char*)node->panel_pos.c_str(), EOL);
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

static void backup_hotlist(char *file)
{
   int i;
   char buf[MAX_PATH];
   char buf2[MAX_PATH];
   
   /* rotate the old hotlists */
   for (i=8; i>=1; i--) {
      sprintf(buf, "%s.bak%d", file, i);
      sprintf(buf2, "%s.bak%d", file, i+1);
      unlink(buf2);
      rename(buf, buf2);
   }

   sprintf(buf, "%s.bak1", file);
   unlink(buf);
   rename(file, buf);

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
#if 1
      if (!bHotlistBak) {
         backup_hotlist(file);
      }
#endif

      FILE *bmFile = fopen(file, "wb+");
      if (bmFile) {
         fprintf(bmFile, 
                 "Opera Hotlist version 2.0\r\n"
                 "Options:encoding=utf8,version=3\r\n\r\n");
         bDOS = 1;
         ret = SaveHotlist(bmFile, &gHotlistRoot);
         fprintf(bmFile, "-\r\n");
         fclose(bmFile);
         gHotlistModified = false;
      }
   }
   return ret;
}
