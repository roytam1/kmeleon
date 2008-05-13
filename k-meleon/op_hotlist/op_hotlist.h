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

#ifndef __OP_HOTLIST_H__
#define __OP_HOTLIST_H__

#ifndef COMPILING_RC
typedef int cmp_t(const char *, const char *, unsigned int);
void quicksort(char *a, size_t n, size_t es, cmp_t *cmp, unsigned int flag);
#  include "BookmarkNode.h"
#  include <stdio.h>
#  include "../kmeleon_plugin.h"
#endif

#include "resource.h"
#include <wininet.h>    // for INTERNET_MAX_URL_LENGTH
#include <commctrl.h>


#define TOOLBAND_NAME "Hotlist"
#define TOOLBAND_FAILED_TO_CREATE "Failed to create hotlist toolbar"
#define PLUGIN_NAME "Opera Hotlist Plugin"
#define MENU_TO_COMMAND(x) (x+SUBMENU_OFFSET)

#define IDB_IMAGES                      158
#define IDB_ICON                        159

#define IMAGE_BLANK         -1
#define IMAGE_FOLDER_CLOSED 0
#define IMAGE_FOLDER_OPEN   1
#define IMAGE_BOOKMARK      2
#define IMAGE_CHEVRON       3
#define IMAGE_FOLDER_SPECIAL_CLOSED 4
#define IMAGE_FOLDER_SPECIAL_OPEN   5

#define HOTLIST_DEFAULT_FILENAME "opera.adr"
#define HOTLIST_FILTER "Hotlist Files\0opera.adr;opera5.adr;opera6.adr\0ADR Files\0*.adr\0"
#define HOTLIST_NOT_FOUND "Your existing hotlist file could not be found.\n\n" \
                            "Would you like to locate this file?\n"


#ifdef WHERE
CBookmarkNode gHotlistRoot(0, "", "", "", "", BOOKMARK_FOLDER, 0);
#else
#define WHERE extern
WHERE CBookmarkNode gHotlistRoot;
#endif

// The interface

#ifndef COMPILING_RC
int op_readFile(char *);
int op_writeFile(char *);
int op_addEntry(char *, CBookmarkNode *);
void BuildMenu(HMENU menu, CBookmarkNode *node, BOOL isContinuation);
void RebuildMenu();
void BuildRebar(HWND hWndTB);
void RebuildRebarMenu(HWND hWndTB);
int ParseHotlist(char **bmFileBuffer, CBookmarkNode &node);
int SaveHotlistEntry(FILE *bmFile, CBookmarkNode *node);
int addLink(char *url, char *title);
void findNick(char *nick, char **url);
LRESULT APIENTRY WndTBSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void FindSkinFile( char *szSkinFile, const char *filename );

extern "C" {
   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis);
}

struct hotlistTB {
   HWND hWnd;
   HWND hWndTB;
   struct hotlistTB *next;
};
typedef struct hotlistTB TB;

WHERE HWND hWndFront;
WHERE TB *root;
WHERE TB *pNewTB;

TB *create_TB(HWND hWnd);
TB *find_TB(HWND hWnd);
void remove_TB(HWND hWnd);
#endif

void Config(HWND parent);

#define PREFERENCE_REBAR_ENABLED "kmeleon.plugins.hotlist.rebar"
#define PREFERENCE_CHEVRON_ENABLED "kmeleon.plugins.hotlist.chevron"
#define PREFERENCE_HOTLIST_FILE  "kmeleon.plugins.hotlist.hotlistFile"
#define PREFERENCE_HOTLIST_RESYNCH  "kmeleon.plugins.hotlist.resynch"
#define PREFERENCE_MENU_MAXLEN "kmeleon.plugins.hotlist.maxMenuLength"
#define PREFERENCE_MENU_AUTOLEN "kmeleon.plugins.hotlist.menuAutoDetect"
#define PREFERENCE_MENU_SORTORDER "kmeleon.plugins.hotlist.sortOrder"
#define PREFERENCE_TOOLBAR_FOLDER "kmeleon.plugins.hotlist.toolbarFolder"
#define PREFERENCE_MENU_FOLDER "kmeleon.plugins.hotlist.menuFolder"
#define PREFERENCE_NEWITEM_FOLDER "kmeleon.plugins.hotlist.newitemFolder"
#define PREFERENCE_BUTTON_MINWIDTH  "kmeleon.plugins.hotlist.buttonMinWidth"
#define PREFERENCE_BUTTON_MAXWIDTH  "kmeleon.plugins.hotlist.buttonMaxWidth"
#define PREFERENCE_BUTTON_ICONS  "kmeleon.plugins.hotlist.buttonIcons"
#define PREFERENCE_EDIT_DLG_LEFT   "kmeleon.plugins.hotlist.editdialog.left"
#define PREFERENCE_EDIT_DLG_TOP    "kmeleon.plugins.hotlist.editdialog.top"
#define PREFERENCE_EDIT_DLG_WIDTH  "kmeleon.plugins.hotlist.editdialog.width"
#define PREFERENCE_EDIT_DLG_HEIGHT "kmeleon.plugins.hotlist.editdialog.height"
#define PREFERENCE_EDIT_ZOOM "kmeleon.plugins.hotlist.editdialog.zoom"
#define PREFERENCE_EDIT_MAX "kmeleon.plugins.hotlist.editdialog.maximized"
#define PREFERENCE_HOTLIST_OPENURL "kmeleon.plugins.hotlist.openurl"

// The globals

WHERE WNDPROC wpOrigTBWndProc;
WHERE BOOL bRebarEnabled;
WHERE BOOL bChevronEnabled;
WHERE BOOL bResynchHotlist;
WHERE BOOL bCreate;
WHERE BOOL bIgnore;
WHERE CHAR gHotlistFile[MAX_PATH];
WHERE CHAR gToolbarFolder[MAX_PATH];
WHERE CHAR gMenuFolder[MAX_PATH];
WHERE CHAR gNewitemFolder[MAX_PATH];

extern kmeleonPlugin kPlugin;

WHERE UINT nConfigCommand;
WHERE UINT nAddCommand;
WHERE UINT nAddLinkCommand;
// WHERE UINT nAddToolbarCommand;
WHERE UINT nEditCommand;
WHERE UINT nDropdownCommand;
WHERE UINT nUpdateTB;
WHERE UINT nFirstHotlistPosition;
WHERE UINT wm_deferhottrack;
WHERE UINT wm_deferbringtotop;

WHERE char *lpszHotlistFile;

#define HOTLIST_TITLE_LEN INTERNET_MAX_URL_LENGTH
#define HOTLIST_STRING_LEN 2048
WHERE char gHotlistTitle[HOTLIST_TITLE_LEN];

WHERE HMENU gMenuHotlist;

WHERE BOOL gHotlistModified;

WHERE int gMaxMenuLength;
WHERE BOOL gMenuAutoDetect;
WHERE int gMenuSortOrder;

WHERE BOOL bDOS;
WHERE BOOL bEmpty;

WHERE int  nButtonMinWidth;
WHERE int  nButtonMaxWidth;
WHERE BOOL bButtonIcons;
WHERE int nHSize, nHRes;

WHERE HWND ghWndEdit;
WHERE HIMAGELIST gImagelist; // the one and only imagelist...
WHERE HANDLE ghMutex;

#endif
