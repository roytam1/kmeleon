/*
*  Copyright (C) 2006 Dorian Boissonnade
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

#include "stdafx.h"
#include "GenKeyPairDialogs.h"

#include <nsIKeygenThread.h>

extern CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

NS_IMPL_ISUPPORTS (CGenKeyPairDialogs, nsIGeneratingKeypairInfoDialogs)

CGenKeyPairDialogs::CGenKeyPairDialogs()
{
}

CGenKeyPairDialogs::~CGenKeyPairDialogs()
{
}

NS_IMETHODIMP CGenKeyPairDialogs::DisplayGeneratingKeypairInfo (nsIInterfaceRequestor *ctx,
						    nsIKeygenThread *runnable)
{
	nsCOMPtr<nsIDOMWindow> parent = do_GetInterface (ctx);
	CWnd* wnd = CWndForDOMWindow(parent);
	if (wnd)
		wnd = wnd->GetLastActivePopup();

	CGenKeyPairDialog dlg(runnable, wnd);
	dlg.DoModal();

	return NS_OK;
}

// Boîte de dialogue CGenKeyPairDialog

//IMPLEMENT_DYNAMIC(CGenKeyPairDialog, CDialog)
CGenKeyPairDialog::CGenKeyPairDialog(nsIKeygenThread* runnable, CWnd* pParent /*=NULL*/)
	: CDialog(CGenKeyPairDialog::IDD, pParent), mRunnable(runnable)
{
}

CGenKeyPairDialog::~CGenKeyPairDialog()
{
}

void CGenKeyPairDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGenKeyPairDialog, CDialog)
	ON_MESSAGE(WM_USER+100, OnGenDone)
END_MESSAGE_MAP()

NS_IMPL_ISUPPORTS(GenKeyPairObserver, nsIObserver);

NS_IMETHODIMP GenKeyPairObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
	if (mWnd)
		mWnd->PostMessage(WM_USER+100, 0, 0);
	return NS_OK;
}

// Gestionnaires de messages CGenKeyPairDialog

void CGenKeyPairDialog::OnCancel()
{
	/* CRASH
	PRBool already_closed = FALSE;
	mRunnable->UserCanceled(&already_closed);
	CDialog::OnCancel();*/
}

BOOL CGenKeyPairDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CWnd* okButton = GetDlgItem(IDOK);
	if (okButton)
		okButton->EnableWindow(FALSE);
	
	mObserver = new GenKeyPairObserver(this);
	mRunnable->StartKeyGeneration(mObserver);
	return TRUE; 
}

LRESULT CGenKeyPairDialog::OnGenDone(WPARAM, LPARAM)
{
	CWnd* okButton = GetDlgItem(IDOK);
	if (okButton)
		okButton->EnableWindow(TRUE);
	OnOK(); 
	return 0;
}

