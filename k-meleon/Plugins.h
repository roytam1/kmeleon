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

// this handles plugin loading/unloading

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "StdAfx.h"

#include <afxtempl.h>

#include "Preferences.h"
#include "kmeleon_plugin.h"

class CPlugins {
  friend CPreferencePagePlugins;

protected:
   CMap<CString, LPCSTR, kmeleonPlugin *, kmeleonPlugin *> pluginList;

public:
	CPlugins();
	~CPlugins();

   void UnLoadAll();

   BOOL TestLoad(const char *file, const char *description);

   int FindAndLoad(const char *pattern = "*.dll");
   kmeleonPlugin * Load(char *file);
   void OnCreate(HWND wnd);
   //LRESULT OnMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
   int  OnUpdate(UINT command);
   void DoRebars(HWND rebarWnd);

   int GetConfigFiles(configFileType *configFiles, int maxFiles);
};

#endif // __PLUGINS_H__