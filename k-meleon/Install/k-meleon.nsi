Name "K-Meleon"
ComponentText "This will install K-Meleon v0.3"
LicenseText "K-Meleon is a GPL product based on Gecko(tm). Please read the license terms below before installing."
LicenseData "license.txt"
OutFile "kmeleon03.exe"
UninstallText "This will uninstall K-Meleon from your computer."
UninstallExeName "Uninstall.exe"
DirText "Please select the directory you want to install K-Meleon in."
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

Section -PostInstall
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
DeleteRegKey HKEY_CURRENT_USER "Software\K-Meleon"

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
