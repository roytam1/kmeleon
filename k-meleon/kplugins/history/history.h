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

#define XP_WIN
#define XPCOM_GLUE
#define MOZILLA_STRICT_API

#ifndef COMPILING_RC
#  include <windows.h>
#  include <commctrl.h>
#  include <stdio.h>
typedef int cmp_t(const char *, const char *, unsigned int);
void quicksort(char *a, size_t n, size_t es, cmp_t *cmp, unsigned int flag);
#  include "../kmeleon_plugin.h"
#endif

#include "resource.h"

#define _Tr(x) kPlugin.kFuncs->Translate(_T(x))

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
void CondenseMenuText(TCHAR *buf, TCHAR *title, int index);
void FindSkinFile( TCHAR *szSkinFile, const TCHAR *filename );
void ClearHistory();

extern int ID_HISTORY_FLAG;
extern int ID_HISTORY;
extern int ID_VIEW_HISTORY;
extern int wm_deferbringtotop;
extern HWND hWndFront;

int CALLBACK ViewProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define PREFERENCE_VIEW_DLG_LEFT   "kmeleon.plugins.history.dialog.left"
#define PREFERENCE_VIEW_DLG_TOP    "kmeleon.plugins.history.dialog.top"
#define PREFERENCE_VIEW_DLG_WIDTH  "kmeleon.plugins.history.dialog.width"
#define PREFERENCE_VIEW_DLG_HEIGHT "kmeleon.plugins.history.dialog.height"
#define PREFERENCE_VIEW_ZOOM       "kmeleon.plugins.history.dialog.zoom"
#define PREFERENCE_VIEW_MAX        "kmeleon.plugins.history.dialog.maximized"
#define PREFERENCE_VIEW_SORT_ORDER "kmeleon.plugins.history.dialog.sortOrder"

extern HWND ghWndView;
extern HIMAGELIST gImagelist;



#define IMAGE_BLANK         -1
#define IMAGE_FOLDER_CLOSED 0
#define IMAGE_FOLDER_OPEN   1
#define IMAGE_BOOKMARK      2
#define IMAGE_CHEVRON       3
#define IMAGE_FOLDER_SPECIAL_CLOSED 4
#define IMAGE_FOLDER_SPECIAL_OPEN   5



#endif
