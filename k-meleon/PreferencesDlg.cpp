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
//  controls the preferences dialog box

#include "StdAfx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Preferences.h"
#include "Plugins.h"

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

   AddItem(_T("General"), IDD_PREFERENCES_GENERAL);
   AddItem(_T("Display"), IDD_PREFERENCES_DISPLAY);
   AddItem(_T("Menus"),   IDD_PREFERENCES_MENUS);
   AddItem(_T("Proxy"),   IDD_PREFERENCES_PROXY);
   AddItem(_T("Paranoia"),IDD_PREFERENCES_PARANOIA);
   AddItem(_T("Plugins"), IDD_PREFERENCES_PLUGINS);

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

  theApp.preferences.Save();

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
  }
  else if (idd == IDD_PREFERENCES_MENUS){
     page = new CPreferencePageMenus;
  }
  else{
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
      DDX_Text(pDX, IDC_EDIT_PLUGINS_DIR, theApp.preferences.pluginsDir);
      DDX_Check(pDX, IDC_CHECK_SOURCE_ENABLED, theApp.preferences.sourceEnabled);
      DDX_Text(pDX, IDC_EDIT_SOURCE_COMMAND, theApp.preferences.sourceCommand);
      break;
    case IDD_PREFERENCES_PROXY:
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY, theApp.preferences.proxyHttp);
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY_PORT, theApp.preferences.proxyHttpPort);
      DDX_Text(pDX, IDC_EDIT_PROXY_NO_PROXY, theApp.preferences.proxyNoProxy);
      DDX_Check(pDX, IDC_CHECK_PROXY_TYPE, theApp.preferences.proxyType);
      break;
    case IDD_PREFERENCES_PARANOIA:
      DDX_Check(pDX, IDC_CHECK_JAVA, theApp.preferences.bJavaEnabled);
      DDX_Check(pDX, IDC_CHECK_JAVASCRIPT, theApp.preferences.bJavascriptEnabled);
      DDX_Check(pDX, IDC_CHECK_COOKIES, theApp.preferences.bCookiesEnabled);
      DDX_Check(pDX, IDC_CHECK_CSS, theApp.preferences.bCSSEnabled);
      DDX_Check(pDX, IDC_CHECK_IMAGES, theApp.preferences.bImagesEnabled);
      DDX_Check(pDX, IDC_CHECK_ANIMATIONS, theApp.preferences.bAnimationsEnabled);
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
      case IDD_PREFERENCES_GENERAL:
         fDlg.m_ofn.lpstrFilter = "Executable Files\0*.exe\0";
         fDlg.DoModal();
         theApp.preferences.sourceCommand = fDlg.GetPathName();
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
      if (kPlugin->pf->Config){
        kPlugin->pf->Config(this->m_hWnd);
      }
      break;
    }
    item--;
  }
}

/**/

BOOL CPreferencePageMenus::OnInitDialog(){
   CDialog::OnInitDialog();

   ShowFile("menus.cfg");
   m_nCurrentFile = 0;

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CPreferencePageMenus::OnDestroy(){
   if (SendDlgItemMessage(IDC_EDIT1, EM_GETMODIFY)){
      if (m_nCurrentFile == 0){
         SaveFile("menus.cfg");
      }
      else if (m_nCurrentFile == 1){
         SaveFile("accel.cfg");
      }
   }
}

void CPreferencePageMenus::OnHelp(){
   if (m_nCurrentFile == 0){
      MessageBox(
         "Start a comment with a # as the first thing on the line \n\
         Start a menu with MenuName { \n\
         End a menu with } \n\
         Use : to insert a submenu (that you defined earlier) \n\
         To insert a plugin, use pluginname.dll(parameters) \n\
         To insert a menu item, use MenuItem \\t AccelKey = ID",
         "menus.cfg Syntax");
   }
   else if (m_nCurrentFile == 1){
      MessageBox(
         "Start a comment with a # as the first thing on the line \n\
         To define an accelerator, use Modifier Key = ID \n\
         Where Modifier is CTRL, ALT, or SHIFT",
         "menus.cfg Syntax");
   }
}

void CPreferencePageMenus::SaveFile(char *filename){
   if (MessageBox("Do you wish to save your changes?", filename, MB_YESNO) == IDNO){
      return;
   }
   CFile file;
   if (file.Open(theApp.preferences.settingsDir + filename, CFile::modeWrite)){
      UpdateData();
      file.Write(m_fileText, m_fileText.GetLength());
   }
   else{
      MessageBox("Error opening file");
   }
   file.Close();
}

void CPreferencePageMenus::ShowFile(char *filename){
   CFile file;
   if (file.Open(theApp.preferences.settingsDir + filename, CFile::modeRead)){
      int length = file.GetLength();
      char *buffer = new char[length+1];
      buffer[file.Read(buffer, length)] = 0;

      m_fileText = buffer;

      delete [] buffer;

      UpdateData(FALSE);
   }
   else{
      MessageBox("Error opening file");
   }
   file.Close();
   m_currentFile = filename;
}

void CPreferencePageMenus::OnMenus(){
   if (SendDlgItemMessage(IDC_EDIT1, EM_GETMODIFY))
      SaveFile(m_currentFile);
   ShowFile("menus.cfg");
   m_nCurrentFile = 0;

   SendDlgItemMessage(IDC_EDIT1, EM_SETMODIFY, 0);
}

void CPreferencePageMenus::OnAccel(){
   if (SendDlgItemMessage(IDC_EDIT1, EM_GETMODIFY))
      SaveFile(m_currentFile);
   ShowFile("accel.cfg");
   m_nCurrentFile = 1;

   SendDlgItemMessage(IDC_EDIT1, EM_SETMODIFY, 0);
}

void CPreferencePageMenus::DoDataExchange(CDataExchange* pDX){
   CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePagePlugins)
   DDX_Text(pDX, IDC_EDIT1, m_fileText);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencePageMenus, CPreferencePage)
	//{{AFX_MSG_MAP(CPreferencePageMenus)
   ON_BN_CLICKED(IDC_BUTTON_HELP, OnHelp)
   ON_BN_CLICKED(IDC_BUTTON_MENUS, OnMenus)
   ON_BN_CLICKED(IDC_BUTTON_ACCEL, OnAccel)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()
