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
#include "nsIClipboardCommands.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsISelection.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserPrint.h"
#include "nsIBaseWindow.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIEditor.h"

class nsITypeAheadFind;
class nsIDOMKeyEvent;
class nsIPrintSettings;
class nsIWebBrowserFocus;

class CBrowserImpl;

class CBrowserWrapper
{
private:
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIBaseWindow> mBaseWindow;
	nsCOMPtr<nsIWebNavigation> mWebNav;
	nsCOMPtr<nsIWebBrowserFocus> mWebBrowserFocus;
	nsCOMPtr<nsITypeAheadFind> mTypeAhead;

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

	BOOL CreateBrowser(CWnd* parent, uint32_t chromeFlags = 0);
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
		if (mBaseWindow) mBaseWindow->SetPositionAndSize(0, 0, cx, cy, true);
	}
	void ShowScrollbars(BOOL visible);
	BOOL ScrollBy(int32_t dx, int32_t dy);

	void GoBack() { if (mWebNav) mWebNav->GoBack(); }
	void GoForward() { if (mWebNav) mWebNav->GoForward(); }
	void Stop() { if(mWebNav) mWebNav->Stop(nsIWebNavigation::STOP_ALL); }
	
	bool SetScrollPosition(POINT scroll)
	{
		nsCOMPtr<nsIDOMWindow> dom;
		if (mWebBrowser) mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		if (!dom) return false;
		return NS_SUCCEEDED(dom->ScrollTo(scroll.x, scroll.y));
	}

	POINT GetScrollPosition() 
	{
		POINT p = {0,0};
		nsCOMPtr<nsIDOMWindow> dom;
		if (mWebBrowser) mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		if (dom) {
			int32_t s = 0;
			dom->GetScrollX(&s);
			p.x = s;
			dom->GetScrollY(&s);
			p.y = s;
		}
		return p;
	}

	BOOL AllowJS(bool allow) 
	{
		nsCOMPtr<nsIDocShell> ds = GetDocShell();
		return NS_SUCCEEDED(ds->SetAllowJavascript(allow));
	}

	BOOL IsJSAllowed()
	{
		nsCOMPtr<nsIDocShell> ds = GetDocShell();
		bool res = false;
		ds->GetAllowJavascript(&res);
		return res;
	}

	BOOL GetEditor2(nsCOMPtr<nsPIDOMWindow> piWin, nsCOMPtr<nsIEditor>& editor)
	{
		if (!piWin) return FALSE;
		nsIDocShell* docShell = piWin->GetDocShell();
		if (!docShell) return FALSE;

		docShell->GetEditor(getter_AddRefs(editor));
		NS_ENSURE_TRUE(editor, FALSE);
		return TRUE;
	}

	BOOL GetEditor1(nsCOMPtr<nsIEditor>& editor)
	{
		nsresult rv;

		NS_ENSURE_TRUE(mWebBrowserFocus, FALSE);

		nsCOMPtr<nsIDOMElement> elem;
		rv = mWebBrowserFocus->GetFocusedElement(getter_AddRefs(elem));
		NS_ENSURE_TRUE(elem, FALSE);

		nsCOMPtr<nsIDOMNSEditableElement> ee(do_QueryInterface(elem));
		NS_ENSURE_TRUE(ee, FALSE);

		rv = ee->GetEditor(getter_AddRefs(editor));
		NS_ENSURE_TRUE(editor, FALSE);

		return TRUE;
	}

	BOOL CanCopy2()
	{
		nsCOMPtr<nsIDOMWindow> dom;
		if (mWebBrowser) mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
		nsCOMPtr<nsPIDOMWindow> domWindow = do_QueryInterface(dom);
		nsCOMPtr<nsIEditor> editor;
		nsCOMPtr<nsISelection> domSelection;
		if (GetEditor1(editor) || GetEditor2(domWindow, editor)) {
			editor->GetSelection(getter_AddRefs(domSelection));
		} else {
			domWindow->GetSelection(getter_AddRefs(domSelection));
		}
		NS_ENSURE_TRUE(domSelection, false);

		bool selectionCollapsed = false;
		domSelection->GetIsCollapsed(&selectionCollapsed);
		return selectionCollapsed;
	}

	BOOL Reload(bool force)
	{
		PRUint32 loadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;
		if (force) loadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE | 
			nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
		nsresult rv = mWebNav->Reload(loadFlags); 
		return NS_SUCCEEDED(rv);
	}

	BOOL CanGoBack()
	{
		bool ret = false;
		if (mWebNav) mWebNav->GetCanGoBack(&ret);
		return ret;
	}

	BOOL CanGoForward()
	{
		bool ret = false;
		if (mWebNav) mWebNav->GetCanGoForward(&ret);
		return ret;
	}

	BOOL CanCut()
	{
		bool ret = false;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanCutSelection(&ret);
		return ret;
	}

	BOOL CanCopy()
	{
		bool ret = false;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanCopySelection(&ret);
		return ret;
	}

	BOOL CanPaste()
	{
		bool ret = false;
		nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(mWebBrowser);
		if (clipCmds) clipCmds->CanPaste(&ret);
		return ret;
	}

	BOOL CanCopyImage()
	{
		bool ret = false;
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

	void DoCommand(const char* cmd)
	{
		nsCOMPtr<nsICommandManager> commandMgr(do_GetInterface(mWebBrowser));
		if (commandMgr) commandMgr->DoCommand(cmd, nullptr, nullptr);
	}

	BOOL CanDoCommand(const char* cmd)
	{
		bool ret = false;
		nsCOMPtr<nsICommandManager> commandMgr(do_GetInterface(mWebBrowser));
		if (commandMgr)commandMgr->IsCommandEnabled(cmd, nullptr, &ret);
		return ret;
	}

	void Undo()
	{
		DoCommand("cmd_undo");
	}

	BOOL CanUndo()
	{
		return CanDoCommand("cmd_undo");
	}

	void Delete()
	{
		DoCommand("cmd_delete");
	}

	BOOL CanDelete()
	{
		return CanDoCommand("cmd_delete");
	}

	void Redo()
	{
		DoCommand("cmd_redo");
	}

	BOOL CanRedo()
	{
		return CanDoCommand("cmd_redo");
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

	already_AddRefed<nsIWebBrowser> GetWebBrowser() const
	{
		//nsIWebBrowser* browser;
		//NS_IF_ADDREF(browser = mWebBrowser);
		nsCOMPtr<nsIWebBrowser> browser = mWebBrowser;
		return browser.forget();
	}

	already_AddRefed<nsIDOMWindow> GetContentWindow() const
	{
		nsIDOMWindow* dom = nullptr;
		if (mWebBrowser) mWebBrowser->GetContentDOMWindow(&dom);
		return (decltype(nullptr))dom;
	}

	NS_IMETHODIMP GetWebBrowser(nsIWebBrowser **aWebBrowser) const
	{
		*aWebBrowser = mWebBrowser;
		NS_IF_ADDREF(*aWebBrowser);
		return NS_OK;
	}

	void SetVisible(BOOL aVisible);
	void SetActive(BOOL aActive);
	int GetSecurityState();

	BOOL GetCharset(char* aCharset);
	BOOL ForceCharset(const char *aCharSet);
	CString GetLang();

	BOOL Print();
	BOOL PrintPreview();
	BOOL InitPrintSettings();
	void PrintSetup();

	BOOL IsPrintPreview() {
		nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(mWebBrowser));
		NS_ENSURE_TRUE(print, FALSE);

		bool isPreview = false;
		nsresult rv = print->GetDoingPrintPreview(&isPreview);
		return isPreview;
	}

	already_AddRefed<nsIContentViewer> GetContentViewer();
	BOOL SetFullZoom(float textzoom);
	float GetFullZoom();
	BOOL ChangeFullZoom(int change);

	BOOL SetTextSize(float textzoom);
	float GetTextSize();
	BOOL ChangeTextSize(int change);	
	BOOL GetSHistory(nsISHistory **aSHistory);
	BOOL CloneSHistory(CBrowserWrapper* newWebBrowser);
	BOOL GotoHistoryIndex(UINT index);
	BOOL GetSHistoryState(int& index, int& count);
	BOOL GetSHistoryInfoAt(PRInt32 index, CString& title, CString& url);

	BOOL GetSelectionInsideForm(nsIDOMElement *element, nsString &aSelText);
	BOOL GetSelection(CString&);
	BOOL GetUSelection(nsString&);
	BOOL InjectCSS(const wchar_t* userStyleSheet);
	BOOL InjectJS(const wchar_t* userJS, CString& result, bool bTopWindow = true);
	BOOL GetSecurityInfo(CString &sign);
	BOOL ShowCertificate();
	BOOL ViewContentContainsFrames();
	BOOL Highlight(const PRUnichar* backcolor, const PRUnichar* word, BOOL matchCase);
	BOOL InputHasFocus(bool typeahead = true);
	BOOL IsClickable(nsIDOMElement* element);
	BOOL IsInputOrObject(nsIDOMElement* element);
	CString GetFrameURL(nsIDOMNode* aNode = NULL);
	CString GetDocURL(nsIDOMNode* aNode = NULL);

	already_AddRefed<nsISupports> GetPageDescriptor(BOOL focus = FALSE);
	BOOL CanSave();
	BOOL SaveURL(LPCTSTR url, LPCTSTR filename = NULL);
	BOOL SaveDocument(BOOL frame, LPCTSTR filename = NULL);
	BOOL Find(const wchar_t* searchString, 
		   BOOL matchCase,
		   BOOL wrapAround,
		   BOOL backwards,
		   BOOL ahead);
	bool TypeAheadFind(nsIDOMKeyEvent* keyEvent);
	void EndTypeAheadFind();
	nsString mSearchString;
	BOOL GetCertificate(nsIX509Cert** certificate);
	nsIDocShell* GetDocShell();
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
	bool CheckNode(nsIDOMElement* elem);
	

#ifndef FINDBAR_USE_TYPEAHEAD
	void CollapseSelToStartInFrame(nsIDOMWindow* dom);
#endif
};
