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

#define IDD_CONFIG                      101
#define IDC_REBARENABLED                1000
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



// The interface

#ifndef COMPILING_RC
int op_readFile(char *);
int op_writeFile(char *);
void BuildMenu(HMENU menu, CBookmarkNode *node, BOOL isContinuation);
void BuildRebar(HWND hWndTB);
int ParseHotlist(char **bmFileBuffer, CBookmarkNode &node);
int SaveHotlistEntry(FILE *bmFile, CBookmarkNode *node);
int addLink(char *url, char *title);
void findNick(char *nick, char *url);
#endif

void Config(HWND parent);


#define _T(x) x
#define PREFERENCE_REBAR_ENABLED _T("kmeleon.plugins.hotlist.rebar")
#define PREFERENCE_HOTLIST_FILE  _T("kmeleon.plugins.hotlist.hotlistFile")
#define PREFERENCE_HOTLIST_RESYNCH  _T("kmeleon.plugins.hotlist.resynch")
#define PREFERENCE_SETTINGS_DIR    "kmeleon.general.settingsDir"
#define PREFERENCE_MENU_MAXLEN _T("kmeleon.plugins.hotlist.maxMenuLength")
#define PREFERENCE_MENU_AUTOLEN _T("kmeleon.plugins.hotlist.menuAutoDetect")
#define PREFERENCE_MENU_SORTORDER _T("kmeleon.plugins.hotlist.sortOrder")
#define PREFERENCE_TOOLBAR_FOLDER _T("kmeleon.plugins.hotlist.toolbarFolder")


// The globals

#ifdef WHERE
CBookmarkNode gHotlistRoot(0, "", "", "", "", BOOKMARK_FOLDER, 0);
#else
#define WHERE extern
WHERE CBookmarkNode gHotlistRoot;
#endif

WHERE BOOL bRebarEnabled;
WHERE BOOL bResynchHotlist;
WHERE BOOL bCreate;
WHERE CHAR gHotlistFile[MAX_PATH];
WHERE CHAR gToolbarFolder[MAX_PATH];

extern kmeleonPlugin kPlugin;

WHERE UINT nConfigCommand;
WHERE UINT nAddCommand;
WHERE UINT nAddLinkCommand;
// WHERE UINT nAddToolbarCommand;
// WHERE UINT nEditCommand;
WHERE UINT nDropdownCommand;
WHERE UINT nFirstHotlistPosition;

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

WHERE HWND ghWndTB;
WHERE HIMAGELIST gImagelist; // the one and only imagelist...
#endif
