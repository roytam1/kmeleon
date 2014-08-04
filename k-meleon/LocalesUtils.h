/*
*  Copyright (C) 2007 Dorian Boissonnade
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

#ifndef __LOCALESUTILS_H_
#define __LOCALESUTILS_H_

#define KMELEON_PLUGIN_EXPORTS
#pragma warning(disable:4996) 

#include <windows.h>
#include <tchar.h>
#include "kmeleon_plugin.h"
#include "DialogUtils.h"
#include "strconv.h"

static HINSTANCE hResDll = NULL;
static HINSTANCE hLangDll = NULL;
static HINSTANCE hModDll = NULL;

class CResStringW : public base_convert<WCHAR>
{
public:
	CResStringW(HINSTANCE hResDll, UINT uStringID)
	{
		wchar_t  *pwchMem, *pwchCur;

		HRSRC hResource = FindResource(hResDll, MAKEINTRESOURCE(uStringID/16 + 1), RT_STRING);
		if (!hResource) return;

		pwchMem = (wchar_t *)LoadResource(hResDll, hResource);
		if (!pwchMem) return;

		pwchCur = pwchMem;
		for(int i = 0; i<16; i++ )
		{
			if (*pwchCur)
			{
				int cchString = *pwchCur;  // String size in characters.
				pwchCur++;
				if (i == uStringID%16)
				{
					// The string has been found in the string table.
					Init(cchString+1);
					wcsncpy(mBuffer, pwchCur, cchString);
					mBuffer[cchString] = 0;
					break;
				}
				pwchCur += cchString;
			}
			else
				pwchCur++;
		}
	}
};

class CResStringA : public base_convert<char>
{
public:
	CResStringA(HINSTANCE hResDll, UINT uStringID)
	{
		Steal(CUTF16_to_ANSI(CResStringW(hResDll, uStringID)));
	}

};


#ifdef _UNICODE
#define CResString CResStringW
#else
#define CResString CResStringA
#endif

class CResStringFormatA : public base_convert<char>
{
public:
	int FormatV(HINSTANCE hResDll, UINT nStringID, va_list argList)
	{
		CResStringA strW(hResDll, nStringID);
		int fLength = _vscprintf(strW, argList);
		if (fLength<0) return -1;
		
		if (!Init(fLength+1))
			return -1;
		
		return vsprintf(mBuffer, strW, argList);
	}

	CResStringFormatA()
	{
	}

	CResStringFormatA(HINSTANCE hResDll, UINT nStringID, ...) 
	{
		va_list argList;
		va_start(argList, nStringID);
		FormatV(hResDll, nStringID, argList);
		va_end(argList);
	}
};

class CResStringFormatW : public base_convert<WCHAR>
{
public:
	int FormatV(HINSTANCE hResDll, UINT nStringID, va_list argList) 
	{
		CResStringW strW = CResStringW(hResDll, nStringID);
		int fLength = _vscwprintf(strW, argList);
		if (fLength<0) return -1;
		
		if (!Init(fLength+1))
			return -1;

		return vswprintf(mBuffer, strW, argList);
	}

	CResStringFormatW()
	{
	}

	CResStringFormatW(HINSTANCE hResDll, UINT nStringID, ...) 
	{
		va_list argList;
		va_start(argList, nStringID);
		FormatV(hResDll, nStringID, argList);
		va_end(argList);
	}
};

#ifdef _UNICODE
#define CResStringFormat CResStringFormatW
#else
#define CResStringFormat CResStringFormatA
#endif

class Locale {
private:
	HINSTANCE hModDll;
	HINSTANCE hResDll;
	HINSTANCE hLangDll;
	BOOL bUserFont;
	const TCHAR* sName;

	static Locale* mLocale;
	
	Locale(HINSTANCE hModule, const TCHAR* localedir, const TCHAR* name, BOOL userFont)
	{
		TCHAR* resfilename = new TCHAR[6 + _tcslen(localedir) + _tcslen(name)];
		_tcscpy(resfilename, localedir);
		_tcscat(resfilename, _T("\\"));
		_tcscat(resfilename, name);
		_tcscat(resfilename, _T(".dll"));

		hModDll = hModule;
		hLangDll = LoadLibrary(resfilename);
		hResDll = hLangDll ? hLangDll : hModule;
		bUserFont = userFont;
		sName = name;
		delete resfilename;
	}

public:

	static Locale* kmInit(kmeleonPlugin* kp)
	{
		char root[MAX_PATH];
		kp->kFuncs->GetFolder(RootFolder, root, MAX_PATH);
		
		int len = kp->kFuncs->GetPreference(PREF_LOCALIZED, "general.useragent.locale", NULL, "en");
		char* langid = new char[len+1];
		kp->kFuncs->GetPreference(PREF_LOCALIZED, "general.useragent.locale", langid, "en");

		char* localeFolder = new char[strlen(root) + strlen(langid) + 15];
		strcpy(localeFolder, root);
		strcat(localeFolder, "\\locales\\");
		strcat(localeFolder, langid);

		BOOL userFont = FALSE;
		kp->kFuncs->GetPreference(PREF_BOOL, "kmeleon.display.dialogs.useUserFont", &userFont, &userFont);

		Locale* l = new Locale(kp->hDllInstance, CUTF8_to_T(localeFolder), CANSI_to_T(kp->dllname), userFont);
		
		delete [] localeFolder;
		delete [] langid;
		return l;
	}

	~Locale()
	{
		if (hLangDll) FreeLibrary(hLangDll);
	}

	inline HWND CreateDialogParam(LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam = NULL)
	{
#if 0
		if (bUserFont)
			return CreateDialogEx(hResDll, lpTemplate, hwndParent, lpDialogProc, lParam);
#endif
		HWND hWnd = ::CreateDialogParam(hResDll, lpTemplate, hwndParent, lpDialogProc, lParam);
		if (hWnd) return hWnd;
		return ::CreateDialogParam(hModDll, lpTemplate, hwndParent, lpDialogProc, lParam);
	}

	inline INT_PTR DialogBoxParam(LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam = NULL)
	{
#if 0
		if (bUserFont)
			return DialogBoxEx(hResDll, lpTemplate, hwndParent, lpDialogProc, lParam);
#endif
		INT_PTR res = ::DialogBoxParam(hResDll, lpTemplate, hwndParent, lpDialogProc, lParam);
		if (res != -1) return res;
		return ::DialogBoxParam(hModDll, lpTemplate, hwndParent, lpDialogProc, lParam);
	}

	CResString GetString(UINT uStringID)
	{
		CResString ret(hResDll, uStringID);
		if (ret) return ret;
		return CResString(hModDll, uStringID);
	}

	CResStringA GetStringA(UINT uStringID)
	{
		CResStringA ret(hResDll, uStringID);
		if (ret) return ret;
		return CResStringA(hModDll, uStringID);
	}

	CResStringW GetStringW(UINT uStringID)
	{
		CResStringW ret(hResDll, uStringID);
		if (ret) return ret;
		return CResStringW(hModDll, uStringID);
	}

	CResStringFormatA GetStringFormatA(UINT uStringID, ...)
	{
		va_list argList;
		va_start(argList, uStringID);
		CResStringFormatA ret = GetStringFormatVA(uStringID, argList);
		va_end(argList);
		return ret;
	}

	CResStringFormatW GetStringFormatVW(UINT uStringID, va_list argList)
	{
		CResStringFormatW ret;
		ret.FormatV(hResDll, uStringID, argList);
		if (!ret) ret.FormatV(hModDll, uStringID, argList);
		return ret;
	}

	CResStringFormatA GetStringFormatVA(UINT uStringID, va_list argList)
	{
		CResStringFormatA ret;
		ret.FormatV(hResDll, uStringID, argList);
		if (!ret) ret.FormatV(hModDll, uStringID, argList);
		return ret;
	}

	CResStringFormatW GetStringFormatW(UINT uStringID, ...)
	{
		va_list argList;
		va_start(argList, uStringID);
		CResStringFormatW ret = GetStringFormatVW(uStringID, argList);
		va_end(argList);
		return ret;
	}
	
	inline CResStringFormat GetStringFormat(UINT uStringID, ...)
	{
		va_list argList;
		va_start(argList, uStringID);
#ifdef _UNICODE
		return GetStringFormatVW(uStringID, argList);
#else
		return GetStringFormatVA(uStringID, argList);
#endif
		va_end(argList);
	}

	HMENU LoadMenu(UINT uMenuID)
	{
		HMENU menu = ::LoadMenu(hResDll, MAKEINTRESOURCE(uMenuID));
		if (menu) return menu;
		return ::LoadMenu(hModDll, MAKEINTRESOURCE(uMenuID));
	}

/*
	wchar_t* GetStringW(UINT uStringID)
	{
		wchar_t   *pwchMem, *pwchCur;

		HRSRC hResource = FindResource(hResDll, MAKEINTRESOURCE(uStringID/16 + 1), RT_STRING);
		if (!hResource) return NULL;

		pwchMem = (wchar_t *)LoadResource(hResDll, hResource);
		if (!pwchMem) return NULL;

		pwchCur = pwchMem;
		for(int i = 0; i<16; i++ )
		{
			if (*pwchCur)
			{
				int cchString = *pwchCur;  // String size in characters.
				pwchCur++;
				if (i == uStringID%16)
				{
					// The string has been found in the string table.
					wchar_t *pwchTemp = (wchar_t*)malloc(sizeof(wchar_t)*(cchString+1));
					wcsncpy(pwchTemp, pwchCur, cchString);
					pwchTemp[cchString] = 0;
					return pwchTemp;
				}
				pwchCur += cchString;
			}
			else
				pwchCur++;
		}
		return NULL;
	}

	wchar_t* GetStringFormatVW(UINT uStringID, va_list argList)
	{
		wchar_t* pszStr = GetStringW(uStringID);
		if (!pszStr) return NULL;

		int nFormatLength = _vscwprintf(pszStr, argList);
		if (nFormatLength<0) return NULL;
		
		wchar_t* buffer = (wchar_t*)malloc(wcslen(pszStr) + nFormatLength);
		if (!buffer) return NULL;

		vswprintf(buffer, pszStr, argList);
		free(pszStr);
		return buffer;
	}

	char* GetStringA(UINT uStringID)
	{
		wchar_t* strw = GetStringW(uStringID);
		char* stra = ansi_from_utf16(strw);
		free(strw);
		return stra;
	}

	wchar_t* GetStringFormatW(UINT uStringID, ... )
	{
		va_list argList;
		va_start(argList, uStringID);
		wchar_t* ret = GetStringFormatVW(uStringID, argList);
		va_end(argList);
		return ret;
	}

	char* GetStringFormatVA(UINT uStringID, va_list argList)
	{
		char* pszStr = GetStringA(uStringID);
		if (!pszStr) return NULL;

		int fLength = _vscprintf(pszStr, argList);
		if (fLength<0) return NULL;
		
		char* buffer = (char*)malloc(fLength +1);
		if (!buffer) return NULL;

		vsprintf(buffer, pszStr, argList);
		free(pszStr);
		return buffer;
	}

	char* GetStringFormatA(UINT uStringID, ...)
	{
		va_list argList;
		va_start(argList, uStringID);
		char* ret = GetStringFormatVA(uStringID, argList);
		va_end(argList);
		return ret;
	}

	inline TCHAR* GetStringFormat(UINT uStringID, ...)
	{
		va_list argList;
		va_start(argList, uStringID);
#ifdef _UNICODE
		return GetStringFormatVW(uStringID, argList);
#else
		return GetStringFormatVA(uStringID, argList);
#endif
		va_end(argList);
	}

	inline TCHAR* GetString(UINT uStringID)
	{
#ifdef _UNICODE
		return GetStringW(uStringID);
#else
		return GetStringA(uStringID);
#endif
	}*/
};

#endif
