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

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include "StdAfx.h"

#include "resource.h"


class CPreferences {
public:
  // -- data
  CString toolbarBackground;
  int bToolbarBackground;

  CString homePage;
  int bStartHome;

  CString searchEngine;
 
  // this holds the menu.txt files
  CString settingsDir;

  // true if the window should default to maximized
  int bMaximized;

  int sourceEnabled;
  CString sourceCommand;
  
  // -- yummy proxies
  int proxyType;

  CString proxyHttp;
  int proxyHttpPort;

  CString proxyNoProxy;

  // -- paranoia
  int bJavaEnabled;
  int bJavascriptEnabled;
  int bCookiesEnabled;
  int bCSSEnabled;
  int bImagesEnabled;
  int bAnimationsEnabled;

  // -- functions

  CPreferences();
  ~CPreferences();

  void Save();
  void Load();

  void SetBool(const char *preference, int value);
  int  GetBool(const char *preference, int defaultVal);

  void SetInt(const char *preference, int value);
  int  GetInt(const char *preference, int defaultVal);

  void SetString(const char *preference, char * value);
  void GetString(const char *preference, char * retValue, char * defaultVal);
};

class CPreferencePage : public CDialog {
public:
  UINT idd;
protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  virtual BOOL OnInitDialog();

  afx_msg void OnBrowse();

  virtual void OnOK();
  virtual void OnCancel();

  DECLARE_MESSAGE_MAP()
};

class CPreferencePagePlugins : public CPreferencePage {
protected:
  CListCtrl m_pluginList;

  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  virtual BOOL OnInitDialog();

  afx_msg void OnConfig();

  DECLARE_MESSAGE_MAP()
};

class CPreferencePageMenus: public CPreferencePage {
protected:
  CString m_fileText;
  int m_nCurrentFile;
  char * m_currentFile;

  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  virtual BOOL OnInitDialog();
  afx_msg void OnDestroy();

  void ShowFile(char *);
  void SaveFile(char *);

  afx_msg void OnHelp();
  afx_msg void OnMenus();
  afx_msg void OnAccel();

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

  void AddItem(char *text, UINT idd);

};


#endif