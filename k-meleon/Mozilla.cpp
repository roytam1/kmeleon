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

#include "stdafx.h"
#include "KMeleon.h"
#include "KMeleonDoc.h"
#include "nsEmbedAPI.h"
#include "MainFrm.h"
#include "Mozilla.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsISHistory.h"
#include "nsIScrollable.h"
#include "nsIContentViewerEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_APPCOMMAND     0x319
#define WM_XBUTTONBACK		1
#define WM_XBUTTONFORWARD	2

/////////////////////////////////////////////////////////////////////////////
// CMozilla

IMPLEMENT_DYNCREATE(CMozilla, CView)

CMozilla::CMozilla()
{
	chrome = NULL;
	mWebNav = NULL;
	mBrowser = NULL;
  last_link[0] = 0;
	panning = 0;
}

CMozilla::~CMozilla()
{
	if(chrome) 
	{
		mWebNav->Stop();
// FIXME:		mRootDocShell->StopLoad();
// FUCKO: needs to stop all loading before calling delete otherwise it crashes
//		delete(chrome);
	}
}


BEGIN_MESSAGE_MAP(CMozilla, CWnd)
	//{{AFX_MSG_MAP(CMozilla)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_GO_BACK, OnGoBack)
	ON_UPDATE_COMMAND_UI(ID_GO_BACK, OnUpdateGoBack)
	ON_COMMAND(ID_GO_FORWARD, OnGoForward)
	ON_UPDATE_COMMAND_UI(ID_GO_FORWARD, OnUpdateGoForward)
	ON_COMMAND(ID_VIEW_STOP, OnViewStop)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STOP, OnUpdateViewStop)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH, OnUpdateViewRefresh)
	ON_WM_MOUSEACTIVATE()
	ON_WM_TIMER()
	ON_COMMAND(ID_POPUP_OPEN, OnLinkOpen)
	ON_COMMAND(ID_POPUP_OPENINANEWWINDOW, OnLinkOpenNewWindow)
	ON_COMMAND(ID_POPUP_COPYSHORTCUT, OnPopupCopyShortcut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMozilla message handlers

void CMozilla::OnDraw(CDC* pDC)
{
	CDocument *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

// utility function
int CMozilla::ResizeEmbedding()
{
   if (!chrome)
       return 0;
   
   nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(chrome);
   
   HWND hWnd;
   baseWindow->GetParentNativeWindow((void**)&hWnd);
   
   if (!hWnd)
       return 0;
    RECT rect;
	::GetClientRect(hWnd, &rect);
   baseWindow->SetPositionAndSize(rect.left, 
                                  rect.top, 
                                  rect.right - rect.left, 
                                  rect.bottom - rect.top,
                                  PR_FALSE);
   
   baseWindow->SetVisibility(PR_TRUE);
   return 1;
}

int CMozilla::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void *CMozilla::createBrowser()
{
	chrome=new WebBrowserChrome(m_hWnd,(CMainFrame *)GetParentFrame());

	NS_ADDREF(chrome); // native window will hold the addref.

  chrome->CreateBrowserWindow(0, -1, -1, -1, -1, &mBrowser);

  if (!mBrowser)
     return NULL;

	nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mBrowser));
	mWebNav = webNav;

	ResizeEmbedding();

	return mBrowser;
}

void CMozilla::OnSize(UINT nType, int cx, int cy) 
{
	ResizeEmbedding();
}

int CMozilla::Navigate(CString *url)
{
  if(mWebNav) mWebNav->LoadURI(NS_ConvertASCIItoUCS2(url->GetBuffer(1024)).GetUnicode(),nsIWebNavigation::LOAD_FLAGS_NONE);
	return 0;
}

void CMozilla::OnGoBack() 
{
  if(mWebNav){
    // BHarris - if you don't stop first, pages get overlaid
    mWebNav->Stop();
    mWebNav->GoBack();
  }
}

void CMozilla::OnUpdateGoBack(CCmdUI* pCmdUI) 
{
	if(mWebNav)
	{
		PRBool a;
		mWebNav->GetCanGoBack(&a);
		pCmdUI->Enable (a);
	}
}

void CMozilla::OnGoForward() 
{
  if(mWebNav){
    // BHarris - if you don't stop first, pages get overlaid
    mWebNav->Stop();
    mWebNav->GoForward();	
  }
}

void CMozilla::OnUpdateGoForward(CCmdUI* pCmdUI) 
{
	if(mWebNav)
	{
		PRBool a;
		mWebNav->GetCanGoForward(&a);
		pCmdUI->Enable (a);
	}
}

void CMozilla::OnViewStop() 
{
	if(mWebNav) mWebNav->Stop();
}

void CMozilla::OnUpdateViewStop(CCmdUI* pCmdUI) 
{
}

void CMozilla::OnViewRefresh() 
{
  if(mWebNav) mWebNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
}

void CMozilla::OnUpdateViewRefresh(CCmdUI* pCmdUI) 
{
}

BOOL CMozilla::OnEraseBkgnd(CDC* pDC) 
{
	if(chrome && !((WebBrowserChrome *)chrome)->working) return 0;
  return CWnd::OnEraseBkgnd(pDC);
}

BOOL CMozilla::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	POINT p={0,0};
	// Send the message to Gecko like if it was generated from the top-left point
	HWND w=::ChildWindowFromPoint(m_hWnd,p);
  if(!w) return 0;
	w=::ChildWindowFromPoint(w,p);	
  if(!w) return 0;
	w=::ChildWindowFromPoint(w,p);
  if(!w) return 0;
	RECT r;
	::GetWindowRect(w,&r);
	::PostMessage(w,WM_MOUSEWHEEL,(zDelta<<16)+nFlags,(r.top<<16)+r.left);
	return 0;
}

LRESULT CMozilla::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message){
      case WM_APPCOMMAND:
				switch((short)(HIWORD(lParam) & ~0x8000)){
					case WM_XBUTTONBACK:
						mWebNav->GoBack();
						return TRUE;
					case WM_XBUTTONFORWARD:
						mWebNav->GoForward();
						return TRUE;
				}
				break;
	}
      	
	return CWnd::WindowProc(message, wParam, lParam);
}

int CMozilla::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) 
{
	switch (message) {
/*		case WM_MBUTTONDOWN:
			{
				panning = 1;
				GetCursorPos(&panning_point);
				SetCapture();
				HCURSOR c=LoadCursor(theApp.m_hInstance,MAKEINTRESOURCE(IDC_PAN_DOWN));
				SetCursor(c);
				ShowCursor(TRUE);
				SetFocus();
			}
			return 1;
		case WM_MBUTTONUP:
			{
				panning = 0;
			}
			return 1;*/
		case WM_MBUTTONDOWN:
			if(panning) StopPanning();
			else StartPanning();
			return TRUE;
		case WM_MBUTTONUP:
			{
			}
			return TRUE;
	}
	
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CMozilla::OnTimer(UINT nIDEvent) 
{
	switch(nIDEvent)
	{
		case 0x1:
			if(panning) {
				nsCOMPtr<nsIScrollable> s(do_QueryInterface(mBrowser));
        
        if(!s) return;

				POINT p;
				GetCursorPos(&p);

				int scroll_x,scroll_y;

				s->GetCurScrollPos(s->ScrollOrientation_X,&scroll_x);
				s->GetCurScrollPos(s->ScrollOrientation_Y,&scroll_y);

				if(abs(p.y-panning_point.y)>4)
					if(p.y>panning_point.y)	scroll_y+=(p.y-panning_point.y)*2;
					else scroll_y-=(panning_point.y-p.y)*2;

				if(abs(p.x-panning_point.x)>4)
					if(p.x>panning_point.x)	scroll_x+=(p.x-panning_point.x)*2;
					else scroll_x-=(panning_point.x-p.x)*2;

				s->SetCurScrollPosEx(scroll_x,scroll_y);

        int cursorx=p.x-panning_point.x,cursory=p.y-panning_point.y;
        int cursor=IDC_PAN;
        if(cursory<-4) cursor=IDC_PAN_UP;
        if(cursory>4) cursor=IDC_PAN_DOWN;
        if(cursorx<-4)
        {
          if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPLEFT;
          else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNLEFT;
          else cursor=IDC_PAN_LEFT;
        }
        if(cursorx>4)
        {
          if(cursor==IDC_PAN_UP) cursor=IDC_PAN_UPRIGHT;
          else if(cursor==IDC_PAN_DOWN) cursor=IDC_PAN_DOWNRIGHT;
          else cursor=IDC_PAN_RIGHT;
        }

        HCURSOR c=LoadCursor(theApp.m_hInstance,MAKEINTRESOURCE(cursor));
        SetCursor(c);
			}
			return;
	}
	
	CWnd::OnTimer(nIDEvent);
}

void CMozilla::StartPanning()
{
	panning = 1;
	GetCursorPos(&panning_point);
	SetCapture();
	SetTimer(0x1,10,NULL);

	SetFocus();
}

void CMozilla::StopPanning()
{
	ReleaseCapture();
	KillTimer(0x1);
	panning = 0;
}

BOOL CMozilla::PreTranslateMessage(MSG* pMsg) 
{
  if(panning && (pMsg->message==WM_SETCURSOR || pMsg->message==WM_MOUSEMOVE))
    return TRUE;

  if(panning && (pMsg->message==WM_LBUTTONDOWN || pMsg->message==WM_RBUTTONDOWN))
    StopPanning();
	
	return CWnd::PreTranslateMessage(pMsg);
}

void CMozilla::OnPopup(int flags)
{
  int hmenu;
  if (flags & nsIContextMenuListener::CONTEXT_LINK)
    hmenu=IDR_LINK_POPUP;
  else
    hmenu=IDR_BROWSER_POPUP;

  CMenu menu;
  menu.LoadMenu(hmenu);

  CPoint point;
	GetCursorPos(&point);

  CMenu* pSubMenu = menu.GetSubMenu(0);

  // BHarris
  pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x - 2, point.y - 2, this);
  /*
  CBCGPopupMenu* pPopupMenu = new CBCGPopupMenu;
  pPopupMenu->Create(this, point.x, point.y, (HMENU)pSubMenu->m_hMenu, FALSE, TRUE);
  */
}

void CMozilla::SetLastLink(char *link)
{
  strncpy(last_link,link,sizeof(last_link)-1);
  last_link[sizeof(last_link)-1]=0;
}

void CMozilla::OnLinkOpen() 
{
  CString s=last_link;
  Navigate(&s);
}

void CMozilla::OnLinkOpenNewWindow() 
{
  CString s=last_link;
  
  nsIWebBrowser *mNewBrowser;
  chrome->CreateBrowserWindow(0, -1, -1, -1, -1, &mNewBrowser);

	nsCOMPtr<nsIWebNavigation> mNewWebNav(do_QueryInterface(mNewBrowser));
  mNewWebNav->LoadURI(NS_ConvertASCIItoUCS2(s.GetBuffer(1024)).GetUnicode(),nsIWebNavigation::LOAD_FLAGS_NONE);
}

void CMozilla::OnPopupCopyShortcut() 
{
	OpenClipboard();
  EmptyClipboard();
  HGLOBAL c=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE ,sizeof(last_link)+1);
  char *p=(char *)GlobalLock(c);
  strcpy(p,last_link);
  GlobalUnlock(p);
  SetClipboardData(CF_TEXT,c);
  CloseClipboard();
}

void CMozilla::OnEditCopy() 
{
  nsIContentViewer *pContentViewer = nsnull;
  nsCOMPtr<nsIDocShell> rootDocShell = do_GetInterface(mBrowser);
	nsresult res = rootDocShell->GetContentViewer(&pContentViewer);
  nsCOMPtr<nsIContentViewerEdit> spContentViewerEdit = do_QueryInterface(pContentViewer);
  if(spContentViewerEdit)
  {
    spContentViewerEdit->CopySelection();
  }
}

void CMozilla::OnEditCut() 
{
  nsIContentViewer *pContentViewer = nsnull;
  nsCOMPtr<nsIDocShell> rootDocShell = do_GetInterface(mBrowser);
	nsresult res = rootDocShell->GetContentViewer(&pContentViewer);
  nsCOMPtr<nsIContentViewerEdit> spContentViewerEdit = do_QueryInterface(pContentViewer);
  if(spContentViewerEdit)
  {
    spContentViewerEdit->CutSelection();
  }
}

void CMozilla::OnEditPaste() 
{
  nsIContentViewer *pContentViewer = nsnull;
  nsCOMPtr<nsIDocShell> rootDocShell = do_GetInterface(mBrowser);
	nsresult res = rootDocShell->GetContentViewer(&pContentViewer);
  nsCOMPtr<nsIContentViewerEdit> spContentViewerEdit = do_QueryInterface(pContentViewer);
  if(spContentViewerEdit)
  {
    spContentViewerEdit->Paste();
  }
}
