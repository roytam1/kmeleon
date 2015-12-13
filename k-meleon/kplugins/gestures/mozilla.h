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

#define XPCOM_GLUE_AVOID_NSPR
#include "mozilla-config.h"
#include <tchar.h>
#undef min
#undef max

#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsEmbedString.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMLocation.h"
#include "nsIImageLoadingContent.h"
#include "nsIURI.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMWindow.h"
#include "nsISelection.h"
#include "nsIIOService.h"
#include "nsServiceManagerUtils.h"
#include "nsIWindowWatcher.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsIBrowserSearchService.h"
#include "kmeleon_plugin.h"

extern kmeleonPlugin kPlugin;

#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLSelectElement.h"

BOOL IsInputOrObject(nsIDOMElement* element)
{
	NS_ENSURE_TRUE(element, false);

	nsCOMPtr<nsIDOMHTMLInputElement> domnsinput = do_QueryInterface(element);
	if (domnsinput) return true;

	nsCOMPtr<nsIDOMHTMLTextAreaElement> tansinput = do_QueryInterface(element);
	if (tansinput) return true;

	nsCOMPtr<nsIDOMHTMLEmbedElement> embed = do_QueryInterface(element);
	if (embed) return true;

	nsCOMPtr<nsIDOMHTMLObjectElement> object = do_QueryInterface(element);
	if (object) return true;

	nsCOMPtr<nsIDOMHTMLSelectElement> select = do_QueryInterface(element);
	if (select) return true;

	nsString attr;
	element->GetAttribute(NS_LITERAL_STRING("contenteditable"), attr);
	if (wcscmp(attr.get(), L"true") == 0)
		return true;

	return false;
}

void doSearch(const wchar_t* query)
{
	nsCOMPtr<nsIBrowserSearchService> ss = do_GetService("@mozilla.org/browser/search-service;1");
	if (!ss) return;

	nsCOMPtr<nsISearchEngine> engine;
	ss->GetCurrentEngine(getter_AddRefs(engine));
	if (!engine) return;

	nsCOMPtr<nsISearchSubmission> sub;
	engine->GetSubmission(nsDependentString(query), nsDependentString(L""), nsDependentString(L""), getter_AddRefs(sub));
	if (!sub) return;

	nsCOMPtr<nsIURI> uri;
	sub->GetUri(getter_AddRefs(uri));
	if (!uri) return;

	nsCString spec;
	uri->GetSpec(spec);
	kPlugin.kFuncs->NavigateTo(spec.get(), OPEN_BACKGROUNDTAB, NULL);
}

class CDomEventListener : public nsIDOMEventListener
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	enum {
		HAS_NONE = 0,
		HAS_TEXT = 1,
		HAS_IMAGE = 2,
		HAS_LINK = 4
	};

	int type;
	nsCString link;
	nsString data;

	CDomEventListener(HWND hwnd)
	{
		mhWnd = hwnd;
	}

	~CDomEventListener()
	{		
	}

	BOOL Init(HWND hwnd)
	{

		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;

		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return FALSE;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("dragstart"),
			this, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("dragover"),
			this, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("drop"),
			this, false);
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("dragend"),
			this, false);

		return TRUE;
	}

	void Done(HWND hwnd)
	{
		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;

		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, );

		if (!mEventTarget) return;
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("dragstart"),
			this, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("dragover"),
			this, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("drop"),
			this, false);
		mEventTarget->RemoveEventListener(NS_LITERAL_STRING("dragend"),
			this, false);
	}

protected:

	nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
	{
		NS_ENSURE_ARG(aWebBrowser);
		nsCOMPtr<nsIDOMWindow> domWin;
		aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
		NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);
	
		return domWin->GetWindowRoot (aTarget);
	}

	HWND mhWnd;
};

NS_IMPL_ISUPPORTS(CDomEventListener, nsIDOMEventListener)

NS_IMETHODIMP CDomEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
	nsresult rv;
	nsString eventType;
	aEvent->GetType(eventType);
	
	nsCOMPtr<nsIDOMEventTarget> eventTarget;
	rv = aEvent->GetOriginalTarget(getter_AddRefs(eventTarget));
	NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(eventTarget);
	if (!targetNode) return NS_OK;

	nsCOMPtr<nsIDOMDocument> doc;
	targetNode->GetOwnerDocument(getter_AddRefs(doc));
	if (!doc) return NS_ERROR_FAILURE;

	if (eventType.Equals(NS_LITERAL_STRING("dragstart"))) {
		type = HAS_NONE;
		data.Truncate();
		link.Truncate();

		nsString protocol;
		nsCOMPtr<nsIDOMLocation> loc;
		doc->GetLocation(getter_AddRefs(loc));
		if (!loc) return NS_ERROR_FAILURE;
		loc->GetProtocol(protocol);
		if (protocol.Compare(L"about") == 0 || protocol.Compare(L"chrome") == 0)
			return NS_OK;

		nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aEvent));
		if (!dragEvent) return NS_ERROR_FAILURE;
		nsCOMPtr<nsIDOMDataTransfer> dt;
		dragEvent->GetDataTransfer(getter_AddRefs(dt));

		nsString _data;
		dt->GetData(NS_LITERAL_STRING("text/plain"), _data);
		_data.Trim("");
		if (_data.Length()) {
			type = HAS_TEXT;
			data = _data;
		}

		nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(targetNode));
		if (content) {
			nsCOMPtr<nsIURI> imgUri;
			content->GetCurrentURI(getter_AddRefs(imgUri));
			type = HAS_IMAGE;
			imgUri->GetSpec(link);
		}

		nsCOMPtr<nsIDOMNode> node = targetNode;
		do {
			PRUint16 nodeType;
			rv = node->GetNodeType(&nodeType);
			NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

			if (nodeType == nsIDOMNode::ELEMENT_NODE) {
				bool hasAttr = false;
				nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));
				if (element) element->HasAttribute(NS_LITERAL_STRING("href"), &hasAttr);
				if (hasAttr) {
					element->GetAttribute(NS_LITERAL_STRING("href"), _data);
					type = HAS_LINK;
					NS_UTF16ToCString(_data, NS_CSTRING_ENCODING_UTF8, link);
					break;
				}
			}
			node->GetParentNode(getter_AddRefs(node));
		} while (node);

	/*	dt->GetData(NS_LITERAL_STRING("text/uri-list"), _data);
		if (_data.Length()) {// && (wcsncmp(_data.get(), L"http", 4) == 0 || wcsncmp(_data.get(), L"file", 4) == 0))
			type = HAS_LINK;
			NS_UTF16ToCString(_data, NS_CSTRING_ENCODING_UTF8, link);
		}*/

		nsCOMPtr < nsIDOMWindow> win;
		doc->GetDefaultView(getter_AddRefs(win));
		if (win) {
			nsCOMPtr<nsISelection> selection;
			win->GetSelection(getter_AddRefs(selection));
			selection->ToString(_data);
			_data.Trim("");
			if (_data.Length()) {
				uint16_t ntype;
				targetNode->GetNodeType(&ntype);
				nsString name;
				targetNode->GetNodeName(name);
				if (ntype == nsIDOMNode::TEXT_NODE && name.Compare(L"#text") == 0) {
					type = HAS_TEXT;
					data = _data;
				}
			}
		}

		if ((type != HAS_LINK && type != HAS_IMAGE) && data.Length()) {
			nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/networkhttp://kmeleonbrowser.org/forum/list.php?8/io-service;1");
			if (ios) {
				nsCOMPtr<nsIURI> uri;
				NS_UTF16ToCString(data, NS_CSTRING_ENCODING_UTF8, link);
				nsresult rv = ios->NewURI(link, nullptr, nullptr, getter_AddRefs(uri));
				if (NS_SUCCEEDED(rv) && uri) type = HAS_LINK;
			}
		}
					
		return NS_OK;
	}

	if (eventType.Equals(NS_LITERAL_STRING("dragover"))) {
		nsCOMPtr<nsIDOMEventTarget> target;
		aEvent->GetTarget(getter_AddRefs(target));
		nsCOMPtr<nsIDOMElement> element(do_QueryInterface(target));
		if (type && !IsInputOrObject(element)) aEvent->PreventDefault();
		return NS_OK;
	}

	if (eventType.Equals(NS_LITERAL_STRING("drop"))) {
		nsCOMPtr<nsIDOMEventTarget> target;
		aEvent->GetTarget(getter_AddRefs(target));
		nsCOMPtr<nsIDOMElement> element(do_QueryInterface(target));
		if (type == HAS_NONE || IsInputOrObject(element)) 
			return NS_OK;

		nsCString pref;
		pref = NS_LITERAL_CSTRING(PREF_);
		pref.Append("SD");

		switch (type) {
		case HAS_TEXT: pref.Append("Text"); break;
		case HAS_LINK: pref.Append("Link"); break;
		case HAS_IMAGE: pref.Append("Image"); break;
		}

		char command[100];
		kPlugin.kFuncs->GetPreference(PREF_STRING, pref.get(), command, (char*)"");
		int id = kPlugin.kFuncs->GetID(command);
		if (id == kPlugin.kFuncs->GetID("navSearch")) {
			doSearch(data.get());
		}
		else {
			kPlugin.kFuncs->RunCommand(NULL, command);
		}

		aEvent->PreventDefault();
		return NS_OK;		
	}
	return NS_OK;
}