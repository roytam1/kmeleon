
#ifndef __MOZUTILS_H__
#define __MOZUTILS_H__

#include "stdafx.h"

inline nsEmbedString CStringToNSString(LPCTSTR aStr);
inline nsEmbedCString CStringToNSCString(LPCTSTR aStr);
nsEmbedCString CStringToNSUTF8String(LPCTSTR aStr);

//__forceinline PRUnichar* CStringToPRUnichar(LPCTSTR aStr) 
//{USES_CONVERSION; return T2W(aStr);}

#define CStringToPRUnichar(str) CT2W(str)

inline CString PRUnicharToCString(const PRUnichar* str);
inline CString NSStringToCString(nsEmbedString& aStr);
CString NSUTF8StringToCString(nsEmbedCString& aStr);
inline CString NSCStringToCString(nsEmbedCString& aStr);


nsresult NewURI(nsIURI **result, const nsACString &spec);
nsresult NewURI(nsIURI **result, const nsAString &spec);
nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget);
CWnd* CWndForDOMWindow(nsIDOMWindow *aWindow);

CString GetMozDirectory(char* dirName);
BOOL GetLinkTitleAndHref(nsIDOMNode* node, CString& aHref, CString& aTitle);
BOOL GetBackgroundImageSrc(nsIDOMNode *aNode, CString& aUrl);
BOOL GetImageSrc(nsIDOMNode *aNode, CString& aUrl);

#endif