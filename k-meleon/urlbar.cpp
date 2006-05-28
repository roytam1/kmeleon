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
#include "BrowserFrm.h"
#include ".\urlbar.h"
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

NS_IMPL_ISUPPORTS1(CACListBox, nsIAutoCompleteListener)

NS_IMETHODIMP CACListBox::OnStatus(const PRUnichar *statusText){
	return NS_OK;
}


NS_IMETHODIMP CACListBox::OnAutoComplete(nsIAutoCompleteResults *result, AutoCompleteStatus status)
{
	m_oldResult = result; // Keep the old result for optimization
	ResetContent();
	if (status == nsIAutoCompleteStatus::matchFound)
	{
		nsCOMPtr<nsISupportsArray> array;
		result->GetItems(getter_AddRefs(array));
		array->EnumerateForwards( CACListBox::enumStatic, this);
		
		int nLine = GetCount();
		int height = GetItemHeight(0);
		
		CRect rec;
		m_edit->GetParent()->GetWindowRect(rec);
		
		//rec.bottom += ::GetSystemMetrics(SM_CYEDGE)*2;

		theApp.m_pMostRecentBrowserFrame->ScreenToClient(rec);

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
		//if (!IsWindowVisible())
		m_ignoreMousemove = 2;

		SetWindowPos(&(CWnd::wndTop),rec.left,rec.bottom,rec.Width(),height*nLine,SWP_SHOWWINDOW);
	}
	else
	{
		if (IsWindowVisible())
			ShowWindow(SW_HIDE);
	}
	return PR_TRUE;
}

NS_IMETHODIMP CACListBox::GetParam(nsISupports * *aParam){
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CACListBox::SetParam(nsISupports * aParam){
	return NS_OK;
}

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

PRBool CACListBox::AddEltToList(nsISupports* aElement)
{
	nsCOMPtr<nsIAutoCompleteItem> acItem = do_QueryInterface(aElement);
	if (!acItem)
		return PR_FALSE;
	
	nsEmbedString nsStr;
	acItem->GetValue(nsStr);
#ifdef _UNICODE
	CString cstr(nsStr.get());
#else
	nsEmbedCString nsCStr;
	NS_UTF16ToCString(nsStr,NS_CSTRING_ENCODING_ASCII,nsCStr);
	CString cstr(nsCStr.get());
#endif
	
	if (GetCount() == 0 && theApp.preferences.GetBool("browser.urlbar.autoFill", false) && !m_bBack)
	{
		CString text,toSelect;
		m_edit->GetWindowText(text);
		toSelect = cstr;
		int start = toSelect.Find(text,0);
        toSelect.Delete(0, start+text.GetLength());
		int i;
		if ( (i = toSelect.FindOneOf(_T("/?&=#"))) != -1)
			toSelect.Truncate(i+1);
		m_edit->SetWindowText(text+toSelect);
		m_edit->SetSel(text.GetLength(), text.GetLength() + toSelect.GetLength(), TRUE);
	}

	AddString(cstr);
	
	return PR_TRUE;
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
	ResetContent();
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

/////////////////////////////////////////////////////////////////////////////
// CUrlBarEdit
/////////////////////////////////////////////////////////////////////////////

CUrlBarEdit::CUrlBarEdit()
{
	oldResult = NULL;
	m_list = NULL;
	//urlbar = GetParent()->GetParent();
}

CUrlBarEdit::~CUrlBarEdit()
{
}

void CUrlBarEdit::StopACSession()
{
	KillTimer(1);
	oldResult = nsnull;
	
	if (m_AutoComplete)	m_AutoComplete->OnStopLookup();
	
	if (m_list&&IsWindow(m_list->m_hWnd))
	{
		m_list->ResetContent();
		m_list->ShowWindow(SW_HIDE);
		m_list->m_oldResult = nsnull;
	}
}

void CUrlBarEdit::OnTimer(UINT nIDEvent)
{
	nsresult rv;
	m_AutoComplete = do_GetService(NS_GLOBALHISTORY_AUTOCOMPLETE_CONTRACTID, &rv);
	if (NS_FAILED(rv)) return;

	CString text;
    USES_CONVERSION;

	KillTimer(nIDEvent);
	GetWindowText(text);

	// I'm not using oldresult to partially fix a weird behavior
	// of mozilla who always cut prefix of url before searching  
	// (so if you type 'w', there will be no result beginning by "www.") 
	// whatever the search string is. A setting to change?
	m_AutoComplete->OnStartLookup(T2CW(text), nsnull /*m_list->m_oldResult*/, m_list);
		
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
	
	if (nChar == VK_BACK) 
		m_list->m_bBack = TRUE;
	else
		m_list->m_bBack = FALSE;

    // We don't want to autocomplete if ctrl is pressed 
	if (!(GetKeyState(VK_CONTROL) & 0x8000))
		SetTimer(1, 100, NULL);


	return;
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
    }
	else
	{
		m_list->SetFont(GetFont());
		m_list->ShowWindow(SW_HIDE);
	}
	CEdit::PreSubclassWindow();
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
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(-1,1);
		return;

	case VK_NEXT:
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(1,1);
		return;

	case VK_UP:
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(-1);
		return;

	case VK_DOWN:
		if (!m_list->IsWindowVisible()) break;
		m_list->Scroll(1);
		return;
	case VK_DELETE: {
		m_list->m_bBack = TRUE;
		SetTimer(1, 100, NULL);
		}


	/*
    	case VK_TAB:
		GetWindowText(str);
		if (str.Length>0 && m_list->GetCount()>0)
		{
			if (m_list->GetCurSel() != LB_ERR)
			{
				m_list->GetCurSel();
				m_list->SetCurSel(m_list->GetCurSel()+1);
				m_list->GetText(m_list->GetCurSel(),str);
				SetWindowText(str);
			}
			else
			{
				m_list->SetCurSel(0);
				m_list->GetText(0, str);
				SetWindowText(str);
			}
		}
		return; */

		
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
	if (newPos != len) newPos--; // We don't want to select the delimiter

	SetSel(
		UrlBreakProc(T2W(text.GetBuffer(0)), pos, text.GetLength(), WB_LEFT),
		newPos,
		FALSE);
}
#endif

BOOL CUrlBarEdit::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_list->OnMouseWheel(nFlags, zDelta, pt);
	return true;
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
					return TRUE;
				case L'/':
					if (ichCurrent+1<cchEditText &&
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
			str.Delete(newPos,cursorPos);
			SetWindowText(str);
			SetSel(newPos,newPos,TRUE); 
			return TRUE;
		}
		else if (pMsg->wParam == VK_ESCAPE)
		{
			StopACSession();
			CString oldUrl;
			((CBrowserFrame*)GetParentFrame())->m_wndBrowserView.GetCurrentURI(oldUrl);
			SetWindowText(oldUrl);
			SetSel(0,-1,FALSE);
			return TRUE;
		}
	}
		
	return CEdit::PreTranslateMessage(pMsg);
}
BEGIN_MESSAGE_MAP(CUrlBar, CComboBoxEx)
	ON_WM_CTLCOLOR()
#ifdef INTERNAL_SITEICONS
	ON_NOTIFY_REFLECT(CBEN_GETDISPINFO, OnCbenGetdispinfo)
#endif
END_MESSAGE_MAP()

HBRUSH CUrlBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// Use the background color depending on the security 
	// state except for the list box
	if (nCtlColor != CTLCOLOR_LISTBOX)
	{
		pDC->SetBkColor(m_crBkclr[m_HighlightType]);
		return m_brBkgnd[m_HighlightType];    
	}

	HBRUSH hbr = CComboBoxEx::OnCtlColor(pDC, pWnd, nCtlColor);
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
	GetParentFrame()->SendMessage(WM_COMMAND, MAKEWPARAM(ID_URL_BAR,CBN_SETFOCUS), (LPARAM)m_hWnd);
}
