/*
 *  Copyright (C) 2006 Dorian Boissonnade
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
 *
 */

#pragma once

class nsIX509Cert;
#include "IBrowserFrameGlue.h"

#include "nsICommandManager.h"
#include "nsIPrintSettings.h"
#include "nsIWebBrowserFind.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowserFocus.h"

class CBrowserImpl;

class CBrowserWrapper
{
private:
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIBaseWindow> mBaseWindow;
	nsCOMPtr<nsIWebNavigation> mWebNav;
	nsCOMPtr<nsIWebBrowserFocus> mWebBrowserFocus;

	nsCOMPtr<nsIDOMEventTarget> mEventTarget;
	//nsCOMPtr<CDomEventListener> mDomEventListener;
	nsCOMPtr<nsIPrintSettings> mPrintSettings;
	nsCOMPtr<nsIDOMNode> mLastMouseActionNode;

	
	PRInt32 mChromeMask;
	PBROWSERFRAMEGLUE mpBrowserFrameGlue;
	PBROWSERGLUE mpBrowserGlue;

	BOOL mChromeContent;
	CWnd* mWndOwner;

	BOOL mIsDocumentLoading;
	BOOL mIsDOMLoaded;

public:
	CBrowserImpl* mpBrowserImpl;
	CBrowserWrapper(void);
	~CBrowserWrapper(void);

	BOOL CreateBrowser(CWnd* parent, BOOL chromeContent = FALSE);
	BOOL DestroyBrowser();
    CString GetTitle();
	CString GetURI(BOOL unescape = FALSE);
	BOOL LoadURL(LPCTSTR url, BOOL currentForRef = FALSE, BOOL allowFixup = FALSE);
	BOOL LoadURL(LPCTSTR url, LPCTSTR referrer, BOOL allowFixup = FALSE);
	
	CBrowserImpl *GetBrowserImpl() { return mpBrowserImpl; }
	void SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue);
	void SetBrowserGlue(PBROWSERGLUE pBrowserGlue);
	void SetChromeMask(INT mask) { mChromeMask = mask; }
	void SetPositionAndSize(int x, int y, int cx, int cy) { 
		if (mBaseWindow) mBaseWindow->SetPositionAndSize(0, 0, cx, cy, PR_TRUE);
	}
	void ShowScrollbars(BOOL visible);
	BOOL ScrollBy(INT dx, INT dy);

	void GoBack() { if (mWebNav) mWebNav->GoBack(); }
	void GoForward() { if (mWebNav) mWebNav->GoForward(); }
	void Stop() { if(mWebNav) mWebNav->Stop(nsIWebNavigation::STOP_ALL); }
	
	BOOL Reload(BOOL force)
	{
		PRUint32 loadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;
		if (force) loadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE | 
			nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
		nsresult rv = mWebNav->Reload(loadFlags); 
		return NS_SUCCEEDED(rv);
	}

	BOOL CanGoBack()
	{
		PRBool ret = PR_FALSE;
		if (mWebNav) mWebNav->GetCanGoBack(&ret);
		return ret;
	}

	BOOL CanGoForward()
	{
		PRBool ret = PR_FALSE;
		if (mWebNav) mWebNav->GetCanGoForward(&ret);
		return ret;
	}

	BOOL CanCut()
	{
	    PRBool ret = PR_FALSE;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanCutSelection(&ret);
		return ret;
	}

	BOOL CanCopy()
	{
	    PRBool ret = PR_FALSE;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanCopySelection(&ret);
		return ret;
	}

	BOOL CanPaste()
	{
	    PRBool ret = PR_FALSE;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanPaste(&ret);
		return ret;
	}

	BOOL CanCopyImage()
	{
		PRBool ret = PR_FALSE;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanCopyImageContents(&ret);
		return ret;
	}

	void Cut()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CutSelection();
	}
	
	void Copy()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CopySelection();
	}
	
	void Paste()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->Paste();
	}

	void CopyImage()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CopyImageContents();
	}

	void SelectAll()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->SelectAll();
	}

	void SelectNone()
	{
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->SelectNone();
	}
	
	void Undo()
	{
		nsCOMPtr<nsICommandManager> commandMgr(do_GetInterface(mWebBrowser));
		if (commandMgr) commandMgr->DoCommand("cmd_undo", nsnull, nsnull);
	}

	BOOL CanUndo()
	{
		PRBool ret = PR_FALSE;
		nsCOMPtr<nsICommandManager> commandMgr(do_GetInterface(mWebBrowser));
		if (commandMgr)commandMgr->IsCommandEnabled("cmd_undo", nsnull, &ret);
		return ret;
	}

	void GetCurrentURI(nsIURI** aURI)
	{
		nsCOMPtr<nsIURI> uri;
		mWebNav->GetCurrentURI(getter_AddRefs(uri));
		NS_IF_ADDREF(*aURI = uri);
	}

	void GetReferringURI(nsIURI** aURI)
	{
		nsCOMPtr<nsIURI> uri;
		mWebNav->GetReferringURI(getter_AddRefs(uri));
		NS_IF_ADDREF(*aURI = uri);
	}

	BOOL IsBusy()
	{
		nsCOMPtr<nsIDocShell> docShell(do_GetInterface(mWebBrowser));
		NS_ENSURE_TRUE(docShell, FALSE);

		PRUint32 flags;
		docShell->GetBusyFlags(&flags);
		return flags != nsIDocShell::BUSY_FLAGS_NONE;
	}

	void SetActive(BOOL aActive)
	{
		NS_ENSURE_TRUE(mWebBrowserFocus, );
		if (aActive) mWebBrowserFocus->Activate(); else mWebBrowserFocus->Deactivate();
	}

	void FocusFirstElement() 
	{
		NS_ENSURE_TRUE(mWebBrowserFocus, );
		mWebBrowserFocus->Activate();
		mWebBrowserFocus->SetFocusAtFirstElement();
	}

	void FocusLastElement() 
	{
		NS_ENSURE_TRUE(mWebBrowserFocus, );
		mWebBrowserFocus->Activate();
		mWebBrowserFocus->SetFocusAtLastElement();
	}

	already_AddRefed<nsIWebBrowser> GetWebBrowser()
	{
		nsIWebBrowser* browser;
		NS_IF_ADDREF(browser = mWebBrowser);
		return browser;
	}

	already_AddRefed<nsIDOMWindow> GetContentWindow()
	{
		nsIDOMWindow* dom = nsnull;
		if (mWebBrowser) mWebBrowser->GetContentDOMWindow(&dom);
		return dom;
	}

	NS_IMETHODIMP GetWebBrowser(nsIWebBrowser **aWebBrowser)
	{
		*aWebBrowser = mWebBrowser;
		NS_IF_ADDREF(*aWebBrowser);
		return NS_OK;
	}

	int GetSecurityState();

	BOOL GetCharset(char* aCharset);
    BOOL ForceCharset(char *aCharSet);

	BOOL Print();
	BOOL PrintPreview();
	BOOL InitPrintSettings();
	void PrintSetup();

	BOOL IsPrintPreview() {
		nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
		NS_ENSURE_TRUE(print, FALSE);
		
		PRBool isPreview = PR_FALSE;
		nsresult rv = print->GetDoingPrintPreview(&isPreview);
		return isPreview;
	}

	BOOL SetTextSize(float textzoom);
	float GetTextSize();
	BOOL ChangeTextSize(int change);	
	BOOL GetSHistory(nsISHistory **aSHistory);
	BOOL CloneSHistory(CBrowserWrapper* newWebBrowser);
	BOOL GotoHistoryIndex(UINT index);
	BOOL GetSHistoryState(int& index, int& count);
	BOOL GetSHistoryInfoAt(PRInt32 index, CString& title, CString& url);
	
	BOOL GetSelectionInsideForm(nsIDOMElement *element, nsEmbedString &aSelText);
	BOOL GetSelection(CString&);
	BOOL GetUSelection(nsEmbedString&);
	BOOL InjectCSS(const wchar_t* userStyleSheet);
	BOOL InjectJS(const wchar_t* userJS, bool bTopWindow = true);
	BOOL GetSecurityInfo(CString &sign);
	BOOL ShowCertificate();
	BOOL ViewContentContainsFrames();
	BOOL Highlight(const PRUnichar* backcolor, const PRUnichar* word, BOOL matchCase);
	BOOL InputHasFocus();
	CString GetFrameURL(nsIDOMNode* aNode = NULL);
	
	already_AddRefed<nsISupports> GetPageDescriptor(BOOL focus = FALSE);
	BOOL CanSave();
	BOOL SaveURL(LPCTSTR url, LPCTSTR filename = NULL);
	BOOL SaveDocument(BOOL frame, LPCTSTR filename = NULL);
	BOOL Find(const wchar_t* searchString, 
		   BOOL matchCase,
		   BOOL wrapAround,
		   BOOL backwards,
		   BOOL ahead);

/*	void SetMatchCase(BOOL);
	void SetWrapAround(BOOL);
	wchar_t* GetSearchString();
#ifdef FINDBAR_USE_TYPEAHEAD
//	BOOL Find(const wchar_t* searchString);
#else
//	BOOL Find(const wchar_t* searchString, BOOL ahead);
#endif
	//BOOL FindNext(BOOL backward);*/


private:
	BOOL AddListeners(void);
	void RemoveListeners(void);
	BOOL _GetSelection(nsIDOMWindow* dom, nsAString& aSelText);
	BOOL _InjectCSS(nsIDOMWindow* dom, const wchar_t* userStyleSheet);
	BOOL _Highlight(nsIDOMWindow* dom, const PRUnichar* backcolor, const PRUnichar* word, BOOL matchCase);
	BOOL _Save(nsIURI* aURI, nsIDOMDocument* aDocument, LPCTSTR filename, nsIURI* aReferrer, nsISupports* aDescriptor);
	BOOL GetCertificate(nsIX509Cert** certificate);
	PRBool CheckNode(nsIDOMElement* elem);

#ifndef FINDBAR_USE_TYPEAHEAD
	void CollapseSelToStartInFrame(nsIDOMWindow* dom);
#endif
};
