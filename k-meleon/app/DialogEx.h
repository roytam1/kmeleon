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

class CDialogEx2: public CDialog
{
public:

	CDialogEx2() : CDialog() {};
	CDialogEx2(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL) 
		: CDialog(lpszTemplateName, pParentWnd) {};
	CDialogEx2(UINT nIDTemplate, CWnd* pParentWnd = NULL)
		: CDialog(nIDTemplate, pParentWnd) {};
	~CDialogEx2() {};
	//int RunModalLoop(DWORD dwFlags = 0);
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

		INT result = _DoModal();//CDialog::DoModal();

		GlobalUnlock(m_dlt.m_hTemplate);

		return result;
	}

	INT_PTR _DoModal()
	{
		// can be constructed with a resource template or InitModalIndirect
		ASSERT(m_lpszTemplateName != NULL || m_hDialogTemplate != NULL ||
			m_lpDialogTemplate != NULL);

		// load resource as necessary
		LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
		HGLOBAL hDialogTemplate = m_hDialogTemplate;
		HINSTANCE hInst = AfxGetResourceHandle();
		if (m_lpszTemplateName != NULL)
		{
			hInst = AfxFindResourceHandle(m_lpszTemplateName, RT_DIALOG);
			HRSRC hResource = ::FindResource(hInst, m_lpszTemplateName, RT_DIALOG);
			hDialogTemplate = LoadResource(hInst, hResource);
		}
		if (hDialogTemplate != NULL)
			lpDialogTemplate = (LPCDLGTEMPLATE)LockResource(hDialogTemplate);

		// return -1 in case of failure to load the dialog template resource
		if (lpDialogTemplate == NULL)
			return -1;

		// disable parent (before creating dialog)
		HWND hWndParent = PreModal();
		AfxUnhookWindowCreate();
		BOOL bEnableParent = FALSE;
#ifndef _AFX_NO_OLE_SUPPORT
		CWnd* pMainWnd = NULL;
		BOOL bEnableMainWnd = FALSE;
#endif
		if (hWndParent && hWndParent != ::GetDesktopWindow() && ::IsWindowEnabled(hWndParent))
		{
			::EnableWindow(hWndParent, FALSE);
			bEnableParent = TRUE;
#ifndef _AFX_NO_OLE_SUPPORT
			pMainWnd = AfxGetMainWnd();
			if (pMainWnd && pMainWnd->IsFrameWnd() && pMainWnd->IsWindowEnabled())
			{
				//
				// We are hosted by non-MFC container
				// 
				pMainWnd->EnableWindow(FALSE);
				bEnableMainWnd = TRUE;
			}
#endif
		}

		TRY
		{
			// create modeless dialog
			AfxHookWindowCreate(this);
			if (CreateDlgIndirect(lpDialogTemplate,
				CWnd::FromHandle(hWndParent), hInst))
			{
				if (m_nFlags & WF_CONTINUEMODAL)
				{
					// enter modal loop
					DWORD dwFlags = MLF_SHOWONIDLE;
					if (GetStyle() & DS_NOIDLEMSG)
						dwFlags |= MLF_NOIDLEMSG;
					VERIFY(RunModalLoop(dwFlags) == m_nModalResult);
				}

				// hide the window before enabling the parent, etc.
				if (m_hWnd != NULL)
					SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
					SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
			}
		}
		CATCH_ALL(e)
		{
			if(e) { e->Delete(); }
			m_nModalResult = -1;
		}
		END_CATCH_ALL

#ifndef _AFX_NO_OLE_SUPPORT
			if (bEnableMainWnd)
				pMainWnd->EnableWindow(TRUE);
#endif
		if (bEnableParent)
			::EnableWindow(hWndParent, TRUE);
		if (hWndParent != NULL && ::GetActiveWindow() == m_hWnd)
			::SetActiveWindow(hWndParent);

		// destroy modal window
		DestroyWindow();
		PostModal();

		// unlock/free resources as necessary
		if (m_lpszTemplateName != NULL || m_hDialogTemplate != NULL)
			UnlockResource(hDialogTemplate);
		if (m_lpszTemplateName != NULL)
			FreeResource(hDialogTemplate);

		return m_nModalResult;
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

#define CDialog CDialogEx2

#endif