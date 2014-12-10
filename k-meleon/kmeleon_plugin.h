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

#pragma once

#include <windows.h>
#include <commctrl.h>

#ifdef CallWindowProc
#undef CallWindowProc
#endif

#ifdef SetWindowLong
#undef SetWindowLong
#endif

#define CallWindowProc(proc, hWnd, message, wParam, lParam) \
	(IsWindowUnicode(hWnd) ? CallWindowProcW(proc, hWnd, message, wParam, lParam) : \
	                        CallWindowProcA(proc, hWnd, message, wParam, lParam))

#define SetWindowLong(hWnd, nIndex, dwNewLong) \
	(IsWindowUnicode(hWnd) ? SetWindowLongW(hWnd, nIndex, dwNewLong) : \
                            SetWindowLongA(hWnd, nIndex, dwNewLong))

class nsIWebBrowser;
#ifndef __KMELEON_PLUGIN_H__
#define __KMELEON_PLUGIN_H__

#define KMEL_PLUGIN_VER_MAJOR 0x0200
#define KMEL_PLUGIN_VER_MINOR 0x0004
#define KMEL_PLUGIN_VER_MINOR_UTF8 0x0010
#define KMEL_PLUGIN_VER KMEL_PLUGIN_VER_MAJOR | KMEL_PLUGIN_VER_MINOR
#define KMEL_PLUGIN_VER_UTF8 KMEL_PLUGIN_VER_MAJOR | KMEL_PLUGIN_VER_MINOR_UTF8

#ifdef KMELEON_PLUGIN_EXPORTS
#define KMELEON_PLUGIN __declspec(dllexport)
#else
#define KMELEON_PLUGIN __declspec(dllimport)
#endif

// windowState values in NavigateTo()
#define OPEN_NORMAL         0    
#define OPEN_NEW            1
#define OPEN_BACKGROUND     2
#define OPEN_NEWTAB         3
#define OPEN_BACKGROUNDTAB  4
#define OPEN_CLONE         16

// folderType value in GetFolder()
enum FolderType {
   RootFolder,
   DefSettingsFolder,
   UserSettingsFolder,
   ProfileFolder,
   PluginsFolder,
   UserPluginsFolder,
   SkinsFolder,
   UserSkinsFolder,
   ResFolder,
   CurrentSkinFolder,
   AppFolder
};

enum WindowVarType {
	Window_UrlBar = 0,      // char*
	Window_Charset = 1,     // char*
	Window_Title = 2,       // char*
	Window_TextZoom = 3,    // int
	Window_URL = 4,         // char*
	Window_Number = 10,     // int
	Window_Tab_Number = 11, // int
	Window_Tab_Index = 12, // int
	
	// Read Only
	Window_SelectedText = 100, // wchar_t*
	Window_LinkURL = 101,      // char*
	Window_ImageURL = 102,     // char*
	Window_FrameURL = 103,     // char*
	Window_LinkTitle = 104,    // char*
	Window_Icon = 110,         // int

	Search_URL = 120 // char*
};

typedef struct configFileType {
   char label[16];
   char file[MAX_PATH];
   char helpUrl[MAX_PATH];
} configFileType;

typedef struct {
   char *title;
   char *url;
   char *iconurl;
   int idxIcon;
} kmeleonDocInfo;

typedef struct {
   char *image;
   char *link;
   char *frame;
   char *page;
   char *linktitle;
   bool isInput;
} kmeleonPointInfo;

typedef struct _AutoCompleteResult {
	char* value;
	char* comment;
	int score;
} AutoCompleteResult;

typedef struct _kmeleonCommand {
	UINT id;
	char cmd[80];
	char desc[256];
} kmeleonCommand;


enum PREFTYPE {
   PREF_BOOL,
   PREF_INT,
   PREF_STRING,
   PREF_UNISTRING,
   PREF_LOCALIZED
};

enum LogFlags {
	errorFlag = 0U,
	warningFlag = 1U ,
	exceptionFlag = 2U,
	strictFlag = 4U
};

typedef struct {
   short type; // One of MENUTYPE
   const char* label; // If either label or command is null then 
   int command;       // this will be a delete operation
   int groupid; // Not used
   long before; // Can be either a int position, a command id or a pointer to a string
} kmeleonMenuItem;

typedef struct {
	const char* name;
	const char* label;
	const char* tooltip;
	const char* action;
	const char* menu;
	const char* hotimage;
	const char* coldimage;
	const char* deadimage;
	int enabled;
	int checked;
	int id;
	int before;	
	unsigned iconWidth, iconHeight; 
} kmeleonButton;

enum MENUTYPE {
   MENU_COMMAND = 0,
   MENU_POPUP = 1,
   MENU_INLINE = 2,
   MENU_PLUGIN = 3,
   MENU_SEPARATOR= 4
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

   int (*_GetMozillaSessionHistory)(char **titles[], char **urls[], int *count, int *index);
   void (*_GotoHistoryIndex)(UINT index);

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
   int (*GetID) (const char *strID);


   // get the link/image/frame at a point
   kmeleonPointInfo *(*GetInfoAtPoint) (int x, int y);

   int (* CommandAtPoint) (int command, WORD x, WORD y);

   BOOL (*GetGlobalVar) (enum PREFTYPE type, char *preference, void *ret);

   char * (*EncodeUTF8) (const char *str);
   char * (*DecodeUTF8) (const char *str);

   void (*GetBrowserviewRect) (HWND mainWnd, RECT *rc);

   HMENU (*GetMenu) (char *menu);

   void (*SetForceCharset)(const char *aCharset);

   void (*SetCheck)(int id, BOOL mark/*=TRUE*/);

   struct kmeleonPlugin * (*Load)(const char *kplugin);

   void (*ClearCache)(int cache/*=0*/); /* STORE_ANYWHERE (nsICache.h) */

   void (*BroadcastMessage)(UINT Msg, WPARAM wParam, LPARAM lParam);

   void (*ParseAccel)(char *str);

   void (*DelPreference)(const char *preference);

   long (*GetPreference)(enum PREFTYPE type, const char *preference, void *ret, void *defaultVal);

// ----------------------------------------------------
// Addition in K-meleon 1.0

   int (*RegisterSideBar) (HWND parentWnd, TCHAR* name, SideBarInitProc proc, int commandID, int visibleOnMenu);
   void (*ToggleSideBar) (HWND parentWnd, int index);

   int (*TranslateEx)(const char* originalText, TCHAR* translatedText, int bufferlen, BOOL forMenu);
	
   HIMAGELIST (*GetIconList)();

   /* Is it possible to get it otherwise ? */
   BOOL (*GetMozillaWebBrowser)(HWND hWnd, nsIWebBrowser** webBrowser);

   /* Prior to kmeleon 2.1 (75) the icon was destroyed after remove */
   void (*AddStatusBarIcon)(HWND hWnd, int id, HICON hIcon, const char* tpText);
   void (*RemoveStatusBarIcon)(HWND hWnd, int id);
   
   BOOL (*InjectJS)(const char*, int, HWND);
   BOOL (*InjectCSS)(const char*, BOOL, HWND);

   kmeleonPointInfo *(*GetInfoAtClick) (HWND);

   /* Return version: 
        high byte = major, following bytes are minor version, subminor 
        version and build number for this version.
   */
   int (*GetKmeleonVersion)();

   void (*reserved)();

   HWND (*NavigateTo)(const char *url, int windowState, HWND mainWnd/*=NULL*/);

   const char* (*Translate) (const char* text); 
   int (*SetGlobalVar)(PREFTYPE, const char*, void*);
 
// ----------------------------------------------------
// Addition in K-meleon 1.1

	long (*GetFolder)(FolderType type, char* path, size_t size);

   /* Set an accelerator:
      key: key to use (Ex: "CTRL ALT X")
      command: id or plugin command. If NULL, delete the accel if it exists
   */
   void (*SetAccel)(const char *key, const char* command);

   /* Edit a menu

      name : name of the menu to add/edit
      item : item to add, the values depend of the type of the menu:
   
      type ==> MENU_COMMAND
         label ==> Text of the menu item
         command ==> id of the command

      type ==> MENU_POPUP
         type ==> MENU_INLINE
         label ==> Name of the popup/inline menu to add

      type ==> MENU_PLUGIN
         label ==> Plugin command

      group ==> not used for now;
      before ==> id command or pointer to a label to indicate
                 where to add the item. -1 for end of menu.
                 Don't work with type = MENU_PLUGIN.
   */
   void (*SetMenu)(const char* name, kmeleonMenuItem* item);

   /* Rebuild the menu after editing */
   void (*RebuildMenu) (const char* name);
	
	UINT (*GetWindowVar) (HWND, WindowVarType, void*);
	BOOL (*SetWindowVar) (HWND, WindowVarType, void*);
	int (*GetMozillaSessionHistory) (HWND hWnd, char ***titles, char ***urls, int *count, int *index);
	int (*SetMozillaSessionHistory) (HWND hWnd, const char **titles, const char **urls, int count, int index);

// ----------------------------------------------------
// Addition in K-meleon 1.5

	int (*GetWindowsList) (HWND* list, unsigned size);
	int (*GetTabsList) (HWND hWnd, HWND* list, unsigned size);
	UINT (*GetIconIdx) (const char* host);
	void (*ReleaseCmdId) (UINT id);
	UINT (*RegisterCmd) (const char* name, const char* desc, const char* arg); // Do not use yet
	void (*UnregisterCmd) (const char* name, const char* plugin);
	unsigned (*GetCmdList) (kmeleonCommand*, unsigned size);
	BOOL (*LoadCSS) (const char* path, BOOL load);
	BOOL (*LogMessage) (const char* category, const char* message, const char* file, UINT line, UINT flags);
	
// ----------------------------------------------------
// Addition in K-meleon 1.6

	BOOL (*InjectJS2)(const char*, int, char* result, unsigned size, HWND);

// ----------------------------------------------------
// Addition in K-meleon 75 (2.1)
	
	bool (*AddToolbar)(const char*, UINT, UINT);
	bool (*AddButton)(const char*, kmeleonButton*);
	bool (*GetButton)(const char*, UINT, kmeleonButton*);
	bool (*SetButton)(const char*, UINT, kmeleonButton*);
	HIMAGELIST (*GetCmdIconList)();
	int (*GetCmdIcon)(UINT id);
	bool (*FindSkinFile)(const wchar_t* name, wchar_t* result, unsigned size);
	void (*GotoHistoryIndex)(HWND hWnd, UINT index);
	bool (*RemoveButton)(const char* name, const char* command);

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
