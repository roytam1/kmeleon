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

typedef struct {
  // Filled in by the plugin
	int version;
	char *description;
	int (*Init)();
	void (*Config)(HWND parent);
	void (*Quit)();
  void (*DoMenu)(HMENU menu, char *param);
  void (*OnCommand)(UINT command);
  // return false to disable the item
  //int (*CommandUpdate)(UINT command);
  void (*DoRebar)(HWND rebarWnd);

  // Filled in by k-meleon
  // this function allocates <num> successive ids for the plugin, then returns the first one.
  // use it to get an unused command id.  this way plugins won't step on others toes.
  UINT (*GetCommandIDs)(int num);
  // if newWindow, open in a new window, otherwise use the current window
  void (*NavigateTo)(char *url, int newWindow);

	HINSTANCE hParentInstance;
	HINSTANCE hDllInstance;
} kmeleonPlugin;

#define KMEL_PLUGIN_VER 0x10

typedef kmeleonPlugin * (*KmeleonPluginGetter)();

#endif