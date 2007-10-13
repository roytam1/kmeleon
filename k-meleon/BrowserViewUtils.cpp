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

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserFrm.h"
#include "BrowserView.h"
#include "MozUtils.h"
#include "nsIWebPageDescriptor.h"
#include "nsCWebBrowserPersist.h"
#include "UnknownContentTypeHandler.h"

// Remove when it stops sucking.
//#define MOZILLA_MIMETYPE_SUCKS
#ifdef MOZILLA_MIMETYPE_SUCKS
extern BOOL GetFromTypeAndExtension(LPCTSTR contentType, LPCTSTR ext, CString& resultExt, CString& desc);
#endif

extern ParsePluginCommand(char *pszCommand, char** plugin, char **parameter);

BOOL CBrowserView::IsViewSourceUrl(CString& strUrl)
{
    return (strUrl.Find(_T("view-source:"), 0) != -1) ? TRUE : FALSE;
}

void OpenFileExternal(const char* uri, LPCTSTR file, nsresult status, void* param)
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
	nsCOMPtr<nsIWebBrowser> browser;
	m_pWindow->GetWebBrowser(getter_AddRefs(browser));
	NS_ENSURE_TRUE(browser, FALSE);

	nsresult rv;

	// Use external viewer
	if (theApp.preferences.bSourceUseExternalCommand && 
		theApp.preferences.sourceCommand) {

			CString tempfile;
			tempfile = GetTempFile();

			USES_CONVERSION;

			// We want to show the source of a local file. Just open this file.
			if ( (!pUrl && _tcsncmp((LPCTSTR)this->GetCurrentURI(), _T("file:///"), 8) == 0) ||
				 (pUrl && strncmp(pUrl, "file:///", 8) == 0))
			{
				char *url = strdup(pUrl?pUrl:T2CA(this->GetCurrentURI()));
				if (!url) return FALSE;

				unsigned int i;
				for (i=0; i<strlen(url); i++)
					if (url[i]=='/')
						url[i]='\\';

				tempfile = A2CT(nsUnescape(url+strlen("file:///")));
				OpenFileExternal("", tempfile, NS_OK,
					_tcsdup((CString)theApp.preferences.sourceCommand));

				delete url;
				return TRUE;
			}

			nsCOMPtr<nsIWebBrowserPersist> persist(do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID));
			NS_ENSURE_TRUE(persist, FALSE);

			nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(browser, &rv);
			NS_ENSURE_SUCCESS(rv, FALSE);

			nsCOMPtr<nsIURI> referrer;
			webNav->GetReferringURI(getter_AddRefs(referrer));

			nsCOMPtr<nsIURI> srcURI;
			nsCOMPtr<nsISupports> cacheDescriptor;
			if (!pUrl)
			{
				nsCOMPtr<nsIDocShell> docShell = do_GetInterface(browser, &rv);
				NS_ENSURE_SUCCESS(rv, FALSE);

				nsCOMPtr<nsIWebPageDescriptor> descriptor;
				descriptor = do_QueryInterface(docShell);
				if (descriptor)
					descriptor->GetCurrentDescriptor(getter_AddRefs(cacheDescriptor));

				rv = webNav->GetCurrentURI(getter_AddRefs(srcURI));
			}
			else
			{
				nsEmbedCString url;
				if (!IsViewSourceUrl(CString(A2CT(pUrl)))) url.Append("view-source://");
				USES_CONVERSION;
				url.Append(pUrl);
				rv = NewURI(getter_AddRefs(srcURI), nsDependentCString(pUrl));
			}

			NS_ENSURE_SUCCESS(rv, FALSE);

			nsCOMPtr<nsILocalFile> file;
#ifdef _UNICODE
			rv = NS_NewLocalFile(nsDependentString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#else
			rv = NS_NewNativeLocalFile(nsDependentCString(tempfile.GetBuffer(0)), TRUE, getter_AddRefs(file));
#endif


			CProgressDialog *progress = new CProgressDialog(FALSE);      
			progress->SetCallBack((ProgressDialogCallback)OpenFileExternal, 
				_tcsdup(theApp.preferences.sourceCommand));
			progress->InitPersist(srcURI, file, persist, FALSE);
			//			progress->InitViewer(persist, theApp.preferences.sourceCommand.GetBuffer(0), tempfile.GetBuffer(0));
			persist->SetPersistFlags(
				nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
				nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);
			rv = persist->SaveURI(srcURI, cacheDescriptor, referrer, nsnull, nsnull, file);
			if (NS_FAILED(rv)) {
				persist->SetProgressListener(nsnull);
				return FALSE;
			}

			return TRUE;
		}
   
    // use the internal viewer

    // Create a new browser frame in which we'll show the document source
    // Note that we're getting rid of the toolbars etc. by specifying
    // the appropriate chromeFlags
    PRUint32 chromeFlags =  nsIWebBrowserChrome::CHROME_WINDOW_BORDERS |
	                    nsIWebBrowserChrome::CHROME_TITLEBAR |
	                    nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                       nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                       nsIWebBrowserChrome::CHROME_WINDOW_MIN |
					   nsIWebBrowserChrome::CHROME_SCROLLBARS  ;

    RECT screen;
    SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);

    int screenWidth   = screen.right - screen.left;
    int screenHeight  = screen.bottom - screen.top;

    int x = screen.left + screenWidth / 20;
    int y = screen.top + screenHeight / 20;
    int w = 15*screenWidth / 20;
    int h = 18*screenHeight/20;

	CString url;
	nsCOMPtr<nsISupports> cacheDescriptor;
	nsCOMPtr<nsIDocShell> docShell = do_GetInterface(browser, &rv);
	NS_ENSURE_SUCCESS(rv, FALSE);

	CBrowserFrame* pFrm = CreateNewBrowserFrame(chromeFlags);
    if(!pFrm) return FALSE;
	pFrm->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER|SWP_SHOWWINDOW);    	

	if (!pUrl)
	{
		nsCOMPtr<nsIWebPageDescriptor> descriptor;
		descriptor = do_QueryInterface(docShell);
		if (descriptor)
			descriptor->GetCurrentDescriptor(getter_AddRefs(cacheDescriptor));

		if (!cacheDescriptor) url = m_pWindow->GetURI();
	}
	else
	{
		USES_CONVERSION;	
		if (IsViewSourceUrl(CString(A2CT(pUrl))))
			url = _T("view-source://");
		url += A2CT(pUrl);
	}

	// Finally, load this URI into the newly created frame
	if (cacheDescriptor) {
		pFrm->GetActiveView()->GetBrowserWrapper()->GetWebBrowser(getter_AddRefs(browser));
		NS_ENSURE_TRUE(browser, FALSE);

		nsCOMPtr<nsIDocShell> docShell = do_GetInterface(browser);
		NS_ENSURE_TRUE(docShell, FALSE);

		 nsCOMPtr<nsIWebPageDescriptor> wpd = do_QueryInterface(docShell);
		 NS_ENSURE_TRUE(wpd, FALSE);
		 wpd->LoadPage(cacheDescriptor, nsIWebPageDescriptor::DISPLAY_AS_SOURCE);
	}
	else
		pFrm->OpenURL(CString("view-source:") + url);
    
    pFrm->BringWindowToTop();

    return TRUE;
}

void CBrowserView::OpenURLWithCommand(UINT idCommand, LPCTSTR url, LPCTSTR refferer, BOOL allowFixup)
{
	switch (idCommand)
	{
        case ID_OPEN_LINK:
            OpenURL(url, refferer, allowFixup);
            break;
        case ID_OPEN_LINK_IN_BACKGROUND:
            OpenURLInNewWindow(url, refferer, TRUE, allowFixup);
            break;
        case ID_OPEN_LINK_IN_NEW_WINDOW:
            OpenURLInNewWindow(url, refferer, FALSE, allowFixup);
            break;
        default:
            OpenURL(url, refferer, allowFixup);
            return;
    }
}

void CBrowserView::OpenMultiURL(LPCTSTR urls, BOOL allowFixup)
{
    char szOpenURLcmd[80];
	int idOpen = 0, idOpenX = 0;
	const char* pref;

	if (_tcschr(urls, '\t'))
        pref = "kmeleon.general.opengroup";
	else
		pref = "kmeleon.general.openurl";

    theApp.preferences.GetString(pref, szOpenURLcmd, "");

	if (*szOpenURLcmd)
	{
		char *altCommand = strchr(szOpenURLcmd, '|');
        if (altCommand)
            *altCommand = 0;

		idOpen  = theApp.GetID(szOpenURLcmd);
		
		if (!idOpen) {
			char *plugin = NULL, *parameter = NULL;
			if (ParsePluginCommand(szOpenURLcmd, &plugin, &parameter)) {
				USES_CONVERSION;
				if (theApp.plugins.SendMessage(plugin, "* kmeleon.exe", parameter, (long)T2CA(urls), 0))
                    return;
				else
					theApp.plugins.SendMessage(plugin, "* kmeleon.exe", "DoAccel", (long) parameter, (long)&idOpen);
			}
		}

        if (altCommand)
            idOpenX = theApp.GetID(altCommand+1);
	}

    TCHAR* p = (TCHAR*)urls;
    while (p) {
        TCHAR *q = _tcschr(p, '\t');
        if (q) *q = 0;
        if (!*p) break;
		OpenURLWithCommand(idOpen, p, GetCurrentURI(), allowFixup);
	    
        idOpen = idOpenX==0 ? idOpen : idOpenX;

        if (q)
            p = q+1;
        else
            break;
    }
}

CString CBrowserView::NicknameLookup(const CString& typedUrl)
{
	CString retUrl = typedUrl;
	retUrl.TrimRight();
	retUrl.TrimLeft();
	int word = retUrl.Find(' ');
	
	if (word!=-1) {
		retUrl.GetBuffer(0);
		retUrl.ReleaseBuffer(word); // Truncate
	}

	char *nickUrl = NULL;
   USES_CONVERSION;
   theApp.plugins.SendMessage("*",	"* FindNick", "FindNick", (long)T2CA(retUrl), (long)&nickUrl);

	if (!nickUrl) return typedUrl;
		
	retUrl = A2T(nickUrl);
	free(nickUrl);

	if (word!=-1)
		if (!retUrl.Replace(_T("%s"), typedUrl.Mid(word+1))) 
			return typedUrl; // See Bug #849

	return retUrl;
} 
/*
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

void CBrowserView::OpenURL(const char* pUrl, nsIURI *refURI, BOOL allowFixup)
{
    nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_ASCII, str);
    OpenURL(str.get(), refURI, allowFixup);
}*/

void CBrowserView::OpenURL(LPCTSTR url, LPCTSTR referrer, BOOL allowFixup)
{
   //mpBrowserFrame->UpdateLocation(url, TRUE);
     
   if ( GetActiveWindow() == mpBrowserFrame &&
	   !::IsChild(m_hWnd, ::GetFocus()))
	  m_pWindow->SetActive(TRUE);

   ((CBrowserGlue*)m_pBrowserGlue)->mPendingLocation = url; // XXXX
   if (!m_pWindow->LoadURL(url, referrer, allowFixup) && m_pBrowserGlue)
	   ((CBrowserGlue*)m_pBrowserGlue)->mPendingLocation = _T("");

}
/*
void CBrowserView::OpenURL(LPCTSTR url, BOOL sendRef, BOOL allowFixup)
{
   mpBrowserFrame->UpdateLocation(url, TRUE);
     
   if ( GetActiveWindow() == mpBrowserFrame &&
	   !::IsChild(m_hWnd, ::GetFocus()))
	  m_pWindow->SetActive(TRUE);

   m_pWindow->LoadURL(url, sendRef, allowFixup);
}*/

CBrowserFrame* CBrowserView::CreateNewBrowserFrame(PRUint32 chromeMask, 
				    PRInt32 x, PRInt32 y, 
				    PRInt32 cx, PRInt32 cy,
				    PRBool bShowWindow)
{  
    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
    if(!pApp)
	return NULL;

    return pApp->CreateNewBrowserFrame(chromeMask, bShowWindow);
}
/*
CBrowserFrame* CBrowserView::OpenURLInNewWindow(const char* pUrl, BOOL bBackground, nsIURI *refURI, BOOL allowFixup)
{
	nsEmbedString str;
    NS_CStringToUTF16(nsEmbedCString(pUrl), NS_CSTRING_ENCODING_UTF8, str);
    return OpenURLInNewWindow(str.get(), bBackground, refURI, allowFixup);
}*/

CBrowserFrame* CBrowserView::OpenURLInNewWindow(LPCTSTR pUrl, LPCTSTR referrer, BOOL bBackground, BOOL allowFixup)
{
	if(!pUrl)
        return NULL; 

	if (GetCurrentURI() == "about:blank") {
		OpenURL(pUrl, referrer, allowFixup);
		return mpBrowserFrame;
	}

	return theApp.CreateNewBrowserFrameWithUrl(pUrl, referrer, bBackground);
}
/*
CBrowserFrame* CBrowserView::OpenURLInNewWindow(LPCTSTR pUrl, BOOL bBackground, BOOL sendRef, BOOL allowFixup)
{
    const TCHAR* ext = _tcschr(pUrl, L'.');
	CBrowserFrame* pFrm;
	PRUint32 chromeFlags;

	CWnd* lastBgWindow = (CWnd*)theApp.m_FrameWndLst.GetHead();

    if ( !bBackground && ext && 
         (_tcsncmp(pUrl, _T("chrome:"), 7) == 0) &&
         (_tcsstr(ext, _T(".xul")) == ext) )
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

    pFrm->OpenURL(pUrl, m_pWindow->GetURI(), allowFixup);

    if (bBackground)
		pFrm->SetWindowPos(lastBgWindow, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    else if (!(chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
    {
		pFrm->ShowWindow(SW_SHOW);
        pFrm->SetFocus();
	}

	return pFrm;
}*/

void CBrowserView::LoadHomePage()
{
   if (theApp.preferences.bStartHome)
      OnNavHome();
   else
      OpenURL(_T("about:blank"));
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

void CBrowserView::ShowSecurityInfo()                                           
{
	CString title, msg;
	title.LoadString(IDS_SECURITY_INFORMATION);
	if (!m_pWindow->ShowCertificate()) {
		msg.LoadString(IDS_NOT_SECURE);
		::MessageBox(m_hWnd, msg, title, MB_OK);
	}
/*
	if(m_pBrowserGlue->m_SecurityState == SECURITY_STATE_INSECURE) {
		msg.LoadString(IDS_NOT_SECURE);
		::MessageBox(m_hWnd, msg, title, MB_OK);
	} else {
		if (!m_pWindow->ShowCertificate()) {
			msg.LoadString(IDS_SECURITY_FAILED);
			::MessageBox(GetParentFrame()->m_hWnd, msg, title, MB_OK);
		}
	}*/
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

/*nsIDOMNode *CBrowserView::GetNodeAtPoint(int x, int y, BOOL bPrepareMenu)
{
	// Make sure a page is actually being displayed
	if (m_pWindow->GetURI().IsEmpty())
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
}*/