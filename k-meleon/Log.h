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

// generic log window thing
// basically a multiline textbox with an ok button
// has the added ability to log to a file

#include "StdAfx.h"
#include "resource.h"

#ifdef ENABLE_LOG

#ifdef LOG_FILE

#define DEFINE_LOG CStdioFile *log;

#define SETUP_LOG(x)             \
   TRY {                         \
      log = new CStdioFile( x ".log", CFile::modeWrite | CFile::modeCreate);    \
   }                             \
   CATCH (CFileException, e) {   \
      log = NULL;                \
   }                             \
   END_CATCH

#define END_LOG() if (log) delete log;

#define LOG_1(msg, var1)        \
   if (log) {                   \
      char err[255];            \
      sprintf(err, msg, var1);  \
      log->WriteString(err);    \
      log->WriteString("\r\n"); \
   }

#define LOG_2(msg, var1, var2)       \
   if (log) {                        \
      char err[255];                 \
      sprintf(err, msg, var1, var2); \
      log->WriteString(err);         \
      log->WriteString("\r\n");      \
   }

#define LOG_ERROR_1(msg, var1)   \
   if (log) {                    \
      char err[255];             \
      sprintf(err, msg, var1);   \
      log->WriteString("*** ");  \
      log->WriteString(err);     \
      log->WriteString("\r\n");  \
   }

#define LOG_ERROR_2(msg, var1, var2)    \
   if (log) {                           \
      char err[255];                    \
      sprintf(err, msg, var1, var2);    \
      log->WriteString("*** ");         \
      log->WriteString(err);            \
      log->WriteString("\r\n");         \
   }

#define LOG_STRICT()
#define LOG_VERBOSE()


#else // LOG_FILE not defined, use a dialog box

class CLog {
protected:
   CString log;
   int error;

public:
   int strict, verbose;
   CString title;

   static BOOL CALLBACK MenuLogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
      if (uMsg == WM_INITDIALOG) {
         CString *log = &((CLog *)lParam)->log;
         SetDlgItemText(hwndDlg, IDC_ERRORS, *log);
         SetWindowText(hwndDlg, ((CLog *)lParam)->title + " Log");
         return true;
      }
      else if (uMsg == WM_COMMAND) {
         if (LOWORD(wParam) == IDOK)
            EndDialog(hwndDlg, 0);
       return true;
     }
     return false;
   }

   CLog() {
      error = 0;
      strict = 0;
      verbose = 0;
   };
   ~CLog() {
   };

   void Show(){
      if ((error && strict) || verbose){
         DialogBoxParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_ERRORBOX), NULL, CLog::MenuLogProc, (LPARAM)this);
      }
      error = 0;
   }

   void WriteString(const char *string){
      log += string;
   }

   void Error(){
      error = 1;
   }
};

#define DEFINE_LOG CLog log;
#define SETUP_LOG(x) log.title = x;
#define END_LOG() log.Show();

#define LOG_1(msg, var1)        \
   {                            \
      char err[255];            \
      sprintf(err, msg, var1);  \
      log.WriteString(err);     \
      log.WriteString("\r\n");  \
   }

#define LOG_2(msg, var1, var2)       \
   {                                 \
      char err[255];                 \
      sprintf(err, msg, var1, var2); \
      log.WriteString(err);          \
      log.WriteString("\r\n");       \
   }

#define LOG_ERROR_1(msg, var1)     \
   {                               \
      char err[255];               \
      sprintf(err, msg, var1);     \
      log.WriteString("*** ");     \
      log.WriteString(err);        \
      log.WriteString("\r\n");     \
      log.Error();                 \
   }

#define LOG_ERROR_2(msg, var1, var2)   \
   {                                   \
      char err[255];                   \
      sprintf(err, msg, var1, var2);   \
      log.WriteString("*** ");         \
      log.WriteString(err);            \
      log.WriteString("\r\n");         \
      log.Error();                     \
   }

// strict alerts when there is an error, verbose always alerts
#define LOG_STRICT() log.strict = 1;
#define LOG_VERBOSE() log.verbose = 1;

#endif // LOG_FILE

#else  // ENABLE_LOG

#define DEFINE_LOG
#define SETUP_LOG(x)
#define LOG_1(x, y)
#define LOG_ERROR_1(x, y)
#define LOG_2(x, y, z)
#define LOG_ERROR_2(x, y, z)
#define END_LOG()

#define LOG_STRICT()
#define LOG_VERBOSE()

#endif // ENABLE_LOG
