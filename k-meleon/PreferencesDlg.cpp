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

   AddItem(_T("Display"), IDD_PREFERENCES_DISPLAY);
   AddItem(_T("General"), IDD_PREFERENCES_GENERAL);
   AddItem(_T("Privacy"), IDD_PREFERENCES_PRIVACY);
   AddItem(_T("Cache"),   IDD_PREFERENCES_CACHE);
   AddItem(_T("Proxy"),   IDD_PREFERENCES_PROXY);
   AddItem(_T("Configs"), IDD_PREFERENCES_CONFIGS);
   AddItem(_T("Settings"), IDD_PREFERENCES_MOZCONFIGS);
   AddItem(_T("Plugins"), IDD_PREFERENCES_PLUGINS);

   m_list.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

   return TRUE;  // return TRUE  unless you set the focus to a control
}

CPreferencesDlg::~CPreferencesDlg(){
}

int CPreferencesDlg::DoModal(){
   int ret = CDialog::DoModal();

   if (page){
      delete page;
      page = NULL;
   }

   theApp.preferences.SaveDlgPrefs();

   return ret;
}

void CPreferencesDlg::OnOK() {
   if (page) {
      page->UpdateData();
      delete page;
      page = NULL;
   }
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

  if (idd == IDD_PREFERENCES_PLUGINS)
    page = new CPreferencePagePlugins;

  else if (idd == IDD_PREFERENCES_CONFIGS)
     page = new CPreferencePageConfigs;

  else if (idd == IDD_PREFERENCES_MOZCONFIGS)
     page = new CPreferencePageMozConfigs;

  else
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
   switch (idd) {
      case IDD_PREFERENCES_PRIVACY:
      {
         char buf[256], uabuf[256], pref[34];
         int x=1,y,index=0;
         SendDlgItemMessage(IDC_COMBO, CB_ADDSTRING, 0, (LONG) "Default");
         do {
            sprintf(pref, "kmeleon.privacy.useragent%d.name", x);
            theApp.preferences.GetString(pref, buf, "");
            if (*buf)
               SendDlgItemMessage(IDC_COMBO, CB_ADDSTRING, 0, (LONG) buf);
            x++;
         } while (*buf);
         
         theApp.preferences.GetString("general.useragent.override", uabuf, "");
         if (*uabuf) {
            for (y=1; y<x; y++) {
               sprintf(pref, "kmeleon.privacy.useragent%d.string", y);
               theApp.preferences.GetString(pref, buf, "");
               if (strcmp(buf, uabuf) == 0)
                  index=y;
            }
         }
         SendDlgItemMessage(IDC_COMBO, CB_SETCURSEL, index, 0);
         
         break;
      }
      
      case IDD_PREFERENCES_DISPLAY:
      {
         int i=0, index=0;
         WIN32_FIND_DATA ffd;
         CString fname = theApp.preferences.skinsDir + "*.*";
         HANDLE hFind = FindFirstFile(fname.GetBuffer(0), &ffd);
         if (hFind != INVALID_HANDLE_VALUE) {
            fname = theApp.preferences.skinsCurrent;
            fname = fname.Left(fname.GetLength()-1);
            do {
               if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
                   ffd.cFileName[0]!='.') {
                  SendDlgItemMessage(IDC_COMBO_SKIN, CB_ADDSTRING, 0, (LONG)ffd.cFileName);
                  if (fname.CompareNoCase(ffd.cFileName) == 0)
                     index=i;
                  i++;
               }
            } while ( FindNextFile(hFind, &ffd) );
            FindClose(hFind);
            SendDlgItemMessage(IDC_COMBO_SKIN, CB_SETCURSEL, index, 0);
         }
         break;
      }
   }
       
   CDialog::OnInitDialog();

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CPreferencePage::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePage)
  switch (idd){
    case IDD_PREFERENCES_DISPLAY:
      DDX_Radio(pDX, IDC_RADIO_START_BLANK, theApp.preferences.bStartHome);
      DDX_Text(pDX, IDC_EDIT_HOMEPAGE, theApp.preferences.homePage);
      // DDX_Text(pDX, IDC_EDIT_TOOLBAR_BACKGROUND, theApp.preferences.toolbarBackground);
      DDX_Check(pDX, IDC_CHECK_TOOLBAR_BACKGROUND, theApp.preferences.bToolbarBackground);
      DDX_Radio(pDX, IDC_RADIO_NEWWINDOW, theApp.preferences.iNewWindowOpenAs);
      DDX_Text(pDX, IDC_EDIT_URL, theApp.preferences.newWindowURL);
      break;
    case IDD_PREFERENCES_GENERAL:
      DDX_Check(pDX, IDC_CHECK_JAVASCRIPT, theApp.preferences.bJavascriptEnabled);
      DDX_Check(pDX, IDC_CHECK_JAVA, theApp.preferences.bJavaEnabled);
      DDX_Check(pDX, IDC_CHECK_PASSWORDS, theApp.preferences.bRememberSignons);
      DDX_Text(pDX, IDC_EDIT_HTTP, theApp.preferences.httpVersion);
      DDX_Text(pDX, IDC_EDIT_SETTINGS_DIR, theApp.preferences.settingsDir);
      DDX_Text(pDX, IDC_EDIT_PLUGINS_DIR, theApp.preferences.pluginsDir);
      DDX_Check(pDX, IDC_CHECK_SOURCE_ENABLED, theApp.preferences.bSourceUseExternalCommand);
      DDX_Text(pDX, IDC_EDIT_SOURCE_COMMAND, theApp.preferences.sourceCommand);
      DDX_Check(pDX, IDC_CHECK_DISABLERESIZE, theApp.preferences.bDisableResize);
      break;
    case IDD_PREFERENCES_PROXY:
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY,        theApp.preferences.proxyHTTP);
      DDX_Text(pDX, IDC_EDIT_HTTP_PROXY_PORT,   theApp.preferences.proxyHTTPPort);
      DDX_Text(pDX, IDC_EDIT_FTP_PROXY,         theApp.preferences.proxyFTP);
      DDX_Text(pDX, IDC_EDIT_FTP_PROXY_PORT,    theApp.preferences.proxyFTPPort);
      DDX_Text(pDX, IDC_EDIT_SSL_PROXY,         theApp.preferences.proxySSL);
      DDX_Text(pDX, IDC_EDIT_SSL_PROXY_PORT,    theApp.preferences.proxySSLPort);
      DDX_Text(pDX, IDC_EDIT_GOPHER_PROXY,      theApp.preferences.proxyGopher);
      DDX_Text(pDX, IDC_EDIT_GOPHER_PROXY_PORT, theApp.preferences.proxyGopherPort);
      DDX_Text(pDX, IDC_EDIT_SOCKS_PROXY,       theApp.preferences.proxySOCKS);
      DDX_Text(pDX, IDC_EDIT_SOCKS_PROXY_PORT,  theApp.preferences.proxySOCKSPort);
      DDX_Radio(pDX, IDC_RADIO_SOCKS,           theApp.preferences.proxySOCKSRadio);

      DDX_Text(pDX, IDC_EDIT_PROXY_AUTO,  theApp.preferences.proxyAutoURL);
      DDX_Text(pDX, IDC_EDIT_PROXY_NO_PROXY, theApp.preferences.proxyNoProxy);
      DDX_Radio(pDX, IDC_RADIO_PROXY, theApp.preferences.proxyType);
      break;
    case IDD_PREFERENCES_CACHE:
      DDX_Text(pDX, IDC_EDIT_MEMORY_CACHE, theApp.preferences.cacheMemory);
      DDX_Text(pDX, IDC_EDIT_DISK_CACHE, theApp.preferences.cacheDisk);
      DDX_Text(pDX, IDC_EDIT_CACHE_DIRECTORY, theApp.preferences.cacheDir);
      DDX_Radio(pDX, IDC_RADIO_CACHE, theApp.preferences.cacheCheckFrequency);
      break;
   case IDD_PREFERENCES_PRIVACY:
      DDX_Check(pDX, IDC_CHECK_ANIMATIONS, theApp.preferences.bAnimationsEnabled);
      DDX_Radio(pDX, IDC_IMAGES_ALL, theApp.preferences.iImagesEnabled);
      DDX_Radio(pDX, IDC_COOKIES_ALL, theApp.preferences.iCookiesEnabled);      
      DDX_Check(pDX, IDC_CHECK_LOAD, theApp.preferences.bDisablePopupsOnLoad);

      DDX_Text(pDX, IDC_EDIT_USERAGENT, theApp.preferences.userAgent);
      break;
  }
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPreferencePage, CDialog)
	//{{AFX_MSG_MAP(CPreferencePage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_MEM_CACHE, OnClearMemCache)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_DISK_CACHE, OnClearDiskCache)
   ON_CBN_SELCHANGE(IDC_COMBO, OnComboChanged)
   ON_CBN_SELCHANGE(IDC_COMBO_SKIN, OnComboChanged)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPreferencePage::OnClearMemCache() {
   nsresult rv;

   nsCOMPtr<nsICacheService> CacheService =
      do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
   if (NS_FAILED(rv)) return;

   CacheService->EvictEntries(nsICache::STORE_IN_MEMORY);
}

void CPreferencePage::OnClearDiskCache() {
   nsresult rv;

   nsCOMPtr<nsICacheService> CacheService =
      do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
   if (NS_FAILED(rv)) return;

   CacheService->EvictEntries(nsICache::STORE_ON_DISK);
   CacheService->EvictEntries(nsICache::STORE_ON_DISK_AS_FILE);
}

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

void CPreferencePage::OnComboChanged() {
   switch (idd) {
      case IDD_PREFERENCES_PRIVACY:
      {
         int index;
         char buf[256], pref[34];

         index = SendDlgItemMessage(IDC_COMBO, CB_GETCURSEL, 0, 0);

          if (index == 0)
             theApp.preferences.GetString("general.useragent.override", buf, "");
          else {
             sprintf(pref, "kmeleon.privacy.useragent%d.string", index);
             theApp.preferences.GetString(pref, buf, "");
          }

         if (*buf)
            SetDlgItemText(IDC_EDIT_USERAGENT, buf);
         else
            SetDlgItemText(IDC_EDIT_USERAGENT, "");

         break;
      }
      case IDD_PREFERENCES_DISPLAY:
      {
         int index;
         char buf[256];

         index = SendDlgItemMessage(IDC_COMBO_SKIN, CB_GETCURSEL, 0, 0);

         if (index != CB_ERR) {
            SendDlgItemMessage(IDC_COMBO_SKIN, CB_GETLBTEXT, index, (LPARAM)buf);
            theApp.preferences.skinsCurrent = buf;
            theApp.preferences.skinsCurrent = theApp.preferences.skinsCurrent + "\\";
         }
         break;
      }
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

   m_imageList.Create(16, 16, TRUE, 4, 4);
   m_imageList.Add(theApp.LoadIcon(IDI_OFF));
   m_imageList.Add(theApp.LoadIcon(IDI_ON));
   m_imageList.Add(theApp.LoadIcon(IDI_OFFCHECK));
   m_imageList.Add(theApp.LoadIcon(IDI_ONCHECK));
   m_pluginList.SetImageList(&m_imageList, LVSIL_SMALL);

   while (pos) {
      theApp.plugins.pluginList.GetNextAssoc(pos, s, kPlugin);

      int item = m_pluginList.GetItemCount();

      char preference[128] = "kmeleon.plugins.";
      strcat(preference, kPlugin->dllname);
      strcat(preference, ".load");

      int image=kPlugin->loaded;
      if(theApp.preferences.GetBool(preference, 1)) image+=2;
      m_pluginList.InsertItem(item, kPlugin->description, image);
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
  ON_BN_CLICKED(IDC_BUTTON_ENABLE, OnEnable)
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

      if (item == 0) {
         if (kPlugin->loaded) {
            theApp.plugins.SendMessage(kPlugin->dllname, "* Prefs Page", "Config", (long)this->m_hWnd);
         }
         else
            AfxMessageBox(IDS_PLUGIN_NOT_LOADED);

         break;
      }
      item--;
   }
}

void CPreferencePagePlugins::OnEnable() {
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
   int i=item;
   while (pos){
      theApp.plugins.pluginList.GetNextAssoc( pos, s, kPlugin);

      if (i == 0) {
         char preference[128] = "kmeleon.plugins.";
         strcat(preference, kPlugin->dllname);
         strcat(preference, ".load");
         theApp.preferences.SetBool(preference, !theApp.preferences.GetBool(preference, 1));

         int image=kPlugin->loaded;
         if(theApp.preferences.GetBool(preference, 1)) image+=2;
         m_pluginList.SetItem(item, 0, LVIF_IMAGE, NULL, image, NULL, NULL, NULL);
         break;
      }
      i--;
   }
}

/**/

BOOL CPreferencePageConfigs::OnInitDialog(){
   CDialog::OnInitDialog();

   CEdit *editBox = (CEdit *)GetDlgItem(IDC_EDIT1);
   editBox->SetTabStops(6);

   AddTab("Menus", theApp.preferences.settingsDir + "menus.cfg", "");
   AddTab("Accelerators", theApp.preferences.settingsDir + "accel.cfg", "");

   configFileType pluginConfigFiles[10];
   int numConfigFiles = theApp.plugins.GetConfigFiles(pluginConfigFiles, 10);
   int i;
   for (i=0; i<numConfigFiles; i++) {
      AddTab(pluginConfigFiles[i].label, pluginConfigFiles[i].file, pluginConfigFiles[i].helpUrl);
   }

   ShowFile(m_configFiles[0]);

	return FALSE;  // return TRUE  unless you set the focus to a control
}

CPreferencePageConfigs::~CPreferencePageConfigs() {
}

void CPreferencePageConfigs::AddTab(const char *label, const char *file, const char *help)
{
   int newItem = m_tabCtrl.GetItemCount();
   m_tabCtrl.InsertItem(newItem, label);
   m_configFiles.InsertAt(newItem, file);
}

void CPreferencePageConfigs::OnHelp(){
   MessageBox("Somewhere on kmeleon.org there should be a help file.  Eventually, the information will be here too.");
}

void CPreferencePageConfigs::SaveFile(const char *filename)
{
   const char *prettyFilename = strrchr(filename, '\\');
   if (prettyFilename) {
      prettyFilename++; // hop over the '\'
   } else {
      prettyFilename = filename;
   }
   if (MessageBox("Do you wish to save your changes?", prettyFilename, MB_YESNO) == IDNO){
      return;
   }
   CFile file;
   if (file.Open(filename, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {
      /* binary is so Write treats cr/lf as 2 characters */
      file.Write(m_fileText, m_fileText.GetLength());
      file.Close();
   }
   else
      MessageBox("Error opening file");
}

void CPreferencePageConfigs::ShowFile(const char *filename){
   CFile file;
   if (file.Open(filename, CFile::modeRead | CFile::typeBinary)){
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
}

void CPreferencePageConfigs::OnSelChange(NMHDR *nmHdr, LRESULT *result)
{
   ShowFile(m_configFiles[m_tabCtrl.GetCurSel()]);

   SendDlgItemMessage(IDC_EDIT1, EM_SETMODIFY, 0);
}

void CPreferencePageConfigs::OnSelChanging(NMHDR *nmHdr, LRESULT *result)
{
   UpdateData();

   *result = false;
}

void CPreferencePageConfigs::DoDataExchange(CDataExchange* pDX){
   CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencePagePlugins)
   DDX_Text(pDX, IDC_EDIT1, m_fileText);
   DDX_Control(pDX, IDC_TAB1, m_tabCtrl);
   //}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate){
      if (SendDlgItemMessage(IDC_EDIT1, EM_GETMODIFY)){
         SaveFile(m_configFiles[m_tabCtrl.GetCurSel()]);
      }
   }
}

BEGIN_MESSAGE_MAP(CPreferencePageConfigs, CPreferencePage)
	//{{AFX_MSG_MAP(CPreferencePageConfigs)
   ON_BN_CLICKED(IDC_BUTTON_HELP, OnHelp)
   ON_NOTIFY(TCN_SELCHANGING, IDC_TAB1, OnSelChanging)
   ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelChange)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/**/

CPreferencePageMozConfigs::CPreferencePageMozConfigs() {
   nsCOMPtr<nsIObserverService> observerService;
   observerService = do_GetService("@mozilla.org/observer-service;1");
   if (observerService) {
     observerService->NotifyObservers(nsnull, "profile-before-change", nsnull);      
   }
}

BOOL CPreferencePageMozConfigs::OnInitDialog(){
   CDialog::OnInitDialog();

   CEdit *editBox = (CEdit *)GetDlgItem(IDC_EDIT1);
   editBox->SetTabStops(6);

   AddTab("Prefs.js", theApp.preferences.profileDir + "prefs.js", "");
   AddTab("User.js", theApp.preferences.profileDir + "user.js", "");
   AddTab("userContent.css", theApp.preferences.profileDir + "chrome\\userContent.css", "");
   AddTab("Cookies", theApp.preferences.profileDir + "cookies.txt", "");
   AddTab("Permissions", theApp.preferences.profileDir + "cookperm.txt", "");
   AddTab("History", theApp.preferences.profileDir + "history.txt", "");

   ShowFile(m_configFiles[0]);

	return FALSE;  // return TRUE  unless you set the focus to a control
}

CPreferencePageMozConfigs::~CPreferencePageMozConfigs() {
   nsCOMPtr<nsIObserverService> observerService;
   observerService = do_GetService("@mozilla.org/observer-service;1");
   if (observerService) {
     observerService->NotifyObservers(nsnull, "profile-do-change", nsnull);      
   }
}
