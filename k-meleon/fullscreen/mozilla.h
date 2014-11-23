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
*
*
*/

#define XPCOM_GLUE_AVOID_NSPR
#include "mozilla-config.h"
#include <tchar.h>

#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsEmbedString.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"

#include "../kmeleon_plugin.h"
extern kmeleonPlugin kPlugin;

class CDomEventListener : public nsIDOMEventListener
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	CDomEventListener(HWND hwnd)
	{
		mFullscreenDoc = nullptr;
		mhWnd = hwnd;
	}

	~CDomEventListener()
	{		
	}

	BOOL Init(HWND hwnd)
	{
		
		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;
		
		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return FALSE;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, FALSE);
	
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("mozfullscreenchange"),
							this, false);

		NS_ENSURE_SUCCESS(rv, FALSE);

		
		return TRUE;
	}

	void Done(HWND hwnd)
	{		
		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;

		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, );

		if (mEventTarget)
			mEventTarget->RemoveEventListener(NS_LITERAL_STRING("mozfullscreenchange"),
				  this, false);
	}

	void CancelFullScreen() 
	{
		/*nsCOMPtr<nsIWebBrowser> mWebBrowser;
		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hTab, getter_AddRefs(mWebBrowser)))
			return;

		nsCOMPtr<nsIDOMWindow> dom;
		mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		NS_ENSURE_TRUE(dom, );

		nsCOMPtr<nsIDOMDocument> document;
		dom->GetDocument(getter_AddRefs(document));
		NS_ENSURE_TRUE(document, );*/

		if (mFullscreenDoc)
			mFullscreenDoc->MozCancelFullScreen();
		mFullscreenDoc = nullptr;
	}

protected:

	nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
	{
		NS_ENSURE_ARG(aWebBrowser);
		nsCOMPtr<nsIDOMWindow> domWin;
		aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
		NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);
	
		return domWin->GetWindowRoot (aTarget);
	}

	HWND mhWnd;
	nsCOMPtr<nsIDOMDocument> mFullscreenDoc;
};


