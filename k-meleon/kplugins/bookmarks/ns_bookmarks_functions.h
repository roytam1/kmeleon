/*
*  Copyright (C) 2000 Brian Harris, Mark Liffiton
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
#include <map>
#include "DialogUtils.h"

// functions exposed to kmeleon
int  Load();
void Create(HWND parent);
void Config(HWND parent);
void Close(HWND hWnd);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);
int  DoAccel(char *param);
void SaveBM(const TCHAR *file);
void LoadBM(const TCHAR *file);

// Window Procs
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK EditProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Utils
BOOL BrowseForBookmarks(TCHAR *file);
UINT GetSiteIcon(CBookmarkNode* node);
void InitImageList(HIMAGELIST& imageList);
BOOL RealDeleteMenu(HMENU menu, UINT pos);
void ParseBookmarks(char *bmFileBuffer, CBookmarkNode &node);
void BuildMenu(HMENU menu, CBookmarkNode *node, BOOL isContinuation);
void BuildRebar(HWND hWndTB);
void CopyRebar(HWND hWndNewTB, HWND hWndOldTB);
void Rebuild();
int addLink(const char *url, const char *title, int flag, const char *iconurl = NULL);
void findNick(char *nick, char **url);
char *stristr(const char *String, const char *Pattern);

#define MENU_TO_COMMAND(x) (x+SUBMENU_OFFSET)
#define COMMAND_TO_MENU(x) (HMENU)(x-SUBMENU_OFFSET)

// Global vars

extern HIMAGELIST gImagelist;
extern HANDLE ghMutex;

extern HMENU gMenuBookmarks;
extern HWND ghWndTB;
extern HWND hWndFront;
extern HWND ghWndEdit;
extern std::map<HWND, HWND> gToolbarList;

extern UINT nConfigCommand;
extern UINT nAddCommand;
extern UINT nAddLinkCommand;
extern UINT nAddToolbarCommand;
extern UINT nEditCommand;
extern UINT nDropdownCommand;
extern UINT nAddBookmarkHereCommand;
extern UINT nFirstBookmarkPosition;
extern UINT wm_deferbringtotop;

extern TCHAR gBookmarkFile[];
extern bool gBookmarkDefFile;
extern CHAR gToolbarFolder[];
extern char gBookmarksTitle[];

extern BOOL gBookmarksModified;
extern BOOL gGeneratedByUs;
extern BOOL gToolbarEnabled;
extern int gMaxMenuLength;
extern BOOL gMenuAutoDetect;
extern int gMaxTBSize;

extern CBookmarkNode* gBookmarkRoot;

extern kmeleonPlugin kPlugin;

extern "C" {
   KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis);
}
