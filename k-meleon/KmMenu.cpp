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
#include ".\kmmenu.h"

#include "mfcembed.h"
#include "browserfrm.h"
#include "browserfrmtab.h"
#include "VisualStylesXP.h"

extern CMfcEmbedApp theApp;
extern BOOL ParsePluginCommand(char *pszCommand, char** plugin, char **parameter);

int Translate(LPTSTR originalText, CString& translatedText)
{
	USES_CONVERSION;
	int r;

	// I have to remove the accel text for translation..
	TCHAR* accel = _tcschr(originalText, _T('\t'));
	if (accel) *accel = 0;
	//TrimWhiteSpace(originalText);

	if (!(r=theApp.lang.Translate(originalText, translatedText)))
		translatedText = originalText;

	//.. and put it back
	/*if (accel) {
	*accel = '\t';
	TrimWhiteSpace(accel);
	translatedText += A2T(accel);
	}*/

	return r;
}

void KmMenu::RemoveItem(KmMenuItem& item)
{

	// Deletion by command
	if (!item.label[0]) { 
		POSITION pos = mMenuDef.GetHeadPosition();
		while (pos) {
			KmMenuItem item2 = mMenuDef.GetAt(pos);
			if (item2.command == item.command) {
				mMenuDef.RemoveAt(pos);
				return;
			}
			mMenuDef.GetNext(pos);
		}
	}

	// Deletion by name
	if (item.command < 1) {
		POSITION pos = mMenuDef.GetHeadPosition();
		while (pos) {
			KmMenuItem item2 = mMenuDef.GetAt(pos);
			if (strcmp(item2.label, item.label) == 0) {
				mMenuDef.RemoveAt(pos);
				USES_CONVERSION;
				KmMenu* iMenu = theApp.menus.GetKMenu(A2CT(item.label));
				if (iMenu) {
					POSITION pos = iMenu->mDependencies.Find(this);
					if (pos) iMenu->mDependencies.RemoveAt(pos);
				}
				break;
			}
			mMenuDef.GetNext(pos);
		}
		return;
	}
}

void KmMenu::AddItem(KmMenuItem& item, long before)
{
	if ((item.command<1) ^ !item.label[0]) {
		RemoveItem(item);
		return;
	}

	if (item.type == MenuString) {
		// There can't be two item with the same id
		POSITION pos = mMenuDef.GetHeadPosition();
		while (pos) {
			KmMenuItem* item2 = &mMenuDef.GetAt(pos);
			if (item2->command == item.command) {
				mMenuDef.RemoveAt(pos);
				break;
				*item2 = item; // Replace old with new
				return;
			}
			mMenuDef.GetNext(pos);
		}
	} 
	else if (item.type == MenuInline || item.type == MenuSpecial) {
		USES_CONVERSION;
		KmMenu* iMenu = theApp.menus.GetKMenu(A2CT(item.label));
		if (!iMenu)
			iMenu = theApp.menus.CreateMenu(A2CT(item.label));
		else {
			// There can't be twice the same inline menu
			if (iMenu->mDependencies.Find(this) != NULL)
				return;
		}
		iMenu->mDependencies.AddHead(this);
	}

	// Custom position 
	if (before != -1) {
		if (before < 100) { // Index position
			POSITION pos = mMenuDef.GetHeadPosition();
			while (pos && before--)
				mMenuDef.GetNext(pos);
			if (pos) {
				mMenuDef.InsertBefore(pos, item);
				return;
			}
		}
		else if (before < 0xffff) { // command position
			POSITION pos = mMenuDef.GetHeadPosition();
			while (pos) {
				KmMenuItem item2 = mMenuDef.GetAt(pos);
				if (item2.command == before) {
					mMenuDef.InsertBefore(pos, item);
					return;
				}
				mMenuDef.GetNext(pos);
			}
		}
		else { // label position
			POSITION pos = mMenuDef.GetHeadPosition();
			while (pos) {
				KmMenuItem item2 = mMenuDef.GetAt(pos);
				if (strcmp(item2.label, (char*)before) == 0) {
					mMenuDef.InsertBefore(pos, item);
					return;
				}
				mMenuDef.GetNext(pos);
			}
		}
	}

	mMenuDef.AddTail(item);
}

BOOL KmMenu::Build()
{
	if (mMenu.m_hMenu) {
		Reset();
	}
	else {
		if (mPopup) mMenu.CreatePopupMenu();
		else mMenu.CreateMenu();
	}

	ASSERT(mMenu.m_hMenu);

	if (!Build(mMenu, -1))
		return FALSE;

	if (this == theApp.menus.GetKMenu(_T("Main")))
	{
		CFrameWnd* pBrowserFrame = NULL;
		POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
		while( pos != NULL ) {
			pBrowserFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetNext(pos);
			if(pBrowserFrame)
				pBrowserFrame->DrawMenuBar();
		}
	}

	mInvalid = FALSE;
	return TRUE;
}

HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
    *phBmp = NULL;

    BITMAPINFO bmi;
    SecureZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmi.bmiHeader.biWidth = psize->cx;
    bmi.bmiHeader.biHeight = psize->cy;
    bmi.bmiHeader.biBitCount = 32;

    HDC hdcUsed = hdc ? hdc : GetDC(NULL);
    if (hdcUsed)
    {
        *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
        if (hdc != hdcUsed)
        {
            ReleaseDC(NULL, hdcUsed);
        }
    }
    return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HBITMAP IconToBitmap(HIMAGELIST il, int idx) { 

	IWICImagingFactory *pFactory;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));

	HICON hicon =  ImageList_ExtractIcon(0,il,idx);
	HBITMAP hbmp = NULL;
    if (SUCCEEDED(hr))

    {

        IWICBitmap *pBitmap;

        hr = pFactory->CreateBitmapFromHICON(hicon, &pBitmap);

        if (SUCCEEDED(hr))

        {

            UINT cx, cy;

            hr = pBitmap->GetSize(&cx, &cy);
			
            if (SUCCEEDED(hr))

            {

                const SIZE sizIcon = { (int)cx, -(int)cy };

                BYTE *pbBuffer;

                hr = Create32BitHBITMAP(NULL, &sizIcon, reinterpret_cast<void **>(&pbBuffer), &hbmp);

                if (SUCCEEDED(hr))

                {

                    const UINT cbStride = cx * sizeof(DWORD);

                    const UINT cbBuffer = cy * cbStride;

                    hr = pBitmap->CopyPixels(NULL, cbStride, cbBuffer, pbBuffer);

                }

            }

            pBitmap->Release();

        }

        pFactory->Release();

    }
	return hbmp;
}

BOOL KmMenu::Build(CMenu &menu, int before)
{
	BOOL wasSeparator = TRUE;
	BOOL mAccelText = theApp.preferences.GetBool("kmeleon.display.accelInMenus", TRUE);
	POSITION pos = mMenuDef.GetHeadPosition();
	for (int i=0;i < mMenuDef.GetCount();i++)
	{
		KmMenuItem item = mMenuDef.GetNext(pos);

		USES_CONVERSION;
		CMenu *popup;
		LPCTSTR label;

		switch (item.type) {
			case MenuPopup: // Popup Menu
				label = A2CT(item.label);
				popup = theApp.menus.GetMenu(label);
				
					
				if (popup) {
					
					menu.InsertMenu(before, MF_POPUP, (UINT)popup->m_hMenu, theApp.lang.Translate(label));
					if (this != theApp.menus.GetKMenu(_T("Main"))) {
						if (theApp.menus.IsOwnerDraw()) {
							int pos;
							for (pos=0;pos<menu.GetMenuItemCount();pos++)
								if (menu.GetSubMenu(pos) == popup)
									break;
							ASSERT(pos<menu.GetMenuItemCount());
							MENUITEMINFO mi = {0};
							mi.cbSize = sizeof(mi);
							mi.fMask = MIIM_DATA | MIIM_FTYPE;
							mi.fType = MF_OWNERDRAW;
							mi.dwItemData = (ULONG_PTR)&item;//(ULONG_PTR)_wcsdup((LPCTSTR)pTranslated);
							menu.SetMenuItemInfo(pos, &mi, TRUE);
						}
					}
					
					//LOG_1("Added popup %s", label);
				}
				else
					ASSERT(TRUE);
					//LOG_ERROR_1("Popup %s not found!", label);
				wasSeparator = FALSE;
				break;

			case MenuSpecial: // Special Menu

				if (strcmpi(item.label+1, "TabList") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame ||
						!theApp.m_pMostRecentBrowserFrame->IsKindOf(RUNTIME_CLASS(CBrowserFrmTab)))
					   break;

					((CBrowserFrmTab*)theApp.m_pMostRecentBrowserFrame)->DrawTabListMenu(menu.GetSafeHmenu());
				}
				else if (strcmpi(item.label+1, "WindowList") == 0) 
					theApp.DrawWindowListMenu(menu.GetSafeHmenu());
				else if (strcmpi(item.label+1, "SHistoryBack") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame) break;
					theApp.m_pMostRecentBrowserFrame->DrawSHBackMenu(menu.GetSafeHmenu());
				}
				else if (strcmpi(item.label+1, "SHistoryForward") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame) break;
					theApp.m_pMostRecentBrowserFrame->DrawSHForwardMenu(menu.GetSafeHmenu());
				}
				else if (strcmpi(item.label+1, "SHistory") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame) break;
					theApp.m_pMostRecentBrowserFrame->DrawSHMenu(menu.GetSafeHmenu());
				}
				else if (strcmpi(item.label+1, "ToolBars") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame) break;
					theApp.m_pMostRecentBrowserFrame->m_wndReBar.DrawToolBarMenu(menu.GetSafeHmenu());
				}
#ifdef INTERNAL_SIDEBAR
				else if (strcmpi(item.label+1, "SideBars") == 0) 
				{
					if (!theApp.m_pMostRecentBrowserFrame) break;
					theApp.m_pMostRecentBrowserFrame->m_wndSideBar.DrawSideBarMenu(menu.GetSafeHmenu());
				}
#endif
				wasSeparator = FALSE;
				break;

			case MenuInline: { // Inline menu
				label = A2CT(item.label);
				KmMenu* inlineMenu = theApp.menus.GetKMenu(label);
				ASSERT(inlineMenu);
				if (inlineMenu && !inlineMenu->IsEmpty()) {	
					if (!wasSeparator)
						menu.InsertMenu(before, MF_SEPARATOR);
					inlineMenu->Build(menu, before);
					wasSeparator = FALSE;
				}
				break;
			}

			case MenuSeparator: {// Separator
				if (wasSeparator || (i == mMenuDef.GetCount()-1)) break;
				menu.InsertMenu(before, MF_SEPARATOR);
				/*MENUITEMINFO mi = {0};
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_DATA | MIIM_TYPE;
				mi.dwItemData = (ULONG_PTR)&item;
				mi.fType = MF_SEPARATOR | MF_OWNERDRAW;
				menu.InsertMenuItem(before, &mi, TRUE);	*/			
				wasSeparator = TRUE;
				//LOG_1("Added Separator", 0);
				break;
			}

			case MenuPlugin: {
				char *plugin, *parameter;
				ParsePluginCommand(item.label, &plugin, &parameter);

				if (theApp.plugins.SendMessage(plugin, "* MenuParser", "DoMenu", (long)menu.GetSafeHmenu(), (long)parameter)) {
					//LOG_2("Called plugin %s with parameter %s", plugin, parameter);
				}
				else {
					//LOG_ERROR_1( "Plugin %s has no menu", plugin);
				}
				wasSeparator = FALSE;
				break;
								  }

			case MenuString: {
				TCHAR* _label = A2T(item.label); 

				CString pTranslated;
				Translate(_label, pTranslated);

				if (mAccelText) {
					CString accel = theApp.accel.GetStrAccel(item.command);
					if (accel.GetLength())
						pTranslated += _T("\t") + accel;
				}

				// Not setting the item to ownerdraw directly allow
				// the menu accelerator to work
				menu.InsertMenu(before, MF_STRING, item.command, pTranslated);
				//menu.ModifyMenu(item.command, MF_STRING | MF_OWNERDRAW, item.command, _wcsdup(pTranslated));
								
				if (!theApp.menus.IsOwnerDraw())
				{
					int idx = theApp.skin.GetIconIndex(item.command);
					if (idx >= 0) {
						MENUITEMINFO mi = {0};
						mi.cbSize = sizeof(mi);
						HIMAGELIST il = theApp.skin.GetIconList();
						mi.fMask = MIIM_CHECKMARKS;
						HICON icon = ImageList_ExtractIcon(0, il, idx);
						ICONINFO ii;
						GetIconInfo(icon, &ii);
						DeleteObject(ii.hbmMask);
						DeleteObject(icon);
						mi.hbmpChecked = mi.hbmpUnchecked = ii.hbmColor;//IconToBitmap(il, idx);
						menu.SetMenuItemInfo(item.command, &mi);
					}
				} else {
					MENUITEMINFO mi = {0};
					mi.cbSize = sizeof(mi);
					mi.fMask = MIIM_FTYPE;
					mi.fType = MF_STRING | MF_OWNERDRAW;
					mi.dwItemData = 0;//(ULONG_PTR)&item;//(ULONG_PTR)_wcsdup((LPCTSTR)pTranslated);
					menu.SetMenuItemInfo(item.command, &mi);
				}				
				
				
				
				/*
				int idx = theApp.skin.GetIconIndex(item.command);
				HIMAGELIST il = theApp.skin.GetIconList();
				
				
				MENUITEMINFO mi = {0};
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_CHECKMARKS;
				//mi.dwItemData = (ULONG_PTR)&item;
				//mi.hbmpChecked = mi.hbmpUnchecked = IconToBitmap(il, idx);
				//mi.hbmpItem = IconToBitmap(il, idx);
				KmImage img;
				img.LoadFromIcon(ImageList_ExtractIcon(0,il,idx));
				//img.LoadIndexedFromSkin(L"menu3.png[0]",16,16);
				mi.hbmpChecked = mi.hbmpUnchecked =  img.GetHBitmap();
				//mi.fType = MF_STRING | MF_OWNERDRAW;
				//mi.wID = item.command;
				UINT toto = menu.SetMenuItemInfo(item.command, &mi);
				UINT tata = GetLastError();*/
				//LOG_2("Added menu item %s with command %d", _label, item.command);
				wasSeparator = FALSE;
				break;
								  }

			default: 
				label = A2CT(item.label); 
				wasSeparator = FALSE;
				//LOG_ERROR_1("Undefined menu %s", label);
		}

	}
	return TRUE;
}

void KmMenu::Reset()
{
	if (!mMenu.m_hMenu) return;
	while (mMenu.GetMenuItemCount()) {
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_CHECKMARKS;
		mMenu.GetMenuItemInfo(0, &mii, TRUE);
		if (mii.hbmpChecked) DeleteObject(mii.hbmpChecked);
		/*if (mii.dwItemData && (mii.fMask & MF_OWNERDRAW) {
			if (mii.dwItemData) LocalFree(mii.dwItemData);
		}*/
		mMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

void KmMenu::Invalidate() 
{
	mInvalid = TRUE;

	// Invalidate menus using this menu 
	POSITION pos = mDependencies.GetHeadPosition();
	while (pos) {
		KmMenu* depMenu = mDependencies.GetNext(pos);
		depMenu->Invalidate();
	}
}

void DrawBitmap(HDC dc, HBITMAP bmp, RECT rc)
{
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;
	HDC src_dc = ::CreateCompatibleDC(dc);
	HGDIOBJ old = ::SelectObject(src_dc, bmp);
	::AlphaBlend(dc, rc.left, rc.top, theApp.skin.GetDefWidth(), theApp.skin.GetDefHeight(), src_dc, 0, 0, theApp.skin.GetDefWidth(), theApp.skin.GetDefHeight(), bf);
	::SelectObject(src_dc, old);
	::DeleteDC(src_dc);
}

void KmMenuService::DrawBitmap(LPDRAWITEMSTRUCT dis)
{
	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_CHECKMARKS;
	GetMenuItemInfo((HMENU)dis->hwndItem, dis->itemID, FALSE, &mi);
	if (mi.hbmpChecked) {
		RECT rc = dis->rcItem;
		rc.top = dis->rcItem.top + ((dis->rcItem.bottom - dis->rcItem.top - theApp.skin.GetDefHeight()) / 2);
		::DrawBitmap(dis->hDC,
			dis->itemState & ODS_CHECKED ? mi.hbmpChecked : mi.hbmpUnchecked,
			rc);
		return;
	}	

	int idx = theApp.skin.GetIconIndex(dis->itemID);
	if (dis->itemState & ODS_CHECKED) {
		int cxCheck = GetSystemMetrics(SM_CXMENUCHECK);
		int cyCheck = GetSystemMetrics(SM_CYMENUCHECK);
		
		HDC hdcMem = CreateCompatibleDC(dis->hDC);
		if (hdcMem) {
			HBITMAP hbmMono = CreateBitmap(cxCheck, cyCheck, 1, 1, NULL);
			if (hbmMono) {
				HBITMAP hbmPrev = (HBITMAP)SelectObject(hdcMem, (HGDIOBJ)hbmMono);
				if (hbmPrev) {
					RECT rc = { 0, 0, cxCheck, cyCheck };
					DrawFrameControl(hdcMem, &rc, DFC_MENU, DFCS_MENUCHECK);
					BitBlt(dis->hDC, dis->rcItem.left, 
						dis->rcItem.top + (dis->rcItem.bottom - dis->rcItem.top - cyCheck)/2, // Seems like it need some margin
						cxCheck, cyCheck, hdcMem, 0, 0, SRCCOPY);
					SelectObject(hdcMem, (HGDIOBJ)hbmPrev);
				}
				DeleteObject(hbmMono);
			}
			DeleteDC(hdcMem);
		}
		
	} else if (idx >= 0) {

			int top = (dis->rcItem.bottom - dis->rcItem.top - theApp.skin.GetDefHeight()) / 2;
			top += dis->rcItem.top;

			if (dis->itemState & ODS_GRAYED)
				ImageList_DrawEx(theApp.skin.GetIconList(), idx, dis->hDC, dis->rcItem.left, top, 0, 0, CLR_NONE, GetSysColor(COLOR_MENU), ILD_BLEND  | ILD_TRANSPARENT);

			else if (dis->itemState & ODS_SELECTED)
				ImageList_Draw(theApp.skin.GetIconList(), idx, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);

			else
				ImageList_Draw(theApp.skin.GetIconList(), idx, dis->hDC, dis->rcItem.left, top, ILD_TRANSPARENT);

	}
}

int KmMenu::GetMaxAccelWidth(HDC hDC)
{
	USES_CONVERSION;
	
	int maxAccelWidth = 0;
	POSITION pos = mMenuDef.GetHeadPosition();
	for (int i=0;i < mMenuDef.GetCount();i++)
	{
		KmMenuItem& item = mMenuDef.GetNext(pos);
		int accelWidth = 0;
		if (item.type == MenuString) {
			SIZE size;
			CString accel = theApp.accel.GetStrAccel(item.command);
			GetTextExtentPoint32(hDC, accel, accel.GetLength(), &size);
			accelWidth = size.cx;
		} else if (item.type == MenuInline) {			
			KmMenu* inlineMenu = theApp.menus.GetKMenu(A2CT(item.label));
			accelWidth = inlineMenu->GetMaxAccelWidth(hDC);
		}
		if (accelWidth > maxAccelWidth) maxAccelWidth = accelWidth;
	}
	return maxAccelWidth;
}

void KmMenuService::DrawItem(LPDRAWITEMSTRUCT dis)
{
	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_FTYPE | MIIM_STRING;
	
	::GetMenuItemInfo((HMENU)dis->hwndItem, dis->itemID, FALSE, &mi);
	if (mi.fType & MFT_SEPARATOR) {
		RECT rc;
		rc.bottom   = dis->rcItem.bottom;
		rc.left     = dis->rcItem.left;
		rc.right    = dis->rcItem.right;
		rc.top      = dis->rcItem.top + ((rc.bottom-dis->rcItem.top)>>1); // vertical center
		DrawEdge(dis->hDC, &rc, EDGE_ETCHED, BF_TOP);   // draw separator line
		return;
	}

	mi.cch++;
	CAutoPtr<TCHAR> text(new TCHAR[mi.cch]);
	mi.dwTypeData = text;
	::GetMenuItemInfo((HMENU)dis->hwndItem, dis->itemID, FALSE, &mi);	

	TCHAR* pAccel = wcschr(text, _T('\t'));
	if (pAccel) *pAccel = 0;
	/*
	KmMenuItem* item = (KmMenuItem*)dis->itemData;	
	if (!item || item->type == MenuSeparator) {
      RECT rc;
      rc.bottom   = dis->rcItem.bottom;
      rc.left     = dis->rcItem.left;
      rc.right    = dis->rcItem.right;
      rc.top      = dis->rcItem.top + ((rc.bottom-dis->rcItem.top)>>1); // vertical center
      DrawEdge(dis->hDC, &rc, EDGE_ETCHED, BF_TOP);   // draw separator line
      return;
   }

	USES_CONVERSION;
	TCHAR* _label = A2T(item->label); 

	CString pTranslated;
	Translate(_label, pTranslated);	
	//CString pTranslated = (wchar_t*)dis->itemData;*/
	
	// Draw the highlight rectangle
	SetBkMode(dis->hDC, TRANSPARENT);
	if (dis->itemState & ODS_SELECTED) {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
	}
	else {
		FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_MENU));
		SetTextColor(dis->hDC, GetSysColor(COLOR_MENUTEXT));
		SetBkColor(dis->hDC, GetSysColor(COLOR_MENU));
	}

	if (dis->itemState & ODS_GRAYED)
		if (dis->itemState & ODS_SELECTED)
			SetTextColor(dis->hDC, GetSysColor(COLOR_MENU));
		else
			SetTextColor(dis->hDC, GetSysColor(COLOR_GRAYTEXT));
	
	dis->rcItem.left += ::GetSystemMetrics(SM_CXEDGE);
	DRAWBITMAPPROC drawProc;
	if (!mProcList.Lookup((HMENU)dis->hwndItem, drawProc) || !drawProc(dis))
		DrawBitmap(dis);
	dis->rcItem.left += theApp.skin.GetDefWidth() + ::GetSystemMetrics(SM_CXEDGE) + 1;

	DrawText(dis->hDC, text, _tcslen(text), &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);

	if (theApp.preferences.GetBool("kmeleon.display.accelInMenus", TRUE)) {
		CString accel = theApp.accel.GetStrAccel(dis->itemID);
		if (accel.GetLength()) {	
			dis->rcItem.right -= GetSystemMetrics(SM_CXMENUCHECK) + 2;
			DrawText(dis->hDC, accel, accel.GetLength(), &dis->rcItem, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);					
		}
	}
}

/*
		CMenu* pMenu = CMenu::FromHandlePermanent(
			(HMENU)lpDrawItemStruct->hwndItem);
		if (pMenu != NULL)
		{
			KmMenu* kmenu = theApp.menus.GetKMenu(pMenu);
			kmenu->DrawItem(lpDrawItemStruct);
			return; 
		}*/

void KmMenuService::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	SIZE size;
	/*
	KmMenuItem* item = (KmMenuItem*)lpMeasureItemStruct->itemData;
	if (!item || item->type == MenuSeparator) {
		lpMeasureItemStruct->itemWidth = 0;
		lpMeasureItemStruct->itemHeight = GetSystemMetrics(SM_CYMENUSIZE) >> 1;
		return;
	}

	
	USES_CONVERSION;
	CString text = theApp.lang.Translate(A2T(item->label));*/

	if (!mLastActivated)
		return;

	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_FTYPE | MIIM_STRING;
	mLastActivated->GetMenuItemInfo(lpMeasureItemStruct->itemID, &mi);
	if (mi.fType & MFT_SEPARATOR) {
		lpMeasureItemStruct->itemWidth = 0;
		lpMeasureItemStruct->itemHeight = GetSystemMetrics(SM_CYMENUSIZE) >> 1;
		return;
	}

	CAutoPtr<TCHAR> text(new TCHAR[++mi.cch+1]);
	mi.dwTypeData = text;
	mLastActivated->GetMenuItemInfo(lpMeasureItemStruct->itemID, &mi);	

	HWND hWnd = theApp.m_pMostRecentBrowserFrame ? NULL : theApp.m_pMostRecentBrowserFrame->m_hWnd;
	HDC hDC = ::GetWindowDC(hWnd);
	HFONT oldFont = (HFONT)SelectObject(hDC, mMenuFont); 
	GetTextExtentPoint32(hDC, _T("X"), 1, &size);
	int spaceBetween = size.cx;

	TCHAR* pAccel = wcschr(text, _T('\t'));
	if (pAccel) *pAccel = 0;

	//GetTextExtentPoint32(hDC, text, _tcslen(text), &size);
	//if (size.cx > mMaxTextLength) mMaxTextLength = size.cx;
	RECT rcText = {0};
	DrawText(hDC, text, -1, &rcText, DT_SINGLELINE|DT_LEFT|DT_VCENTER|DT_CALCRECT);
	if (rcText.right > mMaxTextLength) mMaxTextLength = rcText.right;
	lpMeasureItemStruct->itemWidth = mMaxTextLength; 

	CString accel = theApp.accel.GetStrAccel(lpMeasureItemStruct->itemID);
	if (accel.GetLength()) {
		GetTextExtentPoint32(hDC, accel, accel.GetLength(), &size);
		if (size.cx > mMaxAccelLength) mMaxAccelLength = size.cx;
	}
	if (mMaxAccelLength > 0) 
		lpMeasureItemStruct->itemWidth += spaceBetween + mMaxAccelLength; 
	
	lpMeasureItemStruct->itemHeight = GetSystemMetrics(SM_CYMENUSIZE);
	int height = theApp.skin.GetDefHeight();
	int width = theApp.skin.GetDefWidth();
	lpMeasureItemStruct->itemWidth += width + ::GetSystemMetrics(SM_CXEDGE) * 2 + 1;
	if (lpMeasureItemStruct->itemHeight < height+2)
		lpMeasureItemStruct->itemHeight = height+2;
	SelectObject(hDC, oldFont);
	ReleaseDC(hWnd, hDC);	
}

/*

void KmMenuService::SetMenu(LPCTSTR menu, KmMenuItem item, long before)
{
   KmMenu* kmenu;
   if (!mMenus.Lookup(menu, kmenu))
      kmenu = CreateMenu(menu);

	kmenu->Invalidate();
	kmenu->AddItem(item, before);
   

	if (!kmenu->menu.m_hMenu)
		return;
	
	
	// CANT MODIFY MENU BECAUSE OF BMPMENU ....
	
	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.dwTypeData = NULL;
	mii.fMask = MIIM_DATA | MIIM_TYPE;
   kmenu->menu.GetMenuItemInfo(i, &mii, TRUE);


	// Delete by label
	if (item.command<1) {
		CString label;
		int count = kmenu->menu.GetMenuItemCount();
		for (int i=0; i<count; i++) {
			kmenu->menu.GetMenuString(i, label, MF_BYPOSISION);
			if (label.Compare(theApp.lang.Translate(A2CT(item.label))) == 0) {
				kmenu->menu.DeleteMenu(i, MF_BYPOSITION);

				return;
			}
		}
	}

	// Delete by command
	if (!item.label[0]) {
	kmenu->menu.RemoveMenu(item.command, MF_BYCOMMAND);
		return;
	}

	if (item.type != MenuSpecial || item.type != MenuPlugin)
		if (!kmenu->menu.ModifyMenu(item.command, MF_BYCOMMAND | MF_STRING, item.command, theApp.lang.Translate(A2CT(item.label))))
      	InsertItem(kmenu->menu, item, -1);
}
*/
