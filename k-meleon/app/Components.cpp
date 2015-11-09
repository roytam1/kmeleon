/* Some Gecko interfaces are implemented as components, automatically
registered at application initialization. nsIPrompt is an example:
the default implementation uses XUL, not native windows. Embedding
apps can override the default implementation by implementing the
nsIPromptService interface and registering a factory for it with
the same CID and Contract ID as the default's.
*/

#include "stdafx.h"
#include "mfcembed.h"
#include "UnknownContentTypeHandler.h"
#include "PromptService.h"
#include "CookiePromptService.h"
#include "NSSDialogs.h"
#include "GenKeyPairDialogs.h"
#include "KmAppInfo.h"
#include "KmAbout.h"
//#include "SideBarComp.h"

#include "nsIComponentRegistrar.h"
#include "nsICategoryManager.h"
#include "nsIComponentManager.h"
#include "mozilla/ModuleUtils.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(CPromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CCookiePromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CUnknownContentTypeHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR(CNSSDialogs)
NS_GENERIC_FACTORY_CONSTRUCTOR(CProgressDialog)
NS_GENERIC_FACTORY_CONSTRUCTOR(CGenKeyPairDialogs)
NS_GENERIC_FACTORY_CONSTRUCTOR(KmAppInfo)
NS_GENERIC_FACTORY_CONSTRUCTOR(KmAbout)

NS_DEFINE_NAMED_CID(NS_PROMPTSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_UNKNOWNCONTENTTYPEHANDLER_CID);
NS_DEFINE_NAMED_CID(NS_COOKIEPROMPTSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_NSSDIALOGS_CID);
NS_DEFINE_NAMED_CID(NS_NSSKEYPAIRDIALOGS_CID);
NS_DEFINE_NAMED_CID(NS_DOWNLOAD_CID);
NS_DEFINE_NAMED_CID(NS_KMAPPINFO_CID);
NS_DEFINE_NAMED_CID(NS_KMABOUT_CID);

static const mozilla::Module::CIDEntry kBrowserCIDs[] = {
	{ &kNS_PROMPTSERVICE_CID,false, NULL, CPromptServiceConstructor },
	{ &kNS_UNKNOWNCONTENTTYPEHANDLER_CID,false, NULL, CUnknownContentTypeHandlerConstructor },
	{ &kNS_COOKIEPROMPTSERVICE_CID,false, NULL, CCookiePromptServiceConstructor },
	{ &kNS_NSSDIALOGS_CID,false, NULL, CNSSDialogsConstructor },
	{ &kNS_NSSKEYPAIRDIALOGS_CID,false, NULL, CGenKeyPairDialogsConstructor },
	{ &kNS_DOWNLOAD_CID,false, NULL, CProgressDialogConstructor },
	{ &kNS_KMABOUT_CID,false, NULL, KmAboutConstructor },
	{ NULL }
};

static const mozilla::Module::CIDEntry kBrowserAppCIDs[] = {
	{ &kNS_KMAPPINFO_CID,false, NULL, KmAppInfoConstructor },
	{ NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserContracts[] = {
	{NS_PROMPTSERVICE_CONTRACTID, &kNS_PROMPTSERVICE_CID},
	{NS_CONTENT_PERMISSION_PROMPT_CONTRACTID, &kNS_PROMPTSERVICE_CID },
	{"@mozilla.org/prompter;1", &kNS_PROMPTSERVICE_CID},
	//{NS_HELPERAPPLAUNCHERDLG_CONTRACTID, &kNS_UNKNOWNCONTENTTYPEHANDLER_CID},
	{"@mozilla.org/uriloader/content-handler;1?type=application/k-skin", &kNS_UNKNOWNCONTENTTYPEHANDLER_CID},
	{NS_COOKIEPROMPTSERVICE_CONTRACTID, &kNS_COOKIEPROMPTSERVICE_CID},
	{NS_CERTIFICATEDIALOGS_CONTRACTID, &kNS_NSSDIALOGS_CID},
	{NS_GENERATINGKEYPAIRINFODIALOGS_CONTRACTID, &kNS_NSSKEYPAIRDIALOGS_CID},
	{NS_TRANSFER_CONTRACTID, &kNS_DOWNLOAD_CID},
	{ NS_ABOUT_MODULE_CONTRACTID_PREFIX "home", &kNS_KMABOUT_CID },
	{ NS_ABOUT_MODULE_CONTRACTID_PREFIX "permissions", &kNS_KMABOUT_CID },
	{ NS_ABOUT_MODULE_CONTRACTID_PREFIX "preferences", &kNS_KMABOUT_CID },
	{ NS_ABOUT_MODULE_CONTRACTID_PREFIX "downloads", &kNS_KMABOUT_CID },
	{ NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserAppContracts[] = {
	{"@mozilla.org/xre/app-info;1", &kNS_KMAPPINFO_CID},
	{"@mozilla.org/toolkit/app-startup;1", &kNS_KMAPPINFO_CID},
	{ NULL }	
};

mozilla::Module kBrowserModule = {
    mozilla::Module::kVersion,
    kBrowserCIDs,
    kBrowserContracts
};

mozilla::Module kBrowserModuleApp = {
    mozilla::Module::kVersion,
    kBrowserAppCIDs,
    kBrowserAppContracts
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kBrowserModule;