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

// File Overview....
//
// This file has the IBrowserFrameGlueObj implementation
// This frame glue object is nested inside of the BrowserFrame
// object(See BrowserFrm.h for more info)
//
// This is the place where all the platform specific interaction
// with the browser frame window takes place in response to 
// callbacks from Gecko interface methods
// 
// The main purpose of this interface is to separate the cross 
// platform code in BrowserImpl*.cpp from the platform specific
// code(which is in this file)
//
// You'll also notice the use of a macro named "METHOD_PROLOGUE"
// through out this file. This macro essentially makes the pointer
// to a "containing" class available inside of the class which is
// being contained via a var named "pThis". In our case, the 
// BrowserFrameGlue object is contained inside of the BrowserFrame
// object so "pThis" will be a pointer to a BrowserFrame object
// Refer to MFC docs for more info on METHOD_PROLOGUE macro


#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserFrm.h"
#include "Dialogs.h"
#include "MenuParser.h"
#include "KmeleonConst.h"
#include "nsIDOMHTMLAreaElement.h"

extern CMfcEmbedApp theApp;

/////////////////////////////////////////////////////////////////////////////
// IBrowserFrameGlue implementation

void CBrowserFrame::BrowserFrameGlueObj::UpdateStatusBarText(const PRUnichar *aMessage)
{
#ifndef _UNICODE
   if (aMessage && (wcslen(aMessage) > 1024)) return;
#endif
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
   USES_CONVERSION;
   CString str;
   if (aMessage && wcslen(aMessage) > 0)
	 str = W2CT(aMessage);
   else
	 str.LoadString(AFX_IDS_IDLEMESSAGE);

   pThis->m_wndStatusBar.SetPaneText(0, str);
}

void CBrowserFrame::BrowserFrameGlueObj::UpdateProgress(PRInt32 aCurrent, PRInt32 aMax)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   pThis->m_wndProgressBar.SetRange32(0, aMax);
   pThis->m_wndProgressBar.SetPos(aCurrent);
}

void CBrowserFrame::BrowserFrameGlueObj::UpdateBusyState(PRBool aBusy)
{       
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    // Just notify the view of the busy state
    // There's code in there which will take care of
    // updating the STOP toolbar btn. etc

    pThis->m_wndBrowserView.UpdateBusyState(aBusy);
    //if (!aBusy)	
		pThis->PostMessage(UWM_UPDATEBUSYSTATE, aBusy == PR_TRUE ? 1 : 0, 0);

	if (!aBusy) {
	  CString szUrl;
      pThis->m_wndUrlBar.GetEnteredURL(szUrl);
      if (_tcscmp(szUrl, _T("about:blank"))==0)
        pThis->m_wndUrlBar.MaintainFocus();
	}
	else
		pThis->m_wndBrowserView.mbDOMLoaded = FALSE;

	pThis->m_wndBrowserView.m_lastMouseActionNode = nsnull;
}

// Called from the OnLocationChange() method in the nsIWebProgressListener 
// interface implementation in CBrowserImpl to update the current URI
// Will get called after a URI is successfully loaded in the browser
// We use this info to update the URL bar's edit box
//
void CBrowserFrame::BrowserFrameGlueObj::UpdateCurrentURI(nsIURI *aLocation)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    if(aLocation) 
    {
        USES_CONVERSION;
        nsEmbedCString uriString;
        aLocation->GetSpec(uriString);

        nsEmbedString uriString2;
        NS_CStringToUTF16(uriString, NS_CSTRING_ENCODING_UTF8, uriString2);

		// Reset the popup notification and the icon uri
        if (!(pThis->m_wndBrowserView.m_csHostPopupBlocked.IsEmpty())) {
			pThis->m_wndStatusBar.RemoveIcon(ID_POPUP_BLOCKED_ICON);
			pThis->m_wndBrowserView.m_csHostPopupBlocked.Truncate(0);
		}

		// Prevent to move the caret in the urlbar
		CString currentURL;
		pThis->m_wndUrlBar.GetEnteredURL(currentURL);
		if (currentURL.Compare(W2CT(uriString2.get())) == 0)
			return;

        pThis->m_wndUrlBar.SetCurrentURL(W2CT(uriString2.get()));

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
				pThis->m_wndUrlBar.AddURLToList(CString(A2CW(uriString.get())));
			}
			else if (theApp.preferences.MRUbehavior == 1){
				nsEmbedCString nsScheme, nsHost;
				aLocation->GetScheme(nsScheme);
				aLocation->GetHost(nsHost);
				nsHost.Insert("://",0);
				nsHost.Insert(nsScheme,0);
				pThis->m_wndUrlBar.AddURLToList(CString(A2CW(nsHost.get())));
			}
		}
    }
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameTitle(PRUnichar **aTitle)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    CString title;
    pThis->GetWindowText(title);

/*    TCHAR psz[256];
	CString appTitle;
    appTitle.LoadString(AFX_IDS_APP_TITLE);
	theApp.preferences.GetString("kmeleon.display.title", psz, appTitle.GetBuffer(0));
	appTitle = psz;

    title.Replace(_T(" (") + appTitle + _T(')'), _T(""));
*/
    if(!title.IsEmpty())
    {
        USES_CONVERSION;
        nsEmbedString nsTitle;
        nsTitle.Assign(T2CW(title));
        *aTitle = NS_StringCloneData(nsTitle);
    }
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameTitle(const PRUnichar *aTitle)
{
#ifndef _UNICODE
    if (wcslen(aTitle) > 1024) return;
#endif
	
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    TCHAR psz[256];
	CString appTitle;
    appTitle.LoadString(AFX_IDS_APP_TITLE);
	theApp.preferences.GetString("kmeleon.display.title", psz, appTitle.GetBuffer(0));
	appTitle = psz;
	
    CString title;
    USES_CONVERSION;
    title = W2CT(aTitle);

    if (title.IsEmpty()){
        pThis->m_wndUrlBar.GetEnteredURL(title);
    }

	if (!appTitle.IsEmpty())
		title += _T(" (") + appTitle + _T(")");
    pThis->SetWindowText(title);

    pThis->PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameSize(PRInt32 aCX, PRInt32 aCY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
   
   if (pThis->m_ignoreMoveResize > 0) {
       pThis->m_ignoreMoveResize--;
       return;
   }

   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   pThis->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   pThis->SetWindowPos(NULL, 0, 0, aCX, aCY, 
      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
      );
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserSize(PRInt32 aCX, PRInt32 aCY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
   
   if (pThis->m_ignoreMoveResize > 0) {
       pThis->m_ignoreMoveResize--;
       return;
   }

   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   pThis->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   // first we have to figure out how much bigger the frame is than the view
   RECT frameRect, viewRect;
   pThis->GetClientRect(&frameRect);
   pThis->m_wndBrowserView.GetClientRect(&viewRect);

   int deltax = (frameRect.right - viewRect.right);
   int deltay = (frameRect.bottom - viewRect.bottom);

   pThis->SetWindowPos(NULL, 0, 0, aCX+deltax, aCY+deltay,
      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
      );
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameSize(PRInt32 *aCX, PRInt32 *aCY)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    RECT wndRect;
    pThis->GetWindowRect(&wndRect);

    if (aCX)
        *aCX = wndRect.right - wndRect.left;

    if (aCY)
        *aCY = wndRect.bottom - wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserSize(PRInt32 *aCX, PRInt32 *aCY)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    RECT wndRect;
    pThis->m_wndBrowserView.GetClientRect(&wndRect);

    if (aCX)
        *aCX = wndRect.right - wndRect.left;

    if (aCY)
        *aCY = wndRect.bottom - wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFramePosition(PRInt32 aX, PRInt32 aY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)  
   
   if (pThis->m_ignoreMoveResize > 0) {
       pThis->m_ignoreMoveResize--;
       return;
   }

   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   pThis->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   pThis->SetWindowPos(NULL, aX, aY, 0, 0, 
      SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFramePosition(PRInt32 *aX, PRInt32 *aY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   RECT wndRect;
   pThis->GetWindowRect(&wndRect);

   if (aX)
      *aX = wndRect.left;

   if (aY)
      *aY = wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFramePositionAndSize(PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   RECT wndRect;
   pThis->GetWindowRect(&wndRect);

   if (aX)
      *aX = wndRect.left;

   if (aY)
      *aY = wndRect.top;

   if (aCX)
      *aCX = wndRect.right - wndRect.left;

   if (aCY)
      *aCY = wndRect.bottom - wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFramePositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
   
   if (pThis->m_ignoreMoveResize > 0) {
       pThis->m_ignoreMoveResize--;
       return;
   }

   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   pThis->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   pThis->SetWindowPos(NULL, aX, aY, aCX, aCY, 
      SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserPositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
   
   if (pThis->m_ignoreMoveResize > 0) {
       pThis->m_ignoreMoveResize--;
       return;
   }
   
   WINDOWPLACEMENT wp;
   wp.length = sizeof(WINDOWPLACEMENT);
   pThis->GetWindowPlacement(&wp);
   if (wp.showCmd != SW_SHOWNORMAL)
       return;

   // first we have to figure out how much bigger the frame is than the view
   RECT frameRect, viewRect;
   pThis->GetWindowRect(&frameRect);
   pThis->m_wndBrowserView.GetClientRect(&viewRect);

   int deltax = (frameRect.right-frameRect.left)-(viewRect.right-viewRect.left);
   int deltay = (frameRect.bottom-frameRect.top)-(viewRect.bottom-viewRect.top);

   pThis->SetWindowPos(NULL, aX, aY, aCX+deltax, aCY+(deltay/2),
      SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::SetFocus(){
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
  //if (!::IsChild(pThis->m_hWnd,::GetFocus()))
	   //pThis->SetFocus();

   pThis->m_wndBrowserView.mBaseWindow->SetFocus();
}

void CBrowserFrame::BrowserFrameGlueObj::FocusAvailable(PRBool *aFocusAvail)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    HWND focusWnd = ::GetFocus(); // GetFocus()->m_hWnd;

    if ((focusWnd == pThis->m_hWnd) || ::IsChild(pThis->m_hWnd, focusWnd))
        *aFocusAvail = PR_TRUE;
    else
        *aFocusAvail = PR_FALSE;
}

void CBrowserFrame::BrowserFrameGlueObj::ShowBrowserFrame(PRBool aShow)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
      
    if(aShow)
    {
        if (pThis->IsIconic())
            return;

        if (!pThis->IsWindowVisible()) {
            if (pThis->m_created)
                return;
            pThis->m_created = true;
        }

        pThis->ShowWindow(SW_SHOW);
        // pThis->SetActiveWindow();
        pThis->UpdateWindow();
    }
    else
    {
        pThis->ShowWindow(SW_HIDE);
    }
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameVisibility(PRBool *aVisible)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   // Is the current BrowserFrame the active one?
   CWnd *pWnd = GetActiveWindow();
   if (!pWnd || pWnd->m_hWnd != pThis->m_hWnd)
   {
      *aVisible = PR_FALSE;
      return;
   }

   if (pThis->IsIconic() || !pThis->IsWindowVisible()) {
      *aVisible = PR_FALSE;
      return;
   }

   // We're the active one
   //Return FALSE if we're minimized
   WINDOWPLACEMENT wpl;
   pThis->GetWindowPlacement(&wpl);

   if ((wpl.showCmd == SW_NORMAL) || (wpl.showCmd == SW_MAXIMIZE))
      *aVisible = PR_TRUE;
   else
      *aVisible = PR_FALSE;
}

PRBool CBrowserFrame::BrowserFrameGlueObj::CreateNewBrowserFrame(PRUint32 chromeMask, 
                            PRInt32 x, PRInt32 y, 
                            PRInt32 cx, PRInt32 cy,
                            nsIWebBrowser** aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);

    *aWebBrowser = nsnull;

    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
    if(!pApp)
        return PR_FALSE;

    // Note that we're calling with the last param set to "false" i.e.
    // this instructs not to show the frame window
    // This is mainly needed when the window size is specified in the window.open()
    // JS call. In those cases Gecko calls us to create the browser with a default
    // size (all are -1) and then it calls the SizeBrowserTo() method to set
    // the proper window size. If this window were to be visible then you'll see
    // the window size changes on the screen causing an unappealing flicker
    //
    
    CBrowserFrame* pFrm = pApp->CreateNewBrowserFrame(chromeMask, x, y, cx, cy, PR_FALSE);
    if(!pFrm)
        return PR_FALSE;

    // At this stage we have a new CBrowserFrame and a new CBrowserView
    // objects. The CBrowserView also would have an embedded browser
    // object created. Get the mWebBrowser member from the CBrowserView
    // and return it. (See CBrowserView's CreateBrowser() on how the
    // embedded browser gets created and how it's mWebBrowser member
    // gets initialized)

    NS_IF_ADDREF(*aWebBrowser = pFrm->m_wndBrowserView.mWebBrowser);

    return PR_TRUE;
}

void CBrowserFrame::BrowserFrameGlueObj::DestroyBrowserFrame()
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    pThis->PostMessage(WM_CLOSE, -1, -1);
}

#define GOTO_BUILD_CTX_MENU { bContentHasFrames = FALSE; goto BUILD_CTX_MENU; }

void CBrowserFrame::BrowserFrameGlueObj::ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo)
{  
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    BOOL bContentHasFrames = FALSE;
    BOOL bContentHasImage = FALSE;
    UINT nIDResource = IDR_CTXMENU_DOCUMENT;

    if (pThis->m_wndBrowserView.m_iGetNodeHack == 0) {
         if (GetKeyState(VK_SHIFT) < 0 ||
             GetKeyState(VK_CONTROL) < 0 || 
             GetKeyState(VK_MENU) < 0)
              return;
    }
         
    // Reset the values from the last invocation
    // Clear image src & link url
    nsEmbedString empty;
    pThis->m_wndBrowserView.SetCtxMenuImageSrc(empty);  
    pThis->m_wndBrowserView.SetCtxMenuLinkUrl(empty);
    pThis->m_wndBrowserView.SetCurrentFrameURL(empty);


/*
  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!

  The bGetElementHack flag is part of the GetElementFromPoint function.
  Basically, there's no easy way that I've found to get the link
  information by point from mozilla, so as a workaround, the function
  simply sends a contextmenu notifier with the point we want.  It's
  our job here to make sure the context menu doesn't get shown.
  
  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!
*/

   nsCOMPtr<nsIDOMNode> node;
   aInfo->GetTargetNode(getter_AddRefs(node));

   
   if (pThis->m_wndBrowserView.m_iGetNodeHack == 1) {
      pThis->m_wndBrowserView.m_iGetNodeHack = 0;
      pThis->m_wndBrowserView.m_pGetNode = node;
      return;
   }


    if(aContextFlags & nsIContextMenuListener2::CONTEXT_DOCUMENT)
    {
        nIDResource = IDR_CTXMENU_DOCUMENT;

        // Background image?
        if (aContextFlags & nsIContextMenuListener2::CONTEXT_BACKGROUND_IMAGE)
        {
            // Get the IMG SRC
            nsCOMPtr<nsIURI> imgURI;
            aInfo->GetBackgroundImageSrc(getter_AddRefs(imgURI));
            if (!imgURI)
                return; 
            nsEmbedCString uri;
            imgURI->GetSpec(uri);
            bContentHasImage = TRUE;

            nsEmbedString uri2;
            NS_CStringToUTF16(uri, NS_CSTRING_ENCODING_UTF8, uri2);
            pThis->m_wndBrowserView.SetCtxMenuImageSrc(uri2); // Set the new Img Src
        }
    }
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_TEXT || 
            aContextFlags & nsIContextMenuListener2::CONTEXT_INPUT) 
        nIDResource = IDR_CTXMENU_TEXT;
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_LINK)
    {
        nIDResource = IDR_CTXMENU_LINK;

        // Since we handle all the browser menu/toolbar commands
        // in the View, we basically setup the Link's URL in the
        // BrowserView object. When a menu selection in the context
        // menu is made, the appropriate command handler in the
        // BrowserView will be invoked and the value of the URL
        // will be accesible in the view
        
        nsEmbedString strUrlUcs2;
        nsresult rv = aInfo->GetAssociatedLink(strUrlUcs2);
        if(NS_FAILED(rv))
            return;

        // Update the view with the new LinkUrl
        // Note that this string is in UCS2 format
        pThis->m_wndBrowserView.SetCtxMenuLinkUrl(strUrlUcs2);

        // Test if there is an image URL as well
        nsCOMPtr<nsIURI> imgURI;
        aInfo->GetImageSrc(getter_AddRefs(imgURI));
        if(imgURI)
        {
            nsEmbedCString strImgSrcUtf8;
            imgURI->GetSpec(strImgSrcUtf8);
            if(!strImgSrcUtf8.IsEmpty())
            {
                // Set the new Img Src
                nsEmbedString strImgSrc;
                NS_CStringToUTF16(strImgSrcUtf8, NS_CSTRING_ENCODING_UTF8, strImgSrc);
                pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrc);
                bContentHasImage = TRUE;
            }
        }
    }
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)
    {
        nIDResource = IDR_CTXMENU_IMAGE;

        // Get the IMG SRC
        nsCOMPtr<nsIURI> imgURI;
        aInfo->GetImageSrc(getter_AddRefs(imgURI));
        if(!imgURI)
            return;
        nsEmbedCString strImgSrcUtf8;
        imgURI->GetSpec(strImgSrcUtf8);
        if(strImgSrcUtf8.IsEmpty())
            return;

        // Set the new Img Src
        nsEmbedString strImgSrc;
        NS_CStringToUTF16(strImgSrcUtf8, NS_CSTRING_ENCODING_UTF8, strImgSrc);
        pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrc);
    }

    // Determine if we need to add the Frame related context menu items
    // such as "View Frame Source" etc.
    //
    if(pThis->m_wndBrowserView.ViewContentContainsFrames())
    {
        bContentHasFrames = TRUE;

        //Determine the current Frame URL
        //
        nsresult rv = NS_OK;
        nsCOMPtr<nsIDOMNode> node;
        aInfo->GetTargetNode(getter_AddRefs(node));
        if(!node)
            GOTO_BUILD_CTX_MENU;

        nsCOMPtr<nsIDOMDocument> domDoc;
        rv = node->GetOwnerDocument(getter_AddRefs(domDoc));
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(domDoc, &rv));
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        nsEmbedString strFrameURL;
        rv = htmlDoc->GetURL(strFrameURL);
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        pThis->m_wndBrowserView.SetCurrentFrameURL(strFrameURL); //Set it to the new URL
    }

BUILD_CTX_MENU:

    TCHAR *menuType = _T("<nothing>");
    if (!bContentHasFrames) {
        switch (nIDResource) {
        case IDR_CTXMENU_DOCUMENT:
            menuType = _T("DocumentPopup");
            if (bContentHasImage)
                menuType = _T("DocumentImagePopup");
            break;
        case IDR_CTXMENU_TEXT:
            menuType = _T("TextPopup");
            break;
        case IDR_CTXMENU_LINK:
            menuType = _T("LinkPopup");
            if (bContentHasImage)
                menuType = _T("ImageLinkPopup");
            break;
        case IDR_CTXMENU_IMAGE:
            menuType = _T("ImagePopup");
            break;
        }
    }
    else {
        switch (nIDResource) {
        case IDR_CTXMENU_DOCUMENT:
            menuType = _T("FrameDocumentPopup");
            if (bContentHasImage)
                menuType = _T("FrameDocumentImagePopup");
            break;
        case IDR_CTXMENU_TEXT:
            menuType = _T("FrameTextPopup");
            break;
        case IDR_CTXMENU_LINK:
            menuType = _T("FrameLinkPopup");
            if (bContentHasImage)
                menuType = _T("FrameImageLinkPopup");
            break;
        case IDR_CTXMENU_IMAGE:
            menuType = _T("FrameImagePopup");
            break;
        }
    }


   /*  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  !!BAD HACK!!  */
   if (pThis->m_wndBrowserView.m_iGetNodeHack == 2) {
      pThis->m_wndBrowserView.m_iGetNodeHack = 0;
      pThis->m_wndBrowserView.m_pGetNode = node;
      return;
   }


   CMenu *ctxMenu = theApp.menus.GetMenu(menuType);
   if(ctxMenu)
   {
      POINT cursorPos;
      GetCursorPos(&cursorPos);
      
   /*   int offset = theApp.menus.GetOffset(ctxMenu);
      
      RECT desktopRect;
      SystemParametersInfo(SPI_GETWORKAREA, NULL, &desktopRect, 0);
      int menuHeight = 0;
      unsigned int i;
      MENUITEMINFO info;
      for (i=0; i<ctxMenu->GetMenuItemCount(); i++) {
          ctxMenu->GetMenuItemInfo(i, &info, TRUE);
          if (info.fType & MFT_SEPARATOR)
              menuHeight += 0;
          else
              menuHeight += GetSystemMetrics(SM_CYMENUSIZE);
      }
      if ( (int)(cursorPos.y - offset) < (int)(desktopRect.bottom - menuHeight) ){
         // we only do this if we're not too close to the bottom of the screen
         cursorPos.y -= offset;
         if (cursorPos.y < 0){
            cursorPos.y = 0;
         }
      } else if (cursorPos.y + menuHeight > desktopRect.bottom) {
          cursorPos.y = desktopRect.bottom - menuHeight;
      }*/
      
      ctxMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, pThis);
   
   }
}

HWND CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameNativeWnd()
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
    return pThis->m_hWnd;
}

void CBrowserFrame::BrowserFrameGlueObj::UpdateSecurityStatus(PRInt32 aState)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    pThis->UpdateSecurityStatus(aState);
}

void CBrowserFrame::BrowserFrameGlueObj::ShowTooltip(PRInt32 x, PRInt32 y, const TCHAR *text)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    if (!text) {
        pThis->m_wndToolTip.Hide();
		// WORKAROUND: Because the tooltip is not erased correctly when using the mouse wheel
		pThis->m_wndBrowserView.mBaseWindow->Repaint(PR_FALSE);
        return;
    }

    POINT point;
    ::GetCursorPos(&point);

    pThis->m_wndBrowserView.ScreenToClient(&point);
    point.y += GetSystemMetrics(SM_CYCURSOR)/2 + 4; // jump to below the cursor, otherwise we appear right on top of the cursor

    pThis->m_wndToolTip.Show(text, point.x, point.y);
}

void CBrowserFrame::BrowserFrameGlueObj::FocusNextElement() {
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(pThis->m_wndBrowserView.mWebBrowser));
   if(focus) {
	   focus->Deactivate();
	   if (pThis->m_wndFindBar)
		   pThis->m_wndFindBar->SetFocus();
	   else
		   // pThis->m_wndUrlBar.MaintainFocus();   
		   ::SetFocus(pThis->m_wndUrlBar.m_hwndEdit);
   }
}

void CBrowserFrame::BrowserFrameGlueObj::FocusPrevElement() {
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(pThis->m_wndBrowserView.mWebBrowser));
   if(focus) {
      // pThis->m_wndUrlBar.MaintainFocus();
	  focus->Deactivate();
      ::SetFocus(pThis->m_wndUrlBar.m_hwndEdit);
   }
}

void CBrowserFrame::BrowserFrameGlueObj::MouseAction(nsIDOMNode *node)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
	pThis->m_wndBrowserView.m_lastMouseActionNode = node;
}

void CBrowserFrame::BrowserFrameGlueObj::PopupBlocked(const char* uri)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
	
	// Do nothing if an icon was set already or if the user
	// don't want popup notification
	if (!theApp.preferences.GetBool("browser.popups.showPopupBlocker", PR_TRUE)
		|| !pThis->m_wndBrowserView.m_csHostPopupBlocked.IsEmpty())
		return;
	
	pThis->m_wndStatusBar.AddIcon(ID_POPUP_BLOCKED_ICON);
	HICON hTmpPopupIcon = 
		(HICON)::LoadImage(AfxGetResourceHandle(),
        MAKEINTRESOURCE(IDI_POPUP_BLOCKED), IMAGE_ICON, 16,16,LR_LOADMAP3DCOLORS);
	
	CString tpText;
	USES_CONVERSION;
	const TCHAR* turi = A2CT(uri);
	
	tpText.Format(IDS_POPUP_BLOCKED, turi);
	pThis->m_wndStatusBar.SetIconInfo(ID_POPUP_BLOCKED_ICON, hTmpPopupIcon, tpText);
	
	pThis->m_wndBrowserView.m_csHostPopupBlocked = turi;
}


