/*
*  Copyright (C) 2001 Jeff Doozan
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

// The current ContentListener implementation only calls OnStartURIOpen and IsPreferred

#include "stdafx.h"

#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"


NS_IMETHODIMP CBrowserImpl::OnStartURIOpen(nsIURI* aURI, const char* aWindowTarget, PRBool* aAbortOpen)
{
   nsresult rv;
   
   nsXPIDLCString specString;
   rv = aURI->GetSpec(getter_Copies(specString));
   if (NS_FAILED(rv))
      return rv;

   *aAbortOpen = PR_FALSE;

   return NS_OK;
}

NS_IMETHODIMP 
CBrowserImpl::GetProtocolHandler(nsIURI * aURI, nsIProtocolHandler **aProtocolHandler)
{

   /*
   NS_ENSURE_ARG_POINTER(aProtocolHandler);
   *aProtocolHandler = nsnull;
   return NS_OK;
   */

   return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
CBrowserImpl::DoContent(const char *aContentType, nsURILoadCommand aCommand, const char *aWindowTarget, 
                             nsIRequest *request, nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
   /*
   NS_ENSURE_ARG_POINTER(aContentHandler);
   NS_ENSURE_ARG_POINTER(aAbortProcess);
   *aContentHandler = nsnull;
   *aAbortProcess = PR_FALSE;
   return NS_OK;
   */

   return  NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP 
CBrowserImpl::IsPreferred(const char * aContentType,
                               nsURILoadCommand aCommand,
                               const char * aWindowTarget,
                               char ** aDesiredContentType,
                               PRBool * aCanHandleContent)
{

   NS_ENSURE_ARG_POINTER(aDesiredContentType);
   NS_ENSURE_ARG_POINTER(aCanHandleContent);
   *aDesiredContentType = nsnull;

//   nsCOMPtr<nsIURI> currentURI;
//	rv = mWebBrowser->GetCurrentURI(getter_AddRefs(currentURI));
   
   // claim that we can handle everything
   *aCanHandleContent = PR_TRUE;
   return NS_OK;
}

NS_IMETHODIMP 
CBrowserImpl::CanHandleContent(const char * aContentType,
                                    nsURILoadCommand aCommand,
                                    const char * aWindowTarget,
                                    char ** aDesiredContentType,
                                    PRBool * aCanHandleContent)

{
   /*
   NS_ENSURE_ARG_POINTER(aDesiredContentType);
   NS_ENSURE_ARG_POINTER(aCanHandleContent);
   *aDesiredContentType = nsnull;
   *aCanHandleContent = PR_FALSE;

   return NS_OK;
   */

   return  NS_ERROR_NOT_IMPLEMENTED;

}

NS_IMETHODIMP 
CBrowserImpl::GetParentContentListener(nsIURIContentListener** aParentContentListener)
{
   /*
   NS_ENSURE_ARG_POINTER(aParentContentListener);
   *aParentContentListener = nsnull;

   return NS_OK;
   */
   return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
CBrowserImpl::SetParentContentListener(nsIURIContentListener* aParentContentListener)
{
   /*
   return NS_OK;
   */
   return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
CBrowserImpl::GetLoadCookie(nsISupports ** aLoadCookie)
{
   /*
   NS_ENSURE_ARG_POINTER(aLoadCookie);
   *aLoadCookie = nsnull;
   return NS_OK;
   */
   return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
CBrowserImpl::SetLoadCookie(nsISupports * aLoadCookie)
{
   /*
   return NS_OK;
   */
   return  NS_ERROR_NOT_IMPLEMENTED;
}
