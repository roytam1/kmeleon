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
#include "winuser.h"
#include <afxtempl.h>

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Plugins.h"
#include "kmeleon_plugin.h"
#include "LangParser.h"
#include "MenuParser.h"
#include "Utils.h"

extern BOOL ParsePluginCommand(char *pszCommand, char** plugin, char **parameter);


CMenuParser::CMenuParser()
{
	menusService = &theApp.menus;
}

CMenuParser::CMenuParser(LPCTSTR filename)
{
	Load(filename);
}

CMenuParser::~CMenuParser()
{
}

int CMenuParser::Load(LPCTSTR filename)
{
   SETUP_LOG("Menu");

   mAccelText = theApp.preferences.GetBool("kmeleon.display.accelInMenus", TRUE);

	currentKMenu = NULL;
   int retVal = CParser::Load(filename);

   END_LOG();

   return retVal;
}

extern int GetID(const char *strID);

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

			currentKMenu = menusService->GetKMenu(A2CT(p));
			if (currentKMenu) {
				if (!opEdit) {
					currentKMenu->Empty();
					LOG_1("Reset Menu %s", p);
				}
			}
			else {
				currentKMenu = menusService->CreateMenu(A2CT(p));
				LOG_1("Created Menu %s", p);
			}
		}
	}
	else {
		p = SkipWhiteSpace(p);
		TrimWhiteSpace(p);

		if (p[0] == '}') {
			currentKMenu = NULL;
			LOG_1("Ended Menu", 0);
		}
		else {
			KmMenuItem item;
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
				LOG_1("Added menu group %s.", item.label);
				break;

			case '-': 
				if (p[1]) { // Deletion
					item.command = GetID(item.label);
					if (!item.command) {
						item.command = -1;
						item.SetLabel(++p);
					}
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