/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2003-2004 Ulf Erikson <ulferikson@fastmail.fm>
 *                         Romain Vallet <rom@jalix.org>
 *               2006      Dorian Boissonnade
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

#define _WIN32_WINNT 0x0500

#include "mozilla/Char16.h"
#include <windows.h>
#include <math.h>
#include <tchar.h>

#define KMELEON_PLUGIN_EXPORTS
#define _Tr(x) kPlugin.kFuncs->Translate(_T(x))
#define PLUGIN_NAME "Mouse Gestures Plugin"
#define NO_OPTIONS _Tr("This plugin has no user configurable options.")
#define PREF_ "kmeleon.plugins.gestures."
#define NOTFOUND -1

#include "kmeleon_plugin.h"
#include "../../app/KMeleonConst.h"
#include "strconv.h"
#include "mozilla.h"

BOOL APIENTRY DllMain (
        HANDLE hModule,
        DWORD ul_reason_for_call,
        LPVOID lpReserved) {

  return TRUE;
}

LRESULT CALLBACK WndProc (
        HWND hWnd, UINT message,
        WPARAM wParam,
        LPARAM lParam);

void * KMeleonWndProc;

int Init();
void Create(HWND parent);
void Destroy(HWND parent);
void Config(HWND parent);
void CreateTab(HWND parent, HWND tab);
void DestroyTab(HWND parent, HWND tab);
void Quit();
void DoMenu(HMENU menu, char *param);
long DoMessage(const char *to, const char *from, const char *subject, long data1, long data2);
void DoRebar(HWND rebarWnd);
int DoAccel(char *param);

kmeleonPlugin kPlugin = {
    KMEL_PLUGIN_VER,
    PLUGIN_NAME,
    DoMessage
};

#include <map>
template<class T>
class WinData {
protected:
    typedef std::map<HWND, T> S;
    S s;

public:
    T& Attach(HWND hWnd) {
        auto it = s.find(hWnd);
        if (it != s.end()) return it->second;
        auto p = s.insert(std::pair<HWND, T>(hWnd, T()));
        return p.first->second;
    }

    void Detach(HWND hWnd) {
        s.erase(hWnd);
    }

};

WinData<nsCOMPtr<CDomEventListener>> winData;

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
        else if (strcmp(subject, "Destroy") == 0) {
            Destroy((HWND)data1);
        }
        else if (strcmp(subject, "Config") == 0) {
            Config((HWND)data1);
        }
        else if (strcmp(subject, "Quit") == 0) {
            Quit();
        }
        else if (strcmp(subject, "DoMenu") == 0) {
            DoMenu((HMENU)data1, (char *)data2);
        }
        else if (strcmp(subject, "DoRebar") == 0) {
            DoRebar((HWND)data1);
        }
        else if (strcmp(subject, "DoAccel") == 0) {
            *(int *)data2 = DoAccel((char *)data1);
        }
        else return 0;

        return 1;
    }
    return 0;
}

void InitDefaultPreferences(){
    char szPref[100], szTxt[100];
    strcpy(szPref, PREF_);
    strcat(szPref, "LR");
    kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, (char*)"");
    if (!*szTxt){
        strcpy(szTxt, "ID_NAV_FORWARD");
        kPlugin.kFuncs->SetPreference(PREF_STRING, szPref, szTxt, FALSE);
    }
    strcpy(szPref, PREF_);
    strcat(szPref, "RL");
    kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, (char*)"");
    if (!*szTxt){
        strcpy(szTxt, "ID_NAV_BACK");
        kPlugin.kFuncs->SetPreference(PREF_STRING, szPref, szTxt, FALSE);
    }
}

UINT id_defercapture;

int Init(){
    id_defercapture = kPlugin.kFuncs->GetCommandIDs(1);
    InitDefaultPreferences();
    return true;
}

void Create(HWND parent){
    KMeleonWndProc = (void *)GetWindowLong(parent, GWL_WNDPROC);
    SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);
}

void CreateTab(HWND hWndParent, HWND hTab) {
    nsCOMPtr<CDomEventListener>& l = winData.Attach(hWndParent);
    if (!l) l = new CDomEventListener(hWndParent);
    l->Init(hTab);
}

void DestroyTab(HWND hWndParent, HWND hTab) {
    nsCOMPtr<CDomEventListener>& l = winData.Attach(hWndParent);
    if (l) l->Done(hTab);
}

void Destroy(HWND hWnd) {
    winData.Detach(hWnd);
}

void Config(HWND parent){
    //MessageBox(parent, NO_OPTIONS, _T(PLUGIN_NAME), 0);
}

void Quit(){
}

void DoMenu(HMENU menu, char *param){
}

int DoAccel(char *param) {
    return 0;
}

void DoRebar(HWND rebarWnd){
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef abs
#define abs(x) (((x)>0)?(x):(-(x)))
#endif

enum dir {NOMOVE, RIGHT, UPRIGHT, UP, UPLEFT, LEFT, DOWNLEFT, DOWN, DOWNRIGHT, BADMOVE};
typedef enum dir DIRECTION;


DIRECTION findDir(POINT p1, POINT p2) {

    UINT MINDIST = 20;
    UINT MAXSLIP = 85;
    UINT SPLIT   = 50;

    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"mindist", &MINDIST, &MINDIST);
    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"maxslip", &MAXSLIP, &MAXSLIP);
    kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"split", &SPLIT, &SPLIT);

    INT dx = p2.x - p1.x;
    INT dy = p2.y - p1.y;
    UINT dist = dx*dx + dy*dy;

    if (dist < MINDIST*MINDIST) return NOMOVE;
    if (dist < 3*MINDIST*MINDIST/2) return BADMOVE;

    double h = 0;
    double v = 90;
    double d = (v-h) / 2;

    double s1 = h + SPLIT * (d-h) / 100.0;
    double s2 = v + SPLIT * (d-v) / 100.0;

    double h_max = h + MAXSLIP * (s1-h) / 100.0;
    double v_min = v + MAXSLIP * (s2-v) / 100.0;

    double d_min = d + MAXSLIP * (s1-d) / 100.0;
    double d_max = d + MAXSLIP * (s2-d) / 100.0;

    double dir;
    if (dx == 0)
        dir = v;
    else
        dir = 2*v * abs(atan((double) dy / (double) dx)) / M_PI;

    if (dir <= h_max) {
        return dx > 0 ? RIGHT : LEFT;
    }
    else if (dir >= v_min) {
        return dy > 0 ? DOWN : UP;
    }
    else if (dir >= d_min && dir <= d_max) {
        if (dy > 0)
            return dx > 0 ? DOWNRIGHT : DOWNLEFT;
        else
            return dx > 0 ? UPRIGHT : UPLEFT;
    }

    return BADMOVE;
}

static UINT  m_captured;
static UINT  m_defercapture;
kmeleonPointInfo* m_pInfo = nullptr;
static POINT m_posDown;
static SYSTEMTIME m_stDown;
static int m_virt;

static BOOL m_rocking = FALSE;
static BOOL m_preventpopup = FALSE;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    if (message == WM_COMMAND){
        WORD command = LOWORD(wParam);

        if (command == id_defercapture) {
            m_captured = m_defercapture;
            m_pInfo = kPlugin.kFuncs->GetInfoAtClick(hWnd);
            SetCapture(hWnd);
        }
    }
    else if (m_rocking && ((message == WM_RBUTTONUP) || (message == WM_LBUTTONUP)))
    {
        m_preventpopup--;
        if (!m_preventpopup) {
            m_rocking = FALSE;
            //POINT pos;
            //GetCursorPos(&pos);
            //m_pInfo = kPlugin.kFuncs->GetInfoAtPoint(pos.x,pos.y);
            //if (!m_pInfo->link)
            //if (m_captured == WM_LBUTTONDOWN) // I wonder how mozilla can get a lbuttondown, if it's not the first button clicked ...
            //    PostMessage(GetFocus(), WM_LBUTTONUP, wParam, lParam);
            m_defercapture = m_captured = 0;
            ReleaseCapture();
        }
        return 0;
    }
    else if (message == WM_MOUSEACTIVATE && !m_captured){
        HWND hDesktopWnd = (HWND) wParam;
        UINT nHitTest = LOWORD(lParam);
        UINT mouseMsg = HIWORD(lParam);

        if (nHitTest == HTCLIENT) {

            RECT rc = {0};
            kPlugin.kFuncs->GetBrowserviewRect(hWnd, &rc);

            GetCursorPos(&m_posDown);
            if (m_posDown.x >= rc.left && m_posDown.x <= rc.right &&
                m_posDown.y >= rc.top  && m_posDown.y <= rc.bottom) {

                m_virt = 0;
                if (GetKeyState(VK_SHIFT) < 0)
                    m_virt |= FSHIFT;
                if (GetKeyState(VK_CONTROL) < 0)
                    m_virt |= FCONTROL;
                if (GetKeyState(VK_MENU) < 0)
                    m_virt |= FALT;

                if (mouseMsg == WM_RBUTTONDOWN || m_virt != 0 || mouseMsg == WM_LBUTTONDOWN) {
                    //SetCapture(hWnd);
                    m_defercapture = m_captured = mouseMsg;
                    GetSystemTime(&m_stDown);
                    PostMessage(hWnd, WM_COMMAND, id_defercapture, 0);
                    //m_pInfo = kPlugin.kFuncs->GetInfoAtClick(hWnd);
                    //SetCapture(hWnd);

                    return MA_ACTIVATE;
                }
            }
        }
        else
            m_captured = m_defercapture = 0;
    }
    else if (
             message == WM_LBUTTONDBLCLK ||
             message == WM_MBUTTONDBLCLK ||
             message == WM_RBUTTONDBLCLK){
        ReleaseCapture();
        m_captured = 0;
    }
    else if (message == WM_CAPTURECHANGED && (HWND)lParam!=hWnd)
    {
        ReleaseCapture();
        m_captured = 0;
    }
    else if (message == WM_LBUTTONDOWN && m_captured == WM_RBUTTONDOWN ||
             message == WM_RBUTTONDOWN && m_captured == WM_LBUTTONDOWN)
    {
        char szPref[100], szTxt[100];
        strcpy(szPref, PREF_);

        if (m_captured == WM_LBUTTONDOWN)
            strcat(szPref, "LR");
        else if (m_captured == WM_RBUTTONDOWN)
            strcat(szPref, "RL");
        kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, (char*)"");
        INT id = 0;
        if (*szTxt)
               id = kPlugin.kFuncs->GetID(szTxt);
        PostMessage(hWnd, WM_COMMAND, id, 0L);
        m_rocking = TRUE;
        m_preventpopup = 2;
        //m_captured = 0;
    }
    else if (message == WM_MOUSEMOVE && m_captured == WM_LBUTTONDOWN && !m_rocking) {

        POINT m_posMove;
        GetCursorPos(&m_posMove);
        int allow = 1;
        kPlugin.kFuncs->GetPreference(PREF_BOOL, PREF_"allow_left", &allow, &allow);

        if ( (abs(m_posMove.x - m_posDown.x) > 1 || abs(m_posMove.y - m_posDown.y) > 1)
            && m_virt == 0 || !allow)
            //&& (!(m_pInfo->link && strlen(m_pInfo->link)) || !allow) )
        {
            ReleaseCapture();
            m_captured = 0;

            POINT m_posDownClient = m_posDown;
            ScreenToClient(WindowFromPoint(m_posDown), &m_posDownClient);
            //SetCursorPos(m_posDown.x, m_posDown.y);
            //if (!m_pInfo->isInput && !(m_pInfo->link && *m_pInfo->link) && !(m_pInfo->image && *m_pInfo->image))
            //    PostMessage(WindowFromPoint(m_posDown), WM_LBUTTONUP, wParam, MAKELONG(m_posDownClient.x, m_posDownClient.y));
            PostMessage(WindowFromPoint(m_posDown), WM_LBUTTONDOWN, wParam, MAKELONG(m_posDownClient.x, m_posDownClient.y));
            //SetCursorPos(m_posMove.x, m_posMove.y);
         }
    }
    else if ((message == WM_LBUTTONUP && m_captured == WM_LBUTTONDOWN ||
              message == WM_MBUTTONUP && m_captured == WM_MBUTTONDOWN ||
              message == WM_RBUTTONUP && m_captured == WM_RBUTTONDOWN)
              && !m_rocking)
    {
        POINT m_posUp;
        GetCursorPos(&m_posUp);

        DIRECTION dir = findDir(m_posDown, m_posUp);

        SYSTEMTIME m_stUp;
        GetSystemTime(&m_stUp);

        ULONG u1 = m_stDown.wMilliseconds + 1000 * (m_stDown.wSecond + 60*(m_stDown.wMinute + 60*(m_stDown.wHour)));
        ULONG u2 = m_stUp.wMilliseconds + 1000 * (m_stUp.wSecond + 60*(m_stUp.wMinute + 60*(m_stUp.wHour)));

        UINT MAXTIME = 750;
        kPlugin.kFuncs->GetPreference(PREF_INT, PREF_"maxtime", &MAXTIME, &MAXTIME);

        if ( u2 - u1 > MAXTIME ) {
            dir = BADMOVE;
        }

        char szTxt[100];
        char szPref[100];
        strcpy(szPref, PREF_);

        if (m_virt & FSHIFT)   strcat(szPref, "S");
        if (m_virt & FCONTROL) strcat(szPref, "C");
        if (m_virt & FALT)     strcat(szPref, "A");

        if (m_captured == WM_LBUTTONDOWN) strcat(szPref, "L");
        if (m_captured == WM_MBUTTONDOWN) strcat(szPref, "M");
        if (m_captured == WM_RBUTTONDOWN) strcat(szPref, "R");

        char szDir[10][10] = { "nomove", "right", "upright", "up", "upleft", "left", "downleft", "down", "downright", "badmove" };
        strcat(szPref, szDir[dir]);

        kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, (char*)"");

        kmeleonPointInfo *pInfo = m_pInfo;
        if (pInfo) {
            if (pInfo->link && *pInfo->link)  strcat(szPref, "L");
            if (pInfo->image && *pInfo->image) strcat(szPref, "I");
        }

        kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, szTxt);

        if (!*szTxt && m_captured == WM_RBUTTONDOWN && m_virt == 0) {
            strcpy(szPref, PREF_);
            strcat(szPref, szDir[dir]);
            kPlugin.kFuncs->GetPreference(PREF_STRING, szPref, szTxt, (char*)"");
        }

        int id = 0;
        if (*szTxt)
            id = kPlugin.kFuncs->GetID(szTxt);

        HWND targetWnd = WindowFromPoint(m_posDown);
        ScreenToClient(targetWnd, &m_posDown);
        ScreenToClient(targetWnd, &m_posUp);

        if (m_captured == WM_LBUTTONDOWN && id == 0) {
            //SendMessage(targetWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(m_posDown.x, m_posDown.y));
            PostMessage(targetWnd, WM_LBUTTONUP, wParam, MAKELONG(m_posUp.x, m_posUp.y));
        }
        else if (m_rocking)
            m_rocking = FALSE;
        else if (dir == NOMOVE && m_captured == WM_RBUTTONDOWN && id == 0) {
            //SendMessage(targetWnd, WM_RBUTTONDOWN, wParam, MAKELONG(m_posDown.x, m_posDown.y));
            PostMessage(targetWnd, WM_RBUTTONUP, wParam, MAKELONG(m_posDown.x, m_posDown.y));
            //PostMessage(GetFocus(), WM_CONTEXTMENU, (WPARAM) hWnd, MAKELONG(m_posUp.x, m_posUp.y));
        }
        else if (m_captured == WM_MBUTTONDOWN && id == 0) {
            //SendMessage(targetWnd, WM_MBUTTONDOWN, MK_LBUTTON, MAKELONG(m_posDown.x, m_posDown.y));
            PostMessage(targetWnd, WM_MBUTTONUP, wParam, MAKELONG(m_posUp.x, m_posUp.y));
        }
        else if (dir != BADMOVE && id > 0) {
            if (id == kPlugin.kFuncs->GetID("navSearch")) {
                int len = kPlugin.kFuncs->GetWindowVar(hWnd, Window_SelectedText, NULL);
                wchar_t* query = (wchar_t*)malloc(len*sizeof(wchar_t));
                kPlugin.kFuncs->GetWindowVar(hWnd, Window_SelectedText, query);
                SendMessage(hWnd, WM_COMMAND, id, (LPARAM)query);
                free(query);
            } else
                PostMessage(hWnd, WM_COMMAND, id, 0);
        }

        ReleaseCapture();
        m_captured = m_defercapture = 0;
        return 0;
    }

    return CallWindowProc((WNDPROC)KMeleonWndProc, hWnd, message, wParam, lParam);
}

extern "C" {
    KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
        return &kPlugin;
    }
}
