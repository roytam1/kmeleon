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



#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIClassInfo.h"
#include "mozilla/ModuleUtils.h"

struct nsModuleComponentInfo {
    const char*                                 mDescription;
    nsCID                                       mCID;
    const char*                                 mContractID;
    mozilla::Module::ConstructorProcPtr         mConstructor;    
};

// {3bc97f01-ccdf-11d2-bab8-b548654461fc}
#define NS_GENERICFACTORY_CID                                                 \
  { 0x3bc97f01, 0xccdf, 0x11d2,                                               \
    { 0xba, 0xb8, 0xb5, 0x48, 0x65, 0x44, 0x61, 0xfc } }

// {3bc97f00-ccdf-11d2-bab8-b548654461fc}
#define NS_IGENERICFACTORY_IID                                                \
  { 0x3bc97f00, 0xccdf, 0x11d2,                                               \
    { 0xba, 0xb8, 0xb5, 0x48, 0x65, 0x44, 0x61, 0xfc } }

class nsIGenericFactory : public nsIFactory {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGENERICFACTORY_IID)
    
    NS_IMETHOD SetComponentInfo(const nsModuleComponentInfo *info) = 0;
    NS_IMETHOD GetComponentInfo(const nsModuleComponentInfo **infop) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGenericFactory, NS_IGENERICFACTORY_IID);

/**
 * Most factories follow this simple pattern, so why not just use a function
 * pointer for most creation operations?
 */
class nsGenericFactory : public nsIGenericFactory, public nsIClassInfo {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_GENERICFACTORY_CID)

    nsGenericFactory(const nsModuleComponentInfo *info = NULL);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSICLASSINFO
    
    /* nsIGenericFactory methods */
    NS_IMETHOD SetComponentInfo(const nsModuleComponentInfo *info);
    NS_IMETHOD GetComponentInfo(const nsModuleComponentInfo **infop);

    NS_IMETHOD CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_IMETHOD LockFactory(bool aLock);

    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
private:
    ~nsGenericFactory();

    const nsModuleComponentInfo *mInfo;
};

////////////////////////////////////////////////////////////////////////////////

#include "nsIModule.h"
#include "plhash.h"

class nsGenericModule : public nsIModule
{
public:
    nsGenericModule(const char* moduleName, 
                    PRUint32 componentCount,
                    const nsModuleComponentInfo* components,
                    mozilla::Module::ConstructorProcPtr ctor);

private:
    ~nsGenericModule();

public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIMODULE

    struct FactoryNode
    {
        FactoryNode(nsIFactory* fact, FactoryNode* next) 
        { 
            mFactory = fact; 
            mNext    = next;
        }
        ~FactoryNode(){}

        nsCOMPtr<nsIFactory> mFactory;
        FactoryNode* mNext;
    };




protected:
    nsresult Initialize(nsIComponentManager* compMgr);

    void Shutdown();
    nsresult AddFactoryNode(nsIFactory* fact);

    PRBool                       mInitialized;
    const char*                  mModuleName;
    PRUint32                     mComponentCount;
    const nsModuleComponentInfo* mComponents;
    FactoryNode*                 mFactoriesNotToBeRegistered;
    mozilla::Module::ConstructorProcPtr      mCtor;
    //nsModuleDestructorProc       mDtor;
};
