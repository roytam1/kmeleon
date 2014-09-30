/*
*  Copyright (C) 2000 Brian Harris
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
*/

// this handles plugin loading/unloading

#include "StdAfx.h"
#include "rebar_menu/hot_tracking.h"

#include "nsISHistory.h"
#include "nsISHEntry.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserView.h"
#include "BrowserFrm.h"
#include "BrowserFrmTab.h"

#include "kmeleon_plugin.h"
#include "Plugins.h"
#include "Utils.h"
#include "MenuParser.h"

#include "nsICacheService.h"


int SessionSize=0;
char **pHistory;
char **pHistUrl;
kmeleonDocInfo kDocInfo;
kmeleonPointInfo gPointInfo;


//CBrowserFrame* pBrowserFrame = NULL;
//CBrowserView* pBrowserView = NULL;

#define PLUGIN_HEADER(hWnd, ret) \
	CBrowserFrame* frame;\
	CBrowserView* view;\
	CBrowserWrapper* browser;\
	if (!GetWindows(hWnd, &frame, &view))\
		return ret;\
	if (!view) return ret;\
	browser = view->GetBrowserWrapper();\
	if (!browser)\
		return ret;

static char *safe_strdup(const char *ptr) {
  if (ptr)
    return strdup(ptr);
  return NULL;
}

char *EncodeUTF8(const wchar_t *str)
{
  nsEmbedString aStr;
  aStr.Append(str);
  nsEmbedCString _str;
  NS_UTF16ToCString(aStr, NS_CSTRING_ENCODING_UTF8, _str);
  char *pszStr = safe_strdup(_str.get());
  return pszStr;
}

wchar_t *WDecodeUTF8(const char *str)
{
  nsEmbedString _str;
  NS_CStringToUTF16(nsEmbedCString(str), NS_CSTRING_ENCODING_UTF8, _str);
  wchar_t *pszStr = _wcsdup(_str.get());
  return pszStr;
}

char *EncodeUTF8(const char *str)
{
  USES_CONVERSION;
  return EncodeUTF8(A2CW(str));
}

char *DecodeUTF8(const char *str)
{
  USES_CONVERSION;
  nsEmbedString _str;
  NS_CStringToUTF16(nsEmbedCString(str), NS_CSTRING_ENCODING_UTF8, _str);
  char *pszStr = safe_strdup(W2CA(_str.get()));
  return pszStr;
}

BOOL GetWindows(HWND hWnd, CBrowserFrame** frame, CBrowserView** view)
{
	CWnd* wnd = CWnd::FromHandle(hWnd);

	if (!wnd) {
		*frame = theApp.m_pMostRecentBrowserFrame;
		if (!*frame) return FALSE;
		*view = (*frame)->GetActiveView();
		return TRUE;
	}

	if (wnd->IsKindOf(RUNTIME_CLASS(CBrowserFrame))) {
		*frame = (CBrowserFrame*)wnd;
		*view = (*frame)->GetActiveView();
	}
	else if (wnd->IsKindOf(RUNTIME_CLASS(CBrowserView))) {
		*view = (CBrowserView*)wnd;
		*frame = (CBrowserFrame*)wnd->GetParentFrame();
	}
	else {
		ASSERT(FALSE);
		return FALSE;
	}
	return TRUE;
}

CBrowserFrame* GetFrame(HWND hWnd = NULL) 
{
	if (!hWnd) {
		//We don't want to return a dialog
		if (theApp.m_pMostRecentBrowserFrame && !theApp.m_pMostRecentBrowserFrame->IsDialog())
			return theApp.m_pMostRecentBrowserFrame;

		POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
		CBrowserFrame* pBrowserFrame = NULL;
		while( pos != NULL ) {
			pBrowserFrame = (CBrowserFrame *) theApp.m_FrameWndLst.GetNext(pos);
			if(!pBrowserFrame->IsDialog()) break;
		}
		return pBrowserFrame;
	}
	CWnd* wnd = CWnd::FromHandle(hWnd);

	return (CBrowserFrame*)(wnd->IsFrameWnd() ? wnd : wnd->GetParentFrame());
}

CBrowserWrapper* GetWrapper(HWND hWnd = NULL) 
{
	if (!hWnd) {
		CBrowserView *view = theApp.m_pMostRecentBrowserFrame->GetActiveView();
		if (view) return view->GetBrowserWrapper();
		return NULL;
	}

	CWnd* wnd = CWnd::FromHandle(hWnd);
	if (!wnd) return NULL;

	if (wnd->IsFrameWnd()) {
		ASSERT(wnd->IsKindOf(RUNTIME_CLASS(CBrowserFrame)));
		wnd = ((CBrowserFrame*)wnd)->GetActiveView();
		if (!wnd) return NULL;
	}

	ASSERT(wnd->IsKindOf(RUNTIME_CLASS(CBrowserView)));
	if (!wnd->IsKindOf(RUNTIME_CLASS(CBrowserView)))
		return NULL;

	return ((CBrowserView*)wnd)->GetBrowserWrapper();
}



CPlugins::CPlugins()
{
	kDocInfo.url = NULL;
	kDocInfo.iconurl = NULL;
	kDocInfo.title = NULL;
	gPointInfo.image = NULL;
}

CPlugins::~CPlugins()
{

   if (gPointInfo.image) 
   {
      free(gPointInfo.image);
      free(gPointInfo.link);
      free(gPointInfo.frame);
      free(gPointInfo.page);
	  free(gPointInfo.linktitle);
   }

   if (SessionSize) {
     for (int i=0; i<SessionSize; i++) {
       if (pHistory && pHistory[i])
         delete pHistory[i];
       if (pHistUrl && pHistUrl[i])
          delete pHistUrl[i];
     }
   }
   if (pHistory)
      delete pHistory;
   if (pHistUrl)
      delete pHistUrl;
   pHistory = NULL;
   pHistUrl = NULL;

   if (kDocInfo.url)
      delete kDocInfo.url;

   if (kDocInfo.iconurl)
      delete kDocInfo.iconurl;

   if (kDocInfo.title)
      delete kDocInfo.title;
   
   UnLoadAll();
}

// returns a pointer to the char after the last \ or /
const char *FileNoPath(const char *filepath)
{
   const char *p1 = strrchr(filepath, '\\');
   const char *p2 = strrchr(filepath, '/');
   if (p1 > p2) {
      return p1 + 1;
   }
   else if (p2 > p1) {
      return p2 + 1;
   }
   else {
      return filepath;
   }
}

UINT GetCommandIDs(int num)
{
   return theApp.commands.AllocateId(num);
}

int CPlugins::OnUpdate(UINT command)
{
   if (theApp.commands.IsPluginCommand(command))
      return true;
   return false;
}

BOOL ParsePluginCommand(char *pszCommand, char** plugin, char **parameter)
{
	if (!pszCommand)
		return FALSE;

	*plugin = pszCommand;
	*parameter = strchr(pszCommand, '(');
	if (!*parameter)
		return FALSE;

	char *close = strrchr(*parameter, ')');
	if (!close)
		return FALSE;

	*(*parameter)++ = 0;
	*close = 0;
	return TRUE;
}

HWND NavigateTo(const char *url, int windowState, HWND mainWnd)
{
   CBrowserFrame *frame = GetFrame(mainWnd);
   CBrowserView *view = frame ? frame->GetActiveView() : NULL;

   USES_CONVERSION;
   LPCTSTR lpctUrl = A2CT(url);
   CBrowserFrame* newFrame = NULL;
   CBrowserTab* newTab = NULL;
   BOOL bBackground = FALSE;

   switch(windowState&15) {
   case OPEN_NORMAL:
      if (!frame) return NULL;
      if (lpctUrl) frame->OpenURL(lpctUrl);
      break;
   
   case OPEN_BACKGROUND: 
	   bBackground = TRUE;
   case OPEN_NEW:
      if (!lpctUrl)
         newFrame = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, bBackground);
	  else if (view)
         newFrame = view->OpenURLInNewWindow(lpctUrl, NULL, bBackground);
	  else 
         newFrame = theApp.CreateNewBrowserFrameWithUrl(lpctUrl, NULL, bBackground);
      break;

   case OPEN_BACKGROUNDTAB:
      bBackground = TRUE;
   case OPEN_NEWTAB:
      if (!frame) return NULL;
	  if (!frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab))) {
		  if (!lpctUrl)
			 newFrame = theApp.CreateNewBrowserFrame(nsIWebBrowserChrome::CHROME_ALL, bBackground);
		  else if (view)
			 newFrame = view->OpenURLInNewWindow(lpctUrl, NULL, bBackground);
		  else 
			 newFrame = theApp.CreateNewBrowserFrameWithUrl(lpctUrl, NULL, bBackground);
		  break;
	  }		
      
      if (!lpctUrl)
         newTab = ((CBrowserFrmTab*)frame)->CreateBrowserTab();
      else if (view)
         newTab = ((CBrowserTab*)view)->OpenURLInNewTab(lpctUrl, NULL, TRUE);
      else {
         newTab = ((CBrowserFrmTab*)frame)->CreateBrowserTab();
         if (!newTab) return NULL;
         newTab->OpenURL(lpctUrl);
	  }

      if (!newTab) return NULL;
      if (!bBackground) ((CBrowserFrmTab*)frame)->SetActiveBrowser(newTab);

      if ((windowState & OPEN_CLONE) && view)
         view->CloneBrowser(newTab);
      break;
   }
   
   if (newFrame && frame && (windowState & OPEN_CLONE)) {
	   CBrowserView *view = frame->GetActiveView(); 
	   if (view) view->CloneBrowser(newFrame->GetActiveView());
   }

   return newTab ? newTab->m_hWnd : 
      (newFrame ? newFrame->m_hWnd : 
	     (frame ? frame->GetActiveView()->m_hWnd : NULL));
}

void _NavigateTo(const char *url, int windowState, HWND mainWnd)
{
   NavigateTo(url, windowState, mainWnd);
}

kmeleonDocInfo * GetDocInfoUTF8(HWND mainWnd)
{
   PLUGIN_HEADER(mainWnd, NULL);
   USES_CONVERSION;

   CString url = browser->GetURI();
   char* docurl = EncodeUTF8(url);

   CString title = browser->GetTitle();
   char* doctitle = EncodeUTF8(title);

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.iconurl)
      delete kDocInfo.iconurl;

   if (kDocInfo.title)
      delete kDocInfo.title;

   kDocInfo.title = doctitle;
   kDocInfo.url = docurl;
      
#ifdef INTERNAL_SITEICONS
   nsCString uri;
   if (view->GetBrowserGlue()->mIconURI) {
      view->GetBrowserGlue()->mIconURI->GetSpec(uri);
      kDocInfo.iconurl = strdup(uri.get());
      kDocInfo.idxIcon = theApp.favicons.GetIcon(view->GetBrowserGlue()->mIconURI);
   }
#endif

   return &kDocInfo;
}

kmeleonDocInfo * GetDocInfo(HWND mainWnd)
{
   PLUGIN_HEADER(mainWnd, NULL);
   USES_CONVERSION;

   CString url = browser->GetURI();
   char* docurl = strdup(T2A(url));
   
   CString title = browser->GetTitle();
   char* doctitle = strdup(T2A(title));

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.title)
      delete kDocInfo.title;

   kDocInfo.title = doctitle;
   kDocInfo.url = docurl;
#ifdef INTERNAL_SITEICONS
   kDocInfo.idxIcon = theApp.favicons.GetIcon(view->GetBrowserGlue()->mIconURI);
#endif

   return &kDocInfo;
}

HIMAGELIST GetIconList()
{
#ifdef INTERNAL_SITEICONS
	return theApp.favicons.GetSafeHandle();
#else
	return NULL;
#endif
}

long GetPreference(enum PREFTYPE type, const char *preference, void *ret, void *defVal)
{
   long result = 0;
   switch (type) {
   case PREF_BOOL:
      *(int *)ret = result = theApp.preferences.GetBool(preference, *(int *)defVal);
      break;
   case PREF_INT:
      *(int *)ret = result = theApp.preferences.GetInt(preference, *(int *)defVal);
      break;
   case PREF_STRING:
      result = theApp.preferences.GetString(preference, (char *)ret, (char *)defVal);
      break;
   case PREF_LOCALIZED:
      result = theApp.preferences.GetLocaleString(preference, (char *)ret, (char *)defVal);
      break;
   case PREF_UNISTRING:
      result = theApp.preferences.GetString(preference, (wchar_t *)ret, (wchar_t *)defVal);
      break;
   }
   return result;
}

void _GetPreference(enum PREFTYPE type, char *preference, void *ret, void *defVal)
{
   GetPreference(type, preference, ret, defVal);
}

void SetPreference(enum PREFTYPE type, const char *preference, void *val, BOOL update)
{
   //theApp.preferences.Save(false);

   switch (type) {
      case PREF_BOOL:
         theApp.preferences.SetBool(preference, *(int *)val);
         break;
      case PREF_INT:
         theApp.preferences.SetInt(preference, *(int *)val);
         break;
      case PREF_STRING:
         theApp.preferences.SetString(preference, (char *)val);
         break;
      case PREF_UNISTRING:
         theApp.preferences.SetString(preference, (wchar_t *)val);
         break;
   }

   if (update)
      theApp.preferences.Flush();
   //theApp.preferences.Load();
}

void DelPreference(const char *preference)
{
   theApp.preferences.Clear(preference);
   //theApp.preferences.Flush();
}

void SetStatusBarTextT(const TCHAR *s)
{
   if (theApp.m_pMostRecentBrowserFrame) // Yes, it can happen
       theApp.m_pMostRecentBrowserFrame->UpdateStatus(s);
}

void SetStatusBarTextUTF8(const char *s)
{
   USES_CONVERSION;
   wchar_t *ws = WDecodeUTF8(s);
   SetStatusBarTextT(W2CT(ws));
   free(ws);
}

void SetStatusBarText(const char *s)
{
   USES_CONVERSION;
   SetStatusBarTextT(A2CT(s));
}

#include "nsISHistoryInternal.h"
#include "MozUtils.h"

int SetMozillaSessionHistory (HWND hWnd, const char **titles, const char **urls, int count, int index)
{
   nsresult rv;
   if (count<1) return TRUE;

   CBrowserWrapper *browser = GetWrapper(hWnd);
   if (!browser) return 0;

   nsCOMPtr<nsISHistory> sHistory;
   if (!browser->GetSHistory(getter_AddRefs(sHistory)))
		return 0;

   nsCOMPtr<nsISHistoryInternal> sHInternal(do_QueryInterface(sHistory));
   NS_ENSURE_TRUE(sHInternal, FALSE);

   PRInt32 shcount;
   sHistory->GetCount(&shcount);
   sHistory->PurgeHistory(shcount);

   USES_CONVERSION;
   for (int i=0;i<count;i++)
   {
	   nsCOMPtr<nsISHEntry> newSHEntry = do_CreateInstance(NS_SHENTRY_CONTRACTID);
	   if (!newSHEntry) continue;

	   nsCOMPtr<nsIURI> nsuri;
	   NewURI(getter_AddRefs(nsuri), nsEmbedCString(urls[i]));
	   if (!nsuri) continue;

	   nsString wTitle;
	   nsDependentCString title(titles[i]);
	   CopyUTF8toUTF16(title, wTitle);
	   rv = newSHEntry->Create(nsuri, wTitle, nullptr, nullptr, nullptr, nsEmbedCString(""), nullptr, 0, true);
	   if (NS_SUCCEEDED(rv)) sHInternal->AddEntry(newSHEntry, PR_TRUE);
   }

	if (GetFrame(hWnd)->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)) && theApp.preferences.GetBool("browser.sessionstore.restore_on_demand", false)) {
		if (index>=count) index = count - 1;
		CBrowserView* view = GetFrame(hWnd)->GetActiveView();
		view->GetBrowserGlue()->SetBrowserTitle(NSUTF8StringToCString(nsDependentCString(titles[index])));
		nsCOMPtr<nsIURI> uri;
		NewURI(getter_AddRefs(uri), nsDependentCString(urls[index]));
		view->GetBrowserGlue()->UpdateCurrentURI(uri);
		view->GetBrowserGlue()->mPendingLocation = urls[index];
		view->GetBrowserGlue()->mHIndex = index;
		view->GetBrowserGlue()->mIcon = theApp.favicons.GetHostIcon(A2CW(urls[index]));
		uri->SetPath(NS_LITERAL_CSTRING(""));
		nsCString host;
		uri->GetHost(host);
		host.Insert("http://", 0, 7);
		uri->SetSpec(host);
		view->GetBrowserGlue()->mIconURI = uri;
		((CBrowserFrmTab*) GetFrame(hWnd))->SetTabIcon((CBrowserTab*)view, view->GetBrowserGlue()->mIcon);
		// Let the session plugin know about it
		GetFrame(hWnd)->PostMessageW(UWM_UPDATEBUSYSTATE,0,(LPARAM)hWnd);
	} else 
		browser->GotoHistoryIndex(index);
   return TRUE;
}

int GetMozillaSessionHistory (HWND hWnd, char ***titles, char ***urls, int *count, int *index)
{
   nsresult result;
   int i;

   CBrowserWrapper *browser = GetWrapper(hWnd);
   if (!browser) return 0;

	nsCOMPtr<nsISHistory> h;
	if (!browser->GetSHistory(getter_AddRefs(h)))
		return 0;
   
   h->GetCount (count);
   h->GetIndex (index);

   // Clear the previous table
   if (SessionSize) {
      for (i=0; i<SessionSize; i++) {
         if (pHistory && pHistory[i]) {
            delete [] pHistory[i];
			pHistory[i] = NULL;
		 }
		 if (pHistUrl && pHistUrl[i]) {
			delete [] pHistUrl[i];
		    pHistUrl[i] = NULL;
		 }
      }
   }
   
   SessionSize = *count;
   
   if (pHistory) {
      delete [] pHistory;
	  pHistory = NULL;
   }

   if (!SessionSize) return FALSE;
   pHistory = new char *[SessionSize];
   
   nsCOMPtr<nsIHistoryEntry> he;
   PRUnichar *title;
   
   if (pHistUrl)
      delete [] pHistUrl;
   pHistUrl = new char *[SessionSize];
   
   nsCOMPtr<nsIURI> theUri;
   nsCString uri;
   
   for (i=0; i < SessionSize; i++) {
      pHistory[i] = NULL;
      pHistUrl[i] = NULL;
   }
   
   for (i=0; i < SessionSize; i++) {
      result = h->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs (he));
      if (!NS_SUCCEEDED(result) || (!he)) return FALSE;
      
      result = he->GetURI(getter_AddRefs(theUri));
      if (!NS_SUCCEEDED(result) || (!theUri)) return FALSE;
      
      theUri->GetSpec(uri);
      char *t = strdup(uri.get());

      pHistUrl[i] = t;
      
      result = he->GetTitle (&title);
      if (!NS_SUCCEEDED(result) || (!title)) return FALSE;
      
      // The title is in 16-bit unicode, this converts it to 8bit (UTF)
      int len;
      len = WideCharToMultiByte(CP_UTF8, 0, title, -1, 0, 0, NULL, NULL);
      char *s = new char[len+1];
      len = WideCharToMultiByte(CP_UTF8, 0, title, -1, s, len, NULL, NULL);
      s[len] = 0;
      pHistory[i] = s;
     nsMemory::Free(title);
   }
   
   if (titles)
      *titles = pHistory;
   if (urls)
      *urls = pHistUrl;

   return TRUE;
}

int _GetMozillaSessionHistory (char ***titles, char ***urls, int *count, int *index)
{
   return GetMozillaSessionHistory(NULL, titles, urls, count, index);
}

void GotoHistoryIndex(UINT index)
{
   CBrowserWrapper *browser = GetWrapper();
   if (!browser) return;

   browser->GotoHistoryIndex(index);
}

void RegisterBand(HWND hWnd, char *name, int visibleOnMenu)
{
   USES_CONVERSION;
   theApp.m_pMostRecentBrowserFrame->m_wndReBar.RegisterBand(hWnd, A2T(name), visibleOnMenu);
}

// This lets a plugin create a toolbar within the current browser frame
// the advantage of having K-Meleon create the toolbar is that it will
// be handled through MFC, which will handle the button states through
// UPDATE_UI calls
HWND CreateToolbar(HWND hWnd, UINT style) {
   CBrowserFrame *frame = GetFrame(hWnd);
   if (!frame) return NULL;

   return frame->CreateToolbar(style);
}

int GetID(const char *strID) {
   USES_CONVERSION;
   return theApp.commands.GetId(A2CT(strID));
}

/*
kmeleonPointInfo *GetInfoAtNode(nsIDOMNode* aNode)
{
   delete gPointInfo.image;
   delete gPointInfo.link;
   delete gPointInfo.frame;
   delete gPointInfo.page;
   gPointInfo.image = NULL;
   gPointInfo.link  = NULL;
   gPointInfo.frame = NULL;
   gPointInfo.page  = NULL;

   if (!aNode) 
      return &gPointInfo;

   	CBrowserFrame *pBrowserFrame = GetFrame(NULL);
	if (!pBrowserFrame) return &gPointInfo;

	CBrowserView *pBrowserView = pBrowserFrame->GetActiveView(); 
	if (!pBrowserView) return &gPointInfo;

   // get the page url
   USES_CONVERSION;
   CString url = pBrowserView->GetCurrentURI();
   gPointInfo.page = strdup(T2CA(url));

   nsEmbedString strBuf;
   nsresult rv = NS_OK;


   // check if there's a link
   // Search for an anchor element
   nsCOMPtr<nsIDOMHTMLAnchorElement> linkElement;
   nsCOMPtr<nsIDOMNode> node = aNode;
   while (node) {
      linkElement = do_QueryInterface(node);
      if (linkElement)
         break;
      
      nsCOMPtr<nsIDOMNode> parentNode;
      node->GetParentNode(getter_AddRefs(parentNode));
      node = parentNode;
   }
   if (linkElement) {
      rv = linkElement->GetHref(strBuf);
      if(NS_SUCCEEDED(rv)) {
         if (strBuf.Length()) {
            gPointInfo.link = new char[strBuf.Length() + 1];
            UTF16ToCString(strBuf, gPointInfo.link);
         }
      }
   }


   // check for an image
   nsCOMPtr<nsIDOMHTMLImageElement> imgPointInfo(do_QueryInterface(aNode, &rv));
   if(NS_SUCCEEDED(rv)) {
      rv = imgPointInfo->GetSrc(strBuf);
      if(NS_SUCCEEDED(rv)) {
         gPointInfo.image = new char[strBuf.Length() + 1];
         UTF16ToCString(strBuf, gPointInfo.image);
      }
   }


   // get the current Frame URL
   nsCOMPtr<nsIDOMDocument> domDoc;
   rv = aNode->GetOwnerDocument(getter_AddRefs(domDoc));
   
   if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(domDoc, &rv));
      if(NS_SUCCEEDED(rv)) {
         rv = htmlDoc->GetURL(strBuf);
         if(NS_SUCCEEDED(rv)) {
            gPointInfo.frame = new char[strBuf.Length() +1];
            UTF16ToCString(strBuf, gPointInfo.frame);
         }
      }
   }


   return &gPointInfo;
}
*/
kmeleonPointInfo *GetInfoAtClick(HWND hWnd)
{
   if (gPointInfo.image)
   {
      delete gPointInfo.image;
      delete gPointInfo.link;
      delete gPointInfo.frame;
      delete gPointInfo.page;
      delete gPointInfo.linktitle;
   }
	gPointInfo.image = NULL;
	gPointInfo.link  = NULL;
	gPointInfo.frame = NULL;
	gPointInfo.page  = NULL;
	gPointInfo.linktitle = NULL;

	CBrowserFrame *pBrowserFrame = GetFrame(hWnd);
	if (!pBrowserFrame) return &gPointInfo;

	CBrowserView *pBrowserView = pBrowserFrame->GetActiveView(); 
	if (!pBrowserView) return &gPointInfo;

	USES_CONVERSION;
	gPointInfo.page = strdup(T2CA(pBrowserView->GetCurrentURI()));
	gPointInfo.link = strdup(T2CA(pBrowserView->GetContextLinkUrl()));
	gPointInfo.frame = strdup(T2CA(pBrowserView->GetContextFrameUrl()));
	gPointInfo.image = strdup(T2CA(pBrowserView->GetContextImageUrl()));
	gPointInfo.linktitle = strdup(T2CA(pBrowserView->GetContextLinkTitle()));
	gPointInfo.isInput = pBrowserView->IsContextInputOrObject();
	return &gPointInfo;
}

kmeleonPointInfo *GetInfoAtPoint(int x, int y)
{
	return NULL;
	/*
   if (!theApp.m_pMostRecentBrowserFrame || !theApp.m_pMostRecentBrowserFrame->m_wndBrowserView)
      return GetInfoAtNode(nullptr);

   CBrowserView *pBrowserView;  
   pBrowserView = &theApp.m_pMostRecentBrowserFrame->m_wndBrowserView;

   // get the DOMNode at the point
   nsCOMPtr<nsIDOMNode> aNode;
   aNode = pBrowserView->GetNodeAtPoint(x, y, TRUE);
   return GetInfoAtNode(aNode);*/
}

// return 0 if the function did not succeed (ie, trying to open a link from
// a point that is not a link)
// return 1 if it does succeed

int CommandAtPoint(int command, WORD x, WORD y)
{
	return 0;
	/*
	CBrowserFrame *pBrowserFrame = theApp.m_pMostRecentBrowserFrame;
	if (!pBrowserFrame) return FALSE;

	CBrowserView *pBrowserView = pBrowserFrame->GetActiveView(); 
	if (!pBrowserView) return FALSE;

	CBrowserWrapper *browser = pBrowserView->GetBrowserWrapper(); 
	if (!browser) return FALSE;

  pBrowserView->Activate(TRUE);

   
   pBrowserView->GetNodeAtPoint(x, y, TRUE);

   switch (command) {
   case ID_OPEN_LINK:
   case ID_OPEN_LINK_IN_NEW_WINDOW:
   case ID_OPEN_LINK_IN_BACKGROUND:
   case ID_SAVE_LINK_AS:
   case ID_COPY_LINK_LOCATION:
      if (!pBrowserView->mCtxMenuLinkUrl.Length())
         return 0;
      break;

   case ID_VIEW_IMAGE:
   case ID_SAVE_IMAGE_AS:
   case ID_COPY_IMAGE_LOCATION:
   case ID_COPY_IMAGE_CONTENT:
      if (!pBrowserView->mCtxMenuImgSrc.Length())
         return 0;
      break;

   case ID_OPEN_FRAME:
   case ID_OPEN_FRAME_IN_BACKGROUND:
   case ID_OPEN_FRAME_IN_NEW_WINDOW:
   case ID_VIEW_FRAME_SOURCE:
      if (!pBrowserView->mCtxMenuCurrentFrameURL.Length())
         return 0;
      break;
   }

   pBrowserView->mpBrowserFrame->PostMessage(WM_COMMAND, command, NULL);
   return 1;*/
}

UINT GetWindowVar(HWND hWnd, WindowVarType type, void* ret)
{
    PLUGIN_HEADER(hWnd, 0);

	USES_CONVERSION;
	UINT retLen = 0;
	switch (type)  {

		case Window_UrlBar: {
			CString url = frame->m_wndUrlBar.GetEnteredURL();
			retLen = url.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(url));
			break;
		}

		case Window_Charset: {
			char charset[64] = {0};
			browser->GetCharset(charset);
			retLen = strlen(charset) + 1;
			if (ret) {
				strcpy((char *)ret, charset);
				for(UINT i=0;i<retLen;i++)
					((char*)ret)[i] = tolower(((char*)ret)[i]);
			}
			break;
		}

		case Window_Title: {
			CString title = browser->GetTitle();
			retLen = title.GetLength() + 1;
			if (ret) strcpy((char *)ret, T2CA(title));
				break;
		}

		case Window_TextZoom: {
			int tz = (int)(browser->GetTextSize() * 10.0);
			if (ret) *(int*)ret = tz;
			return 1;
		}
		
		case Window_URL: {
			CString url = browser->GetURI();
			retLen = url.GetLength() + 1;
			if (ret)				
				strcpy((char*)ret, T2CA(url));
			break;
		}

		
		case Window_SelectedText: {
			nsEmbedString sel;  
			browser->GetUSelection(sel);
			retLen = sel.Length() + 1;
			if (ret) wcscpy((wchar_t*)ret, sel.get());
			break;
		}

		case Window_LinkURL: {
			CString url = view->GetContextLinkUrl();
			retLen = url.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(url));
			break;
		}

		case Window_ImageURL: {
			CString url = view->GetContextImageUrl();
			retLen = url.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(url));
			break;
		}

		case Window_LinkTitle: {
			CString title = view->GetContextLinkTitle();
			retLen = title.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(title));
			break;
		}

		case Window_FrameURL: {
			CString url = view->GetContextFrameUrl();
			retLen = url.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(url));
			break;
		}

		case Window_Number: {
			if (ret) *(int*)ret = theApp.m_FrameWndLst.GetCount();
			return 1;
		}

		case Window_Tab_Number: {
			if (!ret) return 1;
			if (frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)))
				*(int*)ret = ((CBrowserFrmTab*)frame)->GetTabCount();
			else
				*(int*)ret = 1;
			return 1;
		}

		case Window_Icon: {
			if (ret) *(int*)ret = theApp.favicons.GetIcon(view->GetBrowserGlue()->mIconURI);
			return 1;
		}

		case Search_URL: {
			CString _url = GetSearchURL(_T("__query__"));
			char* url = EncodeUTF8(_url);
			retLen = strlen(url) + 1;
			if (ret) strcpy((char*)ret, url);
			break;
		}

		default: 
			retLen = 0;
	}

	return retLen;
}

UINT GetWindowVarUTF8(HWND hWnd, WindowVarType type, void* ret)
{
    PLUGIN_HEADER(hWnd, 0);

	USES_CONVERSION;
	UINT retLen = 0;
	switch (type)  {

		case Window_UrlBar: {
			CString url = frame->m_wndUrlBar.GetEnteredURL();
			retLen = url.GetLength() + 1;
			if (ret) {
				char* utf = EncodeUTF8(url);
				strcpy((char*)ret, utf);
				delete utf;
			}
			break;
		}

		case Window_Charset: {
			char charset[64] = {0};
			browser->GetCharset(charset);
			retLen = strlen(charset) + 1;
			if (ret) {
				strcpy((char *)ret, charset);
				for(UINT i=0;i<retLen;i++)
					((char*)ret)[i] = tolower(((char*)ret)[i]);
			}
			break;
		}

		case Window_Title: {
			CString title = browser->GetTitle();
			retLen = title.GetLength() * 3 + 1;
			if (ret) {
				char* utf = EncodeUTF8(title);
				strcpy((char*)ret, utf);
				delete utf;
			}
			break;
		}

		case Window_TextZoom: {
			int tz = (int)(browser->GetTextSize() * 10.0);
			if (ret) *(int*)ret = tz;
			return 1;
		}
		
		case Window_URL: {
			CString url = browser->GetURI();
			retLen = url.GetLength() + 1;
			if (ret) {
				char* utf = EncodeUTF8(url);
				strcpy((char*)ret, utf);
				delete utf;
			}
			break;
		}

		
		case Window_SelectedText: {
			nsEmbedString sel;  
			browser->GetUSelection(sel);
			retLen = sel.Length() * 3 + 1;
			if (ret) {
				char* utf = EncodeUTF8(sel.get());
				strcpy((char*)ret, utf);
				free(utf);
			}
			break;
		}

		case Window_LinkURL: {
			CString url = view->GetContextLinkUrl();
			retLen = url.GetLength() + 1;
			if (ret) strcpy((char*)ret, T2CA(url));
			break;
		}

		case Window_ImageURL: {
			CString url = view->GetContextImageUrl();
			retLen = url.GetLength() + 1;
			if (ret) {
				char* utf = EncodeUTF8(url);
				strcpy((char*)ret, utf);
				free(utf);
			}
			break;
		}

		case Window_LinkTitle: {
			CString title = view->GetContextLinkTitle();
			retLen = title.GetLength() * 3 + 1;
			if (ret) {
				char* utf = EncodeUTF8(title);
				strcpy((char*)ret, utf);
				delete utf;
			}
			break;
		}

		case Window_FrameURL: {
			CString url = view->GetContextFrameUrl();
			retLen = url.GetLength() + 1;
			if (ret) {
				char* utf = EncodeUTF8(url);
				strcpy((char*)ret, utf);
				free(utf);
			}
			break;
		}

		case Window_Number: {
			if (ret) *(int*)ret = theApp.m_FrameWndLst.GetCount();
			return 1;
		}

		case Window_Tab_Number: {
			if (!ret) return 1;
			if (frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)))
				*(int*)ret = ((CBrowserFrmTab*)frame)->GetTabCount();
			else
				*(int*)ret = 1;
			return 1;
		}
		
		case Window_Icon: {
			if (ret) *(int*)ret = theApp.favicons.GetIcon(view->GetBrowserGlue()->mIconURI);
			return 1;
		}

		default: 
			retLen = 0;
	}

	return retLen;
}


BOOL SetWindowVar(HWND hWnd, WindowVarType type, void* value)
{
	PLUGIN_HEADER(hWnd, FALSE);

	USES_CONVERSION;
	BOOL result = FALSE;

	switch (type)  {

		case Window_UrlBar:
			frame->UpdateLocation(A2CT((char*)value), TRUE);
		    result = TRUE;
			break;

		case Window_Charset:
			browser->ForceCharset((char*)value);
			result = TRUE;
			break;

		case Window_Title:
			frame->UpdateTitle(A2CT((char*)value));
			 //if (pBrowserView->m_pBrowserFrameGlue)
		      //pBrowserView->m_pBrowserFrameGlue->SetBrowserFrameTitle(A2CW((char*)value));
			 result = TRUE;
			break;

		case Window_TextZoom: {
			int zoom = (int)(browser->GetTextSize() * 10.0);
			browser->ChangeTextSize(*(int*)value - zoom);
			result = TRUE;
			break;
		}

		case Window_URL:
			browser->LoadURL(A2CT((char*)value));
			break;
	}

	return result;
}

BOOL SetWindowVarUTF8(HWND hWnd, WindowVarType type, void* value)
{
	PLUGIN_HEADER(hWnd, FALSE);

	USES_CONVERSION;
	BOOL result = FALSE;
	wchar_t *ws;

	switch (type)  {

		case Window_UrlBar:
			ws = WDecodeUTF8((char*)value);
			frame->UpdateLocation(W2CT(ws), TRUE);
		    result = TRUE;
			free(ws);
			break;

		case Window_Charset:
			browser->ForceCharset((char*)value);
			result = TRUE;
			break;

		case Window_Title:
			ws = WDecodeUTF8((char*)value);
			frame->UpdateTitle(W2CT(ws));
			 //if (pBrowserView->m_pBrowserFrameGlue)
		      //pBrowserView->m_pBrowserFrameGlue->SetBrowserFrameTitle(A2CW((char*)value));
			result = TRUE;
			delete ws;
			break;

		case Window_TextZoom: {
			int zoom = (int)(browser->GetTextSize() * 10.0);
			browser->ChangeTextSize(*(int*)value - zoom);
			result = TRUE;
			break;
		}

		case Window_URL:
			ws = WDecodeUTF8((char*)value);
			browser->LoadURL(W2CT(ws));
			delete ws;
			break;
	}

	return result;
}

int SetGlobalVar(enum PREFTYPE type, const char *preference, void *value)
{
	if (type == PREF_STRING) {
		if (!stricmp(preference, "URLBAR")) return SetWindowVar(NULL, Window_UrlBar, value);
		else if (!stricmp(preference, "CHARSET")) return SetWindowVar(NULL, Window_Charset, value);
		else if (!stricmp(preference, "TITLE")) return SetWindowVar(NULL, Window_Title, value);
	}
	else if (type == PREF_INT)
		if (!stricmp(preference, "TextZoom")) return SetWindowVar(NULL, Window_TextZoom, value);

	return 0;
}

int GetGlobalVar(enum PREFTYPE type, char *preference, void *ret) {

	switch (type) {
	case PREF_UNISTRING:
		 if (!stricmp(preference, "SelectedText")) 
			return GetWindowVar(NULL, Window_SelectedText, ret);
		 break;
	 case PREF_STRING:
		 if (!stricmp(preference, "URL")) return GetWindowVar(NULL, Window_URL, ret);
		 else if (!stricmp(preference, "URLBAR")) return GetWindowVar(NULL, Window_URL, ret);
		 else if (!stricmp(preference, "LinkURL")) return GetWindowVar(NULL, Window_LinkURL, ret);
		 else if (!stricmp(preference, "ImageURL")) return GetWindowVar(NULL, Window_ImageURL, ret);
		 else if (!stricmp(preference, "FrameURL")) return GetWindowVar(NULL, Window_FrameURL, ret);
		 else if (!stricmp(preference, "TITLE")) return GetWindowVar(NULL, Window_LinkTitle, ret);
		 else if (!stricmp(preference, "SelectedText")) {
			 if (!ret) return GetWindowVar(NULL, Window_SelectedText, 0);
			 else {
				 USES_CONVERSION;
			    wchar_t* uret = new wchar_t[GetWindowVar(NULL, Window_SelectedText, 0) + 1];
				int retlen = GetWindowVar(NULL, Window_SelectedText, ret);
				strcpy((char*)ret, W2A(uret));
				delete [] uret;
				return retlen;
			  }
		 }
		 else if (!stricmp(preference, "CHARSET")) return GetWindowVar(NULL, Window_Charset, ret);
		 break;

	case PREF_INT:
	  if (!stricmp(preference, "TextZoom")) return GetWindowVar(NULL, Window_TextZoom, ret);
      break;
	}

	return 0;
/*


   if (!theApp.m_pMostRecentBrowserFrame) 
     return -1;

	CBrowserView *view = theApp.m_pMostRecentBrowserFrame->GetActiveView(); 
	if (!view) return -1;

	CBrowserWrapper *browser = view->GetBrowserWrapper(); 
	if (!browser) return -1;

	int retLen = -1;
USES_CONVERSION;

   switch (type) {
   case PREF_UNISTRING:
	  if (!stricmp(preference, "SelectedText")) {
		nsEmbedString sel;  
		browser->GetUSelection(sel);
		retLen = sel.Length();
		 if (ret) wcscpy((wchar_t*)ret, sel.get());
	  }
	  break;
   case PREF_STRING:
	  if (!stricmp(preference, "URL")) {
	     CString url = browser->GetURI();
		 retLen = url.GetLength();
		 if (ret) strcpy((char*)ret, T2CA(url));
      }
	  else if (!stricmp(preference, "URLBAR")) {
		 CString url;
		 theApp.m_pMostRecentBrowserFrame->m_wndUrlBar.GetEnteredURL(url);
         retLen = url.GetLength();
		 if (ret) strcpy((char*)ret, T2CA(url));
      }
      else if (!stricmp(preference, "LinkURL")) {
		 retLen = view->m_ctxData.linkUrl.GetLength();
         if (ret) strcpy((char *)ret, T2CA(view->m_ctxData.linkUrl));
      }
      else if (!stricmp(preference, "ImageURL")) {
         retLen = view->m_ctxData.imageUrl.GetLength();
         if (ret) strcpy((char *)ret, T2CA(view->m_ctxData.imageUrl));
      }
      else if (!stricmp(preference, "FrameURL")) {
         retLen = view->m_ctxData.frameUrl.GetLength();
         if (ret) strcpy((char *)ret, T2CA(view->m_ctxData.frameUrl));
      }
      else if (!stricmp(preference, "TITLE")) {
         CString title = view->GetPageTitle();
         retLen = title.GetLength();
         if (ret) strcpy((char *)ret, T2CA(title));
      }
	  else if (!stricmp(preference, "SelectedText")) {
		CString sel;  
		browser->GetSelection(sel);
		retLen = sel.GetLength();
		 if (ret) {
			 strcpy((char*)ret, T2CA(sel));
		 }
	  }
	  else if (!stricmp(preference, "CHARSET")) {
		 char charset[64] = {0};
         browser->GetCharset(charset);
         retLen = strlen(charset);
		 if (ret) {
			 strcpy((char *)ret, charset);
			 for(int i=0;i<retLen;i++)
				((char*)ret)[i] = tolower(((char*)ret)[i]);
		 }
      }
	  break;

	case PREF_INT:
	  if (!stricmp(preference, "TextZoom"))  {
		  float textzoom;
		  browser->GetTextSize(textzoom);
		  if (ret) *(int*)ret = (int)(textzoom * 10.0);
		  return 0;
	  }
      break;
   }
 
   return retLen;*/   
}

void GetBrowserviewRect(HWND mainWnd, RECT *rc)
{
   	CBrowserFrame *frame = GetFrame(mainWnd);
	if (!frame) return;

	CBrowserView *view = frame->GetActiveView(); 
	if (!view) return;

	view->GetWindowRect(rc);
}

HMENU GetMenu(char *menuName){
   CMenu *menu;
   USES_CONVERSION;
   menu = theApp.menus.GetMenu(A2T(menuName));

   return menu ? menu->m_hMenu : NULL;
}

void SetForceCharset(const char *aCharset)
{
	CBrowserWrapper* browser = GetWrapper();
	if (!browser) return;

    browser->ForceCharset(aCharset);
}

void SetCheck(int id, BOOL mark) {
  theApp.menus.SetCheck(id, mark);
}

void ClearCache(int cache) {
   nsresult rv;

   nsCOMPtr<nsICacheService> CacheService =
      do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
   if (NS_FAILED(rv)) return;

   CacheService->EvictEntries(cache);
}

void BroadcastMessage(UINT Msg, WPARAM wParam, LPARAM lParam) {
  theApp.BroadcastMessage(Msg, wParam, lParam);
}

void ParseAccel(char *str) {
  theApp.accel.Parse(str);
}

void SetAccel(const char* key, const char* command) {
  theApp.accel.SetAccel(key, command);
}

void SetMenu(const char* menu, kmeleonMenuItem* kmitem) {
	KmMenuItem item;
	item.type = (KmMenuType)kmitem->type;
	item.SetLabel(kmitem->label);
	item.command = kmitem->command;
	item.groupid = kmitem->groupid;

   USES_CONVERSION;
	theApp.menus.SetMenu(A2CT(menu), item, kmitem->before);
}

void RebuildMenu(const char* menu) {
	USES_CONVERSION;
	theApp.menus.Rebuild(A2CT(menu));
}


kmeleonPlugin * Load(const char *kplugin) {
  USES_CONVERSION;
  return theApp.plugins.Load(CString(A2CT(kplugin)));
}

long CPlugins::SendMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   long retVal = 0;

   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc( pos, s, kPlugin);
      if (kPlugin->loaded && kPlugin->DoMessage) {
         retVal += kPlugin->DoMessage(to, from, subject, data1, data2);
      }
   }
   return retVal;
}

long SendMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   // * is reserved for internal k-meleon messages.  plugins may not use it
   if (from[0] == '*') {
      return 0;
   }

   return theApp.plugins.SendMessage(to, from, subject, data1, data2);
}

int RegisterSideBar (HWND mainWnd, TCHAR *name, SideBarInitProc proc, int commandID, int visibleOnMenu)
{
#ifdef INTERNAL_SIDEBAR
	CSideBar *sidebar = (CSideBar *)CWnd::FromHandle(mainWnd);
	if (!sidebar) return -1;
	USES_CONVERSION;
	return sidebar->RegisterSideBar(name, proc, commandID, visibleOnMenu);
#else
	return -1;
#endif
}

void ToggleSideBar (HWND mainWnd, int index)
{
#ifdef INTERNAL_SIDEBAR
	CBrowserFrame *frame = GetFrame(mainWnd);
	if (!frame) return;

	frame->m_wndSideBar.ToggleVisibility(index);
#endif
}

const char* Translate(const char* text)
{
	USES_CONVERSION;
#ifndef _UNICODE
	return theApp.lang.Translate(A2CT(text));
#else
   static char* Translated = NULL;
   if (Translated) {
      free(Translated);
      Translated = NULL;
   }

   const WCHAR* uText = A2CW(text);
   const WCHAR* uTranslated = theApp.lang.Translate(uText);
   if (uTranslated == uText)
      return text;

   Translated = strdup(W2CA(uTranslated));
   return Translated;
#endif
}

const char* TranslateUTF8(const char* text)
{
	static char* translated = NULL;
	if (translated) {
      free(translated);
      translated = NULL;
	}
#ifdef _UNICODE	
	wchar_t* wtext = WDecodeUTF8(text);
	translated = EncodeUTF8(theApp.lang.Translate(wtext));
	free(wtext);
	return translated;
#else
	char* atext = DecodeUTF8(text);
	char* t  theApp.lang.Translate(A2CT(text));
	free(atext);
	return t;
#endif
}

int TranslateEx(const char* originalText,  TCHAR* translatedText, int bufferlen, BOOL forMenu)
{
	CString csTrans;
	char* accel = 0;
	int IsTranslated;

	if (forMenu)
	{
	   // Strip the accelerator part if any.
	   accel = (char*)strchr(originalText, '\t');
	   if (accel) *accel = 0;
	}

	USES_CONVERSION;
	IsTranslated = theApp.lang.Translate(A2CT(originalText), csTrans);
	
	if (accel) *accel = '\t';

	if (translatedText == NULL)
	{
		if (!IsTranslated) return 0;
		
		int len = csTrans.GetLength();
		if (forMenu)
		{
			accel = (char*)strchr(originalText, '\t');
			if (accel) len = len + strlen(accel);
		}
		
		return len;
	}

	if (IsTranslated)
	{
		if (accel)
		    csTrans += accel; 
		translatedText[0] = 0;
		_tcsncat(translatedText, csTrans, bufferlen);
		return csTrans.GetLength();
	}
	
	return 0;
}

BOOL GetMozillaWebBrowser(HWND hWnd, nsIWebBrowser** webBrowser)
{
	PLUGIN_HEADER(hWnd, false);
	return NS_SUCCEEDED(browser->GetWebBrowser(webBrowser));
}

void AddStatusBarIcon(HWND hWnd, int id, HICON hIcon, const char* tpText)
{
	CBrowserFrame *frame = GetFrame(hWnd);
	if (!frame) return;

	USES_CONVERSION;
	frame->m_wndStatusBar.AddIcon(id);
	frame->m_wndStatusBar.SetIconInfo(id, hIcon, A2CT(tpText));
}

void RemoveStatusBarIcon(HWND hWnd, int id)
{
	CBrowserFrame *frame = GetFrame(hWnd);
	if (!frame) return;
	
	frame->m_wndStatusBar.RemoveIcon(id);
}	

BOOL InjectJS2(const char* js, int bTopWindow, char *result, unsigned size, HWND hWnd)
{
	PLUGIN_HEADER(hWnd, FALSE);
	
	nsEmbedString js2;
	NS_CStringToUTF16(nsDependentCString(js), NS_CSTRING_ENCODING_UTF8, js2);

	CString csresult;
	if (bTopWindow == 2 && frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)))
	{
		BOOL ret = TRUE;
		CBrowserFrmTab* frameTab = (CBrowserFrmTab*)frame;
		int tabCount = frameTab->GetTabCount();
		for (int i=0;i<tabCount;i++) 
			ret &= frameTab->GetTabIndex(i)->GetBrowserWrapper()->InjectJS(js2.get(), csresult);
		return ret;
	}

	BOOL success = browser->InjectJS(js2.get(), csresult, bTopWindow==1);
	if (success && result) {
		char* c = EncodeUTF8(T2W(csresult.GetBuffer(0)));
		strncpy(result, c, size);
		free(c);
		result[size-1] = 0;
	}
	return success;
}

BOOL InjectJS(const char* js, int bTopWindow, HWND hWnd)
{
	return InjectJS2(js, bTopWindow, NULL, 0, hWnd);
}

BOOL InjectCSS(const char* css, BOOL bAll, HWND hWnd)
{
	PLUGIN_HEADER(hWnd, FALSE);	
	
	nsEmbedString css2;
	NS_CStringToUTF16(nsDependentCString(css), NS_CSTRING_ENCODING_UTF8, css2);
	return browser->InjectCSS(css2.get());
}

int GetKmeleonVersion()
{
	return KMELEON_VERSION;
}

long GetFolderUTF8(FolderType type, char* path, size_t size)
{
   USES_CONVERSION;
   CString csPath = theApp.GetFolder(type);
   if (path) {
	  char* utf = EncodeUTF8(csPath);
      strncpy(path, utf, size);
	  delete utf;
      path[size-1] = 0;
   }
   return csPath.GetLength();
}

long GetFolder(FolderType type, char* path, size_t size)
{
   USES_CONVERSION;
   CString csPath = theApp.GetFolder(type);
   if (path) {
      strncpy(path, T2CA(csPath), size);
      path[size-1] = 0;
   }
   return csPath.GetLength();
}

int GetWindowsList(HWND* list, unsigned size)
{
	INT_PTR count = theApp.m_FrameWndLst.GetCount();
	if (!list) return count;

	POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
	unsigned i = 0;
	while (pos) {
		CFrameWnd* frame = (CFrameWnd*)theApp.m_FrameWndLst.GetNext(pos);
		*(list+i) = frame->GetSafeHwnd();
		if (--size == 0) break;
		i++;
	}

	return i;
}

int GetTabsList(HWND hWnd, HWND* list, unsigned size)
{
	CBrowserFrame* frame = GetFrame(hWnd);
	if (!frame) return -1;

	if (!frame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab))) {
		if (list && size) *list = frame->m_hWnd;
		return 1;
	}

	CBrowserFrmTab* tabFrame = (CBrowserFrmTab*)frame;
	int count = tabFrame->GetTabCount();
	if (!list) return count;

	int i;
	for (i=0; i<count; i++) {
		list[i] = tabFrame->GetTabIndex(i)->GetSafeHwnd();
		if (--size == 0) break;
	}

	return i;
}

UINT GetIconIdx(const char* host)
{
	USES_CONVERSION;
	return theApp.favicons.GetIcon(A2CT(host));
}

void ReleaseCmdID(UINT id)
{
	theApp.commands.ReleaseId(id);
}

UINT RegisterCmd(const char* cmd, const char* plugin, unsigned nbArgs)
{
	USES_CONVERSION;
	return theApp.commands.RegisterCommand(A2CT(plugin), A2CT(cmd), nbArgs);
}

void UnregisterCmd(const char* cmd, const char* plugin)
{
	USES_CONVERSION;
	return theApp.commands.UnregisterCommand(A2CT(plugin), A2CT(cmd));
}

unsigned GetCmdList(kmeleonCommand* cmdList, unsigned size)
{
	int num = theApp.plugins.SendMessage("*", "Urlbar", "GetCmds", 0, 0);
	num += theApp.commands.GetDefCount();

	if (!cmdList)
		return num;

	num = theApp.commands.GetList(cmdList, size, TRUE);
	CPluginList *pluginList = theApp.plugins.GetPlugins();
	POSITION pos = pluginList->GetStartPosition();
	kmeleonPlugin * kPlugin;
	CString s;
	while (pos) {
		pluginList->GetNextAssoc( pos, s, kPlugin);
		if (kPlugin->loaded && kPlugin->DoMessage) {
			num += kPlugin->DoMessage(kPlugin->dllname, "*", "GetCmds", (long)&cmdList[num], size-num);
		}
	}

	return num;
}

extern BOOL LoadStyleSheet(LPCTSTR path, BOOL load);

BOOL LoadCSS(const char* path, BOOL load)
{
	USES_CONVERSION;
	return LoadStyleSheet(A2CT(path), load);
}

kmeleonFunctions kmelFuncsUTF8 = {
   SendMessage,
   GetCommandIDs,
   _NavigateTo,
   GetDocInfoUTF8,
   _GetPreference,
   SetPreference,
   SetStatusBarTextUTF8,
   _GetMozillaSessionHistory,
   GotoHistoryIndex,
   RegisterBand,
   CreateToolbar,
   GetID,
   GetInfoAtPoint,
   CommandAtPoint,
   GetGlobalVar,
   EncodeUTF8,
   DecodeUTF8,
   GetBrowserviewRect,
   GetMenu,
   SetForceCharset,
   SetCheck,
   Load,
   ClearCache,
   BroadcastMessage,
   ParseAccel,
   DelPreference,
   GetPreference,
   RegisterSideBar,
   ToggleSideBar,
   TranslateEx,
   GetIconList,
   GetMozillaWebBrowser,
   AddStatusBarIcon,
   RemoveStatusBarIcon,
   InjectJS,
   InjectCSS,
   GetInfoAtClick,
   GetKmeleonVersion,
   NULL,
   NavigateTo,
   TranslateUTF8,
   SetGlobalVar,
   GetFolderUTF8,
   SetAccel,
   SetMenu,
   RebuildMenu,
   GetWindowVarUTF8,
   SetWindowVarUTF8,
   GetMozillaSessionHistory,
   SetMozillaSessionHistory,
   GetWindowsList,
   GetTabsList,
   GetIconIdx,
   ReleaseCmdID,
   RegisterCmd,
   UnregisterCmd,
   GetCmdList,
   LoadCSS,
   LogMessage,
   InjectJS2
};

kmeleonFunctions kmelFuncs = {
   SendMessage,
   GetCommandIDs,
   _NavigateTo,
   GetDocInfo,
   _GetPreference,
   SetPreference,
   SetStatusBarText,
   _GetMozillaSessionHistory,
   GotoHistoryIndex,
   RegisterBand,
   CreateToolbar,
   GetID,
   GetInfoAtPoint,
   CommandAtPoint,
   GetGlobalVar,
   EncodeUTF8,
   DecodeUTF8,
   GetBrowserviewRect,
   GetMenu,
   SetForceCharset,
   SetCheck,
   Load,
   ClearCache,
   BroadcastMessage,
   ParseAccel,
   DelPreference,
   GetPreference,
   RegisterSideBar,
   ToggleSideBar,
   TranslateEx,
   GetIconList,
   GetMozillaWebBrowser,
   AddStatusBarIcon,
   RemoveStatusBarIcon,
   InjectJS,
   InjectCSS,
   GetInfoAtClick,
   GetKmeleonVersion,
   NULL,
   NavigateTo,
   Translate,
   SetGlobalVar,
   GetFolder,
   SetAccel,
   SetMenu,
   RebuildMenu,
   GetWindowVar,
   SetWindowVar,
   GetMozillaSessionHistory,
   SetMozillaSessionHistory,
   GetWindowsList,
   GetTabsList,
   GetIconIdx,
   ReleaseCmdID,
   RegisterCmd,
   UnregisterCmd,
   GetCmdList,
   LoadCSS,
   LogMessage,
   InjectJS2
};

BOOL CPlugins::TestLoad(LPCTSTR file, const char *description)
{
   char preference[128] = "kmeleon.plugins.";

   USES_CONVERSION;
   strncat(preference, T2CA(file), sizeof(preference));
   strncat(preference, ".load", sizeof(preference));
   
   int load = theApp.preferences.GetBool(preference, -1);
   if (load == -1) {
      CString message, title;
      title.LoadString(IDS_NEW_PLUGIN_FOUND_TITLE);
      message.Format(IDS_NEW_PLUGIN_FOUND, theApp.lang.Translate(A2CT(description)));

      if (MessageBox(NULL, message, title, MB_YESNO) == IDYES)
         load = 1;
      else
         load = 0;

      theApp.preferences.SetBool(preference, load);
   }
   return load;
}

BOOL CPlugins::IsLoaded(LPCTSTR pluginName)
{
   kmeleonPlugin* kPlugin;
   if (!pluginList.Lookup(pluginName, kPlugin))
      return FALSE;

   return kPlugin->loaded;
}

kmeleonPlugin * CPlugins::Load(CString file)
{
   file.TrimLeft();
   file.TrimRight();

   int filePos = file.ReverseFind(_T('\\'));
   int filePos2 = file.ReverseFind(_T('/'));
   if (filePos2>filePos) filePos = filePos2;
   ++filePos;

   CString noPath = file.Mid(filePos);
   noPath.MakeLower();

   CString pluginName;
   int extPos = noPath.ReverseFind(_T('.'));
   if (noPath.Mid(extPos).Compare(_T(".dll"))!=0)
      extPos = -1;

   if (extPos!=-1)
      pluginName = noPath.Mid(0, extPos);
   else
      pluginName = noPath;

   // check if the plugin is already loaded
   kmeleonPlugin * kPlugin;
   if (pluginList.Lookup(pluginName, kPlugin))
      return kPlugin; // it's already loaded

   HINSTANCE plugin;

   // if pattern does not contain ':' we need to prepend pluginsDir
   if (file.Find(':') == -1)
      file = theApp.preferences.pluginsDir + file;

   // we need to append .dll because NT4 gets confused if a directory in the path
   // contains a '.' and the file to be loaded does not
   if (extPos == -1)
      file +=_T(".dll");

   plugin = LoadLibrary(file);
   if (!plugin) return NULL;

   KmeleonPluginGetter kpg = (KmeleonPluginGetter)GetProcAddress(plugin, "GetKmeleonPlugin");
   if (!kpg) {
      FreeLibrary(plugin);
      return NULL;
   }

   kPlugin = kpg();

   USES_CONVERSION;
   kPlugin->hParentInstance = AfxGetInstanceHandle();
   kPlugin->hDllInstance = plugin;
   if ((kPlugin->version & ~KMEL_PLUGIN_VER_MAJOR) >= KMEL_PLUGIN_VER_MINOR_UTF8)
      kPlugin->kFuncs = &kmelFuncsUTF8;
   else
      kPlugin->kFuncs = &kmelFuncs;
   kPlugin->dllname = safe_strdup(T2CA(pluginName));

   int loaded = kPlugin->loaded = TestLoad(pluginName, kPlugin->description);
   if (kPlugin->version < KMEL_PLUGIN_VER_MAJOR
/*#ifdef _UNICODE	  
   || kPlugin->version < 0x0204
#endif*/
	   ) {
      CString error;
      error.Format(IDS_OLD_PLUGIN, theApp.lang.Translate(A2CT(kPlugin->description)));
      AfxMessageBox(error);
      loaded = false;
   }

   // If the plugin is enabled, tell it to Init
   if (loaded) {
      if (kPlugin->DoMessage(kPlugin->dllname, "* Plugin Manager", "Load", 0, 0) == -1)
         loaded = false;
   }

   if (!loaded) {
   // otherwise, make a copy of the descripion, and unload it
   //else {
      kmeleonPlugin *temp = new kmeleonPlugin;

      char *sBuf = new char[strlen(kPlugin->description)+1];
      temp->description = sBuf;
      strcpy(temp->description, kPlugin->description);

      temp->dllname = kPlugin->dllname;
      temp->loaded = false;

      kPlugin=temp;

      FreeLibrary(plugin);
   }

   pluginList.SetAt(pluginName, kPlugin);
   return kPlugin;
}

int CPlugins::_FindAndLoad(const TCHAR *pattern)
{
	CFileFind finder;
	BOOL bWorking = finder.FindFile(pattern);
	CString filepath;
	int i = 0;
   while (bWorking) {
      bWorking = finder.FindNextFile();

      filepath = finder.GetFilePath();
      if ( Load(filepath) )
         i++;
   }
   //   SendMessage("*", "* Plugin Manager", "Init");
   return i;
}

int CPlugins::FindAndLoad(const TCHAR *pattern)
{
   if (*pattern!=_T('/') && !_tcschr(pattern, ':')) {
      // if pattern does not contain ':' or does not begin with '/'
      // we need to prepend pluginsDir
      CString search = theApp.GetFolder(PluginsFolder) + _T('\\') + pattern;
      int n = _FindAndLoad(search);
      search = theApp.GetFolder(UserPluginsFolder) + _T('\\') + pattern;
      n += _FindAndLoad(search);
      return n;
   }
   else return _FindAndLoad(pattern);
}

void CPlugins::UnLoadAll()
{
   SendMessage("*", "* Plugin Manager", "Exit");

   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc(pos, s, kPlugin);
      if (kPlugin) {
         free(kPlugin->dllname);
         if (kPlugin->loaded) {
            FreeLibrary(kPlugin->hDllInstance);
         }
         else  { // the plugin was disabled, delete the copied description
            delete kPlugin->description;
            delete kPlugin;
         }
      }
   }
   pluginList.RemoveAll();
   theApp.commands.InitPluginCmdList();
   //currentCmdID = PLUGIN_COMMAND_START_ID;
}

int CPlugins::GetConfigFiles(configFileType *configFiles, int maxFiles)
{
   int numFiles = 0;
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc( pos, s, kPlugin);

      if (kPlugin->loaded && kPlugin->DoMessage) {
         configFileType *tempConfigFiles;
         int numTempConfigFiles=0;
         kPlugin->DoMessage(kPlugin->dllname, "* Plugin Manager", "GetConfigFiles", (long)&tempConfigFiles, (long)&numTempConfigFiles);
         int i = 0;
         while (numFiles < maxFiles && i < numTempConfigFiles) {
            memcpy(&configFiles[numFiles++], &tempConfigFiles[i++], sizeof(configFileType));
         }
      }
   }
   return numFiles;
}
