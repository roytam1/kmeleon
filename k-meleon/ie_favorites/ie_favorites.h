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

#ifndef __IE_FAVORITES_H__
#define __IE_FAVORITES_H__

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
#define IDD_EDIT_FAVORITES		103

#define IDC_REBARENABLED                1000
#define IDC_FAVORITES_PATH		1001
#define IDC_BROWSE                      1002
#define IDC_MAX_MENU_LENGTH             1003
#define IDC_MIN_TB_SIZE                 1004
#define IDC_MAX_TB_SIZE                 1005
#define IDC_MENU_AUTODETECT             1006
#define IDC_TREE_FAVORITES		1007
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

#define TOOLBAND_NAME "Favorites"
#define TOOLBAND_TITLE "Links"
#define TOOLBAND_FOLDER "Links"
#define MENU_FOLDER ""
#define NEWITEM_FOLDER ""
#define TOOLBAND_FAILED_TO_CREATE "Failed to create favorites toolbar"
#define PLUGIN_NAME "IE Favorites Plugin"
#define MENU_TO_COMMAND(x) (x+SUBMENU_OFFSET)

#define IDB_IMAGES                      158

#define IMAGE_BLANK         -1
#define IMAGE_FOLDER_CLOSED 0
#define IMAGE_FOLDER_OPEN   1
#define IMAGE_BOOKMARK      2
#define IMAGE_CHEVRON       3
#define IMAGE_FOLDER_SPECIAL_CLOSED 4
#define IMAGE_FOLDER_SPECIAL_OPEN   5


#ifdef WHERE
CBookmarkNode gFavoritesRoot(0, "", "", BOOKMARK_FOLDER, 0);
#else
#define WHERE extern
WHERE CBookmarkNode gFavoritesRoot;
#endif

// The interface

#ifndef COMPILING_RC
int CreateFavorite(CBookmarkNode *newNode);
int GetFavoritesPath(void);
int ReadFavorites(char *szRoot, char *szPath, CBookmarkNode &newFavoritesNode);
void BuildMenu(HMENU menu, CBookmarkNode *node, BOOL isContinuation);
void RebuildMenu();
void BuildRebar(HWND hWndTB);
void RebuildRebarMenu(HWND hWndTB);
int ParseFavorites(char **bmFileBuffer, CBookmarkNode &node);
int SaveFavoritesEntry(FILE *bmFile, CBookmarkNode *node);
int addLink(char *url, char *title);
void findNick(char *nick, char **url);
LRESULT APIENTRY WndTBSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern "C" {
   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis);
}

struct favoritesTB {
   HWND hWnd;
   HWND hWndTB;
   struct favoritesTB *next;
};
typedef struct favoritesTB TB;

WHERE TB *root;
WHERE TB *pNewTB;

TB *create_TB(HWND hWnd);
TB *find_TB(HWND hWnd);
void remove_TB(HWND hWnd);
#endif

void Config(HWND parent);


#define _T(x) x
#define PREFERENCE_FAVORITES_PATH _T("kmeleon.plugins.favorites.directory")
#define PREFERENCE_REBAR_ENABLED _T("kmeleon.plugins.favorites.rebar")
#define PREFERENCE_CHEVRON_ENABLED _T("kmeleon.plugins.favorites.chevron")
#define PREFERENCE_FAVORITES_RESYNCH  _T("kmeleon.plugins.favorites.resynch")
#define PREFERENCE_SETTINGS_DIR    _T("kmeleon.general.settingsDir")
#define PREFERENCE_MENU_MAXLEN _T("kmeleon.plugins.favorites.maxMenuLength")
#define PREFERENCE_MENU_AUTOLEN _T("kmeleon.plugins.favorites.menuAutoDetect")
#define PREFERENCE_MENU_SORTORDER _T("kmeleon.plugins.favorites.sortOrder")
#define PREFERENCE_TOOLBAR_FOLDER _T("kmeleon.plugins.favorites.toolbarFolder")
#define PREFERENCE_MENU_FOLDER _T("kmeleon.plugins.favorites.menuFolder")
#define PREFERENCE_NEWITEM_FOLDER _T("kmeleon.plugins.favorites.newitemFolder")
#define PREFERENCE_BUTTON_MINWIDTH  _T("kmeleon.plugins.favorites.buttonMinWidth")
#define PREFERENCE_BUTTON_MAXWIDTH  _T("kmeleon.plugins.favorites.buttonMaxWidth")
#define PREFERENCE_BUTTON_ICONS  _T("kmeleon.plugins.favorites.buttonIcons")
#define PREFERENCE_EDIT_DLG_LEFT   _T("kmeleon.plugins.favorites.editdialog.left")
#define PREFERENCE_EDIT_DLG_TOP    _T("kmeleon.plugins.favorites.editdialog.top")
#define PREFERENCE_EDIT_DLG_WIDTH  _T("kmeleon.plugins.favorites.editdialog.width")
#define PREFERENCE_EDIT_DLG_HEIGHT _T("kmeleon.plugins.favorites.editdialog.height")
#define PREFERENCE_REBAR_TITLE   _T("kmeleon.plugins.favorites.title")
#define PREFERENCE_FAVORITES_OPENURL _T("kmeleon.plugins.favorites.openurl")

// The globals

WHERE WNDPROC wpOrigTBWndProc;
WHERE BOOL bRebarEnabled;
WHERE BOOL bChevronEnabled;
WHERE BOOL bResynchFavorites;
WHERE BOOL bCreate;
WHERE BOOL bIgnore;
WHERE CHAR gFavoritesPath[MAX_PATH];
WHERE CHAR gToolbarFolder[MAX_PATH];
WHERE CHAR gMenuFolder[MAX_PATH];
WHERE CHAR gNewitemFolder[MAX_PATH];
WHERE CHAR szTitle[MAX_PATH];
WHERE BOOL bTitleSet;

extern kmeleonPlugin kPlugin;

WHERE UINT nConfigCommand;
WHERE UINT nAddCommand;
WHERE UINT nAddLinkCommand;
// WHERE UINT nAddToolbarCommand;
WHERE UINT nEditCommand;
WHERE UINT nFirstFavoriteCommand;
WHERE UINT nDropdownCommand;
WHERE UINT nUpdateTB;
WHERE UINT nFirstFavoritesPosition;
WHERE UINT wm_deferhottrack;

#define FAVORITES_TITLE_LEN INTERNET_MAX_URL_LENGTH
#define FAVORITES_STRING_LEN 2048
WHERE char gFavoritesTitle[FAVORITES_TITLE_LEN];

WHERE HMENU gMenuFavorites;

WHERE BOOL gFavoritesModified;

#define MAX_MENU_LENGTH 9999
WHERE int gMaxMenuLength;
WHERE BOOL gMenuAutoDetect;
WHERE int gMenuSortOrder;

WHERE BOOL bDOS;
WHERE BOOL bEmpty;

WHERE int  nButtonMinWidth;
WHERE int  nButtonMaxWidth;
WHERE BOOL bButtonIcons;
WHERE int nHSize, nHRes;

// WHERE HWND ghWndTB;
WHERE HIMAGELIST gImagelist; // the one and only imagelist...

#endif
