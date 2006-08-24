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

char* ParseString(char** input, bool *equal)
{
	char* string = *input, *p, *q;
	bool quotes = false;

	*equal = false;
	string = SkipWhiteSpace(string);
	if (*string == '"') {
		quotes = true;
		++string;
	}

	p = string;
	while ( *p )
	{
		if (quotes && *p == '"') {
			*p = 0;
			p = SkipWhiteSpace(p+1);
			if (*p == '=') {
				p++;
				*equal = true;
			}

			*input = p;
			return string;
		}
		else if(!quotes && *p=='=') {
			*p = 0;
			TrimWhiteSpace(string);
			*input = p+1;
			*equal = true;
			return string;
		}

		q = p++;
		if (*q == '\\') 
		{
			switch (*p) {
				case 'n': 
					*q = '\n';
					break;
				case 'r': 
					*q = '\r';
					break;
				case 't':
					*q = '\t';
					break;
				case '\\': 
					*q = '\\';
					break;
				case '"': 
					*q = '"';
					break;
				default:
					continue;
			}
			if (*p) {
				strcpy(p, p+1);
			}
		}
	}
	*input = p;
	TrimWhiteSpace(string);
	return string;
}

int CLangParser::Parse(char *p)
{
	if (!*p) return 0;

	bool equal;
	char *original = ParseString(&p, &equal);
	
	if (!equal) {
		if (strlen(p)>25) p[25] = 0;
		LOG_ERROR_1("Equal '=' expected, found %s", *p ? p : "nothing");
		return 0;
	}

	char *translation = ParseString(&p, &equal);

	if (equal) {
		if (strlen(p)>25) p[25] = 0;
		LOG_ERROR_1("Equal '=' unexpected, %s", *p ? p : "nothing");
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
		langMap[A2T(original)] = translation;
		LOG_1("Add translation for %s", original);
	}

	return 0;
}
