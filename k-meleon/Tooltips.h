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

#include "stdafx.h"

// {0b666e3e-569a-462c-a7f0-b16bb15d42ff}
#define NS_TOOLTIPTEXTPROVIDER_CID     \
{ 0x0b666e3e, 0x569a, 0x462c, \
   {0xa7, 0xf0, 0xb1, 0x6b, 0xb1, 0x5d, 0x42, 0xff} }
static NS_DEFINE_CID(kTooltipTextProviderCID, NS_TOOLTIPTEXTPROVIDER_CID);

class CTooltipTextProvider : public nsITooltipTextProvider {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_TOOLTIPTEXTPROVIDER_CID );

    // ctor/dtor
    CTooltipTextProvider() {
        NS_INIT_REFCNT();
    }
    virtual ~CTooltipTextProvider() {
    }

    // This class implements the nsISupports interface functions.
    NS_DECL_ISUPPORTS

    // This class implements the nsITooltipTextProvider interface functions.
    NS_DECL_NSITOOLTIPTEXTPROVIDER

}; // CTooltipTextProvider


nsresult NewTooltipTextProviderFactory(nsIFactory** aFactory);

