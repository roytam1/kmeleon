/*
*  Copyright (C) 2001 Jeff Doozan
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

#ifndef __HISTORY_H__
#define __HISTORY_H__

#ifndef COMPILING_RC
#  include <windows.h>
#  include <commctrl.h>
#  include <stdio.h>
typedef int cmp_t(const char *, const char *, unsigned int);
void quicksort(char *a, size_t n, size_t es, cmp_t *cmp, unsigned int flag);
#  include "../kmeleon_plugin.h"
#endif

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd, char *name);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void CreateBackMenu (HWND hWndParent, UINT button);
void CreateForwardMenu (HWND hWndParent, UINT button);
void UpdateHistoryMenu(HWND hWndParent);
void CondenseMenuText(char *buf, char *title, int index);
void FindSkinFile( char *szSkinFile, char *filename );

extern int ID_HISTORY_FLAG;
extern int ID_HISTORY;
extern int ID_VIEW_HISTORY;
extern int wm_deferbringtotop;
extern HWND hWndFront;

int CALLBACK ViewProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define PREFERENCE_VIEW_DLG_LEFT   TEXT("kmeleon.plugins.history.dialog.left")
#define PREFERENCE_VIEW_DLG_TOP    TEXT("kmeleon.plugins.history.dialog.top")
#define PREFERENCE_VIEW_DLG_WIDTH  TEXT("kmeleon.plugins.history.dialog.width")
#define PREFERENCE_VIEW_DLG_HEIGHT TEXT("kmeleon.plugins.history.dialog.height")
#define PREFERENCE_VIEW_ZOOM       TEXT("kmeleon.plugins.history.dialog.zoom")
#define PREFERENCE_VIEW_MAX        TEXT("kmeleon.plugins.history.dialog.maximized")
#define PREFERENCE_VIEW_SORT_ORDER TEXT("kmeleon.plugins.history.dialog.sortOrder")

extern HWND ghWndView;
extern HIMAGELIST gImagelist;

#define IDD_VIEW_HISTORY		700
#define IDD_CONFIG                      701
#define IDR_CONTEXTMENU                 759

#define IDC_REBARENABLED                1000
#define IDC_HISTORY_FILE                1001
#define IDC_BROWSE                      1002
#define IDC_MAX_MENU_LENGTH             1003
#define IDC_MIN_TB_SIZE                 1004
#define IDC_MAX_TB_SIZE                 1005
#define IDC_MENU_AUTODETECT             1006
#define IDC_TREE_HOTLIST		1007
#define IDC_STATIC_PROPERTIES		1008
#define IDC_STATIC_URL			1011
#define IDC_URL				1012
#define IDC_STATIC_VISITED		1015
#define IDC_LAST_VISIT			1016
#define IDC_EXPIRE_DAYS                 1017
#define IDB_CLEAR                       1018

#define IDB_IMAGES                      159
#define IDB_ICON                        160

#define IMAGE_BLANK         -1
#define IMAGE_FOLDER_CLOSED 0
#define IMAGE_FOLDER_OPEN   1
#define IMAGE_BOOKMARK      2
#define IMAGE_CHEVRON       3
#define IMAGE_FOLDER_SPECIAL_CLOSED 4
#define IMAGE_FOLDER_SPECIAL_OPEN   5

#define ID__BOOKMARK_DELETE             32791
#define ID__OPEN_BACKGROUND             32814
#define ID__OPEN                        32815
#define ID__ZOOM                        32819
#define ID__SORT_DATE                   32820
#define ID__SORT_URL                    32821

#endif
