/*
*  Copyright (C) 2000 Christophe Thibault
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"
#include "KMeleon.h"

#include "nsEmbedAPI.h"
#include "MainFrm.h"
#include "KMeleonDoc.h"
#include "Mozilla.h"
#include <wininet.h>

#include "HiddenFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKMeleonApp

BEGIN_MESSAGE_MAP(CKMeleonApp, CWinApp)
	//{{AFX_MSG_MAP(CKMeleonApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKMeleonApp construction

CKMeleonApp::CKMeleonApp()
{
}

CKMeleonApp::~CKMeleonApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CKMeleonApp object

CKMeleonApp theApp;

void CKMeleonApp::OnFileNew(){
  CWinApp::OnFileNew();
}

/////////////////////////////////////////////////////////////////////////////
// CKMeleonApp initialization

BOOL CKMeleonApp::InitInstance()
{
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

/*	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}*/

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("K-Meleon"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

  preferences.Load();

	//SetRegistryBase (_T("Settings"));

	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	//InitKeyboardManager();

	NS_InitEmbedding(nsnull,nsnull);

  plugins.FindAndLoad("kmeleon_*.dll");

  if (!menus.Load(preferences.settingsDir + "\\menus.txt")){
    MessageBox(NULL, "Ack! Menus.txt is bad!", NULL, 0);
  }

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
  /*
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CKMeleonDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMozilla)
    );
	AddDocTemplate(pDocTemplate);
  */

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CKMeleonDoc),
		RUNTIME_CLASS(CMainFrame),       // main MDI frame window
		RUNTIME_CLASS(CMozilla));
  AddDocTemplate(pDocTemplate);

	// create main Frame window
	m_pMainWnd = new CHiddenFrame;
	if (!((CMDIFrameWnd *)m_pMainWnd)->LoadFrame(IDR_MAINFRAME))
		return FALSE;


  /*
  CMainFrame* pFrame = new CMainFrame;
  m_pMainWnd = pFrame;
  // create and load the frame with its resources
  pFrame->LoadFrame(IDR_MAINFRAME,
         WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
         NULL);
  // The one and only window has been initialized, so show and update it.
  pFrame->ShowWindow(SW_SHOW);
  pFrame->UpdateWindow();
  */

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

  /*
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
  */

	return TRUE;
}

void CKMeleonApp::LoadCustomState ()
{
}

void CKMeleonApp::SaveCustomState ()
{
}

void CKMeleonApp::createNewBrowser()
{
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CBCGURLLinkButton	m_bntURL;
	CBCGURLLinkButton	m_bntURL_GECKO;
	CBCGURLLinkButton	m_bntURL_BCG;
	CBCGURLLinkButton	m_btnMail;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_URL, m_bntURL);
	DDX_Control(pDX, IDC_URL_GECKO, m_bntURL_GECKO);
	DDX_Control(pDX, IDC_URL_BCG, m_bntURL_BCG);
	DDX_Control(pDX, IDC_MAIL, m_btnMail);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CKMeleonApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CKMeleonApp message handlers


BOOL CAboutDlg::OnInitDialog() {
	CDialog::OnInitDialog();
	
	m_btnMail.SetURLPrefix (_T("mailto:"));
	m_btnMail.SetURL (_T("christophe@nullsoft.com"));
	m_btnMail.SizeToContent ();
	m_btnMail.SetTooltip (_T("Send mail to author"));

	m_bntURL.SizeToContent ();

	m_bntURL_GECKO.SetURLPrefix(_T("http://"));
	m_bntURL_GECKO.SetURL(_T("mozilla.org"));
	m_bntURL_GECKO.SizeToContent ();
	m_bntURL_BCG.SetURLPrefix(_T("http://"));
	m_bntURL_BCG.SetURL(_T("welcome.to/BCGSoft"));
	m_bntURL_BCG.SizeToContent ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CKMeleonApp::ExitInstance() {
	NS_TermEmbedding();
	BCGCBCleanUp ();
	return CWinApp::ExitInstance();
}

BOOL CKMeleonApp::OnIdle( LONG count ){
  BOOL ret = CWinApp::OnIdle( count );

  NS_DoIdleEmbeddingStuff();

  return ret;
}