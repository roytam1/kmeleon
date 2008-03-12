/*
*  Copyright (C) 2007 Dorian Boissonnade
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

// Local Includes
#include "stdafx.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"
#include "MozUtils.h"

// Mozilla Includes
#if GECKO_VERSION < 19 // XXX
#include "nsIRegistry.h"
#endif
#include "nsIProfile.h"
#include "nsDirectoryServiceUtils.h"
#include "jsapi.h"
#include "nsIJSContextStack.h"

// Constants
#define kRegistryGlobalPrefsSubtreeString (nsEmbedString(L"global-prefs"))
#define kRegistryShowProfilesAtStartup "start-show-dialog"

#define kRegistryProfileSubtreeString (NS_LITERAL_STRING("Profiles"))
#define kRegistryCurrentProfileString (NS_LITERAL_STRING("CurrentProfile"))
#define kRegistryCreationTimeString (NS_LITERAL_CSTRING("CreationTime"))
#define kRegistryLastModTimeString (NS_LITERAL_CSTRING("LastModTime"))
#define kRegistryDirectoryString (NS_LITERAL_STRING("directory"))
#define kRegistryStartWithLastString (NS_LITERAL_CSTRING("AutoStartWithLast"))


static BOOL WritePrivateProfileInt(
    IN LPCTSTR lpAppName,
    IN LPCTSTR lpKeyName,
    IN __int64 n,
    IN LPCTSTR lpFileName
    )
{
	TCHAR number[35];
	_i64tot(n, number, 10);
	return WritePrivateProfileString(lpAppName, lpKeyName, number, lpFileName);
}


//*****************************************************************************
//***    CProfileMgr: Object Management
//*****************************************************************************

CProfileMgr::CProfileMgr()
{
}

CProfileMgr::~CProfileMgr()
{
}

//*****************************************************************************
//***    CProfileMgr: Public Methods
//*****************************************************************************

#pragma comment(lib, "profdirserviceprovidersa_s.lib")

BOOL CProfileMgr::StartUp(LPCTSTR aProfileName)
{
    nsresult rv;

	rv = NS_NewProfileDirServiceProvider(PR_TRUE, &mProfileProvider);
	NS_ENSURE_SUCCESS(rv, FALSE);

	CString registryDir = GetMozDirectory(NS_APP_APPLICATION_REGISTRY_DIR);
	if (registryDir.IsEmpty()) return FALSE;

	mProfileIniFile = registryDir + _T("\\profiles.ini");
	CFile file;
	if (!file.Open(mProfileIniFile, CFile::modeRead, NULL))
		ImportRegistry();
	else
		file.Close();
   
	UINT profileCount = GetProfileCount();
	if (!profileCount)
	{
        // Make a new default profile
		if (!(CreateProfile(NULL, NULL, _T("default"), mCurrentProfile)))
			return FALSE;

		SetShowDialogOnStart(FALSE);
	}
	else if (aProfileName && GetProfileByName(aProfileName, mCurrentProfile)) {
	}
	else if (GetShowDialogOnStart()) {
		if (!AskUserForProfile(TRUE, mCurrentProfile))
			return FALSE;
	}
	else
		if (!GetDefaultProfile(mCurrentProfile))
			if (!AskUserForProfile(TRUE, mCurrentProfile))
				return FALSE;

	rv = mProfileProvider->Register();
	NS_ENSURE_SUCCESS(rv, FALSE);

	while (true)
	{
		nsCOMPtr<nsILocalFile> rootDir;
		rv = NS_NewLocalFile(CStringToNSString(mCurrentProfile.mRootDir), PR_TRUE, getter_AddRefs(rootDir));
		NS_ENSURE_SUCCESS(rv, FALSE);
		
		nsCOMPtr<nsILocalFile> localDir;
		rv = NS_NewLocalFile(CStringToNSString(mCurrentProfile.mLocalDir), PR_TRUE, getter_AddRefs(localDir));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = mProfileProvider->SetProfileDir(rootDir, localDir);
		if (NS_SUCCEEDED(rv)) break;	
		AfxMessageBox(IDS_PROFILE_LOAD_FAILED, MB_OK|MB_ICONERROR);
		//profiledirprovider bug workaround 
		mProfileProvider->SetProfileDir(nsnull, nsnull);
		if (!AskUserForProfile(TRUE, mCurrentProfile))
			return FALSE;
	}
	
	SetDefaultProfile(mCurrentProfile);
	return TRUE;
}

BOOL CProfileMgr::ShutDownCurrentProfile(BOOL cleanup)
{
	if (mProfileProvider)
	{
		nsCOMPtr<nsIObserverService> obssvc(do_GetService("@mozilla.org/observer-service;1"));
		NS_ASSERTION(obssvc, "No observer service?");
    	if (!obssvc) return FALSE;
		
		static const PRUnichar kShutdownCleanse[] =
			{'s','h','u','t','d','o','w','n','-','c','l','e','a','n','s','e','\0'};
		static const PRUnichar kShutdownPersist[] =
			{'s','h','u','t','d','o','w','n','-','p','e','r','s','i','s','t','\0'};
		
		obssvc->NotifyObservers(nsnull, "profile-change-net-teardown", cleanup ? kShutdownCleanse : kShutdownPersist);
		obssvc->NotifyObservers(nsnull, "profile-change-teardown", cleanup ? kShutdownCleanse : kShutdownPersist);

	    /*// Phase 2c: Now that things are torn down, force JS GC so that things which depend on
		// resources which are about to go away in "profile-before-change" are destroyed first.
		nsCOMPtr<nsIThreadJSContextStack> stack
			(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
		if (stack)
		{
	        JSContext *cx = nsnull;
		    stack->GetSafeJSContext(&cx);
	        if (cx)
			::JS_GC(cx);
		}*/

		//obssvc->NotifyObservers(nsnull, "profile-before-change", kShutdownPersist);
    
		mProfileProvider->Shutdown();
		NS_RELEASE(mProfileProvider);
		return TRUE;
	}

	return FALSE;

   nsresult rv;

   nsCOMPtr<nsIProfile> profileService = 
      do_GetService(NS_PROFILE_CONTRACTID, &rv);
   if (NS_FAILED(rv)) return FALSE;

   rv = profileService->ShutDownCurrentProfile(
           cleanup ? nsIProfile::SHUTDOWN_CLEANSE : nsIProfile::SHUTDOWN_PERSIST);
   if (NS_FAILED(rv)) return FALSE;

   return TRUE;
}

BOOL CProfileMgr::AskUserForProfile(BOOL atStartup, CProfile& aProfile)
{
	CProfilesDlg dialog(this);
	dialog.m_bAskAtStartUp = GetShowDialogOnStart();
	INT_PTR res = dialog.DoModal();

	if (res != IDOK)
		return FALSE;

	SetShowDialogOnStart(dialog.m_bAskAtStartUp);
	GetProfileByName(dialog.m_SelectedProfile, aProfile);
	return TRUE;
}

UINT CProfileMgr::GetProfileCount()
{
	if (mProfileIniFile.IsEmpty())
		return 0;

	UINT profileNumber = 0;
	while (true)
	{
		CProfile profile;
		if (!GetProfileByIndex(profileNumber, profile))
			break;

		profileNumber++;
	}

	return profileNumber;
}

BOOL CProfileMgr::GetProfileList(CProfile*** profileList, UINT* profileCount)
{
	if (mProfileIniFile.IsEmpty())
		return FALSE;

	*profileCount = GetProfileCount();
	if (!*profileCount)
		return TRUE;

	CProfile **pList = new CProfile*[*profileCount];
	for (UINT profileNumber = 0; profileNumber < *profileCount; profileNumber++) 
	{
		pList[profileNumber] = new CProfile;
		if (!GetProfileByIndex(profileNumber, *pList[profileNumber]))
			continue;
	}

	*profileList = pList;

	return TRUE;
}

BOOL CProfileMgr::GetCurrentProfile(CProfile& profile)
{
	if (!mCurrentProfile.mName.GetLength()) 
		return FALSE;
	profile = mCurrentProfile;
	return TRUE;
}

BOOL CProfileMgr::RenameProfile(LPCTSTR oldName, LPCTSTR newName)
{
	CProfile profile;
	if (!GetProfileByName(oldName, profile))
		return FALSE;
	
	profile.mName = newName;
	if (!SetProfileAtIndex(profile.number, profile))
		return FALSE;

	if (mCurrentProfile.mName == oldName)
		mCurrentProfile.mName = newName;

	return TRUE;
}

BOOL CProfileMgr::RemoveProfile(LPCTSTR oldName, BOOL removeDir)
{
	CProfile profile;
	if (!GetProfileByName(oldName, profile))
		return FALSE;

    if (removeDir)
	{
		nsCOMPtr<nsILocalFile> rootDir;
		nsresult rv = NS_NewLocalFile(CStringToNSString(profile.mRootDir), PR_TRUE, getter_AddRefs(rootDir));
		NS_ENSURE_SUCCESS(rv, FALSE);

		// Only way currently to check if the profile is locked.
		nsCOMPtr<nsIFile> lock;
		rv = rootDir->Clone(getter_AddRefs(lock));
		NS_ENSURE_SUCCESS(rv, FALSE);
        lock->AppendNative(nsEmbedCString("parent.lock"));
		
		PRBool exist = PR_FALSE;
		rv = lock->Exists(&exist);
		if (exist == PR_TRUE)
			return FALSE;

		if (profile.mLocalDir != profile.mRootDir)
		{
			nsCOMPtr<nsILocalFile> localDir;
			rv = NS_NewLocalFile(CStringToNSString(profile.mLocalDir), PR_TRUE, getter_AddRefs(localDir));
			NS_ENSURE_SUCCESS(rv, FALSE);

			localDir->Remove(PR_TRUE);
		}

		rootDir->Remove(PR_TRUE);

		/*if (profile.mLocalDir != profile.mRootDir)
			from = profile.mLocalDir + _T("|");
		
		from = profile.mRootDir + _T("||");
		
		TCHAR* lpszFilter = from.GetBuffer(0);
		for (int i=0; lpszFilter[i]; i++)
			if (lpszFilter[i] == _T('|'))
				lpszFilter[i] = _T('\0');
		
		SHFILEOPSTRUCT fop = {0};
		fop.wFunc = FO_DELETE;
		fop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
		fop.pFrom = lpszFilter;
		int result = SHFileOperation(&fop);
		if (result != 0 && result != 0x402)
			return FALSE;

		from.ReleaseBuffer();*/
    }

	UINT profileCount = GetProfileCount();
	for (UINT index = profile.number+1; index < profileCount; index++)
	{
		CProfile profile;
		if (GetProfileByIndex(index, profile))
			SetProfileAtIndex(index-1, profile);
	}

	CProfile emptyProfile;
	SetProfileAtIndex(profileCount-1, emptyProfile);

	return TRUE;
}

static const char kTable[] =
    { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

static void SaltProfileName(CString& aName)
{
    double fpTime;
    LL_L2D(fpTime, PR_Now());

    // use 1e-6, granularity of PR_Now() on the mac is seconds
    srand((uint)(fpTime * 1e-6 + 0.5));

    char salt[10];

    int i;
    for (i = 0; i < 8; ++i)
        salt[i] = kTable[rand() % NS_ARRAY_LENGTH(kTable)];

    salt[8] = '.';
	salt[9] = 0;

	USES_CONVERSION;
    aName.Insert(0, A2T(salt));
}

BOOL CProfileMgr::CreateProfile(LPCTSTR aRootDir,
                           LPCTSTR aLocalDir,
                           LPCTSTR aName,
						   CProfile& aProfile)
{
    if (GetProfileByName(aName, aProfile))
		return FALSE;

	CString rootDir = aRootDir;
    CString dirName = aName;
    if (!aRootDir)
	{
		SaltProfileName(dirName);
		aProfile.mRootDir = GetMozDirectory(NS_APP_USER_PROFILES_ROOT_DIR) + _T("\\") + dirName;
	}

	CString localDir = aLocalDir;
    if (!aLocalDir) {
        if (aRootDir)
            aProfile.mLocalDir = aRootDir;
        else
			aProfile.mLocalDir = GetMozDirectory(NS_APP_USER_PROFILES_LOCAL_ROOT_DIR) + __T("\\") + dirName;
    }

	aProfile.mName = aName;
	aProfile.number = GetProfileCount();
	aProfile.mDefault = FALSE;
	return SetProfileAtIndex(aProfile.number, aProfile);
}

//*****************************************************************************
//***    CProfileMgr: Protected Methods
//*****************************************************************************

BOOL CProfileMgr::SetProfileAtIndex(UINT index, CProfile& profile)
{
	TCHAR number[35];
	_itot(index, number, 10);
	CString iniKey = _T("Profile");
	iniKey += number;
	
	BOOL isRelative = FALSE;
	CString location = profile.mRootDir;
	CString rootDir = GetMozDirectory(NS_APP_USER_PROFILES_ROOT_DIR);
	if (_tcsnicmp(profile.mRootDir, rootDir, rootDir.GetLength()) == 0) {
		location = location.Mid(rootDir.GetLength()+1);
		location.Replace(_T('\\'), _T('/'));
		isRelative = 1;
	}

	BOOL res = WritePrivateProfileString(iniKey, _T("Name"), profile.mName, mProfileIniFile);
	res &= WritePrivateProfileString(iniKey, _T("Path"), location, mProfileIniFile);
	res &= WritePrivateProfileInt(iniKey, _T("IsRelative"), isRelative, mProfileIniFile);
	res &= WritePrivateProfileInt(iniKey, _T("Default"), profile.mDefault, mProfileIniFile);
	return res;
}

BOOL CProfileMgr::GetProfileByIndex(UINT index, CProfile& profile)
{
	TCHAR number[35];
	_itot(index, number, 10);
	CString iniKey = _T("Profile");
	iniKey += number;
	profile.number = index;
	return GetProfileFromKey(iniKey, profile);
}

BOOL CProfileMgr::GetProfileFromKey(LPCTSTR iniKey, CProfile& aProfile)
{
	GetPrivateProfileString(iniKey, _T("Name"), _T(""), aProfile.mName.GetBuffer(100), 100, mProfileIniFile);
	aProfile.mName.ReleaseBuffer();
	if (aProfile.mName.IsEmpty())
		return FALSE;

	aProfile.mDefault = GetPrivateProfileInt(iniKey, _T("Default"), 0, mProfileIniFile);

	CString path;
	GetPrivateProfileString(iniKey, _T("Path"), _T(""), path.GetBuffer(MAX_PATH), MAX_PATH, mProfileIniFile);
	path.ReleaseBuffer();

	nsEmbedCString filePath = CStringToNSCString(path);
	BOOL isRelative = GetPrivateProfileInt(iniKey, _T("IsRelative"), 0, mProfileIniFile);

	if (isRelative)
		path.Replace(_T('/'), _T('\\'));

	aProfile.mRootDir = isRelative ?
		GetMozDirectory(NS_APP_USER_PROFILES_ROOT_DIR) + _T("\\") +  path : 
		path;

	aProfile.mLocalDir = isRelative ?
		GetMozDirectory(NS_APP_USER_PROFILES_LOCAL_ROOT_DIR) + _T("\\") + path : 
		aProfile.mRootDir;

	return TRUE;
}

BOOL CProfileMgr::SetDefaultProfile(CProfile& profile)
{
	CProfile defProf;
	if (GetDefaultProfile(defProf)) {
		defProf.mDefault = FALSE;
		SetProfileAtIndex(defProf.number, defProf);
	}

	profile.mDefault = TRUE;
	return SetProfileAtIndex(profile.number, profile);
}

BOOL CProfileMgr::GetDefaultProfile(CProfile& aProfile)
{
	if (mProfileIniFile.IsEmpty())
		return FALSE;

	UINT profileNumber = 0;
	while (true)
	{
		if (!GetProfileByIndex(profileNumber, aProfile))
			break;

		profileNumber++;
		if (!aProfile.mDefault)
			continue;

		return TRUE;
	}

	return FALSE;
}

BOOL CProfileMgr::GetProfileByName(LPCTSTR aName, CProfile& aProfile)
{
	if (mProfileIniFile.IsEmpty())
		return FALSE;

	UINT profileNumber = 0;
	while (true)
	{
		if (!GetProfileByIndex(profileNumber, aProfile))
			break;
		
		profileNumber++;
		if (aProfile.mName.Compare(aName) != 0)
			continue;

		return TRUE;
	}

	return FALSE;
}

BOOL CProfileMgr::SetShowDialogOnStart(BOOL showIt)
{
    return WritePrivateProfileInt(_T("General"), _T("StartWithLastProfile"), !showIt, mProfileIniFile);
}

BOOL CProfileMgr::GetShowDialogOnStart()
{
    return !GetPrivateProfileInt(_T("General"), _T("StartWithLastProfile"), 1, mProfileIniFile);
}

BOOL CProfileMgr::ImportShowDialogOnStart(PRBool* showIt)
{
#if GECKO_VERSION < 19
    nsresult rv = NS_OK;
        
    *showIt = PR_TRUE;
                
    nsCOMPtr<nsIRegistry> registry(do_CreateInstance(NS_REGISTRY_CONTRACTID, &rv));
    rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    NS_ENSURE_SUCCESS(rv, FALSE);

    nsRegistryKey profilesTreeKey;
    
    rv = registry->GetKey(nsIRegistry::Common, 
                          kRegistryGlobalPrefsSubtreeString.get(), 
                          &profilesTreeKey);

    if (NS_SUCCEEDED(rv)) 
    {
        PRInt32 flagValue;
        rv = registry->GetInt(profilesTreeKey, 
                              kRegistryShowProfilesAtStartup, 
                              &flagValue);
         
        if (NS_SUCCEEDED(rv))
            *showIt = (flagValue != 0);
    }
    
	return NS_SUCCEEDED(rv);        
#else
	return FALSE;
#endif
}

BOOL CProfileMgr::ImportRegistry()
{
#if GECKO_VERSION < 19
	nsCOMPtr <nsIFile> regName;
    nsresult rv = NS_OK;

	NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_FILE, getter_AddRefs(regName));

    nsCOMPtr<nsIRegistry> registry(do_CreateInstance(NS_REGISTRY_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return FALSE;
    rv = registry->Open(regName);
    if (NS_FAILED(rv)) return FALSE;

 // Enumerate all subkeys (immediately) under the given node.
    nsCOMPtr<nsIEnumerator> enumKeys;
    nsRegistryKey profilesTreeKey;

    rv = registry->GetKey(nsIRegistry::Common,
                            kRegistryProfileSubtreeString.get(),
                            &profilesTreeKey);

	if (NS_FAILED(rv)) return FALSE;
    
	// Get the current profile
	nsEmbedString tmpCurrentProfile;
    rv = registry->GetString(profilesTreeKey,
                               kRegistryCurrentProfileString.get(),
                               getter_Copies(tmpCurrentProfile));
	
	// Get the StartWithLastProfile flag
	BOOL startWithLast = FALSE;
    /*PRInt32 tempLong;
    rv = registry->GetInt(profilesTreeKey,
                           kRegistryShowProfilesAtStartup.get(),
                           &tempLong);
	if (NS_SUCCEEDED(rv))
        startWithLast = (BOOL)tempLong;*/

	ImportShowDialogOnStart(&startWithLast);

	CString registryDir = GetMozDirectory(NS_APP_APPLICATION_REGISTRY_DIR);
	if (registryDir.IsEmpty()) return FALSE;

	CString profileIni = registryDir + _T("\\profiles.ini");
	WritePrivateProfileInt(_T("General"), _T("StartWithLastProfile"), startWithLast, profileIni);

	rv = registry->EnumerateSubtrees( profilesTreeKey, getter_AddRefs(enumKeys));
    if (NS_FAILED(rv)) return FALSE;

    rv = enumKeys->First();
    if (NS_FAILED(rv)) return FALSE;

	UINT profileNumber = 0;
	while (NS_OK != enumKeys->IsDone())
    {
        nsCOMPtr<nsISupports> base;

        rv = enumKeys->CurrentItem( getter_AddRefs(base) );
        if (NS_FAILED(rv)) return FALSE;

        // Get specific interface.
        nsCOMPtr <nsIRegistryNode> node;
        nsIID nodeIID = NS_IREGISTRYNODE_IID;

        rv = base->QueryInterface( nodeIID, getter_AddRefs(node));
        if (NS_FAILED(rv)) return FALSE;

		nsEmbedString profile;
        rv = node->GetName(getter_Copies(profile));
        if (NS_FAILED(rv)) continue;

        nsRegistryKey profKey;
        rv = node->GetKey(&profKey);
        if (NS_FAILED(rv)) return rv;

     /*   PRInt64 tmpCreationTime;
        rv = registry->GetLongLong(profKey,
                                   kRegistryCreationTimeString.get(),
                                   &tmpCreationTime);
			
		PRInt64 tmpLastModTime;
        rv = registry->GetLongLong(profKey,
                                   kRegistryLastModTimeString.get(),
                                   &tmpLastModTime);
*/

        nsEmbedString nslocation;
        rv = registry->GetString(profKey,
                                  kRegistryDirectoryString.get(),
                                  getter_Copies(nslocation));
		if (NS_FAILED(rv)) continue;


		TCHAR number[35];
		_itot(profileNumber, number, 10);
		CString iniKey = _T("Profile");
		iniKey += number;
		
		USES_CONVERSION;
		CString location = W2CT(nslocation.get());
		BOOL isRelative = 0;
		if (_tcsnicmp(location, registryDir, registryDir.GetLength()) == 0) {
			location = location.Mid(registryDir.GetLength()+1);
			isRelative = 1;
		}

		if (isRelative)
			location.Replace(_T('\\'), _T('/'));
		if (profile.IsEmpty())
			profile.Assign(L"Unnamed");
		
		WritePrivateProfileString(iniKey, _T("Name"), W2CT(profile.get()), profileIni);
		WritePrivateProfileString(iniKey, _T("Path"), location, profileIni);
		WritePrivateProfileInt(iniKey, _T("IsRelative"), isRelative, profileIni);
		//WritePrivateProfileInt(iniKey, _T("CreationTime"), tmpCreationTime, profileIni);
		//WritePrivateProfileInt(iniKey, _T("LastModTime"), tmpLastModTime, profileIni);
		if (profile.Equals(tmpCurrentProfile))
			WritePrivateProfileInt(iniKey, _T("Default"), 1, profileIni);

		profileNumber++;
        rv = enumKeys->Next();
        if (NS_FAILED(rv)) return rv;
	}
	
	return TRUE;
#else
	return FALSE;
#endif
}
