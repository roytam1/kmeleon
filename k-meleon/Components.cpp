/* Some Gecko interfaces are implemented as components, automatically
registered at application initialization. nsIPrompt is an example:
the default implementation uses XUL, not native windows. Embedding
apps can override the default implementation by implementing the
nsIPromptService interface and registering a factory for it with
the same CID and Contract ID as the default's.
*/

#include "stdafx.h"
#include "nsIComponentRegistrar.h"
#include "mfcembed.h"
#include "UnknownContentTypeHandler.h"
#include "PromptService.h"
#include "TooltipsProvider.h"

//#include "nsEmbedCID.h" //NS_PROMPTSERVICE_CONTRACTID
//#include "nsICookiePromptService.h" ////NS_COOKIEPROMPTSERVICE_CONTRACTID
//#include "nsIHelperAppLauncherDialog.h" //NS_IHELPERAPPLAUNCHERDLG_CONTRACTID
//#include "nsICertificateDialogs.h" //NS_CERTIFICATEDIALOGS_CONTRACTID
//#include "nsIBadCertListener.h" //NS_BADCERTLISTENER_CONTRACTID
//#include "nsCTooltipTextProvider.h" //NS_TOOLTIPTEXTPROVIDER_CONTRACTID

NS_GENERIC_FACTORY_CONSTRUCTOR(CPromptService)
NS_GENERIC_FACTORY_CONSTRUCTOR(CUnknownContentTypeHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR(CTooltipTextProvider)
NS_GENERIC_FACTORY_CONSTRUCTOR(CProgressDialog)

static const nsModuleComponentInfo sAppComps[] = {
	{
		"Prompt Service",
		NS_PROMPTSERVICE_CID,
		NS_PROMPTSERVICE_CONTRACTID,
		CPromptServiceConstructor
	},
	{
		"Helper App Launcher Dialog",
		NS_UNKNOWNCONTENTTYPEHANDLER_CID,
		NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
		CUnknownContentTypeHandlerConstructor
	},
	{
		"Tooltip Text Provider",
		NS_TOOLTIPTEXTPROVIDER_CID,
		NS_TOOLTIPTEXTPROVIDER_CONTRACTID,
		CTooltipTextProviderConstructor
	},
	{
	    "Download",
		NS_DOWNLOAD_CID,
		NS_TRANSFER_CONTRACTID,
		CProgressDialogConstructor
	}
};

#define NB_COMPONENTS sizeof(sAppComps)/sizeof(nsModuleComponentInfo)

nsresult CMfcEmbedApp::OverrideComponents()
{
   nsCOMPtr<nsIComponentRegistrar> compReg;
   nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
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
	}
  	   
   return rv;
}
