/*
*  Copyright (C) 2000 Brian Harris, Mark Liffiton
*  Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
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

#ifndef HISTORY_NODE_H
#define HISTORY_NODE_H

#include <time.h>
#include <string>
#include <windows.h>
#include <limits.h>

#include "history.h"
#include "../op_hotlist/stristr.c"

const int HISTORY_FOLDER   = 0;  // "children" is not empty.  url is not used
const int HISTORY_SEPARATOR= 1;  // this node is a separator, url, title, and children are not used
const int HISTORY_BOOKMARK = 2;  // this node is a real bookmark, all fields valid

int compareBookmarks(const char *e1, const char *e2, unsigned int sortorder);

class CHistoryNode {
public:
   std::string url;
   int type;
   time_t lastVisit;
   CHistoryNode *next;
   CHistoryNode *child;
   CHistoryNode *lastChild;

   inline CHistoryNode()
   {
      url = "";
      type = 0;
      next = NULL;
      child = NULL;
      lastChild = NULL;

      lastVisit = 0;
   }
   inline CHistoryNode(const char *url, int type, time_t lastVisit=0)
   {
      this->url = url;
      this->type = type;
      this->next = NULL;
      this->child = NULL;
      this->lastChild = NULL;
      this->lastVisit = lastVisit;
   }
   inline CHistoryNode(std::string &url, int type, time_t lastVisit=0)
   {
      this->url = url;
      this->type = type;
      this->next = NULL;
      this->child = NULL;
      this->lastChild = NULL;
      this->lastVisit = lastVisit;
   }
   inline ~CHistoryNode()
   {
      // when we delete next, it's destructor will delete it's next, which will delete it's next, etc...
      if (next)
         delete next;

      // same with child...
      if (child)
         delete child;
   }
   inline CHistoryNode &operator=(CHistoryNode &n2)
   {
      url = n2.url;
      type = n2.type;
      lastVisit = n2.lastVisit;

      if (child) {
         delete child;
      }
      if (next) {
         delete next;
      }

      if (n2.child) {
         child = new CHistoryNode();
         (*child) = (*n2.child);

         // need to rebuild pointer to lastChild - can't just grab it from n2
         lastChild = child;
         while (lastChild->next) lastChild = lastChild->next;
      }
      else {
         child = NULL;
      }

      if (n2.next) {
         next = new CHistoryNode();
         (*next) = (*n2.next);
      }
      else {
         next = NULL;
      }

      return *this;
   }
   inline void AddChild(CHistoryNode *newChild)
   {
      if (child) {
         lastChild->next = newChild;
      }
      else {
         child = newChild;
      }
      lastChild = newChild;
   }
   BOOL UnlinkNode(CHistoryNode *node)
   {
      CHistoryNode *c;
      CHistoryNode *previous = NULL;

      for (c=child; c; previous=c, c=c->next) {
         if (c == node) {
            // found our node to delete!

            // first redirect traffic around it
            if (previous) {
               previous->next = node->next;
            }
            else {
               child = node->next;
            }

            // if we are the last item, set our parent's lastChild to the item before us
            if (!node->next) {
               lastChild = previous;
            }

            // WE HAVE TO SET OUR NEXT TO NULL
            // if we don't do this, when we are deleted, we will try to delete our next, which would result
            // in pretty much the whole menu being deleted
            node->next = NULL;

            return true;
         }
      }
      return false;
   }
   BOOL DeleteNode(CHistoryNode *node)
   {
      if (UnlinkNode(node)) {
         delete node;
         return true;
      }
      return false;
   }
   int Index(int &mypos, CHistoryNode *node)
   {
      CHistoryNode *c;
      for (c=child; c; c=c->next) {
         mypos++;
         if (c->type == HISTORY_SEPARATOR)
            continue;
         else if (node == c)
               return mypos;

         if (c->type == HISTORY_FOLDER) {
            int newpos = c->Index(mypos, node);
            if (newpos >= 0)
               return newpos;
         }
      }
      return -1;
   }
   int Search(const char *str, int pos, int &mypos, int &firstpos, CHistoryNode **node)
   {
      CHistoryNode *c;
      for (c=child; c; c=c->next) {
         mypos++;
         if (c->type == HISTORY_SEPARATOR) {
            continue;
         }
         else if (stristr(c->url.c_str(), str)) {
            if (mypos >= pos) {
               // this is it!
               if (node)
                  *node = c;
               return mypos;
            }
            else if (firstpos == -1) {
               firstpos = mypos;
            }
         }

         if (c->type == HISTORY_FOLDER) {

            int newpos = c->Search(str, pos, mypos, firstpos, node);

            if (newpos >= pos) {
               // found it in a sub-node
               return newpos;
            }
         }
      }
      return firstpos;
   }
   void flatten()
   {
      CHistoryNode *c, *prev = NULL;
      for (c=child; c; c=c->next) {
         if (c->type == HISTORY_FOLDER && c->child != NULL) {
            CHistoryNode *old = c;
            CHistoryNode *tmp = c->next;

            c->flatten();

            c = c->child;
            if (prev)
               prev->next = c;
            else
               child = c;
            while (c->next)
               c = c->next;
            c->next = tmp;

            old->next = NULL;
            old->child = NULL;
            delete old;
         }
         prev = c;
      }
      lastChild = prev;
   }
   int size() {
      int i = 1;

      if (type == HISTORY_FOLDER && child)
         i = i + child->size();
      if (next)
         i = i + next->size();

      return i;
   }
   void sort(int sortorder)
   {
      if (!child)
        return;

      CHistoryNode *c;
      int i = 0;
      for (c=child; c; c=c->next)
         i++;
      if (i == 0)
         return;
      void **buf = (void**)calloc(i, sizeof(void*));
      if (!buf)
         return;
      for (i=0,c=child; c; c=c->next,i++)
         buf[i] = (void*)c;
      quicksort((char*)buf, i, sizeof(void*), &compareBookmarks, sortorder);
      child = ((CHistoryNode*)buf[0]);
      for (int j=0; j<i-1; j++) {
         c = ((CHistoryNode*)buf[j]);
         c->next = ((CHistoryNode*)buf[j+1]);
      }
      c = ((CHistoryNode*)buf[i-1]);
      c->next = NULL;
      lastChild = c;
      free(buf);
      for (c=child; c; c=c->next) {
         if (c->type == HISTORY_FOLDER) {
            c->sort(sortorder);
         }
      }
   }
};


int compareBookmarks(const char *e1, const char *e2, unsigned int sortorder)
{
#define SORT_BITS 2
     int cmp = 0;

     if (e1 == e2) return 0;
     if (!e1) return -1;
     if (!e2) return 1;

     CHistoryNode *c1 = (CHistoryNode *)*((void**)e1);
     CHistoryNode *c2 = (CHistoryNode *)*((void**)e2);

     switch (sortorder & ((1<<SORT_BITS)-1)) {
       case 0:
          cmp = c1->lastVisit - c2->lastVisit;
          return cmp ? cmp : -1;
          break;
       case 1:
          cmp = stricmp((char*)c1->url.c_str(), (char*)c2->url.c_str());
          break;
       case 2:
          cmp = c2->lastVisit - c1->lastVisit;
          break;
       case 3:
          cmp = stricmp((char*)c2->url.c_str(), (char*)c1->url.c_str());
          break;
       default:
          cmp = c1->lastVisit - c2->lastVisit;
          return cmp ? cmp : -1;
          break;
     }
     if (cmp == 0)
       return compareBookmarks(e1, e2, (sortorder >> SORT_BITS));
     return cmp;
}

#endif

