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

#include "stdafx.h"
#include "KMeleon.h"
#include "LinkButton.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CLinkButton, CBCGToolbarButton, 1)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLinkButton::CLinkButton()
{
	Initialize ();
}
//***************************************************************************************
CLinkButton::CLinkButton(LPCTSTR lpszLabel, LPCTSTR lpszURL)
{
	m_strURL = lpszURL;
	m_strText = lpszLabel;

	Initialize ();
}
//***************************************************************************************
CLinkButton::~CLinkButton()
{

}
//***************************************************************************************
void CLinkButton::Initialize ()
{
	m_nID = ID_LINK_1;
	SetImage (15);
	m_bImage = m_bText = TRUE;
}
//*********************************************************************************
void CLinkButton::CopyFrom (const CBCGToolbarButton& src)
{
	CBCGToolbarButton::CopyFrom (src);

	const CLinkButton& srcLinkButton = (const CLinkButton&) src;
	m_strURL = srcLinkButton.m_strURL;
}					
//***************************************************************************************
void CLinkButton::Serialize (CArchive& ar)
{
	CBCGToolbarButton::Serialize (ar);

	if (ar.IsLoading ())
	{
		ar >> m_strURL;
	}
	else
	{
		ar << m_strURL;
	}
}
