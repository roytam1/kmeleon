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
#include "winEmbedFileLocProvider.h"

//#ifdef USE_FILELOCPROVIDER 

#ifndef XP_WIN
#define XP_WIN
#endif

#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsIProperties.h"
#include "nsServiceManagerUtils.h"

#include <windows.h>
#include <shlobj.h>

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;


// WARNING: These hard coded names need to go away. They need to
// come from localizable resources
#define APP_REGISTRY_NAME nsEmbedCString("registry.dat")
#define PROFILE_INI_NAME  nsEmbedCString("profile.ini")

#define PROFILE_ROOT_DIR_NAME       nsEmbedCString("Profiles")
#define DEFAULTS_DIR_NAME           nsEmbedCString("defaults")
#define DEFAULTS_PREF_DIR_NAME      nsEmbedCString("pref")
#define DEFAULTS_PROFILE_DIR_NAME   nsEmbedCString("profile")
#define RES_DIR_NAME                nsEmbedCString("res")
#define CHROME_DIR_NAME             nsEmbedCString("chrome")
#define PLUGINS_DIR_NAME            nsEmbedCString("plugins")
#define SEARCH_DIR_NAME             nsEmbedCString("searchplugins")
#define COMPONENTS_DIR_NAME         nsEmbedCString("components")


//*****************************************************************************
// winEmbedFileLocProvider::Constructor/Destructor
//*****************************************************************************   

winEmbedFileLocProvider::winEmbedFileLocProvider(const nsACString& aAppDataDirName)
{
    mProductDirName = aAppDataDirName;
}

winEmbedFileLocProvider::~winEmbedFileLocProvider()
{
}


//*****************************************************************************
// winEmbedFileLocProvider::nsISupports
//*****************************************************************************   

NS_IMPL_ISUPPORTS1(winEmbedFileLocProvider, nsIDirectoryServiceProvider)

//*****************************************************************************
// winEmbedFileLocProvider::nsIDirectoryServiceProvider
//*****************************************************************************   

NS_IMETHODIMP
winEmbedFileLocProvider::GetFile(const char *prop, PRBool *persistant, nsIFile **_retval)
{    
    nsCOMPtr<nsILocalFile>  localFile;
    nsresult rv = NS_ERROR_FAILURE;

    *_retval = nsnull;
    *persistant = PR_TRUE;
    
    if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
    {
		 rv = GetProductDirectory(getter_AddRefs(localFile));
    }
    else if (strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
    {
		rv = GetProductDirectory(getter_AddRefs(localFile));
	    if (NS_SUCCEEDED(rv))
			rv = localFile->AppendNative(APP_REGISTRY_NAME);
    }
  /*  else if (strcmp(prop, NS_APP_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_PREF_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PREF_DIR_NAME);
        }
    }
    else if (strcmp(prop, NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR) == 0 ||
             strcmp(prop, NS_APP_PROFILE_DEFAULTS_50_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv)) {
            rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
            if (NS_SUCCEEDED(rv))
                rv = localFile->AppendRelativeNativePath(DEFAULTS_PROFILE_DIR_NAME);
        }
    }*/
    else if (strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
    {
        rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile));   
    }
	else if (strcmp(prop, NS_APP_USER_PROFILES_LOCAL_ROOT_DIR) == 0)
	{
		rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile), PR_TRUE);   
	}
	/*
    else if (strcmp(prop, NS_APP_RES_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(RES_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_CHROME_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(CHROME_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_PLUGINS_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(PLUGINS_DIR_NAME);
    }
    else if (strcmp(prop, NS_APP_SEARCH_DIR) == 0)
    {
        rv = CloneMozBinDirectory(getter_AddRefs(localFile));
        if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(SEARCH_DIR_NAME);
    }*/
    else if (strcmp(prop, K_APP_SKINS_DIR) == 0) {
       CString folder = theApp.GetFolder(SkinsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(LPCTSTR(folder)), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(LPCTSTR(folder)), TRUE, getter_AddRefs(localFile));
#endif
    }
    else if (strcmp(prop, K_APP_KPLUGINS_DIR) == 0) {
       CString folder = theApp.GetFolder(PluginsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(folder), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(folder), TRUE, getter_AddRefs(localFile));
#endif
    }
    else if (strcmp(prop, K_USER_SKINS_DIR) == 0) {
       CString folder = theApp.GetFolder(UserSkinsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(folder), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(folder), TRUE, getter_AddRefs(localFile));
#endif
    }
    else if (strcmp(prop, K_USER_KPLUGINS_DIR) == 0) {
       CString folder = theApp.GetFolder(UserPluginsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(folder), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(folder), TRUE, getter_AddRefs(localFile));
#endif
    }
    else if (strcmp(prop, K_APP_SETTING_DEFAULTS) == 0) {
       CString folder = theApp.GetFolder(DefSettingsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(folder), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(folder), TRUE, getter_AddRefs(localFile));
#endif
    }
    else if (strcmp(prop, K_USER_SETTING) == 0) {
       CString folder = theApp.GetFolder(UserSettingsFolder);
#ifdef _UNICODE
       rv = NS_NewLocalFile(nsEmbedString(folder), TRUE, getter_AddRefs(localFile));
#else
       rv = NS_NewNativeLocalFile(nsEmbedCString(folder), TRUE, getter_AddRefs(localFile));
#endif
    }
    
#ifdef XPCOM_GLUE
    //---------------------------------------------------------------
    // Note that by returning a valid localFile's for NS_GRE_DIR and
    // NS_GRE_COMPONENT_DIR your app is indicating to XPCOM that 
    // it found an GRE version with which it's compatible with and 
    // it intends to be "run against" that GRE
    //
    // Please see http://www.mozilla.org/projects/embedding/GRE.html
    // for more info. on GRE
    //---------------------------------------------------------------
    else if (strcmp(prop, NS_GRE_DIR) == 0)
    {
        rv = GRE_GetGREDirectory(getter_AddRefs(localFile));
    }
#endif

    if (localFile && NS_SUCCEEDED(rv))
        return localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)_retval);
        
    return rv;
}

NS_METHOD winEmbedFileLocProvider::CloneMozBinDirectory(nsILocalFile **aLocalFile)
{
    NS_ENSURE_ARG_POINTER(aLocalFile);
    nsresult rv;
    
    if (!mMozBinDirectory)
    {        
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
        }
    }
    
    nsCOMPtr<nsIFile> aFile;
    rv = mMozBinDirectory->Clone(getter_AddRefs(aFile));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsILocalFile> lfile = do_QueryInterface (aFile);
    if (!lfile)
        return NS_ERROR_FAILURE;
    
    NS_IF_ADDREF(*aLocalFile = lfile);
    return NS_OK;
}


//----------------------------------------------------------------------------------------
// GetProductDirectory - Gets the directory which contains the application data folder
//
// WIN    : <Application Data folder on user's machine>\Mozilla 
//----------------------------------------------------------------------------------------
NS_METHOD winEmbedFileLocProvider::GetProductDirectory(nsILocalFile **aLocalFile, PRBool aLocal)
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
	//rv = CloneMozBinDirectory(aLocalFile);
	//return rv;
    
    PRBool exists;
    nsCOMPtr<nsILocalFile> localDir;
	 
	nsCOMPtr<nsILocalFile> appDir;
	rv = CloneMozBinDirectory(getter_AddRefs(appDir));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIFile> aFile;
	appDir->Clone(getter_AddRefs(aFile));
	nsCOMPtr<nsILocalFile> profileIni = do_QueryInterface(aFile, &rv);
	NS_ENSURE_SUCCESS(rv,rv);

	rv = profileIni->AppendNative(PROFILE_INI_NAME);
	NS_ENSURE_SUCCESS(rv, rv);

	profileIni->Exists(&exists);
	if (exists)
	{
#ifdef _UNICODE
		nsEmbedString path;
		profileIni->GetPath(path);
#else
		nsEmbedCString path;
		profileIni->GetNativePath(path);
#endif
		TCHAR pszBuffer[4096] = {0};
		
		// Ugly stuff
		GetPrivateProfileString(_T("Profile"),_T("path"), _T(""), pszBuffer, 4096, path.get());
		UINT IsRelative = GetPrivateProfileInt(_T("Profile"),_T("isrelative"), 1, path.get());

#ifdef UNICODE	
		nsEmbedString buffer(pszBuffer);
#else
		nsEmbedCString buffer(pszBuffer);
#endif
		nsCOMPtr<nsILocalFile> profileDir;
		if (!buffer.IsEmpty())
		{
			if (!IsRelative)
				#ifdef UNICODE
					rv = NS_NewLocalFile(buffer, TRUE, getter_AddRefs(profileDir));
				#else
					rv = NS_NewNativeLocalFile(buffer, TRUE, getter_AddRefs(profileDir));
				#endif
			else
			{
				#ifdef UNICODE
					nsString profPath;
					appDir->GetPath(profPath);
					profPath.Append(L"\\");
					profPath.Append(buffer);
					rv = NS_NewLocalFile(profPath, TRUE, getter_AddRefs(profileDir));
				#else
					nsCString profPath;
					appDir->GetNativePath(profPath);
					profPath.Append("\\");
					profPath.Append(buffer);
					rv = NS_NewNativeLocalFile(profPath, TRUE, getter_AddRefs(profileDir));
				#endif
				NS_ENSURE_SUCCESS(rv, rv);
				rv = profileDir->Normalize();
			}
		}
		else
		{
			rv = CloneMozBinDirectory(getter_AddRefs(profileDir));
			NS_ENSURE_SUCCESS(rv, rv);
	        
			rv = profileDir->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
		}

		NS_ENSURE_SUCCESS(rv, rv);
		rv = profileDir->Exists(&exists);
		if (NS_SUCCEEDED(rv) && !exists)
			rv = profileDir->Create(nsIFile::DIRECTORY_TYPE, 0775);

		*aLocalFile = profileDir;
		NS_ADDREF(*aLocalFile);

		return rv; 
	}
	else
	{
		nsCOMPtr<nsIProperties> directoryService = 
			do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
		if (NS_FAILED(rv)) return rv;
		const char* prop = aLocal ? NS_WIN_LOCAL_APPDATA_DIR : NS_WIN_APPDATA_DIR;
		rv = directoryService->Get(prop, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
		if (NS_SUCCEEDED(rv))
			rv = localDir->Exists(&exists);
		
		if (NS_FAILED(rv) || !exists)
		{
			// On some Win95 machines, NS_WIN_APPDATA_DIR does not exist - revert to NS_WIN_WINDOWS_DIR
			localDir = nsnull;
			//rv = directoryService->Get(NS_WIN_WINDOWS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
			rv = CloneMozBinDirectory(getter_AddRefs(localDir));
			NS_ENSURE_SUCCESS(rv, rv);

			rv = localDir->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
		}
		else
			rv = localDir->AppendNative(mProductDirName);

		
		if (NS_FAILED(rv)) return rv;
		rv = localDir->Exists(&exists);
		if (NS_SUCCEEDED(rv) && !exists)
			rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
		if (NS_FAILED(rv)) return rv;

		*aLocalFile = localDir;
		NS_ADDREF(*aLocalFile);
	}

	return rv; 
}

//----------------------------------------------------------------------------------------
// GetDefaultUserProfileRoot - Gets the directory which contains each user profile dir
//
// WIN    : <Application Data folder on user's machine>\Mozilla\Users50 
//----------------------------------------------------------------------------------------
NS_METHOD winEmbedFileLocProvider::GetDefaultUserProfileRoot(nsILocalFile **aLocalFile, PRBool aLocal)
{
	NS_ENSURE_ARG_POINTER(aLocalFile);

	nsresult rv;
	PRBool exists;
	nsCOMPtr<nsILocalFile> localDir;

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

