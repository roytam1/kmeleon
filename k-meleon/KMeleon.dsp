# Microsoft Developer Studio Project File - Name="kmeleon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=kmeleon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "KMeleon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "KMeleon.mak" CFG="kmeleon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kmeleon - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "kmeleon - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kmeleon - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "../mozilla/mozilla/dist/include/chrome" /I "../mozilla/mozilla/dist/include/imglib2" /I "../mozilla/mozilla/dist/include/windowwatcher" /I "../mozilla/mozilla/dist/include/mimetype" /I "../mozilla/mozilla/dist/include/locale" /I "../mozilla/mozilla/dist/include/chardet" /I "../mozilla/mozilla/dist/include/content" /I "../mozilla/mozilla/dist/include/caps" /I "../mozilla/mozilla/dist/include/xpconnect" /I "../mozilla/mozilla/dist/include/js" /I "../mozilla/mozilla/dist/include/pipboot" /I "../mozilla/mozilla/dist/include/pippki" /I "../mozilla/mozilla/dist/include/pipnss" /I "../mozilla/mozilla/dist/include/cookie" /I "../mozilla/mozilla/dist/include/xpcom_obsolete" /I "../mozilla/mozilla/dist/include/typeaheadfind" /I "../mozilla/mozilla/dist/include/appcomps" /I "../mozilla/mozilla/dist/include/history" /I "../mozilla/mozilla/dist/include/rdf" /I "../mozilla/mozilla/dist/include/docshell" /I "../mozilla/mozilla/dist/include/dom" /I "../mozilla/mozilla/dist/include/embed_base" /I "../mozilla/mozilla/dist/include/exthandler" /I "../mozilla/mozilla/dist/include/find" /I "../mozilla/mozilla/dist/include/gfx" /I "../mozilla/mozilla/dist/include/helperAppDlg" /I "../mozilla/mozilla/dist/include/intl" /I "../mozilla/mozilla/dist/include/layout" /I "../mozilla/mozilla/dist/include/necko" /I "../mozilla/mozilla/dist/include/nkcache" /I "../mozilla/mozilla/dist/include/pref" /I "../mozilla/mozilla/dist/include/profile" /I "../mozilla/mozilla/dist/include/shistory" /I "../mozilla/mozilla/dist/include/string" /I "../mozilla/mozilla/dist/include/uriloader" /I "../mozilla/mozilla/dist/include/wallet" /I "../mozilla/mozilla/dist/include/webBrowser_core" /I "../mozilla/mozilla/dist/include/webbrowserpersist" /I "../mozilla/mozilla/dist/include/webshell" /I "../mozilla/mozilla/dist/include/widget" /I "../mozilla/mozilla/dist/include/xpcom" /I "../mozilla/mozilla/dist/include/nspr" /I "../mozilla/mozilla/dist/include/webbrwsr" /I "../mozilla/mozilla/dist/include/commandhandler" /I "../mozilla/mozilla/dist/include/plugin" /D _WIN32_IE=0x0500 /D "XPCOM_GLUE" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /Fr /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 xpcomglue.lib embed_base_s.lib nspr4.lib plc4_s.lib /nologo /subsystem:windows /machine:I386 /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /libpath:"\projects\mozilla\mozilla\dist\WIN32_o.OBJ\lib" /libpath:"c:\projects\mozilla\mozilla\dist\win32_o.obj\lib" /libpath:"..\mozilla\mozilla\dist\WIN32_o.OBJ\lib" /libpath:"..\mozilla\mozilla\dist\lib" /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../mozilla/mozilla/dist/include/chrome" /I "../mozilla/mozilla/dist/include/imglib2" /I "../mozilla/mozilla/dist/include/windowwatcher" /I "../mozilla/mozilla/dist/include/mimetype" /I "../mozilla/mozilla/dist/include/locale" /I "../mozilla/mozilla/dist/include/pippki" /I "../mozilla/mozilla/dist/include/pipnss" /I "../mozilla/mozilla/dist/include/cookie" /I "../mozilla/mozilla/dist/include/xpcom_obsolete" /I "../mozilla/mozilla/dist/include/typeaheadfind" /I "../mozilla/mozilla/dist/include/appcomps" /I "../mozilla/mozilla/dist/include/history" /I "../mozilla/mozilla/dist/include/rdf" /I "../mozilla/mozilla/dist/include/webbrwsr" /I "../mozilla/mozilla/dist/include/commandhandler" /I "../mozilla/mozilla/dist/include/plugin" /I "../mozilla/mozilla/dist/include/docshell" /I "../mozilla/mozilla/dist/include/dom" /I "../mozilla/mozilla/dist/include/embed_base" /I "../mozilla/mozilla/dist/include/exthandler" /I "../mozilla/mozilla/dist/include/find" /I "../mozilla/mozilla/dist/include/gfx" /I "../mozilla/mozilla/dist/include/helperAppDlg" /I "../mozilla/mozilla/dist/include/intl" /I "../mozilla/mozilla/dist/include/layout" /I "../mozilla/mozilla/dist/include/necko" /I "../mozilla/mozilla/dist/include/nkcache" /I "../mozilla/mozilla/dist/include/pref" /I "../mozilla/mozilla/dist/include/profile" /I "../mozilla/mozilla/dist/include/shistory" /I "../mozilla/mozilla/dist/include/string" /I "../mozilla/mozilla/dist/include/uriloader" /I "../mozilla/mozilla/dist/include/wallet" /I "../mozilla/mozilla/dist/include/webBrowser_core" /I "../mozilla/mozilla/dist/include/webbrowserpersist" /I "../mozilla/mozilla/dist/include/webshell" /I "../mozilla/mozilla/dist/include/widget" /I "../mozilla/mozilla/dist/include/xpcom" /I "../mozilla/mozilla/dist/include/nspr" /D "_DEBUG" /D "XPCOM_GLUE" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 xpcomglue.lib embed_base_s.lib nspr4.lib plc4_s.lib /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"MSVCRT" /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /pdbtype:sept /libpath:"..\mozilla\mozilla\dist\lib"
# SUBTRACT LINK32 /profile /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "kmeleon - Win32 Release"
# Name "kmeleon - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AccelParser.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserFrameGlue.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserImplCtxMenuLstnr.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserImplWebPrgrsLstnr.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserView.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserViewFind.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserViewPanning.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserViewUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdLine.cpp
# End Source File
# Begin Source File

SOURCE=.\Components.cpp
# End Source File
# Begin Source File

SOURCE=.\CookiePromptService.cpp
# End Source File
# Begin Source File

SOURCE=.\CookiesViewerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\defineMap.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\FavIconList.cpp
# End Source File
# Begin Source File

SOURCE=.\FontPackageHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\GenKeyPairDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\HiddenWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\LangParser.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuParser.cpp
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.cpp
# End Source File
# Begin Source File

SOURCE=.\MostRecentUrls.cpp
# End Source File
# Begin Source File

SOURCE=.\MozUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\nsGenericFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\NSSDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\NSSDialogs.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\PasswordViewerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Permissions.cpp
# End Source File
# Begin Source File

SOURCE=.\Plugins.cpp
# End Source File
# Begin Source File

SOURCE=.\Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\PreferencesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PrintProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\PrintSetupDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ProfileMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\ProfilesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PromptService.cpp
# End Source File
# Begin Source File

SOURCE=.\ReBarEx.cpp
# End Source File
# Begin Source File

SOURCE=.\SideBar.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "kmeleon - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Debug"

# ADD CPP /Yc

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.cpp
# End Source File
# Begin Source File

SOURCE=.\Tooltips.cpp
# End Source File
# Begin Source File

SOURCE=.\TooltipsProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\UnknownContentTypeHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\urlbar.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\winEmbedFileLocProvider.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\AccelParser.h
# End Source File
# Begin Source File

SOURCE=.\BrowserFrm.h
# End Source File
# Begin Source File

SOURCE=.\BrowserImpl.h
# End Source File
# Begin Source File

SOURCE=.\BrowserView.h
# End Source File
# Begin Source File

SOURCE=.\CmdLine.h
# End Source File
# Begin Source File

SOURCE=.\Dialogs.h
# End Source File
# Begin Source File

SOURCE=.\HiddenWnd.h
# End Source File
# Begin Source File

SOURCE=.\IBrowserFrameGlue.h
# End Source File
# Begin Source File

SOURCE=.\kmeleon_plugin.h
# End Source File
# Begin Source File

SOURCE=.\KmeleonConst.h
# End Source File
# Begin Source File

SOURCE=.\Log.h
# End Source File
# Begin Source File

SOURCE=.\MenuParser.h
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.h
# End Source File
# Begin Source File

SOURCE=.\Parser.h
# End Source File
# Begin Source File

SOURCE=.\Plugins.h
# End Source File
# Begin Source File

SOURCE=.\Preferences.h
# End Source File
# Begin Source File

SOURCE=.\PrintProgressDialog.h
# End Source File
# Begin Source File

SOURCE=.\PrintSetupDialog.h
# End Source File
# Begin Source File

SOURCE=.\ProfileMgr.h
# End Source File
# Begin Source File

SOURCE=.\ProfilesDlg.h
# End Source File
# Begin Source File

SOURCE=.\PromptService.h
# End Source File
# Begin Source File

SOURCE=.\ReBarEx.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.h
# End Source File
# Begin Source File

SOURCE=.\ToolBarWrapper.h
# End Source File
# Begin Source File

SOURCE=.\Tooltips.h
# End Source File
# Begin Source File

SOURCE=.\UnknownContentTypeHandler.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\winEmbedFileLocProvider.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\back.bmp
# End Source File
# Begin Source File

SOURCE=.\res\broken.ico
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor2.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_10.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_11.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_12.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_13.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_14.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_15.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_16.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_17.cur
# End Source File
# Begin Source File

SOURCE=.\res\Cursor_27.cur
# End Source File
# Begin Source File

SOURCE=.\res\KmeleonDocument.ico
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.rc
# End Source File
# Begin Source File

SOURCE=.\res\MozillaBrowser.ico
# End Source File
# Begin Source File

SOURCE=.\res\off.ico
# End Source File
# Begin Source File

SOURCE=.\res\offcheck.ico
# End Source File
# Begin Source File

SOURCE=.\res\on.ico
# End Source File
# Begin Source File

SOURCE=.\res\oncheck.ico
# End Source File
# Begin Source File

SOURCE=.\res\popupblock.ico
# End Source File
# Begin Source File

SOURCE=.\res\sinsecur.ico
# End Source File
# Begin Source File

SOURCE=.\res\ssecur.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\a.txt
# End Source File
# End Target
# End Project
