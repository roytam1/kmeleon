/*
*  Copyright (C) 2001 Jeff Doozan
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


/////////////////////////////////////////////////////////////////////////////
// CHiddenWnd:
// The Evil Hidden window is used to keep mozilla running while all browser
// windows are closed during a profile change
// Now also used to receive notification messages from the tray icon


#include "KmeleonConst.h"
#include "BrowserFrm.h"

class CHiddenWnd : public CFrameWnd {

public:

   // returns the current state of the browser
   // 0 - don't stay resident
   // 1 - stay resident
   // 2 - currently staying resident
   int   Persisting();

   BOOL  StayResident();

private:

   void  QueryPersistFlags();
   void ShowBrowser(char *URI=NULL);

   // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHiddenWnd)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL


	//{{AFX_MSG(CHiddenWnd)
	afx_msg void OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnClose();
   afx_msg void OnSetPersist(WPARAM flags, LPARAM lParam);
   afx_msg void OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
   afx_msg void OnNewWindow(WPARAM wParam, LPARAM lParam);
   afx_msg void OnShowBrowser(char *URI, LPARAM lParam);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
   BOOL           m_bPersisting;
   BOOL           m_bStayResident;
   BOOL           m_bPreloadWindow;
   BOOL           m_bPreloadStartPage;
   BOOL           m_bShowNow;
   CBrowserFrame* m_pHiddenBrowser;

   BOOL        m_bFirstWindowCreated;
   // used to process the rebar DrawToolbarMenu function, which must only
   // be called once, but must be called after the first window has been created

};


