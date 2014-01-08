/*
*  Copyright (C) 2014
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
#include "KmAbout.h"
#include "nsIIOService.h"
#include "nsIScriptSecurityManager.h"
#include "nsIChannel.h"

struct RedirEntry {
	const char* id;
	const char* url;
	uint32_t flags;
};

static RedirEntry kRedirMap[] = {
	{ 
		"home", "chrome://kmeleon/content/home.xhtml",
		nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
		nsIAboutModule::ALLOW_SCRIPT 
	}
};

static const int kRedirTotal = NS_ARRAY_LENGTH(kRedirMap);
NS_IMPL_ISUPPORTS1(KmAbout, nsIAboutModule)
	
static nsAutoCString
	GetAboutModuleName(nsIURI *aURI)
{
	nsAutoCString path;
	aURI->GetPath(path);

	int32_t f = path.FindChar('#');
	if (f >= 0)
		path.SetLength(f);

	f = path.FindChar('?');
	if (f >= 0)
		path.SetLength(f);

	ToLowerCase(path);
	return path;
}

NS_IMETHODIMP
	KmAbout::NewChannel(nsIURI *aURI, nsIChannel **result) 
{
	NS_ENSURE_ARG_POINTER(aURI);
	NS_ASSERTION(result, "must not be null");

	nsAutoCString path = GetAboutModuleName(aURI);

	nsresult rv;
	nsCOMPtr<nsIIOService> ioService = do_GetService("@mozilla.org/network/io-service;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	for (int i = 0; i < kRedirTotal; i++) {
		if (!strcmp(path.get(), kRedirMap[i].id)) {
			nsCOMPtr<nsIChannel> tempChannel;
			rv = ioService->NewChannel(nsCString(kRedirMap[i].url),
				nullptr, nullptr, getter_AddRefs(tempChannel));
			NS_ENSURE_SUCCESS(rv, rv);

			tempChannel->SetOriginalURI(aURI);

			NS_ADDREF(*result = tempChannel);
			return rv;
		}
	}

	return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
	KmAbout::GetURIFlags(nsIURI *aURI, uint32_t *result)
{
	NS_ENSURE_ARG_POINTER(aURI);

	nsAutoCString name = GetAboutModuleName(aURI);

	for (int i = 0; i < kRedirTotal; i++) {
		if (name.Equals(kRedirMap[i].id)) {
			*result = kRedirMap[i].flags;
			return NS_OK;
		}
	}

	return NS_ERROR_ILLEGAL_VALUE;
}
