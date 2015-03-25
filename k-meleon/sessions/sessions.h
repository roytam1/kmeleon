#include "stdafx.h"
#include "../lib/rapidjson/document.h"
#include <string>
#include <vector>
#include <assert.h>

static const char* SEP = "\t";
extern char* _strtok (char * string, const char * control);

class Session;

class SessionStore {
	static rapidjson::Document data;	
	static bool lock;
	static bool Load();
	static void Unload();
	static void InitData();
	static void UpdateSessionList();
public:
	static bool Read(Session& s);
	static void Deletetab(const char* name, int i);
	static void Write(Session& s);
	static void Delete(const char* name);
	static bool Import();
	static bool WriteFile(TCHAR* filepath = nullptr);

	static std::vector<const char*> sessions_list;
	static const std::vector<const char*>& GetSessionsList() {
		Load();
		return sessions_list;
	}
};


static char* pref_prefix = "kmeleon.plugins.sessions2.";


class Tab {

	friend SessionStore;

	int shcount;
	int index;
	int scrollX;
	int scrollY;
	std::vector<std::string> titles;
	std::vector<std::string> urls;
	HWND parent;

public:

	HWND hWnd;
	bool todelete;

	Tab(HWND ahWnd, HWND phWnd) {
		hWnd = ahWnd;
		parent = phWnd;
		shcount = 0;
		todelete = false;
		index = -1;
		scrollX = scrollY = 0;
	}

	void setParent(HWND ahWnd) { parent = ahWnd; }

	void wasclosed() {
		todelete = true;
		hWnd = NULL;
	}

	void update(int aindex, int acount, const char **aUrls, const char ** aTitles)
	{
		shcount = acount;
		index = aindex;
		titles.clear();
		urls.clear();
		for (int i=0; i<acount; i++) {
			titles.push_back(aTitles[i]);
			urls.push_back(aUrls[i]);
		}
	}

	void saveScrollState() {
		kmeleonDocInfo* di = kPlugin.kFuncs->GetDocInfo(hWnd);
		if (!di) return;
		scrollX = di->scrollX;
		scrollY = di->scrollY;
	}

	bool open(bool currenttab, bool last = true) {

		if (shcount<=0) return false;

		this->hWnd = kPlugin.kFuncs->NavigateTo("", currenttab ? OPEN_NORMAL : last ? OPEN_NEWTAB : OPEN_BACKGROUNDTAB, parent);
		if (!hWnd) return false;

		const char** aUrls  = (const char**)new char*[shcount];
		const char** aTitles  = (const char**)new char*[shcount];
		for (int i=0; i<shcount; i++) {
			aUrls[i] = urls[i].c_str();
			aTitles[i] = titles[i].c_str();
		}

		kPlugin.kFuncs->SetMozillaSessionHistory(hWnd, aTitles, aUrls, shcount, index, scrollX, scrollY);
		if (last) kPlugin.kFuncs->GotoHistoryIndex(hWnd, index);
		return false;
	}
	
};


typedef std::vector<Tab> TABLIST;

class Window {

	friend SessionStore;

	unsigned tabcount;
	TABLIST tabsList;

	unsigned shcount;
	std::vector<std::string> titles;
	std::vector<std::string> urls;

	int index;
	int posx;
	int posy;
	int width;
	int height;
	int state;
	bool active;

public:
	int selectedTab;
	HWND hWnd;
	bool todelete;

	Window(HWND ahWnd) {
		hWnd = ahWnd;
		shcount = 0;
		todelete = false;
		posy = posx = width = height = -1;
		tabcount = 0;
		index = -1;
		state = SW_SHOWNORMAL;
		selectedTab = 0;
		active = false;
	}

	void addTab(const Tab& win) {
		tabsList.push_back(win);
		tabcount++;
	}

	void addTab(HWND hWnd);

	TABLIST::iterator const findTab(HWND hWnd) {
		TABLIST::iterator iter;
		for (iter = tabsList.begin(); iter != tabsList.end(); iter++) {
			if ((*iter).hWnd == hWnd) break;
		}
		return iter;
	}

	void removeTab(HWND hWnd) {
		TABLIST::iterator iter = findTab(hWnd);
		if (iter == tabsList.end()) return;
		(*iter).wasclosed();
		tabcount--;
	}

	void update(int aindex, int acount, const char** aurls, const char** atitles) {
		shcount = acount;
		index = aindex;
		titles.clear();
		urls.clear();
		for (int i=0; i<acount; i++) {
			titles.push_back(atitles[i]);
			urls.push_back(aurls[i]);
		}
	}


	void update(HWND tab, int aindex, int acount, const char** aurls, const char** atitles)
	{
		if (tabcount == 0) {
			update(aindex, acount, aurls, atitles);
			return;
		}

		TABLIST::iterator iter = findTab(tab);
		assert(iter != tabsList.end());
		if (iter == tabsList.end()) return;

		(*iter).update(aindex, acount, aurls, atitles);
		(*iter).saveScrollState();
	}
	
	void setActive(bool a)
	{
		active = a;
	}

	void saveWindowState()
	{
		if (!::IsWindow(hWnd))
			return;

		if (IsIconic(hWnd))
			state = SW_SHOWMINIMIZED;
		else if (IsZoomed(hWnd))
			state = SW_SHOWMAXIMIZED;
		else 
			state = SW_SHOWNORMAL;

		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(hWnd, &wp);

		posx = wp.rcNormalPosition.left;
		posy = wp.rcNormalPosition.top;
		width = wp.rcNormalPosition.right - posx;
		height = wp.rcNormalPosition.bottom - posy;
	}

	bool open() {
		//assert(hWnd == NULL);
		hWnd = kPlugin.kFuncs->NavigateTo(NULL, OPEN_NEW, NULL);
		if (!hWnd) return false;

		state = state & 0xf;
		if (active) state |= 16;

		int screenWidth   = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int screenHeight  = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

		if (width>screenWidth) width = screenWidth;
		if (height>screenHeight) height = screenHeight;
		if (posx<left) posx = left;
		if (posy<top) posy = top;
		if (posx>left+screenWidth) posx = left + screenWidth - width; 
		if (posy>top+screenHeight) posy = top + screenHeight - height; 

		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		wp.flags = 0;
		wp.showCmd = state;
		wp.rcNormalPosition.left = posx;
		wp.rcNormalPosition.top = posy;
		wp.rcNormalPosition.right =  posx + width;
		wp.rcNormalPosition.bottom = posy + height;
		::SetWindowPlacement(hWnd, &wp);

		if (tabcount > 0)  {
			TABLIST::iterator iter;
			int i = 0;
			for (iter = tabsList.begin(); iter != tabsList.end(); iter++) {
				(*iter).setParent(hWnd);
				(*iter).open(iter == tabsList.begin(), i++ == selectedTab);
			}
			ShowWindow(hWnd, state);
			return active;
		}

		const char** aUrls  = (const char**)new char*[shcount];
		const char** aTitles  = (const char**)new char*[shcount];
		for (unsigned i=0; i<shcount; i++) {
			aUrls[i] = urls[i].c_str();
			aTitles[i] = titles[i].c_str();
		}

		kPlugin.kFuncs->SetMozillaSessionHistory(hWnd, aTitles, aUrls, shcount, index, 0, 0);
		ShowWindow(hWnd, state);
		return active;
	}

	void flush() {
		TABLIST::iterator iter = tabsList.begin();
		while (iter != tabsList.end()) {
			if ((*iter).todelete) iter = tabsList.erase(iter);
			else iter++;
		}
	}
	
	bool moveTab(HWND tab1, HWND tab2)
	{
		TABLIST::iterator iter1 = findTab(tab1);
		if (iter1 == tabsList.end()) return false;

		TABLIST::iterator iter2 = findTab(tab2);
		if (iter2 == tabsList.end()) return false;
		
		TABLIST newList;
		TABLIST::iterator iter;
		for (iter = tabsList.begin(); iter < tabsList.end(); iter++)
		{
			if (iter == iter1) 
				continue;
			if (iter == iter2) {
				if (iter1<iter2) {
					newList.push_back(*iter);
					newList.push_back(*iter1);			
					continue;
				}
				newList.push_back(*iter1);			
			}
			newList.push_back(*iter);
		}
		tabsList.swap(newList);
		return true;
	}

	void wasclosed() {
		saveWindowState();
		todelete = true;
	}

	void tabwasclosed(HWND hTab) {
		TABLIST::iterator iter = findTab(hTab);
		if (iter == tabsList.end()) return;
		(*iter).wasclosed();
	}

	static void deletetabpref(std::string& pref) {
		int oldcount = getIntPref((pref + ".count").c_str(), 0);
		kPlugin.kFuncs->DelPreference((char*)(pref + ".count").c_str());
		for (int j=0;j<oldcount;j++) 
			kPlugin.kFuncs->DelPreference((char*)(pref + ".tab" + itos(j)).c_str());
	}

	Tab lastTab() {
		if (tabsList.size()<=0) return Tab(NULL, NULL);
		Tab w = tabsList.back();
		tabsList.pop_back();
		return w;
	}

	Tab getTab(HWND hWnd) {
		TABLIST::iterator iter;
		for (iter = tabsList.begin(); iter != tabsList.end(); iter++) {
			if ((*iter).hWnd == hWnd) break;
		}
		//assert(iter != tabsList.end());
		if (iter == tabsList.end()) return Tab(NULL, NULL);
		return *iter;
	}

	friend class SessionStore;
};

typedef std::vector<Window> WINLIST;

class Session {
	std::string name;
	WINLIST windowsList;
	
public:

	static bool loading;
	static int openOption;

	Session() {
		//name = aName;
	}

	void addWindow(const Window& win) {
		windowsList.push_back(win);
	}

	void addWindow(HWND hWnd) {
		windowsList.push_back(Window(hWnd));
	}

	void addTab(HWND hWnd, HWND hTab) {
		WINLIST::iterator iter = findWindow(hWnd);
		assert(iter != windowsList.end());
		if (iter == windowsList.end()) return;
		(*iter).addTab(hTab);
	}

	WINLIST::iterator const findWindow(HWND hWnd) {
		WINLIST::iterator iter;
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			if ((*iter).hWnd == hWnd) break;
		}
		return iter;
	}

	Window* getWindow(HWND hWnd) {
		WINLIST::iterator iter;
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			if ((*iter).hWnd == hWnd) break;
		}
		assert(iter != windowsList.end());
		if (iter == windowsList.end()) return NULL;
		return &*iter;
	}

	void removeWindow(HWND hWnd) {
		WINLIST::iterator iter = findWindow(hWnd);
		if (iter == windowsList.end()) return;
		(*iter).wasclosed();
	}

	void removeTab(HWND hWnd, HWND hTab) {
		WINLIST::iterator iter = findWindow(hWnd);
		if (iter == windowsList.end()) return;
		(*iter).tabwasclosed(hTab);
	}
	
	Window* findWindowWithTab(HWND hTab) {
		WINLIST::iterator iter;
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			Tab tab = (*iter).getTab(hTab);
			if (tab.hWnd)
				return (&*iter);
		}
		return NULL;
	}

	void updateWindow(HWND hWnd, int index, int shcount, const char** aurls, const char** atitles) {
		flush();
		WINLIST::iterator iter = findWindow(hWnd);
		if (iter == windowsList.end()) return;

		(*iter).update(index, shcount, aurls, atitles);
		(*iter).saveWindowState();
	}

	void updateWindow(HWND hWnd, HWND hTab, int index, int shcount, const char** aurls, const char** atitles) {
		flush();
		WINLIST::iterator iter = findWindow(hWnd);
		if (iter == windowsList.end()) return;

		(*iter).update(hTab, index, shcount, aurls, atitles);
		(*iter).saveWindowState();
	}

	void setActiveWindow(HWND hWnd) {
		WINLIST::iterator iter = windowsList.begin();
		while (iter != windowsList.end()) {
			(*iter).setActive((*iter).hWnd == hWnd);
			iter++;
		}
	}

	void flush() {
		WINLIST::iterator iter = windowsList.begin();
		while (iter != windowsList.end()) {
			if ((*iter).todelete) iter = windowsList.erase(iter);
			else {
				(*iter).flush();
				iter++;
			}
		}
	}

	void saveSession(const char* name) {
		if (loading) return;
		this->name = name;
		return SessionStore::Write(*this);
	}

	bool loadSession(const char* aname) {
		name = aname;
		return SessionStore::Read(*this);
	}

	void deleteSession(const char* name) {
		SessionStore::Delete(name);
	}


	bool open() {
		if (!windowsList.size()) return false;

		HWND activewnd = NULL;
		WINLIST::iterator iter = windowsList.begin();
		
		// XXX Change the open tab option
		int tmp = 0;
		kPlugin.kFuncs->GetPreference(PREF_INT, "kmeleon.tabs.onOpenOption", &openOption, &openOption);
		kPlugin.kFuncs->SetPreference(PREF_INT, "kmeleon.tabs.onOpenOption", &tmp, FALSE);

		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			if ((*iter).open()) activewnd = (*iter).hWnd;
		}

		kPlugin.kFuncs->SetPreference(PREF_INT, "kmeleon.tabs.onOpenOption", &openOption, FALSE);

		if (activewnd) SetForegroundWindow(activewnd);
		return true;
	}

	void limit(unsigned l) {
		while (windowsList.size()>l) windowsList.erase(windowsList.begin());
	}

	Window lastWindow() {
		if (windowsList.size()<=0) return Window(NULL);
		Window w = windowsList.back();
		windowsList.pop_back();
		return w;
	}
 
	void close_except(HWND hWnd) {
		WINLIST::iterator iter;
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			if ((*iter).hWnd != hWnd && !(*iter).todelete) 
				SendMessage((*iter).hWnd, WM_CLOSE, 0, 0);
		}
	}

	void close(HWND hWnd) {
		WINLIST::iterator iter = findWindow(hWnd);
		SendMessage((*iter).hWnd, WM_CLOSE, 0, 0);
		//(*iter).wasclosed();
	}

	void empty() {
		windowsList.empty();
	}

	friend class SessionStore;
};


