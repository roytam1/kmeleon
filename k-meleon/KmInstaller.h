/*
*  Copyright (C) 2014 Dorian Boissonnade
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
*
*/

#pragma once

#include "nsIWebBrowserPersist.h"
#include "nsCWebBrowserPersist.h"
#include "kmeleon_plugin.h"
#include "MozUtils.h"
#include "GenericDlg.h"

class KmInstaller {
protected:
	nsCOMPtr<nsIFile> mFile;
	nsCOMPtr<nsIURI> mURI;
public:

	virtual bool Install() = 0;	

	enum {
		TYPE_SKIN = 0
	};

	nsresult Download(nsIURI* uri, nsString filename) {
		nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
		if(!persist) return NS_ERROR_ABORT;

		CString dest = theApp.GetFolder(UserSkinsFolder);

		NS_NewLocalFile(nsDependentString(dest), TRUE, getter_AddRefs(mFile));
		bool exists;
		mFile->Exists(&exists);
		if (!exists) mFile->Create(nsIFile::DIRECTORY_TYPE, 0);
		mFile->Append(filename);
		mURI = uri;

		CProgressDialog *progress = new CProgressDialog(FALSE);
		//persist->SetProgressListener(progress);
		progress->InitPersist(uri, mFile, persist, FALSE); 
		progress->SetCallBack(KmInstaller::DwnCall, this);
		persist->SetPersistFlags(
			nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
			nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);
		nsresult rv = persist->SaveURI(uri, nullptr, nullptr, nullptr, nullptr, mFile, nullptr);
		if (NS_FAILED(rv)) {
			persist->SetProgressListener(nullptr);
			return rv;
		}
		return NS_OK;
	}	

	static void DwnCall(char*, LPCTSTR file, nsresult, void* param) {
		KmInstaller* installer = (KmInstaller*)param;
		if (!installer->Install()) {
			AfxMessageBox(IDS_INSTALL_FAILED, MB_OK|MB_ICONERROR);
		}
		::DeleteFile(file);
		delete installer;
	}

	static bool InstallFromUrl(unsigned type, nsIURI* uri, nsString filename);
};

class KmSkinInstaller: public KmInstaller
{
	bool Install() {
		nsresult rv;
		
		nsCOMPtr<nsIFile> folder;
		CString dest = theApp.GetFolder(UserSkinsFolder);
		NS_NewLocalFile(nsDependentString(dest), TRUE, getter_AddRefs(folder));
		if (!folder) return false;

		nsString filename;
		mFile->GetLeafName(filename);
		filename.Cut(filename.Length()-4,-1);
		folder->Append(filename);

		bool destExists;
		folder->Exists(&destExists);
		if (!destExists) {
			rv = folder->Create(nsIFile::DIRECTORY_TYPE, 0);
			NS_ENSURE_SUCCESS(rv, false);
		} else {
			CString str;
			str.LoadString(IDS_SKIN_ALREADY_EXISTS);
			if (AfxMessageBox(IDS_SKIN_ALREADY_EXISTS, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
				return false;
			folder->Remove(true);
			rv = folder->Create(nsIFile::DIRECTORY_TYPE, 0);
			NS_ENSURE_SUCCESS(rv, false);
		}
		
		if (!ZipExtractFiles(mFile, folder)) {
			if (!destExists) folder->Remove(true);
			return false;
		}

		return true;
	}
};

bool KmInstaller::InstallFromUrl(unsigned type, nsIURI* uri, nsString filename) {
	CString msg;
	msg.Format(IDS_INSTALL_CONFIRM, filename.get());
	if (AfxMessageBox(msg, MB_YESNO|MB_ICONQUESTION) != IDYES)
		return false;
	KmInstaller* installer = new KmSkinInstaller();
	return NS_SUCCEEDED(installer->Download(uri, filename));
}