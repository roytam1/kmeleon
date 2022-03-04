/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "mozilla-config.h"
#include <windows.h>
#include <winsock.h>
#include <stdio.h> 
#include <string>
#include <iostream>
#define XPCOM_GLUE
#include "xpcom-config.h"
#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsISupports.h>
#include <nsIComponentRegistrar.h>
#include <nsIObserverService.h>
#include <mozilla/ModuleUtils.h>
#include <nsIServiceManager.h>
#include <nsServiceManagerUtils.h>
#include "nsICategoryManager.h"
#include "nsIURI.h"
#include "nsGenericFactory.h"
#include "adcomp.h"
#include "nsEmbedString.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMLocation.h" 
#include "nsIEffectiveTLDService.h" 
#include "nsIDOMNode.h"
#include "nsIPrefService.h"
#include "nsIURI.h"
#include "nsIFile.h"
#include "nsIIOService.h"

#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"
#include "nsILoadGroup.h"
#include "nsIHttpChannel.h"
#include "nsIRunnable.h"
#include "prthread.h"
#include "nsIThreadManager.h"
#include "nsIThread.h"

#include "mozilla/ChaosMode.h" // ChaosMode hack


#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"
//#include "KMeleonConst.h"

#define PLUGIN_NAME "KMeleon Adblock"
long DoMessage(const char *, const char *, const char *, long, long);
kmeleonPlugin kPlugin = {
	KMEL_PLUGIN_VER,
	PLUGIN_NAME,
	DoMessage
};

char logFile[MAX_PATH];

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <vector>
#include <queue>

bool InitSubs();

using namespace std;

void LOG(const char* s) {
	
	nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
	if (!prefService) return;
	nsCOMPtr<nsIPrefBranch> prefs;
	prefService->GetBranch("kmeleon.plugins.adblock.", getter_AddRefs(prefs));
	if (!prefs) return;

	bool logging = false;
	prefs->GetBoolPref("logging", &logging);
	//kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.adblock.logging", &logging, &logging);
	if (!logging) return;
	ofstream input(logFile, ofstream::app);
	input<<s<<"\n";
}

static vector<std::string> &split(const string &s, const char delim, vector<std::string> &elems) {
	istringstream ss(s);
	string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

static vector<string> split(const string &s, const char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

class Filter {
protected:
	string text;
	Filter(const string& s) { text = s; };

	

public:
	static regex elemhideRegExp;
	static regex regexpRegExp;
	static regex optionsRegExp;

	string getText() const { return text; }
	static Filter* FromText(const string& s);
	virtual bool Matches(const string& location, unsigned type, const string& domain, bool thirdParty) = 0;
	virtual bool Blocking() const {return false;}
	
};

regex Filter::elemhideRegExp("^([^\\/\\*\\|\\@\"!]*?)#(\\@)?(?:([\\w\\-]+|\\*)((?:\\([\\w\\-]+(?:[$^*]?=[^\\(\\)\"]*)?\\))*)|#([^{}]+))$");
regex Filter::regexpRegExp("^(@@)?\\/.*\\/(?:\\$~?[\\w\\-]+(?:=[^,\\s]+)?(?:,~?[\\w\\-]+(?:=[^,\\s]+)?)*)?$");
regex Filter::optionsRegExp("\\$(~?[\\w\\-]+(?:=[^,\\s]+)?(?:,~?[\\w\\-]+(?:=[^,\\s]+)?)*)$");

const char* typeMapKey[] = {
	"other", "script", "image", "stylesheet", 
	"object", "subdocument", "document", "xbl", 
	"ping", "xmlhttprequest", "object_subrequest", "dtd",
	"media", "font", "popup" };

const unsigned typeMapValue[] = {1,2,4,8,16,32,64,1,1,2048,4096,1,16384,32768,0x10000000};
const unsigned typePolicyValue[] = {
	nsIContentPolicy::TYPE_OTHER,nsIContentPolicy::TYPE_SCRIPT,
	nsIContentPolicy::TYPE_IMAGE,nsIContentPolicy::TYPE_STYLESHEET,
	nsIContentPolicy::TYPE_OBJECT,nsIContentPolicy::TYPE_SUBDOCUMENT,
	nsIContentPolicy::TYPE_DOCUMENT,nsIContentPolicy::TYPE_XBL,
	nsIContentPolicy::TYPE_PING,nsIContentPolicy::TYPE_XMLHTTPREQUEST,
	nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,nsIContentPolicy::TYPE_DTD,
	nsIContentPolicy::TYPE_MEDIA,nsIContentPolicy::TYPE_FONT, 0xffff
};

const size_t typeLength = sizeof(typePolicyValue)/sizeof(unsigned);

class RegExpFilter:Filter {
	friend Filter;
	
protected:
	int thirdParty;
	bool blocking;
	string regexpSrc;
	regex regexp;
	bool parsed;
	bool matchCase;
	bool collapse;
	string domainsTxt;
	unordered_map<string,bool> domains;
	vector<string> sitekeys;
	unsigned contentType;

	friend std::ostream& operator<<(std::ostream& os, const RegExpFilter& s)
	{
		return os;
	}

	// Extraction operator
	friend std::istream& operator>>(std::istream& is, RegExpFilter& s)
	{
		return is;
	}

	RegExpFilter(const string& s):Filter(s),
		thirdParty(-1), matchCase(false), collapse(false), parsed(false), blocking(true), contentType(0)
	{
		string options;
		size_t optPos = string::npos;
		if (s[0] == '@' && s[1] == '@') blocking = false;
		if (s.find_first_of('$') != string::npos) {
			std::smatch match;
			if (regex_search(s, match, Filter::optionsRegExp)) {
				optPos = match.position();
				options = match[1];
				auto v  = split(options, ',');
				string option, value;
				for (auto i=v.begin(); i!=v.end(); i++) {
					
					auto pos = i->find_first_of('=');
					if (pos != string::npos) {
						option = i->substr(0,pos);
						value = i->substr(pos+1);
					} else option = *i;
					replace(option.begin(), option.end(), '-', '_');
					
					bool negate = false;
					if (option[0] == '~') {
						negate = true;
						option = option.substr(1);
					}

					if (option == "match_case")
						matchCase = !negate;
					else if (option == "domain")
						domainsTxt = value;
					else if (option == "third_party")
						thirdParty = !negate;
					else if (option == "collapse")
						collapse = 1;
					else if (option == "sitekey" && value.length()>0)
						sitekeys = split(value, '|');
					else {
						bool found = false;
						for (int i=0;i<typeLength;i++) 
							if (option.compare(typeMapKey[i])==0) {
								if (negate) {
									if (contentType == 0) contentType = 0x7FFFFFFF;
									contentType &= ~typeMapValue[i];
								} else {
									contentType |= typeMapValue[i];
								}
								found = true;
								break;
							}
						if (!found) 
							LOG(("Unknow option " + option +": "+s).c_str());
					}

				}
			}
		}

		if (contentType==0) contentType = 0x7FFFFFFF;
		regexpSrc = s.substr(blocking ? 0 : 2, blocking ? optPos : optPos - 2);
		if (regexpSrc.length()>2 && regexpSrc[0] == '/' && regexpSrc[regexpSrc.length()-1] == '/') {
			regexpSrc = regexpSrc.substr(1, regexpSrc.length()-2);
		}

	};

	bool MatchDomain(string domain) {
		if (!domains.size() && domainsTxt.length()) {
			vector<string> _domains =  split(domainsTxt, '|');
			bool hasInclude = false;
			for (auto it=_domains.begin();it<_domains.end();it++) {
				bool inc = true;				
				if ((*it)[0] == '~') {
					inc = false;
				} else hasInclude = true;

				domains.insert(pair<string,bool>((*it),inc));
			}
			domains.insert(pair<string,bool>("",!hasInclude));
		}

		if (!domains.size())
			return true;
		
		while (true) {
			auto i = domains.find(domain);
			if (i != domains.end())
				return i->second;

			size_t pos = domain.find_first_of('.');
			if (pos == string::npos) break;
			domain = domain.substr(domain.find_first_of('.')+1);
		}

		return domains.find("")->second;
	}
public:	
	
	virtual bool Blocking() const { return blocking; }
	virtual bool Matches(const string& location, unsigned contentType, const string& docDomain, bool thirdParty)
	{
		const char* searchList[] = {"\\*+", "\\^\\|$", "\\W", "\\\\\\*", "\\\\\\^", "^\\\\\\|\\\\\\|", "^\\\\\\|", "\\\\\\|$", "^(\\.\\*)", "(\\.\\*)$" };
		const char* replaceList[] = {"*", "^", "\\$&", ".*", "(?:[\\x00-\\x24\\x26-\\x2C\\x2F\\x3A-\\x40\\x5B-\\x5E\\x60\\x7B-\\x7F]|$)", "^[\\w\\-]+:\\/+(?!\\/)(?:[^\\/]+\\.)?", "^", "$", "", "" };
		const unsigned optionList[] = {std::tr1::regex_constants::match_any};

		if (!MatchDomain(docDomain) ||
			thirdParty && this->thirdParty == 0 ||
			!thirdParty && this->thirdParty == 1 ||
			((this->contentType & contentType) == 0))
			return false;

		if (!parsed) {
			for (int i = 0; i<10; i++) {
				regex e (searchList[i]);
				regexpSrc = regex_replace(regexpSrc, e, string(replaceList[i]));	
			}
			regexp.assign(regexpSrc, matchCase ? std::tr1::regex_constants::ECMAScript : std::tr1::regex_constants::icase);
			parsed = true;
		}
		
		return regex_search(location, regexp);
	}




	static RegExpFilter* FromText(const string& s) 
	{
		return new RegExpFilter(s);		
	}

};

Filter* Filter::FromText(const string& s) {
	if (!s.length() || s[0] == '!') return nullptr;
	if (s.find_first_of('#') != string::npos) {
		smatch match;
		if (regex_match(s, match, Filter::elemhideRegExp))
			return nullptr; // TODO
	}
	return RegExpFilter::FromText(s);
}

class CacheComp {
	unordered_map<string,unsigned>* h;
public:
	CacheComp(unordered_map<string,unsigned>* ah) : h(ah) {};
	bool operator()(const string& x,const string& y) const { 
		return h->at(x)<h->at(y);
	};
};

class AdRules {
public:
	AdRules() : cacheSize(1000), cache(/*CacheComp(&cacheHit)*/) {
	};
	void setCacheSize(size_t cs) {
		cacheSize = cs;
	}
protected:
	size_t cacheSize;
	unordered_map<string,vector<Filter*>> wKeywordFilter;
	unordered_map<string,vector<Filter*>> bKeywordFilter;
	unordered_map<string,Filter*> cache;
	queue<string> cacheQueue;
	unordered_map<string,unsigned> cacheHit;
	//map<string,string> keywordByFilter;
	vector<Filter*> bFilterList;
	vector<Filter*> wFilterList;
	
	static regex keyRegex;
	string makeKey(Filter* f)
	{
		string res;
		string text = f->getText();
		if (regex_match(text, Filter::regexpRegExp))
			return res;

		std::smatch match;
		if (regex_search(text, match, Filter::optionsRegExp)) {
			size_t optPos = match.position();
			text = text.substr(0, optPos);
		}

		if (text.substr(0, 2) == "@@")
			text = text.substr(2);
		
		string::const_iterator first = text.begin();
		string::const_iterator last = text.end();
			
		unsigned resCount = 0xffff, resLength = 0;
		while (regex_search(first, last, match, keyRegex)) {
			string s = match[0].str().substr(1);
			
			unordered_map<string,vector<Filter*>>::const_iterator it;
			unsigned count;
			if (f->Blocking()) {
				it = bKeywordFilter.find(s);
				count = it != bKeywordFilter.end() ? it->second.capacity() : 0;
			} else {
				it = wKeywordFilter.find(s);
				count = it != wKeywordFilter.end() ? it->second.capacity() : 0;
			}

			if (count<resCount || (count == resCount && s.length() > resLength)) {
				res = s;
				resLength = res.length();
				resCount = count;
			}

			first += match.position() + match.length();
		}
		return res;
	}
public:
	size_t count() 
	{
		return bKeywordFilter.size() + wKeywordFilter.size();
	}

	bool disabled()
	{
		bool res = false;
		nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
		if (!prefService) return false;
		nsCOMPtr<nsIPrefBranch> prefs;
		prefService->GetBranch("kmeleon.plugins.adblock.", getter_AddRefs(prefs));
		if (!prefs) return false;
		prefs->GetBoolPref("disabled", &res);
		//kPlugin.kFuncs->GetPreference(PREF_BOOL, "kmeleon.plugins.adblock.disabled", &res, &res);
		return res;
	}

	void add(Filter* f)
	{
		string key = makeKey(f);

		if (f->Blocking()) {
			auto it = bKeywordFilter.find(key);		
			if (it == bKeywordFilter.end()) {
				vector<Filter*> v;
				v.push_back(f);
				bKeywordFilter.insert( pair<string, vector<Filter*>>(key,  v));
			} else it->second.push_back(f);
			bFilterList.push_back(f);
		} else {
			auto it = wKeywordFilter.find(key);		
			if (it == wKeywordFilter.end()) {
				vector<Filter*> v;
				v.push_back(f);
				wKeywordFilter.insert( pair<string, vector<Filter*>>(key,  v));
			} else it->second.push_back(f);
			wFilterList.push_back(f);
		}		
	}

	void reset() 
	{
		for (auto i=bFilterList.begin();i<bFilterList.end();i++) {
			delete (*i);
		}
		for (auto i=wFilterList.begin();i<wFilterList.end();i++) {
			delete (*i);
		}
		bKeywordFilter.empty();
		bFilterList.empty();
		wKeywordFilter.empty();
		wFilterList.empty();
	}

	Filter* match(const string& location, unsigned type, const string& domain, bool thirdParty)
	{
		string key = location + " " + domain + " " + to_string((_ULonglong)type) + " " + (thirdParty ? "1" : "0");
		auto hit = cacheHit.find(key);
		if (hit != cacheHit.end()) hit->second++; else cacheHit[key] = 1;
		auto it = cache.find(key);
		if (it != cache.end()) return it->second;

		Filter* m = _match(location, type, domain, thirdParty);

		if (cache.size() > cacheSize) {
			cache.erase(cacheQueue.front());
			cacheQueue.pop();
		}
		cache[key] = m;
		cacheQueue.push(key);
		return m;
	}

	Filter* _match(const string& location, unsigned type, const string& domain, bool thirdParty)
	{
		

		vector<string> keywords;
		smatch match;
		regex e("[a-z0-9%]{3,}");
		string::const_iterator first = location.begin();
		string::const_iterator last = location.end();
		while (regex_search(first, last, match, e)) {
			keywords.push_back(match[0].str());
			first += match.position() + match.length();
		}
		keywords.push_back("");

		for (auto i=keywords.begin();i<keywords.end();i++) {
			auto it = wKeywordFilter.find(*i);
			if (it != wKeywordFilter.end()) {
				for (auto itt=it->second.begin();itt<it->second.end();itt++)
					if ((*itt)->Matches(location, type, domain, thirdParty))
						return *itt;
			}
		}
		for (auto i=keywords.begin();i<keywords.end();i++) {
			auto itb = bKeywordFilter.find(*i);
			if (itb != bKeywordFilter.end()) {
				for (auto itt=itb->second.begin();itt<itb->second.end();itt++)
					if ((*itt)->Matches(location, type, domain, thirdParty))						
						return *itt;
			}
		}

		return nullptr;
	}

};

static AdRules rules;
regex AdRules::keyRegex("[^a-z0-9%*][a-z0-9%]{3,}(?=[^a-z0-9%*])");


class DeferClose: public nsIRunnable 
{
	nsCOMPtr<nsIDOMWindow> mDom;
public:
	NS_DECL_ISUPPORTS 
	
	DeferClose(nsIDOMWindow* dom) {
		mDom = dom;
	};

	~DeferClose() {
	}

	NS_IMETHODIMP Run() {
		mDom->Close();
		return NS_OK;
	};
};

NS_IMPL_ISUPPORTS(DeferClose, nsIRunnable);

Policy::Policy()
{
		nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
		if (!observerService) return;		
		observerService->AddObserver(this, "http-on-opening-request", false);
		observerService->AddObserver(this, "content-document-global-created", false);
		if (!rules.count()) InitSubs();
}

bool Policy::IsInWhiteList(nsIURI* aUri)
{
	nsCString scheme;
	aUri->GetScheme(scheme);
	if (scheme.Compare("chrome") == 0 || scheme.Compare("about") == 0)
		return true;
	return false;
}

bool Policy::Process(nsIDOMWindow* wnd, nsIDOMNode* node, nsIURI* aContentLocation, unsigned int contentType)
{
	nsCString domain;
	nsCOMPtr<nsIDOMLocation> location;
	wnd->GetLocation(getter_AddRefs(location));	
	if (location) {
		nsString _domain;
		location->GetHost(_domain);	
		NS_UTF16ToCString(_domain, NS_CSTRING_ENCODING_UTF8, domain);
	}

	bool thirdParty = false;
	if (!tld) tld = do_GetService("@mozilla.org/network/effective-tld-service;1");
	if (tld) {
		nsCString ret;
		nsresult rv = tld->GetBaseDomain(aContentLocation, 0, ret);
		if (NS_SUCCEEDED(rv)) {
			nsCString ret2;
			rv = tld->GetBaseDomainFromHost(domain, 0, ret2);
			if (NS_SUCCEEDED(rv)) {
				thirdParty = ret.Compare(ret2) != 0;
			}
		}
		if (NS_FAILED(rv)) {
			aContentLocation->GetHost(ret);
			thirdParty = ret.Compare(domain) != 0;
		}
	}	

	nsCString spec;
	aContentLocation->GetSpec(spec);	

	string loc(spec.get());
	Filter* res = rules.match(loc, contentType, string(domain.get()), thirdParty);
	if (res) LOG(((res->Blocking()?"Block: ":"Accept: ")+loc+" / Rule: "+res->getText()).c_str());
	return (res && res->Blocking()) ? true : false;
}

NS_IMETHODIMP Policy::ShouldLoad(nsContentPolicyType aContentType, nsIURI *aContentLocation, nsIURI *aRequestOrigin, nsISupports *aContext, const nsACString & aMimeTypeGuess, nsISupports *aExtra, nsIPrincipal *aRequestPrincipal, int16_t *_retval)
{
	*_retval = nsIContentPolicy::ACCEPT;
	if (!aContext || aContentType == Policy::TYPE_DOCUMENT)
		return NS_OK;
	
	if (IsInWhiteList(aContentLocation))
		return NS_OK;

	if (rules.disabled())
		return NS_OK;
	
	unsigned contentType = 0;
	for (int i=0;i<typeLength;i++)
		if (aContentType == typePolicyValue[i]) {
			contentType = typeMapValue[i];
			break;
		}

	nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aContext);
	nsCOMPtr<nsIDOMDocument> owner;
	node->GetOwnerDocument(getter_AddRefs(owner));
	if (!owner) {
		nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aContext);
		if (!doc) return NS_OK;
		doc->GetOwnerDocument(getter_AddRefs(owner));		
		if (!owner)  owner = doc;
	}		

	nsCOMPtr<nsIDOMWindow> wnd;
	owner->GetDefaultView(getter_AddRefs(wnd));
	if (!wnd) return NS_OK;	
	
	bool res = Process(wnd, node, aContentLocation, contentType);
	*_retval = res ? nsIContentPolicy::REJECT_REQUEST : nsIContentPolicy::ACCEPT;	
	return NS_OK;

	nsCString domain;
	nsCOMPtr<nsIDOMLocation> location;
	wnd->GetLocation(getter_AddRefs(location));	
	if (location) {
		nsString _domain;
		location->GetHost(_domain);	
		NS_UTF16ToCString(_domain, NS_CSTRING_ENCODING_UTF8, domain);
	}

	bool thirdParty = false;
	if (!tld) tld = do_GetService("@mozilla.org/network/effective-tld-service;1");
	if (tld) {
		nsCString ret;
		nsresult rv = tld->GetBaseDomain(aContentLocation, 0, ret);
		if (NS_SUCCEEDED(rv)) {
			nsCString ret2;
			rv = tld->GetBaseDomainFromHost(domain, 0, ret2);
			if (NS_SUCCEEDED(rv)) {
				thirdParty = ret.Compare(ret2) != 0;
			}
		}
		if (NS_FAILED(rv)) {
			aContentLocation->GetHost(ret);
			thirdParty = ret.Compare(domain) != 0;
		}
	}

	nsCString spec;
	aContentLocation->GetSpec(spec);
	
	string loc(spec.get());
	Filter* filter = rules.match(loc, contentType, string(domain.get()), thirdParty);
	if (res) LOG(((filter->Blocking()?"Block: ":"Accept: ")+loc+" / Rule: "+filter->getText()).c_str());

	*_retval = (filter && filter->Blocking()) ? nsIContentPolicy::REJECT_REQUEST : nsIContentPolicy::ACCEPT;	
	return NS_OK;
}

NS_IMETHODIMP Policy::ShouldProcess(nsContentPolicyType aContentType, nsIURI *aContentLocation, nsIURI *aRequestOrigin, nsISupports *aContext, const nsACString & aMimeType, nsISupports *aExtra, nsIPrincipal *aRequestPrincipal, int16_t *_retval)
{
	*_retval = nsIContentPolicy::ACCEPT;
	return NS_OK;
}



NS_IMETHODIMP Policy::Observe(nsISupports *aSubject, const char * aTopic, const PRUnichar * aData)
{
	static bool popup = false;
	static vector<nsIDOMWindow*> domPopup;
	if (strcmp(aTopic, "content-document-global-created") == 0)
	{
		nsCOMPtr<nsPIDOMWindow> dom = do_QueryInterface(aSubject);
		if (!dom) return NS_OK;

		nsCOMPtr<nsPIDOMWindow> opener = dom->GetOpener();
		if (!opener) return NS_OK;

		nsCOMPtr<nsIDOMLocation> location;
		dom->GetLocation(getter_AddRefs(location));
		if (!location) return NS_OK;

		nsString href;
		location->GetHref(href);
		/*if (href.Length() && href.Compare(L"about:blank") != 0) {
			nsCOMPtr<nsIDOMDocument> document;
			opener->GetDocument(getter_AddRefs(document));
			nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
			NS_ENSURE_TRUE(ios, NS_OK);
			nsCOMPtr<nsIURI> uri;
			nsCString _href;
			NS_UTF16ToCString(href, NS_CSTRING_ENCODING_UTF8, _href);
			ios->NewURI(_href, nullptr, nullptr, getter_AddRefs(uri));
			if (Process(opener, document, uri, 0x10000000))
				dom->Close();
		}
		else */domPopup.push_back(dom);

	}

	if (strcmp(aTopic, "http-on-opening-request") == 0) 
	{
		if (domPopup.size()) {
			nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aSubject);
			if (!channel) return NS_OK;

			nsCOMPtr<nsIDOMWindow> dom;
			nsCOMPtr<nsIInterfaceRequestor> callbacks;
			channel->GetNotificationCallbacks(getter_AddRefs(callbacks));
			if (callbacks) {
				nsCOMPtr<nsILoadContext> context;
				callbacks->GetInterface(NS_GET_IID(nsILoadContext), getter_AddRefs(context));
				if (context) {
					context->GetAssociatedWindow(getter_AddRefs(dom));
				}
			}
			if (!dom) {
				nsCOMPtr<nsILoadGroup> group;
				channel->GetLoadGroup(getter_AddRefs(group));
				if (group) {
					group->GetNotificationCallbacks(getter_AddRefs(callbacks));
					if (callbacks) {
						nsCOMPtr<nsILoadContext> context;
						callbacks->GetInterface(NS_GET_IID(nsILoadContext), getter_AddRefs(context));
						if (context) {
							context->GetAssociatedWindow(getter_AddRefs(dom));
						}
					}
				}
			}
			
			if (!dom) return NS_OK;

			auto it = find(domPopup.begin(), domPopup.end(), dom);
			if (it == domPopup.end())
				return NS_OK;

			domPopup.erase(it);
			nsCOMPtr<nsIURI> uri;
			channel->GetURI(getter_AddRefs(uri));

			nsCOMPtr<nsPIDOMWindow> pdom = do_QueryInterface(dom);
			nsCOMPtr<nsPIDOMWindow> opener = pdom->GetOpener();
			if (!opener) return NS_OK;

			nsCOMPtr<nsIDOMDocument> document;
			opener->GetDocument(getter_AddRefs(document));

			if (Process(opener, document, uri, 0x10000000)) {
				nsCOMPtr<nsIThreadManager> tm = do_GetService("@mozilla.org/thread-manager;1");
				if (!tm) return NS_OK;
				nsCOMPtr<nsIThread> thread;
				tm->GetCurrentThread(getter_AddRefs(thread));
				if (!thread) return NS_OK;
				thread->Dispatch(new DeferClose(dom), nsIThread::DISPATCH_NORMAL);				
			}				
		}
	}
	
	return NS_OK;
}

NS_IMPL_ISUPPORTS(Policy, nsIContentPolicy, nsIObserver); 

NS_GENERIC_FACTORY_CONSTRUCTOR(Policy)
	
static const mozilla::Module::CIDEntry kBrowserCIDs[] = {
	{ &kPolicyCID,true, NULL, PolicyConstructor },
	{ NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserContracts[] = {
	{"@kmeleon/adblock;1", &kPolicyCID},
	{ NULL }
};

static const mozilla::Module::CategoryEntry kCategory[] = {
	{"content-policy", "adblock", "@kmeleon/adblock;1"},
	{ NULL }
};


static const mozilla::Module kBrowserModule = {
    mozilla::Module::kVersion,
    kBrowserCIDs,
    kBrowserContracts,
	kCategory
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kBrowserModule;

static const nsModuleComponentInfo components = 
{
	"Adblock content policy",  
	POLICY_CID, 
	"@kmeleon/adblock;1",  
	PolicyConstructor
};

static nsCOMPtr<nsIGenericFactory> componentFactory;
extern nsresult NS_NewGenericFactory(nsIGenericFactory* *result,
                     const nsModuleComponentInfo *info);

using namespace std;

#if 0

#define BUFFERSIZE 4096
bool DownloadFile(string url, LPCSTR filename){
    string request; // HTTP Header //
 
    char buffer[BUFFERSIZE];
    struct sockaddr_in serveraddr;
    int sock;
 
    WSADATA wsaData;
    int port = 80;
   
    // Remove's http:// part //
    if(url.find("http://") != -1){
        string temp = url;
        url = temp.substr(url.find("http://") + 7);
    }
   
    // Split host and file location //
    int dm = url.find("/");
    string file = url.substr(dm);
    string shost = url.substr(0, dm);
   
    // Generate http header //
    request += "GET " + file + " HTTP/1.0\r\n";
    request += "Host: " + shost + "\r\n";
    request += "\r\n";
 
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
        return FALSE;
 
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return FALSE;
 
    memset(&serveraddr, 0, sizeof(serveraddr));
   
    // ip address of link //
    hostent *record = gethostbyname(shost.c_str());
    in_addr *address = (in_addr * )record->h_addr;
    string ipd = inet_ntoa(* address);
    const char *ipaddr = ipd.c_str();
 
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ipaddr);
    serveraddr.sin_port = htons(port);
 
    if (connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        return FALSE;
 
    if (send(sock, request.c_str(), request.length(), 0) != request.length())
        return FALSE;
 
    int nRecv, npos;
    nRecv = recv(sock, (char*)&buffer, BUFFERSIZE, 0);
   
    // getting end of header //
    string str_buff = buffer;
    npos = str_buff.find("\r\n\r\n");
   
    // open the file in the beginning //
    HANDLE hFile;
    hFile = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
 
    // Download file //
    DWORD ss;
    while((nRecv > 0) && (nRecv != INVALID_SOCKET)){
        if(npos > 0){
            char bf[BUFFERSIZE];
            // copy from end position of header //
            memcpy(bf, buffer + (npos + 4), nRecv - (npos + 4));
            WriteFile(hFile, bf,nRecv - (npos + 4), &ss, NULL);
        }else{
            // write normally if end not found //
            WriteFile(hFile, buffer, nRecv, &ss, NULL);
        }
       
        // buffer cleanup  //
        ZeroMemory(&buffer, sizeof(buffer));
        nRecv = recv(sock, (char*)&buffer, BUFFERSIZE, 0);
        str_buff = buffer;
        npos = str_buff.find("\r\n\r\n");
    }
   
    // Close connection and handle //
    CloseHandle(hFile);
    closesocket(sock);
    WSACleanup();
 
    return TRUE;
}

#endif

#include "nsIWebBrowserPersist.h"
#include "nsCWebBrowserPersist.h"
#include "nsComponentManagerUtils.h"
#include "nsIIOService.h"
#include "nsIWebProgressListener.h"
#include "nsIHttpChannel.h"

/* void onStateChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in unsigned long aStateFlags, in nsresult aStatus); */
NS_IMETHODIMP Subscriptions::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, uint32_t aStateFlags, nsresult aStatus)
{
	if ( (aStateFlags & nsIWebProgressListener::STATE_STOP)) {
		  if (NS_SUCCEEDED(aStatus)) {
			  LOG(("Download success " + (*(dwnIt-1))).c_str());
			  std::ifstream src(tmpFile);
			  std::ofstream dst(adFile, ofstream::app);
			  dst << src.rdbuf() << "\n";
			  dst.close();
			  src.close();
			  _wremove(tmpFile.c_str());
			  _Download();
			  
		  } else {
			  LOG(("Download failed " + (*(dwnIt - 1))).c_str());
		  }
		  return NS_OK;
	}

	if (aStateFlags & nsIWebProgressListener::STATE_START) {
		nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
		if (!channel) return NS_OK;
		nsCOMPtr<nsIHttpChannel> httpchannel(do_QueryInterface(channel));
		if (httpchannel) {
			httpchannel->GetResponseHeader(nsEmbedCString("content-disposition"), mContentDisposition);
		}
	}

    return NS_OK;
}

/* void onProgressChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in long aCurSelfProgress, in long aMaxSelfProgress, in long aCurTotalProgress, in long aMaxTotalProgress); */
NS_IMETHODIMP Subscriptions::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, int32_t aCurSelfProgress, int32_t aMaxSelfProgress, int32_t aCurTotalProgress, int32_t aMaxTotalProgress)
{
    return NS_OK;
}

/* void onLocationChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsIURI aLocation, [optional] in unsigned long aFlags); */
NS_IMETHODIMP Subscriptions::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *aLocation, uint32_t aFlags)
{
    return NS_OK;
}

/* void onStatusChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in nsresult aStatus, in wstring aMessage); */
NS_IMETHODIMP Subscriptions::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar * aMessage)
{
    return NS_OK;
}

/* void onSecurityChange (in nsIWebProgress aWebProgress, in nsIRequest aRequest, in unsigned long aState); */
NS_IMETHODIMP Subscriptions::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, uint32_t aState)
{
    return NS_OK;
}

#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
Subscriptions::Subscriptions() 
{
	nsCOMPtr<nsIFile> nsDir;
	nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(nsDir));
	if (NS_FAILED(rv)) return;
	
	nsEmbedString pathBuf;
	rv = nsDir->GetPath(pathBuf);

	tmpFile = wstring(pathBuf.get()) + L"\\adblocktmp.txt";	
	adFile = wstring(pathBuf.get()) + L"\\adblock.txt";
	cacheFile = wstring(pathBuf.get()) + L"\\adblock.cache";
}

bool Subscriptions::Parse()
{
	std::ifstream infile(cacheFile);
	if (infile.good()) {
		WIN32_FILE_ATTRIBUTE_DATA attr, attr2;
		GetFileAttributesExW(cacheFile.c_str(), GetFileExInfoStandard, &attr);
		GetFileAttributesExW(adFile.c_str(), GetFileExInfoStandard, &attr2);
		if (CompareFileTime(&attr2.ftLastWriteTime, &attr2.ftLastWriteTime) < 0) {

		}
	}

	std::ofstream cache(cacheFile);
	std::ifstream input(adFile);
    std::string line;
    while( std::getline( input, line ) ) {
		Filter* f = Filter::FromText(line);
		if (!f) continue;
		rules.add(f);
    }
	cache.close();
	return true;
}

bool Subscriptions::_Download()
{
	if (dwnIt == subscriptionList.end()) {
		Parse();
		return true;
	}

	persist = do_CreateInstance(NS_WEBBROWSERPERSIST_CONTRACTID);
	if (!persist) return false;
	
	persist->SetPersistFlags(
				nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION|
				nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES);

	persist->SetProgressListener(this);

	nsCOMPtr<nsIIOService> ios = do_GetService("@mozilla.org/network/io-service;1");
	NS_ENSURE_TRUE(ios, false);

	nsCOMPtr<nsIURI> uri;
	ios->NewURI(nsEmbedCString((*dwnIt).c_str()), nullptr, nullptr, getter_AddRefs(uri));
	nsCOMPtr<nsIFile> file;
	NS_NewLocalFile(nsDependentString(tmpFile.c_str()), TRUE, getter_AddRefs(file));
	
	LOG(("Downloading: " + (*dwnIt)).c_str());
	dwnIt++;
	nsresult rv = persist->SaveURI(uri, nullptr, nullptr, 0, nullptr, nullptr, file, nullptr);
	NS_ENSURE_SUCCESS(rv, false);
    return true;
}

bool Subscriptions::Add(const char* s) 
{
	subscriptionList.push_back(s);
	return true;
}

bool Subscriptions::Download() 
{	
	_wremove(adFile.c_str());	
	dwnIt = subscriptionList.begin();
	if (dwnIt == subscriptionList.end()) return true;
	_Download();
	return true;
}

bool Subscriptions::RulesExists()
{
	std::ifstream infile(adFile);
    return infile.good();
}

NS_IMPL_ISUPPORTS(Subscriptions, nsIWebProgressListener)

bool InitSubs() 
{
	rules.reset();
	Subscriptions* s = new Subscriptions();
	if (!s->RulesExists())
	{
		nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
		if (!prefService) return false;
		nsCOMPtr<nsIPrefBranch> prefs;
		prefService->GetBranch("kmeleon.plugins.adblock.", getter_AddRefs(prefs));
		if (!prefs) return false;

		nsCString nsSubs;
		prefs->GetCharPref("subscriptions", getter_Copies(nsSubs));
		if (!nsSubs.Length()) return true;
		vector<string> subs = split(nsSubs.get(),'|');

		/*int len = kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.adblock.subscriptions", nullptr, nullptr);
		if (!len) return true;

		char* _subs = new char[len+1];
		kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.plugins.adblock.subscriptions", _subs, _subs);
		vector<string> subs = split(_subs,'|');
		delete _subs;	*/
		
		for (auto it = subs.begin();it<subs.end();it++)
			s->Add((*it).c_str());
	
		return s->Download();
	}
	else
		s->Parse();
    delete s;
    return true;
}

BOOL Init() 
{
	nsCOMPtr<nsIComponentRegistrar> compReg;
	nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
	NS_ENSURE_SUCCESS(rv, FALSE);
   
	rv = NS_NewGenericFactory(getter_AddRefs(componentFactory), &components);
	if (NS_FAILED(rv) || !componentFactory)
		return FALSE;

	rv = compReg->RegisterFactory(components.mCID,
				 components.mDescription,
				 components.mContractID,
				 componentFactory);

	nsCOMPtr<nsIServiceManager> servMan; 
	rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsICategoryManager> catman;
	rv = servMan->GetServiceByContractID("@mozilla.org/categorymanager;1",  NS_GET_IID(nsICategoryManager), getter_AddRefs(catman));
	NS_ENSURE_SUCCESS(rv, FALSE);

	rv = catman->AddCategoryEntry("content-policy", components.mContractID, components.mContractID, false, true, nullptr);
	NS_ENSURE_SUCCESS(rv, FALSE);
  	//rv = catman->AddCategoryEntry("net-channel-event-sinks", components.mContractID, components.mContractID, false, true, nullptr);
	//NS_ENSURE_SUCCESS(rv, FALSE);

	InitSubs();

	char rulesFile[MAX_PATH], profDir[MAX_PATH];
	kPlugin.kFuncs->GetFolder(FolderType::ProfileFolder, profDir, MAX_PATH);

	strcpy(rulesFile, profDir);
	strcat(rulesFile, "\\adblock.txt");
	strcpy(logFile, profDir);
	strcat(logFile, "\\adblock.log");

	size_t cacheSize = 1000;
	cacheSize = kPlugin.kFuncs->GetPreference(PREF_INT, "kmeleon.plugins.adblock.cacheSize", &cacheSize, &cacheSize);
	rules.setCacheSize(cacheSize);		
	return TRUE;
}

BOOL Quit()
{
	rules.reset();
	nsresult rv;
	nsCOMPtr<nsIComponentRegistrar> compReg;
	rv = NS_GetComponentRegistrar(getter_AddRefs (compReg));
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	nsCOMPtr<nsICategoryManager> catman = do_GetService("@mozilla.org/categorymanager;1");
	if (catman) catman->DeleteCategoryEntry("content-policy", components.mContractID, false);

	/*nsCOMPtr<nsIServiceManager> servMan; 
	rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	nsCOMPtr<nsICategoryManager> catman;
	rv = servMan->GetServiceByContractID("@mozilla.org/categorymanager;1",  NS_GET_IID(nsICategoryManager), getter_AddRefs(catman));
	NS_ENSURE_SUCCESS(rv, FALSE);

	rv = catman->DeleteCategoryEntry("content-policy", components.mContractID, false);
	NS_ENSURE_SUCCESS(rv, FALSE);*/

	rv = compReg->UnregisterFactory(components.mCID, componentFactory);
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

#include "nsIWebBrowser.h"

void Create(HWND hwnd)
{
	nsCOMPtr<nsIWebBrowser> browser;
	kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(browser));
	if (!browser) return;

	nsCOMPtr<nsIDOMWindow> dom;
	browser->GetContentDOMWindow(getter_AddRefs(dom));
	if (!dom) return;
	
	nsCOMPtr<nsPIDOMWindow> pdom = do_QueryInterface(dom);
	nsCOMPtr<nsIDOMWindow> opener = pdom->GetOpener();

	//kmeleonDocInfo *info = kPlugin.kFuncs->GetDocInfo(hwnd);
	//rules.match(info->url
}

long DoMessage(const char *to, const char *from, const char *subject,
			   long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (strcmp(subject, "Init") == 0) {
			Init();
		}
		else if (strcmp(subject, "Quit") == 0) {
			Quit();
		}
		else if (strcmp(subject, "Create") == 0) {
			Create((HWND)data1);
		}
		else if (strcmp(subject, "DoAccel") == 0) {
			char* accel = (char *)data1;
			//if (strcmp(accel, "update") == 0)
			//	*(int *)data2 = 
			Quit();
		}
		else return 0;
		return 1;
	}
	return 0;
}

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}

extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}

#if 1 //ChaosMode hack
namespace mozilla {
	namespace detail {

		Atomic<uint32_t> gChaosModeCounter(0);
		ChaosFeature gChaosFeatures = None;

	} /* namespace detail */
} /* namespace mozilla */
#endif
