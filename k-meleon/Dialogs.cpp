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

CFindRebar::CFindRebar(CString& csSearchStr, PRBool bMatchCase,
				PRBool bMatchWholeWord, PRBool bWrapAround, CBrowserFrame* pOwner)
				: CReBar()
{
    m_csSearchStr = csSearchStr;
	m_bMatchCase = bMatchCase;
	m_bMatchWholeWord = bMatchWholeWord;
	m_bWrapAround = bWrapAround;
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
	m_pOwner->ClearFindBar();
	delete this;
}

#ifndef FINDBAR_USE_TYPEAHEAD

void CFindRebar::OnEnChangeSearchStr()
{
	if (!m_bAutoSearch) return;

	bool disabled = false;
	
	if (m_csSearchStr.GetLength()==0)
		disabled = true;
	
	m_cEdit.GetWindowText(m_csSearchStr);

	if (m_hid && m_cToolbar.GetToolBarCtrl().IsButtonChecked(m_hid))
		SetTimer(12345, 200, NULL);
	m_bStartsel = true;
	m_pOwner->SendMessage(WM_COMMAND, ID_EDIT_FINDNEXT, 0);
	m_bStartsel = false;
}
#endif

void CFindRebar::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 12345){
	KillTimer(nIDEvent);
	m_pOwner->SendMessage(WM_COMMAND, m_hid, 0);
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
	closeBar.LoadToolBar(IDR_TOOLBAR1);
	closeBar.GetToolBarCtrl().SetBitmapSize(CSize(10,10));

	AddBar(&closeBar, _T(""), NULL, RBBS_NOGRIPPER);

	// Band with the edit control
	m_cEdit.Create(WS_CHILD|ES_AUTOHSCROLL, CRect(0,0,150,18), this, IDC_FIND_EDIT);
	m_cEdit.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
	m_cEdit.SetWindowText(m_csSearchStr);
	
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
			int x = m_ilHot.GetImageCount();
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
	
	button.fsStyle = TBBS_CHECKBOX;

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
	//button.fsState = TBSTATE_ENABLED;
	//button.idCommand = IDC_HIGHLIGHT;
	//m_cToolbar.GetToolBarCtrl().InsertButton(4,&button);
	//str.LoadString(IDS_FIND_HIGHLIGHT);
	//m_cToolbar.SetButtonText(4, (LPCTSTR)str);


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

// File overview....
//
// Contains dialog box code to support Alerts, Prompts such as
// password prompt and username/password prompts
//

//--------------------------------------------------------------------------//
//				CPromptDialog Stuff
//--------------------------------------------------------------------------//

CPromptDialog::CPromptDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                             const TCHAR* pInitPromptText,
                             BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal)
    : CDialog(CPromptDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csPromptText = pText;
    if(pInitPromptText)
        m_csPromptAnswer = pInitPromptText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 
}

void CPromptDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptDialog)
    DDX_Text(pDX, IDC_PROMPT_ANSWER, m_csPromptAnswer);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptDialog::OnInitDialog()
{   
    SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csPromptText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
    if(pChk)
    {
        if (m_bHasCheckBox)
        {
            if(!m_csCheckBoxText.IsEmpty())
                pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
        else
        {
            // Hide the check box control if there's no label text
            // This will be the case when we're not using single sign-on
            pChk->ShowWindow(SW_HIDE); 
        }
    }

    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PROMPT_ANSWER);
    if(pEdit) 
    {
        pEdit->SetWindowText(m_csPromptAnswer);
        pEdit->SetFocus();
        pEdit->SetSel(0, -1);

        return 0; // Returning "0" since we're explicitly setting focus
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CPromptPasswordDialog Stuff
//--------------------------------------------------------------------------//

CPromptPasswordDialog::CPromptPasswordDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                                             const TCHAR* pInitPasswordText,
                                             BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal)
    : CDialog(CPromptPasswordDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck), m_bCheckBoxValue(initCheckVal)
{   
	if(pTitle)
		m_csDialogTitle = pTitle;
	if(pText)
		m_csPromptText = pText;
	if(pInitPasswordText)
	    m_csPassword = pInitPasswordText;
	if(pCheckText)
		m_csCheckBoxText = pCheckText;
}

void CPromptPasswordDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptPasswordDialog)
    DDX_Text(pDX, IDC_PASSWORD, m_csPassword);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptPasswordDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptPasswordDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptPasswordDialog::OnInitDialog()
{   
    SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csPromptText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
    if(pChk)
    {
        if (m_bHasCheckBox)
        {
            if(!m_csCheckBoxText.IsEmpty())
                pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
        else
        {
            // Hide the check box control if there's no label text
            // This will be the case when we're not using single sign-on
            pChk->ShowWindow(SW_HIDE); 
        }
    }

    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PASSWORD);
    if(pEdit) 
    {
        pEdit->SetFocus();

        return 0; // Returning "0" since we're explicitly setting focus
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CPromptUsernamePasswordDialog Stuff
//--------------------------------------------------------------------------//

CPromptUsernamePasswordDialog::CPromptUsernamePasswordDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                                  const TCHAR* pInitUsername, const TCHAR* pInitPassword, 
		                          BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal)
    : CDialog(CPromptUsernamePasswordDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck), m_bCheckBoxValue(initCheckVal)
{
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csPromptText = pText;
    if(pInitUsername)
        m_csUserName = pInitUsername;
    if(pInitPassword)
        m_csPassword = pInitPassword;
    if(pCheckText)
        m_csCheckBoxText = pCheckText;
}

void CPromptUsernamePasswordDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptUsernamePasswordDialog)
    DDX_Text(pDX, IDC_USERNAME, m_csUserName);
    DDX_Text(pDX, IDC_PASSWORD, m_csPassword);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptUsernamePasswordDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptUsernamePasswordDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptUsernamePasswordDialog::OnInitDialog()
{   
   SetWindowText(m_csDialogTitle);
   
   CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
   if(pWnd)
      pWnd->SetWindowText(m_csPromptText);
   
   CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
   if(pChk)
   {
      if(m_bHasCheckBox)
      {
         if (!m_csCheckBoxText.IsEmpty())
            pChk->SetWindowText(m_csCheckBoxText);
         pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
      }
      else
      {
         pChk->ShowWindow(SW_HIDE);
      }
   }
   
   CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PASSWORD);
   if(pEdit) 
   {
      pEdit->SetWindowText(m_csPassword);
   }
   
   pEdit = (CEdit *)GetDlgItem(IDC_USERNAME);
   if(pEdit) 
   {
      pEdit->SetWindowText(m_csUserName);
      pEdit->SetSel(0, -1);
      
      pEdit->SetFocus();
      
      return 0; // Returning "0" since we're explicitly setting focus
   }
   
   return TRUE;
}

//--------------------------------------------------------------------------//
//				CAlertCheckDialog Stuff
//--------------------------------------------------------------------------//

CAlertCheckDialog::CAlertCheckDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                             const TCHAR* pCheckText, int initCheckVal)
        : CDialog(CAlertCheckDialog::IDD, pParent)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csMsgText = pText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 

    m_bCheckBoxValue = initCheckVal;
}

void CAlertCheckDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAlertCheckDialog)
    DDX_Check(pDX, IDC_CHECKBOX, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAlertCheckDialog, CDialog)
    //{{AFX_MSG_MAP(CAlertCheckDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CAlertCheckDialog::OnInitDialog()
{
    SetWindowText(m_csDialogTitle);

    CWnd *pWnd = GetDlgItem(IDC_MSG_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csMsgText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECKBOX);
    if(pChk)
    {
        pChk->SetWindowText(m_csCheckBoxText);
        pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CConfirmCheckDialog Stuff
//--------------------------------------------------------------------------//

CConfirmCheckDialog::CConfirmCheckDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                            const TCHAR* pCheckText, int initCheckVal,
                            const TCHAR*pBtn1Text, const TCHAR*pBtn2Text, 
                            const TCHAR*pBtn3Text)
            : CDialog(CConfirmCheckDialog::IDD, pParent)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csMsgText = pText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 

    m_bCheckBoxValue = initCheckVal;

    if(pBtn1Text)
        m_csBtn1Text = pBtn1Text;
    if(pBtn2Text)
        m_csBtn2Text = pBtn2Text;
    if(pBtn3Text)
        m_csBtn3Text = pBtn3Text;
}

void CConfirmCheckDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfirmCheckDialog)
    DDX_Check(pDX, IDC_CHECKBOX, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConfirmCheckDialog, CDialog)
    //{{AFX_MSG_MAP(CConfirmCheckDialog)
    ON_BN_CLICKED(IDC_BTN1, OnBtn1Clicked)
    ON_BN_CLICKED(IDC_BTN2, OnBtn2Clicked)
    ON_BN_CLICKED(IDC_BTN3, OnBtn3Clicked)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CConfirmCheckDialog::OnInitDialog()
{   
   	SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_MSG_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csMsgText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECKBOX);
    if(pChk)
    {
        if(m_csCheckBoxText.IsEmpty())
        {
            pChk->ShowWindow(SW_HIDE);
        }
        else
        {
            pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
    }

    CButton *pBtn1 = (CButton *)GetDlgItem(IDC_BTN1);
    if(pBtn1)
    {
        if(m_csBtn1Text.IsEmpty())
            pBtn1->ShowWindow(SW_HIDE);
        else
            pBtn1->SetWindowText(m_csBtn1Text);
    }

    CButton *pBtn2 = (CButton *)GetDlgItem(IDC_BTN2);
    if(pBtn2)
    {
        if(m_csBtn2Text.IsEmpty())
            pBtn2->ShowWindow(SW_HIDE);
        else
            pBtn2->SetWindowText(m_csBtn2Text);
    }

    CButton *pBtn3 = (CButton *)GetDlgItem(IDC_BTN3);
    if(pBtn3)
    {
        if(m_csBtn3Text.IsEmpty())
		{
			// Hide the third button and center the other ones
            pBtn3->ShowWindow(SW_HIDE);
			RECT rect, rectbutton;
			GetClientRect(&rect);

			pBtn1->GetWindowRect(&rectbutton);
			ScreenToClient(&rectbutton);

			int buttonwidth = rectbutton.right - rectbutton.left;
			int margin = ((rect.right - rect.left) - 2*buttonwidth + 2) / 2;
			
			pBtn1->SetWindowPos(NULL, margin, rectbutton.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
			pBtn2->SetWindowPos(NULL, margin + buttonwidth + 2, rectbutton.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}
        else
            pBtn3->SetWindowText(m_csBtn3Text);
    }

    return TRUE;
}

void CConfirmCheckDialog::OnBtn1Clicked()
{
    UpdateData();

    EndDialog(0); // where 0 indicates that the btn pressed was at index 0
}

void CConfirmCheckDialog::OnBtn2Clicked()
{
    UpdateData();

    EndDialog(1); // where 1 indicates that the btn pressed was at index 1
}

void CConfirmCheckDialog::OnBtn3Clicked()
{
    UpdateData();

    EndDialog(2); // where 2 indicates that the btn pressed was at index 2
}
