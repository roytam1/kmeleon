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

#include "UnknownContentTypeHandler.h"

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

         CString tempfile;
         tempfile = GetTempFile();

         nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
         if(persist)
         {
            nsCOMPtr<nsILocalFile> file;
            NS_NewNativeLocalFile(nsDependentCString(T2A(tempfile.GetBuffer(0))), TRUE, getter_AddRefs(file));

            CProgressDialog *progress = new CProgressDialog(FALSE);      
            progress->InitViewer(persist, theApp.preferences.sourceCommand.GetBuffer(0), tempfile.GetBuffer(0));

            nsAutoString sURI;
            sURI.AssignWithConversion(pUrl+strlen("View-Source:"));

         	nsCOMPtr<nsIURI> srcURI;
	         nsresult rv = NS_NewURI(getter_AddRefs(srcURI), sURI);
	         if (NS_FAILED(rv)) 
		         return FALSE;
 
            persist->SaveURI(srcURI, nsnull, file);
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
	CBrowserFrame* pFrm = CreateNewBrowserFrame(); //chromeFlags);
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
	nsCAutoString path;
	aURI->GetPath(path);

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




      // Get the persist interface that we'll use for saving the file(s)
      nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
      if(!persist)
         return NS_ERROR_FAILURE;

      nsString filename;
      filename.AssignWithConversion(strFullPath.GetBuffer(0));

      nsCOMPtr<nsILocalFile> file;
      NS_NewNativeLocalFile(nsDependentCString(T2A(strFullPath.GetBuffer(0))), TRUE, getter_AddRefs(file));

      CProgressDialog *progress = new CProgressDialog(FALSE);
      
      progress->InitPersist(aURI, file, persist, TRUE);

      persist->SaveURI(aURI, nsnull, file);

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
   CBrowserFrame* pFrm = CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, PR_FALSE);
	if(!pFrm)
		return;

   // Load the URL into it...

	// Note that OpenURL() is overloaded - one takes a "char *"
	// and the other a "PRUniChar *". We're using the "PRUnichar *"
	// version here 

   pFrm->m_wndBrowserView.OpenURL(pUrl);

   // show the window
   if (bBackground)
      pFrm->ShowWindow(SW_MINIMIZE);
//      pFrm->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
         /* Show the window minimized, instead of on the bottom, because mozilla freaks out if we put it on the bottom */

   else
      pFrm->ShowWindow(SW_SHOW);
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


   if (m_InPrintPreview) {
      nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
      if(print) {
         PRBool isDoingPP;
         print->GetDoingPrintPreview(&isDoingPP);
         if (!isDoingPP) {
            m_InPrintPreview = FALSE;
            CMenu* menu = mpBrowserFrame->GetMenu();
            if (menu) {
               menu->CheckMenuItem( ID_FILE_PRINTPREVIEW, MF_UNCHECKED );
            }
         }
      }
   }
   
   

   if (mpBrowserFrame->m_wndAnimate) {
      if (mbDocumentLoading){
		   mpBrowserFrame->m_wndAnimate.Play(0, -1, -1);
	   }
	   else {
		   mpBrowserFrame->m_wndAnimate.Stop();
		   mpBrowserFrame->m_wndAnimate.Seek(0);
      }
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

void CBrowserView::SetCurrentFrameURL(nsAutoString& strCurrentFrameURL)
{
	mCtxMenuCurrentFrameURL = strCurrentFrameURL;
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
	nsCAutoString uriString;
	rv = currentURI->GetSpec(uriString);
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
      ::MessageBox(m_hWnd, "This page has been NOT been transferred over a secure connection.", "Security Information", MB_OK);
   } else {
      // TEMPORARY.  this should be replaced with something more permanent
      ::MessageBox(m_hWnd, "This page has been transferred over a secure connection.", "Security Information", MB_OK);
   }
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

   m_iGetNodeHack = bPrepareMenu ? 2 : 1;

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
   
   ::SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(x, y));
   ::SetCursorPos(pt.x, pt.y);
   ::ShowCursor(TRUE);

   return m_pGetNode;

}