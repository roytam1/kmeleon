/*
*  Copyright (C) 2000 Christophe Thibault
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

// Local Includes

#include "stdafx.h"
#include "MainFrm.h"

#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsIDocShellTreeItem.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsCWebBrowser.h"
#include "nsWidgetsCID.h"

#include "WebBrowserChrome.h"

nsVoidArray WebBrowserChrome::sBrowserList;

WebBrowserChrome::WebBrowserChrome(nativeWindow nWindow, CMainFrame *parent)
{
	NS_INIT_REFCNT();
	mNativeWindow = nWindow;
	parentFrame = parent;
	need_new_hwnd = 0; working = 0;
	mBaseWindow = NULL;
//	sBrowserList.AppendElement(this);
}

WebBrowserChrome::~WebBrowserChrome()
{
//	sBrowserList.RemoveElement(this);
/*	if(mBaseWindow)
	{
		mBaseWindow->Destroy();
	}*/
}

//*****************************************************************************
// WebBrowserChrome::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(WebBrowserChrome)
NS_IMPL_RELEASE(WebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(WebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIURIContentListener)
   NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)  //optional
   NS_INTERFACE_MAP_ENTRY(nsIPrompt)
NS_INTERFACE_MAP_END

//*****************************************************************************
// WebBrowserChrome::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
   return QueryInterface(aIID, aInstancePtr);
}

//*****************************************************************************
// WebBrowserChrome::nsIWebBrowserChrome
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
  nsString toto(aStatus);
	char *str=toto.ToNewCString();
  switch(aType)
  {
    case nsIWebBrowserChrome::STATUS_SCRIPT: parentFrame->onJSStatus(str); break;
    case nsIWebBrowserChrome::STATUS_SCRIPT_DEFAULT: parentFrame->onJSDefaultStatus(str); break;
    case nsIWebBrowserChrome::STATUS_LINK: parentFrame->onOverLink(str); break;
  }
  Recycle(str);
   return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);
   NS_ENSURE_TRUE(mWebBrowser, NS_ERROR_NOT_INITIALIZED);
   *aWebBrowser = mWebBrowser;
   NS_IF_ADDREF(*aWebBrowser);

   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ENSURE_ARG(aWebBrowser);   // Passing nsnull is NOT OK
   NS_ENSURE_TRUE(mWebBrowser, NS_ERROR_NOT_INITIALIZED);
   NS_ERROR("Who be calling me");
   mWebBrowser = aWebBrowser;
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP WebBrowserChrome::CreateBrowserWindow(PRUint32 chromeMask, PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, nsIWebBrowser **aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);
   *aWebBrowser = nsnull;

	 if(need_new_hwnd) {
		 *aWebBrowser = (nsIWebBrowser *)parentFrame->createNewBrowser();
		 return NS_OK;
	 }

  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);

	if (!mWebBrowser)
        return NS_ERROR_FAILURE;

    mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, this));

    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser);
//    dsti->SetItemType(nsIDocShellTreeItem::typeChromeWrapper);
		dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

    mBaseWindow = do_QueryInterface(mWebBrowser);

//    mNativeWindow = CreateNativeWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, this));

    if (!mNativeWindow)
        return NS_ERROR_FAILURE;


    mBaseWindow->InitWindow( mNativeWindow,
                             nsnull, 
                             0, 0, 450, 450);
    mBaseWindow->Create();
    
    NS_IF_ADDREF(*aWebBrowser = mWebBrowser);

		need_new_hwnd = 1;

    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::FindNamedBrowserItem(const PRUnichar* aName,
                                                  	  nsIDocShellTreeItem ** aBrowserItem)
{
    NS_ENSURE_ARG(aName);
    NS_ENSURE_ARG_POINTER(aBrowserItem);
    *aBrowserItem = nsnull;

    if (!mWebBrowser)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebBrowser));
    NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

    return docShellAsItem->FindItemWithName(aName, NS_STATIC_CAST(nsIWebBrowserChrome*, this), aBrowserItem);
}

NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
	parentFrame->onSizeBrowserTo(aCX,aCY);
  return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::ShowAsModal(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
WebBrowserChrome::SetPersistence(PRBool aPersistX, PRBool aPersistY,
                                  PRBool aPersistCX, PRBool aPersistCY,
                                  PRBool aPersistSizeMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
WebBrowserChrome::GetPersistence(PRBool* aPersistX, PRBool* aPersistY,
                                  PRBool* aPersistCX, PRBool* aPersistCY,
                                  PRBool* aPersistSizeMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//*****************************************************************************
// WebBrowserChrome::nsIWebProgressListener
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                                  PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                                  PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
	if (maxTotalProgress > 0)
	{	
		PRUint32 percentage;
    if(curTotalProgress>maxTotalProgress) percentage=100;
    else percentage=(curTotalProgress * 100) / maxTotalProgress;
		parentFrame->onProgressChange(percentage);
  } else
		parentFrame->onProgressChange(0);
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                               PRInt32 progressStateFlags, PRUint32 status)
{
  if (progressStateFlags & nsIWebProgressListener::STATE_START) working=1;
  if (progressStateFlags & nsIWebProgressListener::STATE_STOP) working=0;

  if (progressStateFlags & nsIWebProgressListener::STATE_IS_NETWORK) {
		if (progressStateFlags & nsIWebProgressListener::STATE_START) {
			 if(parentFrame->IsWindowVisible())
				parentFrame->StartAnimation();
		} else if (progressStateFlags & nsIWebProgressListener::STATE_STOP)
      parentFrame->StopAnimation();
   }
    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                                 nsIRequest* aRequest,
                                                 nsIURI *location)
{
	char *buf = nsnull;

	if (location)
		location->GetSpec(&buf);

	parentFrame->SetAddress(buf);

	if (buf)	
    Recycle(buf);

  return NS_OK;
}

NS_IMETHODIMP 
WebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
	nsresult rv;
   nsCOMPtr<nsIChannel> channel;
    channel = do_QueryInterface(aRequest, &rv);
   if (NS_SUCCEEDED(rv)) {
//     mXULBrowserWindow->OnStatus(channel, aStatus, aMessage);
//		 nsString toto(aMessage);
//		 ::MessageBox(NULL,"toto",toto.ToNewUTF8String(),0);
//		 aMessage->ToNewUTF8String();
   }	
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 state)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

//*****************************************************************************
// WebBrowserChrome::nsIBaseWindow
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* parentWidget, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)   
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::Create()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP WebBrowserChrome::Destroy()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP WebBrowserChrome::SetPosition(PRInt32 x, PRInt32 y)
{
    return mBaseWindow->SetPosition(x, y);
}

NS_IMETHODIMP WebBrowserChrome::GetPosition(PRInt32* x, PRInt32* y)
{
    return mBaseWindow->GetPosition(x, y);
}

NS_IMETHODIMP WebBrowserChrome::SetSize(PRInt32 cx, PRInt32 cy, PRBool fRepaint)
{
    return mBaseWindow->SetSize(cx, cy, fRepaint);
}

NS_IMETHODIMP WebBrowserChrome::GetSize(PRInt32* cx, PRInt32* cy)
{
    return mBaseWindow->GetSize(cx, cy);
}

NS_IMETHODIMP WebBrowserChrome::SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy, PRBool fRepaint)
{
    return mBaseWindow->SetPositionAndSize(x, y, cx, cy, fRepaint);
}

NS_IMETHODIMP WebBrowserChrome::GetPositionAndSize(PRInt32* x, PRInt32* y, PRInt32* cx, PRInt32* cy)
{
    return mBaseWindow->GetPositionAndSize(x, y, cx, cy);
}

NS_IMETHODIMP WebBrowserChrome::Repaint(PRBool aForce)
{
   return mBaseWindow->Repaint(aForce);
}

NS_IMETHODIMP WebBrowserChrome::GetParentWidget(nsIWidget** aParentWidget)
{
   NS_ENSURE_ARG_POINTER(aParentWidget);

   NS_ASSERTION(PR_FALSE, "Not Yet Implemented");
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetParentWidget(nsIWidget* aParentWidget)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   NS_ENSURE_ARG_POINTER(aParentNativeWindow);

   *aParentNativeWindow = mNativeWindow;
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   mNativeWindow = aParentNativeWindow;
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool* aVisibility)
{
   return mBaseWindow->GetVisibility(aVisibility);
}

NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{   
   return mBaseWindow->SetVisibility(aVisibility);
}

NS_IMETHODIMP WebBrowserChrome::GetMainWidget(nsIWidget** aMainWidget)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
   return mBaseWindow->SetFocus();
}

NS_IMETHODIMP WebBrowserChrome::FocusAvailable(nsIBaseWindow* aCurrentFocus, 
   PRBool* aTookFocus)
{
   return mBaseWindow->FocusAvailable(aCurrentFocus, aTookFocus);
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);

   *aTitle = nsnull;
   
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar* aTitle)
{
	nsString toto(aTitle);
	char *str=toto.ToNewCString();
	parentFrame->onPageTitleChange(str);
	Recycle(str);
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
  parentFrame->onPopup(aContextFlags);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::Alert(const PRUnichar *dialogTitle, const PRUnichar *text)
{
	nsString title(dialogTitle);
	nsString txt(text);
	char *strTitle=title.ToNewCString();
	char *strTxt=txt.ToNewCString();
	MessageBox(NULL,strTxt,strTitle,MB_ICONWARNING);
	Recycle(strTxt);
	Recycle(strTitle);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::AlertCheck(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue)
{
  MessageBox(NULL,"AlertCheck called","hu?",0);
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::Confirm(const PRUnichar *dialogTitle, const PRUnichar *text, PRBool *_retval)
{
	nsString title(dialogTitle);
	nsString txt(text);
	char *strTitle=title.ToNewCString();
	char *strTxt=txt.ToNewCString();
	*_retval=(MessageBox(NULL,strTxt,strTitle,MB_OKCANCEL|MB_ICONQUESTION)==IDOK);
	Recycle(strTxt);
	Recycle(strTitle);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ConfirmCheck(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::Prompt(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, const PRUnichar *defaultText, PRUnichar **result, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::PromptUsernameAndPassword(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, PRUnichar **user, PRUnichar **pwd, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::PromptPassword(const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *passwordRealm, PRUint32 savePassword, PRUnichar **pwd, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::Select(const PRUnichar *dialogTitle, const PRUnichar *text, PRUint32 count, const PRUnichar **selectList, PRInt32 *outSelection, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::UniversalDialog(const PRUnichar *titleMessage, const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *checkboxMsg, const PRUnichar *button0Text, const PRUnichar *button1Text, const PRUnichar *button2Text, const PRUnichar *button3Text, const PRUnichar *editfield1Msg, const PRUnichar *editfield2Msg, PRUnichar **editfield1Value, PRUnichar **editfield2Value, const PRUnichar *iconURL, PRBool *checkboxState, PRInt32 numberButtons, PRInt32 numberEditfields, PRInt32 editField1Password, PRInt32 *buttonPressed)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//*****************************************************************************
// WebBrowserChrome::nsIURIContentListener
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::OnStartURIOpen(nsIURI *aURI, const char *aWindowTarget, PRBool *aAbortOpen)
{
  MessageBox(NULL,"start uri open","tata",0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetProtocolHandler(nsIURI *aURI, nsIProtocolHandler **aProtocolHandler)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::DoContent(const char *aContentType, nsURILoadCommand aCommand,
					   const char *aWindowTarget, nsIChannel *aOpenedChannel,
					   nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::IsPreferred(const char *aContentType, nsURILoadCommand aCommand,
					     const char *aWindowTarget, char **aDesiredContentType,
					     PRBool *aCanHandleContent)
{
  *aCanHandleContent = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::CanHandleContent(const char *aContentType, nsURILoadCommand aCommand,
						  const char *aWindowTarget, char **aDesiredContentType,
						  PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetLoadCookie(nsISupports * *aLoadCookie)
{
  MessageBox(NULL,"load cookie","tata",0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetLoadCookie(nsISupports * aLoadCookie)
{
  MessageBox(NULL,"set cookie","tata",0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetParentContentListener(nsIURIContentListener * *aParentContentListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
