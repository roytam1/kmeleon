// search for a switch in the command line
// return -1 if the switch is not found
// for separation purposes, we make the assumption that flags begin with a dash "-",
// but the "-" is  still required in the pSwitch parameter
// otherwise return the number of characters in the argument after the flag
// if pArgs is a valid pointer, copy the argument data into it
// if bRemove is set, the flag (and it's argument, if pArgs is valid) will be removed

#include "StdAfx.h"
#include "CmdLine.h"
#include "Utils.h"

CCmdLine::CCmdLine() {
   m_sProfilesDir = NULL;
   m_bChrome = FALSE;
}

CCmdLine::~CCmdLine() {
   if (m_sProfilesDir)
      delete m_sProfilesDir;
}

void CCmdLine::Initialize(char *cmdLine) {
   int len;

   m_sCmdLine = cmdLine;


   if (GetSwitch("-chrome", NULL, true) > 0)
      m_bChrome =  TRUE;

   // -profilesDir <directory>
   // or -profilesDir $appdata
   len = GetSwitch("-profilesDir", NULL, false);
   if (len == 0) // no arguments, invalid
      GetSwitch("-profilesDir", NULL, true);  // remove from command line
   else if (len > 0) {
      m_sProfilesDir = new char[len+1];
      GetSwitch("-profilesDir", m_sProfilesDir, true);

      if (!stricmp(m_sProfilesDir, "$appdata")) {
         delete m_sProfilesDir;
         m_sProfilesDir = new char[MAX_PATH];

         ITEMIDLIST *idl;
         if (SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &idl) == NOERROR) {
            IMalloc *malloc;
            SHGetPathFromIDListA(idl, m_sProfilesDir);
            SHGetMalloc(&malloc);
            malloc->Free(idl);
            malloc->Release();
         }
         else
            GetWindowsDirectoryA(m_sProfilesDir, MAX_PATH);
         int len = strlen(m_sProfilesDir);
         if (m_sProfilesDir[len-1] != '\\') {
            m_sProfilesDir[len] = '\\';
            m_sProfilesDir[len+1] = 0;
         }
         strcat(m_sProfilesDir, "KMeleon");
      }
   }  
}

int CCmdLine::GetSwitch(const char *pSwitch, char *pArgs, BOOL bRemove) {

   char *p, *c;
   char *pQuote;
   char *pSwitchPos;
   char *pCmdLine;
   char *pArgStart, *pArgEnd;
   int iQuoteCount;
   BOOL bIsValidSwitch;
   int iSwitchLen, iArgLen=0;

   if ( !pSwitch || !*pSwitch )
      return -1;
   else
      iSwitchLen = strlen(pSwitch);

   p = pCmdLine = SkipWhiteSpace(m_sCmdLine);
   do {
      bIsValidSwitch = FALSE;
      
      if ( ((!p) || (!*p)) || !(pSwitchPos = strstr(p, pSwitch)) ) {
         if (pArgs) *pArgs = 0;
         return -1;
      }
      p = SkipWhiteSpace(pSwitchPos + iSwitchLen);

      // if this happens to be the first character on the command line,
      // then we don't need to do any error checking
      if (pSwitchPos == pCmdLine)
         bIsValidSwitch = TRUE;

      // make sure the flag is preceeded by whitespace
      else if ( (*(pSwitchPos-1) != ' ') && (*(pSwitchPos-1) != '\t') )
         continue;

      // make sure the character after the flag is whitespace or null
      else if ( (*(pSwitchPos+iSwitchLen) != ' ') && (*(pSwitchPos+iSwitchLen) != '\t') && (*(pSwitchPos+iSwitchLen) != 0)  )
         continue;

      // if the "argument" starts with a dash, it's another switch, and this
      // switch has no argument
      else if (*p == '-') {
         *pArgs = 0;
         return 0;
      }

      // make sure the switch we've found isn't inside quotation marks
      else {
         c = pCmdLine;
         iQuoteCount = 0;
         do {
            pQuote = strchr(c, '\"');
            if (pQuote) {
               if ( !( *(pQuote-1) == '\\') )
                  iQuoteCount++;
               c = pQuote+1;
            }
         } while ( pQuote && (pQuote < pSwitchPos) );      

         // there are 3 cases when the switch found will be valid
         // 1) if there are the no quotes found before the switch
         // 2) if there an odd number of quotes found, and the last quote is found after the switch
         // 3) if there are an even number of quotes found, and no quotes after the switch

         if (  (iQuoteCount == 0) || \
              ((pQuote) && (iQuoteCount%2)) || \
            ((!pQuote) && (!(iQuoteCount%2))) )
            bIsValidSwitch = TRUE;
      }
   } while (pSwitchPos && !bIsValidSwitch);

   if (pSwitchPos && bIsValidSwitch) {
      pArgStart = SkipWhiteSpace(p);
      
      // check if the argument is inside quotes
      if (*pArgStart == '\"') {
         pArgStart++;
         pArgEnd = strchr(pArgStart, '\"');
      }

      // find the first whitespace
      else {
         char *pTab   = strchr(pArgStart, '\t');
         char *pSpace = strchr(pArgStart, ' ');

         if (pTab && pSpace)
            pArgEnd = ( pTab > pSpace ? pSpace : pTab );
         else
            pArgEnd = ( pTab ? pTab : pSpace );
      }

      if (pArgEnd)
         iArgLen = pArgEnd-pArgStart;
      else
         iArgLen = strlen(pArgStart);

      if (pArgs) {
         if (iArgLen) {
            memcpy(pArgs, pArgStart, iArgLen);
            pArgs[iArgLen] = 0;
         }
         else
            *pArgs = 0;
      }
      
      if (bRemove) {
         char *pNewData;
         if (pArgs)
            pNewData = pArgEnd ? pArgEnd+1 : pArgStart + iArgLen;
         else
            pNewData = pSwitchPos + iSwitchLen;

         pNewData = SkipWhiteSpace(pNewData);
         while (*pNewData)
            *pSwitchPos++ = *pNewData++;
         *pSwitchPos = 0;
      }

   }

   return iArgLen;
}

