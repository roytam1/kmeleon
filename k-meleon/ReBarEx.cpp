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
   tbCount=0;

   CReBar::CReBar();
}

CReBarEx::~CReBarEx() {
   if (tbCount) {
      for (int x=0; x<tbCount; x++) {
         if (tbIndex[x]->name)
            delete tbIndex[x]->name;
         delete tbIndex[x];
      }
      delete tbIndex;
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

   tbBand **newIndex = new tbBand *[tbCount+1];
   if (tbCount) {
      memcpy(newIndex, tbIndex, ((tbCount)*sizeof(tbBand *)));
      delete tbIndex;
   }
   tbIndex = newIndex;

   tbIndex[tbCount] = new tbBand;
   tbIndex[tbCount]->uID = NULL;
   tbIndex[tbCount]->hWnd = hWnd;
   tbIndex[tbCount]->name = NULL;
   tbIndex[tbCount]->visibility = TRUE;

   if (*name) {
      tbIndex[tbCount]->name = new char[strlen(name)+1];
      strcpy(tbIndex[tbCount]->name, name);
   }

   tbCount++;
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
   for (int x=0; x<tbCount; x++)
      if (strcmpi(name, tbIndex[x]->name) == 0)
         return FindByChild(tbIndex[x]->hWnd);
   return -1;
}

int CReBarEx::FindByIndex(int index) {
   return FindByChild(tbIndex[index]->hWnd);
}

void CReBarEx::DrawToolBarMenu() {
   if (!m_menu) return;
   for (int x=0; x<tbCount; x++) {
      if (tbIndex[x]->name) {
         if (tbIndex[x]->visibility)
            InsertMenu(m_menu, 0, MF_BYPOSITION | MF_CHECKED | MF_STRING, TOOLBAR_MENU_START_ID+x, tbIndex[x]->name);
         else
            InsertMenu(m_menu, 0, MF_BYPOSITION | MF_STRING, TOOLBAR_MENU_START_ID+x, tbIndex[x]->name);
      }
   }
}

BOOL CReBarEx::GetVisibility(int index) {
   return tbIndex[index]->visibility;
}

void CReBarEx::SetVisibility(int index, BOOL visibility) {
   GetReBarCtrl().ShowBand(FindByIndex(index), visibility);

   if (tbIndex[index]->visibility)
      CheckMenuItem(m_menu, index+TOOLBAR_MENU_START_ID, MF_BYCOMMAND | MF_UNCHECKED);
   else
      CheckMenuItem(m_menu, index+TOOLBAR_MENU_START_ID, MF_BYCOMMAND | MF_CHECKED);

   DrawMenuBar();

   tbIndex[index]->visibility = visibility;
}

void CReBarEx::ToggleVisibility(int index) {
   if (index < tbCount)
      SetVisibility(index, !GetVisibility(index));
}

void CReBarEx::SaveBandSizes() {
   int x, index;
   char tempPref[256] = _T("kmeleon.toolband."); // 17 chars
   REBARBANDINFO rbbi;
   rbbi.cbSize = sizeof(rbbi);
   rbbi.fMask = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE;

   for (x=0; x < tbCount; x++) {
      index = FindByIndex(x);
      GetReBarCtrl().GetBandInfo(index, &rbbi);

      if (tbIndex[x]->name){
         sprintf(tempPref + 17, _T("%s.size"), tbIndex[x]->name);
         theApp.preferences.SetInt(tempPref, rbbi.cx);

         sprintf(tempPref + 17, _T("%s.index"), tbIndex[x]->name);
         theApp.preferences.SetInt(tempPref, index);

         sprintf(tempPref + 17, _T("%s.visibility"), tbIndex[x]->name);
         theApp.preferences.SetBool(tempPref, tbIndex[x]->visibility);

         sprintf(tempPref + 17, _T("%s.break"), tbIndex[x]->name);
         theApp.preferences.SetInt(tempPref, rbbi.fStyle & RBBS_BREAK);
      }
   }
}

void CReBarEx::RestoreBandSizes(){
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
   for (x=0; x<tbCount; x++) {
      int index = FindByIndex(x);

      rbbi.fMask = 0;

      sprintf(tempPref + 17, _T("%s.break"), tbIndex[x]->name);
      barbreak = theApp.preferences.GetInt(tempPref, 0);
      if (barbreak){
         rbbi.fMask |= RBBIM_STYLE;
         GetReBarCtrl().GetBandInfo(FindByIndex(x), &rbbi);
         rbbi.fStyle |= RBBS_BREAK;
      }

      sprintf(tempPref + 17, _T("%s.size"), tbIndex[x]->name);
      rbbi.cx = theApp.preferences.GetInt(tempPref, 0);
      if (rbbi.cx > 0)
         rbbi.fMask |= RBBIM_SIZE;

      sprintf(tempPref + 17, _T("%s.index"), tbIndex[x]->name);
      int ndx = theApp.preferences.GetInt(tempPref, -1);
      if (ndx >= 0)
         if (index != -1) {
            GetReBarCtrl().MoveBand(index, ndx);
            GetReBarCtrl().SetBandInfo(ndx, &rbbi);
         }

      sprintf(tempPref + 17, _T("%s.visibility"), tbIndex[x]->name);
      if (!theApp.preferences.GetBool(tempPref, TRUE))
      SetVisibility(x, FALSE);
   }
}