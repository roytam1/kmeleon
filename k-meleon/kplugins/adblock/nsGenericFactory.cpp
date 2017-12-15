/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */



// I need this file only when build with XPCOM_GLUE
// When builded with it the components should be in a
// external dll file like the original mfcembed do.
// But we don't really need it to have them in a dll.
//#ifdef XPCOM_GLUE

// DO NOT COPY THIS CODE INTO YOUR SOURCE!  USE NS_IMPL_NSGETMODULE()

#include "nsMemory.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsGenericFactory.h"
#include "nsIProgrammingLanguage.h"


nsGenericFactory::nsGenericFactory(const nsModuleComponentInfo *info)
    : mInfo(info)
{

}

nsGenericFactory::~nsGenericFactory()
{

}

NS_IMPL_ISUPPORTS(nsGenericFactory,
	nsIGenericFactory,
                              nsIFactory,
                              nsIClassInfo)

NS_IMETHODIMP nsGenericFactory::CreateInstance(nsISupports *aOuter,
                                               REFNSIID aIID, void **aResult)
{
    if (mInfo->mConstructor) {
        return mInfo->mConstructor(aOuter, aIID, aResult);
    }

    return NS_ERROR_FACTORY_NOT_REGISTERED;
}

NS_IMETHODIMP nsGenericFactory::LockFactory(bool aLock)
{
    // XXX do we care if (mInfo->mFlags & THREADSAFE)?
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetInterfaces(PRUint32 *countp,
                                              nsIID* **array)
{
    *countp = 0;
    *array = nullptr;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetHelperForLanguage(PRUint32 language,
                                                     nsISupports **helper)
{
    *helper = nullptr;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetContractID(char **aContractID)
{
    if (mInfo->mContractID) {
        *aContractID = (char *)nsMemory::Alloc(strlen(mInfo->mContractID) + 1);
        if (!*aContractID)
            return NS_ERROR_OUT_OF_MEMORY;
        strcpy(*aContractID, mInfo->mContractID);
    } else {
        *aContractID = nullptr;
    }
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetClassDescription(char * *aClassDescription)
{
    if (mInfo->mDescription) {
        *aClassDescription = (char *)
            nsMemory::Alloc(strlen(mInfo->mDescription) + 1);
        if (!*aClassDescription)
            return NS_ERROR_OUT_OF_MEMORY;
        strcpy(*aClassDescription, mInfo->mDescription);
    } else {
        *aClassDescription = nullptr;
    }
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetClassID(nsCID * *aClassID)
{
    *aClassID =
        reinterpret_cast<nsCID*>
                        (nsMemory::Clone(&mInfo->mCID, sizeof mInfo->mCID));
   if (! *aClassID)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetClassIDNoAlloc(nsCID *aClassID)
{
    *aClassID = mInfo->mCID;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetImplementationLanguage(PRUint32 *langp)
{
    *langp = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetFlags(PRUint32 *flagsp)
{
    return NS_OK;
}

// nsIGenericFactory: component-info accessors
NS_IMETHODIMP nsGenericFactory::SetComponentInfo(const nsModuleComponentInfo *info)
{
    mInfo = info;
    return NS_OK;
}

NS_IMETHODIMP nsGenericFactory::GetComponentInfo(const nsModuleComponentInfo **infop)
{
    *infop = mInfo;
    return NS_OK;
}

NS_METHOD nsGenericFactory::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    // sorry, aggregation not spoken here.
    nsresult res = NS_ERROR_NO_AGGREGATION;
    if (outer == NULL) {
        nsGenericFactory* factory = new nsGenericFactory;
        if (factory != NULL) {
            res = factory->QueryInterface(aIID, aInstancePtr);
            if (res != NS_OK)
                delete factory;
        } else {
            res = NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return res;
}


nsresult
NS_NewGenericFactory(nsIGenericFactory* *result,
                     const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsIGenericFactory* fact;
    rv = nsGenericFactory::Create(NULL, NS_GET_IID(nsIFactory), (void**)&fact);
    if (NS_FAILED(rv)) return rv;
    rv = fact->SetComponentInfo(info);
    if (NS_FAILED(rv)) goto error;
    *result = fact;
    return rv;

  error:
    NS_RELEASE(fact);
    return rv;
}


//#endif //XPCOM_GLUE
