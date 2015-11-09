/*
*  Copyright (C) 2001 Jeff Doozan
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

#include "stdafx.h"
#include "HiddenWnd.h"
#include "BrowserView.h" //XX
#include "SaveAsHandler.h"

BEGIN_MESSAGE_MAP(CHiddenWnd, CFrameWnd)
	//{{AFX_MSG_MAP(CHiddenWnd)
   ON_WM_CREATE()
   ON_WM_CLOSE()
   ON_WM_COPYDATA()
   ON_WM_ENDSESSION()
   ON_MESSAGE(UWM_NEWWINDOW, OnNewWindow)
   ON_MESSAGE(UWM_PERSIST_SET, OnSetPersist)
   ON_MESSAGE(UWM_PERSIST_SHOW, OnShowBrowser)
   ON_MESSAGE(WM_DEFERSHOW, OnDeferShow)
   ON_MESSAGE(WM_DEFERSAVEAS, OnDeferSaveAs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CHiddenWnd::PreCreateWindow(CREATESTRUCT& cs) {

   if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
   cs.lpszClass = HIDDEN_WINDOW_CLASS;

   return TRUE;
}

int CHiddenWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {

   // Check if the tray control is running, and get the persist setting from it
   QueryPersistFlags();
   USES_CONVERSION;
   if (m_bStayResident)
      StayResident();
   else {
      BOOL webapp = theApp.cmdline.GetSwitch("-webapp", NULL, TRUE)>=0;
      if (!ShowBrowser(A2T(theApp.cmdline.m_sCmdLine), webapp, TRUE))
		  return -1;
   }

   return CFrameWnd::OnCreate(lpCreateStruct);
}

void CHiddenWnd::QueryPersistFlags() {
   HWND hwndLoader = ::FindWindowEx(NULL, NULL, _T("KMeleon Tray Control"), NULL);
   if (hwndLoader) {
      LRESULT flags = ::SendMessage(hwndLoader, UWM_PERSIST_GET, NULL, NULL);

      m_bStayResident      = ((flags & PERSIST_BROWSER));
      m_bPreloadWindow     = ((flags & PERSIST_WINDOW));
      m_bPreloadStartPage  = ((flags & PERSIST_STARTPAGE));
      m_bShowNow           = ((flags & PERSIST_SHOWNOW));

      // a little sanity checking
      if (m_bPreloadWindow && !m_bStayResident)
         m_bPreloadWindow = FALSE;
      if (m_bPreloadStartPage && ! m_bPreloadWindow)
         m_bPreloadStartPage = FALSE;
   }
   else {
      m_bStayResident      = FALSE;
      m_bPreloadWindow     = FALSE;
      m_bPreloadStartPage  = FALSE;
      m_bShowNow           = FALSE;
   }
}

void CHiddenWnd::OnClose() {
   // if we're not staying resident, call the default handler, which will exit
   if (!m_bStayResident) {
      CFrameWnd::OnClose();
      return;
   }

   // make sure the loader hasn't exited without notifying us
   HWND hwndLoader = ::FindWindowEx(NULL, NULL, _T("KMeleon Tray Control"), NULL);
   if (!hwndLoader) {
      CFrameWnd::OnClose();
      return;
   }

   // else close all the browser windows and stay resident
   CBrowserFrame* pBrowserFrame = NULL;

   POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
   while( pos != NULL ) {
      pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
      if(pBrowserFrame)	{
         pBrowserFrame->ShowWindow(false);
         pBrowserFrame->DestroyWindow();
      }
   }
   theApp.m_FrameWndLst.RemoveAll();
   StayResident();
}

LRESULT CHiddenWnd::OnSetPersist(WPARAM flags, LPARAM lParam) {
   BOOL bNewStayResident = (flags & PERSIST_BROWSER);
   BOOL bNewPreloadWindow = (flags & PERSIST_WINDOW);
   BOOL bNewPreloadStartPage = (flags & PERSIST_STARTPAGE);

   // a little sanity checking
   if (bNewPreloadWindow && !bNewStayResident)
      bNewPreloadWindow = FALSE;
   if (bNewPreloadStartPage && ! bNewPreloadWindow)
      bNewPreloadStartPage = FALSE;


   // update the hidden window with the new settings
   if (m_bPersisting) {

      // exit kmeleon if the stay resident flag is cleared
      if (!bNewStayResident)
         PostMessage(WM_QUIT);
/*
      // preload the window
      if (bNewPreloadWindow && !m_bPreloadWindow) {
         m_pHiddenBrowser = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL,
                                                     -1, -1, -1, -1, PR_FALSE);
         if (bNewPreloadStartPage)
            m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
         else
            m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");
      }

      // don't preload the window
      if (!bNewPreloadWindow && m_bPreloadWindow) {
         m_pHiddenBrowser->DestroyWindow();
         POSITION pos = theApp.m_FrameWndLst.Find(m_pHiddenBrowser);
         theApp.m_FrameWndLst.RemoveAt(pos);
      }

      // preload the start page
      if (bNewPreloadStartPage && !m_bPreloadStartPage)
         m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();

      // don't preload the start page
      if (bNewPreloadWindow && !bNewPreloadStartPage && m_bPreloadStartPage)
         m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");*/
   }

   m_bStayResident = bNewStayResident;
   m_bPreloadWindow = bNewPreloadWindow;
   m_bPreloadStartPage = bNewPreloadStartPage;

   return 0;
}

LRESULT CHiddenWnd::OnShowBrowser(WPARAM URI, LPARAM lParam) {
   USES_CONVERSION;
   ShowBrowser(A2T((char*)URI));

   return 0;
}

BOOL CHiddenWnd::ShowBrowser(LPTSTR URI, BOOL webapp, BOOL atStart) {

   // if we already have a browser, load home page (if necessary), and show the window
  /* if (m_bPersisting && m_bPreloadWindow) {
      if (URI && *URI) {
         if (*URI == _T('\"')) URI++;
         int len = _tcslen(URI);
         if (URI[len-1] == _T('\"')) URI[len-1] = 0;
         m_pHiddenBrowser->m_wndBrowserView.OpenURL(URI);
      }
      else {
         m_pHiddenBrowser->SetFocus();
         // m_pHiddenBrowser->m_wndUrlBar.MaintainFocus();
         if (!m_bPreloadStartPage)
            m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
      }

      if (theApp.preferences.bMaximized) m_pHiddenBrowser->ShowWindow(SW_MAXIMIZE);
      else m_pHiddenBrowser->ShowWindow(SW_SHOW);
      m_pHiddenBrowser->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
   }

   // otherwise, just create a new browser
   else */{

      // If we already have a browser and no url, do nothing.
      // This is to accomodate the session plugin
      if ( atStart && (!URI || !*URI) && theApp.m_pMostRecentBrowserFrame)
          return TRUE;

	  int openmode = theApp.preferences.GetInt("browser.link.open_external", 2);
	  CBrowserFrame* browser = NULL;
	  CBrowserWrapper* wrapper = NULL;
	  if (webapp) {
		  PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_WINDOW_RESIZE |
                     nsIWebBrowserChrome::CHROME_WINDOW_CLOSE |
                     nsIWebBrowserChrome::CHROME_TITLEBAR |
                     nsIWebBrowserChrome::CHROME_WINDOW_MIN;
		  browser = theApp.CreateNewBrowserFrame(chromeMask, FALSE, NULL);
	  } else if (openmode == 2 || !theApp.m_pMostRecentBrowserFrame ) {
		browser = theApp.CreateNewBrowserFrame();
		wrapper = browser->GetActiveView()->GetBrowserWrapper();
		//browser->ShowWindow(SW_SHOW);
	  } else  {
		  if (theApp.m_pMostRecentBrowserFrame) {
			  browser = theApp.m_pMostRecentBrowserFrame;
			  CBrowserGlue* glue = browser->GetActiveView()->GetBrowserGlue();			  
			  if (glue) {
				 wrapper = glue->ReuseWindow(openmode == 1);
                 if (!wrapper) {
					 browser = theApp.CreateNewBrowserFrame(); //XXXXXX
					 wrapper = browser->GetActiveView()->GetBrowserWrapper();
				 }
			  }
		  }
	  }

      if (!wrapper) {
         AfxMessageBox(IDS_FAILED_TO_CREATE_BROWSER);
         return FALSE;
      }

      if (URI && *URI) {
         // if the URI is in quotes, strip them off
         int len = _tcslen(URI);
         if (URI[0] == _T('"')) {
			 if (URI[len-1] == _T('"')) URI[len-1] = 0;
			 URI++;
         }
         //browser->OpenURL(URI);
		 wrapper->LoadURL(URI);
      }
	  else {
		  // browser->m_wndUrlBar.MaintainFocus();
		  //browser->GetActiveView()->LoadHomePage();		  
		  CString homePage = theApp.preferences.bStartHome ? theApp.preferences.GetString("browser.startup.homepage", _T("about:home")) : _T("about:blank");
		  wrapper->LoadURL(homePage);
	  }
	  
	  // Bullshit: the first window in never maximized
      if (atStart && theApp.preferences.bMaximized)
         browser->ShowWindow(SW_SHOWMAXIMIZED);
	  else 
         browser->ShowWindow(SW_SHOW);
	  browser->SetForegroundWindow();
   }

   m_bPersisting = FALSE;
   return TRUE;
}

int CHiddenWnd::Persisting() {
   if (m_bPersisting)
      return PERSIST_STATE_PERSISTING;
   if (m_bStayResident)
      return PERSIST_STATE_ENABLED;
   return PERSIST_STATE_DISABLED;
}


BOOL CHiddenWnd::StayResident() {

   // if the ShowNow flag is set, we're not really going to stay resident
   if (m_bShowNow) {
      m_bShowNow = FALSE;
      m_bPersisting = FALSE;

      ShowBrowser();
   }

   else {
      m_bPersisting = TRUE;
/*
      if (m_bPreloadWindow) {
         m_pHiddenBrowser = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL,
                                                     -1, -1, -1, -1, PR_FALSE);
         if (!m_pHiddenBrowser)
            return FALSE;

         if (m_bPreloadStartPage)
            m_pHiddenBrowser->m_wndBrowserView.LoadHomePage();
         else
            m_pHiddenBrowser->m_wndBrowserView.OpenURL("about:blank");
      }*/
   }

   return TRUE;
}



// This is called from another instance of Kmeleon (via the UWM_NEWWINDOW message),
// when no command line paramaters have been specified
LRESULT CHiddenWnd::OnNewWindow(WPARAM wParam, LPARAM lParam) {
   ShowBrowser();

   return 0;
}

// This is called from another instance of Kmeleon,
// and contains any command line parameters specified
BOOL CHiddenWnd::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) {
   
   CCmdLine cmdline;
   USES_CONVERSION;
   cmdline.Initialize((LPSTR) pCopyDataStruct->lpData);
   BOOL webapp = cmdline.GetSwitch("-webapp", NULL, TRUE)>=0;
   ShowBrowser(A2T(cmdline.m_sCmdLine), webapp);

   return true;
}

////////////////////////////////////////////////////////////////////////
// Lame download defering

#include "UnknownContentTypeHandler.h"

LRESULT CHiddenWnd::OnDeferShow(WPARAM wParam, LPARAM lParam)
{
	if (!lParam) return 0;
	return NS_SUCCEEDED(((CUnknownContentTypeHandler*)lParam)->Show((CWnd*)wParam));
}

LRESULT CHiddenWnd::OnDeferSaveAs(WPARAM wParam, LPARAM lParam)
{
	if (!lParam || !wParam) return 0;
	CSaveAsHandler* handler = (CSaveAsHandler*)wParam;

	nsresult rv = handler->Save((char*)lParam);
	NS_RELEASE(handler);
	free((char*)lParam);
	return NS_SUCCEEDED(rv);
}

#include "nsXULAppAPI.h"
extern XRE_TermEmbeddingType XRE_TermEmbedding;
void CHiddenWnd::OnEndSession(BOOL bEnding)
{
   if (!bEnding) return;
   theApp.plugins.SendMessage("*", "* Plugin Manager", "Quit");
   theApp.favicons.WriteCache();
   theApp.preferences.Flush();
   XRE_TermEmbedding();
   //theApp.plugins.UnLoadAll();
}