/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM kmHelper.idl
 */

#ifndef __gen_kmHelper_h__
#define __gen_kmHelper_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIDOMEventTarget_h__
#include "nsIDOMEventTarget.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    kmIHelper */
#define KMIHELPER_IID_STR "6e4995c3-deef-46a2-be73-154647710190"

#define KMIHELPER_IID \
  {0x6e4995c3, 0xdeef, 0x46a2, \
    { 0xbe, 0x73, 0x15, 0x46, 0x47, 0x71, 0x01, 0x90 }}

class NS_NO_VTABLE kmIHelper : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(KMIHELPER_IID)

  /* void initLogon (in nsIDOMEventTarget browser, in nsIDOMWindow win); */
  NS_IMETHOD InitLogon(nsIDOMEventTarget *browser, nsIDOMWindow *win) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(kmIHelper, KMIHELPER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_KMIHELPER \
  NS_IMETHOD InitLogon(nsIDOMEventTarget *browser, nsIDOMWindow *win); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_KMIHELPER(_to) \
  NS_IMETHOD InitLogon(nsIDOMEventTarget *browser, nsIDOMWindow *win) { return _to InitLogon(browser, win); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_KMIHELPER(_to) \
  NS_IMETHOD InitLogon(nsIDOMEventTarget *browser, nsIDOMWindow *win) { return !_to ? NS_ERROR_NULL_POINTER : _to->InitLogon(browser, win); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class kmHelper : public kmIHelper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_KMIHELPER

  kmHelper();

private:
  ~kmHelper();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(kmHelper, kmIHelper)

kmHelper::kmHelper()
{
  /* member initializers and constructor code */
}

kmHelper::~kmHelper()
{
  /* destructor code */
}

/* void initLogon (in nsIDOMEventTarget browser, in nsIDOMWindow win); */
NS_IMETHODIMP kmHelper::InitLogon(nsIDOMEventTarget *browser, nsIDOMWindow *win)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_kmHelper_h__ */
