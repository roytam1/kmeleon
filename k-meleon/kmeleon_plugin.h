/*
*  Copyright (C) 2000 Brian Harris
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


#include <windows.h>
#include <commctrl.h>

class nsIWebBrowser;
#ifndef __KMELEON_PLUGIN_H__
#define __KMELEON_PLUGIN_H__

#define KMEL_PLUGIN_VER_MAJOR 0x0200
#define KMEL_PLUGIN_VER_MINOR 0x0002
#define KMEL_PLUGIN_VER KMEL_PLUGIN_VER_MAJOR | KMEL_PLUGIN_VER_MINOR

#ifdef KMELEON_PLUGIN_EXPORTS
#define KMELEON_PLUGIN __declspec(dllexport)
#else
#define KMELEON_PLUGIN __declspec(dllimport)
#endif

#define OPEN_NORMAL      0    // valid windowState values in NavigateTo()
#define OPEN_NEW         1
#define OPEN_BACKGROUND  2
#define OPEN_CLONE       16

typedef struct {
   char label[16];
   char file[MAX_PATH];
   char helpUrl[MAX_PATH];
} configFileType;

typedef struct {
   char *title;
   char *url;
   int idxIcon;
} kmeleonDocInfo;

typedef struct {
   char *image;
   char *link;
   char *frame;
   char *page;
} kmeleonPointInfo;

enum PREFTYPE {
   PREF_BOOL,
   PREF_INT,
   PREF_STRING,
   PREF_UNISTRING
};

#ifdef _UNICODE
#define PREF_TSTRING PREF_UNISTRING
#else
#define PREF_TSTRING PREF_STRING
#endif

struct kmeleonPlugin;

typedef  HWND(*SideBarInitProc)(HWND);


typedef struct {
   long (*SendMessage)(const char *to, const char *from, const char *subject, long data1, long data2);

   // this function allocates <num> successive ids for the plugin, then returns the first one.
   // use it to get an unused command id.  this way plugins won't step on others toes.
   UINT (*GetCommandIDs)(int num);

   // changing windowstate will open the url in a current, new, or background window
   void (*_NavigateTo)(const char *url, int windowState, HWND mainWnd/*=NULL*/);

   kmeleonDocInfo * (*GetDocInfo)(HWND mainWnd);

   // gets the preference, stores it in ret
   void (*_GetPreference)(enum PREFTYPE type, char *preference, void *ret, void *defaultVal);
   // sets the preference
   void (*SetPreference)(enum PREFTYPE type, const char *preference, void *val, BOOL update /*= FALSE*/);

   // sets the status bar text
   void (*SetStatusBarText)(const char *s);

   int (*GetMozillaSessionHistory)(char **titles[], char **urls[], int *count, int *index);
	void (*GotoHistoryIndex)(UINT index);

   // Register a rebar band
   void (*RegisterBand) (HWND hWnd, char *name, int visibleOnMenu /*= true*/);

   /*
   CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | //CCS_ADJUSTABLE |
   TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS

   0x00000040
   0x00000008
   0x00000004
   0x00000800
   0x00008000
   0x00001000
   0x00000100
   ----------
   0x0000994F
   */

   HWND (*CreateToolbar) (HWND parentWnd, UINT style/* = 0x0000994F*/);


   // get the value of an identifier, eg "ID_NAV_BACK"
   int (*GetID) (char *strID);


   // get the link/image/frame at a point
   kmeleonPointInfo *(*GetInfoAtPoint) (int x, int y);

   int (* CommandAtPoint) (int command, WORD x, WORD y);

   BOOL (*GetGlobalVar) (enum PREFTYPE type, char *preference, void *ret);

   char * (*EncodeUTF8) (const char *str);
   char * (*DecodeUTF8) (const char *str);

   void (*GetBrowserviewRect) (HWND mainWnd, RECT *rc);

   HMENU (*GetMenu) (char *menu);

   void (*SetForceCharset)(char *aCharset);

   void (*SetCheck)(int id, BOOL mark/*=TRUE*/);

   struct kmeleonPlugin * (*Load)(char *kplugin);

   void (*ClearCache)(int cache/*=0*/); /* STORE_ANYWHERE (nsICache.h) */

   void (*BroadcastMessage)(UINT Msg, WPARAM wParam, LPARAM lParam);

   void (*ParseAccel)(char *str);

   void (*DelPreference)(char *preference);

   long (*GetPreference)(enum PREFTYPE type, const char *preference, void *ret, void *defaultVal);

// ----------------------------------------------------
// Addition in K-meleon 1.0

   int (*RegisterSideBar) (HWND parentWnd, TCHAR* name, SideBarInitProc proc, int commandID, int visibleOnMenu);
   void (*ToggleSideBar) (HWND parentWnd, int index);

   int (*TranslateEx)(const char* originalText, TCHAR* translatedText, int bufferlen, BOOL forMenu);
	
   HIMAGELIST (*GetIconList)();

   /* Is it possible to get it otherwise ? */
   BOOL (*GetMozillaWebBrowser)(HWND hWnd, nsIWebBrowser** webBrowser);

   void (*AddStatusBarIcon)(HWND hWnd, int id, HICON hIcon, char* tpText);
   void (*RemoveStatusBarIcon)(HWND hWnd, int id);
   
   BOOL (*InjectJS)(const char*, bool, HWND);
   BOOL (*InjectCSS)(const char*, bool, HWND);

   kmeleonPointInfo *(*GetInfoAtClick) (HWND);

   /* Return version: 
        high byte = major, following bytes are minor version, subminor 
        version and build number for this version.
   */
   int (*GetKmeleonVersion)();

   void (*reserved)();

   HWND (*NavigateTo)(const char *url, int windowState, HWND mainWnd/*=NULL*/);

   const TCHAR* (*Translate) (const char* text); 
   int (*SetGlobalVar)(PREFTYPE, const char*, void*);

} kmeleonFunctions;

/*

typedef struct {
   int   (*Init)    ();
   void  (*Create)  (HWND parent);
   void  (*Config)  (HWND parent);
	void  (*Quit)    ();
   void  (*DoMenu)  (HMENU menu, char *param);
   void  (*DoRebar) (HWND rebarWnd);
   int   (*DoAccel) (char *param);
   int   (*GetConfigFiles)(configFileType **configFiles);
} pluginFunctions;

*/

typedef struct kmeleonPlugin {
  // Filled in by the plugin
	int version;
	char *description;

   long  (*DoMessage)(const char *to, const char *from, const char *subject, long data1, long data2);

  // Filled in by k-meleon
	HINSTANCE hParentInstance;
	HINSTANCE hDllInstance;

   kmeleonFunctions *kFuncs;

   char *dllname;
   BOOL loaded;

} kmeleonPlugin;

typedef kmeleonPlugin * (*KmeleonPluginGetter)();

#endif
