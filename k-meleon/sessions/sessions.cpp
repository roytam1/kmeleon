.-/*
  *  Copyright (C) 2005 Dorian Boissonnade
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



  /* 
  Menu to use
  Load Session{
  session()
  }

  &Sessions{
  :Load Session
  sessions(save, Save Session)
  sessions(undo, Undo Last Closed)
  }

  Supported accel

  sessions(save)
  sessions(undo)
  */

#include "stdafx.h"
#include "resource.h"

#define _Tr(x) kPlugin.kFuncs->Translate(_T(x))

#define PLUGIN_NAME "Session Saver Plugin"

#define PREFERENCE_SESSION_AUTOLOAD   "kmeleon.plugins.sessions.autoload"
#define PREFERENCE_SESSION_OPENSTART   "kmeleon.plugins.sessions.openStart"
#define PREFERENCE_SESSION_ASKAUTOLOAD   "kmeleon.plugins.sessions.ask_autoload"
#define PREFERENCE_SESSION_MAXUNDO   "kmeleon.plugins.sessions.maxUndo"
#define PREFERENCE_CLEANSHUTDOWN "kmeleon.plugins.sessions.cleanShutdown"
  char* kPreviousSessionName = "Previous Session";
char* kLastSessionName = "Last Session";

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
	KMEL_PLUGIN_VER_UTF8,
	PLUGIN_NAME,
	DoMessage
};

#include "sessions.h"

bool Session::loading = false;
int Session::openOption = 0;

void Window::addTab(HWND hWnd) {	
	int option = 0;
	kPlugin.kFuncs->GetPreference(PREF_INT, "kmeleon.tabs.onOpenOption", &option, &option);
	// Until we have a better option
	if (option != 1 || !tabsList.size())
		tabsList.push_back(Tab(hWnd, this->hWnd));
	else {
		HWND hWndCurrent = kPlugin.kFuncs->GetCurrent(hWnd);	
		TABLIST::iterator iter = findTab(hWndCurrent);
		if (iter != tabsList.end()) iter++;
		tabsList.insert(iter , Tab(hWnd, this->hWnd));
	}
	tabcount++;
}

Session currentSession;
Session undo;
UINT_PTR timerID = NULL;


int Load();
int Init();
void Create(HWND parent);
void Destroy(HWND parent);
void CreateTab(HWND parent, HWND tab);
void DestroyTab(HWND parent, HWND tab);
void MoveTab(HWND parent, HWND tab);

void Config(HWND parent);
void Close(HWND parent);
void DoMenu(HMENU menu, char *param);
int DoAccel(char *param);
void Undo(HWND hWnd);
void Quit();

kmeleonFunctions *kFuncs;
WNDPROC KMeleonWndProc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int id_undo_close;
int id_save_session;
int id_load_session;
int id_delete_session;
int id_open_previous;
int last_session_id;
int id_config;

int bFirstStart = 1;
//int gMaxUndo = 5;
bool gLoading = false;
Locale* gLoc = NULL;

HMENU sessionsMenu = NULL;

TCHAR sessionFile[1024];


void BuildSessionMenu()
{
	if (!IsMenu(sessionsMenu)) return;

	int count;
	while (count = GetMenuItemCount(sessionsMenu))
		DeleteMenu(sessionsMenu, 0, MF_BYPOSITION);

	last_session_id = id_load_session;


	for(unsigned i=0;i<SessionStore::GetSessionsList().size();i++)
	{
		if (stricmp(SessionStore::sessions_list[i], kLastSessionName)!=0) 
		{
			if (strcmp(SessionStore::sessions_list[i], kPreviousSessionName) == 0)
				AppendMenu(sessionsMenu, MF_STRING, last_session_id, gLoc->GetString(IDS_PREVIOUS_SESSION));
			else
				AppendMenuA(sessionsMenu, MF_STRING, last_session_id, SessionStore::sessions_list[i]);

		}
		last_session_id++;
		if (last_session_id>id_load_session + MAX_SAVED_SESSION) break;
	}
}

// Have to use my own, because oji initialisation use the crt one
// The default now skip empty token, breaking everything
char* _strtok (char * string, const char * control)
{
	static char* nextoken;

	if (string)
		nextoken = string;

	char *tok;
	if (nextoken) {
		if ( (tok = strstr(nextoken, control)) ) {
			*tok = 0;
			char *ret = nextoken;
			nextoken = tok + strlen(control);
			return ret;
		}
		else {
			char *ret = nextoken;
			nextoken = nullptr;
			return ret;
		}
	}
	return nullptr;
}


void DeleteSession(const char* name)
{
	currentSession.deleteSession(name);
}

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (strcmp(subject, "Load") == 0) {
			return Load();
		}
		if (strcmp(subject, "UserSetup") == 0) {
			return Init();
		}
		else if (strcmp(subject, "SwitchTab") == 0) {
			int selected = 0;
			if (!data2) return 0;
			Window* w = currentSession.getWindow(GetParent((HWND)data2));
			if (w) {
				kPlugin.kFuncs->GetWindowVar((HWND)data2, Window_Tab_Index, &selected);
				w->selectedTab = selected;
			}
		}	  
		else if (strcmp(subject, "Create") == 0) {
			Create((HWND)data1);
		}
		else if (strcmp(subject, "Destroy") == 0) {
			Destroy((HWND)data1);
		}
		else if (strcmp(subject, "CreateTab") == 0) {
			CreateTab((HWND)data1, (HWND)data2);
		}
		else if (strcmp(subject, "DestroyTab") == 0) {
			DestroyTab((HWND)data1, (HWND)data2);
		}
		else if (strcmp(subject, "MoveTab") == 0) {
			MoveTab((HWND)data1, (HWND)data2);
		}
		else if (strcmp(subject, "DoMenu") == 0) {
			DoMenu((HMENU)data1, (char *)data2);
		}
		else if (strcmp(subject, "DoAccel") == 0) {
			*(int *)data2 = DoAccel((char *)data1);
		}
		else if (strcmp(subject, "Config") == 0) {
			Config((HWND)data1);
		}
		else if (strcmp(subject, "Undo") == 0) {
			Undo(0);
		}
		else if (strcmp(subject, "Quit") == 0) {
			Quit();
		}
		else if (strcmp(subject, "DoLocale") == 0) {
			if (gLoc) delete gLoc;
			gLoc = Locale::kmInit(&kPlugin);
		}
		else return 0;

		return 1;
	}
	return 0;
}




HINSTANCE ghInstance;
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

	// Not compatible with the layers plugin.
	kmeleonPlugin* layers = kFuncs->Load("layers.dll");
	if (layers && layers->loaded) {
		kPlugin.description = PLUGIN_NAME " [Not Compatible With Layers]";
		return -1;
	}

	gLoc = Locale::kmInit(&kPlugin);
	if (!gLoc) {
		kPlugin.description = PLUGIN_NAME " [Locale failed to initialize]";
		return -1;
	}

	char s[1024];
	kPlugin.kFuncs->GetFolder(ProfileFolder,s, sizeof(s));
	strcat_s(s, "\\sessions.json");
	_utf8_to_utf16(s, sessionFile, 1024);
	DWORD dwAttrib = GetFileAttributes(sessionFile);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || 
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{	   
		if (!SessionStore::Import()) {
			DeleteFile(sessionFile);
			MessageBox(NULL, 
				gLoc->GetStringFormat(IDS_IMPORT_FAILED),
				NULL,
				MB_OK|MB_ICONERROR);
		}
	}   

	id_undo_close = kPlugin.kFuncs->GetCommandIDs(1);
	id_save_session = kPlugin.kFuncs->GetCommandIDs(1);
	id_config = kPlugin.kFuncs->GetCommandIDs(1);
	id_open_previous = kPlugin.kFuncs->GetCommandIDs(1);
	last_session_id = id_load_session = kPlugin.kFuncs->GetCommandIDs(MAX_SAVED_SESSION); 
	//id_delete_session = kPlugin.kFuncs->GetCommandIDs(MAX_SAVED_SESSION); 

	return 1;
}

#define PWM_AUTOLOAD WM_APP + 1000
#define PWM_HACKRESTORE WM_APP + 1001

void CreateTab(HWND parent, HWND tab)
{
	currentSession.addTab(parent, tab);
}

void MoveTab(HWND newtab, HWND oldtab)
{
	Window* w = currentSession.findWindowWithTab(newtab);
	if (w) {
		w->moveTab(newtab, oldtab);
		currentSession.saveSession(kLastSessionName);
	}
}

HWND destroying = NULL;

void DestroyTab(HWND parent, HWND tab)
{
	if (destroying != parent) {
		Window* w = currentSession.getWindow(parent);
		if (w) { 
			Tab t = w->getTab(tab);	
			Window ww = Window(NULL);
			ww.addTab(t);
			undo.addWindow(ww);
		}
	}

	currentSession.removeTab(parent, tab);	
}

void RestoreSession(BOOL afterCrash = FALSE)
{
	char name[256];

	name[0] = 0;
	if (afterCrash) // Loading last session after crash
		strcpy(name, kLastSessionName);
	else // Loading defined start session in pref
		kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");

	Session load;
	Session::loading = true;
	if (load.loadSession(name)) {
		load.open();
	}
	Session::loading = false;
}

int Init()
{

	// Look for a bad shutdown
	int ok = IDNO;
	int clean = true;
	kFuncs->GetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &clean, &clean);
	if (!clean) {

		ok = MessageBox(NULL, 
			gLoc->GetString(IDS_SESSION_RECOVERY_MSG),
			gLoc->GetString(IDS_SESSION_RECOVERY),
			MB_YESNO|MB_ICONQUESTION);

		if (ok == IDYES) RestoreSession(TRUE);

		// Copy last to previous
		Session prv;
		prv.loadSession(kLastSessionName);
		prv.saveSession(kPreviousSessionName);
	}

	clean = false;
	kFuncs->SetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &clean, TRUE);

	if (ok != IDYES) {
		// Ask to load the startup the session, but the session is actually
		// loaded later because we can't open a window until one is fully created
		BOOL b = FALSE;
		kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, (void*)&b);
		if (b) {
			// Get the start session name
			char *name = new char[256];
			name[0] = 0;
			kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");

			// If it's not empty
			if (strlen(name)>0) 
			{
				ok = IDYES;
				kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, (void*)&b);
				if (b)
				{	
					// Ask to load the session
					ok = MessageBox(NULL, 
						gLoc->GetStringFormat(IDS_STARTUP_SESSION_MSG, (const TCHAR*)CANSI_to_T(name)),
						gLoc->GetString(IDS_STARTUP_SESSION),
						MB_YESNO|MB_ICONQUESTION);
				}
				if (ok == IDYES) RestoreSession();//PostMessage(hWndParent, PWM_AUTOLOAD, 0, 0);
			}
			delete [] name;
		}
	}
	return 0;
}

void Create(HWND hWndParent)
{
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
	currentSession.addWindow(hWndParent);
}

void Destroy(HWND hWnd) {
	destroying = hWnd;
	currentSession.removeWindow(hWnd);
	undo.addWindow(*(currentSession.getWindow(hWnd)));

	int gMaxUndo = 5;
	kFuncs->GetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, (void*)&gMaxUndo);
	undo.limit(gMaxUndo);
}

void DoMenu(HMENU menu, char *param){
	if (*param != 0){
		char *action = param;
		char *string = strchr(param, ',');
		if (string) {
			*string = 0;
			string = SkipWhiteSpace(string+1);
		}
		else
			string = action;

		int command = 0;
		if (stricmp(action, "Save") == 0){
			command = id_save_session;
			AppendMenuA(menu, MF_STRING, command, string);
		}
		else if (stricmp(action, "Load") == 0){
			if (sessionsMenu) return;
			sessionsMenu = CreateMenu();
			BuildSessionMenu();
			AppendMenuA(menu, MF_POPUP, (UINT_PTR)sessionsMenu, string);
		}
		/*else if (stricmp(action, "Delete") == 0){
		AppendMenu(menu, MF_POPUP, (UINT)sessionsMenu, string);
		}*/
		else if (stricmp(action, "Undo") == 0){
			command = id_undo_close;
			AppendMenuA(menu, MF_STRING, command, string);
		}
	}
	else {
		//if (sessionsMenu) return;
		sessionsMenu = menu;
		BuildSessionMenu();
	}


}

int DoAccel(char *param)
{
	if (stricmp(param, "Save") == 0)
		return id_save_session;
	if (stricmp(param, "Undo") == 0)
		return id_undo_close;
	if (stricmp(param, "Config") == 0)
		return id_config;
	if (stricmp(param, "OpenPrevious") == 0)
		return id_open_previous;

	return 0;
}

void Undo(HWND hWnd = NULL)
{
	Window w = undo.lastWindow();
	if (w.hWnd) w.open();
	else  {
		Tab t = w.lastTab();
		t.setParent(hWnd);
		t.open(false);
	}
}

void Quit()
{
	currentSession.saveSession(kLastSessionName);
	currentSession.saveSession(kPreviousSessionName);

	TCHAR backup[1024];
	_tcscpy_s(backup, sessionFile);
	_tcscat_s(backup, _T(".bak"));	
	SessionStore::WriteFile(backup);

	currentSession.empty();
	undo.empty();

	bool b = true;
	kFuncs->SetPreference(PREF_BOOL, PREFERENCE_CLEANSHUTDOWN, &b, TRUE);
	delete gLoc;
}

void ConfigInitSelect(HWND hwnd)
{
	char* name = new char[256];
	kFuncs->GetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, name, "");

	for (unsigned i=0;i<SessionStore::sessions_list.size();i++)
	{
		LRESULT index, index2;

		if (strcmp(SessionStore::sessions_list[i], kPreviousSessionName) == 0)
		{
			index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)gLoc->GetString(IDS_PREVIOUS_SESSION)); 
			index2 = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)gLoc->GetString(IDS_PREVIOUS_SESSION)); 
		}
		else if (strcmp(SessionStore::sessions_list[i], kLastSessionName) == 0)
		{
			index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)gLoc->GetString(IDS_LAST_SESSION)); 
			index2 = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)gLoc->GetString(IDS_LAST_SESSION)); 
		}
		else
		{
			index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)CANSI_to_T(SessionStore::sessions_list[i])); 
			index2 = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_ADDSTRING, 
				0, (LPARAM)(LPCTSTR)CANSI_to_T(SessionStore::sessions_list[i])); 
		}

		SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETITEMDATA, 
			index, (LPARAM)i);

		if (strcmp(name, SessionStore::sessions_list[i]) == 0)
			SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_SETCURSEL, index, 0); 

		SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_SETITEMDATA, 
			index2, (LPARAM)i); 

	}

	delete [] name;
}

BOOL CALLBACK
	ConfigDlgProc( HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam )
{
	switch (Message) {
	case WM_INITDIALOG: {

		ConfigInitSelect(hwnd);

		int b=0;
		kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, (void*)&b);
		CheckDlgButton(hwnd, IDC_CHECK_AUTOLOAD, b);
		b=0;
		kFuncs->GetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, (void*)&b);
		CheckDlgButton(hwnd, IDC_CHECK_ASK, b);

		int gMaxUndo = 5;
		kFuncs->GetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, (void*)&gMaxUndo);
		SetDlgItemInt(hwnd, IDC_EDIT_MAXUNDO, gMaxUndo, FALSE);
		return TRUE;
						}

	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case IDOK: {
			LRESULT i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETITEMDATA, SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETCURSEL, 0, 0), 0); 
			if (i!=-1) 
				kFuncs->SetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, (void*)SessionStore::sessions_list[i], FALSE);
			else
				kFuncs->SetPreference(PREF_STRING, PREFERENCE_SESSION_OPENSTART, "", FALSE);
			int b = IsDlgButtonChecked(hwnd, IDC_CHECK_AUTOLOAD);
			kFuncs->SetPreference(PREF_BOOL, PREFERENCE_SESSION_AUTOLOAD, (void*)&b, FALSE);
			b = IsDlgButtonChecked(hwnd, IDC_CHECK_ASK);
			kFuncs->SetPreference(PREF_BOOL, PREFERENCE_SESSION_ASKAUTOLOAD, (void*)&b, FALSE);
			int gMaxUndo = GetDlgItemInt(hwnd, IDC_EDIT_MAXUNDO, NULL, FALSE);
			kFuncs->SetPreference(PREF_INT, PREFERENCE_SESSION_MAXUNDO, (void*)&gMaxUndo, FALSE);
			EndDialog( hwnd, IDOK );
				   }
				   break;

		case IDCANCEL:
			EndDialog( hwnd, IDCANCEL );
			break;

		case IDC_BUTTON_DELETE: {
			LRESULT index = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_GETCURSEL, 0, 0);
			LRESULT i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_GETITEMDATA, index , 0); 

			DeleteSession(SessionStore::sessions_list[i]);
			BuildSessionMenu();

			i = SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETITEMDATA, SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_GETCURSEL, 0, 0), 0); 
			SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hwnd, IDC_COMBO_SESSIONSLIST2, CB_RESETCONTENT, 0, 0);

			ConfigInitSelect(hwnd);			
								}
								break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

void Config(HWND parent){
	gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_CONFIG), parent, (DLGPROC)ConfigDlgProc);
}

// Callback for the dialog answering a session name
BOOL CALLBACK
	PromptDlgProc( HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam )
{
	static TCHAR* answer;

	switch (Message) {
	case WM_INITDIALOG: 
		answer = (TCHAR*)lParam;
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hwnd, IDC_ANSWER, answer, 256);
			if ( !_tcscmp(answer, _T("list")) || !_tcscmp(answer, CUTF8_to_T(kLastSessionName)) || _tcschr(answer, _T(',')) )
				MessageBox(hwnd, gLoc->GetString(IDS_INVALID_NAME), _T(""), MB_OK|MB_ICONERROR);
			else
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

void LoadSession(const char* name, HWND currentWnd) 
{
	Session load;
	Session::loading = true;
	if (load.loadSession(name)) {
		BOOL warn = FALSE, nowarn=FALSE;
		kPlugin.kFuncs->GetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &warn, &warn);
		kPlugin.kFuncs->SetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &nowarn, TRUE);
		currentSession.close_except(currentWnd);
		if (load.open())
			currentSession.close(currentWnd);
		kPlugin.kFuncs->SetPreference(PREF_BOOL, "browser.tabs.warnOnClose", &warn, FALSE);
	}
	Session::loading = false;
}

void CALLBACK TimerFunction(HWND hWnd, UINT, UINT_PTR id, DWORD)
{
	currentSession.saveSession(kLastSessionName);
	::KillTimer(hWnd, id);
	timerID = NULL;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_ACTIVATE:
		if (wParam >0) {
			currentSession.setActiveWindow(hWnd);
		}
		break;
	case UWM_UPDATEBUSYSTATE:

		if (wParam != 0 || gLoading) break;

		char **urls, **titles;
		int index, count;
		if (!lParam) {
			if (kFuncs->GetMozillaSessionHistory(hWnd, &titles, &urls, &count, &index))
				currentSession.updateWindow(hWnd, index, count, (const char**)urls, (const char**)titles);
		}
		else {
			if (kFuncs->GetMozillaSessionHistory((HWND)lParam, &titles, &urls, &count, &index))
				currentSession.updateWindow(hWnd, (HWND)lParam, index, count, (const char**)urls, (const char**)titles);
		}
		if (!timerID)
			timerID = ::SetTimer(NULL, NULL, 5000, TimerFunction);

		break;
	case WM_CLOSE:
		break;
	case WM_DESTROY: {
		destroying = hWnd;
		// Flush the window before tabs are destroyed
		Window* w = currentSession.getWindow(hWnd);
		if (w) w->flush();
		break;
					 }
	case WM_COMMAND:
		WORD command = LOWORD(wParam);
		if (command == id_undo_close)
			Undo(hWnd);

		else if (command == id_save_session) {

			// Ask for a session name
			TCHAR* answer = new TCHAR[256];
			answer[0]=0;
			INT_PTR ok = gLoc->DialogBoxParam(
				MAKEINTRESOURCE(IDD_PROMPT), hWnd, (DLGPROC)PromptDlgProc,(LPARAM)answer);

			// Save the session, first remove closed frame
			// Then rebuild the menu
			if (ok == IDOK && _tcslen(answer)>0) {
				currentSession.flush();
				currentSession.saveSession(CT_to_ANSI(answer));
				BuildSessionMenu();
			}
			delete [] answer;
		}

		else if (command >= id_load_session && command<id_load_session+MAX_SAVED_SESSION)
		{
			int n = command - id_load_session;
			const char* name = SessionStore::sessions_list[command - id_load_session];

			if (name) {
				LoadSession(name, hWnd);					
				return 0;
			}
		}
		else if (command == id_open_previous) {
			Session load;
			LoadSession(kPreviousSessionName, hWnd);
			return 0;
		}
		else if (command == id_config)
			Config(hWnd);

		break;
	}


	if (IsWindowUnicode(hWnd))
		return CallWindowProcW(KMeleonWndProc, hWnd, message, wParam, lParam);
	else
		return CallWindowProcA(KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {

	KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
		return &kPlugin;
	}
}

