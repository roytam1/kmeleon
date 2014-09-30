/*
*  Copyright (C) 2005 Dorian Boissonnade
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
*
*/

#pragma once

#include "DialogEx.h"
#include "resource.h"
#include <nsIGenKeypairInfoDlg.h>

// 6a8b1aff-ae8b-4751-982e-4ce5ad544100
#define NS_NSSKEYPAIRDIALOGS_CID	\
 {0x6a8b1aff, 0xae8b, 0x4751, {0x98, 0x2e, 0x4c, 0xe5, 0xad, 0x54, 0x41, 0x10}}

#define GTK_NSSKEYPAIRDIALOGS_CLASSNAME  "Gtk NSS Key Pair Dialogs"

class CGenKeyPairDialogs : public nsIGeneratingKeypairInfoDialogs
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIGENERATINGKEYPAIRINFODIALOGS

	CGenKeyPairDialogs();
	virtual ~CGenKeyPairDialogs();
};

class GenKeyPairObserver : public nsIObserver
{
public:
	NS_DECL_NSIOBSERVER
	NS_DECL_ISUPPORTS

	GenKeyPairObserver(CWnd* wnd) : mWnd(wnd) {};
	virtual ~GenKeyPairObserver() {};

	CWnd* mWnd;
};

class CGenKeyPairDialog : public CDialog
{
	//DECLARE_DYNAMIC(CGenKeyPairDialog)

public:
	CGenKeyPairDialog(nsIKeygenThread* runnable, CWnd* pParent = NULL);
	virtual ~CGenKeyPairDialog();

// Données de boîte de dialogue
	enum { IDD = IDD_GENKEYPAIR };

protected:
    nsIKeygenThread* mRunnable;
	nsCOMPtr<GenKeyPairObserver> mObserver;
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV
	
	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	afx_msg LRESULT OnGenDone(WPARAM, LPARAM);
public:
	virtual BOOL OnInitDialog();
};
