!define         SETUP_VERSION "1.3.0"

!define		NAME "K-Meleon"
!define		YEAR "2007"
!define		VERSION "1.1"
!define		PRODUCT_VERSION_PLUS "1.1.0.5"
!define		ADDRESS "http://kmeleon.sourceforge.net/"  
!define		GECKO_VERSION "1.8.1.3"

!define		LANG     "English"	; see NSIS\Examples\Modern UI\MultiLanguage.nsi: Languages
!define		LANGCODE "en-US"	; specify "xx-YY" if not "en-US"

!if "${LANGCODE}" == ""
  !define	PRODUCT_BUILD "${NAME} ${VERSION}"
!endif
!if "${LANGCODE}" != ""
  !define	PRODUCT_BUILD "${NAME} ${VERSION} ${LANGCODE}"
!endif

!define		PRODUCT_KEY "Software\${NAME}"
!define		PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
!define 	CLIENT_INTERNET_KEY "Software\Clients\StartMenuInternet"
!define		PRODUCT_CLIENT_INTERNET_KEY "${CLIENT_INTERNET_KEY}\k-meleon.exe"
!define		MOZILLA_REG_KEY "Software\Mozilla\${NAME}"
!define		MEDIAPLAYER_REG_KEY "SOFTWARE\Microsoft\MediaPlayer\ShimInclusionList\k-meleon.exe"

; uncomment if IsUserAdmin function is used
#!define _IsUserAdmin

;--------------------------------
;Include Modern UI

!include "MUI.nsh"
!include "Sections.nsh"

;--------------------------------
;General



;Default installation folder
InstallDir "$PROGRAMFILES\${NAME}\"

;Get installation folder from registry if available
InstallDirRegKey HKEY_CURRENT_USER "${PRODUCT_KEY}\General" "InstallDir"

SetCompressor /SOLID lzma
SetDatablockOptimize on
ShowInstDetails show
AutoCloseWindow false
SetOverwrite on
BrandingText /TRIMRIGHT "${NAME} ${VERSION}"
LicenseBkColor "ffffff"

;Name and file
Name		"${NAME} ${VERSION}"
Caption		$(INST_Caption)
OutFile		"${NAME}${VERSION}${LANGCODE}.exe"

;--------------------------------
;Version tab in properties

  VIProductVersion      "${PRODUCT_VERSION_PLUS}"

  VIAddVersionKey        Comments         "${PRODUCT_BUILD} Setup"
  VIAddVersionKey        CompanyName      "${NAME} Team"
  VIAddVersionKey        FileDescription  "${PRODUCT_BUILD} Setup"
  VIAddVersionKey        FileVersion      "${PRODUCT_VERSION_PLUS}"
  VIAddVersionKey        InternalName     "${PRODUCT_BUILD}"
  VIAddVersionKey        LegalCopyright   "Copyright (C) ${YEAR} ${NAME} Team"
  VIAddVersionKey        LegalTrademarks  "${ADDRESS}"
  VIAddVersionKey        OriginalFilename "${NAME}${VERSION}${LANGCODE}.exe"
  VIAddVersionKey        PrivateBuild     ""
  VIAddVersionKey        ProductName      "${NAME}"
  VIAddVersionKey        ProductVersion   "${PRODUCT_VERSION_PLUS}"
  VIAddVersionKey        SpecialBuild     ""

;----------------------------------------
;Interface Settings

!define MUI_ABORTWARNING
 
!define MUI_ICON "install.ico"
!define MUI_UNICON "uninstall.ico"
!define MUI_CHECKBITMAP	"${NSISDIR}\Contrib\Icons\modern.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_NAME "${NAME} ${VERSION}"

!define MUI_INSTALLCOLORS /windows
!define MUI_PROGRESSBAR	 smooth

!define MUI_INSTFILESPAGE_COLORS "000000 ffffff" ;Multiple settings




;--------------------------------
;Pages

!define MUI_WELCOMEFINISHPAGE_BITMAP "${NAME}.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NAME}.bmp"

!define MUI_WELCOMEPAGE_TITLE $(INST_Welcome)
!define MUI_WELCOMEPAGE_TEXT $(MUI_WELCOMEPAGE_TEXT)
!insertmacro MUI_PAGE_WELCOME
  
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_COMPONENTS

!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryLeave
!insertmacro MUI_PAGE_DIRECTORY
;Page custom ProfilePage ProfilePageLeave

!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN "$INSTDIR\k-meleon.exe"
;!define MUI_FINISHPAGE_RUN_PARAMETERS "$INSTDIR\readme.html"
!insertmacro MUI_PAGE_FINISH
  
!define MUI_UNINSTPAGE
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!if ${LANG} != "English"
   !insertmacro MUI_LANGUAGE "English"
   !include "English.nlf"
!endif
!insertmacro MUI_LANGUAGE "${LANG}"
!include "${LANG}.nlf"

;--------------------------------


# ----------------------------------------------------------------------

!macro GET_PROFILES_LOCATION
	Push $R1
	Push $R2
	IfFileExists "$INSTDIR\profile.ini" 0 AppDataProfile
	ReadINIStr $R1 "$INSTDIR\profile.ini" "Profile" "path"
	ReadINIStr $R2 "$INSTDIR\profile.ini" "Profile" "isrelative"
	StrCmp $R1 "" 0 +2			; If path empty 
	StrCpy $R1 "Profiles"		; Copy default folder name
	StrCmp $R2 "1" 0 +2			; If path is relative
	StrCpy $R0 "$INSTDIR\$R1"	; add the install dir to the path
	Goto +2						; else
	StrCpy $R0 $R1				; use the path only

	Goto End_proflocation
AppDataProfile:
	StrCpy $R0 "$APPDATA\K-Meleon"
End_proflocation:

	Pop $R2
	Pop $R1
!macroend

# ----------------------------------------------------------------------
/*
LangString PROFILEPAGE_TITLE ${LANG_ENGLISH} "Profiles Location"
LangString PROFILEPAGE_SUBTITLE ${LANG_ENGLISH} "Choose the location of the profiles."
LangString PROFILEPAGE_CHOOSEDIRDIALOG ${LANG_ENGLISH} "Select a folder to be the root location for the profiles."
LangString PROFILEPAGE_CHOOSEDIR ${LANG_ENGLISH} "Choose Folder:"
LangString PROFILEPAGE_CHECK ${LANG_ENGLISH} "Use default Application Data location (uncheck to set a custom location)."
LangString PROFILEPAGE_HEADER ${LANG_ENGLISH} ""

Var HWND
Var DLGITEM

Function ProfilePageLeave

	ReadINIStr $0 "$PLUGINSDIR\profpage.ini" "Settings" "State"
	StrCmp $0 0 validate
	StrCmp $0 1 checkbutton
	Abort

checkbutton:
	ReadINIStr $0 "$PLUGINSDIR\profpage.ini" "Field 1" "State"
	ReadINIStr $1 "$PLUGINSDIR\profpage.ini" "Field 2" "HWND"
	ReadINIStr $2 "$PLUGINSDIR\profpage.ini" "Field 2" "HWND2"
	StrCmp $0 0 Notchecked

	EnableWindow $1 0
	EnableWindow $2 0
	Goto checkbutton_end

NotChecked:
	EnableWindow $1 1
	EnableWindow $2 1

checkbutton_end:
	Abort
validate:

FunctionEnd

Function ProfilePage
  Push $R0
  WriteINIStr "$PLUGINSDIR\profpage.ini" "Field 1" "Text" "$(PROFILEPAGE_CHECK)"
  WriteINIStr "$PLUGINSDIR\profpage.ini" "Field 2" "Text" "$(PROFILEPAGE_CHOOSEDIRDIALOG)"
  WriteINIStr "$PLUGINSDIR\profpage.ini" "Field 3" "Text" "$(PROFILEPAGE_CHOOSEDIR)"
  WriteINIStr "$PLUGINSDIR\profpage.ini" "Field 4" "Text" "$(PROFILEPAGE_HEADER)"
  !insertmacro MUI_HEADER_TEXT "$(PROFILEPAGE_TITLE)" "$(PROFILEPAGE_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_INITDIALOG "profpage.ini"
  Pop $HWND

  IfFileExists "$INSTDIR\profile.ini" 0 NoProfileIni
  ReadINIStr $1 "$PLUGINSDIR\profpage.ini" "Field 2" "HWND"
  ReadINIStr $2 "$PLUGINSDIR\profpage.ini" "Field 2" "HWND2"
  EnableWindow $1 1
  EnableWindow $2 1

NoProfileIni:
  !insertmacro GET_PROFILES_LOCATION
  GetDlgItem $DLGITEM $HWND 1201
  SendMessage $DLGITEM ${WM_SETTEXT} 0 "STR:$R0"

  Pop $R0
  !insertmacro MUI_INSTALLOPTIONS_SHOW
FunctionEnd
*/
# ----------------------------------------------------------------------


Section ${NAME} SecMain
	SectionIn RO
	
	RMDir /r $INSTDIR\components
	RMDir /r $INSTDIR\defaults
	RMDir /r $INSTDIR\greprefs
	RMDir /r $INSTDIR\ipc
	RMDir /r $INSTDIR\res
	Delete $INSTDIR\chrome\installed-chrome.txt
	Delete   $INSTDIR\chrome\chrome.rdf
	Delete   $INSTDIR\chrome\overlays.rdf
	RMDir /r "$INSTDIR\chrome\overlayinfo"

	; Delete Loader shortcut in Startup Folder to let the choice possible (No by default)
	Delete "$SMSTARTUP\K-Meleon Loader.lnk"
	
	SetOutPath "$INSTDIR"
	File /r .\k-meleon\*.*

        !ifdef _IsUserAdmin
	  Call IsUserAdmin
	  StrCmp $R0 "true" 0 +3
	  SetOutPath $SYSDIR
	  Goto +2
	!endif
	SetOutPath $INSTDIR
	File /nonfatal .\dll\*.*
	File /nonfatal .\misc\*.*
	

	
	WriteRegStr HKLM ${MOZILLA_REG_KEY} "GeckoVer" GECKO_VERSION
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\bin" "PathToExe" "$INSTDIR"
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\Extensions" "Plugins" "$INSTDIR\Plugins"
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\Extensions" "Components" "$INSTDIR\Components"
	WriteRegStr HKLM "Software\mozilla.org\Mozilla" "CurrentVersion" GECKO_VERSION

	; Register K-Meleon as a windows browser for windows > 2K 
	Call GetWindowsVersion
	pop $R0
	StrCpy $R1 $R0 1 0
	StrCmp $R1 "5" RegClientInternet
	StrCmp $R1 "6" RegClientInternet
	Goto NoClientInternet

RegClientInternet:
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}" "" ${NAME}
#	WriteRegStr HKLM "${CLIENT_INTERNET_KEY}\${NAME}.exe" "LocalizedString" "@$INSTDIR\K-Meleon.exe,-9"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,0"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\Shell\Open\Command" "" "$INSTDIR\K-Meleon.exe"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "ReinstallCommand" '"$INSTDIR\SetDefault.exe" /S'
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "HideIconsCommand" '"$INSTDIR\SetDefault.exe" /hide'
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "ShowIconsCommand" '"$INSTDIR\SetDefault.exe" /show'
	WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 0

NoClientInternet:
	StrCpy $R1 $R0 3 0
	StrCmp $R1 "4.0" OldWindow
	Goto NotOldWindow

OldWindow:
	FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
	FileSeek $1 0 END
	FileWrite $1 'user_pref("kmeleon.plugins.rebarmenu.load", false);'
	FileWriteByte $1 "13"
	FileWriteByte $1 "10"
	FileWrite $1 'user_pref("kmeleon.plugins.bmpmenu.load", false);'
	FileWriteByte $1 "13"
	FileWriteByte $1 "10"
	FileClose $1

NotOldWindow:
	WriteRegStr HKEY_CURRENT_USER "${PRODUCT_KEY}\General" "InstallDir" $INSTDIR
	
	WriteUninstaller $INSTDIR\uninstall.exe
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "DisplayName" $(UN_Title)
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "UninstallString" "$INSTDIR\uninstall.exe"
   	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "DisplayIcon" "$INSTDIR\k-meleon.exe,0"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "NoModify" "1"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "NoRepair" "1"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "Publisher" "K-Meleon Team"
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "HelpLink" ${ADDRESS}
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "URLInfoAbout" ${ADDRESS}
	WriteRegStr HKLM ${PRODUCT_UNINST_KEY} "URLUpdateInfo" ${ADDRESS}
	
	;Mediaplayer support
	WriteRegStr HKLM ${MEDIAPLAYER_REG_KEY} "" ""
		
	IfFileExists "$INSTDIR\profile.ini" AlreadyProfile 0
	FileOpen $1 "$INSTDIR\profile.ini" "w"
	FileClose $1 
	WriteINIStr "$INSTDIR\profile.ini" "Profile" "path" "Profiles"
	WriteINIStr "$INSTDIR\profile.ini" "Profile" "isrelative" "1"
AlreadyProfile: 
SectionEnd ; }}}

# ----------------------------------------------------------------------

SubSection $(SECT_Bookmarks) SecBookmarks ; {{{
	Section /o $(SECT_IEFavorites) SecIEFavorites
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.favorites.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd

	Section $(SECT_NSBookmarks) SecNetscapeBookmarks
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.bookmarks.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd

	Section /o $(SECT_Hotlist) SecOperaHotlist
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.hotlist.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd
SubSectionEnd ; }}}

Section $(SECT_Profile) SecProfile
	Delete $INSTDIR\profile.ini
SectionEnd

Section $(SECT_DefaultBrowser) SecAssoc  
SectionIn 1 2
	Exec '"$INSTDIR\SetDefault.exe" /S'
SectionEnd

# ----------------------------------------------------------------------

SubSection $(SECT_CreateShortcut) SecShortcuts ; {{{

	Section $(SECT_WSShortcut) SecDesktop
		CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
		WriteRegDWORD HKLM "${CLIENT_INTERNET_KEY}\k-meleon.exe\InstallInfo" "IconsVisible" 1
	SectionEnd

	Section $(SECT_SMShortcut) SecStartMenu
		CreateDirectory "$SMPROGRAMS\K-Meleon"
		CreateShortCut "$SMPROGRAMS\K-Meleon\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
		CreateShortCut "$SMPROGRAMS\K-Meleon\K-Meleon.lnk" "$INSTDIR\k-meleon.exe" "" "$INSTDIR\k-meleon.exe" 0
		WriteRegDWORD HKLM "${CLIENT_INTERNET_KEY}\k-meleon.exe\InstallInfo" "IconsVisible" 1
	SectionEnd

	Section $(SECT_QLShortcut) SecQuickLaunch
		CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
		WriteRegDWORD HKLM "${CLIENT_INTERNET_KEY}\k-meleon.exe\InstallInfo" "IconsVisible" 1
	SectionEnd

SubSectionEnd

# ----------------------------------------------------------------------

Section /o $(SECT_Loader) SecLoader ; {{{
	CreateShortCut "$SMSTARTUP\K-Meleon Loader.lnk" "$INSTDIR\loader.exe"
SectionEnd

# ----------------------------------------------------------------------

UninstallText $(UN_confirm)

Section Uninstall

	Call un.CloseKMeleon
	Sleep 1000

	ExecWait '"$INSTDIR\SetDefault.exe" /u'

	Delete   $INSTDIR\k-meleon.exe
	Delete   $INSTDIR\k-meleon.exe.manifest
	Delete   $INSTDIR\readme.html
	Delete   $INSTDIR\k-meleonloc.dll
	Delete   $INSTDIR\Language.cfg
	Delete   $INSTDIR\License.txt
	Delete   $INSTDIR\loader.exe
	Delete   $INSTDIR\uninstall.exe

	Delete   $INSTDIR\AccessibleMarshal.dll
	Delete   $INSTDIR\gkgfx.dll
	Delete   $INSTDIR\js3250.dll
	Delete   $INSTDIR\jsj3250.dll
	Delete   $INSTDIR\mozctl.dll
	Delete   $INSTDIR\mozctlx.dll
	Delete   $INSTDIR\mozipcd.exe
	Delete   $INSTDIR\mozz.dll
	Delete   $INSTDIR\nspr4.dll
	Delete   $INSTDIR\nss3.dll
	Delete   $INSTDIR\nssckbi.dll
	Delete   $INSTDIR\plc4.dll
	Delete   $INSTDIR\plds4.dll
	Delete   $INSTDIR\smime3.dll
	Delete   $INSTDIR\softokn3.chk
	Delete   $INSTDIR\softokn3.dll
	Delete   $INSTDIR\ssl3.dll
	Delete   $INSTDIR\xpcom.dll
	Delete   $INSTDIR\xpcom_compat.dll
	Delete   $INSTDIR\xpcom_core.dll
	Delete   $INSTDIR\msvcr71.dll
	Delete   $INSTDIR\msvcp71.dll
	Delete   $INSTDIR\freebl3.dll
	Delete   $INSTDIR\freebl3.chk

	RMdir /r $INSTDIR\chrome
	RMdir /r $INSTDIR\components
	RMdir /r $INSTDIR\defaults
	RMdir /r $INSTDIR\greprefs
	RMdir /r $INSTDIR\ipc
	RMdir /r $INSTDIR\res
	RMdir /r $INSTDIR\kplugins
	RMdir /r $INSTDIR\locales
	RMdir /r $INSTDIR\macros

	Delete   $INSTDIR\plugins\npnul32.dll
	RMdir    $INSTDIR\plugins

	RMdir /r $INSTDIR\skins\Phoenity
	RMdir /r $INSTDIR\skins\Phoenity(Large)
	RMdir /r $INSTDIR\skins\Klassic
	Delete   $INSTDIR\skins\commands.txt
	RMdir    $INSTDIR\skins

	Delete   $INSTDIR\SetDefault.exe

	Push $R0
	!insertmacro GET_PROFILES_LOCATION
	goto AskProfile

AskProfile:
	IfFileExists "$R0" 0 KeepProfiles
		MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 "$(UN_RemoveProfile)$R0" IDNO KeepProfiles
		RMDir /r "$R0"
		Delete $INSTDIR\profile.ini

KeepProfiles:
	RMdir    $INSTDIR

	IfFileExists "$INSTDIR" 0 KeepInstDir
		MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 $(UN_everything) IDNO KeepInstDir
		FindFirst $R2 $R3 "$INSTDIR\*.*"

FindNextFile:
		StrCmp $R3 "" NoFiles
		StrCmp $R3 "." KeepProfilesDir
		StrCmp $R3 ".." KeepProfilesDir
		StrCmp $R3 "Profiles" KeepProfilesDir
		StrCmp $R3 "Profile.ini" KeepProfilesDir
		RMDir /r "$INSTDIR\$R3"

KeepProfilesDir:
		FindNext $R2 $R3
		Goto FindNextFile

NoFiles:
		FindClose $R2
		RMdir    $INSTDIR

KeepInstDir:

	Pop $R0
	
	DeleteRegKey HKLM ${PRODUCT_KEY}
	DeleteRegKey HKLM ${PRODUCT_UNINST_KEY}
	DeleteRegKey HKLM ${PRODUCT_CLIENT_INTERNET_KEY}
	DeleteRegKey HKCR "K-Meleon.HTML"
	DeleteRegKey HKLM ${MOZILLA_REG_KEY}
	
	DeleteRegKey HKCU ${PRODUCT_KEY}
	DeleteRegKey HKCU ${PRODUCT_UNINST_KEY}
	DeleteRegKey HKCU ${PRODUCT_CLIENT_INTERNET_KEY}
	DeleteRegKey HKCU "Software\Classes\K-Meleon.HTML"
	
	DeleteRegKey HKLM ${MEDIAPLAYER_REG_KEY}
	
	Delete $DESKTOP\K-Meleon.lnk
	RMDir /r $SMPROGRAMS\K-Meleon
	Delete "$QUICKLAUNCH\K-Meleon.lnk"
	Delete "$SMSTARTUP\K-Meleon Loader.lnk"
	
	SetShellVarContext all
	Delete $DESKTOP\K-Meleon.lnk
	RMDir /r $SMPROGRAMS\K-Meleon
	Delete "$QUICKLAUNCH\K-Meleon.lnk"
	Delete "$SMSTARTUP\K-Meleon Loader.lnk"
	goto UnEnd
	
UnEnd:

SectionEnd


;--------------------------------
; Close K-Melon Functions

Function CloseKMeleon
	Push $0
	loop1:
		FindWindow $0 "KMeleon"
		IntCmp $0 0 loop2
		SendMessage $0 16 0 0
		Sleep 200
		Goto loop1
	loop2:
		FindWindow $0 "KMeleon Tray Control"
		IntCmp $0 0 done
		SendMessage $0 16 0 0
		Sleep 200
		Goto loop2
	done:
		Pop $0
FunctionEnd

Function un.CloseKMeleon
	Push $0
	loop1:
		FindWindow $0 "KMeleon"
		IntCmp $0 0 loop2
		SendMessage $0 16 0 0
		Sleep 200
		Goto loop1
	loop2:
		FindWindow $0 "KMeleon Tray Control"
		IntCmp $0 0 done
		SendMessage $0 16 0 0
		Sleep 200
		Goto loop2
	done:
		Sleep 1000
		Pop $0
FunctionEnd

;--------------------------------
Function .onInit

#  StrCpy $1 ${SecFull} ; Group 1 - Option 1 is selected by default

;	!insertmacro MUI_LANGDLL_DISPLAY  ; Uncomment for multi language installer
;	!insertmacro MUI_INSTALLOPTIONS_EXTRACT "profpage.ini"

	FindWindow $0 "KMeleon Tray Control"
	StrCmp $0 0 FindKM 0
	Goto QueryClose

FindKM:
	FindWindow $0 "KMeleon"
	StrCmp $0 0 NoKMrunning 0

QueryClose:
	MessageBox MB_OKCANCEL|MB_ICONSTOP $(INIT_KmeleonRunning) IDCANCEL 0 IDOK CloseKM
	Abort

CloseKM:
	Call CloseKMeleon

NoKMrunning:

FunctionEnd

;--------------------------------
;Function un.onInit
;!insertmacro MUI_UNGET
;  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(UN_confirm)" IDYES +2
;  Abort
;FunctionEnd

;--------------------------------
;Function un.onUninstSuccess
;  HideWindow
;  MessageBox MB_ICONINFORMATION|MB_OK "$(UNINST_OK)"
;FunctionEnd

Function DirectoryLeave
	Call CloseKMeleon

	IfFileExists "$INSTDIR\k-meleon.exe" KMFound
	IfFileExists "$INSTDIR\Profiles" KMFound
	Goto Done
KMFound:
		MessageBox MB_YESNO|MB_ICONEXCLAMATION $(INST_AlreadyInstalled) IDYES Done
;		MessageBox MB_YESNOCANCEL|MB_ICONEXCLAMATION $(INST_AlreadyInstalled) IDYES Done IDNO Retry
;		MessageBox MB_YESNO|MB_ICONEXCLAMATION $(INST_Quit) IDNO Retry
;		Quit
;Retry:
		Abort
Done:

FunctionEnd

; ----------------------------------------------------------------------
; Section descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecMain}			$(DESC_SecMain)
	#!insertmacro MUI_DESCRIPTION_TEXT ${SecAllUser}			$(DESC_SecAllUser)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts}		$(DESC_SecShortcuts)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}			$(DESC_SecDesktop)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecStartmenu}		$(DESC_SecStartmenu)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunch}		$(DESC_SecQuickLaunch)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc}			$(DESC_SecAssoc)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecBookmarks}		$(DESC_SecBookmarks)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecNetscapeBookmarks}	$(DESC_SecNetscapeBookmarks)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecIEFavorites}		$(DESC_SecIEFavorites)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecOperaHotlist}		$(DESC_SecOperaHotList)
#	!insertmacro MUI_DESCRIPTION_TEXT ${SecTools}			$(DESC_SecTools)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecLoader}			$(DESC_SecLoader)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecProfile}		        $(DESC_SecProfile)
#	!insertmacro MUI_DESCRIPTION_TEXT ${SecRemote}			$(DESC_SecRemote)
#	!insertmacro MUI_DESCRIPTION_TEXT ${SecSplash}			$(DESC_SecSplash)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
; }}}

;-------------------------------------------------------------------------------------------------
; Author: Lilla (lilla@earthlink.net) 2003-06-13
; function IsUserAdmin uses plugin \NSIS\PlusgIns\UserInfo.dll
; This function is based upon code in \NSIS\Contrib\UserInfo\UserInfo.nsi
; This function was tested under NSIS 2 beta 4 (latest CVS as of this writing).
;
; Usage:
;   Call IsUserAdmin
;   Pop $R0   ; at this point $R0 is "true" or "false"
;

!ifdef _IsUserAdmin

Function IsUserAdmin
Push $R0
Push $R1
Push $R2
 
ClearErrors
UserInfo::GetName
IfErrors Win9x
Pop $R1
UserInfo::GetAccountType
Pop $R2
 
StrCmp $R2 "Admin" 0 Continue
; Observation: I get here when running Win98SE. (Lilla)
; The functions UserInfo.dll looks for are there on Win98 too, 
; but just don't work. So UserInfo.dll, knowing that admin isn't required
; on Win98, returns admin anyway. (per kichik)
; MessageBox MB_OK 'User "$R1" is in the Administrators group'
StrCpy $R0 "true"
Goto Done
 
Continue:
; You should still check for an empty string because the functions
; UserInfo.dll looks for may not be present on Windows 95. (per kichik)
StrCmp $R2 "" Win9x
StrCpy $R0 "false"
;MessageBox MB_OK 'User "$R1" is in the "$R2" group'
Goto Done
 
Win9x:
; comment/message below is by UserInfo.nsi author:
; This one means you don't need to care about admin or
; not admin because Windows 9x doesn't either
;MessageBox MB_OK "Error! This DLL can't run under Windows 9x!"
StrCpy $R0 "true"
 
Done:
;MessageBox MB_OK 'User= "$R1"  AccountType= "$R2"  IsUserAdmin= "$R0"'
 
Pop $R2
Pop $R1
Exch $R0
FunctionEnd

!endif

;-----------------------------------------------------------------------------------------------
Function GetWindowsVersion

	Push $R0

	ClearErrors

	ReadRegStr $R0 HKLM \
	"SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
	IfErrors 0 lbl_winnt

	; we are not NT
	ReadRegStr $R0 HKLM \
	"SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber

lbl_winnt:

	Exch $R0

FunctionEnd

