# Installer script 1.1v for the K-Meleon webbrowser
# This script requires a custom build of nsis to support the APPDATADIR variable

Name "K-Meleon"
ComponentText "This will install K-Meleon v0.4"
OutFile "kmeleon04.exe"

UninstallText "This will uninstall K-Meleon. You need to agree to the following registry changes to completely get rid of it."
UninstallExeName "Uninstall.exe"

LicenseText "K-Meleon is published under the GPL. The Gecko(tm) engine is released under the NPL (as shown in file license.txt)"
LicenseData "GNUlicense.txt"

DirText "Please select the directory where you want to install K-Meleon."
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
#SetOutPath "$TEMP\K-Meleon"
#File concat.exe
# and copying regular files
SetOutPath "$INSTDIR\uninstall"
File K-MeleonUNINST.ini

SetOutPath "$INSTDIR"
File ..\*

SetOutPath $INSTDIR\chrome
File ..\chrome\*

SetOutPath $INSTDIR\components
File ..\components\*

SetOutPath $INSTDIR\defaults
SetOutPath $INSTDIR\defaults\pref
File ..\defaults\pref\*

SetOutPath $INSTDIR\defaults\profile
File ..\defaults\profile\*

SetOutPath $INSTDIR\plugins
File ..\plugins\*

SetOutPath $INSTDIR\res
File ..\res\*

SetOutPath $INSTDIR\res\builtin
File ..\res\builtin\*

IfFileExists $APPDATADIR\KMeleon 0 KeepProfiles
MessageBox MB_YESNO "User profiles from a previous installation of K-Meleon have been detected$\n \
These files must be removed for KMeleon to properly function.$\n \
Do you want to remove the stored user profiles?" IDNO KeepProfiles
RMDir /r $APPDATADIR\KMeleon
KeepProfiles:

SectionEnd

#------------------------------------------------------------------------------------------
Section "Start Menu and Desktop Icons"
SectionIn 1

SetOutPath "$INSTDIR"
CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$SMPROGRAMS\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0

SectionEnd

#------------------------------------------------------------------------------------------
Section "Always open HTML with K-Meleon"
SectionIn 1,2

ReadRegStr $0 HKCR ".htm" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".htm" $0
WriteRegStr HKCR ".htm" "" "K-Meleon.HTML"

ReadRegStr $0 HKCR ".htm" "Content Type"
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".htm\Content Type" $0
WriteRegStr HKCR ".htm" "Content Type" "text/html"

ReadRegStr $0 HKCR ".html" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".html" $0
WriteRegStr HKCR ".html" "" "K-Meleon.HTML"

ReadRegStr $0 HKCR ".html" "Content Type"
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".html\Content Type" $0
WriteRegStr HKCR ".html" "Content Type" "text/html"

ReadRegStr $0 HKCR "http" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" "http" $0
WriteRegStr HKCR "http" "" "URL:HTTP (HyperText Transfer-Protocol)"

ReadRegStr $0 HKCR "http\shell\open\command" ""
WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" "http\shell\open\command" $0
WriteRegStr HKCR "http\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

WriteRegStr HKCR "K-Meleon.HTML" "" "Hypertext Markup Language Document"
WriteRegStr HKCR "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

SectionEnd

#------------------------------------------------------------------------------------------
Section -PostInstall

# Adding the Installation directory which can be used to update k-meleon or by plugin installers
WriteRegStr HKCU "Software\K-Meleon\K-Meleon\General" "InstallDir" "$INSTDIR"
# Adding K-Meleon uninstall info
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" '"$INSTDIR\Uninstall.exe"'

# Cleaning up temp files
Delete $TEMP\K-Meleon\*
RMDir $TEMP\K-Meleon

;Exec '"$INSTDIR\k-meleon.exe" "$INSTDIR\ReadMe.txt"'
ExecShell "open" "$INSTDIR\ReadMe.txt"

SectionEnd

#------------------------------------------------------------------------------------------
# Uninstall part begins here:
Section Uninstall

# First trying to shut down running instances, the Window class is called: Afx:400000:0
# but a couple of other programs use that, so we'll leave it for now
;FindWindow "close" "Afx:400000:0" ""

# delete all registry entries that Kmeleon does on install
#ExecWait 'regedit /s "$INSTDIR\uninstall\K-MeleonUNINST.reg"'

# and restoring the original file associations
ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".htm"
WriteRegStr HKCR ".htm" "" $0

ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".htm\Content Type"
WriteRegStr HKCR ".htm" "Content Type" $0

ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".html"
WriteRegStr HKCR ".html" "" $0

ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" ".html\Content Type"
WriteRegStr HKCR ".html" "Content Type" $0

ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" "http"
WriteRegStr HKCR "http" "" $0

ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "HKCR" "http\shell\open\command"
WriteRegStr HKCR "http\shell\open\command" "" $0

DeleteRegKey HKCR "K-Meleon.HTML"

# ask about removing user profiles/settings
MessageBox MB_YESNO "Do you want to remove the stored user profiles?" IDNO KeepProfiles
RMDir /r $APPDATADIR\KMeleon
KeepProfiles:

# Delete the Systems Setting uninstall info key
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon"

# Deleting all the K-Meleon files
RMDir /r $INSTDIR

Delete "$SMPROGRAMS\K-Meleon.lnk"
Delete "$DESKTOP\K-Meleon.lnk"
Delete "$QUICKLAUNCH\K-Meleon.lnk"

SectionEnd