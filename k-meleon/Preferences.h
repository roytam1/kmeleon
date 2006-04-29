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
//  Holds various preferences for k-meleon.  also controls the dialog box

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include "StdAfx.h"

#include "resource.h"
#include "DialogEx.h"

class CPreferences {
public:

   // -- Find settings
   CString findSearchStr;
	int bFindMatchCase;
	int bFindHighlight;
	int bFindMatchWholeWord;
   int bFindWrapAround;
	int bFindSearchBackwards;
   
   // -- Display
   CString toolbarBackground;
   int bToolbarBackground;

   int iNewWindowOpenAs;
   CString newWindowURL;

   CString homePage;
   int bStartHome;
   BOOL bOffline;

   BOOL bGuestAccount;

   CString searchEngine;
 
   int iSaveType;
   CString settingsDir;
   CString pluginsDir;
   CString profileDir;

   CString skinsDir;
   CString skinsCurrent;
   CString skinsPath;

   int bDisableResize;
   int iFontMinSize;

   // true if we should hide the taskbar buttons
   int bHideTaskBarButtons;

   // true if the window should default to maximized
   int bMaximized;

   // recent window size
   int windowHeight;
   int windowWidth;
   int windowXPos;
   int windowYPos;

   int bNewWindowHasUrlFocus;

   // use external source viewer
   int bSourceUseExternalCommand;
   CString sourceCommand;
  
   // -- yummy proxies
   int proxyType;

   CString  proxyHTTP;
   int      proxyHTTPPort;
   CString  proxyFTP;
   int      proxyFTPPort;
   CString  proxySSL;
   int      proxySSLPort;
   CString  proxyGopher;
   int      proxyGopherPort;
   CString  proxySOCKS;
   int      proxySOCKSPort;
   int      proxySOCKSRadio;   // 0 = version 4, 1 = version 5
   int      proxySOCKSVersion;
   CString  proxyAutoURL;
   CString  proxyNoProxy;


   // -- cache
   int cacheMemory;
   int cacheDisk;
   CString cacheDir;
   int cacheCheckFrequency;

   
   // -- Advanced
   int iCookiesEnabled;
   int iImagesEnabled;

   int bJavaEnabled;
   int bJavascriptEnabled;
   int bRememberSignons;
   int bAnimationsEnabled;


   CString httpVersion;

   int MRUbehavior;

   // -- Privacy

   int bDisablePopupsOnLoad;
   int bRestrictPopups;
   CString restrictedPopupSites;
   CString userAgent;


   // -- Printing

   CString printMarginTop;
   CString printMarginRight;
   CString printMarginLeft;
   CString printMarginBottom;
   int printScaling;
   int printShrinkToFit;
   
   int printBGColors;
   int printBGImages;
   
   int printUnit; // inches or cm
   CString printWidth;
   CString printHeight;
   
   CString printHeaderLeft;
   CString printHeaderRight;
   CString printHeaderMiddle;
   CString printFooterLeft;
   CString printFooterRight;
   CString printFooterMiddle;


   // -- Download
   
   CString saveDir;
   CString downloadDir;
   CString lastDownloadDir;
   int bUseDownloadDir;
   int bAskOpenSave;
   int bCloseDownloadDialog;
   int bShowMinimized;
   int bFlashWhenCompleted;
   int bSaveUnknowContent;
   int bSaveUseTitle;


   // -- functions

   CPreferences();
   ~CPreferences();

   void Flush();
   void Save();
   void Load();

   void SetBool(const char *preference, int value);
   int  GetBool(const char *preference, int defaultVal);

   void SetInt(const char *preference, int value);
   int  GetInt(const char *preference, int defaultVal);

   void SetString(const char *preference, char * value);
   void SetString(const char *preference, wchar_t * value);
   int GetString(const char *preference, char * retValue, char * defaultVal);
   int GetString(const char *preference, wchar_t * retValue, wchar_t * defaultVal);

   void Clear(const char *preference);
   void DeleteBranch(const char *startingAt);
};

class CPreferencePage : public CDialog {
public:
   UINT idd;
protected:

   virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

   virtual BOOL OnInitDialog();

   afx_msg void OnClearDiskCache();
   afx_msg void OnClearMemCache();
   afx_msg void OnBrowse();
   afx_msg void OnComboChanged();
   afx_msg void OnViewCookies();
   afx_msg void OnViewPasswords();
   afx_msg void OnCookiePermissions();
   afx_msg void OnImagePermissions();
   afx_msg void OnPopupPermissions();
   afx_msg void OnBnClickedDisablePopup();

   virtual void OnOK();
   virtual void OnCancel();

   DECLARE_MESSAGE_MAP()
};

class CPreferencePagePlugins : public CPreferencePage {
protected:
  CListCtrl m_pluginList;
  CImageList m_imageList;

  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  virtual BOOL OnInitDialog();

  afx_msg void OnConfig();
  afx_msg void OnEnable();

  DECLARE_MESSAGE_MAP()
};

class CPreferencePageConfigs: public CPreferencePage {
protected:
  CString m_fileText;
  BOOL m_converted;

  CTabCtrl m_tabCtrl;

  CStringArray m_configFiles;

  void AddTab(const TCHAR *label, const TCHAR *file, const char *help);

  ~CPreferencePageConfigs();
  
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  virtual BOOL OnInitDialog();
  //  afx_msg void OnDestroy();

  void ShowFile(const TCHAR *);
  void SaveFile(const TCHAR *);

  afx_msg void OnHelp();
  afx_msg void OnSelChanging(NMHDR *nmHdr, LRESULT *result);
  afx_msg void OnSelChange(NMHDR *nmHdr, LRESULT *result);

  DECLARE_MESSAGE_MAP()
};


class CPreferencesDlg : public CDialog{
  friend CPreferencePage;
protected:
  //{{AFX_DATA(CPreferences)
	enum { IDD = IDD_PREFERENCES };
  CListCtrl	m_list;
  //}}AFX_DATA

  CPreferencePage *page;

public:
  CPreferencesDlg();
  ~CPreferencesDlg();

  int DoModal();

  BOOL Create(CWnd *pParent);

protected:
	//{{AFX_VIRTUAL(CPreferences)
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

  //{{AFX_MSG(CPreferences)
	virtual BOOL OnInitDialog();
	afx_msg void OnListSelect(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  virtual void OnOK();
  virtual void OnCancel();

  void Init();
  void ShowPage(UINT idd);

  void AddItem(LPCTSTR text, UINT idd);

};


#endif
