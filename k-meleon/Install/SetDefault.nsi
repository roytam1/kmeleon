!packhdr		tmp.dat "C:\Progra~1\Upx\bin\upx.exe -9 --best --strip-relocs=1 tmp.dat"

!define		NAME "K-Meleon"
!define		VERSION "1.0"
!define    ADDRESS "http://kmeleon.sourceforge.net/"
!define    GECKO_VERSION "1.8.0"

!define 	CLIENT_INTERNET_KEY "Software\Clients\StartMenuInternet"
!define		PRODUCT_CLIENT_INTERNET_KEY "${CLIENT_INTERNET_KEY}\k-meleon.exe"
!define		MOZILLA_REG_KEY "Software\Mozilla\${NAME}"
!define		PRODUCT_KEY "Software\${NAME}"

!define    SETDEFVERS "1.2.0.0"
  
!include "WinMessages.nsh" 

Var /Global UserIsAdmin

;--------------------------------
;Version tab in properties

  VIProductVersion      ${SETDEFVERS}

  VIAddVersionKey        ProductName      "${NAME} SetDefault"
  VIAddVersionKey        Comments         "${NAME} SetDefault ${SETDEFVERS}"
  VIAddVersionKey        CompanyName      "${ADDRESS}"
  VIAddVersionKey        LegalCopyright   "${ADDRESS}"
  VIAddVersionKey        FileDescription  "${NAME} Browser Configurator"
  VIAddVersionKey        FileVersion      "${SETDEFVERS}"
  VIAddVersionKey        ProductVersion   "${SETDEFVERS}"
  VIAddVersionKey        InternalName     "${NAME} SetDefault ${SETDEFVERS}"
  VIAddVersionKey        LegalTrademarks  "${ADDRESS}"
  VIAddVersionKey        OriginalFilename "SetDefault.exe"
# VIAddVersionKey        PrivateBuild     "http://perso.wanadoo.fr/jujuland/"
  VIAddVersionKey        SpecialBuild     "English version"
  
  
;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
  !include "Sections.nsh"

;--------------------------------
;General

  ;Name and file
  Name	    	        "${NAME} ${VERSION}"
  Caption		"Define ${NAME} ${VERSION} as default browser"
  OutFile		"SetDefault.exe"

  ;Default installation folder
  InstallDir            "$EXEDIR\"

  ;Get installation folder from registry if available
;  InstallDirRegKey 	HKEY_CURRENT_USER "Software\${NAME}\${NAME}\General" "InstallDir"

SetCompressor		lzma
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

!define MUI_UNINSTPAGE

;--------------------------------
;Pages

!define MUI_ICON	"kmeleon.ico"
!define MUI_CHECKBITMAP	"${NSISDIR}\Contrib\Icons\modern.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_INSTFILESPAGE_COLORS "000000 ffffff" ;Multiple settings

!define MUI_PAGE_HEADER_TEXT  $(SD_HeaderText)
!define MUI_PAGE_HEADER_SUBTEXT $(SD_HeaderSubText)
!define MUI_DIRECTORYPAGE_TEXT_TOP $(SD_DirectoryText)
!define MUI_COMPONENTSPAGE_TEXT_TOP $(SD_ComponentText)


BrandingText		/TRIMRIGHT "${NAME} ${VERSION}"

LicenseBkColor		"ffffff"

  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryLeave
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_INSTFILES
  
;--------------------------------
;Languages
 
!insertmacro MUI_LANGUAGE "English"
!include "english.nlf"

;--------------------------------

# ----------------------------------------------------------------------

!macro assocProtocol proto desc
push $0

	ReadRegStr $0 SHCTX "Software\Classes\${proto}" ""
	IfErrors 0 +2
	WriteRegStr SHCTX "Software\Classes\${proto}" "" "${desc}"
	StrCmp $0 "${desc}" +2
	WriteRegStr SHCTX "${PRODUCT_KEY}\Desktop" "${proto}" $0
	

	ReadRegStr $0 SHCTX "Software\Classes\${proto}\shell\open\command" ""
	StrCmp $0 '"$INSTDIR\K-Meleon.exe" "%1"' assocProtocol_End
	WriteRegStr SHCTX "${PRODUCT_KEY}\Desktop" "${proto}\shell\open\command" $0
	ReadRegStr $0 SHCTX "Software\Classes\${proto}\DefaultIcon" ""
	WriteRegStr SHCTX "${PRODUCT_KEY}\Desktop" "${proto}\DefaultIcon" $0
	
	WriteRegStr SHCTX "Software\Classes\${proto}\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,1"
	WriteRegStr SHCTX "Software\Classes\${proto}\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
	DeleteRegKey SHCTX "Software\Classes\${proto}\shell\open\ddeexec"
	WriteRegStr SHCTX "Software\Classes\${proto}\shell\open\ddeexec\Application" "" "K-Meleon"
assocProtocol_End:	
	
	pop $0
!macroend

!macro assocExtension ext type
	push $0
	
	ReadRegStr $0 SHCTX "Software\Classes\${ext}" ""
	
	StrCmp $0 "K-Meleon.HTML" assocExtension_End
	WriteRegStr SHCTX "${PRODUCT_KEY}\Desktop" "${ext}" $0
	WriteRegStr SHCTX "Software\Classes\${ext}" "" "K-Meleon.HTML"

	ReadRegStr $0 SHCTX "Software\Classes\${ext}" "Content Type"
	WriteRegStr SHCTX "Software\Classes\${ext}" "Content Type" "${type}"

	DeleteRegKey SHCTX "Software\Classes\${ext}\PersistentHandler"
assocExtension_End:

	pop $0
!macroend

Function restoreExtension 
	Exch $R0
	Push $0
	
	StrCmp $UserIsAdmin "true" 0 NotAdmin
	ReadRegStr $0 HKLM "Software\Classes\$R0" ""
	StrCmp $0 "K-Meleon.HTML" 0 restoreExtension_End
	ReadRegStr $0 HKLM "${PRODUCT_KEY}\Desktop" "$R0" 
	IfErrors restoreExtension_End
	WriteRegStr HKLM "Software\Classes\$R0" "" $0
	Goto restoreExtension_End
NotAdmin:
	ReadRegStr $0 HKCU "Software\Classes\$R0" ""
	StrCmp $0 "K-Meleon.HTML" 0 restoreExtension_End
	ReadRegStr $0 HKCU "${PRODUCT_KEY}\Desktop" "$R0" 
	StrCmp $0 "" +3 0
	WriteRegStr HKCU "Software\Classes\$R0" "" $0
	Goto restoreExtension_End
	DeleteRegKey HKCU "Software\Classes\$R0"

restoreExtension_End:
	Pop $0
	Pop $R0
FunctionEnd

Function restoreProtocol 
	Exch $R1
	Exch
	Exch $R0
	
	StrCmp $UserIsAdmin "true" 0 NotAdmin
	
	ReadRegStr $0 HKLM "Software\Classes\$R0" ""
	StrCmp $0 "$R1" 0 +3
	ReadRegStr $0 HKLM "${PRODUCT_KEY}\Desktop" "$R0"
	IfErrors restoreProtocol_End
	WriteRegStr HKLM "Software\Classes\$R0" "" $0
	
	ReadRegStr $0 HKLM "Software\Classes\$R0\shell\open\command" ""
	StrCmp $0 '"$INSTDIR\K-Meleon.exe" "%1"' 0 restoreProtocol_End
	ReadRegStr $0 HKLM "${PRODUCT_KEY}\Desktop" "$R0\DefaultIcon"
	IfErrors restoreProtocol_End
	WriteRegStr HKLM "Software\Classes\$R0\DefaultIcon" "" $0
	ReadRegStr $0 HKLM "${PRODUCT_KEY}\Desktop" "$R0\shell\open\command"
	IfErrors restoreProtocol_End
	WriteRegStr HKLM "Software\Classes\$R0\shell\open\command" "" $0
	DeleteRegKey HKLM "Software\Classes\$R0\shell\open\ddeexec"
	Goto restoreProtocol_End
NotAdmin:

	ReadRegStr $0 HKCU "Software\Classes\$R0" ""
	StrCmp $0 "$R1" 0 +3
	ReadRegStr $0 HKCU "${PRODUCT_KEY}\Desktop" "$R0"
	StrCmp $0 "" 0 +3
	DeleteRegKey HKCU "Software\Classes\$R0"
	Goto restoreProtocol_End
	
	WriteRegStr HKCU "Software\Classes\$R0" "" $0
	
	ReadRegStr $0 HKCU "Software\Classes\$R0\shell\open\command" ""
	StrCmp $0 '"$INSTDIR\K-Meleon.exe" "%1"' 0 restoreProtocol_End
	ReadRegStr $0 HKCU "${PRODUCT_KEY}\Desktop" "$R0\DefaultIcon"
	WriteRegStr HKCU "Software\Classes\$R0\DefaultIcon" "" $0
	ReadRegStr $0 HKCU "${PRODUCT_KEY}\Desktop" "$R0\shell\open\command"
	WriteRegStr HKCU "Software\Classes\$R0\shell\open\command" "" $0
	DeleteRegKey HKCU "Software\Classes\$R0\shell\open\ddeexec"

restoreProtocol_End:
	Pop $R0
	Pop $0
	Pop $R0
	Pop $R1
FunctionEnd

# ----------------------------------------------------------------------

Section $(SECT_MainSD) SecMain ; {{{

	SectionIn 1

	DetailPrint "Register ${NAME} as a Mozilla/Gecko product"

	WriteRegStr HKLM ${MOZILLA_REG_KEY} "GeckoVer" GECKO_VERSION
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\bin" "PathToExe" "$INSTDIR"
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\Extensions" "Plugins" "$INSTDIR\Plugins"
	WriteRegStr HKLM "${MOZILLA_REG_KEY}\Extensions" "Components" "$INSTDIR\Components"
	WriteRegStr HKLM "Software\mozilla.org\Mozilla" "CurrentVersion" GECKO_VERSION
	WriteRegStr HKCU "${PRODUCT_KEY}\General" "InstallDir" "$INSTDIR"


SectionEnd ; }}}

# ----------------------------------------------------------------------


Section /o $(SECT_BrowserList) SecBrowserList ; {{{
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}" "" ${NAME}
#	WriteRegStr HKLM "Software\Clients\StartMenuInternet\${NAME}.exe" "LocalizedString" "@$INSTDIR\K-Meleon.exe,-9"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,0"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\Shell\Open\Command" "" "$INSTDIR\K-Meleon.exe"
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "ReinstallCommand" '"$INSTDIR\SetDefault.exe" /S'
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "HideIconsCommand" '"$INSTDIR\SetDefault.exe" /hide'
	WriteRegStr HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "ShowIconsCommand" '"$INSTDIR\SetDefault.exe" /show'
	WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 0
SectionEnd


SubSection "Define ${NAME} as default browser" SecAssocHTML ; {{{
	
	Section
		SectionIn 1 2 RO
		
		#XP Start Menu
		WriteRegStr HKLM ${CLIENT_INTERNET_KEY} "" "k-meleon.exe" 
		IfErrors 0 +3
		WriteRegStr HKCU ${CLIENT_INTERNET_KEY} "" "k-meleon.exe" 
		Goto +2
		WriteRegStr HKCU ${CLIENT_INTERNET_KEY} "" "" 
			
		StrCmp $UserIsAdmin "true" 0 +3
		SetShellVarContext all
		Goto +2
		SetShellVarContext current

		WriteRegStr SHCTX "Software\Classes\K-Meleon.HTML" "" $(SD_DocumentName)
		WriteRegStr SHCTX "Software\Classes\K-Meleon.HTML\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,1"
		WriteRegStr SHCTX "Software\Classes\K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
		
		WriteRegStr HKCU "${PRODUCT_KEY}\General" "InstallDir" "$INSTDIR"
		SendMessage HWND_BROADCAST WM_SETTINGCHANGE 0 "STR:${CLIENT_INTERNET_KEY}" /TIMEOUT=5000
	SectionEnd

	SubSection $(SECT_Protocols) SecAssocProto ; {{{
		Section "http" SecHttp
			SectionIn 1
			DetailPrint $(SET_Default_http)
			!insertmacro assocProtocol  "http"	"URL:HTTP (HyperText Transfer-Protocol)"
		SectionEnd

		Section "https" SecHttps
			SectionIn 1
			DetailPrint $(SET_Default_https)
			!insertmacro assocProtocol  "https"	"URL:HTTPS (HyperText Transfer-Protocol with Security)"
		SectionEnd

	SubSectionEnd ; }}}

	SubSection $(SECT_FileTypes) SecAssocExt ; {{{
		Section $(SECT_HtmFiles) SecHtm
			SectionIn 1
			DetailPrint $(SET_Default_htm)
			!insertmacro assocExtension ".htm"	"text/html"
		SectionEnd

		Section $(SECT_HtmlFiles) SecHtml
			SectionIn 1
			DetailPrint $(SET_Default_html)
			!insertmacro assocExtension ".html"	"text/html"
		SectionEnd

		Section $(SECT_ShtmlFiles) SecShtml
			SectionIn 1
			DetailPrint $(SET_Default_shtml)
			!insertmacro assocExtension ".shtml"	"text/html"
		SectionEnd
		
		Section $(SECT_XhtFiles) SecXht
			SectionIn 1
			DetailPrint $(SET_Default_xht)
			!insertmacro assocExtension ".xht"	"application/xhtml+xml"
		SectionEnd
		
		Section $(SECT_XhtmlFiles) SecXhtml
			SectionIn 1
			DetailPrint $(SET_Default_xhtml)
			!insertmacro assocExtension ".xhtml" "application/xhtml+xml"
		SectionEnd
	SubSectionEnd ; }}}

SubSectionEnd

# ----------------------------------------------------------------------

SubSection $(SECT_CreateShortcut) SecShortcuts ; {{{

	Section /o $(SECT_WSShortcut) SecDesktop
		SetShellVarContext current ; shortcut in AllUsers are annoying
		CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
		WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 1
	SectionEnd

	Section /o $(SECT_SMShortcut) SecStartMenu
		CreateDirectory "$SMPROGRAMS\K-Meleon"
		CreateShortCut "$SMPROGRAMS\K-Meleon\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
		CreateShortCut "$SMPROGRAMS\K-Meleon\K-Meleon.lnk" "$INSTDIR\k-meleon.exe" "" "$INSTDIR\k-meleon.exe" 0
		WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 1
	SectionEnd

	Section /o $(SECT_QLShortcut) SecQuickLaunch
		CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
		WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 1
	SectionEnd

SubSectionEnd

# ----------------------------------------------------------------------

Function DirectoryLeave
	IfFileExists "$INSTDIR\k-meleon.exe" KMFound
	MessageBox MB_RETRYCANCEL|MB_ICONSTOP $(SD_NotDetected) IDRETRY Retry
	MessageBox MB_YESNO|MB_ICONEXCLAMATION $(SD_QuitConfirm) IDNO Retry
	Quit
Retry:
	Abort
KMFound:
#	WriteRegStr HKEY_CURRENT_USER "Software\${NAME}\${NAME}\General" "InstallDir" $INSTDIR
FunctionEnd

# ----------------------------------------------------------------------

; Modern UI Functions {{{

;; !insertmacro MUI_SYSTEM


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecMain}			$(DESC_SecMainSD)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecBrowserList}	$(DESC_SecBrowserList)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssocHTML}		$(DESC_SecAssocHTML)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssocProto}		$(DESC_SecAssocProto)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHttp}			$(DESC_SecHttp)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHttps}			$(DESC_SecHttps)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecAssocExt}		$(DESC_SecAssocExt)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHtm}			$(DESC_SecHtm)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecHtml}			$(DESC_SecHtml)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecShtml}			$(DESC_SecShtml)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecXht}			$(DESC_SecXht)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecXhtml}			$(DESC_SecXhtml)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts}		$(DESC_SecShortcuts)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}    	$(DESC_SecDesktop)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu}		$(DESC_SecStartMenu)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunch}	$(DESC_SecQuickLaunch)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
; }}}


Function GetParameters
 
   Push $R0
   Push $R1
   Push $R2
   Push $R3
   
   StrCpy $R2 1
   StrLen $R3 $CMDLINE
   
   ;Check for quote or space
   StrCpy $R0 $CMDLINE $R2
   StrCmp $R0 '"' 0 +3
     StrCpy $R1 '"'
     Goto loop
   StrCpy $R1 " "
   
   loop:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 $R1 get
     StrCmp $R2 $R3 get
     Goto loop
   
   get:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " get
     StrCpy $R0 $CMDLINE "" $R2
   
   Pop $R3
   Pop $R2
   Pop $R1
   Exch $R0
 
 FunctionEnd

Function .onInit
	Call IsUserAdmin
	Exch $R0
	StrCpy $UserIsAdmin $R0
	
	
	Call GetParameters
	Pop $1

	StrCmp $1 "/u" 0 onInit_noUn
	Push ".htm"
	Call restoreExtension
	Push ".html"
	Call restoreExtension
	Push ".shtml"
	Call restoreExtension
	Push ".xht"
	Call restoreExtension
	Push ".xhtml"
	Call restoreExtension	
	
	Push "http"
	Push "URL:HTTP (HyperText Transfer-Protocol)"
	Call restoreProtocol
	
	Push "https"
	Push "URL:HTTPS (HyperText Transfer-Protocol with Security)"
	Call restoreProtocol
	
#	!insertmacro restoreExtension ".htm"
#	!insertmacro restoreExtension ".html"
#	!insertmacro restoreExtension ".shtml"
#	!insertmacro restoreExtension ".xht"
#	!insertmacro restoreExtension ".xhtml"
#	!insertmacro restoreProtocol  "https"	"URL:HTTPS (HyperText Transfer-Protocol with Security)"
#	!insertmacro restoreProtocol  "http"		"URL:HTTP (HyperText Transfer-Protocol)"
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
	Quit
	
onInit_noUn:
# This is probably more annoying than anything else.
	StrCmp $1 "/hide" 0 onInit_noHide
	SetShellVarContext all
	Delete $DESKTOP\K-Meleon.lnk
	RMDir /r $SMPROGRAMS\K-Meleon
#	Delete "$QUICKLAUNCH\K-Meleon.lnk"
	WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 0
	Quit

onInit_noHide:	
	StrCmp $1 "/show" 0 onInit_noShow
	SetShellVarContext all
	CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
#	CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
	CreateDirectory "$SMPROGRAMS\K-Meleon"
	CreateShortCut "$SMPROGRAMS\K-Meleon\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortCut "$SMPROGRAMS\K-Meleon\K-Meleon.lnk" "$INSTDIR\k-meleon.exe" "" "$INSTDIR\k-meleon.exe" 0
	WriteRegDWORD HKLM "${PRODUCT_CLIENT_INTERNET_KEY}\InstallInfo" "IconsVisible" 1
	Quit

onInit_noShow:
	IfSilent 0 +3
	IfFileExists "$INSTDIR\k-meleon.exe" +2 
	Quit
FunctionEnd

Function .onInstSuccess
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
FunctionEnd

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