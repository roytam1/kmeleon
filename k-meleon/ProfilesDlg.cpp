/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this Mozilla sample software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

// ProfilesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ProfilesDlg.h"
#include "ProfileMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Static Routines
static void ValidateProfileName(const CString& profileName, CDataExchange* pDX)
{
   USES_CONVERSION;

   PRBool exists = FALSE;

   CProfile profile;
   if (theApp.m_ProfileMgr->GetProfileByName(profileName, profile))
   {
       CString errMsg;
       errMsg.Format(IDS_PROFILE_EXISTS, (const TCHAR *)profileName);
       AfxMessageBox( errMsg, MB_ICONEXCLAMATION );
       errMsg.Empty();
       pDX->Fail();
   }
   
    if (profileName.FindOneOf(_T("\\/[]?#:*<>")) != -1)
    {
        AfxMessageBox( IDS_PROFILE_BAD_CHARS, MB_ICONEXCLAMATION );
        pDX->Fail();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileDlg dialog


CNewProfileDlg::CNewProfileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewProfileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewProfileDlg)
	m_LocaleIndex = -1;
	m_Name = _T("");
	//}}AFX_DATA_INIT
}


void CNewProfileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileDlg)
	DDX_CBIndex(pDX, IDC_LOCALE_COMBO, m_LocaleIndex);
	DDX_Text(pDX, IDC_NEW_PROF_NAME, m_Name);
	//}}AFX_DATA_MAP

    pDX->PrepareEditCtrl(IDC_NEW_PROF_NAME);
    if (pDX->m_bSaveAndValidate)
    {
        ValidateProfileName(m_Name, pDX);
    }
}


BEGIN_MESSAGE_MAP(CNewProfileDlg, CDialog)
	//{{AFX_MSG_MAP(CNewProfileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileDlg message handlers


/////////////////////////////////////////////////////////////////////////////
// CRenameProfileDlg dialog


CRenameProfileDlg::CRenameProfileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRenameProfileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRenameProfileDlg)
	m_NewName = _T("");
	//}}AFX_DATA_INIT
}


void CRenameProfileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenameProfileDlg)
	DDX_Text(pDX, IDC_NEW_NAME, m_NewName);
	//}}AFX_DATA_MAP

    pDX->PrepareEditCtrl(IDC_NEW_NAME);
    if (pDX->m_bSaveAndValidate)
    {
        ValidateProfileName(m_NewName, pDX);
    }
}


BEGIN_MESSAGE_MAP(CRenameProfileDlg, CDialog)
	//{{AFX_MSG_MAP(CRenameProfileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenameProfileDlg message handlers


/////////////////////////////////////////////////////////////////////////////
// CProfilesDlg dialog


CProfilesDlg::CProfilesDlg(CProfileMgr* profMgr, CWnd* pParent /*=NULL*/)
	: CDialog(CProfilesDlg::IDD, pParent)
{
	mProfMgr = profMgr;
	//{{AFX_DATA_INIT(CProfilesDlg)
    m_bAtStartUp = FALSE;
	m_bAskAtStartUp = FALSE;
	//}}AFX_DATA_INIT
}


void CProfilesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProfilesDlg)
	DDX_Control(pDX, IDC_LIST1, m_ProfileList);
	DDX_Check(pDX, IDC_CHECK_ASK_AT_START, m_bAskAtStartUp);
	DDX_LBString(pDX, IDC_LIST1, m_SelectedProfile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProfilesDlg, CDialog)
	//{{AFX_MSG_MAP(CProfilesDlg)
	ON_BN_CLICKED(IDC_PROF_NEW, OnNewProfile)
	ON_BN_CLICKED(IDC_PROF_RENAME, OnRenameProfile)
	ON_BN_CLICKED(IDC_PROF_DELETE, OnDeleteProfile)
	ON_LBN_DBLCLK(IDC_LIST1, OnDblclkProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfilesDlg message handlers

BOOL CProfilesDlg::OnInitDialog() 
{
    m_bAskAtStartUp = mProfMgr->GetShowDialogOnStart();
    CDialog::OnInitDialog();

	CProfile defProf;
	mProfMgr->GetDefaultProfile(defProf);

    UINT profileCount;
    CProfile **profileList;
	if (!mProfMgr->GetProfileList(&profileList, &profileCount))
		return TRUE;

	UINT selectedRow = 0;
    for (PRUint32 index = 0; index < profileCount; index++)
    {
        m_ProfileList.AddString(profileList[index]->mName);
		if (defProf.mName == profileList[index]->mName)
            selectedRow = index;
		delete profileList[index];
    }
	
	delete [] profileList;
    m_ProfileList.SetCurSel(selectedRow);
    /*
    if (m_bAtStartUp)
    {
        GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
    }
    */

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProfilesDlg::OnNewProfile() 
{
	CNewProfileDlg dialog;

    if (dialog.DoModal() == IDOK)
    {
  		CProfile profile;
		BOOL result = mProfMgr->CreateProfile((LPCTSTR)NULL, NULL, dialog.m_Name, profile);
        ASSERT(result);
		if (result) {
			int item = m_ProfileList.AddString(dialog.m_Name);
            m_ProfileList.SetCurSel(item);
            GetDlgItem(IDOK)->EnableWindow(TRUE);
		}
	}
}

void CProfilesDlg::OnRenameProfile() 
{
	CRenameProfileDlg dialog;

    int itemIndex = m_ProfileList.GetCurSel();
    ASSERT(itemIndex != LB_ERR);
    if (itemIndex == LB_ERR)
        return;

    m_ProfileList.GetText(itemIndex, dialog.m_CurrentName);

    if (dialog.DoModal() == IDOK)
    {
		BOOL result = mProfMgr->RenameProfile(dialog.m_CurrentName, dialog.m_NewName);
		ASSERT(result);
		if (result) {
		   m_ProfileList.DeleteString(itemIndex);
		   m_ProfileList.InsertString(itemIndex, dialog.m_NewName);
		}
	}
}

void CProfilesDlg::OnDeleteProfile() 
{
    int itemIndex = m_ProfileList.GetCurSel();
    ASSERT(itemIndex != LB_ERR);
    if (itemIndex == LB_ERR)
        return;

    CString selectedProfile;
    m_ProfileList.GetText(itemIndex, selectedProfile);
    
	if (mProfMgr->RemoveProfile(selectedProfile, TRUE))
	{
       int itemCount = m_ProfileList.DeleteString(itemIndex);
       if (itemCount == 0)
          GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		AfxMessageBox(IDS_PROFILE_DELETELOCK, MB_ICONEXCLAMATION );
	}
}

void CProfilesDlg::OnDblclkProfile()
{
  OnOK();
}

void CProfilesDlg::OnOK()
{
	CDialog::OnOK();
	mProfMgr->SetShowDialogOnStart(m_bAskAtStartUp);
}
