/*
*  Copyright (C) 2013 Dorian Boissonnade
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
*
*
*/

#include "nsIXULAppInfo.h"
#include "nsIPlatformInfo.h"
#include "nsIXULRuntime.h"
#include "nsIAppStartup.h"

#define NS_KMAPPINFO_CID \
  {0x76849bf1, 0x199d, 0x41a6, {0xaa, 0xe6, 0x87, 0x3f, 0xca, 0xf1, 0x23, 0xea}}

class KmAppInfo: public nsIXULAppInfo, public nsIXULRuntime, public nsIAppStartup
{
	bool mInterrupted;
public:
	KmAppInfo() : mInterrupted(false) {}
	NS_DECL_ISUPPORTS
	NS_DECL_NSIPLATFORMINFO
	NS_DECL_NSIXULAPPINFO
	NS_DECL_NSIXULRUNTIME
	NS_DECL_NSIAPPSTARTUP
	//NS_DECL_NSIAPPSTARTUP2
};


