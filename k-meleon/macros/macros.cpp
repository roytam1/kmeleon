/*
*  Copyright (C) 2001 Jeff Doozan
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string>

#define PLUGIN_NAME "Macro Extension Plugin"

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\utils.h"
#include "macros.h"

#define _T(x) x

#define NOTFOUND -1


BOOL         APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
int          AddMacro();
void         AddMacroEvent(int macro, char *eventData);
int          AddNewVar(std::string strin);
int          AddVar();
int          BoolVal(std::string input);
void         Create(HWND parent);
void         Config(HWND parent);
void         DoError(char* msg);
void         DoMenu(HMENU menu, char *param);
void         DoRebar(HWND rebarWnd);
int          DoAccel(char *param);
long         DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
std::string  EvalExpression(HWND hWnd,std::string exp);
void         ExecuteMacro(HWND hWnd, int macro);
std::string  ExecuteCommand (HWND hWnd, int command, char *data);
int          FindCommand(char *macroName);
int          FindMacro(char *macroName);
char*        FindMacroName(int id);
int          FindVar(char *varName);
char*        FindVarName(int id);
int          GetConfigFiles(configFileType **configFiles);
std::string  GetVarVal(int varid);
std::string  GetGlobalVarVal(HWND hWnd, char *name, int *found);
int          Init();
int          IntVal(std::string input);
void         LoadMacros(char *filename);
void         Quit();
void         SetOption(std::string option);
void         SetVarExp(int varid, std::string value);
int          strFindFirst(std::string instring,char findchar,bool notinparen=false);
std::string  strTrim(std::string instr);
std::string  strVal(std::string input, int bOnlyQuotes = 0);



kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{  
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Init") == 0) {
         Init();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
         Quit();
      }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
          
          *(int *)data2 = DoAccel((char *)data1);
      }
      else if (stricmp(subject, "GetConfigFiles") == 0) {
         *(int *)data2 = GetConfigFiles((configFileType**)data1);
      }
      else return 0;

      return 1;
   }
   return 0;
}

kmeleonFunctions *kFuncs;

HINSTANCE ghInstance;
WINDOWPLACEMENT wpOld;

struct event {
   int  id;     // 0 based for user defined macros  1000 based for internal commands
   char *data;  // any parameters passed
};

struct macro {
   char *menuString;         // string to be displayed in menus
   char *macroName;          // macro name (as defined in macros.cfg)
   int eventCount;           // number of "events" in this macro
   struct event **eventList;
} **macroList;

struct var {
   char *varname;
   char *expression;
} **varList;

int ID_START    = -1;
int ID_END      = -1;
int iMacroCount = 0;
int iVarCount   = 0;

#define BEGIN_CMD_TEST if (0) {}
#define CMD_TEST(CMD)  else if (stricmp(cmd, #CMD) == 0) { cmdVal = ##CMD; }
#define CMD(CMD)  else if (command == ##CMD)

enum commands {
   open = 1000,      // open url in current window
   opennew,          // open url in new window
   openbg,           // open url in new background window
   setpref,          // set a preference
   getpref,
   togglepref,       // toggle a preference between values
   exec,             // run a program
   id,               // send a command id to the current window
   plugin,            // exectute a plugin command
   statusbar,        // set the text of the status bar in the current window
   alert,
   confirm,
   prompt,
   getclipboard,
   setclipboard,
   macros,
   pluginmsg,
   pluginmsgex,
   gensub,
   gsub,
   index,
   length,
   sub,
   substr,
   basename,
   dirname,
   hostname
};


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         ghInstance = (HINSTANCE) hModule;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}

int Init() {
   kFuncs = kPlugin.kFuncs;

   char szMacroFile[MAX_PATH];

   kFuncs->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szMacroFile, (void*)"");

   if (! *szMacroFile)
      return 0;

   strcat(szMacroFile, "macros.cfg");

   LoadMacros(szMacroFile);
   ID_START = kFuncs->GetCommandIDs(iMacroCount);
   ID_END = ID_START+iMacroCount-1;
   return 1;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND hWndParent) {
   char cfgPath[MAX_PATH];
   kFuncs->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), cfgPath, (void*)"");
   strcat(cfgPath, "macros.cfg");
   ShellExecute(NULL, NULL, "notepad.exe", cfgPath, NULL, SW_SHOW);
}

configFileType g_configFiles[1];

int GetConfigFiles(configFileType **configFiles)
{
   char cfgPath[MAX_PATH];
   kFuncs->GetPreference(PREF_STRING, _T("kmeleon.general.settingsDir"), cfgPath, (void*)"");

   strcpy(g_configFiles[0].file, cfgPath);
   strcat(g_configFiles[0].file, "macros.cfg");

   strcpy(g_configFiles[0].label, "Macros");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}

void Quit() {
   if (iMacroCount == 0) return;

   for (int curMacro=0;curMacro<iMacroCount;curMacro++) {
      // delete macros
      if (macroList[curMacro]->eventCount>0) {
         // delete events and eventlist
         for (int curEvent=0; curEvent<macroList[curMacro]->eventCount; curEvent++) {
            if (macroList[curMacro]->eventList[curEvent]->data)
               delete macroList[curMacro]->eventList[curEvent]->data;
            delete macroList[curMacro]->eventList[curEvent];
         }
         delete macroList[curMacro]->eventList;
      }
      if (macroList[curMacro]->menuString) delete macroList[curMacro]->menuString;
      if (macroList[curMacro]->macroName) delete macroList[curMacro]->macroName;
      delete macroList[curMacro];
   }
   delete macroList;

   if(iVarCount > 0) {
      for(int curVar = 0; curVar < iVarCount; curVar++) {
         if(varList[curVar]->varname) delete varList[curVar]->varname;
         if(varList[curVar]->expression) delete varList[curVar]->expression;
         delete varList[curVar];
      }
   }
   delete varList;

}

void DoMenu(HMENU menu, char *param) {
   if (*param) {
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         do {
            string++;
         } while (*string==' ' || *string=='\t');
      }

      int index = FindMacro(param);
      if (index != NOTFOUND) {
         if (macroList[index]->menuString)
            AppendMenu(menu, MF_STRING, ID_START+index, string ? string : macroList[index]->menuString);
         else if (macroList[index]->macroName)
            AppendMenu(menu, MF_STRING, ID_START+index, string ? string : macroList[index]->macroName);
         else 
            AppendMenu(menu, MF_STRING, ID_START+index, string ? string : "Untitled Macro");
      }
   }
}

int DoAccel(char *param) {
   if (*param) {
      int index = FindMacro(param);
      if (index != NOTFOUND)
         return ID_START+index;
   }
   return 0;
}

void DoRebar(HWND rebarWnd) {
}

int FindCommand(char *cmd) {

   int cmdVal = NOTFOUND;
   
   BEGIN_CMD_TEST
      CMD_TEST(open)
      CMD_TEST(opennew)
      CMD_TEST(openbg)
      CMD_TEST(setpref)
      CMD_TEST(getpref)
      CMD_TEST(togglepref)
      CMD_TEST(exec)
      CMD_TEST(id)
      CMD_TEST(plugin)
      CMD_TEST(statusbar)
      CMD_TEST(alert)
      CMD_TEST(confirm)
      CMD_TEST(prompt)
      CMD_TEST(getclipboard)
      CMD_TEST(setclipboard)
      CMD_TEST(macros)
      CMD_TEST(pluginmsg)
      CMD_TEST(pluginmsgex)
      CMD_TEST(gensub)
      CMD_TEST(gsub)
      CMD_TEST(index)
      CMD_TEST(length)
      CMD_TEST(sub)
      CMD_TEST(substr)
      CMD_TEST(basename)
      CMD_TEST(dirname)
      CMD_TEST(hostname)

   return cmdVal;
}

int FindMacro(char *macroName) {
   int x=0;
   while (x<iMacroCount) {
      if (macroList[x]->macroName)
         if (!strcmp(macroList[x]->macroName, macroName)) {
            return x;
         }
      x++;
   }
   return NOTFOUND;
}

/*
  Adds a new (empty) macro entry to macroList
  Returns the index of the created macro
*/
int AddMacro() {

   // create new, larger index and copy the old index over
   macro ** newMacroList = new macro*[iMacroCount+1];
   if (iMacroCount) { 
      memcpy(newMacroList, macroList, ((iMacroCount)*sizeof(macro**)) );
      delete macroList;
   }
   macroList = newMacroList;

   macroList[iMacroCount] = new macro;
   macroList[iMacroCount]->macroName  = NULL;
   macroList[iMacroCount]->menuString = NULL;
   macroList[iMacroCount]->eventCount = 0;
   macroList[iMacroCount]->eventList  = NULL;

   iMacroCount++;
   return iMacroCount-1;
}

/*
  Adds and initializes a new event entry to the current macro
*/
void AddMacroEvent(int macro, char *eventData) {

   // create new, larger index and copy the old index over
   event ** newEventList = new event*[macroList[macro]->eventCount+1];
   if (macroList[macro]->eventCount) {
      memcpy(newEventList, macroList[macro]->eventList, ((macroList[macro]->eventCount)*sizeof(event**)) );
      delete macroList[macro]->eventList;
   }
   macroList[macro]->eventList = newEventList;

   // set the data
   macroList[macro]->eventList[macroList[macro]->eventCount] = new event;
   //macroList[macro]->eventList[macroList[macro]->eventCount]->id   = eventID;
   if (eventData != NULL) {
      macroList[macro]->eventList[macroList[macro]->eventCount]->data = new char[strlen(eventData) + 1];
      strcpy(macroList[macro]->eventList[macroList[macro]->eventCount]->data, eventData);
   }
   else macroList[macro]->eventList[macroList[macro]->eventCount]->data = NULL;
   macroList[macro]->eventCount++;
}


void ExecuteMacro (HWND hWnd, int macro) {
   if ((macro < 0) || (macro >= iMacroCount))
      return;

   for (int x=0; x<macroList[macro]->eventCount; x++) {
      if(macroList[macro]->eventList[x]->data) {
         EvalExpression(hWnd,macroList[macro]->eventList[x]->data);
      }
   }
/*
   for (int x=0; x<macroList[macro]->eventCount; x++) {
      if (macroList[macro]->eventList[x]->id < 1000)
         //ExecuteMacro(hWnd, macroList[macro]->eventList[x]->id);
         EvalExpression(hWnd,macroList[macro]->eventList[x]->data);
      else ExecuteCommand(hWnd, macroList[macro]->eventList[x]->id, macroList[macro]->eventList[x]->data);
   }
*/
}

// Thanks to Ulf Erikson for the tokenizer to clean things up in here...
class Tokenizer {
protected:
   std::string strData;
   int pos,lpos;
   char lastchar;
   bool instr;
public:
   Tokenizer()           { resetTokenizer(""); }
   Tokenizer(char *data) { resetTokenizer( data ); }

   void resetTokenizer(char *data ) {
	  strData = data;
	  pos = lpos = 0;
	  instr = false;
	  if(strData.length() > 0) {
		 if(strData.at(0) == '"') instr = true;
		 lastchar = strData.at(0);
   }
	  else lastchar = 0;
}

   int nextToken ( std::string *out ) {
	  if (pos >= strData.length())
     return 0;
	  while(++pos <= strData.length()-1) {
		 if(strData.at(pos) == '"' && lastchar != '\\') {
			instr = (instr) ? false : true;
			lastchar = '"';
         continue;
      }
		 if(!instr) {
			if(strData.at(pos) == ',') {
            if (out)
				  *out = strVal(strData.substr(lpos, pos-lpos));
			   lpos = pos+1;
			   lastchar = ',';
            return 1;
         }
      }
		 lastchar = strData.at(pos);
   }
   if (out)
		 *out = strVal(strData.substr(lpos));
   return 1;
}
};

static void parseError(int err, char *cmd, char *args, int data1=0, int data2=0) {
   char title[256];
   char msg[256];
#define WRONGARGS 0
#define WRONGTYPE 1

   /* BUG! -- This should be snprintf. better check the length someway */
   sprintf(title, "Invalid %s command", cmd);
   switch (err) {
   case WRONGARGS:
	  sprintf(msg, "Wrong number of arguments - expected %d, found %d.\r\n\r\n"
			  "%s(%s)",data1, data2, cmd, args);
	  break;
   case WRONGTYPE:
	  sprintf(msg, "Invalid data type in %s command.\r\n\r\n"
			  "%s(%s)", cmd, cmd, args);
	  break;
   }
   MessageBox(NULL, msg, title, MB_OK);
}

std::string title = "";
std::string question = "";
std::string answer = "";

BOOL CALLBACK
PromptDlgProc( HWND hwnd,
	      UINT Message,
	      WPARAM wParam,
	      LPARAM lParam )
{
    switch (Message) {
      case WM_INITDIALOG:
	SetWindowText(hwnd, (char*)title.c_str());
	SetDlgItemText(hwnd, IDC_PROMPT, (char*)question.c_str());
        return TRUE;
      case WM_COMMAND:
        switch (LOWORD(wParam)) {
	  case IDOK:
	    char tmp[256];
	    GetDlgItemText(hwnd, IDC_ANSWER, tmp, 256);
	    answer = tmp;
	    EndDialog( hwnd, IDOK );
	    break;
	  case IDCANCEL:
	    EndDialog( hwnd, IDCANCEL );
	    break;
	}
	break;
	
      default:
        return FALSE;
    }
    return TRUE;
}

std::string protectString( char *pszData ) {
   // put the clipboard data in quotes to distinguish it from commands
   std::string retval = "\"";
   retval.append(pszData);  // the clipboard data
   // escape any " or \ in the clipboard data
   int pos=0;
   while(++pos < retval.length()) {
	  if(retval.at(pos) == '"') {
		 retval.insert(pos++,"\\");               
	  }
	  else if(retval.at(pos) == '\\') {
		 retval.insert(pos++,"\\");
	  }
   }
   
   // add the closing "
   retval.append("\"");
   return retval;
}


std::string GenSub(std::string r, std::string s, std::string h, std::string t)
{
  std::string result = "";
  std::string hd, tl = t;
  int match = 0;

  if (r.length() < 1)
    return t;

  int exch = -1;
  if (h[0] != 'g' && h[0]!='G')
    exch = atoi( (char *)h.c_str() );

  while (tl.length() > 0) {
    int i = tl.find(r);
    if (i != NOTFOUND) {
      match++;

      hd = tl.substr(0,i);
      tl = tl.substr(i+r.length());
      
      if (match == exch || exch < 0)
	result = result + hd + s;
      else
	result = result + hd + r;
    }
    else {
      result = result + tl;
      break;
    }
  }
  result = protectString( (char*)result.c_str() );
  return result;
}

std::string ExecuteCommand (HWND hWnd, int command, char *data) {
   const int nmaxparams = 5;  // maximum num of function parameters
   std::string params[nmaxparams];
   class Tokenizer t(data);
   int nparam = 0;

   while (nparam<nmaxparams && t.nextToken( &params[nparam] )) {
      nparam++;
   }

   BEGIN_CMD_TEST
      CMD(open) {
         if (nparam != 1) {  // open( $0 )
			parseError(WRONGARGS, "open", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)params[0].c_str(), OPEN_NORMAL);
      }
      CMD(opennew) {
         if (nparam != 1) {  // opennew( $0 )
            parseError(WRONGARGS, "opennew", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)params[0].c_str(), OPEN_NEW);
      }
      CMD(openbg) {
         if (nparam != 1) {  // openbg( $0 )
            parseError(WRONGARGS, "openbg", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)params[0].c_str(), OPEN_BACKGROUND);
      }
      CMD(setpref)   {
         enum PREFTYPE preftype;

         if (nparam != 3) {  // setpref( $0, $1, $2 )
            parseError(WRONGARGS, "setpref", data, 3, nparam);
            return "";
         }

         if (!strcmpi((char*)params[0].c_str(), "bool")) preftype = PREF_BOOL;
         else if (!strcmpi((char*)params[0].c_str(), "int")) preftype = PREF_INT;
         else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_STRING;
         else {
            parseError(WRONGTYPE, "setpref", data);
            return "";
         }
         char *pref = (char*)params[1].c_str();
         data = (char*)params[2].c_str();

         // Thanks to Mynen (mark_yen@hotmail.com) for pointing out this bug
         //  as well as submitting a patch
         if (data && *data) {

            if (preftype == PREF_STRING)
               kFuncs->SetPreference(preftype, pref, data, TRUE);

            else if (preftype == PREF_INT) {
               // note that SetPreference() expects third param
               // to be a pointer in all cases, even for int and bool
               int iData = atoi(data);
               kFuncs->SetPreference(preftype, pref, &iData, TRUE);
            } 

            else {   // boolean
               int bData = FALSE;
               if (!strcmpi(data, "true"))
                  bData = TRUE;
               kFuncs->SetPreference(preftype, pref, &bData, TRUE);
            }
         }
      }
      CMD(getpref) {
         enum PREFTYPE preftype;

         if(nparam != 2)  { // getpref( $0, $1 )
            parseError(WRONGARGS, "getpref", data, 2, nparam);
            return "";
         }

         if (!strcmpi((char*)params[0].c_str(), "bool")) preftype = PREF_BOOL;
         else if (!strcmpi((char*)params[0].c_str(), "int")) preftype = PREF_INT;
         else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_STRING;
         else {
			parseError(WRONGTYPE, "getpref", data);
            return "";
         }

         char cRetval[256];
         int nRetval = 0;
         if (preftype == PREF_STRING) {
            kFuncs->GetPreference(preftype,(char*)params[1].c_str(),&cRetval,NULL);
            std::string strRet;
            strRet = protectString( cRetval );
            return strRet;
         }
         else if (preftype == PREF_INT) {
            kFuncs->GetPreference(preftype,(char*)params[1].c_str(),&nRetval,&nRetval);
            char buffer[12];
            _itoa(nRetval,buffer,10);
            return buffer;
         }
         else { //PREF_BOOL
            kFuncs->GetPreference(preftype,(char*)params[1].c_str(),&nRetval,&nRetval);
            if(nRetval) return "true";
            else return "false";
         }
         return "";
      }
      CMD(togglepref) {
         enum PREFTYPE preftype;

         if (nparam) {
            if (!strcmpi((char*)params[0].c_str(), "bool")) preftype = PREF_BOOL;
            else if (!strcmpi((char*)params[0].c_str(), "int")) preftype = PREF_INT;
            else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_STRING;
            else {
               parseError(WRONGTYPE, "togglepref", data);
               return "";
            }
         }

         if ((preftype == PREF_BOOL && nparam != 2) ||
            (preftype != PREF_BOOL && nparam < 3)) {
            parseError(WRONGARGS, "togglepref", data, (preftype==PREF_BOOL)?2:4, nparam);
            return "";
         }

         char sVal[256];
         int  iVal=0;
         char *pref = (char*)params[1].c_str();

         if (preftype == PREF_STRING)
            kFuncs->GetPreference(preftype, pref, &sVal, NULL);
         else {
            kFuncs->GetPreference(preftype, pref, &iVal, &iVal);
            if (preftype == PREF_BOOL) {
               iVal = !iVal;
               kFuncs->SetPreference(preftype, pref, &iVal, TRUE);
               return "";
            }
         }

         t.resetTokenizer( data );
         t.nextToken( NULL );  // params[0] == data type
         t.nextToken( NULL );  // params[1] == preference id
         char *prefdata = (char*)params[2].c_str();

         std::string str;

         BOOL bPrefWritten = FALSE;
         while (!bPrefWritten && t.nextToken( &str)) {
            char *param = (char*)str.c_str();
            if (preftype == PREF_STRING) {
               if (!strcmp(param, sVal)) {
                  if (t.nextToken( &str )) {
                     param = (char*)str.c_str();
                     kFuncs->SetPreference(preftype, pref, param, TRUE);
                  }
                  else
                     kFuncs->SetPreference(preftype, pref, prefdata, TRUE);
                  bPrefWritten = TRUE;
               }
            }
            else if (preftype == PREF_INT) {
               int dataVal = atoi(param);
               if (dataVal == iVal) {
                  if (t.nextToken( &str )) {
                     param = (char*)str.c_str();
                     dataVal = atoi(param);
                  }
                  else
                     dataVal = atoi(prefdata);

                  kFuncs->SetPreference(preftype, pref, &dataVal, TRUE);
                  bPrefWritten = TRUE;
               }
            }
         }

         if (!bPrefWritten) {
            if (preftype == PREF_STRING)
               kFuncs->SetPreference(preftype, pref, prefdata, TRUE);
            else if (preftype == PREF_INT) {
               int val = atoi(prefdata);
               kFuncs->SetPreference(preftype, pref, &val, TRUE);
            }
         }
      }


      CMD(exec) {
         STARTUPINFO si = {0};
         PROCESS_INFORMATION pi = {0};
         CreateProcess(NULL, (char*)params[0].c_str(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
      }

      CMD(id) {
         int cmd;
         cmd = kPlugin.kFuncs->GetID((char*)params[0].c_str());
         if (!cmd)
            cmd = atoi((char*)params[0].c_str());

         SendMessage(hWnd, WM_COMMAND, cmd, (LPARAM)NULL);
      }
      CMD(plugin) {
         int cmd;
         if (nparam != 2) {  // plugin( $0, $1 )
            parseError(WRONGARGS, "plugin", data, 2, nparam);
            return "";
         }
         char *plugin = (char*)params[0].c_str();
         char *param  = (char*)params[1].c_str();
         kPlugin.kFuncs->SendMessage(plugin, PLUGIN_NAME, "DoAccel", (long)param, (long)&cmd);
         SendMessage(hWnd, WM_COMMAND, cmd, NULL);
      }
      CMD(statusbar) {
         if (nparam != 1) {  // statusbar( $0 )
            parseError(WRONGARGS, "statusbar", data, 1, nparam);
            return "";
         }
         kPlugin.kFuncs->SetStatusBarText((char*)params[0].c_str());
         return "";
      }
      CMD(alert) {
         // params are message,title,icon

         int icon = 0;
         if(strcmpi(params[2].c_str(),"EXCLAIM")==0) icon=MB_ICONEXCLAMATION;
         else if(strcmpi(params[2].c_str(),"INFO")==0) icon=MB_ICONINFORMATION;
         else if(strcmpi(params[2].c_str(),"STOP")==0) icon=MB_ICONSTOP;         
         else if(strcmpi(params[2].c_str(),"QUESTION")==0) icon=MB_ICONQUESTION;         

         MessageBox(NULL,params[0].c_str(),params[1].c_str(), MB_OK|icon);
         return "";
      }
      CMD(confirm) {
         // params are message,title,buttons,icon

         int buttons = MB_OKCANCEL;
         if     (strcmpi(params[2].c_str(),"RETRYCANCEL")==0) buttons=MB_RETRYCANCEL;
         else if(strcmpi(params[2].c_str(),"YESNO")==0) buttons=MB_YESNO;
         else if(strcmpi(params[2].c_str(),"YESNOCANCEL")==0) buttons=MB_YESNOCANCEL;         
         else if(strcmpi(params[2].c_str(),"ABORTRETRYIGNORE")==0) buttons=MB_ABORTRETRYIGNORE; 

         int icon = 0;
         if     (strcmpi(params[3].c_str(),"EXCLAIM")==0) icon=MB_ICONEXCLAMATION;
         else if(strcmpi(params[3].c_str(),"INFO")==0) icon=MB_ICONINFORMATION;
         else if(strcmpi(params[3].c_str(),"STOP")==0) icon=MB_ICONSTOP;         
         else if(strcmpi(params[3].c_str(),"QUESTION")==0) icon=MB_ICONQUESTION;         

         int result = MessageBox(NULL,params[0].c_str(),params[1].c_str(), buttons|icon);
         if(result == IDOK) return "OK";
         if(result == IDYES) return "YES";
         if(result == IDNO) return "NO";
         if(result == IDABORT) return "ABORT";
         if(result == IDRETRY) return "RETRY";
         if(result == IDIGNORE) return "IGNORE";
         if(result == IDCANCEL) return "0";         

         return "";      

      }
      CMD(prompt) {
         if (nparam > 2) {  // statusbar( [$0 [,$1]] )
            parseError(WRONGARGS, "statusbar", data, 2, nparam);
            return "";
         }
	question = params[0];
	title = params[1];
	int ok = DialogBox(kPlugin.hDllInstance,
		  MAKEINTRESOURCE(IDD_PROMPT), hWnd, (DLGPROC)PromptDlgProc);
	PostMessage(hWnd, WM_NULL, 0, 0);
	if (ok == IDOK) {
	   answer = protectString( (char*)answer.c_str() );
	   return answer;
	}
	else 
         return "";
      }

      CMD(getclipboard) {
         // get and return data from the clipboard

         if(!IsClipboardFormatAvailable(CF_TEXT)) return "";
         if(!OpenClipboard(NULL)) {
            DoError("Error opening the clipboard.");
            return "";
         }

         char* pszData;
         LPVOID pData;
         HANDLE hcb;
         hcb = GetClipboardData(CF_TEXT);
         pData = GlobalLock(hcb);
         pszData = (char*)malloc(strlen((char*)pData) + 1);
         strcpy(pszData, (LPSTR)pData);
         GlobalUnlock(hcb);
          
         CloseClipboard();

         // put the clipboard data in quotes to distinguish it from commands
         std::string retval;
		 retval = protectString( pszData );
         return retval;
     }

      CMD(setclipboard) {
         // set data to the clipboard

         if(!OpenClipboard(NULL)) {
            DoError("Error opening the clipboard.");
            return "";
         }

         char *pszData = (char*)params[0].c_str();
         HGLOBAL hData;
         LPVOID pData;

         EmptyClipboard();

         hData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,strlen(pszData) + 1);

         pData = GlobalLock(hData);
         strcpy((LPSTR)pData, pszData);
         GlobalUnlock(hData);

         SetClipboardData(CF_TEXT, hData);
         CloseClipboard();

         return "1";
      }
      CMD(macros) {
         // execute another macro

         int cmdid = FindMacro((char*)params[0].c_str());
         if(cmdid == NOTFOUND) {
            std::string msg = "Invalid macro reference. The macro '";
            msg.append(params[0]);
            msg.append("' does not exist.");
            DoError((char*)msg.c_str());
         }
         else {
            ExecuteMacro(hWnd,cmdid);
         }
      }


      /*
         PluginMsg ( "DestPlugin", "Command", "Param1", "Param2");

         Send a command to a plugin that does not need a return value.
         If the plugin needs more than two parameters, it will have to
         parse them out of a string passed as either Param1 or Param2
      */

      CMD(pluginmsg) {
      
         if((nparam != 3) && (nparam != 4))  {
            parseError(WRONGARGS, "PluginMsgEx", data, 4, nparam);
            return "";
         }

         kFuncs->SendMessage((char*)params[0].c_str(), PLUGIN_NAME, (char*)params[1].c_str(), (long) params[2].c_str(), (long) params[3].c_str());
         return "";
         
      }

      /*
         PluginMsgEx ( "DestPlugin", "Command", "Params", [INT|CHAR]);

         Internally, this uses the SendMessage command, to send
         "Command" to DestPlugin with a pointer to "Params" in Data1
         and a pointer to RetVal in Data2

         This introduces a compatability concern - all plugins that wish to
         export commands to the macros this way MUST recieve their params in
         Data1 and return any values in Data2.  If a plugin needs more than
         one paramater, it must be ready to parse it from a string in Data1
         eg, "Params" would actually be "Param1, \"Param 2\", Param3, etc"
      */

      CMD(pluginmsgex) {
         enum PREFTYPE preftype;

         if(nparam != 4)  {
            parseError(WRONGARGS, "PluginMsgEx", data, 4, nparam);
            return "";
         }

         if (!strcmpi((char*)params[3].c_str(), "int")) preftype = PREF_INT;
         else if (!strcmpi((char*)params[3].c_str(), "string")) preftype = PREF_STRING;
         else {
			   parseError(WRONGTYPE, "PluginMsgEx", data);
            return "";
         }

         char cRetval[2048];  // 2k should be enough?
         int nRetval = 0;
         if (preftype == PREF_STRING) {
            kFuncs->SendMessage((char*)params[0].c_str(), PLUGIN_NAME, (char*)params[1].c_str(), (long) params[2].c_str(), (long) &cRetval);
            std::string strRet;
            strRet = protectString( cRetval );
            return strRet;
         }
         else if (preftype == PREF_INT) {
            kFuncs->SendMessage((char*)params[0].c_str(), PLUGIN_NAME, (char*)params[1].c_str(), (long) params[2].c_str(), (long) &nRetval);
            char buffer[12];
            _itoa(nRetval,buffer,10);
            return buffer;
         }
         return "";
      }

      /*
         gensub( r, s, h, t );

	 search the target string t for matches of the string r.  If h
	 is a string beginning with g or G, then replace all matches
	 of r with s.  Otherwise, h is a number indicating which match
	 of r to replace.  The modified string is returned as the
	 result of the function.
      */

      CMD(gensub) {
         if (nparam != 4) {  // substr( $0, $1, $2, $3 )
            parseError(WRONGARGS, "gensub", data, 4, nparam);
            return "";
         }

	 return GenSub(params[0], params[1], params[2], params[3]);
      }

      /*
         gsub( r, s, t );

	 for each substring matching the string r in the string t,
	 substitute the string s, and return the number of
	 substitutions.
      */

      CMD(gsub) {
         if (nparam != 3) {  // substr( $0, $1, $2 )
            parseError(WRONGARGS, "gsub", data, 3, nparam);
            return "";
         }

	 return GenSub(params[0], params[1], "G", params[2]);
      }

      /*
         index( s, t );

	 returns the index of the string t in the string s, or -1 if t
	 is not present.
      */

      CMD(index) {
         if (nparam != 2) {  // substr( $0, $1 )
            parseError(WRONGARGS, "index", data, 2, nparam);
            return "";
         }
	 int i = params[0].find( params[1] );
	 if (i == NOTFOUND)
	   return "-1";
	 else {
	   char buffer[12];
	   _itoa(i,buffer,10);
	   return buffer;
	 }
      }

      /*
         length( s );

	 returns the length of the string s.
      */

      CMD(length) {
         if (nparam != 1) {  // substr( $0 )
            parseError(WRONGARGS, "length", data, 1, nparam);
            return "";
         }
	 char buffer[12];
	 _itoa(params[0].length(),buffer,10);
	 return buffer;
      }

      /*
         sub( t, s, t );

	 just like gsub(), but only the first matching substring is
	 replaced.
      */

      CMD(sub) {
         if (nparam != 3) {  // substr( $0, $1, $2 )
            parseError(WRONGARGS, "sub", data, 3, nparam);
            return "";
         }

	 return GenSub(params[0], params[1], "1", params[2]);
      }

      /*
         substr( s, i [, n] );

	 returns the at most n-character substring of s starting at i.
	 If n is omitted, the rest of s is used.
      */

      CMD(substr) {
         if (nparam != 2 && nparam != 3) {  // substr( $0, $1 [, $2] )
            parseError(WRONGARGS, "substr", data, 3, nparam);
            return "";
         }
		 std::string retval;

	 if (nparam == 2)
			retval = params[0].substr( atoi( (char *)params[1].c_str() ) );
	 else
			retval = params[0].substr( atoi( (char *)params[1].c_str() ),
				    atoi( (char *)params[2].c_str() ) );

		 retval = protectString( (char*)retval.c_str() );
		 return retval;
      }

      /*
         basename( NAME [, SUFFIX] );

	 Returns NAME with any leading directory components removed.
	 If specified, also remove a trailing SUFFIX.
      */

      CMD(basename) {
         if (nparam != 1 && nparam != 2) {  // substr( $0, [, $1] )
            parseError(WRONGARGS, "basename", data, 2, nparam);
            return "";
         }
	 int i, j;

	 if (nparam == 2) {
	   i = params[0].rfind( params[1] );
	   if (i != NOTFOUND )
	     params[0] = params[0].substr(0,i);
	 }
	 
	 i = params[0].rfind( "/" );
	 j = params[0].rfind( "\\" );
	 
	 if (i == NOTFOUND)
	   i = j;
	 if (i != NOTFOUND && j != NOTFOUND && i<j)
	   i = j;

	 if (i != NOTFOUND)
	   params[0] = params[0].substr(i+1);

		 std::string retval;
		 retval = protectString( (char*)params[0].c_str() );
		 return retval;
      }

      /*
         dirname( NAME );

	 Returns NAME with its trailing /component removed; if NAME
	 contains no /'s, output `.' (meaning the current directory).
      */

      CMD(dirname) {
         if (nparam != 1) {  // substr( $0 )
            parseError(WRONGARGS, "dirname", data, 1, nparam);
            return "";
         }
	 int i, j;

	 int len = params[0].length();
	 if (params[0].at(len-1) == '/' ||
		 params[0].at(len-1) == '\\')
		params[0] = params[0].substr(0, len-1);

	 i = params[0].rfind( "/" );
	 j = params[0].rfind( "\\" );

	 if (i == NOTFOUND)
	   i = j;
	 if (i != NOTFOUND && j != NOTFOUND && i<j)
	   i = j;

	 if (i != NOTFOUND)
	   params[0] = params[0].substr(0, i>0 ? i : 1);
		 else
			params[0] = ".";

		 std::string retval;
		 retval = protectString( (char*)params[0].c_str() );
		 return retval;
      }

      /*
         hostname( URL );

	 Returns hostname of given URL.
      */

      CMD(hostname) {
         if (nparam != 1) {  // substr( $0 )
            parseError(WRONGARGS, "hostname", data, 1, nparam);
            return "";
         }
	 int i;

	 i = params[0].find( "://" );
	 if (i != NOTFOUND)
	   params[0] = params[0].substr(i+3);

	 i = params[0].find( "/" );
	 if (i != NOTFOUND)
	   params[0] = params[0].substr(0,i);

		 std::string retval;
		 retval = protectString( (char*)params[0].c_str() );
		 return retval;
      }

   return "";
}

void LoadMacros(char *filename) {
   HANDLE macroFile;

   macroFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, (WPARAM)NULL, NULL);
   if (macroFile == INVALID_HANDLE_VALUE) {
      MessageBox(NULL, "Could not open file", filename, MB_OK);
      return;
   }
   DWORD length = GetFileSize(macroFile, NULL);
   char *buf = new char[length + 1];
   ReadFile(macroFile, buf, length, &length, NULL);
   buf[length] = 0;
   CloseHandle(macroFile);  
   
   std::string fileGuts = buf;
   delete buf;

   fileGuts.append("\r\nEOF\r\n");

   std::string lval,rval,strtemp,thisLine,thisStatement;
   int nCurPos, pos;
   int nLineCount  = 0;
   nCurPos = 0;
   bool buildingMacro,inBlock,isEOF,skipBlock;
   buildingMacro = inBlock = isEOF = skipBlock = false;

   while(!isEOF) {
      ++nLineCount;

      // take it one line at a time (losing the carriage return and newline)
      int n = fileGuts.find("\r\n",nCurPos);
      thisLine = fileGuts.substr(nCurPos,n - nCurPos);
      nCurPos = n+2;

      // get outta here if we're at EOF
      if(thisLine == "EOF") {
         isEOF = true;
         break;
      }

      // convert tabs
      pos = thisLine.find("\t");
      while(pos != std::string::npos) {
         thisLine.replace(pos,1," ");
         pos = thisLine.find("\t");
      }

      // trim whitespace
      thisLine = strTrim(thisLine);

      // skip blank lines
      if(thisLine.length() < 1) continue;

      // skip full line comments
      if(thisLine.at(0) == '#') continue;

      // get rid of everything after the first # including any leading whitespace
      // unless it's within a string
      pos = thisLine.find('#');
      if(pos != std::string::npos) {
         bool instr = false;
         for(pos=0; pos<thisLine.length()-1; ++pos) {
            if(thisLine.at(pos) == '"') {
               instr = (instr) ? false : true;
            }
            if(thisLine.at(pos) == '#' && !instr) {
               thisLine.replace(pos,thisLine.length()-pos,"");
               break;
            }
         }
         thisLine = strTrim(thisLine);
      }

      // if the line starts with a ! it's a macro option so we'll strip the ! and
      // any whitespace then set the option if it's recognized
      pos = 0;
      if(thisLine.at(0) == '!') {
         thisLine = thisLine.substr(1);
         thisLine = strTrim(thisLine);
         //SetOption(thisLine);
         continue;
      }


      // the only things that can be outside a macro def are comments,options and 
      // global variable declarations and assignments
      // we already took care of comments and options
      if(!buildingMacro && thisLine.at(0) == '$') {
         do {
            if(thisLine.at(0) != '$') break;
            pos = strFindFirst(thisLine,';',true);
            if(pos == std::string::npos) {
               thisStatement = thisLine;
               thisLine = "";   // end the loop
            }
            else {
               thisStatement = strTrim(thisLine.substr(0,pos));
               thisLine = strTrim(thisLine.substr(pos+1));
            }
            //AddMacroEvent(nglobalmacro,(char*)thisStatement.c_str());
            EvalExpression(NULL,thisStatement);
         } while(thisLine.length() > 0);
      }
      if(thisLine.length() < 1) continue;


      if(!buildingMacro) {
         pos = thisLine.find_first_of('{');
         if(pos != std::string::npos) {
            // if there's a { we're entering a block. the macro name is before the { and
            // the macro definition may or may not begin on this line after the {.
            // we also may have whitespace to get rid of
            strtemp = strTrim(thisLine.substr(0,pos)); //name of the macro
            thisLine = strTrim(thisLine.substr(pos+1)); // start of the macro def

            inBlock = true;
         }
         else {
            strtemp = thisLine; //name of the macro
            thisLine = "";
         }
         // check for a valid macro name
         if(strtemp.length() < 1) {
            DoError("Missing macro name.");
            break;
         }
         char* badlist = "!@#$%^&*(){}[]<>/ \\\"'`~=?;";
         pos = -1;
         bool isBad = false;
         for(int i=0; i<strlen(badlist); ++i) {
            pos = strtemp.find(badlist[i]);
            if(pos != std::string::npos) {
               isBad = true;
               break;
            }            
         }
         if(isBad) {
            DoError("Bad character in macro name.");
            break;
         }
         // if we made it this far the macro name looks good
         // add new macro if it doesn't already exist
         if(FindMacro((char*)strtemp.c_str()) == NOTFOUND) {
            int curMacro = AddMacro();
            macroList[curMacro]->macroName = new char[strtemp.length() + 1];
            strcpy (macroList[curMacro]->macroName,strtemp.c_str());
            buildingMacro = true;
         }
         else {   // macro exists, give a redefinition error and skip the rest of this macro
            DoError("Macro redefinition.");
         }
      }

      if(thisLine.length() < 1) continue;

      //*else* we're building a macro
      // if we're not in a block yet, this better be the start of one
      //  otherwise they most likely forgot the opening {
      if(!inBlock) {
         if(thisLine.at(0) == '{') {
            // hooray for them, they started a block!
            // if there's anything after { it's either whitespace or the start of the macro def
            // so get rid of the whitespace, and if there's nothing left loop the next line
            inBlock = true;
            thisLine = strTrim(thisLine.substr(1));
            if(thisLine.length() < 1) continue;
         }
         else {
            // somethings not right here so exit with an error
            DoError("Invalid macro definition. Missing opening bracket?");
            break;
         }
      }
     // check for the end of a block
      // a } contained in parenthesis or a string does not close a block
      pos = thisLine.find('}');
      if(pos != std::string::npos) {   // there's a } so check the whole line
         if(thisLine.length() <= 1) {
            buildingMacro = inBlock = false;
            continue;
         }

         pos = strFindFirst(thisLine,'}',true);
         if(pos != std::string::npos) {
            thisLine = strTrim(thisLine.substr(0,pos));
            buildingMacro = inBlock = false;
            if(thisLine.length() < 1) {
               continue;
            }
         }
      }


     // there's no if's, and's or but's about it, we're now processing macro commands
      // go to the next line if we're skipping this block
      bool needBreak = false;
      do {
         pos = strFindFirst(thisLine,';',true);
         if(pos == std::string::npos) {
            thisStatement = thisLine;
            thisLine = "";   // end the loop
         }
         else {
            thisStatement = strTrim(thisLine.substr(0,pos));
            thisLine = strTrim(thisLine.substr(pos+1));
         }   
         // we have a statement, time to do something with it

         //check for menu assignment
         pos = thisStatement.find('=');
         if(pos != std::string::npos) {
            pos = strFindFirst(thisStatement,'=',true);
            if(pos != std::string::npos) {
               lval = strTrim(thisStatement.substr(0,pos));
               if(strcmpi(lval.c_str(),"menu") == 0) {
                  rval = strTrim(thisStatement.substr(pos+1));
                  if(rval.length() < 1) { // trying to set the menu to nothing
                     DoError("Cannot set menu string to empty value.");
                     needBreak = true; break;
                  }
                  if(macroList[iMacroCount-1]->menuString) delete macroList[iMacroCount-1]->menuString;
                  strtemp = strVal(rval, -1); // FIXME: ugly misuse to convert '\'+'t' to '\t'
                  macroList[iMacroCount-1]->menuString = new char[strtemp.length()+1];
                  strcpy(macroList[iMacroCount-1]->menuString,strtemp.c_str());
                  continue;
               }
            }
         }

         // if it begins with an ampersand this is a reference to another macro
         if(thisStatement.at(0) == '&') {
            thisStatement = strTrim(thisStatement.substr(1));
            if(thisStatement.length() < 1) {
               DoError("Invalid macro reference.");
               needBreak = true; break;
            }
            thisStatement = "macros(" + thisStatement + ")";
         }

         // anything that's left by now is an expression of some sort
         AddMacroEvent(iMacroCount-1,(char*)thisStatement.c_str());
         // if(!inBlock) DBUG(ENDBLOCK,"");

      } while (thisLine.length() > 0);

      if(needBreak) break;




   }//end while

}

/**********/
std::string EvalExpression(HWND hWnd,std::string exp) {

   if(exp.length() < 1) return ""; // nothing to evaluate

   int pos,rpos,lpos,lparen,rparen;
   int i = 0;
   bool instr;
   std::string lval,rval,strtemp;

   pos = lpos = rpos = NOTFOUND;
   lparen = rparen = 0;
   instr = false;

   // this could be a comma seperated list of expressions
   if(exp.find_first_of(',') != std::string::npos) {
      if(exp.at(0) == '"') instr = true;
      for(i=1;i<exp.length();++i) {
         if(exp.at(i) == '"' && exp.at(i-1) != '\\') {
            instr = (instr) ? false : true;
            continue;
         }
         if(!instr) {
            if(exp.at(i) == '(') {
               ++lparen;
               continue;
            }
            if(exp.at(i) == ')') {
               ++rparen;
               continue;
            }
            if(exp.at(i) == ',' && lparen==rparen) {
               pos = i;
               break;
            }
         }
      }

      if(pos != NOTFOUND) {
         lval = EvalExpression(hWnd,strTrim(exp.substr(0,pos)));
         rval = EvalExpression(hWnd,strTrim(exp.substr(pos+1)));

         return lval + "," + rval;
      }
   }


   // conditional expression?
   if (exp.find_first_of('?') != std::string::npos &&
       exp.find_first_of(':') != std::string::npos) {
	       
      int epos, qpos, cpos;
      int lparen, rparen;
      bool instr;
      
      epos = qpos = cpos = NOTFOUND;
      lparen = rparen = 0;
      instr = false;
      
      if (exp.at(0) == '"') instr = true;
      else if (exp.at(0) == '(') ++lparen;
      for (int j=1; j<exp.length(); ++j) {
	 if (exp.at(j) == '"' && exp.at(j-1) != '\\') {
	    instr = (instr) ? false : true;
	    continue;
	 }
	 if (!instr) {
	    if(exp.at(j) == '(') {
	       ++lparen;
	       continue;
	    }
	    if(exp.at(j) == ')') {
	       ++rparen;
	       continue;
	    }
	    if (exp.at(j) == '=' && lparen==rparen && epos==NOTFOUND) {
	       epos = j;
	    }
	    else if (exp.at(j) == '?' && lparen==rparen && qpos==NOTFOUND) {
	       qpos = j;
	    }
	    else if (exp.at(j) == ':' && lparen==rparen) {
	       cpos = j;
	       break;
	    }
	 }
      }

      if ( epos != NOTFOUND && qpos != NOTFOUND && epos < qpos &&
	   exp.at(epos+1) != '=' && exp.at(epos-1) != '!' ) {
	 qpos = cpos = NOTFOUND;
      }
      
      if (qpos != NOTFOUND && cpos != NOTFOUND && qpos<cpos) {

	 int b = BoolVal(EvalExpression(hWnd,strTrim(exp.substr(0,qpos))));
	 if (b)
	    strtemp = EvalExpression(hWnd,strTrim(exp.substr(qpos+1,cpos-(qpos+1))));
	 else
	    strtemp = EvalExpression(hWnd,strTrim(exp.substr(cpos+1)));
	 
	 return strtemp;
      }
      else if (qpos != NOTFOUND || cpos != NOTFOUND) {
	 strtemp = "The expression '" + exp + "' is a badly formed conditional expression.";
	 DoError((char*)strtemp.c_str());
	 return "";
      }
   }

   // if there's an equals (=) that's not in a string, and not in parenths
   // check for operators also
   // it's a comparison or an assignment
   bool isComparison = false;
   bool isAssignment = false;
   rpos = lpos = NOTFOUND;
   lparen = rparen = 0;
   instr = false;
   if(exp.at(0) == '"') instr = true;
   else if(exp.at(0) == '(') ++lparen;
   for(i=1;i<exp.length();++i) {
      //break; //skip this for now
      if(exp.at(i) == '"' && exp.at(i-1) != '\\') {
         instr = (instr) ? false : true;
         continue;
      }
      if(!instr) {
         if(exp.at(i) == '(') {
            ++lparen;
            continue;
         }
         if(exp.at(i) == ')') {
            ++rparen;
            continue;
         }

         if(lparen == rparen) {
            if(exp.at(i)=='=') {
               //we have an = that's not in parenths and not in a string
               if(exp.at(i+1)=='=' || exp.at(i-1)=='!') {
                  isComparison = true;
                  if(exp.at(i+1)=='=') ++i;
                  lval = exp.substr(0,i-1);
                  rval = exp.substr(i+1);
                  strtemp = exp.substr(i-1,2); //the comparison operator
                  break;
               }
	       else {
                  isAssignment = true;
                  lval = strTrim(exp.substr(0,i));
                  rval = strTrim(exp.substr(i+1));
                  break;
               }
            }
            else if(exp.at(i) == '+') {   // addition
               int res = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,i)))) + IntVal(EvalExpression(hWnd,strTrim(exp.substr(i+1))));
               itoa(res,(char*)exp.data(),10);
               return exp;
            }
            else if(exp.at(i) == '-') { // subtraction
               int res = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,i)))) - IntVal(EvalExpression(hWnd,strTrim(exp.substr(i+1))));
               itoa(res,(char*)exp.data(),10);
               return exp;
            }
            else if(exp.at(i) == '*') { // multiplication
               int res = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,i)))) * IntVal(EvalExpression(hWnd,strTrim(exp.substr(i+1))));
               itoa(res,(char*)exp.data(),10);
               return exp;
            }
            else if(exp.at(i) == '/') { // division
               int nrval = IntVal(EvalExpression(hWnd,strTrim(exp.substr(i+1))));
               if(nrval == 0) { //illegal division by zero
                  return "0";
               }
               else {
                  int res = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,i)))) / nrval;
                  itoa(res,(char*)exp.data(),10);
                  return exp;
               }
            }
            else if(exp.at(i) == '%') { // remainder
               int nrval = IntVal(EvalExpression(hWnd,strTrim(exp.substr(i+1))));
               if(nrval == 0) { //illegal division by zero
                  return "0";
               }
               else {
                  int res = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,i)))) % nrval;
                  itoa(res,(char*)exp.data(),10);
                  return exp;
               }
            }
            else if(exp.at(i) == '.') { // concat
               lval = strVal(EvalExpression(hWnd, strTrim(exp.substr(0,i))),1);
               rval = strVal(EvalExpression(hWnd, strTrim(exp.substr(i+1))),1);
               return "\"" + lval + rval.c_str() + "\"";
               // yes, the .c_str() is actually necessary.  Without it, the
               // final " isn't appended...  don't ask me why.
            }
         }
      }
   }

   // if we're comparing the left or right hand values could be an expression
   if(isComparison) {
      lval = EvalExpression(hWnd,strTrim(lval));
      rval = EvalExpression(hWnd,strTrim(rval));
      if(strVal(lval) == strVal(rval)) { 
         if(strtemp == "==") return "1";
         else return "0";
      }
      else {
         if(strtemp == "!=") return "1";
         else return "0";
      }
   }

   // if we're assigning the left param must be a variable, the right could be an expression
   else if(isAssignment) {
      rval = EvalExpression(hWnd,strTrim(rval));
      if(lval.at(0) != '$') {
         DoError("Left hand of assignment must be a variable.");
         return "";
      }
      //make sure the variable exists, or if this appears to be a declaration add it
      lval = lval.substr(1);
      lval = strTrim(lval);
      int varid = FindVar((char*)lval.c_str());
      if(varid == NOTFOUND) { //create it
         varid = AddNewVar(lval);
         if(varid == NOTFOUND) {
            DoError("var err");
            return "";
         }
      }
      SetVarExp(varid,rval);

      return "";
   }



   rpos = lpos = NOTFOUND;
   lparen = rparen = NOTFOUND;
   instr = false;
   if(exp.find_first_of('(') != std::string::npos) {
      if(exp.at(0) == '"') instr = true;
      else if(exp.at(0) == '(') { lpos=0;++lparen; }

      for(i=1; i<exp.length(); ++i) {
         if(exp.at(i) == '"' && exp.at(i-1) != '\\') {
            instr = (instr) ? false : true;
            continue;
         }
         if(!instr) {
            if(exp.at(i) == '(') {
               if(lpos == NOTFOUND) lpos = i;
               ++lparen;
               continue;
            }
            if(exp.at(i) == ')') {
               if(++rparen == lparen) {
                  rpos = i;
                  break;
               }
               continue;
            }
         }
      }
      if(lparen > rparen) {
         DoError("Unmatched left parenthesis '('.");
         return "";
      }

      if(lpos == 0 && rpos == exp.length()-1) {
         exp = exp.substr(1,exp.length()-2);
         return EvalExpression(hWnd,strTrim(exp));
      }

      if(lparen != NOTFOUND) {      // anything to the left of ( is a command name,
         lval = strTrim(exp.substr(0,lpos));
         rval = strTrim(exp.substr(rpos+1));
         exp = strTrim(exp.substr(lpos+1,rpos-lpos-1)); // guts

         int cmndid = FindCommand((char*)lval.c_str());
         if(cmndid == NOTFOUND) {
            strtemp = "Command not found '" + lval + "'";
            DoError((char*)strtemp.c_str());
            return "";
         }
         exp = EvalExpression(hWnd,exp);

         return ExecuteCommand(hWnd,cmndid,(char*)exp.c_str()) + rval;

      }
   }

   // if there's no parenthesis this is just a constant value or a variable
   if(exp.at(0) == '$') {  
      exp = exp.substr(1);

      // first check for global variables
      int isGlobal = 0;
      std::string val = GetGlobalVarVal(hWnd, (char*)exp.c_str(), &isGlobal);
      if (isGlobal)
         return (char*)val.c_str();

      // if the variable exists return the value
      int thisvarid = FindVar((char*)exp.c_str());
      if(thisvarid != NOTFOUND) {
         return GetVarVal(thisvarid);
      }
      else { // or else error
        strtemp = "The variable '" + exp + "' does not exist. Did you declare it? Is it in scope?";
        DoError((char*)strtemp.c_str());
        return "";
      }

      // if there's a space treat it as a declaration
      /*
      pos = exp.find_first_of(' ');
      if(pos != std::string::npos) {
         // create it
         int varid = AddNewVar(exp);
         return "";
      }      
      
      // or else find it and return the value
      int thisvarid = FindVar((char*)exp.c_str());
      if(thisvarid != NOTFOUND) {
         return GetVarVal(thisvarid);
      }
      else { // bad var name 
        strtemp = "The variable '" + exp + "' does not exist. Did you declare it? Is it in scope?";
        DoError((char*)strtemp.c_str());
        return "";
      } 
      */
   }

   //DoError((char*)exp.c_str());

   return exp;
}



/**********/
void SetVarExp(int varid, std::string value) {

   if(varList[varid]->expression) delete varList[varid]->expression;
   varList[varid]->expression = new char[value.length()+1];
   strcpy(varList[varid]->expression,(char*)value.c_str());
}

/***************/
int AddNewVar(std::string strin) {
//   std::string strtype;

   int pos;
   //make sure the var name is valid
   bool isBad = false;
   if(strin.length()<1) isBad = true;
   char* badlist = "!@#$%^&*(){}[]<>/ \\\"'`~=?;";
   for(int i=0; i<strlen(badlist); ++i) {
      pos = strin.find_first_of(badlist[i]);
      if(pos != std::string::npos) {
         isBad = true;
         break;
      }
   }
   if(isBad) {
      strin = "Invalid character in variable name '" + strin + "'. Invalid characters are !@#$%^&*(){}[]<>/ \"\\'`~=?;.";
      DoError((char*)strin.c_str());
      return NOTFOUND;
   }

   int varid = AddVar();

   varList[varid]->varname = new char[strin.length()+1];
   strcpy(varList[varid]->varname,strin.c_str());
   //varList[varid]->type = nvartype;
   //varList[varid]->scope = GLOBALVAR;

   return varid;

}

/************** FIND VAR ******************/
int FindVar(char *varName) {
   // returns NOTFOUND if a variable is not defined
   if(iVarCount < 1) return NOTFOUND;

   int x=-1;
   while (++x<iVarCount) {
      if (varList[x]->varname)
         if (!strcmp(varList[x]->varname, varName)) return x;
   }
   return NOTFOUND;
}

/****** FIND VAR NAME ************/
char* FindVarName(int id)
{   if ((id < 0) || (id >= iVarCount))
      return "";
   return varList[id]->varname;
}

/**********/
std::string GetVarVal(int varid)
{
   if(varList[varid]->expression) 
       return EvalExpression(NULL,varList[varid]->expression);
   else
      return "";

};

std::string sGlobalVar;

std::string GetGlobalVarVal(HWND hWnd, char *name, int *found)
{
   *found = 0;
   if (name == NULL)
      return "";
   
   
   int retLen = kPlugin.kFuncs->GetGlobalVar(PREF_STRING, name, NULL);
   if (retLen) {
      *found = 1;
      char *retVal = new char[retLen+1];
      kPlugin.kFuncs->GetGlobalVar(PREF_STRING, name, retVal);
      sGlobalVar = retVal;
      delete retVal;
      return sGlobalVar;
   }


/*
   if (strcmp(name, "URL") == 0) {
      kmeleonDocInfo *dInfo = kPlugin.kFuncs->GetDocInfo(hWnd);
      if (dInfo && dInfo->url) {
         return dInfo->url;
      }
      else 
         return "";
   }
*/
   
  
   *found = 0;
   return "";
}

/************* ADD VAR *****************/
int AddVar() {

   var ** newVarList = new var*[iVarCount+1];
   if(iVarCount) {
      memcpy(newVarList, varList, ((iVarCount)*sizeof(var**)) );
      delete varList;
   }
   varList = newVarList;

   varList[iVarCount] = new var;
   varList[iVarCount]->varname = NULL;
   varList[iVarCount]->expression = NULL;
//   varList[iVarCount]->scope = GLOBALVAR;

   iVarCount++;
   return iVarCount - 1;

}






/************* misc *******************/
/**** DoError ******/
void DoError(char* msg) {
   MessageBox(NULL,msg,"Error",MB_OK);

}

/**** strTrim ***/
std::string strTrim(std::string instr) {
   int lpos = 0;
   int rpos = instr.length()-1;
   while(lpos <= rpos && instr.at(lpos) == ' ') ++lpos;
   while(rpos >=0 && instr.at(rpos) == ' ') --rpos;
   return instr.substr(lpos,rpos-lpos+1);
}

int strFindFirst(std::string instring,char findchar,bool notinparen) {

   bool instr = false;
   int pos = 0;
   int lparen = 0;
   int rparen = 0;
   if(instring.at(0) == '"') instr = true;
   if(instring.at(0) == '(') ++lparen;
   while(++pos < instring.length()) {
      if(instring.at(pos) == '"') {
         instr = (instr) ? false : true;
         continue;
      }
      if(!instr) {
         if(instring.at(pos) == '(') {
            ++lparen;
            continue;
         }
         if(instring.at(pos) == ')') {
            ++rparen;
            continue;
         }
         if(instring.at(pos) == findchar) {
            if(notinparen) {
               if(lparen==rparen) return pos;
            }
            else
               return pos;
            continue;
         }
      }
   }

   return std::string::npos;
}


int IntVal(std::string input) {
   
   input = strVal(input);
   return atoi(input.c_str());
   return 0;
}

int BoolVal(std::string input) {
   input = strVal(input);
   if(input.length() < 1 || strcmpi((char*)input.c_str(),"0")==0 || strcmpi((char*)input.c_str(),"false")==0) return false;
   return true;
}

std::string strVal(std::string input, int bOnlyQuotes) {

   if(input.length() < 1) {
      return input;
   }

   bool isstr = false;
   int len = input.length();
   if(input.at(0) == '"') {
      if(len-1 >=0 && input.at(len-1) == '"') {
         if(len-2 >= 0 && input.at(len-2) == '\\') {
            if(len-3 >= 0 && input.at(len-3) == '\\') {
               isstr = true;
            }
         }
         else isstr = true;
      }
   }
   if(isstr) input = input.substr(1,len-2);

   if (bOnlyQuotes == 1)
	  return input;

//   if(data.at(pos)=='"' && (data.at(pos-1) != '\\' || (data.at(pos-1) == '\\' && pos-2 >= 0 && data.at(pos-2) == '\\'))) {
//   if(input.at(0) == '"' && (input.at(input.length()-1) == '"' && (input.length()-2 > 0 && input.at(input.length()-2) == '\\'))) {
//   if(input.at(0) == '"' && input.at(input.length()-1) == '"' && input.at(input.length()-2) != '\\' && input.at(input.length()-3) != '\\') {
   int pos=-1;
   while(++pos < input.length()) {
      if(input.at(pos) == '\\') {
         if(pos == input.length()-1) {
            input = input.substr(0,pos);
            break;
         }           
         if(input.at(pos+1) == 'n') { //newline
           if (bOnlyQuotes==-1) { // FIXME: why not always convert '\'+'n' to '\n'?
             input.replace(pos,2,"\n");
             ++pos;
             continue;
           }				 
         }
         if(input.at(pos+1) == 't') { //tab
           if (bOnlyQuotes==-1) { // FIXME: why not always convert '\'+'t' to '\t'?
             input.replace(pos,2,"\t");
             ++pos;
             continue;
           }
         }
         if(input.at(pos+1) == '\\') {
            input.replace(pos,1,"");
            ++pos;
            continue;
         }
         input.replace(pos,1,"");
      }
   }

   return input;
}









// Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case WM_COMMAND:
      if ( (LOWORD(wParam) >= ID_START) && (LOWORD(wParam) <= ID_END) )
         ExecuteMacro(hWnd, LOWORD(wParam)-ID_START);
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}
