#pragma once
#include "stdafx.h"
#include "../lib/JSON.h"
#include "../lib/rapidjson/document.h"
#include "../lib/rapidjson/prettywriter.h" 
#include "../lib/rapidjson/filestream.h" 
#include "../lib/rapidjson/writer.h" 
#include <fstream>

/*
{
  version:1,
	sessions:[
		{
			name:"",
			windows:[
				{
					tabs:[
						{
							entries:[
								{url: , title:},
								{url: , title:},
								{url: , title:}
							],
							index:2,
							scroll:"0,10"
						}
					],
					selected:1,
					"width":"1116",
					"height":"716",
					"screenX":"-1218",
					"screenY":"31",
					"sizemode":"maximized"
				}
			],
			selectedWindow:1
		}				
	]
}
*/


using namespace rapidjson;

class Session;
extern TCHAR sessionFile[1024];
extern kmeleonFunctions *kFuncs;
extern char* kLastSessionName; 

#include "sessions.h"

bool  SessionStore::lock = false;
std::vector<const char*> SessionStore::sessions_list;

void replace(std::string& str, const std::string& from, const std::string& to) {
	if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

void SessionStore::UpdateSessionList()
{
	assert(data.HasMember("sessions"));
	sessions_list.erase(sessions_list.begin(), sessions_list.end());
	Value& sessions = data["sessions"];
	for (SizeType i =0; i<sessions.Size(); i++) {		
		if (sessions[i].IsObject() && sessions[i]["name"].IsString()) {
			sessions_list.push_back(sessions[i]["name"].GetString());
		}
	}
}

bool SessionStore::Import() 
{
	char* sessions_list[MAX_SAVED_SESSION] = {0};
	int len = kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.sessions2.list", NULL, NULL);
	char *sessionsList = new char[len+1];
	kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.sessions2.list", sessionsList, _T(""));
		
	int i = 0;
	char* token = strtok(sessionsList, ",");
	while (token) {
		if (i>=MAX_SAVED_SESSION) break;
		sessions_list[i++] = strdup(token);
		token = strtok(NULL, ",");
	}
	sessions_list[i] = kLastSessionName;
	delete [] sessionsList;
	std::string json = "{\"version \":1, \"sessions\":[";

	for (int k = 0; sessions_list[k]; k++) {
		const char* name = sessions_list[k];
		std::string prefname = std::string(pref_prefix) + name;
		unsigned count = getIntPref((prefname + ".count").c_str(), 0);
		if (count<=0) continue;
		if (k>0) json += ",";
		json = json + "{\"name\":\""+name+"\",\"windows\": [";

		unsigned selectedWin = 0;
		DWORD err = GetLastError();
		for (unsigned i=0; i<count; i++) {
			std::string wprefname = prefname + ".window"  + itos(i);
			unsigned tabcount = getIntPref((wprefname + ".count").c_str(), 0);
			std::string wKey = std::string("window")+itos(i);

			if (i>0) json+=",";
			json += "{\"tabs\": [";
			if (tabcount>0) {				
				for (unsigned j=0;j<tabcount;j++) {
					std::string tab;
					if (j>0) tab = ",";
					tab += "{\"entries\": [";
					
					std::string url, title;
					std::string data = getStrPref((wprefname + ".tab"  + itos(j)).c_str(), "");
					
					char* tok = _strtok((char*)data.data(), SEP);
					if (tok == NULL) continue;
					unsigned shcount = atoi(tok);
					if (shcount<=0) continue;
					
					tok = _strtok(NULL, SEP);
					if (tok == NULL) continue;
					int index = atoi(tok);
					
					tok = _strtok(NULL, SEP);
					tok = _strtok(NULL, SEP);

					for (unsigned l=0; l<shcount; l++) {
						if (l>0) tab += ",";
						
						tok = _strtok( NULL, SEP );
						if (tok == NULL) break;
						url = tok;

						tok = _strtok( NULL, SEP );
						if (tok == NULL) break;
						title = tok;

						replace(url, "\\", "\\\\");
						replace(title, "\\", "\\\\");						
						replace(url, "\"", "\\\"");
						replace(title, "\"", "\\\"");
						
						tab += "{ \"url\":\""+url+"\", \"title\":\""+title+"\"}";
					}
					
					tab += "]"; // entries
					tab += ",\"index\":"+itos(index);
					tab += "}"; 

					json += tab;						
				}							
			}
			json += "]"; // tabs
			std::string data = getStrPref(wprefname.c_str(), "");
			std::string win;
			char* tok = _strtok((char*)data.data(), SEP);
			if (tok == NULL) continue;
			win = win + ",\"state\":"+tok;
			if (atoi(tok) & 16) 
				selectedWin = i;
			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			win = win + ",\"screenX\":"+tok;

			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			win = win + ",\"screenY\":"+tok;

			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			win = win + ",\"width\":"+tok;
			
			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			win = win + ",\"height\":"+tok;

			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			//if (*tok) win = win + ",\"selected\":"+tok;
			win = win + ",\"selected\":0";
			tok = _strtok(NULL, SEP); //Reserved

			tok = _strtok(NULL, SEP);
			if (tok == NULL) continue;
			unsigned shcount = atoi(tok);
			if (shcount>0) {
				tok = _strtok(NULL, SEP);
				if (tok != NULL) {
					win = win + ",\"index\":"+tok;
					win += ",\"entries\":[";

					for (unsigned k=0; k<shcount; k++) {
						std::string url, title;
						if (k>0) win += ",";
						tok = _strtok( NULL, SEP );
						if (tok == NULL) break;
						url = tok;

						tok = _strtok( NULL, SEP );
						if (tok == NULL) {break;}
						title = tok;

						replace(url, "\\", "\\\\");
						replace(title, "\\", "\\\\");
						replace(url, "\"", "\\\"");
						replace(title, "\"", "\\\"");

						win += "{ \"url\":\""+url+"\", \"title\":\""+title+"\"}";
					}
					win += "]";
				}
			}
			
			json += win + "}"; 
		}
		json += "],\"selectedWindow\":"+itos(selectedWin);
		json += "}";
	}
	
	json += "]}";

	Document d;
    d.Parse(json.c_str());
	if (d.HasParseError())
		return false;

	std::ofstream osf(sessionFile);
	if (!osf.is_open()) return false;
	osf << json;
	osf.close();

	for (int k = 0; sessions_list[k]; k++) {
		const char* name = sessions_list[k];
		std::string prefname = std::string(pref_prefix) + name;
		int oldcount = getIntPref((prefname + ".count").c_str(), 0);
		for (int j=0;j<oldcount;j++) {
			std::string pref = prefname + ".window" + itos(j);	
			int tabcount = getIntPref((pref + ".count").c_str(), 0);
			kPlugin.kFuncs->DelPreference((char*)(pref + ".count").c_str());
			for (int i=0;i<tabcount;i++) 
				kPlugin.kFuncs->DelPreference((char*)(pref + ".tab" + itos(i)).c_str());
			kPlugin.kFuncs->DelPreference((char*)(prefname + ".window" + itos(j)).c_str());
		}
		kPlugin.kFuncs->DelPreference((prefname + ".count").c_str());
	}	
	kPlugin.kFuncs->DelPreference("kmeleon.plugins.sessions2.list");	

	return true;
}

Document SessionStore::data;

void SessionStore::InitData()
{
	data.SetObject() ;
	data.AddMember("version", 1, data.GetAllocator());
	Value sessions;
	sessions.SetArray();
	data.AddMember("sessions", sessions, data.GetAllocator());
}

bool SessionStore::Load()
{
	if (!data.IsObject()) {
		std::ifstream f(sessionFile);
		if (!f.is_open()) return false;

		struct _stat st;
		if (_tstat(sessionFile, &st) == -1)
			return false;
		
		std::auto_ptr<char> input(new char[st.st_size+1]);
		f.read(input.get(), st.st_size);
		input.get()[f.gcount()] = 0;

		data.Parse(input.get());
		if (data.HasParseError() || !data["sessions"].IsArray())
			InitData();
		UpdateSessionList();
	}
	return true;
}

void SessionStore::Unload()
{
}

bool SessionStore::Read(Session& s)
{
	if (!Load()) return false;
	Value& sessions = data["sessions"];
	
	SizeType i;
	for (i =0; i<sessions.Size(); i++) {		
		if (sessions[i].IsObject() && sessions[i]["name"].IsString() && s.name.compare(sessions[i]["name"].GetString()) == 0) {
			break;
		}
	}

	if (i >= sessions.Size())
		return false;

	Value& session = sessions[i];
	Value& windows = session["windows"];
	
	for (SizeType i=0;i<windows.Size();i++) {
		Window w(NULL);
		Value& win = windows[i];		

		w.width = win["width"].GetInt();
		w.height = win["height"].GetInt();
		w.posx = win["screenX"].GetInt();
		w.posy = win["screenY"].GetInt();
		w.state = win["state"].GetInt();
		w.active = (i == session["selectedWindow"].GetInt());
		
		if (win.HasMember("tabs") && win["tabs"].IsArray() && win["tabs"].Size()) {
			Value& tabs = win["tabs"];
			w.tabcount = tabs.Size();
			w.selectedTab = win["selected"].GetInt();
			w.tabsList.reserve(w.tabcount);
			for (SizeType j=0;j<w.tabcount;j++) {
				Tab tab(NULL, w.hWnd);
				Value& t = tabs[j];

				Value& entries = t["entries"];
				if (!entries.Size()) continue;

				tab.shcount = entries.Size();
				tab.index = t["index"].GetInt();				
				tab.scrollX = t.HasMember("scrollX") ? t["scrollX"].GetInt() : 0;
				tab.scrollY = t.HasMember("scrollY") ? t["scrollY"].GetInt() : 0;
				for (int k=0;k<tab.shcount;k++) {
					tab.urls.push_back(entries[k]["url"].GetString());
					tab.titles.push_back(entries[k]["title"].GetString());
				}
				w.tabsList.push_back(tab);
			}
		}

		if (win.HasMember("entries") && win["entries"].IsArray() && win["entries"].Size() > 0) {
			w.shcount = win["entries"].Size();
			w.index = win["index"].GetInt();
			Value& entries = win["entries"];
			for (unsigned k=0;k<w.shcount;k++) {
				w.urls.push_back(entries[k]["url"].GetString());
				w.titles.push_back(entries[k]["title"].GetString());
			}
		}

		s.windowsList.push_back(w);		
	}

	return s.windowsList.size()>0;
}


void SessionStore::Write(Session& s)
{
	if (!Load()) return;
	if (lock) return;
	lock = true;

	bool isNew = true;
	Document::AllocatorType& allocator = data.GetAllocator();
	Value& sessions = data["sessions"];
	if (!sessions.IsArray()) sessions.SetArray();
	
	Value::ConstValueIterator it = sessions.Begin();
	while (it != sessions.End()) {
		
		if (! (*it).IsObject() || (!(*it)["name"].IsString())) {
			assert(false);
			it = sessions.Erase(it);		
		}
		else if (s.name.compare( (*it)["name"].GetString()) == 0) {
			it = sessions.Erase(it);		
			isNew = false;
		}
			
		else it++;
	}

	Value session;
	session.SetObject();
	Value shit;	
	shit.SetString(s.name.c_str(), allocator);
	session.AddMember("name", shit, allocator);	

	//Delete(s);
	int i = 0, selectedWindow = 0;

	Value wins;
	wins.SetArray();
	WINLIST::iterator iter = s.windowsList.begin();	
	for (iter = s.windowsList.begin(); iter != s.windowsList.end(); iter++) {

		Window& w = (*iter);			
		
		Value win;
		win.SetObject();

		win.AddMember("width", w.width, allocator);
		win.AddMember("height", w.height, allocator);
		win.AddMember("screenX", w.posx, allocator);
		win.AddMember("screenY", w.posy, allocator);
		win.AddMember("state", w.state, allocator);
		if (w.active) selectedWindow = i;		
	
		if (w.tabcount) {
			Value tabs;
			tabs.SetArray();

			Value tab;
			tab.SetObject();
			int j = 0;
			TABLIST::iterator iter;
			for (iter = w.tabsList.begin(); iter != w.tabsList.end(); iter++) {
				Tab& t = (*iter);
				Value tab;
				tab.SetObject();
				tab.AddMember("index", t.index, allocator);
				tab.AddMember("scrollX", t.scrollX, allocator);
				tab.AddMember("scrollY", t.scrollY, allocator);

				Value entries;
				entries.SetArray();

				std::vector<std::string>::iterator iter;
				std::vector<std::string>::iterator iter2;
				for (iter = t.urls.begin(),iter2 = t.titles.begin(); iter != t.urls.end() && iter2 != t.titles.end(); iter++,iter2++) {
					if ((*iter).compare("about:blank") != 0 && (*iter).compare(0, 7, "wyciwyg") != 0) {
						Value e, url, title;
						e.SetObject();
						url.SetString((*iter).c_str(), allocator);
						title.SetString((*iter2).c_str(), allocator);
						e.AddMember("url", url, allocator);
						e.AddMember("title", title, allocator);
						entries.PushBack(e, allocator);
					}
				}

				tab.AddMember("entries", entries, allocator);
				tabs.PushBack(tab, allocator);
			}	
			win.AddMember("tabs", tabs, allocator);
		}

		if (w.shcount > 0) {
			Value entries;
			entries.SetArray();
			std::vector<std::string>::iterator iter;
			std::vector<std::string>::iterator iter2;
			for (iter = w.urls.begin(),iter2 = w.titles.begin(); iter != w.urls.end() && iter2 != w.titles.end(); iter++,iter2++) {
				if ((*iter).compare("about:blank") != 0 && (*iter).compare(0, 7, "wyciwyg") != 0) {
					Value e, url, title;
					e.SetObject();
					url.SetString((*iter).c_str(), allocator);
					title.SetString((*iter2).c_str(), allocator);
					e.AddMember("url", url, allocator);
					e.AddMember("title", title, allocator);
					entries.PushBack(e, allocator);
				}
			}

			win.AddMember("entries", entries, allocator);
			win.AddMember("index", w.index, allocator);
		}

		
		win.AddMember("selected", w.selectedTab, allocator);
		wins.PushBack(win, allocator);
		i++;
	}

	session.AddMember("windows", wins, allocator);		
	session.AddMember("selectedWindow", selectedWindow, allocator);	
	sessions.PushBack(session, allocator);	
	lock = false;
	WriteFile();	
	if (isNew) UpdateSessionList();
}

struct Stream {		
	Stream(TCHAR* file) : of(file) {}
	std::ofstream of;
	void Put (char ch) {of.put (ch);}
	void Flush() {of.close();}
} ;

bool SessionStore::WriteFile(TCHAR* filepath)
{
	bool result = false;
	if (!filepath) filepath = sessionFile;

	TCHAR tmp[1024];
	_tcscpy_s(tmp, filepath);
	_tcscat_s(tmp, _T(".tmp"));	
	
	FILE* f = _tfopen(tmp, _T("w"));
	if (!f) return false;
	FileStream stream(f);
	PrettyWriter<FileStream> filewriter(stream);
	clearerr(f);
	result  = data.Accept(filewriter);
	if (ferror(f)) return false;
	fclose(f);
	
	if (result) {
		TCHAR tmp2[1024];
		_tcscpy_s(tmp2, filepath);
		_tcscat_s(tmp2, _T(".tmp2"));	
		if (MoveFile(filepath, tmp2)) {
			result = MoveFile(tmp, filepath);
			DeleteFile(tmp2);
		} else
			result = MoveFile(tmp, filepath);
	}

	DeleteFile(tmp);
	return result;
}

void SessionStore::Delete(const char* name)
{
	Value& sessions = data["sessions"];
	if (!sessions.IsArray()) sessions.SetArray();
	
	Value::ConstValueIterator it = sessions.Begin();
	while (it != sessions.End()) {
		
		if (
			! (*it).IsObject() 			
			|| (!(*it)["name"].IsString())
		    || (strcmp(name, (*it)["name"].GetString()) == 0)) {
			it = sessions.Erase(it);		
			break;
		}
		else it++;
	}

	WriteFile();
	UpdateSessionList();
}