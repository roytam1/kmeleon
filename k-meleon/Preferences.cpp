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

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Preferences.h"

CPreferences::CPreferences() {
}

CPreferences::~CPreferences() {
  if (bAutoSave)
    Save();
}

CPreferences::Load() {
  bAutoSave = true;

  bMaximized = theApp.GetProfileInt(_T("Display"), _T("Maximized"), 1);

  bToolbarBackground = theApp.GetProfileInt(_T("Display"), _T("BackgroundImageEnabled"), 1);
  toolbarBackground = theApp.GetProfileString(_T("Display"), _T("BackgroundImage"));

  bStartHome = theApp.GetProfileInt(_T("General"), _T("StartHome"), 1);
  homePage = theApp.GetProfileString(_T("General"), _T("HomePage"), _T("http://www.kmeleon.org"));

  settingsDir = theApp.GetProfileString(_T("General"), _T("SettingsDir"));
  if (settingsDir.IsEmpty()){
    //  a better way to do this would be to query mozilla
	  char filename[255];
    char *p;
	  GetModuleFileName(AfxGetInstanceHandle(), filename, sizeof(filename));
	  p = filename + lstrlen(filename);
  	while (p >= filename && *p != '\\') p--;
    *p = 0;

    settingsDir = filename;
  }
  if (settingsDir[settingsDir.GetLength()-1] != '\\'){
    settingsDir += '\\';
  }
}

CPreferences::Save() {
  theApp.WriteProfileInt(_T("Display"), _T("Maximized"), bMaximized);

  theApp.WriteProfileInt(_T("Display"), _T("BackgroundImageEnabled"), bToolbarBackground);
  theApp.WriteProfileString(_T("Display"), _T("BackgroundImage"), toolbarBackground);

  theApp.WriteProfileInt(_T("General"), _T("StartHome"), bStartHome);
  theApp.WriteProfileString(_T("General"), _T("HomePage"), homePage);

  theApp.WriteProfileString(_T("General"), _T("SettingsDir"), settingsDir);
}

CPreferencesDlg::CPreferencesDlg() : CDialog(IDD) {
  page = NULL;
}

BOOL CPreferencesDlg::Create(CWnd *pParent){
  return false;
}

void CPreferencesDlg::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDlg)
	DDX_Control(pDX, IDC_LIST1, m_list);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencesDlg, CDialog)
	//{{AFX_MSG_MAP(CPreferencesDlg)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnListSelect)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPreferencesDlg::OnInitDialog(){
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

CPreferencesDlg::~CPreferencesDlg(){
  if (page)
    delete page;
}

int CPreferencesDlg::DoModal(){
  int ret = CDialog::DoModal();

  if (page){
    delete page;
    page = NULL;
  }

  return ret;
}

void CPreferencesDlg::OnOK(){
  page->UpdateData();
  CDialog::OnOK();
}

void CPreferencesDlg::OnCancel(){
  CDialog::OnCancel();
}

void CPreferencesDlg::AddItem(char *text, UINT idd){
  int item = m_list.GetItemCount();
  m_list.InsertItem(item, text, 0);
  m_list.SetItemData(item, idd);
}

void CPreferencesDlg::OnListSelect(NMHDR* pNMHDR, LRESULT* pResult) {
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

void CPreferencesDlg::ShowPage(UINT idd){
  if (page){
    if (page->idd == idd){
      return;
    }
    page->UpdateData();
    delete page;
  }
  if (idd == IDD_PREFERENCES_PLUGINS){
    page = new CPreferencePagePlugins;
  }else{
    page = new CPreferencePage;
  }
  
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

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CPreferencePage::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePage)
  switch (idd){
    case IDD_PREFERENCES_DISPLAY:
      DDX_Text(pDX, IDC_EDIT_TOOLBAR_BACKGROUND, theApp.preferences.toolbarBackground);
      DDX_Check(pDX, IDC_CHECK_TOOLBAR_BACKGROUND, theApp.preferences.bToolbarBackground);
      break;
    case IDD_PREFERENCES_GENERAL:
      DDX_Radio(pDX, IDC_RADIO_START_BLANK, theApp.preferences.bStartHome);
      DDX_Text(pDX, IDC_EDIT_HOMEPAGE, theApp.preferences.homePage);
      DDX_Text(pDX, IDC_EDIT_SETTINGS_DIR, theApp.preferences.settingsDir);
      break;
  }
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencePage, CDialog)
	//{{AFX_MSG_MAP(CPreferencePage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBrowse)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPreferencePage::OnBrowse() {
  CFileDialog fDlg(TRUE);
  switch (idd){
    case IDD_PREFERENCES_DISPLAY:
      fDlg.m_ofn.lpstrFilter = "Bitmaps\0*.bmp\0";
      fDlg.DoModal();
      theApp.preferences.toolbarBackground = fDlg.GetPathName();
      UpdateData(FALSE);
      break;
  }
}

// these are here to cancel the effects of hitting enter/esc
void CPreferencePage::OnOK(){
}

void CPreferencePage::OnCancel(){
}

/**/

BOOL CPreferencePagePlugins::OnInitDialog(){
  CDialog::OnInitDialog();

  RECT rect;
  m_pluginList.SetParent(this);
	m_pluginList.GetClientRect(&rect);

 	m_pluginList.InsertColumn(0, "Blah", LVCFMT_LEFT, rect.right);

  POSITION pos = theApp.plugins.pluginList.GetStartPosition();
  kmeleonPlugin * kPlugin;
  CString s;
  int i=0;
  while (pos){
    theApp.plugins.pluginList.GetNextAssoc( pos, s, kPlugin);

    int item = m_pluginList.GetItemCount();

    m_pluginList.InsertItem(item, kPlugin->description, 0);
  }
  m_pluginList.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CPreferencePagePlugins::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePagePlugins)
  DDX_Control(pDX, IDC_LIST_PLUGINS, m_pluginList);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencePagePlugins, CPreferencePage)
	//{{AFX_MSG_MAP(CPreferencePagePlugins)
  ON_BN_CLICKED(IDC_BUTTON_CONFIG, OnConfig)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencePagePlugins::OnConfig() {
  int item;
  POSITION pos;
  pos = m_pluginList.GetFirstSelectedItemPosition();
  if (!pos){
    return;
  }
  item = m_pluginList.GetNextSelectedItem(pos);

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
