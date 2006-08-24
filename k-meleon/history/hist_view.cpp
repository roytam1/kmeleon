/*
 * Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef __MINGW32__
#  define _WIN32_IE 0x0500
#  include <windows.h>
#  include <commctrl.h>
#  include "../missing.h"
#endif

#include "history.h"
#include "HistoryNode.h"
#include "../kmeleon_plugin.h"
#include "../KMeleonConst.h"
// Include for global history
#include "nsCOMPtr.h"
#include "nsEmbedString.h"
#include "rdf.h"
#include "nsServiceManagerUtils.h"
#include "nsISimpleEnumerator.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFService.h"
#include "prtime.h"
#include "nsMemory.h"

#include "../Utils.h"
#include <wininet.h>

#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_MINUS VK_OEM_MINUS
#define VK_PERIOD VK_OEM_PERIOD

extern kmeleonPlugin kPlugin;
extern void * KMeleonWndProc;

static CHistoryNode gHistoryRoot("", HISTORY_FOLDER, 0);
static CHistoryNode *workingHist;
static CHistoryNode *freeNode;
static HWND hEditWnd;

#define SEARCH_LEN 256
static char str[SEARCH_LEN];
static int len;
static int pos;
static int circling;

static BOOL zoom = 0;
static BOOL maximized;
static int sortOrder;

#define CUT 1
#define KILL 1
#define PASTE 1

#define SORT_BY_DATE 0
#define SORT_BY_URL  1
#define SORT_BY_DATE_BACKWARDS 2

static void FillTree(HWND hTree, HTREEITEM parent, CHistoryNode &node, int level);
static void DeleteItem(HWND hTree, HTREEITEM item);
static void OnSize(int height, int width);
static void OnRClick(HWND hTree);


static inline CHistoryNode *GetHistoryNode(HWND hTree, HTREEITEM htItem) {
   TVITEMEX tvItem;
   tvItem.mask = TVIF_PARAM;
   tvItem.hItem = htItem;
   TreeView_GetItem(hTree, &tvItem);

   return (CHistoryNode *)tvItem.lParam;
}

static inline void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst) {
   // Note that LONGLONG is a 64-bit value
   LONGLONG ll;
   FILETIME ft1;
   FILETIME ft2;

#ifdef __MINGW32__
   ll = Int32x32To64(t, 10000000) + 116444736000000000LL;
#else
   ll = Int32x32To64(t, 10000000) + 116444736000000000L;
#endif
   ft1.dwLowDateTime = (DWORD)ll;
   ft1.dwHighDateTime = ll >> 32;

   FileTimeToLocalFileTime(&ft1, &ft2);
   FileTimeToSystemTime(&ft2, pst);
}


static int parseInt64(char *buf, LONGLONG *outValue)
{
  int c, charsRead = 0;
  LONGLONG value = 0;

  while (buf && *buf) {
    c = *buf;
    if (c == 0 || !isdigit(c))
      break;
      
    ++charsRead;
    LONGLONG digit = c - '0';
    value *= 10L;
    value += digit;
  }
  if (!charsRead)
    return -1;
  *outValue = value;
  return 0;
}


// Hook function for keyboard hook
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
   if ( (nCode != HC_ACTION) || (lParam & 0x80000000) ) {   // highest bit of lParam high == key being released
      return CallNextHookEx(NULL, nCode, wParam, lParam);
   }

   BOOL fEatKeystroke = false;
   BOOL fakedKey = false;
   HWND hasFocus = GetFocus();
   HWND hTree = GetDlgItem(hEditWnd, IDC_TREE_HOTLIST);

   if (!(hasFocus == hTree ||
         hasFocus == GetDlgItem(hEditWnd, IDC_URL) ||
         hasFocus == GetDlgItem(hEditWnd, IDOK) ||
         hasFocus == GetDlgItem(hEditWnd, IDCANCEL)))
      return  CallNextHookEx(NULL, nCode, wParam, lParam);

   int searching = 0;

   if (wParam == VK_ESCAPE) {
      fEatKeystroke = true;
      if (len == 0)
         SendMessage(GetDlgItem(hEditWnd, IDCANCEL), BM_CLICK, 0, 0);
      else
         len = 0;
   }
   else if (hasFocus == hTree) {
      HTREEITEM hItem = TreeView_GetSelection(hTree);
      if (hItem) {
         switch (wParam) {
         case VK_UP:
            if (GetKeyState(VK_MENU) & 0x80) {   // what genius decided to call ALT "VK_MENU"???
               fEatKeystroke = true;
               HTREEITEM hItemSelect = TreeView_GetPrevSibling(hTree, hItem);
               if (!hItemSelect) hItemSelect = TreeView_GetPrevVisible(hTree, hItem);
               if (!hItemSelect) break;
               TreeView_SelectItem(hTree, hItemSelect);
            }
            break;
         case VK_DOWN:
            if (GetKeyState(VK_MENU) & 0x80) {
               fEatKeystroke = true;
               HTREEITEM hItemSelect = TreeView_GetNextSibling(hTree, hItem);
               if (!hItemSelect) hItemSelect = TreeView_GetNextSibling(hTree, TreeView_GetParent(hTree, hItem));
               if (!hItemSelect) hItemSelect = TreeView_GetNextVisible(hTree, hItem);
               if (!hItemSelect) break;
               TreeView_SelectItem(hTree, hItemSelect);
            }
            break;
         case VK_RETURN:
            {
               fEatKeystroke = true;
               CHistoryNode *node = GetHistoryNode(hTree, hItem);
               if (node->type == HISTORY_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  if (GetKeyState(VK_CONTROL) & 0x80) {
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUND, NULL);
                  }
                  else {
                     kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
                  }
                  TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
                  PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
               }
               else if (node->type == HISTORY_FOLDER) {
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
               }
            }
            break;
         case VK_TAB:
            {
               if (len > 0) {
                  wParam = str[len-1];
                  wParam = toupper(wParam);
                  fakedKey = true;
                  goto search_again;
               }

               fEatKeystroke = true;
               if (zoom)
                  break;

               int idc_next = IDC_URL;
               if (IsWindowEnabled(GetDlgItem(hEditWnd, IDC_URL)) == 0)
                  idc_next = IDOK;
               if (GetKeyState(VK_SHIFT) & 0x80)
                  idc_next = IDCANCEL;
               if (idc_next == IDCANCEL || idc_next == IDOK)
                  SendMessage(GetDlgItem(hEditWnd, idc_next), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
               SetFocus(GetDlgItem(hEditWnd, idc_next));
               if (idc_next != IDCANCEL && idc_next != IDOK)
                  SendDlgItemMessage(hEditWnd, idc_next, EM_SETSEL, 0, -1);  // select all
            }
            break;
         case VK_DELETE:
            // DeleteItem(hTree, hItem);
            break;
         case ' ':
            {
               CHistoryNode *node = GetHistoryNode(hTree, hItem);
               if (node->type == HISTORY_FOLDER && len == 0) {
                  fEatKeystroke = true;
                  TreeView_Expand(hTree, hItem, TVE_TOGGLE);
                  break;
               }
            }
            // Fall through!
         default:
            {
               if (GetKeyState(VK_CONTROL) & 0x80) {
                  switch (wParam) {
                  case 'U':
                     len = 0;
                     break;
                  case 'G':
                     if (len > 0) {
                        wParam = str[len-1];
                        wParam = toupper(wParam);
                        fakedKey = true;
                        goto search_again;
                     }
                     break;
                  }
                  break;
               }
               else if (GetKeyState(VK_MENU) & 0x80) {
                  break;
               }
               else {
                  switch (wParam) {
                  case VK_F3:
                     if (len > 0) {
                        wParam = str[len-1];
                        wParam = toupper(wParam);
                        fakedKey = true;
                        goto search_again;
                     }
                     break;
                  }
               }

               if (wParam == VK_BACK && len > 0) {
                  circling = 0;
                  len--;
               }
               else if ( (wParam == ' ' || 
                          (wParam >= '0' && wParam <= '9') || 
                          (wParam >= 'A' && wParam <= 'Z')) && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = tolower(wParam);
                  len++;
               }
               else if ( (wParam == VK_MINUS) && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = '-';
                  len++;
               }
               else if ( wParam == VK_PERIOD && 
                         len < (SEARCH_LEN-1) ) {
                  str[len] = '.';
                  len++;
               }
               else
                  break;

            search_again:
               fEatKeystroke = true;
               searching = 1;
               str[len] = 0;
               if (len > 0) {
                  CHistoryNode *node;

                  int mypos = 0;
                  int firstpos = -1;
                  int searchfrom = pos;

                  if (len == 1 && !fakedKey) {
                     HTREEITEM hItem = TreeView_GetSelection(hTree);
                     node = GetHistoryNode(hTree, hItem);
                     if (workingHist->Index(searchfrom, node) == -1)
                        searchfrom = pos;
                     else
                        pos = searchfrom;
                  }

                  int newpos = workingHist->Search(str, searchfrom, mypos, firstpos, &node);

                  if (fakedKey || newpos == -1 || 
                      (circling && len > 1 && str[len-1] == str[len-2])) {
                     if (fakedKey || (len > 1 && str[len-1] == str[len-2]))
                        searchfrom++;
                     if (!fakedKey)
                        len--;
                     str[len] = 0;
                     mypos = 0;
                     firstpos = -1;
                     if (len > 0) {
                        newpos = workingHist->Search(str, searchfrom, mypos, firstpos, &node);
                        if (!fakedKey && (newpos == pos || (len > 1 && str[len-1] == str[len-2]))) {
                           circling = 1;
                           while (len > 1 && str[len-1] == str[len-2])
                              len--;
                           str[len] = 0;
                           mypos = 0;
                           firstpos = -1;
                           newpos = workingHist->Search(str, searchfrom, mypos, firstpos, &node);
                        }
                     }
                  }
                  else 
                     circling = 0;

                  if (newpos == -1 || len == 0) {
                     pos = 0;
                     len = 0;
                     str[len] = 0;
                     searching = 0;
                  }
                  else {
                     TCHAR tmp[SEARCH_LEN+64];
					 _stprintf(tmp, _Tr("History  -- Find: \"%s\""), str);
                     SetWindowText( hEditWnd, tmp );

                     HTREEITEM hItem = TreeView_GetRoot(hTree);
                     int i = 1;
                     while (hItem && i < newpos) {
                        HTREEITEM hTmp = TreeView_GetChild(hTree, hItem);
                        if (!hTmp)
                           hTmp = TreeView_GetNextSibling(hTree, hItem);
                        if (!hTmp) {
                           HTREEITEM hTmp2 = TreeView_GetParent(hTree, hItem);
                           while (!hTmp && hTmp2) {
                              hTmp = TreeView_GetNextSibling(hTree, hTmp2);
                              hTmp2 = TreeView_GetParent(hTree, hTmp2);
                           }
                        }
                        hItem = hTmp;
                        i++;
                     }

                     if (hItem) {
                        TreeView_SelectItem(hTree, hItem);
                        TreeView_EnsureVisible(hTree, hItem);
                     }

                     pos = newpos;
                  }
               }
            }
         }
      }
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDOK) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, IDOK), BM_CLICK, 0, 0);
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDCANCEL) && wParam == VK_RETURN) {
      SendMessage(GetDlgItem(hEditWnd, IDCANCEL), BM_CLICK, 0, 0);
   }
   else if (hasFocus == GetDlgItem(hEditWnd, IDC_URL) && wParam == VK_RETURN) {
      fEatKeystroke = true;
      SetFocus(GetDlgItem(hEditWnd, IDC_TREE_HOTLIST));
   }
   else if (wParam == VK_TAB) {
      int fields[] = {IDC_TREE_HOTLIST, IDC_URL, 
                      IDOK, IDCANCEL, IDC_TREE_HOTLIST};
      if (GetDlgItem(hEditWnd, IDOK) == hasFocus ||
          GetDlgItem(hEditWnd, IDCANCEL) == hasFocus)
         SendMessage(hasFocus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
      for (int i=1; fields[i]!=IDC_TREE_HOTLIST; i++) {
         if (GetDlgItem(hEditWnd, fields[i]) == hasFocus) {
            int idc_next = fields[i + ((GetKeyState(VK_SHIFT) & 0x80) ? -1 : 1)];
            if (idc_next == IDC_TREE_HOTLIST || 
                idc_next == IDOK || idc_next == IDCANCEL || 
                IsWindowEnabled(GetDlgItem(hEditWnd, idc_next))) {
               fEatKeystroke = true;
               if (idc_next == IDOK || idc_next == IDCANCEL)
                  SendMessage(GetDlgItem(hEditWnd, idc_next), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
               SetFocus(GetDlgItem(hEditWnd, idc_next));
               if (idc_next != IDC_TREE_HOTLIST && 
                   idc_next != IDOK && idc_next != IDCANCEL)
                  SendDlgItemMessage(hEditWnd, idc_next, EM_SETSEL, 0, -1);  // select all
               break;
            }
            else {
               hasFocus = GetDlgItem(hEditWnd, idc_next);
               i = 0;
               continue;
            }
         }
      }
   }

   if ((searching == 0 || len==0) && 
       wParam != VK_SHIFT && wParam != VK_MENU && wParam != VK_CONTROL) {
      len = 0;
      pos = 0;
      str[len] = 0;
      SetWindowText( hEditWnd, _Tr("History") );
      circling = 0;
   }

   return(fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam));
}
/*
int ParseHistoryEntry(char **p, CHistoryNode &node) 
{
   char *q = NULL;
   char *r1 = NULL;
   char *r2 = NULL;
   char szURL[INTERNET_MAX_URL_LENGTH] = {0};
   time_t lastVisit=0;

   if (*p && **p && (q = strchr(*p, ':')) != NULL) {
      *q++ = 0;

      // Ugly conversion from milliseconds to seconds..
      if (q-(*p) > 4)
         *(q-4) = 0;

      lastVisit = atol(*p);
      strncpy(szURL, q, INTERNET_MAX_URL_LENGTH);
      szURL[INTERNET_MAX_URL_LENGTH-1] = 0;

      CHistoryNode * newNode = 
         new CHistoryNode(szURL, NULL, HISTORY_BOOKMARK, lastVisit);
      if (newNode) {
         node.AddChild(newNode);
         return 1;
      } 
   }

   return 0;
}

int ParseHistory(char **p, CHistoryNode &node) 
{
   int size = 0;
   char *q1 = strchr(*p, '\r');
   char *q2 = strchr(*p, '\n');
   
   while (*p && **p) {
      if (!q1 || (q2 && q2 < q1)) 
         q1 = q2;
      if (q1)
         *q1++ = 0;
      
      while (*p && **p && isspace(**p))
         (*p)++;
      
      if (*p && isdigit(**p))
         size += ParseHistoryEntry(p, node);
      
      if ( (*p = q1) != NULL ) {
         q1 = strchr(*p, '\r');
         q2 = strchr(*p, '\n');
      }
   }

   *p = q1;
   return size;
}

*/

int HistoryAttr(nsCOMPtr<nsIRDFNode> target, TCHAR **v)
{
	nsCOMPtr<nsIRDFLiteral> aR = do_QueryInterface(target);
	if (aR)
	{
		PRUnichar *name;

		aR->GetValue(&name);
		int len;
		len = WideCharToMultiByte(CP_ACP, 0, name, -1, 0, 0, NULL, NULL);
		char *s = new char[len +1];
		len = WideCharToMultiByte(CP_ACP, 0, name, -1, s, len, NULL, NULL);
		s[len] = 0;
		*v = s;
		nsMemory::Free(name);

		
		return true;
	}
	else
	{	
		*v = NULL;
		return false;
	}
}

int readHistory() {

   nsresult rv;
   unsigned int HistorySize = 0;

   while (gHistoryRoot.child)
		gHistoryRoot.RemoveChild();
   while (gHistoryRoot.next)
	    gHistoryRoot.RemoveNext();
	
   nsCOMPtr<nsIRDFDataSource> HistoryRdfDataSource;
   nsCOMPtr<nsISupports> isupports;

	nsCOMPtr<nsIRDFService> RDFService(do_GetService("@mozilla.org/rdf/rdf-service;1", &rv));
	if (NS_FAILED(rv)) return false;

	nsIRDFResource *arcChild;
	nsIRDFResource *HistoryRoot;
	nsIRDFResource *arcName;
	nsIRDFResource *arcURL;
	nsIRDFResource *arcDate;

	rv = RDFService->GetResource(nsDependentCString("NC:HistoryRoot"),&HistoryRoot);
	if (NS_FAILED(rv)) return false;

	rv = RDFService->GetDataSourceBlocking("rdf:history",getter_AddRefs(HistoryRdfDataSource));
	if (NS_FAILED(rv)) return false;

	rv = RDFService->GetResource(nsCString("http://home.netscape.com/NC-rdf#child"), &arcChild);
	if (NS_FAILED(rv)) return false;

	rv = RDFService->GetResource(nsCString("http://home.netscape.com/NC-rdf#URL"), &arcURL);
	if (NS_FAILED(rv)) return false;
	rv = RDFService->GetResource(nsCString("http://home.netscape.com/NC-rdf#Name"), &arcName);
	if (NS_FAILED(rv)) return false;
	rv = RDFService->GetResource(nsCString("http://home.netscape.com/NC-rdf#Date"), &arcDate);
	if (NS_FAILED(rv)) return false;

	nsCOMPtr<nsISimpleEnumerator> targets;
	rv = HistoryRdfDataSource->GetTargets(HistoryRoot,arcChild,true,getter_AddRefs(targets));
	if (NS_FAILED(rv)) return false;

	// Another way to count the number
	// of history entries?
	
   while ( (rv=targets->GetNext(getter_AddRefs(isupports))) == NS_OK) 
		HistorySize++;

   // Should be NS_ERROR_FAILURE...
    if (rv != NS_ERROR_UNEXPECTED) return false;

   	rv = HistoryRdfDataSource->GetTargets(HistoryRoot,arcChild,true,getter_AddRefs(targets));
	if (NS_FAILED(rv)) return false;
	
	for (unsigned int i=0; i < 	HistorySize; i++) 
	{
		PRTime date;
		TCHAR *name = NULL;
		TCHAR *URL = NULL;
		
		nsCOMPtr<nsISupports> isupports;
		rv = targets->GetNext(getter_AddRefs(isupports));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIRDFResource> aTarget = do_QueryInterface(isupports);

		nsCOMPtr<nsIRDFNode> target;

		rv = HistoryRdfDataSource->GetTarget(aTarget,arcDate,true,getter_AddRefs(target));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIRDFDate> nDate;
		nDate = do_QueryInterface(target);
		if (nDate)
		{
			rv = nDate->GetValue(&date);
			if (NS_FAILED(rv)) break;
		}
		
		rv = HistoryRdfDataSource->GetTarget(aTarget,arcName,true,getter_AddRefs(target));
		if (NS_FAILED(rv)) break;
		HistoryAttr(target, &name);

		rv = HistoryRdfDataSource->GetTarget(aTarget,arcURL,true,getter_AddRefs(target));
		if (NS_FAILED(rv)) {delete name;break;}
		HistoryAttr(target, &URL);
		
		time_t t =  date / PR_USEC_PER_SEC;
		CHistoryNode * newNode = new CHistoryNode(URL, name, HISTORY_BOOKMARK, t);
		if (newNode) gHistoryRoot.AddChild(newNode);

		delete name;
		delete URL;
	}
   
   return true;
   /*
// char szHistFile[MAX_PATH];
   kPlugin.kFuncs->GetPreference(PREF_STRING, "kmeleon.general.profileDir", szHistFile, (char*)"");
   strcat(szHistFile, "history.txt");
   FILE *hFile = fopen(szHistFile, "r");

   if (hFile){
      long hFileSize = FileSize(hFile);
      
      char *hFileBuffer = new char[hFileSize+1];
      if (hFileBuffer){

         fread(hFileBuffer, sizeof(char), hFileSize, hFile);
         hFileBuffer[hFileSize] = 0;
         
         char *szTmpBuf = hFileBuffer;
         ret = ParseHistory(&szTmpBuf, gHistoryRoot);
         
         delete [] hFileBuffer;
      }
      fclose(hFile);
   }
   
   return ret;*/
}


CHistoryNode *groupURLs(CHistoryNode *oldList)
{
   if (!oldList)
      return NULL;

   CHistoryNode *newList;
   CHistoryNode *tmp;
   newList = new CHistoryNode(oldList->url.c_str(), oldList->name.c_str(), HISTORY_FOLDER, 0);

   sortOrder = SORT_BY_URL;
   oldList->flatten();
   oldList->sort(sortOrder);
   tmp = oldList->child;

   while (tmp) {
      int len;
      char *str = (char*)tmp->url.c_str();
      char *p = strchr(str, '/');
      if (p && *(p+1) == '/')
         p = strchr(p+2, '/');
      if (p) {
         len = p - str;
         *p = 0;
      }
      else 
         len = strlen(str);
      
      CHistoryNode *newFolder = 
         new CHistoryNode(str, str, HISTORY_FOLDER, 0);
      newList->AddChild(newFolder);

      if (p) {
         *p = '/';
      }
      
      while (tmp && strncmp(str, tmp->url.c_str(), len) == 0) {
         CHistoryNode *newNode = new CHistoryNode(tmp->url.c_str(), tmp->name.c_str(), HISTORY_BOOKMARK, tmp->lastVisit);
         newFolder->AddChild(newNode);
         tmp = tmp->next;
      }
   }
   while (oldList->child)
		oldList->RemoveChild();
   while (oldList->next)
	    oldList->RemoveNext();

   oldList->child = newList->child;
   oldList->next = newList->next;
   return newList;
}


struct tm subSec(struct tm t, int sec) {
   time_t date = mktime(&t);
   date -= sec;
   struct tm *pt = localtime(&date);
   return *pt;
}

struct tm subMin(struct tm t, int min) {
   return subSec(t, 60*min);
}

struct tm subHour(struct tm t, int h) {
   return subMin(t, 60*h);
}

struct tm subDay(struct tm t, int d) {
   return subHour(t, 24*d);
}

struct tm subMonth(struct tm t, int m) {
   t.tm_mon -= m;
   while (t.tm_mon < 0) {
      t.tm_mon += 12;
      t.tm_year--;
   }

   time_t date = mktime(&t);
   struct tm *pt = localtime(&date);
   return *pt;
}

struct tm subYear(struct tm t, int y) {
   struct tm newt = {0};

   t.tm_year -= y;
   time_t date = mktime(&t);
   struct tm *pt = localtime(&date);

   if (pt)
     newt = *pt;

   return newt;
}


CHistoryNode *groupDates(CHistoryNode *oldList)
{
   const char *weekday[] = {_Tr("Sunday"), _Tr("Monday"), _Tr("Tuesday"), _Tr("Wednesday"), _Tr("Thursday"), _Tr("Friday"), _Tr("Saturday")};
   const char *month[] = {_Tr("January"), _Tr("February"), _Tr("March"), _Tr("April"), _Tr("May"), _Tr("June"), _Tr("July"), _Tr("August"), _Tr("September"), _Tr("October"), _Tr("November"), _Tr("December")};

   if (!oldList)
      return NULL;

   CHistoryNode *newList;
   CHistoryNode *tmp;
   newList = new CHistoryNode(oldList->url.c_str(), oldList->name.c_str(), HISTORY_FOLDER, 0);

   sortOrder = SORT_BY_DATE;
   oldList->flatten();
   oldList->sort(SORT_BY_DATE_BACKWARDS);
   tmp = oldList->child;

   int pass = 0;
   time_t date = time(NULL);

   struct tm *pt = localtime(&date);
   struct tm n = *pt;

   while (tmp) {
      char str[100];
      strcpy(str, _Tr("<error>"));

      pt = localtime(&date);
      struct tm t = *pt;

      switch (pass) {
        case 0:
           t.tm_sec = 0;
           t.tm_min = 0;
           strcpy(str, _Tr("Last Hour"));
           break;
        case 1: 
           t.tm_hour = 0;
           strcpy(str, _Tr("Today"));
           break;
        case 2:
        case 3:
        case 4:
           t = subDay(t, 1);
           strcpy(str, weekday[t.tm_wday]);
           break;
        case 5:
           if (t.tm_wday < n.tm_wday) {
              t = subDay(t, t.tm_wday);
              strcpy(str, _Tr("This Week"));
           }
           else {
              t = subDay(t, t.tm_wday);
              strcpy(str, _Tr("Last Week"));
              pass++;
           }
           break;
        case 6:
           t = subDay(t, 7);
           strcpy(str, _Tr("Last Week"));
           break;
        case 7:
           t = subDay(t, 7);
           strcpy(str, _Tr("2 Weeks Ago"));
           break;
        case 8:
           t = subDay(t, 7);
           strcpy(str, _Tr("3 Weeks Ago"));
           break;
        case 9:
           if (t.tm_mday < n.tm_mday) {
              t = subDay(t, (t.tm_mday-1));
              strcpy(str, month[t.tm_mon]);
           }
           else {
              t = subDay(t, (t.tm_mday-1));
              strcpy(str, month[t.tm_mon]);
              pass++;
           }
           break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
           t = subMonth(t, 1);
           strcpy(str, month[t.tm_mon]);
           break;
        case 16:
           if (t.tm_yday > n.tm_yday)
              break;
           t = subMonth(t, t.tm_mon);
           itoa(t.tm_year, str, 10);
           break;
        default:
           t = subYear(t, 1);
	   if (t.tm_year)
	     itoa(1900 + t.tm_year, str, 10); 
	   else
	     strcpy(str, _Tr("Never"));
          break;
      }

      date = mktime(&t);

      if (tmp->lastVisit >= date) {

         CHistoryNode *newFolder = new CHistoryNode(str, str, HISTORY_FOLDER, 0);
         while (tmp && (tmp->lastVisit > date)) {
			 CHistoryNode *newNode = new CHistoryNode(tmp->url.c_str(), tmp->name.c_str(), HISTORY_BOOKMARK, tmp->lastVisit);
            newFolder->AddChild(newNode);
            tmp = tmp->next;
         }

         CHistoryNode *tmpFolder = groupURLs(newFolder);
         newList->AddChild(tmpFolder);

         tmpFolder = tmpFolder->child;
         while (tmpFolder) {
            tmpFolder->sort(SORT_BY_DATE);
            tmpFolder = tmpFolder->next;
         }

         newFolder->child = NULL;
         newFolder->next = NULL;
         delete newFolder;
      }

      pass++;
   }

   while (oldList->child)
		oldList->RemoveChild();
   while (oldList->next)
	    oldList->RemoveNext();

   oldList->child = newList->child;
   oldList->next = newList->next;
   sortOrder = SORT_BY_DATE;
   return newList;
}


int CALLBACK ViewProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static int error;
   static HHOOK hHook;
   static HWND hTree;

   switch (uMsg){
   case WM_INITDIALOG:
      {
         if (!readHistory()) {
            MessageBox(NULL, _Tr("Mozilla history reading failed"), _Tr("History"), MB_OK);
            ghWndView = NULL;
            error = 1;
            PostMessage(GetDlgItem(hDlg, IDCANCEL), BM_CLICK, 0, 0);
            EndDialog(hDlg, 0);
            return 0;
         }

         error = 0;
         hEditWnd = hDlg;
         len = 0;
         str[len] = 0;
         pos = 0;
         circling = 0;
         
         freeNode = new CHistoryNode(_Tr("Delete"), _Tr("Delete"), HISTORY_FOLDER, 0);

         HICON hIcon;
         TCHAR szFullPath[MAX_PATH];
         FindSkinFile(szFullPath, _T("history-view.ico"));

         if (*szFullPath==0 || (hIcon = (HICON)LoadImage( NULL, szFullPath, IMAGE_ICON, 0,0, LR_DEFAULTSIZE | LR_LOADFROMFILE ))==NULL)
            hIcon = (HICON)LoadImage( kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 0,0, LR_DEFAULTSIZE );
         if (hIcon)
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

         if (*szFullPath==0 || (hIcon = (HICON)LoadImage( NULL, szFullPath, IMAGE_ICON, 16,16, LR_LOADFROMFILE ))==NULL)
            hIcon = (HICON)LoadImage( kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON), IMAGE_ICON, 16,16, 0 );
         if (hIcon)
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

         hTree = GetDlgItem(hDlg, IDC_TREE_HOTLIST);
         TreeView_SetImageList(hTree, gImagelist, TVSIL_NORMAL);

         sortOrder = SORT_BY_DATE;
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_VIEW_SORT_ORDER, &sortOrder, &sortOrder);  
         if (sortOrder == SORT_BY_DATE)
            workingHist = groupDates(&gHistoryRoot);
         else
            workingHist = groupURLs(&gHistoryRoot);

         TVINSERTSTRUCT tvis;
         tvis.hParent = NULL;
         tvis.hInsertAfter = NULL;
         tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
         tvis.itemex.iImage = IMAGE_FOLDER_SPECIAL_CLOSED;
         tvis.itemex.iSelectedImage = IMAGE_FOLDER_SPECIAL_OPEN;
         tvis.itemex.pszText = (TCHAR*)_Tr("All URLs");
         tvis.itemex.lParam = (long)&workingHist;

         // HTREEITEM newItem = TreeView_InsertItem(hTree, &tvis);

         FillTree(hTree, NULL, *workingHist, sortOrder == SORT_BY_URL);

         TreeView_SelectItem(hTree, TreeView_GetRoot(hTree));

         int dialogleft = 50, dialogtop = 50, dialogwidth = 500, dialogheight = 500;
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_VIEW_DLG_LEFT, &dialogleft, &dialogleft);  
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_VIEW_DLG_TOP, &dialogtop, &dialogtop);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_VIEW_DLG_WIDTH, &dialogwidth, &dialogwidth);
         kPlugin.kFuncs->GetPreference(PREF_INT, PREFERENCE_VIEW_DLG_HEIGHT, &dialogheight, &dialogheight);
         SetWindowPos(hDlg, 0, dialogleft, dialogtop, dialogwidth, dialogheight, 0);
         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_VIEW_ZOOM, &zoom, &zoom);
         kPlugin.kFuncs->GetPreference(PREF_BOOL, PREFERENCE_VIEW_MAX, &maximized, &maximized);
         SetWindowPos(hDlg, 0, dialogleft, dialogtop, dialogwidth, dialogheight, 0);
         if (maximized)
            ShowWindow(hDlg, SW_MAXIMIZE);
         else
            ShowWindow(hDlg, SW_NORMAL);

         hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, GetCurrentThreadId());
         RECT rect;
         GetClientRect(hDlg, &rect);
         OnSize(rect.bottom, rect.right);

         TreeView_SelectItem(hTree, TreeView_GetRoot(hTree));  // just to fire off a SELCHANGED notifier to update the view (zoom)
         if (sortOrder == SORT_BY_DATE)
            TreeView_Expand(hTree, TreeView_GetRoot(hTree), TVE_EXPAND);
      }
      return false;

   case WM_NOTIFY:
      {
         NMTREEVIEW *nmtv = (NMTREEVIEW *)lParam;

         // Selection changed
         if (nmtv->hdr.code == TVN_SELCHANGED){
            // Put the new url/title into the box
            CHistoryNode *newNode = (CHistoryNode *)nmtv->itemNew.lParam;

            if (newNode == workingHist) {
               // root and separators have nothing
               SetDlgItemText(hDlg, IDC_URL, _T(""));
               SetDlgItemText(hDlg, IDC_LAST_VISIT, _T(""));
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), false);
               return true;   // a lazy-man's "else"
            }

            // everything else has at least title, added date, and properties in general
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PROPERTIES), true);

            SYSTEMTIME st;
            TCHAR pszTmp[1024];
            TCHAR pszDate[900];

            if (newNode->type == HISTORY_BOOKMARK) {
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), true);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), true);
               SetDlgItemText(hDlg, IDC_URL, newNode->url.c_str());
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), true);
               EnableWindow(GetDlgItem(hDlg, IDC_LAST_VISIT), true);
            }
            else {
               SetDlgItemText(hDlg, IDC_URL, _T(""));
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_URL), false);
               EnableWindow(GetDlgItem(hDlg, IDC_STATIC_VISITED), false);
               EnableWindow(GetDlgItem(hDlg, IDC_LAST_VISIT), false);
            }
            
            if (newNode->lastVisit) {
               UnixTimeToSystemTime(newNode->lastVisit, &st);
               GetDateFormat(GetThreadLocale(), DATE_SHORTDATE, &st, NULL, pszDate, 899);
               _tcscpy(pszTmp, pszDate);
               _tcscat(pszTmp, _T(" "));
               GetTimeFormat(GetThreadLocale(), NULL, &st, NULL, pszDate, 899);
               _tcscat(pszTmp, pszDate);
               SetDlgItemText(hDlg, IDC_LAST_VISIT, pszTmp);
            }
            else {
               SetDlgItemText(hDlg, IDC_LAST_VISIT, newNode->type == HISTORY_BOOKMARK ? _Tr("Never") : _T(""));
            }
         }
         else if (nmtv->hdr.code == (UINT) NM_DBLCLK){
            TVHITTESTINFO hti;
            GetCursorPos(&hti.pt);
            ScreenToClient(hTree, &hti.pt);

            HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
            if (hItem){
               CHistoryNode *node = GetHistoryNode(hTree, hItem);
               if (node->type == HISTORY_BOOKMARK) {
                  node->lastVisit = time(NULL);
                  kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
                  TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
                  PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);

                  return true;
               }
            }
         }
         // right click...
         else if (nmtv->hdr.code == (UINT) NM_RCLICK){
            OnRClick(hTree);
         }
      }
      break;
      
   case WM_SIZE:
      {
         if(wParam != SIZE_MINIMIZED) {
            RECT rect;            
            GetClientRect(hDlg, &rect);            
            OnSize(rect.bottom, rect.right);
         }
      }
      break;

   case WM_GETMINMAXINFO:
      LPMINMAXINFO lpminmaxinfo;
      lpminmaxinfo=(LPMINMAXINFO)lParam;
      lpminmaxinfo->ptMinTrackSize.x = 195;
      lpminmaxinfo->ptMinTrackSize.y = 300;
      break;

   case WM_COMMAND:
      {
         if (HIWORD(wParam) == BN_CLICKED) {
            WORD id = LOWORD(wParam);
            switch(id){

            case IDCANCEL:
               if (error) {
                  error = 0;
                  ghWndView = NULL;
                  if (hWndFront)
                     PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
                  DestroyWindow(hDlg);
                  break;
               }
               if (!zoom) {
                  ghWndView = NULL;

                  if (gHistoryRoot.child)
                     delete gHistoryRoot.child;
                  if (gHistoryRoot.next)
                     delete gHistoryRoot.next;
                  gHistoryRoot.child = NULL;
                  gHistoryRoot.next = NULL;

                  UnhookWindowsHookEx(hHook);
                  if (hWndFront)
                     PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
				  DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
				  DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));
                  DestroyWindow(hDlg);
                  break;
               }
               // fall through!

            case IDOK:
            {
               WINDOWPLACEMENT wp;
               wp.length = sizeof(wp);
               GetWindowPlacement(hDlg, &wp);

               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_VIEW_DLG_LEFT, &wp.rcNormalPosition.left, FALSE);
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_VIEW_DLG_TOP, &wp.rcNormalPosition.top, FALSE);
               int temp;
               temp = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_VIEW_DLG_WIDTH, &temp, FALSE);
               temp = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_VIEW_DLG_HEIGHT, &temp, FALSE);
               kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_VIEW_ZOOM, &zoom, FALSE);
               temp = (wp.showCmd == SW_SHOWMAXIMIZED);
               kPlugin.kFuncs->SetPreference(PREF_BOOL, PREFERENCE_VIEW_MAX, &temp, FALSE);

               kPlugin.kFuncs->SetPreference(PREF_INT, PREFERENCE_VIEW_SORT_ORDER, &sortOrder, FALSE);

               ghWndView = NULL;

               if (gHistoryRoot.child)
                  delete gHistoryRoot.child;
               if (gHistoryRoot.next)
                  delete gHistoryRoot.next;
               gHistoryRoot.child = NULL;
               gHistoryRoot.next = NULL;

               UnhookWindowsHookEx(hHook);
               if (hWndFront)
                  PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
			   DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
			   DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));
               DestroyWindow(hDlg);

               // Delete all selected history entries
               if (freeNode && freeNode->child) {
                  // This is not likely to be implemented in a while..

                  MessageBox(NULL, _Tr("Deletion is not implemented"), _Tr("History"), MB_OK);

                  /*
                  freeNode->flatten();
                  CHistoryNode *ptr = freeNode->child;
                  while (ptr) {
                     MessageBox(NULL, ptr->url.c_str(), "Delete", MB_OK);
                     ptr = ptr->next;
                  }
                  */
               }
               if (freeNode)
                  delete freeNode;

               break;
            }
            }
         }
      }
      break;
   }

   return false;
}

static void FillTree(HWND hTree, HTREEITEM parent, CHistoryNode &node, int level)
{
   TVINSERTSTRUCT tvis;
   tvis.hParent = parent;
   tvis.hInsertAfter = NULL;
   tvis.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   tvis.itemex.iImage = IMAGE_BLANK;
   tvis.itemex.iSelectedImage = IMAGE_BLANK;

   int type;
   CHistoryNode *child;
   for (child=node.child; child; child=child->next) {
      tvis.itemex.lParam = (long)child;

      type = child->type;
      if (type == HISTORY_FOLDER) {
         tvis.itemex.iImage = level == 0 ? IMAGE_FOLDER_SPECIAL_CLOSED : IMAGE_FOLDER_CLOSED;
         tvis.itemex.iSelectedImage = level == 0 ? IMAGE_FOLDER_SPECIAL_OPEN : IMAGE_FOLDER_OPEN;
         tvis.itemex.pszText = (char *)child->name.c_str();

         HTREEITEM thisItem = TreeView_InsertItem(hTree, &tvis);
         
         FillTree(hTree, thisItem, *child, level+1);
      }
      else if (type == HISTORY_SEPARATOR) {
         tvis.itemex.iImage = IMAGE_BLANK;
         tvis.itemex.iSelectedImage = IMAGE_BLANK;
         tvis.itemex.pszText = _T("---");
         TreeView_InsertItem(hTree, &tvis);
      }
      else {
         tvis.itemex.iImage = IMAGE_BOOKMARK;
         tvis.itemex.iSelectedImage = IMAGE_BOOKMARK;
         tvis.itemex.pszText = (char *)child->name.c_str();
         TreeView_InsertItem(hTree, &tvis);
      }
   }
}


static void DeleteItem(HWND hTree, HTREEITEM item) {
   CHistoryNode *node, *parentNode;

   node = GetHistoryNode(hTree, item);

   HTREEITEM parent = TreeView_GetParent(hTree, item);
   if (parent){
      parentNode = GetHistoryNode(hTree, parent);
   }
   else{
      parentNode = workingHist;
   }

   if (freeNode && parentNode && parentNode->UnlinkNode(node))
      freeNode->AddChild(node);

   // select a new item
   HTREEITEM hSelect = TreeView_GetNextSibling(hTree, item);
   if (!hSelect) hSelect = TreeView_GetPrevSibling(hTree, item);
   if (!hSelect) hSelect = TreeView_GetParent(hTree, item);

   TreeView_DeleteItem(hTree, item);

   // must call selectitem after deleteitem, otherwise the SELCHANGED handler will try to update the deleted node!
   TreeView_SelectItem(hTree, hSelect);

}


static void OnRClick(HWND hTree)
{
   POINT mouse;
   GetCursorPos(&mouse);

   TVHITTESTINFO hti;
   hti.pt.x = mouse.x;
   hti.pt.y = mouse.y;
   ScreenToClient(hTree, &hti.pt);

   HTREEITEM hItem = TreeView_HitTest(hTree, &hti);
   if (hItem) {
      TreeView_SelectItem(hTree, hItem);

      HMENU topMenu = LoadMenu(kPlugin.hDllInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));
      HMENU contextMenu = GetSubMenu(topMenu, 0);

      CheckMenuItem(contextMenu, ID__SORT_DATE, MF_BYCOMMAND | MF_UNCHECKED);
      CheckMenuItem(contextMenu, ID__SORT_URL, MF_BYCOMMAND | MF_UNCHECKED);
      if (sortOrder == SORT_BY_DATE)
         CheckMenuItem(contextMenu, ID__SORT_DATE, MF_BYCOMMAND | MF_CHECKED);
      if (sortOrder == SORT_BY_URL)
         CheckMenuItem(contextMenu, ID__SORT_URL, MF_BYCOMMAND | MF_CHECKED);

      if (zoom)
	CheckMenuItem(contextMenu, ID__ZOOM, MF_BYCOMMAND | MF_UNCHECKED);
      else 
	CheckMenuItem(contextMenu, ID__ZOOM, MF_BYCOMMAND | MF_CHECKED);
      
      int command = TrackPopupMenu(contextMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD, mouse.x, mouse.y, 0, hTree, NULL);

      switch (command) {
      case ID__OPEN:
         {
            CHistoryNode *node = GetHistoryNode(hTree, hItem);
            if (node->type == HISTORY_BOOKMARK) {
               kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_NORMAL, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
               PostMessage(hWndFront, WM_COMMAND, wm_deferbringtotop, (LPARAM)NULL);
            }
         }
         break;
      case ID__OPEN_BACKGROUND:
         {
            CHistoryNode *node = GetHistoryNode(hTree, hItem);
            if (node->type == HISTORY_BOOKMARK) {
               kPlugin.kFuncs->NavigateTo(node->url.c_str(), OPEN_BACKGROUND, NULL);
               TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier to update the status (last visited!)
            }
         }
         break;
      case ID__BOOKMARK_DELETE:
         DeleteItem(hTree, hItem);
         break;
         break;

      case ID__SORT_URL:
      case ID__SORT_DATE:
         {
            if ((command == ID__SORT_URL && sortOrder == SORT_BY_URL) ||
                (command == ID__SORT_DATE && sortOrder == SORT_BY_DATE)) {
               break;
            }

            // TreeView_DeleteAllItems(hTree);

            TreeView_SelectItem(hTree, TreeView_GetRoot(hTree));
            HTREEITEM hTmp = TreeView_GetLastVisible(hTree);
            while (hTmp) {
               TreeView_DeleteItem(hTree, hTmp);
               hTmp = TreeView_GetLastVisible(hTree);
            }
            
            if (command == ID__SORT_URL) {
               workingHist = groupURLs(&gHistoryRoot);
            }
            else if (command == ID__SORT_DATE) {
               workingHist = groupDates(&gHistoryRoot);
            }
            
            FillTree(hTree, NULL, *workingHist, command == ID__SORT_URL);
            
            TreeView_SelectItem(hTree, TreeView_GetRoot(hTree));
            if (command == ID__SORT_DATE)
               TreeView_Expand(hTree, TreeView_GetRoot(hTree), TVE_EXPAND);

            break;
         }
      case ID__ZOOM:
         {
            zoom = !zoom;

            RECT rect;            
            GetClientRect(hEditWnd, &rect);            
            OnSize(rect.bottom, rect.right);

            TreeView_SelectItem(hTree, hItem);  // just to fire off a SELCHANGED notifier
         }
         break;
      }
   }
}


// this is ugly, see...  all this need to be changed each time the dialog is re-arranged...
#define BORDER 8  // this one is pixels, the rest are DLUs
#define BUTTON_HEIGHT 15
#define OK_WIDTH 45
#define CANCEL_WIDTH 45
#define PROPERTIES_HEIGHT 50
#define EDITBOXES_LEFT 36
#define EDITBOXES_TOP (276-174)
#define EDITBOXES_HEIGHT 12
#define DATES_WIDTH 78
#define OTHER_WIDTH 35

#define convX(x) MulDiv((int)(x), buX, 4)
#define convY(y) MulDiv((int)(y), buY, 8)

static void OnSize(int height, int width) {
   int buX, buY;
   RECT rc;    // GetDialogBaseUnits returns incorrect values...?
   SetRect(&rc, 0, 0, 4, 8);
   MapDialogRect(hEditWnd, &rc);
   buY = rc.bottom;
   buX = rc.right;

   // resize tree
   SetWindowPos(GetDlgItem(hEditWnd, IDC_TREE_HOTLIST), 0, 
                zoom ? 0 : BORDER, 
                zoom ? 0 : BORDER, 
                zoom ? width : width-(BORDER*2), 
                zoom ? height : height-BORDER*4-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                0);

   // move cancel button
   SetWindowPos(GetDlgItem(hEditWnd, IDCANCEL), 0, 
                width-BORDER-convX(CANCEL_WIDTH), 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);

   // move ok button
   SetWindowPos(GetDlgItem(hEditWnd, IDOK), 0, 
                width-BORDER*2-convX(CANCEL_WIDTH+OK_WIDTH), 
                zoom ? height + BORDER : height-BORDER-convY(BUTTON_HEIGHT), 
                0, 0, SWP_NOSIZE);

   // move/resize properties box
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_PROPERTIES), 0, 
                BORDER, 
                zoom ? height + BORDER : height-BORDER*2-convY(BUTTON_HEIGHT+PROPERTIES_HEIGHT), 
                width-BORDER*2, 
                convY(PROPERTIES_HEIGHT), 
                0);

   // move/resize properties widgets
   int x_max = convX(EDITBOXES_LEFT+DATES_WIDTH)+BORDER;
   int x_half = width/2 + BORDER/2;
   int x2 = min(x_max, x_half);
   int w1 = x2 - convX(EDITBOXES_LEFT) - BORDER;
   int w2 = width - x2 - 2*BORDER;

   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_VISITED), 0, 
                BORDER*2, 
                zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*5.0),
                0, 
                0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_LAST_VISIT), 0, 
                convX(EDITBOXES_LEFT)+5, 
                zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*5.0),
                w1, // convX(DATES_WIDTH), 
                convY(EDITBOXES_HEIGHT), 
                0);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_STATIC_URL), 0, 
                BORDER*2, 
                zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75), 
                0, 0, SWP_NOSIZE);
   SetWindowPos(GetDlgItem(hEditWnd, IDC_URL), 0, 
                convX(EDITBOXES_LEFT), 
                zoom ? height + BORDER : height-convY(EDITBOXES_TOP-EDITBOXES_HEIGHT*3.75)-2, 
                width-convX(EDITBOXES_LEFT)-BORDER*2, 
                convY(EDITBOXES_HEIGHT), 
                0);
   InvalidateRgn(hEditWnd, NULL, FALSE);
   UpdateWindow(hEditWnd);
}
