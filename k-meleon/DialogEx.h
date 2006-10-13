// This class is just here so I can change some settings
// in the template, using the font set in the control 
// panel for the dialogs

#ifndef _DIALOGS_EX_H_
#define _DIALOGS_EX_H_

#include "afxpriv.h" 

class CDialogTemplateEx: public CDialogTemplate
{
public:
	int SetTemplate(const DLGTEMPLATE *pTemplate) {
		return CDialogTemplate::SetTemplate(pTemplate, GetTemplateSize(pTemplate));
	}
};

class CDialogEx: public CDialog
{
public:

	CDialogEx() : CDialog() {};
	CDialogEx(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL) 
		: CDialog(lpszTemplateName, pParentWnd) {};
	CDialogEx(UINT nIDTemplate, CWnd* pParentWnd = NULL)
		: CDialog(nIDTemplate, pParentWnd) {};
	~CDialogEx() {};

	INT DoModal()
	{  
		//CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
		//if (pApp->preferences.GetBool("kmeleon.display.dialogs.useUserDialogFont", true)) {
		//	return CDialog::DoModal();
		//}

		ASSERT(m_lpszTemplateName != NULL || m_lpDialogTemplate != NULL);
		if (m_lpszTemplateName != NULL) {
			if (!m_dlt.Load(m_lpszTemplateName)) 
				return -1;
		}
		else
			if (!m_dlt.SetTemplate(m_lpDialogTemplate))
				return -1;

		struct font_info font;
		if (GetMessageFont(font))
			m_dlt.SetFont(font.lpFaceName, (WORD)font.nPointSize);

		LPSTR pdata = (LPSTR)GlobalLock(m_dlt.m_hTemplate);

		m_lpszTemplateName = NULL;
		m_lpDialogTemplate = NULL;
		InitModalIndirect(pdata);

		INT result = CDialog::DoModal();

		GlobalUnlock(m_dlt.m_hTemplate);

		return result;
	}

	BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
	{
		ASSERT(m_lpszTemplateName == NULL);
		if (!m_dlt.Load(lpszTemplateName)) return FALSE;
		struct font_info font;
		if ( 
			//pApp->preferences.GetBool("kmeleon.display.dialogs.useUserFont", true) &&
			  GetMessageFont(font))
			m_dlt.SetFont(font.lpFaceName, (WORD)font.nPointSize);

		HINSTANCE hInst = AfxFindResourceHandle(lpszTemplateName, RT_DIALOG);

		return CDialog::CreateIndirect(m_dlt.m_hTemplate, pParentWnd, hInst);
	}

	BOOL Create(UINT nIDTemplate, CWnd* pParentWnd)
	{
		return Create(MAKEINTRESOURCE(nIDTemplate), pParentWnd);
	}

	BOOL CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd* pParentWnd = NULL)
	{
		if (!m_dlt.SetTemplate(lpDialogTemplate))
			return FALSE;
		
		struct font_info font;
		if (GetMessageFont(font))
			m_dlt.SetFont(font.lpFaceName, (WORD)font.nPointSize);

		return CDialog::CreateIndirect(m_dlt.m_hTemplate, pParentWnd);
	}

protected:
	CDialogTemplateEx m_dlt;

	struct font_info {
		int nPointSize;
		CString lpFaceName;
	};

	BOOL GetMessageFont(struct font_info &fi)
	{
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0))
			return FALSE;

		LOGFONT &lf = ncm.lfMessageFont;

		if (lf.lfHeight < 0)
			lf.lfHeight = -lf.lfHeight;
		
		HDC hDC    = ::GetDC(NULL);
		fi.nPointSize  = MulDiv(lf.lfHeight,72,GetDeviceCaps(hDC, LOGPIXELSY));
		::ReleaseDC(NULL,hDC);
	
		fi.lpFaceName = lf.lfFaceName;

		return TRUE;
	}
};

#define CDialog CDialogEx

#endif