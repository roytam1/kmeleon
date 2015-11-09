/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * Copyright (C) 2005 Ulf Erikson <ulferikson@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 *  Based on the MiniDump demo by Vladimir Sedach
 *  http://www.codeproject.com/tools/minidump.asp
 */

#include <windows.h>
#include <tlhelp32.h>

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"

#define PLUGIN_NAME "Crash Report Plugin"
#define NO_OPTIONS "This plugin has no user configurable options."

#define _T(x) (x)

#define PREFERENCE_CRASHFILE  _T("kmeleon.plugins.crash.reportFile")

PCHAR Str;
PCHAR Dump_Path;
PCHAR TmpStr;

static LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS* pException);

typedef HANDLE (WINAPI * CREATE_TOOL_HELP32_SNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL (WINAPI * MODULE32_FIRST)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
typedef BOOL (WINAPI * MODULE32_NEXT)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

CREATE_TOOL_HELP32_SNAPSHOT CreateToolhelp32Snapshot_;
MODULE32_FIRST Module32First_;
MODULE32_NEXT Module32Next_;

#define DUMP_SIZE_MAX (8*1024)
#define CALL_TRACE_MAX ((DUMP_SIZE_MAX - (2*1024)) / (MAX_PATH + 40))
#define NL  "\r\n"

BOOL APIENTRY DllMain (
        HANDLE hModule,
        DWORD ul_reason_for_call,
        LPVOID lpReserved)
{
  return TRUE;
}

LRESULT CALLBACK WndProc (
        HWND hWnd, UINT message,
        WPARAM wParam,
        LPARAM lParam);
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void * KMeleonWndProc;

INT Load();
void Config(HWND parent);
void Exit();
LONG DoMessage(LPCTSTR to, LPCTSTR from, LPCTSTR subject, LONG data1, LONG data2);

kmeleonPlugin kPlugin =
{
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};
kmeleonFunctions *kFuncs;

#define lenof(a) (sizeof(a) / sizeof((a)[0]))

// Global variables initialization
BOOL InitGlobals()
{
    const char *dllname[] = { "kernel32.dll", "tlhelp32.dll" };
    HMODULE  hModule;
    int i;

    for ( i = 0; i < lenof( dllname ); ++ i ) {
        hModule = GetModuleHandle(dllname[i]);
        if (hModule) {
            CreateToolhelp32Snapshot_ =
                (CREATE_TOOL_HELP32_SNAPSHOT)GetProcAddress(hModule,
                                                            "CreateToolhelp32Snapshot");
            Module32First_ = (MODULE32_FIRST)GetProcAddress(hModule,
                                                            "Module32First");
            Module32Next_ = (MODULE32_NEXT)GetProcAddress(hModule,
                                                          "Module32Next");

            if (CreateToolhelp32Snapshot_ && Module32First_ && Module32Next_)
                break;

            hModule = NULL;
        }
    }

    Str = (CHAR*) malloc(DUMP_SIZE_MAX * sizeof(CHAR));
    if (Str)
      Str[0] = '\0';
    Dump_Path = (CHAR*) malloc(MAX_PATH * sizeof(CHAR));
    if (Dump_Path) {
        CHAR *cptr = NULL;

        kFuncs->GetPreference(PREF_STRING, PREFERENCE_CRASHFILE, Dump_Path, _T(""));

        if (*Dump_Path) {
            cptr = strrchr(Dump_Path, '\\');
            if (cptr && !*(cptr+1))
                lstrcat(cptr, "k-meleon.crash");
        }
        else {
            GetModuleFileName(NULL, Dump_Path, MAX_PATH);
            cptr = strrchr(Dump_Path, '.');
            lstrcpy(cptr ? cptr+1 : Dump_Path + lstrlen(Dump_Path), cptr ? "crash" : ".crash");
        }
    }
    TmpStr = (CHAR*) malloc(MAX_PATH * sizeof(CHAR));
    if (TmpStr)
      TmpStr[0] = '\0';

    return hModule != NULL;
}

INT Load()
{
    kFuncs = kPlugin.kFuncs;
    if (InitGlobals())
        SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
    return TRUE;
}

void Config(HWND parent)
{
   MessageBox(parent, NO_OPTIONS, PLUGIN_NAME, 0);
}

void Exit()
{
    SetUnhandledExceptionFilter(NULL);
}

LONG DoMessage(LPCTSTR to, LPCTSTR from, LPCTSTR subject, LONG data1, LONG data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Exit") == 0) {
         Exit();
      }
      else return 0;

      return 1;
   }
   return 0;
}

extern "C"
{
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin()
   {
          return &kPlugin;
   }
}






static TCHAR *GetExpectionCodeText(DWORD dwExceptionCode) {
    switch(dwExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION: return _T("ACCESS VIOLATION");
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return _T("ARRAY BOUNDS EXCEEDED");
    case EXCEPTION_BREAKPOINT: return _T("BREAKPOINT");
    case EXCEPTION_DATATYPE_MISALIGNMENT: return _T("DATATYPE MISALIGNMENT");
    case EXCEPTION_FLT_DENORMAL_OPERAND: return _T("FLT DENORMAL OPERAND");
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return _T("FLT DIVIDE BY ZERO");
    case EXCEPTION_FLT_INEXACT_RESULT: return _T("FLT INEXACT RESULT");
    case EXCEPTION_FLT_INVALID_OPERATION: return _T("FLT INVALID OPERATION");
    case EXCEPTION_FLT_OVERFLOW: return _T("FLT OVERFLOW");
    case EXCEPTION_FLT_STACK_CHECK: return _T("FLT STACK CHECK");
    case EXCEPTION_FLT_UNDERFLOW: return _T("FLT UNDERFLOW");
    case EXCEPTION_ILLEGAL_INSTRUCTION: return _T("ILLEGAL INSTRUCTION");
    case EXCEPTION_IN_PAGE_ERROR: return _T("IN PAGE ERROR");
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return _T("INT DIVIDE BY ZERO");
    case EXCEPTION_INT_OVERFLOW: return _T("INT OVERFLOW");
    case EXCEPTION_INVALID_DISPOSITION: return _T("INVALID DISPOSITION");
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return _T("NONCONTINUABLE EXCEPTION");
    case EXCEPTION_PRIV_INSTRUCTION: return _T("PRIV INSTRUCTION");
    case EXCEPTION_SINGLE_STEP: return _T("SINGLE STEP");
    case EXCEPTION_STACK_OVERFLOW: return _T("STACK OVERFLOW");
    case DBG_CONTROL_C : return _T("DBG CONTROL C");
    default:
        return _T("<unkown exception>");
    }
}


BOOL WINAPI Get_Module_By_Ret_Addr(PBYTE Ret_Addr, PCHAR Module_Name, PBYTE & Module_Addr)
{
    MODULEENTRY32 M = {sizeof(M)};
    HANDLE hSnapshot;

    Module_Name[0] = 0;

    if (CreateToolhelp32Snapshot_) {
        hSnapshot = CreateToolhelp32Snapshot_(TH32CS_SNAPMODULE, 0);

        if ((hSnapshot != INVALID_HANDLE_VALUE) && Module32First_(hSnapshot, &M)) {
            do
            {
                if (DWORD(Ret_Addr - M.modBaseAddr) < M.modBaseSize) {
                    lstrcpyn(Module_Name, M.szExePath, MAX_PATH);
                    Module_Addr = M.modBaseAddr;
                    break;
                }
            } while (Module32Next_(hSnapshot, &M));
        }

        CloseHandle(hSnapshot);
    }

    return !!Module_Name[0];
}


int WINAPI Get_Call_Stack(PEXCEPTION_POINTERS pException, PCHAR Str)
{
    PBYTE Module_Addr = 0;
    PBYTE Module_Addr_1;
    int Str_Len;

    typedef struct STACK
    {
        STACK *Ebp;
        PBYTE  Ret_Addr;
        DWORD *Param;
    } STACK, * PSTACK;

    STACK Stack = {0, 0};
    PSTACK Ebp;

    if (pException)
    {
        Stack.Ebp = (PSTACK)pException->ContextRecord->Ebp;
        Stack.Ret_Addr =
            (PBYTE)pException->ExceptionRecord->ExceptionAddress;
        Ebp = &Stack;
    }
    else
        {
            Ebp = (PSTACK)&pException - 1;    //frame addr of Get_Call_Stack()

            // Skip frame of Get_Call_Stack().
            if (!IsBadReadPtr(Ebp, sizeof(PSTACK)))
                Ebp = Ebp->Ebp;        //caller ebp
        }

    Str[0] = 0;
    Str_Len = 0;

    // Trace CALL_TRACE_MAX calls maximum - not to exceed DUMP_SIZE_MAX.
    // Break trace on wrong stack frame.
    for (int Ret_Addr_I = 0;
         (Ret_Addr_I < CALL_TRACE_MAX) && !IsBadReadPtr(Ebp, sizeof(PSTACK)) &&
             !IsBadCodePtr(FARPROC(Ebp->Ret_Addr));
         Ret_Addr_I++, Ebp = Ebp->Ebp)
    {
        // If module with Ebp->Ret_Addr found.
        if (Get_Module_By_Ret_Addr(Ebp->Ret_Addr, TmpStr, Module_Addr_1)) {
            if (Module_Addr_1 != Module_Addr) {
                // Save module's address and full path.
                Module_Addr = Module_Addr_1;
                Str_Len += wsprintf(Str + Str_Len, NL "  %08X  %s",
                                    Module_Addr, TmpStr);
            }

            // Save call offset.
            Str_Len += wsprintf(Str + Str_Len, NL " +%08X", Ebp->Ret_Addr - Module_Addr);

            // Save 5 params of the call. We don't know the real number of params.
            if (pException && !Ret_Addr_I)    //fake frame for exception address
                Str_Len += wsprintf(Str + Str_Len, "  <- FAULT");
            else if (!IsBadReadPtr(Ebp, sizeof(PSTACK)) &&
                     !IsBadReadPtr(Ebp->Param, 5 * sizeof(DWORD))) {
                Str_Len += wsprintf(Str + Str_Len, "  %08X %08X %08X %08X %08X",
                                    Ebp->Param[0], Ebp->Param[1], Ebp->Param[2],
                                    Ebp->Param[3], Ebp->Param[4]);
            }
        }
        else
            Str_Len += wsprintf(Str + Str_Len, NL "%08X", Ebp->Ret_Addr);
    }

    return Str_Len;
}


int WINAPI Get_System_Str(PCHAR Str)
{
    int Str_Len = 0;
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);

    if (siSysInfo.wProcessorArchitecture !=
        PROCESSOR_ARCHITECTURE_UNKNOWN) {
        Str_Len += wsprintf(Str + Str_Len, "Number of processors: %u" NL,
                            siSysInfo.dwNumberOfProcessors);

        Str_Len += wsprintf(Str + Str_Len, "Processor Type:  %s",
                            siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86" :
                            siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ? "IA64" :
                            siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "AMD64" :
                            "<unknown>");

        if (siSysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
            Str_Len += wsprintf(Str + Str_Len, "  Family %u",
                                siSysInfo.wProcessorLevel);
            Str_Len += wsprintf(Str + Str_Len, "  Model %u",
                                siSysInfo.wProcessorRevision / 256);
            Str_Len += wsprintf(Str + Str_Len, "  Stepping %u" NL,
                                siSysInfo.wProcessorRevision % 256);
        }
        else {
            if (siSysInfo.wProcessorLevel)
                Str_Len += wsprintf(Str + Str_Len, "  Level %u",
                                    siSysInfo.wProcessorLevel);
            if (siSysInfo.wProcessorRevision)
                Str_Len += wsprintf(Str + Str_Len, "  Revision %u" NL,
                                    siSysInfo.wProcessorRevision);
        }
    }

    return Str_Len;
}


int WINAPI Get_Version_Str(PCHAR Str)
{
    int Str_Len = 0;
    OSVERSIONINFOEX  V = {sizeof(OSVERSIONINFOEX)};

    if (!GetVersionEx((POSVERSIONINFO)&V)) {
        ZeroMemory(&V, sizeof(V));
        V.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx((POSVERSIONINFO)&V);
    }

    if (V.dwPlatformId != VER_PLATFORM_WIN32_NT)
        V.dwBuildNumber = LOWORD(V.dwBuildNumber);

    if (V.dwMajorVersion == 5) {
        if (V.dwMinorVersion == 0)
            Str_Len += wsprintf(Str, "Windows 2000 Version:  ");
        else if (V.dwMinorVersion == 1)
            Str_Len += wsprintf(Str, "Windows XP %sVersion:  ",
                                V.wSuiteMask & VER_SUITE_PERSONAL ? "(Home Edition) " :
                                "");
        else if (V.dwMinorVersion == 2)
            Str_Len += wsprintf(Str, "Windows Server 2003 Version:  ");
    }
    else if (V.dwMajorVersion == 4) {
        if (V.dwMinorVersion == 0) {
            if (V.dwPlatformId == VER_PLATFORM_WIN32_NT)
                Str_Len += wsprintf(Str, "Windows NT-4 Version:  ");
            else
                Str_Len += wsprintf(Str, "Windows 95 Version:  ");
        }
        else if (V.dwMinorVersion == 10)
            Str_Len += wsprintf(Str, "Windows 98 Version:  ");
        else if (V.dwMinorVersion == 90)
            Str_Len += wsprintf(Str, "Windows ME Version:  ");
    }

    if (!*Str) {
        Str_Len += wsprintf(Str, "Windows:  %d.%d.%d, SP %d.%d" NL,
                            V.dwMajorVersion, V.dwMinorVersion, V.dwBuildNumber,
                            V.wServicePackMajor, V.wServicePackMinor);
    }
    else {
        Str_Len += wsprintf(Str + Str_Len, "%d.%d", V.dwMajorVersion,
                            V.dwMinorVersion);
        if (V.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            Str_Len += wsprintf(Str + Str_Len, " %s", V.szCSDVersion);
        }
        Str_Len += wsprintf(Str + Str_Len, NL);

        if (V.dwBuildNumber) {
            Str_Len += wsprintf(Str + Str_Len, "Current Build:  %d" NL,
                                V.dwBuildNumber);
        }

        if (V.wServicePackMajor) {
            Str_Len += wsprintf(Str + Str_Len, "Service Pack:  %d",
                                V.wServicePackMajor);
            if (V.wServicePackMinor)
                Str_Len += wsprintf(Str + Str_Len, ".%d,",
                                    V.wServicePackMinor);
            Str_Len += wsprintf(Str + Str_Len, NL);
        }

        if (V.wProductType) {
            Str_Len += wsprintf(Str + Str_Len, "Current Type:  %s" NL,
                                V.wProductType == VER_NT_WORKSTATION ? "Workstation" :
                                V.wProductType == VER_NT_DOMAIN_CONTROLLER ? "Domain Controller" :
                                V.wProductType == VER_NT_SERVER ? "Server" :
                                "<unknown>");
        }
    }

    return Str_Len;
}


PCHAR WINAPI Get_Exception_Info(PEXCEPTION_POINTERS pException)
{
    int Str_Len;
    PBYTE Module_Addr;
    HANDLE hFile;
    FILETIME Last_Write_Time;
    FILETIME Local_File_Time;
    DWORD dwFileSize;
    DWORD dwRet;
    SYSTEMTIME T;
    TIME_ZONE_INFORMATION TZI;

    if (!Str)
        return NULL;

    Str_Len = 0;

    Str_Len += wsprintf(Str + Str_Len, NL "K-Meleon Crash Information" NL NL);

    Str_Len += wsprintf(Str + Str_Len, "Application exception occurred:" NL);

    GetLocalTime(&T);
    Str_Len += wsprintf(Str + Str_Len, "  Date:  %02d-%02d-%02d @ %02d:%02d",
                        T.wYear, T.wMonth, T.wDay, T.wHour, T.wMinute);

    dwRet = GetTimeZoneInformation(&TZI);
    if (TZI.Bias && !(TZI.Bias % 60)) {
        Str_Len += wsprintf(Str + Str_Len, " %s%d",
                            TZI.Bias < 0 ? "+" : "-", abs(TZI.Bias) / 60);
    }

    WideCharToMultiByte(CP_OEMCP, 0,
                        dwRet == TIME_ZONE_ID_STANDARD ? TZI.StandardName :
                        dwRet == TIME_ZONE_ID_DAYLIGHT ? TZI.DaylightName :
                        L"", -1, TmpStr, MAX_PATH, NULL, NULL);

    Str_Len += wsprintf(Str + Str_Len, ",  %s" NL, TmpStr);

    Str_Len += wsprintf(Str + Str_Len, "  Exception:  %08X   (",
                        pException->ExceptionRecord->ExceptionCode);
    Str_Len += wsprintf(Str + Str_Len,
                        GetExpectionCodeText(pException->ExceptionRecord->ExceptionCode));
    if (pException->ExceptionRecord->ExceptionCode ==
        EXCEPTION_ACCESS_VIOLATION) {
        Str_Len += wsprintf(Str + Str_Len, ", %s Address: %08X",
                            (pException->ExceptionRecord->ExceptionInformation[0]) ?
                            "Writing" : "Reading",
                            pException->ExceptionRecord->ExceptionInformation[1]);
    }
    Str_Len += wsprintf(Str + Str_Len, ")" NL);

    Str_Len += wsprintf(Str + Str_Len, "  Process:  ");
    GetModuleFileName(NULL, TmpStr, MAX_PATH);
    Str_Len += wsprintf(Str + Str_Len, "%s" NL, TmpStr);

    if (Get_Module_By_Ret_Addr((PBYTE)pException->ExceptionRecord->ExceptionAddress,
                               TmpStr, Module_Addr)) {
        Str_Len += wsprintf(Str + Str_Len, "  Module:  %s" NL, TmpStr);

        Str_Len += wsprintf(Str + Str_Len, "  File Date and Size:  ");
        if ((hFile = CreateFile(TmpStr, GENERIC_READ,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
            if (GetFileTime(hFile, NULL, NULL, &Last_Write_Time)) {
                FileTimeToLocalFileTime(&Last_Write_Time,
                                        &Local_File_Time);
                FileTimeToSystemTime(&Local_File_Time, &T);

                Str_Len += wsprintf(Str + Str_Len, "%02d-%02d-%02d @ %02d:%02d,  ",
                                    T.wYear, T.wMonth, T.wDay, T.wHour, T.wMinute);
            }

            dwFileSize = GetFileSize(hFile,  NULL);
            if (dwFileSize != INVALID_FILE_SIZE)
                Str_Len += wsprintf(Str + Str_Len, "%u bytes",
                                    dwFileSize);
            CloseHandle(hFile);
        }
        Str_Len += wsprintf(Str + Str_Len, NL);
    }
    else {
        Str_Len += wsprintf(Str + Str_Len, "  Exception Addr:  %08X" NL,
                            pException->ExceptionRecord->ExceptionAddress);
    }

    Str_Len += wsprintf(Str + Str_Len, NL "*----> System Information <----*" NL);
    Str_Len += Get_System_Str(Str + Str_Len);
    Str_Len += Get_Version_Str(Str + Str_Len);

    Str_Len += wsprintf(Str + Str_Len, NL "*----> Stack Back Trace <----*");
    Str_Len += wsprintf(Str + Str_Len, NL "ReturnAddr  Param#1  Param#2  Param#3  Param#4  Param#5");
    Str_Len += Get_Call_Stack(pException, Str + Str_Len);

    Str_Len += wsprintf(Str + Str_Len, NL);

    return Str;
}


static LONG __stdcall CrashHandlerExceptionFilter(EXCEPTION_POINTERS* pException)
{
    if (pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (Str && !*Str)
        Str = Get_Exception_Info(pException);

    if (Str && *Str) {
        HANDLE hDump_File;
        DWORD Bytes;

        lstrcat(Str, NL NL);

        hDump_File = CreateFile(Dump_Path,
                                GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hDump_File) {
            WriteFile(hDump_File, Str, lstrlen(Str), &Bytes, NULL);
            CloseHandle(hDump_File);
        }
    }

    if (Str) {
        CHAR *cptr = NULL;
        GetModuleFileName(NULL, TmpStr, MAX_PATH);
        if (*TmpStr)
            cptr = strrchr(TmpStr, '\\');

        wsprintf(Str,
                 "The application %s%shas generated an error and will be closed by Windows." NL NL 
                 "An error log has been written:" NL
                 "    %s" NL NL
                 "You will need to restart the program. If the problem persists, contact the" NL
                 "program vendor. Describe the problem in detail and attach the latest error log." NL NL,
                 cptr ? cptr+1 : *TmpStr ? TmpStr : "",
                 cptr ? " " : *TmpStr ? " " : "", Dump_Path);
        MessageBox(NULL, Str, "Application Error", MB_ICONWARNING | MB_OK);

        // FatalAppExit(0, Str);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
