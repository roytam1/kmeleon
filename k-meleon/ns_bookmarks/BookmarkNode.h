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

#ifndef BOOKMARK_NODE_H
#define BOOKMARK_NODE_H

#include <time.h>
#include <string>

const int BOOKMARK_BOOKMARK = 0;  // this node is a real bookmark, all fields valid
const int BOOKMARK_FOLDER   = 1;  // "children" is not empty.  url is not used
const int BOOKMARK_SEPARATOR= 2;  // this node is a separator, url, title, and children are not used

const int BOOKMARK_FLAG_TB = 0x1;	// Toolbar Folder
const int BOOKMARK_FLAG_NB = 0x2;	// New Bookmarks Folder
const int BOOKMARK_FLAG_BM = 0x4;	// Bookmark Menu Folder
const int BOOKMARK_FLAG_GRP = 0x8;	// Bookmark Group Folder

typedef int cmp_t(const char *, const char *, unsigned int);
void quicksort(char *a, size_t n, size_t es, cmp_t *cmp, unsigned int flag);
static int compareBookmarks(const char *e1, const char *e2, unsigned int sortorder);
char *stristr(const char *String, const char *Pattern);

class CBookmarkNode {
public:
   int id;
   std::string text;
   std::string url;
   std::string nick;
   std::string desc;
   std::string charset;
   int type;
   int flags;
   time_t addDate;
   time_t lastVisit;
   time_t lastModified;
   CBookmarkNode *next;
   CBookmarkNode *child;
   CBookmarkNode *lastChild;

   inline CBookmarkNode()
   {
      id = 0;
      text = "";
      url = "";
      nick = "";
      desc = "";
      charset = "";
      type = 0;
	  flags = 0;
      next = NULL;
      child = NULL;
      lastChild = NULL;

      addDate = 0;
      lastVisit = 0;
      lastModified = 0;
   }
   inline CBookmarkNode(int id, const char *text, const char *url, const char *nick, const char *desc, const char *charset, int type, time_t addDate=0, time_t lastVisit=0, time_t lastModified=0)
   {
      this->id = id;
      this->text = text;
      this->url = url;
      this->nick = nick ? nick : "";
      this->desc = desc ? desc : "";
      this->charset = charset ? charset : "";
      this->type = type;
	  this->flags = 0;
      this->next = NULL;
      this->child = NULL;
      this->lastChild = NULL;
      this->addDate = addDate;
      this->lastVisit = lastVisit;
      this->lastModified = lastModified;
   }
   inline CBookmarkNode(int id, std::string &text, std::string &url, std::string &nick, std::string &desc, std::string &charset, int type, time_t addDate=0, time_t lastVisit=0, time_t lastModified=0)
   {
      this->id = id;
      this->text = text;
      this->url = url;
      this->nick = nick;
      this->desc = desc;
      this->charset = charset;
      this->type = type;
	  this->flags = 0;
      this->next = NULL;
      this->child = NULL;
      this->lastChild = NULL;
      this->addDate = addDate;
      this->lastVisit = lastVisit;
      this->lastModified = lastModified;
   }
   inline ~CBookmarkNode()
   {
      // when we delete next, it's destructor will delete it's next, which will delete it's next, etc...
      if (next)
         delete next;

      // same with child...
      if (child)
         delete child;
   }
   inline CBookmarkNode &operator=(CBookmarkNode &n2)
   {
      id = n2.id;
      text = n2.text;
      url = n2.url;
      nick = n2.nick;
      desc = n2.desc;
      charset = n2.charset;
      type = n2.type;
      flags = n2.flags;
      addDate = n2.addDate;
      lastVisit = n2.lastVisit;
      lastModified = n2.lastModified;

      if (child) {
         delete child;
      }
      if (next) {
         delete next;
      }

      if (n2.child) {
         child = new CBookmarkNode();
         (*child) = (*n2.child);

         // need to rebuild pointer to lastChild - can't just grab it from n2
         lastChild = child;
         while (lastChild->next) lastChild = lastChild->next;
      }
      else {
         child = NULL;
      }

      if (n2.next) {
         next = new CBookmarkNode();
         (*next) = (*n2.next);
      }
      else {
         next = NULL;
      }

      return *this;
   }
   inline void AddChild(CBookmarkNode *newChild)
   {
      if (child) {
         lastChild->next = newChild;
      }
      else {
         child = newChild;
      }
      lastChild = newChild;
   }
   BOOL UnlinkNode(CBookmarkNode *node)
   {
      CBookmarkNode *c;
      CBookmarkNode *previous = NULL;

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
   BOOL DeleteNode(CBookmarkNode *node)
   {
      if (UnlinkNode(node)) {
         delete node;
         return true;
      }
      return false;
   }
   CBookmarkNode *FindNode(int id)
   {
      CBookmarkNode *c;
      for (c=child; c; c=c->next) {
         if (c->type == BOOKMARK_SEPARATOR) {
            continue;
         }
         else if (c->type == BOOKMARK_FOLDER) {
// FIXME - they currently could be stored out of order after being shuffled around in the edit dialog
// so we can't use this optimization.
// - Perhaps re-ID bookmarks after edit?  It might be worth it, if speed becomes an issue.
/*
            // this little bit of code is for optimizations
            // it assumes the items are stored in order.
            // if the items ever get out of order, we need to remove this code
            CBookmarkNode *lc = c->lastChild;
            if (lc && lc->type == BOOKMARK_BOOKMARK && lc->id < id) {
               continue;
            }
*/

            CBookmarkNode *retNode = c->FindNode(id);

            if (retNode) {
               // found it in a sub-node
               return retNode;
            }
         }
         else if (c->id == id) {
            // this is it!
            return c;
         }
      }
//      // We couldn't find it.  Rather than returning null and risking a null pointer crash, return ourself
//      return this;
      // Scratch that.  We return NULL and just fix anything that crashes.
      return NULL;
   }
   CBookmarkNode *FindNick(char *nick)
   {
      CBookmarkNode *c;
      for (c=child; c; c=c->next) {
         if (c->type == BOOKMARK_SEPARATOR) {
            continue;
         }
         else if (c->type == BOOKMARK_FOLDER) {
            if (strcmp((char*)c->nick.c_str(), nick) == 0) {
               // this is it!
               return c;
            }

            CBookmarkNode *retNode = c->FindNick(nick);

            if (retNode) {
               // found it in a sub-node
               return retNode;
            }
         }
         else if (strcmp((char*)c->nick.c_str(), nick) == 0) {
            // this is it!
            return c;
         }
      }
      return NULL;
   }
   CBookmarkNode *FindSpecialNode(int flag)
   {
      CBookmarkNode *c;
      for (c=child; c; c=c->next) {
         if (c->flags & flag) {
            // this is it!
            return c;
         }
         else if (c->type == BOOKMARK_FOLDER) {
            CBookmarkNode *retNode = c->FindSpecialNode(flag);

            if (retNode != c) {
               // found it in a sub-node
               return retNode;
            }
         }
      }
      // We couldn't find it.  Rather than returning null and risking a null pointer crash, return ourself
      return this;
   }
   int Index(int &mypos, CBookmarkNode *node)
   {
      CBookmarkNode *c;
      for (c=child; c; c=c->next) {
         mypos++;
         if (c->type == BOOKMARK_SEPARATOR)
            continue;
         else if (node == c)
               return mypos;

         if (c->type == BOOKMARK_FOLDER) {
            int newpos = c->Index(mypos, node);
            if (newpos >= 0)
               return newpos;
         }
      }
      return -1;
   }
   int Search(const char *str, int pos, int &mypos, int &firstpos, CBookmarkNode **node)
   {
      CBookmarkNode *c;
      for (c=child; c; c=c->next) {
         mypos++;
         if (c->type == BOOKMARK_SEPARATOR) {
            continue;
         }
         else if (stristr(c->text.c_str(), str) ||
		  stristr(c->url.c_str(), str) ||
		  stristr(c->nick.c_str(), str) ||
		  stristr(c->desc.c_str(), str)) {
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

         if (c->type == BOOKMARK_FOLDER) {

            int newpos = c->Search(str, pos, mypos, firstpos, node);

            if (newpos >= pos) {
               // found it in a sub-node
               return newpos;
            }
         }
      }
      return firstpos;
   }
   void sort(int sortorder)
   {
      if (!child)
        return;

      CBookmarkNode *c;
      int i = 0;
      for (c=child; c; c=c->next)
         i++;
      if (i == 0)
         return;
      void **buf = (void**)calloc(i, sizeof(void*));
      if (!buf)
         return;
      for (i=0,c=child; c; c=c->next,i++) {
         buf[i] = (void*)c;
      }
      quicksort((char*)buf, i, sizeof(void*), &compareBookmarks, sortorder);
      child = ((CBookmarkNode*)buf[0]);
      for (int j=0; j<i-1; j++) {
         c = ((CBookmarkNode*)buf[j]);
         c->next = ((CBookmarkNode*)buf[j+1]);
      }
      c = ((CBookmarkNode*)buf[i-1]);
      c->next = NULL;
      lastChild = c;
      free(buf);
      for (c=child; c; c=c->next) {
         if (c->type == BOOKMARK_FOLDER && (sortorder & 1)) {
            c->sort(sortorder);
         }
      }
   }
};

static int compareBookmarks(const char *e1, const char *e2, unsigned int sortorder)
{
#define SORT_BITS 3
     int cmp = 0;

     if (e1 == e2) return 0;
     if (!e1) return -1;
     if (!e2) return 1;

     int recurse = (sortorder & 1);
     int order = ((sortorder>>1) & ((1<<SORT_BITS)-1));
     int neworder = ((sortorder >> (SORT_BITS+1)) << 1) | recurse;

     CBookmarkNode *c1 = (CBookmarkNode *)*((void**)e1);
     CBookmarkNode *c2 = (CBookmarkNode *)*((void**)e2);

     switch (order) {
       case 1:
          cmp = c1->type - c2->type;
          break;
       case 2:
          cmp = stricmp((char*)c1->text.c_str(), (char*)c2->text.c_str());
          break;
       case 3:
          cmp = stricmp((char*)c1->url.c_str(), (char*)c2->url.c_str());
          break;
       case 4:
         cmp = c2->id - c1->id;
         return cmp ? cmp : -1;
       case 5:
          cmp = c2->type - c1->type;
          break;
       case 6:
          cmp = stricmp((char*)c2->text.c_str(), (char*)c1->text.c_str());
          break;
       case 7:
          cmp = stricmp((char*)c2->url.c_str(), (char*)c1->url.c_str());
          break;
       default:
         cmp = c1->id - c2->id;
         return cmp ? cmp : -1;
         break;
     }
     if (cmp == 0)
       return compareBookmarks(e1, e2, neworder);
     return cmp;
}

#endif
