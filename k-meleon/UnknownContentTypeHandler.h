#ifndef __UNKNOWNCONTENTTYPE__
#define __UNKNOWNCONTENTTYPE__

//static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

// {42770B50-03E9-11d3-8068-00600811A9C3}
#define NS_UNKNOWNCONTENTTYPEHANDLER_CID \
    { 0x42770b50, 0x3e9, 0x11d3, { 0x80, 0x68, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3 } }
static NS_DEFINE_CID(kUnknownContentTypeHandlerCID, NS_UNKNOWNCONTENTTYPEHANDLER_CID);

#define NS_DOWNLOAD_CID \
{ 0xe3fa9d0a, 0x1dd1, 0x11b2, { 0xbd, 0xef, 0x8c, 0x72, 0x0b, 0x59, 0x74, 0x45 } }
static NS_DEFINE_CID(kDownloadCID, NS_DOWNLOAD_CID);

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
#include "nsITransfer.h"

class CProgressDialog : public CDialog,
						public nsITransfer,
						public nsSupportsWeakReference
					    //,public nsIWebProgressListener2
{
public:
   enum { IDD = IDD_PROGRESS };

   NS_DECL_ISUPPORTS
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_DECL_NSIWEBPROGRESSLISTENER2
   NS_DECL_NSITRANSFER

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
   //nsCOMPtr<nsIObserver> mObserver;
   nsCOMPtr<nsIWebBrowserPersist> mPersist;
   nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;
   nsCOMPtr<nsICancelable> mCancelable;
   int m_HandleContentOp;

   // this is used to calculate speed
   PRInt64 mStartTime;

   // this is used to slow down the updates
   PRInt64 mLastUpdateTime;

   PRInt64 mTotalBytes;

   int mDone;
   BOOL m_bAuto;     // automatically invoked by a download
   BOOL m_bClose;    // close the window when the download is finished
   BOOL m_bWindow;   // display a progress window
   BOOL m_bViewer;   // open the downloaded file in external viewer
   
   nsEmbedCString m_uri;

   char *mFileName;
   char *mFilePath;
   char *mViewer;
   char *mTempFile;

   void InitControl(const char *uri, const char *filepath);
   virtual void OnCancel();
   afx_msg void OnOpen();
   afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()
};

#endif
