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
 *
 * ***** END LICENSE BLOCK ***** */

#include "StdAfx.h"
#include "winEmbedFileLocProvider.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;


// WARNING: These hard coded names need to go away. They need to
// come from localizable resources
#define APP_REGISTRY_NAME           NS_LITERAL_CSTRING("profiles.dat")

#define PROFILE_ROOT_DIR_NAME       NS_LITERAL_CSTRING("Profiles")
#define DEFAULTS_DIR_NAME           NS_LITERAL_CSTRING("defaults")
#define DEFAULTS_PREF_DIR_NAME      NS_LITERAL_CSTRING("pref")
#define DEFAULTS_PROFILE_DIR_NAME   NS_LITERAL_CSTRING("profile")
#define RES_DIR_NAME                NS_LITERAL_CSTRING("res")
#define CHROME_DIR_NAME             NS_LITERAL_CSTRING("chrome")
#define PLUGINS_DIR_NAME            NS_LITERAL_CSTRING("plugins")
#define SEARCH_DIR_NAME             NS_LITERAL_CSTRING("searchplugins")


//*****************************************************************************
// winEmbedFileLocProvider::Constructor/Destructor
//*****************************************************************************   

winEmbedFileLocProvider::winEmbedFileLocProvider()
{
   NS_INIT_ISUPPORTS();
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
   
   if (nsCRT::strcmp(prop, NS_APP_APPLICATION_REGISTRY_DIR) == 0)
   {
      if (theApp.cmdline.m_sProfilesDir) {
         nsCOMPtr<nsILocalFile> tempPath(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
         rv = tempPath->InitWithNativePath(nsDependentCString(theApp.cmdline.m_sProfilesDir));
         if (NS_FAILED(rv)) return rv;
         getter_AddRefs(localFile = tempPath);
         if (NS_FAILED(rv)) return rv;
      }   
      else {
         rv = CloneMozBinDirectory(getter_AddRefs(localFile));
         if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
      }
   }
   else if (nsCRT::strcmp(prop, NS_APP_APPLICATION_REGISTRY_FILE) == 0)
   {
      if (theApp.cmdline.m_sProfilesDir) {
         nsCOMPtr<nsILocalFile> tempPath(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
         rv = tempPath->InitWithNativePath(nsDependentCString(theApp.cmdline.m_sProfilesDir));
         if (NS_FAILED(rv)) return rv;
         getter_AddRefs(localFile = tempPath);
         if (NS_FAILED(rv)) return rv;
      }   
      else {
         rv = CloneMozBinDirectory(getter_AddRefs(localFile));
         if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
      }
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendNative(APP_REGISTRY_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_DEFAULTS_50_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_PREF_DEFAULTS_50_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv)) {
         rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
         if (NS_SUCCEEDED(rv))
            rv = localFile->AppendRelativeNativePath(DEFAULTS_PREF_DIR_NAME);
      }
   }
   else if (nsCRT::strcmp(prop, NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR) == 0 ||
      nsCRT::strcmp(prop, NS_APP_PROFILE_DEFAULTS_50_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(DEFAULTS_DIR_NAME);
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(DEFAULTS_PROFILE_DIR_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_USER_PROFILES_ROOT_DIR) == 0)
   {
      rv = GetDefaultUserProfileRoot(getter_AddRefs(localFile));   
   }
   else if (nsCRT::strcmp(prop, NS_APP_RES_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(RES_DIR_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_CHROME_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(CHROME_DIR_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_PLUGINS_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(PLUGINS_DIR_NAME);
   }
   else if (nsCRT::strcmp(prop, NS_APP_SEARCH_DIR) == 0)
   {
      rv = CloneMozBinDirectory(getter_AddRefs(localFile));
      if (NS_SUCCEEDED(rv))
         rv = localFile->AppendRelativeNativePath(SEARCH_DIR_NAME);
   }

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
NS_METHOD winEmbedFileLocProvider::GetProductDirectory(nsILocalFile **aLocalFile)
{
   NS_ENSURE_ARG_POINTER(aLocalFile);    
   nsresult rv;
   
/*
   // nt based systems should keep the profile data inside the appdata dir
   OSVERSIONINFO info;
   info.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   if (GetVersionEx(&info) && (info.dwPlatformId == VER_PLATFORM_WIN32_NT)) {
      PRBool exists;
      nsCOMPtr<nsILocalFile> localDir;
      
      nsCOMPtr<nsIProperties> directoryService = 
         do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
      
      if (NS_FAILED(rv)) return rv;
      rv = directoryService->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
      if (NS_SUCCEEDED(rv))
         rv = localDir->Exists(&exists);
      if (NS_FAILED(rv) || !exists)
      {
         // On some Win95 machines, NS_WIN_APPDATA_DIR does not exist - revert to NS_WIN_WINDOWS_DIR
         localDir = nsnull;
         rv = directoryService->Get(NS_WIN_WINDOWS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
      }
      if (NS_FAILED(rv)) return rv;
      
      rv = localDir->AppendRelativeNativePath("K-Meleon");
      if (NS_FAILED(rv)) return rv;
      rv = localDir->Exists(&exists);
      if (NS_SUCCEEDED(rv) && !exists)
         rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
      if (NS_FAILED(rv)) return rv;
      
      *aLocalFile = localDir;
      NS_ADDREF(*aLocalFile);      
   }
   // non-nt based systems should use the kmeleon directory
   else {
*/
      rv = CloneMozBinDirectory(aLocalFile);
//   }
   
   return rv;   
}


//----------------------------------------------------------------------------------------
// GetDefaultUserProfileRoot - Gets the directory which contains each user profile dir
//
// WIN    : <Application Data folder on user's machine>\Mozilla\Users50 
//----------------------------------------------------------------------------------------
NS_METHOD winEmbedFileLocProvider::GetDefaultUserProfileRoot(nsILocalFile **aLocalFile)
{
   NS_ENSURE_ARG_POINTER(aLocalFile);
   
   nsresult rv;
   PRBool exists;
   nsCOMPtr<nsILocalFile> localDir;
   
   if (theApp.cmdline.m_sProfilesDir) {
      nsCOMPtr<nsILocalFile> tempPath(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
      rv = tempPath->InitWithNativePath(nsDependentCString(theApp.cmdline.m_sProfilesDir));
      if (NS_FAILED(rv)) return rv;
      getter_AddRefs(localDir = tempPath);
      if (NS_FAILED(rv)) return rv;
   }   
   else {      
      rv = CloneMozBinDirectory(getter_AddRefs(localDir));
      if (NS_FAILED(rv)) return rv;
      
      // These 3 platforms share this part of the path - do them as one
      rv = localDir->AppendRelativeNativePath(PROFILE_ROOT_DIR_NAME);
      if (NS_FAILED(rv)) return rv;
   }
   
   rv = localDir->Exists(&exists);
   if (NS_SUCCEEDED(rv) && !exists)
      rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
   if (NS_FAILED(rv)) return rv;      
   
   *aLocalFile = localDir;
   NS_ADDREF(*aLocalFile);
   
   return rv; 
}
