/*
*  Copyright (C) 2000 Christophe Thibault
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

#if !defined(AFX_MAINTOOLBAR_H__F83C8E1C_F33E_11D2_A713_0090274409AC__INCLUDED_)
#define AFX_MAINTOOLBAR_H__F83C8E1C_F33E_11D2_A713_0090274409AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainToolBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMainToolBar window

class CMainToolBar : public CBCGToolBar
{
	DECLARE_SERIAL(CMainToolBar)

// Construction
public:
	CMainToolBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainToolBar)
	//}}AFX_VIRTUAL

	virtual BOOL CanBeRestored () const
	{
		return TRUE;
	}

	virtual BOOL RestoreOriginalstate ();

// Implementation
public:
	virtual ~CMainToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainToolBar)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINTOOLBAR_H__F83C8E1C_F33E_11D2_A713_0090274409AC__INCLUDED_)
