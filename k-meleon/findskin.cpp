
// look for "filename" first in the settingsDir,
// then skinsDir\CurrentSkin,
// then skinsDir\default
// copy the full path into "szSkinFile"
// if it's not anywhere, return settingsDir

void FindSkinFile( char *szSkinFile, char *filename ) 
{
   WIN32_FIND_DATA FindData;
   HANDLE hFile;
   
   char szTmpSkinDir[MAX_PATH];
   char szTmpSkinName[MAX_PATH];
   char szTmpSkinFile[MAX_PATH] = "";

   if (!szSkinFile || !filename || !*filename)
      return;

   // check for the file in the settingsDir
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szTmpSkinFile, (char*)"");
   if (*szTmpSkinFile) {
      strcat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }
   

   // it wasn't in settingsDir, check the current skin   

   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsDir", szTmpSkinDir, (char*)"");
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.skinsCurrent", szTmpSkinName, (char*)"");

   if (*szTmpSkinDir) {
      while (*szTmpSkinName) {
	 int len = strlen(szTmpSkinDir);
	 if (szTmpSkinDir[len-1] != '\\')
	    strcat(szTmpSkinDir, "\\");

	 len = strlen(szTmpSkinName);
	 if (szTmpSkinName[len-1] != '\\')
	    strcat(szTmpSkinName, "\\");

	 strcpy(szTmpSkinFile, szTmpSkinDir);
	 strcat(szTmpSkinFile, szTmpSkinName);
	 strcat(szTmpSkinFile, filename);

	 hFile = FindFirstFile(szTmpSkinFile, &FindData);
	 if(hFile != INVALID_HANDLE_VALUE) {   
	    FindClose(hFile);
	    strcpy( szSkinFile, szTmpSkinFile );
	    return;
	 }

	 len = strlen(szTmpSkinName);
	 if (len > 1)
            szTmpSkinName[len-2] = 0;
      }

      // it wasn't in the current skin directory, check the default

      strcpy(szTmpSkinFile, szTmpSkinDir);
      strcat(szTmpSkinFile, "default\\");
      strcat(szTmpSkinFile, filename);

      hFile = FindFirstFile(szTmpSkinFile, &FindData);
      if(hFile != INVALID_HANDLE_VALUE) {   
         FindClose(hFile);
         strcpy( szSkinFile, szTmpSkinFile );
         return;
      }
   }

   // it wasn't anywhere, return the path to the settingsDir, in case the file is being created
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szSkinFile, (char*)"");
   if (! *szSkinFile)      // no settingsDir, bad
      strcpy(szSkinFile, filename);
   else
      strcat(szSkinFile, filename);
}
