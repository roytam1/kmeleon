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
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string>

#define PLUGIN_NAME "Macro Extension Plugin"

#define KMELEON_PLUGIN_EXPORTS
#include "..\\kmeleon_plugin.h"
#include "..\\KMeleonConst.h"
#include "..\\utils.h"
#include "..\\strconv.h"
#include "macros.h"
#include <afxres.h>     // for ID_APP_EXIT


struct GVar {
	char *name;
	WindowVarType vartype;
	short type;
};

const struct GVar GlobalVars[] = {
	{ "SelectedText", Window_SelectedText, 3 },
	{ "TextZoom", Window_TextZoom, 1 },
	{ "URLBAR", Window_UrlBar, 2 },
	{ "URL", Window_URL, 2 },
	{ "TITLE", Window_Title, 2 },
	{ "LinkURL", Window_LinkURL, 2 },
	{ "ImageURL", Window_ImageURL, 2 },
	{ "FrameURL", Window_FrameURL, 2 },
	{ "CHARSET", Window_Charset, 2 },
};

#define GLOBALVARSCOUNT (sizeof(GlobalVars) / sizeof(struct GVar))

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
void         DoError(char* msg, char* filename, int line);
void         DoMenu(HMENU menu, char *param);
void         DoRebar(HWND rebarWnd);
int          DoAccel(char *param);
long         DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
std::string  EvalExpression(HWND hWnd,std::string exp);
std::string  ExecuteMacro(HWND hWnd, int macro);
std::string  ExecuteCommand (HWND hWnd, int command, char *data);
int          FindCommand(char *macroName);
int          FindMacro(char *macroName);
char*        FindMacroName(int id);
int          FindVar(char *varName);
char*        FindVarName(int id);
int          GetConfigFiles(configFileType **configFiles);
std::string  GetVarVal(int varid);
std::string  GetGlobalVarVal(HWND hWnd, char *name, int *found);
int SetGlobalVarVal(HWND hWnd, char *name, const char* value);
int          Load();
int          IntVal(std::string input);
bool         LoadMacros(TCHAR *filename);
void         Close(HWND hWnd);
void         CloseFrame(HWND hWnd, LONG windows);
void         Quit();
void         SetOption(std::string option);
void         SetVarExp(int varid, std::string value);
int          strFindFirst(const std::string& instring,char findchar,bool notinparen=false);
std::string  strTrim(const std::string& instr);
std::string  strVal(std::string input, int bOnlyQuotes = 0);

#ifdef _UNICODE
#define gUnicode TRUE
#else
BOOL gUnicode = FALSE;
#endif

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};
bool mustEscape(unsigned char c)
{
	static const char chars[]= " #$&+,/:;=?@";
	if (c>127 || strchr(chars, c)) return true;
	return false;
}

std::string Escape(std::string url)
{	
	static const char hexChars[] = "0123456789ABCDEF";
    url = strTrim(url);
	std::string ret;
	int len = url.length();
	for (int i=0;i<len;i++) {
		register unsigned char c = (unsigned char)url[i];
		if ( !mustEscape( c ) )
			ret += url[i];
		else if (url[i]==' ') {
			ret += "%20";
			while (i+1<len && url[i+1] == ' ') 
				++i;
		}
		else{
			ret += "%";
			ret += hexChars[c >> 4];
			ret += hexChars[c & 0x0f];
		}
	}
	return ret;
}

std::string EscapeURL(std::string url)
{	
	static const char hexChars[] = "0123456789ABCDEF";

	std::string ret;
	int len = url.length();
	bool escaped = false;
	for (int i=0;i<len;i++) {
		register unsigned char c = (unsigned char)url[i];
		if (  ! (c > 127 || (c == '|' && escaped)) )
			ret += url[i];
		else{
			ret += "%";
			ret += hexChars[c >> 4];
			ret += hexChars[c & 0x0f];
		}
		if (c > 127) escaped = true;
	}
	return ret;
}

BOOL SetWindowTextUTF8(HWND hWnd, LPCTSTR lpString)
{
	if (gUnicode)
		return SetWindowTextW(hWnd, CUTF8_to_UTF16(lpString));
	else
		return SetWindowTextA(hWnd, CUTF8_to_ANSI(lpString));
}

BOOL SetDlgItemTextUTF8(HWND hDlg, int nIDDlgItem, LPCTSTR lpString)
{
	if (gUnicode)
		return SetDlgItemTextW(hDlg, nIDDlgItem, CUTF8_to_UTF16(lpString));
	else
		return SetDlgItemTextA(hDlg, nIDDlgItem, CUTF8_to_ANSI(lpString));
}

BOOL GetWindowTextUTF8(HWND hwnd, std::string & str)
{
	int len = GetWindowTextLength(hwnd) + 1;
	if (len<=1) return FALSE;

	if (gUnicode) {
		WCHAR* tmp = (WCHAR*)malloc(len * sizeof(WCHAR));
		if (!tmp) return FALSE;
	
		GetWindowTextW(hwnd, tmp, len);
		str.assign(CUTF16_to_UTF8(tmp));
		free(tmp);
	}
	else {
		char* tmp = (char*)malloc(len * sizeof(char));
		if (!tmp) return FALSE;

		GetWindowTextA(hwnd, tmp, len);
		str.assign(CANSI_to_UTF8(tmp));
		free(tmp);
	}
	return TRUE;
}

UINT GetDlgItemTextUTF8(HWND hwnd, int nIDDlgItem, std::string & str)
{
	return GetWindowTextUTF8(GetDlgItem(hwnd, nIDDlgItem),  str);
}

UINT GetDlgItemTextUTF8(HWND hwnd, int nIDDlgItem, char* lpString, int nMaxCount)
{
	char *res = 0;
	if (gUnicode)
	{
		WCHAR* tmp = (WCHAR*)malloc(nMaxCount * sizeof(WCHAR));
		GetDlgItemTextW(hwnd, IDC_ANSWER, tmp, nMaxCount);
		res = utf8_from_utf16(tmp);
		free(tmp);
	}
	else
	{
		char* tmp = (char*)malloc(nMaxCount * sizeof(char));
		GetDlgItemTextA(hwnd, IDC_ANSWER, tmp, nMaxCount);
		res = utf8_from_ansi(tmp);
		free(tmp);
	}
	if (res) {
		int len = strlen(res);	
		if (len>=nMaxCount) res[nMaxCount-1] = 0;
		strcpy(lpString, res);
		free(res);
		return len;
	}
	return 0;
}

int MessageBoxUTF8(HWND hWnd, const char* lpText, const char* lpCaption, UINT uType)
{
	if (gUnicode)
		return MessageBoxW(hWnd, CUTF8_to_UTF16(lpText), CUTF8_to_UTF16(lpCaption), uType);
	else
		return MessageBoxA(hWnd, CUTF8_to_ANSI(lpText), CUTF8_to_ANSI(lpCaption), uType);
}

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{  
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
	 	if (stricmp(subject, "Init") == 0) {
         int index = FindMacro("OnInit");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
      }
	  if (stricmp(subject, "Init2") == 0) {
         int index = FindMacro("OnInit2");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
      }
	  if (stricmp(subject, "Setup") == 0) {
         int index = FindMacro("OnSetup2");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
      }
	  if (stricmp(subject, "Setup") == 0) {
         int index = FindMacro("OnSetup");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
      }
	  if (stricmp(subject, "Setup") == 0) {
         int index = FindMacro("OnSetup2");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
      }
	  if (stricmp(subject, "UserSetup") == 0) {
         int index = FindMacro("OnUserSetup");
         if (index != NOTFOUND)
            ExecuteMacro(NULL, index);
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
      else if (stricmp(subject, "Close") == 0) {
         Close((HWND)data1);
      }
      else if (stricmp(subject, "CloseFrame") == 0) {
         CloseFrame((HWND)data1, data2);
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
   std::string menuCheckState;
   std::string menuGrayState;
   struct event **eventList;
} **macroList;

struct var {
   char *varname;
   char *expression;
} **varList;


int wm_deferopenmsg = -1;
int iMacroCount = 0;
int iVarCount   = 0;
int bStartup    = 1;

#define BEGIN_CMD_TEST if (0) {}
#define CMD_TEST(CMD)  else if (stricmp(cmd, #CMD) == 0) { cmdVal = ##CMD; }
#define CMD(CMD)  else if (command == ##CMD)

enum commands {
   open = 1000,      // open url in current window
   opennew,          // open url in new window
   openbg,           // open url in new background window
   setpref,          // set a preference
   getpref,
   delpref,
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
   hostname,
   injectJS,
   injectCSS,
   forcecharset,
   readfile,
   readreg,
   promptforfile,
   promptforfolder,
   setcheck, 
   _,
   urlencode,
   getfolder,
   setmenu,
   setaccel,
   rebuildmenu,
   getwinvar,
   setwinvar
};

std::string sGlobalArg;

class ArgList {

  class Node {
  public:
    int id;
    std::string macro;
    std::string arg;
    class Node *next;
    Node(char *macro, char *arg) {
      this->macro = macro;
      this->arg = arg;
      id = kFuncs->GetCommandIDs(1);
      next = NULL;
    }
  };

protected:
  int min;
  int max;
  class Node *root;

public:
  ArgList() {
    root = NULL;
  }
  ~ArgList() {
	  class Node *ptr;
	  while (root)
	  {
		  ptr = root;
		  root = ptr->next;
		  delete ptr;
	  }

  }

  int add(char *macro, char *arg) {
    class Node *ptr = root;
    while (ptr && (strcmp(ptr->macro.c_str(), macro) || strcmp(ptr->arg.c_str(), arg)))
      ptr = ptr->next;
    if (ptr)
      return ptr->id;
    ptr = new Node(macro, arg);
    if (root == NULL)
      min = max = ptr->id;
    if (max < ptr->id)
      max = ptr->id;
    ptr->next = root;
    root = ptr;
    return ptr->id;
  }

  BOOL getfromid(int id, char**macro, char** arg) {
	  class Node *ptr = root;
	  while (ptr && ptr->id !=id) ptr = ptr->next;
	  if (!ptr) return FALSE;
	  *macro = (char*)ptr->macro.c_str();
	  *arg = (char*)ptr->arg.c_str();
	  return TRUE;
  }

  BOOL execute(HWND hWnd, int id) {
    if (id < min || id > max)
      return FALSE;
    class Node *ptr = root;
    while (ptr && ptr->id != id)
      ptr = ptr->next;
    if (ptr && ptr->id == id) {
      int cmdid = FindMacro((char*)ptr->macro.c_str());
      if (cmdid != NOTFOUND) {
	sGlobalArg = ptr->arg;
	ExecuteMacro(hWnd, cmdid);
	sGlobalArg = "";
	return TRUE;
      }
    }
    return FALSE;
  }
}* arglist;


bool LoadDir(TCHAR* macrosDir)
{
	TCHAR szMacroFile[MAX_PATH];
	_tcscpy(szMacroFile, macrosDir);
    _tcscat(szMacroFile, _T("*.kmm"));

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile(szMacroFile, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	do {
		if (_tcsicmp("main.kmm", FindFileData.cFileName) == 0)
			continue;

		char prefload[MAX_PATH+40];
		CharLowerBuff(FindFileData.cFileName, MAX_PATH);
		strcpy(prefload, "kmeleon.plugins.macros.modules.");
		strncat(prefload, FindFileData.cFileName, strrchr(FindFileData.cFileName, '.')-FindFileData.cFileName); //uni
		strcat(prefload, ".load");

		BOOL load = TRUE;
		kFuncs->GetPreference(PREF_BOOL, prefload, &load, &load);

		if (load) {
			_tcscpy(szMacroFile, macrosDir);
			_tcscat(szMacroFile, FindFileData.cFileName);
			LoadMacros(szMacroFile);
		}
	} while (FindNextFile(hFind, &FindFileData));
	
	return true;
}

bool LoadNewMacros()
{
	TCHAR macrosDir[MAX_PATH];
	kFuncs->GetFolder(RootFolder, macrosDir, MAX_PATH);
	_tcscat(macrosDir, "\\macros\\");

	TCHAR profileMacrosDir[MAX_PATH];
	kFuncs->GetFolder(UserSettingsFolder, profileMacrosDir, MAX_PATH);
	_tcscat(profileMacrosDir, "\\macros\\");

	TCHAR szMacroFile[MAX_PATH];
	_tcscpy(szMacroFile, macrosDir);
	_tcscat(szMacroFile, _T("main.kmm"));
	if (!LoadMacros(szMacroFile))
		return false;

	LoadDir(macrosDir);
	LoadDir(profileMacrosDir);
	
	return true;
}

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

int Load() {
   kFuncs = kPlugin.kFuncs;
   arglist = new ArgList;
   
#ifndef _UNICODE
   OSVERSIONINFO osinfo;
   osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osinfo);
   gUnicode = (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
#endif

   TCHAR szMacroFile[MAX_PATH];
   kFuncs->GetFolder(UserSettingsFolder, szMacroFile, MAX_PATH);

   if (! *szMacroFile)
      return 0;

   _tcscat(szMacroFile, _T("\\macros.cfg"));
   if (!LoadMacros(szMacroFile)) {
	  LoadNewMacros();
   }
      
   wm_deferopenmsg = kPlugin.kFuncs->GetCommandIDs(1);

   return 1;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);

	PostMessage(hWndParent, WM_COMMAND, wm_deferopenmsg, 0);
}

void Config(HWND hWndParent) {
   char cfgPath[MAX_PATH];
   kFuncs->GetFolder(UserSettingsFolder, cfgPath, MAX_PATH);
   strcat(cfgPath, "\\macros");
   ShellExecuteA(NULL, NULL, cfgPath, NULL, NULL, SW_SHOW);
}

configFileType g_configFiles[1];

int GetConfigFiles(configFileType **configFiles)
{
   char cfgPath[MAX_PATH];
   kFuncs->GetFolder(UserSettingsFolder, cfgPath, MAX_PATH);

   strcpy(g_configFiles[0].file, cfgPath);
   strcat(g_configFiles[0].file, "macros.cfg");

   strcpy(g_configFiles[0].label, "Macros");

   strcpy(g_configFiles[0].helpUrl, "http://www.kmeleon.org");

   *configFiles = g_configFiles;

   return 1;
}

void Close(HWND hWnd) {
  int index = FindMacro("OnCloseWindow");
  if (index != NOTFOUND)
    ExecuteMacro(hWnd, index);
}

void CloseFrame(HWND hWnd, LONG windows) {
  int index = FindMacro("OnCloseGroup");
  if (index != NOTFOUND)
    ExecuteMacro(hWnd, index);
}

void Quit() {
   if (iMacroCount == 0) return;

   int index = FindMacro("OnQuit");
   if (index != NOTFOUND)
      ExecuteMacro(NULL, index);

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
   delete arglist;

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

      int id = -1;

      char *arg = strchr(param, '(');
      if (arg) {
	*(arg++) = 0;
	char *p = strchr(arg, ')');
	if (p) {
	  *p = 0;
	  id = arglist->add(param, arg);
	}
      }

      int index = FindMacro(param);

      if (id != -1) {
	if (string)
	  AppendMenu(menu, MF_STRING, id, string);
	else if (index != NOTFOUND && macroList[index]->menuString)
	  AppendMenu(menu, MF_STRING, id, CUTF8_to_T(macroList[index]->menuString));
	else if (index != NOTFOUND && macroList[index]->macroName)
	  AppendMenu(menu, MF_STRING, id, CUTF8_to_T(macroList[index]->macroName));
	else 
	  AppendMenu(menu, MF_STRING, id, _T("Untitled Macro"));
      }
   }
}

int DoAccel(char *param) {
   if (*param) {
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         do {
            string++;
         } while (*string==' ' || *string=='\t');
      }

      int id = -1;

		char *arg = strchr(param, '(');
		if (arg) {
			*(arg++) = 0;
			char *p = strrchr(arg, ')');
			if (p) {
				*p = 0;
			}
		}
		
		id = arglist->add(param, arg ? arg : "");
		return id;
   }
   return 0;
}

void DoRebar(HWND rebarWnd) {
}

int FindCommand(char *cmd) {

   int cmdVal = NOTFOUND;
   
   BEGIN_CMD_TEST
      CMD_TEST(_)
      CMD_TEST(setcheck)
      CMD_TEST(getwinvar)
      CMD_TEST(setwinvar)
      CMD_TEST(open)
      CMD_TEST(opennew)
      CMD_TEST(openbg)
      CMD_TEST(setpref)
      CMD_TEST(getpref)
      CMD_TEST(delpref)
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
      CMD_TEST(urlencode)
      CMD_TEST(getfolder)
      CMD_TEST(setmenu)
      CMD_TEST(setaccel)
      CMD_TEST(basename)
      CMD_TEST(dirname)
      CMD_TEST(hostname)
      CMD_TEST(injectJS)
      CMD_TEST(injectCSS)
      CMD_TEST(readfile)
      CMD_TEST(readreg)
      CMD_TEST(promptforfile)
      CMD_TEST(promptforfolder)
      CMD_TEST(forcecharset)
      CMD_TEST(rebuildmenu)

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


std::string ExecuteMacro (HWND hWnd, int macro) {
   if ((macro < 0) || (macro >= iMacroCount))
      return "";

   std::string ret;
   for (int x=0; x<macroList[macro]->eventCount; x++) {
      if(macroList[macro]->eventList[x]->data) {
         ret = EvalExpression(hWnd,macroList[macro]->eventList[x]->data);
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
   return ret;
}

// Thanks to Ulf Erikson for the tokenizer to clean things up in here...
class Tokenizer {
protected:
   std::string strData;
   unsigned int pos,lpos;
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
       if(strData.at(pos) == '\\' && lastchar == '\\') {
         lastchar = 0;
         continue;
       }
		 if(strData.at(pos) == '"' && lastchar != '\\') {
			instr = (instr) ? false : true;
			lastchar = '"';
         continue;
      }
		 if(!instr) {
			if(strData.at(pos) == ',') {
            if (out)
				  *out = strVal(strData.substr(lpos, pos-lpos), -1);
			   lpos = pos+1;
			   lastchar = ',';
            return 1;
         }
      }
		 lastchar = strData.at(pos);
   }
   if (out)
		 *out = strVal(strData.substr(lpos), -1);
   return 1;
}
};

static void parseError(int err, char *cmd, char *args, int data1=0, int data2=0) {
   char title[256];
   char* msg = new char[strlen(cmd) + strlen(args) + 70];
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
   delete [] msg;
}

int CALLBACK BrowseProc(      
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, true, lpData);
	else if (uMsg == BFFM_VALIDATEFAILED)
		return 1;
	return 0;
}

std::string title = "";
std::string question = "";
std::string instring = "";
std::string answer = "";

BOOL CALLBACK
PromptDlgProc( HWND hwnd,
	      UINT Message,
	      WPARAM wParam,
	      LPARAM lParam )
{
    switch (Message) {
      case WM_INITDIALOG: {
	    SetWindowTextUTF8(hwnd, title.c_str());
        SetDlgItemTextUTF8(hwnd, IDC_PROMPT, question.c_str());
        SetDlgItemTextUTF8(hwnd, IDC_ANSWER, instring.c_str());
        return TRUE;
					}
      case WM_COMMAND:
        switch (LOWORD(wParam)) {
	  case IDOK: {
	    char tmp[256];
		GetDlgItemTextUTF8(hwnd, IDC_ANSWER, tmp, 256);
	    answer = tmp;
	    EndDialog( hwnd, IDOK );
	    break;
				 }
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

std::string protectString( const char *pszData ) {
   if (!pszData) return "\"\"";
   // put the clipboard data in quotes to distinguish it from commands
   std::string retval = "\"";
   retval.append(pszData);  // the clipboard data
   // escape any " or \ in the clipboard data
   unsigned int pos=0;
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
	   CMD(_) {
		   if (nparam != 1) {  // open( $0 )
			parseError(WRONGARGS, "_", data, 1, nparam);
            return "";
         }

		   return protectString( 
			 (LPCSTR)CANSI_to_UTF8(
			   kFuncs->Translate( 
			     (LPCSTR)CUTF8_to_ANSI(params[0].c_str()) ) ) );

	   }

      CMD(open) {
         if (nparam != 1) {  // open( $0 )
			parseError(WRONGARGS, "open", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)(EscapeURL(params[0]).c_str()), OPEN_NORMAL, NULL);
      }
      CMD(opennew) {
         if (nparam != 1) {  // opennew( $0 )
            parseError(WRONGARGS, "opennew", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)EscapeURL(params[0]).c_str(), OPEN_NEW, NULL);
      }
      CMD(openbg) {
         if (nparam != 1) {  // openbg( $0 )
            parseError(WRONGARGS, "openbg", data, 1, nparam);
            return "";
         }
         kFuncs->NavigateTo((char*)EscapeURL(params[0]).c_str(), OPEN_BACKGROUND, NULL);
      }
      CMD(setpref)   {
         enum PREFTYPE preftype;

         if (nparam != 3) {  // setpref( $0, $1, $2 )
            parseError(WRONGARGS, "setpref", data, 3, nparam);
            return "";
         }

         if (!strcmpi((char*)params[0].c_str(), "bool")) preftype = PREF_BOOL;
         else if (!strcmpi((char*)params[0].c_str(), "int")) preftype = PREF_INT;
         else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_UNISTRING;
         else {
            parseError(WRONGTYPE, "setpref", data);
            return "";
         }
         char *pref = (char*)params[1].c_str();
         data = (char*)params[2].c_str();

         // Thanks to Mynen (mark_yen@hotmail.com) for pointing out this bug
         //  as well as submitting a patch
         if (data) {

			 if (preftype == PREF_UNISTRING)
				 kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(data), TRUE);

            else if (preftype == PREF_INT) {
	      if (*data) {
               // note that SetPreference() expects third param
               // to be a pointer in all cases, even for int and bool
               int iData = atoi(data);
               kFuncs->SetPreference(preftype, pref, &iData, TRUE);
	      }
            } 

            else {   // boolean
	      if (*data) {
               int bData = FALSE;
               if (!strcmpi(data, "true"))
                  bData = TRUE;
               kFuncs->SetPreference(preftype, pref, &bData, TRUE);
	      }
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
         else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_UNISTRING;
         else {
			parseError(WRONGTYPE, "getpref", data);
            return "";
         }

         int nRetval = 0;
         if (preftype == PREF_UNISTRING) {
			long len = kFuncs->GetPreference(preftype,(char*)params[1].c_str(),0,L"");			
			wchar_t* cRetval = (wchar_t*)calloc(sizeof(wchar_t), len+1);
			kFuncs->GetPreference(preftype,(char*)params[1].c_str(),cRetval,L"");
            std::string strRet;
			strRet = protectString(CUTF16_to_UTF8(cRetval));
			free(cRetval);
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
      CMD(delpref)   {
         if (nparam != 1) {  // setpref( $0 )
            parseError(WRONGARGS, "delpref", data, 1, nparam);
            return "";
         }

         char *pref = (char*)params[0].c_str();
         kFuncs->DelPreference(pref);
      }
      CMD(togglepref) {
         enum PREFTYPE preftype;

         if (nparam) {
            if (!strcmpi((char*)params[0].c_str(), "bool")) preftype = PREF_BOOL;
            else if (!strcmpi((char*)params[0].c_str(), "int")) preftype = PREF_INT;
            else if (!strcmpi((char*)params[0].c_str(), "string")) preftype = PREF_UNISTRING;
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

         int  iVal=0;
         const char *pref = params[1].c_str();
		 std::string sVal;

		 if (preftype == PREF_UNISTRING) {
			long len = kFuncs->GetPreference(preftype,(char*)params[1].c_str(),0,0);			
			wchar_t* tmp = (wchar_t*)calloc(sizeof(wchar_t), len+1);
			kFuncs->GetPreference(preftype,pref,tmp,"");
			sVal = CUTF16_to_UTF8(tmp);
			free(tmp);
		 }

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
            if (preftype == PREF_UNISTRING) {
               if (!strcmp(param, sVal.c_str())) {
                  if (t.nextToken( &str )) {
                     param = (char*)str.c_str();
					 kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(param), TRUE);
                  }
                  else
                     kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(prefdata), TRUE);
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
			if (preftype == PREF_TSTRING)
                 kFuncs->SetPreference(preftype, pref, (void*)(const char*)CUTF8_to_T(prefdata), TRUE);
            else if (preftype == PREF_INT) {
               int val = atoi(prefdata);
               kFuncs->SetPreference(preftype, pref, &val, TRUE);
            }
         }
      }

      CMD(exec) {
         STARTUPINFO si = {0};
         PROCESS_INFORMATION pi = {0};

#ifdef _UNICODE
		 CUTF8_to_UTF16 strcommand(params[0].c_str());
#else
		 CUTF8_to_ANSI strcommand(params[0].c_str());
#endif
		 TCHAR command[MAX_PATH];
		 ExpandEnvironmentStrings(strcommand, command, MAX_PATH);
     	 CreateProcess(NULL, command, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
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
	     kPlugin.kFuncs->SetStatusBarText(CUTF8_to_T(params[0].c_str()));
         return "";
      }
      CMD(alert) {
         // params are message,title,icon

         int icon = 0;
         if(strcmpi(params[2].c_str(),"EXCLAIM")==0) icon=MB_ICONEXCLAMATION;
         else if(strcmpi(params[2].c_str(),"INFO")==0) icon=MB_ICONINFORMATION;
         else if(strcmpi(params[2].c_str(),"STOP")==0) icon=MB_ICONSTOP;         
         else if(strcmpi(params[2].c_str(),"QUESTION")==0) icon=MB_ICONQUESTION;         

		 MessageBoxUTF8(hWnd, params[0].c_str(), params[1].c_str(), MB_OK|icon|MB_TASKMODAL);

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

	     int result = MessageBoxUTF8(hWnd, strVal(params[0], -1).c_str(), params[1].c_str(), buttons|icon|MB_TASKMODAL);
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
         if (nparam > 3) {  // statusbar( [$0 [,$1]] )
            parseError(WRONGARGS, "prompt", data, 3, nparam);
            return "";
         }


	question = strVal(params[0],-1);
	title = params[1];
	instring = params[2];
	int ok;

	if (gUnicode)
		ok = DialogBoxW(kPlugin.hDllInstance,
		  MAKEINTRESOURCEW(IDD_PROMPT), hWnd, (DLGPROC)PromptDlgProc);
	else
		ok = DialogBoxA(kPlugin.hDllInstance,
		  MAKEINTRESOURCEA(IDD_PROMPT), hWnd, (DLGPROC)PromptDlgProc);

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
	
         if(  !IsClipboardFormatAvailable(CF_UNICODETEXT)
			 && !IsClipboardFormatAvailable(CF_TEXT) )
			 return "";

         if(!OpenClipboard(NULL)) {
            DoError("Error opening the clipboard.");
            return "";
         }

         void* pszData;
         LPVOID pData;
         HANDLE hcb;

		 if (gUnicode) {
			hcb = GetClipboardData(CF_UNICODETEXT);
		}
		 else {
			hcb = GetClipboardData(CF_TEXT);
		 }
		 
		 if (!hcb) {
			 CloseClipboard();
			 return ""; 
		 }

         pData = GlobalLock(hcb);
		 if (gUnicode){
             pszData = malloc( sizeof(WCHAR)* (wcslen((WCHAR*)pData) + 1) );
			 wcscpy((WCHAR*)pszData, (LPWSTR)pData);
		 }
		 else
		 {
			 pszData = malloc( sizeof(char)* (strlen((char*)pData) + 1) );
			 strcpy((char*)pszData, (LPSTR)pData);
		 }

         GlobalUnlock(hcb);
          
         CloseClipboard();

         // put the clipboard data in quotes to distinguish it from commands
         std::string retval;
		 char* tmp = 0;
		 if (gUnicode)
			 tmp = utf8_from_utf16((const WCHAR*)pszData);
		 else
			 tmp = utf8_from_ansi((const char*)pszData);
		 if (tmp) {
			retval = protectString( tmp );
			free(tmp);
		 }

		 free(pszData);
         return retval;
     }

      CMD(setclipboard) {
         // set data to the clipboard
         if(!OpenClipboard(NULL)) {
            DoError("Error opening the clipboard.");
            return "";
         }
		 
		 int len, datasize, clipformat;
		 void* pszData;
		 if (gUnicode) {
			pszData = (void*)utf16_from_utf8(params[0].c_str());
			datasize = sizeof(wchar_t);
			len = wcslen((wchar_t*)pszData);
			clipformat = CF_UNICODETEXT;
			}
		 else {
			 pszData = (void*)ansi_from_utf8(params[0].c_str());
			 datasize = sizeof(char);
			 len = strlen((char*)pszData); 
			 clipformat = CF_TEXT;
		 }
		
         HGLOBAL hData;
         LPVOID pData;

         EmptyClipboard();
		 std::string ret = "0";
		 
		 hData = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,datasize*(len + 1));
		 if (hData) {
			pData = GlobalLock(hData);
			if (gUnicode)
				wcscpy((LPWSTR)pData, (wchar_t*)pszData);
			else
				strcpy((LPSTR)pData, (char*)pszData);
			GlobalUnlock(hData);
		
			SetClipboardData(clipformat, hData);

	        ret = "1";
		 }
		 CloseClipboard();
		 free(pszData);
		 return ret;
      }
      CMD(macros) {
         // execute another macro
		 std::string ret;
		 std::string macro;

		 int pos = params[0].find(';');
		 macro = strTrim(params[0].substr(0,pos));

		 while (macro.length()) {
				/*int parampos = macro.find('(');
				if (parampos != macro.npos && (macro[macro.length()-1] == ')')) {
					sGlobalArg =  EvalExpression(hWnd, macro.substr(parampos+1, macro.length()-parampos-2));
					macro = macro.substr(0, parampos);
				}*/
			
            int cmdid = FindMacro((char*)macro.c_str());
            if(cmdid == NOTFOUND) {
               std::string msg = "Invalid macro reference. The macro '";
               msg.append(macro.c_str());
               msg.append("' does not exist.");
               DoError((char*)msg.c_str());
            }
            else {
               ret = ExecuteMacro(hWnd,cmdid);
            }
			
			pos != params[0].npos ?
			   params[0].erase(0,pos+1) :
			   params[0].erase();

            pos = params[0].find(';');
	        macro = params[0].substr(0,pos);
		 }
		 return ret;
      }


      /*
         PluginMsg ( "DestPlugin", "Command", "Param1", "Param2");

         Send a command to a plugin that does not need a return value.
         If the plugin needs more than two parameters, it will have to
         parse them out of a string passed as either Param1 or Param2
      */

      CMD(pluginmsg) {
      
         if((nparam != 3) && (nparam != 4))  {
            parseError(WRONGARGS, "PluginMsg", data, 4, nparam);
            return "";
         }
		 
		 kFuncs->SendMessage(CUTF8_to_ANSI(params[0].c_str()),
							 PLUGIN_NAME, 
							 CUTF8_to_ANSI(params[1].c_str()), 
							 (long) (const char*)CUTF8_to_ANSI(params[2].c_str()), 
							 (long) (const char*)CUTF8_to_ANSI(params[3].c_str()));
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
		 std::string strRet;
         if (preftype == PREF_STRING) {
            char *cRetval = NULL;
			strRet = protectString("");
           
			kFuncs->SendMessage(
				CUTF8_to_ANSI(params[0].c_str()), 
				PLUGIN_NAME, 
				CUTF8_to_ANSI(params[1].c_str()), 
				(long) (const char*)CUTF8_to_ANSI(params[2].c_str()), 
				(long) &cRetval);

			if (cRetval) {
			   strRet = protectString(CANSI_to_UTF8(cRetval));
			   free(cRetval);
			}
         }
         else if (preftype == PREF_INT) {
            int nRetval = 0;

            kFuncs->SendMessage(
				CUTF8_to_ANSI(params[0].c_str()), 
				PLUGIN_NAME, 
				CUTF8_to_ANSI(params[1].c_str()), 
				(long) (const char*)CUTF8_to_ANSI(params[2].c_str()), 
				(long) &nRetval);

            char buffer[12];
            _itoa(nRetval,buffer,10);
             strRet = buffer;
         }
         return strRet;
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
         if (nparam != 4) {  // gensub( $0, $1, $2, $3 )
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
         if (nparam != 3) {  // gsub( $0, $1, $2 )
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
         if (nparam != 2) {  // index( $0, $1 )
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
         if (nparam != 1) {  // length( $0 )
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
         if (nparam != 3) {  // sub( $0, $1, $2 )
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
		 int i = atoi( (char *)params[1].c_str() );
		 int n = nparam == 3 ? atoi( (char *)params[2].c_str() ) : 0;
		 int len = params[0].length();
		 if (i<0)     i = 0;
		 if (i>len)   i = len;
		 if (n<0)     n = 0;
		 if (n>len-i) n = len-i;

	 if (nparam == 2)
			retval = params[0].substr(i);
	 else
			retval = params[0].substr(i, n);

		 retval = protectString( (char*)retval.c_str() );
		 return retval;
      }

	  /* escape( string ) */
	  CMD(urlencode) {
	      if (nparam != 1) { 
            parseError(WRONGARGS, "urlencode", data, 1, nparam);
            return "";
         }

	     return protectString( (char*)Escape(params[0]).c_str() );
	  }
      
      /*
         basename( NAME [, SUFFIX] );

	 Returns NAME with any leading directory components removed.
	 If specified, also remove a trailing SUFFIX.
      */

      CMD(basename) {
         if (nparam != 1 && nparam != 2) {  // basename( $0, [, $1] )
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
         if (nparam != 1) {  // dirname( $0 )
            parseError(WRONGARGS, "dirname", data, 1, nparam);
            return "";
         }
	 int i, j;

	 int len = params[0].length();
	 if (len < 1) return protectString("");

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
         if (nparam != 1) {  // hostname( $0 )
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

	  CMD(injectJS) {
		  if (nparam > 2) {  
            parseError(WRONGARGS, "injectJS", data, 2, nparam);
            return "";
         }
		 bool bTopWindow = (params[1] != "frame");
		 kPlugin.kFuncs->InjectJS(params[0].c_str(), bTopWindow, hWnd);
	  }

	  CMD(injectCSS) {
		  if (nparam != 1) {  
            parseError(WRONGARGS, "injectCSS", data, 1, nparam);
            return "";
         }

		  kPlugin.kFuncs->InjectCSS(params[0].c_str(), true, hWnd);
	  }

	  CMD(readfile) {
		 if (nparam != 1) {  
            parseError(WRONGARGS, "readfile", data, 1, nparam);
            return "";
         }
		 if (params[0].empty())
			 return "";
		 FILE* f = fopen(params[0].c_str(), "r");
		 if (f) {
			 char* buffer = new char[32768];
			 int size = fread(buffer, sizeof(char), 32768-1, f);
			 buffer[size] = 0;
			 std::string ret = protectString(buffer);
			 fclose(f);
             delete  [] buffer;
			 return ret;
		 }
		 return protectString("");
	  }

	  CMD(readreg) {
		  if (nparam != 2) {
			parseError(WRONGARGS, "readreg", data, 2, nparam);
            return "";
         }
		
	     HKEY root;
         if (params[0] == "HKLM") root = HKEY_LOCAL_MACHINE;
		 else if (params[0] == "HKCU") root = HKEY_CURRENT_USER;
		 else if (params[0] == "HKCR") root = HKEY_CLASSES_ROOT;
		 else if (params[0] == "HKU") root = HKEY_USERS;
		 else if (params[0] == "HKCC") root = HKEY_CURRENT_CONFIG;

		 TCHAR* keyPath = t_from_utf8(params[1].c_str());
		 if (!keyPath) return "";

		 TCHAR* name = keyPath + _tcslen(keyPath);
		 while (name>keyPath && *name!=_T('\\'))
			 name--;
		 
		 if (name != keyPath)
			*name++ = 0;
		 else 
			 name = NULL;

		 HKEY key;
		 if (RegOpenKeyEx(root, keyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
			free(keyPath);			
			return "";
		 }
      
		 std::string ret;
		 DWORD dwSize, dwType;
		 if (RegQueryValueEx(key, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS &&
			 dwSize>0)
		 {
  		   
  		   LPBYTE lpData = (LPBYTE)malloc(dwSize);
		   if (lpData)
		   {
			 if (RegQueryValueEx(key, name, 0, &dwType, lpData, &dwSize) == ERROR_SUCCESS)
			 {
				if (dwType == REG_DWORD)
				{
					char dword[33];
					_itoa(*((LPDWORD)lpData), dword, 10);
					ret = protectString(dword);
				}
				else
					ret = protectString(CT_to_UTF8((LPTSTR)lpData));
			 }
			 free(lpData);
		   }
		 }
		 free(keyPath);
		 RegCloseKey(key);
		 return ret;
	  }

	  CMD(promptforfolder) {
		  if (nparam > 2) {
			parseError(WRONGARGS, "promptforfolder", data, 1, nparam);
            return "";
         }

		  CUTF8_to_ANSI caption(params[0].c_str());
		  if (!(LPCTSTR)caption) return "";

		  CUTF8_to_ANSI initFolder(params[1].c_str());

          LPITEMIDLIST pidlRetBrowse = NULL, pidlRoot = NULL;
		  
		  CoInitialize(NULL);
		  std::string ret;
		  LPITEMIDLIST idl; 
		  TCHAR pszDisplayName[MAX_PATH];
		  BROWSEINFO bi = {hWnd, NULL, pszDisplayName, caption, BIF_EDITBOX | BIF_NEWDIALOGSTYLE,BrowseProc,(LPARAM)(LPCTSTR)initFolder,NULL};
		  idl = SHBrowseForFolder(&bi);
		  if (idl != NULL)
		  {
			  if (SHGetPathFromIDList(idl, pszDisplayName))
				  ret = protectString(CT_to_UTF8(pszDisplayName));
			  CoTaskMemFree(idl);
		  }
		  CoUninitialize();
		  return ret;
	  }

	  CMD(promptforfile) {
		  if (nparam != 3) {  
            parseError(WRONGARGS, "promptforfile", data, 3, nparam);
            return "";
         }
		 
		 std::string filter = params[1] + std::string("|") + params[2] + std::string("||");

		 TCHAR* pszFilter = t_from_utf8(filter.c_str());
		 if (!pszFilter) return "";

		 for (TCHAR* p = pszFilter; *p; p++)
			if ((*p) == '|') (*p) = 0;

		 OPENFILENAME ofn;
		 memset(&ofn, 0, sizeof(ofn));
		 ofn.lStructSize = sizeof(ofn);
		 ofn.hwndOwner = hWnd;
		 ofn.lpstrFilter =pszFilter;
		 ofn.lpstrFile = new TCHAR[MAX_PATH]; 
		 ofn.lpstrFile[0] = 0;
		 ofn.nMaxFile = MAX_PATH;
		 if (!params[0].empty())
		    ofn.lpstrInitialDir = t_from_utf8(params[0].c_str());

		 ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		 std::string ret;
		 if( ::GetOpenFileName(&ofn) ) 
			 ret = protectString(CT_to_UTF8(ofn.lpstrFile));
		 else
		 	ret = protectString("");
		 delete[] ofn.lpstrFile;
		 free(pszFilter);
		 if (ofn.lpstrInitialDir)
			 free((void*)ofn.lpstrInitialDir);
        
		 return ret;
	  }
      /*
         forcecharset( charset );
      */

      CMD(forcecharset) {
         if (nparam > 1) {  // forcecharset( $0 )
            parseError(WRONGARGS, "forcecharset", data, 1, nparam);
            return "";
         }

         kFuncs->SetForceCharset(nparam < 1 ? "" : (char*)strVal(params[0], 1).c_str());
         return "";
      }

      /*
         setcheck( id );
      */

      CMD(setcheck) {
         if (nparam < 1 || nparam > 2) {  // setcheck( $0, $1 )
            parseError(WRONGARGS, "setcheck", data, 2, nparam);
            return "";
         }

         int cmd;
         cmd = kPlugin.kFuncs->GetID((char*)params[0].c_str());
         if (!cmd)
            cmd = atoi((char*)params[0].c_str());

         BOOL mark = TRUE;
         if (nparam == 2)
            mark = BoolVal(EvalExpression(hWnd, params[1]));

         kFuncs->SetCheck(cmd, mark);

         return "";
      }

      /*
         setmenu( name, type, rebuild, ... );
      */

      CMD(setmenu) {
         if (nparam < 2) {  
            parseError(WRONGARGS, "setmenu", data, 2, nparam);
            return "";
         }

			//setmenu( "name", "command", "label", command, where)
			//setmenu( "name", "macro", "label", macroname, where)
			//setmenu( "name", "popup", "label", where)
			//setmenu( "name", "inline", "label", where)
			//setmenu( "name", "separator", where)
			//setmenu( "name", "plugin", plugin, where)

			kmeleonMenuItem item;
			int whereparam = 0;
			item.label = params[2].c_str();
         item.command = 1;

			if (params[1] == "command") {
				whereparam = 4;
				item.type = MENU_COMMAND;
				item.command = kFuncs->GetID((char*)params[3].c_str());
				if (!item.command)
					item.command = IntVal(params[3].c_str());
			}
			else if (params[1] == "macro") {
				whereparam = 4;
				item.type = MENU_COMMAND;
				item.command = DoAccel((char*)params[3].c_str());
				if (!item.command)
					item.command = IntVal(params[3].c_str());
			}
			else if (params[1] == "popup") {
				whereparam = 3;
				item.type = MENU_POPUP;
			}
			else if (params[1] == "inline") {
				whereparam = 3;
				item.type = MENU_INLINE;
			}
			else if (params[1] == "separator") {
				whereparam = 2;
				item.type = MENU_SEPARATOR;
			}
			else if (params[1] == "plugin") {
				whereparam = 2;
				item.type = MENU_PLUGIN;
			}
			else {
				parseError(WRONGARGS, "setmenu", data, 2, nparam);
				return "0";
			}

			if (params[whereparam].empty())
				item.before = -1;
			else {
				item.before = IntVal(params[whereparam]);
            if (!item.before && params[whereparam] != "0") {
				   item.before = kFuncs->GetID((char*)std::string(params[whereparam]).c_str()); 
				   if (!item.before)
					   item.before = (long)params[whereparam].c_str();
            }
			}

			kFuncs->SetMenu(params[0].c_str(), &item);
			return "1";
		}

		CMD(rebuildmenu) {
			if (nparam != 1) {  
				parseError(WRONGARGS, "rebuildmenu", data, 1, nparam);
				return "";
			}
			kFuncs->RebuildMenu(params[0].c_str());
		}

      CMD(setaccel) {
         if (nparam != 2) {  
            parseError(WRONGARGS, "setaccel", data, 3, nparam);
            return "";
         }
         
         kFuncs->SetAccel(params[0].c_str(), (char*)params[1].c_str());
         return "";
      }

      CMD(getfolder) {
         if (nparam != 1) {  
            parseError(WRONGARGS, "setcheck", data, 1, nparam);
            return "";
         }
         char path[MAX_PATH] = {0};
         const char *dirname = params[0].c_str();
            FolderType foldertype;
         
         if (stricmp(dirname, "RootFolder") == 0)
            foldertype = RootFolder;
         else if (stricmp(dirname, "SettingsFolder") == 0)
            foldertype = UserSettingsFolder;
         else if (stricmp(dirname, "ProfileFolder") == 0)
            foldertype = ProfileFolder;
         else if (stricmp(dirname, "ResFolder") == 0)
            foldertype = ResFolder;
         else if (stricmp(dirname, "SkinFolder") == 0)
            foldertype = CurrentSkinFolder;
         else if (stricmp(dirname, "MacroFolder") == 0) {
            kFuncs->GetFolder(RootFolder, path, MAX_PATH);
            strcat(path, "\\macros");
            return protectString(path);
         }
         else if (stricmp(dirname, "UserMacroFolder") == 0) {
            kFuncs->GetFolder(UserSettingsFolder, path, MAX_PATH);
            strcat(path, "\\macros");
            return protectString(path);
			}
         else return "";

         kFuncs->GetFolder(foldertype, path, MAX_PATH);
         return protectString(CT_to_UTF8(path));
      }


/*
		CMD(getwinvar) {
			if (nparam != 1) {  
            parseError(WRONGARGS, "getwinvar", data, 1, nparam);
            return "";
         }
         
			WindowVarType type;
			if (params[0] == "SelectedText") {
				int l = kPlugin.kFuncs->GetWindowVar(hWnd, Window_SelectedText, NULL);
				if (l<1) return "";
				wchar_t* buf = new wchar_t[l];
				kPlugin.kFuncs->GetWindowVar(hWnd, Window_SelectedText, buf);
				std::string ret = protectString(CUTF16_to_UTF8(buf));
				delete [] buf;
				return ret;
			}

			if (params[0] == "TextZoom") {
				int zoom;
				kPlugin.kFuncs->GetWindowVar(hWnd, Window_TextZoom, &zoom);
				char buf[34];
				_itoa(zoom, buf, 10);
				return buf;
			}
			
			if (params[0] == "URLBAR")
				type = Window_UrlBar;
			else if (params[0] == "URL")
				type = Window_URL;
			else if (params[0] == "TITLE")
				type = Window_Title;
			else if (params[0] == "LinkURL")
				type = Window_LinkURL;
			else if (params[0] == "ImageURL")
				type = Window_ImageURL;
			else if (params[0] == "FrameURL")
				type = Window_FrameURL;
			else if (params[0] == "CHARSET")
				type = Window_Charset;
			else return "";

			int l = kPlugin.kFuncs->GetWindowVar(hWnd, type, NULL);
			if (l<1) return "";

			char* buf = new char[l];
			kPlugin.kFuncs->GetWindowVar(hWnd, type, buf);
			std::string ret = protectString(CANSI_to_UTF8(buf));
			delete [] buf;
			return ret;
		}

		CMD(setwinvar) {
			if (nparam != 2) {  
            parseError(WRONGARGS, "setwinvar", data, 2, nparam);
            return "";
         }

			WindowVarType type;
			if (params[0] == "TextZoom") {
				kPlugin.kFuncs->SetWindowVar(hWnd, Window_TextZoom, (void*)IntVal(params[1].c_str()));
				return "";
			}
			else if (params[0] == "URLBAR")
				type = Window_UrlBar;
			else if (params[0] == "URL")
				type = Window_URL;
			else if (params[0] == "TITLE")
				type = Window_Title;
			else
				return "";

			kPlugin.kFuncs->SetWindowVar(hWnd, type, (void*)(const char*)CUTF8_to_ANSI(params[1].c_str()));
		}
*/
   return "";
}

bool IsNumber(const char* string)
{
	char* p=SkipWhiteSpace((char*)string);
	if (*p != '+' && *p!='-') return false;
	while (*++p) {
		if (*p<'0'||*p>'9')
			return false;
	}
	return true;
}

bool LoadMacros(TCHAR *filename) {
   HANDLE macroFile;

   macroFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, (WPARAM)NULL, NULL);
   if (macroFile == INVALID_HANDLE_VALUE) {
      //macroFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, (WPARAM)NULL, NULL);
      //MessageBox(NULL, _T("Could not open file"), filename, MB_OK);
      return false;
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

        // Convert data to UTF8.
	    char* utf8 = utf8_from_ansi(thisLine.c_str());
		thisLine = utf8;
		free(utf8);

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
            DoError("Missing macro name.", filename, nLineCount);
            break;
         }
         char* badlist = "!@#$%^&*(){}[]<>/ \\\"'`~=?;";
         pos = -1;
         bool isBad = false;
         for(unsigned int i=0; i<strlen(badlist); ++i) {
            pos = strtemp.find(badlist[i]);
            if(pos != std::string::npos) {
               isBad = true;
               break;
            }            
         }
         if(isBad) {
            DoError("Bad character in macro name.", filename, nLineCount);
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
		    const char* tr = "Macro redefinition: %s.";
		    char* msg = new char[strtemp.length() + strlen(tr) + 1];
			sprintf(msg, tr, strtemp.c_str());
            DoError(msg, filename, nLineCount);
			delete [] msg;
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
            DoError("Invalid macro definition. Missing opening bracket?", filename, nLineCount);
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
                     DoError("Cannot set menu string to empty value.", filename, nLineCount);
                     needBreak = true; break;
                  }
                  if(macroList[iMacroCount-1]->menuString) delete macroList[iMacroCount-1]->menuString;
                  strtemp = strVal(EvalExpression(NULL,rval), -1); // FIXME: ugly misuse to convert '\'+'t' to '\t'
                  macroList[iMacroCount-1]->menuString = new char[strtemp.length()+1];
                  strcpy(macroList[iMacroCount-1]->menuString,strtemp.c_str());
                  continue;
               }
					else if(strcmpi(lval.c_str(),"menuchecked") == 0) {
						rval = strTrim(thisStatement.substr(pos+1));
						macroList[iMacroCount-1]->menuCheckState = rval;
						continue;
					}
					else if(strcmpi(lval.c_str(),"menugrayed") == 0) {
						rval = strTrim(thisStatement.substr(pos+1));
						macroList[iMacroCount-1]->menuGrayState = rval;
						continue;
					}
            }
         }

         // if it begins with an ampersand this is a reference to another macro
         if(thisStatement.at(0) == '&') {
            thisStatement = strTrim(thisStatement.substr(1));
            if(thisStatement.length() < 1) {
               DoError("Invalid macro reference.", filename, nLineCount);
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

   return true;
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
      char lastchar = exp.at(0);
      for(i=1; i<exp.length(); ++i) {
         if(exp.at(i) == '\\' && lastchar == '\\') {
            lastchar = 0;
            continue;
         }
         if(exp.at(i) == '"' && lastchar != '\\') {
            instr = (instr) ? false : true;
            continue;
         }
         lastchar = exp.at(i);
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

   // if it begins with an ampersand this is a reference to another macro
   if(exp.at(0) == '&') {
     strtemp = "macros(" + exp.substr(1) + ")";
     strtemp = EvalExpression(hWnd, strTrim(strtemp));
     return strtemp.c_str();
   }

   // while (expression) statement;
   if (strncmp(exp.c_str(), "while", 5) == 0) {
      int lpos, rpos;
      int lparen, rparen;
      bool instr;
      
      lpos = rpos = NOTFOUND;
      lparen = rparen = 0;
      instr = false;
      
      for (unsigned int j=1; j<exp.length(); ++j) {
	 if (exp.at(j) == '"' && exp.at(j-1) != '\\') {
	    instr = (instr) ? false : true;
	    continue;
	 }
	 if (!instr) {
	    if(exp.at(j) == '(') {
	       ++lparen;
	       if (lparen == 1)
		 lpos = j;
	       continue;
	    }
	    if(exp.at(j) == ')') {
	       ++rparen;
	       if (lparen == rparen)
		 rpos = j;
	       break;
	    }
	 }
      }

      if (lpos != NOTFOUND && rpos != NOTFOUND) {
	std::string pre = strTrim(exp.substr(0,lpos));
	std::string expr = strTrim(exp.substr(lpos+1,rpos-(lpos+1)));
	std::string stmt = strTrim(exp.substr(rpos+1));

	std::string copy;
	std::string result = "";

	copy = expr.c_str();
	int b = BoolVal(EvalExpression(hWnd, copy));
	while (b) {
	  copy = stmt.c_str();
	  result = EvalExpression(hWnd, copy);
	  copy = expr.c_str();
	  b = BoolVal(EvalExpression(hWnd, copy));
	}
	return result;
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
      for (unsigned int j=1; j<exp.length(); ++j) {
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
	   exp.at(epos+1) != '=' && exp.at(epos-1) != '!' && 
	   exp.at(epos-1) != '<' && exp.at(epos-1) != '>') {
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
	  if (instr && exp.at(i) == '\\') {
		 i++;
		 continue;
	  }
	  if(exp.at(i) == '"') {
	  //if(exp.at(i) == '"' && exp.at(i-1) != '\\') {
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
            else if(exp.at(i) == '<' ||
		    exp.at(i) == '>') {
	      isComparison = true;
	      int pos = i;
	      int size = 1;
	      if(exp.at(i+1)=='=') ++i, ++size;
	      lval = exp.substr(0,pos);
	      rval = exp.substr(pos+size);
	      strtemp = exp.substr(pos,size); //the comparison operator
	      break;
	    }
            else if(exp.at(i) == '+' ||   // addition 
		    exp.at(i) == '-') {   // subtraction
               int last_op = i;
               int j = i;
	       for(j=i; j<exp.length(); ++j)
		  if(exp.at(j) == '"' && exp.at(j-1) != '\\')
		     instr = (instr) ? false : true;
		  else if(!instr)
		     if(exp.at(j) == '(')
			++lparen;
		     else if(exp.at(j) == ')')
			++rparen;
		     else if(lparen == rparen)
			if(exp.at(j) == '+' ||   // addition 
			   exp.at(j) == '-')     // subtraction
			   last_op = j;
	       
               int res_l = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,last_op))));
	       int res_r = IntVal(EvalExpression(hWnd,strTrim(exp.substr(last_op+1))));
	       int res = 0;
	       if (exp.at(last_op) == '+')
		  res = res_l + res_r;
	       else if (exp.at(last_op) == '-')
		  res = res_l - res_r;
			   char buf[34];
               itoa(res,buf,10);
               return buf;
            }
            else if(exp.at(i) == '*' ||   // multiplication
		    exp.at(i) == '/' ||   // division
		    exp.at(i) == '%') {   // remainder
               int last_op = i;
               int j = i;
	       for(j=i; j<exp.length(); ++j)
		  if(exp.at(j) == '"' && exp.at(j-1) != '\\')
		     instr = (instr) ? false : true;
		  else if(!instr)
		     if(exp.at(j) == '(')
			++lparen;
		     else if(exp.at(j) == ')')
			++rparen;
		     else if(lparen == rparen)
			if(exp.at(j) == '*' ||   // multiplication
			   exp.at(j) == '/' ||   // division
			   exp.at(j) == '%')     // remainder
			   last_op = j;
	       
               int res_l = IntVal(EvalExpression(hWnd,strTrim(exp.substr(0,last_op))));
	       int res_r = IntVal(EvalExpression(hWnd,strTrim(exp.substr(last_op+1))));
	       int res = 0;
	       if (exp.at(last_op) == '*')
		  res = res_l * res_r;
	       else if (exp.at(last_op) == '/')
		  if (res_r == 0)   // illegal division by zero
		     return "0";
		  else
		     res = res_l / res_r;
	       else if (exp.at(last_op) == '%')
		  if (res_r == 0)   // illegal division by zero
		     return "0";
		  else
		     res = res_l % res_r;
			   char buf[34];
               itoa(res,buf,10);
               return buf;
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
	  if (IsNumber(strVal(lval, 1).c_str()) && IsNumber(strVal(rval, 1).c_str()))
	  {
		  int lival = IntVal(lval);
		  int rival = IntVal(rval);
		  if (strtemp == "==") 
			  return (lival == rival) ? "1" : "0";
		  else if (strtemp == "!=") 
			  return (lival != rival) ? "1" : "0";
		  else if (strtemp == "<") 
			  return (lival < rival) ? "1" : "0";
		  else if (strtemp == ">") 
			  return (lival > rival) ? "1" : "0";
		  else if (strtemp == "<=") 
			  return (lival <= rival) ? "1" : "0";
		  else if (strtemp == ">=") 
			  return (lival >= rival) ? "1" : "0";
		  else
			  return "";
	  }

      if (strtemp == "==") 
	return (strVal(lval) == strVal(rval)) ? "1" : "0";
      else if (strtemp == "!=") 
	return (strVal(lval) != strVal(rval)) ? "1" : "0";
      else if (strtemp == "<") 
	return (strVal(lval) < strVal(rval)) ? "1" : "0";
      else if (strtemp == ">") 
	return (strVal(lval) > strVal(rval)) ? "1" : "0";
      else if (strtemp == "<=") 
	return (strVal(lval) <= strVal(rval)) ? "1" : "0";
      else if (strtemp == ">=") 
	return (strVal(lval) >= strVal(rval)) ? "1" : "0";
      else
	return "";
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
	  if (SetGlobalVarVal(hWnd, (char*)lval.c_str(), strVal(rval).c_str()))
		  return "";
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
		
      char lastchar = exp.at(0);
      for(i=1; i<exp.length(); ++i) {
         if(exp.at(i) == '\\' && lastchar == '\\') {
            lastchar = 0;
            continue;
         }
         if(exp.at(i) == '"' && lastchar != '\\') {
            instr = (instr) ? false : true;
            continue;
         }
         lastchar = exp.at(i);
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
         return protectString( (char*)val.c_str() );

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
   for(unsigned int i=0; i<strlen(badlist); ++i) {
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

int SetGlobalVarVal(HWND hWnd, char *name, const char* value)
{
	for (int i=0;i<GLOBALVARSCOUNT;i++) 
		if ( stricmp(GlobalVars[i].name, name) == 0) {
			switch (GlobalVars[i].type) {
				case 0: 
				case 1:
					kPlugin.kFuncs->SetWindowVar(hWnd, GlobalVars[i].vartype, (void*)IntVal(value));
					return 1;

				case 2:
					kPlugin.kFuncs->SetWindowVar(
						hWnd, GlobalVars[i].vartype, 
						(void*)(const char*)CUTF8_to_ANSI(value));
					return 1;

				case 3:
					kPlugin.kFuncs->SetWindowVar(
						hWnd, GlobalVars[i].vartype, 
						(void*)(const wchar_t*)CUTF8_to_UTF16(value));
					return 1;
			}
		}
	
	return 0;
	/*
	int ret = kPlugin.kFuncs->SetGlobalVar(PREF_STRING, name, (void*)(const char*)CUTF8_to_ANSI(value));
	if (!ret) {
		int val = IntVal(value);
		ret = kPlugin.kFuncs->SetGlobalVar(PREF_INT, name, (void*)&val);
	}
	return ret;*/
}

std::string GetGlobalVarVal(HWND hWnd, char *name, int *found)
{
   *found = 0;
   if (name == NULL)
      return "";
   
   if (strcmp(name, "ARG") == 0) {
      *found = 1;
      return sGlobalArg;
   }

	for (int i=0;i<GLOBALVARSCOUNT;i++) 
		if ( stricmp(GlobalVars[i].name, name) == 0) {
			*found = 1;
			switch (GlobalVars[i].type) {
				case 0: 
				case 1: {
					int val;
					kPlugin.kFuncs->GetWindowVar(hWnd, GlobalVars[i].vartype, &val);
					char buf[34];
					_itoa(val, buf, 10);
					return buf;
				}

				case 2: {
					int l = kPlugin.kFuncs->GetWindowVar(hWnd, GlobalVars[i].vartype, NULL);
					if (l<1) return "";
					char* buf = new char[l];
					kPlugin.kFuncs->GetWindowVar(hWnd, GlobalVars[i].vartype, buf);
					std::string ret = CANSI_to_UTF8(buf);
					delete [] buf;
					return ret;
				}

				case 3: {
					int l = kPlugin.kFuncs->GetWindowVar(hWnd, GlobalVars[i].vartype, NULL);
				   if (l<1) return "";
				   wchar_t* buf = new wchar_t[l];
				   kPlugin.kFuncs->GetWindowVar(hWnd, GlobalVars[i].vartype, buf);
				   std::string ret = CUTF16_to_UTF8(buf);
				   delete [] buf;
				   return ret;
				}
			}
		}
  
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
   MessageBoxUTF8(NULL,msg,"Error",MB_OK);
}
void DoError(char* msg, TCHAR* filename, int line) {
   TCHAR* slash = _tcsrchr(filename, '\\');
   if (slash) filename = slash+1;

#ifdef _UNICODE
   CANSI_to_UTF16 tMsg(msg);
#else
   const char* tMsg = msg;
#endif

   TCHAR lineBuf[34];
   _itot(line, lineBuf, 10);

   TCHAR* buf = new TCHAR[lstrlen(tMsg) + lstrlen(filename) + 55];
   _stprintf(buf, "%s \r\nIn file %s at line %d", tMsg, filename, line);

   MessageBox(NULL,buf,_T("Error"),MB_OK);
   delete buf;
}

/**** strTrim ***/
std::string strTrim(const std::string& instr) {
   int lpos = 0;
   int rpos = instr.length()-1;
   while(lpos <= rpos && isspace(instr.at(lpos))) ++lpos;
   while(rpos >=0 && isspace(instr.at(rpos))) --rpos;
   return instr.substr(lpos,rpos-lpos+1);
}

int strFindFirst(const std::string& instring,char findchar,bool notinparen) {

   bool instr = false;
   unsigned int pos = 0;
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
      else {
         if(instring.at(pos) == '\\') {
            ++pos;
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
             continue;
           }				 
         }
         if(input.at(pos+1) == 't') { //tab
           if (bOnlyQuotes==-1) { // FIXME: why not always convert '\'+'t' to '\t'?
             input.replace(pos,2,"\t");
             continue;
           }
         }
         if(input.at(pos+1) == '\\') {
            input.replace(pos,1,"");
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
     if (LOWORD(wParam)==ID_APP_EXIT) {
       int index = FindMacro("OnWMAppExit");
       if (bStartup && index != NOTFOUND)
         ExecuteMacro(hWnd, index);
     }
#if 0
     else if (LOWORD(wParam)==WM_CLOSE) {
       int index = FindMacro("OnWMClose");
       if (bStartup && index != NOTFOUND)
         ExecuteMacro(hWnd, index);
     }
#endif
     else if (LOWORD(wParam)==wm_deferopenmsg) {
       int index = FindMacro("OnStartup");
       if (bStartup && index != NOTFOUND)
          ExecuteMacro(hWnd, index);
       bStartup = 0;
       
       index = FindMacro("OnOpenWindow");
       if (index != NOTFOUND)
          ExecuteMacro(hWnd, index);
     }
     else
         arglist->execute(hWnd, LOWORD(wParam));
     break;
   case UWM_UPDATEBUSYSTATE:
      {
		  if (wParam == 0) {
         int index = FindMacro("OnLoad");
         if (index != NOTFOUND)
            ExecuteMacro(hWnd, index);
      }
      }
      break;
   case WM_ACTIVATE:
	   if ( LOWORD(wParam) == WA_ACTIVE
		   || LOWORD(wParam) == WA_CLICKACTIVE) {	

	      // Let k-meleon update the last browser frame value.
		  LRESULT result = CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
	   	  int index = FindMacro("OnActivateWindow");
		  if (index != NOTFOUND) 
            ExecuteMacro(hWnd, index);
	      return result;
			}

	case WM_INITMENUPOPUP: {

		// Let MFC do his little update
		LRESULT res = CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);

		HMENU menu = (HMENU)wParam;
		int count = GetMenuItemCount(menu);
		for (int i=0;i<count;i++)
		{
			int id = GetMenuItemID(menu, i);
			if (id>0) {
				char *macro, *param;
				if (arglist->getfromid(id, &macro, &param)) {
					sGlobalArg = param;
					int mid = FindMacro(macro);
					if (mid != NOTFOUND) {

						MENUITEMINFO mif;
						mif.cbSize = sizeof(mif);
						mif.fMask = MIIM_STATE;
						GetMenuItemInfo(menu, i, TRUE, &mif);


						//std::string label = EvalExpression(hWnd, macroList[mid]->menuString);
						if (macroList[mid]->menuCheckState.length()) {
							BOOL checked = BoolVal(EvalExpression(hWnd, macroList[mid]->menuCheckState));
							mif.fState &= ~MF_CHECKED & ~MF_UNCHECKED;
							mif.fState |= checked ? MF_CHECKED : MF_UNCHECKED;
						}

						if (macroList[mid]->menuGrayState.length()) {
							BOOL grayed = BoolVal(EvalExpression(hWnd, macroList[mid]->menuGrayState));
							mif.fState &= ~MF_GRAYED & ~MF_ENABLED;
							mif.fState |= grayed ? MF_GRAYED : MF_ENABLED;
						}
							
						::SetMenuItemInfo(menu, i, TRUE, &mif);
					}
					sGlobalArg = "";
				}

			}
		}
		return res;
		}
		
		break;
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}
