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
//  Holds various prefernces for k-meleon.  also controls the dialog box

#include "StdAfx.h"
#include "KMeleon.h"

#include "Preferences.h"

extern CKMeleonApp theApp;

CPreferences::CPreferences(){
  Init();
}

CPreferences::CPreferences(CWnd* pParent /*=NULL*/)
	: CDialog(CPreferences::IDD, pParent)
{
  Init();
}

BOOL CPreferences::Create(CWnd *pParent){
  return false;
}

void CPreferences::Init(){
  bStartHome = theApp.GetProfileInt(_T("General"), _T("StartHome"), 1);
  homePage = theApp.GetProfileString(_T("General"), _T("HomePage"), "http://www.kmeleon.org");

  toolbarBackground = theApp.GetProfileString(_T("Display"), _T("BackgroundImage"));
  bToolbarBackground = theApp.GetProfileInt(_T("Display"), _T("BackgroundImageEnabled"), 1);

  page = NULL;
}

void CPreferences::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferences)
	DDX_Control(pDX, IDC_LIST1, m_list);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferences, CDialog)
	//{{AFX_MSG_MAP(CPreferences)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnListSelect)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPreferences::OnInitDialog(){
  CDialog::OnInitDialog();

  UpdateData(true);

  RECT rect;
  m_list.SetParent(this);
	m_list.GetClientRect(&rect);

 	m_list.InsertColumn(0, "Blah", LVCFMT_LEFT, rect.right);

  AddItem("General", IDD_PREFERENCES_GENERAL);
  AddItem("Display", IDD_PREFERENCES_DISPLAY);
  AddItem("Proxy",   IDD_PREFERENCES_PROXY);
  AddItem("Plugins", IDD_PREFERENCES_PLUGINS);

  m_list.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

CPreferences::~CPreferences(){
  if (page)
    delete page;
}

int CPreferences::DoModal(){
  int ret = CDialog::DoModal();

  if (page){
    delete page;
    page = NULL;
  }

  theApp.WriteProfileInt(_T("Display"), _T("BackgroundImageEnabled"), bToolbarBackground);
  theApp.WriteProfileString(_T("Display"), _T("BackgroundImage"), toolbarBackground);

  theApp.WriteProfileInt(_T("General"), _T("StartHome"), bStartHome);
  theApp.WriteProfileString(_T("General"), _T("HomePage"), homePage);

  return ret;
}

void CPreferences::OnOK(){
  page->UpdateData();
  CDialog::OnOK();
}

void CPreferences::OnCancel(){
  CDialog::OnCancel();
}

void CPreferences::AddItem(char *text, UINT idd){
  int item = m_list.GetItemCount();
  m_list.InsertItem(item, text, 0);
  m_list.SetItemData(item, idd);
}

void CPreferences::OnListSelect(NMHDR* pNMHDR, LRESULT* pResult) {
  int item;
  POSITION pos;
  pos = m_list.GetFirstSelectedItemPosition();
  if (!pos){
    return;
  }
  item = m_list.GetNextSelectedItem(pos);

  UINT idd = m_list.GetItemData(item);

  ShowPage(idd);
}

void CPreferences::ShowPage(UINT idd){
  if (page){
    if (page->idd == idd){
      return;
    }
    page->UpdateData();
    delete page;
  }
  page = new CPreferencePage;
  
  page->idd = idd;

  page->Create(idd, this);

  page->SetParent(this);

  RECT rect;
  CWnd *container = GetDlgItem(IDC_CONTAINER);
  container->GetClientRect(&rect);
  container->ClientToScreen(&rect);

  ScreenToClient(&rect);

  page->MoveWindow(&rect);

  page->ShowWindow(SW_SHOW);
}

/**/

BOOL CPreferencePage::OnInitDialog(){
  CDialog::OnInitDialog();

  if (idd == IDD_PREFERENCES_PLUGINS){
    CPreferences *parent = (CPreferences *)GetParent();
    RECT rect;
    parent->m_pluginList.SetParent(this);
	  parent->m_pluginList.GetClientRect(&rect);

 	  parent->m_pluginList.InsertColumn(0, "Blah", LVCFMT_LEFT, rect.right);

    POSITION pos = theApp.plugins.pluginList.GetStartPosition();
    kmeleonPlugin * kPlugin;
    CString s;
    int i=0;
    while (pos){
      theApp.plugins.pluginList.GetNextAssoc( pos, s, kPlugin);

      int item = parent->m_pluginList.GetItemCount();

      parent->m_pluginList.InsertItem(item, kPlugin->description, 0);
    }
    parent->m_pluginList.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
  }

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CPreferencePage::DoDataExchange(CDataExchange* pDX){
  CPreferences *parent = (CPreferences *)GetParent();
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePage)
  switch (idd){
    case IDD_PREFERENCES_DISPLAY:
      DDX_Text(pDX, IDC_EDIT_TOOLBAR_BACKGROUND, parent->toolbarBackground);
      DDX_Check(pDX, IDC_CHECK_TOOLBAR_BACKGROUND, parent->bToolbarBackground);
      break;
    case IDD_PREFERENCES_GENERAL:
      DDX_Radio(pDX, IDC_RADIO_START_BLANK, parent->bStartHome);
      DDX_Text(pDX, IDC_EDIT_HOMEPAGE, parent->homePage);
      break;
    case IDD_PREFERENCES_PLUGINS:
      DDX_Control(pDX, IDC_LIST_PLUGINS, parent->m_pluginList);
      break;
  }
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencePage, CDialog)
	//{{AFX_MSG_MAP(CPreferencePage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBrowse)
  ON_BN_CLICKED(IDC_BUTTON_CONFIG, OnConfig)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPreferencePage::OnBrowse() {
  CPreferences *parent = (CPreferences *)GetParent();

  CFileDialog fDlg(TRUE);
  switch (idd){
    case IDD_PREFERENCES_DISPLAY:
      fDlg.m_ofn.lpstrFilter = "Bitmaps\0*.bmp\0";
      fDlg.DoModal();
      if (parent){
        parent->toolbarBackground = fDlg.GetPathName();
        UpdateData(FALSE);
      }
      break;
  }
}

void CPreferencePage::OnConfig() {
  CPreferences *parent = (CPreferences *)GetParent();

  int item;
  POSITION pos;
  pos = parent->m_pluginList.GetFirstSelectedItemPosition();
  if (!pos){
    return;
  }
  item = parent->m_pluginList.GetNextSelectedItem(pos);

  kmeleonPlugin * kPlugin;
  CString s;
  pos = theApp.plugins.pluginList.GetStartPosition();
  while (pos){
    theApp.plugins.pluginList.GetNextAssoc( pos, s, kPlugin);

    if (item == 0){
      if (kPlugin->Config){
        kPlugin->Config(this->m_hWnd);
      }
      break;
    }
    item--;
  }
}

// these are here to cancel the effects of hitting enter/esc
void CPreferencePage::OnOK(){
}

void CPreferencePage::OnCancel(){
}