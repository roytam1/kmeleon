/*
*  Copyright (C) 2001 Jeff Doozan
*  Copyright (C) 2006 Dorian Boissonnade
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

#include <map>
#include <stdio.h>
#include <ctime>

///////////////////////////
// Utilities 

#define WRONGARGS 0
#define WRONGTYPE 1
#define NEEDTRUST(data) if (!checkTrust(data)) return Value();

Value ExecuteFunction(const char* name);

	static void DoError(const char* msg, Statement* stat)
	{
		DoError(msg, stat?stat->getFile():"", stat?stat->getLine():-1);
	}

	static void parseError(int err, const  char *cmd, const char *args, int data1=0, int data2=0, Statement* stat = NULL)
	{
		char* msg = new char[strlen(cmd) + strlen(args) + 70];

		switch (err) {
		case WRONGARGS:
			sprintf(msg, "Wrong number of arguments - expected %d, found %d.\r\n\r\n"
				"%s(%s)",data1, data2, cmd, args);
			break;file:///H:/projects/km3/debug/browser/readme.html
		case WRONGTYPE:
			sprintf(msg, "Invalid data type in %s command.\r\n\r\n"
				"%s(%s)", cmd, cmd, args);
			break;
		}
		DoError(msg, stat);
		delete [] msg;
	}

	void checkArgs(const char* name, FunctionData* data, int min, int max) 
	{
		if (data->nparam  < min || (max != -1 && data->nparam  > max))
			parseError(WRONGARGS, name, "", min, data->nparam, data->stat);
	}

	void checkArgs(const char* name, FunctionData* data, int n) 
	{
		if (data->nparam !=n)
			parseError(WRONGARGS, name, "", n, data->nparam, data->stat);
	}

	bool checkTrust(FunctionData* data)
	{
		if (data->c.mf->denied)
			return false;

		if (data->c.mf->trusted && (!data->c.origmf || data->c.origmf->trusted))
			return true;
			
		return true; // Always trust, to do later.
	}
	
	void invalidOp(FunctionData* data) {
		MacroFile* mf = data->stat->getMFile();
		if (!mf) return;
			
		const char* msg = kPlugin.kFuncs->Translate("The macro %s has done an invalid operation and will be deactivated.");
		int len = strlen(msg) + mf->name.length();
		char* buf = new char[len];
		sprintf_s(buf, len, msg, mf->name.c_str());
		MessageBoxUTF8(data->c.hWnd, buf, "Security",  MB_OK|MB_ICONSTOP|MB_TASKMODAL);
		delete [] buf;

		DisableMacros(mf);
	}

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
			if (i != tl.npos) {
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
		return result;
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
			SetDlgItemTextUTF8(hwnd, IDCANCEL, kFuncs->Translate("Cancel"));
			SetDlgItemTextUTF8(hwnd, IDOK, kFuncs->Translate("OK"));
			return TRUE;
								  }
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
		case IDOK: {
			char tmp[4096];
			GetDlgItemTextUTF8(hwnd, IDC_ANSWER, tmp, 4096);
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

	DWORD GetPrivateProfileStringUTF8(const char* lpAppName, const char* lpKeyName, const char* lpDefault, std::string & str, const char* filename)
	{
		DWORD ret;
		if (gUnicode) {
			WCHAR tmp[4096];
			ret = GetPrivateProfileStringW(CUTF8_to_UTF16(lpAppName), CUTF8_to_UTF16(lpKeyName), CUTF8_to_UTF16(lpDefault), tmp, sizeof(tmp)/sizeof(WCHAR), CUTF8_to_UTF16(filename));
			str.assign(CUTF16_to_UTF8(tmp));
		}
		else {
			char tmp[4096];
			ret = GetPrivateProfileStringA(CUTF8_to_ANSI(lpAppName), CUTF8_to_ANSI(lpKeyName), CUTF8_to_ANSI(lpDefault), tmp, sizeof(tmp), CUTF8_to_ANSI(filename));
			str.assign(CANSI_to_UTF8(tmp));
		}	
		return ret;
	}

	BOOL WritePrivateProfileStringUTF8(const char* lpAppName, const char* lpKeyName, const char* lpString, const char* filename)
	{
		if (gUnicode)
			return WritePrivateProfileStringW(CUTF8_to_UTF16(lpAppName), CUTF8_to_UTF16(lpKeyName), CUTF8_to_UTF16(lpString), CUTF8_to_UTF16(filename));
		else
			return WritePrivateProfileStringA(CUTF8_to_ANSI(lpAppName), CUTF8_to_ANSI(lpKeyName), CUTF8_to_ANSI(lpString), CUTF8_to_ANSI(filename));
	}


	//////////////////////////
	// API

	Value _(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);		
		return  kFuncs->Translate(data->getstr(1)) ;
	}

	Value open(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		if (!data->getstr(1).length()) return Value();
		int w = OPEN_NORMAL;
		if (data->getstr(2).length()) {
			if (data->getstr(2).compare("window") == 0)
				w = OPEN_NEW;
			else if (data->getstr(2).compare("bgwindow") == 0)
				w = OPEN_BACKGROUND;
			else if (data->getstr(2).compare("new") == 0 || data->getstr(2).compare("tab") == 0)
				w = OPEN_NEWTAB;
			else if (data->getstr(2).compare("bgnew") == 0 || data->getstr(2).compare("bgtab") == 0)
				w = OPEN_BACKGROUNDTAB;
			// Compatibility
			else if (data->getstr(2).compare("ID_OPEN_LINK_IN_NEW_WINDOW") == 0)
				w = OPEN_NEW;
			else if (data->getstr(2).compare("ID_OPEN_LINK_IN_BACKGROUND") == 0)
				w = OPEN_BACKGROUND;
			else if (data->getstr(2).compare("ID_OPEN_LINK_IN_NEW_TAB") == 0)
				w = OPEN_NEWTAB;
			else if (data->getstr(2).compare("ID_OPEN_LINK_IN_BACKGROUNDTAB") == 0)
				w = OPEN_BACKGROUNDTAB;
			else if (data->getstr(2).compare("opennew") == 0)
				w = OPEN_NEW;
			else if (data->getstr(2).compare("openbg") == 0)
				w = OPEN_BACKGROUND;
			else if (data->getstr(2).compare("opentab") == 0)
				w = OPEN_NEWTAB;
			else if (data->getstr(2).compare("openbgtab") == 0)
				w = OPEN_BACKGROUNDTAB;
		}
		kFuncs->NavigateTo((char*)(data->getstr(1).c_str()), w, NULL);
		return "";
	}

	Value opennew(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->NavigateTo((char*)EscapeURL(data->getstr(1)).c_str(), OPEN_NEW, NULL);
		return "";
	}

	Value openbg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->NavigateTo((char*)EscapeURL(data->getstr(1)).c_str(), OPEN_BACKGROUND, NULL);
		return "";
	}

	Value opentab(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->NavigateTo((char*)EscapeURL(data->getstr(1)).c_str(), OPEN_NEWTAB, NULL);
		return "";
	}

	Value openbgtab(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->NavigateTo((char*)EscapeURL(data->getstr(1)).c_str(), OPEN_BACKGROUNDTAB, NULL);
		return "";
	}

	Value setpref(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		MString type = data->getstr(1);

		enum PREFTYPE preftype;
		if (!strcmpi(type, "bool")) preftype = PREF_BOOL;
		else if (!strcmpi(type, "int")) preftype = PREF_INT;
		else if (!strcmpi(type, "string")) preftype = PREF_UNISTRING;
		else {
			parseError(WRONGTYPE, "setpref", type, 0, 0, data->stat);
			return false;
		}

		MString pref = data->getstr(2);
		if (pref.find("kmeleon.plugins.macros.modules.") != MString::npos) {
			invalidOp(data);
			return Value();
		}

		if (preftype == PREF_UNISTRING) {
			MString value = data->getstr(3);
			kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(value), TRUE);
		}

		else if (preftype == PREF_INT) {
			int value = data->getint(3);
			kFuncs->SetPreference(preftype, pref, &value, TRUE);
		}

		else {   // boolean
			int value = data->getbool(3);
			kFuncs->SetPreference(preftype, pref, &value, TRUE);
		}

		return true;
	}

	Value getpref(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2);
		MString type = data->getstr(1);

		enum PREFTYPE preftype;
		if (!strcmpi(type, "bool")) preftype = PREF_BOOL;
		else if (!strcmpi(type, "int")) preftype = PREF_INT;
		else if (!strcmpi(type, "string")) preftype = PREF_UNISTRING;
		else {
			parseError(WRONGTYPE, "getpref", type, 0, 0, data->stat);
			return "";
		}

		MString pref = data->getstr(2);

		if (preftype == PREF_UNISTRING) {
			long len = kFuncs->GetPreference(PREF_STRING, pref, 0, L"");			
			if (!len) return Value("");
			std::auto_ptr<char> cRetval((char*)calloc(sizeof(char), len+1));
			kFuncs->GetPreference(PREF_STRING, pref, cRetval.get(), L"");
			if (strncmp(cRetval.get(), "chrome:",7) == 0) {
				len = kFuncs->GetPreference(PREF_LOCALIZED, pref, 0, L"");			
				if (!len) return Value("");
				cRetval.reset((char*)calloc(sizeof(char), len+1));
				kFuncs->GetPreference(PREF_LOCALIZED, pref, cRetval.get(), L"");
			}
			Value v(cRetval.get());
			return v;
		}
		else if (preftype == PREF_INT) {
			int nRetval = 0;
			kFuncs->GetPreference(preftype, pref, &nRetval, &nRetval);
			return nRetval;
		}
		else { //PREF_BOOL
			int nRetval = 0;
			kFuncs->GetPreference(preftype, pref, &nRetval, &nRetval);
			return nRetval;
		}
		return "";
	}

	Value delpref(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->DelPreference(data->getstr(1));
		return "";
	}

	Value togglepref(FunctionData* data)
	{
		enum PREFTYPE preftype;
		MString type = data->getstr(1);

		if (!strcmpi(type, "bool")) preftype = PREF_BOOL;
		else if (!strcmpi(type, "int")) preftype = PREF_INT;
		else if (!strcmpi(type, "string")) preftype = PREF_UNISTRING;
		else {
			parseError(WRONGTYPE, "getpref", type, 0, 0, data->stat);
			return "";
		}

		MString pref = data->getstr(2);
		if (pref.find("kmeleon.plugins.macros.modules.") != MString::npos) {
			invalidOp(data);
			return Value();
		}

		if (preftype == PREF_BOOL)
		{
			int iVal = 0;
			checkArgs(__FUNCTION__, data, 2);
			kFuncs->GetPreference(preftype, pref, &iVal, &iVal);
			iVal = !iVal;
			kFuncs->SetPreference(preftype, pref, &iVal, TRUE);
			return Value(iVal);
		}

		checkArgs(__FUNCTION__, data, 4, -1);
		
		if (preftype == PREF_UNISTRING)
		{
			MString val;
			for (int i = 3; i <= data->nparam; i++)
			{
				long len = kFuncs->GetPreference(preftype, pref, 0, 0);	
				wchar_t* tmp = (wchar_t*)calloc(sizeof(wchar_t), len+1);
				kFuncs->GetPreference(preftype, pref, tmp, L"");
				val = (const char*)CUTF16_to_UTF8(tmp);
				free(tmp);
				if (val == data->getstr(i))
				{
					if (i == data->nparam) i = 2;
					val = data->getstr(i+1);
					kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(val), FALSE);
					return val;
				}
			}
			kFuncs->SetPreference(preftype, pref, (void*)(const wchar_t*)CUTF8_to_UTF16(data->getstr(3)), FALSE);
		} 
		else
		{
			int iVal = 0;
			for (int i = 3; i <= data->nparam; i++)
			{
				kFuncs->GetPreference(preftype, pref, &iVal, &iVal);
				if (iVal == data->getint(i))
				{
					if (i == data->nparam) i = 2;
					iVal = data->getint(i+1);
					kFuncs->SetPreference(preftype, pref, (void*)&iVal, FALSE);
					return iVal;
				}
			}

			iVal = data->getint(3);
			kFuncs->SetPreference(preftype, pref, (void*)&iVal, FALSE);
		}

		return data->getarg(3);
	}

	Value exec(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data,  1, 2);
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi = {0};

#ifdef _UNICODE
		CUTF8_to_UTF16 strcommand(data->getstr(1));
#else
		CUTF8_to_ANSI strcommand(data->getstr(1));
#endif
		TCHAR command[4096];
		ExpandEnvironmentStrings(strcommand, command, sizeof(command));
		CreateProcess(NULL, command, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

		if (data->nparam > 1) 
			WaitForSingleObject(pi.hProcess, data->getint(2));
		
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return "";
	}

	Value id(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		HWND current = kPlugin.kFuncs->GetCurrent(data->c.hWnd);
		bool r = kPlugin.kFuncs->RunCommand(data->c.hWnd, data->getstr(1));
		if (current == data->c.hWnd) // Allow cmd like ID_TAB_NEXT to change context
			data->setWin(kPlugin.kFuncs->GetCurrent(data->c.hWnd)); 
		return r;
	}

	Value plugin(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2);
		MString plugin = data->getstr(1);
		MString param = data->getstr(2);
		plugin = plugin + "(" + param + ")";
		return kPlugin.kFuncs->RunCommand(data->c.hWnd, plugin);
	}

	Value statusbar(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kPlugin.kFuncs->SetStatusBarText(data->getstr(1));
		return "";
	}

	Value alert(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 3);
		int icon = 0;
		MString type = data->getstr(3);
		if(strcmpi(type,"EXCLAIM")==0) icon=MB_ICONEXCLAMATION;
		else if(strcmpi(type,"INFO")==0) icon=MB_ICONINFORMATION;
		else if(strcmpi(type,"STOP")==0) icon=MB_ICONSTOP;         
		else if(strcmpi(type,"QUESTION")==0) icon=MB_ICONQUESTION;         

		MessageBoxUTF8(data->c.hWnd, data->getstr(1), data->getstr(2), MB_OK|icon|MB_TASKMODAL);

		return "";
	}

	Value confirm(FunctionData* data)
	{
		// params are message,title,buttons,icon
		checkArgs(__FUNCTION__, data, 4);
		MString sButtons = data->getstr(3);
		MString sIcon = data->getstr(4);

		int buttons = MB_OKCANCEL;
		if     (strcmpi(sButtons,"RETRYCANCEL")==0) buttons=MB_RETRYCANCEL;
		else if(strcmpi(sButtons,"YESNO")==0) buttons=MB_YESNO;
		else if(strcmpi(sButtons,"YESNOCANCEL")==0) buttons=MB_YESNOCANCEL;         
		else if(strcmpi(sButtons,"ABORTRETRYIGNORE")==0) buttons=MB_ABORTRETRYIGNORE; 

		int icon = 0;
		if     (strcmpi(sIcon,"EXCLAIM")==0) icon=MB_ICONEXCLAMATION;
		else if(strcmpi(sIcon,"INFO")==0) icon=MB_ICONINFORMATION;
		else if(strcmpi(sIcon,"STOP")==0) icon=MB_ICONSTOP;         
		else if(strcmpi(sIcon,"QUESTION")==0) icon=MB_ICONQUESTION;         

		int result = MessageBoxUTF8(data->c.hWnd, data->getstr(1), data->getstr(2), buttons|icon|MB_TASKMODAL);
		if(result == IDOK) return "OK";
		if(result == IDYES) return "YES";
		if(result == IDNO) return "NO";
		if(result == IDABORT) return "ABORT";
		if(result == IDRETRY) return "RETRY";
		if(result == IDIGNORE) return "IGNORE";
		if(result == IDCANCEL) return "0";         

		return "";      
	}

	Value prompt(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2, 3);
		question = data->getstr(1);
		title = data->getstr(2);
		instring = data->getstr(3);
		int ok;

		if (gUnicode)
			ok = DialogBoxW(kPlugin.hDllInstance,
			MAKEINTRESOURCEW(IDD_PROMPT), data->c.hWnd, (DLGPROC)PromptDlgProc);
		else
			ok = DialogBoxA(kPlugin.hDllInstance,
			MAKEINTRESOURCEA(IDD_PROMPT), data->c.hWnd, (DLGPROC)PromptDlgProc);

		PostMessage(data->c.hWnd, WM_NULL, 0, 0);
		if (ok == IDOK)
			return answer;
		else 
			return "";
	}

	Value getclipboard(FunctionData* data)
	{         // get and return data from the clipboard
		checkArgs(__FUNCTION__, data, 0);
		if(  !IsClipboardFormatAvailable(CF_UNICODETEXT)
			&& !IsClipboardFormatAvailable(CF_TEXT) )
			return "";

		if(!OpenClipboard(NULL)) {
			DoError("Error opening the clipboard.", data->stat);
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
		Value retval;
		char* tmp = 0;
		if (gUnicode)
			tmp = utf8_from_utf16((const WCHAR*)pszData);
		else
			tmp = utf8_from_ansi((const char*)pszData);
		
		std::string ret;
		if (tmp) {
			ret = tmp;
			free(tmp);
		}

		free(pszData);
		return ret;
	}

	Value setclipboard(FunctionData* data)
	{       
		// set data to the clipboard
		checkArgs(__FUNCTION__, data, 1);

		if(!OpenClipboard(NULL)) {
			DoError("Error opening the clipboard.", data->stat);
			return "";
		}

		int len, datasize, clipformat;
		void* pszData;
		if (gUnicode) {
			pszData = (void*)utf16_from_utf8(data->getstr(1));
			datasize = sizeof(wchar_t);
			len = wcslen((wchar_t*)pszData);
			clipformat = CF_UNICODETEXT;
		}
		else {
			pszData = (void*)ansi_from_utf8(data->getstr(1));
			datasize = sizeof(char);
			len = strlen((char*)pszData); 
			clipformat = CF_TEXT;
		}

		HGLOBAL hData;
		LPVOID pData;

		EmptyClipboard();
		MString ret = "0";

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

	Value macros(FunctionData* data)
	{
		// execute other macros
		checkArgs(__FUNCTION__, data, 1);
		std::string ret;
		std::string macrolist = data->getstr(1);

		int pos = macrolist.find(';');
		std::string macro = macrolist.substr(0,pos);

		while (macro.length()) {
			ret = ExecuteMacro(data->c.hWnd, macro.c_str(), &data->c);
			
			pos != macrolist.npos ?
				macrolist.erase(0,pos+1) :
			   macrolist.erase();

			pos = macrolist.find(';');
			macro = macrolist.substr(0,pos);
		}
		return ret;
	}

	/*
	PluginMsg ( "DestPlugin", "Command", "Param1", "Param2");

	Send a command to a plugin that does not need a return value.
	If the plugin needs more than two parameters, it will have to
	parse them out of a string passed as either Param1 or Param2
	*/
	Value pluginmsg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3, 4);

		// Layer compatibility
		if (data->getstr(1) == "layers")
		{
			if (data->getstr(2) == "OpenURL") {
				kFuncs->NavigateTo((char*)EscapeURL(data->getstr(3)).c_str(), OPEN_NEWTAB, NULL);
				return true;
			}

			if (data->getstr(2) == "OpenURLBg") {
				kFuncs->NavigateTo((char*)EscapeURL(data->getstr(3)).c_str(), OPEN_BACKGROUNDTAB, NULL);
				return true;
			}

		}
		
		kFuncs->SendMessage(CUTF8_to_ANSI(data->getstr(1)),
			PLUGIN_NAME, 
			CUTF8_to_ANSI(data->getstr(2)), 
			(long) (const char*)CUTF8_to_ANSI(data->getstr(3)), 
			(long) (const char*)CUTF8_to_ANSI(data->getstr(4)));
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

	Value pluginmsgex(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 4);
		enum PREFTYPE preftype;

		if (!strcmpi(data->getstr(4), "int")) preftype = PREF_INT;
		else if (!strcmpi(data->getstr(4), "string")) preftype = PREF_STRING;
		else {
			parseError(WRONGTYPE, "PluginMsgEx", data->getstr(4), 0, 0, data->stat);
			return "";
		}
		std::string strRet;
		if (preftype == PREF_STRING) {
			char *cRetval = NULL;
			strRet = "";

			kFuncs->SendMessage(
				CUTF8_to_ANSI(data->getstr(1)), 
				PLUGIN_NAME, 
				CUTF8_to_ANSI(data->getstr(2)), 
				(long) (const char*)CUTF8_to_ANSI(data->getstr(3)), 
				(long) &cRetval);

			if (cRetval) {
				strRet = CANSI_to_UTF8(cRetval);
				free(cRetval);
				return strRet;
			}
		}
		else if (preftype == PREF_INT) {
			int nRetval = 0;

			kFuncs->SendMessage(
				CUTF8_to_ANSI(data->getstr(1)), 
				PLUGIN_NAME, 
				CUTF8_to_ANSI(data->getstr(2)), 
				(long) (const char*)CUTF8_to_ANSI(data->getstr(3)), 
				(long) &nRetval);

			return nRetval;
		}
		return 0;
	}

	/*
	gensub( r, s, h, t );

	search the target string t for matches of the string r.  If h
	is a string beginning with g or G, then replace all matches
	of r with s.  Otherwise, h is a number indicating which match
	of r to replace.  The modified string is returned as the
	result of the function.
	*/
	Value gensub(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 4);
		return GenSub(data->getstr(1), data->getstr(2), data->getstr(3), data->getstr(4));
	}

	/*
	gsub( r, s, t );

	for each substring matching the string r in the string t,
	substitute the string s, and return the number of
	substitutions.
	*/
	Value gsub(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		return GenSub(data->getstr(1), data->getstr(2), "G", data->getstr(3));
	}

	/*
	index( s, t );

	returns the index of the string t in the string s, or -1 if t
	is not present.
	*/
	Value index(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2);
		std::string str = data->getstr(1);
		int i = str.find(data->getstr(2));
		if (i != str.npos)
			return i;
		else
			return -1;
	}
	/*
	length( s );

	returns the length of the string s.
	*/

	Value length(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		return (int)strlen(data->getstr(1));
	}


	/*
	sub( t, s, t );

	just like gsub(), but only the first matching substring is
	replaced.
	*/
	Value sub(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		return GenSub(data->getstr(1), data->getstr(2), "1", data->getstr(3));
	}

	/*
	substr( s, i [, n] );

	returns the at most n-character substring of s starting at i.
	If n is omitted, the rest of s is used.
	*/
	Value substr(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2, 3);
		
		int i = data->getint(2);
		int n = data->nparam > 2 ? data->getint(3) : 0;
		int len =strlen(data->getstr(1));
		if (i<0)     i = 0;
		if (i>len)   i = len;
		if (n<0)     n = 0;
		if (n>len-i) n = len-i;

		std::string retval = data->getstr(1);
		if (data->nparam > 2)
			retval = retval.substr(i, n);
		else
			retval = retval.substr(i);

		return retval;
	}

	Value urlencode(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		return  Escape(data->getstr(1)).c_str() ;
	}

	Value urldecode(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		char* str = strdup(data->getstr(1));
		nsUnescape(str);
		Value ret(str);
		free(str);
		return ret;
	}

	/*
	basename( NAME [, SUFFIX] );

	Returns NAME with any leading directory components removed.
	If specified, also remove a trailing SUFFIX.
	*/
	Value basename(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		int i, j;
		std::string name = data->getstr(1);

		if (data->nparam == 2) {
			i = name.rfind(data->getstr(2));
			if (i != name.npos )
				name = name.substr(0,i);
		}

		i = name.rfind( "/" );
		j = name.rfind( "\\" );

		if (i == name.npos)
			i = j;
		if (i != name.npos && j != name.npos && i<j)
			i = j;

		if (i != name.npos)
			name = name.substr(i+1);

		return name;
	}


	/*
	dirname( NAME );

	Returns NAME with its trailing /component removed; if NAME
	contains no /'s, output `.' (meaning the current directory).
	*/
	Value dirname(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		std::string name = data->getstr(1);
		
		int i, j;

		int len = name.length();
		if (len < 1) return "";

		if (name.at(len-1) == '/' ||
			name.at(len-1) == '\\')
			name = name.substr(0, len-1);

		i = name.rfind( "/" );
		j = name.rfind( "\\" );

		if (i == name.npos)
			i = j;
		if (i != name.npos && j != name.npos && i<j)
			i = j;

		if (i != name.npos)
			name = name.substr(0, i>0 ? i : 1);
		else
			name = ".";

		return name;
	}

	/*
	hostname( URL );

	Returns hostname of given URL.
	*/
	Value hostname(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		int i;
		std::string name = data->getstr(1);

		i = name.find( "://" );
		if (i != name.npos)
			name = name.substr(i+3);

		i = name.find( "/" );
		if (i != name.npos)
			name = name.substr(0,i);

		return name;
	}

	Value injectJS(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		
		int bTopWindow;

		if (strcmp(data->getstr(2), "frame")==0)
			bTopWindow = 0;
		else if (strcmp(data->getstr(2), "alltabs")==0)
			bTopWindow = 2;
		else if (strcmp(data->getstr(2), "hidden")==0)
			bTopWindow = -1;
		else
			bTopWindow = 1;
		
		char result[4096];
		kPlugin.kFuncs->InjectJS2(data->getstr(1), bTopWindow, result, 4096, data->c.hWnd);
		return result;
	}

	Value injectCSS(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kPlugin.kFuncs->InjectCSS(data->getstr(1), true, data->c.hWnd);
		return "";
	}

	Value readfile(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		MString file = data->getstr(1);
		if (file.empty())
			return "";
		FILE* f = _wfopen(data->getstr(1).utf16(), L"r");
		if (f) {
			char* buffer = new char[65536];
			int size = fread(buffer, sizeof(char), 65536-1, f);
			buffer[size] = 0;
			Value ret = buffer;
			fclose(f);
			delete  [] buffer;
			return ret;
		}
		return "";
	}

	Value writefile(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 2);
		
		FILE* f = _wfopen(data->getstr(1).utf16(), L"w");
		if (!f) return false;

		MString buf = data->getstr(2);
		size_t result = fwrite(buf.c_str(), sizeof(char), buf.length(), f);
		fclose(f);
		return result == buf.length();
	}

	Value appendfile(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 2);
		
		FILE* f = _wfopen(data->getstr(1).utf16(), L"a");
		if (!f) return false;

		MString buf = data->getstr(2);
		size_t result = fwrite(buf.c_str(), sizeof(char), buf.length(), f);
		fclose(f);
		return result == buf.length();
	}

	Value renamefile(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 2);		
		int res = _wrename(data->getstr(1).utf16(), data->getstr(2).utf16());
		return res == 0;		
	}

	Value copyfile(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 2);		
		return CopyFile(data->getstr(1).utf16(), data->getstr(2).utf16(), TRUE) == TRUE;
	}

	Value mkdir(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 1);		
		return CreateDirectory(data->getstr(1).utf16(), NULL) == 0;
	}

	Value deletefile(FunctionData* data)
	{
		//NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 1);		
		int res = _wunlink(data->getstr(1).utf16());
		return res == 0;	
	}

	Value readreg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2);
		HKEY root;
		if (data->getstr(1) == "HKLM") root = HKEY_LOCAL_MACHINE;
		else if (data->getstr(1) == "HKCU") root = HKEY_CURRENT_USER;
		else if (data->getstr(1) == "HKCR") root = HKEY_CLASSES_ROOT;
		else if (data->getstr(1) == "HKU") root = HKEY_USERS;
		else if (data->getstr(1) == "HKCC") root = HKEY_CURRENT_CONFIG;

		TCHAR* keyPath = t_from_utf8(data->getstr(2));
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
					 ret = dword;
				 }
				 else
					 ret = CT_to_UTF8((LPTSTR)lpData);
			 }
			 free(lpData);
			}
		}
		free(keyPath);
		RegCloseKey(key);
		return ret;
	}

	Value promptforfolder(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		CUTF8_to_T caption(data->getstr(1));
		if (!(LPCTSTR)caption) return "";

		CUTF8_to_T initFolder(data->getstr(2));

		LPITEMIDLIST pidlRetBrowse = NULL, pidlRoot = NULL;

		CoInitialize(NULL);
		std::string ret;
		LPITEMIDLIST idl; 
		TCHAR pszDisplayName[MAX_PATH];
		BROWSEINFO bi = {data->c.hWnd, NULL, pszDisplayName, caption, BIF_EDITBOX | BIF_NEWDIALOGSTYLE,BrowseProc,(LPARAM)(LPCTSTR)initFolder,NULL};
		idl = SHBrowseForFolder(&bi);
		if (idl != NULL)
		{
			if (SHGetPathFromIDList(idl, pszDisplayName))
				ret = CT_to_UTF8(pszDisplayName);
			CoTaskMemFree(idl);
		}
		CoUninitialize();
		return ret;
	}

	Value promptforfile(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		std::string filter = data->getstr(2) + std::string("|") + data->getstr(3) + std::string("||");

		TCHAR* pszFilter = t_from_utf8(filter.c_str());
		if (!pszFilter) return "";

		for (TCHAR* p = pszFilter; *p; p++)
			if ((*p) == '|') (*p) = 0;

		MString initDir = data->getstr(1);
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = data->c.hWnd;
		ofn.lpstrFilter =pszFilter;
		ofn.lpstrFile = new TCHAR[MAX_PATH]; 
		ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = MAX_PATH;
		if (!initDir.empty())
			ofn.lpstrInitialDir = t_from_utf8(data->getstr(1));

		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		std::string ret;
		if( ::GetOpenFileName(&ofn) ) 
			ret = CT_to_UTF8(ofn.lpstrFile);
		else
			ret = "";
		delete[] ofn.lpstrFile;
		free(pszFilter);
		if (ofn.lpstrInitialDir)
			free((void*)ofn.lpstrInitialDir);

		return ret;
	}

	Value forcecharset(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 0, 1);
		kFuncs->SetForceCharset(data->nparam < 1 ? "" : data->getstr(1));
		return "";
	}

	Value setcheck(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		int cmd;
		cmd = kPlugin.kFuncs->GetID(data->getstr(1));
		if (!cmd)
			cmd = data->getint(1);

		BOOL mark = TRUE;
		if (data->nparam > 1)
			mark = data->getbool(2);

		kFuncs->SetCheck(cmd, mark);

		return "";
	}
	
	Value getfolder(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		char path[MAX_PATH] = {0};
		MString dirname = data->getstr(1);
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
			return path;
		}
		else if (stricmp(dirname, "UserMacroFolder") == 0) {
			kFuncs->GetFolder(UserSettingsFolder, path, MAX_PATH);
			strcat(path, "\\macros");
			return path;
		}
		else return "";

		kFuncs->GetFolder(foldertype, path, MAX_PATH);
		return path;
	}

	Value setmenu(FunctionData* data) 
	{
		checkArgs(__FUNCTION__, data, 2, 5);

		//setmenu( "name", "command", "label", command, where)
		//setmenu( "name", "macro", "label", macroname, where)
		//setmenu( "name", "popup", "label", where)
		//setmenu( "name", "inline", "label", where)
		//setmenu( "name", "separator", where)
		//setmenu( "name", "plugin", plugin, where)
		
		MString label = data->getstr(3);
		MString menutype = data->getstr(2);

		kmeleonMenuItem item;
		int whereparam = 0;
		item.command = 1;
		item.label = ansi_from_utf8(label.c_str());
				
		if (strcmp(menutype, "command") == 0) {
			whereparam = 5;
			item.type = MENU_COMMAND;
			item.command = kFuncs->GetID(data->getstr(4));
			if (!item.command)
				item.command = data->params[3].intval();
		}
		else if (strcmp(menutype, "macro") == 0) {
			whereparam = 5;
			item.type = MENU_COMMAND;
			item.command = DoAccel(data->getstr(4));
			if (!item.command)
				item.command = data->params[3].intval();
		}
		else if (strcmp(menutype, "popup") == 0) {
			whereparam = 4;
			item.type = MENU_POPUP;
		}
		else if (strcmp(menutype, "inline") == 0) {
			whereparam = 4;
			item.type = MENU_INLINE;
		}
		else if (strcmp(menutype, "separator") == 0) {
			whereparam = 3;
			item.type = MENU_SEPARATOR;
		}
		else if (strcmp(menutype, "plugin") == 0) {
			whereparam = 3;
			item.type = MENU_PLUGIN;
		}
		else {
			parseError(WRONGARGS, "setmenu", menutype, 2, data->nparam, data->stat);
			return "0";
		}


		MString where = data->getstr(whereparam);
		if ( whereparam > data->nparam || where.empty())
			item.before = -1;
		else {
			item.before = data->getint(whereparam);
			if (!item.before && where != "0") {
				item.before = kFuncs->GetID((char*)where.c_str()); 
				if (!item.before)
					item.before = (long)(char*)where.c_str();
			}
/*
         item.before = DoAccel((char*)where);
			if (!item.before)
				item.before = data->getint(whereparam);
			if (!item.before)
				item.before = kFuncs->GetID((char*)where); 
			if (!item.before)
				item.before = (long)(const char*)where;*/
		}

		kFuncs->SetMenu(CUTF8_to_ANSI(data->getstr(1)), &item);
		if (item.label) free((void*)item.label);
		return "1";
	}

	Value setaccel(FunctionData* data) 
	{
		checkArgs(__FUNCTION__, data, 2);
		kFuncs->SetAccel(data->getstr(1), data->getstr(2));
		return "1";
	}

	Value rebuildmenu(FunctionData* data) 
	{
		checkArgs(__FUNCTION__, data, 1);
		kFuncs->RebuildMenu(data->getstr(1));
		return "1";
	}

	Value getwinvar(FunctionData* data) 
	{
		checkArgs(__FUNCTION__, data, 1);

		MString name = data->getstr(1);
		WindowVarType type;
		/*if (name == "SelectedText") {
			int l = kPlugin.kFuncs->GetWindowVar(data->c.hWnd, Window_SelectedText, NULL);
			if (l<1) return "";

			wchar_t* buf = new wchar_t[l];
			kPlugin.kFuncs->GetWindowVar(data->c.hWnd, Window_SelectedText, buf);
			std::string ret = CUTF16_to_UTF8(buf);
			delete [] buf;
			return ret;
		}*/

		if (name == "VERSION") {
			return kPlugin.kFuncs->GetKmeleonVersion();
		}

		if (name == "TextZoom") {
			int zoom;
			kPlugin.kFuncs->GetWindowVar(data->c.hWnd, Window_TextZoom, &zoom);
			char buf[34];
			_itoa(zoom, buf, 10);
			return buf;
		}

		if (name == "WindowNumber")
		{
			int nb;
			kPlugin.kFuncs->GetWindowVar(data->c.hWnd, Window_Number, &nb);
			return nb;
		}

		if (name == "TabNumber")
		{
			int nb;
			kPlugin.kFuncs->GetWindowVar(data->c.hWnd, Window_Tab_Number, &nb);
			return nb;
		}
			
		if (name == "SelectedText")
			type = Window_SelectedText;
		else if (name == "URLBAR")
			type = Window_UrlBar;
		else if (name == "URL")
			type = Window_URL;
		else if (name == "TITLE")
			type = Window_Title;
		else if (name == "LinkURL")
			type = Window_LinkURL;
		else if (name == "ImageURL")
			type = Window_ImageURL;
		else if (name == "FrameURL")
			type = Window_FrameURL;
		else if (name == "CHARSET")
			type = Window_Charset;
		else if (name == "SEARCHURL")
			type = Search_URL;
		else if (name == "CommandLine")
			return CT_to_UTF8(::GetCommandLine());
		else if (name == "LANG")
			type = Window_Lang;
		else return "";

		int l = kPlugin.kFuncs->GetWindowVar(data->c.hWnd, type, NULL);
		if (l<1) return "";

		char* buf = new char[l];
		kPlugin.kFuncs->GetWindowVar(data->c.hWnd, type, buf);
		MString ret = buf;
		delete [] buf;
		return ret;
	}

	Value w(FunctionData* data)
	{
		return  getwinvar(data);
	}


	Value setwinvar(FunctionData* data) 
	{
		checkArgs(__FUNCTION__, data, 2);
		WindowVarType type;
		MString name = data->getstr(1);
		if (name == "TextZoom") {
			int i = data->getint(2);
			kPlugin.kFuncs->SetWindowVar(data->c.hWnd, Window_TextZoom, &i);
			return "";
		}
		else if (name == "URLBAR")
			type = Window_UrlBar;
		else if (name == "URL")
			type = Window_URL;
		else if (name == "TITLE")
			type = Window_Title;
		else
			return "";

		kPlugin.kFuncs->SetWindowVar(data->c.hWnd, type, (void*)data->getstr(2).c_str());
		return Value();
	}

	Value iniread(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 4);
		std::string buffer;
		::GetPrivateProfileStringUTF8(data->getstr(1), data->getstr(2), data->getstr(3), buffer, data->getstr(4));
		return buffer;
	}

	Value iniwrite(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 4);
		return ::WritePrivateProfileStringUTF8(data->getstr(1), data->getstr(2), data->getstr(3), data->getstr(4));
	}
	
	Value pluginexist(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1);
		kmeleonPlugin* kp = kPlugin.kFuncs->Load(data->getstr(1));
		return kp && kp->loaded;
	}

	Value addtoolbar(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1,3);
		return kPlugin.kFuncs->AddToolbar(data->getstr(1), data->getint(2), data->getint(3));
	}

	Value addbutton(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2,4);
		return kPlugin.kFuncs->AddButton(data->getstr(1), data->getstr(2), data->getstr(3), data->getstr(4));		
	}

	Value removebutton(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2,3);
		return kPlugin.kFuncs->RemoveButton(data->getstr(1), data->getstr(2));
	}

	Value setcmdicon(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2,6);
		MString name = data->getstr(1);
		MString icon = data->getstr(2);
		MString hot = data->getstr(3);
		MString dead = data->getstr(4);
		
		//RECT r = {0,0,data->getint(3),data->getint(4)};
		LPRECT pr = nullptr;
		return kPlugin.kFuncs->SetCmdIcon(
			name, icon, pr, hot, pr, dead, pr
		);
		return Value();
	}

	Value setbuttonimg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2,6);
		MString name = data->getstr(1);
		int id = kPlugin.kFuncs->GetID(data->getstr(2));
		MString icon = data->getstr(3);
		MString hot = data->getstr(4);
		MString dead = data->getstr(5);
		
		//RECT r = {0,0,data->getint(3),data->getint(4)};
		LPRECT pr = nullptr;
		return kPlugin.kFuncs->SetButtonIcon(
			name, id, icon, pr, hot, pr, dead, pr
		);
		return Value();
	}

	// toolbar, name, command, menu, label, tooltip, cold, hot, dead, w, h
	Value addbuttonex(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3,11);

		MString name = data->getstr(2);
		MString label = data->getstr(5);
		MString tooltip = data->getstr(6);
		MString command = data->getstr(3);
		MString menu = data->getstr(4);
		MString hot = data->getstr(8);
		MString cold = data->getstr(7);
		MString dead = data->getstr(9);


		kmeleonButton kbutton = {
			name,
			label,
			tooltip,
			command,
			menu,
			hot,
			cold,
			dead,
			true,
			false,
			0,
			0,
			data->getint(10),
			data->getint(11)
	   };

	   return kPlugin.kFuncs->AddButtonEx(data->getstr(1), &kbutton);
	}

	Value enablebutton(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		kmeleonButton b = {0};
		b.enabled = data->getbool(3);
		b.checked = -1;
		return kPlugin.kFuncs->SetButton(data->getstr(1), kPlugin.kFuncs->GetID(data->getstr(2)), &b);
	}

	Value checkbutton(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3);
		kmeleonButton b = {0};
		b.enabled = -1;
		b.checked = data->getbool(3);
		return kPlugin.kFuncs->SetButton(data->getstr(1), kPlugin.kFuncs->GetID(data->getstr(2)), &b);
	}
	/*
	Value setbuttonimg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2,7);

		MString hot = data->getstr(4);
		MString cold = data->getstr(3);
		MString dead = data->getstr(5);

		kmeleonButton b = {0};
		b.checked = -1;
		b.enabled = -1;
		b.hotimage = hot;
		b.deadimage = dead;
		b.coldimage = cold;
		b.iconWidth = data->getint(6);
		b.iconHeight = data->getint(7);

		return kPlugin.kFuncs->SetButton(data->getstr(1), kPlugin.kFuncs->GetID(data->getstr(2)), &b);
	}*/

#define MAX_TIMERS 25
#define OFFSET_TIMERS 1000

	typedef struct TimerStruct {
		std::string macro;
		UINT_PTR idEvent;
		HWND hWnd;
		bool onlyonce;
		TimerStruct() : idEvent(0), hWnd(NULL), onlyonce(true) {}
	} TimerStruct;
	
	TimerStruct timers[MAX_TIMERS];

	VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) 
	{
		int i;
		for (i=0;i<MAX_TIMERS;i++)
			if (timers[i].idEvent == idEvent)
				break;

		if (i >= MAX_TIMERS) return;	

		if (timers[i].hWnd && !IsWindow(timers[i].hWnd)) {
			// Tab was closed, just kill the timer
			KillTimer(NULL, idEvent);		
			timers[i].idEvent = 0;
			return;
		}

		if (timers[i].onlyonce) {
			KillTimer(NULL, idEvent);		
			timers[i].idEvent = 0;
		}

		ExecuteMacro(timers[i].hWnd, timers[i].macro.c_str());		
	}
	
	Value settimer(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 2, 3);

		int i = 0;
		for (i=0;i<MAX_TIMERS;i++)
			if (!timers[i].idEvent)
				break;

		if (i >= MAX_TIMERS) {
			DoError("No timer available.", data->stat);
			return 0;
		}

		if (data->c.hWnd && !IsWindow(data->c.hWnd))
			return 0;

		if (data->getint(2) < 1)
			return 0;
		
		timers[i].hWnd = data->c.hWnd;
		timers[i].onlyonce = true;
		if (data->getstr(3).compare("free") == 0)
			timers[i].hWnd = NULL;
		else if (data->getstr(3).compare("continuous") == 0)
			timers[i].onlyonce = false;
		
		timers[i].macro = data->getstr(1);
		timers[i].idEvent = SetTimer(NULL, (UINT_PTR)&timers[i], data->getint(2)*1000, TimerProc);
		return timers[i].idEvent;
	}

	Value killtimer(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 0, 1);
		int i, idEvent  = data->getint(1);
		
		if (idEvent == 0) {
			for (i=0;i<MAX_TIMERS;i++)
				if (timers[i].idEvent && timers[i].hWnd == data->c.hWnd) {
					KillTimer(NULL, timers[i].idEvent);
					timers[i].idEvent = 0;
				}
			return true;
		}

		for (i=0;i<MAX_TIMERS;i++)
			if (timers[i].idEvent == idEvent)
				break;
		if (i >= MAX_TIMERS) return false;
		//if (i<0 || i>=MAX_TIMERS) return false;
		KillTimer(NULL, idEvent);
		timers[i].idEvent = 0;
		return true;
	}

	/* url, type, perm, sessionOnly */
	Value addperm(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 3, 4);
		return kPlugin.kFuncs->AddPermission(data->getstr(1), data->getstr(2), data->getstr(3), data->getbool(4));
	}

	Value fileexists(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 1);
		DWORD dwAttrib = GetFileAttributes(data->getstr(1).utf16());

		return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
				!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	Value logmsg(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		UINT flags = 5;
		MString type = data->getstr(2);
		if (type == "error") flags = 0;
		else if (type == "warning") flags = 1;
		return kPlugin.kFuncs->LogMessage("macro", data->getstr(1), data->stat->getFile(), data->stat->getLine(), flags);
	}

	Value popupmenu(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 1, 2);
		return kPlugin.kFuncs->ShowMenu(data->c.hWnd, data->getstr(1).c_str(), data->getint(2,1));
	}

	Value time(FunctionData* data)
	{
		checkArgs(__FUNCTION__, data, 0);
		return std::time(nullptr);
	}

	Value date(FunctionData* data)
	{
		std::time_t time;
		int t = data->getint(2,-1);
		time = t>=0 ? t : std::time(nullptr);
		char buf[100];
		std::strftime(buf, sizeof(buf),  data->getstr(1), std::localtime(&time));
		return buf;
	}

	void OnDownload(const char* url, const char* path, int result, void* data) {
		ExecuteFunction((char*)data);
		delete data;
	}

	Value download(FunctionData* data)
	{
		NEEDTRUST(data);
		checkArgs(__FUNCTION__, data, 2, 3);
		void* cdata = (void*)strdup(data->getstr(3));
		if (!kPlugin.kFuncs->Download(data->getstr(1), data->getstr(2), &OnDownload, cdata)) {
			delete cdata;
			return false;
		}
		return true;
	}



#define FUNCSYMBOL(X) "!"#X

#ifndef MACROSFUNC_ADD
#define MACROSFUNC_ADD(entry) m->AddSymbol(FUNCSYMBOL(entry), ValueFunc((MacroFunction)&entry));
#endif

void InitFunctions(Mac* m)
{
	MACROSFUNC_ADD(_);
	MACROSFUNC_ADD(setcheck);
	MACROSFUNC_ADD(open);
	MACROSFUNC_ADD(opennew);
	MACROSFUNC_ADD(openbg);
	MACROSFUNC_ADD(opentab);
	MACROSFUNC_ADD(openbgtab);
	MACROSFUNC_ADD(setpref);
	MACROSFUNC_ADD(getpref);
	MACROSFUNC_ADD(delpref);
	MACROSFUNC_ADD(togglepref);
	MACROSFUNC_ADD(exec);
	MACROSFUNC_ADD(id);
	MACROSFUNC_ADD(plugin);
	MACROSFUNC_ADD(statusbar);
	MACROSFUNC_ADD(alert);
	MACROSFUNC_ADD(confirm);
	MACROSFUNC_ADD(prompt);
	MACROSFUNC_ADD(getclipboard);
	MACROSFUNC_ADD(setclipboard);
	MACROSFUNC_ADD(macros);
	MACROSFUNC_ADD(pluginmsg);
	MACROSFUNC_ADD(pluginmsgex);
	MACROSFUNC_ADD(gensub);
	MACROSFUNC_ADD(gsub);
	MACROSFUNC_ADD(index);
	MACROSFUNC_ADD(length);
	MACROSFUNC_ADD(sub);
	MACROSFUNC_ADD(substr);
	MACROSFUNC_ADD(urlencode);
	MACROSFUNC_ADD(urldecode);
	MACROSFUNC_ADD(getfolder);
	MACROSFUNC_ADD(basename);
	MACROSFUNC_ADD(dirname);
	MACROSFUNC_ADD(hostname);
	MACROSFUNC_ADD(injectJS);
	MACROSFUNC_ADD(injectCSS);
	MACROSFUNC_ADD(readfile);
	MACROSFUNC_ADD(readreg);
	MACROSFUNC_ADD(promptforfile);
	MACROSFUNC_ADD(promptforfolder);
	MACROSFUNC_ADD(forcecharset);
	MACROSFUNC_ADD(setmenu);
	MACROSFUNC_ADD(setaccel);
	MACROSFUNC_ADD(rebuildmenu);
	MACROSFUNC_ADD(getwinvar);
	MACROSFUNC_ADD(setwinvar);
	MACROSFUNC_ADD(iniread);
	MACROSFUNC_ADD(iniwrite);
	MACROSFUNC_ADD(pluginexist);
	MACROSFUNC_ADD(addtoolbar);
	MACROSFUNC_ADD(addbutton);
	MACROSFUNC_ADD(removebutton);
	MACROSFUNC_ADD(addbuttonex);
	MACROSFUNC_ADD(enablebutton);
	MACROSFUNC_ADD(checkbutton);
	MACROSFUNC_ADD(setbuttonimg);
	MACROSFUNC_ADD(setcmdicon);	
	MACROSFUNC_ADD(settimer);	
	MACROSFUNC_ADD(killtimer);	
	MACROSFUNC_ADD(addperm);	
	MACROSFUNC_ADD(fileexists);
	MACROSFUNC_ADD(writefile);
	MACROSFUNC_ADD(renamefile);
	MACROSFUNC_ADD(deletefile);
	MACROSFUNC_ADD(appendfile);
	MACROSFUNC_ADD(copyfile);	
	MACROSFUNC_ADD(mkdir);	
	MACROSFUNC_ADD(logmsg);
	MACROSFUNC_ADD(popupmenu);
	MACROSFUNC_ADD(time);
	MACROSFUNC_ADD(date);
	MACROSFUNC_ADD(download);
}