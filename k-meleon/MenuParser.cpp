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

#include "StdAfx.h"
#include <afxtempl.h>

#include "Kmeleon.h"
#include "Plugins.h"

#include "MenuParser.h"

CMenuParser::CMenuParser(){
}

CMenuParser::CMenuParser(CString &filename){
	Load(filename);
}

CMenuParser::~CMenuParser(){
  POSITION pos = menus.GetStartPosition();
  CMenu *m;
  CString s;
  while (pos){
    menus.GetNextAssoc( pos, s, m);
    if (m){
      m->DestroyMenu();
    }
  }
}

void TranslateTabs(char *buffer){
  char *p;
  for (p=buffer; *p; p++){
    if (*p == '\\'){
      if (*(p+1) == 't'){
        *p = ' ';
        *(p+1) = '\t';
      }
    }
  }
}

#define ERROR_BOX_1(msg, var1) \
   { char err[255]; \
     sprintf(err, msg, var1); \
     MessageBox(NULL, err, "Error", 0); \
   }

#define ERROR_BOX_2(msg, var1, var2) \
   { char err[255]; \
     sprintf(err, msg, var1, var2); \
     MessageBox(NULL, err, "Error", 0); \
   }

int CMenuParser::Load(CString &filename){
  TRY {
    menuFile = new CFile(filename, CFile::modeRead);
  }
  CATCH (CFileException, e) {
    menuFile = NULL;
    return 0;
  }
  END_CATCH

  long length = menuFile->GetLength();
  char *buffer = new char[length + 1];
  menuFile->Read(buffer, length);
  // force the terminating 0
  buffer[length] = 0;
  
  menuFile->Close();
  delete menuFile;
  menuFile = NULL;

  TranslateTabs(buffer);

  CMap<CString, LPCSTR, CMenu *, CMenu *&> menuMap;

  char *currentMenuName = NULL;
  CMenu *currentMenu = NULL;

  char *p = strtok(buffer, "\r\n");
  while (p){
    if (*p == '#'){
    }
    else if (strnicmp(p, _T("MenuDef"), 7) == 0){
      p+=8;
      currentMenuName = NULL;
      currentMenu = new CMenu();

      if (strstr(p, _T("Popup"))){
        currentMenu->CreatePopupMenu();
      }else{
        currentMenu->CreateMenu();
      }
      menus[p] = currentMenu;
    }
    else if (strnicmp(p, _T(":EndMenu"), 8) == 0){
    }
    else if (strnicmp(p, _T("Popup "), 6) == 0){
      if (currentMenuName || currentMenu){
        MessageBox(NULL, "Found Popup before :EndPopup", "", 0);
      }
      else{
        p += 6; // jump past "Popup "
        currentMenuName = p;
        currentMenu = new CMenu();
        currentMenu->CreatePopupMenu();
      }
    }
    else if (strnicmp(p, _T(":EndPopup"), 9) == 0){
      menuMap[CString(currentMenuName)] = currentMenu;
      currentMenu = NULL;
      currentMenuName = NULL;
    }
    else if (strnicmp(p, _T(":Popup "), 6) == 0){
      p += 7;
      CMenu *popup = NULL;
      menuMap.Lookup(CString(p), popup);
      if (popup){
        currentMenu->AppendMenu(MF_POPUP | MF_STRING, (UINT)popup->m_hMenu, p);
      }
    }
    else if (strnicmp(p, _T(":Seperator"), 10) == 0){
      currentMenu->AppendMenu(MF_SEPARATOR);
    }
    else if (strnicmp(p, _T(":Plugin "), 8) == 0){
      p+=8;
      char *e = strrchr(p, '=');
      if (e){
        *e = 0;
        e++;
        kmeleonPlugin * kPlugin = theApp.plugins.Load(e);
        if (kPlugin) {
          if (kPlugin->DoMenu){
            kPlugin->DoMenu(currentMenu->GetSafeHmenu(), p);
          }else{
            ERROR_BOX_1( "plugin %s has no menu", e);
          }
        }else{
          ERROR_BOX_1( "Could not load plugin %s", e )
        }
      }
    }
    else if (*p == ':'){
      // unknown command
    }
    else {
      if (!currentMenu){
        MessageBox(NULL, "Found Menu item with no menu!", "", 0);
      }
      else {
        char *e = strrchr(p, '=');
        if (e){
          *e = 0;
          e++;
          currentMenu->AppendMenu(MF_STRING, atoi(e), p);
        }
      }
    }
    p = strtok(NULL, "\r\n");
  };

  POSITION pos = menuMap.GetStartPosition();
  CMenu *m;
  CString s;
  while (pos){
    menuMap.GetNextAssoc( pos, s, m);
    if (m){
      m->Detach();
      m->DestroyMenu();
    }
  }

  delete [] buffer;

  return 1;
}

CMenu *CMenuParser::GetMenu(char *menuName){
  CMenu *menu;
  if (!menus.Lookup(menuName, menu)){
    return NULL;
  }
	return menu;
}
