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

#ifndef __ACCELPARSER_H__
#define __ACCELPARSER_H__

#include "StdAfx.h"

#include "Parser.h"

#define MAX_ACCEL 127
#define MAX_MOUSE 12

class CAccelParser : public CParser {
protected:
  ACCEL accelerators[MAX_ACCEL];
  int numAccelerators;
  ACCEL mouse[MAX_MOUSE];
  int numMKeys;

  HACCEL accelTable;

  int Parse(char *p);

public:
  CAccelParser();
  CAccelParser(CString &filename);

  ~CAccelParser();

  int Load(CString &filename);

  HACCEL GetTable();

  int CheckMouse(UINT message);
};

#endif // __ACCELPARSER_H__
