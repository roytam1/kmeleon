#include "stdafx.h"

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

// {42770B50-03E9-11d3-8068-00600811A9C3}
#define NS_UNKNOWNCONTENTTYPEHANDLER_CID \
    { 0x42770b50, 0x3e9, 0x11d3, { 0x80, 0x68, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3 } }
static NS_DEFINE_CID(kUnknownContentTypeHandlerCID, NS_UNKNOWNCONTENTTYPEHANDLER_CID);

class CUnknownContentTypeHandler : public nsIHelperAppLauncherDialog {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_UNKNOWNCONTENTTYPEHANDLER_CID );

    // ctor/dtor
    CUnknownContentTypeHandler() {
        NS_INIT_REFCNT();
    }
    virtual ~CUnknownContentTypeHandler() {
    }

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIHelperAppLauncherDialog interface functions.
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

}; // CUnknownContentTypeHandler


nsresult NewUnknownContentHandlerFactory(nsIFactory** aFactory);
nsresult NewDownloadFactory(nsIFactory** aFactory);



#include "resource.h"

class CProgressDialog : public CDialog,
                        public nsIWebProgressListener,
                        public nsIDownload
// not needed?
//                        , public nsSupportsWeakReference
                         {
public:
   enum { IDD = IDD_PROGRESS };

   NS_DECL_ISUPPORTS
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_DECL_NSIDOWNLOAD

   CProgressDialog();
   virtual ~CProgressDialog();

   void Cancel();
   void Close();

//   void PostNcDestroy();

// seems unnecessary
//   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   nsCOMPtr<nsIObserver> mObserver;

   // this is used to calculate speed
   PRInt64 mStartTime;

   // this is used to slow down the updates
   PRInt64 mLastUpdateTime;

   PRInt32 mTotalBytes;

   int mDone;

   char *mFileName;
   char *mFilePath;

   virtual void OnCancel();
   afx_msg void OnOpen();
   afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()
};
