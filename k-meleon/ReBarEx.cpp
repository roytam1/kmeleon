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

#include "StdAfx.h"
#include "ReBarEx.h"
#include "BrowserFrm.h"
#include "rebar_menu/hot_tracking.cpp"

#if _MSC_VER >= 1300 
BEGIN_MESSAGE_MAP(CReBarEx, CReBar)
	ON_NOTIFY_REFLECT( RBN_CHEVRONPUSHED, OnChevronPushed )
END_MESSAGE_MAP()
#endif

CReBarEx::CReBarEx() {
   m_menu = theApp.m_toolbarControlsMenu;
   m_iCount=0;

   CReBar::CReBar();
}

CReBarEx::~CReBarEx() {
   if (m_iCount) {
      for (int x=0; x<m_iCount; x++) {
         if (m_index[x]->name)
            delete m_index[x]->name;
         delete m_index[x];
      }
      delete m_index;
   }
   CReBar::~CReBar();
}

#if _MSC_VER >= 1300 
void CReBarEx::OnChevronPushed( NMHDR * pNotifyStruct, LRESULT* result )
{
	NMREBARCHEVRON* pChev = (NMREBARCHEVRON*) pNotifyStruct;

	// This is currently a simple version that can support only
	// button with text. It has to be improved to support image too

	// Has the band of the chevron that generated this message
	int	iBand = pChev->uBand;

	// Have to get the child window handle this band holds
	REBARBANDINFO	rbinfo;
	rbinfo.cbSize = sizeof(rbinfo);
	rbinfo.fMask = RBBIM_CHILD;
	GetReBarCtrl().GetBandInfo ( iBand, &rbinfo );

	// Have to verify that we have a toolbar and not 
	// something else !! 
	// Can't use MFC because I can't create the 
	// bookmark toolbar with MFC
		
	// Create a popup menu to show hidden buttons
	CMenu	pop;
	pop.CreatePopupMenu ();

	// Get band rectangle.
	CRect	rectBand;
	GetReBarCtrl ().GetRect( iBand, &rectBand );
	ClientToScreen(&rectBand);
	
	// Remove chevron size
	CRect rectChevron;
	rectChevron = pChev->rc;
	rectBand.right -= rectChevron.Width ();
	
	// Screen co-ordinates for Menu to be displayed
	CPoint	ptMenu;
	ptMenu.x = rectChevron.left;
	ptMenu.y = rectChevron.bottom;
	ClientToScreen ( &ptMenu );
 
	// This flag indicates if atleast one has been added to the menu
	// POPUP Menu is shown only if atleast one item has to be shown
	BOOL	bAtleastOne=FALSE;
	int iButtonCount = ::SendMessage(rbinfo.hwndChild, TB_BUTTONCOUNT, 0, 0);
	//int iButtonCount = pTBar->GetToolBarCtrl().GetButtonCount();
	for ( int iCount = 0 ; iCount < iButtonCount ; iCount++ )
	{
		// Get the id of the toolbar button
		TBBUTTON button;
		::SendMessage(rbinfo.hwndChild, TB_GETBUTTON, iCount, (LPARAM)&button);
		int id = button.idCommand;
		//int id = pTBar->GetItemID( iCount );
		
		// If the button is a separator then we can also add a separator to the
		// popup menu
		if (  button.fsStyle /*pTBar->GetButtonStyle ( iCount )*/ & TBSTYLE_SEP )
		{
			// It wouldnt be nice if there is a separator as the first item in the menu
			if ( bAtleastOne )
				pop.AppendMenu ( MF_SEPARATOR );
		}
		else
		{
			// Get the button rectangle
			//RECT rectButton;
			RECT rectButton;
			::SendMessage(rbinfo.hwndChild, TB_GETITEMRECT, iCount, (LPARAM)&rectButton);
			POINT p,p2;
			p.x = rectButton.left;
			p.y = rectButton.top;
			p2.x = rectButton.right;
			p2.y = rectButton.bottom;
			::ClientToScreen(rbinfo.hwndChild, &p);
			::ClientToScreen(rbinfo.hwndChild, &p2);
			rectButton.left = p.x;
			rectButton.top = p.y;
			rectButton.right = p2.x;
			rectButton.bottom = p2.y;

			//pTBar->GetItemRect ( iCount, &rectButton );
			//pTBar->ClientToScreen(&rectButton);

			// Get the text of the button
			//CString csButtonText = pTBar->GetButtonText(iCount);
			
			TCHAR* buttonText;
			int l = ::SendMessage(rbinfo.hwndChild, TB_GETBUTTONTEXT, id, 0);
			if (l==-1) continue; // Can't support button with no text
		
			buttonText = new TCHAR[l+1];
			::SendMessage(rbinfo.hwndChild, TB_GETBUTTONTEXT, id, (LPARAM)buttonText);
			

			// Check the intersection of the button and the band
			CRect interRect;
			interRect.IntersectRect ( &rectButton, &rectBand );

			// if the intersection is not the same as button then
			// the button is not completely visible, so add to menu
			if ( interRect != rectButton )
			{

				UINT	iMenuStyle = MF_STRING;
				 
				
				// Have to reflect the state of the menu item, so check the state of the 
				// button and enable or disable the items
				if (::SendMessage(rbinfo.hwndChild, TB_ISBUTTONENABLED, id, 0))
				//if ( pTBar->GetToolBarCtrl().IsButtonEnabled ( id ) )
					iMenuStyle |= MF_ENABLED;
				else
					iMenuStyle |= MF_DISABLED;

				// Add the item to the menu with the id of the toolbar button
				// or add the submenu 
				if (::IsMenu((HMENU)(id - SUBMENU_OFFSET))) 
					pop.AppendMenu( iMenuStyle|MF_POPUP, id-SUBMENU_OFFSET, buttonText);
				else
					pop.AppendMenu ( iMenuStyle, id, buttonText);
				
				bAtleastOne=TRUE;
			}
			delete buttonText;
		}
	}

//	SendMessage("bmpmenu", "", "SetOwnerDrawn", (long)pop.m_hMenu, (long)DrawBitmap);
	
	if ( bAtleastOne )
		pop.TrackPopupMenu ( TPM_LEFTALIGN|TPM_TOPALIGN, ptMenu.x, ptMenu.y, this );

	// Delete our menu but keep the submenu alive
	int mCount = pop.GetMenuItemCount();
	for (int i=0;i<mCount;i++)
		pop.RemoveMenu(0, MF_BYPOSITION);
	pop.DestroyMenu ();
}
#endif
/*
void CReBarEx::MaximizeBand( UINT uBand ) {
   return;
}
*/

void CReBarEx::RegisterBand(HWND hWnd, TCHAR *name, int visibleOnMenu) {

   if (FindByName(name) != -1)
      return;

   tbBand **newIndex = new tbBand *[m_iCount+1];
   if (m_iCount) {
      memcpy(newIndex, m_index, ((m_iCount)*sizeof(tbBand *)));
      delete m_index;
   }
   m_index = newIndex;

   m_index[m_iCount] = new tbBand;
   m_index[m_iCount]->uID = NULL;
   m_index[m_iCount]->hWnd = hWnd;
   m_index[m_iCount]->name = NULL;
   m_index[m_iCount]->visibility = TRUE;
   m_index[m_iCount]->visibleOnMenu = visibleOnMenu;

   if (*name) {
      m_index[m_iCount]->name = new TCHAR[_tcslen(name)+1];
      _tcscpy(m_index[m_iCount]->name, name);
   }

   m_iCount++;
}

int CReBarEx::FindByChild(HWND hWnd) {
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);
   rbbi.fMask = RBBIM_ID | RBBIM_CHILD;

   int count = GetReBarCtrl().GetBandCount();
   for (int x=0; x<count; x++) {
      GetReBarCtrl().GetBandInfo(x, &rbbi);
      if (rbbi.hwndChild == hWnd)
            return x;
   }
   return -1;
}

int CReBarEx::FindByName(TCHAR *name) {
   for (int x=0; x<m_iCount; x++)
      if (_tcsicmp(name, m_index[x]->name) == 0)
         return FindByChild(m_index[x]->hWnd);
   return -1;
}

int CReBarEx::FindByIndex(int index) {
   return FindByChild(m_index[index]->hWnd);
}

int CReBarEx::ChildToListIndex(HWND hWnd) {
   for (int x=0; x < m_iCount; x++)
      if (m_index[x]->hWnd == hWnd)
         return x;
   return -1;
}

void CReBarEx::DrawToolBarMenu() {
   if (!m_menu) return;

   for (int x=0; x<m_iCount; x++) {
      if (m_index[x]->name && m_index[x]->visibleOnMenu) {
		  InsertMenu(m_menu, 0, MF_BYPOSITION | MF_STRING, TOOLBAR_MENU_START_ID+x, theApp.lang.Translate(m_index[x]->name));
      }
   }
}

BOOL CReBarEx::GetVisibility(int index) {
   ASSERT(index>=0 && index<m_iCount);
   // XXX A bad index can be passed here because of the layers plugin
   // not always registering the layer bar.
   if (index>=0 && index<m_iCount)
      return m_index[index]->visibility;
   return FALSE;
}

void CReBarEx::lineup() {
   REBARBANDINFO rbbi;

   for (int x=0; x<m_iCount; x++) {
     rbbi.cbSize = sizeof(rbbi);
     rbbi.fMask = RBBIM_STYLE;
     GetReBarCtrl().GetBandInfo(x, &rbbi);
     rbbi.fStyle |= RBBS_BREAK;
     GetReBarCtrl().SetBandInfo(x, &rbbi);
   }
}

void CReBarEx::ShowBand(int index, BOOL visibility) {
   GetReBarCtrl().ShowBand(index, visibility);
}

void CReBarEx::SetVisibility(int index, BOOL visibility) {
   GetReBarCtrl().ShowBand(FindByIndex(index), visibility);


   DrawMenuBar();

   m_index[index]->visibility = visibility;

   char tempPref[256] = "kmeleon.toolband."; // 17 chars
   sprintf(tempPref + 17, "%s.visibility", m_index[index]->name);
   theApp.preferences.SetBool(tempPref, m_index[index]->visibility);
}

void CReBarEx::ToggleVisibility(int index) {
   if (index < m_iCount)
      SetVisibility(index, !GetVisibility(index));
}

#define PREF_TOOLBAND_LOCKED "kmeleon.general.toolbars_locked"

void CReBarEx::SaveBandSizes() {
   int x, index;
   char tempPref[256] = "kmeleon.toolband."; // 17 chars

   BOOL locked = theApp.preferences.GetBool(PREF_TOOLBAND_LOCKED, false);
   if (locked)
      return;

   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);
   rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_ID | RBBIM_STYLE;

   theApp.preferences.DeleteBranch("kmeleon.toolband.");

   int iSkipped = 0; // counter for unindexed bands (right now just the throbber)

   int count = GetReBarCtrl().GetBandCount();
   for (x=0; x < count; x++) {
      GetReBarCtrl().GetBandInfo(x, &rbbi);
      index = ChildToListIndex(rbbi.hwndChild);

      if ((index != -1) && m_index[index]->name) {

		 // Bad conversion
		 USES_CONVERSION;
		 char *name = T2A(m_index[index]->name);

         sprintf(tempPref + 17, "%s.size", name);
         theApp.preferences.SetInt(tempPref, rbbi.cx);

         sprintf(tempPref + 17, "%s.index", name);
         theApp.preferences.SetInt(tempPref, x-iSkipped);

         sprintf(tempPref + 17, "%s.visibility", name);
         theApp.preferences.SetBool(tempPref, m_index[index]->visibility);

         sprintf(tempPref + 17, "%s.break", name);
         theApp.preferences.SetInt(tempPref, rbbi.fStyle & RBBS_BREAK);
      }
      else iSkipped++;
   }
}

void CReBarEx::RestoreBandSizes() {
   int x;
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);

   char tempPref[256] = "kmeleon.toolband."; // 17 chars
   int  *newIndex = new int[m_iCount];

   // Note: Yes, I know it looks odd to go through the same array twice, but
   //       we *HAVE* to move the bands around before setting their styles or else
   //       they don't show up in the right order (took hours to figure this out :| )
   for (x=0; x<m_iCount; x++) {
      int barIndex = FindByIndex(x); // index of the bar on the Rebar

	  // Bad conversion
      USES_CONVERSION;
      strcpy(tempPref + 17, T2A(m_index[x]->name));
      int offset = _tcslen(m_index[x]->name) + 17;

      strcpy(tempPref + offset, ".index");
      
      newIndex[x] = theApp.preferences.GetInt(tempPref, -1);
   }
   
   for (x=0; x<m_iCount; x++) {
      for (int i=0; i<m_iCount; i++) {
         if (newIndex[i] == x) {  // newIndex[i] >= 0
            int barIndex = FindByIndex(i); // index of the bar on the Rebar
            if (barIndex != -1 && barIndex != newIndex[i]) {
               GetReBarCtrl().MoveBand(barIndex, newIndex[i]);
            }
         }
      }
   }

   delete [] newIndex;

   BOOL barbreak;
   for (x=0; x<m_iCount; x++) {
      int barIndex = FindByIndex(x); // index of the bar on the Rebar

      rbbi.fMask = 0;

	  // Bad conversion
      USES_CONVERSION;
      strcpy(tempPref + 17, T2A(m_index[x]->name));
      int offset = _tcslen(m_index[x]->name) + 17;

      strcpy(tempPref + offset, ".break");
      barbreak = theApp.preferences.GetInt(tempPref, 1);
      if (barbreak) {
         rbbi.fMask |= RBBIM_STYLE;
         GetReBarCtrl().GetBandInfo(barIndex, &rbbi);
         rbbi.fStyle |= RBBS_BREAK;
      }

      strcpy(tempPref + offset, ".size");
      rbbi.cx = theApp.preferences.GetInt(tempPref, 0);
      if (rbbi.cx > 0)
         rbbi.fMask |= RBBIM_SIZE;

      GetReBarCtrl().SetBandInfo(barIndex, &rbbi);

      strcpy(tempPref + offset, ".visibility");
      if (!theApp.preferences.GetBool(tempPref, TRUE))
         SetVisibility(x, FALSE);
   }
}

void CReBarEx::LockBars(BOOL lock)
{
   int x;
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);

   for (x=0; x<m_iCount; x++) {
      int barIndex = FindByIndex(x); // index of the bar on the Rebar

      rbbi.fMask = RBBIM_STYLE;

      GetReBarCtrl().GetBandInfo(barIndex, &rbbi);

      if (lock)
         rbbi.fStyle |= RBBS_NOGRIPPER;
      else
         rbbi.fStyle &= ~RBBS_NOGRIPPER;

      GetReBarCtrl().SetBandInfo(barIndex, &rbbi);
   }
}


