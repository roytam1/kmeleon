/*
*  Copyright (C) 2000 Brian Harris
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
// ie_favorites.cpp : Plugin that supports ie-style boomarks
//

#include "stdafx.h"
//#include "resource.h"

#pragma warning( disable : 4786 ) // C4786 bitches about the std::map template name expanding beyond 255 characters
#include <map>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#define MAX_FAVORITES 255

/*
// MFC handles this for us (how nice)
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  switch (ul_reason_for_call)
  {
		case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}
*/

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

kmeleonPlugin kPlugin = {
  KMEL_PLUGIN_VER,
  "IE Favorites Plugin",
  Init,
  Create,
  Config,
  Quit,
  DoMenu,
  DoRebar
};

CMenu m_menuFavorites;

UINT nConfigCommand;
UINT nAddCommand;
UINT nEditCommand;
UINT nFirstFavoriteCommand;

TCHAR szPath[MAX_PATH];

int Init(){
  nConfigCommand = kPlugin.GetCommandIDs(1);
  nAddCommand = kPlugin.GetCommandIDs(1);
  nEditCommand = kPlugin.GetCommandIDs(1);

  nFirstFavoriteCommand = kPlugin.GetCommandIDs(MAX_FAVORITES);

	TCHAR           sz[MAX_PATH];
	HKEY            hKey;
	DWORD           dwSize;

	// find out from the registry where the favorites are located.
	if(RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"), &hKey) != ERROR_SUCCESS)
	{
		TRACE0("Favorites folder not found\n");
		//return;
    strcpy(sz, _T("c:\\windows\\favorites"));
	}
	dwSize = sizeof(sz);
	RegQueryValueEx(hKey, _T("Favorites"), NULL, NULL, (LPBYTE)sz, &dwSize);
	ExpandEnvironmentStrings(sz, szPath, MAX_PATH);
	RegCloseKey(hKey);

  return true;
}

typedef std::map<HWND, void *> WndProcMap;
WndProcMap KMeleonWndProcs;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
	KMeleonWndProcs[parent] = (void *) GetWindowLong(parent, GWL_WNDPROC);
	SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
  MessageBox(parent, "This plugin brought to you by the letter M", "IE Favorites plugin", 0);
}

void Quit(){
  m_menuFavorites.DestroyMenu();
}

static CStringArray m_astrFavoriteURLs;
static CMap<CString, LPCTSTR, int, int>	m_URLIcons;

static int        m_iInternetShortcutIcon;
static HIMAGELIST m_himSystem;
static CSize      m_SysImageSize;
static int        m_iFolderIcon;

int BuildFavoritesMenu(LPCTSTR pszPath, int nStartPos, CMenu* pMenu)
{
	CString         strPath(pszPath);
	CString         strPath2;
	CString         str;
	WIN32_FIND_DATA wfd;
	HANDLE          h;
	int             nPos;
	int             nEndPos;
	int             nNewEndPos;
	int             nLastDir;
	TCHAR           buf[MAX_PATH];
	CStringArray    astrFavorites;
	CStringArray    astrDirs;
	CMenu*          pSubMenu;

	// make sure there's a trailing backslash
	if(strPath[strPath.GetLength() - 1] != _T('\\'))
		strPath += _T('\\');
	strPath2 = strPath;
	strPath += _T("*.*");

	// now scan the directory, first for .URL files and then for subdirectories
	// that may also contain .URL files
	h = FindFirstFile(strPath, &wfd);
	if(h != INVALID_HANDLE_VALUE)
	{
		nEndPos = nStartPos;
		do
		{
			if((wfd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM))==0)
			{
				str = wfd.cFileName;
				if(str.Right(4).CompareNoCase(_T(".url")) == 0)
				{
					// an .URL file is formatted just like an .INI file, so we can
					// use GetPrivateProfileString() to get the information we want
					::GetPrivateProfileString(_T("InternetShortcut"), _T("URL"),
											  _T(""), buf, MAX_PATH,
											  strPath2 + str);
					str = str.Left(str.GetLength() - 4);

					// scan through the array and perform an insertion sort
					// to make sure the menu ends up in alphabetic order
          /*
					for(nPos = nStartPos ; nPos < nEndPos ; ++nPos)	{
						if(str.CompareNoCase(astrFavorites[nPos]) < 0)
							break;
					}
          */
          nPos = m_astrFavoriteURLs.GetSize();
					astrFavorites.InsertAt(nPos, str);
					m_astrFavoriteURLs.InsertAt(nPos, buf);

					TCHAR ext[_MAX_EXT];
					_tsplitpath (buf, NULL, NULL, NULL, ext);

					int iIcon;
					if (!m_URLIcons.Lookup (ext, iIcon))
					{
						// Retrieve icon file
						SHFILEINFO sfi;
						if (::SHGetFileInfo (strPath2 + wfd.cFileName, 0, &sfi, sizeof(SHFILEINFO), 
							SHGFI_SMALLICON | SHGFI_SYSICONINDEX) &&
							sfi.iIcon >= 0)
						{
							m_URLIcons.SetAt (ext, sfi.iIcon);

							if (m_iInternetShortcutIcon == -1)
							{
								m_iInternetShortcutIcon = sfi.iIcon;
							}
						}
					}

					++nEndPos;
				}
			}
		} while(FindNextFile(h, &wfd));
		FindClose(h);
		// Now add these items to the menu
		for(nPos = nStartPos ; nPos < nEndPos ; ++nPos)	{
			pMenu->AppendMenu(MF_STRING | MF_ENABLED, nFirstFavoriteCommand + nPos, astrFavorites[nPos]);
		}

		// now that we've got all the .URL files, check the subdirectories for more
		nLastDir = 0;
		h = FindFirstFile(strPath, &wfd);
		ASSERT(h != INVALID_HANDLE_VALUE);
		do
		{
			if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// ignore the current and parent directory entries
				if(lstrcmp(wfd.cFileName, _T(".")) == 0 || lstrcmp(wfd.cFileName, _T("..")) == 0)
					continue;

        /*
				for(nPos = 0 ; nPos < nLastDir ; ++nPos)
				{
					if(astrDirs[nPos].CompareNoCase(wfd.cFileName) > 0)
						break;
				}*/
				pSubMenu = new CMenu;
				pSubMenu->CreatePopupMenu();

				// call this function recursively.
				nNewEndPos = BuildFavoritesMenu(strPath2 + wfd.cFileName, nEndPos, pSubMenu);
				if(nNewEndPos != nEndPos)	{
					// only intert a submenu if there are in fact .URL files in the subdirectory
					nEndPos = nNewEndPos;
          //pMenu->AppendMenu(MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)pSubMenu->m_hMenu, wfd.cFileName);
					pMenu->InsertMenu(nFirstFavoriteCommand + nStartPos, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT)pSubMenu->m_hMenu, wfd.cFileName);
					pSubMenu->Detach();
					astrDirs.InsertAt(nPos, wfd.cFileName);
					++nLastDir;
				}
				delete pSubMenu;
			}
		} while(FindNextFile(h, &wfd));
		FindClose(h);
	}
	return nEndPos;
}


void DoMenu(HMENU menu, char *param){

  if (stricmp(param, _T("Config")) == 0){
    AppendMenu(menu, MF_STRING, nConfigCommand, "&Config");
    return;
  }
  if (stricmp(param, _T("Add")) == 0){
    AppendMenu(menu, MF_STRING, nAddCommand, "&Add Favorite");
    return;
  }
  if (stricmp(param, _T("Edit")) == 0){
    AppendMenu(menu, MF_STRING, nEditCommand, "&Edit Favorites");
    return;
  }
	m_menuFavorites.Attach(menu);

	BuildFavoritesMenu(szPath, 0, &m_menuFavorites);

	SHFILEINFO sfi;
	m_himSystem = (HIMAGELIST)SHGetFileInfo( szPath,
                                       0,
                                       &sfi, 
                                       sizeof(SHFILEINFO), 
                                       SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if (m_himSystem != NULL) {
		int cx, cy;

		::ImageList_GetIconSize (m_himSystem, &cx, &cy);
		m_SysImageSize = CSize (cx, cy);

		m_iFolderIcon = sfi.iIcon;
	}
}

void DoRebar(HWND rebarWnd){
/*
  TODO: Add a favorites ReBar Band
*/
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
  WndProcMap::iterator WndProcIterator;
  if (message == WM_COMMAND){
    WORD command = LOWORD(wParam);
    if (command == nConfigCommand){
      Config(NULL);
      return true;
    }
    if (command == nAddCommand){
      kmeleonDocInfo *dInfo = kPlugin.GetDocInfo(hWnd);

      CString filename = szPath;
      filename += _T('\\');
      filename += dInfo->title;
      filename += _T(".url");
      ::WritePrivateProfileString(_T("InternetShortcut"), _T("URL"), dInfo->url, filename);

      UINT nPos = m_astrFavoriteURLs.GetSize();
      m_astrFavoriteURLs.InsertAt(nPos, dInfo->url);

      m_menuFavorites.AppendMenu(MF_STRING, nFirstFavoriteCommand+nPos, dInfo->title);

      DrawMenuBar(hWnd);

      return true;
    }
    if (command == nEditCommand){
      ShellExecute(hWnd, "explore", szPath, NULL, szPath, SW_SHOWNORMAL);
      return true;
    }
    if (command >= nFirstFavoriteCommand && command < (nFirstFavoriteCommand + MAX_FAVORITES)){
      kPlugin.NavigateTo((char *)(LPCTSTR)m_astrFavoriteURLs.GetAt(command - nFirstFavoriteCommand), 0);
      return true;
    }
  }else if (message == WM_COMMAND){
    WndProcIterator = KMeleonWndProcs.find(hWnd); 
    KMeleonWndProcs.erase(WndProcIterator);
    return 0;
  }
  WndProcIterator = KMeleonWndProcs.find(hWnd);

  if (WndProcIterator != KMeleonWndProcs.end()){
    return CallWindowProc((WNDPROC)WndProcIterator->second, hWnd, message, wParam, lParam);
  }
  return 0;
}


// so it doesn't munge the function name
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
  return &kPlugin;
}

}
