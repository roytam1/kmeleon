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

#ifndef __KMELEON_PLUGIN_H__
#define __KMELEON_PLUGIN_H__

#ifdef KMELEON_PLUGIN_EXPORTS
#define KMELEON_PLUGIN __declspec(dllexport)
#else
#define KMELEON_PLUGIN __declspec(dllimport)
#endif

#define OPEN_NORMAL      0    // valid windowState values in NavigateTo()
#define OPEN_NEW         1
#define OPEN_BACKGROUND  2

typedef struct {
   char label[16];
   char file[MAX_PATH];
   char helpUrl[MAX_PATH];
} configFileType;

typedef struct {
   char *title;
   char *url;
} kmeleonDocInfo;

enum PREFTYPE {
   PREF_BOOL,
   PREF_INT,
   PREF_STRING
};

typedef struct {
   // this function allocates <num> successive ids for the plugin, then returns the first one.
   // use it to get an unused command id.  this way plugins won't step on others toes.
   UINT (*GetCommandIDs)(int num);

   // changing windowstate will open the url in a current, new, or background window
   void (*NavigateTo)(char *url, int windowState);

   kmeleonDocInfo * (*GetDocInfo)(HWND mainWnd);

   // gets the preference, stores it in ret
   void (*GetPreference)(enum PREFTYPE type, char *preference, void *ret, void *defaultVal);
   // sets the preference
   void (*SetPreference)(enum PREFTYPE type, char *preference, void *val);

   int (*GetMozillaSessionHistory)(char **titles[], int *count, int *index);
	void (*GotoHistoryIndex)(UINT index);

   // Register a rebar band
   void (*RegisterBand) (HWND hWnd, char *name, int visibleOnMenu = true);
} kmeleonFunctions;

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

typedef struct {
  // Filled in by the plugin
	int version;
	char *description;

   pluginFunctions *pf;

  // Filled in by k-meleon
	HINSTANCE hParentInstance;
	HINSTANCE hDllInstance;

   kmeleonFunctions *kf;
   char *dllname;
   BOOL loaded;

} kmeleonPlugin;

#define KMEL_PLUGIN_VER 0x10

typedef kmeleonPlugin * (*KmeleonPluginGetter)();

#endif