Name K-Meleon
ComponentText "This will install K-Meleon v0.2.1."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
LicenseText "K-Meleon is a GPL product based on Gecko(tm). Please read the license terms below before installing."
LicenseData license.txt
OutFile kmeleon021.exe
InstallDir $PROGRAMFILES\K-Meleon
EnabledBitmap checked.bmp
DisabledBitmap unchecked.bmp

InstType Standard

Section "K-Meleon (required)"

SetOutPath $INSTDIR
SetOverwrite on
Delete $INSTDIR\*
Delete $INSTDIR\components\*
File k-meleon.exe
File *.dll
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
SetOutPath $INSTDIR\res\entityTables
File res\entityTables\*
SetOutPath $INSTDIR\res\gfx
File res\gfx\*
SetOutPath $INSTDIR\res\html
File res\html\*

Section "Start Menu and Desktop Icons"
SectionIn 1
CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$STARTMENU\Programs\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0

Section -PostInstall
DeleteRegValue HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\Settings\BCGToolBar-593980" "Buttons"
DeleteRegValue HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\Settings\BCGToolBar-593980" "Name"
DeleteRegValue HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\Settings\BCGToolBar-59395" "Buttons"
DeleteRegValue HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\Settings\BCGToolBar-59395" "Name"
DeleteRegKey HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\Settings\BCGToolBar-59395"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" '"$INSTDIR\nsuninst.exe"'
Exec '"$WINDIR\notepad.exe" "$INSTDIR\ReadMe.txt"'
;Exec $INSTDIR\K-Meleon.exe

Section Uninstall
DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString"
DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName"
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
Delete $INSTDIR\res\html\*
RMDir $INSTDIR\res\html
Delete $INSTDIR\res\gfx\*
RMDir $INSTDIR\res\gfx
Delete $INSTDIR\res\entitytables\*
RMDir $INSTDIR\res\entitytables
Delete $INSTDIR\res\*
RMDir $INSTDIR\res
Delete $INSTDIR\*
RMDir $INSTDIR

Delete "$STARTMENU\Programs\K-Meleon.lnk"
Delete "$DESKTOP\K-Meleon.lnk"
Delete "$QUICKLAUNCH\K-Meleon.lnk"
