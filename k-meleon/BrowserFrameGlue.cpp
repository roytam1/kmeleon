/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chak Nanga <chak@netscape.com> 
 */

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
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

      nsCString strStatus; 

   if(aMessage)
      strStatus.AssignWithConversion(aMessage);

   pThis->m_wndStatusBar.SetPaneText(0, strStatus.get());
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
      if (!aBusy)
         pThis->PostMessage(UWM_UPDATEBUSYSTATE, 0, 0);
}

// Called from the OnLocationChange() method in the nsIWebProgressListener 
// interface implementation in CBrowserImpl to update the current URI
// Will get called after a URI is successfully loaded in the browser
// We use this info to update the URL bar's edit box
//
void CBrowserFrame::BrowserFrameGlueObj::UpdateCurrentURI(nsIURI *aLocation){
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   if(aLocation) {
      nsCAutoString uriString;
      aLocation->GetSpec(uriString);

      pThis->m_wndUrlBar.SetCurrentURL(uriString.get());
   }
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameTitle(PRUnichar **aTitle)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   CString title;
   pThis->GetWindowText(title);

   CString appTitle;
   appTitle.LoadString(AFX_IDS_APP_TITLE);

   title.Replace(" (" + appTitle + ')', "");

   if(!title.IsEmpty())
   {
      nsString nsTitle;
      nsTitle.AssignWithConversion(title.GetBuffer(0));

      *aTitle = ToNewUnicode(nsTitle);
   }
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameTitle(const PRUnichar *aTitle)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   USES_CONVERSION;

   CString appTitle;
   appTitle.LoadString(AFX_IDS_APP_TITLE);

   CString title = W2T(aTitle);

   if (title.IsEmpty()){
      pThis->m_wndUrlBar.GetEnteredURL(title);
   }

   title += " (" + appTitle + ')';
   pThis->SetWindowText(title);

   pThis->PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameSize(PRInt32 aCX, PRInt32 aCY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
      
   pThis->SetWindowPos(NULL, 0, 0, aCX, aCY, 
      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
      );
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserSize(PRInt32 aCX, PRInt32 aCY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   // first we have to figure out how much bigger the frame is than the view
   RECT frameRect, viewRect;
   pThis->GetWindowRect(&frameRect);
   pThis->m_wndBrowserView.GetClientRect(&viewRect);

   int deltax = (frameRect.right-frameRect.left)-(viewRect.right-viewRect.left);
   int deltay = (frameRect.bottom-frameRect.top)-(viewRect.bottom-viewRect.top);

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

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFramePosition(PRInt32 aX, PRInt32 aY)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)  

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

   pThis->SetWindowPos(NULL, aX, aY, aCX, aCY, 
      SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserPositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

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

   pThis->SetFocus();
}

void CBrowserFrame::BrowserFrameGlueObj::FocusAvailable(PRBool *aFocusAvail)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

      HWND focusWnd = ::GetFocus();

   if ((focusWnd == pThis->m_hWnd) || ::IsChild(pThis->m_hWnd, focusWnd))
      *aFocusAvail = PR_TRUE;
   else
      *aFocusAvail = PR_FALSE;
}

void CBrowserFrame::BrowserFrameGlueObj::ShowBrowserFrame(PRBool aShow)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
      
   if(aShow){
      pThis->ShowWindow(SW_SHOW);
      pThis->SetActiveWindow();
      pThis->UpdateWindow();
   }else{
      pThis->ShowWindow(SW_HIDE);
   }
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameVisibility(PRBool *aVisible)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   // Is the current BrowserFrame the active one?
   if (GetActiveWindow()->m_hWnd != pThis->m_hWnd){
      *aVisible = PR_FALSE;
      return;
   }

   // We're the active one
   //Return FALSE if we're minimized
   WINDOWPLACEMENT wpl;
   pThis->GetWindowPlacement(&wpl);

   if ((wpl.showCmd == SW_RESTORE) || (wpl.showCmd == SW_MAXIMIZE))
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
    nsAutoString empty;
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
            nsCAutoString uri;
            imgURI->GetSpec(uri);
            bContentHasImage = TRUE;

            pThis->m_wndBrowserView.SetCtxMenuImageSrc(NS_ConvertUTF8toUCS2(uri)); // Set the new Img Src
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
        
        nsAutoString strUrlUcs2;
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
            nsCAutoString strImgSrcUtf8;
            imgURI->GetSpec(strImgSrcUtf8);
            if(!strImgSrcUtf8.IsEmpty())
            {
                // Set the new Img Src
                pThis->m_wndBrowserView.SetCtxMenuImageSrc(NS_ConvertUTF8toUCS2(strImgSrcUtf8));
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
        nsCAutoString strImgSrcUtf8;
        imgURI->GetSpec(strImgSrcUtf8);
        if(strImgSrcUtf8.IsEmpty())
            return;

        // Set the new Img Src
        pThis->m_wndBrowserView.SetCtxMenuImageSrc(NS_ConvertUTF8toUCS2(strImgSrcUtf8));
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

        nsAutoString strFrameURL;
        rv = htmlDoc->GetURL(strFrameURL);
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        pThis->m_wndBrowserView.SetCurrentFrameURL(strFrameURL); //Set it to the new URL
    }

BUILD_CTX_MENU:

    char *menuType = _T("<nothing>");
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
      
      int offset = theApp.menus.GetOffset(ctxMenu);
      
      RECT desktopRect;
      SystemParametersInfo(SPI_GETWORKAREA, NULL, &desktopRect, 0);
      if ( (int)(cursorPos.y - offset) < (int)(desktopRect.bottom - (ctxMenu->GetMenuItemCount() * GetSystemMetrics(SM_CYMENUSIZE)) + (GetSystemMetrics(SM_CYEDGE)*2)) ){
         // we only do this if we're not too close to the bottom of the screen
         cursorPos.y -= offset;
         if (cursorPos.y < 0){
            cursorPos.y = 0;
         }
      }
      
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

void CBrowserFrame::BrowserFrameGlueObj::ShowTooltip(PRInt32 x, PRInt32 y, const char *text)
{
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   if (!text) {
      pThis->m_wndToolTip.Hide();
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
      pThis->m_wndUrlBar.MaintainFocus();   
      ::SetFocus(pThis->m_wndUrlBar.m_hwndEdit);
   }
}

void CBrowserFrame::BrowserFrameGlueObj::FocusPrevElement() {
   METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

   nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(pThis->m_wndBrowserView.mWebBrowser));
   if(focus) {
      pThis->m_wndUrlBar.MaintainFocus();
      ::SetFocus(pThis->m_wndUrlBar.m_hwndEdit);
   }
}
