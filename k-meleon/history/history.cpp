/*
*  Copyright (C) 2001 Jeff Doozan
*  Copyright (C) 2003 Ulf Erikson <ulferikson@fastmail.fm>
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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

//#include "stdafx.h"
#include "history.h"
#include "..\Utils.h"
#include "..\DialogUtils.h"
#include "..\KmeleonConst.h"
#include "../LocalesUtils.h"
Locale* gLoc;

#include "AutoComplete.h"
#include "nsCOMPtr.h"
#include "nsIBrowserHistory.h"
#include "nsServiceManagerUtils.h"

#define PLUGIN_NAME "History Plugin"

#define PREFERENCE_HISTORY_LENGTH "kmeleon.plugins.history.length"
static INT nHistoryLength = 25;
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS "browser.history_expire_days"

/*
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}
*/

int Load();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);


struct menulist {
   HMENU hMenu;
   struct menulist *next;
};
typedef struct menulist MenuList;
MenuList *gMenuList = NULL;

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

int ID_HISTORY_FLAG = -1;
int ID_HISTORY = -1;
int ID_VIEW_HISTORY = -1;
int ID_CONFIG_HISTORY = -1;
int ID_CLEAR_HISTORY = -1;
int wm_deferbringtotop = -1;
int ID_NAV_BACK = -1;
int ID_NAV_FORWARD = -1;
HWND hWndFront;

HWND ghWndView = NULL;
HIMAGELIST gImagelist;


long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Load") == 0) {
         Load();
      }
      else if (stricmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
      else if (stricmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (stricmp(subject, "Quit") == 0) {
		 AutoComplete("", NULL);
         Quit();
      }
	  else if (stricmp(subject, "AutoComplete") == 0) {
		  return AutoComplete((char*)data1, (AutoCompleteResult**)data2);
	  }
      else if (stricmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else if (stricmp(subject, "DoRebar") == 0) {
         DoRebar((HWND)data1);
      }
      else if (stricmp(subject, "DoAccel") == 0) {
         *(int *)data2 = DoAccel((char *)data1);
      }
	  else if (stricmp(subject, "DoLocale") == 0) {
         if (gLoc) delete gLoc;
		 gLoc = Locale::kmInit(&kPlugin);
	  }
      else return 0;

      return 1;
   }
   return 0;
}



#include "../findskin.cpp"


int Load(){
   gLoc = Locale::kmInit(&kPlugin);
   kPlugin.kFuncs->GetPreference(PREF_INT,  PREFERENCE_HISTORY_LENGTH, &nHistoryLength, &nHistoryLength);
   if (nHistoryLength<0)
      nHistoryLength = 0;
   
   wm_deferbringtotop = kPlugin.kFuncs->GetCommandIDs(1);
   ID_HISTORY = kPlugin.kFuncs->GetCommandIDs(nHistoryLength*2);
   ID_VIEW_HISTORY = kPlugin.kFuncs->GetCommandIDs(1);
   ID_CONFIG_HISTORY = kPlugin.kFuncs->GetCommandIDs(1);
   ID_CLEAR_HISTORY = kPlugin.kFuncs->GetCommandIDs(1);
   ID_NAV_BACK = kPlugin.kFuncs->GetID("ID_NAV_BACK");
   ID_NAV_FORWARD = kPlugin.kFuncs->GetID("ID_NAV_FORWARD");

   HBITMAP bitmap;
   int ilc_bits = ILC_COLOR;
     
   TCHAR szFullPath[MAX_PATH];
   FindSkinFile(szFullPath, _T("history.bmp"));
   FILE *fp = _tfopen(szFullPath, _T("r"));
   if (fp) {
      fclose(fp);
      bitmap = (HBITMAP)LoadImage(NULL, szFullPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   } else {
      bitmap = LoadBitmap(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_IMAGES));
   }
   
   BITMAP bmp;
   GetObject(bitmap, sizeof(BITMAP), &bmp);
   
   ilc_bits = (bmp.bmBitsPixel == 32 ? ILC_COLOR32 : (bmp.bmBitsPixel == 24 ? ILC_COLOR24 : (bmp.bmBitsPixel == 16 ? ILC_COLOR16 : (bmp.bmBitsPixel == 8 ? ILC_COLOR8 : (bmp.bmBitsPixel == 4 ? ILC_COLOR4 : ILC_COLOR)))));
   gImagelist = ImageList_Create(bmp.bmWidth/6, bmp.bmHeight, ILC_MASK | ilc_bits, 4, 4);
   if (gImagelist && bitmap)
      ImageList_AddMasked(gImagelist, bitmap, RGB(255, 0, 255));
   if (bitmap)
      DeleteObject(bitmap);
   
   return true;
}

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

// Preferences Dialog function
BOOL CALLBACK PrefDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   int nTmp;
   
   switch (uMsg) {

      case WM_INITDIALOG:
         nTmp = 7;
         kPlugin.kFuncs->GetPreference(PREF_INT, PREF_BROWSER_HISTORY_EXPIRE_DAYS, &nTmp, &nTmp);
         SetDlgItemInt(hWnd, IDC_EXPIRE_DAYS, nTmp, TRUE);
         EnableWindow(GetDlgItem(hWnd, IDB_CLEAR), false);
         break;

      case WM_COMMAND:
         switch(HIWORD(wParam)) {
            case BN_CLICKED:
               switch (LOWORD(wParam)) {
                  case IDB_CLEAR:
                     if (MessageBox(hWnd, _Tr("Delete all items in your History folder?"), _Tr("History"), MB_YESNO) == IDYES) {
                        MessageBox(NULL, _Tr("Deletion is not implemented"), _Tr("History"), MB_OK);
                     }
                     break;

                  case IDOK:
                     nTmp = GetDlgItemInt(hWnd, IDC_EXPIRE_DAYS, NULL, TRUE);
                     kPlugin.kFuncs->SetPreference(PREF_INT, PREF_BROWSER_HISTORY_EXPIRE_DAYS, &nTmp, FALSE);
                     // fall through...

                  case IDCANCEL:
                     SendMessage(hWnd, WM_CLOSE, 0, 0);
               }
         }
         break;

      case WM_CLOSE:
         EndDialog(hWnd, 0);
         break;

      default:
         return FALSE;
   }

   return TRUE;
}

void Config(HWND parent){
   gLoc->DialogBoxParam(MAKEINTRESOURCE(IDD_CONFIG), parent, (DLGPROC)PrefDlgProc, 0);
}

void Quit(){
   hWndFront = NULL;
   
   MenuList* ml;
   while (gMenuList)
   {
	   ml = gMenuList;
	   gMenuList = gMenuList->next;
	   free(ml);
   }

   if (ghWndView)
      SendMessage(ghWndView, WM_CLOSE, 0, 0);
   
   delete gLoc;
}

void DoMenu(HMENU menu, char *param){
   if (*param != 0){
      char *action = param;
      char *string = strchr(param, ',');
      if (string) {
         *string = 0;
         string = SkipWhiteSpace(string+1);
      }
      else
         string = action;
      
      int command = 0;
      if (stricmp(action, "View") == 0){
         command = ID_VIEW_HISTORY;
      }
      if (stricmp(action, "Config") == 0){
         command = ID_CONFIG_HISTORY;
	  }
	  if (stricmp(action, "Clear") == 0){
         command = ID_CLEAR_HISTORY;
      }
      if (command && string && *string) {
         AppendMenu(menu, MF_STRING, command, string);
         return;
      }
      return;
   }

   MenuList *tmp;
   tmp = (MenuList *) calloc(1, sizeof(struct menulist));
   tmp->hMenu = menu;
   tmp->next = gMenuList;
   gMenuList = tmp;
   AppendMenu(menu, MF_SEPARATOR, 0, "");
}

int DoAccel(char *param) {
   if (stricmp(param, "View") == 0)
      return ID_VIEW_HISTORY;
   if (stricmp(param, "Config") == 0)
      return ID_CONFIG_HISTORY;
   if (stricmp(param, "clear") == 0)
      return ID_CLEAR_HISTORY;
   return 0;
}

void DoRebar(HWND rebarWnd) {
}

void ShowMenuUnderButton(HWND hWndParent, HMENU hMenu, UINT uMouseButton, int iID) {
   // Find the toolbar
   HWND hReBar = FindWindowEx(GetActiveWindow(), NULL, REBARCLASSNAME, NULL);
   int uBandCount = SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);  
   int x = 0;
   BOOL bFound = FALSE;
   REBARBANDINFO rb;
   rb.cbSize = sizeof(REBARBANDINFO);
   rb.fMask = RBBIM_CHILD;
   while (x < uBandCount && !bFound) {
      
      if (!SendMessage(hReBar, RB_GETBANDINFO, (WPARAM) x++, (LPARAM) &rb))
         continue;
      
      // toolbar hwnd
      HWND tb = rb.hwndChild;
      RECT rc;
      
      int ButtonID = SendMessage(tb, TB_COMMANDTOINDEX, iID, 0);
      if (ButtonID < 0)
         continue;
      if (ButtonID == 0) {
         TBBUTTON button;
         SendMessage(tb, TB_GETBUTTON, 0, (LPARAM) &button);
         if (button.idCommand != iID)
            continue;
      }
      
      SendMessage(tb, TB_GETITEMRECT, ButtonID, (LPARAM) &rc);
      POINT pt = { rc.left, rc.bottom };
      ClientToScreen(tb, &pt);
      DWORD SelectionMade = TrackPopupMenu(hMenu, TPM_LEFTALIGN | uMouseButton | TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, &rc);
      
      PostMessage(hWndParent, UWM_REFRESHTOOLBARITEM, (WPARAM) iID, 0);
      
      if (SelectionMade > 0)
         kPlugin.kFuncs->GotoHistoryIndex(SelectionMade-ID_HISTORY);
      
      bFound = TRUE;
   }
}

void CreateBackMenu (HWND hWndParent, UINT button) {
   int index, count, i, limit;
   char **titles, **urls, buf[47];
   
   if (!kPlugin.kFuncs->_GetMozillaSessionHistory (&titles, &urls, &count, &index)) {
      return;
   }
   
   if (index>nHistoryLength)
      limit = index-nHistoryLength;
   else limit=0;
   
   HMENU menu = CreatePopupMenu();
   
   int x=0;
   for (i = index - 1; i >= limit; i--) {
      CondenseMenuText(buf, titles[i], x++);
      AppendMenu(menu, MF_STRING, ID_HISTORY+i, buf);
   }
   
   ShowMenuUnderButton(hWndParent, menu, button, ID_NAV_BACK);
   
   DestroyMenu(menu);
}


void CreateForwardMenu (HWND hWndParent, UINT button) {
   
   int index, count, i, limit;
   char **titles, **urls, buf[47];
   
   if (!kPlugin.kFuncs->_GetMozillaSessionHistory (&titles, &urls, &count, &index)) {
      return;
   }
   
   HMENU menu = CreatePopupMenu();
   
   if (count-index > nHistoryLength)
      limit = index+nHistoryLength;
   else limit=count;
   
   int x=0;
   for (i = index + 1; i < limit; i++) {
      CondenseMenuText(buf, titles[i], x++);
      AppendMenu(menu, MF_STRING, ID_HISTORY+i, buf);
   }
   
   ShowMenuUnderButton(hWndParent, menu, button, ID_NAV_FORWARD);
   
   DestroyMenu(menu);
}


void UpdateHistoryMenu (HWND hWndParent) {
   int index, count, i;
   char **titles, **urls;
   char buf[47];  //  3 spaces for "&# " 20 for beginning of title 3 for "..." 20 for end of title
   
   MenuList *tmpMenu = gMenuList;
   while (tmpMenu) {
      HMENU ghMenu = tmpMenu->hMenu;
      
      if (!ghMenu)
         return;
      
      // Clear the existing history menu 
      for (i=ID_HISTORY;i<ID_HISTORY+nHistoryLength;i++)
         DeleteMenu(ghMenu, i, MF_BYCOMMAND);
      
      // Add the local history to the menu
      if (!kPlugin.kFuncs->_GetMozillaSessionHistory (&titles, &urls, &count, &index)) return;
      if (count > nHistoryLength) count = nHistoryLength;
      
      for (i=count-1;i>=0;i--) {
         CondenseMenuText(buf, titles[i], (count-1 - i) );
         
         if (i == index)
            AppendMenu(ghMenu, MF_ENABLED | MF_STRING | MF_CHECKED, ID_HISTORY+i, buf);
         else
            AppendMenu(ghMenu, MF_ENABLED | MF_STRING, ID_HISTORY+i, buf);
      }
      tmpMenu = tmpMenu->next;
   }
}


void CondenseMenuText(TCHAR *buf, TCHAR *title, int index) {
   int len;
   TCHAR str[3];

   if ( (index >= 0) && (index <10) ) {
      buf[0] = _T('&');
      //buf[1] = index +48; // convert int to ascii
      //buf[2] = ' ';
	   
      _itot(index, str, 10); // convert int to ascii
	  buf[1] = str[0];
	  buf[2] = _T(' ');
   }
   else
      memcpy(buf, _T("   "), 3*sizeof(TCHAR));
   
   len = lstrlen(title);
   if (len > 43)
      CondenseString(title, 43);
   
   _tcscpy(buf+3, title);
}

void ClearHistory()
{
	nsresult rv;

	//NS_GLOBALHISTORY2_CONTRACTID
	nsCOMPtr<nsIBrowserHistory> BrowserHistory(do_GetService("@mozilla.org/browser/global-history;2", &rv));
	if (NS_FAILED(rv)) return;

	BrowserHistory->RemoveAllPages();
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
   
   switch (message) {
      
      
   case WM_SETFOCUS:
      hWndFront = hWnd;
      // Fall through!
   case UWM_UPDATESESSIONHISTORY:
      UpdateHistoryMenu(hWnd);
      break;
   case TB_LBUTTONHOLD:
	   if (ID_NAV_BACK == wParam)
          CreateBackMenu(hWnd, TPM_LEFTBUTTON);
	   else if (ID_NAV_FORWARD == wParam)
          CreateForwardMenu(hWnd, TPM_LEFTBUTTON);
      break;
   case TB_RBUTTONDOWN:
      if (ID_NAV_BACK == wParam)
          CreateBackMenu(hWnd, TPM_RIGHTBUTTON);
	   else if (ID_NAV_FORWARD == wParam)
          CreateForwardMenu(hWnd, TPM_RIGHTBUTTON);
      break;
      
   case WM_COMMAND:
      WORD command;
      command = LOWORD(wParam);
      if ((command >= ID_HISTORY) && (command < ID_HISTORY+nHistoryLength)) {
         kPlugin.kFuncs->GotoHistoryIndex(command-ID_HISTORY);
         return true;
      }
      else if (command == ID_VIEW_HISTORY) {
         if (ghWndView) {
            ShowWindow(ghWndView, SW_RESTORE);
            BringWindowToTop(ghWndView);
         }
         else {
			 ghWndView = gLoc->CreateDialogParam(MAKEINTRESOURCE(IDD_VIEW_HISTORY), NULL, ViewProc, 0);
         }
         return true;
      }
      else if (command == ID_CONFIG_HISTORY) {
         Config(hWnd);
         return 1;
      }
	  else if (command == ID_CLEAR_HISTORY) {
		  ClearHistory();
		  return TRUE;
      }
      else if (command == wm_deferbringtotop) {
         BringWindowToTop(hWnd);
         return true;
      }
      break;
   case WM_MENUSELECT:
      {
         int command = LOWORD(wParam);

         if ((command >= ID_HISTORY) && (command < ID_HISTORY+nHistoryLength)) {
            char **titles, **urls;
            int count, index;
            if (kPlugin.kFuncs->_GetMozillaSessionHistory (&titles, &urls, &count, &index)) {
               kPlugin.kFuncs->SetStatusBarText(urls[command - ID_HISTORY]);
               return true;
            }
         }
         else if (command == ID_VIEW_HISTORY) {
            kPlugin.kFuncs->SetStatusBarText(gLoc->GetString(IDS_VIEW));
            return true;
         }
         else if (command == ID_CONFIG_HISTORY) {
            kPlugin.kFuncs->SetStatusBarText(gLoc->GetString(IDS_CONFIGURE));
            return true;
         }

         break;
      }
   }
   
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
	return &kPlugin;
}

}
