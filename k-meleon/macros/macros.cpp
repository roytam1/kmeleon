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

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\utils.h"


#define NOTFOUND -1

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);
int AddMacro();
void AddMacroEvent(int macro, int eventID, char *eventData);
int FindMacro(char *macroName);
int FindCommand(char *macroName);
void LoadMacros(char *filename);
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved );
void ExecuteMacro(int macro);
void ExecuteCommand (int command, char *data);

pluginFunctions pFunc = {
   Init,
   Create,
   Config,
   Quit,
   DoMenu,
   DoRebar,
   DoAccel
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   "Macro Extension Plugin",
   &pFunc
};

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


int ID_START    = -1;
int ID_END      = -1;
int iMacroCount = 0;


#define BEGIN_CMD_TEST if (0) {}
#define CMD_TEST(CMD)  else if (stricmp(cmd, #CMD) == 0) { cmdVal = ##CMD; }
#define CMD(CMD)  else if (command == ##CMD)

enum commands {
   open = 1000,
   opennew,
   openbg
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
   char szMacroFile[MAX_PATH];

   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.general.settingsDir", szMacroFile, "");

   if (! *szMacroFile)
      return 0;

   strcat(szMacroFile, "macros.cfg");

   LoadMacros(szMacroFile);
   ID_START = kPlugin.kf->GetCommandIDs(iMacroCount);
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
}

void DoMenu(HMENU menu, char *param) {
   if (*param) {
      int index = FindMacro(param);
      if (index != NOTFOUND) {
         if (macroList[index]->menuString)
            AppendMenu(menu, MF_STRING, ID_START+index, macroList[index]->menuString);
         else if (macroList[index]->macroName)
            AppendMenu(menu, MF_STRING, ID_START+index, macroList[index]->macroName);
         else 
            AppendMenu(menu, MF_STRING, ID_START+index, "Untitled Macro");
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
void AddMacroEvent(int macro, int eventID, char *eventData) {

   // create new, larger index and copy the old index over
   event ** newEventList = new event*[macroList[macro]->eventCount+1];
   if (macroList[macro]->eventCount) {
      memcpy(newEventList, macroList[macro]->eventList, ((macroList[macro]->eventCount)*sizeof(event**)) );
      delete macroList[macro]->eventList;
   }
   macroList[macro]->eventList = newEventList;

   // set the data
   macroList[macro]->eventList[macroList[macro]->eventCount] = new event;
   macroList[macro]->eventList[macroList[macro]->eventCount]->id   = eventID;
   if (eventData != NULL) {
      macroList[macro]->eventList[macroList[macro]->eventCount]->data = new char[strlen(eventData) + 1];
      strcpy(macroList[macro]->eventList[macroList[macro]->eventCount]->data, eventData);
   }
   else macroList[macro]->eventList[macroList[macro]->eventCount]->data = NULL;
   macroList[macro]->eventCount++;
}


void ExecuteMacro (int macro) {
   if ((macro < 0) || (macro >= iMacroCount))
      return;

   for (int x=0; x<macroList[macro]->eventCount; x++) {
      if (macroList[macro]->eventList[x]->id < 1000)
         ExecuteMacro(macroList[macro]->eventList[x]->id);
      else ExecuteCommand(macroList[macro]->eventList[x]->id, macroList[macro]->eventList[x]->data);
   }
}

void ExecuteCommand (int command, char *data) {

   BEGIN_CMD_TEST
      CMD(open)      kPlugin.kf->NavigateTo(data, OPEN_NORMAL);
      CMD(opennew)   kPlugin.kf->NavigateTo(data, OPEN_NEW);
      CMD(openbg)    kPlugin.kf->NavigateTo(data, OPEN_BACKGROUND);

}

void LoadMacros(char *filename) {
   HANDLE macroFile;

   macroFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, NULL, NULL);
   if (macroFile == INVALID_HANDLE_VALUE) {
      MessageBox(NULL, "Could not open file", filename, MB_OK);
      return;
   }
   DWORD length = GetFileSize(macroFile, NULL);
   char *buf = new char[length + 1];
   ReadFile(macroFile, buf, length, &length, NULL);
   buf[length] = 0;
   CloseHandle(macroFile);  
   

   BOOL buildingMacro = false;

   char *p = strtok(buf, "\r\n");
   while (p) {

      p = SkipWhiteSpace(p);
      TrimWhiteSpace(p);

      if (p[0] == '#') {
      }
      
      // "MacroName {"
      else if (!buildingMacro) {
         // There can only be 2 things outside a macro
         //   comments, and the beginning of a macro block
         char *cb = strchr(p, '{');
         if (cb) {
            *cb = 0;
            p = SkipWhiteSpace(p);
            TrimWhiteSpace(p);
            // add new macro, if it doesn't already exist
            if (FindMacro(p) == NOTFOUND) {
               int curMacro = AddMacro();
               macroList[curMacro]->macroName = new char[strlen(p) + 1];
               strcpy (macroList[curMacro]->macroName, p);
               buildingMacro = true;
            }
         }
      }

      else {
         p = SkipWhiteSpace(p);
         TrimWhiteSpace(p);

         // reference to another macro
         if (p[0] == '&'){
            p++;
            int index = FindMacro(p);
            if (index != NOTFOUND)
               AddMacroEvent(iMacroCount-1, index, NULL);
         }

         // end block
         else if (p[0] == '}') {
            buildingMacro = false;
         }

         // just a normal event, it's either an assignment or a command
         else {
            char *op = NULL;
            char *e = NULL;

            // if there's an open parenthesis, we'll assume it's a command
            if ((op = strchr(p, '('))) {
               char *params = op + 1;
               char *cp = strrchr(params, ')');
               if (cp) *cp = 0;
               *op = 0;

               p=SkipWhiteSpace(p);
               TrimWhiteSpace(p);

               // figure out event id of string "p"
               int command = FindCommand(p);
               if (command != NOTFOUND)
                  AddMacroEvent(iMacroCount-1, command, params);
            }
            // check for an assignment
            else if ((e = strchr(p, '='))) {
               *e = 0;
               e++;
               e = SkipWhiteSpace(e);
               TrimWhiteSpace(e);

               p = SkipWhiteSpace(p);
               TrimWhiteSpace(p);

               // it's a menu assignment
               if (strcmpi("menu", p) == 0) {
                  macroList[iMacroCount-1]->menuString = new char[strlen(e) + 1];
                  strcpy(macroList[iMacroCount-1]->menuString, e);
               }
            }            
         }
      } // currentMacro
      p = strtok(NULL, "\r\n");
   } // while

   delete buf;
}


// Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case WM_COMMAND:
      if ( (LOWORD(wParam) >= ID_START) && (LOWORD(wParam) <= ID_END) )
         ExecuteMacro(LOWORD(wParam)-ID_START);
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}