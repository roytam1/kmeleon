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
#include "MainToolBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL(CMainToolBar, CBCGToolBar, 1)

/////////////////////////////////////////////////////////////////////////////
// CMainToolBar

CMainToolBar::CMainToolBar()
{
}

CMainToolBar::~CMainToolBar()
{
}


BEGIN_MESSAGE_MAP(CMainToolBar, CBCGToolBar)
	//{{AFX_MSG_MAP(CMainToolBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainToolBar message handlers

BOOL CMainToolBar::RestoreOriginalstate ()
{
	RemoveAllButtons ();

	CString str;

	CMenu menuHistory;
	menuHistory.LoadMenu (IDR_HISTORY_POPUP);

	str.LoadString(IDS_BACK);
	InsertButton (CBCGToolbarMenuButton (ID_GO_BACK, menuHistory, 0, str));

	str.LoadString(IDS_FORWARD);
	InsertButton (CBCGToolbarMenuButton (ID_GO_FORWARD, menuHistory, 1, str));

	str.LoadString(IDS_STOP);
	InsertButton (CBCGToolbarButton (ID_VIEW_STOP, 2, str));

	str.LoadString(IDS_REFRESH);
	InsertButton (CBCGToolbarButton (ID_VIEW_REFRESH, 3, str));

	str.LoadString(IDS_HOME);
	InsertButton (CBCGToolbarButton (ID_GO_START_PAGE, 4, str));

	InsertSeparator ();
	
	str.LoadString(IDS_SEARCH);
	InsertButton (CBCGToolbarButton (ID_GO_SEARCH_THE_WEB, 5, str));

	CMenu menuFavorites;
	menuFavorites.LoadMenu (IDR_FAVORITES_POPUP);
	str.LoadString(IDS_FAVORITES);
	InsertButton (CBCGToolbarMenuButton (-1, menuFavorites, 6, str));

	InsertSeparator ();

	str.LoadString(IDS_PRINT);
	InsertButton (CBCGToolbarButton (ID_FILE_PRINT, 7, str));

	CMenu menuFonts;
	menuFonts.LoadMenu (IDR_FONT_POPUP);
	str.LoadString(IDS_FONT);
	InsertButton (CBCGToolbarMenuButton (-1, *menuFonts.GetSubMenu (0), 8, str));

	EnableCustomizeButton (TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));

	AdjustLayout ();

	SetWindowText (_T("Standard"));
	return TRUE;
}
