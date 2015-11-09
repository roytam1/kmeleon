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
#include "KmFileLocProvider.h"
#include "kmeleon_plugin.h"
// Mozilla Includes
#include "nsIToolkitProfileService.h"

// Constants
#define kRegistryGlobalPrefsSubtreeString (nsString(L"global-prefs"))
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

extern XRE_LockProfileDirectoryType XRE_LockProfileDirectory;

//*****************************************************************************
//***    CProfileMgr: Object Management
//*****************************************************************************

CProfileMgr::CProfileMgr()
{
	mProfileLock = nullptr;
}

CProfileMgr::~CProfileMgr()
{
}

//*****************************************************************************
//***    CProfileMgr: Public Methods
//*****************************************************************************

//#pragma comment(lib, "profdirserviceprovidersa_s.lib")

BOOL CProfileMgr::StartUp(KmFileLocProvider* aProvider, LPCTSTR aProfileName)
{
	nsresult rv;
	/*
	nsCOMPtr<nsIToolkitProfileService> profService =  
	do_GetService(NS_PROFILESERVICE_CONTRACTID, &rv);

	return TRUE;*/
	mDirProvider = aProvider;
	CString registryDir = mDirProvider->GetProductDirectory();//GetMozDirectory(NS_APP_APPLICATION_REGISTRY_DIR);
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

	//rv = mProfileProvider->Register();
	//NS_ENSURE_SUCCESS(rv, FALSE);

	while (true)
	{
		nsCOMPtr<nsIFile> rootDir;
		rv = NS_NewLocalFile(CStringToNSString(mCurrentProfile.mRootDir), PR_TRUE, getter_AddRefs(rootDir));
		NS_ENSURE_SUCCESS(rv, FALSE);

		/*nsCOMPtr<nsIFile> localDir;
		rv = NS_NewLocalFile(CStringToNSString(mCurrentProfile.mLocalDir), PR_TRUE, getter_AddRefs(localDir));
		NS_ENSURE_SUCCESS(rv, FALSE);*/

		if (mDirProvider->SetProfile(CStringToNSString(mCurrentProfile.mRootDir), CStringToNSString(mCurrentProfile.mLocalDir)))
		{
			rv = XRE_LockProfileDirectory(rootDir, &mProfileLock);
			if (NS_SUCCEEDED(rv)) break;	
		}

		//rv = mProfileProvider->SetProfileDir(rootDir, localDir);		
		AfxMessageBox(IDS_PROFILE_LOAD_FAILED, MB_OK|MB_ICONERROR);
		//profiledirprovider bug workaround 
		//mProfileProvider->SetProfileDir(nullptr, nullptr);
		if (!AskUserForProfile(TRUE, mCurrentProfile))
			return FALSE;
	}

	SetDefaultProfile(mCurrentProfile);
	return TRUE;
}

BOOL CProfileMgr::ShutDownCurrentProfile(BOOL cleanup)
{
	NS_IF_RELEASE(mProfileLock);
	return FALSE;
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
		nsCOMPtr<nsIFile> rootDir;
		nsresult rv = NS_NewLocalFile(CStringToNSString(profile.mRootDir), PR_TRUE, getter_AddRefs(rootDir));
		NS_ENSURE_SUCCESS(rv, FALSE);

		// Only way currently to check if the profile is locked.
		nsCOMPtr<nsIFile> lock;
		rv = rootDir->Clone(getter_AddRefs(lock));
		NS_ENSURE_SUCCESS(rv, FALSE);
		lock->AppendNative(nsCString("parent.lock"));

		bool exist = PR_FALSE;
		rv = lock->Remove(false);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (profile.mLocalDir != profile.mRootDir)
		{
			nsCOMPtr<nsIFile> localDir;
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
	long fpTime;
	//LL_L2D(fpTime, PR_Now());
	fpTime = time(0);
	// use 1e-6, granularity of PR_Now() on the mac is seconds
	srand((UINT)(fpTime * 1e-6 + 0.5));

	char salt[10];

	int i;
	for (i = 0; i < 8; ++i)
		salt[i] = kTable[rand() % NS_ARRAY_LENGTH(kTable)];

	salt[8] = '.';
	salt[9] = 0;

	USES_CONVERSION;
	aName.Insert(0, A2T(salt));
}

BOOL CProfileMgr::InitProfile(CProfile* aProfile)
{
	nsresult rv;
	nsCOMPtr<nsIFile> rootDir;
	NS_NewLocalFile(CStringToNSString(aProfile->mRootDir), true, getter_AddRefs(rootDir));
	NS_ENSURE_TRUE(rootDir, FALSE);

	nsCOMPtr<nsIFile> localDir;
	NS_NewLocalFile(CStringToNSString(aProfile->mLocalDir), true, getter_AddRefs(localDir));
	NS_ENSURE_TRUE(localDir, FALSE);

	bool exists;
	rv = rootDir->Exists(&exists);
	NS_ENSURE_SUCCESS(rv, FALSE);

	if (exists) {
		rv = rootDir->IsDirectory(&exists);
		NS_ENSURE_SUCCESS(rv, FALSE);

		if (!exists)
			return FALSE;
	}
	else {
		nsCOMPtr<nsIFile> profileDefaultsDir;
		nsCOMPtr<nsIFile> profileDirParent;
		nsAutoString profileDirName;

		rv = rootDir->GetParent(getter_AddRefs(profileDirParent));
		NS_ENSURE_SUCCESS(rv, FALSE);

		rv = rootDir->GetLeafName(profileDirName);
		NS_ENSURE_SUCCESS(rv, FALSE);

		CString appFolder = theApp.GetFolder(AppFolder) + _T("\\defaults\\profile");
		rv = NS_NewLocalFile(CStringToNSString(appFolder), true, getter_AddRefs(profileDefaultsDir));
		//rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_50_DIR, getter_AddRefs(profileDefaultsDir));

		if (NS_SUCCEEDED(rv) && profileDefaultsDir)
			rv = profileDefaultsDir->CopyTo(profileDirParent, profileDirName);
		if (NS_FAILED(rv) || !profileDefaultsDir) {
			// if copying failed, lets just ensure that the profile directory exists.
			rv = rootDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
			NS_ENSURE_SUCCESS(rv, FALSE);
		}
		rootDir->SetPermissions(0700);
	}

	rv = localDir->Exists(&exists);
	NS_ENSURE_SUCCESS(rv, FALSE);

	if (!exists) {
		rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
		NS_ENSURE_SUCCESS(rv, FALSE);
	}

	return TRUE;
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
		aProfile.mRootDir = mDirProvider->GetProductDirectory() + _T("\\") + dirName;
	}

	CString localDir = aLocalDir;
	if (!aLocalDir) {
		if (aRootDir)
			aProfile.mLocalDir = aRootDir;
		else
			aProfile.mLocalDir = mDirProvider->GetProductDirectory(false) + __T("\\") + dirName;
	}

	aProfile.mName = aName;
	aProfile.number = GetProfileCount();
	aProfile.mDefault = FALSE;

	InitProfile(&aProfile);
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
	CString rootDir = mDirProvider->GetProductDirectory();
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

	nsCString filePath = CStringToNSCString(path);
	BOOL isRelative = GetPrivateProfileInt(iniKey, _T("IsRelative"), 0, mProfileIniFile);

	if (isRelative)
		path.Replace(_T('/'), _T('\\'));

	aProfile.mRootDir = isRelative ?
		mDirProvider->GetProductDirectory() + _T("\\") +  path : 
	path;

	aProfile.mLocalDir = isRelative ?
		mDirProvider->GetProductDirectory(true) + _T("\\") + path : 
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
	return FALSE;
}

BOOL CProfileMgr::ImportRegistry()
{
	return FALSE;
}
/*
nsresult
	CProfileMgr::GetProfileDefaultsDir(nsIFile* *aResult)
{
	NS_ASSERTION(mGREDir, "nsXREDirProvider not initialized.");
	NS_PRECONDITION(aResult, "Null out-param");

	nsresult rv;
	nsCOMPtr<nsIFile> defaultsDir;

	rv = GetAppDir()->Clone(getter_AddRefs(defaultsDir));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = defaultsDir->AppendNative(NS_LITERAL_CSTRING("defaults"));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = defaultsDir->AppendNative(NS_LITERAL_CSTRING("profile"));
	NS_ENSURE_SUCCESS(rv, rv);

	NS_ADDREF(*aResult = defaultsDir);
	return NS_OK;
}*/