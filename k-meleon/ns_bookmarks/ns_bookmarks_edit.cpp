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

#include "../rebar_menu/hot_tracking.h"

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

// Our functions

static void FillTree(HWND hTree, HTREEITEM parent, CBookmarkNode &node);
static void OnSize(int height, int width);
static void OnDrop(HWND hTree);
static void OnRClick(HWND hTree);


// Local vars

static HCURSOR hCursorDrag;
static BOOL bDragging;
static HWND hEditWnd;

static CBookmarkNode workingBookmarks; // this will hold a copy of the bookmarks for us to fuck with

CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg){
   case WM_INITDIALOG:
      {
         hEditWnd = hDlg;

         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         TreeView_SetImageList(hTree, gImagelist, TVSIL_NORMAL);

         workingBookmarks = gBookmarkRoot;

         FillTree(hTree, NULL, workingBookmarks);

         hCursorDrag = LoadCursor(kPlugin.hDllInstance, MAKEINTRESOURCE(IDC_DRAG_CURSOR));
         bDragging = false;

         RECT rect;
         GetClientRect(hDlg, &rect);
         OnSize(rect.bottom, rect.right);
      }
      return false;
   case WM_NOTIFY:
      {
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         NMTREEVIEW *nmtv = (NMTREEVIEW *)lParam;

         // Selection changed, save the old url, put the new url into the box
         if (nmtv->hdr.code == TVN_SELCHANGED){
            CBookmarkNode *oldNode = (CBookmarkNode *)nmtv->itemOld.lParam;

            if (oldNode && oldNode->type == BOOKMARK_BOOKMARK){
               char buffer[1025];
               GetDlgItemText(hDlg, IDC_EDIT_URL, buffer, 1024);

               if (oldNode->url.compare(buffer) != 0) {
                  oldNode->url = buffer;
                  gBookmarksModified = true;
               }
            }

            CBookmarkNode *newNode = (CBookmarkNode *)nmtv->itemNew.lParam;
            if (newNode && newNode->type == BOOKMARK_BOOKMARK){
               SetDlgItemText(hDlg, IDC_EDIT_URL, newNode->url.c_str());
            }
            else{
               SetDlgItemText(hDlg, IDC_EDIT_URL, "");
            }
         }
         // start a drag operation
         else if (nmtv->hdr.code == TVN_BEGINDRAG){
            TreeView_SelectItem(hTree, nmtv->itemNew.hItem);

            SetCursor(hCursorDrag);
            bDragging = true;
            SetCapture(hDlg);
         }
         else if (nmtv->hdr.code == NM_DBLCLK){
            TVHITTESTINFO hti;
            GetCursorPos(&hti.pt);
            ScreenToClient(hTree, &hti.pt);

            HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
            if (hItem){
               TVITEMEX item;
               item.hItem = hItem;
               item.mask = TVIF_PARAM;
               TreeView_GetItem(hTree, &item);

               CBookmarkNode *node = (CBookmarkNode *)item.lParam;

               if (node->type == BOOKMARK_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  kPlugin.kf->NavigateTo(node->url.c_str(), OPEN_NORMAL);

                  if (gBookmarksModified) {
                     if (MessageBox(hDlg, BOOKMARKS_SAVE_CHANGES, PLUGIN_NAME, MB_YESNO) == IDYES) {
                        // save the changes
                        gBookmarkRoot = workingBookmarks;
                        EndDialog(hDlg, 1);
                        break;
                     }
                     else {
                        EndDialog(hDlg, 0);
                        break;
                     }
                  }
                  EndDialog(hDlg, 0);
               }
            }
            break;
         }
         // right click...
         else if (nmtv->hdr.code == NM_RCLICK){
            OnRClick(hTree);
         }
         // start a label edit.  only let them edit non-separators
         else if (nmtv->hdr.code == TVN_BEGINLABELEDIT){
            NMTVDISPINFO *nmtvdi = (NMTVDISPINFO *)nmtv;
            CBookmarkNode *node = (CBookmarkNode *)nmtvdi->item.lParam;
            if (node->type == BOOKMARK_SEPARATOR) {
               // return true to disallow the edit
               SetWindowLong(hDlg, DWL_MSGRESULT, true);
            }
            else {
               SetWindowLong(hDlg, DWL_MSGRESULT, false);
            }
            return true;
         }
         // end a label edit.  save the name
         else if (nmtv->hdr.code == TVN_ENDLABELEDIT){
            NMTVDISPINFO *nmtvdi = (NMTVDISPINFO *)nmtv;
            if (nmtvdi->item.pszText){
               CBookmarkNode *node = (CBookmarkNode *)nmtvdi->item.lParam;
               if (node->type == BOOKMARK_BOOKMARK){
                  // if this is a bookmark, save the name
                  node->text = nmtvdi->item.pszText;
               }
               gBookmarksModified = true;
            }
            SetWindowLong(hDlg, DWL_MSGRESULT, true);
         }
         return true;
      }
      break;
   case WM_MOUSEMOVE:
      // update the insertion mark if we're dragging
      if (bDragging){
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);

         TVHITTESTINFO hti;
         GetCursorPos(&hti.pt);
         ScreenToClient(hTree, &hti.pt);

         HTREEITEM item = TreeView_HitTest(hTree, &hti);
         if (item){
            TreeView_SetInsertMark(hTree, item, false);
         }
         else {
            item = TreeView_GetLastVisible(hTree);
            TreeView_SetInsertMark(hTree, item, true);
         }
         return true;
      }
      break;
   case WM_LBUTTONUP:
      if (bDragging){
         OnDrop(GetDlgItem(hDlg, IDC_TREE_BOOKMARK));
      }
      break;

   case WM_SIZE:
      if(wParam != SIZE_MINIMIZED) {
		   OnSize(HIWORD(lParam), LOWORD(lParam));
      }
      break;

   case WM_GETMINMAXINFO:
      LPMINMAXINFO lpminmaxinfo;
      lpminmaxinfo=(LPMINMAXINFO)lParam;
      lpminmaxinfo->ptMinTrackSize.x = 300;
      lpminmaxinfo->ptMinTrackSize.y = 300;
      break;

   case WM_COMMAND:
      {
         if (HIWORD(wParam) == BN_CLICKED){
            WORD id = LOWORD(wParam);
            switch(id){
            case IDOK:
               {
                  // we need to save the url in the url box before we exit
                  
                  HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
                  HTREEITEM hSelection = TreeView_GetSelection(hTree);
                  if (hSelection) {
                     TVITEM item;
                     item.hItem = hSelection;
                     item.mask = TVIF_PARAM;
                     TreeView_GetItem(hTree, &item);

                     CBookmarkNode *node = (CBookmarkNode *)item.lParam;
                     if (node->type == BOOKMARK_BOOKMARK) {
                        char buffer[1025];
                        GetDlgItemText(hDlg, IDC_EDIT_URL, buffer, 1024);

                        if (node->url.compare(buffer) != 0) {
                           node->url = buffer;
                           gBookmarksModified = true;
                        }
                     }
                  }
               }
               // save the changes
               gBookmarkRoot = workingBookmarks;

               EndDialog(hDlg, 1);
               break;

            case IDCANCEL:
               gBookmarksModified = false;

               EndDialog(hDlg, 0);
               break;
            }
         }
      }
   }
   return false;
}

static void FillTree(HWND hTree, HTREEITEM parent, CBookmarkNode &node)
{
   TVINSERTSTRUCT tvis;
   tvis.hParent = parent;
   tvis.hInsertAfter = NULL;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   tvis.itemex.iImage = IMAGE_BLANK;
   tvis.itemex.iSelectedImage = IMAGE_BLANK;

   int type;
   CBookmarkNode *child;
   for (child=node.child; child; child=child->next) {

      tvis.itemex.lParam = (long)child;

      type = child->type;
      if (type == BOOKMARK_FOLDER) {
         tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
         tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
         tvis.itemex.pszText = (char *)child->text.c_str();

         HTREEITEM thisItem = TreeView_InsertItem(hTree, &tvis);

         FillTree(hTree, thisItem, *child);
      }
      else if (type == BOOKMARK_FOLDER_TB) {
         tvis.itemex.iImage = IMAGE_FOLDER_TB_CLOSED;
         tvis.itemex.iSelectedImage = IMAGE_FOLDER_TB_OPEN;
         tvis.itemex.pszText = (char *)child->text.c_str();

         HTREEITEM thisItem = TreeView_InsertItem(hTree, &tvis);

         FillTree(hTree, thisItem, *child);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         tvis.itemex.iImage = IMAGE_BLANK;
         tvis.itemex.iSelectedImage = IMAGE_BLANK;
         tvis.itemex.pszText = "-";
         TreeView_InsertItem(hTree, &tvis);
      }
      else {
         tvis.itemex.iImage = IMAGE_BOOKMARK;
         tvis.itemex.iSelectedImage = IMAGE_BOOKMARK;
         tvis.itemex.pszText = (char *)child->text.c_str();
         TreeView_InsertItem(hTree, &tvis);
      }
   }
}

static void CopyBranch(HWND hTree, HTREEITEM item, HTREEITEM newParent)
{
   char buffer[MAX_PATH];

   TVINSERTSTRUCT tvis;
   tvis.hParent = newParent;
   tvis.hInsertAfter = TVI_LAST;
   tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
   tvis.item.pszText = buffer;
   tvis.item.cchTextMax = MAX_PATH;

   HTREEITEM newItem;

   tvis.item.hItem = TreeView_GetChild(hTree, item);
   while(tvis.item.hItem) {
      TreeView_GetItem(hTree, &tvis.item);
      newItem = TreeView_InsertItem(hTree, &tvis);

      if (tvis.item.cChildren) {
         CopyBranch(hTree, tvis.item.hItem, newItem);
      }

      tvis.item.hItem = TreeView_GetNextSibling(hTree, tvis.item.hItem);
   }
}

static void OnDrop(HWND hTree)
{
   TreeView_SetInsertMark(hTree, NULL, false);

   HTREEITEM item = TreeView_GetSelection(hTree);
   if (item){
      char buffer[MAX_PATH];

      // get the current item
      TVINSERTSTRUCT tvis;
      tvis.item.hItem = item;
      tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
      tvis.item.pszText = buffer;
      tvis.item.cchTextMax = MAX_PATH;
      TreeView_GetItem(hTree, &tvis.item);

      // create a new copy

      TVHITTESTINFO hti;
      GetCursorPos(&hti.pt);
      ScreenToClient(hTree, &hti.pt);

      HTREEITEM itemDrop = TreeView_HitTest(hTree, &hti);
      if (itemDrop) {
         tvis.hParent = TreeView_GetParent(hTree, itemDrop);
         tvis.hInsertAfter = TreeView_GetPrevSibling(hTree, itemDrop);
         if (!tvis.hInsertAfter)
            tvis.hInsertAfter = TVI_FIRST;
      }
      else {
         itemDrop = TreeView_GetLastVisible(hTree);
         tvis.hParent = TreeView_GetParent(hTree, itemDrop);
         tvis.hInsertAfter = TVI_LAST;
      }

      HTREEITEM newParent = TreeView_InsertItem(hTree, &tvis);

      // copy it's children
      CopyBranch(hTree, item, newParent);

      // then delete the item
      TreeView_DeleteItem(hTree, item);

      gBookmarksModified = true;
   }

   SetCursor(LoadCursor(NULL, IDC_ARROW));
   bDragging = false;
   ReleaseCapture();
}

static void OnRClick(HWND hTree)
{
   POINT mouse;
   GetCursorPos(&mouse);

   HMENU topMenu = LoadMenu(kPlugin.hDllInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));
   HMENU contextMenu = GetSubMenu(topMenu, 0);
   int command = TrackPopupMenu(contextMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD, mouse.x, mouse.y, 0, hTree, NULL);
   if (command == ID_BOOKMARK_DELETE){ // delete
      TVHITTESTINFO hti;
      hti.pt.x = mouse.x;
      hti.pt.y = mouse.y;
      ScreenToClient(hTree, &hti.pt);

      HTREEITEM item = TreeView_HitTest(hTree, &hti);
      if (item){
         TVITEMEX itemData;
         itemData.mask = TVIF_PARAM;
         itemData.hItem = item;
         TreeView_GetItem(hTree, &itemData);

         CBookmarkNode *parentNode;
         HTREEITEM parent = TreeView_GetParent(hTree, item);
         if (parent){
            TVITEMEX parentItem;
            parentItem.mask = TVIF_PARAM;
            parentItem.hItem = parent;
            TreeView_GetItem(hTree, &parentItem);

            parentNode = (CBookmarkNode *)parentItem.lParam;
         }
         else{
            parentNode = &workingBookmarks;
         }
         CBookmarkNode *node = (CBookmarkNode *)itemData.lParam;

         parentNode->DeleteNode(node);

         TreeView_DeleteItem(hTree, item);

         gBookmarksModified = true;
      }
   }
   else if (command = ID_SET_TOOLBAR_FOLDER){
      TVHITTESTINFO hti;
      hti.pt.x = mouse.x;
      hti.pt.y = mouse.y;
      ScreenToClient(hTree, &hti.pt);

      HTREEITEM item = TreeView_HitTest(hTree, &hti);
      if (item){
         TVITEMEX itemData;
         itemData.mask = TVIF_PARAM;
         itemData.hItem = item;
         TreeView_GetItem(hTree, &itemData);

         workingBookmarks.FindToolbarNode()->type = BOOKMARK_FOLDER;

         CBookmarkNode *node = (CBookmarkNode *)itemData.lParam;
         if (node->type == BOOKMARK_FOLDER){ // this is a submenu
            node->type = BOOKMARK_FOLDER_TB;
         }
         else{ // this is an item, set the toolbar folder to it's parent
            HTREEITEM parent = TreeView_GetParent(hTree, item);
            if (parent){
               TVITEMEX parentItem;
               parentItem.mask = TVIF_PARAM;
               parentItem.hItem = parent;
               TreeView_GetItem(hTree, &parentItem);

               ((CBookmarkNode *)parentItem.lParam)->type = BOOKMARK_FOLDER_TB;
            }
            else{
               // the item's parent is the root, don't do anything
            }
         }

         gBookmarksModified = true;
      }
   }
}

#define BORDER 7
#define OK_WIDTH 64
#define CANCEL_WIDTH 64

static void OnSize(int height, int width)
{
   static int oldheight = 0, oldwidth = 0;

   if ((height == oldheight) && (width == oldwidth))
      return;

   HWND hWndChild;
   RECT rect;
   int newwidth, newheight;

   int editboxHeight;

   // resize and move edit box
   hWndChild = GetDlgItem(hEditWnd, IDC_EDIT_URL);
   GetWindowRect(hWndChild, &rect); // use GWR rather than GetClientRect because we need the frame too
   editboxHeight = rect.bottom - rect.top;

   newwidth = width - ((BORDER*3) + OK_WIDTH + CANCEL_WIDTH);
   MoveWindow(hWndChild, BORDER, height-(BORDER+editboxHeight), newwidth, editboxHeight, TRUE);

   // resize tree
   hWndChild = GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK);
   newwidth = width - (BORDER*2);
   newheight = height - ((BORDER*3) + editboxHeight);
   MoveWindow(hWndChild, BORDER, BORDER, newwidth, newheight, TRUE);

   // move ok button
   hWndChild = GetDlgItem(hEditWnd, IDOK);
   MoveWindow(hWndChild, width-(BORDER+CANCEL_WIDTH+OK_WIDTH), height-(BORDER+editboxHeight), OK_WIDTH, editboxHeight, TRUE);

   // move cancel button
   hWndChild = GetDlgItem(hEditWnd, IDCANCEL);
   MoveWindow(hWndChild, width-(BORDER+CANCEL_WIDTH), height-(BORDER+editboxHeight), CANCEL_WIDTH, editboxHeight, TRUE);

	oldheight = height;
	oldwidth = width;
}
