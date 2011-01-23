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
//#include "TooltipsProvider.h"
#include "NSSDialogs.h"
#if GECKO_VERSION < 19
#include "FontPackageHandler.h"
#else
#include "ContentDispatchChooser.h"

//52be7412-eff1-46b6-aec6-38bb1c997dfc
#define NS_CONTENTDISPATCHCHOOSER_CID \
	{ 0x52be7412, 0xeff1, 0x46b6, { 0xae, 0xc6, 0x38, 0xbb, 0x1c, 0x99, 0x7d, 0xfc } }



#endif
#include "GenKeyPairDialogs.h"
//#include "SideBarComp.h"

#include "nsIComponentRegistrar.h"
#include "nsICategoryManager.h"
#include "nsIComponentManager.h"


//#include "nsEmbedCID.h" //NS_PROMPTSERVICE_CONTRACTID
//#include "nsICookiePromptService.h" ////NS_COOKIEPROMPTSERVICE_CONTRACTID
//#include "nsIHelperAppLauncherDialog.h" //NS_IHELPERAPPLAUNCHERDLG_CONTRACTID
//#include "nsICertificateDialogs.h" //NS_CERTIFICATEDIALOGS_CONTRACTID
//#include "nsIBadCertListener.h" //NS_BADCERTLISTENER_CONTRACTID
//#include "nsCTooltipTextProvider.h" //NS_TOOLTIPTEXTPROVIDER_CONTRACTID

NS_GENERIC_FACTORY_CONSTRUCTOR(CPromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CCookiePromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CUnknownContentTypeHandler)
//NS_GENERIC_FACTORY_CONSTRUCTOR(CTooltipTextProvider)
NS_GENERIC_FACTORY_CONSTRUCTOR(CNSSDialogs)
NS_GENERIC_FACTORY_CONSTRUCTOR(CProgressDialog)

#if GECKO_VERSION < 19
NS_GENERIC_FACTORY_CONSTRUCTOR(CFontPackageHandler)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR(ContentDispatchChooser)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(CGenKeyPairDialogs)
//NS_GENERIC_FACTORY_CONSTRUCTOR(CSideBarComp)

/*NS_DECL_CLASSINFO(CSideBarComp)

static NS_METHOD
RegisterSidebar(nsIComponentManager *aCompMgr, nsIFile *aPath,
                const char *registryLocation, const char *componentType,
                const nsModuleComponentInfo *info)
{
	nsCOMPtr<nsICategoryManager> cm =
		do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
	NS_ENSURE_TRUE (cm, NS_ERROR_FAILURE);

	return cm->AddCategoryEntry("JavaScript global property",
				    "sidebar", NS_SIDEBAR_CONTRACTID,
				    PR_FALSE, PR_TRUE, nsnull);
}*/

static const nsModuleComponentInfo sAppComps[] = {
	{
		"Prompt Service",
		NS_PROMPTSERVICE_CID,
		NS_PROMPTSERVICE_CONTRACTID,
		CPromptServiceConstructor
	},
#if GECKO_VERSION < 193
	{ 
		"Nonblocking Alert Service", 
		NS_PROMPTSERVICE_CID, 
		NS_NONBLOCKINGALERTSERVICE_CONTRACTID, 
		CPromptServiceConstructor
	},
#endif
	{
		"Helper App Launcher Dialog",
		NS_UNKNOWNCONTENTTYPEHANDLER_CID,
		NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
		CUnknownContentTypeHandlerConstructor
	},
	/*{
		"Tooltip Text Provider",
		NS_TOOLTIPTEXTPROVIDER_CID,
		NS_TOOLTIPTEXTPROVIDER_CONTRACTID,
		CTooltipTextProviderConstructor
	},*/
 	{
		"Cookie Prompt Service",
		NS_COOKIEPROMPTSERVICE_CID,
		NS_COOKIEPROMPTSERVICE_CONTRACTID,
		CCookiePromptServiceConstructor
	},
#if GECKO_VERSION < 19
	{
		"PSM Dialog Impl",
		NS_NSSDIALOGS_CID,
		NS_BADCERTLISTENER_CONTRACTID,
		CNSSDialogsConstructor
	},
	{ "nsFontPackageHandler", 
	   NS_FONTPACKAGEHANDLER_CID,
       "@mozilla.org/locale/default-font-package-handler;1",
       CFontPackageHandlerConstructor
	},
#else
	{
		"Content Dispatcher Service",
		NS_CONTENTDISPATCHCHOOSER_CID,
		"@mozilla.org/content-dispatch-chooser;1",
		ContentDispatchChooserConstructor
	},


#endif
	{
		"PSM Dialog Impl",
		NS_NSSDIALOGS_CID,
		NS_CERTIFICATEDIALOGS_CONTRACTID,
		CNSSDialogsConstructor
	},
	{
		GTK_NSSKEYPAIRDIALOGS_CLASSNAME,
		NS_NSSKEYPAIRDIALOGS_CID,
		NS_GENERATINGKEYPAIRINFODIALOGS_CONTRACTID,
		CGenKeyPairDialogsConstructor
	},
	{
	    "Download",
		NS_DOWNLOAD_CID,
		NS_TRANSFER_CONTRACTID,
		CProgressDialogConstructor
	}
	/*,
	{	
		"nsSideBar",
		NS_SIDEBAR_CID,
		NS_SIDEBAR_CONTRACTID,
		CSideBarCompConstructor,
		RegisterSidebar,
		nsnull,
		nsnull,
		NS_CI_INTERFACE_GETTER_NAME(CSideBarComp),
		nsnull,
		&NS_CLASSINFO_NAME(CSideBarComp),
		nsIClassInfo::DOM_OBJECT
	}*/
};

#define NB_COMPONENTS sizeof(sAppComps)/sizeof(nsModuleComponentInfo)

extern NS_COM_GLUE nsresult NS_NewGenericFactory(nsIGenericFactory* *result,
                     const nsModuleComponentInfo *info);

nsresult CMfcEmbedApp::OverrideComponents()
{
	nsCOMPtr<nsIComponentRegistrar> compReg;
	nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIComponentManager> compManager;
	rv = NS_GetComponentManager(getter_AddRefs(compManager));
	NS_ENSURE_SUCCESS(rv, rv);

	for (int i=0; i<NB_COMPONENTS ; i++)
	{
		nsCOMPtr<nsIGenericFactory> componentFactory;
		rv = NS_NewGenericFactory(getter_AddRefs(componentFactory), &(sAppComps[i]));
		if (NS_FAILED(rv) || !componentFactory)
		{
			AfxMessageBox(IDS_UNABLE_REGISTER_FACTORY, MB_OK, 0);
			continue;
		}

		rv = compReg->RegisterFactory(sAppComps[i].mCID,
			sAppComps[i].mDescription,
			sAppComps[i].mContractID,
			componentFactory);

		if (sAppComps[i].mRegisterSelfProc)
			rv = sAppComps[i].mRegisterSelfProc(compManager, nsnull, nsnull, nsnull, &sAppComps[i]);
	}

	return rv;
}
