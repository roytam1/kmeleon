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
#include "wininet.h"    // for INTERNET_MAX_URL_LENGTH
#include "windows.h"    // for hook functions

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
static void OnRClick(HWND hTree);
static void ImportFavorites(HWND hTree);
static void BuildFavoritesTree(TCHAR *FavoritesPath, char* strPath, CBookmarkNode *newFavoritesNode);
static void MoveItem(HWND hTree, HTREEITEM item, int mode);
static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type);
static void ChangeSpecialFolder(HWND hTree, HTREEITEM *htiOld, HTREEITEM htiNew, int flag);
static void DeleteItem(HWND hTree, HTREEITEM item);
static void UpdateTitle(HWND hDlg, HTREEITEM item);
static void UpdateURL(HWND hDlg, HTREEITEM item);

// Local vars

static HCURSOR hCursorDrag;
static BOOL bDragging;
static HWND hEditWnd;

static CBookmarkNode workingBookmarks; // this will hold a copy of the bookmarks for us to fuck with
static BOOL bookmarksEdited;    // separate from gBookmarksModified - only tracks changes within edit dialog

static HTREEITEM hTBitem;   // current toolbar folder treeview item
static HTREEITEM hNBitem;   // current new bookmark folder treeview item
static HTREEITEM hBMitem;   // current bookmark menu folder treeview item

// Commonly used functions

static inline CBookmarkNode *GetBookmarkNode(HWND hTree, HTREEITEM htItem) {
   TVITEMEX tvItem;
   tvItem.mask = TVIF_PARAM;
   tvItem.hItem = htItem;
   TreeView_GetItem(hTree, &tvItem);

   return (CBookmarkNode *)tvItem.lParam;
}

static inline void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst) {
   // Note that LONGLONG is a 64-bit value
   LONGLONG ll;
   FILETIME ft1;
   FILETIME ft2;

   ll = Int32x32To64(t, 10000000) + 116444736000000000;
   ft1.dwLowDateTime = (DWORD)ll;
   ft1.dwHighDateTime = ll >> 32;

   FileTimeToLocalFileTime(&ft1, &ft2);
   FileTimeToSystemTime(&ft2, pst);
}


// Hook function for keyboard hook - snags SHIFT-UP and SHIFT-DOWN in the treeview, only.
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
   BOOL fEatKeystroke = false;
   HWND hTree = GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK);

   if (nCode == HC_ACTION
      && (GetKeyState(VK_SHIFT) & 0x80)
      && !(lParam & 0x80000000)
      && GetFocus() == hTree
   ) {
      HTREEITEM hSelection;
      switch (wParam) {
      case VK_UP:
         fEatKeystroke = true;
         hSelection = TreeView_GetSelection(hTree);
         if (hSelection) MoveItem(hTree, hSelection, 1);
         break;
      case VK_DOWN:
         fEatKeystroke = true;
         hTree = GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK);
         hSelection = TreeView_GetSelection(hTree);
         if (hSelection) MoveItem(hTree, hSelection, 2);
         break;
      }
   }

   return(fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam));
}

CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HHOOK hHook;
   static bool bTimer = false;  // semi-crude hack to make scrolling smoother
   static HTREEITEM htCurHover = NULL;
   static HTREEITEM htDummyItem = NULL;

   switch (uMsg){
   case WM_INITDIALOG:
      {
         hEditWnd = hDlg;

         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         TreeView_SetImageList(hTree, gImagelist, TVSIL_NORMAL);

         workingBookmarks = gBookmarkRoot;
         bookmarksEdited = false;

         TVINSERTSTRUCT tvis;
         tvis.hParent = NULL;
         tvis.hInsertAfter = NULL;
         tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
         tvis.itemex.iImage = IMAGE_FOLDER_SPECIAL_CLOSED;
         tvis.itemex.iSelectedImage = IMAGE_FOLDER_SPECIAL_OPEN;
         tvis.itemex.pszText = "All Bookmarks";
         tvis.itemex.lParam = (long)&workingBookmarks;

         HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);

         // root starts out with all specials set to it - FillTree will unset any that are set elsewhere with ChangeSpecialFolders
         hTBitem = hNBitem = hBMitem = newItem;
         workingBookmarks.flags = BOOKMARK_FLAG_TB | BOOKMARK_FLAG_NB | BOOKMARK_FLAG_BM;

         FillTree(hTree, newItem, workingBookmarks);

         TreeView_Expand(hTree, newItem, TVE_EXPAND);

         hCursorDrag = LoadCursor(kPlugin.hDllInstance, MAKEINTRESOURCE(IDC_DRAG_CURSOR));
         bDragging = false;

//         RECT rect;
//         GetClientRect(hDlg, &rect);
//         OnSize(rect.bottom, rect.right);

         hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, GetCurrentThreadId());
      }
      return false;
   case WM_NOTIFY:
      {
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         NMTREEVIEW *nmtv = (NMTREEVIEW *)lParam;

         // Selection changed
         if (nmtv->hdr.code == TVN_SELCHANGED){
            // Put the new url/title into the box
            CBookmarkNode *newNode = (CBookmarkNode *)nmtv->itemNew.lParam;

            if (newNode == &workingBookmarks || newNode->type == BOOKMARK_SEPARATOR) {
               // root and separators have nothing
               SetDlgItemText(hDlg, IDC_TITLE, "");
               SetDlgItemText(hDlg, IDC_URL, "");
               SetDlgItemText(hDlg, IDC_ADDED, "");
               SetDlgItemText(hDlg, IDC_LAST_VISIT, "");
               SetDlgItemText(hDlg, IDC_OTHER, "");
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TITLE), false);
               EnableWindow(GetDlgItem(hDlg, IDC_TITLE), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ADDED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_OTHER), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), false);
               return true;   // a lazy-man's "else"
            }

            // everything else has at least title, added date, and properties in general
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TITLE), true);
            EnableWindow(GetDlgItem(hDlg, IDC_TITLE), true);
            SetDlgItemText(hDlg, IDC_TITLE, newNode->text.c_str());
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ADDED), true);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), true);

            SYSTEMTIME st;
            char pszTmp[1024];
            char pszDate[900];

            if (newNode->type == BOOKMARK_BOOKMARK) {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), true);
               SetDlgItemText(hDlg, IDC_URL, newNode->url.c_str());

               if (newNode->lastVisit) {
                  UnixTimeToSystemTime(newNode->lastVisit, &st);
                  GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
                  strcpy(pszTmp, pszDate);
                  strcat(pszTmp, " ");
                  GetTimeFormat(GetThreadLocale(), NULL, &st, NULL, pszDate, 899);
                  strcat(pszTmp, pszDate);
                  SetDlgItemText(hDlg, IDC_LAST_VISIT, pszTmp);
               }
               else {
                  SetDlgItemText(hDlg, IDC_LAST_VISIT, "Never");
               }
            }
            else {
               SetDlgItemText(hDlg, IDC_URL, "");
               SetDlgItemText(hDlg, IDC_LAST_VISIT, "");
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
            }

            UnixTimeToSystemTime(newNode->addDate, &st);
            GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
            strcpy(pszTmp, pszDate);
            strcat(pszTmp, " ");
            GetTimeFormat(GetThreadLocale(), NULL, &st, NULL, pszDate, 899);
            strcat(pszTmp, pszDate);
            SetDlgItemText(hDlg, IDC_ADDED, pszTmp);

            strcpy(pszTmp, "");
            if (newNode->flags & BOOKMARK_FLAG_TB) {
               strcat(pszTmp, "Toolbar Folder\n");
            }
            if (newNode->flags & BOOKMARK_FLAG_NB) {
               strcat(pszTmp, "New Bookmarks Folder\n");
            }
            if (newNode->flags & BOOKMARK_FLAG_BM) {
               strcat(pszTmp, "Bookmark Menu\n");
            }
            if (pszTmp[0]) {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_OTHER), true);
               SetDlgItemText(hDlg, IDC_OTHER, pszTmp);
            }
            else {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_OTHER), false);
               SetDlgItemText(hDlg, IDC_OTHER, "");
            }
         }
         // start a drag operation
         else if (nmtv->hdr.code == TVN_BEGINDRAG){
            TreeView_SelectItem(hTree, nmtv->itemNew.hItem);
            // don't move the root folder (thus, only move something if it has a parent)
            if (TreeView_GetParent(hTree, nmtv->itemNew.hItem)) {
               SetCursor(hCursorDrag);
               bDragging = true;
               SetCapture(hDlg);
            }
         }
         // key pressed
         else if (nmtv->hdr.code == TVN_KEYDOWN) {
            HTREEITEM hSelection = TreeView_GetSelection(hTree);
            if (hSelection) {
               switch(((LPNMTVKEYDOWN)lParam)->wVKey) {
               case VK_DELETE:
                  DeleteItem(hTree, hSelection);
                  break;
               case VK_INSERT:
                  CreateNewObject(hTree, hSelection, BOOKMARK_BOOKMARK);
                  break;
               }
            }
         }
         else if (nmtv->hdr.code == NM_DBLCLK){
            TVHITTESTINFO hti;
            GetCursorPos(&hti.pt);
            ScreenToClient(hTree, &hti.pt);

            HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
            if (hItem){
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);

               if (node->type == BOOKMARK_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  bookmarksEdited = true;
                  kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL);

                  if (bookmarksEdited && MessageBox(hDlg, BOOKMARKS_SAVE_CHANGES, PLUGIN_NAME, MB_YESNO) == IDYES) {
                     // save the changes
                     gBookmarkRoot = workingBookmarks;
                     Save(gBookmarkFile);
                     UnhookWindowsHookEx(hHook);
                     EndDialog(hDlg, 1);
                     return true;
                  }
                  UnhookWindowsHookEx(hHook);
                  EndDialog(hDlg, 0);
                  return true;
               }
            }
         }
         // right click...
         else if (nmtv->hdr.code == NM_RCLICK){
            OnRClick(hTree);
         }
         return true;
      }
      break;

   case WM_TIMER:
      {
         // only one-time timers - will reset selves if necessary
         KillTimer(hDlg, wParam);

         if (wParam < 2) {
            // scrolling, and wParam conveniently holds the direction
            bTimer = false;
            HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
            SendMessage(hTree, WM_VSCROLL, wParam, NULL);
         }
         else if (wParam == 2) {
            // need to expand folder
            HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);

            // the folder might be empty, in which case we add an empty dummy item to allow it to expand
            if (!TreeView_GetChild(hTree, htCurHover)) {
               TVINSERTSTRUCT tvis;
               tvis.hParent = htCurHover;
               tvis.hInsertAfter = TVI_FIRST;
               tvis.itemex.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
               tvis.itemex.iImage = IMAGE_BLANK;
               tvis.itemex.iSelectedImage = IMAGE_BLANK;
               htDummyItem = TreeView_InsertItem(hTree, &tvis);
            }

            TreeView_SetInsertMark(hTree, NULL, false); // clean up insertion mark before expand messes it up
            TreeView_Expand(hTree, htCurHover, TVE_EXPAND);
         }
      }
      // no break, because we want to update the insertion mark and set timers again if needed

   case WM_MOUSEMOVE:
      // update the insertion mark if we're dragging
      if (bDragging){
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);

         // find current coordinates
         TVHITTESTINFO hti;
         GetCursorPos(&hti.pt);
         ScreenToClient(hTree, &hti.pt);

         RECT tvRect;

         // set timers to scroll, if dragging to top/bottom
         GetClientRect(hTree, &tvRect);
         if (!bTimer && hti.pt.y >= 0 && hti.pt.y < 25) {
            SetTimer(hDlg,
               0,                 // timer id - passed as wParam through WM_TIMER to WM_VSCROLL (thus, 0=up, 1=down)
               50,                // 0.05-second interval 
               (TIMERPROC) NULL); // no timer callback 
            bTimer = true;
         }
         else if (!bTimer && tvRect.bottom - hti.pt.y >= 0 && tvRect.bottom - hti.pt.y < 25) {
            SetTimer(hDlg, 1, 50, (TIMERPROC) NULL);
            bTimer = true;
         }

         HTREEITEM item = TreeView_HitTest(hTree, &hti);
         if (item) {

            if (item != htCurHover) {
               // it might be an empty folder, so we unfortunately have to check the node type itself
               // we also need to check if it's expanded or not
               TVITEMEX tvItem;
               tvItem.mask = TVIF_PARAM | TVIF_STATE;
               tvItem.hItem = item;
               TreeView_GetItem(hTree, &tvItem);

               // delete any old dummyitem we have lying around
               if (htDummyItem) {
                   TreeView_Expand(hTree, htCurHover, TVE_COLLAPSE);
                   TreeView_DeleteItem(hTree, htDummyItem);
                   htDummyItem = NULL;

                   // resend the message, to avoid crashing on our dummyitem
                   // (only necessary if item==htDummyItem, but can't hurt to do it either way...)
                   SendMessage(hDlg, uMsg, wParam, lParam);
                   break;
               }

               if (((CBookmarkNode *)(tvItem.lParam))->type == BOOKMARK_FOLDER) {
                  // folder - expand it after hovering for 1 second
                  SetTimer(hDlg, 2, 1000, (TIMERPROC) NULL);
               }
               else {
                  KillTimer(hDlg, 2);
               }

               htCurHover = item;
            }

            if (!TreeView_GetParent(hTree, item)) {
               // item is the root - crudely fake being below it (no moving stuff above root)
               hti.pt.y += 100;
            }

            TreeView_GetItemRect(hTree, item, &tvRect, false);
            if (hti.pt.y < tvRect.top + (tvRect.bottom - tvRect.top)/2) {
               // top half of item - place insertion mark before item
               TreeView_SetInsertMark(hTree, item, false);
            }
            else {
               // bottom half of item
               TVITEMEX tvItem;
               tvItem.mask = TVIF_STATE;
               tvItem.hItem = item;
               TreeView_GetItem(hTree, &tvItem);
               if (tvItem.state & TVIS_EXPANDED) {
                  // expanded folder - place insertion mark before first child
                  TreeView_SetInsertMark(hTree, TreeView_GetChild(hTree, item), false);
               }
               else {
                  // bookmark, separator, or collapsed folder - place insertion mark after item
                  TreeView_SetInsertMark(hTree, item, true);
               }
            }
         }
         else {
            // over no item, default to end of entire list
            item = TreeView_GetLastVisible(hTree);
            TreeView_SetInsertMark(hTree, item, true);
         }
         return true;
      }
      break;

   case WM_LBUTTONUP:
      if (bDragging){
         HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
         HTREEITEM hSelection = TreeView_GetSelection(hTree);
         if (hSelection) MoveItem(hTree, hSelection, 3);

         // cleanup
         TreeView_SetInsertMark(hTree, NULL, false);
         SetCursor(LoadCursor(NULL, IDC_ARROW));
         bDragging = false;
         ReleaseCapture();
         KillTimer(hDlg, 2);
         if (htDummyItem) {
            // if there's a dummy item, we must be over it's parent
            // so we can collapse the curhover if it only has the one child (the dummy item)
            if (!TreeView_GetNextSibling(hTree, TreeView_GetChild(hTree, htCurHover))) {
               TreeView_Expand(hTree, htCurHover, TVE_COLLAPSE);
            }
            TreeView_DeleteItem(hTree, htDummyItem);
            htDummyItem = NULL;
         }
      }
      break;

/*
   case WM_SIZE:
      if(wParam != SIZE_MINIMIZED) {
         OnSize(HIWORD(lParam), LOWORD(lParam));
      }
      break;
*/

   case WM_GETMINMAXINFO:
      LPMINMAXINFO lpminmaxinfo;
      lpminmaxinfo=(LPMINMAXINFO)lParam;
      lpminmaxinfo->ptMinTrackSize.x = 300;
      lpminmaxinfo->ptMinTrackSize.y = 300;
      break;

   case WM_COMMAND:
      {
         if (HIWORD(wParam) == EN_CHANGE) {
            int id = LOWORD(wParam);
            if (id == IDC_TITLE && SendMessage(GetDlgItem(hDlg, IDC_TITLE), EM_GETMODIFY, 0, 0)) {
               SendMessage(GetDlgItem(hDlg, IDC_TITLE), EM_SETMODIFY, FALSE, 0);
               HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateTitle(hDlg, hSelection);
               }
            }
            else if (id == IDC_URL && SendMessage(GetDlgItem(hDlg, IDC_URL), EM_GETMODIFY, 0, 0)) {
               SendMessage(GetDlgItem(hDlg, IDC_URL), EM_SETMODIFY, FALSE, 0);
               HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateURL(hDlg, hSelection);
               }
            }
         }
         else if (HIWORD(wParam) == BN_CLICKED) {
            WORD id = LOWORD(wParam);
            switch(id){
            case ID_IMPORT_FAVORITES:
               ImportFavorites(GetDlgItem(hDlg, IDC_TREE_BOOKMARK));
               break;

            case IDOK:
               if (bookmarksEdited) {
                  gBookmarkRoot = workingBookmarks;
                  Save(gBookmarkFile);

                  Rebuild();
               }

               UnhookWindowsHookEx(hHook);
               EndDialog(hDlg, 1);
               break;

            case IDCANCEL:
               UnhookWindowsHookEx(hHook);
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

         if (child->flags & BOOKMARK_FLAG_TB) {
            ChangeSpecialFolder(hTree, &hTBitem, thisItem, BOOKMARK_FLAG_TB);
         }
         if (child->flags & BOOKMARK_FLAG_BM) {
            ChangeSpecialFolder(hTree, &hBMitem, thisItem, BOOKMARK_FLAG_BM);
         }
         if (child->flags & BOOKMARK_FLAG_NB) {
            ChangeSpecialFolder(hTree, &hNBitem, thisItem, BOOKMARK_FLAG_NB);
         }

         FillTree(hTree, thisItem, *child);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         tvis.itemex.iImage = IMAGE_BLANK;
         tvis.itemex.iSelectedImage = IMAGE_BLANK;
         tvis.itemex.pszText = "---";
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

static void MoveItem(HWND hTree, HTREEITEM item, int mode) {
   if (item == TreeView_GetRoot(hTree)) return;

   CBookmarkNode *moveNode, *oldPreviousNode, *oldParentNode, *newPreviousNode, *newParentNode;
   char buffer[MAX_PATH];

   // setup the insert item
   TVINSERTSTRUCT tvis;
   tvis.item.hItem = item;
   tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
   tvis.item.pszText = buffer;
   tvis.item.cchTextMax = MAX_PATH;
   TreeView_GetItem(hTree, &tvis.item);

   // get the current item's bookmarknode
   moveNode = (CBookmarkNode *)tvis.item.lParam;

   // setup the old parent
   HTREEITEM oldParentItem = TreeView_GetParent(hTree, item);
   if (oldParentItem) {
      oldParentNode = GetBookmarkNode(hTree, oldParentItem);
   }
   else {
      oldParentNode = &workingBookmarks;
   }

   // determine the new location in the treeview
   HTREEITEM itemDrop;
   switch (mode) {
   case 1:  // move up one
      // if we're at the top, do nothing
      if (TreeView_GetPrevVisible(hTree, item) == TreeView_GetRoot(hTree)) return;

      itemDrop = TreeView_GetPrevSibling(hTree, item);

      if (!itemDrop) {  // we are first child of parent - move above parent,
         itemDrop = TreeView_GetPrevSibling(hTree, TreeView_GetParent(hTree, item));
         tvis.hInsertAfter = itemDrop;
         if (!itemDrop) { // parent is first child of parent's parent - place after parent's parent
            itemDrop = TreeView_GetParent(hTree, item);
            tvis.hInsertAfter = TVI_FIRST;
         }

         PostMessage(hTree, WM_VSCROLL, SB_LINEUP, NULL);
      }
      else {
         TVITEMEX tvItem;
         tvItem.mask = TVIF_STATE;
         tvItem.hItem = itemDrop;
         TreeView_GetItem(hTree, &tvItem);
         if (tvItem.state & TVIS_EXPANDED) {    // we're below an expanded folder - move to last child of folder
            itemDrop = TreeView_GetPrevVisible(hTree, item);
            tvis.hInsertAfter = itemDrop;
         }
         else {   // otherwise, just place after prev siblings's prev sibling
            itemDrop = TreeView_GetPrevSibling(hTree, itemDrop);
            tvis.hInsertAfter = itemDrop;
            if (!itemDrop) {  // item is the second child
               itemDrop = item;
               tvis.hInsertAfter = TVI_FIRST;
            }

            PostMessage(hTree, WM_VSCROLL, SB_LINEUP, NULL);
         }
      }

      if (tvis.hInsertAfter == TVI_FIRST) {
         newPreviousNode = NULL;
      }
      else {
         newPreviousNode = GetBookmarkNode(hTree, tvis.hInsertAfter);
      }

      break;
   case 2:  // move down one
      if (!TreeView_GetNextVisible(hTree, item)) return;  // do nothing if we're at the very end
      itemDrop = TreeView_GetNextSibling(hTree, item);
      if (itemDrop) {
         TVITEMEX tvItem;
         tvItem.mask = TVIF_STATE;
         tvItem.hItem = itemDrop;
         TreeView_GetItem(hTree, &tvItem);
         if (tvItem.state & TVIS_EXPANDED) {
            // expanded folder - insert before first child
            tvis.hInsertAfter = NULL;
            itemDrop = TreeView_GetChild(hTree, itemDrop);
         }
         else {
            // bookmark or collapsed folder - insert after item
            tvis.hInsertAfter = itemDrop;
         }

         if (tvis.hInsertAfter) {
            newPreviousNode = GetBookmarkNode(hTree, tvis.hInsertAfter);
         }
         else {
            newPreviousNode = NULL;
            tvis.hInsertAfter = TVI_FIRST;
         }

         PostMessage(hTree, WM_VSCROLL, SB_LINEDOWN, NULL);
      }
      else {
         // if there is no next sibling, try moving it down a level (place it after its parent)
         itemDrop = TreeView_GetParent(hTree, item);

         // if we're at the end of the list, we'll be trying to place after the root - just return
         if (itemDrop == TreeView_GetRoot(hTree)) return;

         tvis.hInsertAfter = itemDrop;
         newPreviousNode = GetBookmarkNode(hTree, tvis.hInsertAfter);
      }

      break;
   case 3:  // move to drag'n'drop location
      TVHITTESTINFO hti;
      GetCursorPos(&hti.pt);
      ScreenToClient(hTree, &hti.pt);

      itemDrop = TreeView_HitTest(hTree, &hti);
      if (itemDrop) {
         if (!TreeView_GetParent(hTree, itemDrop)) {
            // item is the root - crudely fake being below it (no moving stuff above root)
            hti.pt.y += 100;
         }
         RECT tvRect;
         TreeView_GetItemRect(hTree, itemDrop, &tvRect, false);
         if (hti.pt.y < tvRect.top + (tvRect.bottom - tvRect.top)/2) {
            // top half of item - insert before item
            tvis.hInsertAfter = TreeView_GetPrevSibling(hTree, itemDrop);
         }
         else {
            // bottom half of item
            TVITEMEX tvItem;
            tvItem.mask = TVIF_STATE;
            tvItem.hItem = itemDrop;
            TreeView_GetItem(hTree, &tvItem);
            if (tvItem.state & TVIS_EXPANDED) {
               // expanded folder - insert before first child
               tvis.hInsertAfter = NULL;
               itemDrop = TreeView_GetChild(hTree, itemDrop);
            }
            else {
               // bookmark or collapsed folder - insert after item
               tvis.hInsertAfter = itemDrop;
            }
         }

         if (tvis.hInsertAfter) {
            newPreviousNode = GetBookmarkNode(hTree, tvis.hInsertAfter);
         }
         else {
            newPreviousNode = NULL;
            tvis.hInsertAfter = TVI_FIRST;
         }
      }
      else {
         // default to end of entire tree...  is this desirable?  e.g., what if the user drags it out of the treeview area, we just throw it on the end???
         tvis.hInsertAfter = TVI_LAST;

         itemDrop = TreeView_GetLastVisible(hTree);

         newPreviousNode = GetBookmarkNode(hTree, itemDrop);
      }

      break;
   }

   // setup the new parent
   tvis.hParent = TreeView_GetParent(hTree, itemDrop);
   if (tvis.hParent){
      newParentNode = GetBookmarkNode(hTree, tvis.hParent);
   }
   else{
      newParentNode = &workingBookmarks;
   }

   // This conditional only works because we check newPreviousNode's existence before checking its nexts, which will crash if the node is NULL)
   // And maybe this could be somewhat simpler?  But it works well, so hey.
   if ((newPreviousNode && moveNode != newPreviousNode && moveNode != newPreviousNode->next) || (!newPreviousNode && moveNode != newParentNode && moveNode != newParentNode->child)) {

      // remove node from the bookmark structure
      if (oldParentNode->child == moveNode) {
         oldParentNode->child = moveNode->next;
         if (!moveNode->next)
            oldParentNode->lastChild = NULL;
      }
      else {
         // find the old previous node (only necessary to find it if it exists)
         for (oldPreviousNode = oldParentNode->child ; oldPreviousNode->next != moveNode ; oldPreviousNode = oldPreviousNode->next);
         oldPreviousNode->next = moveNode->next;
         if (!moveNode->next)
            oldParentNode->lastChild = oldPreviousNode;
      }

      // insert node in its new location in the bookmark structure
      if (newPreviousNode) {
         moveNode->next = newPreviousNode->next;
         newPreviousNode->next = moveNode;
      }
      else {
         moveNode->next = newParentNode->child;
         newParentNode->child = moveNode;
      }

      // collapse its parent if it's going to be empty (doesn't work if we collapse after deletion, for some reason)
      if (!TreeView_GetNextSibling(hTree, TreeView_GetChild(hTree, oldParentItem))) {
         TreeView_Expand(hTree, oldParentItem, TVE_COLLAPSE);
      }

      // create a new copy of the treeview item
      HTREEITEM newParent = TreeView_InsertItem(hTree, &tvis);

      // copy its children
      CopyBranch(hTree, item, newParent);

      // select it
      TreeView_SelectItem(hTree, newParent);

      // delete it 
      TreeView_DeleteItem(hTree, item);

      bookmarksEdited = true;
   }
}

static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type) {
   CBookmarkNode *newNode;

   TVINSERTSTRUCT tvis;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

   switch (type) {
   case BOOKMARK_BOOKMARK:
      newNode = new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), "New Bookmark", "http://kmeleon.sourceforge.net/", BOOKMARK_BOOKMARK, time(NULL));
      tvis.itemex.pszText = "New Bookmark";
      tvis.itemex.iImage = IMAGE_BOOKMARK;
      tvis.itemex.iSelectedImage = IMAGE_BOOKMARK;
      break;
   case BOOKMARK_FOLDER:
      newNode = new CBookmarkNode(0, "New Folder", "", BOOKMARK_FOLDER, time(NULL));
      tvis.itemex.pszText = "New Folder";
      tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
      tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
      break;
   case BOOKMARK_SEPARATOR:
      newNode = new CBookmarkNode(0, "", "", BOOKMARK_SEPARATOR);
      tvis.itemex.pszText = "---";
      tvis.itemex.iImage = IMAGE_BLANK;
      tvis.itemex.iSelectedImage = IMAGE_BLANK;
      break;
   }

   tvis.itemex.lParam = (long)newNode;

   TVITEMEX itemData;
   itemData.mask = TVIF_PARAM;
   itemData.hItem = fromItem;
   TreeView_GetItem(hTree, &itemData);
   CBookmarkNode *fromNode = (CBookmarkNode *)itemData.lParam;

   if (fromNode->type == BOOKMARK_FOLDER) {
      // if we're adding to a folder, make it the first child
      newNode->next = fromNode->child;
      fromNode->child = newNode;

      tvis.hParent = fromItem;
      tvis.hInsertAfter = TVI_FIRST;
   }
   else {
      // otherwise just put it directly after
      newNode->next = fromNode->next;
      fromNode->next = newNode;

      tvis.hParent = TreeView_GetParent(hTree, fromItem);
      tvis.hInsertAfter = fromItem;
   }

   HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);

   TreeView_SelectItem(hTree, newItem);

   bookmarksEdited = true;
}

static void ChangeSpecialFolder(HWND hTree, HTREEITEM *htiOld, HTREEITEM htiNew, int flag) {
   TVITEMEX itemData;

   CBookmarkNode *node = GetBookmarkNode(hTree, htiNew);

   if (node->type != BOOKMARK_FOLDER){
      htiNew = TreeView_GetParent(hTree, htiNew);
      if (!htiNew){
         // something screwy is going on...  just bail out
         return;
      }
   }

   if (htiNew == *htiOld) return;

   // old item - might not exist
   if (*htiOld) {
      itemData.mask = TVIF_PARAM;
      itemData.hItem = *htiOld;
      TreeView_GetItem(hTree, &itemData);

      node = (CBookmarkNode *)itemData.lParam;

      node->flags &= ~flag;

      if (!node->flags) {
         // if not special anymore, set back to normal folder icons
         itemData.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
         itemData.iImage = IMAGE_FOLDER_CLOSED;
         itemData.iSelectedImage = IMAGE_FOLDER_OPEN;
         TreeView_SetItem(hTree, &itemData);
      }
   }

   // new item
   itemData.mask = TVIF_PARAM;
   itemData.hItem = htiNew;
   TreeView_GetItem(hTree, &itemData);

   node = (CBookmarkNode *)itemData.lParam;
   node->flags |= flag;

   itemData.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   itemData.iImage = IMAGE_FOLDER_SPECIAL_CLOSED;
   itemData.iSelectedImage = IMAGE_FOLDER_SPECIAL_OPEN;
   TreeView_SetItem(hTree, &itemData);

   // set our stored HTREEITEM
   *htiOld = htiNew;
}

static void DeleteItem(HWND hTree, HTREEITEM item) {
   CBookmarkNode *node, *parentNode;

   node = GetBookmarkNode(hTree, item);

   HTREEITEM parent = TreeView_GetParent(hTree, item);
   if (parent){
      parentNode = GetBookmarkNode(hTree, parent);
   }
   else{
      // no deleting the root node
      return;
   }

   parentNode->DeleteNode(node);

   // select a new item
   HTREEITEM hSelect = TreeView_GetNextSibling(hTree, item);
   if (!hSelect) hSelect = TreeView_GetPrevSibling(hTree, item);
   if (!hSelect) hSelect = TreeView_GetParent(hTree, item);

   TreeView_DeleteItem(hTree, item);

   // must call selectitem after deleteitem, otherwise the SELCHANGED handler will try to update the deleted node!
   TreeView_SelectItem(hTree, hSelect);

   bookmarksEdited = true;
}

// This just updates a node with the current Title and URL edit contents.
// Maybe it needn't update the tree if we're just exiting, but it's not too big a deal.
static void UpdateTitle(HWND hDlg, HTREEITEM item) {
   HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
   CBookmarkNode *node = GetBookmarkNode(hTree, item);

   if (node->type == BOOKMARK_BOOKMARK || node->type == BOOKMARK_FOLDER) {
      // FIXME - surely there's a better way to do this than allocating 1K for each string?
      // Isn't this a memory leak?  (Overwriting old buffer location, but old buffer remains allocated?)
      char TitleBuffer[1024];
      GetDlgItemText(hDlg, IDC_TITLE, TitleBuffer, 1023);
      if (node->text.compare(TitleBuffer) != 0) {
         node->text = TitleBuffer;
         bookmarksEdited = true;

         // and update the title in the treeview
         TVITEMEX itemData;
         itemData.hItem = item;
         itemData.mask = TVIF_TEXT;
         itemData.pszText = TitleBuffer;
         TreeView_SetItem(hTree, &itemData);
      }
   }
}

// Ditto, but for the URL
static void UpdateURL(HWND hDlg, HTREEITEM item) {
   HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
   CBookmarkNode *node = GetBookmarkNode(hTree, item);

   if (node->type == BOOKMARK_BOOKMARK) {
      char URLBuffer[1024];
      GetDlgItemText(hDlg, IDC_URL, URLBuffer, 1023);
      if (node->url.compare(URLBuffer) != 0) {
         node->url = URLBuffer;
         bookmarksEdited = true;
      }
   }
}

static void OnRClick(HWND hTree)
{
   POINT mouse;
   GetCursorPos(&mouse);

   HMENU topMenu = LoadMenu(kPlugin.hDllInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));
   HMENU contextMenu = GetSubMenu(topMenu, 0);
   int command = TrackPopupMenu(contextMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD, mouse.x, mouse.y, 0, hTree, NULL);

   TVHITTESTINFO hti;
   hti.pt.x = mouse.x;
   hti.pt.y = mouse.y;
   ScreenToClient(hTree, &hti.pt);

   HTREEITEM item = TreeView_HitTest(hTree, &hti);
   if (item) {
      switch (command) {
      case ID__NEW_FOLDER:
         CreateNewObject(hTree, item, BOOKMARK_FOLDER);
         break;
      case ID__NEW_SEPARATOR:
         CreateNewObject(hTree, item, BOOKMARK_SEPARATOR);
         break;
      case ID__NEW_BOOKMARK:
         CreateNewObject(hTree, item, BOOKMARK_BOOKMARK);
         break;
      case ID__SETAS_TOOLBARFOLDER:
         ChangeSpecialFolder(hTree, &hTBitem, item, BOOKMARK_FLAG_TB);
         bookmarksEdited = true;    // ChangeSpecialFolder is used when filling the tree initially, as well, so we don't want it to set bookmarksModified itself
         break;
      case ID__SETAS_NEWBOOKMARKFOLDER:
         ChangeSpecialFolder(hTree, &hNBitem, item, BOOKMARK_FLAG_NB);
         bookmarksEdited = true;
         break;
      case ID__SETAS_BOOKMARKMENU:
         ChangeSpecialFolder(hTree, &hBMitem, item, BOOKMARK_FLAG_BM);
         bookmarksEdited = true;
         break;
      case ID_BOOKMARK_DELETE:
         DeleteItem(hTree, item);
         break;
      }
   }
}

#define BORDER 8
#define BUTTON_HEIGHT 24
#define OK_WIDTH 68
#define CANCEL_WIDTH 68

static void OnSize(int height, int width) {
   static int oldheight = -1, oldwidth = -1;

   // resize tree
   SetWindowPos(GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK), 0, 0, 0, width-(BORDER*2), height-(BORDER*3 + BUTTON_HEIGHT), SWP_NOMOVE);

   // move import favorites button
   SetWindowPos(GetDlgItem(hEditWnd, ID_IMPORT_FAVORITES), 0, BORDER, height-(BORDER+BUTTON_HEIGHT), 0, 0, SWP_NOSIZE);

   // move ok button
   SetWindowPos(GetDlgItem(hEditWnd, IDOK), 0, width-(BORDER*2+CANCEL_WIDTH+OK_WIDTH), height-(BORDER+BUTTON_HEIGHT), 0, 0, SWP_NOSIZE);

   // move cancel button
   SetWindowPos(GetDlgItem(hEditWnd, IDCANCEL), 0, width-(BORDER+CANCEL_WIDTH), height-(BORDER+BUTTON_HEIGHT), 0, 0, SWP_NOSIZE);

   oldheight = height;
   oldwidth = width;
}


// Import Favorites
// Most code grabbed from ie_favorites.cpp - if that changes, this should too, probably

#define REG_USER_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define REG_SHELL_FOLDERS _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define REG_FAVORITES_KEY _T("Favorites")

static void ImportFavorites(HWND hTree) {

   TCHAR FavoritesPath[MAX_PATH];

/* From Init() */
   TCHAR           sz[MAX_PATH];
   HKEY            hKey;
   DWORD           dwSize;
   ITEMIDLIST *idl;

   long rslt = ERROR_SUCCESS;

   // first try the correct way
   if (SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &idl) == NOERROR) {
      IMalloc *malloc;
      SHGetPathFromIDList(idl, sz);
      SHGetMalloc(&malloc);
      malloc->Free(idl);
      malloc->Release();
   }  
   else {
      // if the correct way failed, find out from the registry where the favorites are located.
      if(RegOpenKey(HKEY_CURRENT_USER, REG_USER_SHELL_FOLDERS, &hKey) == ERROR_SUCCESS) {
         dwSize = MAX_PATH;
         rslt = RegQueryValueEx(hKey, REG_FAVORITES_KEY, NULL, NULL, (LPBYTE)sz, &dwSize);
         RegCloseKey(hKey);

         if (rslt != ERROR_SUCCESS) {
            if (RegOpenKey(HKEY_CURRENT_USER, REG_SHELL_FOLDERS, &hKey) == ERROR_SUCCESS) {
               rslt = RegQueryValueEx(hKey, REG_FAVORITES_KEY, NULL, NULL, (LPBYTE)sz, &dwSize);
               RegCloseKey(hKey);
            }
            else {
// FIXME - error messagebox here
//               TRACE0("Favorites folder not found\n");
//               rslt = -1;
            }
         }
      }
      else {
// FIXME - error messagebox here
//         TRACE0("Favorites folder not found\n");
//         rslt = -1;
      }
   }
   if (rslt == ERROR_SUCCESS) {
      ExpandEnvironmentStrings(sz, FavoritesPath, MAX_PATH);

      strcat(FavoritesPath, "\\");
   }
   else {
      FavoritesPath[0] = 0;
   }


/* from DoMenu() */
   // there are no favorites
   if (!*FavoritesPath) {
      // FIXME - error messagebox here
      return;
   }


/* new */

   // make new node for favorites (child off of WorkingBookmarks)
   CBookmarkNode *newFavoritesNode = new CBookmarkNode(0, "Imported Favorites", "", BOOKMARK_FOLDER, time(NULL));
   workingBookmarks.AddChild(newFavoritesNode);

   BuildFavoritesTree(FavoritesPath, "", newFavoritesNode);

   bookmarksEdited = true;

   // Add the new folder to the TreeView
   TVINSERTSTRUCT tvis;
   tvis.hParent = TreeView_GetRoot(hTree);
   tvis.hInsertAfter = NULL;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
   tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
   tvis.itemex.pszText = "Imported Favorites";
   tvis.itemex.lParam = (long)newFavoritesNode;

   HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);

   FillTree(hTree, newItem, *newFavoritesNode);

   TreeView_SelectItem(hTree, newItem);
}

static void BuildFavoritesTree(TCHAR *FavoritesPath, char* strPath, CBookmarkNode *newFavoritesNode) {

   int pathLen = strlen(strPath);
   int FavoritesPathLen = strlen(FavoritesPath);

   char * searchString = new char[FavoritesPathLen + pathLen + 2];
   strcpy(searchString, FavoritesPath);
   strcat(searchString, strPath);
   strcat(searchString, "*");

   char * urlFile;
   char * subPath;

   WIN32_FIND_DATA wfd;
   HANDLE h = FindFirstFile(searchString, &wfd);

   delete [] searchString;

   if(h == INVALID_HANDLE_VALUE) {
      return;
   }

   do {
      if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         // ignore the current and parent directory entries
         if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
            continue;

         subPath = new char[pathLen + strlen(wfd.cFileName) + 2];
         strcpy(subPath, strPath);
         strcat(subPath, wfd.cFileName);         
         strcat(subPath, "/");

         // make new node for favorites (child off of our current node)
         CBookmarkNode *newFavoritesChildNode = new CBookmarkNode(0, wfd.cFileName, "", BOOKMARK_FOLDER, time(NULL));
         newFavoritesNode->AddChild(newFavoritesChildNode);

         // build the tree for this directory
         BuildFavoritesTree(FavoritesPath, subPath, newFavoritesChildNode);

         delete [] subPath;

      }else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file

         char *dot = strrchr(wfd.cFileName, '.');
         if(dot && stricmp(dot, ".url") == 0) {

            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new char[pathLen + filenameLen + 1];
            strcpy(urlFile, strPath);
            strcat(urlFile, wfd.cFileName);

            // format for display in the menu
            // chop off the .url
            *dot = 0;
            // condense the string and escape ampersands
            char *pszTemp = fixString(wfd.cFileName, 40);

            FavoritesPath[FavoritesPathLen] = 0;

            char path[MAX_PATH];
            strcpy(path, FavoritesPath);
            strcpy(path+FavoritesPathLen, urlFile);

            // a .URL file is formatted just like an .INI file, so we can
            // use GetPrivateProfileString() to get the information we want
            char url[INTERNET_MAX_URL_LENGTH];
            GetPrivateProfileString(_T("InternetShortcut"), _T("URL"), _T(""), url, INTERNET_MAX_URL_LENGTH, path);

            // insert node
            newFavoritesNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), wfd.cFileName, url, BOOKMARK_BOOKMARK, time(NULL)));

            delete pszTemp;
            delete [] urlFile;
         }
      }
   } while(FindNextFile(h, &wfd));
   FindClose(h);
}