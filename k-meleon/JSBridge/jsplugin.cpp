#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h> 

#define MOZILLA_STRICT_API
#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsISupports.h>
#include <nsIComponentRegistrar.h>
#include <mozilla/ModuleUtils.h>
#include <nsIServiceManager.h>
#include <nsServiceManagerUtils.h>
#include "nsGenericFactory.h"
#include "nsIJSBridge.h"
#include "jscomp.h"

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\KMeleonConst.h"

#define PLUGIN_NAME "Kmeleon JS Bridge"
long DoMessage(const char *, const char *, const char *, long, long);
kmeleonPlugin kPlugin = {
	KMEL_PLUGIN_VER,
	PLUGIN_NAME,
	DoMessage
};


NS_IMPL_ISUPPORTS1(CJSBridge, nsIJSBridge); 
NS_GENERIC_FACTORY_CONSTRUCTOR(CJSBridge) 

static const nsModuleComponentInfo components = 
{
	"Kmeleon JS Bridge",  
	JSBRIDGE_CID, 
	"@kmeleon/jsbridge;1",  
	CJSBridgeConstructor
};

static nsCOMPtr<nsIGenericFactory> componentFactory;
extern NS_COM_GLUE nsresult NS_NewGenericFactory(nsIGenericFactory* *result,
                     const nsModuleComponentInfo *info);
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
  	   
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

BOOL Quit()
{
	nsresult rv;
	nsCOMPtr<nsIComponentRegistrar> compReg;
	rv = NS_GetComponentRegistrar(getter_AddRefs (compReg));
	NS_ENSURE_SUCCESS(rv, FALSE);


	rv = compReg->UnregisterFactory(components.mCID, componentFactory);
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

#include "nsIWebBrowser.h"

void Create(HWND hWnd)
{
	nsCOMPtr<nsIServiceManager> servMan; 
	nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
	NS_ENSURE_SUCCESS(rv, );

	nsCOMPtr<nsIJSBridge> jsb;
	rv = servMan->GetServiceByContractID("@kmeleon/jsbridge;1",  NS_GET_IID(nsIJSBridge), getter_AddRefs(jsb));
	NS_ENSURE_SUCCESS(rv, );
}

long DoMessage(const char *to, const char *from, const char *subject,
			   long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (stricmp(subject, "Init") == 0) {
			Init();
		}
#ifdef _DEBUG
		else if (stricmp(subject, "Create") == 0) {
			Create((HWND)data1);
		}
#endif
		else if (stricmp(subject, "Quit") == 0) {
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

