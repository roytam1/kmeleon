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

/*
void CReBarEx::MaximizeBand( UINT uBand ) {
   return;
}
*/

void CReBarEx::RegisterBand(HWND hWnd, char *name, int visibleOnMenu) {

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
      m_index[m_iCount]->name = new char[strlen(name)+1];
      strcpy(m_index[m_iCount]->name, name);
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

int CReBarEx::FindByName(char *name) {
   for (int x=0; x<m_iCount; x++)
      if (strcmpi(name, m_index[x]->name) == 0)
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
         if (m_index[x]->visibility)
            InsertMenu(m_menu, 0, MF_BYPOSITION | MF_CHECKED | MF_STRING, TOOLBAR_MENU_START_ID+x, m_index[x]->name);
         else
            InsertMenu(m_menu, 0, MF_BYPOSITION | MF_STRING, TOOLBAR_MENU_START_ID+x, m_index[x]->name);
      }
   }
}

BOOL CReBarEx::GetVisibility(int index) {
   return m_index[index]->visibility;
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

   if (m_index[index]->visibility)
      CheckMenuItem(m_menu, index+TOOLBAR_MENU_START_ID, MF_BYCOMMAND | MF_UNCHECKED);
   else
      CheckMenuItem(m_menu, index+TOOLBAR_MENU_START_ID, MF_BYCOMMAND | MF_CHECKED);

   DrawMenuBar();

   m_index[index]->visibility = visibility;
}

void CReBarEx::ToggleVisibility(int index) {
   if (index < m_iCount)
      SetVisibility(index, !GetVisibility(index));
}

#define PREF_TOOLBAND_LOCKED "kmeleon.general.toolbars_locked"

void CReBarEx::SaveBandSizes() {
   int x, index;
   char tempPref[256] = _T("kmeleon.toolband."); // 17 chars

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
         sprintf(tempPref + 17, _T("%s.size"), m_index[index]->name);
         theApp.preferences.SetInt(tempPref, rbbi.cx);

         sprintf(tempPref + 17, _T("%s.index"), m_index[index]->name);
         theApp.preferences.SetInt(tempPref, x-iSkipped);

         sprintf(tempPref + 17, _T("%s.visibility"), m_index[index]->name);
         theApp.preferences.SetBool(tempPref, m_index[index]->visibility);

         sprintf(tempPref + 17, _T("%s.break"), m_index[index]->name);
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

      strcpy(tempPref + 17, m_index[x]->name);
      int offset = strlen(m_index[x]->name) + 17;

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

      strcpy(tempPref + 17, m_index[x]->name);
      int offset = strlen(m_index[x]->name) + 17;

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


