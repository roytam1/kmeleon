/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM idl/nsIJSBridge.idl
 */

#ifndef __gen_nsIJSBridge_h__
#define __gen_nsIJSBridge_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIDOMWindow_h__
#include "nsIDOMWindow.h"
#endif

#ifndef __gen_nsIWebBrowser_h__
#include "nsIWebBrowser.h"
#endif

#include "js/Value.h"

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsISimpleEnumerator; /* forward declaration */

class nsIArray; /* forward declaration */

class nsIDOMWindow; /* forward declaration */

class nsIObserver; /* forward declaration */


/* starting interface:    kmICommandFunction */
#define KMICOMMANDFUNCTION_IID_STR "83910267-7670-4493-99c1-dd540a036ef3"

#define KMICOMMANDFUNCTION_IID \
  {0x83910267, 0x7670, 0x4493, \
    { 0x99, 0xc1, 0xdd, 0x54, 0x0a, 0x03, 0x6e, 0xf3 }}

class NS_NO_VTABLE kmICommandFunction : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMICOMMANDFUNCTION_IID)

  /* void onCommand (in nsIDOMWindow win, in unsigned long mode, [optional] in string arg); */
  NS_IMETHOD OnCommand(nsIDOMWindow *win, uint32_t mode, const char * arg) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmICommandFunction, KMICOMMANDFUNCTION_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMICOMMANDFUNCTION \
  NS_IMETHOD OnCommand(nsIDOMWindow *win, uint32_t mode, const char * arg) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMICOMMANDFUNCTION(_to) \
  NS_IMETHOD OnCommand(nsIDOMWindow *win, uint32_t mode, const char * arg) override { return _to OnCommand(win, mode, arg); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMICOMMANDFUNCTION(_to) \
  NS_IMETHOD OnCommand(nsIDOMWindow *win, uint32_t mode, const char * arg) override { return !_to ? NS_ERROR_NULL_POINTER : _to->OnCommand(win, mode, arg); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmCommandFunction : public kmICommandFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMICOMMANDFUNCTION

  kmCommandFunction();

private:
  ~kmCommandFunction();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmCommandFunction, kmICommandFunction)

kmCommandFunction::kmCommandFunction()
{
  /* member initializers and constructor code */
}

kmCommandFunction::~kmCommandFunction()
{
  /* destructor code */
}

/* void onCommand (in nsIDOMWindow win, in unsigned long mode, [optional] in string arg); */
NS_IMETHODIMP kmCommandFunction::OnCommand(nsIDOMWindow *win, uint32_t mode, const char * arg)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    kmICallback */
#define KMICALLBACK_IID_STR "83910267-7670-4493-99c1-dd540a036ef4"

#define KMICALLBACK_IID \
  {0x83910267, 0x7670, 0x4493, \
    { 0x99, 0xc1, 0xdd, 0x54, 0x0a, 0x03, 0x6e, 0xf4 }}

class NS_NO_VTABLE kmICallback : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMICALLBACK_IID)

  /* bool run ([optional] in string arg); */
  NS_IMETHOD Run(const char * arg, bool *_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmICallback, KMICALLBACK_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMICALLBACK \
  NS_IMETHOD Run(const char * arg, bool *_retval) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMICALLBACK(_to) \
  NS_IMETHOD Run(const char * arg, bool *_retval) override { return _to Run(arg, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMICALLBACK(_to) \
  NS_IMETHOD Run(const char * arg, bool *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->Run(arg, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmCallback : public kmICallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMICALLBACK

  kmCallback();

private:
  ~kmCallback();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmCallback, kmICallback)

kmCallback::kmCallback()
{
  /* member initializers and constructor code */
}

kmCallback::~kmCallback()
{
  /* destructor code */
}

/* bool run ([optional] in string arg); */
NS_IMETHODIMP kmCallback::Run(const char * arg, bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    kmIWindow */
#define KMIWINDOW_IID_STR "83910267-7670-4493-99c1-dd540a036ef5"

#define KMIWINDOW_IID \
  {0x83910267, 0x7670, 0x4493, \
    { 0x99, 0xc1, 0xdd, 0x54, 0x0a, 0x03, 0x6e, 0xf5 }}

class NS_NO_VTABLE kmIWindow : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMIWINDOW_IID)

  /* readonly attribute voidPtr handle; */
  NS_IMETHOD GetHandle(void **aHandle) = 0;

  /* void getTabs ([optional] out unsigned long length, [array, size_is (length), retval] out nsIWebBrowser list); */
  NS_IMETHOD GetTabs(uint32_t *length, nsIWebBrowser * **list) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmIWindow, KMIWINDOW_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMIWINDOW \
  NS_IMETHOD GetHandle(void **aHandle) override; \
  NS_IMETHOD GetTabs(uint32_t *length, nsIWebBrowser * **list) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMIWINDOW(_to) \
  NS_IMETHOD GetHandle(void **aHandle) override { return _to GetHandle(aHandle); } \
  NS_IMETHOD GetTabs(uint32_t *length, nsIWebBrowser * **list) override { return _to GetTabs(length, list); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMIWINDOW(_to) \
  NS_IMETHOD GetHandle(void **aHandle) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetHandle(aHandle); } \
  NS_IMETHOD GetTabs(uint32_t *length, nsIWebBrowser * **list) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTabs(length, list); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmWindow : public kmIWindow
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMIWINDOW

  kmWindow();

private:
  ~kmWindow();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmWindow, kmIWindow)

kmWindow::kmWindow()
{
  /* member initializers and constructor code */
}

kmWindow::~kmWindow()
{
  /* destructor code */
}

/* readonly attribute voidPtr handle; */
NS_IMETHODIMP kmWindow::GetHandle(void **aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getTabs ([optional] out unsigned long length, [array, size_is (length), retval] out nsIWebBrowser list); */
NS_IMETHODIMP kmWindow::GetTabs(uint32_t *length, nsIWebBrowser * **list)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    kmITab */
#define KMITAB_IID_STR "83910267-7670-4493-99c1-dd540a036ef6"

#define KMITAB_IID \
  {0x83910267, 0x7670, 0x4493, \
    { 0x99, 0xc1, 0xdd, 0x54, 0x0a, 0x03, 0x6e, 0xf6 }}

class NS_NO_VTABLE kmITab : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMITAB_IID)

  /* readonly attribute voidPtr handle; */
  NS_IMETHOD GetHandle(void **aHandle) = 0;

  /* readonly attribute nsIWebBrowser browser; */
  NS_IMETHOD GetBrowser(nsIWebBrowser * *aBrowser) = 0;

  /* readonly attribute nsIDOMEventTarget root; */
  NS_IMETHOD GetRoot(nsIDOMEventTarget * *aRoot) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmITab, KMITAB_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMITAB \
  NS_IMETHOD GetHandle(void **aHandle) override; \
  NS_IMETHOD GetBrowser(nsIWebBrowser * *aBrowser) override; \
  NS_IMETHOD GetRoot(nsIDOMEventTarget * *aRoot) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMITAB(_to) \
  NS_IMETHOD GetHandle(void **aHandle) override { return _to GetHandle(aHandle); } \
  NS_IMETHOD GetBrowser(nsIWebBrowser * *aBrowser) override { return _to GetBrowser(aBrowser); } \
  NS_IMETHOD GetRoot(nsIDOMEventTarget * *aRoot) override { return _to GetRoot(aRoot); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMITAB(_to) \
  NS_IMETHOD GetHandle(void **aHandle) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetHandle(aHandle); } \
  NS_IMETHOD GetBrowser(nsIWebBrowser * *aBrowser) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBrowser(aBrowser); } \
  NS_IMETHOD GetRoot(nsIDOMEventTarget * *aRoot) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetRoot(aRoot); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmTab : public kmITab
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMITAB

  kmTab();

private:
  ~kmTab();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmTab, kmITab)

kmTab::kmTab()
{
  /* member initializers and constructor code */
}

kmTab::~kmTab()
{
  /* destructor code */
}

/* readonly attribute voidPtr handle; */
NS_IMETHODIMP kmTab::GetHandle(void **aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIWebBrowser browser; */
NS_IMETHODIMP kmTab::GetBrowser(nsIWebBrowser * *aBrowser)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMEventTarget root; */
NS_IMETHODIMP kmTab::GetRoot(nsIDOMEventTarget * *aRoot)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    kmICommand */
#define KMICOMMAND_IID_STR "3d8ce8f0-5214-11db-b0de-0800200c9a65"

#define KMICOMMAND_IID \
  {0x3d8ce8f0, 0x5214, 0x11db, \
    { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x65 }}

class NS_NO_VTABLE kmICommand : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMICOMMAND_IID)

  /* readonly attribute AUTF8String name; */
  NS_IMETHOD GetName(nsACString & aName) = 0;

  /* readonly attribute AUTF8String desc; */
  NS_IMETHOD GetDesc(nsACString & aDesc) = 0;

  /* readonly attribute AUTF8String accel; */
  NS_IMETHOD GetAccel(nsACString & aAccel) = 0;

  /* readonly attribute kmICommandFunction command; */
  NS_IMETHOD GetCommand(kmICommandFunction * *aCommand) = 0;

  /* attribute AUTF8String image; */
  NS_IMETHOD GetImage(nsACString & aImage) = 0;
  NS_IMETHOD SetImage(const nsACString & aImage) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmICommand, KMICOMMAND_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMICOMMAND \
  NS_IMETHOD GetName(nsACString & aName) override; \
  NS_IMETHOD GetDesc(nsACString & aDesc) override; \
  NS_IMETHOD GetAccel(nsACString & aAccel) override; \
  NS_IMETHOD GetCommand(kmICommandFunction * *aCommand) override; \
  NS_IMETHOD GetImage(nsACString & aImage) override; \
  NS_IMETHOD SetImage(const nsACString & aImage) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMICOMMAND(_to) \
  NS_IMETHOD GetName(nsACString & aName) override { return _to GetName(aName); } \
  NS_IMETHOD GetDesc(nsACString & aDesc) override { return _to GetDesc(aDesc); } \
  NS_IMETHOD GetAccel(nsACString & aAccel) override { return _to GetAccel(aAccel); } \
  NS_IMETHOD GetCommand(kmICommandFunction * *aCommand) override { return _to GetCommand(aCommand); } \
  NS_IMETHOD GetImage(nsACString & aImage) override { return _to GetImage(aImage); } \
  NS_IMETHOD SetImage(const nsACString & aImage) override { return _to SetImage(aImage); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMICOMMAND(_to) \
  NS_IMETHOD GetName(nsACString & aName) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_IMETHOD GetDesc(nsACString & aDesc) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDesc(aDesc); } \
  NS_IMETHOD GetAccel(nsACString & aAccel) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAccel(aAccel); } \
  NS_IMETHOD GetCommand(kmICommandFunction * *aCommand) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCommand(aCommand); } \
  NS_IMETHOD GetImage(nsACString & aImage) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetImage(aImage); } \
  NS_IMETHOD SetImage(const nsACString & aImage) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetImage(aImage); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmCommand : public kmICommand
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMICOMMAND

  kmCommand();

private:
  ~kmCommand();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmCommand, kmICommand)

kmCommand::kmCommand()
{
  /* member initializers and constructor code */
}

kmCommand::~kmCommand()
{
  /* destructor code */
}

/* readonly attribute AUTF8String name; */
NS_IMETHODIMP kmCommand::GetName(nsACString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String desc; */
NS_IMETHODIMP kmCommand::GetDesc(nsACString & aDesc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AUTF8String accel; */
NS_IMETHODIMP kmCommand::GetAccel(nsACString & aAccel)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute kmICommandFunction command; */
NS_IMETHODIMP kmCommand::GetCommand(kmICommandFunction * *aCommand)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute AUTF8String image; */
NS_IMETHODIMP kmCommand::GetImage(nsACString & aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP kmCommand::SetImage(const nsACString & aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    kmIButton */
#define KMIBUTTON_IID_STR "3d8ce8f0-5214-11db-b0de-0800200c9a67"

#define KMIBUTTON_IID \
  {0x3d8ce8f0, 0x5214, 0x11db, \
    { 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x67 }}

class NS_NO_VTABLE kmIButton : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMIBUTTON_IID)

  /* attribute string image; */
  NS_IMETHOD GetImage(char * *aImage) = 0;
  NS_IMETHOD SetImage(const char * aImage) = 0;

  /* attribute bool checked; */
  NS_IMETHOD GetChecked(bool *aChecked) = 0;
  NS_IMETHOD SetChecked(bool aChecked) = 0;

  /* attribute bool disabled; */
  NS_IMETHOD GetDisabled(bool *aDisabled) = 0;
  NS_IMETHOD SetDisabled(bool aDisabled) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmIButton, KMIBUTTON_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMIBUTTON \
  NS_IMETHOD GetImage(char * *aImage) override; \
  NS_IMETHOD SetImage(const char * aImage) override; \
  NS_IMETHOD GetChecked(bool *aChecked) override; \
  NS_IMETHOD SetChecked(bool aChecked) override; \
  NS_IMETHOD GetDisabled(bool *aDisabled) override; \
  NS_IMETHOD SetDisabled(bool aDisabled) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMIBUTTON(_to) \
  NS_IMETHOD GetImage(char * *aImage) override { return _to GetImage(aImage); } \
  NS_IMETHOD SetImage(const char * aImage) override { return _to SetImage(aImage); } \
  NS_IMETHOD GetChecked(bool *aChecked) override { return _to GetChecked(aChecked); } \
  NS_IMETHOD SetChecked(bool aChecked) override { return _to SetChecked(aChecked); } \
  NS_IMETHOD GetDisabled(bool *aDisabled) override { return _to GetDisabled(aDisabled); } \
  NS_IMETHOD SetDisabled(bool aDisabled) override { return _to SetDisabled(aDisabled); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMIBUTTON(_to) \
  NS_IMETHOD GetImage(char * *aImage) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetImage(aImage); } \
  NS_IMETHOD SetImage(const char * aImage) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetImage(aImage); } \
  NS_IMETHOD GetChecked(bool *aChecked) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetChecked(aChecked); } \
  NS_IMETHOD SetChecked(bool aChecked) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetChecked(aChecked); } \
  NS_IMETHOD GetDisabled(bool *aDisabled) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDisabled(aDisabled); } \
  NS_IMETHOD SetDisabled(bool aDisabled) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDisabled(aDisabled); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmButton : public kmIButton
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMIBUTTON

  kmButton();

private:
  ~kmButton();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(kmButton, kmIButton)

kmButton::kmButton()
{
  /* member initializers and constructor code */
}

kmButton::~kmButton()
{
  /* destructor code */
}

/* attribute string image; */
NS_IMETHODIMP kmButton::GetImage(char * *aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP kmButton::SetImage(const char * aImage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute bool checked; */
NS_IMETHODIMP kmButton::GetChecked(bool *aChecked)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP kmButton::SetChecked(bool aChecked)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute bool disabled; */
NS_IMETHODIMP kmButton::GetDisabled(bool *aDisabled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP kmButton::SetDisabled(bool aDisabled)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIJSBridge */
#define NS_IJSBRIDGE_IID_STR "83910267-7670-4493-99c1-dd540a036ef2"

#define NS_IJSBRIDGE_IID \
  {0x83910267, 0x7670, 0x4493, \
    { 0x99, 0xc1, 0xdd, 0x54, 0x0a, 0x03, 0x6e, 0xf2 }}

class NS_NO_VTABLE nsIJSBridge : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSBRIDGE_IID)

  enum {
    MENU_COMMAND = 1U,
    MENU_POPUP = 2U,
    MENU_INLINE = 3U,
    MENU_PLUGIN = 4U,
    MENU_SEPARATOR = 5U,
    OPEN_NORMAL = 0U,
    OPEN_NEW = 1U,
    OPEN_BACKGROUND = 2U,
    OPEN_NEWTAB = 3U,
    OPEN_BACKGROUNDTAB = 4U,
    OPEN_CLONE = 16U
  };

  /* void SetMenuCallback (in string menu, in string label, in kmICommandFunction command, [optional] in string before); */
  NS_IMETHOD SetMenuCallback(const char * menu, const char * label, kmICommandFunction *command, const char * before) = 0;

  /* void SetMenu (in string menu, in unsigned short type, in string label, in string command, [optional] in string before); */
  NS_IMETHOD SetMenu(const char * menu, uint16_t type, const char * label, const char * command, const char * before) = 0;

  /* void RebuildMenu (in string menu); */
  NS_IMETHOD RebuildMenu(const char * menu) = 0;

  /* kmIButton CreateButton (in string cmd, [optional] in string menu, [optional] in string tooltip, [optional] in string label); */
  NS_IMETHOD CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) = 0;

  /* kmIButton CreateCallbackButton (in kmICommandFunction command, [optional] in string menu, [optional] in string tooltip, [optional] in string label); */
  NS_IMETHOD CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) = 0;

  /* void AddToolbar (in string toolbar, [optional] in unsigned long width, [optional] in unsigned long height); */
  NS_IMETHOD AddToolbar(const char * toolbar, uint32_t width, uint32_t height) = 0;

  /* void AddButton (in string toolbar, in string command, [optional] in string menu, [optional] in string tooltip); */
  NS_IMETHOD AddButton(const char * toolbar, const char * command, const char * menu, const char * tooltip) = 0;

  /* void RemoveButton (in string toolbar, in string command); */
  NS_IMETHOD RemoveButton(const char * toolbar, const char * command) = 0;

  /* void id (in nsIDOMWindow window, in string id); */
  NS_IMETHOD Id(nsIDOMWindow *window, const char * id) = 0;

  /* long SendMessage (in string plugin, in string to, in string from, in string data1); */
  NS_IMETHOD SendMessage(const char * plugin, const char * to, const char * from, const char * data1, int32_t *_retval) = 0;

  /* void GetCmdList ([optional] out unsigned long length, [array, size_is (length), retval] out kmICommand list); */
  NS_IMETHOD GetCmdList(uint32_t *length, kmICommand * **list) = 0;

  /* [implicit_jscontext] long RegisterCmd (in string name, in string desc, in kmICommandFunction command, [optional] in jsval icon, [optional] in kmICallback enabled, [optional] in kmICallback checked); */
  NS_IMETHOD RegisterCmd(const char * name, const char * desc, kmICommandFunction *command, JS::HandleValue icon, kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval) = 0;

  /* void UnregisterCmd (in string name); */
  NS_IMETHOD UnregisterCmd(const char * name) = 0;

  /* [implicit_jscontext] void SetCmdIcon (in string name, in jsval icon); */
  NS_IMETHOD SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx) = 0;

  /* [implicit_jscontext] void SetButtonIcon (in string toolbar, in string command, in jsval icon); */
  NS_IMETHOD SetButtonIcon(const char * toolbar, const char * command, JS::HandleValue icon, JSContext* cx) = 0;

  /* void SetAccel (in string key, in string command); */
  NS_IMETHOD SetAccel(const char * key, const char * command) = 0;

  /* nsIWebBrowser Open (in string url, in unsigned short state); */
  NS_IMETHOD Open(const char * url, uint16_t state, nsIWebBrowser * *_retval) = 0;

  /* nsIWebBrowser GetActiveBrowser (); */
  NS_IMETHOD GetActiveBrowser(nsIWebBrowser * *_retval) = 0;

  /* kmIWindow GetCurrentWindow (); */
  NS_IMETHOD GetCurrentWindow(kmIWindow * *_retval) = 0;

  /* void GetWindows ([optional] out unsigned long length, [array, size_is (length), retval] out kmIWindow list); */
  NS_IMETHOD GetWindows(uint32_t *length, kmIWindow * **list) = 0;

  /* void AddListener (in nsIObserver listener); */
  NS_IMETHOD AddListener(nsIObserver *listener) = 0;

  /* void RemoveListener (in nsIObserver listener); */
  NS_IMETHOD RemoveListener(nsIObserver *listener) = 0;

  /* void LoadPlugin (in string path); */
  NS_IMETHOD LoadPlugin(const char * path) = 0;

  /* long ShowMenu (in string name, [optional] in bool sendCommand); */
  NS_IMETHOD ShowMenu(const char * name, bool sendCommand, int32_t *_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSBridge, NS_IJSBRIDGE_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIJSBRIDGE \
  NS_IMETHOD SetMenuCallback(const char * menu, const char * label, kmICommandFunction *command, const char * before) override; \
  NS_IMETHOD SetMenu(const char * menu, uint16_t type, const char * label, const char * command, const char * before) override; \
  NS_IMETHOD RebuildMenu(const char * menu) override; \
  NS_IMETHOD CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override; \
  NS_IMETHOD CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override; \
  NS_IMETHOD AddToolbar(const char * toolbar, uint32_t width, uint32_t height) override; \
  NS_IMETHOD AddButton(const char * toolbar, const char * command, const char * menu, const char * tooltip) override; \
  NS_IMETHOD RemoveButton(const char * toolbar, const char * command) override; \
  NS_IMETHOD Id(nsIDOMWindow *window, const char * id) override; \
  NS_IMETHOD SendMessage(const char * plugin, const char * to, const char * from, const char * data1, int32_t *_retval) override; \
  NS_IMETHOD GetCmdList(uint32_t *length, kmICommand * **list) override; \
  NS_IMETHOD RegisterCmd(const char * name, const char * desc, kmICommandFunction *command, JS::HandleValue icon, kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval) override; \
  NS_IMETHOD UnregisterCmd(const char * name) override; \
  NS_IMETHOD SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx) override; \
  NS_IMETHOD SetButtonIcon(const char * toolbar, const char * command, JS::HandleValue icon, JSContext* cx) override; \
  NS_IMETHOD SetAccel(const char * key, const char * command) override; \
  NS_IMETHOD Open(const char * url, uint16_t state, nsIWebBrowser * *_retval) override; \
  NS_IMETHOD GetActiveBrowser(nsIWebBrowser * *_retval) override; \
  NS_IMETHOD GetCurrentWindow(kmIWindow * *_retval) override; \
  NS_IMETHOD GetWindows(uint32_t *length, kmIWindow * **list) override; \
  NS_IMETHOD AddListener(nsIObserver *listener) override; \
  NS_IMETHOD RemoveListener(nsIObserver *listener) override; \
  NS_IMETHOD LoadPlugin(const char * path) override; \
  NS_IMETHOD ShowMenu(const char * name, bool sendCommand, int32_t *_retval) override; 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIJSBRIDGE(_to) \
  NS_IMETHOD SetMenuCallback(const char * menu, const char * label, kmICommandFunction *command, const char * before) override { return _to SetMenuCallback(menu, label, command, before); } \
  NS_IMETHOD SetMenu(const char * menu, uint16_t type, const char * label, const char * command, const char * before) override { return _to SetMenu(menu, type, label, command, before); } \
  NS_IMETHOD RebuildMenu(const char * menu) override { return _to RebuildMenu(menu); } \
  NS_IMETHOD CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override { return _to CreateButton(cmd, menu, tooltip, label, _retval); } \
  NS_IMETHOD CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override { return _to CreateCallbackButton(command, menu, tooltip, label, _retval); } \
  NS_IMETHOD AddToolbar(const char * toolbar, uint32_t width, uint32_t height) override { return _to AddToolbar(toolbar, width, height); } \
  NS_IMETHOD AddButton(const char * toolbar, const char * command, const char * menu, const char * tooltip) override { return _to AddButton(toolbar, command, menu, tooltip); } \
  NS_IMETHOD RemoveButton(const char * toolbar, const char * command) override { return _to RemoveButton(toolbar, command); } \
  NS_IMETHOD Id(nsIDOMWindow *window, const char * id) override { return _to Id(window, id); } \
  NS_IMETHOD SendMessage(const char * plugin, const char * to, const char * from, const char * data1, int32_t *_retval) override { return _to SendMessage(plugin, to, from, data1, _retval); } \
  NS_IMETHOD GetCmdList(uint32_t *length, kmICommand * **list) override { return _to GetCmdList(length, list); } \
  NS_IMETHOD RegisterCmd(const char * name, const char * desc, kmICommandFunction *command, JS::HandleValue icon, kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval) override { return _to RegisterCmd(name, desc, command, icon, enabled, checked, cx, _retval); } \
  NS_IMETHOD UnregisterCmd(const char * name) override { return _to UnregisterCmd(name); } \
  NS_IMETHOD SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx) override { return _to SetCmdIcon(name, icon, cx); } \
  NS_IMETHOD SetButtonIcon(const char * toolbar, const char * command, JS::HandleValue icon, JSContext* cx) override { return _to SetButtonIcon(toolbar, command, icon, cx); } \
  NS_IMETHOD SetAccel(const char * key, const char * command) override { return _to SetAccel(key, command); } \
  NS_IMETHOD Open(const char * url, uint16_t state, nsIWebBrowser * *_retval) override { return _to Open(url, state, _retval); } \
  NS_IMETHOD GetActiveBrowser(nsIWebBrowser * *_retval) override { return _to GetActiveBrowser(_retval); } \
  NS_IMETHOD GetCurrentWindow(kmIWindow * *_retval) override { return _to GetCurrentWindow(_retval); } \
  NS_IMETHOD GetWindows(uint32_t *length, kmIWindow * **list) override { return _to GetWindows(length, list); } \
  NS_IMETHOD AddListener(nsIObserver *listener) override { return _to AddListener(listener); } \
  NS_IMETHOD RemoveListener(nsIObserver *listener) override { return _to RemoveListener(listener); } \
  NS_IMETHOD LoadPlugin(const char * path) override { return _to LoadPlugin(path); } \
  NS_IMETHOD ShowMenu(const char * name, bool sendCommand, int32_t *_retval) override { return _to ShowMenu(name, sendCommand, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIJSBRIDGE(_to) \
  NS_IMETHOD SetMenuCallback(const char * menu, const char * label, kmICommandFunction *command, const char * before) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMenuCallback(menu, label, command, before); } \
  NS_IMETHOD SetMenu(const char * menu, uint16_t type, const char * label, const char * command, const char * before) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMenu(menu, type, label, command, before); } \
  NS_IMETHOD RebuildMenu(const char * menu) override { return !_to ? NS_ERROR_NULL_POINTER : _to->RebuildMenu(menu); } \
  NS_IMETHOD CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateButton(cmd, menu, tooltip, label, _retval); } \
  NS_IMETHOD CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateCallbackButton(command, menu, tooltip, label, _retval); } \
  NS_IMETHOD AddToolbar(const char * toolbar, uint32_t width, uint32_t height) override { return !_to ? NS_ERROR_NULL_POINTER : _to->AddToolbar(toolbar, width, height); } \
  NS_IMETHOD AddButton(const char * toolbar, const char * command, const char * menu, const char * tooltip) override { return !_to ? NS_ERROR_NULL_POINTER : _to->AddButton(toolbar, command, menu, tooltip); } \
  NS_IMETHOD RemoveButton(const char * toolbar, const char * command) override { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveButton(toolbar, command); } \
  NS_IMETHOD Id(nsIDOMWindow *window, const char * id) override { return !_to ? NS_ERROR_NULL_POINTER : _to->Id(window, id); } \
  NS_IMETHOD SendMessage(const char * plugin, const char * to, const char * from, const char * data1, int32_t *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SendMessage(plugin, to, from, data1, _retval); } \
  NS_IMETHOD GetCmdList(uint32_t *length, kmICommand * **list) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCmdList(length, list); } \
  NS_IMETHOD RegisterCmd(const char * name, const char * desc, kmICommandFunction *command, JS::HandleValue icon, kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->RegisterCmd(name, desc, command, icon, enabled, checked, cx, _retval); } \
  NS_IMETHOD UnregisterCmd(const char * name) override { return !_to ? NS_ERROR_NULL_POINTER : _to->UnregisterCmd(name); } \
  NS_IMETHOD SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetCmdIcon(name, icon, cx); } \
  NS_IMETHOD SetButtonIcon(const char * toolbar, const char * command, JS::HandleValue icon, JSContext* cx) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetButtonIcon(toolbar, command, icon, cx); } \
  NS_IMETHOD SetAccel(const char * key, const char * command) override { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAccel(key, command); } \
  NS_IMETHOD Open(const char * url, uint16_t state, nsIWebBrowser * *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->Open(url, state, _retval); } \
  NS_IMETHOD GetActiveBrowser(nsIWebBrowser * *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetActiveBrowser(_retval); } \
  NS_IMETHOD GetCurrentWindow(kmIWindow * *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCurrentWindow(_retval); } \
  NS_IMETHOD GetWindows(uint32_t *length, kmIWindow * **list) override { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWindows(length, list); } \
  NS_IMETHOD AddListener(nsIObserver *listener) override { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(listener); } \
  NS_IMETHOD RemoveListener(nsIObserver *listener) override { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(listener); } \
  NS_IMETHOD LoadPlugin(const char * path) override { return !_to ? NS_ERROR_NULL_POINTER : _to->LoadPlugin(path); } \
  NS_IMETHOD ShowMenu(const char * name, bool sendCommand, int32_t *_retval) override { return !_to ? NS_ERROR_NULL_POINTER : _to->ShowMenu(name, sendCommand, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsJSBridge : public nsIJSBridge
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJSBRIDGE

  nsJSBridge();

private:
  ~nsJSBridge();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS(nsJSBridge, nsIJSBridge)

nsJSBridge::nsJSBridge()
{
  /* member initializers and constructor code */
}

nsJSBridge::~nsJSBridge()
{
  /* destructor code */
}

/* void SetMenuCallback (in string menu, in string label, in kmICommandFunction command, [optional] in string before); */
NS_IMETHODIMP nsJSBridge::SetMenuCallback(const char * menu, const char * label, kmICommandFunction *command, const char * before)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void SetMenu (in string menu, in unsigned short type, in string label, in string command, [optional] in string before); */
NS_IMETHODIMP nsJSBridge::SetMenu(const char * menu, uint16_t type, const char * label, const char * command, const char * before)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void RebuildMenu (in string menu); */
NS_IMETHODIMP nsJSBridge::RebuildMenu(const char * menu)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* kmIButton CreateButton (in string cmd, [optional] in string menu, [optional] in string tooltip, [optional] in string label); */
NS_IMETHODIMP nsJSBridge::CreateButton(const char * cmd, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* kmIButton CreateCallbackButton (in kmICommandFunction command, [optional] in string menu, [optional] in string tooltip, [optional] in string label); */
NS_IMETHODIMP nsJSBridge::CreateCallbackButton(kmICommandFunction *command, const char * menu, const char * tooltip, const char * label, kmIButton * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AddToolbar (in string toolbar, [optional] in unsigned long width, [optional] in unsigned long height); */
NS_IMETHODIMP nsJSBridge::AddToolbar(const char * toolbar, uint32_t width, uint32_t height)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AddButton (in string toolbar, in string command, [optional] in string menu, [optional] in string tooltip); */
NS_IMETHODIMP nsJSBridge::AddButton(const char * toolbar, const char * command, const char * menu, const char * tooltip)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void RemoveButton (in string toolbar, in string command); */
NS_IMETHODIMP nsJSBridge::RemoveButton(const char * toolbar, const char * command)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void id (in nsIDOMWindow window, in string id); */
NS_IMETHODIMP nsJSBridge::Id(nsIDOMWindow *window, const char * id)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long SendMessage (in string plugin, in string to, in string from, in string data1); */
NS_IMETHODIMP nsJSBridge::SendMessage(const char * plugin, const char * to, const char * from, const char * data1, int32_t *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetCmdList ([optional] out unsigned long length, [array, size_is (length), retval] out kmICommand list); */
NS_IMETHODIMP nsJSBridge::GetCmdList(uint32_t *length, kmICommand * **list)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] long RegisterCmd (in string name, in string desc, in kmICommandFunction command, [optional] in jsval icon, [optional] in kmICallback enabled, [optional] in kmICallback checked); */
NS_IMETHODIMP nsJSBridge::RegisterCmd(const char * name, const char * desc, kmICommandFunction *command, JS::HandleValue icon, kmICallback *enabled, kmICallback *checked, JSContext* cx, int32_t *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UnregisterCmd (in string name); */
NS_IMETHODIMP nsJSBridge::UnregisterCmd(const char * name)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] void SetCmdIcon (in string name, in jsval icon); */
NS_IMETHODIMP nsJSBridge::SetCmdIcon(const char * name, JS::HandleValue icon, JSContext* cx)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] void SetButtonIcon (in string toolbar, in string command, in jsval icon); */
NS_IMETHODIMP nsJSBridge::SetButtonIcon(const char * toolbar, const char * command, JS::HandleValue icon, JSContext* cx)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void SetAccel (in string key, in string command); */
NS_IMETHODIMP nsJSBridge::SetAccel(const char * key, const char * command)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIWebBrowser Open (in string url, in unsigned short state); */
NS_IMETHODIMP nsJSBridge::Open(const char * url, uint16_t state, nsIWebBrowser * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIWebBrowser GetActiveBrowser (); */
NS_IMETHODIMP nsJSBridge::GetActiveBrowser(nsIWebBrowser * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* kmIWindow GetCurrentWindow (); */
NS_IMETHODIMP nsJSBridge::GetCurrentWindow(kmIWindow * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetWindows ([optional] out unsigned long length, [array, size_is (length), retval] out kmIWindow list); */
NS_IMETHODIMP nsJSBridge::GetWindows(uint32_t *length, kmIWindow * **list)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AddListener (in nsIObserver listener); */
NS_IMETHODIMP nsJSBridge::AddListener(nsIObserver *listener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void RemoveListener (in nsIObserver listener); */
NS_IMETHODIMP nsJSBridge::RemoveListener(nsIObserver *listener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void LoadPlugin (in string path); */
NS_IMETHODIMP nsJSBridge::LoadPlugin(const char * path)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long ShowMenu (in string name, [optional] in bool sendCommand); */
NS_IMETHODIMP nsJSBridge::ShowMenu(const char * name, bool sendCommand, int32_t *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIJSBridge_h__ */
