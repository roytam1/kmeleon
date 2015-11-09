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

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "stdafx.h"
#include "resource.h"
#include "wininet.h"    // for INTERNET_MAX_URL_LENGTH
#include "windows.h"    // for hook functions
#include "imm.h"

#include "defines.h"

#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"
#include "Utils.h"
#include "LocalesUtils.h"

#include "../rebar_menu/hot_tracking.h"

#include "BookmarkNode.h"

#include "ns_bookmarks_functions.h"

#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_MINUS VK_OEM_MINUS
#define VK_PERIOD VK_OEM_PERIOD

extern kmeleonPlugin kPlugin;
extern void * KMeleonWndProc;

#define CUT 1
#define KILL 1
#define PASTE 1

// Our functions

static void FillTree(HWND hTree, HTREEITEM parent, CBookmarkNode &node, BOOL useSiteicon);
static void OnSize(int height, int width);
static void OnRClick(HWND hTree);
static void ImportFavorites(HWND hTree);
static void BuildFavoritesTree(TCHAR *FavoritesPath, TCHAR* strPath, CBookmarkNode *newFavoritesNode);
static void MoveItem(HWND hTree, HTREEITEM item, int mode);
static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type, int mode=0);
static void ChangeSpecialFolder(HWND hTree, HTREEITEM *htiOld, HTREEITEM htiNew, int flag);
static void DeleteItem(HWND hTree, HTREEITEM item, int mode=0);
static void CopyItem(HWND hTree, HTREEITEM item);
static void UpdateTitle(HWND hDlg, HTREEITEM item);
static void UpdateURL(HWND hDlg, HTREEITEM item);
static void UpdateNick(HWND hDlg, HTREEITEM item);
static void UpdateDesc(HWND hDlg, HTREEITEM item);

// Local vars

static CBookmarkNode *freeNode, *freeParentNode;
static HCURSOR hCursorDrag;
static BOOL bDragging;
static HWND hEditWnd;
static BOOL bTracking;

#define SEARCH_LEN 256
static char str[SEARCH_LEN];
static int len;
static int pos;
static int circling;


static CBookmarkNode* workingBookmarks; // this will hold a copy of the bookmarks for us to fuck with
static BOOL bookmarksEdited;    // separate from gBookmarksModified - only tracks changes within edit dialog

static HTREEITEM hTBitem;   // current toolbar folder treeview item
static HTREEITEM hNBitem;   // current new bookmark folder treeview item
static HTREEITEM hBMitem;   // current bookmark menu folder treeview item
static BOOL zoom;
static BOOL maximized;

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

   ll = Int32x32To64(t, 10000000) + 116444736000000000L;
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
   BOOL fakedKey = false;
   BOOL fIME = false;
   HIMC hIMC = (HIMC) NULL;
   HWND hasFocus = GetFocus();
   HWND hTree = GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK);

   if (!(hasFocus == hTree ||
         hasFocus == GetDlgItem(hEditWnd, IDC_TITLE) ||
         hasFocus == GetDlgItem(hEditWnd, IDC_URL) ||
         hasFocus == GetDlgItem(hEditWnd, IDC_NICK) ||
         hasFocus == GetDlgItem(hEditWnd, IDC_DESC) ||
         hasFocus == GetDlgItem(hEditWnd, IDOK) ||
         hasFocus == GetDlgItem(hEditWnd, IDCANCEL) ||
         hasFocus == GetDlgItem(hEditWnd, ID_IMPORT_FAVORITES) ||
         hasFocus == GetDlgItem(hEditWnd, ID_KEYBINDINGS)))
      return  CallNextHookEx(NULL, nCode, wParam, lParam);

   int searching = 0;

   hIMC = ImmGetContext(hasFocus);
   if (hIMC) { 
     //fIME = ImmGetOpenStatus(hIMC);
	 fIME = (ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0) > 0) ? TRUE : FALSE;
     ImmReleaseContext(hasFocus, hIMC);
   }

   if (wParam == VK_ESCAPE && !bTracking) {
      fEatKeystroke = true;
      if (len == 0)
         SendMessage(GetDlgItem(hEditWnd, IDCANCEL), BM_CLICK, 0, 0);
      else
         len = 0;
   }
   else if (hasFocus == hTree) {
      HTREEITEM hItem = TreeView_GetSelection(hTree);
      if (hItem) {
         switch (wParam) {
         case VK_F2:
            fEatKeystroke = true;
            SetFocus(GetDlgItem(hEditWnd, IDC_TITLE));
            SendDlgItemMessage(hEditWnd, IDC_TITLE, EM_SETSEL, 0, -1);  // select all
            break;
         case VK_PRIOR:
            if (bDragging) {
               fEatKeystroke = true;
               SendDlgItemMessage(hEditWnd, IDC_TREE_BOOKMARK, WM_VSCROLL, SB_PAGEUP, 0);
               SendMessage(hEditWnd, WM_MOUSEMOVE, 0, 0);  // update the insertion mark
            }
            break;
         case VK_UP:
            if (bDragging) {
               fEatKeystroke = true;
               SendDlgItemMessage(hEditWnd, IDC_TREE_BOOKMARK, WM_VSCROLL, SB_LINEUP, 0);
               SendMessage(hEditWnd, WM_MOUSEMOVE, 0, 0);  // update the insertion mark
            }
            else if (GetKeyState(VK_SHIFT) & 0x80) {
               fEatKeystroke = true;
               MoveItem(hTree, hItem, 1);
            }
            else if (GetKeyState(VK_MENU) & 0x80) {   // what genius decided to call ALT "VK_MENU"???
               fEatKeystroke = true;
               HTREEITEM hItemSelect = TreeView_GetPrevSibling(hTree, hItem);
               if (!hItemSelect) hItemSelect = TreeView_GetPrevVisible(hTree, hItem);
               if (!hItemSelect) break;
               TreeView_SelectItem(hTree, hItemSelect);
            }
            break;
         case VK_NEXT:
            if (bDragging) {
               fEatKeystroke = true;
               SendDlgItemMessage(hEditWnd, IDC_TREE_BOOKMARK, WM_VSCROLL, SB_PAGEDOWN, 0);
               SendMessage(hEditWnd, WM_MOUSEMOVE, 0, 0);  // update the insertion mark
            }
            break;
         case VK_DOWN:
            if (bDragging) {
               fEatKeystroke = true;
               SendDlgItemMessage(hEditWnd, IDC_TREE_BOOKMARK, WM_VSCROLL, SB_LINEDOWN, 0);
               SendMessage(hEditWnd, WM_MOUSEMOVE, 0, 0);  // update the insertion mark
            }
            else if (GetKeyState(VK_SHIFT) & 0x80) {
               fEatKeystroke = true;
               MoveItem(hTree, hItem, 2);
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
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUND, NULL);
                     TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
                  }
                  else {
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
                     TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
                     PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
                  }
                  TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
               }
               else if (node->type == BOOKMARK_FOLDER) {
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
               }
            }
            break;
         case VK_TAB:
            {
               if (len > 0) {
                  wParam = str[len-1];
                  wParam = toupper(wParam);
                  if (wParam == '-') wParam = VK_MINUS;
                  if (wParam == '.') wParam = VK_PERIOD; 
                  fakedKey = true;
                  goto search_again;
               }
               int idc_next = IDC_TITLE;
               fEatKeystroke = true;
               if (zoom)
                  break;
               if (GetKeyState(VK_SHIFT) & 0x80) {
                  idc_next = IDCANCEL;
                  SendMessage(GetDlgItem(hEditWnd, idc_next), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
               }
               else if (!IsWindowEnabled(GetDlgItem(hEditWnd, IDC_TITLE))) {
                  idc_next = ID_KEYBINDINGS;
                  SendMessage(GetDlgItem(hEditWnd, idc_next), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
               }
               SetFocus(GetDlgItem(hEditWnd, idc_next));
               if (idc_next == IDC_TITLE)
                  SendDlgItemMessage(hEditWnd, idc_next, EM_SETSEL, 0, -1);  // select all
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
               CreateNewObject(hTree, hItem, BOOKMARK_BOOKMARK);
               if (!zoom) {
                  SetFocus(GetDlgItem(hEditWnd, IDC_TITLE));
                  SendDlgItemMessage(hEditWnd, IDC_TITLE, EM_SETSEL, 0, -1);
               }
            }
            break;
         case ' ':
            {
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
               if (node->type == BOOKMARK_FOLDER && len == 0) {
                  fEatKeystroke = true;
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
                  break;
               }
            }
            // Fall through!
         default:
            {
               if (GetKeyState(VK_CONTROL) & 0x80) {
                  switch (wParam) {
                  case 'D':
                  case 'X':
                     // Ctrl X == Cut
                     DeleteItem(hTree, hItem, CUT);
                     fEatKeystroke = true;
                     break;
                  case 'C':
                     // Ctrl C == Copy
                     CopyItem(hTree, hItem);
                     fEatKeystroke = true;
                     break;
                  case 'V':
                     // Ctrl V == Paste
                     if (freeNode) {
                        CreateNewObject(hTree, hItem, freeNode->type, PASTE);
                        fEatKeystroke = true;
                     }
                     break;
                  case 'N':
                     CreateNewObject(hTree, hItem, BOOKMARK_BOOKMARK);
                     fEatKeystroke = true;
                     break;
                  case 'U':
                     len = 0;
                     fEatKeystroke = true;
                     break;
                  case 'G':
                     if (len > 0) {
                        wParam = str[len-1];
                        wParam = toupper(wParam);
                        if (wParam == '-') wParam = VK_MINUS;
                        if (wParam == '.') wParam = VK_PERIOD; 
                        fakedKey = true;
                        goto search_again;
                     }
                     break;
                  }
                  break;
               }
               else if (GetKeyState(VK_MENU) & 0x80) {
                  break;
               }
               else {
                  switch (wParam) {
                  case VK_F3:
                     if (len > 0) {
                        wParam = str[len-1];
                        wParam = toupper(wParam);
                        if (wParam == '-') wParam = VK_MINUS;
                        if (wParam == '.') wParam = VK_PERIOD; 
                        fakedKey = true;
                        goto search_again;
                     }
                     break;
                  }
               }

               if (wParam == VK_BACK && len > 0) {
                  circling = 0;
                  len--;
               }
               else if ( (wParam == ' ' || 
                          (wParam >= '0' && wParam <= '9') || 
                          (wParam >= 'A' && wParam <= 'Z')) && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = tolower(wParam);
                  len++;
               }
               else if ( (wParam == VK_MINUS) && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = '-';
                  len++;
               }
               else if ( wParam == VK_PERIOD && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = '.';
                  len++;
               }
               else
                  break;

            search_again:
               fEatKeystroke = true;
               searching = 1;
               str[len] = 0;
               if (len > 0) {
                  CBookmarkNode *node;

                  int mypos = 0;
                  int firstpos = -1;
                  int searchfrom = pos;

                  if (len == 1 && !fakedKey) {
                     HTREEITEM hItem = TreeView_GetSelection(hTree);
                     node = GetBookmarkNode(hTree, hItem);
                     if (workingBookmarks->Index(searchfrom, node) == -1)
                        searchfrom = pos;
                     else
                        pos = searchfrom;
                  }

                  int newpos = workingBookmarks->Search(str, searchfrom, mypos, firstpos, &node);

                  if (fakedKey || newpos == -1 || 
                      (circling && len > 1 && str[len-1] == str[len-2])) {
                     if (fakedKey || (len > 1 && str[len-1] == str[len-2]))
                        searchfrom++;
                     if (!fakedKey)
                        len--;
                     str[len] = 0;
                     mypos = 0;
                     firstpos = -1;
                     if (len > 0) {
                        newpos = workingBookmarks->Search(str, searchfrom, mypos, firstpos, &node);
                        if (!fakedKey && (newpos == pos || (len > 1 && str[len-1] == str[len-2]))) {
                           circling = 1;
                           while (len > 1 && str[len-1] == str[len-2])
                              len--;
                           str[len] = 0;
                           mypos = 0;
                           firstpos = -1;
                           newpos = workingBookmarks->Search(str, searchfrom, mypos, firstpos, &node);
                        }
                     }
                  }
                  else 
                     circling = 0;

                  if (newpos == -1 || len == 0) {
                     pos = 0;
                     len = 0;
                     str[len] = 0;
                     searching = 0;
                  }
                  else {
                     TCHAR tmp[SEARCH_LEN+32];
					 _stprintf(tmp, gLoc->GetString(IDS_FIND), (const TCHAR*)CANSI_to_T(str));
                     SetWindowText( hEditWnd, tmp );

                     HTREEITEM hItem = TreeView_GetRoot(hTree);
                     int i = 0;
                     while (hItem && i < newpos) {
                        HTREEITEM hTmp = TreeView_GetChild(hTree, hItem);
                        if (!hTmp)
                           hTmp = TreeView_GetNextSibling(hTree, hItem);
                        if (!hTmp) {
                           HTREEITEM hTmp2 = TreeView_GetParent(hTree, hItem);
                           while (!hTmp && hTmp2) {
                              hTmp = TreeView_GetNextSibling(hTree, hTmp2);
                              hTmp2 = TreeView_GetParent(hTree, hTmp2);
                           }
                        }
                        hItem = hTmp;
                        i++;
                     }

                     if (hItem) {
                        TreeView_SelectItem(hTree, hItem);
                        TreeView_EnsureVisible(hTree, hItem);
                     }

                     pos = newpos;
                  }
               }
            }
            break;
         }
      }
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDC_TITLE) && wParam == VK_RETURN && ! fIME) {
      fEatKeystroke = true;
      if (IsWindowEnabled(GetDlgItem(hEditWnd, IDC_URL))) {
         SetFocus(GetDlgItem(hEditWnd, IDC_URL));
         SendDlgItemMessage(hEditWnd, IDC_URL, EM_SETSEL, 0, -1);  // select all
      }
      else {
         SetFocus(GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK));
      }
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDOK) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, IDOK), BM_CLICK, 0, 0);
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDCANCEL) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, IDCANCEL), BM_CLICK, 0, 0);
   }
   else if (hasFocus == GetDlgItem(hEditWnd, ID_KEYBINDINGS) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, ID_KEYBINDINGS), BM_CLICK, 0, 0);
   }
   else if (hasFocus == GetDlgItem(hEditWnd, ID_IMPORT_FAVORITES) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, ID_IMPORT_FAVORITES), BM_CLICK, 0, 0);
   }
   else if ((hasFocus == GetDlgItem(hEditWnd, IDC_URL) ||
             hasFocus == GetDlgItem(hEditWnd, IDC_DESC) ||
             hasFocus == GetDlgItem(hEditWnd, IDC_NICK)) && 
             wParam == VK_RETURN && ! fIME) {
      fEatKeystroke = true;
      SetFocus(GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK));
   }
   else if (wParam == VK_TAB) {
      int fields[] = {IDC_TREE_BOOKMARK, IDC_TITLE, IDC_URL, 
                      IDC_NICK, IDC_DESC, 
                      ID_KEYBINDINGS, ID_IMPORT_FAVORITES, 
                      IDOK, IDCANCEL, IDC_TREE_BOOKMARK};
      for (int i=1; fields[i]!=IDC_TREE_BOOKMARK; i++) {
         if (GetDlgItem(hEditWnd, fields[i]) == hasFocus) {
            int idc_next = fields[i + ((GetKeyState(VK_SHIFT) & 0x80) ? -1 : 1)];
            if (idc_next == IDC_TREE_BOOKMARK || 
                idc_next == IDOK || idc_next == IDCANCEL || 
                idc_next == ID_KEYBINDINGS || idc_next == ID_IMPORT_FAVORITES ||
                IsWindowEnabled(GetDlgItem(hEditWnd, idc_next))) {
               fEatKeystroke = true;
               if (fields[i] == IDOK || fields[i] == IDCANCEL ||
                   fields[i] == ID_KEYBINDINGS || fields[i] == ID_IMPORT_FAVORITES)
                  SendMessage(hasFocus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
               if (idc_next == IDOK || idc_next == IDCANCEL ||
                   idc_next == ID_KEYBINDINGS || idc_next == ID_IMPORT_FAVORITES)
                  SendMessage(GetDlgItem(hEditWnd, idc_next), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
               SetFocus(GetDlgItem(hEditWnd, idc_next));
               if (idc_next != IDC_TREE_BOOKMARK && 
                   idc_next != IDOK && idc_next != IDCANCEL && 
                   idc_next != ID_KEYBINDINGS && idc_next != ID_IMPORT_FAVORITES)
                  SendDlgItemMessage(hEditWnd, idc_next, EM_SETSEL, 0, -1);  // select all
               break;
            }
            else {
               if (fields[i] == IDOK || fields[i] == IDCANCEL ||
                   fields[i] == ID_KEYBINDINGS || fields[i] == ID_IMPORT_FAVORITES)
                  SendMessage(hasFocus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
               hasFocus = GetDlgItem(hEditWnd, idc_next);
               i = 0;
               continue;
            }
         }
      }
   }

   if ((searching == 0 || len==0) && 
       wParam != VK_SHIFT && wParam != VK_MENU && wParam != VK_CONTROL) {
      len = 0;
      pos = 0;
      str[len] = 0;
      SetWindowText( hEditWnd, gLoc->GetString(IDS_BOOKMARKS));
      circling = 0;
   }

   return(fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam));
}

int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static HHOOK hHook;
   static bool bTimer = false;  // semi-crude hack to make scrolling smoother
   static HWND hTree;
   static HTREEITEM htCurHover = NULL;
   static HTREEITEM htDummyItem = NULL;

   switch (uMsg){
   case WM_INITDIALOG:
      {
         if (bookmarksEdited) {
            SaveBM(gBookmarkFile);
         }

         hEditWnd = hDlg;
         len = 0;
         str[len] = 0;
         pos = 0;
         circling = 0;

         HICON hIcon;
         TCHAR szFullPath[MAX_PATH];
         kPlugin.kFuncs->FindSkinFile(L"bookmarks-edit.ico", szFullPath, MAX_PATH);

         if (*szFullPath==0 || (hIcon = (HICON)LoadImage( NULL, szFullPath, IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE ))==NULL)
            hIcon = (HICON)LoadImage( kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 0,0, LR_DEFAULTSIZE );
         if (hIcon)
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

         if (*szFullPath==0 || (hIcon = (HICON)LoadImage( NULL, szFullPath, IMAGE_ICON, 16,16, LR_LOADFROMFILE ))==NULL)
            hIcon = (HICON)LoadImage( kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 16,16, 0 );
         if (hIcon)
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

         hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);

         TreeView_SetImageList(hTree, gImagelist, TVSIL_NORMAL);

         workingBookmarks = new CBookmarkNode();
		 *workingBookmarks = *gBookmarkRoot;
         bookmarksEdited = false;

		 CResString strAllBookmarks = gLoc->GetString(IDS_ALL_BOOKMARKS);
         TVINSERTSTRUCT tvis;
         tvis.hParent = NULL;
         tvis.hInsertAfter = NULL;
         tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
         tvis.itemex.iImage = IMAGE_FOLDER_SPECIAL_CLOSED;
         tvis.itemex.iSelectedImage = IMAGE_FOLDER_SPECIAL_OPEN;
         tvis.itemex.pszText = (TCHAR*)(const TCHAR*)strAllBookmarks;
         tvis.itemex.lParam = (long)workingBookmarks;

         HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);

         // root starts out with all specials set to it - FillTree will unset any that are set elsewhere with ChangeSpecialFolders
         hTBitem = hNBitem = hBMitem = newItem;
         workingBookmarks->flags = BOOKMARK_FLAG_TB | BOOKMARK_FLAG_NB | BOOKMARK_FLAG_BM;

		 HIMAGELIST siteIcons = kPlugin.kFuncs->GetIconList();
         BOOL useSiteicon = TRUE;
         kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.bookmarks.displaySiteicon", &useSiteicon, &useSiteicon);
         useSiteicon &= (siteIcons != NULL); 
            		
         FillTree(hTree, newItem, *workingBookmarks, useSiteicon);

         TreeView_Expand(hTree, newItem, TVE_EXPAND);

         hCursorDrag = LoadCursor(kPlugin.hDllInstance, MAKEINTRESOURCE(IDC_DRAG_CURSOR));
         bDragging = false;

         int dialogleft = 50, dialogtop = 50, dialogwidth = 500, dialogheight = 500;
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_LEFT, &dialogleft, &dialogleft);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_TOP, &dialogtop, &dialogtop);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_WIDTH, &dialogwidth, &dialogwidth);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_EDIT_DLG_HEIGHT, &dialogheight, &dialogheight);
         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_EDIT_ZOOM, &zoom, &zoom);
         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_EDIT_MAX, &maximized, &maximized);
         SetWindowPos(hDlg, 0, dialogleft, dialogtop, dialogwidth, dialogheight, 0);
         if (maximized)
            ShowWindow(hDlg, SW_MAXIMIZE);
         else
            ShowWindow(hDlg, SW_NORMAL);

         hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, GetCurrentThreadId());
      }
      return false;
   case WM_NOTIFY:
      {
         NMTREEVIEW *nmtv = (NMTREEVIEW *)lParam;

         // Selection changed
         if (nmtv->hdr.code == (UINT) TVN_SELCHANGED){
            // Put the new url/title into the box
            CBookmarkNode *newNode = (CBookmarkNode *)nmtv->itemNew.lParam;

            if (newNode == workingBookmarks || newNode->type == BOOKMARK_SEPARATOR) {
               // root and separators have nothing
               SetDlgItemText(hDlg, IDC_TITLE, _T(""));
               SetDlgItemText(hDlg, IDC_URL, _T(""));
               SetDlgItemText(hDlg, IDC_NICK, _T(""));
               SetDlgItemText(hDlg, IDC_ADDED, _T(""));
               SetDlgItemText(hDlg, IDC_LAST_VISIT, _T(""));
               SetDlgItemText(hDlg, IDC_DESC, _T(""));
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TITLE), false);
               EnableWindow(GetDlgItem(hDlg, IDC_TITLE), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NICK), false);
               EnableWindow(GetDlgItem(hDlg, IDC_NICK), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ADDED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DESC), false);
               EnableWindow(GetDlgItem(hDlg, IDC_DESC), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), false);
               return true;   // a lazy-man's "else"
            }

            // everything else has at least title, added date, and properties in general
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TITLE), true);
            EnableWindow(GetDlgItem(hDlg, IDC_TITLE), true);
            SetDlgItemText(hDlg, IDC_TITLE, CUTF8_to_T(newNode->text.c_str()));
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ADDED), true);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), true);

            SYSTEMTIME st;
            TCHAR pszTmp[1024];
            TCHAR pszDate[900];

            if (newNode->type == BOOKMARK_BOOKMARK) {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NICK), true);
               EnableWindow(GetDlgItem(hDlg, IDC_NICK), true);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), true);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DESC), true);
               EnableWindow(GetDlgItem(hDlg, IDC_DESC), true);
               SetDlgItemText(hDlg, IDC_URL, CUTF8_to_T(newNode->url.c_str()));
               SetDlgItemText(hDlg, IDC_NICK, CUTF8_to_T(newNode->nick.c_str()));
               SetDlgItemText(hDlg, IDC_DESC, CUTF8_to_T(newNode->desc.c_str()));

               if (newNode->lastVisit) {
                  UnixTimeToSystemTime(newNode->lastVisit, &st);
                  GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
                  _tcscpy(pszTmp, pszDate);
                  _tcscat(pszTmp, _T(" "));
                  GetTimeFormat(GetThreadLocale(), (DWORD) NULL, &st, NULL, pszDate, 899);
                  _tcscat(pszTmp, pszDate);
                  SetDlgItemText(hDlg, IDC_LAST_VISIT, pszTmp);
               }
               else {
                  SetDlgItemText(hDlg, IDC_LAST_VISIT, gLoc->GetString(IDS_NEVER));
               }
            }
            else {
               SetDlgItemText(hDlg, IDC_URL, _T(""));
               SetDlgItemText(hDlg, IDC_NICK, _T(""));
               SetDlgItemText(hDlg, IDC_LAST_VISIT, _T(""));
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NICK), true);
               EnableWindow(GetDlgItem(hDlg, IDC_NICK), true);
               SetDlgItemText(hDlg, IDC_NICK, CUTF8_to_T(newNode->nick.c_str()));
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DESC), true);
               EnableWindow(GetDlgItem(hDlg, IDC_DESC), true);
               SetDlgItemText(hDlg, IDC_DESC, CUTF8_to_T(newNode->desc.c_str()));
            }

            UnixTimeToSystemTime(newNode->addDate, &st);
            GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
            _tcscpy(pszTmp, pszDate);
            _tcscat(pszTmp, _T(" "));
            GetTimeFormat(GetThreadLocale(), (DWORD) NULL, &st, NULL, pszDate, 899);
            _tcscat(pszTmp, pszDate);
            SetDlgItemText(hDlg, IDC_ADDED, pszTmp);
         }
         // start a drag operation
         else if (nmtv->hdr.code == (UINT) TVN_BEGINDRAG){
            TreeView_SelectItem(hTree, nmtv->itemNew.hItem);
            // don't move the root folder (thus, only move something if it has a parent)
            if (TreeView_GetParent(hTree, nmtv->itemNew.hItem)) {
               SetCursor(hCursorDrag);
               bDragging = true;
               SetCapture(hDlg);
            }
         }
         else if (nmtv->hdr.code == (UINT) NM_DBLCLK){
            TVHITTESTINFO hti;
            GetCursorPos(&hti.pt);
            ScreenToClient(hTree, &hti.pt);

            HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
            if (hItem){
               CBookmarkNode *node = GetBookmarkNode(hTree, hItem);

               if (node->type == BOOKMARK_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  bookmarksEdited = true;
                  kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
                  TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
                  PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);

                  return true;
               }
            }
         }
         // right click...
         else if (nmtv->hdr.code == (UINT) NM_RCLICK){
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
            SendMessage(hTree, WM_VSCROLL, wParam, (LPARAM) NULL);
         }
         else if (wParam == 2) {
            // need to expand folder

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

   case WM_SIZE:
      if(wParam != SIZE_MINIMIZED) {
         RECT rect;
         GetClientRect(hDlg, &rect);
         OnSize(rect.bottom, rect.right);
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
         if (HIWORD(wParam) == EN_CHANGE) {
            int id = LOWORD(wParam);
            if (id == IDC_TITLE && SendDlgItemMessage(hDlg, IDC_TITLE, EM_GETMODIFY, 0, 0)) {
               SendDlgItemMessage(hDlg, IDC_TITLE, EM_SETMODIFY, FALSE, 0);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateTitle(hDlg, hSelection);
               }
            }
            else if (id == IDC_URL && SendDlgItemMessage(hDlg, IDC_URL, EM_GETMODIFY, 0, 0)) {
               SendDlgItemMessage(hDlg, IDC_URL, EM_SETMODIFY, FALSE, 0);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateURL(hDlg, hSelection);
               }
            }
            else if (id == IDC_NICK && SendDlgItemMessage(hDlg, IDC_NICK, EM_GETMODIFY, 0, 0)) {
               SendDlgItemMessage(hDlg, IDC_NICK, EM_SETMODIFY, FALSE, 0);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateNick(hDlg, hSelection);
               }
            }
            else if (id == IDC_DESC && SendDlgItemMessage(hDlg, IDC_DESC, EM_GETMODIFY, 0, 0)) {
               SendDlgItemMessage(hDlg, IDC_DESC, EM_SETMODIFY, FALSE, 0);
               HTREEITEM hSelection = TreeView_GetSelection(hTree);
               if (hSelection) {
                  UpdateDesc(hDlg, hSelection);
               }
            }
         }
         else if (HIWORD(wParam) == BN_CLICKED) {
            WORD id = LOWORD(wParam);
            switch(id){

            case IDCANCEL:
               if (!zoom) {
                  if (bookmarksEdited) {

					 if (MessageBox(hDlg, (const TCHAR*)gLoc->GetString(IDS_CANCEL_CHANGES), gLoc->GetString(IDS_CANCEL_CAPTION), MB_OKCANCEL) == IDCANCEL)
						 return 0;
						 /*
                     FILE *bmFile = _tfopen(gBookmarkFile, _T("r"));
                     if (bmFile){
                        long bmFileSize = FileSize(bmFile);
                        
                        char *bmFileBuffer = new char[bmFileSize];
                        if (bmFileBuffer){
                           fread(bmFileBuffer, sizeof(char), bmFileSize, bmFile);
			   if (feof(bmFile)) {
			     delete gBookmarkRoot->child;
			     delete gBookmarkRoot->next;
			     gBookmarkRoot->child = NULL;
			     gBookmarkRoot->next = NULL;
			     
			     strtok(bmFileBuffer, "\n");
			     ParseBookmarks(bmFileBuffer, *gBookmarkRoot);
                             Rebuild();
                           }

                           delete [] bmFileBuffer;
                        }
                        fclose(bmFile);
                     }*/
				  }
                  UnhookWindowsHookEx(hHook);

                  if (hWndFront)
                     PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM) NULL);
                  ghWndEdit = NULL;

				  DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
				  DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));
				  delete workingBookmarks;
				  TreeView_DeleteAllItems(hTree);
                  DestroyWindow(hDlg);
                  break;
               }
               // fall through!

            case IDOK:
               TreeView_DeleteAllItems(hTree);
               if (bookmarksEdited) {
				  delete gBookmarkRoot;
                  gBookmarkRoot = workingBookmarks;
                  SaveBM(gBookmarkFile);

                  Rebuild();
               }
			   else
				   delete workingBookmarks;

               WINDOWPLACEMENT wp;
               wp.length = sizeof(wp);
               GetWindowPlacement(hDlg, &wp);

               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_LEFT, &wp.rcNormalPosition.left, FALSE);
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_TOP, &wp.rcNormalPosition.top, FALSE);
               int temp;
               temp = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_WIDTH, &temp, FALSE);
               temp = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_EDIT_DLG_HEIGHT, &temp, FALSE);
               kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_EDIT_ZOOM, &zoom, FALSE);
               temp = (wp.showCmd == SW_SHOWMAXIMIZED);
               kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_EDIT_MAX, &temp, FALSE);

               UnhookWindowsHookEx(hHook);
               if (hWndFront)
                  PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM) NULL);
               ghWndEdit = NULL;

			   
 			   DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
			   DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));			   
               DestroyWindow(hDlg);
               break;


            case ID_IMPORT_FAVORITES:
               ImportFavorites(GetDlgItem(hDlg, IDC_TREE_BOOKMARK));
               break;

            case ID_KEYBINDINGS:
               MessageBox(hDlg, gLoc->GetString(IDS_KEYS_HELP), gLoc->GetString(IDS_KEYBINDINGS), MB_ICONINFORMATION | MB_OK);
               break;
            }
         }
      }
   }
   return false;
}

static void FillTree(HWND hTree, HTREEITEM parent, CBookmarkNode &node, BOOL useSiteicon)
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
		 CUTF8_to_T title(child->text.c_str());
         tvis.itemex.pszText = (LPTSTR)(LPCTSTR)title; // XXX

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

         FillTree(hTree, thisItem, *child, useSiteicon);
      }
      else if (type == BOOKMARK_SEPARATOR) {
         tvis.itemex.iImage = IMAGE_BLANK;
         tvis.itemex.iSelectedImage = IMAGE_BLANK;
         tvis.itemex.pszText = _T("---");
         TreeView_InsertItem(hTree, &tvis);
      }
      else {
         tvis.itemex.iImage = IMAGE_BOOKMARK;
         if (useSiteicon) {
			UINT idx = GetSiteIcon(child);
            if (idx>0) {
			   HICON icon = ImageList_GetIcon(kPlugin.kFuncs->GetIconList(), idx, ILD_NORMAL);
			   if (icon) {
                  idx = ImageList_AddIcon(gImagelist, icon);
				  if (idx != -1)
					  tvis.itemex.iImage = idx;
                  DestroyIcon(icon);
			   }
            }
		 }
		 CUTF8_to_T title(child->text.c_str());
         tvis.itemex.iSelectedImage = tvis.itemex.iImage;
         tvis.itemex.pszText = (LPTSTR)(LPCTSTR)title; // XXX
         TreeView_InsertItem(hTree, &tvis);
      }
   }
}

static void CopyBranch(HWND hTree, HTREEITEM item, HTREEITEM newParent)
{
   TCHAR buffer[MAX_PATH];

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
   TCHAR buffer[MAX_PATH];

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
      oldParentNode = workingBookmarks;
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

         PostMessage(hTree, WM_VSCROLL, SB_LINEUP, (LPARAM) NULL);
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

            PostMessage(hTree, WM_VSCROLL, SB_LINEUP, (LPARAM) NULL);
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

         PostMessage(hTree, WM_VSCROLL, SB_LINEDOWN, (LPARAM) NULL);
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

   // check to see if we're dropping something in itself
   for (HTREEITEM temp = TreeView_GetParent(hTree, itemDrop); temp != NULL ; temp = TreeView_GetParent(hTree, temp)) {
      if (temp == item) {return;}
   }

   // setup the new parent
   tvis.hParent = TreeView_GetParent(hTree, itemDrop);
   if (tvis.hParent){
      newParentNode = GetBookmarkNode(hTree, tvis.hParent);
   }
   else{
      newParentNode = workingBookmarks;
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
		 if (!moveNode->next)
			 newParentNode->lastChild = moveNode;
      }
      else {
         moveNode->next = newParentNode->child;
         newParentNode->child = moveNode;
		 if (!moveNode->next)
			 newParentNode->lastChild = moveNode;
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

static void CreateNewObject(HWND hTree, HTREEITEM fromItem, int type, int mode) {
   CBookmarkNode *newNode;

   TVINSERTSTRUCT tvis;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   CResString strItem = gLoc->GetString(IDS_NEW_FOLDER);
   switch (type) {
   case BOOKMARK_BOOKMARK: {
      if (mode == PASTE && freeNode) {
         newNode = freeNode;
         freeNode = NULL;
      } else
         newNode = new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), CT_to_UTF8(gLoc->GetString(IDS_NEW_BOOKMARK)), "http://kmeleon.sourceforge.net/", "", "", "", BOOKMARK_BOOKMARK, time(NULL));
	  CUTF8_to_T title(newNode->text.c_str());
	  tvis.itemex.pszText = (LPTSTR)(LPCTSTR)title; // XXX
      tvis.itemex.iImage = IMAGE_BOOKMARK;
      tvis.itemex.iSelectedImage = IMAGE_BOOKMARK;
      break;
   }
   case BOOKMARK_FOLDER:
      newNode = new CBookmarkNode(0, CT_to_UTF8(gLoc->GetString(IDS_NEW_FOLDER)), "", "", "", "", BOOKMARK_FOLDER, time(NULL));
	  tvis.itemex.pszText = (TCHAR*)(const TCHAR*)strItem;
      tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
      tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
      break;
   case BOOKMARK_SEPARATOR:
      newNode = new CBookmarkNode(0, "", "", "", "", "", BOOKMARK_SEPARATOR);
      tvis.itemex.pszText = _T("---");
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
   
   // This function still fail somewhere
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
      if (TreeView_GetItem(hTree, &itemData))
	  {
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

static void CopyItem(HWND hTree, HTREEITEM item) {
   CBookmarkNode *node = GetBookmarkNode(hTree, item);
   HTREEITEM parent = TreeView_GetParent(hTree, item);
   if (parent) {
      if (freeNode && freeParentNode)
         freeParentNode->DeleteNode(freeNode);
      
      freeNode = new CBookmarkNode();
      *freeNode = *node;
      freeNode->id = kPlugin.kFuncs->GetCommandIDs(1);
      freeParentNode = GetBookmarkNode(hTree, parent);
   }
}

static void DeleteItem(HWND hTree, HTREEITEM item, int mode) {
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

// This just updates a node with the current Title and URL edit contents.
// Maybe it needn't update the tree if we're just exiting, but it's not too big a deal.
static void UpdateTitle(HWND hDlg, HTREEITEM item) {
   HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
   CBookmarkNode *node = GetBookmarkNode(hTree, item);

   if (node->type == BOOKMARK_BOOKMARK || node->type == BOOKMARK_FOLDER) {
      // FIXME - surely there's a better way to do this than allocating 1K for each string?
      // Isn't this a memory leak?  (Overwriting old buffer location, but old buffer remains allocated?)
      TCHAR TitleBuffer[1024];
      GetDlgItemText(hDlg, IDC_TITLE, TitleBuffer, 1023);
	  auto buffer = CT_to_UTF8(TitleBuffer);
      if (node->text.compare(buffer) != 0) {
         node->text = buffer;
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
      TCHAR URLBuffer[1024];
      GetDlgItemText(hDlg, IDC_URL, URLBuffer, 1023);
	  auto buffer = CT_to_UTF8(URLBuffer);
      if (node->url.compare(buffer) != 0) {
         node->url = buffer;
         bookmarksEdited = true;
      }
   }
}

static void UpdateNick(HWND hDlg, HTREEITEM item) {
   HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
   CBookmarkNode *node = GetBookmarkNode(hTree, item);

   if (node->type == BOOKMARK_BOOKMARK || node->type == BOOKMARK_FOLDER) {
      TCHAR szBuffer[1024];
      GetDlgItemText(hDlg, IDC_NICK, szBuffer, 1023);
	  auto buffer = CT_to_UTF8(szBuffer);
      if (node->nick.compare(buffer) != 0) {
         node->nick = buffer;
         bookmarksEdited = true;
      }
   }
}


static void UpdateDesc(HWND hDlg, HTREEITEM item) {
   HWND hTree = GetDlgItem(hDlg, IDC_TREE_BOOKMARK);
   CBookmarkNode *node = GetBookmarkNode(hTree, item);

   if (node->type == BOOKMARK_BOOKMARK || node->type == BOOKMARK_FOLDER) {
      TCHAR szBuffer[1024];
      GetDlgItemText(hDlg, IDC_DESC, szBuffer, 1023);
	  auto buffer = CT_to_UTF8(szBuffer);
      if (node->desc.compare(buffer) != 0) {
         node->desc = buffer;
         bookmarksEdited = true;
      }
   }
}


static void OnRClick(HWND hTree)
{
   POINT mouse;
   GetCursorPos(&mouse);

   TVHITTESTINFO hti;
   hti.pt.x = mouse.x;
   hti.pt.y = mouse.y;
   ScreenToClient(hTree, &hti.pt);

   HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
   if (hItem) {
      TreeView_SelectItem(hTree, hItem);

      HMENU topMenu = gLoc->LoadMenu(IDR_CONTEXTMENU);
      HMENU contextMenu = GetSubMenu(topMenu, 0);

      CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
      if (node && node->type != BOOKMARK_FOLDER) {
         EnableMenuItem(contextMenu, ID__OPEN, MF_BYCOMMAND | MF_ENABLED);
         EnableMenuItem(contextMenu, ID__OPEN_BACKGROUND, MF_BYCOMMAND | MF_ENABLED);
		 EnableMenuItem(contextMenu, ID__OPEN_NEWTAB, MF_BYCOMMAND | MF_ENABLED);
		 EnableMenuItem(contextMenu, ID__OPEN_BACKGROUNDTAB, MF_BYCOMMAND | MF_ENABLED);
         EnableMenuItem(contextMenu, ID__SETAS_TOOLBARFOLDER, MF_BYCOMMAND | MF_GRAYED);
         EnableMenuItem(contextMenu, ID__SETAS_BOOKMARKMENU, MF_BYCOMMAND | MF_GRAYED);
         EnableMenuItem(contextMenu, ID__SETAS_NEWBOOKMARKFOLDER, MF_BYCOMMAND | MF_GRAYED);
      }
      else {
         EnableMenuItem(contextMenu, ID__OPEN, MF_BYCOMMAND | MF_GRAYED);
         EnableMenuItem(contextMenu, ID__OPEN_BACKGROUND, MF_BYCOMMAND | MF_GRAYED);
		 EnableMenuItem(contextMenu, ID__OPEN_NEWTAB, MF_BYCOMMAND | MF_GRAYED);
		 EnableMenuItem(contextMenu, ID__OPEN_BACKGROUNDTAB, MF_BYCOMMAND | MF_GRAYED);
         EnableMenuItem(contextMenu, ID__SETAS_TOOLBARFOLDER, MF_BYCOMMAND | MF_ENABLED);
         EnableMenuItem(contextMenu, ID__SETAS_BOOKMARKMENU, MF_BYCOMMAND | MF_ENABLED);
         EnableMenuItem(contextMenu, ID__SETAS_NEWBOOKMARKFOLDER, MF_BYCOMMAND | MF_ENABLED);
      }

      if (node && (node->flags & BOOKMARK_FLAG_TB))
         CheckMenuItem(contextMenu, ID__SETAS_TOOLBARFOLDER, MF_BYCOMMAND | MF_CHECKED);
      else
         CheckMenuItem(contextMenu, ID__SETAS_TOOLBARFOLDER, MF_BYCOMMAND | MF_UNCHECKED);
      if (node && (node->flags & BOOKMARK_FLAG_BM))
         CheckMenuItem(contextMenu, ID__SETAS_BOOKMARKMENU, MF_BYCOMMAND | MF_CHECKED);
      else
         CheckMenuItem(contextMenu, ID__SETAS_BOOKMARKMENU, MF_BYCOMMAND | MF_UNCHECKED);
      if (node && (node->flags & BOOKMARK_FLAG_NB))
         CheckMenuItem(contextMenu, ID__SETAS_NEWBOOKMARKFOLDER, MF_BYCOMMAND | MF_CHECKED);
      else
         CheckMenuItem(contextMenu, ID__SETAS_NEWBOOKMARKFOLDER, MF_BYCOMMAND | MF_UNCHECKED);

      if (zoom)
	CheckMenuItem(contextMenu, ID__ZOOM, MF_BYCOMMAND | MF_UNCHECKED);
      else 
	CheckMenuItem(contextMenu, ID__ZOOM, MF_BYCOMMAND | MF_CHECKED);
      
      bTracking = TRUE;
      int command = TrackPopupMenu(contextMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD, mouse.x, mouse.y, 0, hTree, NULL);
      bTracking = FALSE;

      switch (command) {
      case ID__OPEN:
         {
            CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
            if (node->type == BOOKMARK_BOOKMARK) {
               node->lastVisit = time(NULL);
               bookmarksEdited = true;
               kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
               PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
            }
         }
         break;
      case ID__OPEN_BACKGROUND:
         {
            CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
            if (node->type == BOOKMARK_BOOKMARK) {
               node->lastVisit = time(NULL);
               bookmarksEdited = true;
               kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUND, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
            }
         }
         break;
      case ID__OPEN_BACKGROUNDTAB:
         {
            CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
            if (node->type == BOOKMARK_BOOKMARK) {
               node->lastVisit = time(NULL);
               bookmarksEdited = true;
			   kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUNDTAB, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
            }
         }
         break;
      case ID__OPEN_NEWTAB:
         {
            CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
            if (node->type == BOOKMARK_BOOKMARK) {
               node->lastVisit = time(NULL);
               bookmarksEdited = true;
			   kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NEWTAB, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
			   PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
            }
         }
         break;
      case ID__NEW_FOLDER:
         CreateNewObject(hTree, hItem, BOOKMARK_FOLDER);
         SetFocus(GetDlgItem(hEditWnd, IDC_TITLE));
         SendDlgItemMessage(hEditWnd, IDC_TITLE, EM_SETSEL, 0, -1);
         break;
      case ID__NEW_SEPARATOR:
         CreateNewObject(hTree, hItem, BOOKMARK_SEPARATOR);
         break;
      case ID__NEW_BOOKMARK:
         CreateNewObject(hTree, hItem, BOOKMARK_BOOKMARK);
         SetFocus(GetDlgItem(hEditWnd, IDC_TITLE));
         SendDlgItemMessage(hEditWnd, IDC_TITLE, EM_SETSEL, 0, -1);
         break;
      case ID__SETAS_TOOLBARFOLDER:
         ChangeSpecialFolder(hTree, &hTBitem, hItem, BOOKMARK_FLAG_TB);
         bookmarksEdited = true;    // ChangeSpecialFolder is used when filling the tree initially, as well, so we don't want it to set bookmarksModified itself
         break;
      case ID__SETAS_NEWBOOKMARKFOLDER:
         ChangeSpecialFolder(hTree, &hNBitem, hItem, BOOKMARK_FLAG_NB);
         bookmarksEdited = true;
         break;
      case ID__SETAS_BOOKMARKMENU:
         ChangeSpecialFolder(hTree, &hBMitem, hItem, BOOKMARK_FLAG_BM);
         bookmarksEdited = true;
         break;
      case ID__BOOKMARK_DELETE:
         DeleteItem(hTree, hItem);
         break;
      case ID__SORT:
      case ID__SORT_ALL:
         {
            CBookmarkNode *node = GetBookmarkNode(hTree, hItem);
            if (!node) {
               break;
            }
            if (node->type != BOOKMARK_FOLDER){
               hItem = TreeView_GetParent(hTree, hItem);
               node = GetBookmarkNode(hTree, hItem);
               if (!node || node->type != BOOKMARK_FOLDER) {
                  break;
               }
            }

            TreeView_SelectItem(hTree, hItem);
            HTREEITEM hChild = TreeView_GetChild(hTree, hItem);
            while (hChild) {
               TreeView_DeleteItem(hTree, hChild);
               hChild = TreeView_GetChild(hTree, hItem);
            }
            
            int order = 21;
            node->sort( (order << 1) | (command==ID__SORT_ALL) );
            
            BOOL useSiteicon = TRUE;
            kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.bookmarks.displaySiteicon", &useSiteicon, &useSiteicon);
            useSiteicon &= (kPlugin.kFuncs->GetIconList() != NULL); 
            FillTree(hTree, hItem, *node, useSiteicon);
            
            TreeView_Expand(hTree, hItem, TVE_EXPAND);
            TreeView_SelectItem(hTree, hItem);

            bookmarksEdited = true;
         }
         break;
      case ID__ZOOM:
         {
            zoom = !zoom;

            RECT rect;
            GetClientRect(hEditWnd, &rect);            
            OnSize(rect.bottom, rect.right);

            TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier
         }
         break;
      }
	  DestroyMenu(topMenu);
   }
}

// this is ugly, see...  all this need to be changed each time the dialog is re-arranged...
#define BORDER 8  // this one is pixels, the rest are DLUs
#define BUTTON_HEIGHT 15
#define HELP_WIDTH 27
#define OK_WIDTH 45
#define CANCEL_WIDTH 45
#define PROPERTIES_HEIGHT 73
#define EDITBOXES_LEFT 33
#define EDITBOXES_TOP 85
#define EDITBOXES_HEIGHT 12
#define DATES_WIDTH 78
#define DESC_WIDTH 25
#define NICK_WIDTH 25

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
   SetWindowPos(GetDlgItem(hEditWnd, IDC_TREE_BOOKMARK), 0, 
                zoom ? 0 : BORDER, 
                zoom ? 0 : BORDER, 
                zoom ? width : width-(BORDER*2), 
                zoom ? height : height-BORDER*4-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                0);
   
   // move cancel button
   SetWindowPos(GetDlgItem(hEditWnd, IDCANCEL), 0, 
                width-BORDER-convX(CANCEL_WIDTH), 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);
   
   // move ok button
   SetWindowPos(GetDlgItem(hEditWnd, IDOK), 0, 
                width-BORDER*2-convX(CANCEL_WIDTH+OK_WIDTH), 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);
   
   // move import favorites button
   SetWindowPos(GetDlgItem(hEditWnd, ID_IMPORT_FAVORITES), 0, 
                BORDER*2+convX(HELP_WIDTH), 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);
   
   // move help button
   SetWindowPos(GetDlgItem(hEditWnd, ID_KEYBINDINGS), 0, 
                BORDER, 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);

   // move/resize properties box
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_PROPERTIES), 0, 
                BORDER, 
                zoom ? height + BORDER : height-BORDER*2-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                width-BORDER*2, 
                convY(PROPERTIES_HEIGHT), 
                0);

   // move/resize properties widgets
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_TITLE), 0, BORDER*2, zoom ? height + BORDER : height-convY(EDITBOXES_TOP), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_TITLE), 0, convX(EDITBOXES_LEFT), zoom ? height + BORDER : height-convY(EDITBOXES_TOP)-2, width-convX(EDITBOXES_LEFT)-BORDER*2, convY(EDITBOXES_HEIGHT), 0);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_URL), 0, BORDER*2, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*1.25), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_URL), 0, convX(EDITBOXES_LEFT), zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*1.25)-2, width-convX(EDITBOXES_LEFT)-BORDER*2, convY(EDITBOXES_HEIGHT), 0);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_ADDED), 0, BORDER*2, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_ADDED), 0, convX(EDITBOXES_LEFT)+5, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), convX(DATES_WIDTH), convY(EDITBOXES_HEIGHT), 0);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_VISITED), 0, BORDER*2, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_LAST_VISIT), 0, convX(EDITBOXES_LEFT)+5, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), convX(DATES_WIDTH), convY(EDITBOXES_HEIGHT), 0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_NICK), 0, convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_NICK), 0, convX(EDITBOXES_LEFT+DATES_WIDTH+NICK_WIDTH)+BORDER, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*2.5), width-convX(EDITBOXES_LEFT+DATES_WIDTH+NICK_WIDTH)-BORDER*3, convY(EDITBOXES_HEIGHT), 0);

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_DESC), 0, convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), 0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_DESC), 0, convX(EDITBOXES_LEFT+DATES_WIDTH+DESC_WIDTH)+BORDER, zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75)-2, width-convX(EDITBOXES_LEFT+DATES_WIDTH+DESC_WIDTH)-BORDER*3, convY(EDITBOXES_HEIGHT), 0);

   // eliminate those ugly artifacts (that show up on my machine, at least...)
   InvalidateRgn(hEditWnd, NULL, FALSE);
   UpdateWindow(hEditWnd);
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

#ifndef __MINGW32__
   // first try the correct way
   if (SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &idl) == NOERROR) {
      IMalloc *malloc;
      SHGetPathFromIDList(idl, sz);
      SHGetMalloc(&malloc);
      malloc->Free(idl);
      malloc->Release();
   }  
   else 
#endif
   {
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

      _tcscat(FavoritesPath, _T("\\"));
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
   CBookmarkNode *newFavoritesNode = new CBookmarkNode(0, CT_to_UTF8(gLoc->GetString(IDS_IMPORTED_FAVORITES)), "", "", "", "", BOOKMARK_FOLDER, time(NULL));
   workingBookmarks->AddChild(newFavoritesNode);

   BuildFavoritesTree(FavoritesPath, _T(""), newFavoritesNode);

   bookmarksEdited = true;
   CResString strImpFav = gLoc->GetString(IDS_IMPORTED_FAVORITES);
   // Add the new folder to the TreeView
   TVINSERTSTRUCT tvis;
   tvis.hParent = TreeView_GetRoot(hTree);
   tvis.hInsertAfter = NULL;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   tvis.itemex.iImage = IMAGE_FOLDER_CLOSED;
   tvis.itemex.iSelectedImage = IMAGE_FOLDER_OPEN;
   tvis.itemex.pszText = (TCHAR*)(const TCHAR*)strImpFav;
   tvis.itemex.lParam = (long)newFavoritesNode;

   HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);
   
   BOOL useSiteicon = TRUE;
   kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.bookmarks.displaySiteicon", &useSiteicon, &useSiteicon);
   useSiteicon &= (kPlugin.kFuncs->GetIconList() != NULL); 
   FillTree(hTree, newItem, *newFavoritesNode, useSiteicon);

   TreeView_SelectItem(hTree, newItem);
}

static void BuildFavoritesTree(TCHAR *FavoritesPath, TCHAR* strPath, CBookmarkNode *newFavoritesNode) {

   int pathLen = _tcslen(strPath);
   int FavoritesPathLen = _tcslen(FavoritesPath);

   TCHAR * searchString = new TCHAR[FavoritesPathLen + pathLen + 2];
   _tcscpy(searchString, FavoritesPath);
   _tcscat(searchString, strPath);
   _tcscat(searchString, _T("*"));

   TCHAR * urlFile;
   TCHAR * subPath;

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

         subPath = new TCHAR[pathLen + _tcslen(wfd.cFileName) + 2];
         _tcscpy(subPath, strPath);
         _tcscat(subPath, wfd.cFileName);         
         _tcscat(subPath, _T("/"));

         // make new node for favorites (child off of our current node)
         CBookmarkNode *newFavoritesChildNode = new CBookmarkNode(0, CT_to_UTF8(wfd.cFileName), "", "", "", "", BOOKMARK_FOLDER, time(NULL));
         newFavoritesNode->AddChild(newFavoritesChildNode);

         // build the tree for this directory
         BuildFavoritesTree(FavoritesPath, subPath, newFavoritesChildNode);

         delete [] subPath;

      }else if ((wfd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0) {
         // if it's not a hidden or system file

         TCHAR *dot = _tcsrchr(wfd.cFileName, '.');
         if(dot && _tcsicmp(dot, _T(".url")) == 0) {

            int filenameLen = (dot - wfd.cFileName) + 4;
            urlFile = new TCHAR[pathLen + filenameLen + 1];
            _tcscpy(urlFile, strPath);
            _tcscat(urlFile, wfd.cFileName);

            // format for display in the menu
            // chop off the .url
            *dot = 0;
            // condense the string and escape ampersands
            const TCHAR *pszTemp = fixString(wfd.cFileName, 40);

            FavoritesPath[FavoritesPathLen] = 0;

            TCHAR path[MAX_PATH];
            _tcscpy(path, FavoritesPath);
            _tcscpy(path+FavoritesPathLen, urlFile);

            // a .URL file is formatted just like an .INI file, so we can
            // use GetPrivateProfileString() to get the information we want
            TCHAR url[INTERNET_MAX_URL_LENGTH];
            GetPrivateProfileString(_T("InternetShortcut"), _T("URL"), _T(""), url, INTERNET_MAX_URL_LENGTH, path);

            // insert node
            newFavoritesNode->AddChild(new CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), CT_to_UTF8(wfd.cFileName), CT_to_UTF8(url), "", "", "", BOOKMARK_BOOKMARK, time(NULL)));

            delete pszTemp;
            delete [] urlFile;
         }
      }
   } while(FindNextFile(h, &wfd));
   FindClose(h);
}
