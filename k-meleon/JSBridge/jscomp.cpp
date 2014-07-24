#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <nsIDOMWindow.h>
#include <nsIWindowWatcher.h>
#include <nsServiceManagerUtils.h>
#include <nsIWebBrowserChrome.h>
#include <nsIEmbeddingSiteWindow.h>

#include "jscomp.h"
#include "..\kmeleon_plugin.h"

extern kmeleonPlugin kPlugin;
extern CCmdList* cmdList;

NS_IMETHODIMP CJSBridge::SetMenuCallback(const char *menu, const char *label, nsICommandFunction *command, const char *before)
{
	kmeleonMenuItem item;
	item.label = label;
	item.type = MENUTYPE::MENU_COMMAND;

	//JSFunction* function;
    //if (JS_ConvertArguments(cx, argc, JS_ARGV(cx, command), "f", &function)) {}
	//JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(JS_ARGV(cx, command)), 0, NULL, &retVal);
	//JS_GetStringCharsZ(ctx->GetNativeContext(), command.toString()
	item.command = kPlugin.kFuncs->GetCommandIDs(1);
	cmdList->Add(item.command, command);

	if (before && *before) {
		item.before = atoi(before);
		if (!item.before && strcmp(before, "0") != 0) {
			item.before = kPlugin.kFuncs->GetID(before);
			if (!item.before) 
				item.before = (long)before;
		}
	}
	else item.before = -1;
	kPlugin.kFuncs->SetMenu(menu, &item);
	
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::SetMenu(const char *menu, PRUint16 type, const char *label, const char *command, const char *before)
{
	kmeleonMenuItem item;
	item.label = label;
	item.type = type - 1;

	item.command = *command ? kPlugin.kFuncs->GetID(command) : 1;
	if (before && *before) {
		item.before = atoi(before);
		if (!item.before && strcmp(before, "0") != 0) {
			item.before = kPlugin.kFuncs->GetID(before);
			if (!item.before) 
				item.before = (long)before;
		}
	}
	else item.before = -1;
	kPlugin.kFuncs->SetMenu(menu, &item);
	
	return NS_OK;
}

/* void RebuildMenu (in string menu); */
NS_IMETHODIMP CJSBridge::RebuildMenu(const char *menu)
{
	kPlugin.kFuncs->RebuildMenu(menu);
	return NS_OK;
}

/* void id (in string id); */
NS_IMETHODIMP CJSBridge::Id(nsIDOMWindow *window, const char *id)
{
	nsCOMPtr<nsIWindowWatcher> mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
	NS_ENSURE_TRUE (mWWatch, NS_ERROR_FAILURE);

	nsCOMPtr<nsIWebBrowserChrome> chrome;
	HWND hWin = NULL;

	if (mWWatch) {
		nsCOMPtr<nsIDOMWindow> fosterParent;
		if (!window) { // it will be a dependent window. try to find a foster parent.
			mWWatch->GetActiveWindow(getter_AddRefs(fosterParent));
			window = fosterParent;
		}
		mWWatch->GetChromeForWindow(window, getter_AddRefs(chrome));
	}

	if (chrome) {
		nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
		if (site)
			site->GetSiteWindow(reinterpret_cast<void **>(&hWin));
	}
	
	NS_ENSURE_TRUE(hWin, NS_ERROR_FAILURE);

	::SendMessage(hWin, WM_COMMAND, MAKELONG(kPlugin.kFuncs->GetID(id), 1), (LPARAM)NULL);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::SendMessage(const char *plugin, const char *to, const char *from, PRInt32 data1, PRInt32 *data2)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetCmdList (out unsigned long length, [array, size_is (length), retval] out kICmdList list); */
NS_IMETHODIMP CJSBridge::GetCmdList(PRUint32 *length, kICmdList ***list)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CJSBridge::Open(const char * url, uint16_t state, nsIWebBrowser * *_retval)
{
	HWND h = kPlugin.kFuncs->NavigateTo(url, state, nullptr);
	kPlugin.kFuncs->GetMozillaWebBrowser(h, _retval);
    return NS_OK;
}

NS_IMETHODIMP CJSBridge::GetActiveBrowser(nsIWebBrowser * *_retval) 
{
	kPlugin.kFuncs->GetMozillaWebBrowser(NULL, _retval);
	return NS_OK;
}

