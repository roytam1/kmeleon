#define WIN32_LEAN_AND_MEAN
#include <mozilla/Char16.h>
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

CCmdList* cmdList;

NS_GENERIC_FACTORY_CONSTRUCTOR(CJSBridge) 
/*
static const mozilla::Module::CIDEntry kBrowserCIDs[] = {
	{ &kJSBridgeCID,true, NULL, CJSBridgeConstructor },
	{ NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserContracts[] = {
	{"@kmeleon/jsbridge;1", &kJSBridgeCID},
	{ NULL }
};

static const mozilla::Module kBrowserModule = {
    mozilla::Module::kVersion,
    kBrowserCIDs,
    kBrowserContracts
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kBrowserModule;*/

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

int Load()
{
	cmdList = new CCmdList();
	return 1;
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
  	   
	
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

BOOL Quit()
{
	delete cmdList;
	nsresult rv;
	nsCOMPtr<nsIComponentRegistrar> compReg;
	rv = NS_GetComponentRegistrar(getter_AddRefs (compReg));
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	rv = compReg->UnregisterFactory(components.mCID, componentFactory);
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

#include "nsIWebBrowser.h"

WNDPROC KMeleonWndProc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND hWnd)
{
#ifdef _DEBUG
	nsCOMPtr<nsIServiceManager> servMan; 
	nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
	NS_ENSURE_SUCCESS(rv, );

	nsCOMPtr<nsIJSBridge> jsb;
	rv = servMan->GetServiceByContractID("@kmeleon/jsbridge;1",  NS_GET_IID(nsIJSBridge), getter_AddRefs(jsb));
	NS_ENSURE_SUCCESS(rv, );
#endif
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWnd, GWL_WNDPROC);
	SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WndProc);
}

long DoMessage(const char *to, const char *from, const char *subject,
			   long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (strcmp(subject, "Init") == 0) {
			Init();
		}
		else if (strcmp(subject, "Load") == 0) {
			Load();
		}
		else if (strcmp(subject, "Create") == 0) {
			Create((HWND)data1);
		}

		else if (strcmp(subject, "Quit") == 0) {
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

extern std::map<UINT, nsCOMPtr<kmICommandFunction>> cmdMap;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_COMMAND:
		if (cmdList->Run(LOWORD(wParam), lParam)) return 0;
	}
	return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}

