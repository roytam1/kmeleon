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

#if !defined(AFX_LINKBUTTON_H__9BDC9476_FAE1_11D2_A713_0090274409AC__INCLUDED_)
#define AFX_LINKBUTTON_H__9BDC9476_FAE1_11D2_A713_0090274409AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CLinkButton : public CBCGToolbarButton  
{
	DECLARE_SERIAL(CLinkButton)

public:
	CLinkButton();
	CLinkButton(LPCTSTR lpszLabel, LPCTSTR lpszURL);

	virtual ~CLinkButton();

	LPCTSTR GetURL () const
	{
		return m_strURL;
	}

protected:
	void Initialize ();
	virtual void CopyFrom (const CBCGToolbarButton& src);
	virtual void Serialize (CArchive& ar);

	CString	m_strURL;

	virtual BOOL IsEditable () const
	{
		return FALSE;
	}
};

#endif // !defined(AFX_LINKBUTTON_H__9BDC9476_FAE1_11D2_A713_0090274409AC__INCLUDED_)
