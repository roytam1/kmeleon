
// look for "filename" first in the settingsDir,
// then skinsDir\CurrentSkin,
// then skinsDir\default
// copy the full path into "szSkinFile"
// if it's not anywhere, return settingsDir


#include <tchar.h>


BOOL _FindSkinFile( TCHAR* szSkinFile, TCHAR* szSkinDir, TCHAR* filename)
{
	WIN32_FIND_DATA FindData;
    HANDLE hFile;
	TCHAR szTmpSkinName[MAX_PATH];
    TCHAR szTmpSkinFile[MAX_PATH];

	kPlugin.kFuncs->GetPreference(PREF_TSTRING, "kmeleon.general.skinsCurrent", szTmpSkinName, (TCHAR*)_T(""));

	if (*szSkinDir) {
		while (*szTmpSkinName) {
			int len = _tcslen(szSkinDir);
			if (szSkinDir[len-1] != _T('\\'))
				_tcscat(szSkinDir, _T("\\"));

			len = _tcslen(szTmpSkinName);
			if (szTmpSkinName[len-1] != '\\')
				_tcscat(szTmpSkinName, _T("\\"));

			_tcscpy(szTmpSkinFile, szSkinDir);
			_tcscat(szTmpSkinFile, szTmpSkinName);
			_tcscat(szTmpSkinFile, filename);

			hFile = FindFirstFile(szTmpSkinFile, &FindData);
			if(hFile != INVALID_HANDLE_VALUE) {   
				FindClose(hFile);
				_tcscpy( szSkinFile, szTmpSkinFile );
				return true;
			}

			len = _tcslen(szTmpSkinName);
			if (len > 1)
				szTmpSkinName[len-2] = 0;
		}

		// it wasn't in the current skin directory, check the default

		_tcscpy(szTmpSkinFile, szSkinDir);
		_tcscat(szTmpSkinFile, _T("default\\"));
		_tcscat(szTmpSkinFile, filename);

		hFile = FindFirstFile(szTmpSkinFile, &FindData);
		if(hFile != INVALID_HANDLE_VALUE) {   
			FindClose(hFile);
			_tcscpy( szSkinFile, szTmpSkinFile );
			return true;
		}
	}
	return false;
}

//To really do
void FindSkinFile( TCHAR *szSkinFile, TCHAR *filename ) 
{
   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   
   TCHAR szTmpSkinDir[MAX_PATH];
   TCHAR szTmpSkinFile[MAX_PATH] = _T("");

   if (!szSkinFile || !filename || !*filename)
      return;

   // check for the file in the settingsDir
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, "kmeleon.general.settingsDir", szTmpSkinDir, (TCHAR*)_T(""));
   if (*szTmpSkinDir) {
	  _tcscpy(szTmpSkinFile, szTmpSkinDir);
      _tcscat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         _tcscpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }
   
   // it wasn't in settingsDir, check the current skin   
   
   _tcscat(szTmpSkinDir, _T("Skins\\"));
   if (_FindSkinFile(szSkinFile, szTmpSkinDir, filename))
	   return;
   
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, "kmeleon.general.skinsDir", szTmpSkinDir, (TCHAR*)_T(""));
   if (_FindSkinFile(szSkinFile, szTmpSkinDir, filename))
	   return;

   // it wasn't anywhere, return the path to the settingsDir, in case the file is being created
   kPlugin.kFuncs->GetPreference(PREF_TSTRING, "kmeleon.general.settingsDir", szSkinFile, (TCHAR*)_T(""));
   if (! *szSkinFile)      // no settingsDir, bad
      _tcscpy(szSkinFile, filename);
   else
      _tcscat(szSkinFile, filename);
}
