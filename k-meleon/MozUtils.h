
#ifndef __MOZUTILS_H__
#define __MOZUTILS_H__

#include "stdafx.h"

inline nsString CStringToNSString(LPCTSTR aStr);
inline nsCString CStringToNSCString(LPCTSTR aStr);
nsCString CStringToNSUTF8String(LPCTSTR aStr);

//__forceinline PRUnichar* CStringToPRUnichar(LPCTSTR aStr) 
//{USES_CONVERSION; return T2W(aStr);}

#define CStringToPRUnichar(str) CT2W(str)

inline CString PRUnicharToCString(const PRUnichar* str);
inline CString NSStringToCString(const nsString& aStr);
CString NSUTF8StringToCString(const nsCString& aStr);
inline CString NSCStringToCString(const nsCString& aStr);

bool ZipExtractFiles(nsIFile* zipFile, nsIFile* dir); 
nsresult NewURI(nsIURI **result, const nsACString &spec);
nsresult NewURI(nsIURI **result, const nsAString &spec);
nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget);
CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);
CString GetUriForDocument(nsIDOMDocument *aWindow);
CString GetUriForDOMWindow(nsIDOMWindow *aWindow);

CString GetMozDirectory(const char* dirName);
BOOL GetLinkTitleAndHref(nsIDOMNode* node, CString& aHref, CString& aTitle);
BOOL GetLinkTitleAndHref(nsIDOMNode* node, nsString& aHref, nsString& aTitle);
BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, CString& aUrl);
BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, nsString& aUrl);
BOOL GetImageSrc(nsIDOMNode *aNode, CString& aUrl);
BOOL GetImageSrc(nsIDOMNode *aNode, nsCString& aUrl);
BOOL IsContentEditable(nsIDOMNode* node);
CString GetSearchURL(LPCTSTR query);

bool GetFrameURL(nsIWebBrowser* aWebBrowser, nsIDOMNode* aNode, nsString& url);

BOOL LogMessage(const char* category, const char* message, const char* file, uint line, uint flags);

#endif