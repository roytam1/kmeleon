# Microsoft Developer Studio Project File - Name="kmeleon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=kmeleon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kmeleon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kmeleon.mak" CFG="kmeleon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kmeleon - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "kmeleon - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "kmeleon - Win32 Profile Release" (based on "Win32 (x86) Application")
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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /D _WIN32_IE=0x0500 /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 xpcom.lib baseembed_s.lib plc4.lib /nologo /subsystem:windows /machine:I386 /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /libpath:"c:\projects\mozilla\mozilla\dist\win32_o.obj\lib" /libpath:"..\mozilla\mozilla\dist\WIN32_o.OBJ\lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Updating Version
PostBuild_Cmds=version BUILD_NUMBER version.h
# End Special Build Tool

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kmeleon___Win32_Debug"
# PROP BASE Intermediate_Dir "kmeleon___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 xpcom.lib baseembed_s.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /pdbtype:sept /libpath:"..\mozilla\mozilla\dist\WIN32_o.OBJ\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Updating Version
PostBuild_Cmds=version BUILD_NUMBER version.h
# End Special Build Tool

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Profile Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kmeleon___Win32_Profile_Release"
# PROP BASE Intermediate_Dir "kmeleon___Win32_Profile_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ProfileRelease"
# PROP Intermediate_Dir "ProfileRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /D _WIN32_IE=0x0500 /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "PROFILE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /D _WIN32_IE=0x0500 /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 xpcom.lib baseembed_s.lib plc4.lib /nologo /subsystem:windows /machine:I386 /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /libpath:"c:\projects\mozilla\mozilla\dist\win32_o.obj\lib" /libpath:"..\mozilla\mozilla\dist\WIN32_o.OBJ\lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 xpcom.lib baseembed_s.lib plc4.lib /nologo /subsystem:windows /profile /map:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.map" /debug /machine:I386 /out:"..\mozilla\mozilla\dist\WIN32_o.OBJ\Embed\k-meleon.exe" /libpath:"c:\projects\mozilla\mozilla\dist\win32_o.obj\lib" /libpath:"..\mozilla\mozilla\dist\WIN32_o.OBJ\lib"

!ENDIF 

# Begin Target

# Name "kmeleon - Win32 Release"
# Name "kmeleon - Win32 Debug"
# Name "kmeleon - Win32 Profile Release"
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

SOURCE=.\BrowserImplPrompt.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserImplWebPrgrsLstnr.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserView.cpp
# End Source File
# Begin Source File

SOURCE=.\defineMap.cpp

!IF  "$(CFG)" == "kmeleon - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Profile Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuParser.cpp
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.cpp
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.rc
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

SOURCE=.\ProfileMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\ProfilesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.cpp
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

SOURCE=.\Dialogs.h
# End Source File
# Begin Source File

SOURCE=.\IBrowserFrameGlue.h
# End Source File
# Begin Source File

SOURCE=.\kmeleon_plugin.h
# End Source File
# Begin Source File

SOURCE=.\MenuEx.h
# End Source File
# Begin Source File

SOURCE=.\MenuParser.h
# End Source File
# Begin Source File

SOURCE=.\MfcEmbed.h
# End Source File
# Begin Source File

SOURCE=.\Plugins.h
# End Source File
# Begin Source File

SOURCE=.\Preferences.h
# End Source File
# Begin Source File

SOURCE=.\ProfileMgr.h
# End Source File
# Begin Source File

SOURCE=.\ProfilesDlg.h
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

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\version.h

!IF  "$(CFG)" == "kmeleon - Win32 Release"

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Updating Version
InputPath=.\version.h

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	version.exe BUILD_NUMBER $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "kmeleon - Win32 Profile Release"

!ENDIF 

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

SOURCE=.\res\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\res\MozillaBrowser.ico
# End Source File
# Begin Source File

SOURCE=.\res\temp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Tool1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Tool2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Tool3.bmp
# End Source File
# End Group
# End Target
# End Project
