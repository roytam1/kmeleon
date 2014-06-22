/*
*  Copyright (C) 2013 Dorian Boissonnade
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

#include "stdafx.h"
#include "KmAppInfo.h"
#include "KMeleonConst.h"

NS_IMPL_ISUPPORTS2(KmAppInfo, nsIXULAppInfo, nsIXULRuntime)

/* readonly attribute ACString vendor; */
NS_IMETHODIMP KmAppInfo::GetVendor(nsACString & aVendor)
{
	aVendor = "";
    return NS_OK;
}

/* readonly attribute ACString name; */
NS_IMETHODIMP KmAppInfo::GetName(nsACString & aName)
{
	aName = "K-Meleon";
    return NS_OK;
}

/* readonly attribute ACString ID; */
NS_IMETHODIMP KmAppInfo::GetID(nsACString & aID)
{
	aID = "kmeleon@";
    return NS_OK;
}

/* readonly attribute ACString version; */
NS_IMETHODIMP KmAppInfo::GetVersion(nsACString & aVersion)
{
	aVersion = NS_STRINGIFY(KMELEON_UVERSION);
    return NS_OK;
}

/* readonly attribute ACString appBuildID; */
NS_IMETHODIMP KmAppInfo::GetAppBuildID(nsACString & aAppBuildID)
{
	aAppBuildID = NS_STRINGIFY(KMELEON_BUILDID);
    return NS_OK;
}

/* readonly attribute ACString platformVersion; */
NS_IMETHODIMP KmAppInfo::GetPlatformVersion(nsACString & aPlatformVersion)
{
	aPlatformVersion = NS_STRINGIFY(MOZILLA_VERSION);
    return NS_OK;
}

/* readonly attribute ACString platformBuildID; */
NS_IMETHODIMP KmAppInfo::GetPlatformBuildID(nsACString & aPlatformBuildID)
{
	aPlatformBuildID = NS_STRINGIFY(MOZILLA_BUILDID);
    return NS_OK;
}

/* readonly attribute ACString UAName; */
NS_IMETHODIMP KmAppInfo::GetUAName(nsACString & aUAName)
{
	aUAName = "K-Meleon";
    return NS_OK;
}


// CRAP 

/* readonly attribute boolean inSafeMode; */
NS_IMETHODIMP KmAppInfo::GetInSafeMode(bool *aInSafeMode)
{
	*aInSafeMode = false;
    return NS_OK;
}

/* attribute boolean logConsoleErrors; */
NS_IMETHODIMP KmAppInfo::GetLogConsoleErrors(bool *aLogConsoleErrors)
{
	*aLogConsoleErrors = true;
    return NS_OK;
}
NS_IMETHODIMP KmAppInfo::SetLogConsoleErrors(bool aLogConsoleErrors)
{
    return NS_OK;
}

/* readonly attribute AUTF8String OS; */
NS_IMETHODIMP KmAppInfo::GetOS(nsACString & aOS)
{
	aOS = NS_STRINGIFY("WINNT");
    return NS_OK;
}

/* readonly attribute AUTF8String XPCOMABI; */
NS_IMETHODIMP KmAppInfo::GetXPCOMABI(nsACString & aXPCOMABI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String widgetToolkit; */
NS_IMETHODIMP KmAppInfo::GetWidgetToolkit(nsACString & aWidgetToolkit)
{
	aWidgetToolkit = NS_STRINGIFY("windows");
    return NS_OK;
}

/* readonly attribute unsigned long processType; */
NS_IMETHODIMP KmAppInfo::GetProcessType(uint32_t *aProcessType)
{
	NS_ENSURE_ARG_POINTER(aProcessType);
	*aProcessType = XRE_GetProcessType();
	return NS_OK;
}

#include "nsINIParser.h"
#define NS_LINEBREAK "\015\012"
#define FILE_COMPATIBILITY_INFO NS_LITERAL_CSTRING("compatibility.ini")
/* void invalidateCachesOnRestart (); */
NS_IMETHODIMP KmAppInfo::InvalidateCachesOnRestart()
{
/*
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DIR_STARTUP, 
                                       getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;
  if (!file)
    return NS_ERROR_NOT_AVAILABLE;
  
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsINIParser parser;
  rv = parser.Init(file);
  if (NS_FAILED(rv)) {
    // This fails if compatibility.ini is not there, so we'll
    // flush the caches on the next restart anyways.
    return NS_OK;
  }
  
  nsAutoCString buf;
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  
  if (NS_FAILED(rv)) {
    PRFileDesc *fd = nullptr;
    file->OpenNSPRFileDesc(PR_RDWR | PR_APPEND, 0600, &fd);
    if (!fd) {
      NS_ERROR("could not create output stream");
      return NS_ERROR_NOT_AVAILABLE;
    }
    static const char kInvalidationHeader[] = NS_LINEBREAK "InvalidateCaches=1" NS_LINEBREAK;
    PR_Write(fd, kInvalidationHeader, sizeof(kInvalidationHeader) - 1);
    PR_Close(fd);
  }*/
  return NS_OK;
}

/* void ensureContentProcess (); */
NS_IMETHODIMP KmAppInfo::EnsureContentProcess()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute PRTime replacedLockTime; */
NS_IMETHODIMP KmAppInfo::GetReplacedLockTime(PRTime *aReplacedLockTime)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute DOMString lastRunCrashID; */
NS_IMETHODIMP KmAppInfo::GetLastRunCrashID(nsAString & aLastRunCrashID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}