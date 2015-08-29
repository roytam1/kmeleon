#include "stdafx.h"
#include "MozUtils.h"
#include "nsIWindowWatcher.h"
#include "nsIIOService.h"

#include "nsDirectoryServiceUtils.h"

#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLObjectElement.h"

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSValueList.h"

#include "nsIDOMCharacterData.h"

#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLAnchorElement.h"

#include "nsIDOMDocument.h"
#include "nsIDOMLocation.h"
#include "nsPIDOMWindow.h" 
#include "nsIImageLoadingContent.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebBrowserFocus.h"
#include "nsIURIFixup.h"

#include "nsIZipReader.h"
#include "nsIStringEnumerator.h"

nsString CStringToNSString(LPCTSTR aStr)
{
	USES_CONVERSION;
	return nsString(T2CW(aStr));
}

nsCString CStringToNSCString(LPCTSTR aStr)
{
	USES_CONVERSION;
	return nsCString(T2CA(aStr));
}

nsCString CStringToNSUTF8String(LPCTSTR aStr)
{
	USES_CONVERSION;
	nsCString aCStr;
	NS_UTF16ToCString(nsString(T2CW(aStr)), NS_CSTRING_ENCODING_UTF8, aCStr);
	return aCStr;
}

CString NSStringToCString(const nsString& aStr)
{
	USES_CONVERSION;
	return CString(W2CT(aStr.get()));
}

CString NSUTF8StringToCString(const nsCString& aStr)
{
	USES_CONVERSION;
	nsString aUStr;
	NS_CStringToUTF16(aStr, NS_CSTRING_ENCODING_UTF8, aUStr);
	return CString(W2CT(aUStr.get()));
}

CString NSCStringToCString(const nsCString& aStr)
{
	USES_CONVERSION;
	return CString(A2CT(aStr.get()));
}

CString PRUnicharToCString(const PRUnichar* str)
{
	USES_CONVERSION;
	return CString(W2CT(str));
}

nsresult NewURI(nsIURI **result, const nsACString &spec)
{
  nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, NS_ERROR_UNEXPECTED);

  return ios->NewURI(spec, nullptr, nullptr, result);
}

nsresult NewURI(nsIURI **result, const nsAString &spec)
{
  nsCString specUtf8;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, specUtf8);
  return NewURI(result, specUtf8);
}


nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
{
	NS_ENSURE_ARG(aWebBrowser);
	nsCOMPtr<nsIDOMWindow> domWin;
	aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
	NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);

	return domWin->GetWindowRoot (aTarget);

	nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWin));
	if (!piWin) return NS_ERROR_FAILURE;

	mozilla::dom::EventTarget* eventTarget = piWin->GetChromeEventHandler();
	NS_ENSURE_TRUE(eventTarget, NS_ERROR_FAILURE);
	nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(eventTarget);
	NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
	*aTarget = target;
    NS_ADDREF(*aTarget);
	return NS_OK;
}

CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow)
{
	nsCOMPtr<nsIWindowWatcher> mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
	NS_ENSURE_TRUE(mWWatch, NULL);

	nsCOMPtr<nsIWebBrowserChrome> chrome;
	CWnd *val = 0;
	
	if (!aWindow) {
		// it will be a dependent window. try to find a foster parent.
		nsCOMPtr<nsIDOMWindow> fosterParent;
		mWWatch->GetActiveWindow(getter_AddRefs(fosterParent));
		aWindow = fosterParent;
	}

	mWWatch->GetChromeForWindow(aWindow, getter_AddRefs(chrome));
	NS_ENSURE_TRUE(chrome, NULL);

	nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
	NS_ENSURE_TRUE(site, NULL);

	HWND w;
	site->GetSiteWindow(reinterpret_cast<void **>(&w));

	return CWnd::FromHandle(w);
}

CString GetUriForDocument(nsIDOMDocument *document)
{
	nsCOMPtr<nsIDOMLocation> location;
	document->GetLocation(getter_AddRefs(location));
	NS_ENSURE_TRUE(location, _T(""));
	
	nsString url;
	nsresult rv = location->GetHref(url);
	NS_ENSURE_SUCCESS(rv, _T(""));

	return NSStringToCString(url);
}

CString GetUriForDOMWindow(nsIDOMWindow *aWindow)
{
	NS_ENSURE_TRUE(aWindow, _T(""));

	nsCOMPtr<nsIDOMDocument> document;
	nsresult rv = aWindow->GetDocument(getter_AddRefs(document));
	NS_ENSURE_TRUE(document, _T(""));
	
	return GetUriForDocument(document);
}

CString GetMozDirectory(const char* dirName)
{
   nsCOMPtr<nsIFile> nsDir;
   nsresult rv = NS_GetSpecialDirectory(dirName, getter_AddRefs(nsDir));
   if (NS_FAILED(rv))
      return _T("");

#ifdef _UNICODE
   nsString pathBuf;
   rv = nsDir->GetPath(pathBuf);
#else
   nsCString pathBuf;
   rv = nsDir->GetNativePath(pathBuf);
#endif

   return pathBuf.get();
}

void GatherTextUnder(nsIDOMNode* aNode, nsString& aResult) 
{
	ASSERT(aNode);
	if (!aNode) return;

	nsString text;
	nsCOMPtr<nsIDOMNode> node;
	aNode->GetFirstChild(getter_AddRefs(node));
	PRUint32 depth = 1;
	while (node && depth) {
		nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(node));
		PRUint16 nodeType;
		node->GetNodeType(&nodeType);
		if (charData && nodeType == nsIDOMNode::TEXT_NODE) {
			// Add this text to our collection.
			text += NS_LITERAL_STRING(" ");
			nsString data;
			charData->GetData(data);
			text += data;
		}
		else {
			nsCOMPtr<nsIDOMHTMLImageElement> img(do_QueryInterface(node));
			if (img) {
				nsString altText;
				img->GetAlt(altText);
				if (!altText.IsEmpty()) {
					text = altText;
					break;
				}
			}
		}

		// Find the next node to test.
		bool hasChildNodes;
		node->HasChildNodes(&hasChildNodes);
		if (hasChildNodes) {
			nsCOMPtr<nsIDOMNode> temp = node;
			temp->GetFirstChild(getter_AddRefs(node));
			depth++;
		}
		else {
			nsCOMPtr<nsIDOMNode> nextSibling;
			node->GetNextSibling(getter_AddRefs(nextSibling));
			if (nextSibling)
				node = nextSibling;
			else {
				nsCOMPtr<nsIDOMNode> parentNode;
				node->GetParentNode(getter_AddRefs(parentNode));
				if (!parentNode)
					node = nullptr;
				else {
					nsCOMPtr<nsIDOMNode> nextSibling2;
					parentNode->GetNextSibling(getter_AddRefs(nextSibling2));
					node = nextSibling2;
					depth--;
				}
			}
		}
	}

	aResult = text;
}

void GatherTextUnder(nsIDOMNode* aNode, CString& aResult) 
{
	nsString result;
	::GatherTextUnder(aNode, result);
	aResult = NSStringToCString(result);
}

BOOL GetLinkTitleAndHref(nsIDOMNode* node, nsString& aHref, nsString& aTitle)
{
	NS_ENSURE_TRUE(node, FALSE);

	nsresult rv;
	nsCOMPtr<nsIDOMNode> next;
	do {
		PRUint16 nodeType;
		rv = node->GetNodeType(&nodeType);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (nodeType == nsIDOMNode::ELEMENT_NODE) {

			// Test if the element has an associated link
			nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));

			bool hasAttr = false;
			if (element) element->HasAttribute(NS_LITERAL_STRING("href"), &hasAttr);
			if (hasAttr)
			{
				nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(element));
                if (anchor)
					anchor->GetHref(aHref);
				else {
					nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(element));
					if (area)
						area->GetHref(aHref);
					else {
						nsCOMPtr<nsIDOMHTMLLinkElement> link(do_QueryInterface(element));
						if (link)
							link->GetHref(aHref);
					}
				}
				if (aHref.Length()) {
					GatherTextUnder(element, aTitle);
					return TRUE;
				}
			}
		}

		// walk-up-the-tree
		node->GetParentNode(getter_AddRefs(next));
		node = next;
	} while (node);
	
	return FALSE;
}

BOOL IsContentEditable(nsIDOMNode* node)
{
	NS_ENSURE_TRUE(node, FALSE);

	nsresult rv;
	nsCOMPtr<nsIDOMNode> next;
	do {
		PRUint16 nodeType;
		rv = node->GetNodeType(&nodeType);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (nodeType == nsIDOMNode::ELEMENT_NODE) {
			// Test if the element has an associated link
			nsString attr;
			nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node));		
			if (!element) break;
			element->GetAttribute(NS_LITERAL_STRING("contenteditable"), attr);
			if (wcscmp(attr.get(), L"true") == 0)
				return TRUE;
		}

		// walk-up-the-tree
		node->GetParentNode(getter_AddRefs(next));
		node = next;
	} while (node);
	
	return FALSE;
}

BOOL GetLinkTitleAndHref(nsIDOMNode* node, CString& aHref, CString& aTitle)
{
	nsString url, title;
	if (::GetLinkTitleAndHref(node, url, title)) {
		aHref = NSStringToCString(url);
		aTitle = NSStringToCString(title);
		return TRUE;
	}
	return FALSE;
}

nsresult GetCSSBackground(nsIDOMNode *node, nsString& aUrl)
{
	NS_ENSURE_ARG(node);
	nsresult rv;

	nsCOMPtr<nsIDOMDocument> document;
	node->GetOwnerDocument(getter_AddRefs(document));
	NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

	nsCOMPtr<nsIDOMWindow> domWin;
	document->GetDefaultView(getter_AddRefs(domWin));
	NS_ENSURE_TRUE(domWin, NS_ERROR_FAILURE);

	nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(node);
	NS_ENSURE_TRUE(domElement, NS_ERROR_FAILURE);

	nsCOMPtr<nsIDOMCSSStyleDeclaration> computedStyle;
	rv = domWin->GetComputedStyle(domElement, nsString(), getter_AddRefs(computedStyle));
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCOMPtr<nsIDOMCSSValue> cssValue;
	computedStyle->GetPropertyCSSValue(NS_LITERAL_STRING("background-image"), getter_AddRefs(cssValue));
	
	nsString bgUrl;
	PRUint16 type;
	rv = cssValue->GetCssValueType(&type);
	if (type == nsIDOMCSSValue::CSS_VALUE_LIST) {
		// gg
		cssValue->GetCssText(bgUrl); 		
		if (bgUrl.IsEmpty() || bgUrl.Compare(L"none") == 0)
			return NS_ERROR_FAILURE;
		int32_t pos = bgUrl.Find("url(");
		if ( pos != -1) {
			bgUrl.Cut(0,4);
			pos = bgUrl.FindChar(')');
			bgUrl.Cut(pos,-1);
			if (bgUrl.First() == '"') {
				bgUrl.Cut(0,1);				
				bgUrl.Cut(bgUrl.Length()-1,1);
			}
		}
	} else {
		nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue = do_QueryInterface(cssValue);
		if (!primitiveValue) return NS_ERROR_FAILURE;

		rv = primitiveValue->GetPrimitiveType(&type);
		NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

		if (type != nsIDOMCSSPrimitiveValue::CSS_URI) 
			return NS_ERROR_FAILURE;
		
		rv = primitiveValue->GetStringValue(bgUrl);
		NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
	}

	// Resolve the url with the base
	nsString nsURL;
	rv = document->GetDocumentURI(nsURL);
	NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
/*
	nsCOMPtr<nsIDOMNSDocument> nsDocument(do_QueryInterface(document));
	NS_ENSURE_TRUE(document, FALSE);

	nsCOMPtr<nsIDOMLocation> location;
	nsDocument->GetLocation(getter_AddRefs(location));
	NS_ENSURE_TRUE(location, FALSE);

	nsString nsURL;
	location->GetHref(nsURL);*/

	nsCOMPtr<nsIURI> docUri;
	rv = NewURI(getter_AddRefs(docUri), nsURL);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString bgCUrl, bgCUrl2;
	NS_UTF16ToCString (bgUrl, NS_CSTRING_ENCODING_UTF8, bgCUrl);	
	rv = docUri->Resolve(bgCUrl, bgCUrl2);

	NS_CStringToUTF16(bgCUrl2, NS_CSTRING_ENCODING_UTF8, aUrl);

	return rv;
}

BOOL GetImageSrc(nsIDOMNode *aNode, nsCString& aUrl)
{
	NS_ENSURE_TRUE(aNode, FALSE);

	nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(aNode));
	NS_ENSURE_TRUE(content, FALSE);

	nsCOMPtr<nsIURI> imgUri;
	content->GetCurrentURI(getter_AddRefs(imgUri));
	NS_ENSURE_TRUE(imgUri, FALSE);

	imgUri->GetSpec(aUrl);
	return TRUE;
}

BOOL GetImageSrc(nsIDOMNode *aNode, CString& aUrl)
{
	nsCString url;
	if (!::GetImageSrc(aNode, url))
		return FALSE;
	aUrl = NSUTF8StringToCString(url);
	return TRUE;
}

BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, nsString& aUrl)
{
	NS_ENSURE_TRUE(aNode, FALSE);
	nsresult rv;
	nsString bgImg;
	nsCOMPtr<nsIDOMNode> node = aNode;

	nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(aNode);
	if (htmlElement)
	{
		nsString nameSpace;
		htmlElement->GetNamespaceURI(nameSpace);
	//	if (nameSpace.IsEmpty())
		{
			nsCOMPtr<nsIDOMNode> next;
			do{
				rv = GetCSSBackground(node, bgImg);
				if (NS_SUCCEEDED(rv)) break;
				node->GetParentNode(getter_AddRefs(next));
				node = next;
			} while (node);
			
			if (NS_FAILED(rv))	
			{
				// no background-image found
				nsCOMPtr<nsIDOMDocument> document;
				aNode->GetOwnerDocument(getter_AddRefs(document));
				nsCOMPtr<nsIDOMHTMLDocument> htmlDocument(do_QueryInterface(document));
				NS_ENSURE_TRUE(htmlDocument, FALSE);

				nsCOMPtr<nsIDOMHTMLElement> body;
				htmlDocument->GetBody(getter_AddRefs(body));
				node = do_QueryInterface(body);
				NS_ENSURE_TRUE(node, FALSE);
			}
		}
	}

	if (bgImg.IsEmpty())
	{
		nsCOMPtr<nsIDOMNode> next;
		do{
			rv = GetCSSBackground(node, bgImg);
			if (NS_SUCCEEDED(rv)) break;

			node->GetParentNode(getter_AddRefs(next));
			node = next;
		}while (node);
	}

	if (bgImg.IsEmpty())
		return FALSE;

	aUrl = bgImg;
	return TRUE;
}

BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, CString& aUrl)
{
	nsString url;
	if (!::GetBackgroundImageSrc(aNode, url))
		return false;
	aUrl = NSStringToCString(url);
	return true;
}

 bool GetFrameURL(nsIWebBrowser* aWebBrowser, nsIDOMNode* aNode, nsString& aUrl)
{
	nsresult rv;
	nsCOMPtr<nsIDOMDocument> contextDocument;

	if (aNode) {
		rv = aNode->GetOwnerDocument(getter_AddRefs(contextDocument));
		if (NS_FAILED(rv) || !contextDocument) return false;
	}
	else {
		nsCOMPtr<nsIDOMWindow> dom;
		nsCOMPtr<nsIWebBrowserFocus> browserFocus = do_QueryInterface(aWebBrowser, &rv);
		NS_ENSURE_TRUE(browserFocus, false);
		browserFocus->GetFocusedWindow(getter_AddRefs(dom));
		NS_ENSURE_TRUE(dom, false);
		rv = dom->GetDocument(getter_AddRefs(contextDocument));
		NS_ENSURE_TRUE(contextDocument, false);
	}

	nsCOMPtr<nsIDOMWindow> domWindow;
	aWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	if (NS_FAILED(rv) || !domWindow) 
		return false;

	nsCOMPtr<nsIDOMDocument> document;
	rv = domWindow->GetDocument(getter_AddRefs(document));
	if (NS_FAILED(rv)) return false;

	if(document == contextDocument) 
		return false;

	nsCOMPtr<nsIDOMLocation> location;
	contextDocument->GetLocation(getter_AddRefs(location));
	NS_ENSURE_TRUE(location, false);

	rv = location->GetHref(aUrl);
	NS_ENSURE_SUCCESS(rv, false);

	return true;
}

BOOL LogMessage(const char* category, const char* message, const char* file, uint32_t line, uint32_t flags)
{
	nsCOMPtr<nsIConsoleService> consoleService = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
	if (!consoleService) FALSE;

	if (flags == 5) {
		return NS_SUCCEEDED(consoleService->LogStringMessage(NS_ConvertUTF8toUTF16(message).get()));
	}
	
	nsCOMPtr<nsIScriptError> scriptError = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
	if (!scriptError) FALSE;
	
	nsresult rv = scriptError->Init(
		NS_ConvertUTF8toUTF16(message),
		NS_ConvertUTF8toUTF16(file),
		nsString(L""),	line, 0,
		flags,
		category);
	
	if (NS_FAILED(rv)) return FALSE;

	rv = consoleService->LogMessage(scriptError);
	return NS_SUCCEEDED(rv);
}

CString GetSearchURL(LPCTSTR query) 
{
	nsCOMPtr<nsIURIFixup> fixup(do_GetService("@mozilla.org/docshell/urifixup;1"));
	nsCOMPtr<nsIURI> uri;
	fixup->KeywordToURI(CStringToNSCString(query), nullptr, getter_AddRefs(uri));
	if (!uri)  return _T("");

	nsCString spec;
	uri->GetSpec(spec);
	return NSCStringToCString(spec);
}

bool ZipExtractFiles(nsIFile* zipFile, nsIFile* folder) 
{
	nsresult rv;
	nsCOMPtr<nsIZipReader> zipReader = do_GetService("@mozilla.org/libjar/zip-reader;1");
	if (!zipReader) return false;

	rv = zipReader->Open(zipFile);
	NS_ENSURE_SUCCESS(rv, false);

	nsCOMPtr<nsIUTF8StringEnumerator> entries;
	zipReader->FindEntries(NS_LITERAL_CSTRING("*/"), getter_AddRefs(entries));

	while (1) {
		bool hasMore;
		entries->HasMore(&hasMore);
		if (!hasMore) break;
		nsCString e;
		entries->GetNext(e);

		nsCOMPtr<nsIFile> target;
		folder->Clone(getter_AddRefs(target));
		while (e.FindChar('/')) {				
			target->Append(CStringToNSString(NSCStringToCString(e).Left(e.FindChar('/'))));
			e.Cut(0, e.FindChar('/'));
		}
			
		bool exists;
		target->Exists(&exists);
		if (!exists) {
			rv = target->Create(nsIFile::DIRECTORY_TYPE, 0);
			NS_ENSURE_SUCCESS(rv, false);
		}
	}

	zipReader->FindEntries(NS_LITERAL_CSTRING("*"), getter_AddRefs(entries));
	while (1) {
		bool hasMore;
		entries->HasMore(&hasMore);
		if (!hasMore) break;
		nsCString e;
		entries->GetNext(e);

		nsCOMPtr<nsIFile> target;
		folder->Clone(getter_AddRefs(target));
		nsCString ee = e;
		while (ee.FindChar('/')!=-1) {				
			target->Append(CStringToNSString(NSCStringToCString(e).Left(ee.FindChar('/'))));
			ee.Cut(0, e.FindChar('/')+1);
		}
		target->Append(NS_ConvertUTF8toUTF16(ee));
		bool exists;
		target->Exists(&exists);
		ee.get();
		if (exists) {
			bool dir;
			target->IsDirectory(&dir);
			if (dir) continue;
			target->Remove(false);
		}

		rv = zipReader->Extract(e, target);
		NS_ENSURE_SUCCESS(rv, false);
	}

	zipReader->Close();
	return true;
}

#include "js-config.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsAppShellCID.h"
#include "nsIAppShellService.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsIJSContextStack.h"
#include "jsapi.h"

bool InjectJS(nsIDOMWindow* dom, const wchar_t* userScript, CString& result)
{
	nsresult rv;
	nsCOMPtr<nsIJSContextStack> cs = do_GetService("@mozilla.org/js/xpc/XPConnect;1");
	if (!cs) {		
		nsCOMPtr<nsIDOMDocument> document;
		dom->GetDocument(getter_AddRefs(document));
		NS_ENSURE_TRUE(document, false);

		nsCOMPtr<nsIDOMElement> body;
		rv = document->GetDocumentElement (getter_AddRefs(body));
		NS_ENSURE_SUCCESS(rv, FALSE);

		nsCOMPtr<nsIDOMElement> scriptElement;
		rv = document->CreateElement(nsDependentString(L"script"), getter_AddRefs(scriptElement));
		NS_ENSURE_SUCCESS(rv, FALSE);

		nsCOMPtr<nsIDOMHTMLScriptElement> scriptTag = do_QueryInterface(scriptElement);
		NS_ENSURE_TRUE(scriptTag, FALSE);

		scriptTag->SetText(nsDependentString(userScript));
		scriptTag->SetType(nsDependentString(L"text/javascript"));

		nsCOMPtr<nsIDOMNode> notused, node = do_QueryInterface(scriptTag);
		rv = body->AppendChild(node, getter_AddRefs(notused));
		BOOL ret = NS_SUCCEEDED(rv);
		body->RemoveChild(node, getter_AddRefs(notused));
		return ret;
	}

	nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(dom));
	if (!piWin) return false;
	nsIDocShell* docShell = piWin->GetDocShell();
	if (!docShell) return false;

	nsCOMPtr<nsIScriptGlobalObject> global = docShell->GetScriptGlobalObject();
	NS_ENSURE_TRUE(global, FALSE);

    nsPIDOMWindow *innerWin = piWin->GetCurrentInnerWindow();
	nsCOMPtr<nsIScriptGlobalObject> innerGlobal = do_QueryInterface(innerWin);
	NS_ENSURE_TRUE(innerGlobal, FALSE);
	nsCOMPtr<nsIScriptContext> scriptContext = global->GetContext();
	NS_ENSURE_TRUE(scriptContext, FALSE);
	
	JSContext* cx = scriptContext->GetNativeContext();
	if (!cx) return FALSE;

	NS_ENSURE_SUCCESS(cs->Push(cx), FALSE);
	JSAutoRequest ar(cx);
	JSAutoCompartment ac(cx, scriptContext->GetWindowProxy());
	JS::Rooted<JSObject*> globalJSObject(cx, innerGlobal->GetGlobalJSObject());
	JS::Rooted<JS::Value> v (cx, JS::UndefinedValue());
	JS_EvaluateUCScript(cx, globalJSObject, userScript, wcslen(userScript), "kmeleon", 0, &v);
	
	if (!v.isObject() && v.toString())
		result = NSStringToCString(nsString(JS_GetStringCharsZ(cx, v.toString())));
	cs->Pop(nullptr);	
	return TRUE;
}

bool RunJS(const wchar_t* userScript, CString& result)
{
	nsCOMPtr<nsIAppShellService> appShellService(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
	NS_ENSURE_TRUE(appShellService, false);
	nsCOMPtr<nsIDOMWindow> win;
	appShellService->GetHiddenDOMWindow(getter_AddRefs(win));
	NS_ENSURE_TRUE(win, false);
	return InjectJS(win, userScript, result);
}

#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "KmImage.h"

NS_IMPL_ISUPPORTS(streamListener, nsIStreamListener, nsIRequestObserver, nsISupportsWeakReference);

NS_IMETHODIMP streamListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *aContext, nsIInputStream *aInputStream, uint64_t aOffset, uint32_t aCount)
{
	if (!mStream) return NS_ERROR_FAILURE;
	char buffer[8192];
	uint32_t bytesRead;
	uint32_t bytesRemaining = aCount;

	while (bytesRemaining) {
		nsresult rv = aInputStream->Read(buffer, sizeof(buffer), &bytesRead);
		NS_ENSURE_SUCCESS(rv, rv);
		if (!bytesRead) break;

		ULONG rr;
		HRESULT hr = mStream->Write(buffer, bytesRead, &rr);
		if (hr != S_OK) return NS_ERROR_FAILURE;
		bytesRemaining -= bytesRead;
	}
    return NS_OK;
}

NS_IMETHODIMP streamListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{	
	if (!mStream)
		CreateStreamOnHGlobal(NULL, TRUE, &mStream);
    return NS_OK;
}

NS_IMETHODIMP streamListener::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatusCode)
{
	if (NS_SUCCEEDED(aStatusCode))
		mStream->Commit(STGC_DEFAULT);
	if (mObserver)
		mObserver->OnDownload(nullptr, aStatusCode, mStream.p, mPath);		
    return NS_OK;
}

bool DownloadToStream(nsIURI* uri, IDownloadObserver* observer)
{
	nsresult rv;
	nsCOMPtr<nsIIOService> io = do_GetService("@mozilla.org/network/io-service;1", &rv);
	NS_ENSURE_SUCCESS(rv, false);

	nsCOMPtr<nsIChannel> channel;
	io->NewChannelFromURI(uri, getter_AddRefs(channel));
	NS_ENSURE_TRUE(channel, false);
	
	nsCOMPtr<nsIStreamListener> l = new streamListener(observer);
	rv = channel->AsyncOpen(l, nullptr);
	return NS_SUCCEEDED(rv);
}

bool DownloadToFile(nsIURI* uri, LPCTSTR path, IDownloadObserver* observer)
{
	nsresult rv;
	nsCOMPtr<nsIIOService> io = do_GetService("@mozilla.org/network/io-service;1", &rv);
	NS_ENSURE_SUCCESS(rv, false);

	nsCOMPtr<nsIChannel> channel;
	io->NewChannelFromURI(uri, getter_AddRefs(channel));
	NS_ENSURE_TRUE(channel, false);
	
	CComPtr<IStream> stream;
	HRESULT hr = SHCreateStreamOnFile(path, 
		STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE, 
		&stream);
	/*HRESULT hr = SHCreateStreamOnFileEx(path, 
		STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_FAILIFTHERE, 
		FILE_ATTRIBUTE_NORMAL, TRUE, NULL, &stream);*/
	if (hr != S_OK) return false;

	nsCOMPtr<nsIStreamListener> l = new streamListener(observer, path, stream.p);
	rv = channel->AsyncOpen(l, nullptr);
	return NS_SUCCEEDED(rv);
}
