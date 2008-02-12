/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this Mozilla sample software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __ProfileMgr__
#define __ProfileMgr__

// Mozilla Includes
#include "nsError.h"
#include "profdirserviceprovider/nsProfileDirServiceProvider.h"

// Forward Declarations
class nsIRegistry;

//*****************************************************************************
//***    CProfileMgr
//*****************************************************************************

class CProfile
{
public:
	CString mName;
	UINT number;
	CString mRootDir;
	CString mLocalDir;
	BOOL mDefault;
	CProfile() {}
};

class CProfileMgr
{
  public:
                        CProfileMgr();
    virtual             ~CProfileMgr();
    
	
	UINT				GetProfileCount();
	BOOL				GetCurrentProfile(CProfile& profile);
    BOOL				StartUp(LPCTSTR aProfileName = NULL);
    BOOL                ShutDownCurrentProfile(BOOL cleanup=FALSE);
	BOOL				GetProfileList(CProfile*** profileList, UINT* profileCount);
	BOOL				CreateProfile(LPCTSTR aRootDir, LPCTSTR aLocalDir, LPCTSTR aName, CProfile& aProfile);
    BOOL	            GetShowDialogOnStart();
	BOOL	            SetShowDialogOnStart(BOOL showIt);
	BOOL				GetDefaultProfile(CProfile& aProfile);
	BOOL				GetProfileByName(LPCTSTR aName, CProfile& aProfile);
	BOOL				RenameProfile(LPCTSTR oldName, LPCTSTR newName);
	BOOL				RemoveProfile(LPCTSTR oldName, BOOL removeDir);

  protected:
	CString				mProfileIniFile;
	BOOL				GetProfileByIndex(UINT index, CProfile& profile);
	BOOL				SetProfileAtIndex(UINT index, CProfile& profile);
	BOOL				GetProfileFromKey(LPCTSTR key, CProfile& aProfile);
	BOOL				AskUserForProfile(BOOL atStartup, CProfile& profile);
	BOOL				SetDefaultProfile(CProfile& profile);
	BOOL	            ImportShowDialogOnStart(PRBool* showIt);
	BOOL				ImportRegistry();

	nsProfileDirServiceProvider* mProfileProvider;
	CProfile			mCurrentProfile;
};




#endif
