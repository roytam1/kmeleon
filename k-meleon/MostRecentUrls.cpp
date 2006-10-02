/*
*  Copyright (C) 2001 Jeff Doozan
*  Copyright (C) 2006 Dorian Boissonnade
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

#include "StdAfx.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "MostRecentUrls.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

CMostRecentUrls::CMostRecentUrls()
{
   LoadURLs();
}

CMostRecentUrls::~CMostRecentUrls()
{
   SaveURLs();
}

void CMostRecentUrls::LoadURLs()
{
   m_maxURLs = theApp.preferences.GetInt("kmeleon.MRU.maxURLs", 16); 
   if (m_maxURLs <= 0) {
      m_maxURLs=0;
      return;
   }

   char sPref[20] = "kmeleon.MRU.URL", sBuf[5];
   char *sCount = sPref + strlen(sPref);
   int x=0;

   for (x=0; x<m_maxURLs; x++) {
      itoa(x, sBuf, 10);
      strcpy(sCount, sBuf);                              // create  "kmeleon.MRU.URL##" string
      int len = theApp.preferences.GetString(sPref, NULL, "");
      if (len > 0) {
         CString url;
         theApp.preferences.GetString(sPref, url.GetBufferSetLength(len), _T(""));
         url.ReleaseBuffer(len);
         AddTail(url);
      }
   }
}

void CMostRecentUrls::SaveURLs()
{
   if (m_maxURLs < 1)  return;

   char sPref[20] = "kmeleon.MRU.URL", sBuf[5];
   char *sCount = sPref + strlen(sPref);

   theApp.preferences.SetInt("kmeleon.MRU.maxURLs", m_maxURLs); 

   int y = 0;
   POSITION pos = GetHeadPosition();
   while (pos) {
      itoa(y++, sBuf, 10);
      strcpy(sCount, sBuf);   // create  "kmeleon.MRU.URL##" string
      CString url = GetNext(pos);
      if (!url.IsEmpty())
         theApp.preferences.SetString(sPref, url);
   }
}

void CMostRecentUrls::RefreshURLs()
{
   DeleteURLs();
   LoadURLs();
}

void CMostRecentUrls::AddURL(LPCTSTR aURL)
{  
   RefreshURLs(); // Else macros/plugins can't delete them
   m_maxURLs = theApp.preferences.GetInt("kmeleon.MRU.maxURLs", 16); 
   while (GetCount() > m_maxURLs)
      RemoveTail();
   
   if (m_maxURLs<1) return;

   // check list for previous entry
   POSITION pos = Find(aURL);
   if (!pos) {
      // It's a new entry, if list is full, remove the bottom entry
      if (GetCount() == m_maxURLs)  
         RemoveTail();
   }
   else 
      RemoveAt(pos);

   AddHead(aURL);
   SaveURLs();
   theApp.BroadcastMessage(UWM_REFRESHMRULIST, 0, 0);
}