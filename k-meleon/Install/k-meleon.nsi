Name "K-Meleon"
ComponentText "This will install K-Meleon v0.3.1"
LicenseText "K-Meleon is published under the GPL. The Gecko(tm) engine is released under the NPL (as shown in file license.txt)"
LicenseData "GNUlicense.txt"
OutFile "kmeleon031.exe"
UninstallText "This will uninstall K-Meleon from your computer."
UninstallExeName "Uninstall.exe"
DirText "Please select the directory you want to install K-Meleon in."
InstallDirRegKey HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\General" "InstallDir"
InstallDir "$PROGRAMFILES\K-Meleon"
EnabledBitmap yes.bmp
DisabledBitmap no.bmp
SetOverwrite on

InstType Standard

Section "K-Meleon (required)"

# delete previous kmeleon instances
# First trying to shut down running instances, the Window class is called: Afx:400000:0
# but a couple of other programs use that, so we'll leave it for now
#FindWindow "close" "Afx:400000:0" ""

SetOutPath $INSTDIR
File k-meleon.exe
File *.dll
File *.cfg
File ReadMe.txt
File License.txt


SetOutPath $INSTDIR\chrome
File chrome\*
SetOutPath $INSTDIR\components
File components\*
SetOutPath $INSTDIR\defaults
SetOutPath $INSTDIR\defaults\pref
File defaults\pref\*
SetOutPath $INSTDIR\plugins
File plugins\*
SetOutPath $INSTDIR\res
File res\*
SetOutPath $INSTDIR\res\builtin
File res\builtin\*

Section "Start Menu and Desktop Icons"
SectionIn 1
CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$SMPROGRAMS\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0

Section "Always open HTML by K-Meleon"
SectionIn 2
# maybe we should save the old values somewhere and restore them on uninstall?
WriteRegStr HKEY_CLASSES_ROOT ".htm" "" "K-Meleon.HTML"
WriteRegStr HKEY_CLASSES_ROOT ".htm" "Content Type" "text/html"
WriteRegStr HKEY_CLASSES_ROOT ".html" "" "K-Meleon.HTML"
WriteRegStr HKEY_CLASSES_ROOT ".html" "Content Type" "text/html"

WriteRegStr HKEY_CLASSES_ROOT "K-Meleon.HTML" "" Hypertext Markup Language Document"
WriteRegStr HKEY_CLASSES_ROOT "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\k-meleon.exe" "%1"'


Section -PostInstall
# Adding the Installation directory which can be used to update k-meleon or by plugin installers
WriteRegStr HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\General" "InstallDir" "$INSTDIR"
# Adding uninstall info
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" '"$INSTDIR\Uninstall.exe"'
;Exec '"$WINDIR\notepad.exe" "$INSTDIR\ReadMe.txt"'
Exec '"$INSTDIR\K-Meleon.exe" "$INSTDIR\ReadMe.txt"'


#------------------------------------
# Uninstall part begins here:
Section Uninstall

# First trying to shut down running instances, the Window class is called: Afx:400000:0
# but a couple of other programs use that, so we'll leave it for now
;FindWindow "close" "Afx:400000:0" ""
# delete all registry entries that Kmeleon does on install
# optionally we'll need to remove .html file association if K-Meleon had registered itself
;DeleteRegKey HKEY_CLASSES_ROOT "K-Meleon.HTML"

# No need to remove the seingle entries before deleting the whole key???
;DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString"
;DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName"
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon"

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
Delete $INSTDIR\*
RMDir $INSTDIR

Delete "$SMPROGRAMS\K-Meleon.lnk"
Delete "$DESKTOP\K-Meleon.lnk"
Delete "$QUICKLAUNCH\K-Meleon.lnk"
