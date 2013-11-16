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

NS_IMPL_ISUPPORTS1(KmAppInfo, nsIXULAppInfo)

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
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString platformVersion; */
NS_IMETHODIMP KmAppInfo::GetPlatformVersion(nsACString & aPlatformVersion)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString platformBuildID; */
NS_IMETHODIMP KmAppInfo::GetPlatformBuildID(nsACString & aPlatformBuildID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString UAName; */
NS_IMETHODIMP KmAppInfo::GetUAName(nsACString & aUAName)
{
	aUAName = "K-Meleon";
    return NS_OK;
}