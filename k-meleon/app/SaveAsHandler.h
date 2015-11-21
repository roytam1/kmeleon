#pragma once
#include "stdafx.h"

class nsIWebBrowserPersist;
class nsIDOMDocument;
class nsISupports;
class nsIURI;
class CBrowserFrame;

class CSaveAsHandler : public nsIWebProgressListener					    
{
public:
	CSaveAsHandler(nsIWebBrowserPersist* aPersist, nsIURI* aURL, nsIDOMDocument* aDocument,nsISupports* aDescriptor, nsIURI* aReferrer);
	virtual ~CSaveAsHandler();
	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEBPROGRESSLISTENER
	NS_IMETHOD Save(const char* contentType, const char* disposition = NULL);
	NS_IMETHOD DownloadTo(nsString& aFilename, BOOL isHTML = FALSE, int saveType = 1);
	void SetTempFile(nsIFile* aFile) { mFile = aFile; } 

protected:
	nsIWebBrowserPersist* mPersist; 
	nsCOMPtr<nsISupports> mDescriptor;
	nsCOMPtr<nsIFile> mFile;
	nsCOMPtr<nsIURI> mURL;
	nsCOMPtr<nsIURI> mRealURI;
	nsCOMPtr<nsIDOMDocument> mDocument;
	nsCOMPtr<nsIURI> mReferrer;
	nsCString mContentDisposition;
	nsCString mContentType;
};

class CMozSaveAs
{
public:
	CMozSaveAs(void);
	~CMozSaveAs(void);
};
