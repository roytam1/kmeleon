
#include <vector>
#include "nsIContentPolicy.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserPersist.h"
#include "nsIObserver.h"
#include "nsEmbedString.h"
#include "nsIDOMWindow.h"

class nsIEffectiveTLDService;


#define POLICY_CID { 0x842170b0, 0x5220, 0x12db, { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}}
static NS_DEFINE_CID(kPolicyCID, POLICY_CID);

class Policy:public nsIContentPolicy, public nsIObserver {
protected:
	nsCOMPtr<nsIEffectiveTLDService> tld;
public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_NSICONTENTPOLICY
	NS_DECL_NSIOBSERVER

	Policy();
	virtual ~Policy() {
		nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
		if (!observerService) return;
		//observerService->RemoveObserver(this, "content-document-global-created");
		//observerService->RemoveObserver(this, "http-on-opening-request");
	};

	bool IsInWhiteList(nsIURI* aUri);
	bool Process(nsIDOMWindow* wnd, nsIDOMNode* node, nsIURI* location, unsigned int contentType);
};



class Subscriptions : nsIWebProgressListener
{
protected:
	std::vector<std::string> subscriptionList;
	std::vector<std::string>::iterator dwnIt;
	nsCOMPtr<nsIWebBrowserPersist> persist;
	nsEmbedCString mContentDisposition;
	bool _Download();
	std::wstring tmpFile;
	std::wstring adFile;
	std::wstring cacheFile;
public:
	NS_DECL_ISUPPORTS 
	NS_DECL_NSIWEBPROGRESSLISTENER;

	Subscriptions();
	virtual ~Subscriptions() {};

	bool RulesExists();
	bool Parse();
	bool Download();
	bool Add(const char* url);
};
