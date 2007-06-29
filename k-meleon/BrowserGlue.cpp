 /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
/*
 *  Copyright (C) 2006 Dorian Boissonnade
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
 * ***** END LICENSE BLOCK ***** */
 

#include "stdafx.h"
#include "BrowserFrm.h"
#include "BrowserView.h"
#include "KmeleonConst.h"
#include "MozUtils.h"
#include "nsIURIFixup.h" // XXX

CBrowserGlue::~CBrowserGlue()
{
	if ( !mPopupBlockedHost.IsEmpty())
		mpBrowserFrame->UpdatePopupNotification(NULL);
}

void CBrowserGlue::UpdateStatusBarText(LPCTSTR aMessage)
{
	if (aMessage && *aMessage)
		mStatusText = aMessage;
	else
		mStatusText.LoadString(AFX_IDS_IDLEMESSAGE);

	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdateStatus(mStatusText);
}

void CBrowserGlue::UpdateProgress(int aCurrent, int aMax)
{
	mProgressMax = aMax;
	mProgressCurrent = aCurrent;
	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdateProgress(mProgressCurrent, mProgressMax);
}

void CBrowserGlue::UpdateBusyState(BOOL aBusy)
{       
    mDOMLoaded = mLoading = aBusy;
	if (aBusy) {
		mContextNode = nsnull;
		mpBrowserView->m_contextNode = nsnull;
	}
	else {
		SetFavIcon(nsnull);
		mPendingLocation = _T("");
	}

	mpBrowserFrame->PostMessage(UWM_UPDATEBUSYSTATE, aBusy == PR_TRUE ? 1 : 0, (LPARAM)mpBrowserView->GetSafeHwnd());
	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdateLoading(aBusy);
}

void CBrowserGlue::UpdateCurrentURI(nsIURI *aLocation)
{
    if(aLocation) 
    {
		nsEmbedCString uriString;
		nsCOMPtr<nsIURI> exposable;
		nsCOMPtr<nsIURIFixup> fixup(do_GetService("@mozilla.org/docshell/urifixup;1"));
		if (fixup && NS_SUCCEEDED(fixup->CreateExposableURI(aLocation, getter_AddRefs(exposable))) && exposable)
			exposable->GetSpec(uriString);
		else
			aLocation->GetSpec(uriString);

#ifdef INTERNAL_SITEICONS
		// Must be done here, before testing if we have the same address
		// because xul error page have its own icon, and since the address
		// doesn't change when retrying, the icon may stay in the urlbar.
		mIconURI = nsnull;
#endif
		mLocation = NSUTF8StringToCString(uriString);

		// XXX Since Mozilla 1.8.0.2 about:blank is always passed here
	    // before anything else, broking stuffs, so ignore it!
		if (mLocation.Compare(_T("about:blank")) == 0 &&
			mPendingLocation.GetLength())
			mLocation = mPendingLocation;


		if (mpBrowserFrame->GetActiveView() == mpBrowserView) {
			mpBrowserFrame->UpdateLocation(mLocation);
			if (!(mPopupBlockedHost.IsEmpty()))
				mpBrowserFrame->UpdatePopupNotification(NULL);
		}
		
		mPopupBlockedHost.Empty();

		// Add a MRU entry. Note that I'm only only allowing
		// http or https uri
		
		PRBool allowMRU,b;
		aLocation->SchemeIs("http", &b);
		allowMRU = b;
		aLocation->SchemeIs("https", &b);
		allowMRU |= b;

		if ( allowMRU ) {
			if (theApp.preferences.MRUbehavior == 0){
				nsEmbedCString password;
				aLocation->GetUsername(password);
				aLocation->SetUserPass(password);
				aLocation->GetSpec(uriString);
				theApp.m_MRUList->AddURL(NSUTF8StringToCString(uriString));
			}
			else if (theApp.preferences.MRUbehavior == 1){
				nsEmbedCString nsScheme, nsHost;
				aLocation->GetScheme(nsScheme);
				aLocation->GetHost(nsHost);
				nsHost.Insert("://",0);
				nsHost.Insert(nsScheme,0);
				theApp.m_MRUList->AddURL(NSUTF8StringToCString(nsHost));
			}
		}
    }
}

void CBrowserGlue::GetBrowserTitle(CString& aTitle)
{
	mpBrowserFrame->GetWindowText(aTitle);
}

void CBrowserGlue::SetBrowserTitle(LPCTSTR aTitle)
{
	mTitle = aTitle;
	if (mpBrowserFrame->GetActiveView() != mpBrowserView)
		return;

	mpBrowserFrame->UpdateTitle(mTitle);
	mpBrowserFrame->PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);   
}

void CBrowserGlue::SetBrowserSize(int aCX, int aCY)
{/*
   if (theApp.preferences.GetBool("kmeleon.display.dontResizeNewWindow", FALSE) && 
      !(mpBrowserFrame->IsDialog()))
	  return;*/

   CWnd* frame = mpBrowserView->GetParentFrame();
   
   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   frame->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   // first we have to figure out how much bigger the frame is than the view
   RECT frameRect, viewRect;
   frame->GetWindowRect(&frameRect);
   mpBrowserView->GetClientRect(&viewRect);

   int deltax = frameRect.right - frameRect.left - viewRect.right;
   int deltay = frameRect.bottom - frameRect.top - viewRect.bottom;

   frame->SetWindowPos(NULL, 0, 0, aCX+deltax, aCY+deltay,
      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
      );
}

void CBrowserGlue::SetFocus()
{
  //if (!::IsChild(pThis->m_hWnd,::GetFocus()))
	// pThis->BringWindowToTop();
}

void CBrowserGlue::SetVisibility(BOOL aVisibility)
{
	TRACE2("Set Visibility %u for window %s\n", aVisibility, mTitle);
    if(aVisibility)
    {
        if (mpBrowserFrame->IsIconic())
            return;
/*
        if (!mpBrowserFrame->IsWindowVisible()) {
            if (pThis->m_created)
                return;
            pThis->m_created = true;
        }*/

        mpBrowserFrame->ShowWindow(SW_SHOW);
        // pThis->SetActiveWindow();
        mpBrowserFrame->UpdateWindow();
    }
    else
    {
        mpBrowserFrame->ShowWindow(SW_HIDE);
    }
}

void CBrowserGlue::GetVisibility(BOOL *aVisible)
{
	*aVisible = mpBrowserFrame->IsIconic() || !mpBrowserFrame->IsWindowVisible() ? PR_FALSE : PR_TRUE;
}

void CBrowserGlue::DestroyBrowserFrame()
{
    mpBrowserFrame->PostMessage(WM_CLOSE, -1, -1);
}

HWND CBrowserGlue::GetBrowserFrameNativeWnd()
{
    return mpBrowserFrame->GetSafeHwnd();
}

void CBrowserGlue::UpdateSecurityStatus(PRInt32 aState)
{
	mSecurityState = aState;
	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdateSecurityStatus(aState);
}

void CBrowserGlue::ShowTooltip(PRInt32 x, PRInt32 y, const TCHAR *text)
{
    if (!text) {
        mpBrowserFrame->m_wndToolTip.Hide();
        return;
    }

    POINT point;
    ::GetCursorPos(&point);

	// XXX For an unknow reason the tooltips are also displayed when the cursor is 
	// outside the view, so verify that the mouse is on the view ...
	CRect r;
	mpBrowserView->GetWindowRect(r);
	if (!r.PtInRect(point))
		return;

	mpBrowserFrame->ScreenToClient(&point);
    point.y += GetSystemMetrics(SM_CYCURSOR)/2 + 4; // jump to below the cursor, otherwise we appear right on top of the cursor

    mpBrowserFrame->m_wndToolTip.Show(text, point.x, point.y);
}

BOOL CBrowserGlue::MouseAction(nsIDOMNode *node, UINT flags)
{
	mContextNode = node;
	mpBrowserView->m_contextNode = node;

	UINT id = theApp.accel.CheckMouse(flags);

	// Since accels can't currently be triggered depending on what's below
	// the cursor (link, img, ...) the middle mouse button for panning 
	// has to be handled separately.

	CString href, title;
	if (flags == MK_MBUTTON &&
	   (!id || !::GetLinkTitleAndHref(node, href, title))) {
		mpBrowserView->StartPanning(TRUE);
		return TRUE;
	} else if (id>0) {
		mpBrowserFrame->PostMessage(WM_COMMAND, (WPARAM)id, 0);
		return TRUE;
	}

	return FALSE;
}

void CBrowserGlue::PopupBlocked(const char* uri)
{
	// Do nothing if an icon was set already or if the user
	// don't want popup notification
	if (!theApp.preferences.GetBool("browser.popups.showPopupBlocker", PR_TRUE)
		|| !mPopupBlockedHost.IsEmpty())
		return;
	
	USES_CONVERSION;
	mPopupBlockedHost = A2CT(uri);
	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdatePopupNotification(mPopupBlockedHost);
}

void CBrowserGlue::SetFavIcon(nsIURI* favUri)
{	
#ifdef INTERNAL_SITEICONS	
	if (favUri == nsnull) 
	{
		// XXX Temporary set m_bDOMLoaded here
		mDOMLoaded = TRUE;

		// No site icon found then we're looking for a IE favicon
		// Note that this can be called twice, when DOMContentLoaded
		// is fired, and when the page finished to load.
		// DOMContentLoaded is not fired when page are loaded from cache
		// so I'm calling it also when the page is loaded to be sure we
		// checked for an IE icon. 

		if (mIconURI != nsnull) return;
		mIcon = theApp.favicons.GetDefaultIcon();
		
		if (theApp.preferences.GetBool("browser.chrome.favicons", PR_TRUE))
		{
			nsCOMPtr<nsIURI> currentURI;
			nsEmbedCString nsUri;

			mpBrowserView->GetBrowserWrapper()->GetCurrentURI(getter_AddRefs(currentURI));
			if (!currentURI) return;

			PRBool ishttp, b;
			currentURI->SchemeIs("http", &b);
			ishttp = b;
			currentURI->SchemeIs("https", &b);
			ishttp |= b;

			if (ishttp)
			{
				currentURI->Resolve(NS_LITERAL_CSTRING("/favicon.ico"), nsUri);

				nsCOMPtr<nsIURI> iconURI;
				nsresult rv = NewURI(getter_AddRefs(iconURI), nsUri);
				if(NS_FAILED(rv) || !iconURI) return;
				
				mIconURI = iconURI;
				mIcon = theApp.favicons.GetIcon(iconURI, TRUE);
			}
		}
	}
	else
	{
		mIconURI = favUri;
		mIcon = theApp.favicons.GetIcon(favUri, TRUE);
	}
	
	if (mpBrowserFrame->GetActiveView() == mpBrowserView)
		mpBrowserFrame->UpdateSiteIcon(mIcon);
#endif
}

BOOL CBrowserGlue::FocusNextElement()
{
   if (mpBrowserFrame->m_wndFindBar){
	  mpBrowserFrame->GetActiveView()->Activate(FALSE);
      mpBrowserFrame->m_wndFindBar->SetFocus();
   }
   else if (mpBrowserFrame->m_wndUrlBar.IsWindowVisible()) {
      mpBrowserFrame->GetActiveView()->Activate(FALSE);
      ::SetFocus(mpBrowserFrame->m_wndUrlBar.m_hwndEdit);
   }
   else 
      return FALSE;
   return TRUE;
}

BOOL CBrowserGlue::FocusPrevElement()
{
   if (mpBrowserFrame->m_wndUrlBar.IsWindowVisible()) {
      mpBrowserFrame->GetActiveView()->Activate(FALSE);
      mpBrowserFrame->m_wndFindBar->SetFocus();
	  return TRUE;
   }

   return FALSE;
}

void  CBrowserGlue::ShowContextMenu(UINT aContextFlags, nsIDOMNode* node)
{
	CString menu;
	if ( !(aContextFlags & nsIContextMenuListener2::CONTEXT_LINK) &&
		!(aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE) &&
		mpBrowserView->GetBrowserWrapper()->CanCopy())
	{
		menu = _T("SelectedText");
	}
	else {
		if (mpBrowserView->GetBrowserWrapper()->GetFrameURL(node).GetLength())
			menu = _T("Frame");

		if(aContextFlags & nsIContextMenuListener2::CONTEXT_DOCUMENT)
		{
			if ((aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE) ||
			    (aContextFlags & nsIContextMenuListener2::CONTEXT_BACKGROUND_IMAGE))
				menu += _T("DocumentImagePopup");
			else
				menu += _T("DocumentPopup");
		}
		else if(aContextFlags & nsIContextMenuListener2::CONTEXT_TEXT)
		{
			menu += _T("TextPopup");
		}
		else if(aContextFlags & nsIContextMenuListener2::CONTEXT_LINK)
		{
			if (aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)
				menu += _T("ImageLinkPopup");
			else
				menu += _T("LinkPopup");
		}
		else if(aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)
		{
			menu += _T("ImagePopup");
		}
		else menu += _T("DocumentPopup");
	}

	mpBrowserView->m_contextNode = node;
	CMenu *ctxMenu = theApp.menus.GetMenu(menu);
	if(ctxMenu)
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ctxMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, mpBrowserFrame);
	}

}

CBrowserWrapper* CBrowserGlue::CreateNewBrowser(PRUint32 chromeMask)
{
	return NULL;
}

CBrowserWrapper* CBrowserGlue::ReuseWindow(BOOL useCurrent)
{
	if (useCurrent && !mpBrowserFrame->IsPopup() && !mpBrowserFrame->IsDialog())
		return mpBrowserView->GetBrowserWrapper();

	return NULL;
}