/*
*  Copyright (C) 2000 Brian Harris
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "StdAfx.h"
#include <afxtempl.h>

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "AccelParser.h"
#include "Utils.h"
#include "resource.h"

#include "Plugins.h"
#include "kmeleon_plugin.h"

#define BEGIN_VK_TEST if (0){}
#define VK_TEST(KEY)  else if (stricmp(p, #KEY) == 0){ key = VK_##KEY; }



BOOL CALLBACK AccelLogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
   if (uMsg == WM_INITDIALOG) {
      CString *log = (CString *)lParam;
      SetDlgItemText(hwndDlg, IDC_CREDITS, *log);
      SetWindowText(hwndDlg, _T("Accel Log"));
      return true;
   }
   else if (uMsg == WM_COMMAND) {
      if (LOWORD(wParam) == IDOK)
         EndDialog(hwndDlg, 0);
    return true;
  }
  return false;
}

class CAccelLog {
protected:
   CString log;
   int error;
public:
   CAccelLog() {
      error = 0;
   };
   ~CAccelLog() {
      if (error)
         DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, AccelLogProc, (LPARAM)&log);
   };

   void WriteString(const char *string){
      log += string;
   }

   void Error(){
      error = 1;
   }
};

#if 0
#define LOG_1(msg, var1)       \
   if (log) {                  \
      char err[255];            \
      sprintf(err, msg, var1);  \
      log->WriteString(err);    \
      log->WriteString("\r\n"); \
   }

#define LOG_2(msg, var1, var2)      \
   if (log) {                       \
      char err[255];                 \
      sprintf(err, msg, var1, var2); \
      log->WriteString(err);         \
      log->WriteString("\r\n");      \
   }
#else
#define LOG_1(a, b)
#define LOG_2(a, b, c)
#endif

#define LOG_ERROR_1(msg, var1)   \
   if (log) {                     \
      log->Error();               \
      char err[255];              \
      sprintf(err, msg, var1);    \
      log->WriteString("*** ");    \
      log->WriteString(err);      \
      log->WriteString("\r\n");   \
   }

#define LOG_ERROR_2(msg, var1, var2)   \
   if (log) {                     \
      log->Error();               \
      char err[255];              \
      sprintf(err, msg, var1, var2);    \
      log->WriteString("*** ");    \
      log->WriteString(err);      \
      log->WriteString("\r\n");   \
   }


CAccelParser::CAccelParser(){
   accelTable = NULL;
   numAccelerators = 0;
   memset(accelerators, 0, sizeof(ACCEL) * MAX_ACCEL);
}

CAccelParser::CAccelParser(CString &filename){
   accelTable = NULL;
   numAccelerators = 0;
   memset(accelerators, 0, sizeof(ACCEL) * MAX_ACCEL);

   Load(filename);
}

CAccelParser::~CAccelParser(){
   if (accelTable){
      DestroyAcceleratorTable(accelTable);
   }
}

int CAccelParser::Load(CString &filename){
   if (accelTable){
      DestroyAcceleratorTable(accelTable);
      accelTable = NULL;
      numAccelerators = 0;
   }

   CFile *accelFile;
   TRY {
      accelFile = new CFile(filename, CFile::modeRead);
   }
   CATCH (CFileException, e) {
      accelFile = NULL;
      return 0;
   }
   END_CATCH

   CAccelLog *log;
   TRY {
      log = new CAccelLog(); //("accel.log", CFile::modeWrite | CFile::modeCreate);
      log->WriteString("Accel File: ");
      log->WriteString(filename);
      log->WriteString("\r\n\r\n");
   }
   CATCH (CFileException, e) {
      log = NULL;
   }
   END_CATCH

   long length = accelFile->GetLength();
   char *buffer = new char[length + 1];
   accelFile->Read(buffer, length);
   // force the terminating 0
   buffer[length] = 0;

   accelFile->Close();
   delete accelFile;
   accelFile = NULL;

   CMap<CString, LPCSTR, int, int &> defineMap;


#include "defineMap.cpp"

   char *e, *s;
   char *alt, *ctrl, *shift;
   BYTE virt;
   int command;
   int key;
   char *p = strtok(buffer, "\r\n");
   while (p){
      if (p[0] == '#'){
      }
      else {
         // <modifiers> <key> = <command>
         e = strchr(p, '=');
         if (e){
            *e = 0;
            e++;
            e = SkipWhiteSpace(e);
            TrimWhiteSpace(e);

            char *op = strchr(e, '(');
            if (op) {                        // if there's an open parenthesis, we'll assume it's a plugin
               char *parameter = op + 1;
               char *cp = strrchr(parameter, ')');
               if (cp) *cp = 0;
               *op = 0;

               TrimWhiteSpace(e);

               kmeleonPlugin * kPlugin = theApp.plugins.Load(e);

               if (kPlugin) {
                  if (kPlugin->pf->DoAccel) {
                     command = kPlugin->pf->DoAccel(parameter);
                     LOG_2("Called plugin %s with parameter %s", e, parameter);
                  }
                  else
                     LOG_ERROR_2( "Plugin %s has no accelerator %s", e, parameter);
               }
               else
                  LOG_ERROR_2( "Could not load plugin %s\r\n\twith parameter %s", e, parameter );
            }
            
            else if (!defineMap.Lookup(e, command)) {
               command = atoi(e);
            }

            TrimWhiteSpace(p);
            s = p;

            virt = 0;
            alt = strstr(s, "ALT");
            if (alt){
               virt |= FALT;
               p = alt + 3;
            }
            ctrl = strstr(s, "CTRL");
            if (ctrl){
               virt |= FCONTROL;
               if (ctrl > alt){
                  p = ctrl + 4;
               }
            }
            shift = strstr(s, "SHIFT");
            if (shift){
               virt |= FSHIFT;
               if ((shift > alt) && (shift > ctrl)){
                  p = shift + 5;
               }
            }
            virt |= FVIRTKEY;
            // by now, p should be past the modifiers and point to " <key>"
            p = SkipWhiteSpace(p);

            if (strncmp(p, "VK_", 3) == 0){
               p+=3;
               key = 0;

               // these should be in order of frequency of use to speed up parsing
               BEGIN_VK_TEST

                  VK_TEST(ESCAPE)
                  VK_TEST(LEFT)
                  VK_TEST(RIGHT)

                  VK_TEST(F1)
                  VK_TEST(F2)
                  VK_TEST(F3)
                  VK_TEST(F4)
                  VK_TEST(F5)
                  VK_TEST(F6)
                  VK_TEST(F7)
                  VK_TEST(F8)
                  VK_TEST(F9)
                  VK_TEST(F10)
                  VK_TEST(F11)
                  VK_TEST(F12)

                  VK_TEST(HOME)
                  VK_TEST(END)

                  VK_TEST(PRIOR)  // page up
                  VK_TEST(NEXT)   // page down
                  VK_TEST(UP)
                  VK_TEST(DOWN)
                  VK_TEST(INSERT)
                  VK_TEST(DELETE)
                  VK_TEST(SPACE)
                  VK_TEST(HELP)
                  VK_TEST(EXECUTE)
                  VK_TEST(SELECT)
                  VK_TEST(PRINT)
                  VK_TEST(SNAPSHOT) // print screen?

                  VK_TEST(BACK)
                  VK_TEST(TAB)
                  VK_TEST(CLEAR)

                  VK_TEST(RETURN)

                  VK_TEST(MULTIPLY)
                  VK_TEST(ADD)
                  VK_TEST(SUBTRACT)
                  VK_TEST(DECIMAL)
                  VK_TEST(DIVIDE)
                  VK_TEST(SEPARATOR)

                  VK_TEST(PAUSE)
                  VK_TEST(CAPITAL)
                  VK_TEST(MENU)

                  VK_TEST(KANA)
                  VK_TEST(JUNJA)
                  VK_TEST(FINAL)
                  VK_TEST(HANJA)
                  VK_TEST(KANJI)

                  VK_TEST(CONVERT)
                  VK_TEST(NONCONVERT)
                  VK_TEST(ACCEPT)
                  VK_TEST(MODECHANGE)

                  VK_TEST(LWIN)
                  VK_TEST(RWIN)
                  VK_TEST(APPS)

                  VK_TEST(NUMPAD0)
                  VK_TEST(NUMPAD1)
                  VK_TEST(NUMPAD2)
                  VK_TEST(NUMPAD3)
                  VK_TEST(NUMPAD4)
                  VK_TEST(NUMPAD5)
                  VK_TEST(NUMPAD6)
                  VK_TEST(NUMPAD7)
                  VK_TEST(NUMPAD8)
                  VK_TEST(NUMPAD9)

                  VK_TEST(F13)
                  VK_TEST(F14)
                  VK_TEST(F15)
                  VK_TEST(F16)
                  VK_TEST(F17)
                  VK_TEST(F18)
                  VK_TEST(F19)
                  VK_TEST(F20)
                  VK_TEST(F21)
                  VK_TEST(F22)
                  VK_TEST(F23)
                  VK_TEST(F24)

                  VK_TEST(NUMLOCK)
                  VK_TEST(SCROLL)
            }
            else {
               // regular key...
               key = (WORD)*p;
            }

            accelerators[numAccelerators].cmd = command;
            accelerators[numAccelerators].fVirt = virt;
            accelerators[numAccelerators].key = key;
            numAccelerators++;
      } // if e
    }
    p = strtok(NULL, "\r\n");
  } // while

  delete [] buffer;

  return true;
}

HACCEL CAccelParser::GetTable(){
   if (!accelTable){
      accelTable = CreateAcceleratorTable(accelerators, numAccelerators);
   }
   return accelTable;
}
