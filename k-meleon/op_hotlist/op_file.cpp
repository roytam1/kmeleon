/*
 * Copyright (C) 2002 Ulf Erikson <ulferikson@fastmail.fm>
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

#include <stdio.h>
#include <sys/stat.h>
#include <wininet.h>    // for INTERNET_MAX_URL_LENGTH

#ifndef MAX
#  define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static bool found_tb = false;

int ParseHotlistFolder(char **p, CBookmarkNode &node) 
{
   char *q = NULL;
   int size = 0;
   char szName[HOTLIST_TITLE_LEN] = {0};
   
   while (p && *p && **p && (q = strchr(*p, '\n')) != NULL) {
      *q++ = 0;
      
      while (isspace(**p))
         (*p)++;
      
      if (strnicmp(*p, "NAME=", 5) == 0) {
         strncpy(szName, *p+5, HOTLIST_TITLE_LEN);
         szName[HOTLIST_TITLE_LEN-1] = 0;
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
         new CBookmarkNode(0, szName, "", "", BOOKMARK_FOLDER, 0);
      if (newNode) {
         node.AddChild(newNode);
         
         size += ParseHotlist(p, *newNode);
         
         if ( !found_tb &&
              ((strcmp(szName, gToolbarFolder) == 0)))
            {
               newNode->flags |= BOOKMARK_FLAG_TB;
               found_tb = true;
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
      
      else if (strnicmp(*p, "URL=", 4) == 0) {
         strncpy(szURL, *p+4, INTERNET_MAX_URL_LENGTH);
         szURL[INTERNET_MAX_URL_LENGTH-1] = 0;
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
         new CBookmarkNode(id, szName[0] ? szName : szURL, szURL, szNick, BOOKMARK_BOOKMARK, 0, 0, 0);
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
   fprintf(bmFile, "\tNAME=%s%s", node->text.c_str(), EOL);
   fprintf(bmFile, "\tURL=%s%s", node->url.c_str(), EOL);
   fprintf(bmFile, "\tCREATED=%ld%s", node->addDate, EOL);
   fprintf(bmFile, "\tVISITED=%ld%s", node->lastVisit, EOL);
   fprintf(bmFile, "\tORDER=%d%s", 0, EOL);
   fprintf(bmFile, "\tDESCRIPTION=%s%s", (char*)"", EOL);
   fprintf(bmFile, "\tSHORT NAME=%s%s", (char*)"", EOL);
   
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
         fprintf(bmFile, "\tVISITED=%s%s", (char*)"", EOL);
         fprintf(bmFile, "\tORDER=%s%s", (char*)"", EOL);
         fprintf(bmFile, "\tDESCRIPTION=%s%s", (char*)"", EOL);
         fprintf(bmFile, "\tSHORT NAME=%s%s", (char*)"", EOL);
         fprintf(bmFile, "%s", EOL);
         SaveHotlist(bmFile, child);
         fprintf(bmFile, "-%s", EOL);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         // fprintf(bmFile, "%s", EOL);
      }
      else if (type == BOOKMARK_BOOKMARK) {
         SaveHotlistEntry(bmFile, child);
         if (child->next)
            fprintf(bmFile, "%s", EOL);
      }
   }
   return 0;
}


int op_writeFile(char *file) {
   int ret = -1;
   /*
     FILE *bmFile = fopen(file, "ab+");
     if (bmFile) {
     ret = SaveHotlist(bmFile, &gHotlistRoot);
     }
   */
   return ret;
}
