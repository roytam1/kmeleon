/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "stdafx.h"

#include "UnknownContentTypeHandler.h"

#include "BrowserFrm.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

// HandleUnknownContentType (from nsIUnknownContentTypeHandler) implementation.
// XXX We can get the content type from the channel now so that arg could be dropped.

NS_IMETHODIMP
CUnknownContentTypeHandler::HandleUnknownContentType( nsIRequest *request,
                                                       const char *aContentType,
                                                       nsIDOMWindowInternal *aWindow ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIChannel> aChannel;
    nsCOMPtr<nsISupports> channel;
    nsCAutoString         contentDisp;

    // this function never seems to get called...
    MessageBox(NULL, "CHandleUnknownContentType()", NULL, MB_OK);

    /*
    if ( request ) {
        
      aChannel = do_QueryInterface(request);

        // Need root nsISupports for later JS_PushArguments call.
        channel = do_QueryInterface( aChannel );

        // Try to get HTTP channel.
        nsCOMPtr<nsIHTTPChannel> httpChannel = do_QueryInterface( aChannel );
        if ( httpChannel ) {
            // Get content-disposition response header.
            nsCOMPtr<nsIAtom> atom = dont_AddRef(NS_NewAtom( "content-disposition" ));
            if ( atom ) {
                nsXPIDLCString disp; 
                rv = httpChannel->GetResponseHeader( atom, getter_Copies( disp ) );
                if ( NS_SUCCEEDED( rv ) && disp ) {
                    contentDisp = disp; // Save the response header to pass to dialog.
                }
            }
        }

        // Cancel input channel now.
        rv = request->Cancel(NS_BINDING_ABORTED);
        if ( NS_FAILED( rv ) ) {
          NS_WARNING("Cancel failed");
        }
    }

    if ( NS_SUCCEEDED( rv ) && channel && aContentType && aWindow ) {
        // Open "Unknown content type" dialog.
        // We pass in the channel, the content type, and the content disposition.
        // Note that the "parent" browser window will be window.opener within the
        // new dialog.
    
        // Get JS context from parent window.
        nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( aWindow, &rv );
        if ( NS_SUCCEEDED( rv ) && sgo ) {
            nsCOMPtr<nsIScriptContext> context;
            sgo->GetContext( getter_AddRefs( context ) );
            if ( context ) {
                JSContext *jsContext = (JSContext*)context->GetNativeContext();
                if ( jsContext ) {
                    void *stackPtr;
                    jsval *argv = JS_PushArguments( jsContext,
                                                    &stackPtr,
                                                    "sss%ipss",
                                                    "chrome://global/content/unknownContent.xul",
                                                    "_blank",
                                                    "chrome,titlebar",
                                                    (const nsIID*)(&NS_GET_IID(nsIChannel)),
                                                    (nsISupports*)channel.get(),
                                                    aContentType,
                                                    contentDisp.get() );
                    if ( argv ) {
                        nsCOMPtr<nsIDOMWindowInternal> newWindow;
                        rv = aWindow->OpenDialog( jsContext, argv, 6, getter_AddRefs( newWindow ) );
                        NS_ASSERTION(NS_SUCCEEDED(rv), "OpenDialog failed");
                        JS_PopArguments( jsContext, stackPtr );
                    } else {
                        NS_ASSERTION(0, "JS_PushArguments failed");
                        rv = NS_ERROR_FAILURE;
                    }
                } else {
                    NS_ASSERTION(0, "GetNativeContext failed");
                    rv = NS_ERROR_FAILURE;
                }
            } else {
                NS_ASSERTION(0, "GetContext failed");
                rv = NS_ERROR_FAILURE;
            }
        } else {
            NS_ASSERTION(0, "QueryInterface (for nsIScriptGlobalObject) failed");
        }
    } else {
        // If no error recorded so far, set one now.
        if ( NS_SUCCEEDED( rv ) ) {
            rv = NS_ERROR_NULL_POINTER;
        }
    }
    */

    return rv;
}


NS_IMETHODIMP
CUnknownContentTypeHandler::ShowProgressDialog(nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {

    // this is here because mozilla won't do anything untill we call this function
    // eventually we should probably pop up a "file saving" box or something
    aLauncher->SetWebProgressListener (NULL);

    return NS_OK;
}

// Show the helper app launch confirmation dialog as instructed.
NS_IMETHODIMP
CUnknownContentTypeHandler::Show( nsIHelperAppLauncher *aLauncher, nsISupports *aContext ) {

    // we always want to download it.
    // later on, we may want to say, "hey, we have no clue how to handle this, do you want to
    // save it or open it in some application?"

   aLauncher->SaveToDisk(nsnull, false);

   return NS_OK;
}

// prompt the user for a file name to save the unknown content to as instructed
NS_IMETHODIMP
CUnknownContentTypeHandler::PromptForSaveToFile(nsISupports * aWindowContext, const PRUnichar * aDefaultFile, const PRUnichar * aSuggestedFileExtension, nsILocalFile ** aNewFile)
{
// change this to 0 to use the mozilla file picker
#if 1
   USES_CONVERSION;

   CString filter = W2T(aSuggestedFileExtension);
   filter += " files|*";
   filter += W2T(aSuggestedFileExtension);
   filter += "|All Files|*.*||";

   CFileDialog cf(FALSE, W2T(aSuggestedFileExtension), W2T(aDefaultFile), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter, (CWnd *)theApp.m_pMostRecentBrowserFrame);
   if(cf.DoModal() == IDOK){
      NS_ENSURE_ARG_POINTER(aNewFile);

      if (!cf.GetPathName().IsEmpty()){
         nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));

         NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

         file->InitWithPath(cf.GetPathName());

         NS_ADDREF(*aNewFile = file);

         return NS_OK;
      }
   }

   return NS_ERROR_FAILURE;
#else
   nsresult rv = NS_OK;

   nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1", &rv);
   if (filePicker)
   {
      nsAFlatString title = NS_LITERAL_STRING ("Save As:");

      nsCOMPtr<nsIDOMWindowInternal> parent( do_GetInterface( aWindowContext ) );
      filePicker->Init(parent, title.get(), nsIFilePicker::modeSave);
      filePicker->SetDefaultString(aDefaultFile);
      nsAutoString wildCardExtension (NS_LITERAL_STRING("*").get());
      if (aSuggestedFileExtension) {
         wildCardExtension.Append(aSuggestedFileExtension);
         filePicker->AppendFilter(wildCardExtension.GetUnicode(), wildCardExtension.GetUnicode());
      }

      filePicker->AppendFilters(nsIFilePicker::filterAll);

      nsCOMPtr<nsILocalFile> startDir;
      // Pull in the user's preferences and get the default download directory.
      nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
      if ( prefs ) 
      {
         rv = prefs->GetFileXPref( "browser.download.dir", getter_AddRefs( startDir ) );
         if ( NS_SUCCEEDED(rv) && startDir ) 
         {
            PRBool isValid = PR_FALSE;
            startDir->Exists( &isValid );
            if ( isValid )  // Set file picker so startDir is used.
               filePicker->SetDisplayDirectory( startDir );
         }
      }

      PRInt16 dialogResult;
      filePicker->Show(&dialogResult);
      if (dialogResult == nsIFilePicker::returnCancel)
         rv = NS_ERROR_FAILURE;
      else
      {
         // be sure to save the directory the user chose as the new browser.download.dir
         rv = filePicker->GetFile(aNewFile);
         if (*aNewFile)
         {
            nsCOMPtr<nsIFile> newDirectory;
            (*aNewFile)->GetParent(getter_AddRefs(newDirectory));
            nsCOMPtr<nsILocalFile> newLocalDirectory (do_QueryInterface(newDirectory));

            if (newLocalDirectory)
               prefs->SetFileXPref( "browser.download.dir", newLocalDirectory);
         }
      }
   }
   return NS_OK;
#endif
}


NS_GENERIC_FACTORY_CONSTRUCTOR(CUnknownContentTypeHandler)

static nsModuleComponentInfo components[] = {
  { NS_IUNKNOWNCONTENTTYPEHANDLER_CLASSNAME, 
    NS_UNKNOWNCONTENTTYPEHANDLER_CID, 
    NS_IUNKNOWNCONTENTTYPEHANDLER_CONTRACTID,
    CUnknownContentTypeHandlerConstructor },
  { NS_IHELPERAPPLAUNCHERDLG_CLASSNAME, 
    NS_UNKNOWNCONTENTTYPEHANDLER_CID, 
    NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, 
    CUnknownContentTypeHandlerConstructor },
};

NS_IMPL_NSGETMODULE("CUnknownContentTypeHandler", components )

/* nsISupports Implementation for the class */
NS_IMPL_ISUPPORTS2(CUnknownContentTypeHandler,
                   nsIUnknownContentTypeHandler,
                   nsIHelperAppLauncherDialog)





 
//*****************************************************************************
// CUnknownContentHandlerFactory
//*****************************************************************************   

class CUnknownContentHandlerFactory : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CUnknownContentHandlerFactory();
  virtual ~CUnknownContentHandlerFactory();
};

//*****************************************************************************   

NS_IMPL_ISUPPORTS1(CUnknownContentHandlerFactory, nsIFactory)

CUnknownContentHandlerFactory::CUnknownContentHandlerFactory()
{
  NS_INIT_ISUPPORTS();
}

CUnknownContentHandlerFactory::~CUnknownContentHandlerFactory()
{
}

NS_IMETHODIMP CUnknownContentHandlerFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CUnknownContentTypeHandler *inst = new CUnknownContentTypeHandler;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CUnknownContentHandlerFactory::LockFactory(PRBool lock)
{
    return NS_OK;
}


nsresult NewUnknownContentHandlerFactory(nsIFactory** aFactory) {
   NS_ENSURE_ARG_POINTER(aFactory);
   *aFactory = nsnull;
   CUnknownContentHandlerFactory *result = new CUnknownContentHandlerFactory;
   if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(result);
   *aFactory = result;
   return NS_OK;
}



