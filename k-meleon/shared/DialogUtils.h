#ifndef __DIALOG_UTILS_H__
#define __DIALOG_UTILS_H__

#include <tchar.h>

inline LPTSTR _LoadString(HINSTANCE hInst, UINT id, LPTSTR buffer, int bufferSize)
{
	if (!LoadString(hInst, id, buffer, bufferSize))
		return NULL;
	return buffer;
}

#define LOAD_STRING(id) _LoadString(kPlugin.hDllInstance, id, (LPTSTR)_alloca(512*sizeof(TCHAR)), 512)
#define LOAD_STRING_SIZE(id, size) _LoadString(kPlugin.hDllInstance, id, (LPTSTR)_alloca(size*sizeof(TCHAR)), size)

#pragma pack(push, 1)

typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;

#pragma pack(pop)

typedef TCHAR* (*TranslateProc)(const TCHAR*);
BOOL TranslateDialog(HWND hWnd, TranslateProc tp);

HWND CreateDialogEx(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam = NULL);
INT_PTR DialogBoxEx(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam = NULL);

#endif // __DIALOG_UTILS_H__