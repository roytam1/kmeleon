# Installer script 1.2 for the K-Meleon webbrowser
# This script requires NSIS 1.9x
# Contribution : René Rhéaume (rener@mediom.qc.ca) for this version only

Name "K-Meleon"
ComponentText "This will install K-Meleon v0.6.5 BETA"
OutFile "kmeleon065.exe"
Icon "kmeleon.ico"

UninstallText "This will uninstall K-Meleon. You need to agree to the following registry changes to completely get rid of it."

LicenseText "K-Meleon is published under the GPL. The Gecko(tm) engine is released under the NPL (as shown in file license.txt)"
LicenseData "GNUlicense.txt"

DirText "Please select the directory where you want to install K-Meleon."
InstallDir "$PROGRAMFILES\K-Meleon"
InstallDirRegKey HKEY_CURRENT_USER "Software\K-Meleon\K-Meleon\General" "InstallDir"
SetOverwrite on
SetCompress auto
SetDatablockOptimize on
ShowInstDetails show
AutoCloseWindow true

InstType Standard

#------------------------------------------------------------------------------------------
Section "K-Meleon (required)"

# Close any running instances
;FindWindow "prompt" "KMeleon" "You must close KMeleon before continuing with the install process"
Call CloseKMeleon

# and copying regular files
SetOutPath "$INSTDIR\uninstall"
File K-MeleonUNINST.ini

SetOutPath "$INSTDIR"
File License.txt
File Readme.htm


File ..\*

SetOutPath "$INSTDIR\chrome"
File ..\chrome\*

SetOutPath "$INSTDIR\components"
File ..\components\*

SetOutPath "$INSTDIR\plugins"
File ..\plugins\*

SetOutPath "$INSTDIR\defaults"
SetOutPath "$INSTDIR\defaults\pref"
File ..\defaults\pref\*

SetOutPath "$INSTDIR\defaults\profile"
File ..\defaults\profile\*

SetOutPath "$INSTDIR\res"
File ..\res\*

SetOutPath "$INSTDIR\res\builtin"
File ..\res\builtin\*

SetOutPath "$INSTDIR\res\fonts"
File ..\res\fonts\*

IfFileExists "$INSTDIR\Profiles" 0 KeepProfiles
MessageBox MB_YESNO "User profiles from a previous installation of K-Meleon have been detected$\n \
These files must be removed for KMeleon to properly function.$\n \
Do you want to remove the stored user profiles?" IDNO KeepProfiles
RMDir /r "$INSTDIR\Profiles"
KeepProfiles:

SectionEnd


#------------------------------------------------------------------------------------------
Section "K-Meleon loader (for faster startup times)"
;SectionIn 1

SetOutPath $INSTDIR
File ..\loader.exe
CreateShortCut "$SMSTARTUP\K-Meleon Loader.lnk" "$INSTDIR\loader.exe"

SectionEnd

#------------------------------------------------------------------------------------------
Section "Stardard Plugins"
SectionIn 1

SetOutPath "$INSTDIR\kplugins"
File ..\kplugins\history.dll
File ..\kplugins\bmpmenu.dll
File ..\kplugins\rebarmenu.dll
File ..\kplugins\fullscreen.dll
File ..\kplugins\macros.dll
File ..\kplugins\toolbars.dll

SectionEnd

#------------------------------------------------------------------------------------------
Section "Bookmarks Plugin (for Netscape users)"
SectionIn 1

SetOutPath "$INSTDIR\kplugins"
File ..\kplugins\bookmarks.dll

SectionEnd

#------------------------------------------------------------------------------------------
Section "IE Favorites Plugin (for IE users)"
SectionIn 1

SetOutPath "$INSTDIR\kplugins"
File ..\kplugins\favorites.dll

SectionEnd

#------------------------------------------------------------------------------------------
Section "Start Menu and Desktop Icons"
SectionIn 1

;SetOutPath "$INSTDIR"
CreateShortCut "$DESKTOP\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$SMPROGRAMS\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0
CreateShortCut "$QUICKLAUNCH\K-Meleon.lnk" "$INSTDIR\K-Meleon.exe" "" "" 0

SectionEnd

#------------------------------------------------------------------------------------------
Section "Always open HTML with K-Meleon"
SectionIn 1

WriteINIStr "$INSTDIR\uninstall\K-MeleonUNINST.ini" "Registry" "Associations" "Yes"

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
WriteRegStr HKCR "K-Meleon.HTML\DefaultIcon" "" "$INSTDIR\K-Meleon.exe,1"
WriteRegStr HKCR "K-Meleon.HTML\shell\open\command" "" '"$INSTDIR\K-Meleon.exe" "%1"'

SectionEnd

#------------------------------------------------------------------------------------------
Section -PostInstall

# Creating uninstaller
Delete "$INSTDIR\Uninstall.exe"
WriteUninstaller "$INSTDIR\Uninstall.exe"

# Adding the Installation directory which can be used to update k-meleon or by plugin installers
WriteRegStr HKCU "Software\K-Meleon\K-Meleon\General" "InstallDir" "$INSTDIR"
# Adding K-Meleon uninstall info
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "DisplayName" "K-Meleon (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon" "UninstallString" '"$INSTDIR\Uninstall.exe"'

# Cleaning up temp files
Delete $TEMP\K-Meleon\*
RMDir $TEMP\K-Meleon

#Exec '"$INSTDIR\loader.exe"'
Exec '"$INSTDIR\k-meleon.exe" "$INSTDIR\ReadMe.htm"'
#ExecShell "open" "$INSTDIR\ReadMe.txt"

SectionEnd

#------------------------------------------------------------------------------------------
# Uninstall part begins here:
Section Uninstall

# Prompt the user to close the program
;FindWindow "prompt" "KMeleon" "You must close K-Meleon before continuing with the install process"
Call un.CloseKMeleon

# and restoring the original file associations
ReadINIStr $0 "$INSTDIR\uninstall\K-MeleonUNINST.ini" "Registry" "Associations"
StrCmp $0 "Yes" RestoreRegistry DontRestoreRegistry

RestoreRegistry:

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

DontRestoreRegistry:

# ask about removing user profiles/settings
MessageBox MB_YESNO "Do you want to remove the stored user profiles?" IDNO KeepProfiles
RMDir /r "$INSTDIR\Profiles"
KeepProfiles:

# Delete the Systems Setting uninstall info key
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\K-Meleon"

# Deleting all the K-Meleon files
RMDir /r $INSTDIR

Delete "$SMPROGRAMS\K-Meleon.lnk"
Delete "$DESKTOP\K-Meleon.lnk"
Delete "$SMSTARTUP\K-Meleon Loader.lnk"
Delete "$QUICKLAUNCH\K-Meleon.lnk"

SectionEnd

;------------------------------------------------------------------- 
; CloseKMeleon
; 
; Closes all running instances of KMeleon 
; 
; modifies no other variables 
Function CloseKMeleon
Push $0 
loop: 
FindWindow $0 "KMeleon" 
IntCmp $0 0 done 
SendMessage $0 16 0 0 
Sleep 100 
Goto loop 
done: 
Pop $0 

FunctionEnd
;------------------------------------------------------------------- 
; un.CloseKMeleon
; 
; Closes all running instances of KMeleon (for Uninstall section)
; 
; modifies no other variables 
Function un.CloseKMeleon
Push $0 
loop: 
FindWindow $0 "KMeleon" 
IntCmp $0 0 done 
SendMessage $0 16 0 0 
Sleep 100 
Goto loop 
done: 
Pop $0 
FunctionEnd

