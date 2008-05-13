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
#include "KmMenu.h"

class CMenuParser : public CParser{
protected:
	KmMenuService* menusService;
	KmMenu* currentKMenu;
	int opEdit;
	BOOL mAccelText;

public:
	CMenuParser();

	CMenuParser(LPCTSTR filename);

	~CMenuParser();

	int Load(LPCTSTR filename);
	int Parse(char *p);
};

#endif // __MENUPARSER_H__
