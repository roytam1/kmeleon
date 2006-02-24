#pragma once

#include "resource.h"
#include "nsIFontPackageHandler.h"
#include "DialogEx.h"

/* a3328b5b-388c-4c9a-a5b7-2213621e1744 */
#define NS_FONTPACKAGEHANDLER_CID \
{ 0xa3328b5b, \
  0x388c, \
  0x4c9a, \
  {0xa5, 0xb7, 0x22, 0x13, 0x62, 0x1e, 0x17, 0x44} }

class CFontPackageHandler : public nsIFontPackageHandler
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIFONTPACKAGEHANDLER
	
	CFontPackageHandler(void);
	virtual ~CFontPackageHandler(void);
};


// Boîte de dialogue CDownloadFontDialog

class CDownloadFontDialog : public CDialog
{
	//DECLARE_DYNAMIC(CDownloadFontDialog)

public:
	CDownloadFontDialog(const char* langcode, CWnd* pParent = NULL);   // constructeur standard
	virtual ~CDownloadFontDialog();
	CString m_LangCode;
	CString m_csFontName;
	CString m_csFontSize;

// Données de boîte de dialogue
	enum { IDD = IDD_DOWNLOADFONT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};


// Boîte de dialogue CFontNeededDialog

class CFontNeededDialog : public CDialog
{
	//DECLARE_DYNAMIC(CFontNeededDialog)

public:
	CFontNeededDialog(const char* langcode, CWnd* pParent = NULL);   // constructeur standard
	virtual ~CFontNeededDialog();
	CString m_LangCode;
	CString m_csFontName;

// Données de boîte de dialogue
	enum { IDD = IDD_NEEDFONT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge DDX/DDV

	DECLARE_MESSAGE_MAP()
};
