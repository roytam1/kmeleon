/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
* Version: Mozilla-sample-code 1.0
*
* Copyright (c) 2002 Netscape Communications Corporation and
* other contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this Mozilla sample software and associated documentation files
* (the "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
* Contributor(s):
*   Conrad Carlen <conrad@ingress.com>
*   Benjamin Smedberg <bsmedberg@covad.net>
*
* ***** END LICENSE BLOCK ***** */

#include "StdAfx.h"
#include "KmFileLocProvider.h"
#include "MozUtils.h"

//#ifdef USE_FILELOCPROVIDER 

#ifndef XP_WIN
#define XP_WIN
#endif

#include "nsXULAppAPI.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsIProperties.h"
#include "nsServiceManagerUtils.h"
#include "nsISimpleEnumerator.h"

#include <windows.h>
#include <shlobj.h>

#include "MfcEmbed.h"
#include "kmeleon_plugin.h"
extern CMfcEmbedApp theApp;


// WARNING: These hard coded names need to go away. They need to
// come from localizable resources
#define PROFILE_INI_NAME  NS_LITERAL_STRING("profile.ini")
#define PROFILE_ROOT_DIR_NAME NS_LITERAL_STRING("Profiles")

/*#define DEFAULTS_DIR_NAME           nsCString("defaults")
#define DEFAULTS_PREF_DIR_NAME      nsCString("pref")
#define DEFAULTS_PROFILE_DIR_NAME   nsCString("profile")
#define RES_DIR_NAME                nsCString("res")
#define CHROME_DIR_NAME             nsCString("chrome")
#define PLUGINS_DIR_NAME            nsCString("plugins")
#define SEARCH_DIR_NAME             nsCString("searchplugins")
#define COMPONENTS_DIR_NAME         nsCString("components")*/

struct KmDirEntry {
	const char* nsProp;
	const FolderType kmProp;
};

static KmDirEntry kDirMap[] = {
	{K_APP_SKINS_DIR, SkinsFolder},
	{K_APP_KPLUGINS_DIR, PluginsFolder},
	{K_USER_SKINS_DIR, UserSkinsFolder},
	{K_USER_KPLUGINS_DIR, UserPluginsFolder},
	{K_APP_SETTING_DEFAULTS, DefSettingsFolder},
	{K_USER_SETTING, UserSettingsFolder}
};

static const int kDirTotal = NS_ARRAY_LENGTH(kDirMap);

#define K_APP_SKINS_DIR             "KASkins"
#define K_APP_KPLUGINS_DIR          "KAPlugins"
#define K_USER_SKINS_DIR            "KUSkins"
#define K_USER_KPLUGINS_DIR         "KUPlugins"
#define K_APP_SETTING_DEFAULTS      "KDefSettings"
#define K_USER_SETTING              "KUserSettings"

class CSimpleFileEnumerator : nsISimpleEnumerator
{
public:
	NS_DECL_ISUPPORTS

	CSimpleFileEnumerator() : mPos (NULL) {}
	~CSimpleFileEnumerator() {}

	void AddElement(LPCTSTR str)
	{
		mList.AddTail(str);
		mPos = mList.GetHeadPosition();
	}

	NS_IMETHODIMP HasMoreElements(bool *_retval)
	{
		*_retval =  (mPos != NULL);
		return NS_OK;
	}

	NS_IMETHODIMP GetNext(nsISupports **_retval)
	{
		NS_ENSURE_ARG_POINTER(_retval);
		*_retval = nullptr;

		if (mPos == NULL) return NS_ERROR_FAILURE;

		nsCOMPtr<nsIFile> localFile;
		CString str = mList.GetNext(mPos);

		nsresult rv;
#ifdef _UNICODE
		rv = NS_NewLocalFile(nsString(str), TRUE, getter_AddRefs(localFile));
#else
		rv = NS_NewNativeLocalFile(nsCString(str), TRUE, getter_AddRefs(localFile));
#endif
		NS_ENSURE_SUCCESS(rv, rv);

		localFile->QueryInterface(NS_GET_IID(nsISupports), (void**)_retval);
		NS_IF_ADDREF(*_retval);
		return NS_OK;
	}

protected:

	CList<CString, LPCTSTR> mList;
	POSITION mPos;

};

NS_IMPL_ISUPPORTS(CSimpleFileEnumerator, nsISimpleEnumerator)


//*****************************************************************************
// KmFileLocProvider::Constructor/Destructor
//*****************************************************************************   

KmFileLocProvider::KmFileLocProvider(const nsAString& aAppDataDirName)
{
	mProductDirName = aAppDataDirName;
}

KmFileLocProvider::~KmFileLocProvider()
{
}

//*****************************************************************************
// KmFileLocProvider::nsISupports
//*****************************************************************************   

NS_IMPL_ISUPPORTS(KmFileLocProvider, nsIDirectoryServiceProvider2, nsIDirectoryServiceProvider)

	//*****************************************************************************
	// KmFileLocProvider::nsIDirectoryServiceProvider
	//*****************************************************************************   

	NS_IMETHODIMP
	KmFileLocProvider::GetFiles(const char *prop, nsISimpleEnumerator **_retval)
{ 
	*_retval = nullptr;
	nsresult rv = NS_ERROR_FAILURE;

	/*	if (strcmp(prop, "ChromeML") == 0)
	{
		CSimpleFileEnumerator* fileEnum = new CSimpleFileEnumerator();
		if (!fileEnum) return NS_ERROR_OUT_OF_MEMORY;
		fileEnum->AddElement(GetMozDirectory(NS_APP_CHROME_DIR));
		fileEnum->AddElement(GetMozDirectory(NS_APP_USER_CHROME_DIR));
		fileEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void**)_retval);
		return NS_SUCCESS_AGGREGATE_RESULT;
	}

	Can't work yet
	if (strcmp(prop, NS_APP_PREFS_DEFAULTS_DIR_LIST) == 0)
	{
		CSimpleFileEnumerator* fileEnum = new CSimpleFileEnumerator();
		if (!fileEnum) return NS_ERROR_OUT_OF_MEMORY;
			CString profileDir = GetMozDirectory(NS_APP_USER_PROFILE_50_DIR);
		if (profileDir.GetLength())
			fileEnum->AddElement( + _T("\\default"));
		fileEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void**)_retval);
		return NS_SUCCESS_AGGREGATE_RESULT;
	}*/

	return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
KmFileLocProvider::GetFile(const char *prop, bool *persistant, nsIFile **_retval)
{    
	nsCOMPtr<nsIFile> localFile;
	nsresult rv = NS_ERROR_FAILURE;

	*_retval = nullptr;
	*persistant = PR_TRUE;
	if ((strcmp(prop, NS_APP_USER_PROFILE_50_DIR) == 0
		|| strcmp(prop, NS_APP_PROFILE_DIR_STARTUP) == 0)
		&& mProfileDirectory)
	{
		rv = mProfileDirectory->Clone(getter_AddRefs(localFile));
	}
	else if ((strcmp(prop, NS_APP_USER_PROFILE_LOCAL_50_DIR) == 0
		|| strcmp(prop, NS_APP_PROFILE_LOCAL_DIR_STARTUP) == 0)
		&& mProfileLocalDirectory)
	{
		rv = mProfileLocalDirectory->Clone(getter_AddRefs(localFile));
	}
	else if (strcmp(prop, XRE_EXECUTABLE_FILE) == 0)
	{
		TCHAR path[_MAX_PATH+1];
		::GetModuleFileName(0, path, _MAX_PATH);
		rv = NS_NewLocalFile(nsDependentString(path), TRUE, getter_AddRefs(localFile));
	} 
	else if (strcmp(prop, XRE_UPDATE_ROOT_DIR) == 0) 
	{
		// Prevent crash
		rv = CloneMozBinDirectory(getter_AddRefs(localFile));			
		localFile->Append(NS_LITERAL_STRING("dummy"));
	}
	/*else if(strcmp(prop, NS_GRE_DIR) == 0)
	{
		rv = CloneMozBinDirectory(getter_AddRefs(localFile));			
	}
		else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
	{
		rv = GetProductDirectory(getter_AddRefs(localFile));
	}
		else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
	{
		rv = GetProductDirectory(getter_AddRefs(localFile));
		if (NS_SUCCEEDED(rv))
		rv = localFile->AppendNative(APP_REGISTRY_NAME);
	}*/
	else if (!strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) ||
		!strcmp(prop, XRE_USER_APP_DATA_DIR)) {
			rv = GetProductDirectory(getter_AddRefs(localFile));
	}
	else if (strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
	{
		rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile));   
	}
	else if (strcmp(prop, NS_APP_USER_PROFILES_LOCAL_ROOT_DIR) == 0)
	{
		rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile), true);   
	}

	else {
		// Check for kmeleon related directories
		for (int i = 0; i < kDirTotal; i++) {
			if (strcmp(prop, kDirMap[i].nsProp) == 0) {
				CString folder = theApp.GetFolder(kDirMap[i].kmProp);
				rv = NS_NewLocalFile(nsDependentString(LPCTSTR(folder)), TRUE, getter_AddRefs(localFile));
				break;
			}
		}
	}

	if (localFile && NS_SUCCEEDED(rv))
		return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);

	return rv;
}

NS_METHOD KmFileLocProvider::CloneMozBinDirectory(nsIFile **aLocalFile)
{
	NS_ENSURE_ARG_POINTER(aLocalFile);
	nsresult rv;

	if (!mMozBinDirectory)
	{        

		CString path;
		::GetModuleFileName(0, path.GetBuffer(_MAX_PATH), _MAX_PATH);
		path.ReleaseBuffer(path.ReverseFind(_T('\\')));

		NS_NewLocalFile(nsDependentString(path),true,getter_AddRefs(mMozBinDirectory));
		/*
		// Get the mozilla bin directory
		// 1. Check the directory service first for NS_XPCOM_CURRENT_PROCESS_DIR
		//    This will be set if a directory was passed to NS_InitXPCOM
		// 2. If that doesn't work, set it to be the current process directory

		nsCOMPtr<nsIProperties> directoryService = 
			do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
		if (NS_FAILED(rv))
			return rv;

		rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), getter_AddRefs(mMozBinDirectory));
		if (NS_FAILED(rv)) {
			rv = directoryService->Get(NS_OS_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile), getter_AddRefs(mMozBinDirectory));
			if (NS_FAILED(rv))
				return rv;
		}*/
	}

	rv = mMozBinDirectory->Clone(aLocalFile);
	NS_IF_ADDREF(*aLocalFile);
	return rv;
}


//----------------------------------------------------------------------------------------
// GetProductDirectory - Gets the directory which contains the application data folder
//
// WIN    : <Application Data folder on user's machine>\Mozilla 
//----------------------------------------------------------------------------------------
NS_METHOD KmFileLocProvider::GetProductDirectory(nsIFile **aLocalFile, bool aLocal)
{
	NS_ENSURE_ARG_POINTER(aLocalFile);

	nsresult rv;

	if (theApp.cmdline.m_sProfilesDir)
	{
		nsCOMPtr<nsILocalFile> tempPath(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
		rv = tempPath->InitWithNativePath(nsDependentCString(theApp.cmdline.m_sProfilesDir));
		if (NS_FAILED(rv)) return rv;
		*aLocalFile = tempPath;
		NS_ADDREF(*aLocalFile);
		return NS_OK;
	}  		

	if (!aLocal && !mProductDirectory || aLocal && !mLocalProductDirectory)
	{

		bool exists;
		nsCOMPtr<nsIFile> localDir;

		nsCOMPtr<nsIFile> appDir;
		rv = CloneMozBinDirectory(getter_AddRefs(appDir));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIFile> profileIni;
		appDir->Clone(getter_AddRefs(profileIni));
		rv = profileIni->Append(PROFILE_INI_NAME);

		profileIni->Exists(&exists);
		if (exists)
		{
#ifdef _UNICODE
			nsString path;
			profileIni->GetPath(path);
#else
			nsCString path;
			profileIni->GetNativePath(path);
#endif
			TCHAR pszBuffer[4096] = {0};

			// Ugly stuff
			GetPrivateProfileString(_T("Profile"),_T("path"), _T(""), pszBuffer, 4096, path.get());
			UINT IsRelative = GetPrivateProfileInt(_T("Profile"),_T("isrelative"), 1, path.get());

#ifdef UNICODE	
			nsDependentString buffer(pszBuffer);
#else
			nsDependentCString buffer(pszBuffer);
#endif
			if (!buffer.IsEmpty())
			{
				if (!IsRelative)
#ifdef UNICODE
					rv = NS_NewLocalFile(buffer, TRUE, getter_AddRefs(localDir));
#else
					rv = NS_NewNativeLocalFile(buffer, TRUE, getter_AddRefs(localDir));
#endif
				else
				{
#ifdef UNICODE
					nsString profPath;
					appDir->GetPath(profPath);
					profPath.Append(L"\\");
					profPath.Append(buffer);
					rv = NS_NewLocalFile(profPath, TRUE, getter_AddRefs(localDir));
#else
					nsCString profPath;
					appDir->GetNativePath(profPath);
					profPath.Append("\\");
					profPath.Append(buffer);
					rv = NS_NewNativeLocalFile(profPath, TRUE, getter_AddRefs(localDir));
#endif
					NS_ENSURE_SUCCESS(rv, rv);
					rv = localDir->Normalize();
				}
			}
			else
			{
				rv = CloneMozBinDirectory(getter_AddRefs(localDir));
				NS_ENSURE_SUCCESS(rv, rv);
				rv = localDir->AppendRelativePath(PROFILE_ROOT_DIR_NAME);
			}

			NS_ENSURE_SUCCESS(rv, rv);
			rv = localDir->Exists(&exists);
			if (NS_SUCCEEDED(rv) && !exists)
				rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);

			*aLocalFile = localDir;
			NS_ADDREF(*aLocalFile);
		}
		else
		{
			int clsid = aLocal ? CSIDL_LOCAL_APPDATA : CSIDL_APPDATA;
			CString path;
			SHGetFolderPath(NULL, clsid, NULL, 0, path.GetBuffer(MAX_PATH));
			path.ReleaseBuffer();
			rv = NS_NewLocalFile(nsString(path), true, getter_AddRefs(localDir));
			if (NS_SUCCEEDED(rv))
				rv = localDir->Exists(&exists);

			if (NS_FAILED(rv) || !exists)
			{
				// If no appData, use installation folder
				rv = CloneMozBinDirectory(getter_AddRefs(localDir));
				NS_ENSURE_SUCCESS(rv, rv);
				rv = localDir->AppendRelativePath(PROFILE_ROOT_DIR_NAME);
			}
			else
				rv = localDir->Append(mProductDirName);
			NS_ENSURE_SUCCESS(rv, rv);

			rv = localDir->Exists(&exists);
			if (!exists) rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
			NS_ENSURE_SUCCESS(rv, rv);			
		}

		if (aLocal)
			mLocalProductDirectory = localDir ;
		else
			mProductDirectory = localDir;
	}

	if (aLocal)
		rv = mLocalProductDirectory->Clone(aLocalFile);
	else
		rv = mProductDirectory->Clone(aLocalFile);
	NS_IF_ADDREF(*aLocalFile);
	return rv;
}

//----------------------------------------------------------------------------------------
// GetDefaultUserProfileRoot - Gets the directory which contains each user profile dir
//
// WIN    : <Application Data folder on user's machine>\Mozilla\Users50 
//----------------------------------------------------------------------------------------
NS_METHOD KmFileLocProvider::GetDefaultUserProfileRoot(nsIFile **aLocalFile, bool aLocal)
{
	NS_ENSURE_ARG_POINTER(aLocalFile);

	nsresult rv;
	bool exists;
	nsCOMPtr<nsIFile> localDir;

	//if (!mProfileDirectory)
	{
		rv = GetProductDirectory(getter_AddRefs(localDir), aLocal);
		NS_ENSURE_SUCCESS(rv, rv);

		//rv = localDir->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
		//if (NS_FAILED(rv)) return rv;

		rv = localDir->Exists(&exists);
		if (NS_SUCCEEDED(rv) && !exists)
			rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	/*nsCOMPtr<nsIFile> aFile;
	rv = mProfileDirectory->Clone(getter_AddRefs(aFile));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsILocalFile> lfile = do_QueryInterface (aFile);
	if (!lfile) return NS_ERROR_FAILURE;*/

	NS_IF_ADDREF(*aLocalFile = localDir);

	return rv; 
}

CString KmFileLocProvider::GetProductDirectory(bool local)
{
	nsCOMPtr<nsIFile> nsDir;
	nsresult rv = GetProductDirectory(getter_AddRefs(nsDir), local);
	NS_ENSURE_SUCCESS(rv, _T(""));

#ifdef _UNICODE
	nsString pathBuf;
	rv = nsDir->GetPath(pathBuf);
#else
	nsCString pathBuf;
	rv = nsDir->GetNativePath(pathBuf);
#endif

	return pathBuf.get();
}

BOOL KmFileLocProvider::SetProfile(nsAString& aDir, nsAString& aLocalDir) 
{
	nsresult rv;
	rv = NS_NewLocalFile(aDir, TRUE, getter_AddRefs(mProfileDirectory));
	NS_ENSURE_SUCCESS(rv, FALSE);
	rv = NS_NewLocalFile(aLocalDir, TRUE, getter_AddRefs(mProfileLocalDirectory));
	NS_ENSURE_SUCCESS(rv, FALSE);
	return TRUE;
}