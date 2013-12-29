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

#include "stdafx.h"
#include <math.h>
#include "mozilla.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsServiceManagerUtils.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompleteController.h"
#include "nsIDOMElement.h"
#include "nsIDOMClientRect.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebNavigation.h"
#include "../utf.h"
#include "nsIMarkupDocumentViewer.h"

nsresult NewURI(nsIURI **result, const nsACString &spec)
{
  nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, NS_ERROR_UNEXPECTED);

  return ios->NewURI(spec, nullptr, nullptr, result);
}

nsresult NewURI(nsIURI **result, const nsAString &spec)
{
  nsEmbedCString specUtf8;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, specUtf8);
  return NewURI(result, specUtf8);
}

NS_IMPL_ISUPPORTS1(CDomEventListener, nsIDOMEventListener)

NS_IMETHODIMP CDomEventListener::HandleEvent (nsIDOMEvent* aEvent)
{
	nsresult rv;
	nsEmbedString type;
	aEvent->GetType(type);

	if (type.Equals(NS_LITERAL_STRING("DOMContentLoaded")))
	{

		nsCOMPtr<nsIDOMEventTarget> target;
		aEvent->GetTarget(getter_AddRefs(target));
		NS_ENSURE_TRUE (target, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(target);
		NS_ENSURE_TRUE (elem, NS_ERROR_FAILURE);

		nsCOMPtr<nsIDOMDocument> domDoc;
		elem->GetOwnerDocument (getter_AddRefs(domDoc));
		NS_ENSURE_TRUE (domDoc, NS_ERROR_FAILURE);

		nsEmbedString spec;
		rv = domDoc->GetDocumentURI (spec);
		NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);
		
		nsCOMPtr<nsIURI> docUri;
		rv = NewURI(getter_AddRefs (docUri), spec);
		NS_ENSURE_SUCCESS (rv, NS_ERROR_FAILURE);

	}

	return NS_OK;
}

 
NS_IMPL_ISUPPORTS1(AutocompletePopup, nsIAutoCompletePopup)


/* readonly attribute nsIAutoCompleteInput input; */
NS_IMETHODIMP AutocompletePopup::GetInput(nsIAutoCompleteInput * *aInput)
{
	*aInput = mInput;
    return NS_OK;
}

/* readonly attribute AString overrideValue; */
NS_IMETHODIMP AutocompletePopup::GetOverrideValue(nsAString & aOverrideValue)
{
    return NS_OK;
}

/* attribute long selectedIndex; */
NS_IMETHODIMP AutocompletePopup::GetSelectedIndex(int32_t *aSelectedIndex)
{
	*aSelectedIndex = ::SendMessage(hList, LB_GETCURSEL, 0, 0); 
    return NS_OK;
}
NS_IMETHODIMP AutocompletePopup::SetSelectedIndex(int32_t aSelectedIndex)
{
	::SendMessage(hList, LB_SETCURSEL, aSelectedIndex, 0);
    return NS_OK;
}

/* readonly attribute boolean popupOpen; */
NS_IMETHODIMP AutocompletePopup::GetPopupOpen(bool *aPopupOpen)
{
	*aPopupOpen = IsWindowVisible(hList);
	return NS_OK;
}

int GetDPI() 
{
	HDC hDC = ::GetDC(NULL);
	int r = GetDeviceCaps(hDC, LOGPIXELSY);
	::ReleaseDC(NULL,hDC);
	return r;
}

int GetFontSize()
{
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0))
		return FALSE;

	LOGFONT &lf = ncm.lfMessageFont;	
	int nPointSize  = MulDiv(14,GetDPI(),96);
	
	return nPointSize;
}

/* void openAutocompletePopup (in nsIAutoCompleteInput input, in nsIDOMElement element); */
NS_IMETHODIMP AutocompletePopup::OpenAutocompletePopup(nsIAutoCompleteInput *input, nsIDOMElement *element)
{
	nsresult rv;
	mInput = input;
	mElement = element;

	nsCOMPtr<nsIAutoCompleteController> controller;
	mInput->GetController(getter_AddRefs(controller));
	NS_ENSURE_TRUE(controller, NS_ERROR_NOT_AVAILABLE);
	
	uint32_t nLine;
	controller->GetMatchCount(&nLine);
	if (nLine == 0) {
		ClosePopup();
		return NS_OK;
	}
	
	nsCOMPtr<nsIDOMClientRect> rect;
	rv = element->GetBoundingClientRect(getter_AddRefs(rect));
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCOMPtr<nsIDOMDocument> domDoc;
	rv = element->GetOwnerDocument(getter_AddRefs(domDoc));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIDOMWindow> domWin;
	rv = domDoc->GetDefaultView(getter_AddRefs(domWin));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(domWin));
	nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(webNav);
	NS_ENSURE_TRUE(docShell, NS_ERROR_NOT_AVAILABLE);

	nsCOMPtr<nsIContentViewer> contentViewer;
	rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMarkupDocumentViewer> markupViewer = do_QueryInterface(contentViewer, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	// Calc height of the list
	int lineHeight = ::SendMessage(hList, LB_GETITEMHEIGHT, 0, 0L);
	int height = nLine > mNbLine ? (mNbLine+1)*lineHeight : lineHeight*(nLine+1);

	// Match the font size with the zoom
	float zoom, fzoom;
	markupViewer->GetTextZoom(&zoom);
	markupViewer->GetFullZoom(&fzoom);
	
	zoom *= fzoom;
	if (zoom != mZoom) {
		HFONT hFont = (HFONT)::SendMessage(hList, WM_GETFONT, 0, 0);
		LOGFONT logFont = {0};
		if (hFont) {
			GetObject(hFont, sizeof(LOGFONT), &logFont);
			DeleteObject(hFont);
		}
		
		logFont.lfHeight = floor(GetFontSize() * zoom);
		HFONT hNewFont = CreateFontIndirect(&logFont);
		::SendMessage(hList, WM_SETFONT, (WPARAM)hNewFont, 0);		
		mZoom = zoom;
	}	

	// Get input coordinate
	int dpi = GetDPI();
	float ratio, left, bottom, width;
	rect->GetLeft(&left);
	rect->GetBottom(&bottom);
	rect->GetWidth(&width);	
	domWin->GetDevicePixelRatio(&ratio);

	// Calculate offset, toolbars
	// Should make the view as parent?
	int x  = GetSystemMetrics(SM_CYSIZEFRAME);
	RECT rcView, rcFrame, rectDesktop;
	kPlugin.kFuncs->GetBrowserviewRect(::GetParent(hList), &rcView);
	::GetWindowRect(::GetParent(hList), &rcFrame);
	//::GetWindowRect(::GetDesktopWindow(), &rectDesktop );
	int offsetY = rcView.top - rcFrame.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYSIZEFRAME) + 3;
	int offsetX = 3;

	// Set the popup on top when too low
	int posY = bottom*ratio;
	if (posY + height - lineHeight + rcView.top > rcView.bottom) {
		float top;
		rect->GetTop(&top);
		posY = top*ratio - height + lineHeight - 2;
	}

	::SetWindowPos(hList, HWND_TOP,left*ratio+offsetX, posY+offsetY, width*ratio,height,SWP_SHOWWINDOW);
	Invalidate();
	return NS_OK;
}

#include "nsIFocusManager.h"
/* void closePopup (); */
NS_IMETHODIMP AutocompletePopup::ClosePopup()
{
	::ShowWindow(hList, SW_HIDE);
	::SetActiveWindow(GetParent(hList));
	
	mElement = nullptr;
	mInput = nullptr;
    return NS_OK;
}

/* void invalidate (); */
NS_IMETHODIMP AutocompletePopup::Invalidate()
{
	if (!mInput) return NS_OK;
	::SendMessage(hList, LB_RESETCONTENT, 0, 0);
	uint32_t nLine;
	
	nsCOMPtr<nsIAutoCompleteController> controller;
	mInput->GetController(getter_AddRefs(controller));

	controller->GetMatchCount(&nLine);
	for (uint i=0;i<nLine;i++) {
		nsString line;
		controller->GetLabelAt(i, line);
		::SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)line.get());
	}

	RECT rect;
	::GetWindowRect(hList, &rect);
	
	int height = ::SendMessage(hList, LB_GETITEMHEIGHT, 0, 0L);
	height = nLine > mNbLine ? (mNbLine+1)*height : height*(nLine+1);
	::SetWindowPos(hList, HWND_TOP,0, 0, rect.right-rect.left,height,SWP_NOZORDER|SWP_NOMOVE);

    return NS_OK;
}

/* void selectBy (in boolean reverse, in boolean page); */
NS_IMETHODIMP AutocompletePopup::SelectBy(bool reverse, bool page)
{
	int count = ::SendMessage(hList, LB_GETCOUNT, 0, 0);
	int newsel, sel = ::SendMessage(hList, LB_GETCURSEL, 0, 0); 
	int inc = page ? mNbLine : 1;
	
	if (sel == LB_ERR) 
		newsel = reverse ? count-1 : 0;
	else {
		newsel = reverse ? sel-inc : sel+inc;
	}
	if (newsel < 0) newsel = page ? 0 : count - 1;
	else if (newsel >= count) newsel = page ? count-1 : 0;
	::SendMessage(hList, LB_SETCURSEL, newsel, 0);
    return NS_OK;
}


HWND CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
	LPCTSTR lpszWindowName, DWORD dwStyle,
	int x, int y, int nWidth, int nHeight,
	HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	CREATESTRUCT cs;
	cs.dwExStyle = dwExStyle;
	cs.lpszClass = lpszClassName;
	cs.lpszName = lpszWindowName;
	cs.style = dwStyle;
	cs.x = x;
	cs.y = y;
	cs.cx = nWidth;
	cs.cy = nHeight;
	cs.hwndParent = hWndParent;
	cs.hMenu = nIDorHMenu;
	cs.hInstance = kPlugin.hParentInstance;
	cs.lpCreateParams = lpParam;

	return ::CreateWindowEx(cs.dwExStyle, cs.lpszClass,
			cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
			cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);

}