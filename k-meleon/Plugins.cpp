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

#include "StdAfx.h"

#include "Plugins.h"
#include "kmeleon_plugin.h"

CPlugins::CPlugins(){
}

CPlugins::~CPlugins(){
  POSITION pos = pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  while (pos){
    pluginList.GetNextAssoc( pos, s, kPlugin);
    if (kPlugin){
      kPlugin->Quit();
      FreeLibrary(kPlugin->hDllInstance);
    }
  }
}

// returns a pointer to the char after the last \ or /
const char *FileNoPath(const char *filepath){
  char *p1 = strrchr(filepath, '\\');
  char *p2 = strrchr(filepath, '/');
  if (p1 > p2){
    return p1 + 1;
  }
  else if (p2 > p1){
    return p2 + 1;
  }
  else{
    return filepath;
  }
}

kmeleonPlugin * CPlugins::Load(const char *file){
  kmeleonPlugin * kPlugin;
  if (pluginList.Lookup(FileNoPath(file), kPlugin)){
    return kPlugin; // it's already loaded
  }

  HINSTANCE plugin = LoadLibrary(file);
  KmeleonPluginGetter kpg = (KmeleonPluginGetter)GetProcAddress(plugin, "GetKmeleonPlugin");

  if (!kpg){
    FreeLibrary(plugin);
    return 0;
  }

  kPlugin = kpg();

  if (!kPlugin){
    FreeLibrary(plugin);
    return 0;
  }

  kPlugin->hParentInstance = AfxGetInstanceHandle();
  kPlugin->hDllInstance = plugin;

  kPlugin->Init();

  pluginList.SetAt(FileNoPath(file), kPlugin);

  return kPlugin;
}

int CPlugins::FindAndLoad(char *pattern = "*.dll"){
  CString filepath;
  CFileFind finder;
  BOOL bWorking = finder.FindFile(pattern);
  int i = 0;
  while (bWorking)
  {
    bWorking = finder.FindNextFile();

    filepath = finder.GetFilePath();
    if ( Load(filepath) ){
      i++;
    }
  }
  return i;
}

void CPlugins::OnCommand(UINT command){
  POSITION pos = pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  while (pos){
    pluginList.GetNextAssoc( pos, s, kPlugin);
    if (kPlugin && kPlugin->OnCommand){
      kPlugin->OnCommand(command);
    }
  }
}

