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

#ifndef __KMMENU_H__
#define __KMMENU_H__

enum KmMenuType 
{
	MenuString = 0,
	MenuPopup = 1,
	MenuInline = 2,
	MenuPlugin = 3,
	MenuSeparator = 4,
	MenuSpecial = 5
};

struct KmMenuItem
{
	KmMenuType type;
	char id[80];
	char label[80];
	int command;
	int groupid;

	void SetLabel(const char* psz) {
		strncpy(label, psz, 80);
		label[79] = 0;
	}

	void SetID(const char* psz) {
		strncpy(id, psz, 80);
		id[79] = 0;
	}
};

class KmMenu {
public:
	KmMenu(BOOL popup) : mPopup(popup), mInvalid(TRUE) {};
	~KmMenu(void) {};

	void RemoveItem(KmMenuItem& item);
	void AddItem(KmMenuItem& item, long before = -1);
	BOOL Build();
	void Invalidate();

	void Empty() {
		mMenuDef.RemoveAll();
		Reset();
	}

	HMENU GetHMenu() {
		return mMenu.m_hMenu;
	}

	CMenu* GetMenu() { 
		if (mInvalid || !mMenu.m_hMenu) Build();
		return &mMenu;
	}

protected:

	void Reset();
	BOOL Build(CMenu &menu, int before);
	BOOL IsEmpty() { return (mMenuDef.GetCount() == 0); }

	CMenu mMenu;
	CList<KmMenuItem, KmMenuItem&> mMenuDef;
	CList<KmMenu*, KmMenu*> mDependencies;
	BOOL mInvalid;
	BOOL mPopup;
};

class KmMenuService
{
public:

	KmMenuService() : mLastActivated(NULL) {

	}

	KmMenu* CreateMenu(LPCTSTR name) {
		KmMenu* kmenu = new KmMenu(_tcsstr(name, _T("Main")) == NULL);
		mMenus[name] = kmenu;
		return kmenu;
	}

	KmMenu* GetKMenu(LPCTSTR name) {
		KmMenu* kmenu;
		if (!mMenus.Lookup(name, kmenu))
			return NULL;
		return kmenu;
	}

	KmMenu* GetKMenu(CMenu* cmenu) {
		/*MENUINFO mi;
		mi.cbSize = sizeof(mi);
		mi.fMask = MIM_MENUDATA;
		cmenu->GetMenuInfo(&mi);

		return (KMenu*)mi.dwMenuData;*/

		KmMenu *kmenu;
		CString s;
		POSITION pos = mMenus.GetStartPosition();
		while (pos) {
			mMenus.GetNextAssoc( pos, s, kmenu);
			if (kmenu->GetHMenu() == cmenu->GetSafeHmenu())
				return kmenu;
		}
		return NULL;
	}

	CMenu* GetMenu(LPCTSTR name) {
		KmMenu* kmenu;
		if (!mMenus.Lookup(name, kmenu))
			return NULL;

		return kmenu->GetMenu();
	}

	BOOL DeleteMenu(LPCTSTR name) {
		KmMenu* kmenu;
		if (!mMenus.Lookup(name, kmenu))
			return FALSE;
		mMenus.RemoveKey(name);
		delete kmenu;
	}

	void SetMenu(LPCTSTR menu, KmMenuItem &item, long before = -1)
	{
		KmMenu* kmenu;
		if (!mMenus.Lookup(menu, kmenu))
			kmenu = CreateMenu(menu);

		kmenu->Invalidate();
		kmenu->AddItem(item, before);
	}

	void RebuildAll() 
	{
		KmMenu* kmenu;
		CString s;
		POSITION pos = mMenus.GetStartPosition();
		while (pos) {
			mMenus.GetNextAssoc(pos, s, kmenu);
			kmenu->Invalidate();
			if (s == _T("Main")) kmenu->GetMenu(); // To force the rebuild
		}
	}

	BOOL Rebuild(LPCTSTR menu) 
	{
		KmMenu* kmenu;
		if (!mMenus.Lookup(menu, kmenu))
			return FALSE;

		kmenu->Invalidate();
		kmenu->Build();
		return TRUE;
	}


	void SetCheck(UINT id, BOOL checked)
	{
		KmMenu *m;
		CString s;
		POSITION pos = mMenus.GetStartPosition();
		while (pos) {
			mMenus.GetNextAssoc(pos, s, m);
			CMenu *menu = m->GetMenu();
			if (menu)
				menu->CheckMenuItem( id, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED) );
		}
	}

	void Destroy() {
		POSITION pos = mMenus.GetStartPosition();
		KmMenu *kmenu;
		CString s;
		while (pos) {
			mMenus.GetNextAssoc( pos, s, kmenu);
			if (kmenu) delete kmenu;
		}
		mMenus.RemoveAll();
	}

	KmMenu* Activate(CMenu* menu) {
		KmMenu* kmenu = GetKMenu(menu);
		if (kmenu) kmenu->GetMenu();
		mLastActivated = kmenu;
		return kmenu;
	}

	BOOL MenuCommand(UINT id) {
		if (!mLastActivated)
			return FALSE;

		mLastActivated = NULL;
		return FALSE;
	}

	~KmMenuService() {
		Destroy();
	}

protected:
	CMap<CString, LPCTSTR, KmMenu*, KmMenu*> mMenus;
	KmMenu* mLastActivated;
};


#endif
