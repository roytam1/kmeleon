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

#if _MSC_VER > 1000
#pragma once
#endif

class CReBarEx : public CReBar {
public:
   CReBarEx();
   ~CReBarEx();
   void RegisterBand(HWND hWnd, char *name, int visibleOnMenu);
   void DrawToolBarMenu();
   void ToggleVisibility(int index);
   void SaveBandSizes();
   void RestoreBandSizes();
   BOOL GetVisibility(int index);
   void SetVisibility(int index, BOOL visibility);

   void LockBars(BOOL lock);

public:
   HMENU m_menu;

private:
   int FindByChild (HWND hWnd);
   int FindByName  (char *name);
   int FindByIndex (int index);
   int ChildToListIndex(HWND hWnd);

   struct tbBand {
      UINT uID;
      char *name;
      HWND hWnd;
      BOOL visibility;
      BOOL visibleOnMenu;
   } **m_index;
   int m_iCount;
};