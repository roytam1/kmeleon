/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chak Nanga <chak@netscape.com> 
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "stdafx.h"
#include "Dialogs.h"
#include "BrowserFrm.h"
#include "BrowserView.h"
#include "MfcEmbed.h"
#include "resource.h"
extern CMfcEmbedApp theApp;

//--------------------------------------------------------------------------//
//				CFindReBar 
//--------------------------------------------------------------------------//

#ifndef _UNICODE
CFindRebar::CFindRebar(WCHAR* szSearch, PRBool bMatchCase,
				PRBool bMatchWholeWord, PRBool bWrapAround, 
				PRBool bHighlight, CBrowserFrame* pOwner)
{
	memset(m_szUStr, 0, sizeof(m_szUStr)*sizeof(char));
	if (szSearch)
		wcsncpy(m_szUStr, szSearch, sizeof(m_szUStr)/sizeof(WCHAR)-1);
#else
CFindRebar::CFindRebar(CString csSearchStr, PRBool bMatchCase,
				PRBool bMatchWholeWord, PRBool bWrapAround, 
				PRBool bHighlight, CBrowserFrame* pOwner)
				: CReBar()
{
    m_csSearchStr = csSearchStr;
#endif
	m_bMatchCase = bMatchCase;
	m_bMatchWholeWord = bMatchWholeWord;
	m_bWrapAround = bWrapAround;
	m_bHighlight = bHighlight;
	m_pOwner = pOwner;
	m_hid = 0;

	// Stay false until we have initialised the edit control
	m_bAutoSearch = false;
	m_clrBkgnd = RGB(255,128,128);
	m_brBkgnd.CreateSolidBrush( m_clrBkgnd  );
	m_NotFound = false;
}

BEGIN_MESSAGE_MAP(CFindRebar, CReBar)
#ifndef FINDBAR_USE_TYPEAHEAD
	ON_EN_CHANGE(IDC_FIND_EDIT, OnEnChangeSearchStr)
#endif
	ON_WM_CREATE()
	ON_COMMAND(ID_CLOSE_FINDBAR, Close)
	ON_WM_SETFOCUS()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()

CFindRebar::~CFindRebar()
{
	m_bStartsel = false;
}

BOOL CFindRebar::Create(CWnd* parent, DWORD dwStyle)
{
	return CReBar::Create(parent,   RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE, dwStyle | WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
}

void CFindRebar::Close()
{
	DestroyWindow();
}

void CFindRebar::PostNcDestroy()
{
#ifndef _UNICODE
	if (theApp.m_bUnicode){
		::DestroyWindow(m_cEdit.Detach());
	}
#endif
	m_pOwner->ClearFindBar();
	//CReBar::PostNcDestroy();
}

#ifndef FINDBAR_USE_TYPEAHEAD

void CFindRebar::OnEnChangeSearchStr()
{
	if (!m_bAutoSearch) return;

	bool disabled = false;
	
	if (m_csSearchStr.GetLength()==0)
		disabled = true;
#ifndef _UNICODE
	if (theApp.m_bUnicode) {
		//int len = ::GetWindowTextLengthW(m_hWnd);
		::GetWindowTextW(m_cEdit.m_hWnd, m_szUStr, sizeof(m_szUStr)/sizeof(WCHAR)-1);
	}else{
#endif
	m_cEdit.GetWindowText(m_csSearchStr);
#ifndef _UNICODE
	USES_CONVERSION;
	wcsncpy(m_szUStr, A2W(m_csSearchStr), sizeof(m_szUStr)/sizeof(WCHAR)-1);
	}
#endif
	if (Highlight())
		SetTimer(12345, 300, NULL);

	m_bStartsel = true;
	m_pOwner->SendMessage(WM_COMMAND, ID_EDIT_FINDNEXT, 0);
	m_bStartsel = false;
}
#endif

void CFindRebar::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 12345){
		KillTimer(nIDEvent);
		m_pOwner->SendMessage(WM_COMMAND, IDC_HIGHLIGHT, 0);
	}
	CReBar::OnTimer(nIDEvent);
}

int CFindRebar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CReBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CString str;

	// Band with the close button
	closeBar.CreateEx(this, TBSTYLE_FLAT|TBSTYLE_TRANSPARENT);
	closeBar.LoadToolBar(IDR_TOOLBAR_CLOSE);
	closeBar.GetToolBarCtrl().SetBitmapSize(CSize(10,10));

	AddBar(&closeBar, _T(""), NULL, RBBS_NOGRIPPER);

	// Band with the edit control
#ifndef _UNICODE 
	if (theApp.m_bUnicode){
		HWND hWnd = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
			NULL, WS_CHILD|ES_AUTOHSCROLL, 0, 0, 150, 18,
			this->m_hWnd, (HMENU)(UINT_PTR)IDC_FIND_EDIT, AfxGetInstanceHandle(), NULL);

		m_cEdit.Attach(hWnd);
		::SetWindowTextW(hWnd, m_szUStr);
	}else{
#endif
	m_cEdit.Create(WS_CHILD|ES_AUTOHSCROLL, CRect(0,0,150,18), this, IDC_FIND_EDIT);
	m_cEdit.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
	m_cEdit.SetWindowText(m_csSearchStr);
#ifndef _UNICODE 
	}
#endif
	
	str.LoadString(IDS_FIND);
	AddBar(&m_cEdit, str, NULL, RBBS_NOGRIPPER);

	// Toolbar with next/Previous and options
	m_cToolbar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS
		);//,WS_CHILD|WS_VISIBLE|CBRS_FLYBY);

	m_cToolbar.GetToolBarCtrl().SetImageList(NULL);

	CString skinFile;
	if (theApp.FindSkinFile(skinFile, _T("findhot.bmp")))
	{
		m_ilHot.Create(16, 16, ILC_MASK | ILC_COLOR8, 4, 8);
		HBITMAP bitmap = (HBITMAP)LoadImage(NULL, skinFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
		if (bitmap) {
			ImageList_AddMasked(m_ilHot.GetSafeHandle(), bitmap, RGB(255, 0, 255));
			DeleteObject(bitmap);
			m_cToolbar.GetToolBarCtrl().SetHotImageList(&m_ilHot);
		}
	}
	
	if (theApp.FindSkinFile(skinFile, _T("findcold.bmp")))
	{
		m_ilCold.Create(16, 16, ILC_MASK | ILC_COLOR8, 4, 8);
		HBITMAP bitmap = (HBITMAP)LoadImage(NULL, skinFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
		if (bitmap) {
			ImageList_AddMasked(m_ilCold.GetSafeHandle(), bitmap, RGB(255, 0, 255));
			DeleteObject(bitmap);
			m_cToolbar.GetToolBarCtrl().SetImageList(&m_ilCold);
		}
	}

	
	TBBUTTON button = {0};
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	//int stringID = toolbar->GetToolBarCtrl().AddString(IDS_FINDNEXT);
	int stringID=-1;
    button.idCommand = ID_EDIT_FINDNEXT;
	button.iString =stringID;
	m_cToolbar.GetToolBarCtrl().InsertButton(0,&button);
	
	str.LoadString(IDS_FINDNEXT);
	m_cToolbar.SetButtonText(0, (LPCTSTR)str);

	//stringID = toolbar->GetToolBarCtrl().AddString(IDS_FINDPREV);
	button.idCommand = ID_EDIT_FINDPREV;
	button.iBitmap = 1;
	button.iString = stringID;
	m_cToolbar.GetToolBarCtrl().InsertButton(1,&button);
	str.LoadString(IDS_FINDPREV);
	m_cToolbar.SetButtonText(1, (LPCTSTR)str);
   
	//button.fsStyle = TBSTYLE_CHECK | TB_AUTOSIZE;
	
	button.fsStyle = TBBS_CHECKBOX | TBSTYLE_AUTOSIZE;

	// Wrap around button
	if (m_bWrapAround)
		button.fsState = TBSTATE_CHECKED;
	
	button.idCommand = IDC_WRAP_AROUND;
	button.iBitmap = 2;
	m_cToolbar.GetToolBarCtrl().InsertButton(2,&button);
	str.LoadString(IDS_FIND_WRAPAROUND);
	m_cToolbar.SetButtonText(2, (LPCTSTR)str);

	// Match case button
	
	button.fsState = TBSTATE_ENABLED;
	if (m_bMatchCase)
		button.fsState = TBSTATE_CHECKED;
	
	button.idCommand = IDC_MATCH_CASE;
	button.iBitmap = 3;
	m_cToolbar.GetToolBarCtrl().InsertButton(3,&button);
	str.LoadString(IDS_FIND_MATCHCASE);
	m_cToolbar.SetButtonText(3, (LPCTSTR)str);
	//toolbar->SetButtonStyle(2, TBBS_CHECKBOX);
	
	// Highlight button
	button.fsState = TBSTATE_ENABLED;
	if (m_bHighlight)
		button.fsState = TBSTATE_CHECKED;
	button.idCommand = IDC_HIGHLIGHT;
	m_cToolbar.GetToolBarCtrl().InsertButton(4,&button);
	str.LoadString(IDS_FIND_HIGHLIGHT);
	m_cToolbar.SetButtonText(4, (LPCTSTR)str);


	AddBar(&m_cToolbar, _T(""), NULL, RBBS_NOGRIPPER);

	// Setting the font of the edit control
	m_cEdit.SetFont(m_cToolbar.GetFont()); 

	// I wonder why I have to do that ...
	REBARBANDINFO rbbi;
    rbbi.cbSize = sizeof(rbbi);
    rbbi.fMask = RBBIM_CHILDSIZE;
    rbbi.cxMinChild = 150;
    rbbi.cyMinChild = HIWORD(m_cToolbar.GetToolBarCtrl().GetButtonSize()); // Not sure what constant I should set here
	GetReBarCtrl().SetBandInfo (1, &rbbi);
	rbbi.cxMinChild = 20;
	GetReBarCtrl().SetBandInfo (2, &rbbi);
	rbbi.cyMinChild = 16;
	GetReBarCtrl().SetBandInfo (0, &rbbi);
	
	m_cEdit.SetFocus();
	m_cEdit.SetSel(0,-1);
	m_bAutoSearch = true; 
	
	return 0;
}


void CFindRebar::OnSetFocus(CWnd* pOldWnd)
{
	CReBar::OnSetFocus(pOldWnd);
	m_cEdit.SetFocus();
	m_cEdit.SetSel(0,-1);
}


BOOL CFindRebar::PreTranslateMessage(MSG* pMsg)
{
	// to test: Ctrl+F
	
	if(pMsg->message==WM_KEYDOWN){
		if (pMsg->wParam!=VK_SHIFT ){
			if(pMsg->wParam==VK_F3)
				return false; // browserview will take care of it
		}
		if (pMsg->wParam==VK_ESCAPE){
			Close();
			return true;
		}
		if (pMsg->wParam==VK_RETURN){
			m_bStartsel = false;
			m_pOwner->SendMessage(WM_COMMAND, ID_EDIT_FINDNEXT, 0);
			//m_pOwner->SendMessage(WM_FINDMSG, (WPARAM)this, 1);
			return true;
		}
#ifdef FINDBAR_USE_TYPEAHEAD
		m_pOwner->m_wndBrowserView.PostMessage(WM_KEYDOWN, pMsg->lParam, pMsg->wParam);
#endif
	}

	return CReBar::PreTranslateMessage(pMsg);
}

HBRUSH CFindRebar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_EDIT && m_NotFound)
	{
		pDC->SetBkColor( m_clrBkgnd );  
		return m_brBkgnd;    
	}

	HBRUSH hbr = CReBar::OnCtlColor(pDC, pWnd, nCtlColor);
	return hbr;
}


#if 0
// File overview....
//
// Contains find dialog stuff
//
//--------------------------------------------------------------------------//
//				CFindDialog Stuff
//--------------------------------------------------------------------------//

CFindDialog::CFindDialog(CString& csSearchStr, PRBool bMatchCase,
				PRBool bMatchWholeWord, PRBool bWrapAround,
				PRBool bSearchBackwards, CBrowserView* pOwner)
				: CFindReplaceDialog()
{
	// Save these initial settings off in member vars
	// We'll use these to initialize the controls
	// in InitDialog()     
   
   m_csSearchStr = csSearchStr;
	m_bMatchCase = bMatchCase;
	m_bMatchWholeWord = bMatchWholeWord;
	m_bWrapAround = bWrapAround;
	m_bSearchBackwards = bSearchBackwards;
	m_pOwner = pOwner;

	// Set up to load our customized Find dialog template
	// rather than the default one MFC provides
	m_fr.Flags |= FR_ENABLETEMPLATE;
	m_fr.hInstance = AfxGetInstanceHandle();
	m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_FINDDLG);
}

BOOL CFindDialog::OnInitDialog() 
{
	CFindReplaceDialog::OnInitDialog();


   // -- Find settings
   m_bMatchCase = theApp.preferences.bFindMatchCase;
   m_bSearchBackwards = theApp.preferences.bFindSearchBackwards;
   m_bWrapAround = theApp.preferences.bFindWrapAround;  


   CEdit* pEdit = (CEdit *)GetDlgItem(IDC_FIND_EDIT);
	if(pEdit)
		pEdit->SetWindowText(m_csSearchStr);

	CButton* pChk = (CButton *)GetDlgItem(IDC_MATCH_CASE);
	if(pChk)
		pChk->SetCheck(m_bMatchCase);

	pChk = (CButton *)GetDlgItem(IDC_MATCH_WHOLE_WORD);
	if(pChk)
		pChk->SetCheck(m_bMatchWholeWord);

	pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);	
	if(pChk)
		pChk->SetCheck(m_bWrapAround);

	pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);
	if(pChk)
		pChk->SetCheck(m_bSearchBackwards);

	return TRUE; 
}

void CFindDialog::OnCancel() {

	CButton* pChk = (CButton *)GetDlgItem(IDC_MATCH_CASE);
	if(pChk)
		theApp.preferences.bFindMatchCase = pChk->GetCheck();

	pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);	
	if(pChk)
		theApp.preferences.bFindWrapAround = pChk->GetCheck();
	
	pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);
	if(pChk)
		theApp.preferences.bFindSearchBackwards = pChk->GetCheck();

	CFindReplaceDialog::OnCancel();

}

void CFindDialog::PostNcDestroy()	
{
   // Let the owner know we're gone
	if(m_pOwner != NULL)	
		m_pOwner->ClearFindDialog();

	CFindReplaceDialog::PostNcDestroy();
}

BOOL CFindDialog::WrapAround()
{
	CButton* pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);

	return pChk ? pChk->GetCheck() : FALSE;
}

BOOL CFindDialog::SearchBackwards()
{
	CButton* pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);

	return pChk ? pChk->GetCheck() : FALSE;
}
#endif

//IMPLEMENT_DYNAMIC(CSelectDialog, CDialog)
CSelectDialog::CSelectDialog(CWnd* pParent, LPCTSTR pTitle, LPCTSTR pText)
	: CDialog(CSelectDialog::IDD, pParent)
{
	if(pTitle) m_csDialogTitle = pTitle;
    if(pText) m_csMsgText = pText;
	m_iChoice = 0;
}

CSelectDialog::~CSelectDialog()
{
}

void CSelectDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SELECT, m_cList);
}


BEGIN_MESSAGE_MAP(CSelectDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_LBN_DBLCLK(IDC_LIST_SELECT, OnLbnDblclkListSelect)
END_MESSAGE_MAP()


// Gestionnaires de messages SelectDialog

BOOL CSelectDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	POSITION pos = m_clChoices.GetHeadPosition();
	for (int i=0;i < m_clChoices.GetCount();i++) {
		int idx = m_cList.AddString(m_clChoices.GetNext(pos));
		m_cList.SetItemData(idx, i);
	}
	
   	SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_MSG_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csMsgText);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION : les pages de propriétés OCX devraient retourner FALSE
}

void CSelectDialog::AddChoice(LPCTSTR text)
{
	m_clChoices.AddTail(text);
}

void CSelectDialog::OnBnClickedOk()
{
	int idx = m_cList.GetCaretIndex();
	m_iChoice = m_cList.GetItemData(idx);
	OnOK();
}

void CSelectDialog::OnLbnDblclkListSelect()
{
	OnBnClickedOk();
}
