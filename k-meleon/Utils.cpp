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

#include "string.h"

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

void TrimWhiteSpace(char *string){
  char *p;
  for ( p = string + strlen(string) - 1; p >= string; p-- ){
    if (*p == ' ' || *p == '\t'){
      *p = 0;
    }else{
      break;
    }
  }
}

char *SkipWhiteSpace(char *string){
  char *p;
  for (p = string; *p; p++){
    if (*p != ' ' && *p != '\t'){
      return p;
    }
  }
  return string;
}

void FreeStringArray(char *array[], int size) {
	int i;
	for (i = 0; i < size; i++)
		delete (array[i]);
	delete (array);
}

void CondenseMenuText(char *buf, char *title, int index) {
	int len;

	if ( (index >= 0) && (index <10) ) {
		buf[0] = '&';
		buf[1] = index +48; // convert int to ascii
		buf[2] = ' ';
	}
	else
		memcpy(buf, "   ", 3);

	len = strlen(title);
	if (len > 43)  {
		memcpy(buf+3, title, 20);
		memcpy(buf+23, "...", 3);
		strcpy(buf+26, title+len-21);
	}
	else
		strcpy(buf+3, title);
}