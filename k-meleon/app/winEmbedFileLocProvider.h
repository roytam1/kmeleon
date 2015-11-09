/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this Mozilla sample software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Contributor(s):
 *   Conrad Carlen <conrad@ingress.com>
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsXPCOMGlue.h"

#define K_APP_SKINS_DIR             "KASkins"
#define K_APP_KPLUGINS_DIR          "KAPlugins"
#define K_USER_SKINS_DIR            "KUSkins"
#define K_USER_KPLUGINS_DIR         "KUPlugins"
#define K_APP_SETTING_DEFAULTS      "KDefSettings"
#define K_USER_SETTING              "KUserSettings"

class nsILocalFile;

//*****************************************************************************
// class winEmbedFileLocProvider
//*****************************************************************************   

class winEmbedFileLocProvider : public nsIDirectoryServiceProvider2
{
public:
    // aProductDirName is the name (not path) of the dir
    // in which the application registry and profiles live.
    winEmbedFileLocProvider(const nsACString& aProductDirName);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER
	NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

protected:
    virtual              ~winEmbedFileLocProvider();

    NS_METHOD            CloneMozBinDirectory(nsIFile **aLocalFile);   
    NS_METHOD            GetProductDirectory(nsIFile **aLocalFile, PRBool aLocal = PR_FALSE);
    NS_METHOD            GetDefaultUserProfileRoot(nsIFile **aLocalFile, bool aLocal = PR_FALSE);

    nsEmbedCString         mProductDirName;
    nsCOMPtr<nsIFile> mMozBinDirectory;
	nsCOMPtr<nsIFile> mProfileDirectory;
};
