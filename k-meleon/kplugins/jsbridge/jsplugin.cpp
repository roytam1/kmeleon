#define WIN32_LEAN_AND_MEAN
#include "mozilla-config.h"
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

#include "mozilla/ChaosMode.h" // ChaosMode hack

#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"
#include "../../app/KMeleonConst.h"

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
extern nsresult NS_NewGenericFactory(nsIGenericFactory* *result,
                     const nsModuleComponentInfo *info);

int Load()
{
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

static nsCOMPtr<CJSBridge> gJSB = nullptr;

BOOL Quit()
{
	if (cmdList) delete cmdList;
	nsresult rv;
	nsCOMPtr<nsIComponentRegistrar> compReg;
	rv = NS_GetComponentRegistrar(getter_AddRefs (compReg));
	NS_ENSURE_SUCCESS(rv, FALSE);
	gJSB = nullptr;

	rv = compReg->UnregisterFactory(components.mCID, componentFactory);
	return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

#include "nsIWebBrowser.h"

WNDPROC KMeleonWndProc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



CJSBridge* getJSB() {
	if (!gJSB) {
		nsCOMPtr<nsIServiceManager> servMan; 
		nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan)); 
		NS_ENSURE_SUCCESS(rv, nullptr);

		nsCOMPtr<nsIJSBridge> jsb;
		rv = servMan->GetServiceByContractID("@kmeleon/jsbridge;1",  NS_GET_IID(nsIJSBridge), getter_AddRefs(jsb));
		NS_ENSURE_SUCCESS(rv, nullptr);

		gJSB = reinterpret_cast<CJSBridge*>(jsb.get());
	}
	return gJSB;
}

void Create(HWND hWnd)
{
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWnd, GWL_WNDPROC);
	SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WndProc);
	if (getJSB()) getJSB()->OnCreateWindow(hWnd, 0);
}

long DoMessage(const char *to, const char *from, const char *subject,
			   long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (strcmp(subject, "GetState") == 0) {
			if (!cmdList) return 0;
			unsigned res = cmdList->GetState(data1);
			if (res == -1) return 0;
			*((unsigned*)data2) = res;
			return 1;
		}
		else if (strcmp(subject, "Init") == 0) {
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

		else if (strcmp(subject, "SwitchTab") == 0) {
			if (getJSB()) getJSB()->OnSwitchTab((HWND)data1, (HWND)data2);
		}	
		else if (strcmp(subject, "CreateTab") == 0) {
			if (getJSB()) getJSB()->OnCreateTab((HWND)data2);
		}
		else if (strcmp(subject, "DestroyTab") == 0) {
			if (getJSB()) getJSB()->OnDestroyTab((HWND)data2);
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
		if (cmdList && cmdList->Run(hWnd, LOWORD(wParam), 0)) return 0;
		break;
	case TB_MBUTTONDOWN:
		if (cmdList && cmdList->Run(hWnd, LOWORD(wParam), 2)) return 0;
		break;

	case WM_INITMENUPOPUP: {
		if (!cmdList) break;
		// Let MFC do its little update
		LRESULT res = CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
		
		HMENU menu = (HMENU)wParam;
		int count = GetMenuItemCount(menu);
		for (int i=0;i<count;i++)
		{
			int id = GetMenuItemID(menu, i);
			if (!id>0 || !cmdList->Get(id)) continue;

			MENUITEMINFO mif;
			mif.cbSize = sizeof(mif);
			mif.fMask = MIIM_STATE;
			GetMenuItemInfo(menu, i, TRUE, &mif);

			int state = cmdList->GetChecked(id);
			if (state != -1) {
				mif.fState &= ~MF_CHECKED & ~MF_UNCHECKED;
				mif.fState |= state ? MF_CHECKED : MF_UNCHECKED;
			}

			state = cmdList->GetEnabled(id);
			if (state != -1) {
				mif.fState &= ~MF_GRAYED & ~MF_ENABLED;
				mif.fState |= state ? MF_ENABLED : MF_GRAYED;
			}

			::SetMenuItemInfo(menu, i, TRUE, &mif);
		}
		return res;
	}
	/*case TB_RBUTTONDOWN:
		if (cmdList && cmdList->Run(hWnd, LOWORD(wParam), 2)) return 0;
		break;*/
   }
	return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

/*
static const mozilla::Module::CIDEntry kCIDs[] = {
	{ &kJSBridgeCID,true, NULL, CJSBridgeConstructor },
	{ NULL }
};

static const mozilla::Module::ContractIDEntry kContracts[] = {
	{"@kmeleon/jsbridge;1", &kJSBridgeCID},
	{ NULL }
};

static const mozilla::Module::CategoryEntry kCategory[] = {
	{ NULL }
};


static const mozilla::Module kModule = {
    mozilla::Module::kVersion,
    kCIDs,
    kContracts,
	kCategory
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kModule;*/

extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}

#if 1 //ChaosMode hack
namespace mozilla {
namespace detail {

Atomic<uint32_t> gChaosModeCounter(0);

} /* namespace detail */
} /* namespace mozilla */
#endif
