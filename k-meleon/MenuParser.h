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

#ifndef __MENUPARSER_H__
#define __MENUPARSER_H__

#include "StdAfx.h"

#include "Parser.h"

enum MenuType 
{
	MenuString = 0,
	MenuPopup = 1,
	MenuInline = 2,
	MenuPlugin = 3,
	MenuSeparator = 4,
	MenuSpecial = 5
};

struct MenuItem
{
	MenuType type;
	char label[80];
	int command;
	int groupid;

	void SetLabel(const char* psz) {
		strncpy(label, psz, 80);
		label[79] = 0;
	}

};

class KMenu {
public:

	CMenu menu;
	CList<MenuItem, MenuItem&> menuDef;
	CList<KMenu*, KMenu*> dependencies;

	void RemoveItem(MenuItem& item);
	void AddItem(MenuItem& item, long before = -1);
};

class CMenuParser : public CParser{
protected:
	//CMap<CString, LPCTSTR, CMenu *, CMenu *&> menus;
	CMap<CMenu *, CMenu *&, int, int&> menuOffsets;
	CMap<CString, LPCTSTR, KMenu*, KMenu*> menus2;
	KMenu* currentKMenu;

	int opEdit;
	BOOL mAccelText;

public:
	CMenuParser();
	CMenuParser(LPCTSTR filename);

	~CMenuParser();

	int Load(LPCTSTR filename);
	int Parse(char *p);
	void SetMenu(LPCTSTR menu, MenuItem item, long before = -1);
	BOOL BuildMenu(CMenu &menu, CList<MenuItem, MenuItem&> &menuDef, int before = -1);
	void InsertItem(CMenu &menu, MenuItem item, int before = -1);
	void Rebuild(LPCTSTR menu);

	void Destroy();

	CMenu *GetMenu(LPCTSTR menuName);
	int GetOffset(CMenu *menu);

	void SetCheck(UINT id, BOOL checked = TRUE);

private:
	void ClearSeparators(KMenu* menu);
	inline void ResetMenu(CMenu& menu);

};

#endif // __MENUPARSER_H__
