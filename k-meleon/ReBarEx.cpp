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

void CReBarEx::RegisterBand(HWND hWnd, char *name) {

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

   for (UINT x=0; x<GetReBarCtrl().GetBandCount(); x++) {
      GetReBarCtrl().GetBandInfo(x, &rbbi);
      if (rbbi.hwndChild == hWnd)
         return GetReBarCtrl().IDToIndex(rbbi.wID);
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

void CReBarEx::DrawToolBarMenu() {
   if (!m_menu) return;

   for (int x=0; x<m_iCount; x++) {
      if (m_index[x]->name) {
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

void CReBarEx::SaveBandSizes() {
   int x, index;
   char tempPref[256] = _T("kmeleon.toolband."); // 17 chars
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);
   rbbi.fMask = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE;

   for (x=0; x < m_iCount; x++) {
      index = FindByIndex(x);
      GetReBarCtrl().GetBandInfo(index, &rbbi);

      if (m_index[x]->name){
         sprintf(tempPref + 17, _T("%s.size"), m_index[x]->name);
         theApp.preferences.SetInt(tempPref, rbbi.cx);

         sprintf(tempPref + 17, _T("%s.index"), m_index[x]->name);
         theApp.preferences.SetInt(tempPref, index);

         sprintf(tempPref + 17, _T("%s.visibility"), m_index[x]->name);
         theApp.preferences.SetBool(tempPref, m_index[x]->visibility);

         sprintf(tempPref + 17, _T("%s.break"), m_index[x]->name);
         theApp.preferences.SetInt(tempPref, rbbi.fStyle & RBBS_BREAK);
      }
   }
}

void CReBarEx::RestoreBandSizes() {
   int x;
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);

   for (x=0; (UINT) x<GetReBarCtrl().GetBandCount(); x++) {
      rbbi.fMask = RBBIM_ID;
      rbbi.wID = PLUGIN_REBAR_START_ID + x;
      GetReBarCtrl().SetBandInfo(x, &rbbi);
   }
      
   char tempPref[256] = _T("kmeleon.toolband."); // 17 chars
   BOOL barbreak;
   for (x=0; x<m_iCount; x++) {
      int index = FindByIndex(x);

      rbbi.fMask = 0;

      sprintf(tempPref + 17, _T("%s.break"), m_index[x]->name);
      barbreak = theApp.preferences.GetInt(tempPref, 0);
      if (barbreak) {
         rbbi.fMask |= RBBIM_STYLE;
         GetReBarCtrl().GetBandInfo(FindByIndex(x), &rbbi);
         rbbi.fStyle |= RBBS_BREAK;
      }

      sprintf(tempPref + 17, _T("%s.size"), m_index[x]->name);
      rbbi.cx = theApp.preferences.GetInt(tempPref, 0);
      if (rbbi.cx > 0)
         rbbi.fMask |= RBBIM_SIZE;

      sprintf(tempPref + 17, _T("%s.index"), m_index[x]->name);
      int ndx = theApp.preferences.GetInt(tempPref, -1);
      if (ndx >= 0)
         if (index != -1) {
            GetReBarCtrl().MoveBand(index, ndx);
            GetReBarCtrl().SetBandInfo(ndx, &rbbi);
         }

      sprintf(tempPref + 17, _T("%s.visibility"), m_index[x]->name);
      if (!theApp.preferences.GetBool(tempPref, TRUE))
      SetVisibility(x, FALSE);
   }
}