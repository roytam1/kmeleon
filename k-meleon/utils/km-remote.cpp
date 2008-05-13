
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "kmeleon_plugin.h"
#include "CmdLine.h"


#define CD_NAVIGATETO   1
#define CD_GETID        2


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    if (!lpCmdLine || lpCmdLine[0] == 0) {
        return 0;
    }

    HWND hWndKM = FindWindow("KMeleon Browser Window", NULL);

    if (hWndKM == NULL) {
        HKEY hKey;
        char kmeleon[1025] = "";
        DWORD dwSize = 1024;
        if (RegOpenKey(HKEY_CURRENT_USER, 
                       (LPCTSTR)"Software\\K-Meleon\\K-Meleon\\General", 
                       &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueEx(hKey, "InstallDir", NULL, NULL, (LPBYTE)kmeleon, &dwSize) == ERROR_SUCCESS)
                strcat(kmeleon, "\\k-meleon.exe");
            RegCloseKey(hKey);
        }
        ShellExecute(NULL, "open", kmeleon, lpCmdLine, "", SW_SHOWNORMAL);
        return 0;
    }

    COPYDATASTRUCT cdsSend = {0};

    cdsSend.dwData = (DWORD) CD_GETID;
    cdsSend.cbData = 1 + strlen(lpCmdLine);
    cdsSend.lpData = (char*) malloc( cdsSend.cbData );
    strcpy( (char*)cdsSend.lpData, lpCmdLine );

    int id = SendMessage(hWndKM, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cdsSend);

    free((void*)cdsSend.lpData);


    if (id) {
        SendMessage(hWndKM, WM_COMMAND, (WPARAM) id, (LPARAM) NULL);
    }
    else {
        CCmdLine cmdline;
        cmdline.Initialize(lpCmdLine);

        cmdline.GetSwitch("-P", NULL, TRUE);  
        cmdline.GetSwitch("-chrome", NULL, TRUE);
        cmdline.GetSwitch("-profilesDir", NULL, TRUE);

        cdsSend.dwData = (DWORD) CD_NAVIGATETO;
        cdsSend.cbData = 2 + strlen(lpCmdLine);
        cdsSend.lpData = (char*) malloc( cdsSend.cbData );
        ((char*)cdsSend.lpData)[0] = OPEN_NORMAL;
        strcpy( ((char*)cdsSend.lpData) + 1, lpCmdLine);

        SendMessage(hWndKM, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cdsSend);

        free((void*)cdsSend.lpData);
    }
    

    return 0;
}
