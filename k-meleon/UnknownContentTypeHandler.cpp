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

#include "UnknownContentTypeHandler.h"

#include "BrowserFrm.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;


NS_IMETHODIMP
CUnknownContentTypeHandler::ShowProgressDialog(nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

// Show the helper app launch confirmation dialog as instructed.
NS_IMETHODIMP
CUnknownContentTypeHandler::Show( nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {

    // we always want to download it.
    // later on, we may want to say, "hey, we have no clue how to handle this, do you want to
    // save it or open it in some application?"
   
   aLauncher->SaveToDisk(nsnull, false);

   return NS_OK;
}

// prompt the user for a file name to save the unknown content to as instructed
NS_IMETHODIMP
CUnknownContentTypeHandler::PromptForSaveToFile(nsISupports * aWindowContext, const PRUnichar * aDefaultFile, const PRUnichar * aSuggestedFileExtension, nsILocalFile ** aNewFile)
{
// change this to 0 to use the mozilla file picker
#if 0
   USES_CONVERSION;

   CString filter = W2T(aSuggestedFileExtension);
   filter += " files|*";
   filter += W2T(aSuggestedFileExtension);
   filter += "|All Files|*.*||";

   CString defaultFile = W2T(aDefaultFile);
   // unmunge the filename
   defaultFile.Replace("%20", " ");
   defaultFile.Replace("%27", "\'");
   defaultFile.Replace("%28", "(");
   defaultFile.Replace("%29", ")");
   defaultFile.Replace("%2C", ",");

   defaultFile = theApp.preferences.saveDir + defaultFile;

   CFileDialog cf(FALSE, W2T(aSuggestedFileExtension), defaultFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter, (CWnd *)theApp.m_pMostRecentBrowserFrame);
   if(cf.DoModal() == IDOK){
      NS_ENSURE_ARG_POINTER(aNewFile);

      CString pathName = cf.GetPathName();

      if (!pathName.IsEmpty()){
         nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));

         NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

         file->InitWithPath(pathName);

         NS_ADDREF(*aNewFile = file);

         // Be sure to save off the new default saveDir
         int slash = pathName.ReverseFind('\\');
         if (slash == -1) {
            pathName.ReverseFind('/');
         }
         if (slash != -1) {
            theApp.preferences.saveDir = pathName.Left(slash+1);
         }

         return NS_OK;
      }
   }

   return NS_ERROR_FAILURE;
#else

   nsresult rv = NS_OK;

   nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
//   nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance(NS_FILEPICKER_CID, &rv);
   if (filePicker)
   {
      nsAFlatString title = NS_LITERAL_STRING ("Save As:");

      nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aWindowContext ) );
      filePicker->Init(parent, title.get(), nsIFilePicker::modeSave);
      filePicker->SetDefaultString(aDefaultFile);
      nsAutoString wildCardExtension (NS_LITERAL_STRING("*").get());
      if (aSuggestedFileExtension) {
         wildCardExtension.Append(aSuggestedFileExtension);
         filePicker->AppendFilter(wildCardExtension.get(), wildCardExtension.get());
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
NS_IMPL_ISUPPORTS1(CUnknownContentTypeHandler,
                   nsIHelperAppLauncherDialog)


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


/********************************************************************************************************
file save progress dialog box
********************************************************************************************************/

// WeakReference not needed?
//NS_IMPL_ISUPPORTS3(CProgressDialog, nsIWebProgressListener, nsISupportsWeakReference, nsIDownload)
NS_IMPL_ISUPPORTS2(CProgressDialog, nsIDownload, nsIWebProgressListener)

CProgressDialog::CProgressDialog() {
   NS_INIT_ISUPPORTS();

   mFileName = NULL;
   mFilePath = NULL;

   mStartTime = 0;

   // assume we're done until we get data
   // for small files, we'll be done before the box even pops up
   mDone = true;

   Create(IDD_PROGRESS, GetDesktopWindow());

   BOOL bClose = theApp.preferences.GetBool("kmeleon.general.CloseDownloadDialog", true);
   CheckDlgButton(IDC_CLOSE_WHEN_DONE, bClose);

   theApp.RegisterWindow(this);
}

CProgressDialog::~CProgressDialog(){
   if (mFileName)
      delete mFileName;
   if (mFilePath)
      delete mFilePath;
}

/* void onStateChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aStateFlags, in unsigned long aStatus); */
NS_IMETHODIMP CProgressDialog::OnStateChange(nsIWebProgress *aWebProgress, 
                                          nsIRequest *aRequest, PRUint32 aStateFlags, 
                                          nsresult aStatus)
{
   if (aStateFlags & nsIWebProgressListener::STATE_STOP){
      if (IsDlgButtonChecked(IDC_CLOSE_WHEN_DONE)) {
         Close();
         theApp.preferences.SetBool("kmeleon.general.CloseDownloadDialog", true);
      }
      else
      {
         CString statusText;
         PRInt64 now = PR_Now();
         PRInt64 timeSpent = now - mStartTime;
         statusText.Format(IDS_DOWNLOAD_DONE, ((double)mTotalBytes)/1024, (int)(timeSpent/1000000.0l));
         SetDlgItemText(IDC_STATUS, statusText);

         SetDlgItemText(IDCANCEL, "Close");

         GetDlgItem(IDC_OPEN)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_CLOSE_WHEN_DONE)->ShowWindow(SW_HIDE);

         mDone = true;
         theApp.preferences.SetBool("kmeleon.general.CloseDownloadDialog", false);
      }
   }else if (aStateFlags & nsIWebProgressListener::STATE_REDIRECTING){
      SetDlgItemText(IDC_STATUS, "Redirecting...");
   }else if (aStateFlags & nsIWebProgressListener::STATE_TRANSFERRING){
      SetDlgItemText(IDC_STATUS, "Downloading...");
   }else if (aStateFlags & nsIWebProgressListener::STATE_NEGOTIATING){
      SetDlgItemText(IDC_STATUS, "Negotiating...");
   }else if (aStateFlags & nsIWebProgressListener::STATE_START){
      SetDlgItemText(IDC_STATUS, "Contacting...");
   }
   return NS_OK;
}

/* void onProgressChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aCurSelfProgress, in long aMaxSelfProgress, in long aCurTotalProgress, in long aMaxTotalProgress); */
NS_IMETHODIMP CProgressDialog::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress){
   if (aMaxTotalProgress && (
         PR_Now() > mLastUpdateTime + 100000.0l    // enforce a minimum delay between updates - gives a large speed increase for super-fast downloads which wasted CPU cycles updating the dialog constantly
         || aCurTotalProgress == aMaxTotalProgress // and be sure to catch the very last one, in case it would otherwise be skipped by the time check
         ) )
   {
      mLastUpdateTime = PR_Now();
      mDone = false;

      // strangely enough, this isn't a parameter in nsIDownload::Init()
      if (aMaxTotalProgress) {
         mTotalBytes = aMaxTotalProgress;
      }

      int percent = (int)(((float)aCurTotalProgress / (float)aMaxTotalProgress) * 100.0f);

      CString progressString;
      progressString.Format(IDS_DOWNLOAD_PROGRESS, percent, ((double)aCurTotalProgress)/1024, ((double)aMaxTotalProgress)/1024);
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

      char speedString[50];
      sprintf(speedString, "Speed: %.2f KBps ", speed_kbs);

      SetDlgItemText(IDC_SPEED, speedString);

		if (speed) {
         PRInt32 remaining = (PRInt32)((aMaxTotalProgress - aCurTotalProgress)/speed +.5);

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
	
         CString timeString;
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

         SetDlgItemText(IDC_TIME_LEFT, timeString);
      }

      CString titleString;
      titleString.Format(IDS_PERCENT_OF_FILE, percent, mFileName);
      SetWindowText(titleString);

      HWND progressBar;
      GetDlgItem(IDC_DOWNLOAD_PROGRESS, &progressBar);
      ::SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
      ::SendMessage(progressBar, PBM_SETPOS, (WPARAM) percent, 0);
   }
   return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI location); */
NS_IMETHODIMP CProgressDialog::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location){
   return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP CProgressDialog::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage){
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
	ON_COMMAND(WM_CLOSE, OnClose)
END_MESSAGE_MAP()

void CProgressDialog::Cancel() {
   mObserver->Observe(nsnull, "oncancel", nsnull);
   Close();
}

void CProgressDialog::Close() {
   theApp.UnregisterWindow(this);
   DestroyWindow();

   // we need launcher to call CloseProgressWindow() and be properly disposed
   nsresult rv;
   nsCOMPtr<nsIHelperAppLauncher> launcher( do_QueryInterface(mObserver, &rv) );
   if (rv == NS_OK) {
      launcher->CloseProgressWindow();
   }
}

void CProgressDialog::OnCancel() {
   if (mDone){
      Close();
   }
   else {
      Cancel();
   }
}

void CProgressDialog::OnClose() {
   if (!mDone) {
      Cancel();
   }
}

void CProgressDialog::OnOpen() {
   char *directory = strdup(mFilePath);
   char *last_slash = strrchr(directory, '\\');
   *last_slash = 0;

   ShellExecute(NULL, "open", mFilePath, "", directory, SW_SHOW);

   delete directory;
}



/* void init (in nsIURI aSource, in nsILocalFile aTarget, in wstring aDisplayName, in nsIMIMEInfo aMIMEInfo, in long long startTime, in nsIWebBrowserPersist aPersist); */
NS_IMETHODIMP CProgressDialog::Init(nsIURI *aSource, nsILocalFile *aTarget, const PRUnichar *aDisplayName, nsIMIMEInfo *aMIMEInfo, PRInt64 startTime, nsIWebBrowserPersist *aPersist)
{
	nsCAutoString uri;
   nsCAutoString filepath;

   aSource->GetSpec(uri);
	aTarget->GetNativePath(filepath);

   SetDlgItemText(IDC_SOURCE, uri.get());
   SetDlgItemText(IDC_DESTINATION, filepath.get());

   char *file = strrchr(filepath.get(), '\\')+1;
   mFileName = strdup(file);
   mFilePath = strdup(filepath.get());

   mStartTime = startTime;
   mLastUpdateTime = 0;

   return NS_OK;
}

/* readonly attribute nsIURI source; */
NS_IMETHODIMP CProgressDialog::GetSource(nsIURI * *aSource)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsILocalFile target; */
NS_IMETHODIMP CProgressDialog::GetTarget(nsILocalFile * *aTarget)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIWebBrowserPersist persist; */
NS_IMETHODIMP CProgressDialog::GetPersist(nsIWebBrowserPersist * *aPersist)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute PRInt32 percentComplete; */
NS_IMETHODIMP CProgressDialog::GetPercentComplete(PRInt32 *aPercentComplete)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute wstring displayName; */
NS_IMETHODIMP CProgressDialog::GetDisplayName(PRUnichar * *aDisplayName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CProgressDialog::SetDisplayName(const PRUnichar * aDisplayName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long long startTime; */
NS_IMETHODIMP CProgressDialog::GetStartTime(PRInt64 *aStartTime)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIMIMEInfo MIMEInfo; */
NS_IMETHODIMP CProgressDialog::GetMIMEInfo(nsIMIMEInfo * *aMIMEInfo)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIWebProgressListener listener; */
NS_IMETHODIMP CProgressDialog::GetListener(nsIWebProgressListener * *aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CProgressDialog::SetListener(nsIWebProgressListener * aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIObserver observer; */
NS_IMETHODIMP CProgressDialog::GetObserver(nsIObserver * *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CProgressDialog::SetObserver(nsIObserver * aObserver)
{
   mObserver = aObserver;
   return NS_OK;
}

 
//*****************************************************************************
// CUnknownContentHandlerFactory
//*****************************************************************************   

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
  NS_INIT_ISUPPORTS();
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
