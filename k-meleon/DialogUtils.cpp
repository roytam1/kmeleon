#include <windows.h>
#include "DialogUtils.h"

inline BOOL IsDialogEx(const DLGTEMPLATE* pTemplate)
{
	return ((DLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF;
}

inline int FontAttrSize(BOOL bDialogEx)
{
	return (int)sizeof(WORD) * (bDialogEx ? 3 : 1);
}

inline BOOL HasFont(const DLGTEMPLATE* pTemplate)
{
	return (DS_SETFONT &
		(IsDialogEx(pTemplate) ? ((DLGTEMPLATEEX*)pTemplate)->style : pTemplate->style));
}

BYTE* GetFontSizeField(const DLGTEMPLATE* pTemplate)
{
	BOOL bDialogEx = IsDialogEx(pTemplate);
	WORD* pw;

	if (bDialogEx)
		pw = (WORD*)((DLGTEMPLATEEX*)pTemplate + 1);
	else
		pw = (WORD*)(pTemplate + 1);

	// Skip menu name string or ordinal
	if (*pw == (WORD)-1)        
		pw += 2; 
	else
		while(*pw++);

	// Skip class name string or ordinal
	if (*pw == (WORD)-1)        
		pw += 2; 
	else
		while(*pw++);

	// Skip caption string
	while (*pw++);          

	return (BYTE*)pw;
}

BOOL GetMessageFont(WORD *nPointSize, TCHAR* lpFaceName)
{
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0))
		return FALSE;

	LOGFONT &lf = ncm.lfMessageFont;

	if (lf.lfHeight < 0)
		lf.lfHeight = -lf.lfHeight;

	HDC hDC    = ::GetDC(NULL);
	*nPointSize  = MulDiv(lf.lfHeight,72,GetDeviceCaps(hDC, LOGPIXELSY));
	::ReleaseDC(NULL,hDC);

	_tcscpy(lpFaceName, lf.lfFaceName);

	return TRUE;
}

BOOL CALLBACK _EnumTranslate(HWND hWnd, LPARAM lParam)
{
	TCHAR text[1024];
	TranslateProc tp = (TranslateProc)lParam;
	if (GetDlgCtrlID(hWnd)>0) {
		GetWindowText(hWnd, text, sizeof(text)/sizeof(TCHAR));
		SetWindowText(hWnd, tp(text));
	}
	return TRUE;
}

BOOL TranslateDialog(HWND hWnd, TranslateProc tp)
{
	TCHAR text[1024];
	GetWindowText(hWnd, text, sizeof(text)/sizeof(TCHAR));
	SetWindowText(hWnd, tp(text));

/*	HWND child = GetWindow(hWnd, GW_CHILD | GW_HWNDFIRST);
	if (!child) return FALSE;

	do {
		GetDlgCtrlID

	} while (child = GetWindow(child, GW_HWNDNEXT));
*/
	return EnumChildWindows(hWnd, _EnumTranslate, (LPARAM)tp);
}


HGLOBAL GetTemplate(HINSTANCE hInst, LPCTSTR lpTemplate)
{
	HGLOBAL hTemplate;

	HRSRC hRsrc = FindResource(hInst, lpTemplate, RT_DIALOG);
	HGLOBAL tTemplate = LoadResource(hInst, hRsrc);
	DLGTEMPLATE* pTemplate = (DLGTEMPLATE*)LockResource(tTemplate);
	UINT dwTemplateSize = SizeofResource(hInst, hRsrc);
	if ((hTemplate = GlobalAlloc(GPTR, dwTemplateSize + LF_FACESIZE * 2)) == NULL)
		return NULL;
	DLGTEMPLATE* pNew = (DLGTEMPLATE*)GlobalLock(hTemplate);
	memcpy((BYTE*)pNew, pTemplate, (size_t)dwTemplateSize);

	GlobalUnlock(hTemplate);
	UnlockResource(tTemplate);
	FreeResource(tTemplate);

	TCHAR lpFaceName[128];
	WORD nFontSize;
	if (GetMessageFont(&nFontSize, lpFaceName))
	{
		pTemplate = (DLGTEMPLATE*)GlobalLock(hTemplate);

		BOOL bDialogEx = IsDialogEx(pTemplate);
		BOOL bHasFont = HasFont(pTemplate);
		int cbFontAttr = FontAttrSize(bDialogEx);

		if (bDialogEx)
			((DLGTEMPLATEEX*)pTemplate)->style |= DS_SETFONT;
		else
			pTemplate->style |= DS_SETFONT;

		int nFaceNameLen = lstrlen(lpFaceName);
		if( nFaceNameLen < LF_FACESIZE )
		{
#ifdef _UNICODE
			int cbNew = cbFontAttr + ((nFaceNameLen + 1) * sizeof(TCHAR));
			BYTE* pbNew = (BYTE*)lpFaceName;
#else
			WCHAR wszFaceName [LF_FACESIZE];
			int cbNew = cbFontAttr + 2 * MultiByteToWideChar(CP_ACP, 0, lpFaceName, -1, wszFaceName, LF_FACESIZE);
			BYTE* pbNew = (BYTE*)wszFaceName;
#endif

			BYTE* pb = GetFontSizeField(pTemplate);
			int cbOld = (int)(bHasFont ? cbFontAttr + 2 * (wcslen((WCHAR*)(pb + cbFontAttr)) + 1) : 0);

			BYTE* pOldControls = (BYTE*)(((DWORD_PTR)pb + cbOld + 3) & ~DWORD_PTR(3));
			BYTE* pNewControls = (BYTE*)(((DWORD_PTR)pb + cbNew + 3) & ~DWORD_PTR(3));

			WORD nCtrl = bDialogEx ? (WORD)((DLGTEMPLATEEX*)pTemplate)->cDlgItems :
			(WORD)pTemplate->cdit;

			if (cbNew != cbOld && nCtrl > 0)
				memmove(pNewControls, pOldControls, (size_t)(dwTemplateSize - (pOldControls - (BYTE*)pTemplate)));

			*(WORD*)pb = nFontSize;
			memmove(pb + cbFontAttr, pbNew, cbNew - cbFontAttr);

			dwTemplateSize += ULONG(pNewControls - pOldControls);
		}
		GlobalUnlock(hTemplate);
	}

	return hTemplate;
}

HWND CreateDialogEx(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam)
{
	HGLOBAL hTemplate = GetTemplate(hInst, lpTemplate);
	LPCDLGTEMPLATE lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hTemplate);
	HWND hWnd = ::CreateDialogIndirectParam(hInst, lpDialogTemplate,
		hwndParent, lpDialogProc, lParam);

	UnlockResource(hTemplate);
	GlobalFree(hTemplate);
	return hWnd;
}

INT_PTR DialogBoxEx(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hwndParent, DLGPROC lpDialogProc, LPARAM lParam)
{
	HGLOBAL hTemplate = GetTemplate(hInst, lpTemplate);
	LPCDLGTEMPLATE lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hTemplate);
	INT_PTR result = DialogBoxIndirectParam(hInst, lpDialogTemplate, hwndParent, lpDialogProc, lParam);

	UnlockResource(hTemplate);
	GlobalFree(hTemplate);
	return result;
}