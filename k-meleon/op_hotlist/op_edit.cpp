/*
 * Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
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
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "op_hotlist.h"
#include "../kmeleon_plugin.h"
#include "..\\rebar_menu\\hot_tracking.h"
#include "../KMeleonConst.h"

#include "../Utils.h"

extern kmeleonPlugin kPlugin;
extern void * KMeleonWndProc;

static CBookmarkNode workingBookmarks;
static CBookmarkNode *freeNode, *freeParentNode;
static BOOL bookmarksEdited;
static HWND hEditWnd;

#define CUT 1
#define KILL 1
#define PASTE 1

static void FillTree(HWND hTree, HTREEITEM parent, CBookmarkNode &node);
static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type, int mode=0);
static void DeleteItem(HWND hTree, HTREEITEM item, int mode=0);
static void CopyItem(HWND hTree, HTREEITEM item);
static void OnSize(int height, int width);


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

   ll = Int32x32To64(t, 10000000) + 116444736000000000LL;
   ft1.dwLowDateTime = (DWORD)ll;
   ft1.dwHighDateTime = ll >> 32;

   FileTimeToLocalFileTime(&ft1, &ft2);
   FileTimeToSystemTime(&ft2, pst);
}


// Hook function for keyboard hook
// I have to use the hook to capture SHIFT-UP, SHIFT-DOWN, ENTER and other "system"-type keypresses
// And to keep things consistent, I just threw all keypress handling in here
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
   if ( (nCode != HC_ACTION) || (lParam & 0x80000000) ) {   // highest bit of lParam high == key being released
      return CallNextHookEx(NULL, nCode, wParam, lParam);
   }

   BOOL fEatKeystroke = false;
   HWND hasFocus = GetFocus();
   HWND hTree = GetDlgItem(hEditWnd, IDC_TREE_HOTLIST);

   if (hasFocus == hTree) {
      HTREEITEM hItem = TreeView_GetSelection(hTree);
      if (hItem) {
         switch (wParam) {
         case VK_UP:
            if (GetKeyState(VK_SHIFT) & 0x80) {
               // fEatKeystroke = true;
               // MoveItem(hTree, hItem, 1);
            }
            else if (GetKeyState(VK_MENU) & 0x80) {   // what genius decided to call ALT "VK_MENU"???
               fEatKeystroke = true;
               HTREEITEM hItemSelect = TreeView_GetPrevSibling(hTree, hItem);
               if (!hItemSelect) hItemSelect = TreeView_GetPrevVisible(hTree, hItem);
               if (!hItemSelect) break;
               TreeView_SelectItem(hTree, hItemSelect);
            }
            break;
         case VK_DOWN:
            if (GetKeyState(VK_SHIFT) & 0x80) {
               // fEatKeystroke = true;
               // MoveItem(hTree, hItem, 2);
            }
            else if (GetKeyState(VK_MENU) & 0x80) {
               fEatKeystroke = true;
               HTREEITEM hItemSelect = TreeView_GetNextSibling(hTree, hItem);
               if (!hItemSelect) hItemSelect = TreeView_GetNextSibling(hTree, TreeView_GetParent(hTree, hItem));
               if (!hItemSelect) hItemSelect = TreeView_GetNextVisible(hTree, hItem);
               if (!hItemSelect) break;
               TreeView_SelectItem(hTree, hItemSelect);
            }
            break;
         case VK_RETURN:
            {
               fEatKeystroke = true;
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
               if (node->type == BOOKMARK_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  bookmarksEdited = true;
                  if (GetKeyState(VK_CONTROL) & 0x80) {
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUND);
                  }
                  else {
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL);
                  }
                  TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
               }
               else if (node->type == BOOKMARK_FOLDER) {
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
               }
            }
            break;
         case VK_DELETE:
            fEatKeystroke = true;
            if (GetKeyState(VK_SHIFT) & 0x80) {
               // Shift delete == Cut
               DeleteItem(hTree, hItem, CUT);
            } else if (GetKeyState(VK_CONTROL) & 0x80) {
               // Ctrl delete == Kill
               DeleteItem(hTree, hItem, KILL);
            } else {
               // Delete == Kill
               DeleteItem(hTree, hItem);
            }
            break;
         case VK_INSERT:
            fEatKeystroke = true;
            if (GetKeyState(VK_SHIFT) & 0x80) {
               // Shift insert == Paste
               if (freeNode)
                  CreateNewObject(hTree, hItem, freeNode->type, PASTE);
            } else if (GetKeyState(VK_CONTROL) & 0x80) {
               // Ctrl insert == Copy
               CopyItem(hTree, hItem);
            } else {
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
               if (node)
                  CreateNewObject(hTree, hItem, node->type);
               else
                  CreateNewObject(hTree, hItem, BOOKMARK_FOLDER);
            }
            break;
         case 'X':
            if (GetKeyState(VK_CONTROL) & 0x80) {
               // Ctrl X == Cut
               DeleteItem(hTree, hItem, CUT);
            }
            break;
         case 'C':
            if (GetKeyState(VK_CONTROL) & 0x80) {
               // Ctrl C == Copy
               CopyItem(hTree, hItem);
            }
            break;
         case 'V':
            if (GetKeyState(VK_CONTROL) & 0x80) {
               // Ctrl V == Paste
               if (freeNode)
                  CreateNewObject(hTree, hItem, freeNode->type, PASTE);
            }
            break;
         case ' ':
            {
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
               if (node->type == BOOKMARK_FOLDER) {
                  fEatKeystroke = true;
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
               }
            }
            break;
         }
      }
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDC_NAME) && wParam == VK_RETURN) {
      fEatKeystroke = true;
      SetFocus(GetDlgItem(hEditWnd, IDC_URL));
      SendDlgItemMessage(hEditWnd, IDC_URL, EM_SETSEL, 0, -1);  // select all
   }
   else if ((hasFocus == GetDlgItem(hEditWnd, IDC_URL) ||
             hasFocus == GetDlgItem(hEditWnd, IDC_ORDER) ||
             hasFocus == GetDlgItem(hEditWnd, IDC_DESCRIPTION) ||
             hasFocus == GetDlgItem(hEditWnd, IDC_SHORT_NAME)) && 
            wParam == VK_RETURN) {
      fEatKeystroke = true;
      SetFocus(GetDlgItem(hEditWnd, IDC_TREE_HOTLIST));
   }

   return(fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam));
}


int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HHOOK hHook;
   static HWND hTree;

   switch (uMsg){
   case WM_INITDIALOG:
      {
         if (gHotlistModified)
            op_writeFile(lpszHotlistFile);

         hEditWnd = hDlg;

         hTree = GetDlgItem(hDlg, IDC_TREE_HOTLIST);
         TreeView_SetImageList(hTree, gImagelist, TVSIL_NORMAL);

         workingBookmarks = gHotlistRoot;
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

         FillTree(hTree, newItem, workingBookmarks);

         TreeView_Expand(hTree, newItem, TVE_EXPAND);

         int dialogleft = 50, dialogtop = 50, dialogwidth = 500, dialogheight = 500;
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_LEFT, &dialogleft, &dialogleft);  
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_TOP, &dialogtop, &dialogtop);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_WIDTH, &dialogwidth, &dialogwidth);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_HEIGHT, &dialogheight, &dialogheight);
         SetWindowPos(hDlg, 0, dialogleft, dialogtop, dialogwidth, dialogheight, 0);

         hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, GetCurrentThreadId());
      }
      return false;

   case WM_NOTIFY:
      {
         NMTREEVIEW *nmtv = (NMTREEVIEW *)lParam;

         // Selection changed
         if (nmtv->hdr.code == TVN_SELCHANGED){
            // Put the new url/title into the box
            CBookmarkNode *newNode = (CBookmarkNode *)nmtv->itemNew.lParam;

            if (newNode == &workingBookmarks) {
               // root and separators have nothing
               SetDlgItemText(hDlg, IDC_NAME, "");
               SetDlgItemText(hDlg, IDC_URL, "");
               SetDlgItemText(hDlg, IDC_CREATED, "");
               SetDlgItemText(hDlg, IDC_LAST_VISIT, "");
               SetDlgItemText(hDlg, IDC_ORDER, "");
               SetDlgItemText(hDlg, IDC_DESCRIPTION, "");
               SetDlgItemText(hDlg, IDC_SHORT_NAME, "");
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), false);
               EnableWindow(GetDlgItem(hDlg, IDC_NAME), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CREATED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ORDER), false);
               EnableWindow(GetDlgItem(hDlg, IDC_ORDER), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DESC), false);
               EnableWindow(GetDlgItem(hDlg, IDC_DESCRIPTION), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SHORT), false);
               EnableWindow(GetDlgItem(hDlg, IDC_SHORT_NAME), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), false);
               return true;   // a lazy-man's "else"
            }

            // everything else has at least title, added date, and properties in general
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), true);

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), true);
            EnableWindow(GetDlgItem(hDlg, IDC_NAME), true);
            SetDlgItemText(hDlg, IDC_NAME, newNode->text.c_str());

            SYSTEMTIME st;
            char pszTmp[1024];
            char pszDate[900];

            if (newNode->type == BOOKMARK_BOOKMARK) {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), true);
               SetDlgItemText(hDlg, IDC_URL, newNode->url.c_str());
            }
            else {
               SetDlgItemText(hDlg, IDC_URL, "");
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
            }
            
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CREATED), true);
            UnixTimeToSystemTime(newNode->addDate, &st);
            GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
            strcpy(pszTmp, pszDate);
            strcat(pszTmp, " ");
            GetTimeFormat(GetThreadLocale(), NULL, &st, NULL, pszDate, 899);
            strcat(pszTmp, pszDate);
            SetDlgItemText(hDlg, IDC_CREATED, pszTmp);

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), true);
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

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ORDER), true);
            EnableWindow(GetDlgItem(hDlg, IDC_ORDER), true);
            SetDlgItemInt(hDlg, IDC_ORDER, (UINT)newNode->order, false);
            
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DESC), true);
            EnableWindow(GetDlgItem(hDlg, IDC_DESCRIPTION), true);
            SetDlgItemText(hDlg, IDC_DESCRIPTION, newNode->desc.c_str());

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SHORT), true);
            EnableWindow(GetDlgItem(hDlg, IDC_SHORT_NAME), true);
            SetDlgItemText(hDlg, IDC_SHORT_NAME, newNode->nick.c_str());

         }
      }
      break;
      
   case WM_SIZE:
      {
         if(wParam != SIZE_MINIMIZED) {
            RECT rect;            
            GetClientRect(hDlg, &rect);            
            OnSize(rect.bottom, rect.right);
         }
      }
      break;

   case WM_GETMINMAXINFO:
      LPMINMAXINFO lpminmaxinfo;
      lpminmaxinfo=(LPMINMAXINFO)lParam;
      lpminmaxinfo->ptMinTrackSize.x = 195;
      lpminmaxinfo->ptMinTrackSize.y = 300;
      break;

   case WM_COMMAND:
      {
         if (HIWORD(wParam) == EN_CHANGE) {
            int id = LOWORD(wParam);

            if (id == IDC_NAME || id == IDC_URL ||
                id == IDC_ORDER || id == IDC_DESCRIPTION || id == IDC_SHORT_NAME) {
               if (SendDlgItemMessage(hDlg, id, EM_GETMODIFY, 0, 0)) {
                  SendDlgItemMessage(hDlg, id, EM_SETMODIFY, FALSE, 0);
                  HTREEITEM hSelection = TreeView_GetSelection(hTree);
                  if (hSelection) {
                     CBookmarkNode *node = GetBookmarkNode(hTree, hSelection);
                     
                     if (node->type == BOOKMARK_BOOKMARK || node->type == BOOKMARK_FOLDER) {
                        if (id == IDC_ORDER && node->order != GetDlgItemInt(hDlg, id, NULL, false)) {
                           node->order = GetDlgItemInt(hDlg, id, NULL, true);
                           bookmarksEdited = true;
                        }
                        else {
                           char buffer[1024];
                           GetDlgItemText(hDlg, id, buffer, 1023);
                           if (id == IDC_NAME && node->text.compare(buffer) != 0) {
                              node->text = buffer;
                              bookmarksEdited = true;
                              
                              // and update the title in the treeview
                              TVITEMEX itemData;
                              itemData.hItem = hSelection;
                              itemData.mask = TVIF_TEXT;
                              itemData.pszText = buffer;
                              TreeView_SetItem(hTree, &itemData);
                           }
                           else if (id == IDC_URL && node->url.compare(buffer) != 0) {
                              node->url = buffer;
                              bookmarksEdited = true;
                           }
                           else if (id == IDC_DESCRIPTION && node->desc.compare(buffer) != 0) {
                              node->desc = buffer;
                              bookmarksEdited = true;
                           }
                           else if (id == IDC_SHORT_NAME && node->nick.compare(buffer) != 0) {
                              node->nick = buffer;
                              bookmarksEdited = true;
                           }
                        }
                     }
                  }
               }
            }
         }
         else if (HIWORD(wParam) == BN_CLICKED) {
            WORD id = LOWORD(wParam);
            switch(id){

            case IDOK:
               if (bookmarksEdited) {
                  gHotlistRoot = workingBookmarks;
                  op_writeFile(lpszHotlistFile);
                  
                  RebuildMenu();
               }
               
               RECT DlgPos;
               GetWindowRect(hDlg, &DlgPos);
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_LEFT, &DlgPos.left);
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_TOP, &DlgPos.top);
               int temp;
               temp = DlgPos.right - DlgPos.left;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_WIDTH, &temp);
               temp = DlgPos.bottom - DlgPos.top;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_HEIGHT, &temp);

               UnhookWindowsHookEx(hHook);
               EndDialog(hDlg, 1);
               break;
            
            case IDCANCEL:
               if (bookmarksEdited) {
                  delete gHotlistRoot.child;
                  delete gHotlistRoot.next;
                  gHotlistRoot.child = NULL;
                  gHotlistRoot.next = NULL;
                  if (op_readFile(lpszHotlistFile) > 0) {
                     RebuildMenu();
                  }
               }

               UnhookWindowsHookEx(hHook);
               EndDialog(hDlg, 0);
               break;
            }
         }
      }
      break;
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
         
         /*
         if (child->flags & BOOKMARK_FLAG_TB) {
            ChangeSpecialFolder(hTree, &hTBitem, thisItem, BOOKMARK_FLAG_TB);
         }
         if (child->flags & BOOKMARK_FLAG_BM) {
            ChangeSpecialFolder(hTree, &hBMitem, thisItem, BOOKMARK_FLAG_BM);
         }
         if (child->flags & BOOKMARK_FLAG_NB) {
            ChangeSpecialFolder(hTree, &hNBitem, thisItem, BOOKMARK_FLAG_NB);
         }
         */

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


static void CopyItem(HWND hTree, HTREEITEM item) {
   CBookmarkNode *node = GetBookmarkNode(hTree, item);
   HTREEITEM parent = TreeView_GetParent(hTree, item);
   if (parent) {
      if (freeNode && freeParentNode)
         freeParentNode->DeleteNode(freeNode);
      
      freeNode = new CBookmarkNode();
      *freeNode = *node;
      freeParentNode = GetBookmarkNode(hTree, parent);
   }
}

static void DeleteItem(HWND hTree, HTREEITEM item, int mode=0) {
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

   if (mode == CUT) {
      parentNode->UnlinkNode(node);
      freeNode = node;
      freeParentNode = parentNode;
   } else
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

static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type, int mode=0) {
   CBookmarkNode *newNode;

   TVINSERTSTRUCT tvis;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

   switch (type) {
   case BOOKMARK_BOOKMARK:
      if (mode == PASTE && freeNode) {
         newNode = freeNode;
         freeNode = NULL;
      } else
         newNode = new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), "New Bookmark", "http://kmeleon.sourceforge.net/", "", "", BOOKMARK_BOOKMARK, time(NULL));
      tvis.itemex.pszText = (TCHAR*)newNode->text.c_str();
      tvis.itemex.iImage = IMAGE_BOOKMARK;
      tvis.itemex.iSelectedImage = IMAGE_BOOKMARK;
      break;
   case BOOKMARK_FOLDER:
      if (mode == PASTE && freeNode) {
         newNode = freeNode;
         freeNode = NULL;
      } else
         newNode = new CBookmarkNode(0, "New Folder", "", "", "", BOOKMARK_FOLDER, time(NULL));
      tvis.itemex.pszText = (TCHAR*)newNode->text.c_str();
      tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
      tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
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
   if (type == BOOKMARK_FOLDER && newNode->child)
      FillTree(hTree, newItem, *newNode);

   TreeView_SelectItem(hTree, newItem);

   bookmarksEdited = true;
}


// this is ugly, see...  all this need to be changed each time the dialog is re-arranged...
#define BORDER 8  // this one is pixels, the rest are DLUs
#define BUTTON_HEIGHT 15
#define OK_WIDTH 45
#define CANCEL_WIDTH 45
#define PROPERTIES_HEIGHT 88
#define EDITBOXES_LEFT 36
#define EDITBOXES_TOP (276-174)
#define EDITBOXES_HEIGHT 12
#define DATES_WIDTH 78
#define OTHER_WIDTH 35

#define convX(x) MulDiv((int)(x), buX, 4)
#define convY(y) MulDiv((int)(y), buY, 8)

static void OnSize(int height, int width) {
   int buX, buY;
   RECT rc;    // GetDialogBaseUnits returns incorrect values...?
   SetRect(&rc, 0, 0, 4, 8);
   MapDialogRect(hEditWnd, &rc);
   buY = rc.bottom;
   buX = rc.right;


   // resize tree
   SetWindowPos(GetDlgItem(hEditWnd, IDC_TREE_HOTLIST), 0, 
                BORDER, 
                BORDER, 
                width-(BORDER*2), 
                height-BORDER*4-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                0);

   // move cancel button
   SetWindowPos(GetDlgItem(hEditWnd, IDCANCEL), 0, 
                width-BORDER-convX(CANCEL_WIDTH), 
                height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);

   // move ok button
   SetWindowPos(GetDlgItem(hEditWnd, IDOK), 0, 
                width-BORDER*2-convX(CANCEL_WIDTH+OK_WIDTH), 
                height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);

   // move/resize properties box
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_PROPERTIES), 0, 
                BORDER, 
                height-BORDER*2-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                width-BORDER*2, 
                convY(PROPERTIES_HEIGHT), 
                0);

   // move/resize properties widgets
   int x_max = convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER;
   int x_half = width/2 + BORDER/2;
   int x2 = min(x_max, x_half);
   int w1 = x2 - convX(EDITBOXES_LEFT) - BORDER;
   int w2 = width - x2 - 2*BORDER;

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_CREATED), 0, 
                BORDER*2, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 
                0, 
                0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_CREATED), 0, 
                convX(EDITBOXES_LEFT)+5, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 
                w1, // convX(DATES_WIDTH), 
                convY(EDITBOXES_HEIGHT), 
                0);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_VISITED), 0, 
                x2, // convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_LAST_VISIT), 0, 
                x2 + OTHER_WIDTH+5, // convX(EDITBOXES_LEFT+DATES_WIDTH+OTHER_WIDTH)+BORDER+5, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 
                w1 - OTHER_WIDTH, // width-convX(EDITBOXES_LEFT+DATES_WIDTH+OTHER_WIDTH)-BORDER*3, 
                convY(EDITBOXES_HEIGHT), 0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_SHORT), 0, 
                2*BORDER, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*5.0), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_SHORT_NAME), 0, 
                convX(EDITBOXES_LEFT), 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*5.0)-2, 
                convX(DATES_WIDTH),
                convY(EDITBOXES_HEIGHT), 
                0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_DESC), 0, 
                x2, // convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_DESCRIPTION), 0, 
                x2 + OTHER_WIDTH, // convX(EDITBOXES_LEFT+DATES_WIDTH+OTHER_WIDTH)+BORDER, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75)-2, 
                w2 - OTHER_WIDTH, // width-convX(EDITBOXES_LEFT+DATES_WIDTH+OTHER_WIDTH)-BORDER*3, 
                convY(EDITBOXES_HEIGHT), 
                0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_ORDER), 0, 
                2*BORDER, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_ORDER), 0, 
                convX(EDITBOXES_LEFT), 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75)-2, 
                w1, // convX(DATES_WIDTH),
                convY(EDITBOXES_HEIGHT), 
                0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_URL), 0, 
                BORDER*2, 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*1.25), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_URL), 0, 
                convX(EDITBOXES_LEFT), 
                height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*1.25)-2, 
                width-convX(EDITBOXES_LEFT)-BORDER*2, 
                convY(EDITBOXES_HEIGHT), 
                0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_NAME), 0, 
                BORDER*2, 
                height-convY(EDITBOXES_TOP), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_NAME), 0, 
                convX(EDITBOXES_LEFT), 
                height-convY(EDITBOXES_TOP)-2, 
                width-convX(EDITBOXES_LEFT)-BORDER*2, 
                convY(EDITBOXES_HEIGHT), 
                0);

   InvalidateRgn(hEditWnd, NULL, FALSE);
   UpdateWindow(hEditWnd);
}
