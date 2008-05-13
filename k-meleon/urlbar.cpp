/*
*  Copyright (C) 2005 Dorian Boissonnade 
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
*
*
*/


#include "stdafx.h"
#include "urlbar.h"
#include "kmeleon_plugin.h"
#include <wininet.h>

BEGIN_MESSAGE_MAP(CUrlBarEdit, CEdit)
	ON_WM_CHAR()
	ON_WM_TIMER()
	ON_WM_GETDLGCODE()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CACListBox, CListBox)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CACListBox
/////////////////////////////////////////////////////////////////////////////

void CACListBox::Scroll(short dir, short q)
{
	if (!IsWindowVisible()) 
		return;
	
	if (GetCurSel() == LB_ERR || q==2)
	{
		if (dir>0)
			SetCurSel(0);
		else
			SetCurSel(GetCount()-1);
	}
    else
	{
		if (q == 1) 
			dir = dir * theApp.preferences.GetInt("kmeleon.urlbar.dropdown_lines", 6);

		int newsel = GetCurSel() + dir;
		if (newsel < 0) 
			newsel = 0;
		else if (newsel >=GetCount())
			newsel = GetCount()-1;
		SetCurSel(newsel);
	}
	CString str;
	GetText(GetCurSel(), str);
	m_edit->SetWindowText(str);

	//Set the cursor at the end of the text for easy editing
	m_edit->SetSel(str.GetLength(),str.GetLength(),true);
}

BOOL CACListBox::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	return CListBox::OnMouseWheel(nFlags, zDelta, pt);
}

void CACListBox::OnKillFocus(CWnd* pNewWnd)
{
	if (pNewWnd == this) return;
	CListBox::OnKillFocus(pNewWnd);
	ShowWindow(SW_HIDE);
	((CUrlBarEdit*)m_edit)->StopACSession();
}
/*
void CACListBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	
	CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
	
	if (nChar == VK_ESCAPE)
	{
		m_edit->SetFocus();
		return;
	}

	int pos = GetCurSel();
	if (pos == LB_ERR) return;
	
	CString str;
	GetText(pos,str);

	if (nChar == VK_RETURN)
	{
		m_edit->SetWindowText(str);
		GetParentFrame()->SendMessage(WM_COMMAND, MAKEWORD(IDOK,0), (LPARAM)m_hWnd);
		return;
	}

	m_edit->SetWindowText(str);
}
*/
void CACListBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	CListBox::OnLButtonDown(nFlags, point);
	int pos = GetCurSel();
	
	if(pos != LB_ERR)
	{
		CString str;
		GetText(pos,str);
		m_edit->SetWindowText(str);
		GetParentFrame()->SendMessage(WM_COMMAND, MAKEWORD(IDOK,0), (LPARAM)m_hWnd);
	}
}

int CACListBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_edit = (CEdit*)lpCreateStruct->lpCreateParams;	
	return 0;
}

void CACListBox::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_ignoreMousemove>0){
		m_ignoreMousemove--;
		return;
	}
	
	CListBox::OnMouseMove(nFlags, point);
	BOOL b;
	SetCurSel(ItemFromPoint(point, b));
}

void CACListBox::AutoComplete(CString& text)
{
	AutoCompleteResult *results = NULL;

	USES_CONVERSION;
	int count = theApp.plugins.SendMessage("*", "Urlbar", "AutoComplete", (long)T2CA(text), (long)&results);
	ResetContent();
	
	if (count && results)
	{
		USES_CONVERSION;
		if (theApp.preferences.GetBool("browser.urlbar.autoFill", false) && !m_bBack)
		{
			CString text,toSelect;
			m_edit->GetWindowText(text);
			toSelect = A2CT(results->value);
			int start = toSelect.Find(text,0);
			toSelect.Delete(0, start+text.GetLength());
			int i;
			if ( (i = toSelect.FindOneOf(_T("/?&=#"))) != -1) {
				toSelect.GetBuffer(0);
				toSelect.ReleaseBuffer(i+1); // Truncate
			}
			m_edit->SetWindowText(text+toSelect);
			m_edit->SetSel(text.GetLength(), text.GetLength() + toSelect.GetLength(), TRUE);
		}

		for (int i=0; i<count; i++)
		{
			AddString(A2CT(results->value));
			results++;
		}
		
		int nLine = GetCount();
		int height = GetItemHeight(0);
		
		CRect rec;
		m_edit->GetParent()->GetWindowRect(rec);
		
		//rec.bottom += ::GetSystemMetrics(SM_CYEDGE)*2;

		GetParentFrame()->ScreenToClient(rec);

		int maxline = theApp.preferences.GetInt("kmeleon.urlbar.dropdown_lines", 6);
		if(nLine > maxline) 
		{
			nLine = maxline;
			//rec.right += ::GetSystemMetrics(SM_CXVSCROLL); // size of the scrollbar
		}
		
		// I'm not sure what I'm missing but it works with that
		nLine++;
		
		// Prevent the mouse to select an element when the 
		// autocomplete list popup.
		if (!IsWindowVisible())
			m_ignoreMousemove = 2;

		SetWindowPos(&(CWnd::wndTop),rec.left,rec.bottom,rec.Width(),height*nLine,SWP_SHOWWINDOW);
	}
	else
	{
		if (IsWindowVisible())
			ShowWindow(SW_HIDE);
	}
	m_bBack = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CUrlBarEdit
/////////////////////////////////////////////////////////////////////////////

CUrlBarEdit::CUrlBarEdit()
{
	m_list = NULL;
	//urlbar = GetParent()->GetParent();
}

CUrlBarEdit::~CUrlBarEdit()
{
}

void CUrlBarEdit::StopACSession()
{
	KillTimer(1);
	
	theApp.plugins.SendMessage("*", "Urlbar", "AutoComplete", (long)"", (long)0);
	
	if (m_list&&IsWindow(m_list->m_hWnd))
	{
		m_list->ResetContent();
		m_list->ShowWindow(SW_HIDE);
	}
}

void CUrlBarEdit::OnTimer(UINT nIDEvent)
{
	if (!m_list)
		return;

	KillTimer(nIDEvent);
	CString text;
	GetWindowText(text);
	m_list->AutoComplete(text);
		
	CEdit::OnTimer(nIDEvent);
}

void CUrlBarEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CEdit::OnChar(nChar, nRepCnt, nFlags);

	if (nChar == VK_RETURN)
	{
		StopACSession();
		GetParentFrame()->SendMessage(WM_COMMAND, MAKEWPARAM(IDOK,0), (LPARAM)m_hWnd);
		return;
	}

    // We don't want to autocomplete if ctrl is pressed 
	if (!(GetKeyState(VK_CONTROL) & 0x8000))
		SetTimer(1, 100, NULL);
}

UINT CUrlBarEdit::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
	//return CEdit::OnGetDlgCode();
}

void CUrlBarEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	if(m_list && IsWindow(m_list->m_hWnd) && pNewWnd && pNewWnd->m_hWnd != m_list->m_hWnd )
		StopACSession();
}

void CUrlBarEdit::PreSubclassWindow()
{	
	CEdit::PreSubclassWindow();
	
	if (!theApp.preferences.GetBool("browser.urlbar.autocomplete.enabled", true))
		return;

	m_list = new CACListBox();
	if (!m_list->CreateEx(
		WS_EX_TOPMOST|WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE,
		_T("ListBox"),_T("AutoComplete List"),
		LBS_NOTIFY|WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL,
		CRect(0,0,0,0),
		//GetDesktopWindow(),
		GetParentFrame(),
		0,this))
	{
		TRACE0("Failed to create ACListBox\n");
		delete m_list;
		m_list = NULL;
		return;
	}

	m_list->SetFont(GetFont());
	m_list->ShowWindow(SW_HIDE);
}

void CUrlBarEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CString str;
	switch (nChar)
	{
#ifndef URLBAR_USE_SETWORDBREAKPROC	
	// Since the EM_SETWORDBREAKPROC doesn't work on W9x I'm implementing
	// my own Ctrl+Arrow and double click handler.
	case VK_RIGHT:
	case VK_LEFT:
		if ((GetKeyState(VK_CONTROL) & 0x8000))
		{
			int sel = GetSel();
			WORD cursorPos = HIWORD(sel);
			WORD cursorStart = LOWORD(sel);
			
			CString text;
			GetWindowText(text);
			int len = text.GetLength();

			if ((cursorStart == len && cursorPos == len && nChar == VK_RIGHT) || 
				(cursorStart == 0 && cursorPos == 0 && nChar == VK_LEFT))
				return;

			int dir = (nChar == VK_RIGHT ? WB_RIGHT : WB_LEFT);

			// Get the caret position. I set the caret to the right or to the left
			// of the selection depending of the "direction" of the selection
			CPoint ptCaret = GetCaretPos();
			WORD caretPos = LOWORD(SendMessage(EM_CHARFROMPOS, 0, MAKELPARAM(ptCaret.x, ptCaret.y)));
			if (caretPos == -1) return; // 

			if (caretPos != cursorPos && caretPos!=cursorStart)
				// W9x shitty hack
				caretPos = (cursorPos - caretPos) > (caretPos - cursorStart) ? cursorStart : cursorPos ;
			
			USES_CONVERSION;
						
			if ((GetKeyState(VK_SHIFT) & 0x8000))
			{
				int newPos = UrlBreakProc(T2W(text.GetBuffer(0)), caretPos, len, dir);
				if (caretPos == cursorPos)
				{
					if (newPos < cursorStart)
						// The selection is beggining in the middle of a "word"
						// and so it's going to change side.
						cursorPos = cursorStart;
					SetSel(cursorStart, newPos, FALSE);
				}
				else
					SetSel(newPos, cursorPos, FALSE);
				
				
				// Set the caret to the new position ** Totally Useless **
				//int newCaretPos = SendMessage(EM_POSFROMCHAR, newPos, 0);
				//if (newCaretPos!=-1)
				//	SetCaretPos(CPoint(LOWORD(newCaretPos), HIWORD(newCaretPos)));

				// ** HACK **
				// There is no way to set the caret to the beginning of the selection.
				// So I'm sending mouse messages like we're doing a selection with the
				// mouse to simulate this behavior
				
				if ( (caretPos == cursorStart && newPos < cursorPos)
					|| (nChar==VK_LEFT && newPos < cursorStart) )

				{
					int newMousePos;
					if ( cursorPos == len )
					// Since EM_POSFROMCHAR return always -1 for the last character *sigh*
					// I'm hacking my way to get a somewhat correct value
					{
						newMousePos = SendMessage(EM_POSFROMCHAR, cursorPos-1, 0);
						newMousePos = MAKELPARAM(LOWORD(newMousePos)+20, HIWORD(newMousePos));
					}
					else
						newMousePos = SendMessage(EM_POSFROMCHAR, cursorPos, 0);
					
					SendMessage(WM_LBUTTONDOWN, 0, newMousePos);
				
					newMousePos = SendMessage(EM_POSFROMCHAR, newPos, 0);
					newMousePos = MAKELPARAM(LOWORD(newMousePos)-1, HIWORD(newMousePos)); // Another shitty W9x hack
					SendMessage(WM_MOUSEMOVE, 0, newMousePos);
					SendMessage(WM_LBUTTONUP, 0, newMousePos);
				}
			}
			else
			{
				int newPos = UrlBreakProc(T2W(text.GetBuffer(0)), caretPos, len, dir);
				SetSel(newPos, newPos, FALSE);
			}
			return;
		}
#endif
		break;

	/*case VK_END:
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(-1, 2);
		return;

	case VK_HOME:
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(1, 2);
		return;*/

	case VK_PRIOR:
		if (!m_list || !m_list->IsWindowVisible()) break;
		m_list->Scroll(-1,1);
		return;

	case VK_NEXT:
		if (!m_list || !m_list->IsWindowVisible()) break;
		m_list->Scroll(1,1);
		return;

	case VK_UP:
		if (!m_list || !m_list->IsWindowVisible()) break;
		m_list->Scroll(-1);
		return;

	case VK_DOWN:
		if (!m_list || !m_list->IsWindowVisible()) break;
		m_list->Scroll(1);
		return;
	
   case VK_BACK:
   case VK_DELETE:
		if (!m_list) break;
		m_list->m_bBack = TRUE;
		SetTimer(1, 100, NULL);
   
   case 'A':
      if ((GetKeyState(VK_CONTROL) & 0x8000))
         this->SetSel(0,-1, 0);
   }

   CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

#ifndef URLBAR_USE_SETWORDBREAKPROC	
// Used to select a "word"
void CUrlBarEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int pos = LOWORD(SendMessage(EM_CHARFROMPOS, 0, MAKELPARAM(point.x, point.y)));
	
	CString text;
	GetWindowText(text);
	int len = text.GetLength();

	USES_CONVERSION;
	
	int newPos = UrlBreakProc(T2W(text.GetBuffer(0)), pos, text.GetLength(), WB_RIGHT);
	if (newPos>0) {
		if (newPos != len) newPos--; // We don't want to select the delimiter
		else {
			if (UrlBreakProc(T2W(text.GetBuffer(0)), newPos, text.GetLength(), WB_ISDELIMITER))
				newPos--;
		}
	}

	SetSel(
		UrlBreakProc(T2W(text.GetBuffer(0)), pos, text.GetLength(), WB_LEFT),
		newPos,
		FALSE);
}
#endif

BOOL CUrlBarEdit::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_list && m_list->IsWindowVisible())
		m_list->OnMouseWheel(nFlags, zDelta, pt);
	return TRUE;
}

void CUrlBarEdit::OnDestroy()
{
	if (m_list&&IsWindow(m_list->m_hWnd))
	{
		m_list->DestroyWindow();
		delete m_list;
	}
	CEdit::OnDestroy();
}

int CALLBACK CUrlBarEdit::UrlBreakProc(LPWSTR lpszEditText, int ichCurrent,
                                int cchEditText, int wActionCode)
{
	switch (wActionCode) {
		case WB_ISDELIMITER:

			//Because the cursor is set on the delimiter, not on 
			//the character following it, the delimiter have to be
			//the next character.
			ichCurrent--;

			switch (lpszEditText[ichCurrent]) {
				case L'.':
				case L'?':
				case L'&':
				case L'=':
				case L'#':
				case L' ':
					return TRUE;
				case L'/':
					if (ichCurrent+1 >= cchEditText ||
					    lpszEditText[ichCurrent+1]!=L'/') return TRUE;
			}
			return FALSE;

		case WB_LEFT:
			if (ichCurrent<2) return 0;
			{/*VC6*/for (int i=ichCurrent-1;i>0;i--)
			{
				if (UrlBreakProc(lpszEditText,i,cchEditText,WB_ISDELIMITER))
					return i;
			}}
			return 0;
		
		case WB_RIGHT: {
			if (ichCurrent>cchEditText-1) break;
			
			// i should be initialised to ichCurrent if using windows internal
			// routine. Windows call this function twice, one  time with ichCurrent
			// = the current position and the second time with ichCurrent = the 
			// result of the previous call + 1
			
#ifdef URLBAR_USE_SETWORDBREAKPROC
			for (int i=ichCurrent;i<cchEditText;i++)
#else
			for (int i=ichCurrent+1;i<cchEditText;i++)
#endif
			{
				if (UrlBreakProc(lpszEditText,i,cchEditText,WB_ISDELIMITER))
					return i;
			}}
			return cchEditText;
	}
	return ichCurrent;
}

BOOL CUrlBarEdit::PreTranslateMessage(MSG* pMsg)
{
	CString str;
	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_BACK && GetKeyState(VK_CONTROL) & 0x8000 ) 
		{// Bug #764 
			USES_CONVERSION;
			GetWindowText(str);
			int cursorPos = HIWORD(GetSel());
			int newPos = UrlBreakProc(T2W(str.LockBuffer()), cursorPos, str.GetLength(),WB_LEFT);
			str.UnlockBuffer();
			str.Delete(newPos,cursorPos-newPos);
			SetWindowText(str);
			SetSel(newPos,newPos,TRUE); 
			return TRUE;
		}
		else if (pMsg->wParam == VK_ESCAPE)
		{
			StopACSession();
			CString url;
			((CUrlBar*)GetParent()->GetParent())->ResetURL();
			SetSel(0,-1,FALSE);
			return TRUE;
		}
		else if (pMsg->wParam == VK_TAB)
		{
			CString str;
			GetWindowText(str);
			if (m_list && str.GetLength()>0 && m_list->GetCount()>0)
			{
				int curSel = m_list->GetCurSel();
				if (curSel != LB_ERR)
				{
					curSel = (curSel == m_list->GetCount()-1 ? 0 : curSel+1);
					m_list->SetCurSel(curSel);
					m_list->GetText(curSel, str);
					SetWindowText(str);
				}
				else
				{
					m_list->SetCurSel(0);
					m_list->GetText(0, str);
					SetWindowText(str);
				}
				return TRUE;
			}
		}
	}
		
	return CEdit::PreTranslateMessage(pMsg);
}
BEGIN_MESSAGE_MAP(CUrlBar, CComboBoxEx)
	ON_WM_CTLCOLOR()
#ifdef INTERNAL_SITEICONS
	ON_NOTIFY_REFLECT(CBEN_GETDISPINFO, OnCbenGetdispinfo)
#endif
	ON_NOTIFY_REFLECT(CBEN_ENDEDIT, OnCbenEndedit)
	ON_CONTROL_REFLECT(CBN_EDITCHANGE, OnCbnEditchange)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnCbnSelchange)
END_MESSAGE_MAP()

HBRUSH CUrlBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CComboBoxEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// Use the background color depending on the security 
	// state except for the list box
	if (nCtlColor != CTLCOLOR_LISTBOX)
	{
		pDC->SetBkColor(m_crBkclr[m_HighlightType]);
		hbr = m_brBkgnd[m_HighlightType];    
	}
	
	return hbr;
}

#ifdef INTERNAL_SITEICONS
void CUrlBar::OnCbenGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMCOMBOBOXEX pCBEx = reinterpret_cast<PNMCOMBOBOXEX>(pNMHDR);
	USES_CONVERSION;

	CString url;
	nsCOMPtr<nsIURI> URI;

	/*if (pCBEx->ceItem.iItem == -1)
	{
		int l = frame->m_wndBrowserView.GetCurrentURI(NULL);
		if (l>0){
			char* oldUri = new char[l+1];
			frame->m_wndBrowserView.GetCurrentURI(oldUri);
			url = oldUri;
			delete oldUri;
		}
		else
			GetWindowText(url);
	}
	else*/
	if (pCBEx->ceItem.iItem != -1)
	{
		//url = W2T(pCBEx->ceItem.pszText); 
		GetLBText(pCBEx->ceItem.iItem, url);
		pCBEx->ceItem.iImage = pCBEx->ceItem.iSelectedImage = theApp.favicons.GetHostIcon(url);
	}
	
	*pResult = 0;
}
#endif

void CUrlBarEdit::OnSetFocus(CWnd* pOldWnd)
{
	// Because the combobox is shitty and send focus notification 
	// only when you click on the arrow.
	CEdit::OnSetFocus(pOldWnd);
	//GetParentFrame()->SendMessage(WM_COMMAND, MAKEWPARAM(ID_URL_BAR,CBN_SETFOCUS), (LPARAM)m_hWnd);
}

void CUrlBar::OnCbenEndedit(NMHDR *pNMHDR, LRESULT *pResult)
{
	TRACE0("EditChanged FALSE in OnCbenEndedit\n");
	EditChanged(FALSE);
	*pResult = 0;
}

void CUrlBar::OnCbnEditchange()
{
	TRACE0("EditChanged TRUE in OnCbnEditchange\n");
	EditChanged(TRUE);
}

void CUrlBar::OnCbnSelchange()
{
	TRACE0("EditChanged TRUE in OnCbnSelchange\n");
	EditChanged(FALSE);
}
