/*
*  Copyright (C) 2005 Dorian Boissonnade 
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
*/

#include "nsICookie.h"
#include "nsEmbedString.h"
#include "resource.h"

class CCookie {

public:
	nsEmbedCString m_host;
	nsEmbedCString m_name;
	nsEmbedCString m_path;
	CString m_csHost;
	CString m_csName;
	CString m_csValue;
	CString m_csPath;
	CString m_csExpire;
	BOOL  m_secure;

	
	CCookie(nsICookie* cookie)
	{
		if (cookie) {

		USES_CONVERSION;
		
		cookie->GetName(m_name);
		m_csName = A2CT(m_name.get());

		nsEmbedCString str;
		cookie->GetValue(str);
		m_csValue = A2CT(str.get());

		cookie->GetHost(m_host);
		nsEmbedString _str;
		NS_CStringToUTF16(m_host, NS_CSTRING_ENCODING_UTF8, _str);
		m_csHost = W2CT(_str.get());

		cookie->GetPath(m_path);
		m_csPath = A2CT(m_path.get());

		PRBool bIsSecure;
		cookie->GetIsSecure(&bIsSecure);
		m_secure = (bIsSecure == PR_TRUE);

		/*if (bIsSecure == PR_TRUE)
			dlg.m_csSendFor.LoadString(IDS_FOR_SECURE);
		else
			dlg.m_csSendFor.LoadString(IDS_FOR_ANY);*/

		PRUint64 expires;
		cookie->GetExpires(&expires);
		if (expires>0){
			char fDate[128];
			tm* tmTime = localtime ((time_t*)&expires);
			if (tmTime)
			{
				strftime (fDate, sizeof(fDate)-1, "%a %d %b %Y %H:%M", tmTime);
				m_csExpire = fDate;
			}
		}
		else
			m_csExpire.LoadString(IDS_ESPIRES_EOS);
		}
	}
	
	~CCookie() 
	{
	}
	
};
