# Microsoft Developer Studio Project File - Name="KMeleon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=KMeleon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "KMeleon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "KMeleon.mak" CFG="KMeleon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "KMeleon - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "KMeleon - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "KMeleon - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Bin"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_BCGCONTROLBAR_STATIC_" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "BCGControlBar" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 xpcom.lib baseembed_s.lib appfilelocprovider_s.lib plc4.lib BCGCB474StaticS.lib /nologo /subsystem:windows /machine:I386 /out:"Release/k-meleon.exe" /libpath:"..\mozilla\mozilla\dist\WIN32_O.OBJ\lib" /OPT:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "KMeleon - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Bin"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\mozilla\mozilla\nsprpub\pr\include" /I "..\mozilla\mozilla\nsprpub\_o.obj\include" /I "..\mozilla\mozilla\include" /I "..\mozilla\mozilla\xpcom\components" /I "..\mozilla\mozilla\dist\include" /I "..\mozilla\mozilla\dist\WIN32_O.OBJ\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_BCGCONTROLBAR_STATIC_" /D "HW_THREADS" /D "XP_PC" /D "XP_WIN" /D "XP_WIN32" /D WINVER=0x400 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "BCGControlBar" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 xpcom.lib baseembed_s.lib appfilelocprovider_s.lib plc4.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/k-meleon.exe" /pdbtype:sept /libpath:"..\mozilla\mozilla\dist\WIN32_D.OBJ\lib"

!ENDIF 

# Begin Target

# Name "KMeleon - Win32 Release"
# Name "KMeleon - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\KMeleon.cpp
# End Source File
# Begin Source File

SOURCE=.\KMeleon.rc
# End Source File
# Begin Source File

SOURCE=.\KMeleonDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\LinkButton.cpp
# End Source File
# Begin Source File

SOURCE=.\LinksBar.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainToolBar.cpp
# End Source File
# Begin Source File

SOURCE=.\Mozilla.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\WebBrowserChrome.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\KMeleon.h
# End Source File
# Begin Source File

SOURCE=.\KMeleonDoc.h
# End Source File
# Begin Source File

SOURCE=.\LinkButton.h
# End Source File
# Begin Source File

SOURCE=.\LinksBar.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MainToolBar.h
# End Source File
# Begin Source File

SOURCE=.\Mozilla.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WebBrowserChrome.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\back.bmp
# End Source File
# Begin Source File

SOURCE=.\res\coldtool.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
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

SOURCE=.\res\hottoolb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\KMeleon.ico
# End Source File
# Begin Source File

SOURCE=.\res\KMeleon.rc2
# End Source File
# Begin Source File

SOURCE=.\res\KMeleonDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\menu_img.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MozillaBrowser.ico
# End Source File
# Begin Source File

SOURCE=.\res\TOOL1.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TOOL2.BMP
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\out.avi
# End Source File
# End Target
# End Project
