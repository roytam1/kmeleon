!packhdr		tmp.dat "c:\apps\upx\upx.exe -9 --best --strip-relocs=1 tmp.dat"

!define		NAME "K-Meleon"
!define		VERSION "0.9"

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
  !include "Sections.nsh"

;--------------------------------
;General

  ;Name and file
  Name			"${NAME} ${VERSION}"
  Caption		"${NAME} ${VERSION} - Setup"
  OutFile		"${NAME}.exe"

  ;Default installation folder
  InstallDir 		"$PROGRAMFILES\${NAME}"
  
  ;Get installation folder from registry if available
  InstallDirRegKey 	HKEY_CURRENT_USER "Software\${NAME}\${NAME}\General" "InstallDir"

SetCompressor		lzma
; SetCompressor		best
; SetCompressor		zlib
SetDatablockOptimize	on
ShowInstDetails		show
AutoCloseWindow		false
SetOverwrite		on

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
 
!define MUI_NAME		"${NAME} ${VERSION}"

!define MUI_INSTALLCOLORS	/windows
!define MUI_PROGRESSBAR		smooth

!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN "$INSTDIR\k-meleon.exe"
;; !define MUI_FINISHPAGE_RUN_PARAMETERS "$INSTDIR\readme.html"

!define MUI_UNINSTPAGE

; !define MUI_UI			"${NSISDIR}\Contrib\UIs\modern.exe"
; !define MUI_INTERFACE
; !define MUI_SYSTEM
; !define MUI_LANGUAGEFILE_BEGIN

;--------------------------------
;Pages

!define MUI_ICON	"install.ico"
!define MUI_UNICON	"uninstall.ico"
!define MUI_CHECKBITMAP	"${NSISDIR}\Contrib\Icons\modern.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_INSTFILESPAGE_COLORS "000000 ffffff" ;Multiple settings

BrandingText		/TRIMRIGHT "${NAME} ${VERSION}"

LicenseBkColor		"ffffff"


!define MUI_WELCOMEPAGE_TITLE "${NAME} ${VERSION} Install Wizard"
; !define MUI_WELCOMEPAGE_TEXT "Text"


;!define MUI_COMPONENTSPAGE_TEXT_TOP "top"
;!define MUI_COMPONENTSPAGE_TEXT_COMPLIST "complist"
;!define MUI_COMPONENTSPAGE_TEXT_INSTTYPE "insttype"
;!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_TITLE "desc.title"
;!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO  "desc.info"


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryLeave
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

# ----------------------------------------------------------------------

Section "K-Meleon" SecMain
	SectionIn RO

	SetOutPath "$INSTDIR"
	File /r .\k-meleon\*.*
	Delete   $INSTDIR\chrome\chrome.rdf
	RMDir /r "$INSTDIR\chrome\overlayinfo"
	Delete   $INSTDIR\component\compreg.dat
	Delete   $INSTDIR\component\xpti.dat

	WriteRegStr HKLM "Software\Mozilla\K-Meleon" "GeckoVer" "1.7.5"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\bin" "PathToExe" "$INSTDIR"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\Extensions" "Plugins" "$INSTDIR\Plugins"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\Extensions" "Components" "$INSTDIR\Components"
	WriteRegStr HKLM "Software\mozilla.org\Mozilla" "CurrentVersion" "1.7.5"

	WriteRegStr HKCR "K-Meleon.HTML" "" "Hypertext Markup Language Document"
	WriteRegStr HKCR "K-Meleon.HTML\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,1"
	WriteRegStr HKCR "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

	WriteUninstaller $INSTDIR\uninstall.exe
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" "$INSTDIR\uninstall.exe"

#	IfFileExists "$INSTDIR\Profiles" 0 KeepProfiles
#		MessageBox MB_YESNO|MB_ICONEXCLAMATION "User profiles from a previous installation of K-Meleon have been detected$\n\
#These files might have to be removed for K-Meleon to function properly.$\n\
#Do you want to remove the stored user profiles?" IDNO KeepProfiles
#		RMDir /r "$INSTDIR\Profiles"
#
#KeepProfiles:

SectionEnd ; }}}

# ----------------------------------------------------------------------

SubSection "Bookmark Support" SecBookmarks ; {{{
	Section /o "Internet Explorer Favorites" SecIEFavorites
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.favorites.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd

	Section "Netscape Bookmarks" SecNetscapeBookmarks
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.bookmarks.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd

	Section /o "Opera Hotlist" SecOperaHotlist
		FileOpen $1 "$INSTDIR\defaults\profile\Prefs.js" "a"
		FileSeek $1 0 END
		FileWrite $1 'user_pref("kmeleon.plugins.hotlist.load", true);'
		FileWriteByte $1 "13"
		FileWriteByte $1 "10"
		FileClose $1
	SectionEnd
SubSectionEnd ; }}}

# ----------------------------------------------------------------------

!macro assocProtocol proto desc
	push $0

	ReadRegStr $0 HKCR "${proto}" ""
	WriteRegStr HKCR "${proto}" "" "${desc}"

	ReadRegStr $0 HKCR "${proto}\shell\open\command" ""
	WriteRegStr HKCR "${proto}\DefaultIcon" "" ""
	WriteRegStr HKCR "${proto}\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
	DeleteRegKey HKCR "${proto}\shell\open\ddeexec"
	WriteRegStr HKCR "${proto}\shell\open\ddeexec\Application" "" "K-Meleon"

	pop $0
!macroend


!macro assocExtension ext type
	push $0

	ReadRegStr $0 HKCR "${ext}" ""
	WriteRegStr HKCR "${ext}" "" "K-Meleon.HTML"

	ReadRegStr $0 HKCR "${ext}" "Content Type"
	WriteRegStr HKCR "${ext}" "Content Type" "${type}"

	DeleteRegKey HKCR "${ext}\PersistentHandler"

	pop $0
!macroend

# ----------------------------------------------------------------------

SubSection "Set K-Meleon as your default browser" SecAssoc ; {{{
	Section
		SectionIn 1 2 RO
	SectionEnd

	SubSection "Protocols" SecAssocProto ; {{{
		Section /o "http" SecHttp
			SectionIn 1
			DetailPrint "Set K-Meleon as your default application for the http protocol"
			!insertmacro assocProtocol  "http"	"URL:HTTP (HyperText Transfer-Protocol)"
		SectionEnd

		Section /o "https" SecHttps
			SectionIn 1
			DetailPrint "Set K-Meleon as your default application for the https protocol"
			!insertmacro assocProtocol  "https"	"URL:HTTPS (HyperText Transfer-Protocol with Security)"
		SectionEnd

	SubSectionEnd ; }}}

	SubSection "File types" SecAssocExt ; {{{
		Section /o ".htm files" SecHtm
			SectionIn 1
			DetailPrint "Set K-Meleon as your default application for .htm files"
			!insertmacro assocExtension ".htm"	"text/html"
		SectionEnd

		Section /o ".html files" SecHtml
			SectionIn 1
			DetailPrint "Set K-Meleon as your default application for .html files"
			!insertmacro assocExtension ".html"	"text/html"
		SectionEnd

		Section /o ".shtml files" SecShtml
			SectionIn 1
			DetailPrint "Set K-Meleon as your default application for .shtml files"
			!insertmacro assocExtension ".shtml"	"text/html"
		SectionEnd
	SubSectionEnd ; }}}

;	ReadRegStr $0 HKCR "shtml_auto_file\shell\open\command" ""
;	WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" "shtml_auto_file\shell\open\command" $0
;	WriteRegStr HKCR "shtml_auto_file\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
;
;	ReadRegStr $0 HKLM "Software\CLASSES\http\shell\open\command" ""
;	WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKLM" "Software\CLASSES\http\shell\open\command" $0
;	WriteRegStr HKLM "Software\CLASSES\http\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
;
;	ReadRegStr $0 HKLM "Software\CLASSES\InternetShortcut\shell\open\command" ""
;	WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKLM" "Software\CLASSES\InternetShortcut\shell\open\command" $0
;	WriteRegStr HKLM "Software\CLASSES\InternetShortcut\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%l" %1'
;
;	ReadRegStr $0 HKLM "Software\CLASSES\shtml_auto_file\shell\open\command" ""
;	WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKLM" "Software\CLASSES\shtml_auto_file\shell\open\command" $0
;	WriteRegStr HKLM "Software\CLASSES\shtml_auto_file\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

SubSectionEnd

# ----------------------------------------------------------------------

SubSection "Create Windows Shortcuts for K-Meleon" SecShortcuts ; {{{

	Section /o "Desktop Shortcut" SecDesktop
		CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
	SectionEnd

	Section /o "Start Menu Shortcuts" SecStartMenu
		CreateDirectory "$SMPROGRAMS\K-Meleon"
		CreateShortCut "$SMPROGRAMS\K-Meleon\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
		CreateShortCut "$SMPROGRAMS\K-Meleon\K-Meleon.lnk" "$INSTDIR\k-meleon.exe" "" "$INSTDIR\k-meleon.exe" 0
	SectionEnd

	Section /o "Quick Launch Shortcuts" SecQuickLaunch
		CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
	SectionEnd

SubSectionEnd

# ----------------------------------------------------------------------

SubSection "Miscellaneous K-Meleon Tools" SecTools ; {{{

	Section /o "K-Meleon Loader" SecLoader ; {{{
		SetOutPath $INSTDIR
		File .\misc\loader.exe
		CreateShortCut "$SMSTARTUP\K-Meleon Loader.lnk" "$INSTDIR\loader.exe"
	SectionEnd

#	Section /o "K-Meleon Remote Control" SecRemote ; {{{
#		SetOutPath $INSTDIR
#		File .\misc\km-remote.exe
#	SectionEnd
#
#	Section /o "Splash screen tool" SecSplash ; {{{
#		SetOutPath $INSTDIR
#		File .\misc\splash.exe
#		File .\misc\splash.ini
#	SectionEnd

SubSectionEnd

# ----------------------------------------------------------------------

UninstallText "This will remove K-Meleon from your computer. Click Uninstall to start the uninstallation."

Section Uninstall ; {{{

	Call un.CloseKMeleon
	Sleep 1000

	Delete   $INSTDIR\k-meleon.exe
	Delete   $INSTDIR\k-meleon.exe.manifest
	Delete   $INSTDIR\readme.html
	Delete   $INSTDIR\relnotes09b.html
	Delete   $INSTDIR\License.txt
	Delete   $INSTDIR\SetDefault.exe
	Delete   $INSTDIR\loader.exe
	Delete   $INSTDIR\km-remote.exe
	Delete   $INSTDIR\splash.exe
	Delete   $INSTDIR\splash.ini
	Delete   $INSTDIR\!K-meleon.nsi
	Delete   $INSTDIR\uninstall.exe
	Delete   $INSTDIR\mozilla-ipcd.exe
	Delete   $INSTDIR\mfcembed.exe
	Delete   $INSTDIR\mfcEmbedComponents.dll

	RMdir /r $INSTDIR\kplugins
	RMdir /r $INSTDIR\skins\default
	Delete   $INSTDIR\skins\commands.txt
	RMdir    $INSTDIR\skins

	Delete   $INSTDIR\plugins\npnul32.dll
	RMdir    $INSTDIR\plugins

	RMdir /r $INSTDIR\chrome
	RMdir /r $INSTDIR\components
	RMdir /r $INSTDIR\defaults
	RMdir /r $INSTDIR\greprefs
	RMdir /r $INSTDIR\ipc
	RMdir /r $INSTDIR\res

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

	IfFileExists "$INSTDIR\Profiles" 0 KeepProfiles
		MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 "Would you like to remove the user profiles?$\n$\n(The user profile contains your personal preferences, bookmarks, and history)" IDNO KeepProfiles
		RMDir /r "$INSTDIR\Profiles"
KeepProfiles:

	RMdir    $INSTDIR

	IfFileExists "$INSTDIR" 0 KeepInstDir
		MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 "There seems to be more files left in the install directory, would you like to remove everything?$\n$\n(This is most likely installed plugins, skins, or other downloaded files)" IDNO KeepInstDir
		FindFirst $R2 $R3 "$INSTDIR\*.*"
FindNextFile:
		StrCmp $R3 "" NoFiles
		StrCmp $R3 "." KeepProfilesDir
		StrCmp $R3 ".." KeepProfilesDir
		StrCmp $R3 "Profiles" KeepProfilesDir
		RMDir /r "$INSTDIR\$R3"
KeepProfilesDir:
		FindNext $R2 $R3
		Goto FindNextFile
NoFiles:
		FindClose $R2
		RMdir    $INSTDIR
KeepInstDir:

	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\K-Meleon"
 	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon"
	Delete $DESKTOP\K-Meleon.lnk
	RMDir /r $SMPROGRAMS\K-Meleon

SectionEnd ; }}}

# ----------------------------------------------------------------------

; Close K-Melon Functions {{{
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

# ----------------------------------------------------------------------

Function .onInit ; {{{

#        StrCpy $1 ${SecFull} ; Group 1 - Option 1 is selected by default

	FindWindow $0 "KMeleon Tray Control"
	StrCmp $0 0 FindKM 0
	Goto QueryClose
FindKM:
	FindWindow $0 "KMeleon"
	StrCmp $0 0 NoKMrunning 0
QueryClose:
	MessageBox MB_OKCANCEL|MB_ICONSTOP "It appears that K-Meleon or the loader is currently running.$\r$\nIt must be closed before ${NAME} ${VERSION} can continue installation.$\r$\n$\r$\nPress Ok to close now, or Cancel to abort installation" IDCANCEL 0 IDOK CloseKM
	Abort
CloseKM:
	Call CloseKMeleon
NoKMrunning:

FunctionEnd ; }}}

# ----------------------------------------------------------------------

Function DirectoryLeave
	Call CloseKMeleon

	IfFileExists "$INSTDIR\k-meleon.exe" KMFound
	IfFileExists "$INSTDIR\Profiles" KMFound
	Goto Done
KMFound:
		MessageBox MB_YESNOCANCEL|MB_ICONEXCLAMATION "An earlier install of ${NAME} has been detected in ''$INSTDIR'' !$\n\
Are you sure you want to install ${NAME} in this directory?" IDYES Done IDNO Retry
		MessageBox MB_YESNO|MB_ICONEXCLAMATION "Are you sure you want to quit ${NAME} ${VERSION} - Setup?" IDNO Retry
		Quit
Retry:
		Abort
Done:
	WriteRegStr HKEY_CURRENT_USER "Software\${NAME}\${NAME}\General" "InstallDir" $INSTDIR
FunctionEnd

# ----------------------------------------------------------------------



; Modern UI Functions {{{

;; !insertmacro MUI_SYSTEM

LangString DESC_SecMain				${LANG_ENGLISH} "Install the required ${NAME} files."
LangString DESC_SecBookmarks			${LANG_ENGLISH} "Choose what type of bookmark support you wish to have enabled by default."
LangString DESC_SecNetscapeBookmarks		${LANG_ENGLISH} "Enable support for Netscape compatible bookmarks."
LangString DESC_SecIEFavorites			${LANG_ENGLISH} "Enable support for Internet Explorer compatible Favorites."
LangString DESC_SecOperaHotlist			${LANG_ENGLISH} "Enable support for Opera compatible bookmarks (Hotlist)."
LangString DESC_SecAssoc			${LANG_ENGLISH} 'These entries in the register ask other applications to use ${NAME} when opening web pages.'
LangString DESC_SecAssocProto			${LANG_ENGLISH} "Chose what web protocols to open with ${NAME}."
LangString DESC_SecHttp				${LANG_ENGLISH} "Always open http:// urls with ${NAME}."
LangString DESC_SecHttps			${LANG_ENGLISH} "Always open https:// urls with ${NAME}."
LangString DESC_SecAssocExt			${LANG_ENGLISH} "Chose what file types to open with ${NAME}."
LangString DESC_SecHtm				${LANG_ENGLISH} "Always open .HTM files with ${NAME}."
LangString DESC_SecHtml				${LANG_ENGLISH} "Always open .HTML files with ${NAME}."
LangString DESC_SecShtml			${LANG_ENGLISH} "Always open .SHTML files with ${NAME}."
LangString DESC_SecShortcuts			${LANG_ENGLISH} "Select which shortcuts you would like to have created."
LangString DESC_SecDesktop			${LANG_ENGLISH} "Create a desktop shortcut."
LangString DESC_SecStartMenu			${LANG_ENGLISH} "Create a Start Menu folder with shortcuts."
LangString DESC_SecQuickLaunch			${LANG_ENGLISH} "Create a Quick Launch shortcut."
LangString DESC_SecTools			${LANG_ENGLISH} "Select which extra ${NAME} tools you would like to install."
LangString DESC_SecLoader 			${LANG_ENGLISH} "Install the ${NAME} Loader, for significantly faster startup times."
#LangString DESC_SecRemote 			${LANG_ENGLISH} "Install the ${NAME} Remote Control tool. (requires the external program control plugin to be enabled)"
#LangString DESC_SecSplash 			${LANG_ENGLISH} "Install a splash screen tool."


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecMain}			$(DESC_SecMain)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts}		$(DESC_SecShortcuts)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}			$(DESC_SecDesktop)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecStartmenu}		$(DESC_SecStartmenu)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunch}		$(DESC_SecQuickLaunch)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc}			$(DESC_SecAssoc)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssocProto}		$(DESC_SecAssocProto)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHttp}			$(DESC_SecHttp)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHttps}			$(DESC_SecHttps)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssocExt}		$(DESC_SecAssocExt)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHtm}			$(DESC_SecHtm)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHtml}			$(DESC_SecHtml)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecShtml}			$(DESC_SecShtml)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecBookmarks}		$(DESC_SecBookmarks)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecNetscapeBookmarks}	$(DESC_SecNetscapeBookmarks)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecIEFavorites}		$(DESC_SecIEFavorites)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecOperaHotlist}		$(DESC_SecOperaHotList)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecTools}			$(DESC_SecTools)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecLoader}			$(DESC_SecLoader)
#	!insertmacro MUI_DESCRIPTION_TEXT ${SecRemote}			$(DESC_SecRemote)
#	!insertmacro MUI_DESCRIPTION_TEXT ${SecSplash}			$(DESC_SecSplash)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
; }}}
