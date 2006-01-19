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

#include "StdAfx.h"
#include "Tooltips.h"

// our tooltip window class

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;


BEGIN_MESSAGE_MAP(CKmToolTip, CWnd)
	//{{AFX_MSG_MAP(CKmToolTip)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CKmToolTip::CKmToolTip() {
   m_pszWndClass = AfxRegisterWndClass(CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW);
   pszText = NULL;
   
   NONCLIENTMETRICS ncm = {0};
   ncm.cbSize = sizeof(ncm);
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,(PVOID)&ncm,FALSE);
   m_pFont = CreateFontIndirect(&ncm.lfStatusFont);
}

void CKmToolTip::Create(CWnd *pWnd) {
   CWnd::Create(m_pszWndClass, NULL, WS_CHILD | WS_BORDER,
      CRect(10,10,150,50), pWnd, NULL, NULL);
}

CKmToolTip::~CKmToolTip() {
   DestroyWindow();
   UnregisterClass(m_pszWndClass, theApp.m_hInstance);
   DeleteObject(m_pFont);
   delete pszText;
}

void CKmToolTip::Hide() {
   ShowWindow(SW_HIDE);
   delete pszText;
   pszText = NULL;
}

void CKmToolTip::Show(const TCHAR *text, int x, int y) {
   
   delete pszText;
   pszText = _tcsdup(text);
   
   // measure the size of the text
   CDC *DC = GetWindowDC();
   int nSavedDC = DC->SaveDC();

   DC->SelectObject(m_pFont);
   CRect rect;
   rect.top = 0;
   rect.bottom = 0;
   rect.left = 0;
   rect.right = 300; // max width of the tooltip

   DC->DrawText(text, rect, DT_CALCRECT | DT_WORDBREAK );
   DC->RestoreDC(nSavedDC);
   
   // add a bit of padding on the sides
   int width = rect.Width() + 10;
   int height = rect.Height() +5;
   
   // keep the tooltip from running off the window
   CWnd *pWndParent = GetParent();
   pWndParent->GetWindowRect(&rect);
   pWndParent->ScreenToClient(&rect);
   if ( x + width > rect.right)
      x = rect.right - width;
   if (y + height > rect.bottom)
      y = rect.bottom - height;
   
   SetWindowPos(NULL, x, y, width, height, SWP_NOZORDER /* | SWP_DRAWFRAME*/);
   ShowWindow(SW_SHOW);
}


void CKmToolTip::OnPaint() {      
   CPaintDC DC(this);
   int nSavedDC = DC.SaveDC();
   
   // Draw background
   COLORREF clrBackground = RGB(255, 255, 255);
   clrBackground = ::GetSysColor(COLOR_INFOBK);
   CRect ClientRect;
   GetClientRect(ClientRect);
   DC.Rectangle(ClientRect);
   ClientRect.DeflateRect(1,1,1,1);
   DC.FillSolidRect(ClientRect, clrBackground);
   
   // Draw text
   DC.SelectObject(m_pFont);
   DC.SetTextColor(::GetSysColor(COLOR_INFOTEXT));
   DC.SetBkMode(TRANSPARENT);
   
   ClientRect.left += 3;
   
   DC.DrawText(pszText, -1, ClientRect, DT_WORDBREAK);
   
   DC.RestoreDC(nSavedDC);
}

