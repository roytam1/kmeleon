/*
*  Copyright (C) 2002 Jeff Doozan
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

#define PLUGIN_NAME "External Program Control Plugin"
#define NO_OPTIONS "This plugin has no user configurable options."

#define KMELEON_PLUGIN_EXPORTS
#include "..\kmeleon_plugin.h"
#include "..\utils.h"

#define _T(x) x

int Load();
void Create(HWND parent);
void Config(HWND parent);
void Close(HWND parent);
void Quit();

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

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
      else return 0;

      return 1;
   }
   return 0;
}

HINSTANCE ghInstance;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         ghInstance = (HINSTANCE) hModule;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}


kmeleonFunctions *kFuncs;

int Load() {
   kFuncs = kPlugin.kFuncs;
   return 1;
}

WNDPROC KMeleonWndProc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void Create(HWND hWndParent) {
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
}

void Config(HWND parent){
   MessageBox(parent, NO_OPTIONS, PLUGIN_NAME, 0);
}


/*
kmeleonPointInfo *(*GetInfoAtPoint) (int x, int y);
void (*NavigateTo)(const char *url, int windowState);
int (*GetID) (char *strID);

  
void (*GetPreference)(enum PREFTYPE type, char *preference, void *ret, void *defaultVal);
void (*SetPreference)(enum PREFTYPE type, char *preference, void *val, BOOL update = FALSE);
*/


void ReturnPointInfo(HWND hWndReturn, WORD x, WORD y) {

   kmeleonPointInfo *pInfo = kFuncs->GetInfoAtPoint(x, y);

   if (!pInfo)
      return;

   WORD pageLen, linkLen, imageLen, frameLen, strLen ;

   pageLen  = pInfo->page  ? strlen(pInfo->page)  +1 : 0;
   linkLen  = pInfo->link  ? strlen(pInfo->link)  +1 : 0;
   imageLen = pInfo->image ? strlen(pInfo->image) +1 : 0;
   frameLen = pInfo->frame ? strlen(pInfo->frame) +1 : 0;
   strLen   = pageLen + linkLen + imageLen + frameLen;

   int headerLen = 4 * sizeof(WORD);   // 4 identifiers, 2 bytes each
   int offset = headerLen;             // offset of string data
   int dataLen = headerLen + strLen;

   char *data = new char[dataLen];
   WORD *index = (WORD *) data;
   
   // save pInfo->page
   if (pageLen) {
      index[0] = offset;
      lstrcpy(data+offset, pInfo->page);
      offset += pageLen;
   }
   else
      index[0] = 0;

   // save pInfo->link
   if (linkLen) {
      index[1] = offset;
      lstrcpy(data+offset, pInfo->link);
      offset += linkLen;
   }
   else
      index[1] = 0;

   // save pInfo->image
   if (imageLen) {
      index[2] = offset;
      lstrcpy(data+offset, pInfo->image);
      offset += imageLen;
   }
   else
      index[2] = 0;

   // save pInfo->frame
   if (frameLen) {
      index[3] = offset;
      lstrcpy(data+offset, pInfo->frame);
      offset += frameLen;
   }
   else
      index[3] = 0;


   
   COPYDATASTRUCT cdsSend;
   cdsSend.dwData = NULL;
   cdsSend.cbData = dataLen;
   cdsSend.lpData = data;
   SendMessage(hWndReturn, WM_COPYDATA, NULL, (LPARAM) &cdsSend);

   delete data;

}


int CommandAtPoint(int command, WORD x, WORD y) {
   return kFuncs->CommandAtPoint(command, x, y);
}



/*
kmeleonPointInfo *GetPointInfo(char *data) {
   WORD *index = (WORD *) data;

   kmeleonPointInfo *pi = new kmeleonPointInfo;
   pi->page  = index[0] ? strdup(data+index[0]) : NULL;
   pi->link  = index[1] ? strdup(data+index[1]) : NULL;
   pi->image = index[2] ? strdup(data+index[2]) : NULL;
   pi->frame = index[3] ? strdup(data+index[3]) : NULL;

   return pi;
}
*/



// synatx SendMessage(UWM_GETPOINTINFO, HWND hWndReturnTo, NULL)
#define UWM_GETPOINTINFO      WM_USER+2222
#define UWM_COMMANDATPOINT    WM_USER+2223

#define CD_NAVIGATETO   1
#define CD_GETID        2


 // Subclassed window function

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){

   switch (message) {
   case UWM_GETPOINTINFO:
      ReturnPointInfo((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
      break;

   case UWM_COMMANDATPOINT:
      int ret;
      ret = CommandAtPoint(wParam, LOWORD(lParam), HIWORD(lParam));
      SetWindowLong(hWnd, DWL_MSGRESULT, ret);
      return ret;

   case WM_COPYDATA: {

      HWND hWndReturn = (HWND) wParam;
      COPYDATASTRUCT *cds = (COPYDATASTRUCT*) lParam;
      
      switch (cds->dwData) {
      case CD_NAVIGATETO:
         kFuncs->NavigateTo((char *) cds->lpData+1, ((char *) cds->lpData)[0], NULL );
         return 0;

      case CD_GETID:
         int id;
         id = kFuncs->GetID((char *) cds->lpData);
         SetWindowLong(hWnd, DWL_MSGRESULT, id);
         return id;
      }

      break; }
   }

   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


// function mutilation protection
extern "C" {

KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
   return &kPlugin;
}

}









/*


void *dataRecieved = NULL;

   case WM_COPYDATA: {
      COPYDATASTRUCT *cdsRecv = (COPYDATASTRUCT*) lParam;
      delete dataRecieved;
      dataRecieved = new char[cdsRecv->cbData];
      mmemcpy(dataRecieved, cdsRecv->lpData, cdsRecv->cbData);
      break; }




#define UWM_GETPOINTINFO   WM_USER+2222



typedef struct {
   char *image;
   char *link;
   char *frame;
   char *page;
} kmeleonPointInfo;

kmeleonPointInfo *GetPointInfo(HWND hWnd, WORD x, WORD y) {

   delete dataRecieved;
   dataRecieved = NULL;


   SendMessage(hWnd, UWM_GETPOINTINFO, (WPARAM) ghWndRecv, MAKELONG(x, y));

   if (dataRecieved) {
      
      WORD *index = (WORD *) dataRecieved;
      char *data = (char *) dataRecieved;
      
      kmeleonPointInfo *pi = new kmeleonPointInfo;
      pi->page  = index[0] ? mstrdup(data+index[0]) : NULL;
      pi->link  = index[1] ? mstrdup(data+index[1]) : NULL;
      pi->image = index[2] ? mstrdup(data+index[2]) : NULL;
      pi->frame = index[3] ? mstrdup(data+index[3]) : NULL;
      
      return pi;
   }

   return NULL;
}

#define CD_NAVIGATETO   1
#define CD_GETID        2

#define OPEN_NORMAL      0    // valid windowState values in NavigateTo()
#define OPEN_NEW         1
#define OPEN_BACKGROUND  2



void NavigateTo(HWND hWnd, char *url, int windowState) {

   if (hWnd && url && *url) {

      int len = 1+ lstrlen(url)+1; // 1 + string (1 = windowState)
      char *data = new char[len];
      data[0] = windowState;
      lstrcpy(data+1, url);

      COPYDATASTRUCT cds;
      cds.dwData = CD_NAVIGATETO;
      cds.cbData = len;
      cds.lpData = data;

      SendMessage(hWnd, WM_COPYDATA, (WPARAM) ghWndRecv, (LPARAM) &cds);

      delete data;

   }
}

*/
