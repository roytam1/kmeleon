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

#include <stdio.h>
#include <string.h>
#include <tchar.h>

void TranslateTabs(char *buffer);
void TrimWhiteSpace(char *string);
char *SkipWhiteSpace(char *string);
int CondenseString(char *buf, int size);
char *fixString(const char *inString, int size);
long FileSize(FILE *file);
char* nsUnescape(char * str);
void MakeFilename(TCHAR* name);
