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

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserFrm.h"
#include "BrowserView.h"

BOOL CBrowserView::IsViewSourceUrl(CString& strUrl)
{
   return (strUrl.Find("view-source:", 0) != -1) ? TRUE : FALSE;
}

BOOL CBrowserView::OpenViewSourceWindow(const char* pUrl)
{
   // Use external viewer
   if (theApp.preferences.bSourceUseExternalCommand) {
      if (theApp.preferences.sourceCommand) {

         char *tempfile = GetTempFile();

         nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
         if(persist)
         {
            nsCOMPtr<nsILocalFile> file;
            NS_NewLocalFile(T2A(tempfile), TRUE, getter_AddRefs(file));


            persist->SaveDocument(nsnull, file, nsnull);

            char *command = new char[theApp.preferences.sourceCommand.GetLength() + strlen(tempfile) +2];
            
            strcpy(command, theApp.preferences.sourceCommand);
            strcat(command, " ");                              //append " filename" to the viewer command
            strcat(command, tempfile);
            
            STARTUPINFO si = { 0 };
            PROCESS_INFORMATION pi;
            si.cb = sizeof STARTUPINFO;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOW;

            CreateProcess(0,command,0,0,0,0,0,0,&si,&pi);      // launch external viewer
         }
         return TRUE;
      }
   }
   
   // use the internal viewer

	// Create a new browser frame in which we'll show the document source
	// Note that we're getting rid of the toolbars etc. by specifying
	// the appropriate chromeFlags
	PRUint32 chromeFlags =  nsIWebBrowserChrome::CHROME_WINDOW_BORDERS |
							nsIWebBrowserChrome::CHROME_TITLEBAR |
							nsIWebBrowserChrome::CHROME_WINDOW_RESIZE;
	CBrowserFrame* pFrm = CreateNewBrowserFrame(chromeFlags);
	if(!pFrm)
		return FALSE;

	// Finally, load this URI into the newly created frame
	pFrm->m_wndBrowserView.OpenURL(pUrl);

   pFrm->BringWindowToTop();

   return TRUE;
}                                                                               

void CBrowserView::RefreshToolBarItem(WPARAM ItemID, LPARAM unused)
{
	switch (ItemID) {
		case ID_NAV_BACK:
			m_refreshBackButton = TRUE;
			break;
		case ID_NAV_FORWARD:
			m_refreshForwardButton = TRUE;
			break;
	}
}

void CBrowserView::GetPageTitle(CString& title)
{
   USES_CONVERSION;

   PRUnichar *aTitle;
   mpBrowserFrameGlue->GetBrowserFrameTitle(&aTitle);
   title = W2A(aTitle);
}

NS_IMETHODIMP CBrowserView::URISaveAs(nsIURI* aURI, bool bDocument)
{

   NS_ENSURE_ARG_POINTER(aURI);

	// Get the "path" portion (see nsIURI.h for more info
	// on various parts of a URI)
	nsXPIDLCString path;
	aURI->GetPath(getter_Copies(path));

   char sDefault[] = "default.htm";
   char *pFileName = sDefault;
   char *pBuf = NULL;

   if (strlen(path.get()) > 1) {
	   // The path may have the "/" char in it - strip those
	   pBuf = new char[strlen(path.get()) + 5];      // +5 for ".htm" to be safely appended, if necessary
      strcpy(pBuf, path.get());
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
   	aURI->GetHost(getter_Copies(path));
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
      strcat(lpszFilter,"Text File (*.txt)|*.txt|"
        "All Files (*.*)|*.*||");
  
   CFileDialog cf(FALSE, extension, pFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					lpszFilter, this);

   cf.m_ofn.lpstrInitialDir = theApp.preferences.saveDir;

   if(cf.DoModal() == IDOK) {
		CString strFullPath = cf.GetPathName();            // Will be like: c:\tmp\junk.htm
      theApp.preferences.saveDir = cf.GetPathName();
      int idxSlash;
      idxSlash = theApp.preferences.saveDir.ReverseFind('\\');
      theApp.preferences.saveDir = theApp.preferences.saveDir.Mid(0, idxSlash+1);
		char *pStrFullPath = strFullPath.GetBuffer(0);     // Get char * for later use
		
		CString strDataPath; 
		char *pStrDataPath = NULL;
      if (bDocument && (cf.m_ofn.nFilterIndex == 3)) {

         // cf.m_ofn.nFilterIndex == 3 indicates that the
			// user wants to save the complete document including
			// all frames, images, scripts, stylesheets etc.

			int idxPeriod = strFullPath.ReverseFind('.');
			strDataPath = strFullPath.Mid(0, idxPeriod);
			strDataPath += "_files";

			// At this stage strDataPath will be of the form
			// c:\tmp\junk_files - assuming we're saving to a file
			// named junk.htm
			// Any images etc in the doc will be saved to a dir
			// with the name c:\tmp\junk_files

			pStrDataPath = strDataPath.GetBuffer(0); //Get char * for later use
		}

      // Save the file
      nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
      if(persist)
      {
         nsCOMPtr<nsILocalFile> file;
         NS_NewLocalFile(T2A(pStrFullPath), TRUE, getter_AddRefs(file));

         nsCOMPtr<nsILocalFile> dataPath;
         if (pStrDataPath)
         {
            NS_NewLocalFile(pStrDataPath, TRUE, getter_AddRefs(dataPath));
         }

         if (bDocument)
            persist->SaveDocument(nsnull, file, dataPath);
         else
            persist->SaveURI(aURI, nsnull, file);
      }
	}

   if (pBuf)
      delete pBuf;

   return NS_OK;
}

void CBrowserView::OpenURL(const char* pUrl)
{
   OpenURL(NS_ConvertASCIItoUCS2(pUrl).get());                                 
}

void CBrowserView::OpenURL(const PRUnichar* pUrl)
{
   mWebNav->LoadURI(pUrl,                              // URI string
                    nsIWebNavigation::LOAD_FLAGS_NONE, // Load flags
                    nsnull,                            // Refering URI
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

void CBrowserView::OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground)
{
	if(!pUrl)
		return;

   // create hidden window
   CBrowserFrame* pFrm = CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, false);
	if(!pFrm)
		return;

   // show the window
   if (bBackground)
         pFrm->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
   else
      pFrm->ShowWindow(SW_SHOW);

   pFrm->UpdateWindow();
   
   // Load the URL into it...

	// Note that OpenURL() is overloaded - one takes a "char *"
	// and the other a "PRUniChar *". We're using the "PRUnichar *"
	// version here

	pFrm->m_wndBrowserView.OpenURL(pUrl);
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
	mbDocumentLoading = aBusy;

	if (mbDocumentLoading){
		mpBrowserFrame->m_wndAnimate.Play(0, -1, -1);
	}
	else {
		mpBrowserFrame->m_wndAnimate.Stop();
		mpBrowserFrame->m_wndAnimate.Seek(0);
  }
}

void CBrowserView::SetCtxMenuLinkUrl(nsAutoString& strLinkUrl)
{
	mCtxMenuLinkUrl = strLinkUrl;
}

void CBrowserView::SetCtxMenuImageSrc(nsAutoString& strImgSrc)
{
	mCtxMenuImgSrc = strImgSrc;
}

char * CBrowserView::GetTempFile()
{
   m_tempFileCount++;
   
   char ** newFileList = new char*[m_tempFileCount];                             // create new index

   memcpy(newFileList, m_tempFileList, ((m_tempFileCount-1)*sizeof(char**)) );   // copy old index

   if (m_tempFileCount>1) delete m_tempFileList;                                 // delete old index
   m_tempFileList = newFileList;

   char *newFile = new char[MAX_PATH];
 
   char temppath[MAX_PATH];
   GetTempPath(MAX_PATH, temppath);
   GetTempFileName(temppath, "kme", 0, newFile);                                 // create tempfile name
   
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

int CBrowserView::GetCurrentURI(char *sURI)
{
   if(! mWebNav)
		return 0;

	nsresult rv = NS_OK;
   
   nsCOMPtr<nsIURI> currentURI;
	rv = mWebNav->GetCurrentURI(getter_AddRefs(currentURI));
	if(NS_FAILED(rv) || !currentURI)
      return 0;

   // Get the uri string associated with the nsIURI object
	nsXPIDLCString uriString;
	rv = currentURI->GetSpec(getter_Copies(uriString));
	if(NS_FAILED(rv))
		return 0;

   int len = strlen(uriString.get());
   if (sURI)
      strcpy(sURI, uriString.get());
   return len;
}

void CBrowserView::ShowSecurityInfo()                                           
{
   HWND hParent = mpBrowserFrame->m_hWnd;

   if(m_SecurityState == SECURITY_STATE_INSECURE) { 
      AfxMessageBox(IDS_NOSECURITY_INFO);
   } else {
      // TEMPORARY.  this should be replaced with something more permanent
      AfxMessageBox("This page has been transferred over a secure connection.");
   }
}
