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

class CPreferences {
public:
  // -- data
  CString toolbarBackground;
  int bToolbarBackground;

  CString homePage;
  int bStartHome;

  // this holds the menu.txt files
  CString settingsDir;

  // if true, will call Save on destruction
  int bAutoSave;

  // -- functions

  CPreferences();
  ~CPreferences();

  Save();
  Load();
};

#endif