/*
*  Copyright (C) 2000 Christophe Thibault, Brian Harris, Jeff Doozan
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

/*
  These are little utils and stuff for the CBrowserView
  it's mainly here to get it out of BrowserView.cpp which should
  theoretically just contain overridden functions and message handlers
*/

#include "stdafx.h"
#include <wininet.h>
#include "Utils.h"

#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h" 
#include "nsIDocCharset.h"
#include "nsISelection.h"
#include "nsISHistoryInternal.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMWindowCollection.h"
#include "nsIWebPageDescriptor.h"

#include "nsIMIMEHeaderParam.h"
#include "nsIMIMEService.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIStandardURL.h"
#include "nsIURL.h"
#include "nsIDOMNSDocument.h"

#include "nsISecureBrowserUI.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsICertificateDialogs.h"
#include "nsIX509Cert.h"

#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMHTMLInputElement.h"

#include "UnknownContentTypeHandler.h"
#include "nsCWebBrowserPersist.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserFrm.h"
#include "BrowserView.h"

extern nsresult NewURI(nsIURI **result, const nsAString &spec);

// Remove when it stops sucking.
//#define MOZILLA_MIMETYPE_SUCKS
#ifdef MOZILLA_MIMETYPE_SUCKS
extern BOOL GetFromTypeAndExtension(LPCTSTR contentType, LPCTSTR ext, CString& resultExt, CString& desc);
#endif

BOOL CBrowserView::IsViewSourceUrl(CString& strUrl)
{
    return (strUrl.Find(_T("view-source:"), 0) != -1) ? TRUE : FALSE;
}

BOOL CBrowserView::OpenViewSourceWindow(const PRUnichar* pUrl)
{
    nsEmbedString wUrl( pUrl );
	nsEmbedCString cUrl;
	NS_UTF16ToCString(wUrl,NS_CSTRING_ENCODING_ASCII,cUrl);
    return OpenViewSourceWindow(cUrl.get());
}

void OpenFileExternal(char* uri, TCHAR* file, nsresult status, void* param)
{
	LPTSTR viewer = (TCHAR*)param;
	if (NS_SUCCEEDED(status))
	{
		TCHAR *command = new TCHAR[_tcsclen(viewer) + _tcsclen(file) +4];
      
		_tcscpy(command, viewer);
		_tcscat(command, _T(" \""));                              //append " filename" to the viewer command
		_tcscat(command, file);
		_tcscat(command, _T("\""));
      
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi;
		si.cb = sizeof STARTUPINFO;
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOW;
      
		CreateProcess(0,command,0,0,0,0,0,0,&si,&pi);      // launch external viewer
			
		delete command;
	}
	free(viewer);
	// Have to show an error message
}

BOOL CBrowserView::OpenViewSourceWindow(const char* pUrl)
{
    // Use external viewer
    if (theApp.preferences.bSourceUseExternalCommand) {
	if (theApp.preferences.sourceCommand) {

	    CString tempfile;
	    tempfile = GetTempFile();

	    char *url = strdup(pUrl);
         
	    if (url && strnicmp(url, "view-source:file:///", 20) == 0) {
			unsigned int i;
			for (i=0; i<strlen(url); i++)
				if (url[i]=='/')
					url[i]='\\';
			tempfile = nsUnescape(url+strlen("view-source:file:///"));
			OpenFileExternal("", tempfile.GetBuffer(0), NS_OK,
				_tcsdup(theApp.preferences.sourceCommand.GetBuffer(0)));
			return TRUE;
	    }
	    
	    nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
	    if(persist)
	    {
			nsresult rv;

			nsCOMPtr<nsIWebPageDescriptor> descriptor;
			nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
			if (docShell)
				descriptor = do_QueryInterface(docShell);

			nsCOMPtr<nsIURI> referrer;
			mWebNav->GetReferringURI(getter_AddRefs(referrer));
		
			nsCOMPtr<nsILocalFile> file;
#ifdef _UNICODE
			rv = NS_NewLocalFile(nsDependentString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#else
			rv = NS_NewNativeLocalFile(nsDependentCString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#endif
			nsEmbedString sURI;
			NS_CStringToUTF16(nsDependentCString(pUrl+strlen("View-Source:")), NS_CSTRING_ENCODING_UTF8, sURI);

			nsCOMPtr<nsIURI> srcURI;
			rv = NewURI(getter_AddRefs(srcURI), sURI);
			if (NS_FAILED(rv)) {
			    if (url) delete url;
			    return FALSE;
			}

			CProgressDialog *progress = new CProgressDialog(FALSE);      
			progress->SetCallBack(OpenFileExternal, 
				 _tcsdup(theApp.preferences.sourceCommand.GetBuffer(0)));
			progress->InitPersist(srcURI, file, persist, FALSE);
//			progress->InitViewer(persist, theApp.preferences.sourceCommand.GetBuffer(0), tempfile.GetBuffer(0));
			persist->SetPersistFlags(
				nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
				nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);
			rv = persist->SaveURI(srcURI, descriptor, referrer, nsnull, nsnull, file);
			if (NS_FAILED(rv))
				persist->SetProgressListener(nsnull);

	    }
	    if (url)
		delete url;
	    return TRUE;
	}
    }
   
    // use the internal viewer

    // Create a new browser frame in which we'll show the document source
    // Note that we're getting rid of the toolbars etc. by specifying
    // the appropriate chromeFlags
    PRUint32 chromeFlags =  nsIWebBrowserChrome::CHROME_WINDOW_BORDERS |
	                    nsIWebBrowserChrome::CHROME_TITLEBAR |
	                    nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                       nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                       nsIWebBrowserChrome::CHROME_WINDOW_MIN;

    RECT screen;
    SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);

    int screenWidth   = screen.right - screen.left;
    int screenHeight  = screen.bottom - screen.top;

    int x = screen.left + screenWidth / 20;
    int y = screen.top + screenHeight / 20;
    int w = 15*screenWidth / 20;
    int h = 18*screenHeight/20;

    CBrowserFrame* pFrm = CreateNewBrowserFrame(chromeFlags, x, y, w, h);
    if(!pFrm)
	return FALSE;

    // Finally, load this URI into the newly created frame
    pFrm->m_wndBrowserView.OpenURL(pUrl);
    
    pFrm->BringWindowToTop();

    return TRUE;
}

LRESULT CBrowserView::RefreshToolBarItem(WPARAM ItemID, LPARAM unused)
{
	switch (ItemID) {
		case ID_NAV_BACK:
			m_refreshBackButton = TRUE;
			break;
		case ID_NAV_FORWARD:
			m_refreshForwardButton = TRUE;
			break;
	}

	return 0;
}

void CBrowserView::GetPageTitle(CString& title)
{
   USES_CONVERSION;
   PRUnichar *aTitle;
   //mpBrowserFrameGlue->GetBrowserFrameTitle(&aTitle);
   if (mBaseWindow) {
     mBaseWindow->GetTitle(&aTitle);
     title = W2T(aTitle);
     nsMemory::Free(aTitle);
  }
}
/*
BOOL MultiSave(nsIURI* aURI, nsILocalFile* file) {
   nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
   if(!persist)
      return FALSE;
    
   CProgressDialog *progress = new CProgressDialog(FALSE);
   progress->InitPersist(aURI, file, persist, TRUE);
   persist->SaveURI(aURI, nsnull, nsnull, nsnull, nsnull, file);
   return TRUE;
}*/



NS_IMPL_ISUPPORTS1(CSaveAsHandler, nsIWebProgressListener);

CSaveAsHandler::CSaveAsHandler(
		nsIWebBrowserPersist* aPersist, nsIFile* aFile, 
		nsIURI* aURL, nsIDOMDocument* aDocument, 
		nsISupports* aDescriptor, nsIURI* aReferrer, CBrowserFrame* aBrowser)
{
	mPersist = aPersist;
	mDescriptor = aDescriptor;
	mFile = aFile;
	mURL = aURL;
	mDocument = aDocument;
	mReferrer = aReferrer;
	mBrowser = aBrowser;
}

CSaveAsHandler::~CSaveAsHandler() {}

NS_IMETHODIMP CSaveAsHandler::OnStateChange(nsIWebProgress *aWebProgress, 
											nsIRequest *aRequest, PRUint32 aStateFlags, 
											nsresult aStatus)
{
	nsresult rv = NS_OK;

	if (aStateFlags & nsIWebProgressListener::STATE_START)
	{
		nsCOMPtr<nsIWebProgressListener> KeepAlive(this);

		nsEmbedCString sContentType;

		nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
		if (!channel) return rv;

		channel->GetContentType(sContentType);

		channel->GetURI(getter_AddRefs(mRealURI));
		
		nsCOMPtr<nsIHttpChannel> httpchannel(do_QueryInterface(channel));
		if (httpchannel) {
			PRBool b;
			rv = httpchannel->GetRequestSucceeded(&b);
			// If we get an error, remove the content type since it's html and
			// the download will fail
			if (NS_SUCCEEDED(rv) && b == PR_FALSE)	{
				sContentType.SetLength(0);
			}
			httpchannel->GetResponseHeader(nsEmbedCString("content-disposition"), mContentDisposition);
		}
		mPersist->CancelSave();
		mFile->Remove(PR_FALSE);

		// Defering the save as dialog to not stall other transferts
		theApp.m_pMainWnd->PostMessage(WM_DEFERSAVEAS, (WPARAM)this, (LPARAM)strdup(sContentType.get()));
		NS_ADDREF_THIS();
		//rv = Save(sContentType.get());
	}
	return rv;
}

NS_IMETHODIMP CSaveAsHandler::Save(const char* contentType, const char* disposition)
{
	NS_ENSURE_ARG(contentType);
	NS_ENSURE_ARG(mURL || mRealURI);
	if (!mRealURI) mRealURI = mURL;

	if (disposition && *disposition)
		mContentDisposition.Assign(disposition);

	nsresult rv;
	nsEmbedString fileName;
	
	PRBool isHTML = 
		(strcmp(contentType, "text/html") == 0) ||
		(strcmp(contentType, "text/xml") == 0) ||
		(strcmp(contentType, "application/xhtml+xml") == 0) ;

	if (mContentDisposition.Length())
	{
		/* 1 Use the HTTP header suggestion. */
		nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar = do_GetService("@mozilla.org/network/mime-hdrparam;1");  
		if (mimehdrpar)
		{
			nsEmbedCString fallbackCharset;
			if (mRealURI) mRealURI->GetOriginCharset(fallbackCharset);

			rv = mimehdrpar->GetParameter (mContentDisposition, 
				"filename",
				fallbackCharset, PR_TRUE, nsnull,
				fileName);
			if (NS_FAILED(rv) || !fileName.Length() || wcsicmp(fileName.get(), L"untitled") == 0)
			{
				rv = mimehdrpar->GetParameter (mContentDisposition, "name",
					fallbackCharset, PR_TRUE, nsnull,
					fileName);
			}
		}
	}

	if ( (!fileName.Length() || wcsicmp(fileName.get(), L"untitled") == 0) && mDocument && isHTML)
	{
		/* Use the title of the document. */
		nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
		if (htmlDoc) htmlDoc->GetTitle(fileName);

		PRUnichar* wc = (PRUnichar*)fileName.get();
		for (;*wc;wc++) 
			if (*wc == L'.') *wc = L'_';
	}

	if (!fileName.Length() || (mDocument && isHTML && !theApp.preferences.bSaveUseTitle))
	{
		/* For file URLs, use the file name. */
		nsCOMPtr<nsIURL> url(do_QueryInterface(mRealURI));
		
		if (!url)
		{
			// In some case (wyciwyg) there is no nsIURL, so I'm creating one
			nsCOMPtr<nsIStandardURL> test = do_CreateInstance("@mozilla.org/network/standard-url;1");
			
			nsEmbedCString spec;
			mRealURI->GetSpec(spec);

			nsEmbedCString charset;
			mRealURI->GetOriginCharset(charset);

			test->Init(nsIStandardURL::URLTYPE_STANDARD, 80, spec, charset.get(), nsnull);
			url  = do_QueryInterface(test);
		}
		
		if (url)
		{
			nsEmbedCString CfileName;
			url->GetFileName(CfileName);
			char * ufn = nsUnescape(strdup(CfileName.get()));
			CfileName = ufn;
			free(ufn);

			NS_CStringToUTF16 (CfileName, NS_CSTRING_ENCODING_UTF8, fileName);
			//XXX for some reason it may not be in UTF8
			if (CfileName.Length() && !fileName.Length())
				NS_CStringToUTF16 (CfileName, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM, fileName);
		}
	}

	if (!fileName.Length() && mRealURI)
	{
		/* Use the host. */
		nsEmbedCString hostName;
		mURL->GetHost(hostName);
		NS_CStringToUTF16 (hostName, NS_CSTRING_ENCODING_UTF8, fileName);

		PRUnichar* wc = (PRUnichar*)fileName.get();
		for (;*wc;wc++) 
			if (*wc == L'.') *wc = L'_';
	}

	if (!fileName.Length())
		fileName = L"Untitled";

	CString extension; 
	CString description;

	USES_CONVERSION;
	const WCHAR* pExtension = wcsrchr(fileName.get(), L'.');

#ifdef MOZILLA_MIMETYPE_SUCKS
	GetFromTypeAndExtension(A2CT(contentType), W2CT(pExtension), extension, description);
	if (extension.GetLength())
		extension = extension.Mid(1);
#else
	nsCOMPtr<nsIMIMEService> MimeService = do_GetService("@mozilla.org/uriloader/external-protocol-service;1");
	if (MimeService)
	{
		nsCOMPtr<nsIMIMEInfo> mimeInfo;

		nsEmbedCString typeFromExt;
		
		MimeService->GetTypeFromExtension(
			pExtension ? nsDependentCString(W2CA(pExtension+1)) : NS_LITERAL_CSTRING(""),
			typeFromExt);

		// If the extension is unknow our better choice for now is to 
		// ignore the content type.

		//if (typeFromExt.Length())
		{
			// If both content type and extension are empty the mime type
			// editor return windows media file ?????????
			if (!strlen(contentType) && !pExtension)
				rv = MimeService->GetFromTypeAndExtension(
					NS_LITERAL_CSTRING("text/html"), 
					NS_LITERAL_CSTRING(""),
					getter_AddRefs(mimeInfo));

			else
				rv = MimeService->GetFromTypeAndExtension(
					nsDependentCString(contentType), 
					pExtension ? nsDependentCString(W2CA(pExtension+1)) : NS_LITERAL_CSTRING(""),
					getter_AddRefs(mimeInfo));

			if (NS_SUCCEEDED(rv) && mimeInfo) 
			{
				//PRBool extMatch = PR_TRUE;
				//if (pExtension)
				//	mimeInfo->ExtensionExists(W2CA(pExtension+1), &extMatch);

				//if (extMatch)
				//{
					nsEmbedCString nsCExt;
					mimeInfo->GetPrimaryExtension(nsCExt);

					nsEmbedString nsExt;
					NS_CStringToUTF16(nsCExt, NS_CSTRING_ENCODING_UTF8, nsExt);
					extension = W2CT(nsExt.get());

					nsEmbedString nsDesc;
					mimeInfo->GetDescription(nsDesc);
					description = W2CT(nsDesc.get());
				//}
			}
		}
	}

	if(extension.IsEmpty() && pExtension)
		extension = W2CT(pExtension + 1);
#endif

	TCHAR* szFileName = new TCHAR[MAX_PATH+1];
	_tcsncpy(szFileName, W2CT(fileName.get()), MAX_PATH);
	szFileName[MAX_PATH] = 0;
	MakeFilename(szFileName);

	//["@mozilla.org/intl/texttosuburi;1"]

	CString filter;

	if (isHTML && mDocument) {
		filter.LoadString(IDS_WEBPAGE_HTMLONLY);
		filter += _T(" (*.htm;*.html;*.xhtml)|*.htm;*.html;*.xhtml|");
		CString complete;
		complete.LoadString(IDS_WEBPAGE_COMPLETE);
		filter += complete + _T(" (*.htm;*.html;*.xhtml)|*.htm;*.html;*.xhtml|");
	}
	else
	{
		if (!description.IsEmpty())
			filter = description + _T(" (*.") + extension + _T(")|*.") + extension + _T("|");
		else if (!extension.IsEmpty())
			filter = extension + _T(" Files (*.") + extension + _T(")|*.") + extension + _T("|");
	}

	CString filt;
	filt.LoadString(IDS_ALLFILES);
	filter += filt + _T(" (*.*)|*.*||");

	TCHAR* lpszFilter = filter.GetBuffer(0);
	for (int i=0; lpszFilter[i]; i++)
		if (lpszFilter[i] == _T('|'))
			lpszFilter[i] = _T('\0');

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = lpszFilter;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir = theApp.preferences.saveDir;
	ofn.lpstrDefExt = extension;
	ofn.nFilterIndex = (mDocument && isHTML) ? theApp.preferences.iSaveType == 2 ? 2 : 1 : 1;

	rv = NS_ERROR_ABORT;
	if( ::GetSaveFileName(&ofn) )
	{
		CString strFullPath = szFileName;

		theApp.preferences.saveDir = strFullPath;
		int idxSlash;
		idxSlash = theApp.preferences.saveDir.ReverseFind(_T('\\'));
		theApp.preferences.saveDir = theApp.preferences.saveDir.Mid(0, idxSlash+1);

		nsCOMPtr<nsILocalFile> file;
#ifdef _UNICODE
		rv = NS_NewLocalFile(nsDependentString(strFullPath.GetBuffer(0)), TRUE, getter_AddRefs(file));
#else
		rv = NS_NewNativeLocalFile(nsDependentCString(strFullPath.GetBuffer(0)), TRUE, getter_AddRefs(file));
#endif
		NS_ENSURE_SUCCESS(rv,rv);

		// Get the persist interface that we'll use for saving the file
		nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID, &rv));
		NS_ENSURE_SUCCESS (rv, rv);

		if (!mDocument)
		{
			// Add flag for content conversion (Bug #687)
			persist->SetPersistFlags(
			nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
			nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);

			// Create the progress dialog and initiate the download
			CProgressDialog *progress = new CProgressDialog(FALSE);
			progress->InitPersist(mRealURI, file, persist, TRUE);
			//persist->SetProgressListener(progress);
			rv = persist->SaveURI(mRealURI, nsnull, mReferrer, nsnull, nsnull, file);
			if (NS_FAILED(rv)) { //Remove cycling reference, avoid leaking
				persist->SetProgressListener(nsnull);
				progress->Close();
			}
		}
		else
		{
			if (isHTML) 
			{
				theApp.preferences.iSaveType = ofn.nFilterIndex;
				if (ofn.nFilterIndex == 2) 
				{  
					// cf.m_ofn.nFilterIndex == 3 indicates
					// user want to save the complete document including
					// all frames, images, scripts, stylesheets etc.

					CString strDataPath;

					int idxPeriod = strFullPath.ReverseFind(_T('.'));
					strDataPath = strFullPath.Mid(0, idxPeriod);
					strDataPath += _T("_files");

					// At this stage strDataPath will be of the form
					// c:\tmp\junk_files - assuming we're saving to a file
					// named junk.htm
					// Any images etc in the doc will be saved to a dir
					// with the name c:\tmp\junk_files

					nsCOMPtr<nsILocalFile> dataPath;
#ifdef _UNICODE
					rv = NS_NewLocalFile(nsDependentString(strDataPath), TRUE, getter_AddRefs(dataPath));
#else
					rv = NS_NewNativeLocalFile(nsDependentCString(strDataPath), TRUE, getter_AddRefs(dataPath));
#endif
					NS_ENSURE_SUCCESS(rv,rv);

					rv = persist->SaveDocument(mDocument, file, dataPath, contentType, 0, 0);
				}
				else
					rv = persist->SaveDocument(mDocument, file, nsnull, contentType, 0, 0);
			}
			else
				rv = persist->SaveURI(mURL, mDescriptor, mReferrer, nsnull, nsnull, file);
		}
	}

	filter.ReleaseBuffer();
	delete szFileName;

	return rv;
}

NS_IMETHODIMP CSaveAsHandler::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
	return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP CSaveAsHandler::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
   return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP CSaveAsHandler::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
	return NS_OK;
}

NS_IMETHODIMP CSaveAsHandler::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
   return NS_OK;
}



BOOL CBrowserView::URISaveAs(PRUnichar* aURI , int bDocument)
{
	// Try to get the file name part from the URL
    // To do that we first construct an obj which supports 
    // nsIRUI interface. Makes it easy to extract portions
    // of a URL like the filename, scheme etc. + We'll also
    // use it while saving this link to a file
    nsresult rv;
    nsCOMPtr<nsIURI> linkURI;
    rv = NewURI(getter_AddRefs(linkURI), nsDependentString(aURI));
    NS_ENSURE_SUCCESS(rv, FALSE);

	return URISaveAs(linkURI, bDocument);
}

BOOL CBrowserView::URISaveAs(nsIURI* aURI , int bDocument)
{
	// NEED better error handling 

	nsresult rv;

	nsCOMPtr<nsIWebPageDescriptor> descriptor;

	nsCOMPtr<nsIDOMDocument> document;
	nsCOMPtr<nsIURI> referrer;
	nsEmbedCString contentType;
	BOOL isHTML = FALSE;

	if (bDocument > 0) // Save Page As 
	{	

		//TODO: Test for XUL

		nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser, &rv);
		NS_ENSURE_SUCCESS(rv, FALSE);

		descriptor = do_QueryInterface(docShell);
		//NS_ENSURE_TRUE (descriptor, NS_ERROR_FAILURE);

		mWebNav->GetReferringURI(getter_AddRefs(referrer));

		nsCOMPtr<nsIWebBrowserPersist> browserPersist = do_QueryInterface(mWebBrowser);
		PRUint32 currentState;
		browserPersist->GetCurrentState(&currentState);
		if (currentState == nsIWebBrowserPersist::PERSIST_STATE_SAVING) {
			AfxMessageBox(IDS_NOT_FINISHED_LOADING, MB_OK);
			return FALSE;
		}


		if (bDocument == 2 && m_lastMouseActionNode) // Save Frame As
		{
			rv = m_lastMouseActionNode->GetOwnerDocument(getter_AddRefs(document));
		}
		else
		{
			nsCOMPtr<nsIDOMWindow> dom;
			rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
			NS_ENSURE_SUCCESS(rv, FALSE);

			rv = dom->GetDocument(getter_AddRefs(document));
		}

		NS_ENSURE_SUCCESS(rv, FALSE);
			
		nsCOMPtr<nsIDOMNSDocument> nsdoc = do_QueryInterface(document);
		NS_ENSURE_TRUE (nsdoc, NULL);

		nsEmbedString type;
		rv = nsdoc->GetContentType(type);
		NS_UTF16ToCString(type, NS_CSTRING_ENCODING_ASCII, contentType);

		const char* ctype = contentType.get();
		isHTML = 
		  (strcmp(ctype, "text/html") == 0) ||
		  (strcmp(ctype, "text/xml") == 0) ||
		  (strcmp(ctype, "application/xhtml+xml") == 0) ;

		if (mbDocumentLoading && !mbDOMLoaded && isHTML
			&& !theApp.preferences.GetBool("kmeleon.download.allowIncompleteSave", FALSE))
		{
			// Needed or the document will not be saved complete
			CString msg;
			msg.LoadString(IDS_NOT_FINISHED_LOADING);
			MessageBox(msg, _T(""), MB_OK|MB_ICONEXCLAMATION);
			return TRUE;
		}
	}
	else // Save Link AS, Save Image As
	{
		mWebNav->GetCurrentURI(getter_AddRefs(referrer));
	}

    CString tempfile;
	tempfile = GetTempFile();

	nsCOMPtr<nsILocalFile> file;
#ifdef _UNICODE
	NS_NewLocalFile(nsDependentString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#else
	NS_NewNativeLocalFile(nsDependentCString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#endif
	
	nsCOMPtr<nsIWebBrowserPersist> persist = do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID);
	if (!persist) return FALSE;

	CSaveAsHandler* handler = new CSaveAsHandler(persist, file, aURI, document, descriptor, referrer, mpBrowserFrame);
	
	if (isHTML || 
		theApp.preferences.GetBool("kmeleon.download.disableContentSniffingOnSave", false)) 
	{
		rv = handler->Save(contentType.get());
		delete handler;
	}else{
		persist->SetProgressListener(handler);
		rv = persist->SaveURI(aURI, descriptor, referrer, nsnull, nsnull, file);
		if (NS_FAILED(rv))
			persist->SetProgressListener(nsnull);
	}
	
	return (NS_SUCCEEDED(rv) || rv == NS_ERROR_ABORT);
}
/*
NS_IMETHODIMP CBrowserView::URISaveAs(nsIURI* aURI, bool bDocument)
{

   NS_ENSURE_ARG_POINTER(aURI);

	// Get the "path" portion (see nsIURI.h for more info
	// on various parts of a URI)
	nsEmbedCString path;
	aURI->GetPath(path);

   char sDefault[] = "default.htm";
   char *pFileName = sDefault;
   char *pBuf = NULL;

   if (strlen(path.get()) > 1) {
	   // The path may have the "/" char in it - strip those
	   pBuf = new char[strlen(path.get()) + 5];      // +5 for ".htm" to be safely appended, if necessary
      strcpy(pBuf, path.get());
      nsUnescape(pBuf);
	   char* slash = strrchr(pBuf, '/');

      if (slash) {
         if (strlen(slash) > 1)
            pFileName = slash+1;                                  // filename = file.ext
         else {
            while ((slash > pBuf) && (strlen(slash) <= 1)) {      // strip off extra /es
               *slash = 0;
               slash--;
   	         slash = strrchr(pBuf, '/');
            }
            if (slash && (strlen(slash) > 0)) {
               pFileName=slash+1;                                 // filename = directory.htm
               strcat(pFileName, ".htm");
            }
         }
      }
      else {
         // if there is no slash, then it's probably an invalid url (javascript: link, etc)
         MessageBox((CString)("Cannot Save URL ") + path.get());
         return NS_ERROR_FAILURE;
      }
   }
   else {
   	aURI->GetHost(path);
      if (strlen(path.get()) >= 1) {
   	   pBuf = new char[strlen(path.get()) + 5];  // +5 for ".htm" to be safely appended, if necessary
         strcpy(pBuf, path.get());
         pFileName = pBuf;
         for (int x=strlen(pBuf)-1; x>=0; x--)
            if (pBuf[x] == '.') pBuf[x] = '_';
         strcat(pBuf, ".htm");                     // filename = www_host_com.htm
      }
   }

   // This is so saving cgi-scripts doesn't produce invalid filenames
   // but create empty string with url beginning with '?' ...
   char *questionMark = strchr(pFileName, '?');
   if (questionMark)
      *questionMark = 0;

   char *extension = strrchr(pFileName, '.');
   if (!extension) {
      extension = strrchr(sDefault, '.');
      strcat(pFileName, extension);
   }
   extension++;

   char lpszFilter[256];
      strcpy(lpszFilter, extension);
      strcat(lpszFilter, " Files (*.");
      strcat(lpszFilter, extension);
      strcat(lpszFilter, ")|*.");
      strcat(lpszFilter, extension);
      strcat(lpszFilter, "|");
      strcat(lpszFilter, "Web Page, HTML Only (*.htm;*.html)|*.htm;*.html|");
      if (bDocument)
        strcat(lpszFilter, "Web Page, Complete (*.htm;*.html)|*.htm;*.html|");
      // strcat(lpszFilter,"Text File (*.txt)|*.txt|");
      strcat(lpszFilter,"All Files (*.*)|*.*||");

   for (int i=0; lpszFilter[i]; ) {
     if (lpszFilter[i] == '|')
       lpszFilter[i] = '\0';
     i++;
   }

   nsEmbedString fileName;
   GetBrowserWindowTitle(fileName); // Suggest the window title as the filename
   USES_CONVERSION;

   OPENFILENAME ofn;
   char *szFileName = new char[INTERNET_MAX_URL_LENGTH];

   memset(&ofn, 0, sizeof(ofn));
   memset(szFileName, 0, INTERNET_MAX_URL_LENGTH);
   strcat(szFileName, pFileName);
   ofn.lStructSize = sizeof(ofn);
   ofn.lpstrFilter = lpszFilter;
   ofn.lpstrFile = szFileName;
   if (bDocument && strstr(extension, "htm")) {
     const char *pszTitle = W2CT(fileName.get());
     if (theApp.preferences.iSaveType == 3 && pszTitle && *pszTitle)
       strcpy(szFileName, pszTitle);
     ofn.nFilterIndex = theApp.preferences.iSaveType == 3 ? 3 : 2;
   }
   ofn.nMaxFile = INTERNET_MAX_URL_LENGTH;
   ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
   ofn.lpstrInitialDir = theApp.preferences.saveDir;
   ofn.lpstrDefExt = extension;

    if( ::GetSaveFileName(&ofn) ) {
      CString strFullPath = szFileName;
      delete szFileName;
      szFileName = NULL;

      theApp.preferences.saveDir = strFullPath;
      int idxSlash;
      idxSlash = theApp.preferences.saveDir.ReverseFind('\\');
      theApp.preferences.saveDir = theApp.preferences.saveDir.Mid(0, idxSlash+1);

      BOOL bSaveAll = FALSE;
      CString strDataPath;
      char *pStrDataPath = NULL;
      if (bDocument && strstr(extension, "htm"))
	theApp.preferences.iSaveType = ofn.nFilterIndex;
      if(bDocument && ofn.nFilterIndex == 3) {
         // cf.m_ofn.nFilterIndex == 3 indicates
         // user want to save the complete document including
         // all frames, images, scripts, stylesheets etc.

         bSaveAll = TRUE;

         int idxPeriod = strFullPath.ReverseFind('.');
         strDataPath = strFullPath.Mid(0, idxPeriod);
         strDataPath += "_files";

         // At this stage strDataPath will be of the form
         // c:\tmp\junk_files - assuming we're saving to a file
         // named junk.htm
         // Any images etc in the doc will be saved to a dir
         // with the name c:\tmp\junk_files

         pStrDataPath = strDataPath.GetBuffer(0); // Get char * for later use
      }


      // Get the persist interface that we'll use for saving the file(s)
      nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
      if(!persist)
         return NS_ERROR_FAILURE;

      //nsString filename;
      //filename.AssignWithConversion(strFullPath.GetBuffer(0));

      nsCOMPtr<nsILocalFile> file;
      NS_NewNativeLocalFile(nsDependentCString(T2A(strFullPath.GetBuffer(0))), TRUE, getter_AddRefs(file));

      nsCOMPtr<nsILocalFile> dataPath;
      if (pStrDataPath)
         NS_NewNativeLocalFile(nsDependentCString(pStrDataPath), TRUE, getter_AddRefs(dataPath));

      if(!bDocument) {
         PRUint32 currentState;
         persist->GetCurrentState(&currentState);
         if (currentState != nsIWebBrowserPersist::PERSIST_STATE_FINISHED) {
            // Now we save the file using a throw away persist if we are already saving...
            if (MultiSave(aURI, file) == TRUE) {
               if (pBuf) delete pBuf;
                  return NS_OK;
            }
            else
               return NS_ERROR_FAILURE;
	 }

         CProgressDialog *progress = new CProgressDialog(FALSE);
      
         progress->InitPersist(aURI, file, persist, TRUE);

         persist->SaveURI(aURI, nsnull, nsnull, nsnull, nsnull, file);
      }
      else if(bSaveAll)
         persist->SaveDocument(nsnull, file, dataPath, nsnull, 0, 0);
      else
         persist->SaveURI(aURI, nsnull, nsnull, nsnull, nsnull, file);
   }

   if (pBuf)
      delete pBuf;
   if (szFileName)
      delete szFileName;

   return NS_OK;
}
*/
void CBrowserView::OpenURL(const char* pUrl, nsIURI *refURI)
{
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    OpenURL(str.get(), refURI);
}

void CBrowserView::OpenURL(const PRUnichar* pUrl, nsIURI *refURI)
{
   USES_CONVERSION;
   mpBrowserFrame->m_wndUrlBar.SetCurrentURL(W2CT(pUrl));
   mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);
  
   if ( GetActiveWindow() == mpBrowserFrame &&
	   !::IsChild(m_hWnd, ::GetFocus()))
	   Activate(TRUE);

   if(mWebNav)
       mWebNav->LoadURI(pUrl,                          // URI string
                    nsIWebNavigation::LOAD_FLAGS_NONE, // Load flags
                    refURI,                            // Refering URI
                    nsnull,                            // Post data
                    nsnull);                           // Extra headers
}

CBrowserFrame* CBrowserView::CreateNewBrowserFrame(PRUint32 chromeMask, 
				    PRInt32 x, PRInt32 y, 
				    PRInt32 cx, PRInt32 cy,
				    PRBool bShowWindow)
{  
    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
    if(!pApp)
	return NULL;

    return pApp->CreateNewBrowserFrame(chromeMask, x, y, cx, cy, bShowWindow);
}

CBrowserFrame* CBrowserView::OpenURLInNewWindow(const char* pUrl, BOOL bBackground, nsIURI *refURI )
{
	nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_UTF8, str);
    return OpenURLInNewWindow(str.get(), bBackground, refURI);
}

CBrowserFrame* CBrowserView::OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground, nsIURI *refURI)
{
    if(!pUrl)
        return NULL; 

    PRUnichar* ext = wcsrchr(pUrl, L'.');
	CBrowserFrame* pFrm;
	PRUint32 chromeFlags;


    if ( !bBackground && ext && 
         (wcsncmp(pUrl, L"chrome:", 7) == 0) &&
         (wcsstr(ext, L".xul") == ext) )
	   chromeFlags = nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                     nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                     nsIWebBrowserChrome::CHROME_TITLEBAR |
                     nsIWebBrowserChrome::CHROME_OPENAS_CHROME|
                     nsIWebBrowserChrome::CHROME_WINDOW_MIN;
	else
    	chromeFlags = nsIWebBrowserChrome::CHROME_ALL;

	// create hidden window
	pFrm = CreateNewBrowserFrame(chromeFlags, -1, -1, -1, -1, PR_FALSE);

    if(!pFrm)
	return NULL;

    // Load the URL into it...

    // Note that OpenURL() is overloaded - one takes a "char *"
    // and the other a "PRUniChar *". We're using the "PRUnichar *"
    // version here 

    pFrm->m_wndBrowserView.OpenURL(pUrl, refURI);

   /* Show the window minimized, instead of on the bottom, because mozilla freaks out if we put it on the bottom */
   /* As of Oct 30, 2002, this seems to be working again.  Good. */  

   /* Reverting to open minimized again hoping the statusbar reappears */
   /* If the window is not maximized, and is opened on the bottom, the statusbar does not get drawn */

    if (bBackground) {
	//if (theApp.preferences.bMaximized)
	    pFrm->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	/*else  
	    pFrm->ShowWindow(SW_MINIMIZEDNA);
		pFrm->SetBackgroundWindow();*/
    }
   
    // show the window
    else if (!(chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
    {
		pFrm->ShowWindow(SW_SHOW);
        pFrm->SetFocus();
	}

	return pFrm;
}

BOOL CBrowserView::CloneSHistory(CBrowserView& newWebBrowser)
{
	nsCOMPtr<nsISHistory> oldSH;
	nsCOMPtr<nsISHistory> newSH;
		
	mWebNav->GetSessionHistory(getter_AddRefs(oldSH));
	newWebBrowser.mWebNav->GetSessionHistory(getter_AddRefs(newSH));
	
	nsCOMPtr<nsISHistoryInternal> newSHInternal(do_QueryInterface(newSH));
	if (newSHInternal)
	{
		nsCOMPtr<nsISHEntry> she;
		nsCOMPtr<nsIHistoryEntry> he;

		PRInt32 count;
		oldSH->GetCount(&count);
		for (int i=0;i<count;i++)
		{
			oldSH->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs(he));

			she = do_QueryInterface(he);
			if (she) {
				nsCOMPtr<nsISHEntry> newSHEntry;
				she->Clone(getter_AddRefs(newSHEntry));
				if (newSHEntry) newSHInternal->AddEntry(newSHEntry, PR_TRUE);
			}
		}

		if (count>0)
		{
			PRInt32 index;
			oldSH->GetIndex(&index);
			newWebBrowser.mWebNav->GotoIndex(index);
		}
		else
			newWebBrowser.OpenURL("about:blank");
	}
	else
		return false;

	return true;
}

void CBrowserView::LoadHomePage()
{
   if (theApp.preferences.bStartHome)
      OnNavHome();
   else
      OpenURL("about:blank");
}

// Called from the busy state related methods in the 
// BrowserFrameGlue object
//
// When aBusy is TRUE it means that browser is busy loading a URL
// When aBusy is FALSE, it's done loading
// We use this to update our STOP tool bar button
//
// We basically note this state into a member variable
// The actual toolbar state will be updated in response to the
// ON_UPDATE_COMMAND_UI method - OnUpdateNavStop() being called
//
void CBrowserView::UpdateBusyState(PRBool aBusy)
{
    if (mbDocumentLoading && !aBusy && m_InPrintPreview) 
    {
	nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
	if(print) {
	    PRBool isDoingPP;
	    print->GetDoingPrintPreview(&isDoingPP);
	    if (!isDoingPP) 
	    {
		m_InPrintPreview = FALSE;
		CMenu* menu = mpBrowserFrame->GetMenu();
		if (menu) {
		    menu->CheckMenuItem( ID_FILE_PRINTPREVIEW, MF_UNCHECKED );
		}
	    }
	}
    }

    mbDocumentLoading = aBusy;

    if (mpBrowserFrame->m_wndAnimate) {
	if (mbDocumentLoading){
	    mpBrowserFrame->m_wndAnimate.Play(0, -1, -1);
	}
	else {
	    mpBrowserFrame->m_wndAnimate.Stop();
	    mpBrowserFrame->m_wndAnimate.Seek(0);
#ifdef INTERNAL_SITEICONS
		mpBrowserFrameGlue->SetFavIcon(nsnull);
#endif
	}
    }
}

void CBrowserView::SetCtxMenuLinkUrl(nsEmbedString& strLinkUrl)
{
    mCtxMenuLinkUrl = strLinkUrl;
}

void CBrowserView::SetCtxMenuImageSrc(nsEmbedString& strImgSrc)
{
    mCtxMenuImgSrc = strImgSrc;
}

void CBrowserView::SetCurrentFrameURL(nsEmbedString& strCurrentFrameURL)
{
    mCtxMenuCurrentFrameURL = strCurrentFrameURL;
}

BOOL CBrowserView::GetSecurityInfo(CString &sign)
{
	nsresult rv;

	nsCOMPtr<nsIDocShell> docShell (do_GetInterface (mWebBrowser, &rv));
	NS_ENSURE_SUCCESS(rv, FALSE);
		
	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	rv = docShell->GetSecurityUI (getter_AddRefs (securityInfo));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsEmbedString tooltip;
	rv = securityInfo->GetTooltipText (tooltip);
	NS_ENSURE_SUCCESS(rv, FALSE);

	sign = tooltip.get();
    
	return TRUE;
}

BOOL CBrowserView::GetCertificate(nsIX509Cert** certificate)
{
	nsresult rv;
	
	nsCOMPtr<nsIDocShell> docShell(do_GetInterface(mWebBrowser, &rv));
	NS_ENSURE_SUCCESS(rv, FALSE);
		
	nsCOMPtr<nsISecureBrowserUI> securityInfo;
	rv = docShell->GetSecurityUI(getter_AddRefs(securityInfo));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsISSLStatusProvider> SSLProvider = do_QueryInterface(securityInfo,&rv);
	if (!SSLProvider) return FALSE;

	nsCOMPtr<nsISSLStatus> SSLStatus;
	SSLProvider->GetSSLStatus(getter_AddRefs(SSLStatus));
	if (!SSLStatus) return FALSE;

	nsCOMPtr<nsIX509Cert> cert;
	SSLStatus->GetServerCert(getter_AddRefs (cert));
	if (!cert) return FALSE;
	
	*certificate = cert;
	NS_ADDREF(*certificate);
	
	return TRUE;
}

void CBrowserView::ShowSecurityInfo()                                           
{
    HWND hParent = mpBrowserFrame->m_hWnd;

    if(m_SecurityState == SECURITY_STATE_INSECURE) { 
	::MessageBox(m_hWnd, _T("This page has not been transferred over a secure connection."), _T("Security Information"), MB_OK);
    } else {
		
		nsresult rv;
		
		nsCOMPtr<nsIDocShell> docShell (do_GetInterface (mWebBrowser, &rv));
		if (NS_FAILED(rv)) return;
		
		nsCOMPtr<nsIX509Cert> cert;
		nsCOMPtr<nsISecureBrowserUI> securityInfo;
		rv = docShell->GetSecurityUI (getter_AddRefs (securityInfo));
		if (NS_FAILED(rv) || !(GetCertificate(getter_AddRefs(cert))))
		{
			::MessageBox(m_hWnd, _T("Failed to get the security information."), _T("Security Information"), MB_OK);
			return;
		}
	
		nsCOMPtr<nsICertificateDialogs> certDialogs = do_GetService (NS_CERTIFICATEDIALOGS_CONTRACTID, &rv);
		if (!certDialogs) return;
	
		certDialogs->ViewCert(NULL, cert);
    }
}


TCHAR * CBrowserView::GetTempFile()
{
   m_tempFileCount++;
   
   TCHAR ** newFileList = new TCHAR*[m_tempFileCount];                             // create new index

   memcpy(newFileList, m_tempFileList, ((m_tempFileCount-1)*sizeof(TCHAR**)) );   // copy old index

   if (m_tempFileCount>1) delete m_tempFileList;                                 // delete old index
   m_tempFileList = newFileList;

   TCHAR *newFile = new TCHAR[MAX_PATH];
 
   TCHAR temppath[MAX_PATH];
   GetTempPath(MAX_PATH, temppath);
   GetTempFileName(temppath, _T("kme"), 0, newFile);                                 // create tempfile name
   
   m_tempFileList[m_tempFileCount-1] = newFile;

   return newFile;
}

void CBrowserView::DeleteTempFiles()
{
   for (int x=0;x<m_tempFileCount;x++) {
      DeleteFile(m_tempFileList[x]);
      delete m_tempFileList[x];
   }
   if (m_tempFileCount > 0) delete m_tempFileList;
}

BOOL CBrowserView::GetCurrentURI(CString& uri)
{
   NS_ENSURE_TRUE(mWebNav, FALSE);
   
	nsresult rv = NS_OK;
   
   nsCOMPtr<nsIURI> currentURI;
	rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
      return FALSE;

   // Get the uri string associated with the nsIURI object
	nsEmbedCString uriString;
	rv = currentURI->GetSpec(uriString);
	NS_ENSURE_SUCCESS(rv, FALSE);

#ifdef _UNICODE
	nsEmbedString str;
    NS_CStringToUTF16(uriString, NS_CSTRING_ENCODING_UTF8, str);
	uri = str.get();
#else
   uri = uriString.get();
#endif
   return TRUE;
}

BOOL GetSelectionInsideForm(nsIDOMElement *element, nsEmbedString &aSelText)
{
	nsCOMPtr<nsIDOMNSHTMLInputElement> domnsinput = do_QueryInterface(element);
	if (domnsinput)
	{
		PRInt32 start, end;
		nsresult rv;
		rv = domnsinput->GetSelectionStart(&start);
		rv |= domnsinput->GetSelectionEnd(&end);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (start >= end) return FALSE;

		nsCOMPtr<nsIDOMHTMLInputElement> dominput = do_QueryInterface(element);
		if (!dominput) return FALSE;

		nsEmbedString value;
		dominput->GetValue(value);
		value.Cut(end,-1);
		if (start>value.Length())
			return FALSE;

		aSelText = value.get()+start;
		return TRUE;
	}

	nsCOMPtr<nsIDOMNSHTMLTextAreaElement> tansinput = do_QueryInterface(element);
	if (tansinput)
	{
		PRInt32 start, end;
		nsresult rv;
		rv = tansinput->GetSelectionStart(&start);
		rv |= tansinput->GetSelectionEnd(&end);
		NS_ENSURE_SUCCESS(rv, FALSE);
		if (start >= end) return FALSE;

		nsCOMPtr<nsIDOMHTMLTextAreaElement> tainput = do_QueryInterface(element);
		if (!tansinput) return FALSE;

		nsEmbedString value;
		tainput->GetValue(value);
		value.Cut(end,-1);
		if (start>value.Length())
			return FALSE;

		aSelText = value.get()+start;
		return TRUE;
	}

	return FALSE;
}

BOOL CBrowserView::_GetSelection(nsIDOMWindow* dom, nsAString &aSelText)
{
	nsCOMPtr<nsISelection> sel;
	dom->GetSelection(getter_AddRefs(sel));
	if (sel) {
		USES_CONVERSION;
		
		PRUnichar* selText;
		nsresult rv = sel->ToString(&selText);
		NS_ENSURE_SUCCESS(rv, FALSE);

		aSelText = selText;
		nsMemory::Free(selText);
		if (aSelText.Length()>0)
			return TRUE;
	}

	BOOL ret = FALSE;

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	NS_ENSURE_TRUE(frames, FALSE);

	PRUint32 nbframes;
	frames->GetLength(&nbframes);
	if (nbframes>0)
	{
		nsCOMPtr<nsIDOMWindow> frame;
		for (PRUint32 i = 0; i<nbframes; i++)
		{
			frames->Item(i, getter_AddRefs(frame));
			ret = ret || _GetSelection(frame, aSelText);
		}
	}
	return ret;
}

BOOL CBrowserView::GetUSelection(nsEmbedString& aSelText)
{
	nsresult rv;

	nsCOMPtr<nsIDOMWindow> dom(do_GetInterface(mWebBrowser));
	NS_ENSURE_TRUE(dom, FALSE);

	// Check selection inside the focused element if it's an 
	// input text or a textarea element.
	nsCOMPtr<nsIDOMElement> element;
	mWebBrowserFocus->GetFocusedElement(getter_AddRefs(element));
	if (element)
	{
		if (GetSelectionInsideForm(element, aSelText))
			return TRUE;
	}

	// Check normal selection inside the document
	if (_GetSelection(dom, aSelText))
		return TRUE;

	return FALSE;
}

BOOL CBrowserView::GetSelection(CString& aSelText)
{
	nsEmbedString selText;
	if (!GetUSelection(selText))
		return FALSE;

	USES_CONVERSION;
	aSelText = W2CT(selText.get());
	return TRUE;
}

/*

   The GetNodeAtPoint function is a really gross hack.
   Essentially, Mozilla doesn't expose any way for us to
   get information about the DOM given a specific point.
   As a workaround, we simply tell mozilla to post a context
   menu, and then trap it right before the menu pops up.

   This becomes even worse when we find that mozilla ignores
   the coordinates specified in the window message and, instead,
   calls GetMessagePos, which means that we need to call
   SetCursorPos() so that windows will attach the coordinates we
   want to the message we send to mozilla.

   Even worse, windows doesn't seem to immediately process the
   SetCursorPos() function, so we muck about with the message
   queue for a bit to let windows figure out that the cursor
   has changed positions.

   The bPrepareMenu flag determines whether or not the global
   variables that get set in preparation for use by identifiers
   like ID_OPEN_LINK_IN_NEW_WINDOW

*/

nsIDOMNode *CBrowserView::GetNodeAtPoint(int x, int y, BOOL bPrepareMenu) {

	// Make sure a page is actually being displayed
   nsresult rv = NS_OK;
   nsCOMPtr<nsIURI> currentURI;
   rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
   if(NS_FAILED(rv) || !currentURI)
      return NULL;

   HWND hWnd = ::GetFocus();  
   if (!::IsChild(m_hWnd, hWnd)) {
      SetFocus();
      hWnd = ::GetFocus();
   }

   POINT pt;
   ::GetCursorPos(&pt);
   ::ShowCursor(FALSE);
   ::SetCursorPos(x, y);

   // swing throught the message pump a bit, so that windows can process
   // the cursor movement (important, because mozilla uses GetMessagePos to
   // determine where the mouse was clicked)
   MSG msg;
   while (::PeekMessage(&msg,0,0,0,PM_NOREMOVE)) { 
      if (!theApp.PumpMessage()) { 
         ::PostQuitMessage(0);  break; 
      }
   } 
   
   m_iGetNodeHack = bPrepareMenu ? 2 : 1;

   ::SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(x, y));
   ::SetCursorPos(pt.x, pt.y);
   ::ShowCursor(TRUE);

   return m_pGetNode;

}


nsIDocShell *CBrowserView::GetDocShell()
{
   nsresult result = NS_OK;
   nsCOMPtr<nsIDocShell> docShell;
   
   nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser, &result);
   if(NS_FAILED(result) || !dsti)
      return NULL;
   
   nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
   result = dsti->GetTreeOwner(getter_AddRefs(treeOwner));
   if (NS_FAILED(result) || !treeOwner) 
      return  NULL;

   nsCOMPtr<nsIDocShellTreeItem> contentItem;
   result = treeOwner->GetPrimaryContentShell(getter_AddRefs(contentItem));
   if (NS_FAILED(result) || !contentItem) 
      return NULL;

   docShell = do_QueryInterface(contentItem);

   return docShell;
} 

BOOL CBrowserView::GetCharset(char* aCharset)
{
   NS_ENSURE_TRUE(mWebBrowser, FALSE);

   // Look for the forced charset
   nsresult result;
   nsCOMPtr<nsIDocShell> DocShell;

   DocShell = GetDocShell();

   if (!DocShell) 
     return FALSE;

   nsCOMPtr<nsIContentViewer> contentViewer;
   result = DocShell->GetContentViewer (getter_AddRefs(contentViewer));
   if (NS_FAILED(result) || !contentViewer) 
     return FALSE;

   nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(contentViewer,&result);
   if (NS_FAILED(result) || !mdv) 
     return FALSE;

   nsCString mCharset;
   result = mdv->GetForceCharacterSet(mCharset);

   if (NS_FAILED(result) || mCharset.IsEmpty() )
   {
	   // If no forced charset look for the document charset
	   nsCOMPtr<nsIDocCharset> docCharset = do_GetInterface(mWebBrowser);
	   NS_ENSURE_TRUE(docCharset, FALSE);

	   char *charset;
	   result = docCharset->GetCharset (&charset);
	   if (!charset)
	   {
		   // If no document charset use default
		   result = mdv->GetDefaultCharacterSet(mCharset);
	   }
	   else
	   {
		   mCharset = charset;
		   nsMemory::Free (charset);
	   }
   }

   strncpy(aCharset, mCharset.get(), 63);
   aCharset[63] = 0;
   return NS_SUCCEEDED(result);
}

BOOL CBrowserView::ForceCharset(char *aCharSet)
{
   nsresult result;
   nsCOMPtr<nsIDocShell> DocShell;

   DocShell = GetDocShell();

   if (!DocShell) 
     return FALSE;

   nsCOMPtr<nsIContentViewer> contentViewer;
   result = DocShell->GetContentViewer (getter_AddRefs(contentViewer));
   if (NS_FAILED(result) || !contentViewer) 
     return FALSE;

   nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(contentViewer,&result);
   if (NS_FAILED(result) || !mdv) 
     return FALSE;

   nsCString mCharset;
   mCharset = aCharSet;
   result = mdv->SetForceCharacterSet(mCharset);
   
   if (NS_SUCCEEDED(result))
       mWebNav->Reload(nsIWebNavigation::LOAD_FLAGS_CHARSET_CHANGE);

   return NS_SUCCEEDED(result);
} 


BOOL CBrowserView::_InjectCSS(nsIDOMWindow* dom, const wchar_t* userStyleSheet)
{
	if (!dom) return FALSE;
	
	nsresult rv;

	nsCOMPtr<nsIDOMDocument> document;
	rv = dom->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMElement> styleElement;
	rv = document->CreateElement(nsDependentString(L"style"), getter_AddRefs(styleElement));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMText> textStyle;
	rv = document->CreateTextNode(nsDependentString(userStyleSheet), getter_AddRefs(textStyle));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMNode> notused;
	rv = styleElement->AppendChild(textStyle, getter_AddRefs(notused));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMNodeList> headList;
	rv = document->GetElementsByTagName(nsDependentString(L"head"), getter_AddRefs(headList));
	if (headList)
	{
		nsCOMPtr<nsIDOMNode> headNode;
		rv = headList->Item(0, getter_AddRefs(headNode));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = headNode->AppendChild(styleElement, getter_AddRefs(notused));
	}
	else
	{
		nsCOMPtr<nsIDOMElement> documentElement;
		document->GetDocumentElement(getter_AddRefs(documentElement));
		rv = documentElement->AppendChild(styleElement, getter_AddRefs(notused));
	}

	BOOL b = TRUE;
	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		frames->GetLength(&nbframes);
		if (nbframes>0)
		{
			nsCOMPtr<nsIDOMWindow> frame;
			for (PRUint32 i = 0; i<nbframes; i++)
			{
				rv = frames->Item(i, getter_AddRefs(frame));
				if (NS_FAILED(rv)) return FALSE;
				b = b && _InjectCSS(frame, userStyleSheet);
			}
		}
	}

	return b && NS_SUCCEEDED(rv);
}

BOOL CBrowserView::InjectCSS(const wchar_t* userStyleSheet)
{
	nsresult rv;
	nsCOMPtr<nsIDOMWindow> dom;
	
	rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	// Use a recursive function to inject the CSS in all frames.
	return _InjectCSS(dom, userStyleSheet);
}

BOOL CBrowserView::InjectJS(const wchar_t* userScript, bool bTopWindow)
{
	nsresult rv;
	nsCOMPtr<nsIDOMDocument> document;
	
	int jsEnabled = theApp.preferences.GetBool("javascript.enabled", true);
	theApp.preferences.SetBool("javascript.enabled", true);

	if (!bTopWindow)
	{
		if (m_lastMouseActionNode)
			rv = m_lastMouseActionNode->GetOwnerDocument(getter_AddRefs(document));
		else
		{
			nsCOMPtr<nsIDOMWindow> dom;
			mWebBrowserFocus->GetFocusedWindow(getter_AddRefs(dom));
			if (dom)
				rv = dom->GetDocument(getter_AddRefs(document));
		}
		
	}

	if (!document)
	{
		nsCOMPtr<nsIDOMWindow> dom;
		rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = dom->GetDocument(getter_AddRefs(document));
	}

	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMElement> body;
	nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(document));
	/*if (htmlDoc)
	{
		nsCOMPtr<nsIDOMHTMLElement> htmlBody;
		htmlDoc->GetBody (getter_AddRefs(htmlBody));
		body = do_QueryInterface(htmlBody);
		NS_ENSURE_TRUE(body, FALSE);
	}	
	else*/
	{
		rv = document->GetDocumentElement (getter_AddRefs(body));
		NS_ENSURE_SUCCESS(rv, FALSE);
	}

	nsCOMPtr<nsIDOMElement> scriptElement;
	rv = document->CreateElement(nsDependentString(L"script"), getter_AddRefs(scriptElement));
	NS_ENSURE_SUCCESS(rv, FALSE);
/*
	nsCOMPtr<nsIDOMText> textScript;
	rv = document->CreateTextNode(nsDependentString(userScript), getter_AddRefs(textScript));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMNode> notused;
	rv = scriptElement->AppendChild(textScript, getter_AddRefs(notused));
	NS_ENSURE_SUCCESS(rv, FALSE);*/

	nsCOMPtr<nsIDOMHTMLScriptElement> scriptTag = do_QueryInterface(scriptElement);
	NS_ENSURE_TRUE(scriptTag, FALSE);

	scriptTag->SetText(nsDependentString(userScript));
	scriptTag->SetType(nsDependentString(L"text/javascript"));

	nsCOMPtr<nsIDOMNode> notused;
	rv = body->AppendChild(scriptTag, getter_AddRefs(notused));
	NS_ENSURE_SUCCESS(rv, FALSE);

	rv = body->RemoveChild(scriptTag, getter_AddRefs(notused));

	theApp.preferences.SetBool("javascript.enabled", jsEnabled);

	return TRUE;
}
