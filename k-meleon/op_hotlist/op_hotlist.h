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

#include <wininet.h>    // for INTERNET_MAX_URL_LENGTH
#include <commctrl.h>


#define ID_CREATE                       501
#define ID_SEARCH                       502

#define IDD_CONFIG                      101
#define IDD_INSTALL                     102
#define IDD_EDIT_HOTLIST		103
#define IDR_CONTEXTMENU                 159
#define IDC_DRAG_CURSOR                 160

#define IDC_REBARENABLED                1000
#define IDC_HOTLIST_FILE                1001
#define IDC_BROWSE                      1002
#define IDC_MAX_MENU_LENGTH             1003
#define IDC_MIN_TB_SIZE                 1004
#define IDC_MAX_TB_SIZE                 1005
#define IDC_MENU_AUTODETECT             1006
#define IDC_TREE_HOTLIST		1007
#define IDC_STATIC_PROPERTIES		1008
#define IDC_STATIC_NAME			1009
#define IDC_NAME			1010
#define IDC_STATIC_URL			1011
#define IDC_URL				1012
#define IDC_STATIC_CREATED		1013
#define IDC_CREATED			1014
#define IDC_STATIC_VISITED		1015
#define IDC_LAST_VISIT			1016
#define IDC_STATIC_ORDER		1017
#define IDC_ORDER			1018
#define IDC_STATIC_SHORT		1019
#define IDC_SHORT_NAME			1020
#define IDC_STATIC_DESC			1021
#define IDC_DESCRIPTION			1022
#define IDC_SORTORDER_AZ		1023
#define IDC_SORTORDER_ZA		1024
#define IDC_FOLDERFIRST			1025
#define IDC_USEORDER			1026

#define ID__BOOKMARK_DELETE             32791
#define ID__SET_TOOLBAR_FOLDER          32792
#define ID__NEW_FOLDER                  32795
#define ID__NEW_SEPARATOR               32796
#define ID__NEW_BOOKMARK                32797
#define ID__SETAS_TOOLBARFOLDER         32800
#define ID__SETAS_NEWBOOKMARKFOLDER     32802
#define ID__SETAS_BOOKMARKMENU          32803
#define ID__OPEN_BACKGROUND             32814
#define ID__OPEN                        32815

#define TOOLBAND_NAME "Hotlist"
#define TOOLBAND_FAILED_TO_CREATE "Failed to create hotlist toolbar"
#define PLUGIN_NAME "Opera Hotlist Plugin"
#define MENU_TO_COMMAND(x) (x+SUBMENU_OFFSET)

#define IDB_IMAGES                      158

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
void findNick(char *nick, char *url);
LRESULT APIENTRY WndTBSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct hotlistTB {
   HWND hWnd;
   HWND hWndTB;
   struct hotlistTB *next;
};
typedef struct hotlistTB TB;

WHERE TB *root;
WHERE TB *pNewTB;

TB *create_TB(HWND hWnd);
TB *find_TB(HWND hWnd);
void remove_TB(HWND hWnd);
#endif

void Config(HWND parent);


#define _T(x) x
#define PREFERENCE_REBAR_ENABLED _T("kmeleon.plugins.hotlist.rebar")
#define PREFERENCE_HOTLIST_FILE  _T("kmeleon.plugins.hotlist.hotlistFile")
#define PREFERENCE_HOTLIST_RESYNCH  _T("kmeleon.plugins.hotlist.resynch")
#define PREFERENCE_SETTINGS_DIR    _T("kmeleon.general.settingsDir")
#define PREFERENCE_MENU_MAXLEN _T("kmeleon.plugins.hotlist.maxMenuLength")
#define PREFERENCE_MENU_AUTOLEN _T("kmeleon.plugins.hotlist.menuAutoDetect")
#define PREFERENCE_MENU_SORTORDER _T("kmeleon.plugins.hotlist.sortOrder")
#define PREFERENCE_TOOLBAR_FOLDER _T("kmeleon.plugins.hotlist.toolbarFolder")
#define PREFERENCE_BUTTON_MINWIDTH  _T("kmeleon.plugins.hotlist.buttonMinWidth")
#define PREFERENCE_BUTTON_MAXWIDTH  _T("kmeleon.plugins.hotlist.buttonMaxWidth")
#define PREFERENCE_BUTTON_ICONS  _T("kmeleon.plugins.hotlist.buttonIcons")
#define PREFERENCE_EDIT_DLG_LEFT   _T("kmeleon.plugins.hotlist.editdialog.left")
#define PREFERENCE_EDIT_DLG_TOP    _T("kmeleon.plugins.hotlist.editdialog.top")
#define PREFERENCE_EDIT_DLG_WIDTH  _T("kmeleon.plugins.hotlist.editdialog.width")
#define PREFERENCE_EDIT_DLG_HEIGHT _T("kmeleon.plugins.hotlist.editdialog.height")

// The globals

WHERE WNDPROC wpOrigTBWndProc;
WHERE BOOL bRebarEnabled;
WHERE BOOL bResynchHotlist;
WHERE BOOL bCreate;
WHERE BOOL bIgnore;
WHERE CHAR gHotlistFile[MAX_PATH];
WHERE CHAR gToolbarFolder[MAX_PATH];

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

#endif
