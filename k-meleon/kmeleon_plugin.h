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
	int version;
	char *description;
	int (*Init)();
	void (*Config)(HWND parent);
	void (*Quit)();
  HGLOBAL (*GetMenu)();
  void (*OnCommand)(UINT command);
	HINSTANCE hParentInstance;
	HINSTANCE hDllInstance;
} kmeleonPlugin;

#define KMEL_PLUGIN_VER 0x10

typedef kmeleonPlugin * (*KmeleonPluginGetter)();

#endif