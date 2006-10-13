// GenericDlg.h : fichier d'en-tête
//

#ifndef _GENERIC_DLG_H
#define _GENERIC_DLG_H

#include "afxwin.h"
#include "afxtempl.h"
#include "DialogEx.h"
// boîte de dialogue CGenericDlg
class CGenericDlg : public CDialog
{
// Construction
public:
	CGenericDlg(CWnd* pParent = NULL);	// constructeur standard

	void AddButton(UINT nID, LPCTSTR pszText);
	void AddButton(UINT nID, UINT nIDText);
	
	inline void SetDefaultButton(UINT id) { m_uDefault = id; }
	inline void SetCancelButton(UINT id) { m_uCancel = id; }

	inline void SetTitle(LPCTSTR pszTitle) { m_csTitle = pszTitle; }
	inline void SetMsg(LPCTSTR pszMsg) { m_csMsgText = pszMsg; }

	inline void SetMsgIcon(HICON hIcon) { m_hIcon = hIcon; }
	inline void SetDlgIcon(HICON hIcon) { m_hDlgIcon = hIcon; }

	void AddCheckBox(BOOL* result, LPCTSTR pszCheckMsg);
	void AddCheckBox(BOOL* result, UINT nIDText);

	void AddEdit(CString* result, LPCTSTR pszCheckMsg, BOOL password = FALSE);
	void AddEdit(CString* result, UINT nIDText, BOOL password = FALSE);

	INT_PTR DoModal();
	BOOL DoModeless();

protected:
	//virtual void DoDataExchange(CDataExchange* pDX);	// Prise en charge DDX/DDV

// Implémentation
protected:
	HICON m_hIcon;
	HICON m_hDlgIcon;
	CString m_csTitle;
    CString m_csMsgText;
    CString m_csCheckBoxText;
    CStatic m_stIconCtrl;
	CStatic m_edCtrl;

    BOOL m_bCheckBoxValue;
	UINT m_uDefault;
	UINT m_uCancel;
	BOOL m_IsModeless;

	struct ButtonInfos
	{
		UINT id;                   
		CString text;               
	};

	CArray<ButtonInfos,const ButtonInfos&> m_aButtons;   

	struct CheckBoxInfos
	{
		UINT id;
		BOOL* result;                   
		CString text;               
	};

	CArray<CheckBoxInfos,const CheckBoxInfos&> m_aCheckBoxes;   

	struct EditInfos
	{
		UINT id;
		BOOL password;
		CString* result;                   
		CString label;               
	};

	CArray<EditInfos,const EditInfos&> m_aEdits;   

	enum { BORDER_TOP = 10 };
	enum { BORDER_BOTTOM = 5 };
	enum { BORDER_LEFT = 10 };
	enum { BORDER_RIGHT = 10 };
	enum { MSG_SPACE = 5 };
	enum { BUTTON_SPACE = 5 };
	enum { BUTTON_MIN = 40 };
	enum { BUTTON_MARGIN_X = 5 };
	enum { BUTTON_MARGIN_Y = 3 };
	
	enum { CHECKBOX_SPACE = 1 };
	
	enum {EDIT_SPACE = 2};
	enum {EDIT_LABEL_SPACE = 2};
	enum {EDIT_SIZE_X = 120};

	enum { MSG_BASE_WIDTH = 100 };
	enum { MSG_RATIO_BASE = 100 };
	enum { MSG_RATIO = 5 };

	enum { CHECKBOX_FIRST_ID = 60000 };
	enum { EDIT_FIRST_ID = 61000 };

	CSize m_bu;
	enum { DLGX = 10 };
	enum { DLGY = 10 };
	inline int ConvX(int x){ return MulDiv((int)(x), m_bu.cx, DLGX); }
	inline int ConvY(int y){ return MulDiv((int)(y), m_bu.cy, DLGY); }

	// Fonctions générées de la table des messages
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void CloseDialog(int nResult);
protected:
	virtual void PostNcDestroy();
public:
	afx_msg void OnClose();
};

#endif //_GENERIC_DLG_H
