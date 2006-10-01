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

// Just a trick for the plugins using it

#include "Utils.h"
#include <ctype.h>

void MakeFilename(TCHAR* name)
{
	TCHAR *p;
	for (p=name; *p; p++)
	{
		if (*p==_T('\\') ||
			*p==_T('/') ||
			*p==_T('<') ||
			*p==_T('>') ||
			*p==_T(':') ||
			*p==_T('?') ||
			*p==_T('|') ||
			*p==_T('*') ||
			*p==_T('"') )
		*p = _T('_');
	}

	while ( p = _tcsstr(name, _T(" _"))) {
		_tcscpy(p, p+1);
		*p = _T(' ');
	}

	while ( p = _tcsstr(name, _T("  ")))
		_tcscpy(p, p+1);

	while ( p = _tcsstr(name, _T("...")))
		_tcscpy(p, p+2);
}

void TranslateTabs(char *buffer)
{
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

void TrimWhiteSpace(char *string)
{
  char *p;
  for ( p = string + strlen(string) - 1; p >= string; p-- ){
    if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'){
      *p = 0;
    }else{
      break;
    }
  }
}

char *SkipWhiteSpace(char *string)
{
  char *p;
  for (p = string; *p; p++){
    if (*p != ' ' && *p != '\t'){
      return p;
    }
  }
  return string;
}


//  Remove duplicate tabs and spaces
//  compress other characters into 'size' string
//  Ex  "This   is  a test", 15 = "This i...a test"
//  If size is 0, just remove duplicate tabs and spaces
//  returns the length of the modified string
//  note, this modifies the string passed to it, so make
//  a copy if you need to reference the original

int CondenseString(TCHAR *buf, int size)
{
	int firstlen, secondlen, len;
   TCHAR *read, *write;

   for (write=read=buf; *read; read++,write++) {    // condense tabs and spaces
      if ( isspace((unsigned char)(*read)) ) {
          *write = *read;
		  while (*(read+1) && isspace((unsigned char)(*(read+1)))) ++read;
	  }
	  else
		*write = *read;
   }
   *write = 0;                         // null terminator

   len = _tcslen(buf);
   if ((size == 0) || (len < size))
      return len;

   memcpy(buf+size-3, _T("..."), 3*sizeof(TCHAR));	
   buf[size] = 0;
   return _tcslen(buf);

   if (size%2) { // if even
      firstlen = ((size +1) - 3) / 2;
      secondlen= firstlen-1;
   }
   else {
      firstlen = (size - 3) / 2;
      secondlen= firstlen;
   }

   memcpy(buf+firstlen, _T("..."), 3*sizeof(TCHAR));
   _tcscpy(buf+firstlen+3, buf+len-secondlen-1);
   
   return _tcslen(buf);
}

// condenses a string and changes & to && (for display in menus)
// returns a new string
TCHAR *fixString(const TCHAR *inString, int size) {
   TCHAR *string = _tcsdup(inString);

   CondenseString(string, size);

   int iCount = 0;
   TCHAR *p = string;
   while (p = _tcschr(p, '&')) {
      p++;
      iCount++;
   }

   if (iCount) {
      TCHAR *newString = new TCHAR[_tcslen(string) + iCount + 2];
      TCHAR *out = newString;
      TCHAR *in = string;
      while (*in) {
         *out = *in;
         // double up the ampersand
         if (*in == '&') {
            out++;
            *out = '&';
         }
         out++;
         in++;
      }
      // can't forget to null terminate!!
      *out = 0;
      *(out+1) = 0; // double null for tb_addstring

      delete string;
      string = _tcsdup(newString);
      delete newString;
   }

   return string;
}

long FileSize(FILE *file)
{
   long oldPosition = ftell(file);

   fseek(file, 0, SEEK_END);

   long filesize = ftell(file);

   fseek(file, oldPosition, SEEK_SET);

   return filesize;
}

//Got it from nsEscape.cpp

#define HEX_ESCAPE '%'

#define UNHEX(C) \
    ((C >= '0' && C <= '9') ? C - '0' : \
     ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
     ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))

//----------------------------------------------------------------------------------------
int nsUnescapeCount(char * str)
//----------------------------------------------------------------------------------------
{
    register char *src = str;
    register char *dst = str;
    static const char hexChars[] = "0123456789ABCDEFabcdef";

    char c1[] = " ";
    char c2[] = " ";
    char* const pc1 = c1;
    char* const pc2 = c2;

    while (*src)
    {
        c1[0] = *(src+1);
        if (*(src+1) == '\0') 
            c2[0] = '\0';
        else
            c2[0] = *(src+2);

        if (*src != HEX_ESCAPE || strpbrk(pc1, hexChars) == 0 || 
                                  strpbrk(pc2, hexChars) == 0 )
        	*dst++ = *src++;
        else 	
		{
        	src++; /* walk over escape */
        	if (*src)
            {
            	*dst = UNHEX(*src) << 4;
            	src++;
            }
        	if (*src)
            {
            	*dst = (*dst + UNHEX(*src));
            	src++;
            }
        	dst++;
        }
    }

    *dst = 0;
    return (int)(dst - str);

} /* NET_UnEscapeCnt */

//----------------------------------------------------------------------------------------
char* nsUnescape(char * str)
//----------------------------------------------------------------------------------------
{
	nsUnescapeCount(str);
	return str;
}
