#include "stdafx.h"
#include <string>
#include <vector>
#include <assert.h>

static const char* SEP = "\t";
extern char* _strtok (char * string, const char * control);

static char* pref_prefix = "kmeleon.plugins.sessions2.";

	std::string itos(int i) {
		char s[35];
		::itoa(i, s, 10);
		return s;
	}

	void setIntPref(const char* prefname, int value, bool flush = FALSE) {
		kPlugin.kFuncs->SetPreference(PREF_INT, prefname, &value, flush);
	}

	void setStrPref(const char* prefname, char* value, bool flush = FALSE) {
		kPlugin.kFuncs->SetPreference(PREF_STRING, prefname, value, flush);
	}

	int getIntPref(const char* prefname, int defvalue) {
		int value = defvalue;
		kPlugin.kFuncs->GetPreference(PREF_INT, prefname, &value, &value);
		return value;
	}

	std::string getStrPref(const char* prefname, const char* defvalue) {
		int len = kPlugin.kFuncs->GetPreference(PREF_STRING, prefname, 0, (void*)defvalue);
		char* str = new char[len+1];
		kPlugin.kFuncs->GetPreference(PREF_STRING, prefname, (void*)str, (void*)defvalue);
		std::string ret = str;
		delete [] str;
		return ret;
	}

class Tab {
	int shcount;
	int index;
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
	}

	void setParent(HWND ahWnd) { parent = ahWnd; }

	std::string topref() {
		std::string purls;
		std::string ptitles;
		if (shcount<=0) return "";

		std::vector<std::string>::iterator iter;
		std::vector<std::string>::iterator iter2;
		for (iter = urls.begin(),iter2 = titles.begin(); iter != urls.end() && iter2 != titles.end(); iter++,iter2++) {
			if ((*iter).compare("about:blank") != 0)
				purls += *iter + SEP + *iter2 + SEP;
			else shcount--;
		}

		return itos(shcount) + SEP + itos(index) + SEP + SEP + SEP + purls;
	}

	bool frompref(char* pref) {
		char* tok = _strtok(pref, SEP);
		if (tok == NULL) return false;
		shcount = atoi(tok);
		if (shcount<=0) return false;

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		index = atoi(tok);

		tok = _strtok(NULL, SEP);
		tok = _strtok(NULL, SEP);

		int i;
		for (i=0; i<shcount; i++) {
			tok = _strtok( NULL, SEP );
			if (tok == NULL) break;
			urls.push_back(tok);

			tok = _strtok( NULL, SEP );
			if (tok == NULL) {urls.pop_back();break;}
			titles.push_back(tok);
		}

		shcount = i;
		if (shcount>0 && index>shcount) index = shcount - 1;
		return shcount>0;
	}

	void wasclosed() {
		todelete = true;
		hWnd = NULL;
	}

	bool save(std::string& pref) {
		if (index>=0 && urls[index].compare(0, 7, "wyciwyg") != 0)
			setStrPref(pref.c_str(), (char*)topref().c_str());
		return true;
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

	bool open(bool currenttab, bool last = true) {

		if (shcount<=0) return false;

		this->hWnd = kPlugin.kFuncs->NavigateTo("", currenttab ? OPEN_NORMAL : OPEN_NEWTAB, parent);
		if (!hWnd) return false;

		const char** aUrls  = (const char**)new char*[shcount];
		const char** aTitles  = (const char**)new char*[shcount];
		for (int i=0; i<shcount; i++) {
			aUrls[i] = urls[i].c_str();
			aTitles[i] = titles[i].c_str();
		}

		kPlugin.kFuncs->SetMozillaSessionHistory(hWnd, aTitles, aUrls, shcount, index);
		if (last) kPlugin.kFuncs->GotoHistoryIndex(index);
		return false;
	}
	
};


typedef std::vector<Tab> TABLIST;

class Window {
	int tabcount;
	TABLIST tabsList;

	int shcount;
	std::vector<std::string> titles;
	std::vector<std::string> urls;

	int index;
	int posx;
	int posy;
	int width;
	int height;
	int state;

public:

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
	}

	void addTab(const Tab& win) {
		tabsList.push_back(win);
		tabcount++;
	}

	void addTab(HWND hWnd) {
		tabsList.push_back(Tab(hWnd, this->hWnd));
		tabcount++;
	}

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

		if (GetActiveWindow() == hWnd)
			state |= 16;

		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(hWnd, &wp);

		posx = wp.rcNormalPosition.left;
		posy = wp.rcNormalPosition.top;
		width = wp.rcNormalPosition.right - posx;
		height = wp.rcNormalPosition.bottom - posy;
	}

    std::string topref() {
		std::string purls;
		std::string ptitles;

		saveWindowState();
		
		std::vector<std::string>::iterator iter;
		std::vector<std::string>::iterator iter2;
		for (iter = urls.begin(),iter2 = titles.begin(); iter != urls.end() && iter2 != titles.end(); iter++,iter2++) {
			purls += *iter + SEP + *iter2 + SEP;
		}

		return 
		itos(state) + SEP + itos(posx) + SEP + itos(posy) + SEP + 
		itos(width) + SEP + itos(height) + SEP + SEP + SEP + itos(shcount) + SEP + 
		itos(index) + SEP  + purls;
	}

	bool frompref(std::string& prefname) {

		std::string p = getStrPref(prefname.c_str(), "");
		char* pref = (char*)p.c_str();

		char* tok = _strtok(pref, SEP);
		if (tok == NULL) return false;
		state = atoi(tok);

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		posx = atoi(tok);

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		posy = atoi(tok);

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		width = atoi(tok);
		
		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		height = atoi(tok);

		RECT screen;
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);

		if (posx>screen.right) posx = 0;
		if (posy>screen.bottom) posy = 0;
		if (width < 10) width = 10;
		if (height < 10) height = 10;

		tabcount = getIntPref((prefname + ".count").c_str(), 0);
		if (tabcount>0) {
			for (int i=0;i<tabcount;i++) {
				Tab tab(NULL, hWnd);
				if (tab.frompref((char*)getStrPref((prefname + ".tab"  + itos(i)).c_str(), "").c_str()))
					tabsList.push_back(tab);
			}

			tabcount = tabsList.size();
			return tabcount>0;
		}

		tok = _strtok(NULL, SEP); //Reserved
		tok = _strtok(NULL, SEP);

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		shcount = atoi(tok);
		if (shcount<=0) return false;

		tok = _strtok(NULL, SEP);
		if (tok == NULL) return false;
		index = atoi(tok);

		int i;
		for (i=0; i<shcount; i++) {
			tok = _strtok( NULL, SEP );
			if (tok == NULL) break;
			urls.push_back(tok);

			tok = _strtok( NULL, SEP );
			if (tok == NULL) {urls.pop_back();break;}
			titles.push_back(tok);
		}

		shcount = i;
		if (shcount>0 && index>shcount) index = shcount - 1;
		return shcount > 0;
	}

	bool open() {
		//assert(hWnd == NULL);
		hWnd = kPlugin.kFuncs->NavigateTo(NULL, OPEN_NEW, NULL);
		if (!hWnd) return false;

        bool active = state & 16;
		state = state & 0xf;

		RECT screen;
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, 0);
		int screenWidth   = screen.right - screen.left;
		int screenHeight  = screen.bottom - screen.top;

		if (width>screenWidth) width = screenWidth;
		if (height>screenHeight) height = screenHeight;
		if (posx<0) posx = 0;
		if (posy<0) posy = 0;
		if (posx>screenWidth) posx = screenWidth - width; 
		if (posy>screenHeight) posy = screenHeight - height; 

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
			for (iter = tabsList.begin(); iter != tabsList.end(); iter++) {
				(*iter).setParent(hWnd);
				(*iter).open(iter == tabsList.begin(), iter == (--tabsList.end()));
			}
			ShowWindow(hWnd, state);
			return active;
		}

		const char** aUrls  = (const char**)new char*[shcount];
		const char** aTitles  = (const char**)new char*[shcount];
		for (int i=0; i<shcount; i++) {
			aUrls[i] = urls[i].c_str();
			aTitles[i] = titles[i].c_str();
		}

		kPlugin.kFuncs->SetMozillaSessionHistory(hWnd, aTitles, aUrls, shcount, index);
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


	bool save(std::string& pref) {
		if (tabcount || (index>=0 && urls[index].compare(0, 7, "wyciwyg") != 0))
			setStrPref(pref.c_str(), (char*)topref().c_str());
		if (tabcount) {
			deletetabpref(pref);
			setIntPref((pref + ".count").c_str(), tabsList.size());

			int i = 0;
			TABLIST::iterator iter;
			for (iter = tabsList.begin(); iter != tabsList.end(); iter++) {
				(*iter).save(pref + ".tab" + itos(i));
				i++;
			}
		}
		return true;
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

};

typedef std::vector<Window> WINLIST;

class Session {
	std::string name;
	WINLIST windowsList;
public:

	static bool loading;

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
	}

	void updateWindow(HWND hWnd, HWND hTab, int index, int shcount, const char** aurls, const char** atitles) {
		flush();
		WINLIST::iterator iter = findWindow(hWnd);
		if (iter == windowsList.end()) return;

		(*iter).update(hTab, index, shcount, aurls, atitles);
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
		deleteSession(name);

		std::string prefname = std::string(pref_prefix) + name;

		int i = 0;
		WINLIST::iterator iter = windowsList.begin();
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			(*iter).save(prefname + ".window" + itos(i));
			i++;
		}
		setIntPref((prefname + ".count").c_str(), windowsList.size(), TRUE);
	}

	bool loadSession(const char* aname) {
		name = aname;
		std::string prefname = std::string(pref_prefix) + name;
		int count = getIntPref((prefname + ".count").c_str(), 0);
		if (count<=0) return false;

		for (int i=0; i<count; i++) {
			Window win(NULL);
			if (win.frompref(prefname + ".window" + itos(i)))
				windowsList.push_back(win);
		}

		return windowsList.size()>0;
	}

	void deleteSession(const char* name) {
		std::string prefname = std::string(pref_prefix) + name;
		int oldcount = getIntPref((prefname + ".count").c_str(), windowsList.size());
		for (int j=0;j<oldcount;j++) {
			Window::deletetabpref(prefname + ".window" + itos(j));
			kPlugin.kFuncs->DelPreference((char*)(prefname + ".window" + itos(j)).c_str());
		}
		kPlugin.kFuncs->DelPreference((prefname + ".count").c_str());
	}


	bool open() {
		if (!windowsList.size()) return false;

		HWND activewnd = NULL;
		WINLIST::iterator iter = windowsList.begin();
		for (iter = windowsList.begin(); iter != windowsList.end(); iter++) {
			if ((*iter).open()) activewnd = (*iter).hWnd;
		}
		if (activewnd) SetForegroundWindow(activewnd);
		return true;
	}

	void limit(int l) {
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

};