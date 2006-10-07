/*
*  Copyright (C) 2000 Brian Harris
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

#include "StdAfx.h"
#include <afxtempl.h>

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Plugins.h"
#include "kmeleon_plugin.h"
#include "LangParser.h"
#include "MenuParser.h"
#include "Utils.h"

extern BOOL ParsePluginCommand(char *pszCommand, char** plugin, char **parameter);

int Translate(LPCTSTR originalText, CString& translatedText)
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

void KMenu::RemoveItem(MenuItem& item)
{

	// Deletion by command
	if (!item.label[0]) { 
		POSITION pos = menuDef.GetHeadPosition();
		while (pos) {
			MenuItem item2 = menuDef.GetAt(pos);
			if (item2.command == item.command) {
				menuDef.RemoveAt(pos);
				return;
			}
			menuDef.GetNext(pos);
		}
	}

	// Deletion by name
	if (item.command < 1) {
		POSITION pos = menuDef.GetHeadPosition();
		while (pos) {
			MenuItem item2 = menuDef.GetAt(pos);
			if (strcmp(item2.label, item.label) == 0) {
				menuDef.RemoveAt(pos);
				break;
			}
			menuDef.GetNext(pos);
		}
		return;
	}
}

void KMenu::AddItem(MenuItem& item, long before)
{
	if (item.command<1 || !item.label[0]) {
		RemoveItem(item);
		return;
	}

	if (item.type == MenuString) {
		// There can't be two item with the same id
		POSITION pos = menuDef.GetHeadPosition();
		while (pos) {
			MenuItem* item2 = &menuDef.GetAt(pos);
			if (item2->command == item.command) {
				*item2 = item; // Replace old with new
				return;
			}
			menuDef.GetNext(pos);
		}
	}

   // Custom position 
	if (before != -1) {
		if (before < 100) { // Index position
			POSITION pos = menuDef.GetHeadPosition();
			while (pos && before--)
				menuDef.GetNext(pos);
			if (pos) {
            menuDef.InsertBefore(pos, item);
				return;
			}
		}
		else if (before < 0xffff) { // command position
			POSITION pos = menuDef.GetHeadPosition();
			while (pos) {
				MenuItem item2 = menuDef.GetAt(pos);
				if (item2.command == before) {
					menuDef.InsertBefore(pos, item);
					return;
				}
				menuDef.GetNext(pos);
			}
		}
		else { // label position
			POSITION pos = menuDef.GetHeadPosition();
			while (pos) {
				MenuItem item2 = menuDef.GetAt(pos);
				if (strcmp(item2.label, (char*)before) == 0) {
					menuDef.InsertBefore(pos, item);
					return;
				}
				menuDef.GetNext(pos);
			}
		}
	}

	menuDef.AddTail(item);
}

CMenuParser::CMenuParser()
{
}

CMenuParser::CMenuParser(LPCTSTR filename)
{
	Load(filename);
}

CMenuParser::~CMenuParser()
{
   Destroy();
}

void CMenuParser::Destroy()
{
   POSITION pos = menus2.GetStartPosition();
   KMenu *kmenu;
   CString s;
   while (pos) {
      menus2.GetNextAssoc( pos, s, kmenu);
      if (kmenu)
         delete kmenu;
   }
   menus2.RemoveAll();
}

int CMenuParser::Load(LPCTSTR filename)
{
   SETUP_LOG("Menu");

   mAccelText = theApp.preferences.GetBool("kmeleon.display.accelInMenus", TRUE);

   int retVal = CParser::Load(filename);

   END_LOG();

   return retVal;
}

void CMenuParser::Rebuild(LPCTSTR menu)
{
	KMenu* kmenu;
	if (!menus2.Lookup(menu, kmenu))
		return;

   ClearSeparators(kmenu);
	BOOL mainChanged = FALSE;

	// Rebuild the current menu
	if (kmenu->menu.m_hMenu) {
		ResetMenu(kmenu->menu);
		BuildMenu(kmenu->menu, kmenu->menuDef);
	}

   // Rebuild all menu using this menu 
   POSITION pos = kmenu->dependencies.GetHeadPosition();
	while (pos) {
		KMenu* depMenu = kmenu->dependencies.GetNext(pos);
		if (depMenu->menu.m_hMenu) {
			ResetMenu(depMenu->menu);
			BuildMenu(depMenu->menu, depMenu->menuDef);
		}
		if (GetMenu("Main") == &kmenu->menu)
			mainChanged = true;
	}

	if (mainChanged || (_tcsicmp(menu, _T("Main")) == 0)) {
		CFrameWnd* pBrowserFrame = NULL;
		POSITION pos = theApp.m_FrameWndLst.GetHeadPosition();
		while( pos != NULL ) {
			pBrowserFrame = (CFrameWnd *) theApp.m_FrameWndLst.GetNext(pos);
			if(pBrowserFrame)
				pBrowserFrame->DrawMenuBar();
		}
	}
}

void CMenuParser::SetMenu(LPCTSTR menu, MenuItem item, long before)
{
   KMenu* kmenu;
   if (!menus2.Lookup(menu, kmenu)) {
      kmenu = new KMenu;
      if (!kmenu) return;
      menus2[menu] = kmenu;
   }

	kmenu->AddItem(item, before);

   if (item.command<1) {
      KMenu* iMenu;
      if (menus2.Lookup(menu, iMenu)) {
         POSITION pos = iMenu->dependencies.Find(kmenu);
         if (pos) iMenu->dependencies.RemoveAt(pos);
      }
   }
   else if (item.type == MenuInline) {
      KMenu* iMenu;
      if (!menus2.Lookup(item.label, iMenu)) {
         iMenu = new KMenu;
         if (!iMenu) return;
         menus2[item.label] = iMenu;
      }
      iMenu->dependencies.AddHead(kmenu);
   }
   
/*
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
	*/
}

extern int GetID(char *strID);

int CMenuParser::Parse(char *p)
{
	if (!currentKMenu) {
		// There can only be 3 things outside a menu:
		//   comments, metacommands, and the beginning of a menu block
		char *cb = strchr(p, '{');
		if (cb) {
			*cb = 0;

			p = SkipWhiteSpace(p);
			TrimWhiteSpace(p);

			USES_CONVERSION;
			if (*p == '!') {
				++p;
				opEdit = 1;
			}
			else
				opEdit = 0;

			if (menus2.Lookup(A2CT(p), currentKMenu)) {
				if (!opEdit) {
					currentKMenu->menuDef.RemoveAll();
					if (currentKMenu->menu.m_hMenu)
						ResetMenu(currentKMenu->menu);
					LOG_1("Reset Menu %s", p);
				}
			}
			else {
				currentKMenu = new KMenu;
				menus2[A2CT(p)] = currentKMenu;
				LOG_1("Created Menu %s", p);
			}
		}
	}
	else {
		p = SkipWhiteSpace(p);
		TrimWhiteSpace(p);

		if (p[0] == '}') {
			if (opEdit)
				ClearSeparators(currentKMenu);
			currentKMenu = NULL;
			LOG_1("Ended Menu", 0);
		}
		else {
			MenuItem item;
			item.command = 1;
			long before = -1;

			char *posInfo = strchr(p, _T('|'));
			if (posInfo) {
				*(posInfo++) = 0;
				posInfo = SkipWhiteSpace(posInfo);
				TrimWhiteSpace(p);
				before = GetID(posInfo);
				if (!before)
					before = (long)posInfo;
			}

			switch (p[0]) {
			case ':': // Popup Menu
				item.type = MenuPopup;
				item.SetLabel(++p);
				LOG_1("Added popup menu %s.", item.label);
				break;

			case '@': // Special Menu
				item.type = MenuSpecial;
				item.SetLabel(++p);
				LOG_1("Added special menu %s.", item.label);
				break;

			case '!': // Inline menu
				item.type = MenuInline;
				item.SetLabel(++p);
				KMenu* iMenu;
				if (!menus2.Lookup(item.label, iMenu)) {
					iMenu = new KMenu;
					if (!iMenu) return 0;
					menus2[item.label] = iMenu;
				}
				iMenu->dependencies.AddHead(currentKMenu);
				LOG_1("Added menu group %s.", item.label);
				break;

			case '-': 
				if (p[1]) { // Deletion
					item.command = -1;
					item.SetLabel(++p);
				} else { // Separator
					item.type = MenuSeparator;
				}
				LOG_0("Added separator.");
				break;

			default: { // Normal item
				item.type = MenuPlugin;
				TranslateTabs(p);

				char *pszCmd = strrchr(p, '=');
				if (pszCmd) {
					item.type = MenuString;
					*pszCmd = 0;
					pszCmd = SkipWhiteSpace(pszCmd+1);
					TrimWhiteSpace(p);

					int val;
					val = GetID(pszCmd);
					if (!val)
						val = atoi(pszCmd);

					if (val) {
						item.command = val;
						item.SetLabel(p);
						LOG_2("Added menu item %s with command %s", p, pszCmd);
					}
					else {
						LOG_ERROR_1("Incorrect command value: %s", pszCmd);
						return 0;
					}
				}
				else {
					char *plugin, *parameter;
					if (ParsePluginCommand(p, &plugin, &parameter))
					{
						if (*parameter) {
							item.type = MenuString;
							char* sep = strchr(parameter, ',');
							if (sep) {
								*sep = 0;
								char * pszLabel = SkipWhiteSpace(sep+1);
								TrimWhiteSpace(parameter);

								int val;
								if (theApp.plugins.SendMessage(plugin, "* MenuParser", "DoAccel", (long)parameter, (long)&val)	&& val)	{
									LOG_3("Added menu item %s with command %s(%s) [deprecated]", pszLabel, plugin, parameter);
									item.command = val;
									item.SetLabel(pszLabel);
									break;
								}
								*sep = ',';
							}
						} 

						LOG_2("Added call to plugin %s with parameter '%s'", plugin, parameter);

						*(parameter-1) = '(';
						*(parameter+strlen(parameter)) = ')'; //Bleh

						item.type = MenuPlugin;
						item.SetLabel(p);
					}
					else {
						LOG_ERROR_1("I don't know what to do with %s", p);
						return 0;
					}
				}
				}
			}
			currentKMenu->AddItem(item, before);
		}
	}

	return 1;
}

CMenu *CMenuParser::GetMenu(LPCTSTR menuName)
{
	KMenu* kmenu;
	if (!menus2.Lookup(menuName, kmenu))
		return NULL;

	if (kmenu->menu.m_hMenu)
		return &kmenu->menu;

	if (_tcsstr(menuName, _T("Main")))
		kmenu->menu.CreateMenu();
	else
		kmenu->menu.CreatePopupMenu();

	BuildMenu(kmenu->menu, kmenu->menuDef);

	return &kmenu->menu;
}

void CMenuParser::ClearSeparators(KMenu* menu) 
{
	CList<MenuItem, MenuItem&> *def = &menu->menuDef;

	// Remove top separators
	while (!def->IsEmpty()
		&& def->GetHead().type == MenuSeparator)
		def->RemoveHead();

	// Remove bottom separators
	while (!def->IsEmpty()
		&& def->GetHead().type == MenuSeparator)
		def->RemoveTail();

	// Remove separators following another one.
	BOOL sep = FALSE;
	POSITION pos = def->GetHeadPosition();
	while (pos)
	{  
		POSITION oldpos = pos;
		BOOL oldsep = sep;

		MenuItem item = def->GetNext(pos);
		sep = item.type == MenuSeparator;
		if (sep && oldsep)
			def->RemoveAt(oldpos);
	}
}

void CMenuParser::InsertItem(CMenu &menu, MenuItem item, int before)
{

	USES_CONVERSION;
	CMenu *popup;
	LPCTSTR label;

	switch (item.type) {
		case MenuPopup: // Popup Menu
			label = A2CT(item.label);
			popup = GetMenu(label);
			if (popup) {
				menu.InsertMenu(before, MF_POPUP | MF_STRING, (UINT)popup->m_hMenu, theApp.lang.Translate(label));
				//LOG_1("Added popup %s", label);
			}
			else
				LOG_ERROR_1("Popup %s not found!", label);
			break;

		case MenuSpecial: // Special Menu

			if (strcmpi(item.label, "ToolBars") == 0) 
				theApp.m_toolbarControlsMenu = menu.GetSafeHmenu();
#ifdef INTERNAL_SIDEBAR
			if (strcmpi(item.label, "SideBars") == 0) 
				theApp.m_sidebarControlsMenu = menu.GetSafeHmenu();
#endif
			break;

		case MenuInline: { // Inline menu
			label = A2CT(item.label);
			KMenu* inlineMenu = NULL;
			if (menus2.Lookup(label, inlineMenu))
				BuildMenu(menu, inlineMenu->menuDef, before);
			else
				LOG_ERROR_1("Popup %s not found!", label);
			break;
		}

		case MenuSeparator: // Separator
			menu.InsertMenu(before, MF_SEPARATOR);
			//LOG_1("Added Separator", 0);
			break;

		case MenuPlugin: {
			char *plugin, *parameter;
			ParsePluginCommand(item.label, &plugin, &parameter);

			if (theApp.plugins.SendMessage(plugin, "* MenuParser", "DoMenu", (long)menu.GetSafeHmenu(), (long)parameter)) {
				//LOG_2("Called plugin %s with parameter %s", plugin, parameter);
			}
			else {
				LOG_ERROR_1( "Plugin %s has no menu", plugin);
			}
			break;
		}

		case MenuString: {
			label = A2CT(item.label); 

			CString pTranslated;
			Translate(label, pTranslated);

			if (mAccelText) {
				CString accel = theApp.accel.GetStrAccel(item.command);
				if (accel.GetLength())
					pTranslated += _T("\t") + accel;
			}

			menu.InsertMenu(before, MF_STRING, item.command, pTranslated);
			//LOG_2("Added menu item %s with command %d", label, item.command);
			break;
		}

		default: 
			label = A2CT(item.label); 
			LOG_ERROR_1("Undefined menu %s", label);
	}
}

BOOL CMenuParser::BuildMenu(CMenu &menu, CList<MenuItem, MenuItem&> &menuDef, int before)
{
	POSITION pos = menuDef.GetHeadPosition();
	for (int i=0;i < menuDef.GetCount();i++)
		InsertItem(menu, menuDef.GetNext(pos), before);
	return TRUE;
}

void CMenuParser::ResetMenu(CMenu& menu)
{
	// XXX Crappy hack until we get rid of this crappy plugin
	theApp.plugins.SendMessage("bmpmenu", "* MenuParser", "UnSetOwnerDrawn", (long)menu.m_hMenu, 0);
	while (menu.GetMenuItemCount())
		menu.RemoveMenu(0, MF_BYPOSITION);
}

int CMenuParser::GetOffset(CMenu *menu){
   int offset;
   if (!menuOffsets.Lookup(menu, offset))
      return 0;

   return offset;
}

void CMenuParser::SetCheck(UINT id, BOOL checked)
{
   POSITION pos = menus2.GetStartPosition();
   KMenu *m;
   CString s;
   while (pos) {
      menus2.GetNextAssoc(pos, s, m);
      CMenu *menu = GetMenu(s);
      if (menu)
         menu->CheckMenuItem( id, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED) );
   }
}
