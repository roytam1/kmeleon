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
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "op_hotlist.h"
#include "../kmeleon_plugin.h"
#include "..\\rebar_menu\\hot_tracking.h"

#include "../Utils.h"
#include <sys/stat.h>
#include <errno.h>

extern kmeleonPlugin kPlugin;
extern void * KMeleonWndProc;

#include <string.h>



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
#define cmenu 18
   
   // space to allow above menu for title bar, menu bar (assuming maximized window), and extra frame junk
#define MENUPADDING 50
   
   //   int maxLength = (gMenuAutoDetect) ? (int)((cy-MENUPADDING)/cmenu) : gMaxMenuLength;
   int maxLength = 100;
   
   CBookmarkNode *child;
   int count = GetMenuItemCount(menu);
   
   for (child = (isContinuation) ? node : node->child ; child ; child = child->next , count++) {
      bEmpty = false;
      if (count == maxLength) {
         HMENU childMenu = CreatePopupMenu();
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, "[more]");
         BuildMenu(childMenu, child, true);
         break;
      }
      else if (child->type == BOOKMARK_SEPARATOR) {
         AppendMenu(menu, MF_SEPARATOR, 0, "");
      }
      else if (child->type == BOOKMARK_FOLDER) {
         HMENU childMenu = CreatePopupMenu();
         child->id = (UINT)childMenu; // we have to save off the HMENU for the rebar
         // condense the title and escape ampersands
         char *pszTemp = fixString(child->text.c_str(), 40);
         AppendMenu(menu, MF_STRING|MF_POPUP, (UINT)childMenu, pszTemp);
         delete pszTemp;
         BuildMenu(childMenu, child, false);
      }
      else if (child->type == BOOKMARK_BOOKMARK) {
         // condense the title and escape ampersands
         char *pszTemp = fixString(child->text.c_str(), 40);
         AppendMenu(menu, MF_STRING, child->id, pszTemp);
         delete pszTemp;
      }
   }
}


// Build Rebar
void BuildRebar(HWND hWndTB)
{
   if (!bRebarEnabled || !hWndTB || !gImagelist)
      return;
   
   CBookmarkNode *toolbarNode = &gHotlistRoot;
   
   SetWindowText(hWndTB, TOOLBAND_NAME);
   
   //SendMessage(hWndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
   
   SendMessage(hWndTB, TB_SETIMAGELIST, 0, (LPARAM)gImagelist);
   
   SendMessage(hWndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));
   
   int stringID;
   
   CBookmarkNode *child;
   
   for (child=toolbarNode->child; child; child=child->next) {
      if (child->type == BOOKMARK_SEPARATOR) {
         continue;
      }
      
      // condense the title and escape ampersands
      char *buttonString = fixString(child->text.c_str(), 20);
      stringID = SendMessage(hWndTB, TB_ADDSTRING, (WPARAM)NULL, (LPARAM)buttonString);
      delete buttonString;
      
      TBBUTTON button = {0};
      button.iString = stringID;
      button.fsState = TBSTATE_ENABLED;
      button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
      
      if (child->type == BOOKMARK_FOLDER){
         // toolbar may not be contained in bookmark menu, so we can't just use its submenus
         HMENU childMenu = CreatePopupMenu();
         BuildMenu(childMenu, child, false);
         button.idCommand = MENU_TO_COMMAND((UINT)childMenu);
         button.iBitmap = IMAGE_FOLDER_CLOSED;
         button.fsStyle |= TBSTYLE_DROPDOWN;
      }
      else {
         button.idCommand = child->id;
         button.iBitmap = IMAGE_BOOKMARK;
      }
      
      SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)-1, (LPARAM)&button);
   }
   /*
     TBBUTTON button = {0};
     button.fsState = TBSTATE_ENABLED;
     button.fsStyle = TBSTYLE_SEP;
     SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);
     
     //button.iBitmap = IMAGE_CHEVRON;
     button.idCommand = nDropdownCommand;
     button.fsState = TBSTATE_ENABLED;
     button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
     button.iString = -1;
     SendMessage(hWndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);
   */
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   // store these in static vars so that the BeginHotTrack call can access them
   static NMTOOLBAR tbhdr;
   static NMHDR hdr;
   
   if (message == WM_COMMAND) {
      WORD command = LOWORD(wParam);
      
      if (command == nConfigCommand){
         Config(hWnd);
         return 1;
      }
      else if (command == nAddCommand ||
               command == nAddLinkCommand) {
         char *title = 0;
         char *url = 0;
         
         if (command == nAddCommand) {
            kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
            if (dInfo) {
               url = dInfo->url;
               title = dInfo->title;
            }
         }
         else {
#if 0
            /*
              POINTS curpos;
              GetCursorPos(&curpos);
            */
            POINTS pos;
            DWORD msgpos = GetMessagePos();
            pos = MAKEPOINTS(msgpos);
            kmeleonPointInfo *pInfo = 
               kPlugin.kFuncs->GetInfoAtPoint(pos.x, pos.y);
            if (pInfo) {
               url = pInfo->link;
            }
#else
            return 0;
#endif
         }
         if (url) {
            // int bHotlistExists = gHotlistRoot.child != NULL;
            
            CBookmarkNode *newNode = new 
               CBookmarkNode(kPlugin.kFuncs->GetCommandIDs(1), 
                             title ? title : url, url, 
                             BOOKMARK_BOOKMARK, time(NULL));
            if (!newNode)
               return false;

            gHotlistRoot.AddChild(newNode);
            
            if (lpszHotlistFile && *lpszHotlistFile) {
               
               int bNewFile = 0;
               struct stat sb;
               if (stat (lpszHotlistFile, &sb) == -1 && errno == ENOENT)
                  bNewFile = 1;
               
               if (bNewFile) {
                  FILE *bmFile = fopen(lpszHotlistFile, "w+");
                  if (bmFile)
                     fclose(bmFile);
               }
               FILE *bmFile = fopen(lpszHotlistFile, "rb+");
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
                  SaveHotlistEntry(bmFile, newNode);
                  if (bDOS) 
                     fprintf(bmFile, "\r");
                  fprintf(bmFile, "\n");
                  fprintf(bmFile, "-");
                  if (bDOS) 
                     fprintf(bmFile, "\r");
                  fprintf(bmFile, "\n");
                  fclose(bmFile);
               }
               else
                  MessageBox(hWnd, "Unable to save hotlist", "Error", MB_ICONSTOP|MB_OK);
            }
            else
               MessageBox(hWnd, "Unable to save hotlist", "Error", MB_ICONSTOP|MB_OK);
            
            if (!bEmpty) {
               
               // FIXME!
               // this "deletes everything after the first bookmark position"
               // which is not robust as there may be normal menu items there
               
               while (DeleteMenu(gMenuHotlist, nFirstHotlistPosition, MF_BYPOSITION))
                  ;
            }
            bEmpty = true;
            BuildMenu(gMenuHotlist, &gHotlistRoot, false);

#if 0
            // BUG!
            // Only the last toolbar would get updated
            // The ghWndTB must be one per frame, not one global

            if (ghWndTB) {
               // delete the old rebar
               while (SendMessage(ghWndTB, TB_DELETEBUTTON, 0 /*index*/, 0));
               // and rebuild
               BuildRebar(ghWndTB);
            }
#endif

         }
         return true;
      }
      else if (CBookmarkNode *node = gHotlistRoot.FindNode(command)) {
         node->lastVisit = time(NULL);
         gHotlistModified = true;   // this doesn't call for instant saving, it can wait until we add/edit/quit
         kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL);
         
         return true;
      }
   }
   else if (message == WM_NOTIFY) {
      hdr = *((LPNMHDR)lParam);
      if (hdr.code == TBN_DROPDOWN) {
         tbhdr = *((LPNMTOOLBAR)lParam);
         
         /*
           // this is the little down arrow thing
           if (tbhdr.iItem == nDropdownCommand){
              RECT rc;
              WPARAM index = 0;
              SendMessage(tbhdr.hdr.hwndFrom, TB_GETITEMRECT, index, (LPARAM) &rc);
              POINT pt = { rc.left, rc.bottom };
              ClientToScreen(tbhdr.hdr.hwndFrom, &pt);
              TrackPopupMenu((HMENU)gMenuHotlist, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            }
            else 
         */
         if (IsMenu((HMENU)(tbhdr.iItem-SUBMENU_OFFSET))){
            char toolbarName[11];
            GetWindowText(tbhdr.hdr.hwndFrom, toolbarName, 10);
            if (strcmp(toolbarName, TOOLBAND_NAME) != 0) {
               // oops, this isn't our toolbar
               return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
            }
            
            // post a message to defer exceution of BeginHotTrack
            PostMessage(hWnd, WM_DEFERHOTTRACK, (WPARAM)NULL, (LPARAM)NULL);
            
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
   }
   else if (message == WM_DEFERHOTTRACK) {
      BeginHotTrack(&tbhdr, kPlugin.hDllInstance, hWnd);
      return true;
   }
   else if (message == WM_MENUSELECT) {
      UINT id = LOWORD(wParam);
      if (id >= nConfigCommand && id < nDropdownCommand) {
         if (id == nConfigCommand) 
            kPlugin.kFuncs->SetStatusBarText("Configure the hotlist plugin");
         else if (id == nAddCommand) 
            kPlugin.kFuncs->SetStatusBarText("Add to hotlist");
         else if (id == nAddLinkCommand) 
            kPlugin.kFuncs->SetStatusBarText("Add link to hotlist");
      }
      else if (CBookmarkNode *node = gHotlistRoot.FindNode(LOWORD(id))) {
         kPlugin.kFuncs->SetStatusBarText((char *)node->url.c_str());
         return true;
      }
   }
#if 0
   else if (message == WM_SETFOCUS) {
      kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_HOTLIST_RESYNCH, &bResynchHotlist, &bResynchHotlist);
      if (bResynchHotlist) {
         if (ReloadHotlist()) {
            if (!bEmpty)
               while (DeleteMenu(gMenuHotlist, nFirstHotlistPosition, MF_BYPOSITION))
                  ;
            bEmpty = true;
            BuildMenu(gMenuHotlist, &gHotlistRoot, false);
         }
      }
   }
#endif
   
   return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
}
