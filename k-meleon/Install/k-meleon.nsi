# Installer script 1.1h for the K-Meleon webbrowser, please note that it uses feature
# of the Winamp SuperPimp installer >1.1h

# comment next line out to produce a "real" installer, otherwise you'll get a dummy
#!define debug

Name "K-Meleon"
ComponentText "This will install K-Meleon v0.3.1"
OutFile "kmeleon031.exe"

UninstallText "This will uninstall K-Meleon. You need to agree to the following registry changes to completely get rid of it."
UninstallExeName "Uninstall.exe"

LicenseText "K-Meleon is published under the GPL. The Gecko(tm) engine is released under the NPL (as shown in file license.txt)"
LicenseData "GNUlicense.txt"

DirText "Please select the directory you want to install K-Meleon in."
InstallDir "$PROGRAMFILES\K-Meleon"
InstallDirRegKey HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\General" "InstallDir"
EnabledBitmap yes.bmp
DisabledBitmap no.bmp
SetOverwrite on

InstType Standard

#------------------------------------------------------------------------------------------
Section "K-Meleon (required)"

# First trying to shut down running instances, the Window class is called: Afx:400000:0
# but a couple of other programs use that (ICQ), so we'll better leave it for now
;FindWindow "close" "Afx:400000:0" ""

# Copying temp files
SetOutPath "$TEMP\K-Meleon"
File concat.exe
# and copying regular files
SetOutPath "$INSTDIR\uninstall"
File K-MeleonUNINST.reg

SetOutPath "$INSTDIR"
File ..\ReadMe.txt
File ..\License.txt
File ..\k-meleon.exe
!ifndef debug
File ..\*.dll
File ..\*.cfg

SetOutPath $INSTDIR\chrome
File ..\chrome\*
SetOutPath $INSTDIR\components
File ..\components\*
SetOutPath $INSTDIR\defaults
SetOutPath $INSTDIR\defaults\pref
File ..\defaults\pref\*
SetOutPath $INSTDIR\plugins
File ..\plugins\*
SetOutPath $INSTDIR\res
File ..\res\*
SetOutPath $INSTDIR\res\builtin
File ..\res\builtin\*
!endif

#------------------------------------------------------------------------------------------
Section "Start Menu and Desktop Icons"
SectionIn 1

CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$SMPROGRAMS\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0

#------------------------------------------------------------------------------------------
Section "Always open HTML by K-Meleon"
SectionIn 1,2
# save the old values somewhere and restore them on uninstall
ExecWait 'regedit /e "$TEMP\K-Meleon\RESTOREhtm.tmp" HKEY_CLASSES_ROOT\.htm'
ExecWait 'regedit /e "$TEMP\K-Meleon\RESTOREhtml.tmp" HKEY_CLASSES_ROOT\.html'
ExecWait 'regedit /e "$TEMP\K-Meleon\RESTOREhttp.tmp" HKEY_CLASSES_ROOT\http'
ExecWait '"$TEMP\K-Meleon\concat.exe" "$TEMP\K-Meleon\RESTOREhtm.tmp" "$TEMP\K-Meleon\RESTOREfileAsso.reg"'
ExecWait '"$TEMP\K-Meleon\concat.exe" "$TEMP\K-Meleon\RESTOREhtml.tmp" "$TEMP\K-Meleon\RESTOREfileAsso.reg"'
ExecWait '"$TEMP\K-Meleon\concat.exe" "$TEMP\K-Meleon\RESTOREhttp.tmp" "$TEMP\K-Meleon\RESTOREfileAsso.reg"'
# -m: Do not mov when destination already exists
ExecWait '"$TEMP\K-Meleon\concat.exe" "$TEMP\K-Meleon\RESTOREfileAsso.reg" "$INSTDIR\uninstall\RESTOREfileAsso.reg" -m'

WriteRegStr HKEY_CLASSES_ROOT ".htm" "" "K-Meleon.HTML"
WriteRegStr HKEY_CLASSES_ROOT ".htm" "Content Type" "text/html"
WriteRegStr HKEY_CLASSES_ROOT ".html" "" "K-Meleon.HTML"
WriteRegStr HKEY_CLASSES_ROOT ".html" "Content Type" "text/html"
WriteRegStr HKEY_CLASSES_ROOT "K-Meleon.HTML" "" "Hypertext Markup Language Document"
WriteRegStr HKEY_CLASSES_ROOT "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'
WriteRegStr HKEY_CLASSES_ROOT "http" "" "URL:HTTP (HyperText Transfer-Protokoll)"
WriteRegStr HKEY_CLASSES_ROOT "http\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

# Uninstall information
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.reg" "-HKEY_CURRENT_USER\Software\K-Meleon" "" ""

WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.reg" "-HKEY_CLASSES_ROOT\.htm" "" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.reg" "-HKEY_CLASSES_ROOT\.html" "" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.reg" "-HKEY_CLASSES_ROOT\K-Meleon.HTML" "" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.reg" "-HKEY_CLASSES_ROOT\http" "" ""

#------------------------------------------------------------------------------------------
Section -PostInstall
# Adding the Installation directory which can be used to update k-meleon or by plugin installers
WriteRegStr HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\General" "InstallDir" "$INSTDIR"
# Adding K-Meleon uninstall info
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" '"$INSTDIR\Uninstall.exe"'

# Cleaning up temp files
Delete $TEMP\K-Meleon\*
RMDir $TEMP\K-Meleon

;Exec '"$INSTDIR\k-meleon.exe" "$INSTDIR\ReadMe.txt"'
ExecShell "open" "$INSTDIR\ReadMe.txt"


#------------------------------------------------------------------------------------------
# Uninstall part begins here:
Section Uninstall

# First trying to shut down running instances, the Window class is called: Afx:400000:0
# but a couple of other programs use that, so we'll leave it for now
;FindWindow "close" "Afx:400000:0" ""

# delete all registry entries that Kmeleon does on install
ExecWait 'regedit /s "$INSTDIR\uninstall\K-MeleonUNINST.reg"'

# and restoring the original file associations
MessageBox MB_YESNO "Should we restore the original file associations as default browser?" IDNO 1
ExecWait "regedit /s $INSTDIR\uninstall\RESTOREfileAsso.reg"

!ifndef debug
# Delete the Systems Setting uninstall info key
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon"
!endif

!ifndef debug
# Deleting all the K-Meleon files
Delete $INSTDIR\chrome\*
RMDir $INSTDIR\chrome
Delete $INSTDIR\components\*
RMDir $INSTDIR\components
Delete $INSTDIR\defaults\pref\*
RMDir $INSTDIR\defaults\pref
RMDir $INSTDIR\defaults
Delete $INSTDIR\plugins\*
RMDir $INSTDIR\plugins
Delete $INSTDIR\res\builtin\*
RMDir $INSTDIR\res\builtin
Delete $INSTDIR\res\*
RMDir $INSTDIR\res
Delete $INSTDIR\uninstall\*
RMDir $INSTDIR\uninstall
Delete $INSTDIR\*
RMDir $INSTDIR

Delete "$SMPROGRAMS\K-Meleon.lnk"
Delete "$DESKTOP\K-Meleon.lnk"
Delete "$QUICKLAUNCH\K-Meleon.lnk"
!endif