/*
*  
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

#pragma once

#include "mfcembed.h"
struct _AutoCompleteResult;
extern CMfcEmbedApp theApp;
class CACListener;

// Have to use my own list box because the combobox is buggy
class CACListBox : public CListBox
{
public:
	void Scroll(short dir, short q = 0);
	void AutoComplete(CString&);
	void OnResult(_AutoCompleteResult* result, int count);

	BOOL m_bBack;
	DECLARE_MESSAGE_MAP()
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
public:
   afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	

protected:
	CEdit *m_edit;
	CFont m_font;		
	int m_ignoreMousemove;
	static void CALLBACK ACCallback(_AutoCompleteResult* result, int count, void* self) {
		((CACListBox*)self)->OnResult(result, count);
	}
};

class CUrlBarEdit : public CEdit
{
protected:
	CString m_ACStr;
	CACListBox* m_list;

public:
	CUrlBarEdit();
	~CUrlBarEdit();
	int static CALLBACK UrlBreakProc(LPWSTR lpszEditText, int ichCurrent,
                                int cchEditText, int wActionCode);
	void StopACSession();
	bool m_AntiLazyIdiot;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnSetText(WPARAM w, LPARAM l);
	afx_msg LRESULT OnGetText(WPARAM w, LPARAM l);
#ifndef URLBAR_USE_SETWORDBREAKPROC	
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
#endif
	

protected:
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


#define SECURE_COLOR RGB(255,249,168)
#define BROKEN_COLOR RGB(255,132,132)
// A simple UrlBar class...
class CUrlBar : public CComboBoxEx
{
public:
	CUrlBar(){
        m_changed = FALSE;

		// Initialise background brushs & colors for highlight
		m_HighlightType = 0;
		m_crBkclr[0] = GetSysColor(COLOR_WINDOW);
		m_crBkclr[1] = theApp.preferences.GetInt("browser.urlbar.highlight.secure", SECURE_COLOR);
		m_crBkclr[2] = theApp.preferences.GetInt("browser.urlbar.highlight.broken", BROKEN_COLOR);
		m_brBkgnd[0].CreateSysColorBrush(COLOR_WINDOW);
		m_brBkgnd[1].CreateSolidBrush(m_crBkclr[1]); 
		m_brBkgnd[2].CreateSolidBrush(m_crBkclr[2]);
	}

	~CUrlBar(){
	}

    HWND m_hwndEdit;

	int Create(DWORD style, RECT &rect, CWnd *parentWnd, UINT id) {
        int ret = CComboBoxEx::Create(style | CBS_AUTOHSCROLL, rect, parentWnd, id);
        SetExtendedStyle(/*CBES_EX_PATHWORDBREAKPROC|*/CBES_EX_CASESENSITIVE, CBES_EX_PATHWORDBREAKPROC|CBES_EX_CASESENSITIVE);
		
        COMBOBOXEXITEM ci;
        ci.mask = CBEIF_IMAGE;
        ci.iItem = -1;
#ifdef INTERNAL_SITEICONS
		ci.iImage = theApp.favicons.GetDefaultIcon();
#endif
        SetItem(&ci);
		LimitText(0);
        CEdit *edit = GetEditCtrl();
		if (!edit) return -1;
        m_hwndEdit = edit->m_hWnd;
		
		// Bug #783
#ifdef URLBAR_USE_SETWORDBREAKPROC
		edit->SendMessage(EM_SETWORDBREAKPROC, 0,
			(LPARAM)&(CUrlBarEdit::UrlBreakProc));
		// Subclassing edit box for autocomplete
		if (theApp.preferences.GetBool("browser.urlbar.autocomplete.enabled", true))
			m_UrlBarEdit.SubclassWindow(m_hwndEdit);
#else
		//Subclassing edit box for autocomplete and ctrl navigation
		//Making our own combo box would be better
		m_UrlBarEdit.SubclassWindow(m_hwndEdit);
#endif

		// Set the height of the dropdown
		edit->GetClientRect(&rect);
		int height = rect.bottom + 4 + GetItemHeight(0) * (theApp.preferences.GetInt("kmeleon.urlbar.dropdown_lines", 10));
		GetComboBoxCtrl()->SetWindowPos(0,0,0,50,height,SWP_NOMOVE|SWP_NOZORDER);
		return ret;

    }
    
	inline CString GetEnteredURL(BOOL ignoreTyped = FALSE) {
		if (!ignoreTyped) {
			CString url;
			GetWindowText(url);
			return url;
		}

		return m_currentURL;
    }

	inline void ResetURL()
	{
		SetWindowText(m_currentURL);
	}

    inline BOOL GetSelectedURL(CString& url) {
          int nIndex = GetCurSel();
          if (nIndex != LB_ERR) {
              GetLBText(nIndex, url);
              return TRUE;
          }
        return FALSE;
    }   
    inline void SetCurrentURL(LPCTSTR pUrl, BOOL always = FALSE) {
		m_currentURL = pUrl;
		if (!m_changed || always && m_currentURL != GetEnteredURL()) {
			DWORD oldSelection = GetEditCtrl()->GetSel();
			if (HIWORD(oldSelection) != LOWORD(oldSelection) || GetWindowTextLength() == 0)
				oldSelection = MAKELONG(LOWORD(oldSelection), -1);

			m_UrlBarEdit.m_AntiLazyIdiot = false;
            if (_tcsncicmp(pUrl, _T("javascript:"), 11))
                SetWindowText(m_currentURL);
			m_UrlBarEdit.m_AntiLazyIdiot = true;

			TRACE0("EditChanged FALSE in SetCurrentURL\n");
            EditChanged(FALSE);
			//if (!CheckFocus())
				GetEditCtrl()->SetSel(oldSelection, TRUE);
			//else
//				GetEditCtrl()->SetSel(0,-1, TRUE);
        }
    }   

    inline void AddURLToList(CString& url) {
        USES_CONVERSION;
        COMBOBOXEXITEM ci;
        ci.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        ci.iItem = 0;
        ci.iImage = ci.iSelectedImage = I_IMAGECALLBACK;

        ci.pszText = const_cast<TCHAR *>((LPCTSTR)url);
        InsertItem(&ci);
    }
    inline void LoadMRUList() {
         ResetContent();
		 if (!theApp.m_MRUList) return;
         POSITION pos = theApp.m_MRUList->GetTailPosition();
         while (pos)
            AddURLToList(theApp.m_MRUList->GetPrev(pos));
    }
    /*int SetSoftFocus() {
        if (IsIconic() || !IsWindowVisible())
	        return 0;
        HWND toplevelWnd = m_hWnd;
        while (::GetParent(toplevelWnd))
            toplevelWnd = ::GetParent(toplevelWnd);
        if (toplevelWnd != ::GetForegroundWindow() || 
            toplevelWnd != ::GetActiveWindow())
            return 0;
        SetFocus();
        return 1;
    }
    void MaintainFocus() {
	   //if (m_bFocusEnabled) {
          if (SetSoftFocus()) {
             m_preserveUrlBarFocus = TRUE;
             m_iFocusCount = 0;
          }
       //}
    }
    BOOL CheckFocus() {
        return m_preserveUrlBarFocus;
    }
    void ReturnFocus(BOOL bDocumentLoading) {
        if (m_preserveUrlBarFocus && --m_iFocusCount >= 0) {
            if (!SetSoftFocus())
                EndFocus();
        }
        else {
            EndFocus();
        }
    }
    void EndFocus() {
        m_preserveUrlBarFocus = FALSE;
    }*/
    inline void EditChanged(BOOL state) {
        m_changed = state;
    }

	inline BOOL GetIsTyped() {
		return m_changed;
	}

	void Highlight(int type){
		m_HighlightType = type;
		//Have to invalidate both for correct redrawing
		GetEditCtrl()->Invalidate();
		Invalidate();
	}

protected:
	COLORREF m_crBkclr[3]; // Background colors (for text)
	CBrush m_brBkgnd[3]; // Background brushs
    int m_HighlightType; // Current background color

    BOOL m_changed;
    CString m_currentURL;
	
	CUrlBarEdit m_UrlBarEdit;		
public:
	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
#ifdef INTERNAL_SITEICONS
	afx_msg void OnCbenGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
#endif
	afx_msg void OnCbenEndedit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnEditchange();
	afx_msg void OnCbnSelchange();

};

