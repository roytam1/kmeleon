/*
*  Copyright (C) 2005 Dorian Boissonnade
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
*
*/

#include "stdafx.h"
#include "BrowserFrm.h"
#include "BrowserFrmTab.h"

class CBrowserGlueTab : public CBrowserGlue
{
public:
	CBrowserGlueTab(CBrowserFrame* frame, CBrowserView* view) :
		CBrowserGlue(frame, view)
	{}

	CBrowserWrapper* ReuseWindow(BOOL useCurrent)
	{
		if (mpBrowserFrame->IsPopup() || mpBrowserFrame->IsDialog())
			return NULL;

		if (useCurrent) 
			return mpBrowserView->GetBrowserWrapper();

		CBrowserTab* tab = ((CBrowserFrmTab*)mpBrowserFrame)->CreateBrowserTab();
		if ( !theApp.preferences.GetBool("kmeleon.tabs.loadDivertedInBackground", FALSE)
			&& mpBrowserView == mpBrowserFrame->GetActiveView())
			((CBrowserFrmTab*)mpBrowserFrame)->SetActiveBrowser(tab);
		return tab->GetBrowserWrapper();
	}

	void SetBrowserTitle(LPCTSTR aTitle) 
	{
		CBrowserGlue::SetBrowserTitle(aTitle);
		if (!theApp.preferences.GetBool("kmeleon.tabs.useLoadingTitle", FALSE) || !mLoading)
			((CBrowserFrmTab*)mpBrowserFrame)->SetTabTitle((CBrowserTab*)mpBrowserView, aTitle);
	}

	void SetFavIcon(nsIURI *favUri) 
	{
		CBrowserGlue::SetFavIcon(favUri);
		if (!mLoading || !theApp.preferences.GetBool("kmeleon.tabs.useLoadingIcon", TRUE)) {
			mIcon = theApp.favicons.GetIcon(mIconURI);
			((CBrowserFrmTab*)mpBrowserFrame)->SetTabIcon((CBrowserTab*)mpBrowserView, mIcon);
		}
	}

	void UpdateBusyState(BOOL aBusy)
	{
		CBrowserGlue::UpdateBusyState(aBusy);
		CString title;
			
		if (aBusy) {
			if (theApp.preferences.GetBool("kmeleon.tabs.useLoadingIcon", TRUE))
				((CBrowserFrmTab*)mpBrowserFrame)->SetTabIcon((CBrowserTab*)mpBrowserView, theApp.favicons.GetLoadingIcon());

			if (!theApp.preferences.GetBool("kmeleon.tabs.useLoadingTitle", FALSE) && mLocation.GetLength())
				return; //title = mLocation;
			else
				title.LoadString(IDS_LOADING);
		} else {
			mIcon = theApp.favicons.GetIcon(mIconURI);
			((CBrowserFrmTab*)mpBrowserFrame)->SetTabIcon((CBrowserTab*)mpBrowserView, mIcon);
			if (mTitle.IsEmpty()) {
				if (mLocation.IsEmpty() || mLocation.Compare(_T("about:blank")) == 0)
					title.LoadString(IDS_EMPTY);
				else
					title = mLocation;
			}
			else title = mTitle;
		}
		

		((CBrowserFrmTab*)mpBrowserFrame)->SetTabTitle((CBrowserTab*)mpBrowserView, title);
	}

	void GetVisibility(bool *aVisible)
	{
		if (mpBrowserFrame->GetActiveView() != mpBrowserView)
			*aVisible = false;
		else
			CBrowserGlue::GetVisibility(aVisible);
	}

	void DestroyBrowserFrame()
	{
		CBrowserFrmTab* frame = ((CBrowserFrmTab*)mpBrowserFrame);
		if (frame->GetTabCount()>1)
			frame->CloseTab((CBrowserTab*)mpBrowserView);
		else
			frame->PostMessage(WM_CLOSE, -1, -1);
	}

};

IMPLEMENT_DYNAMIC(CBrowserFrmTab, CBrowserFrame)

BEGIN_MESSAGE_MAP(CBrowserFrmTab, CBrowserFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_CLOSE, OnClose)
	ON_MESSAGE(WM_OPENTAB, OnOpenTab)
	ON_MESSAGE(WM_CLOSETAB, OnCloseTab)
	ON_COMMAND(ID_TAB_NEXT, OnNextTab)
	ON_COMMAND(ID_TAB_PREV, OnPrevTab)
	ON_COMMAND(ID_TAB_LAST, OnLastTab)
	ON_COMMAND(ID_NEW_TAB, OnNewTab)
	ON_COMMAND(ID_CLOSE_TAB, OnCloseTab)
	ON_COMMAND(ID_CLOSE_ALLTAB, OnCloseAllTab)
	ON_COMMAND(ID_CLOSE_ALLOTHERTAB, OnCloseAllOtherTab)
	ON_UPDATE_COMMAND_UI_RANGE(TABS_START_ID, TABS_STOP_ID, OnUpdateTabs)
	ON_COMMAND_RANGE(TABS_START_ID, TABS_STOP_ID, OnTabSelect)
	ON_NOTIFY(TBN_BEGINDRAG, ID_TABS_BAR, OnTbnBeginDrag)
	ON_MESSAGE(UWM_NEWSITEICON, OnNewSiteIcon)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_COMMAND(ID_EDIT_FIND, OnShowFindBar)
//	ON_MESSAGE(UWM_GETFAVICON, OnGetFavIcon)
	ON_WM_KEYDOWN()
	ON_WM_SYSCOMMAND()
	ON_WM_MENUSELECT()
END_MESSAGE_MAP()

BOOL CBrowserFrmTab::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	static char* tip = NULL;
	static WCHAR* wtip = NULL;

	int tooltipWidth = theApp.preferences.GetInt("kmeleon.tabs.tooltipWidth", 100);

	if (pNMHDR->idFrom < TABS_START_ID || pNMHDR->idFrom > TABS_STOP_ID) 
		return CBrowserFrame::OnToolTipText(id, pNMHDR, pResult);

	CBrowserTab* tab = (CBrowserTab*)m_Tabs[IDTOTABINDEX(pNMHDR->idFrom)];
	if (!tab) return CBrowserFrame::OnToolTipText(id, pNMHDR, pResult);

	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	
	CString tabBarTip = tab->GetBrowserGlue()->mLocation;
	if (tabBarTip.GetLength()>tooltipWidth)
		tabBarTip = tabBarTip.Mid(0, tooltipWidth-3) + _T("...");
	if (!tab->GetBrowserGlue()->mTitle.IsEmpty() && tab->GetBrowserGlue()->mTitle.Compare(tab->GetBrowserGlue()->mLocation) != 0) 
		tabBarTip = tab->GetBrowserGlue()->mTitle + _T("\n") + tabBarTip;

	USES_CONVERSION;
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		if (tip) free(tip);
		tip = strdup(T2CA(tabBarTip));
		pTTTA->lpszText = tip;
	}
	else
	{
		if (wtip) free(wtip);
		wtip = wcsdup(T2CW(tabBarTip));
		pTTTW->lpszText = wtip;
	}

	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

	return TRUE;   
}

CBrowserFrmTab::CBrowserFrmTab(PRUint32 chromeMask, LONG style)
: CBrowserFrame(chromeMask,style)
{
	m_pPreviousSelectedTab = NULL;
	m_wndCBrowserTab = NULL;
	m_wndTabs = NULL;
	m_iBrowserCount = 0;
}

CBrowserFrmTab::~CBrowserFrmTab()
{
/*	for (int i=0;i<m_iBrowserCount;i++)
		delete m_Tabs[i];*/
	if (m_wndTabs)
		delete m_wndTabs;
}
void CBrowserFrmTab::OnDestroy()
{
	m_wndCBrowserTab = NULL;
	for (int i=0;i<m_iBrowserCount;i++)
		m_Tabs[i]->DestroyWindow();
	
	CBrowserFrame::OnDestroy();
}

void CBrowserFrmTab::OnClose()
{
   int ConfirmClose = theApp.preferences.GetBool("browser.tabs.warnOnClose", 1);
   if (ConfirmClose && m_iBrowserCount>1)
   {
	   SetForegroundWindow();
	   CString str;
	   str.Format(IDS_CLOSE_SEVERAL_TABS, m_iBrowserCount);
	   if (MessageBox(str, 0, MB_OKCANCEL|MB_ICONWARNING) != IDOK)
		   return;
   }

   //for (int i = 0;i<m_iBrowserCount;i++)
//      m_Tabs[i]->Destroy();

   CBrowserFrame::OnClose();
}


int CBrowserFrmTab::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
       return -1;

	// tell all our plugins that we were created
	if (!IsDialog())
		theApp.plugins.SendMessage("*", "* OnCreate", "Create", (long)this->m_hWnd, 1 & (IsPopup()?2:0) );

	// Create a ReBar window to which the toolbar and UrlBar 
    // will be added
    BOOL hasLine = theApp.preferences.GetBool("kmeleon.display.toolbars_line", TRUE);
    if (!m_wndReBar.Create(this, RBS_DBLCLKTOGGLE | RBS_VARHEIGHT | (hasLine ? RBS_BANDBORDERS:0)))
    {
        TRACE0("Failed to create ReBar\n");
        return -1;      // fail to create
    }
	m_wndReBar.SetNeedSeparator(true);

	// Create the bar which will contain the tab list
	// It would be better to use a tabcontrol but, currently
	// i'm not sure how to deal with it, and it would not fit
	// the skin.
    m_wndTabs = new CTabReBar();
	if (!m_wndTabs->Create(&m_wndReBar, ID_TABS_BAR))
	{
		TRACE0("Failed to create the tab toolbar\n");
		return -1;
	}

	// Set the favicon image list for the tab bar
	m_wndTabs->GetToolBarCtrl().SetImageList(&theApp.favicons);

	// Create the first tab
	if (CreateBrowserTab(true)==NULL)
	{
        TRACE0("Failed to create view window\n");
        return -1;
    }

	if (CBrowserFrame::InitLayout() == -1)
		return -1;

	m_wndTabs->Init(&m_wndReBar);

	// If we try to hide it sooner then CrashBadaboum
	if (theApp.preferences.bAutoHideTabControl) {
		int index = m_wndReBar.FindByName(_T("Tabs"));
		m_wndReBar.ShowBand(index, FALSE);
	}
	
	return 0;
}

CBrowserTab* CBrowserFrmTab::CreateBrowserTab(bool first)
{
  	// Fail if we reached the maximun number of tabs
	if (m_iBrowserCount >= MAX_TABS_NUMBER) 
	{
		::AfxMessageBox(IDS_MAXTABSNUMBER, MB_ICONSTOP );
		return NULL;
	}

	// Create a new Tab
	m_Tabs[m_iBrowserCount] = new CBrowserTab();

	// Pass "this" to the View for later callbacks
    // and/or access to any public data members, if needed
    m_Tabs[m_iBrowserCount]->SetBrowserFrame(this);

	// Pass on the BrowserFrameGlue also to the View which
    // it will use during the Init() process after creation
    // of the BrowserImpl obj. Essentially, the View object
    // hooks up the Embedded browser's callbacks to the BrowserFrame
    // via this BrowserFrameGlue object
	// It will be deleted by the view.
	m_Tabs[m_iBrowserCount]->SetBrowserGlue(new CBrowserGlueTab(this, m_Tabs[m_iBrowserCount]));

	// Create the view
	BOOL r = m_Tabs[m_iBrowserCount]->Create(this, m_iBrowserCount);

	if (r) 
	{
		//
		CString str;
		str.LoadString(IDS_EMPTY);
		int buttonIndex = m_iBrowserCount;
        if (theApp.preferences.iOnOpenTab == 1
			&& m_iCBrowserView>=0)
			buttonIndex = m_wndTabs->FindById(TABINDEXTOID(m_iCBrowserView)) + 1;

		m_wndTabs->InsertItem(buttonIndex, TABINDEXTOID(m_iBrowserCount), (LPCTSTR)str, (DWORD_PTR)m_Tabs[m_iBrowserCount]);

		if (first)
		{
			m_wndCBrowserTab = m_Tabs[0];
			m_iCBrowserView = 0;

			// This is our current active tab but it doesn't mean it can
			// get the focus yet.
			m_Tabs[0]->SetActive(true, false);
			//SetActiveBrowser(m_Tabs[0]);
		}
		//else
		//	m_Tabs[m_iBrowserCount]->SetActive(false);
		UpdateTabListMenu();
		return m_Tabs[m_iBrowserCount++];
	}
	
	// Failed to create the view. Cleaning.
	delete m_Tabs[m_iBrowserCount];
		
	return NULL;
}

void CBrowserFrmTab::OnTabSelect(UINT id)
{
	int index = IDTOTABINDEX(id) ;
	SetActiveBrowser(m_Tabs[index]);
}

BOOL CBrowserFrmTab::SafeSetActiveBrowser(CBrowserTab* aNewActiveTab)
{
	unsigned count = GetTabCount();
	if (!count) return FALSE;
	
	for (unsigned i=0; i<count; i++) 
		if (GetTabIndex(i) == aNewActiveTab) {
			SetActiveBrowser(aNewActiveTab);
			return TRUE;
		}

	return FALSE;
}

//void CBrowserFrame::SetActiveBrowser(int index)
void CBrowserFrmTab::SetActiveBrowser(CBrowserTab* aNewActiveTab)
{
	if (!aNewActiveTab || aNewActiveTab->IsActive()) return;
	BOOL focus = TRUE;

	if (m_wndCBrowserTab && aNewActiveTab!=m_wndCBrowserTab)
	{
		m_wndCBrowserTab->SetActive(false);
		m_wndCBrowserTab->SetTypedLocation(!m_wndUrlBar.GetIsTyped() ? _T("") : m_wndUrlBar.GetEnteredURL());
		m_pPreviousSelectedTab = m_wndCBrowserTab;
		focus = m_wndCBrowserTab->IsChild(GetFocus());
		m_wndCBrowserTab->GetBrowserWrapper()->SetVisible(false);
		PostMessage(UWM_UPDATESESSIONHISTORY, 0, 0);
	}
	
	CBrowserTab* oldTab = m_wndCBrowserTab;
	m_iCBrowserView = aNewActiveTab->m_iIndex;
	m_wndCBrowserTab = aNewActiveTab;
	
	//SendMessage(WM_SWITCHTAB, (WPARAM)m_wndCBrowserTab, (LPARAM)aNewActiveTab);
	theApp.plugins.SendMessage("*", "*", "SwitchTab", (long)oldTab->GetSafeHwnd(), (long)aNewActiveTab->GetSafeHwnd());

	//m_wndTabs->SelectItem(TABINDEXTOID(aNewActiveTab->m_iIndex));

	//m_wndCBrowserView->Activate(WA_ACTIVE, NULL, false);
	//m_Tabs[index].xBrowserFrameGlueObj->m_bActive = true;
	
	UpdateTitle(m_wndCBrowserTab->GetBrowserGlue()->mTitle);
	UpdateSecurityStatus(m_wndCBrowserTab->GetBrowserGlue()->mSecurityState);
	UpdatePopupNotification(m_wndCBrowserTab->GetBrowserGlue()->mPopupBlockedHost);
	UpdateLocation(m_wndCBrowserTab->GetBrowserGlue()->mPendingLocation.GetLength() == 0 ? m_wndCBrowserTab->GetLocation() : m_wndCBrowserTab->GetBrowserGlue()->mPendingLocation, TRUE);
	UpdateLoading(m_wndCBrowserTab->GetBrowserGlue()->mLoading);
	UpdateProgress(m_wndCBrowserTab->GetBrowserGlue()->mProgressCurrent, m_wndCBrowserTab->GetBrowserGlue()->mProgressMax);
	UpdateSiteIcon(theApp.favicons.GetIcon(m_wndCBrowserTab->GetBrowserGlue()->mIconURI));
	UpdateStatus(m_wndCBrowserTab->GetBrowserGlue()->mStatusText);
	if (m_wndCBrowserTab->GetBrowserGlue()->mHIndex>=0) {
		m_wndCBrowserTab->GetBrowserWrapper()->GotoHistoryIndex(m_wndCBrowserTab->GetBrowserGlue()->mHIndex);
		m_wndCBrowserTab->GetBrowserGlue()->mHIndex = -1;		
	}

	// BUG: We may be here because of a call to this function.
	aNewActiveTab->SetActive(true, focus);
	aNewActiveTab->GetBrowserWrapper()->SetVisible(true);
	// Don't wait for MFC to update the tab bar
	m_wndTabs->GetToolBarCtrl().CheckButton(TABINDEXTOID(aNewActiveTab->m_iIndex), 1);
	// Remove a possible tooltip
	m_wndToolTip.Hide();

	// Reposition the view
	RecalcLayout();
	
}
								 

// I'm overriding RecalcLayout so the current active browser view
// is repositionned.
void CBrowserFrmTab::RecalcLayout(BOOL bNotify)
{
	if (m_bInRecalcLayout)
		return;

	m_bInRecalcLayout = TRUE;
	// clear idle flags for recalc layout if called elsewhere
	if (m_nIdleFlags & idleNotify)
		bNotify = TRUE;
	m_nIdleFlags &= ~(idleLayout|idleNotify);

	
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST + m_iCBrowserView, reposExtra, &m_rectBorder);
	m_bInRecalcLayout = FALSE;
}

void CBrowserFrmTab::OnNewTab()
{
	if (IsDialog() || IsPopup())
		return;

	CBrowserTab* tab;
	if ( (tab=CreateBrowserTab()) == NULL)
	{
        TRACE0("Failed to create view window\n");
        return;
    }
    
    switch (theApp.preferences.iNewWindowOpenAs) {
      case PREF_NEW_WINDOW_CURRENT:
		  if (m_wndCBrowserTab)
            m_wndCBrowserTab->CloneBrowser(tab);
         break;
      case PREF_NEW_WINDOW_HOME:
         tab->LoadHomePage();
         break;
      case PREF_NEW_WINDOW_BLANK:
         tab->OpenURL(_T("about:blank"));
         break;
	  case PREF_NEW_WINDOW_URL: {
	     CString newUrl = theApp.preferences.newWindowURL;
         if (newUrl.IsEmpty())
            tab->OpenURL(_T("about:blank"));
         else
			tab->OpenURL(newUrl);
      }
         break;
      }

	SetActiveBrowser(tab);

	if (theApp.preferences.GetBool("kmeleon.display.NewWindowHasUrlFocus", FALSE))
		m_wndUrlBar.SetFocus();
}

void CBrowserFrmTab::OnUpdateTabs(CCmdUI* pCmd)
{
	pCmd->Enable();
	if (pCmd->m_nID == TABINDEXTOID(m_iCBrowserView))
		pCmd->SetCheck(1);
	else
		pCmd->SetCheck(0);
}

#include "nsISHistory.h"
#include ".\browserfrmtab.h"

void CBrowserFrmTab::OnCloseAllTab()
{
	while (m_iBrowserCount>1)
		CloseTab(m_wndCBrowserTab);

	CBrowserWrapper* browser = GetActiveView()->GetBrowserWrapper();
	if (!browser) return;

	nsCOMPtr<nsISHistory> sHistory;
	if (!browser->GetSHistory(getter_AddRefs(sHistory)))
		return;

	GetActiveView()->OpenURL(_T("about:blank"));
	PRInt32 shcount;
	sHistory->GetCount(&shcount);
	sHistory->PurgeHistory(shcount);
}

void CBrowserFrmTab::OnCloseAllOtherTab()
{
	CBrowserView* current = GetActiveView();

	while (m_iBrowserCount>1) {
		if (current != this->m_Tabs[m_iCBrowserView])
			CloseTab(m_Tabs[m_iCBrowserView]);
		else if (m_iCBrowserView>0)
			CloseTab(m_Tabs[m_iCBrowserView-1]);
		else
			CloseTab(m_Tabs[m_iCBrowserView+1]);
	}
}

void CBrowserFrmTab::OnCloseTab()
{
	CloseTab(m_wndCBrowserTab);
}

LRESULT CBrowserFrmTab::OnOpenTab(WPARAM wParam, LPARAM lParam)
{
	CBrowserTab* tab = CreateBrowserTab();
	if (!tab) return 0;

	USES_CONVERSION;
	if (wParam) tab->OpenURL(A2CT((char*)wParam));
	SetActiveBrowser(tab);
	return (LRESULT)tab;	
}

LRESULT CBrowserFrmTab::OnCloseTab(WPARAM wParam, LPARAM lParam)
{
	CBrowserTab* tab = m_Tabs[IDTOTABINDEX(wParam)];
	if (tab)  CloseTab(tab);
	return -1;
}

BOOL CBrowserFrmTab::CloseTab(CBrowserTab* tab)
{
	//if (index <0 || index >=m_iBrowserCount) return FALSE;
	
	// Close if it's a popup
	if (IsPopup() || IsDialog()) {
		SendMessage(WM_CLOSE,0,0);
		return TRUE;
	}

	//If only one tab, close the window.
	if (m_iBrowserCount<2){
		
		switch (theApp.preferences.iOnCloseLastTab)
		{
			case 0:
				SendMessage(WM_CLOSE,0,0);
				return TRUE;
			case 1:
				m_wndCBrowserTab->OpenURL(_T("about:blank"), nullptr);
				return FALSE;
			case 2:
				return FALSE;
		}
	}
	
	// Check if we're closing the last active tab, so we can set the
	// pointer to something valid.
	if (m_pPreviousSelectedTab == tab)
		m_pPreviousSelectedTab = NULL; // Have to fix that
	
	int index = tab->m_iIndex;
	m_Tabs[index] = NULL;
	tab->Destroy();
	//delete m_Tabs[index];
		
	// If we're closing the active tab
	CBrowserTab* newActiveTab;
	if (tab==m_wndCBrowserTab)
	{
		m_wndCBrowserTab = NULL;
		int newTabIndex;
		switch (theApp.preferences.iOnCloseTab)
		{
		case 2:
			if (m_pPreviousSelectedTab){
				newActiveTab = m_pPreviousSelectedTab; 
				break;
			}

		case 0: // Next
			newTabIndex = IDTOTABINDEX(m_wndTabs->GetNextItem(TABINDEXTOID(index),false));
			newActiveTab = m_Tabs[newTabIndex];
			break;
		
		default:
		case 1: // Previous 
			newTabIndex = IDTOTABINDEX(m_wndTabs->GetPreviousItem(TABINDEXTOID(index),false));
			newActiveTab = m_Tabs[newTabIndex];
			break;
		
		}
	}

	m_wndTabs->DeleteItem(TABINDEXTOID(index));
	m_iBrowserCount--;

	// There are better way to do that but ...
	// Update our BrowserView array
	for (int i=index;i<m_iBrowserCount;i++)
	{
		m_Tabs[i] = m_Tabs[i+1];
		m_Tabs[i]->SetIndex(i);
		m_wndTabs->SetItemCommandID(TABINDEXTOID(i + 1), TABINDEXTOID(i));
	}

	if (index==m_iCBrowserView)
	{
		m_iCBrowserView = -1;
		m_wndCBrowserTab = NULL;
		SetActiveBrowser(newActiveTab);
	}
	else if (index<m_iCBrowserView)
		m_iCBrowserView--;

	UpdateTabListMenu();
	return TRUE;
}

void CBrowserFrmTab::OnNextTab()
{
	int newTabID = IDTOTABINDEX(m_wndTabs->GetNextItem(TABINDEXTOID(m_iCBrowserView)));

	/*
	int newBrowser;
	if (m_iCBrowserView<m_iBrowserCount-1)
		newBrowser = m_iCBrowserView + 1;
	else
		newBrowser = 0;*/
	
	SetActiveBrowser(m_Tabs[newTabID]);
}

void CBrowserFrmTab::OnPrevTab()
{
	int newTabID = IDTOTABINDEX(m_wndTabs->GetPreviousItem(TABINDEXTOID(m_iCBrowserView)));
	/*
	int newBrowser;
	if (m_iCBrowserView>0)
		newBrowser = m_iCBrowserView - 1;
	else
		newBrowser = m_iBrowserCount - 1;
	*/
	SetActiveBrowser(m_Tabs[newTabID]);
}

void CBrowserFrmTab::OnLastTab()
{
	SetActiveBrowser(m_pPreviousSelectedTab);
}

LRESULT CBrowserFrmTab::OnGetFavIcon(WPARAM wParam, LPARAM lParam)
{
	nsCOMPtr<nsIURI> sURI;
	nsDependentCString uri;
	int index = IDTOTABINDEX(wParam);
	int iIcon = theApp.favicons.GetDefaultIcon();
/*
	if (m_Tabs[index]->mWebNav) {
		nsresult rv = m_Tabs[index]->mWebNav->GetCurrentURI(getter_AddRefs(sURI));
		if (NS_SUCCEEDED(rv)) {
			iIcon = theApp.favicons.GetIcon(sURI);
		}
	}
*/
	return iIcon;
}

LRESULT CBrowserFrmTab::OnNewSiteIcon(WPARAM url, LPARAM index)
{
	for  (int i =0; i<this->m_iBrowserCount; i++)
	{	
		int icon = theApp.favicons.GetIcon(m_Tabs[i]->GetBrowserGlue()->mIconURI);
		if (icon==-1) {// The icon doesn't exist anymore, was deleted
			m_Tabs[i]->GetBrowserGlue()->mIconURI = nullptr;
			if (i == m_iCBrowserView)
				SetFavIcon(theApp.favicons.GetDefaultIcon());
			SetTabIcon(m_Tabs[i], theApp.favicons.GetDefaultIcon());
		}
		else
			if (index==-1 || index == icon) {
				if (i == m_iCBrowserView)
					SetFavIcon(icon);
				SetTabIcon(m_Tabs[i], icon);
			}
	}

	return 0;
}

void CBrowserFrmTab::OnTbnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
   USES_CONVERSION;
   *pResult = 0;

   LPNMTOOLBAR pNMTB = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);

   UINT cfTabPt = ::RegisterClipboardFormat(CFSTR_TABPT);

   CBrowserTab* tab = (CBrowserTab*)m_Tabs[IDTOTABINDEX(pNMTB->iItem)];
   
   HGLOBAL hTab = GlobalAlloc(GHND, sizeof(CBrowserTab*));
   CBrowserTab** pTab = (CBrowserTab**)GlobalLock(hTab);
   *pTab = tab;
   GlobalUnlock(hTab);

   COleDataSource datasource;
   //datasource.CacheGlobalData(cfTabPt, hTab);
   tab->AddURLAndPerformDrag(datasource);

   GlobalFree(hTab);
}

void CBrowserFrmTab::DrawTabListMenu(HMENU menu)
{
	for (int i=0; i<m_iBrowserCount; i++)
	{
		CBrowserWrapper* wrapper = m_Tabs[i]->GetBrowserWrapper();
		if (!wrapper) continue;

		CString title = wrapper->GetTitle();
		if (title.IsEmpty()) 
			title = m_Tabs[i]->GetBrowserGlue()->mTitle; // For restore_on_demand
		if (title.IsEmpty())
			title = wrapper->GetURI();
		if (title.GetLength()>50) title = title.Left(50) + _T("...");
		AppendMenu(menu, MF_ENABLED | MF_STRING, TABINDEXTOID(i), title);
	}
}

void CBrowserFrmTab::UpdateTabListMenu()
{
	KmMenu* menu = theApp.menus.GetKMenu(_T("@TabList"));
	if (menu) menu->Invalidate();
}

void CBrowserFrmTab::OnShowFindBar()
{
	CBrowserFrame::OnShowFindBar();
	m_wndTabs->UpdateVisibility(); // Ugly hack
}

void CBrowserFrmTab::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	if (nItemID >= TABS_START_ID && nItemID <= TABS_STOP_ID) {
		CBrowserTab* tab = (CBrowserTab*)m_Tabs[IDTOTABINDEX(nItemID)];
		if (tab) {
			UpdateStatus(tab->GetCurrentURI());
			return;
		}
	}
		
	CBrowserFrame::OnMenuSelect(nItemID, nFlags, hSysMenu);
}

/**********************************************/

CBrowserTab::CBrowserTab() : CBrowserView()
{
	m_bActive = false;
	mpFrameTab = NULL;
}

CBrowserTab::~CBrowserTab()
{
}

BOOL CBrowserTab::Create(CBrowserFrmTab* aFrame, int index)
{
	mpFrameTab = aFrame;
	m_iIndex = index;

	return CBrowserView::Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
        CRect(0, 0, 0, 0), aFrame, AFX_IDW_PANE_FIRST+index, NULL);
}

void CBrowserTab::Destroy()
{
	DestroyWindow();
}

void CBrowserTab::SetIndex(int i)
{
	m_iIndex = i;
//	m_xBrowserFrameGlueObj.m_iViewIndex = i;
	SetWindowLong(m_hWnd, GWL_ID, AFX_IDW_PANE_FIRST + i);
}

bool CBrowserTab::SetActive(bool state, bool haveFocus) 
{
	bool old = m_bActive;
	m_bActive = state;

	if (state)
	{
		if (haveFocus && GetParentFrame() == GetActiveWindow())
			CBrowserView::Activate(TRUE);
		ShowWindow(SW_SHOW);
	}
	else
	{
		CBrowserView::Activate(FALSE);
		ShowWindow(SW_HIDE);
	}

	return old;
}

CBrowserFrame* CBrowserTab::OpenURLInNewWindow(LPCTSTR pUrl, LPCTSTR referrer, BOOL bBackground, BOOL allowFixup)
{
	if (!theApp.preferences.GetBool("kmeleon.forceTab", FALSE))
		return CBrowserView::OpenURLInNewWindow(pUrl, referrer, bBackground, allowFixup);

	const TCHAR* ext = _tcschr(pUrl, L'.');

	if (ext && (_tcsstr(ext, _T(".xul")) == ext) && (_tcsncmp(pUrl, _T("chrome:"), 7) == 0))
		return theApp.CreateNewBrowserFrameWithUrl(pUrl, referrer, bBackground);

	if (!((CBrowserGlue*)m_pBrowserGlue)->mLoading &&
		GetCurrentURI() == "about:blank" )
	{
		OpenURL(pUrl, referrer, allowFixup);
		return mpBrowserFrame;
	}

	OpenURLInNewTab(pUrl, referrer, bBackground, allowFixup);
	return mpBrowserFrame;
}

CBrowserTab* CBrowserTab::OpenURLInNewTab(LPCTSTR url, LPCTSTR refferer, BOOL bBackground, BOOL allowFixup)
{
	CBrowserTab* tab;
	if ( (!GetBrowserGlue()->mLoading && GetBrowserGlue()->mHIndex == -1) &&
	     GetCurrentURI().Compare(_T("about:blank")) == 0 )
		tab = this;
	else if ( (tab=mpFrameTab->CreateBrowserTab()) == NULL)
	{
        TRACE0("Failed to create view window\n");
        return NULL;
    }

	tab->OpenURL(url, refferer, allowFixup);

	if (!bBackground)
		mpFrameTab->SetActiveBrowser(tab);
	
	return tab;
}

IMPLEMENT_DYNAMIC(CBrowserTab, CBrowserView)

BEGIN_MESSAGE_MAP(CBrowserTab, CBrowserView)
	ON_WM_CREATE()
    ON_WM_DESTROY()
	ON_COMMAND(ID_OPEN_LINK_IN_NEW_TAB, OnOpenLinkInNewTab)
	ON_COMMAND(ID_OPEN_LINK_IN_BACKGROUNDTAB, OnOpenLinkInBackgroundTab)
	ON_COMMAND(ID_OPEN_FRAME_IN_NEW_TAB, OnOpenFrameInNewTab)
	ON_COMMAND(ID_OPEN_FRAME_IN_BACKGROUNDTAB, OnOpenFrameInBackgroundTab)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
END_MESSAGE_MAP()

int CBrowserTab::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CBrowserView::OnCreate(lpCreateStruct) == -1)
		return -1;

	theApp.plugins.SendMessage("*", "* OnCreateTab", "CreateTab", (long)mpBrowserFrame->m_hWnd, (long)this->m_hWnd);
//	GetParent()->SendMessage(WM_CREATETAB, 0, (LPARAM)this);

	return 0;
}

void CBrowserTab::OnDestroy()
{
	theApp.plugins.SendMessage("*", "* OnDestroyTab", "DestroyTab", (long)mpBrowserFrame->m_hWnd, (long)this->m_hWnd);
	CBrowserView::OnDestroy();
	//GetParent()->SendMessage(WM_CLOSETAB, m_iIndex, (LPARAM)this);
}

void CBrowserTab::OpenURLWithCommand(UINT idCommand, LPCTSTR url, LPCTSTR refferer, BOOL allowFixup) 
{
	switch (idCommand)
	{
        case ID_OPEN_LINK_IN_NEW_TAB:
            OpenURLInNewTab(url, refferer, FALSE, allowFixup);
            break;
        case ID_OPEN_LINK_IN_BACKGROUNDTAB:
            OpenURLInNewTab(url, refferer, TRUE, allowFixup);
            break;
        default:
			CBrowserView::OpenURLWithCommand(idCommand, url, refferer, allowFixup);
            return;
    }
}

void CBrowserTab::OnCloseTab()
{
	theApp.plugins.SendMessage("*", "* OnCloseTab", "CloseTab", (long)mpBrowserFrame->m_hWnd, (long)this->m_hWnd);
	Destroy();
}

void CBrowserTab::OnOpenLinkInBackgroundTab()
{
	CString url, title;
	if (!GetLinkTitleAndHref(m_contextNode, url, title))
		return;
    OpenURLInNewTab(url,  m_pWindow->GetDocURL(m_contextNode), TRUE);
}

void CBrowserTab::OnOpenLinkInNewTab()
{
	CString url, title;
	if (!GetLinkTitleAndHref(m_contextNode, url, title))
		return;
    OpenURLInNewTab(url,  m_pWindow->GetDocURL(m_contextNode), FALSE);
}

void CBrowserTab::OnOpenFrameInNewTab()
{
	CString url = m_pWindow->GetFrameURL(m_contextNode);
	if (url.IsEmpty()) return;
	OpenURLInNewTab(url, GetCurrentURI(), FALSE);
}

void CBrowserTab::OnOpenFrameInBackgroundTab()
{
	CString url = m_pWindow->GetFrameURL(m_contextNode);
	if (url.IsEmpty()) return;
    OpenURLInNewTab(url, GetCurrentURI(), TRUE);
}




void CBrowserFrmTab::OnSysCommand(UINT nID, LPARAM lParam)
{
	CBrowserFrame::OnSysCommand(nID, lParam);	
	if (nID == SC_MAXIMIZE || nID == SC_RESTORE)
		m_wndTabs->FixMaximizeRestoreRebarBug();
}

void CBrowserFrmTab::AllowJS(BOOL allow)
{
	for (int i=0;i<m_iBrowserCount;i++) {
		if (m_Tabs[i])
			m_Tabs[i]->GetBrowserWrapper()->AllowJS(allow);
	}
}
