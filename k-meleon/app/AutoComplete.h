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



typedef void (CALLBACK* AutoCompleteCallback)(ACResult*, void*) ;

class CACListener :  public nsIAutoCompleteObserver
{	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIAUTOCOMPLETEOBSERVER	

	CACListener::CACListener(AutoCompleteCallback callback, void* data) {
		m_Callback = callback;
		m_data = data;
	}

	CACListener::~CACListener() {
	}

	static void AutoCompleteStop();
	static void AutoComplete(const CString& aSearchString, AutoCompleteCallback callback, void* data);
protected:

	PRBool AddEltToList(nsISupports* aElement);
	nsCOMPtr<nsIAutoCompleteResult> m_oldResult;
	AutoCompleteCallback m_Callback;
	void* m_data;
	static nsString previousSearch;
};

NS_IMPL_ISUPPORTS(CACListener, nsIAutoCompleteObserver)

NS_IMETHODIMP CACListener::OnUpdateSearchResult(nsIAutoCompleteSearch *search, nsIAutoCompleteResult *result)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CACListener::OnSearchResult(nsIAutoCompleteSearch *search, nsIAutoCompleteResult *result)
{
	uint16_t status;
	result->GetSearchResult(&status);
	int32_t defaultIndex;
	result->GetDefaultIndex(&defaultIndex);
	if (status == nsIAutoCompleteResult::RESULT_SUCCESS || status == nsIAutoCompleteResult::RESULT_NOMATCH)
	{
		m_oldResult = result; // Keep the old result for optimization
		uint32_t count;
		result->GetMatchCount(&count);				
		ACResult* acresult = count ? new ACResult(count) : nullptr;
		for (PRUint32 i = 0; i<count; i++)
		{
			nsString nsStr, nsCStr;
			result->GetValueAt(i, nsStr);
			result->GetCommentAt(i, nsCStr);
			acresult->AddTail(ACResultItem(NSStringToCString(nsStr), NSStringToCString(nsCStr)));			
		}
		(*m_Callback)(acresult, m_data);
	}
	else
		m_oldResult = nullptr;

	
	return NS_OK;
}

nsString CACListener::previousSearch;

void CACListener::AutoCompleteStop()
{
	nsresult rv;
	nsCOMPtr<nsIAutoCompleteSearch> autoComplete = do_GetService("@mozilla.org/autocomplete/search;1?name=history", &rv);
	NS_ENSURE_TRUE(autoComplete, );
	autoComplete->StopSearch();
	previousSearch = L"";
}

void CACListener::AutoComplete(const CString& aSearchString, AutoCompleteCallback callback, void* data)
{	
	nsresult rv;

	nsCOMPtr<nsIAutoCompleteSearch> autoComplete = do_GetService("@mozilla.org/autocomplete/search;1?name=history", &rv);
	NS_ENSURE_TRUE(autoComplete, );

	nsString searchString = CStringToNSString(aSearchString);
	if (!searchString.Equals(previousSearch)) {
		previousSearch = searchString;
		nsCOMPtr<nsIAutoCompleteObserver> listener = new CACListener(callback, data);
		autoComplete->StartSearch(searchString, EmptyString(), nullptr/*m_oldResult*/, listener);
	}
}



