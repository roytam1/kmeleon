/*
*  Copyright (C) 2001 Jeff Doozan
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

CMostRecentUrls::CMostRecentUrls() {
  m_hMutex = CreateMutex(NULL, FALSE, NULL);
  m_URLs = NULL;
  LoadURLs();
}

void CMostRecentUrls::LoadURLs() {

  DWORD dwWaitResult; 
  do {
    dwWaitResult = WaitForSingleObject( m_hMutex, 1000L);
    Sleep(1);
  } while (dwWaitResult != WAIT_OBJECT_0);

   m_maxURLs = theApp.preferences.GetInt("kmeleon.MRU.maxURLs", 16); 
   if (m_maxURLs <= 0) {
      m_maxURLs=0;
      m_URLCount=0;
      m_URLs=NULL;
      ReleaseMutex(m_hMutex);
      return;
   }

   ASSERT(!m_URLs);
   m_URLs = new char*[m_maxURLs];
   
   char sPref[20] = "kmeleon.MRU.URL", sBuf[5];
   char *sCount = sPref + strlen(sPref);
   int x=0;
   int y=0;

   do {
      itoa(x, sBuf, 10);
      strcpy(sCount, sBuf);                              // create  "kmeleon.MRU.URL##" string
      int len = theApp.preferences.GetString(sPref, NULL, "");
      if (len > 0) {
         m_URLs[y] = new char[len+1];
         theApp.preferences.GetString(sPref, m_URLs[y], "");
	 y++;
      }
   } while (++x<m_maxURLs);
   
   m_URLCount = y;
   // nullify the empty entries
   for(;y<m_maxURLs;y++)
      m_URLs[y] = NULL;

   ReleaseMutex(m_hMutex);
}

CMostRecentUrls::~CMostRecentUrls() {
  SaveURLs();
  DeleteURLs();
}

void CMostRecentUrls::SaveURLs() {

   if (m_maxURLs < 1)  return;

  DWORD dwWaitResult; 
  do {
    dwWaitResult = WaitForSingleObject( m_hMutex, 1000L);
    Sleep(1);
  } while (dwWaitResult != WAIT_OBJECT_0);

   char sPref[20] = "kmeleon.MRU.URL", sBuf[5];
   char *sCount = sPref + strlen(sPref);

   theApp.preferences.SetInt("kmeleon.MRU.maxURLs", m_maxURLs); 

   int y = 0;
   for (int x=0; x<m_URLCount; x++) {
      itoa(y, sBuf, 10);
      strcpy(sCount, sBuf);                              // create  "kmeleon.MRU.URL##" string
      if (strlen(m_URLs[x]) > 0) {
        theApp.preferences.SetString(sPref, m_URLs[x]);
	y++;
      }
   }

   ReleaseMutex(m_hMutex);
}

void CMostRecentUrls::DeleteURLs() {

  DWORD dwWaitResult; 
  do {
    dwWaitResult = WaitForSingleObject( m_hMutex, 1000L);
    Sleep(1);
  } while (dwWaitResult != WAIT_OBJECT_0);

   for (int x=0;x<m_maxURLs;x++)
      if(m_URLs[x]) delete m_URLs[x];

   delete m_URLs;
   m_URLs = NULL;

   ReleaseMutex(m_hMutex);
}

char * CMostRecentUrls::GetURL(int aInx) {
    if (aInx < m_URLCount) return m_URLs[aInx];
    return NULL;
}

void CMostRecentUrls::RefreshURLs() {
  DeleteURLs();
  LoadURLs();
}

void CMostRecentUrls::AddURL(const char * aURL) {

   char * newurl;
   if (m_maxURLs<1) return;

  DWORD dwWaitResult; 
  do {
    dwWaitResult = WaitForSingleObject( m_hMutex, 1000L);
    Sleep(1);
  } while (dwWaitResult != WAIT_OBJECT_0);

   // check list for previous entry
   for (int x=0; x<m_URLCount; x++)	{
      if(m_URLs[x]) {
         if(strcmp(m_URLs[x], aURL) == 0)
            break; 
      }
   }

   // if no match
   if((x==m_URLCount) || (m_URLCount == 0)) {
      if (m_URLCount==m_maxURLs)
         delete m_URLs[--x];            // if list is full, remove the bottom entry
      else {                          // otherwise add a new entry and increase the count
         m_URLCount++;
      }
	  newurl = new char[strlen(aURL)+1]; // Allocating the new entry
	  strcpy(newurl,aURL);
   }
   else
	   newurl = m_URLs[x];

   // shift everything down
   while (x) {
      m_URLs[x] = m_URLs[x-1];
      x--;
   }

	// add the new url
   m_URLs[0] = newurl;

   ReleaseMutex(m_hMutex);
}
