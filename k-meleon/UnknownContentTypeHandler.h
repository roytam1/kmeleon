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
        NS_INIT_ISUPPORTS();
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

   CProgressDialog(BOOL bAuto=TRUE);
   virtual ~CProgressDialog();

   void InitViewer(nsIWebBrowserPersist *aPersist, char *pExternalViewer, char *pLocalFile);
   void InitPersist(nsIURI *aSource, nsILocalFile *aTarget, nsIWebBrowserPersist *aPersist, BOOL bShowDialog);

   void Cancel();
   void Close();

//   void PostNcDestroy();

// seems unnecessary
//   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   nsCOMPtr<nsIObserver> mObserver;
   nsCOMPtr<nsIWebBrowserPersist> mPersist;

   // this is used to calculate speed
   PRInt64 mStartTime;

   // this is used to slow down the updates
   PRInt64 mLastUpdateTime;

   PRInt32 mTotalBytes;

   int mDone;
   BOOL m_bAuto;     // automatically invoked by a download
   BOOL m_bClose;    // close the window when the download is finished
   BOOL m_bWindow;   // display a progress window
   BOOL m_bViewer;   // open the downloaded file in external viewer

   nsCAutoString m_uri;

   char *mFileName;
   char *mFilePath;
   char *mViewer;

   virtual void OnCancel();
   afx_msg void OnOpen();
   afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()
};
