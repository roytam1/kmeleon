/*
*  Copyright (C) 2006 Dorian Boissonnade
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
#include "GenericDlg.h"


const static BYTE tplGenericDlg[] = 
{
	0x01, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0xc8, 0x00, 0xc8, 0x90, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x4d, 0x00, 0x53, 0x00, 0x20, 0x00, 0x53, 0x00, 0x68, 0x00, 
		0x65, 0x00, 0x6c, 0x00, 0x6c, 0x00, 0x20, 0x00, 0x44, 0x00, 0x6c, 0x00, 0x67, 0x00, 0x00, 0x00
};

CGenericDlg::CGenericDlg(CWnd* pParent /*=NULL*/)
{
	m_pParentWnd = pParent;
	m_hIcon = NULL;
	m_hDlgIcon = NULL;
	const BYTE* caca = tplGenericDlg;
	m_uDefault = -1;
	m_uCancel = -1;
	m_IsModeless = FALSE;
}

INT_PTR CGenericDlg::DoModal()
{
	InitModalIndirect((LPCDLGTEMPLATE)tplGenericDlg, m_pParentWnd);
	return CDialog::DoModal();
}

BOOL CGenericDlg::DoModeless()
{
	m_IsModeless = TRUE;
	if (CreateIndirect((LPCDLGTEMPLATE)tplGenericDlg, m_pParentWnd)) {
		ShowWindow(SW_SHOW);
		return TRUE;
	}
	return FALSE;
}

void CGenericDlg::AddButton(UINT nID, LPCTSTR pszText)
{
	ASSERT(nID<CHECKBOX_FIRST_ID);
	ButtonInfos bi = {nID, pszText};
	m_aButtons.Add(bi);
}

void CGenericDlg::AddButton(UINT nID, UINT nIDText)
{
	CString str;
	str.LoadString(nIDText);
	AddButton(nID, (LPCTSTR)str);
}

void CGenericDlg::AddCheckBox(BOOL* result, LPCTSTR pszCheckMsg)
{
	CheckBoxInfos cbi = {CHECKBOX_FIRST_ID + (int)m_aCheckBoxes.GetSize(), result, pszCheckMsg};
	m_aCheckBoxes.Add(cbi);
}

void CGenericDlg::AddCheckBox(BOOL* result, UINT nIDText)
{
	CString str;
	str.LoadString(nIDText);
	AddCheckBox(result, (LPCTSTR)str);
}

void CGenericDlg::AddEdit(CString* result, LPCTSTR pszLabel, BOOL password)
{
	EditInfos ei = {EDIT_FIRST_ID + (int)m_aEdits.GetSize(), password, result, pszLabel};
	m_aEdits.Add(ei);
}

void CGenericDlg::AddEdit(CString* result, UINT nIDText, BOOL password)
{
	CString str;
	str.LoadString(nIDText);
	AddEdit(result, (LPCTSTR)str, password);
}

BEGIN_MESSAGE_MAP(CGenericDlg, CDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// gestionnaires de messages pour CGenericDlg

BOOL CGenericDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rc (0, 0, DLGX, DLGY);
	MapDialogRect(rc);
	m_bu = rc.Size();

	const int yMsgSpace = ConvY(MSG_SPACE);

	if (m_hDlgIcon) {
		SetIcon(m_hDlgIcon, TRUE);		// Définir une grande icône
		SetIcon(m_hDlgIcon, FALSE);		// Définir une petite icône
	}

	SetWindowText(m_csTitle);

	CPaintDC dc(this);
	CFont* pFont = GetFont();
	dc.SelectObject(pFont);

	// Get the size of the icon.
	int msgPadding = 0;
	CSize iconSize(0,0);
	if (m_hIcon)
	{
		BITMAP bm;	
		ICONINFO iconinfo;
		GetIconInfo(m_hIcon, &iconinfo);
		GetObject(iconinfo.hbmMask, sizeof(BITMAP), &bm);
		iconSize.cx = bm.bmWidth;
		iconSize.cy = bm.bmHeight;
		DeleteObject(iconinfo.hbmColor);
		DeleteObject(iconinfo.hbmMask);

		msgPadding = iconSize.cx + ConvX(MSG_SPACE);
	}

	// Edit size
	// This basically screwed support for the edit control is currently 
	// enough for what we need now.
	CSize editSize(0,0);
	int ned = (int)m_aEdits.GetSize();
	for (int i=0;i<ned;i++)
	{   
		EditInfos ei = m_aEdits[i];

		// Get the max width needed for labels
		CSize size = dc.GetTextExtent(ei.label);
		editSize.cx = max(editSize.cx, size.cx);
		//editSize.cy = max(editSize.cy, size.cy);
	}

	editSize.cy += dc.GetTextExtent(_T("X")).cy + GetSystemMetrics(SM_CYEDGE) * 2;
	if (editSize.cx) editSize.cx += ConvX(EDIT_LABEL_SPACE);

	int labelEditWidth = editSize.cx;
	int totalEditHeight = editSize.cy * ned + ConvY(EDIT_SPACE) * (ned-1);
	editSize.cx += ConvX(EDIT_SIZE_X);	

	// Compute the needed size for the check boxes
	CSize cbxSize(ConvX(BUTTON_MIN),0);
	int ncb = (int)m_aCheckBoxes.GetSize();
	for (int i=0;i<ncb;i++)
	{
		CheckBoxInfos cbi = m_aCheckBoxes[i];
		CSize size = dc.GetTextExtent(cbi.text);
		cbxSize.cx = max(cbxSize.cx, size.cx);
		// XXX: Each check box should have their own height
		// Must support multiline.
		cbxSize.cy = max(cbxSize.cy, size.cy);
	}

	cbxSize.cx += GetSystemMetrics(SM_CXMENUCHECK);
	int totalCheckBoxHeight = cbxSize.cy *ncb + ConvY(CHECKBOX_SPACE) * (ncb - 1);

	// Compute the needed size for the buttons
	CSize buttonSize(ConvX(BUTTON_MIN),0);
	int nb = (int)m_aButtons.GetSize();
	for (int i=0;i<nb;i++)
	{
		ButtonInfos bi = m_aButtons[i];
		CSize size = dc.GetTextExtent(bi.text);
		buttonSize.cx = max(buttonSize.cx, size.cx);
		buttonSize.cy = max(buttonSize.cy, size.cy);
	}

	buttonSize.cx += 2*ConvX(BUTTON_MARGIN_X);
	buttonSize.cy += 2*ConvX(BUTTON_MARGIN_Y);

	// Total width needed for the buttons
	int totalButtonsWidth = nb*buttonSize.cx + (nb-1)*ConvX(BUTTON_SPACE);

	// Message size
	CSize textSize(0,0);
	int baseWidth = max(ConvX(MSG_BASE_WIDTH), totalButtonsWidth - msgPadding);
	baseWidth = max(baseWidth, cbxSize.cx);
	baseWidth = max(baseWidth, editSize.cx);

	CRect rect(0, 0, baseWidth, 10);
	dc.DrawText(m_csMsgText, rect ,DT_CALCRECT|DT_WORDBREAK);

	CSize size = dc.GetTextExtent(_T("X"));
	if (rect.Height()>size.cy)
	{
		BOOL stop = FALSE;
		int maxSize = ::GetSystemMetrics(SM_CXFULLSCREEN)*2/3;

		// If the message is too narrow try to widen it to give
		// it a better aspect.
		int width = rect.Width();
		while (width<MSG_RATIO*rect.Height()) {
			// Widen the box size a bit.
			rect.right = width + MSG_RATIO * (rect.Height()/(MSG_RATIO+1));

			// If the size is too big, use the max size possible and stop
			if (rect.Width() > maxSize) {
				rect.right = maxSize;
				stop = TRUE;
			}
			dc.DrawText(m_csMsgText, rect ,DT_CALCRECT|DT_WORDBREAK);

			// If the width is still the same than the previous one
			// or narrower, stop too. We can't make it bigger.
			if (stop || width >= rect.Width()) break;
			width = rect.Width();
		}
	}

	textSize.cx = rect.Width();
	textSize.cy = rect.Height();

	// Compute the dialog size
	CSize dlgSize;

	dlgSize.cx = max(textSize.cx + msgPadding, totalButtonsWidth);
	dlgSize.cx = max(dlgSize.cx, cbxSize.cx + msgPadding);
	dlgSize.cx = max(dlgSize.cx, editSize.cx + msgPadding);
	dlgSize.cx += ConvX(BORDER_LEFT) + ConvX(BORDER_RIGHT);

	dlgSize.cy = 
		ConvY(BORDER_TOP) + ConvY(BORDER_BOTTOM) 
		+ max(textSize.cy, iconSize.cy)
		+ (ned ? yMsgSpace + totalEditHeight : 0)
		+ (ncb ? yMsgSpace + totalCheckBoxHeight : 0)
		+ (nb ? yMsgSpace + buttonSize.cy : 0);

	int maxWidth = ::GetSystemMetrics(SM_CXFULLSCREEN) - 2*::GetSystemMetrics(SM_CXEDGE);
	ASSERT(dlgSize.cx < maxWidth);
	if (dlgSize.cx > maxWidth) dlgSize.cx = maxWidth;

	rc.SetRect(0, 0, dlgSize.cx, dlgSize.cy);
	CalcWindowRect(rc);
	MoveWindow(rc);
	CenterWindow();

	// Create and place the controls.
	msgPadding = max( (dlgSize.cx - max(baseWidth, textSize.cx) - msgPadding)/2 + msgPadding,
		ConvX(BORDER_LEFT) + msgPadding);

	// Message 
	m_edCtrl.Create(m_csMsgText, WS_CHILD|WS_VISIBLE|SS_LEFT, CRect(0,0,0,0), this, (UINT)IDC_STATIC);
	//m_edCtrl.Create(WS_CHILD|WS_VISIBLE|ES_LEFT|ES_MULTILINE|ES_READONLY, CRect(0,0,0,0), this, (UINT)IDC_STATIC);
	//m_edCtrl.SetBackgroundColor(FALSE, ::GetSysColor(COLOR_3DFACE));
	m_edCtrl.SetFont(GetFont());

	m_edCtrl.MoveWindow(
		msgPadding, 
		ConvY(BORDER_TOP), 
		textSize.cx, 
		textSize.cy);

	// Icon
	if (m_hIcon)
	{
		m_stIconCtrl.Create(NULL, WS_CHILD|WS_VISIBLE|WS_DISABLED|SS_ICON, CRect(0,0,0,0), this);
		m_stIconCtrl.SetIcon(m_hIcon);
		m_stIconCtrl.MoveWindow(
			ConvX(BORDER_LEFT),
			ConvY(BORDER_TOP),
			iconSize.cx,
			iconSize.cy);
	}

	// Edit controls
	int bx = msgPadding;
	int by = ConvY(BORDER_TOP) + max(textSize.cy, iconSize.cy) + yMsgSpace;

	for(int i=0;i<ned;++i)
	{
		EditInfos ei = m_aEdits[i];

		// Create the label
		if (labelEditWidth) {
			CStatic label;
			label.Create(m_aEdits[i].label, WS_CHILD|WS_VISIBLE|SS_LEFT, CRect(0,0,0,0), this, (UINT)IDC_STATIC);
			label.SetFont(pFont);
			label.MoveWindow(bx, by, labelEditWidth, editSize.cy);
			label.UnsubclassWindow();
		}

		// Create the edit control.
		CEdit edit;
		int style = ei.password ? ES_PASSWORD : 0;
		edit.Create(style|ES_AUTOHSCROLL|WS_CHILD|WS_VISIBLE|WS_TABSTOP, CRect(0,0,0,0), this, ei.id);
		edit.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
		edit.SetWindowText(*ei.result);
		edit.SetFont(pFont);
		edit.MoveWindow(
			bx + labelEditWidth,
			by, 
			ConvX(EDIT_SIZE_X), 
			editSize.cy);

		edit.UnsubclassWindow();

		by += editSize.cy + ConvY(EDIT_SPACE);
	}

	if (ned) by += -ConvY(EDIT_SPACE) + yMsgSpace;

	// Checkboxes
	for(int i=0;i<ncb;++i)
	{
		CheckBoxInfos cbi = m_aCheckBoxes[i];

		CButton button;
		button.Create(cbi.text, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
			CRect(0, 0, 0, 0), this, cbi.id);
		button.SetCheck(*cbi.result);
		button.SetFont(pFont);
		button.MoveWindow(bx, by, cbxSize.cx, cbxSize.cy);
		button.UnsubclassWindow();

		by += cbxSize.cy + ConvY(CHECKBOX_SPACE);
	}

	if (ncb) by += -ConvY(CHECKBOX_SPACE) + yMsgSpace;

	// Buttons
	bx = (dlgSize.cx - totalButtonsWidth) / 2;
	for(int i=0;i<nb;++i)
	{
		ButtonInfos bi = m_aButtons[i];

		CButton button;
		button.Create(bi.text, WS_CHILD|WS_VISIBLE|WS_TABSTOP,
			CRect(0,0,0,0), this, bi.id);
		button.SetFont(pFont);
		button.MoveWindow(bx, by, buttonSize.cx, buttonSize.cy);
		button.UnsubclassWindow();

		bx += buttonSize.cx + ConvX(BUTTON_SPACE);
	}

	// Init default button
	if(m_uDefault != -1)
	{
		CWnd* pButton = GetDlgItem(m_uDefault);
		SetDefID(m_uDefault);
		// We dont want to set the focus on the default button
		// if there are edit controls.
		if (!ned) {
			if (pButton) pButton->SetFocus();
			return FALSE;
		}
	}

	return TRUE;  
}

BOOL CGenericDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if(pHandlerInfo == NULL && nCode == CN_COMMAND)
	{
		// Look if it's one of the button, since it could also
		// be also one of the checkbox.
		int nb = (int)m_aButtons.GetSize();
		for (int i=0;i<nb;i++)
		{
			ButtonInfos bi = m_aButtons[i];
			if (bi.id == nID) {
				CloseDialog(nID);     
				return TRUE;
			}
		}
	}
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CGenericDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_CANCEL)
		{
			if (m_uCancel != -1)
				CloseDialog(m_uCancel);
			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CGenericDlg::CloseDialog(int nResult)
{
	int ncb = (int)m_aCheckBoxes.GetSize();
	for (int i=0;i<ncb;i++) {
		CheckBoxInfos* cbi = &m_aCheckBoxes[i];
		*(cbi->result) = IsDlgButtonChecked(cbi->id);
	}

	int ned = (int)m_aEdits.GetSize();
	for (int i=0;i<ned;i++) {
		EditInfos* ei = &m_aEdits[i];
		CWnd* pEdit = GetDlgItem(ei->id);
		ASSERT(pEdit);
		if (pEdit)
			pEdit->GetWindowText(*(ei->result));
	}

	if (m_IsModeless)
		DestroyWindow();
	else
		EndDialog(nResult);
}

void CGenericDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	if (m_IsModeless)
		delete this;
}

void CGenericDlg::OnClose()
{
	if (m_uCancel != -1)
		CloseDialog(m_uCancel);
}
