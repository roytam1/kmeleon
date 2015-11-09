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

#include "StdAfx.h"
#include <afxtempl.h>
#include "LangParser.h"
#include "Utils.h"
#include "strconv.h"

CLangParser::CLangParser()
{
}

CLangParser::CLangParser(LPCTSTR filename)
{
	Load(filename);
}

CLangParser::~CLangParser(void)
{
}

int CLangParser::Load(LPCTSTR filename)
{
	SETUP_LOG("Lang");
	int retVal = CParser::Load(filename);
	END_LOG();
	return retVal;
}

TCHAR* ParseString(TCHAR** input, bool *equal)
{
	TCHAR* string = *input, *p, *q;
	bool quotes = false;

	*equal = false;
	string = SkipWhiteSpace(string);
	if (*string == _T('"')) {
		quotes = true;
		++string;
	}

	p = string;
	while ( *p )
	{
		if (quotes && *p == _T('"')) {
			*p = 0;
			p = SkipWhiteSpace(p+1);
			if (*p == _T('=')) {
				p++;
				*equal = true;
			}

			*input = p;
			return string;
		}
		else if(!quotes && *p==_T('=')) {
			*p = 0;
			TrimWhiteSpace(string);
			*input = p+1;
			*equal = true;
			return string;
		}

		q = p++;
		if (*q == _T('\\')) 
		{
			switch (*p) {
				case _T('n'): 
					*q = _T('\n');
					break;
				case _T('r'): 
					*q = _T('\r');
					break;
				case _T('t'):
					*q = _T('\t');
					break;
				case _T('\\'): 
					*q = _T('\\');
					break;
				case _T('"'): 
					*q = _T('"');
					break;
				default:
					continue;
			}
			if (*p) {
				_tcscpy(p, p+1);
			}
		}
	}
	*input = p;
	TrimWhiteSpace(string);
	return string;
}

int CLangParser::Parse(char *p)
{
	return _Parse((TCHAR*)(const TCHAR*)CUTF8_to_T(p));
}

int CLangParser::_Parse(TCHAR *p)
{
	if (!*p) return 0;

	bool equal;
	TCHAR *original = ParseString(&p, &equal);

	if (!equal) {
		USES_CONVERSION;
		if (_tcslen(p)>25) p[25] = 0;
		LOG_ERROR_1("Equal '=' expected, found %s", *p ? T2A(p) : "nothing");
		return 0;
	}

	TCHAR *translation = ParseString(&p, &equal);

	if (equal) {
		USES_CONVERSION;
		if (_tcslen(p)>25) p[25] = 0;
		LOG_ERROR_1("Equal '=' unexpected, %s", *p ? T2A(p) : "nothing");
		return 0;
	}
/*

	p = SkipWhiteSpace(p);
	if (!*p) return 0;
    
	if ( *p == '"')
	{
		equal = p+1;
		while (*equal!=0 && (*equal!='"' || *(equal-1)=='\\'))
			equal++;
		
		if (!*equal) {
			LOG_ERROR_1("Unclosed quote found", 0);
			return 0;
		}

		original = p+1;
		*equal = 0;
		equal = SkipWhiteSpace(equal+1);

		if (*equal != '=') {
			if (strlen(equal)>25) equal[25] = 0;
			LOG_ERROR_1("Equal '=' expected, found %s", *equal ? equal : "nothing");
			return 0;
		}

		*equal = 0;
		//equal = strchr(equal+1, '=');
	}
	else
	{
		original = p;
       	equal = strchr(p, '=');
		if (equal) {
			*equal = 0;
			TrimWhiteSpace(original);
		} else {
			LOG_ERROR_1("Equal '=' not found", 0);
			return 0;
		}
	}

	char *translation;
	translation = equal+1;
	translation = SkipWhiteSpace(translation);
	TrimWhiteSpace(translation);
	if (*translation == '"')
	{
		translation = equal = translation+1;
		while (*equal!=0 && (*equal!='"' || *(equal-1)=='\\'))
			equal++;
		*equal = 0;
	}*/

	USES_CONVERSION;
	if (*translation) {
		langMap[original] = translation;
		LOG_1("Add translation for %s", T2A(original));
	}

	return 0;
}
