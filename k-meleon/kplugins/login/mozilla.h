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

#define MOZILLA_STRICT_API
#include <tchar.h>

#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsEmbedString.h"
#include "kmIHelper.h"
#include <nsIServiceManager.h>
#include <nsServiceManagerUtils.h>
#include "nsIAutoCompletePopup.h"
#include "nsIAutoCompleteInput.h"
#include "nsIFormFillController.h"
#include "nsPIDOMWindow.h"
#include "nsIContentViewer.h" 
#include "nsIDocShell.h"

#include "kmeleon_plugin.h"
extern kmeleonPlugin kPlugin;

HWND CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
	LPCTSTR lpszWindowName, DWORD dwStyle,
	int x, int y, int nWidth, int nHeight,
	HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam);

class AutocompletePopup : public nsIAutoCompletePopup
{
	protected:
	uint32_t mNbLine;
	HWND hList;
	float mZoom;
	nsCOMPtr<nsIAutoCompleteInput> mInput;
	nsCOMPtr<nsIDOMElement> mElement;	

	public:
	WNDPROC mDefWndProc;
	NS_DECL_ISUPPORTS
	NS_DECL_NSIAUTOCOMPLETEPOPUP

	AutocompletePopup(HWND hParent)
	{
		hList = CreateEx(
			WS_EX_TOPMOST|WS_EX_CONTROLPARENT|WS_EX_WINDOWEDGE,
			_T("ListBox"),_T("AutoComplete Form"),
			LBS_NOTIFY|WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL,
			0,0,0,0,
			hParent,
			0,this);
		::ShowWindow(hList, SW_HIDE);
		
		mDefWndProc = (WNDPROC) GetWindowLongW(hList, GWL_WNDPROC);
		SetWindowLongPtrW(hList, GWL_WNDPROC, (LONG_PTR)PopupWndProc);
		SetProp(hList, _T("popup"), reinterpret_cast<HANDLE>(this));

		mNbLine = 6;
		mZoom = 1;
	}

	void OnMouseMove(WPARAM w, LPARAM l) 
	{
		int sel = ::SendMessage(hList, LB_ITEMFROMPOINT, 0, l);
		::SendMessage(hList, LB_SETCURSEL, sel, 0);
	}

	void OnLeftButtonDown(WPARAM w, LPARAM l)
	{
		bool b;
		int sel = ::SendMessage(hList, LB_ITEMFROMPOINT, 0, l);
		::SendMessage(hList, LB_SETCURSEL, sel, 0);
		
		nsCOMPtr<nsIAutoCompleteController> controller;
		mInput->GetController(getter_AddRefs(controller));		
		if (controller) controller->HandleEnter(true, &b);
	}

	~AutocompletePopup()
	{
		::DestroyWindow(hList);
	}

	static LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		AutocompletePopup* popup = reinterpret_cast<AutocompletePopup*>(GetProp(hWnd, _T("popup")));
		if (message == WM_LBUTTONDOWN) {
			popup->OnLeftButtonDown(wParam, lParam);
		} else if (message == WM_MOUSEMOVE) {
			popup->OnMouseMove(wParam, lParam);
		}
		return CallWindowProc(popup->mDefWndProc, hWnd, message, wParam, lParam);
	}

};

class CDomEventListener : public nsIDOMEventListener
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER
	
	nsCOMPtr<nsIAutoCompletePopup> popup;

	CDomEventListener(HWND hParent)
	{
		mEventTarget = nullptr;
		mWebBrowser = nullptr;
		popup = new AutocompletePopup(hParent);
	}

	~CDomEventListener()
	{		
	}

	bool Init(HWND hwnd)
	{
		
		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return FALSE;

		nsCOMPtr<nsIServiceManager> servMan; 
		nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
		NS_ENSURE_SUCCESS(rv, false);

		nsCOMPtr<nsIFormFillController> fc;
		rv = servMan->GetServiceByContractID("@mozilla.org/satchel/form-fill-controller;1",  NS_GET_IID(nsIFormFillController), getter_AddRefs(fc));
		NS_ENSURE_SUCCESS(rv, false);		
		
		nsCOMPtr<nsIDOMWindow> domWin;
		mWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
		NS_ENSURE_TRUE(domWin, false);
		
		// Prevent crash in embed component if the browser was closed
		nsCOMPtr<nsIDOMDocument> doc;
		domWin->GetDocument(getter_AddRefs(doc));
		NS_ENSURE_TRUE(doc, false);

		nsCOMPtr<nsIDOMEventTarget> target;	
		domWin->GetWindowRoot (getter_AddRefs(target));
		NS_ENSURE_TRUE(target, false);
		
		nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWin));
		if (!piWin) return NULL;
		fc->AttachToBrowser(piWin->GetDocShell(), popup);
		
		nsCOMPtr<kmIHelper> klm;
		rv = servMan->GetServiceByContractID("@kmeleon/helper;1",  NS_GET_IID(kmIHelper), getter_AddRefs(klm));
		if (klm) klm->InitLogon(target, domWin);
		
		return TRUE;
	
		rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, FALSE);
	
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("DOMContentLoaded"),
							this, PR_FALSE);

		NS_ENSURE_SUCCESS(rv, FALSE);

		mhWnd = hwnd;
		return TRUE;
	}

	void Done(HWND hwnd)
	{
		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return;

		nsCOMPtr<nsIDOMWindow> domWin;
		mWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
		NS_ENSURE_TRUE (domWin, );
		nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWin));

		nsCOMPtr<nsIServiceManager> servMan; 
		nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
		NS_ENSURE_SUCCESS(rv, );

		nsCOMPtr<nsIFormFillController> fc;
		rv = servMan->GetServiceByContractID("@mozilla.org/satchel/form-fill-controller;1",  NS_GET_IID(nsIFormFillController), getter_AddRefs(fc));

		if (fc && piWin) fc->DetachFromBrowser(piWin->GetDocShell());
		
		if (mEventTarget)
			mEventTarget->RemoveEventListener(NS_LITERAL_STRING("DOMContentLoaded"),
				  this, PR_FALSE);
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

	nsCOMPtr<nsIDOMEventTarget> mEventTarget;
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	HWND mhWnd;
};
