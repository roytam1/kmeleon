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
   CMap<CString, LPCTSTR, kmeleonPlugin *, kmeleonPlugin *> pluginList;
	int _FindAndLoad(const TCHAR *pattern);

public:
	CPlugins();
	~CPlugins();

   void UnLoadAll();

   BOOL IsLoaded(LPCTSTR pluginName);
   BOOL TestLoad(LPCTSTR file, const char *description);

   int FindAndLoad(const TCHAR *pattern = _T("*.dll"));
   kmeleonPlugin * Load(CString file);
   int  OnUpdate(UINT command);

   long SendMessage(const char *to, const char *from, const char *subject, long data1=0, long data2=0);

   int GetConfigFiles(configFileType *configFiles, int maxFiles);
};

#endif // __PLUGINS_H__