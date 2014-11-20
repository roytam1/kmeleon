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
 *   Chak Nanga <chak@netscape.com> 
 *
 * ***** END LICENSE BLOCK ***** */


#include "stdafx.h"


#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"

#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLInputElement.h"


//*****************************************************************************
// CBrowserImpl::nsIContextMenuListener
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::OnShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo)
{
   NS_ENSURE_TRUE(m_pBrowserFrameGlue, NS_OK);
   
   // No context menu for chrome
   if (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
        return NS_OK;

   /*
   SContextData ctx;
    
   nsCOMPtr<nsIDOMNode> node;
   nsresult rv = aInfo->GetTargetNode(getter_AddRefs(node));
   NS_ENSURE_SUCCESS(rv, rv);

   ctx.node = node;

   */

	nsCOMPtr<nsIDOMNode> node;
	nsresult rv = aInfo->GetTargetNode(getter_AddRefs(node));
	NS_ENSURE_SUCCESS(rv, rv);

	if (aContextFlags & nsIContextMenuListener2::CONTEXT_INPUT) 
	{
		if (!(aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)) {
			// Mozilla don't tell if the input is of type text or password...
			nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(node));
			if (inputElement) {
				nsString inputElemType;
				inputElement->GetType(inputElemType);
				if ((wcsicmp(inputElemType.get(), L"text") == 0) ||
					(wcsicmp(inputElemType.get(), L"password") == 0))
					aContextFlags |= nsIContextMenuListener2::CONTEXT_TEXT;
			}
		}
	}
	m_pBrowserFrameGlue->ShowContextMenu(aContextFlags, node);

	return NS_OK;
}
