#include "nsIJSBridge.h"
#include "nsEmbedString.h"
#include "nsCOMArray.h"
#include <map>
#include <list>
class nsIObserver;

#define JSBRIDGE_CID { 0x842170a0, 0x5210, 0x11db, { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}
static NS_DEFINE_CID(kJSBridgeCID, JSBRIDGE_CID);

class CCmdList {
private:
	std::map<UINT, nsCOMPtr<kmICommandFunction>> cmdMap;
	std::map<UINT, nsCOMPtr<kmICallback>> eMap;
	std::map<UINT, nsCOMPtr<kmICallback>> cMap;
public:
	void Add(UINT id, kmICommandFunction* command, kmICallback* enabled = nullptr, kmICallback* checked = nullptr) {
		cmdMap.insert(std::pair<UINT,kmICommandFunction*>(id, command));
		if (enabled) eMap.insert(std::pair<UINT,kmICallback*>(id, enabled));
		if (checked) cMap.insert(std::pair<UINT,kmICallback*>(id, checked));
	}
	bool Run(HWND hwnd, UINT command, UINT mode);
	int GetState(int id);
	kmICommandFunction* Get(int id) {
		auto iter = cmdMap.find(id);
		if (iter != cmdMap.end()) return iter->second;
		return nullptr;
	}
	int GetChecked(int id) {
		bool b;
		auto cIter = cMap.find(id);
		if (cIter == cMap.end()) return -1;
		cIter->second->Run(nullptr, &b);
		return b?1:0;
	}
	int GetEnabled(int id) {
		bool b;
		auto eIter = eMap.find(id);
		if (eIter == eMap.end()) return -1;
		eIter->second->Run(nullptr, &b);
		return b?1:0;
	}

};

typedef std::string string;

class CJSCommand: public kmICommand
{
	public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_KMICOMMAND

	CJSCommand() {};
	virtual ~CJSCommand() {};

	nsCString name;
	nsCString desc;
	nsCString accel;
};

class CJSButton : public kmIButton
{
	public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_KMIBUTTON
 
	CJSButton() {};
	virtual ~CJSButton() {};

	string cmd;
	string label;
	string menu;
	string tooltip;
	int id;
};

class CJSBridge : public nsIJSBridge 
{ 
public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_NSIJSBRIDGE
 
	CJSBridge() {};
	virtual ~CJSBridge() {};

	void OnCreateWindow(HWND hWnd, int flag);
	void OnSwitchTab(HWND oldhWnd, HWND newhWnd);
	void OnCreateTab(HWND hWnd);
	void OnDestroyTab(HWND hWnd);
protected:
	nsCOMArray<nsIObserver> mListeners;
};

