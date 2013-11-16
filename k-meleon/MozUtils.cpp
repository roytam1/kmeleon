#include "stdafx.h"
#include "nsIWindowWatcher.h"
#include "nsIIOService.h"

#include "nsDirectoryServiceUtils.h"

#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLObjectElement.h"

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsIDOMCharacterData.h"

#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLAnchorElement.h"

#include "nsIDOMDocument.h"
#include "nsIDOMLocation.h"

#include "nsIImageLoadingContent.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsPIDOMWindow.h"

nsEmbedString CStringToNSString(LPCTSTR aStr)
{
	USES_CONVERSION;
	return nsEmbedString(T2CW(aStr));
}

nsEmbedCString CStringToNSCString(LPCTSTR aStr)
{
	USES_CONVERSION;
	return nsEmbedCString(T2CA(aStr));
}

nsEmbedCString CStringToNSUTF8String(LPCTSTR aStr)
{
	USES_CONVERSION;
	nsEmbedCString aCStr;
	NS_UTF16ToCString(nsEmbedString(T2CW(aStr)), NS_CSTRING_ENCODING_UTF8, aCStr);
	return aCStr;
}

CString NSStringToCString(const nsEmbedString& aStr)
{
	USES_CONVERSION;
	return CString(W2CT(aStr.get()));
}

CString NSUTF8StringToCString(const nsEmbedCString& aStr)
{
	USES_CONVERSION;
	nsEmbedString aUStr;
	NS_CStringToUTF16(aStr, NS_CSTRING_ENCODING_UTF8, aUStr);
	return CString(W2CT(aUStr.get()));
}

CString NSCStringToCString(const nsEmbedCString& aStr)
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
  nsEmbedCString specUtf8;
  NS_UTF16ToCString(spec, NS_CSTRING_ENCODING_UTF8, specUtf8);
  return NewURI(result, specUtf8);
}


nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
{
	NS_ENSURE_ARG(aWebBrowser);
	nsCOMPtr<nsIDOMWindow> domWin;
	aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
	NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);

  	//nsCOMPtr<nsIDOMWindow2> domWin2 (do_QueryInterface (domWin));
	//NS_ENSURE_TRUE (domWin2, NS_ERROR_FAILURE);
	
	return domWin->GetWindowRoot (aTarget);
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
	
	nsEmbedString url;
	nsresult rv = location->GetHref(url);
	NS_ENSURE_SUCCESS(rv, _T(""));

	return NSStringToCString(url);
}

CString GetUriForDOMWindow(nsIDOMWindow *aWindow)
{
	NS_ENSURE_TRUE(aWindow, _T(""));

	nsCOMPtr<nsIDOMDocument> document;
	nsresult rv = aWindow->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, _T(""));
	
	return GetUriForDocument(document);
}

CString GetMozDirectory(const char* dirName)
{
   nsCOMPtr<nsIFile> nsDir;
   nsresult rv = NS_GetSpecialDirectory(dirName, getter_AddRefs(nsDir));
   if (NS_FAILED(rv))
      return _T("");

#ifdef _UNICODE
   nsEmbedString pathBuf;
   rv = nsDir->GetPath(pathBuf);
#else
   nsEmbedCString pathBuf;
   rv = nsDir->GetNativePath(pathBuf);
#endif

   return pathBuf.get();
}

void GatherTextUnder(nsIDOMNode* aNode, CString& aResult) 
{
	ASSERT(aNode);
	if (!aNode) return;

	nsEmbedString text;
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
			nsEmbedString data;
			charData->GetData(data);
			text += data;
		}
		else {
			nsCOMPtr<nsIDOMHTMLImageElement> img(do_QueryInterface(node));
			if (img) {
				nsEmbedString altText;
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

	aResult = NSStringToCString(text);
}

BOOL GetLinkTitleAndHref(nsIDOMNode* node, CString& aHref, CString& aTitle)
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

			bool hasAttr = PR_FALSE;
			rv = element->HasAttribute(NS_LITERAL_STRING("href"), &hasAttr);
			if (NS_SUCCEEDED(rv) && hasAttr)
			{
				nsEmbedString nsHref;

				nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(element));
                if (anchor)
					anchor->GetHref(nsHref);
				else {
					nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(element));
					if (area)
						area->GetHref(nsHref);
					else {
						nsCOMPtr<nsIDOMHTMLLinkElement> link(do_QueryInterface(element));
						if (link)
							link->GetHref(nsHref);
					}
				}
			    
				aHref = NSStringToCString(nsHref);
				GatherTextUnder(element, aTitle);
				return TRUE;
			}
		}

		// walk-up-the-tree
		node->GetParentNode(getter_AddRefs(next));
		node = next;
	} while (node);
	
	return FALSE;
}



nsresult GetCSSBackground(nsIDOMNode *node, nsEmbedString& aUrl)
{
	NS_ENSURE_ARG(node);
	nsresult rv;

	nsCOMPtr<nsIDOMDocument> document;
	node->GetOwnerDocument(getter_AddRefs(document));
	
	nsCOMPtr<nsPIDOMWindow> piWindow = do_QueryInterface(document/*domWindow*/, &rv); // How to get piWindow (from domWindow)
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(node);
	NS_ENSURE_TRUE(domElement, NS_ERROR_FAILURE);

	nsCOMPtr<nsIDOMCSSStyleDeclaration> computedStyle;
	rv = piWindow->GetComputedStyle(domElement, nsEmbedString(), getter_AddRefs(computedStyle));
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCOMPtr<nsIDOMCSSValue> cssValue;
	computedStyle->GetPropertyCSSValue(NS_LITERAL_STRING("background-image"), getter_AddRefs(cssValue));

	nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue = do_QueryInterface(cssValue);
	if (!primitiveValue) return NS_ERROR_FAILURE;
	
	PRUint16 type;
	rv = primitiveValue->GetPrimitiveType(&type);
	NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

	if (type != nsIDOMCSSPrimitiveValue::CSS_URI) 
		return NS_ERROR_FAILURE;

	nsEmbedString bgUrl;
	rv = primitiveValue->GetStringValue(bgUrl);
	NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

	// Resolve the url with the base
	nsEmbedString nsURL;
	rv = document->GetDocumentURI(nsURL);
	NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
/*
	nsCOMPtr<nsIDOMNSDocument> nsDocument(do_QueryInterface(document));
	NS_ENSURE_TRUE(document, FALSE);

	nsCOMPtr<nsIDOMLocation> location;
	nsDocument->GetLocation(getter_AddRefs(location));
	NS_ENSURE_TRUE(location, FALSE);

	nsEmbedString nsURL;
	location->GetHref(nsURL);*/

	nsCOMPtr<nsIURI> docUri;
	rv = NewURI(getter_AddRefs(docUri), nsURL);
	NS_ENSURE_SUCCESS(rv, rv);

	nsEmbedCString bgCUrl, bgCUrl2;
	NS_UTF16ToCString (bgUrl, NS_CSTRING_ENCODING_UTF8, bgCUrl);	
	rv = docUri->Resolve(bgCUrl, bgCUrl2);

	NS_CStringToUTF16(bgCUrl2, NS_CSTRING_ENCODING_UTF8, aUrl);

	return rv;
}

BOOL GetImageSrc(nsIDOMNode *aNode, CString& aUrl)
{
	NS_ENSURE_TRUE(aNode, FALSE);

	nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(aNode));
	NS_ENSURE_TRUE(content, FALSE);

	nsCOMPtr<nsIURI> imgUri;
	content->GetCurrentURI(getter_AddRefs(imgUri));
	NS_ENSURE_TRUE(imgUri, FALSE);

	nsEmbedCString url;
	imgUri->GetSpec(url);
	aUrl = NSUTF8StringToCString(url);
	return TRUE;
}

BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, CString& aUrl)
{
	NS_ENSURE_TRUE(aNode, FALSE);
	nsresult rv;
	nsEmbedString bgImg;
	nsCOMPtr<nsIDOMNode> node = aNode;

	nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(aNode);
	if (htmlElement)
	{
		nsEmbedString nameSpace;
		htmlElement->GetNamespaceURI(nameSpace);
		if (nameSpace.IsEmpty())
		{
			nsEmbedString bgImg;
			nsresult rv = GetCSSBackground(aNode, bgImg);
			if (NS_FAILED(rv)) 
			{
				// no background-image found
				nsCOMPtr<nsIDOMDocument> document;
				node->GetOwnerDocument(getter_AddRefs(document));
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

	aUrl = NSStringToCString(bgImg);
	return TRUE;
}

BOOL LogMessage(const char* category, const char* message, const char* file, uint line, uint flags)
{
	nsCOMPtr<nsIConsoleService> consoleService = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
	if (!consoleService) FALSE;
	
	nsCOMPtr<nsIScriptError> scriptError = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
	if (!scriptError) FALSE;
	
	USES_CONVERSION;
	nsresult rv = scriptError->Init(
		nsString(A2CW(message)),
		nsString(A2CW(file)),
		nsString(L""),	line, 0,
		flags,
		category);
	
	if (NS_FAILED(rv)) return FALSE;

	rv = consoleService->LogMessage(scriptError);
	return NS_SUCCEEDED(rv);
}