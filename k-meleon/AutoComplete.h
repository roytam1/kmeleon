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
*/

// Temporary (maybe not) fix for autocompletion

#pragma once

#include "stdafx.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
#include "kmeleon_plugin.h"
#include "MozUtils.h"

nsCOMPtr<nsIAutoCompleteResult> m_oldResult;
AutoCompleteResult* gACResults = NULL;
unsigned int gACCountResults = 0;

typedef void (CALLBACK* AutoCompleteCallback)(_AutoCompleteResult*, int, void*) ;

class CACListener :  public nsIAutoCompleteObserver
{	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIAUTOCOMPLETEOBSERVER	

	CACListener::CACListener(AutoCompleteCallback callback, void* data) {
		m_ACIt = NULL;
		m_Callback = callback;
		m_data = data;
	}

protected:
	PRBool AddEltToList(nsISupports* aElement);
	AutoCompleteResult* m_ACIt;
	AutoCompleteCallback m_Callback;
	void* m_data;
};

NS_IMPL_ISUPPORTS1(CACListener, nsIAutoCompleteObserver)

NS_IMETHODIMP CACListener::OnUpdateSearchResult(nsIAutoCompleteSearch *search, nsIAutoCompleteResult *result)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CACListener::OnSearchResult(nsIAutoCompleteSearch *search, nsIAutoCompleteResult *result)
{
	if (gACCountResults > 0) {
		m_ACIt = gACResults;
		while (gACCountResults--){
			free(m_ACIt->value);
			free(m_ACIt->comment);
			m_ACIt++;
		}
		free(gACResults);
		gACResults = NULL;
		gACCountResults = 0;
	}

	PRUint16 status;
	result->GetSearchResult(&status);
	if (status == nsIAutoCompleteResult::RESULT_SUCCESS || status == nsIAutoCompleteResult::RESULT_NOMATCH)
	{
		m_oldResult = result; // Keep the old result for optimization

		result->GetMatchCount(&gACCountResults);
		gACResults = (AutoCompleteResult*)calloc(gACCountResults, sizeof(AutoCompleteResult));
		m_ACIt = gACResults;

		for (PRUint32 i = 0; i<gACCountResults; i++)
		{
			nsEmbedString nsStr;
			nsEmbedCString nsCStr;

			result->GetValueAt(i, nsStr);
			NS_UTF16ToCString(nsStr,NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,nsCStr);
			m_ACIt->value = strdup(nsCStr.get());

			result->GetCommentAt(i, nsStr);
			NS_UTF16ToCString(nsStr,NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,nsCStr);
			m_ACIt->comment = strdup(nsCStr.get());
	
			m_ACIt->score = 0;
			m_ACIt++;
		}
		(*m_Callback)(gACResults, gACCountResults, m_data);
	}
	else
		m_oldResult = nullptr;

	
	return NS_OK;
}

int AutoComplete(const CString& aSearchString, AutoCompleteCallback callback, void* data)
{
	static nsEmbedString previousSearch;
	nsresult rv;

	nsCOMPtr<nsIAutoCompleteSearch> autoComplete = do_GetService("@mozilla.org/autocomplete/search;1?name=history", &rv);
	if (NS_FAILED(rv)) return 0;

	nsEmbedString searchString = CStringToNSString(aSearchString);
	if (!searchString.Equals(previousSearch)) {
		previousSearch = searchString;
		nsCOMPtr<nsIAutoCompleteObserver> listener = new CACListener(callback, data);
		autoComplete->StartSearch(searchString, EmptyString(), m_oldResult, listener);
	}

	return gACCountResults;
}



