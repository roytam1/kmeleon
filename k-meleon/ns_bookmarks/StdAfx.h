// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__42896FFA_F6AF_11D4_AA28_00E02916DA72__INCLUDED_)
#define AFX_STDAFX_H__42896FFA_F6AF_11D4_AA28_00E02916DA72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define OEMRESOURCE

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// for import favorites
#include <shlobj.h>


#include <windows.h>

#include "commctrl.h"
#include "commdlg.h"

#include <time.h>

#pragma warning( disable : 4786 ) // C4786 bitches about the std template names expanding beyond 255 characters
#include <string>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__42896FFA_F6AF_11D4_AA28_00E02916DA72__INCLUDED_)
