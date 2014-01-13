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
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString version; */
NS_IMETHODIMP KmAppInfo::GetVersion(nsACString & aVersion)
{
	aVersion = "74.0";
    return NS_OK;
}

/* readonly attribute ACString appBuildID; */
NS_IMETHODIMP KmAppInfo::GetAppBuildID(nsACString & aAppBuildID)
{
	aAppBuildID = '7400';
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
	aPlatformBuildID = NS_STRINGIFY(WHATEVER);
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
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean logConsoleErrors; */
NS_IMETHODIMP KmAppInfo::GetLogConsoleErrors(bool *aLogConsoleErrors)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP KmAppInfo::SetLogConsoleErrors(bool aLogConsoleErrors)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String OS; */
NS_IMETHODIMP KmAppInfo::GetOS(nsACString & aOS)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String XPCOMABI; */
NS_IMETHODIMP KmAppInfo::GetXPCOMABI(nsACString & aXPCOMABI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String widgetToolkit; */
NS_IMETHODIMP KmAppInfo::GetWidgetToolkit(nsACString & aWidgetToolkit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long processType; */
NS_IMETHODIMP KmAppInfo::GetProcessType(uint32_t *aProcessType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void invalidateCachesOnRestart (); */
NS_IMETHODIMP KmAppInfo::InvalidateCachesOnRestart()
{
    return NS_ERROR_NOT_IMPLEMENTED;
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