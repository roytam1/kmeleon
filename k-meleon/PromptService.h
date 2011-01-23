/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef __PromptService_h
#define __PromptService_h

// this component is for an MFC app; it's Windows. make sure this is defined.
#ifndef XP_WIN
#define XP_WIN
#endif

class nsIFactory;

#include "nsIPromptService.h"
#if GECKO_VERSION < 193
#include "nsINonBlockingAlertService.h"
#endif
#include "nsIWindowWatcher.h"
#include "nsEmbedCID.h"

#define NS_PROMPTSERVICE_CID \
{0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}
static NS_DEFINE_CID(kPromptServiceCID, NS_PROMPTSERVICE_CID);

class CPromptService: public nsIPromptService
#if GECKO_VERSION < 193
					  ,public nsINonBlockingAlertService
#endif

{
public:
                 CPromptService();
  virtual       ~CPromptService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROMPTSERVICE
#if GECKO_VERSION < 193
  NS_DECL_NSINONBLOCKINGALERTSERVICE
#endif
  
};

// factory creator, in hard and soft link formats
extern "C" NS_EXPORT nsresult NS_NewPromptServiceFactory(nsIFactory** aFactory);
typedef nsresult (__cdecl *MakeFactoryType)(nsIFactory **);
#define kPromptServiceFactoryFuncName "NS_NewPromptServiceFactory"

// initialization function, in hard and soft link formats
extern "C" NS_EXPORT void InitPromptService(HINSTANCE instance);
typedef nsresult (__cdecl *InitPromptServiceType)(HINSTANCE instance);
#define kPromptServiceInitFuncName "InitPromptService"

#endif
