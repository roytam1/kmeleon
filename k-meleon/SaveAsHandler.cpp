#include "stdafx.h"
#include "SaveAsHandler.h"
#include "Utils.h"
#include "MozUtils.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "nsIMIMEHeaderParam.h"
#include "nsIMIMEService.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIStandardURL.h"
#include "nsIURL.h"


#include "UnknownContentTypeHandler.h"
#include "nsCWebBrowserPersist.h"
#include "nsIMIMEInfo.h"

NS_IMPL_ISUPPORTS1(CSaveAsHandler, nsIWebProgressListener);

CSaveAsHandler::CSaveAsHandler(
		nsIWebBrowserPersist* aPersist, 
		nsIURI* aURL, nsIDOMDocument* aDocument, 
		nsISupports* aDescriptor, nsIURI* aReferrer)
{
	mPersist = aPersist;
	mDescriptor = aDescriptor;
	mURL = aURL;
	mFile = nullptr;
	mDocument = aDocument;
	mReferrer = aReferrer;
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
			bool b;
			rv = httpchannel->GetRequestSucceeded(&b);
			// If we get an error, remove the content type since it's html and
			// the download will fail
			if (NS_SUCCEEDED(rv) && b == PR_FALSE)	{
				sContentType.SetLength(0);
			}
			httpchannel->GetResponseHeader(nsEmbedCString("content-disposition"), mContentDisposition);
		}
		mPersist->CancelSave();
		if (mFile) mFile->Remove(PR_FALSE);

		// Defering the save as dialog to not stall other transferts
		
		::AfxGetApp()->m_pMainWnd->PostMessage(WM_DEFERSAVEAS, (WPARAM)this, (LPARAM)strdup(sContentType.get()));
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
	mContentType = contentType;

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
				fallbackCharset, PR_TRUE, nullptr,
				fileName);
			if (NS_FAILED(rv) || !fileName.Length() || wcsicmp(fileName.get(), L"untitled") == 0)
			{
				rv = mimehdrpar->GetParameter (mContentDisposition, "name",
					fallbackCharset, PR_TRUE, nullptr,
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

			test->Init(nsIStandardURL::URLTYPE_STANDARD, 80, spec, charset.get(), nullptr);
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

	if (theApp.preferences.GetBool("kmeleon.download.useSaveDir", FALSE))
	{
		CString saveDir = theApp.preferences.GetString("kmeleon.download.saveDir", _T(""));
		if (!saveDir.IsEmpty())
			return DownloadTo(CStringToNSString(saveDir + _T('\\') + szFileName + _T('.') + extension),
					isHTML, theApp.preferences.iSaveType == 2);
	}

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
		else if (!extension.IsEmpty()) {
			filter.Format(IDS_UNKNOW_TYPE, extension);
			filter+=_T(" (*.") + extension + _T(")|*.") + extension + _T("|");
		}
	}

	CString filt;
	filt.LoadString(IDS_ALLFILES);
	filter += filt + _T(" (*.*)|*.*||");

	rv = NS_ERROR_ABORT;
/*	CFileDialog fileDlg(FALSE, extension, MakeFilename(W2T(fileName.get())), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
	fileDlg.GetOFN().nFilterIndex = (mDocument && isHTML) ? theApp.preferences.iSaveType == 2 ? 2 : 1 : 1;
	fileDlg.GetOFN().lpstrInitialDir = theApp.preferences.saveDir;
	if (fileDlg.DoModal() == IDOK)
	{
		CString strFullPath = fileDlg.GetPathName();
		fileDlg.Get
	*/

	
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

	if( ::GetSaveFileName(&ofn) )
	{
		CString strFullPath = szFileName;

		int idxSlash;
		idxSlash = strFullPath.ReverseFind(_T('\\'));
		theApp.preferences.saveDir = strFullPath.Mid(0, idxSlash+1);

		if (isHTML)
			theApp.preferences.iSaveType = ofn.nFilterIndex;
		
		rv = DownloadTo(CStringToNSString(strFullPath), isHTML, theApp.preferences.iSaveType == 2);
	}
	
	filter.ReleaseBuffer();
	delete szFileName;
	
	return rv;
}

NS_IMETHODIMP CSaveAsHandler::DownloadTo(nsString& aFilename, BOOL isHTML, BOOL saveComplete)
{	
		NS_ENSURE_TRUE(aFilename.Length(), NS_ERROR_FAILURE);

		nsCOMPtr<nsIFile> file;
		nsresult rv = NS_NewLocalFile(aFilename, TRUE, getter_AddRefs(file));
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
			rv = persist->SaveURI(mRealURI, nullptr, mReferrer, nullptr, nullptr, file, nullptr);
			if (NS_FAILED(rv)) { //Remove cycling reference, avoid leaking
				persist->SetProgressListener(nullptr);
				progress->Close();
			}
		}
		else
		{
			persist->SetPersistFlags(nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);
			if (isHTML) 
			{
				nsCOMPtr<nsIFile> dataFolder;

				if (saveComplete) 
				{  
					// cf.m_ofn.nFilterIndex == 3 indicates
					// user want to save the complete document including
					// all frames, images, scripts, stylesheets etc.

					USES_CONVERSION;
					CString strDataPath = W2CT(aFilename.get()), suffix;
					suffix.LoadString(IDS_SAVEPAGE_SUFFIX);

					int idxPeriod = strDataPath.ReverseFind(_T('.'));
					strDataPath = strDataPath.Mid(0, idxPeriod);
					strDataPath += suffix;

					// At this stage strDataPath will be of the form
					// c:\tmp\junk_files - assuming we're saving to a file
					// named junk.htm
					// Any images etc in the doc will be saved to a dir
					// with the name c:\tmp\junk_files
#ifdef _UNICODE
					rv = NS_NewLocalFile(nsDependentString(strDataPath), TRUE, getter_AddRefs(dataFolder));
#else
					rv = NS_NewNativeLocalFile(nsDependentCString(strDataPath), TRUE, getter_AddRefs(dataFolder));
#endif
					NS_ENSURE_SUCCESS(rv,rv);
				}
				rv = persist->SaveDocument(mDocument, file, dataFolder, mContentType.get(), 0, 0);
			}
			else
				rv = persist->SaveURI(mURL, mDescriptor, mReferrer, nullptr, nullptr, file, nullptr);
		}
	return rv;
}

NS_IMETHODIMP CSaveAsHandler::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
	return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP CSaveAsHandler::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location, uint32_t aFlags)
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


