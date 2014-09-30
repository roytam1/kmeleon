!packhdr		tmp.dat "c:\apps\upx\upx.exe -9 --best --strip-relocs=1 tmp.dat"

!define			NAME "K-Meleon"
!define			VERSION "0.9"

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
  !include "Sections.nsh"

;--------------------------------
;General

  ;Name and file
  Name			"${NAME} ${VERSION}"
  Caption		"${NAME} ${VERSION} - SetDefault"
  OutFile		"SetDefault.exe"

  ;Default installation folder
  InstallDir 		"$PROGRAMFILES\${NAME}"
  
  ;Get installation folder from registry if available
  InstallDirRegKey 	HKEY_CURRENT_USER "Software\${NAME}\${NAME}\General" "InstallDir"

; SetCompressor		bzip2
; SetCompress		auto
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
!define MUI_FINISHPAGE_RUN_PARAMETERS

!define MUI_UNINSTPAGE

; !define MUI_UI			"${NSISDIR}\Contrib\UIs\modern.exe"
; !define MUI_INTERFACE
; !define MUI_SYSTEM
; !define MUI_LANGUAGEFILE_BEGIN

;--------------------------------
;Pages

!define MUI_ICON	"kmeleon.ico"
; !define MUI_UNICON	"kmeleon.ico"
!define MUI_CHECKBITMAP	"${NSISDIR}\Contrib\Icons\modern.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_INSTFILESPAGE_COLORS "000000 FFFFFF" ;Multiple settings

BrandingText		/TRIMRIGHT "${NAME} ${VERSION}"

LicenseBkColor		"ffffff"


!define MUI_WELCOMEPAGE_TITLE "${NAME} ${VERSION} Setup Wizard"
; !define MUI_WELCOMEPAGE_TEXT "Text"


; !define MUI_COMPONENTSPAGE_TEXT_TOP "top"
; !define MUI_COMPONENTSPAGE_TEXT_COMPLIST "complist"
; !define MUI_COMPONENTSPAGE_TEXT_INSTTYPE "insttype"
; !define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_TITLE "desc.title"
; !define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO  "desc.info"


  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------

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

Section "Register K-Meleon as a Mozilla/Gecko product" SecGecko ; {{{

	DetailPrint "Register K-Meleon as a Mozilla/Gecko product"
#	DetailPrint '  HKLM "Software\Mozilla\K-Meleon" "GeckoVer" "1.7.5"'
#	DetailPrint '  HKLM "Software\Mozilla\K-Meleon\bin" "PathToExe" "$INSTDIR"'
#	DetailPrint '  HKLM "Software\Mozilla\K-Meleon\Extensions" "Plugins" "$INSTDIR\Plugins"'
#	DetailPrint '  HKLM "Software\Mozilla\K-Meleon\Extensions" "Components" "$INSTDIR\Components"'
#	DetailPrint '  HKLM "Software\mozilla.org\Mozilla" "CurrentVersion" "1.7.5"'

	WriteRegStr HKLM "Software\Mozilla\K-Meleon" "GeckoVer" "1.7.5"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\bin" "PathToExe" "$INSTDIR"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\Extensions" "Plugins" "$INSTDIR\Plugins"
	WriteRegStr HKLM "Software\Mozilla\K-Meleon\Extensions" "Components" "$INSTDIR\Components"
	WriteRegStr HKLM "Software\mozilla.org\Mozilla" "CurrentVersion" "1.7.5"

SectionEnd ; }}}

# ----------------------------------------------------------------------

SubSection "Set K-Meleon as your default browser" SecAssoc ; {{{
	Section
		SectionIn RO
		WriteRegStr HKCR "K-Meleon.HTML" "" "Hypertext Markup Language Document"
		WriteRegStr HKCR "K-Meleon.HTML\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,1"
		WriteRegStr HKCR "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
	SectionEnd

	SubSection "Protocols" SecAssocProto ; {{{
		Section "http" SecHttp
			DetailPrint "Set K-Meleon as your default application for the http protocol"
			!insertmacro assocProtocol  "http"	"URL:HTTP (HyperText Transfer-Protocol)"
		SectionEnd

		Section "https" SecHttps
			DetailPrint "Set K-Meleon as your default application for the https protocol"
			!insertmacro assocProtocol  "https"	"URL:HTTPS (HyperText Transfer-Protocol with Security)"
		SectionEnd

	SubSectionEnd ; }}}

	SubSection "File types" SecAssocExt ; {{{
		Section ".htm files" SecHtm
			DetailPrint "Set K-Meleon as your default application for .htm files"
			!insertmacro assocExtension ".htm"	"text/html"
		SectionEnd

		Section ".html files" SecHtml
			DetailPrint "Set K-Meleon as your default application for .html files"
			!insertmacro assocExtension ".html"	"text/html"
		SectionEnd

		Section ".shtml files" SecShtml
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


; Modern UI Functions {{{

;; !insertmacro MUI_SYSTEM

LangString DESC_SecGecko			${LANG_ENGLISH} 'These entries in the register are needed for some Netscape/Mozilla plugins to work properly.'
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
LangString DESC_SecStartMenu			${LANG_ENGLISH} "Create a Start Menu shortcut."
LangString DESC_SecQuickLaunch			${LANG_ENGLISH} "Create a Quick Launch shortcut."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecGecko}			$(DESC_SecGecko)
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

!insertmacro MUI_FUNCTION_DESCRIPTION_END
; }}}
