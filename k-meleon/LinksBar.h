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

#if !defined(AFX_LINKSBAR_H__9BDC9475_FAE1_11D2_A713_0090274409AC__INCLUDED_)
#define AFX_LINKSBAR_H__9BDC9475_FAE1_11D2_A713_0090274409AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LinksBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLinksBar window

class CLinksBar : public CBCGToolBar
{
	DECLARE_SERIAL(CLinksBar)

// Construction
public:
	CLinksBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinksBar)
	//}}AFX_VIRTUAL

	virtual BOOL OnSendCommand (const CBCGToolbarButton* pButton);
	virtual BOOL CanBeRestored () const
	{
		return TRUE;
	}
	virtual BOOL RestoreOriginalstate ();

// Implementation
public:
	virtual ~CLinksBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLinksBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINKSBAR_H__9BDC9475_FAE1_11D2_A713_0090274409AC__INCLUDED_)
