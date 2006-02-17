/*
*  Copyright (C) 2000 Brian Harris
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

#pragma once

#include "stdafx.h"

class CKmToolTip :  public CWnd {

public:
   CKmToolTip();
   void Create(CWnd *pWnd);
   ~CKmToolTip();
   void Hide();
   void Show(const TCHAR *text, int x, int y);

   TCHAR *pszText;

protected:
	//{{AFX_MSG(CKmToolTip)
   afx_msg void OnPaint();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:
   LPCTSTR m_pszWndClass;
   HFONT  m_pFont;

};
