#define MOZILLA_STRICT_API
#define XPCOM_GLUE
#include "xpcom-config.h"
#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsISupports.h>
#include "nsIJSBridge.h"
#include <map>

#define JSBRIDGE_CID { 0x842170a0, 0x5210, 0x11db, { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}
static NS_DEFINE_CID(kJSBridgeCID, JSBRIDGE_CID);

class CCmdList {
private:
	std::map<UINT, nsCOMPtr<nsICommandFunction>> cmdMap;
public:
	void Add(UINT id, nsICommandFunction* command) {
		cmdMap.insert(std::pair<UINT,nsICommandFunction*>(id, command));
	}
	bool Run(UINT command, UINT mode) {
		auto iter = cmdMap.find(command);
		if (iter != cmdMap.end() && iter->second) {
			iter->second->OnCommand(mode);
			return true;
		}
		return false;
	}
};

class CJSBridge : public nsIJSBridge 
{ 
public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_NSIJSBRIDGE
 
	CJSBridge() {};
	virtual ~CJSBridge() {};

};

