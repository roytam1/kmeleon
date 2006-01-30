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
	m_oldResult = result;
	ResetContent();
	if (status == nsIAutoCompleteStatus::matchFound)
	{
		nsCOMPtr<nsISupportsArray> array;
		result->GetItems(getter_AddRefs(array));
		array->EnumerateForwards( CACListBox::enumStatic, this);
		
		int nLine = GetCount();
		int height = GetItemHeight(0);
		
		CRect rec;
		m_edit->GetWindowRect(rec);
		
		rec.bottom += ::GetSystemMetrics(SM_CYEDGE)*2;

		theApp.m_pMostRecentBrowserFrame->ScreenToClient(rec);

		if(nLine > 6) 
		{
			nLine = 6;
			rec.right += ::GetSystemMetrics(SM_CXVSCROLL);
		}
				
		nLine++;
		
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

PRBool CACListBox::AddEltToList(nsISupports* aElement)
{
	nsCOMPtr<nsIAutoCompleteItem> acItem = do_QueryInterface(aElement);
	if (!acItem)
		return PR_FALSE;
	
	nsEmbedString nsStr;
	nsEmbedCString nsCStr;

	acItem->GetValue(nsStr);
	NS_UTF16ToCString(nsStr,NS_CSTRING_ENCODING_ASCII,nsCStr);
	CString cstr(nsCStr.get());
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
		::SendMessage(theApp.m_pMostRecentBrowserFrame->m_hWnd, WM_COMMAND, MAKEWORD(IDOK,0), (LPARAM)m_hWnd);
		return;
	}

	m_edit->SetWindowText(str);
}

void CACListBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	CListBox::OnLButtonDown(nFlags, point);
	int pos = GetCurSel();
	
	if(pos != LB_ERR)
	{
		CString str;
		GetText(pos,str);
		m_edit->SetWindowText(str);
		::SendMessage(theApp.m_pMostRecentBrowserFrame->m_hWnd, WM_COMMAND, MAKEWORD(IDOK,0), (LPARAM)m_hWnd);
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
	// Useless
	//if (m_AutoComplete)	m_AutoComplete->OnStopLookup();
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

	m_AutoComplete->OnStartLookup(T2CW(text), nsnull/*m_list->m_oldResult*/, m_list);
		
	CEdit::OnTimer(nIDEvent);
}

void CUrlBarEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CEdit::OnChar(nChar, nRepCnt, nFlags);

	if (nChar == VK_RETURN)
	{
		StopACSession();
		::SendMessage(theApp.m_pMostRecentBrowserFrame->m_hWnd, WM_COMMAND, MAKEWORD(IDOK,0), (LPARAM)m_hWnd);
		return;
	}

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
		theApp.m_pMostRecentBrowserFrame,
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
	case VK_UP:
	case VK_DOWN:
		if (!m_list->IsWindowVisible()) break;
		if (m_list->GetCurSel() == LB_ERR)
		{
			if (nChar == VK_DOWN)
				m_list->SetCurSel( m_list->GetTopIndex());
			else
				m_list->SetCurSel(m_list->GetCount()-1);
		
		}
		else
			if (nChar == VK_DOWN)
				m_list->SetCurSel( m_list->GetCurSel() + 1);
			else
				m_list->SetCurSel( m_list->GetCurSel() - 1);

		m_list->GetText(m_list->GetCurSel(), str);
		SetWindowText(str);
		::SetFocus(m_list->m_hWnd);
		return;

	case VK_ESCAPE:	{
		USES_CONVERSION;
		StopACSession();
		char* oldUri = new char[theApp.m_pMostRecentBrowserFrame->m_wndBrowserView.GetCurrentURI(NULL)+1];
		theApp.m_pMostRecentBrowserFrame->m_wndBrowserView.GetCurrentURI(oldUri);
		SetWindowText(A2T(oldUri));
		delete oldUri;
		return;
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
			//the character following it, the delimiter, become
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
		
		case WB_RIGHT:
			if (ichCurrent>cchEditText-1) break;
			{/*VC6*/for (int i=ichCurrent;i<cchEditText;i++)
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
	if (pMsg->message==WM_KEYDOWN && pMsg->wParam == VK_BACK
		&& GetKeyState(VK_CONTROL) & 0x8000 ) 
	{// Bug #764 
		USES_CONVERSION;
		GetWindowText(str);
		int cursorPos = HIWORD(GetSel());
		int newPos = UrlBreakProc(T2W(str), cursorPos, str.GetLength(),WB_LEFT);
		str.Delete(newPos,cursorPos);
		SetWindowText(str);
		SetSel(newPos,newPos,true); 
		return TRUE;
	}
		
	return CEdit::PreTranslateMessage(pMsg);
}
