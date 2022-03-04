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
*
*/

#include "stdafx.h"

#include <stdlib.h>
#include <tchar.h>
#include <list>
#include <string>
#include <vector>
#include "strconv.h"

#define KMELEON_PLUGIN_EXPORTS

#include "kmeleon_plugin.h"
#include "../../app/KMeleonConst.h"
#include "mozilla.h"
#include "resource.h"

#include "mozilla/ChaosMode.h" // ChaosMode hack

#define PLUGIN_NAME "Login Manager"
#define SIDEBARPROPNAME _T("LoginManagerData")

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WNDPROC KMeleonWndProc;

int Init();
void Create(HWND parent);
void CreateTab(HWND parent, HWND tab);
void DestroyTab(HWND parent, HWND tab);
void Destroy(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);

#define MAX_FEED 10

int id_rssicon;
int id_firstrss;


HINSTANCE ghInstance;

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   DoMessage
};

std::vector<HWND> wList;

long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
      if (stricmp(subject, "Init") == 0) {
         Init();
      }
      else if (strcmp(subject, "Create") == 0) {
         Create((HWND)data1);
      }
	  else if (strcmp(subject, "CreateTab") == 0) {
         CreateTab((HWND)data1, (HWND)data2);
      }
	  else if (strcmp(subject, "DestroyTab") == 0) {
         DestroyTab((HWND)data1, (HWND)data2);
      }
      else if (strcmp(subject, "Config") == 0) {
         Config((HWND)data1);
      }
      else if (strcmp(subject, "Quit") == 0) {
         Quit();
      }
	  else if (strcmp(subject, "Destroy") == 0) {
         Destroy((HWND)data1);
      }
      else if (strcmp(subject, "DoMenu") == 0) {
         DoMenu((HMENU)data1, (char *)data2);
      }
      else return 0;

      return 1;
   }
   return 0;
}

int Init(){
   return true;
}

void CreateTab(HWND hWnd, HWND hTab) {
	
}

void DestroyTab(HWND hWnd, HWND hTab) {
	std::vector<HWND>::iterator iter;
	for (iter = wList.begin(); iter != wList.end(); iter++) {
		if ((*iter) == hTab) break;
	}
	if (iter != wList.end()) wList.erase(iter);

	CDomEventListener* listener = reinterpret_cast<CDomEventListener*>(GetProp(hWnd, SIDEBARPROPNAME));
	if (listener == NULL) return;
	NS_ADDREF(listener);
	listener->Done(hTab);
}

void Create(HWND hWndParent) {
	if (IsWindowUnicode(hWndParent))
	{
		KMeleonWndProc = (WNDPROC) GetWindowLongW(hWndParent, GWL_WNDPROC);
		SetWindowLongPtrW(hWndParent, GWL_WNDPROC, (LONG_PTR)WndProc);
	}
	else
	{
		KMeleonWndProc = (WNDPROC) GetWindowLongA(hWndParent, GWL_WNDPROC);
		SetWindowLongPtrA(hWndParent, GWL_WNDPROC, (LONG_PTR)WndProc);
	}
	
	CDomEventListener* listener = new CDomEventListener(hWndParent);
	SetProp(hWndParent, SIDEBARPROPNAME, reinterpret_cast<HANDLE>(listener));
}

void Destroy(HWND hWnd) {
	CDomEventListener* listener = reinterpret_cast<CDomEventListener*>(GetProp(hWnd, SIDEBARPROPNAME));
	if (listener == NULL) return;
	// listener->Done(); // Will destroy it
	NS_RELEASE(listener);
}

void DoMenu(HMENU menu, char *param) {
}

void Config(HWND hWndParent) {
}

void Quit() {
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {		
		case UWM_UPDATEBUSYSTATE:
			if (wParam) {
				std::vector<HWND>::iterator iter;
				for (iter = wList.begin(); iter != wList.end(); iter++) {
					if ((*iter) == (HWND)lParam) break;
				}
				if (iter == wList.end()) {
					CDomEventListener* listener = reinterpret_cast<CDomEventListener*>(GetProp(hWnd, SIDEBARPROPNAME));
					if (listener && listener->Init((HWND)lParam)) {					
						wList.push_back((HWND)lParam);
					}
				}
			}
		break;
	}

	if (IsWindowUnicode(hWnd))
		return CallWindowProcW(KMeleonWndProc, hWnd, message, wParam, lParam);
	else
		return CallWindowProcA(KMeleonWndProc, hWnd, message, wParam, lParam);
}


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

extern "C" {
	KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
		return &kPlugin;
	}
}

#if 1 //ChaosMode hack
namespace mozilla {
	namespace detail {

		Atomic<uint32_t> gChaosModeCounter(0);
		ChaosFeature gChaosFeatures = None;

	} /* namespace detail */
} /* namespace mozilla */
#endif
