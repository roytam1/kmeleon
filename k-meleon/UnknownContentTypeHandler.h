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

#include "DialogEx.h"

class CUnknownContentTypeHandler : public nsIHelperAppLauncherDialog
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_UNKNOWNCONTENTTYPEHANDLER_CID );

    // ctor/dtor
    CUnknownContentTypeHandler() {
    }
    virtual ~CUnknownContentTypeHandler() {
    }

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsIHelperAppLauncherDialog interface functions.
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

	// Defered show
	NS_IMETHOD Show(CWnd* parent = NULL);

protected:
	nsCOMPtr<nsIHelperAppLauncher> mAppLauncher;

}; // CUnknownContentTypeHandler


nsresult NewUnknownContentHandlerFactory(nsIFactory** aFactory);
nsresult NewDownloadFactory(nsIFactory** aFactory);

#include "resource.h"
#include "nsITransfer.h"
#include "afxwin.h"

// Idiot callback function
typedef void (*ProgressDialogCallback)(char* uri, TCHAR* file, nsresult, void*);

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

   void InitViewer(nsIWebBrowserPersist *aPersist, TCHAR *pExternalViewer, TCHAR *pLocalFile);
   void InitPersist(nsIURI *aSource, nsILocalFile *aTarget, nsIWebBrowserPersist *aPersist, BOOL bShowDialog);

   void SetCallBack(ProgressDialogCallback, void*);

   //void InitLauncher(nsIHelperAppLauncher *aLauncher, int aHandleContentOp = 0);

   void Cancel();
   void Close();

//   void PostNcDestroy();

// seems unnecessary
//   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   //nsCOMPtr<nsIObserver> mObserver;
   //nsCOMPtr<nsIWebBrowserPersist> mPersist;
   nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;
   nsCOMPtr<nsICancelable> mCancelable;

   ProgressDialogCallback mCallback;
   void*  mParam;

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

   TCHAR *mFileName;
   TCHAR *mFilePath;
   TCHAR *mViewer;
   TCHAR *mTempFile;
   char *mUri;

   void InitControl(const char *uri, const TCHAR *filepath);
   virtual void OnCancel();
   afx_msg void OnOpen();
   afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
public:
	afx_msg void OnBnClickedCloseWhenDone();
};

// Boîte de dialogue COpenSaveDlg

class COpenSaveDlg : public CDialog
{
	//DECLARE_DYNAMIC(COpenSaveDlg)

public:
	COpenSaveDlg(CWnd* pParent = NULL);   // constructeur standard
	virtual ~COpenSaveDlg();

// Données de boîte de dialogue
	enum { IDD = IDD_OPENSAVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
	
public:
	CString m_csFilename;
	CString m_csFiletype;
	CString m_csSource;
	afx_msg void OnBnClickedOpen();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	CStatic m_cFileIcon;
	afx_msg void OnDestroy();
};

#endif
