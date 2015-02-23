#define WIN32_LEAN_AND_MEAN
#include <mozilla/Char16.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <nsIDOMWindow.h>
#include <nsIWindowWatcher.h>
#include <nsServiceManagerUtils.h>
#include <nsIWebBrowserChrome.h>
#include <nsIEmbeddingSiteWindow.h>
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsMemory.h>
#include <jsapi.h>

#include "jscomp.h"
#include "..\kmeleon_plugin.h"

class CWindowEnumerator : public nsISimpleEnumerator {

	HWND* list;
	int count;
	int pos;

public:
	CWindowEnumerator(HWND* hWnd, int count)
	{
		list = hWnd;
		count = count;

	}

	virtual ~CWindowEnumerator()
	{
		if (list) delete [] list;
	}

	NS_IMETHOD GetNext(nsISupports **retval) {
		
	}

	NS_IMETHOD HasMoreElements(bool *retval) {
		*retval = !(pos >= count);
		return NS_OK;
	}

	NS_DECL_ISUPPORTS

};

extern kmeleonPlugin kPlugin;
extern CCmdList* cmdList;
CCmdList* GetCmdList() {
	if (!cmdList) cmdList = new CCmdList();
	return cmdList;
}

bool CCmdList::Run(HWND hwnd, UINT command, UINT mode) {
	nsCOMPtr<nsIWebBrowser> browser;
	kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(browser));
	nsCOMPtr<nsIDOMWindow> dom;
	browser->GetContentDOMWindow(getter_AddRefs(dom));
	auto iter = cmdMap.find(command);
	if (iter != cmdMap.end() && iter->second) {
		iter->second->OnCommand(dom, mode, nullptr);
		return true;
	}
	return false;
}

NS_IMPL_ISUPPORTS (CJSCommand, kmICommand)
NS_IMETHODIMP CJSCommand::GetName(char * *aName)
{
	*aName = (char*)nsMemory::Clone(name.BeginReading(), name.Length()+1);
    return NS_OK;
}
NS_IMETHODIMP CJSCommand::GetDesc(char * *aDesc)
{
	*aDesc = (char*)nsMemory::Clone(desc.BeginReading(), desc.Length()+1);
    return NS_OK;
}
NS_IMETHODIMP CJSCommand::GetCommand(kmICommandFunction * *aCommand)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute string image; */
NS_IMETHODIMP CJSCommand::GetImage(char * *aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CJSCommand::SetImage(const char * aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMPL_ISUPPORTS (CJSButton, kmIButton)

/* attribute string image; */
NS_IMETHODIMP CJSButton::GetImage(char * *aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CJSButton::SetImage(const char * aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute bool checked; */
NS_IMETHODIMP CJSButton::GetChecked(bool *aChecked)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CJSButton::SetChecked(bool aChecked)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute bool disabled; */
NS_IMETHODIMP CJSButton::GetDisabled(bool *aDisabled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP CJSButton::SetDisabled(bool aDisabled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMPL_ISUPPORTS (CJSBridge, nsIJSBridge)

NS_IMETHODIMP CJSBridge::SetMenuCallback(const char *menu, const char *label, kmICommandFunction *command, const char *before)
{
	kmeleonMenuItem item;
	item.label = label;
	item.type = MENUTYPE::MENU_COMMAND;

	//JSFunction* function;
    //if (JS_ConvertArguments(cx, argc, JS_ARGV(cx, command), "f", &function)) {}
	//JS_CallFunctionValue(cx, JS_GetGlobalObject(cx), *(JS_ARGV(cx, command)), 0, NULL, &retVal);
	//JS_GetStringCharsZ(ctx->GetNativeContext(), command.toString()
	item.command = kPlugin.kFuncs->GetCommandIDs(1);
	::GetCmdList()->Add(item.command, command);

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

NS_IMETHODIMP CJSBridge::SendMessage(const char *to, const char *from, const char *subject, const char *data1, int32_t *data2)
{
	if (!kPlugin.kFuncs) 
		return NS_ERROR_NOT_INITIALIZED;
	*data2 = 0;
	kPlugin.kFuncs->SendMessage(to, from, subject, (long)(void*)data1, (long)data2);
    return NS_OK;
}

/* void GetCmdList (out unsigned long length, [array, size_is (length), retval] out kICmdList list); */
NS_IMETHODIMP CJSBridge::GetCmdList(PRUint32 *length, kmICommand ***list)
{
	unsigned size = kPlugin.kFuncs->GetCmdList(nullptr, 0);
	kmeleonCommand* kcs = new kmeleonCommand[size];
	size = kPlugin.kFuncs->GetCmdList(kcs, size);
	kmICommand** cmds = static_cast<kmICommand**>(NS_Alloc(size*sizeof(kmICommand*)));
	for (unsigned i = 0;i<size;i++) {
		CJSCommand* cmd = new CJSCommand();
		cmd->name = kcs[i].cmd;
		cmd->desc = kcs[i].desc;
		void *result;
		cmd->QueryInterface(NS_GET_TEMPLATE_IID(kmICommand), &result);
		cmds[i] = static_cast<kmICommand*>(result);
	}
	delete [] kcs;
	if (length) *length = size;
	*list = cmds;
    return NS_OK;
}

nsresult GetPathAndRect(JSContext* cx, JS::HandleValue& icon, char** path, RECT* rect)
{
	*path = nullptr;
	rect->bottom = rect->top = rect->left = rect->right = 0;
	if (icon.isObject()) {
		JS::RootedObject obj(cx);
		obj = icon.toObjectOrNull();
		JS::Rooted<JS::Value> vpath(cx);
		JS::Rooted<JS::Value> vt(cx);
		JS::Rooted<JS::Value> vb(cx);
		JS::Rooted<JS::Value> vr(cx);
		JS::Rooted<JS::Value> vl(cx);
		
		if (!JS_GetProperty(cx, obj, "path", &vpath))
			return NS_ERROR_INVALID_ARG;
		if (!JS_GetProperty(cx, obj, "top", &vt))
			return NS_ERROR_INVALID_ARG;
		if (!JS_GetProperty(cx, obj, "bottom", &vb))
			return NS_ERROR_INVALID_ARG;
		if (!JS_GetProperty(cx, obj, "left", &vl))
			return NS_ERROR_INVALID_ARG;
		if (!JS_GetProperty(cx, obj, "right", &vr))
			return NS_ERROR_INVALID_ARG;

		RECT r = {vl.toInt32(), vt.toInt32(), vr.toInt32(), vb.toInt32()};	
		*rect = r;
		*path = JS_EncodeString(cx, vpath.toString());	
	} else {
		if (icon.isString())
			*path = JS_EncodeString(cx, icon.toString());
	}
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::RegisterCmd(const char * name, const char * desc, 
	kmICommandFunction *command, JS::HandleValue icon,
	kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval)
{
	if (!kPlugin.kFuncs) return NS_ERROR_NOT_INITIALIZED;
	
	UINT id;
	RECT rect;
	char* iconPath = nullptr;

	nsresult rv = GetPathAndRect(cx, icon, &iconPath, &rect);
	NS_ENSURE_SUCCESS(rv, rv);
	
	if (rect.bottom != 0 || rect.right != 0) {
		id = kPlugin.kFuncs->RegisterCmd(name, desc, nullptr);	
		kPlugin.kFuncs->SetCmdIcon(name, iconPath, &rect, nullptr, nullptr, nullptr, nullptr);	
	} else
		id = kPlugin.kFuncs->RegisterCmd(name, desc, iconPath);
	
	::GetCmdList()->Add(id, command);
	if (iconPath) JS_free(cx, iconPath);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx)
{
	RECT rect;
	char* iconPath = nullptr;
	nsresult rv = GetPathAndRect(cx, icon, &iconPath, &rect);
	kPlugin.kFuncs->SetCmdIcon(name, iconPath, &rect, nullptr, nullptr, nullptr, nullptr);	
	if (iconPath) JS_free(cx, iconPath);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::AddButton(const char * name, const char * command, const char * menu, const char* tooltip)
{
	kPlugin.kFuncs->AddButton(name, command, menu, tooltip);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::RemoveButton(const char * name, const char * command)
{
	kPlugin.kFuncs->RemoveButton(name, command);
	return NS_OK;
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

NS_IMETHODIMP CJSBridge::SetAccel(const char * key, const char * command) 
{
	kPlugin.kFuncs->SetAccel(key, command);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval)
{
	CJSButton* button = new CJSButton();
	nsresult rv;
	void *result;
	rv = button->QueryInterface(NS_GET_TEMPLATE_IID(kmIButton), &result);
	*_retval = (kmIButton*)result;
    return rv;
}

NS_IMETHODIMP CJSBridge::CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) 
{
	CJSButton* button = new CJSButton();
	nsresult rv;
	void *result;
	rv = button->QueryInterface(NS_GET_TEMPLATE_IID(kmIButton), &result);
	*_retval = (kmIButton*)result;

	button->id = kPlugin.kFuncs->GetCommandIDs(1);
	::GetCmdList()->Add(button->id, command);
    return rv;
}

class CWin : public kmIWindow {
public: 
	NS_DECL_ISUPPORTS 
	NS_DECL_KMIWINDOW

	CWin() {};
	virtual ~CWin() {};
	HWND hWnd;
};

NS_IMPL_ISUPPORTS (CWin, kmIWindow)

NS_IMETHODIMP CWin::GetHandle(void **aHandle) {
	*aHandle = hWnd;
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::GetWindows(uint32_t *length, kmIWindow * **list)
{
	*list = nullptr;
	*length = 0;
	int size = kPlugin.kFuncs->GetWindowsList(NULL, 0);
	if (size == 0) return NS_OK;

	HWND* hList = new HWND[size];
	kPlugin.kFuncs->GetWindowsList(hList, size);
	kmIWindow** wins = static_cast<kmIWindow**>(NS_Alloc(size*sizeof(kmIWindow*)));
	for (unsigned i = 0;i<size;i++) {
		CWin* win = new CWin();
		win->hWnd = hList[i];
		void *result;
		win->QueryInterface(NS_GET_TEMPLATE_IID(kmIWindow), &result);
		wins[i] = static_cast<kmIWindow*>(result);
	}
	delete [] hList;
	if (length) *length = size;
	*list = wins;
    return NS_OK;
}

NS_IMETHODIMP CJSBridge::AddListener(nsIObserver* listener)
{
	NS_ENSURE_ARG_POINTER(listener);
	mListeners.AppendObject(listener);
	return NS_OK;
}

NS_IMETHODIMP CJSBridge::RemoveListener(nsIObserver* listener)
{
	NS_ENSURE_ARG_POINTER(listener);
	mListeners.RemoveObject(listener);
	return NS_OK;
}

bool notifyOpenWindow(nsIObserver *aListener, void* aData)
{
	kmIWindow* winData = static_cast<kmIWindow*>(aData);
	aListener->Observe(winData, "kmeleon-createwindow", nullptr);
	return true;
}

void CJSBridge::OnCreateWindow(HWND hWnd, int flag)
{
	CWin* win = new CWin();
	win->hWnd = hWnd;
	void *result;
	win->QueryInterface(NS_GET_TEMPLATE_IID(kmIWindow), &result);
	mListeners.EnumerateForwards(notifyOpenWindow, result);
}

bool notifySwitchTab(nsIObserver *aListener, void* aData)
{
	nsIDOMWindow* dom = static_cast<nsIDOMWindow*>(aData);
	aListener->Observe(dom, "kmeleon-switchtab", nullptr);
	return true;
}

void CJSBridge::OnSwitchTab(HWND oldhWnd, HWND newhWnd)
{
	nsCOMPtr<nsIWebBrowser> browser;
	kPlugin.kFuncs->GetMozillaWebBrowser(newhWnd, getter_AddRefs(browser));
	if (!browser) return;

	nsCOMPtr<nsIDOMWindow> dom;
	browser->GetContentDOMWindow(getter_AddRefs(dom));
	if (!dom) return;

	mListeners.EnumerateForwards(notifySwitchTab, dom);
}