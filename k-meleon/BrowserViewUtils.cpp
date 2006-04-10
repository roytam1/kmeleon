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

#include "UnknownContentTypeHandler.h"
#include "nsCWebBrowserPersist.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserFrm.h"
#include "BrowserView.h"

extern nsresult NewURI(nsIURI **result, const nsAString &spec);

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
		tempfile = url+strlen("view-source:file:///");
	    }
	    
	    nsCOMPtr<nsIWebBrowserPersist> persist(do_QueryInterface(mWebBrowser));
	    if(persist)
	    {
	        char *tmp = strdup(nsUnescape(tempfile.GetBuffer(0)));
                tempfile = tmp;
                free(tmp);

		nsCOMPtr<nsILocalFile> file;
		NS_NewNativeLocalFile(nsDependentCString(T2A(tempfile.GetBuffer(0))), TRUE, getter_AddRefs(file));
		
		CProgressDialog *progress = new CProgressDialog(FALSE);      
		progress->InitViewer(persist, theApp.preferences.sourceCommand.GetBuffer(0), tempfile.GetBuffer(0));
		
		nsEmbedString sURI;
		NS_CStringToUTF16(nsEmbedCString(pUrl+strlen("View-Source:")), NS_CSTRING_ENCODING_ASCII, sURI);
		
		nsCOMPtr<nsIURI> srcURI;
		nsresult rv = NewURI(getter_AddRefs(srcURI), sURI);
		if (NS_FAILED(rv)) {
		    if (url)
			delete url;
		    return FALSE;
		}
 
		persist->SaveURI(srcURI, nsnull, nsnull, nsnull, nsnull, file);
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
	                    nsIWebBrowserChrome::CHROME_WINDOW_RESIZE;

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
   mpBrowserFrameGlue->GetBrowserFrameTitle(&aTitle);
   title = W2T(aTitle);
}

BOOL MultiSave(nsIURI* aURI, nsILocalFile* file) {
   nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
   if(!persist)
      return FALSE;
    
   CProgressDialog *progress = new CProgressDialog(FALSE);
   progress->InitPersist(aURI, file, persist, TRUE);
   persist->SaveURI(aURI, nsnull, nsnull, nsnull, nsnull, file);
   return TRUE;
}


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

void CBrowserView::OpenURL(const char* pUrl, nsIURI *refURI)
{
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    OpenURL(str.get(), refURI);
}

void CBrowserView::OpenURL(const PRUnichar* pUrl, nsIURI *refURI)
{
   CString str = pUrl;
   mpBrowserFrame->m_wndUrlBar.SetCurrentURL((char*)str.GetBuffer(0));
   mpBrowserFrame->m_wndUrlBar.EditChanged(FALSE);

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
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    return OpenURLInNewWindow(str.get(), bBackground, refURI);
}

CBrowserFrame* CBrowserView::OpenURLInNewWindow(const PRUnichar* pUrl, BOOL bBackground, nsIURI *refURI)
{
    if(!pUrl)
        return NULL; 
   
    // create hidden window
    CBrowserFrame* pFrm = CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, -1, -1, -1, -1, PR_FALSE);
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
	if (theApp.preferences.bMaximized)
	    pFrm->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	else  
	    pFrm->ShowWindow(SW_MINIMIZE);
    }
   
    // show the window
    else
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
	nsCOMPtr<nsISHistoryInternal> shi;
		
	mWebNav->GetSessionHistory(getter_AddRefs(oldSH));
	newWebBrowser.mWebNav->GetSessionHistory(getter_AddRefs(newSH));
	shi = do_QueryInterface(newSH);
	if (shi)
	{
		nsCOMPtr<nsISHEntry> she;
		nsCOMPtr<nsIHistoryEntry> he;

		PRInt32 count;
		oldSH->GetCount(&count);
		for (int i=0;i<count;i++)
		{
	        nsCOMPtr<nsISHEntry> sheN;
			oldSH->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs(he));

			she = do_QueryInterface(he);
			if (she) {
				she->Clone(getter_AddRefs(sheN));
				if (sheN) shi->AddEntry(sheN, PR_TRUE);
			}
		}

		if (count>0)
		{
			PRInt32 index;
			oldSH->GetIndex(&index);
			newWebBrowser.mWebNav->GotoIndex(index);
		}
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

void CBrowserView::ShowSecurityInfo()                                           
{
    HWND hParent = mpBrowserFrame->m_hWnd;

    if(m_SecurityState == SECURITY_STATE_INSECURE) { 
	::MessageBox(m_hWnd, "This page has not been transferred over a secure connection.", "Security Information", MB_OK);
    } else {
	// TEMPORARY.  this should be replaced with something more permanent
	::MessageBox(m_hWnd, "This page has been transferred over a secure connection.", "Security Information", MB_OK);
    }
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
	nsEmbedCString uriString;
	rv = currentURI->GetSpec(uriString);
	if(NS_FAILED(rv))
		return 0;

   int len = strlen(uriString.get());
   if (sURI)
      strcpy(sURI, uriString.get());
   return len;
}

_GetSelection(nsIDOMWindow* dom, CString& aSelText)
{
	nsCOMPtr<nsISelection> sel;
	dom->GetSelection(getter_AddRefs(sel));
	if (sel) {
		USES_CONVERSION;
		
		PRUnichar* selText;
		nsresult rv = sel->ToString(&selText);
		NS_ENSURE_SUCCESS(rv, FALSE);

		aSelText = W2CT(selText);
		nsMemory::Free(selText);
		if (aSelText.GetLength()>0)
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

BOOL CBrowserView::GetSelection(CString& aSelText)
{
	nsresult rv;

	nsCOMPtr<nsIDOMWindow> dom(do_GetInterface(mWebBrowser));
	NS_ENSURE_TRUE(dom, FALSE);
	return _GetSelection(dom, aSelText);
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
