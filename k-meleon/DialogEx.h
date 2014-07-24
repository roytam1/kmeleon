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

	/*int RunModalLoop(DWORD dwFlags)
	{
		ASSERT(::IsWindow(m_hWnd)); // window must be created
		ASSERT(!(m_nFlags & WF_MODALLOOP)); // window must not already be in modal state

		// for tracking the idle time state
		BOOL bIdle = TRUE;
		LONG lIdleCount = 0;
		BOOL bShowIdle = (dwFlags & MLF_SHOWONIDLE) && !(GetStyle() & WS_VISIBLE);
		HWND hWndParent = ::GetParent(m_hWnd);
		m_nFlags |= (WF_MODALLOOP|WF_CONTINUEMODAL);
		MSG *pMsg = AfxGetCurrentMessage();

		// acquire and dispatch messages until the modal state is done
		for (;;)
		{
			ASSERT(ContinueModal());

			// phase1: check to see if we can do idle work
			while (bIdle &&
				!::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
			{
				ASSERT(ContinueModal());

				// show the dialog when the message queue goes idle
				if (bShowIdle)
				{
					ShowWindow(SW_SHOWNORMAL);
					UpdateWindow();
					bShowIdle = FALSE;
				}

				// call OnIdle while in bIdle state
				if (!(dwFlags & MLF_NOIDLEMSG) && hWndParent != NULL && lIdleCount == 0)
				{
					// send WM_ENTERIDLE to the parent
					::SendMessage(hWndParent, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)m_hWnd);
				}
				if ((dwFlags & MLF_NOKICKIDLE) ||
					!SendMessage(WM_KICKIDLE, MSGF_DIALOGBOX, lIdleCount++))
				{
					// stop idle processing next time
					bIdle = FALSE;
				}
			}

			// phase2: pump messages while available
			do
			{
				ASSERT(ContinueModal());

				while (::PeekMessage(&(pState->m_msgCur), NULL, NULL, WM_USER, PM_NOREMOVE))
				{
					if (!theApp.PumpMessage2(WM_USER))
					{
						AfxPostQuitMessage(0);
						return -1;
					}

					// show the window when certain special messages rec'd
					if (bShowIdle &&
						(pMsg->message == 0x118 || pMsg->message == WM_SYSKEYDOWN))
					{
						ShowWindow(SW_SHOWNORMAL);
						UpdateWindow();
						bShowIdle = FALSE;
					}

					if (!ContinueModal())
						goto ExitModal;

					// reset "no idle" state after pumping "normal" message
					if (AfxIsIdleMessage(pMsg))
					{
						bIdle = TRUE;
						lIdleCount = 0;
					}
				}

				// pump message, but quit on WM_QUIT
				if (!AfxPumpMessage())
				{
					AfxPostQuitMessage(0);
					return -1;
				}				

			} while (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE));
		}

	ExitModal:
		m_nFlags &= ~(WF_MODALLOOP|WF_CONTINUEMODAL);
		return m_nModalResult;
	}*/

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