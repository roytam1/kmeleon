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
}

#define GetBool(_pref, _value, _defaultValue) { \
    PRBool tempBool;                            \
    rv = prefs->GetBoolPref(_pref, &tempBool);  \
    if (NS_SUCCEEDED(rv))                       \
      _value = tempBool;                        \
    else                                        \
      _value = _defaultValue;                   \
  }

#define GetInt(_pref, _value, _defaultValue) {  \
    PRInt32 tempInt;                            \
    rv = prefs->GetIntPref(_pref, &tempInt);    \
    if (NS_SUCCEEDED(rv))                       \
      _value = tempInt;                         \
    else                                        \
      _value = _defaultValue;                   \
  }

#define GetString(_pref, _value, _defaultValue) { \
    nsXPIDLCString tempString;                    \
    rv = prefs->CopyCharPref(_pref, getter_Copies(tempString));  \
    if (NS_SUCCEEDED(rv))                         \
      _value = tempString;                        \
    else                                          \
      _value = _defaultValue;                     \
  }

void CPreferences::Load() {

  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {	  

    PRBool inited;
    rv = prefs->GetBoolPref("kmeleon.prefs_inited", &inited);
    if (NS_FAILED(rv) || !inited) {
      // Set up prefs for first run
      rv = prefs->SetBoolPref(_T("kmeleon.prefs_inited"), PR_TRUE);
      rv = prefs->SavePrefFile();
    }
    GetBool(_T("kmeleon.display.maximized"), bMaximized, true);
    GetBool(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground, true);

    GetString(_T("kmeleon.display.backgroundImage"), toolbarBackground, _T(""));

    GetBool(_T("kmeleon.general.startHome"), bStartHome, true);
    GetString(_T("kmeleon.general.homePage"), homePage, _T("http://www.kmeleon.org"));

    GetString(_T("kmeleon.general.settingsDir"), settingsDir, _T(""));
    if (settingsDir.IsEmpty()){
      nsCOMPtr<nsIFile> profileDir;
      rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDir));
      NS_ASSERTION(profileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");
      if (NS_SUCCEEDED(rv)){
        nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(profileDir));
        NS_ASSERTION(localFile, "Cannot get nsILocalFile from profile dir");

        nsXPIDLCString descriptorString;
        rv = localFile->GetPersistentDescriptor(getter_Copies(descriptorString));

        settingsDir = descriptorString;
      }
    }
    if (settingsDir[settingsDir.GetLength() - 1] != '\\'){
      settingsDir += '\\';
    }

    GetString(_T("network.proxy.http"), proxyHttp, _T("proxy"));
    GetInt(_T("network.proxy.http_port"), proxyHttpPort, 80);

    GetString(_T("network.proxy.no_proxies_on"), proxyNoProxy, _T("localhost"));

    GetInt(_T("network.proxy.type"), proxyType, 0);
  }
  else
    NS_ASSERTION(PR_FALSE, "Could not get preferences service");
}

void CPreferences::Save() {
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {	  
    rv = prefs->SetBoolPref(_T("kmeleon.display.maximized"), bMaximized);
    rv = prefs->SetBoolPref(_T("kmeleon.display.backgroundImageEnabled"), bToolbarBackground);
    rv = prefs->SetCharPref(_T("kmeleon.display.backgroundImage"), toolbarBackground);

    rv = prefs->SetBoolPref(_T("kmeleon.general.startHome"), bStartHome);
    rv = prefs->SetCharPref(_T("kmeleon.general.homePage"), homePage);

    rv = prefs->SetCharPref(_T("kmeleon.general.settingsDir"), settingsDir);

    rv = prefs->SetCharPref(_T("network.proxy.http"), proxyHttp);
    rv = prefs->SetIntPref(_T("network.proxy.http_port"), proxyHttpPort);

    rv = prefs->SetCharPref(_T("network.proxy.no_proxies_on"), proxyNoProxy);

    rv = prefs->SetIntPref(_T("network.proxy.type"), proxyType);

    rv = prefs->SavePrefFile();
  }
  else
    NS_ASSERTION(PR_FALSE, "Could not get preferences service");
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
    case IDD_PREFERENCES_PROXY:
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY, theApp.preferences.proxyHttp);
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY_PORT, theApp.preferences.proxyHttpPort);
      DDX_Text(pDX, IDC_EDIT_PROXY_NO_PROXY, theApp.preferences.proxyNoProxy);
      DDX_Check(pDX, IDC_CHECK_PROXY_TYPE, theApp.preferences.proxyType);
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
