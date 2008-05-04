#define MOZILLA_STRICT_API
#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsISupports.h>
#include "nsIJSBridge.h"

#define JSBRIDGE_CID { 0x842170a0, 0x5210, 0x11db, { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}
static NS_DEFINE_CID(kJSBridgeCID, JSBRIDGE_CID);


class CJSBridge : public nsIJSBridge 
{ 
public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_NSIJSBRIDGE
 
	CJSBridge() {};
	virtual ~CJSBridge() {};

};

