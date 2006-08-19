/*
*  Copyright (C) 2005 Dorian Boissonnade
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

#ifndef __LANGPARSER_H__
#define __LANGPARSER_H__

#include "Parser.h"

class CLangParser : public CParser
{
protected:
	CMap<CString, LPCTSTR, CString, LPCTSTR> langMap;
    int Parse(char *p);

public:
	CLangParser(void);
	CLangParser(CString &filename);
	~CLangParser(void);

	int Load(CString &filename);
	LPCTSTR Translate(LPCTSTR originalText) {
#if _MSC_VER >= 1300 
		CMap<CString, LPCTSTR, CString, LPCTSTR>::CPair * p = langMap.PLookup(originalText);
		return p ? p->value.GetBuffer(0) : originalText;
#else
		static CString t;
		if (langMap.Lookup(originalText, t))
			return t;
		return originalText;
#endif
	}

	int Translate(LPCTSTR originalText, CString& translatedText){ 
		return langMap.Lookup(originalText,translatedText);
	}
};

#endif // __LANGPARSER_H__