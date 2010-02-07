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
					menu.InsertMenu(before, MF_POPUP | MF_STRING, (UINT)popup->m_hMenu, theApp.lang.Translate(label));
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
					theApp.m_pMostRecentBrowserFrame->DrawSHBackMenu(menu.GetSafeHmenu());
				else if (strcmpi(item.label+1, "SHistoryForward") == 0) 
					theApp.m_pMostRecentBrowserFrame->DrawSHForwardMenu(menu.GetSafeHmenu());
				else if (strcmpi(item.label+1, "SHistory") == 0) 
					theApp.m_pMostRecentBrowserFrame->DrawSHMenu(menu.GetSafeHmenu());
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

			case MenuSeparator: // Separator
				if (wasSeparator || (i == mMenuDef.GetCount()-1)) break;
				menu.InsertMenu(before, MF_SEPARATOR);
				wasSeparator = TRUE;
				//LOG_1("Added Separator", 0);
				break;

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

				menu.InsertMenu(before, MF_STRING, item.command, pTranslated);
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
	// XXX Crappy hack until we get rid of this crappy plugin
	theApp.plugins.SendMessage("bmpmenu", "* MenuParser", "UnSetOwnerDrawn", (long)mMenu.m_hMenu, 0);
	while (mMenu.GetMenuItemCount())
		mMenu.RemoveMenu(0, MF_BYPOSITION);
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
