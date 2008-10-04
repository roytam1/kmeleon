/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "stdafx.h"
//#include "nsEscape.h"
#include <wininet.h>
#include <cderr.h>
#include "UnknownContentTypeHandler.h"
#include "Utils.h"
#include "MozUtils.h"
#include "MfcEmbed.h"

#include "nsIFileURL.h"
#include "nsIHttpChannel.h"
#include "nsIMIMEInfo.h"
#include "nsIWindowWatcher.h"

NS_IMETHODIMP
CUnknownContentTypeHandler::Show(CWnd* parent)
{
	nsresult rv;
	nsCOMPtr<nsIHelperAppLauncherDialog> kungFuDeathGrip(this);

	if (theApp.preferences.GetBool("kmeleon.download.SaveUnkownContent", true))
	{ 
		if (theApp.preferences.bAskOpenSave)
		{
			CoInitialize(NULL);

			COpenSaveDlg dlg(parent);

			USES_CONVERSION;
			// Set the filename
			nsEmbedString filename;
			mAppLauncher->GetSuggestedFileName(filename);
			dlg.m_csFilename = W2CT(filename.get());

			// Set the mime type
			
			nsCOMPtr<nsIMIMEInfo> mimeInfo;
			mAppLauncher->GetMIMEInfo(getter_AddRefs(mimeInfo));
			dlg.m_csFiletype = GetTypeName();

			// Set the source
			nsCOMPtr<nsIURI> uri;
			rv = mAppLauncher->GetSource(getter_AddRefs(uri));
			if(NS_SUCCEEDED(rv))
			{
				nsEmbedCString sourceURI;
				uri->GetHost(sourceURI);
				
				if (!sourceURI.IsEmpty())
					dlg.m_csSource = NSUTF8StringToCString(sourceURI);
				else
				{
					uri->GetScheme(sourceURI);
					if (sourceURI.Equals(nsCString("file")))
						dlg.m_csSource = _T("localhost");		
				}
			}

			switch (dlg.DoModal()) {

				case IDOK:
					rv = mAppLauncher->SaveToDisk(nsnull, false);
					break;
				case IDOPEN:
					if (mimeInfo) {
						// This code prevent gecko to throw an error when 
						// trying to open a file with no association
						PRBool hasHandler;
						mimeInfo->GetHasDefaultHandler(&hasHandler);
						if (!hasHandler)
							mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
					}
					rv = mAppLauncher->LaunchWithApplication(nsnull, PR_FALSE);
					break;
				default:
					rv = mAppLauncher->Cancel(NS_ERROR_ABORT);
			}

			CoUninitialize();
		}
		else
			rv = mAppLauncher->SaveToDisk(nsnull, false);
	}
	else
		rv = mAppLauncher->Cancel(NS_ERROR_ABORT);

	// If the window that launched the download is "about:blank"
	// then we need to close it.
	BOOL closeParent = !GetUriForDOMWindow(mDomWindow).Compare(_T("about:blank"));
	if (mDomWindow && closeParent)
	{
		nsCOMPtr<nsIWindowWatcher> mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
		if (mWWatch) {
			nsCOMPtr<nsIWebBrowserChrome> chrome;
			CWnd *val = 0;

			nsCOMPtr<nsIDOMWindow> fosterParent;
			mWWatch->GetChromeForWindow(mDomWindow, getter_AddRefs(chrome));
			if (chrome)
				chrome->DestroyBrowserWindow();
		}
	}

	return rv;
}

// Show the helper app launch confirmation dialog as instructed.
NS_IMETHODIMP
CUnknownContentTypeHandler::Show( nsIHelperAppLauncher *aLauncher, nsISupports *aContext,  PRUint32 aReason)
{
	NS_ENSURE_ARG_POINTER(aLauncher);
	mAppLauncher = aLauncher;
	
	// This function must return immediately or all transferts are stalled.
    // XXX: Stupid mozilla
	// If an error happen (ex: temporary folder is full), an error message 
	// is show to the user when asking to open/save or for the location. 
	// The dialogs asking open/save or for the location must be closed at 
	// the same time than the error message.

	mDomWindow = do_GetInterface (aContext);
	CWnd* wnd = CWndForDOMWindow(mDomWindow);

	theApp.m_pMainWnd->PostMessage(WM_DEFERSHOW, (WPARAM)wnd, (LPARAM)this);

	return NS_OK;
}

#include "nsIDOMWindowInternal.h"
// prompt the user for a file name to save the unknown content to as instructed
NS_IMETHODIMP
#if GECKO_VERSION>18
CUnknownContentTypeHandler::PromptForSaveToFile(nsIHelperAppLauncher *aLauncher, nsISupports * aWindowContext, const PRUnichar * aDefaultFile, const PRUnichar * aSuggestedFileExtension, PRBool aForcePrompt, nsILocalFile ** aNewFile)
#else
CUnknownContentTypeHandler::PromptForSaveToFile(nsIHelperAppLauncher *aLauncher, nsISupports * aWindowContext, const PRUnichar * aDefaultFile, const PRUnichar * aSuggestedFileExtension, nsILocalFile ** aNewFile)
#endif
{
	NS_ENSURE_ARG_POINTER(aNewFile);
	NS_ENSURE_ARG(aDefaultFile);
	NS_ENSURE_ARG(aSuggestedFileExtension);

	// change this to 0 to use the mozilla file picker
#if 1
	USES_CONVERSION;

	CString pathName;

	CString downloadDir = theApp.preferences.downloadDir;
	if (theApp.preferences.bUseDownloadDir && !downloadDir.IsEmpty()
#if GECKO_VERSION>18
		&& !aForcePrompt
#endif
		)
	{
		if (downloadDir[downloadDir.GetLength()-1] != '\\')
			downloadDir += '\\';
		pathName = downloadDir + PRUnicharToCString(aDefaultFile);
	}
	else
	{
		CString defaultFile = (CString)theApp.preferences.lastDownloadDir + PRUnicharToCString(aDefaultFile);
		CString fileType;

		CString filter;
		const TCHAR *ext = _T("");
		if (*aSuggestedFileExtension) {
			
			fileType = GetTypeName();

			if (!fileType.IsEmpty())
				filter = fileType;
			else {
				filter = _T("*");
				filter += W2CT(aSuggestedFileExtension);
			}
			filter += _T("|*");
			filter += W2CT(aSuggestedFileExtension);
			filter += _T("|");

			ext = W2CT(aSuggestedFileExtension+1);
		}

		filter += _T("All Files|*.*||");

		TCHAR lpszFilter[1024];
		_tcscpy(lpszFilter, filter);
		for (int i=0; lpszFilter[i]; ) {
			if (lpszFilter[i] == _T('|'))
				lpszFilter[i] = _T('\0');
			i++;
		}

		OPENFILENAME ofn;
		TCHAR *szFileName = new TCHAR[INTERNET_MAX_URL_LENGTH];

		memset(&ofn, 0, sizeof(ofn));
		memset(szFileName, 0, sizeof(TCHAR)*INTERNET_MAX_URL_LENGTH);
		_tcscat(szFileName, defaultFile);
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = lpszFilter;
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = INTERNET_MAX_URL_LENGTH;
		ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		ofn.lpstrInitialDir = theApp.preferences.lastDownloadDir;
		ofn.lpstrDefExt = ext;

		BOOL bGetFile;
		if( !(bGetFile=::GetSaveFileName(&ofn)) ) 
		{
			// Fix: If the download dir contains invalid character, it'll
			// never show up
			if (CommDlgExtendedError() == FNERR_INVALIDFILENAME)
			{
				_tcscpy(szFileName, W2CT(aDefaultFile));
				theApp.preferences.lastDownloadDir = _T("");
				bGetFile = ::GetSaveFileName(&ofn);
			}
		}
		if (bGetFile)
			pathName = szFileName;

		delete szFileName;
		szFileName = NULL;
	}
	
	if (!pathName.IsEmpty())
	{
		nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));

		NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

#ifdef _UNICODE
		nsresult rv = file->InitWithPath(nsDependentString(pathName));
#else
		nsresult rv = file->InitWithNativePath(nsDependentCString(pathName));
#endif
		NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

		NS_ADDREF(*aNewFile = file);

		// Be sure to save off the new default saveDir
		int slash = pathName.ReverseFind('\\');
		if (slash == -1) {
			pathName.ReverseFind('/');
		}
		if (slash != -1) {
			theApp.preferences.lastDownloadDir = pathName.Left(slash+1);
		}

		return NS_OK;
	}

	return NS_ERROR_FAILURE;
#else

   nsresult rv = NS_OK;

   nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
//   nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance(NS_FILEPICKER_CID, &rv);
   if (filePicker)
   {
      nsEmbedString title = NS_LITERAL_STRING ("Save As:");

      nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aWindowContext ) );
      filePicker->Init(parent, title, nsIFilePicker::modeSave);
      filePicker->SetDefaultString(nsDependentString(aDefaultFile));
      nsEmbedString wildCardExtension (NS_LITERAL_STRING("*").get());
      if (aSuggestedFileExtension) {
         wildCardExtension.Append(aSuggestedFileExtension);
         filePicker->AppendFilter(wildCardExtension, wildCardExtension);
      }

      filePicker->AppendFilters(nsIFilePicker::filterAll);

      nsCOMPtr<nsILocalFile> startDir;
      // Pull in the user's preferences and get the default download directory.
      nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
      if ( prefs ) 
      {
         rv = prefs->GetFileXPref( "browser.download.dir", getter_AddRefs( startDir ) );
         if ( NS_SUCCEEDED(rv) && startDir ) 
         {
            PRBool isValid = PR_FALSE;
            startDir->Exists( &isValid );
            if ( isValid )  // Set file picker so startDir is used.
               filePicker->SetDisplayDirectory( startDir );
         }
      }

      PRInt16 dialogResult;
      filePicker->Show(&dialogResult);
      if (dialogResult == nsIFilePicker::returnCancel)
         rv = NS_ERROR_FAILURE;
      else
      {
         // be sure to save the directory the user chose as the new browser.download.dir
         rv = filePicker->GetFile(aNewFile);
         if (*aNewFile)
         {
            nsCOMPtr<nsIFile> newDirectory;
            (*aNewFile)->GetParent(getter_AddRefs(newDirectory));
            nsCOMPtr<nsILocalFile> newLocalDirectory (do_QueryInterface(newDirectory));

            if (newLocalDirectory)
               prefs->SetFileXPref( "browser.download.dir", newLocalDirectory);
         }
      }
   }
   return rv;

   /*
   USES_CONVERSION;
   NS_ENSURE_ARG_POINTER(aNewFile);
   char *lpszFilter = "All Files (*.*)|*.*||";
   CFileDialog cf(FALSE, W2T(aSuggestedFileExtension), W2T(aDefaultFile),
      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
      lpszFilter, NULL);
   if(cf.DoModal() == IDOK)
   {
      CString m_FileName = cf.GetPathName(); // Will be like: c:\tmp\junk.exe
      
      return NS_NewLocalFile(m_FileName, PR_FALSE, aNewFile);
   }
   else
      return NS_ERROR_FAILURE;
   */

#endif
}

CString CUnknownContentTypeHandler::GetTypeName()
{
	USES_CONVERSION;
	nsresult rv;

	nsCOMPtr<nsIMIMEInfo> mimeInfo;
	mAppLauncher->GetMIMEInfo(getter_AddRefs(mimeInfo));
			
	if(mimeInfo) 
	{
		nsEmbedCString mimeType;
		rv = mimeInfo->GetMIMEType(mimeType);
		nsEmbedString mimeDesc;
		rv = mimeInfo->GetDescription(mimeDesc);
		if(NS_SUCCEEDED(rv)) {
			if (*mimeDesc.get())
				return CString(W2CA(mimeDesc.get()));
			if (*mimeType.get())	
				return CString(A2CT(mimeType.get()));
		}
	}
	
	nsEmbedString filename;
	mAppLauncher->GetSuggestedFileName(filename);
	SHFILEINFO shfi;
	if (SHGetFileInfo(W2CT(filename.get()), 0, &shfi, sizeof(SHFILEINFO),SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME)) 
		return CString(shfi.szTypeName);
	return CString();
}

/* -- Mark L.: Leftover from a forgotten era?
NS_GENERIC_FACTORY_CONSTRUCTOR(CUnknownContentTypeHandler)

static nsModuleComponentInfo components[] = {
  { NS_IHELPERAPPLAUNCHERDLG_CLASSNAME, 
    NS_UNKNOWNCONTENTTYPEHANDLER_CID, 
    NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, 
    CUnknownContentTypeHandlerConstructor },
};

NS_IMPL_NSGETMODULE("CUnknownContentTypeHandler", components )
*/

/* nsISupports Implementation for the class */
NS_IMPL_ISUPPORTS1(CUnknownContentTypeHandler, nsIHelperAppLauncherDialog)


//*****************************************************************************
// CUnknownContentHandlerFactory
//*****************************************************************************   

class CUnknownContentHandlerFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CUnknownContentHandlerFactory();
  virtual ~CUnknownContentHandlerFactory();
};

//*****************************************************************************   
/*
NS_IMPL_ISUPPORTS1(CUnknownContentHandlerFactory, nsIFactory)

CUnknownContentHandlerFactory::CUnknownContentHandlerFactory()
{
  NS_INIT_ISUPPORTS();
}

CUnknownContentHandlerFactory::~CUnknownContentHandlerFactory()
{
}

NS_IMETHODIMP CUnknownContentHandlerFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CUnknownContentTypeHandler *inst = new CUnknownContentTypeHandler;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CUnknownContentHandlerFactory::LockFactory(PRBool lock)
{
    return NS_OK;
}


nsresult NewUnknownContentHandlerFactory(nsIFactory** aFactory) {
   NS_ENSURE_ARG_POINTER(aFactory);
   *aFactory = nsnull;
   CUnknownContentHandlerFactory *result = new CUnknownContentHandlerFactory;
   if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(result);
   *aFactory = result;
   return NS_OK;
}
*/

/********************************************************************************************************
file save progress dialog box
********************************************************************************************************/



/*

   23 October, 2002

   This class needs to be entirely reworked, it's pretty much a gross hack to
   get us just what we need at the moment, when it's cleaned up, we'll actually
   be able to do some better stuff.  We need an implementation that supports:

      1) automatic (factory) downloads
      2) manual (save link as) downloads
      3) behind-the-scenes (view-source) downloads, with some sort of "file complete" callback or flag

   -- Jeff

*/


// WeakReference not needed?
//NS_IMPL_ISUPPORTS3(CProgressDialog, nsITransfer, nsIWebProgressListener2, nsIWebProgressListener)
NS_IMPL_ISUPPORTS4(CProgressDialog, nsITransfer, nsIWebProgressListener2, nsIWebProgressListener, nsISupportsWeakReference)

CProgressDialog::CProgressDialog(BOOL bAuto) {
   NS_INIT_ISUPPORTS();

   //mObserver = NULL;
   mCancelable = nsnull;
   m_HelperAppLauncher = nsnull;
   mStartTime = 0;
   mPaused = FALSE;

   m_HandleContentOp = 0;

   // assume we're done until we get data
   // for small files, we'll be done before the box even pops up
   // mDone = true;
   mDone = false;
   mTotalBytes = 0;

   m_bClose = theApp.preferences.bCloseDownloadDialog;
      
   // the instance was created automatically by a download
   // if it's false, then we can expect an Attach() call to bind this to a persist object
   // if it's true, then we'll pop up the download dialog here
  /* m_bAuto = bAuto;
   if (m_bAuto) {   
      m_bWindow = TRUE; 
      Create(IDD_PROGRESS, GetDesktopWindow());
      CheckDlgButton(IDC_CLOSE_WHEN_DONE, TRUE);
      GetDlgItem(IDC_CLOSE_WHEN_DONE)->EnableWindow(FALSE);
      theApp.RegisterWindow(this);
   }
   else*/
      m_bWindow = FALSE;
	  mUri = NULL;

	  mCallback = NULL;
}

CProgressDialog::~CProgressDialog(){
   if (mUri)
	  delete mUri;
}

NS_IMETHODIMP CProgressDialog::OnProgressChange64(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt64 aCurSelfProgress, PRInt64 aMaxSelfProgress, PRInt64 aCurTotalProgress, PRInt64 aMaxTotalProgress)
{
	if (!mTotalBytes)
		mTotalBytes = aMaxTotalProgress;

	if (!mRequest) {
		mRequest = aRequest;
		CWnd* button = GetDlgItem(IDC_PAUSE);
		if (button)
			button->ShowWindow(SW_SHOW);
	}
/*
   
   if (aMaxTotalProgress <= 0)
      return NS_OK;
   else if (!mTotalBytes)
      // strangely enough, this isn't a parameter in nsIDownload::Init()
      mTotalBytes = aMaxTotalProgress;
   
   if (!m_bWindow)   // if there's no window, there's no need to update it :)
      return NS_OK;

   if (mStartTime <= 0)
       return NS_OK;
*/ 

  if (!m_bWindow)   // if there's no window, there's no need to update it :)
      return NS_OK;


   if (aMaxTotalProgress && (
         PR_Now() > mLastUpdateTime + 100000.0l    // enforce a minimum delay between updates - gives a large speed increase for super-fast downloads which wasted CPU cycles updating the dialog constantly
         || aCurTotalProgress >= aMaxTotalProgress // and be sure to catch the very last one, in case it would otherwise be skipped by the time check
         ) )
   {
      mLastUpdateTime = PR_Now();
      mDone = false;

      int percent = (int)(((float)aCurTotalProgress / (float)aMaxTotalProgress) * 100.0f);

      CString progressString;
      if (percent>=0 && percent<=100)
        progressString.Format(IDS_DOWNLOAD_PROGRESS, percent, ((double)aCurTotalProgress)/1024, ((double)aMaxTotalProgress)/1024);
      else
        progressString.Format(IDS_DOWNLOAD_PROGRESS2, ((double)aCurTotalProgress)/1024);
      SetDlgItemText(IDC_STATUS, progressString);

      PRInt64 now = PR_Now();
      PRInt64 timeSpent = now - mStartTime;
      PRInt64 delta = aCurTotalProgress;

      // given in bytes per second!
      double speed = 0.0;

      double timeSpent_seconds = ((double)timeSpent/1000000.0l);
      if (timeSpent_seconds > 0)
         speed = delta / timeSpent_seconds;

      double speed_kbs = speed/1024;

      CString speedString;
	  speedString.Format(IDS_SPEED_STRING, speed_kbs);
      SetDlgItemText(IDC_SPEED, speedString);

		if (speed) {
         PRInt32 remaining = (PRInt32)((aMaxTotalProgress - aCurTotalProgress)/speed +.5);

         CString timeString;

         if (remaining >= 0) {
         int remainHours=0, remainMin=0, remainSec=0, remainTemp;
         remainTemp = remaining;
         
         if (remainTemp > 3600) {
            remainHours = remainTemp/3600;
            remainTemp %= 3600;
         }
         if (remainTemp > 60) {
            remainMin = remainTemp/60;
            remainTemp %= 60;
         }
         remainSec = remainTemp;
	
         if (remainHours)
            timeString.Format(IDS_TIMELEFT_HOURS, remainHours, remainMin, remainSec);
         else if (remainMin) {
            if (remainMin == 1)
               timeString.Format(IDS_TIMELEFT_MINUTE, remainSec);
            else
               timeString.Format(IDS_TIMELEFT_MINUTES, remainMin, remainSec);
         }
         else
            timeString.Format(IDS_TIMELEFT_SECONDS, remainSec);
         }
         else
            timeString.Format(IDS_TIMELEFT_UNKNOWN);
         SetDlgItemText(IDC_TIME_LEFT, timeString);
      }

      if (percent>=0 && percent<=100) {
         CString titleString;
         titleString.Format(IDS_PERCENT_OF_FILE, percent, mFileName);
         SetWindowText(titleString);
      } 
      else
         SetWindowText(mFileName);


      HWND progressBar;
      GetDlgItem(IDC_DOWNLOAD_PROGRESS, &progressBar);
      ::SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
      ::SendMessage(progressBar, PBM_SETPOS, (WPARAM) percent, 0);
   }
   return NS_OK;
}
/* void onStateChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aStateFlags, in unsigned long aStatus); */
NS_IMETHODIMP CProgressDialog::OnStateChange(nsIWebProgress *aWebProgress, 
                                          nsIRequest *aRequest, PRUint32 aStateFlags, 
                                          nsresult aStatus)
{
	if (aStateFlags & nsIWebProgressListener::STATE_START &&
	    aStateFlags & nsIWebProgressListener::STATE_IS_REQUEST)
	{
		// Failing on a http error instead of downloading an erroneous file
		nsCOMPtr<nsIHttpChannel> httpchannel = do_QueryInterface(aRequest);
		if (httpchannel)
		{
			PRBool b;
			nsresult rv = httpchannel->GetRequestSucceeded(&b);
			if (NS_SUCCEEDED(rv) && b == PR_FALSE)	{
				Cancel();
				return NS_OK;
			}
		}
	}
	
	CString statusText;

   if ( (aStateFlags & nsIWebProgressListener::STATE_STOP))
   {
 		if (mCallback)
			mCallback(mUri, (LPTSTR)(LPCTSTR)mFilePath, aStatus, mParam);

		//nsCOMPtr<nsITransfer> kungFuDeathGrip(this);
		mCancelable = nsnull;
   

   if (!m_bWindow)    // if there's no window, there's no need to update it :)
      return NS_OK;
	
   
   

	   if (NS_SUCCEEDED(aStatus)) {

		   if (IsDlgButtonChecked(IDC_CLOSE_WHEN_DONE)) {
			   if (!m_bAuto)
				   theApp.preferences.bCloseDownloadDialog = true;
			   Close();
		   }
		   else
		   {

			   PRInt64 now = PR_Now();
			   PRInt64 timeSpent = now - mStartTime;
			   statusText.Format(IDS_DOWNLOAD_DONE, ((double)mTotalBytes)/1024, (int)(timeSpent/1000000.0l));


			   // "save link as..." saving never gets the final progress change,
			   // which leaves the progress bar hanging around 90% or so
			   HWND progressBar;
			   GetDlgItem(IDC_DOWNLOAD_PROGRESS, &progressBar);
			   ::SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
			   ::SendMessage(progressBar, PBM_SETPOS, (WPARAM) 100, 0);


			   SetDlgItemText(IDC_STATUS, statusText);

			   statusText.LoadString(IDS_CLOSE);
			   SetDlgItemText(IDCANCEL, statusText);

			   if (m_HandleContentOp!=1) {
			      GetDlgItem(IDC_OPEN)->ShowWindow(SW_SHOW);
			      CWnd* button = GetDlgItem(IDC_OPENFOLDER);
			      if (button) button->ShowWindow(SW_SHOW);
				  GetDlgItem(IDC_CLOSE_WHEN_DONE)->ShowWindow(SW_HIDE);
			   }

			   CWnd* button = GetDlgItem(IDC_PAUSE);
			   if (button) button->ShowWindow(SW_HIDE);

			   mDone = true;
			   mRequest = nsnull;
			   theApp.preferences.bCloseDownloadDialog = false;
				
			   if (theApp.preferences.bFlashWhenCompleted)
				 FlashWindow(TRUE);
		   }
	   }
	   else
	   {	
		   statusText.LoadString(IDS_DOWNLOAD_FAILED);

		   nsCOMPtr<nsIHttpChannel> httpchannel = do_QueryInterface(aRequest);
		   if (httpchannel)
		   {
			   PRBool b;
				nsresult rv = httpchannel->GetRequestSucceeded(&b);
				if (NS_SUCCEEDED(rv) && b == PR_FALSE)	{
					nsEmbedCString str;
					httpchannel->GetResponseStatusText(str);
					// Fix me : text can be encoded in anything
#ifdef UNICODE					
					nsEmbedString _str;
					NS_CStringToUTF16(str, NS_CSTRING_ENCODING_ASCII, _str);
					statusText += _str.get();
#else
					statusText += str.get();
#endif
					statusText += _T("!");
				}

		   }
		   if (m_bWindow) {
			SetDlgItemText(IDC_STATUS, statusText);
			GetDlgItem(IDC_CLOSE_WHEN_DONE)->ShowWindow(SW_HIDE);
			statusText.LoadString(IDS_CLOSE);
			SetDlgItemText(IDCANCEL, statusText);
		   }

		   mDone = true;
		   if (theApp.preferences.bFlashWhenCompleted)
		     FlashWindow(TRUE);
	   }
   }

   if (!m_bWindow)   
      return NS_OK;

   if (aStateFlags & nsIWebProgressListener::STATE_REDIRECTING){
      statusText.LoadString(IDS_REDIRECTING);
	  SetDlgItemText(IDC_STATUS, statusText);
   }else if (aStateFlags & nsIWebProgressListener::STATE_TRANSFERRING){
      statusText.LoadString(IDS_DOWNLOADING);
	  SetDlgItemText(IDC_STATUS, statusText);
   }else if (aStateFlags & nsIWebProgressListener::STATE_NEGOTIATING){
      statusText.LoadString(IDS_NEGOTIATING);
	  SetDlgItemText(IDC_STATUS, statusText);
   }else if (aStateFlags & nsIWebProgressListener::STATE_START){
	  // Since fix for BUG #752 the download can be finished before the 
	  // progress dialog is created. In that case, mozilla send a 
	  // STATE_START after a STATE_STOP ...
	  if (!mDone) {
		statusText.LoadString(IDS_CONTACTING);
        SetDlgItemText(IDC_STATUS, statusText);
	  }
   }
   
   return NS_OK;
}

/* void onProgressChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aCurSelfProgress, in long aMaxSelfProgress, in long aCurTotalProgress, in long aMaxTotalProgress); */
NS_IMETHODIMP CProgressDialog::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
	return OnProgressChange64(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP CProgressDialog::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location){
   return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP CProgressDialog::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage){
	USES_CONVERSION;
	::AfxMessageBox(W2CT(aMessage),MB_OK|MB_ICONEXCLAMATION);
	Cancel();
	if (m_bWindow) 
		Close();
	/*MessageBox(W2CA(aMessage), "", MB_OK|MB_ICONERROR);
	if (m_bWindow)
	{
		CString statusText;
		USES_CONVERSION;
		statusText = W2CA(aMessage);
	    SetDlgItemText(IDC_STATUS, statusText);
	}*/
	return NS_OK;
}

NS_IMETHODIMP CProgressDialog::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
   return NS_OK;
}


// We're overriding a function with...  itself?
//void CProgressDialog::DoDataExchange(CDataExchange* pDX){
//   CDialog::DoDataExchange(pDX);
//}


BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	ON_COMMAND(IDC_OPEN, OnOpen)
	ON_COMMAND(IDC_PAUSE, OnPause)
	ON_COMMAND(IDC_OPENFOLDER, OnOpenFolder)
	ON_COMMAND(WM_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_CLOSE_WHEN_DONE, OnBnClickedCloseWhenDone)
END_MESSAGE_MAP()

void CProgressDialog::Cancel() {
   
   if (mCancelable)
   {
		mCancelable->Cancel(NS_BINDING_ABORTED);
		if (mTempFile.GetLength()) 
			DeleteFile(mTempFile);
		else if (mFileName.GetLength()) 
			DeleteFile(mFileName);
   }
   else
	   if (mFileName.GetLength()) DeleteFile(mFileName);

   // Connection is not correctly closed when download 
   // is in pause. This fix it. // XXX MOZILLA
   if (mRequest)
	   mRequest->Cancel(NS_BINDING_ABORTED);
   
   //Close();

/*   if (mObserver) {
     mObserver->Observe(nsnull, "OnCancel", nsnull);
     mObserver->Observe(nsnull, "oncancel", nsnull);
   }*/
}

void CProgressDialog::Close() {
   
	theApp.UnregisterWindow(this);
    DestroyWindow();

	m_bWindow = false;
	
/*	if (mCancelable)
		mCancelable = nsnull;*/
	
	NS_RELEASE_THIS();
	/*
	// Not needed, mozilla have corrected the bug
   if (m_bAuto) {
      // we need launcher to call CloseProgressWindow() and be properly disposed
      nsresult rv;
      nsCOMPtr<nsIHelperAppLauncher> launcher( do_QueryInterface(mObserver, &rv) );
      if (rv == NS_OK) {
         launcher->CloseProgressWindow();
      }
   }*/
 
}

void CProgressDialog::OnOpenFolder()
{
   CString directory = mFilePath;
   int last_slash = directory.ReverseFind(_T('\\'));
   directory.GetBuffer(0);
   directory.ReleaseBuffer(last_slash); // Truncate
   ShellExecute(NULL, _T("open"), directory, _T(""), directory, SW_SHOW);
   Close();
}

void CProgressDialog::OnPause()
{
	if (!mRequest) return;
	CString label;
	if (mPaused) {
		mRequest->Resume();
		label.LoadString(IDS_PAUSE);
		SetDlgItemText(IDC_PAUSE, label);
		mPaused = FALSE;
	}
	else {
		mRequest->Suspend();
		label.LoadString(IDS_RESUME);
		SetDlgItemText(IDC_PAUSE, label);
		mPaused = TRUE;
	}
}

void CProgressDialog::OnCancel() {
    if (mDone){
      Close();
   }
   else {
      Cancel();
	  Close();
   }
}

void CProgressDialog::OnClose() {
   if (mDone){
      Close();
   }
   else {
      Cancel();
   }
}

void CProgressDialog::OnOpen() {
   CString directory = mFilePath;
   int last_slash = directory.ReverseFind(_T('\\'));
   directory.GetBuffer(0);
   directory.ReleaseBuffer(last_slash); // Truncate
   ShellExecute(NULL, _T("open"), mFilePath, _T(""), directory, SW_SHOW);
   Close();
}

void CProgressDialog::OnBnClickedCloseWhenDone()
{
	if (IsDlgButtonChecked(IDC_CLOSE_WHEN_DONE)) 
		theApp.preferences.bCloseDownloadDialog = true;
	else
		theApp.preferences.bCloseDownloadDialog = false;
}

void CProgressDialog::InitControl(const char *uri, const TCHAR *filepath)
{ 
  
   const TCHAR *file = _tcsrchr(filepath, '\\')+1;
   mFileName = file;
   mFilePath = filepath;
   mUri = strdup(uri);
   
   mLastUpdateTime = 0;
   mTotalBytes = 0;

   if (m_bWindow) {
	   //To avoid behind released by HelperAppLauncher when the
	   //download is finished.
   	   NS_ADDREF_THIS();
	  
      Create(IDD_PROGRESS, GetDesktopWindow());
      CheckDlgButton(IDC_CLOSE_WHEN_DONE, m_bClose);
      theApp.RegisterWindow(this);
      USES_CONVERSION;
      SetDlgItemText(IDC_SOURCE, A2CT(uri));
      SetDlgItemText(IDC_DESTINATION, filepath); 
	  if (theApp.preferences.bShowMinimized)
          ShowWindow(SW_MINIMIZE);
	  else
		  ShowWindow(SW_SHOW);
	  SetIcon(theApp.GetDefaultIcon(FALSE), FALSE);
	  SetIcon(theApp.GetDefaultIcon(TRUE), TRUE);
   }
}

void CProgressDialog::SetCallBack(ProgressDialogCallback aCallback, void* aParam)
{
	mCallback = aCallback;
	mParam = aParam;
}

void CProgressDialog::InitPersist(nsIURI *aSource, nsILocalFile *aTarget, nsIWebBrowserPersist *aPersist, BOOL bShowDialog)
{
   mCancelable = aPersist;
   aPersist->SetProgressListener(this);
   m_bWindow = bShowDialog;

   nsEmbedCString uri;
   aSource->GetSpec(uri);
#ifdef _UNICODE
   nsEmbedString filepath;
   aTarget->GetPath(filepath);
#else
   nsEmbedCString filepath;
   aTarget->GetNativePath(filepath);
#endif

   mStartTime = PR_Now();

   InitControl(uri.get(), filepath.get());
}

NS_IMETHODIMP CProgressDialog::Init(nsIURI *aSource, nsIURI *aTarget, const nsAString & aDisplayName, nsIMIMEInfo *aMIMEInfo, PRTime startTime, nsILocalFile *aTempFile, nsICancelable *aCancelable) 
//NS_IMETHODIMP CProgressDialog::Init(nsIURI *aSource, nsIURI *aTarget, const nsAString & aDisplayName, nsIMIMEInfo *aMIMEInfo, PRTime startTime, nsICancelable *aCancelable) 
{
   nsEmbedCString uri;
#ifdef _UNICODE  
   nsEmbedString filepath;
   nsEmbedString tempfile;
#else
   nsEmbedCString filepath;
   nsEmbedCString tempfile;
#endif

   aSource->GetSpec(uri);

   nsCOMPtr<nsIFileURL> tFileUrl = do_QueryInterface(aTarget);
   if (tFileUrl)
   {
     nsCOMPtr<nsIFile> tFileRef;
	 nsCOMPtr<nsIFile> tTempRef;
     tFileUrl->GetFile(getter_AddRefs(tFileRef));
	 //tTempRef->GetFile(getter_AddRefs(aTempFile));
#ifdef _UNICODE 
	 tFileRef->GetPath(filepath);
	  aTempFile->GetPath(tempfile);
#else
	 tFileRef->GetNativePath(filepath);
	 aTempFile->GetNativePath(tempfile);
#endif
   }

   if (aMIMEInfo)
	{
#if GECKO_VERSION > 18
		nsHandlerInfoAction action;		
#else
		nsMIMEInfoHandleAction action;
#endif		
		if (NS_SUCCEEDED(aMIMEInfo->GetPreferredAction(&action)))
		{
			if (action == nsIMIMEInfo::useHelperApp || action == nsIMIMEInfo::useSystemDefault )
				m_HandleContentOp = 1;
		}
	}

   mStartTime = startTime;
   mCancelable = aCancelable;
   m_bWindow = TRUE;
   mTempFile = tempfile.get();

   InitControl(uri.get(), filepath.get());
/*

   SetDlgItemText(IDC_SOURCE, uri.get());
   SetDlgItemText(IDC_DESTINATION, filepath.get());

   char *file = strrchr(filepath.get(), '\\')+1;
   mFileName = strdup(file);
   mFilePath = strdup(filepath.get());

   
   mLastUpdateTime = 0;
*/
   return NS_OK;
}

#if GECKO_VERSION > 18
NS_IMETHODIMP CProgressDialog::OnRefreshAttempted(nsIWebProgress *aWebProgress, nsIURI *aRefreshURI, PRInt32 aMillis, PRBool aSameURI, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

void CProgressDialog::OnOK()
{
	// Prevent the window to close when hitting return
	//__super::OnOK();
}

//*****************************************************************************
// CUnknownContentHandlerFactory
//*****************************************************************************   
/*
class CDownloadFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CDownloadFactory();
  virtual ~CDownloadFactory();
};


//*****************************************************************************   

NS_IMPL_ISUPPORTS1(CDownloadFactory, nsIFactory)

CDownloadFactory::CDownloadFactory()
{
}

CDownloadFactory::~CDownloadFactory()
{
}

NS_IMETHODIMP CDownloadFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
	
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CProgressDialog *inst = new CProgressDialog;
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {
    // We didn't get the right interface, so clean up  
    delete inst;  
  }

  return rv;
}

NS_IMETHODIMP CDownloadFactory::LockFactory(PRBool lock)
{
   return NS_OK;
}


nsresult NewDownloadFactory(nsIFactory** aFactory) {
   NS_ENSURE_ARG_POINTER(aFactory);
   *aFactory = nsnull;
   CDownloadFactory *result = new CDownloadFactory;
   if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(result);
   *aFactory = result;
   return NS_OK;
}

*/

// Boû‘e de dialogue COpenSaveDlg

//IMPLEMENT_DYNAMIC(COpenSaveDlg, CDialog)
COpenSaveDlg::COpenSaveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenSaveDlg::IDD, pParent)
	, m_csFilename(_T(""))
	, m_csFiletype(_T(""))
	, m_csSource(_T(""))
{
}

COpenSaveDlg::~COpenSaveDlg()
{
}

void COpenSaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DOWNLOAD_NAME, m_csFilename);
	DDX_Text(pDX, IDC_DOWNLOAD_TYPE, m_csFiletype);
	DDX_Text(pDX, IDC_DOWNLOAD_SOURCE, m_csSource);
	DDX_Control(pDX, IDC_FILE_ICON, m_cFileIcon);
}


BEGIN_MESSAGE_MAP(COpenSaveDlg, CDialog)
	ON_BN_CLICKED(IDOPEN, OnBnClickedOpen)
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void COpenSaveDlg::OnBnClickedOpen()
{
	EndDialog(IDOPEN);
}

// Use the same routine than mozilla to avoid trouble
BOOL IsExecutable(const TCHAR* filename)
{
    // Get extension.
    TCHAR * _ext = (TCHAR *) _tcsrchr(filename, _T('.'));
	USES_CONVERSION;
	char* ext = T2A(_ext);
    if ( ext ) {
        // Convert extension to lower case.
        for( char *p = ext; *p; p++ )
            *p = tolower(*p);
        
        // Search for any of the set of executable extensions.
        const char * const executableExts[] = {
            ".ad",
            ".adp",
            ".asp",
            ".bas",
            ".bat",
            ".chm",
            ".cmd",
            ".com",
            ".cpl",
            ".crt",
            ".exe",
            ".hlp",
            ".hta",
            ".inf",
            ".ins",
            ".isp",
            ".js",
            ".jse",
            ".lnk",
            ".mdb",
            ".mde",
            ".msc",
            ".msi",
            ".msp",
            ".mst",
            ".pcd",
            ".pif",
            ".reg",
            ".scr",
            ".sct",
            ".shb",
            ".shs",
            ".url",
            ".vb",
            ".vbe",
            ".vbs",
            ".vsd",
            ".vss",
            ".vst",
            ".vsw",
            ".ws",
            ".wsc",
            ".wsf",
            ".wsh",
            0 };
        for ( int i = 0; executableExts[i]; i++ ) {
            if ( strcmp( executableExts[i], ext ) == 0 ) {
				return TRUE;
            }
        }
	}
	return FALSE;
}

BOOL COpenSaveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// If we have an application disable the open button 
	if (IsExecutable(m_csFilename))
	{
		CWnd* openButton = GetDlgItem(IDOPEN);
		openButton->EnableWindow(FALSE);
		//SetTimer(2, 2000, NULL);
	}

	// Set the icon
	SHFILEINFO shfi;
	if (SHGetFileInfo(m_csFilename, 0, &shfi, sizeof(SHFILEINFO),SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_SHELLICONSIZE)) 
		m_cFileIcon.SetIcon(shfi.hIcon);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void COpenSaveDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 2)
	{
		KillTimer(nIDEvent);
		CWnd* openButton = GetDlgItem(IDOPEN);
		openButton->EnableWindow();
	}

	CDialog::OnTimer(nIDEvent);
}

void COpenSaveDlg::OnDestroy()
{
	CDialog::OnDestroy();
	DestroyIcon(m_cFileIcon.GetIcon());
}


