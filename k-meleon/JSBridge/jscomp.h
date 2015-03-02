#define MOZILLA_STRICT_API
#define XPCOM_GLUE
#include "xpcom-config.h"
#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsISupports.h>
#include "nsIJSBridge.h"
#include "nsIObserver.h"
#include "nsEmbedString.h"
#include "nsCOMArray.h"
#include <map>
#include <list>

#define JSBRIDGE_CID { 0x842170a0, 0x5210, 0x11db, { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}
static NS_DEFINE_CID(kJSBridgeCID, JSBRIDGE_CID);

class CCmdList {
private:
	std::map<UINT, nsCOMPtr<kmICommandFunction>> cmdMap;
public:
	void Add(UINT id, kmICommandFunction* command) {
		cmdMap.insert(std::pair<UINT,kmICommandFunction*>(id, command));
	}
	bool Run(HWND hwnd, UINT command, UINT mode);
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
protected:
	nsCOMArray<nsIObserver> mListeners;
};

