/*
*  Copyright (C) 2000 Brian Harris
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*

  Why this is here:
  The mozilla team has decided that they are going to follow W3C standards rather than maintain
  backwards compatibility by only providing tooltips for "title" rather than "alt"
  While this is really great for standards, it's pretty crappy for users, so this little implementation
  will check the node to see if it has a "title" tag, and if it doesn't it will use the "alt" tag.

*/

#include "StdAfx.h"
#include "Tooltips.h"

// The Object:

NS_IMETHODIMP CTooltipTextProvider::GetNodeText(nsIDOMNode *aNode, PRUnichar **aText, PRBool *_retval)
{
   *_retval = PR_FALSE;

   nsCOMPtr<nsIDOMNamedNodeMap> attributes;
   aNode->GetAttributes(getter_AddRefs(attributes));
   if (attributes)
   {
      nsString tipText;
      nsCOMPtr<nsIDOMNode> attrNode;
      nsAutoString attr;

      // first try to find the title
      attr.AssignWithConversion("title");
      attributes->GetNamedItem(attr, getter_AddRefs(attrNode));
      if (attrNode)
      {
         attrNode->GetNodeValue(tipText);
         *aText = ToNewUnicode(tipText);
         *_retval = PR_TRUE;
         return NS_OK;
      }

      // then look for the alt
      attr.AssignWithConversion("alt");
      attributes->GetNamedItem(attr, getter_AddRefs(attrNode));
      if (attrNode)
      {
         nsString tipText;
         attrNode->GetNodeValue(tipText);
         *aText = ToNewUnicode(tipText);
         *_retval = PR_TRUE;
         return NS_OK;
      }
   }
   return NS_OK;
}



// The Factory:


NS_GENERIC_FACTORY_CONSTRUCTOR(CTooltipTextProvider)

/*
static nsModuleComponentInfo components[] = {
   { "Tooltip Text Provider",
    NS_TOOLTIPTEXTPROVIDER_CID, 
    NS_TOOLTIPTEXTPROVIDER_CONTRACTID,
    CTooltipTextProviderConstructor }
};

NS_IMPL_NSGETMODULE("CTooltipTextProvider", components )
*/

/* nsISupports Implementation for the class */
NS_IMPL_ISUPPORTS1(CTooltipTextProvider,
                   nsITooltipTextProvider)



 
//*****************************************************************************
// CTooltipTextProviderFactory
//*****************************************************************************   

class CTooltipTextProviderFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CTooltipTextProviderFactory();
  virtual ~CTooltipTextProviderFactory();
};

//*****************************************************************************   

NS_IMPL_ISUPPORTS1(CTooltipTextProviderFactory, nsIFactory)

CTooltipTextProviderFactory::CTooltipTextProviderFactory()
{
  NS_INIT_ISUPPORTS();
}

CTooltipTextProviderFactory::~CTooltipTextProviderFactory()
{
}

NS_IMETHODIMP CTooltipTextProviderFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CTooltipTextProvider *inst = new CTooltipTextProvider;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CTooltipTextProviderFactory::LockFactory(PRBool lock)
{
    return NS_OK;
}


nsresult NewTooltipTextProviderFactory(nsIFactory** aFactory) {
   NS_ENSURE_ARG_POINTER(aFactory);
   *aFactory = nsnull;
   CTooltipTextProviderFactory *result = new CTooltipTextProviderFactory;
   if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(result);
   *aFactory = result;
   return NS_OK;
}
