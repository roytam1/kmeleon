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
#include "KMeleonDoc.h"
#include "LinksBar.h"
#include "LinkButton.h"
#include "MainFrm.h"
#include "Mozilla.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL(CLinksBar, CBCGToolBar, 1)

/////////////////////////////////////////////////////////////////////////////
// CLinksBar

CLinksBar::CLinksBar()
{
}

CLinksBar::~CLinksBar()
{
}


BEGIN_MESSAGE_MAP(CLinksBar, CBCGToolBar)
	//{{AFX_MSG_MAP(CLinksBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLinksBar message handlers

int CLinksBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CBCGToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	RestoreOriginalstate ();
	return 0;
}

BOOL CLinksBar::OnSendCommand (const CBCGToolbarButton* pButton)
{
	CLinkButton* pLinkButton = DYNAMIC_DOWNCAST (CLinkButton, pButton);
	if (pLinkButton == NULL)
	{
		// Defauult processing
		return FALSE;
	}

	CString strURL = pLinkButton->GetURL ();

	((CMozilla *)GetParentFrame ()->GetActiveView())->Navigate (&strURL);
	return TRUE;
}

BOOL CLinksBar::RestoreOriginalstate ()
{
	RemoveAllButtons ();

	InsertButton (CLinkButton (_T("K-Meleon"), _T("http://www.kmeleon.org/")));
	InsertButton (CLinkButton (_T("Mozilla"), _T("http://www.mozilla.org/")));
	InsertButton (CLinkButton (_T("SlashDot"), _T("http://slashdot.org/")));
	InsertButton (CLinkButton (_T("WinAmp"), _T("http://www.winamp.com/")));
	InsertButton (CLinkButton (_T("Google"), _T("http://www.google.com/")));

	EnableCustomizeButton (TRUE, -1, _T(""));

	AdjustLayout ();
	Invalidate ();

	return TRUE;
}
