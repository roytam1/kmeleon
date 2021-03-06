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
#include "nsIExternalHelperAppService.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIContentHandler.h"

// Idiot callback function
typedef void (*ProgressDialogCallback)(char* uri, LPCTSTR file, nsresult, void*);

class CUnknownContentTypeHandler : public nsIHelperAppLauncherDialog, public nsIContentHandler
{
public:
	NS_DEFINE_STATIC_CID_ACCESSOR( NS_UNKNOWNCONTENTTYPEHANDLER_CID );

	// ctor/dtor
	CUnknownContentTypeHandler() {}
	virtual ~CUnknownContentTypeHandler() {}

	// This class implements the nsISupports interface functions.
	NS_DECL_ISUPPORTS

	// This class implements the nsIHelperAppLauncherDialog interface functions.
	NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

	NS_DECL_NSICONTENTHANDLER

	// Defered show
	NS_IMETHOD Show(CWnd* parent = NULL);

protected:
	CString GetTypeName();
	nsCOMPtr<nsIHelperAppLauncher> mAppLauncher;
	nsCOMPtr<nsIDOMWindow> mDomWindow;

}; // CUnknownContentTypeHandler


//nsresult NewUnknownContentHandlerFactory(nsIFactory** aFactory);
//nsresult NewDownloadFactory(nsIFactory** aFactory);

#include "resource.h"
#include "nsITransfer.h"
class nsIWebBrowserPersist;

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

	void InitPersist(nsIURI *aSource, nsIFile *aTarget, nsIWebBrowserPersist *aPersist, BOOL bShowDialog);

	void SetCallBack(ProgressDialogCallback, void*);

	//void InitLauncher(nsIHelperAppLauncher *aLauncher, int aHandleContentOp = 0);

	void Cancel();
	void Close();

protected:
	//nsCOMPtr<nsIObserver> mObserver;
	//nsCOMPtr<nsIWebBrowserPersist> mPersist;
	nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;
	nsCOMPtr<nsICancelable> mCancelable;
	nsCOMPtr<nsIRequest> mRequest;
	nsCOMPtr<nsIMIMEInfo> mMIMEInfo;
	nsCOMPtr<nsIURI> mTarget;
	nsCOMPtr<nsIFile> mTempFile;

	ProgressDialogCallback mCallback;
	void*  mParam;
	BOOL mPaused;

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

	nsCString m_uri;

	CString mFileName;
	CString mFilePath;
	//CString mTempFile;
	char *mUri;

	void InitControl(const char *uri, const TCHAR *filepath);
	virtual void OnCancel();
	afx_msg void OnOpen();
	afx_msg void OnClose();
	afx_msg void OnPause();
	afx_msg void OnOpenFolder();

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	nsresult ExecuteDesiredAction();
	nsresult OpenWithApplication();
	nsresult MoveTempToTarget();
	nsresult GetTargetFile(nsIFile **aTargetFile);

public:
	afx_msg void OnBnClickedCloseWhenDone();
	afx_msg BOOL OnQueryEndSession();
};

class COpenSaveDlg : public CDialog
{
	//DECLARE_DYNAMIC(COpenSaveDlg)

public:
	COpenSaveDlg(CWnd* pParent = NULL);   // constructeur standard
	virtual ~COpenSaveDlg();
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
