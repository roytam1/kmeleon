/*
*  Copyright (C) 2008 Dorian Boissonnade
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
#include "MfcEmbed.h"
#include "KmCommand.h"
#include "kmeleon_plugin.h"

UINT KmCmdService::RegisterCommand(LPCTSTR name, LPCTSTR desc, LPCTSTR icon)
{
	UINT id = AllocateId();
	mCommandList[name] = KmCommand(id, desc);
	if (icon) theApp.skin.mImages->AddIcon(icon, 0, 0, id);
	return id;
}

CString KmCmdService::GetDescription(LPCTSTR command)
{
	KmCommand kcommand;
	if (!mCommandList.Lookup(command, kcommand))
		return _T("");
	
	UINT id = kcommand.id;
	if (!IsPluginCommand(id)) {
		CString str;
		str.LoadString(id);
		return str;
	}

	if (kcommand.GetDesc().GetLength())
		return theApp.lang.Translate(kcommand.GetDesc());
	
	USES_CONVERSION;
	char *desc = NULL;
	char *plugin, *parameter;
	char *aCmd = strdup(T2CA(command));
	if (ParseCommand(aCmd, &plugin, &parameter))
		theApp.plugins.SendMessage(plugin, "* GetID", "GetCmd", (long)id, (long)desc);
	free(aCmd);
	return desc?A2T(desc):_T("");
}

UINT KmCmdService::GetList(kmeleonCommand* cmdList, UINT size, BOOL def)
{
	UINT num = def ? GetDefCount() : GetCount();
	if (!cmdList) return num;

	USES_CONVERSION;
	CCmdMap::CPair* pCurVal = mCommandList.PGetFirstAssoc();

	UINT i = 0;
	while (pCurVal != NULL)
	{
		cmdList[i].id = pCurVal->value.id;
		WideCharToMultiByte(CP_UTF8, 0, pCurVal->key, -1, cmdList[i].cmd, sizeof(cmdList[i].cmd), NULL, NULL);
		//if (!IsPluginCommand(pCurVal->value.id)) {
			CString str = pCurVal->value.GetDesc();
			if (!str.GetLength())
				str.LoadString(pCurVal->value.id);
			WideCharToMultiByte(CP_UTF8, 0, str, -1, cmdList[i].desc, sizeof(cmdList[i].desc), NULL, NULL);
		/*}
		else {
			char *plugin, *parameter;
			if (ParseCommand(aCmd, &plugin, &parameter))
				theApp.plugins.SendMessage(plugin, "*", "GetCmds", (long)parameter, (long)&cmdList[i]);
		}*/

		ASSERT(i<=num);
		if (++i > num) break;
		if (--size == 0) break;
		pCurVal = mCommandList.PGetNextAssoc(pCurVal);
	}
    
	return i;
}

bool KmCmdService::GetCommand(LPCTSTR command, KmCommand& kcommand)
{
	return mCommandList.Lookup(command, kcommand);
}

UINT KmCmdService::GetId(LPCTSTR command)
{
	UINT id = 0;
	if (defineMap.Lookup(command, id))
		return id;

	KmCommand kcommand;
	if (mCommandList.Lookup(command, kcommand))
		return kcommand.id;

	if (command[0] == _T('@')) { //Menu
		return RegisterCommand(command);
	}

	char *plugin, *parameter;
	USES_CONVERSION;
	
	LPCSTR tmpCmd = T2CA(command);
	char* aCmd = (char*)alloca(strlen(tmpCmd)+1);
	strcpy(aCmd, tmpCmd);
	if (ParseCommand(aCmd, &plugin, &parameter)) {
		theApp.plugins.SendMessage(plugin, "* GetID", "DoAccel", (long) parameter, (long)&id);
		return id;
	}
	
	id = atoi(aCmd);
	return id;
}

void KmCmdService::InitializeDefineMap()
{
	#include "defineMap.cpp"
}

void KmCmdService::InitDefaultCmd()
{
	#define ADD_DEFCMD(command, id) mCommandList[_T(#command)] = KmCommand(id);

	mCommandList[_T("navBack")] = KmCommand(ID_NAV_BACK);
	mCommandList[_T("navForward")] = KmCommand(ID_NAV_FORWARD);
	mCommandList[_T("navHome")] = KmCommand(ID_NAV_HOME);
	mCommandList[_T("navStop")] = KmCommand(ID_NAV_STOP);
	mCommandList[_T("navReload")] = KmCommand(ID_NAV_RELOAD);
	mCommandList[_T("navForceReload")] = KmCommand(ID_NAV_FORCE_RELOAD);
	mCommandList[_T("navSearch")] = KmCommand(ID_NAV_SEARCH);
	mCommandList[_T("navGo")] = KmCommand(ID_NAV_GO);
	mCommandList[_T("navOffline")] = KmCommand(ID_OFFLINE);
	ADD_DEFCMD(navDisableJS, ID_DISABLE_JS);
	ADD_DEFCMD(navEnableJS, ID_ENABLE_JS);
	ADD_DEFCMD(navToggleJS, ID_TOGGLE_JS);
	
	mCommandList[_T("viewSource")] = KmCommand(ID_VIEW_SOURCE);
	mCommandList[_T("viewToolbar")] = KmCommand(ID_VIEW_TOOLBAR);
	mCommandList[_T("viewStatus")] = KmCommand(ID_VIEW_STATUS_BAR);
	mCommandList[_T("viewPageInfo")] = KmCommand(ID_VIEW_PAGE_INFO);
	mCommandList[_T("viewFrameInfo")] = KmCommand(ID_VIEW_FRAME_INFO);
	ADD_DEFCMD(viewImage, ID_VIEW_IMAGE);
	ADD_DEFCMD(viewFrameSource, ID_VIEW_FRAME_SOURCE);

	mCommandList[_T("saveAs")] = KmCommand(ID_FILE_SAVE_AS);
	mCommandList[_T("saveFrameAs")] = KmCommand(ID_FILE_SAVE_FRAME_AS);
	ADD_DEFCMD(saveImageAs, ID_SAVE_IMAGE_AS);
	ADD_DEFCMD(saveLinkAs, ID_SAVE_LINK_AS);

	mCommandList[_T("fileOpen")] = KmCommand(ID_FILE_OPEN);
	mCommandList[_T("filePrint")] = KmCommand(ID_FILE_PRINT);
	mCommandList[_T("filePrintPreview")] = KmCommand(ID_FILE_PRINTPREVIEW);
	mCommandList[_T("filePrintSetup")] = KmCommand(ID_FILE_PRINTSETUP);

	mCommandList[_T("editCut")] = KmCommand(ID_EDIT_CUT);
	mCommandList[_T("editCopy")] = KmCommand(ID_EDIT_COPY);
	mCommandList[_T("editPaste")] = KmCommand(ID_EDIT_PASTE);
	mCommandList[_T("editUndo")] = KmCommand(ID_EDIT_UNDO);
	mCommandList[_T("editRedo")] = KmCommand(ID_EDIT_REDO);
	mCommandList[_T("editClear")] = KmCommand(ID_EDIT_CLEAR);
	mCommandList[_T("editSelectAll")] = KmCommand(ID_EDIT_SELECT_ALL);
	mCommandList[_T("editSelectNone")] = KmCommand(ID_EDIT_SELECT_NONE);
	mCommandList[_T("editFind")] = KmCommand(ID_EDIT_FIND);
	mCommandList[_T("editFindNext")] = KmCommand(ID_EDIT_FINDNEXT);
	mCommandList[_T("editFindPrev")] = KmCommand(ID_EDIT_FINDPREV);
	ADD_DEFCMD(editFindMatchCase, ID_MATCH_CASE)
	ADD_DEFCMD(editFindHighlight, ID_HIGHLIGHT)
	ADD_DEFCMD(editCopyLinkLocation, ID_COPY_LINK_LOCATION);
	ADD_DEFCMD(editCopyImageLocation, ID_COPY_IMAGE_LOCATION);
	ADD_DEFCMD(editCopyImage, ID_COPY_IMAGE_CONTENT);
	ADD_DEFCMD(editSelectUrl, ID_SELECT_URL);	
	ADD_DEFCMD(pageFontIncrease, ID_FONT_INCREASE);
	ADD_DEFCMD(pageFontDecrease, ID_FONT_DECREASE);
	ADD_DEFCMD(pageZoomIncrease, ID_FULLZOOM_INCREASE);
	ADD_DEFCMD(pageZoomDecrease, ID_FULLZOOM_DECREASE);
	ADD_DEFCMD(pageEnableJS, ID_PAGE_ENABLE_JS);
	ADD_DEFCMD(pageDisableJS, ID_PAGE_DISABLE_JS);
	ADD_DEFCMD(pageToggleJS, ID_PAGE_TOGGLE_JS);

	ADD_DEFCMD(appExit, ID_APP_EXIT);
	ADD_DEFCMD(appAbout, ID_APP_ABOUT);
	ADD_DEFCMD(appRestart, ID_APP_RESTART);
	ADD_DEFCMD(windowClose, ID_FILE_CLOSE);
	ADD_DEFCMD(windowNew, ID_NEW_BROWSER);
	ADD_DEFCMD(windowNext, ID_WINDOW_NEXT);
	ADD_DEFCMD(windowPrev, ID_WINDOW_PREV);
	ADD_DEFCMD(windowMaximize, ID_MAXIMIZE_WINDOW);
	ADD_DEFCMD(windowMinimize, ID_MINIMIZE_WINDOW);
	ADD_DEFCMD(windowRestore, ID_RESTORE_WINDOW);
	ADD_DEFCMD(windowToggle, ID_TOGGLE_WINDOW);

	ADD_DEFCMD(tabNew, ID_NEW_TAB);
	ADD_DEFCMD(tabClose, ID_CLOSE_TAB);
	ADD_DEFCMD(tabCloseAll, ID_CLOSE_ALLTAB);
	ADD_DEFCMD(tabCloseAllOther, ID_CLOSE_ALLOTHERTAB);
	ADD_DEFCMD(tabNext, ID_TAB_NEXT);
	ADD_DEFCMD(tabPrev, ID_TAB_PREV);
	ADD_DEFCMD(tabLast, ID_TAB_LAST);

	ADD_DEFCMD(openPrefs, ID_PREFERENCES);
	ADD_DEFCMD(openCookies, ID_COOKIES_VIEWER);
	ADD_DEFCMD(openPasswords, ID_PASSWORDS_VIEWER);
	ADD_DEFCMD(openCookiesPerm, ID_COOKIE_PERM);
	ADD_DEFCMD(openImagesPerm, ID_IMAGE_PERM);
	ADD_DEFCMD(openPopupsPerm, ID_POPUP_PERM);
	ADD_DEFCMD(openManageProfiles, ID_MANAGE_PROFILES);
	
	ADD_DEFCMD(openLink, ID_OPEN_LINK);
	ADD_DEFCMD(openFrame, ID_OPEN_FRAME);
	ADD_DEFCMD(openFrameInBackground, ID_OPEN_FRAME_IN_BACKGROUND);
	ADD_DEFCMD(openLinkInNewWindow, ID_OPEN_LINK_IN_NEW_WINDOW);
	ADD_DEFCMD(openLinkInBackground, ID_OPEN_LINK_IN_BACKGROUND);
	ADD_DEFCMD(openFrameInNewWindow, ID_OPEN_FRAME_IN_NEW_WINDOW);
	ADD_DEFCMD(openLinkInNewTab, ID_OPEN_LINK_IN_NEW_TAB);
	ADD_DEFCMD(openLinkInBackgroundTab, ID_OPEN_LINK_IN_BACKGROUNDTAB);
	ADD_DEFCMD(openFrameInNewTab, ID_OPEN_FRAME_IN_NEW_TAB);
	ADD_DEFCMD(openFrameInBackgroundTab, ID_OPEN_FRAME_IN_BACKGROUNDTAB);

	ADD_DEFCMD(goHome, ID_LINK_KMELEON_HOME);
	ADD_DEFCMD(goForum, ID_LINK_KMELEON_FORUM);
	ADD_DEFCMD(goFAQ, ID_LINK_KMELEON_FAQ);
	ADD_DEFCMD(goManual, ID_LINK_KMELEON_MANUAL);
	ADD_DEFCMD(goAboutPlugins, ID_LINK_ABOUT_PLUGINS);

	ADD_DEFCMD(toolbarsLock, ID_TOOLBARS_LOCK);
	mDefCount = mCommandList.GetCount();
}