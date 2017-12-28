/*
*  Copyright (C) 2013 Dorian Boissonnade
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
*
*
*/

#include "stdafx.h"
#include "KmAppInfo.h"
#include "KMeleonConst.h"
#include "MfcEmbed.h"
#include "nsAppShellCID.h"
#include "nsIAppShellService.h"
#include "nsXULAppAPI.h"
#include "nsIPrefService.h"

NS_IMPL_ISUPPORTS(KmAppInfo, nsIXULAppInfo, nsIXULRuntime, nsIAppStartup, nsIAppStartup)

const unsigned char mozilla_buildid[] =
{
    BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
    BUILD_MONTH_CH0, BUILD_MONTH_CH1,
    BUILD_DAY_CH0, BUILD_DAY_CH1,
    '\0'
};


/* readonly attribute ACString vendor; */
NS_IMETHODIMP KmAppInfo::GetVendor(nsACString & aVendor)
{
	aVendor = "";
	return NS_OK;
}

/* readonly attribute ACString name; */
NS_IMETHODIMP KmAppInfo::GetName(nsACString & aName)
{
	aName = "K-Meleon";
	return NS_OK;
}

/* readonly attribute ACString ID; */
NS_IMETHODIMP KmAppInfo::GetID(nsACString & aID)
{
	bool ff = false;
	nsCOMPtr<nsIPrefService> m_prefservice = do_GetService(NS_PREFSERVICE_CONTRACTID);
	if (m_prefservice) {
		nsCOMPtr<nsIPrefBranch> m_prefs;
		m_prefservice->GetBranch("", getter_AddRefs(m_prefs));
		if (m_prefs) {
			nsresult rv = m_prefs->GetBoolPref("kmeleon.install_firefox_extension", &ff);
		}
	}
	aID = !ff ? NS_STRINGIFY(KMELEON_UUID) : "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}";
	return NS_OK;
}

/* readonly attribute ACString version; */
NS_IMETHODIMP KmAppInfo::GetVersion(nsACString & aVersion)
{
#if 0
	bool ff = false;
	nsCOMPtr<nsIPrefService> m_prefservice = do_GetService(NS_PREFSERVICE_CONTRACTID);
	if (m_prefservice) {
		nsCOMPtr<nsIPrefBranch> m_prefs;
		m_prefservice->GetBranch("", getter_AddRefs(m_prefs));
		if (m_prefs) {
			nsresult rv = m_prefs->GetBoolPref("kmeleon.install_firefox_extension", &ff);
		}
	}
	aVersion = !ff ? NS_STRINGIFY(KMELEON_UVERSION) : MOZILLA_VERSION;
#else
	aVersion = NS_STRINGIFY(KMELEON_UVERSION);
#endif
	return NS_OK;
}

/* readonly attribute ACString appBuildID; */
NS_IMETHODIMP KmAppInfo::GetAppBuildID(nsACString & aAppBuildID)
{
	aAppBuildID = NS_STRINGIFY(KMELEON_BUILDID);
	return NS_OK;
}

NS_IMETHODIMP KmAppInfo::GetProcessID(uint32_t *aProcessID)
{
	aProcessID = 0;
	return NS_OK;
}

/* readonly attribute ACString platformVersion; */
NS_IMETHODIMP KmAppInfo::GetPlatformVersion(nsACString & aPlatformVersion)
{
#if 0
	aPlatformVersion = NS_STRINGIFY(MOZILLA_VERSION_U);
#else
	aPlatformVersion = NS_STRINGIFY(KMELEON_UVERSION);
#endif
	return NS_OK;
}

/* readonly attribute ACString platformBuildID; */
NS_IMETHODIMP KmAppInfo::GetPlatformBuildID(nsACString & aPlatformBuildID)
{
	nsCString string;
	nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
	nsCOMPtr<nsIPrefBranch> prefs;
	if (!prefService) goto _pbid_fallback;
	prefService->GetBranch("", getter_AddRefs(prefs));

	if (!prefs || !NS_SUCCEEDED(prefs->GetCharPref("platform.buildid", getter_Copies(string))))
		goto _pbid_fallback;
	if (string.Length() < 8)
		goto _pbid_fallback;

	string.Cut(9, 0); // remove everything after yyyymmdd
	aPlatformBuildID = string;
	return NS_OK;

_pbid_fallback:
//	aPlatformBuildID = NS_STRINGIFY(MOZILLA_BUILDID);
	// for fallback
	aPlatformBuildID.AssignASCII((char*)&mozilla_buildid[0]);

	return NS_OK;
}

/* readonly attribute ACString UAName; */
NS_IMETHODIMP KmAppInfo::GetUAName(nsACString & aUAName)
{
	aUAName = "K-Meleon";
	return NS_OK;
}


// CRAP 

/* readonly attribute boolean inSafeMode; */
NS_IMETHODIMP KmAppInfo::GetInSafeMode(bool *aInSafeMode)
{
	*aInSafeMode = false;
	return NS_OK;
}

/* attribute boolean logConsoleErrors; */
NS_IMETHODIMP KmAppInfo::GetLogConsoleErrors(bool *aLogConsoleErrors)
{
	*aLogConsoleErrors = true;
	return NS_OK;
}
NS_IMETHODIMP KmAppInfo::SetLogConsoleErrors(bool aLogConsoleErrors)
{
	return NS_OK;
}

/* readonly attribute AUTF8String OS; */
NS_IMETHODIMP KmAppInfo::GetOS(nsACString & aOS)
{
	aOS = NS_STRINGIFY("WINNT");
	return NS_OK;
}

/* readonly attribute AUTF8String XPCOMABI; */
NS_IMETHODIMP KmAppInfo::GetXPCOMABI(nsACString & aXPCOMABI)
{
	aXPCOMABI = "x86-msvc";
	return NS_OK;
}

/* readonly attribute AUTF8String widgetToolkit; */
NS_IMETHODIMP KmAppInfo::GetWidgetToolkit(nsACString & aWidgetToolkit)
{
	aWidgetToolkit = NS_STRINGIFY("windows");
	return NS_OK;
}

/* readonly attribute unsigned long processType; */
NS_IMETHODIMP KmAppInfo::GetProcessType(uint32_t *aProcessType)
{
	NS_ENSURE_ARG_POINTER(aProcessType);
	*aProcessType = XRE_GetProcessType();
	return NS_OK;
}

#include "nsINIParser.h"
#define NS_LINEBREAK "\015\012"
#define FILE_COMPATIBILITY_INFO NS_LITERAL_CSTRING("compatibility.ini")
/* void invalidateCachesOnRestart (); */
NS_IMETHODIMP KmAppInfo::InvalidateCachesOnRestart()
{
/*
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DIR_STARTUP, 
                                       getter_AddRefs(file));
  if (NS_FAILED(rv))
    return rv;
  if (!file)
    return NS_ERROR_NOT_AVAILABLE;
  
  file->AppendNative(FILE_COMPATIBILITY_INFO);

  nsINIParser parser;
  rv = parser.Init(file);
  if (NS_FAILED(rv)) {
    // This fails if compatibility.ini is not there, so we'll
    // flush the caches on the next restart anyways.
    return NS_OK;
  }
  
  nsAutoCString buf;
  rv = parser.GetString("Compatibility", "InvalidateCaches", buf);
  
  if (NS_FAILED(rv)) {
    PRFileDesc *fd = nullptr;
    file->OpenNSPRFileDesc(PR_RDWR | PR_APPEND, 0600, &fd);
    if (!fd) {
      NS_ERROR("could not create output stream");
      return NS_ERROR_NOT_AVAILABLE;
    }
    static const char kInvalidationHeader[] = NS_LINEBREAK "InvalidateCaches=1" NS_LINEBREAK;
    PR_Write(fd, kInvalidationHeader, sizeof(kInvalidationHeader) - 1);
    PR_Close(fd);
  }*/
	return NS_OK;
}

/* void ensureContentProcess (); */
NS_IMETHODIMP KmAppInfo::EnsureContentProcess()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute PRTime replacedLockTime; */
NS_IMETHODIMP KmAppInfo::GetReplacedLockTime(PRTime *aReplacedLockTime)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute DOMString lastRunCrashID; */
/*NS_IMETHODIMP KmAppInfo::GetLastRunCrashID(nsAString & aLastRunCrashID)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}*/

NS_IMETHODIMP KmAppInfo::GetBrowserTabsRemoteAutostart(bool *aBrowserTabsRemote)
{
	*aBrowserTabsRemote = false;
	return NS_OK;
}

/* readonly attribute boolean accessibilityEnabled; */
NS_IMETHODIMP KmAppInfo::GetAccessibilityEnabled(bool *aAccessibilityEnabled)
{
	*aAccessibilityEnabled = false;
    return NS_OK;
}

NS_IMETHODIMP KmAppInfo::GetDefaultUpdateChannel(nsACString & aDefaultUpdateChannel)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP KmAppInfo::GetDistributionID(nsACString & aDistributionID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP KmAppInfo::GetKeyboardMayHaveIME(bool *ime)
{
	*ime = true;
    return NS_OK;
}

NS_IMETHODIMP KmAppInfo::GetIsOfficial(bool *)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean isReleaseBuild; */
NS_IMETHODIMP KmAppInfo::GetIsReleaseBuild(bool *aIsReleaseBuild)
{
#ifdef _DEBUG
	*aIsReleaseBuild = false;
#else
	*aIsReleaseBuild = true;
#endif
	return NS_OK;
}

/* readonly attribute boolean isOfficialBranding; */
NS_IMETHODIMP KmAppInfo::GetIsOfficialBranding(bool *aIsOfficialBranding)
{
	*aIsOfficialBranding = true;
	return NS_OK;
}

/* void createHiddenWindow (); */
NS_IMETHODIMP KmAppInfo::CreateHiddenWindow()
{
	nsCOMPtr<nsIAppShellService> appShellService(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
	NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);
	return appShellService->CreateHiddenWindow();
}

/* void destroyHiddenWindow (); */
NS_IMETHODIMP KmAppInfo::DestroyHiddenWindow()
{
	nsCOMPtr<nsIAppShellService> appShellService(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
	NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);
	return appShellService->DestroyHiddenWindow();
}

/* void run (); */
NS_IMETHODIMP KmAppInfo::Run()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* void enterLastWindowClosingSurvivalArea (); */
NS_IMETHODIMP KmAppInfo::EnterLastWindowClosingSurvivalArea()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* void exitLastWindowClosingSurvivalArea (); */
NS_IMETHODIMP KmAppInfo::ExitLastWindowClosingSurvivalArea()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean automaticSafeModeNecessary; */
NS_IMETHODIMP KmAppInfo::GetAutomaticSafeModeNecessary(bool *aAutomaticSafeModeNecessary)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* void restartInSafeMode (in uint32_t aQuitMode); */
NS_IMETHODIMP KmAppInfo::RestartInSafeMode(uint32_t aQuitMode)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* bool trackStartupCrashBegin (); */
NS_IMETHODIMP KmAppInfo::TrackStartupCrashBegin(bool *_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* void trackStartupCrashEnd (); */
NS_IMETHODIMP KmAppInfo::TrackStartupCrashEnd()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* void quit (in uint32_t aMode); */
NS_IMETHODIMP KmAppInfo::Quit(uint32_t aMode)
{
	uint32_t ferocity = (aMode & 0xF);
	if (ferocity == eAttemptQuit || ferocity == eForceQuit) {
		PostQuitMessage(0);
		if (aMode & eRestart) 
			((CMfcEmbedApp*)::AfxGetApp())->SetRestart(TRUE);
		return NS_OK;
	}
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean shuttingDown; */
NS_IMETHODIMP KmAppInfo::GetShuttingDown(bool *aShuttingDown)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean startingUp; */
NS_IMETHODIMP KmAppInfo::GetStartingUp(bool *aStartingUp)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void doneStartingUp (); */
NS_IMETHODIMP KmAppInfo::DoneStartingUp()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean restarting; */
NS_IMETHODIMP KmAppInfo::GetRestarting(bool *aRestarting)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean wasRestarted; */
NS_IMETHODIMP KmAppInfo::GetWasRestarted(bool *aWasRestarted)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean restartingTouchEnvironment; */
/*NS_IMETHODIMP KmAppInfo::GetRestartingTouchEnvironment(bool *aRestartingTouchEnvironment)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}*/

/* [implicit_jscontext] jsval getStartupInfo (); */
NS_IMETHODIMP KmAppInfo::GetStartupInfo(JSContext* cx, JS::MutableHandleValue _retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;	
}

/* attribute boolean interrupted; */
NS_IMETHODIMP KmAppInfo::GetInterrupted(bool *aInterrupted)
{
	*aInterrupted = mInterrupted;
	return NS_OK;
}

NS_IMETHODIMP KmAppInfo::SetInterrupted(bool aInterrupted)
{
	mInterrupted = aInterrupted;
	return NS_OK;    
}
/*
NS_IMETHODIMP KmAppInfo::ProcessNativeEvent(void* aMsg)
{
	_AFX_THREAD_STATE *pState = AfxGetThreadState();
	MSG* msg = (MSG*)aMsg;
	pState->m_msgCur = *msg;
	if (msg->message != WM_KICKIDLE && !AfxPreTranslateMessage(msg))
	{
		::TranslateMessage(msg);
		::DispatchMessage(msg);
	}
	return NS_OK;    
}
*/
#include "nsIAccessibilityService.h"
NS_IMETHODIMP KmAppInfo::GetAccessibilityIsUIA(bool *aAccessibilityIsUIA)
{
	*aAccessibilityIsUIA = false;
#if defined(ACCESSIBILITY) && defined(XP_WIN)
	nsCOMPtr<nsIAccessibilityService> serv = do_GetService("@mozilla.org/accessibilityService;1");
		// This is the same check the a11y service does to identify uia clients.
	if (serv != nullptr &&
		(::GetModuleHandleW(L"uiautomation") ||
		::GetModuleHandleW(L"uiautomationcore"))) {
		*aAccessibilityIsUIA = true;
	}
#endif
	return NS_OK;
}
