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

#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsEmbedString.h"
#include "nsIBrowserHistory.h"
#include "nsServiceManagerUtils.h"
#if GECKO_VERSION > 18 
#include "nsToolkitCompsCID.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
nsCOMPtr<nsIAutoCompleteResult> m_oldResult;
#else
#include "nsIAutoCompleteSession.h"
#include "nsIAutoCompleteListener.h"
nsCOMPtr<nsIAutoCompleteResults> m_oldResult;
#endif

AutoCompleteResult* gACResults = NULL;
unsigned int gACCountResults = 0;

#if GECKO_VERSION > 18

class CACListener :  public nsIAutoCompleteObserver
{	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIAUTOCOMPLETEOBSERVER

	CACListener::CACListener() {
		m_ACIt = NULL;
	}

protected:
	PRBool AddEltToList(nsISupports* aElement);
	AutoCompleteResult* m_ACIt;
};

NS_IMPL_ISUPPORTS1(CACListener, nsIAutoCompleteObserver)

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
	if (status == nsIAutoCompleteResult::RESULT_SUCCESS)
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
	}
	else
		m_oldResult = NULL;

	return NS_OK;
}

#else

class CACListener :  public nsIAutoCompleteListener
{	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIAUTOCOMPLETELISTENER

	CACListener::CACListener() {
		m_ACIt = NULL;
	}

	protected:
	static PRBool enumStatic(nsISupports* aElement, void *aData){
		return  ((CACListener*)aData)->AddEltToList(aElement);
	}
	static PRBool count(nsISupports* aElement, void *aData){
		gACCountResults++;
		return PR_TRUE;
	}
	
	PRBool AddEltToList(nsISupports* aElement);
	AutoCompleteResult* m_ACIt;
};

NS_IMPL_ISUPPORTS1(CACListener, nsIAutoCompleteListener)

NS_IMETHODIMP CACListener::OnStatus(const PRUnichar *statusText){
	return NS_OK;
}

NS_IMETHODIMP CACListener::OnAutoComplete(nsIAutoCompleteResults *result, AutoCompleteStatus status)
{
	m_oldResult = result; // Keep the old result for optimization

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

	if (status == nsIAutoCompleteStatus::matchFound)
	{
		nsCOMPtr<nsISupportsArray> array;
		result->GetItems(getter_AddRefs(array));
        array->EnumerateForwards( CACListener::count, this); // MOZILLA SUCKS
		gACResults = (AutoCompleteResult*)calloc(gACCountResults, sizeof(AutoCompleteResult));
		m_ACIt = gACResults;
		array->EnumerateForwards( CACListener::enumStatic, this);
	}

	return NS_OK;
}

NS_IMETHODIMP CACListener::GetParam(nsISupports * *aParam){
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CACListener::SetParam(nsISupports * aParam){
	return NS_OK;
}

PRBool CACListener::AddEltToList(nsISupports* aElement)
{
	nsCOMPtr<nsIAutoCompleteItem> acItem = do_QueryInterface(aElement);
	if (!acItem)
		return PR_FALSE;
	
	nsEmbedString nsStr;
	acItem->GetValue(nsStr);
	nsEmbedCString nsCStr;
	NS_UTF16ToCString(nsStr,NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,nsCStr);
	m_ACIt->value = strdup(nsCStr.get());

	PRUnichar* comment;
	acItem->GetComment(&comment);
	NS_UTF16ToCString(nsDependentString(comment),NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,nsCStr);
	m_ACIt->comment = strdup(nsCStr.get());
	nsMemory::Free(comment);

	m_ACIt->score = 0;
	m_ACIt++;
	return PR_TRUE;
}

#endif


int AutoComplete(char* aSearchString, AutoCompleteResult** results)
{
	static nsEmbedString previousSearch;
	nsresult rv;
	
#if GECKO_VERSION > 18
	nsCOMPtr<nsIAutoCompleteSearch> autoComplete = do_GetService(NS_GLOBALHISTORY_AUTOCOMPLETE_CONTRACTID, &rv);
#else	
	nsCOMPtr<nsIAutoCompleteSession> autoComplete = do_GetService(NS_GLOBALHISTORY_AUTOCOMPLETE_CONTRACTID, &rv);
#endif
	if (NS_FAILED(rv)) return 0;

	nsEmbedString searchString;
	NS_CStringToUTF16(nsDependentCString(aSearchString),NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,searchString);

	if (!searchString.Equals(previousSearch)) {
		previousSearch = searchString;
		CACListener listener;

#if GECKO_VERSION > 18		
		// Now there is a bug preventing the usage of the old result.
		// The new search string is not passed to the result *-)
		autoComplete->StartSearch(searchString, EmptyString(), NULL /*m_oldResult*/, &listener);
#else
		// I'm not using oldresult to partially fix a weird behavior
		// of mozilla who always cut prefix of url before searching  
		// (so if you type 'w', there will be no result beginning by "www.") 
		// whatever the search string is. 
		autoComplete->OnStartLookup(searchString.get(), nsnull /*m_oldResult*/, &listener);
#endif
	}

	if (results) *results = gACResults;
	return gACCountResults;
}


