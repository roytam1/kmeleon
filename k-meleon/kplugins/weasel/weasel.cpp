/*
*  Copyright (C) 2000 Brian Harris
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
// weasel.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "resource.h"

#include <vector>

typedef std::vector<HWND> hwndVector;

#define KMELEON_PLUGIN_EXPORTS
#include "../kmeleon_plugin.h"

#include "pop.h"

#define PLUGIN_NAME "Weasel Email Plugin"

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
   switch (ul_reason_for_call)
   {
   case DLL_PROCESS_ATTACH:
   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
   case DLL_PROCESS_DETACH:
      break;
   }

   return TRUE;
}

int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

pluginFunctions pFunc = {
   Init,
   Create,
   Config,
   Quit,
   DoMenu,
   DoRebar
};

kmeleonPlugin kPlugin = {
   KMEL_PLUGIN_VER,
   PLUGIN_NAME,
   &pFunc
};

static DWORD WINAPI MainThread( LPVOID lpParameter /* thread data*/ );
static HANDLE gMainThread;

static int gDie;
static int gCheckNow;
static int gEnabled;

static HIMAGELIST gImages;

static UINT gCheckID;
static UINT gMailID;
static UINT gConfigID;

static int gNumberOfMessages;

char szServerName[255];
char szUsername[255];
char szPassword[255];

char szNewMailSound[255];

int Init(){
   // allocate some ids
   gCheckID = kPlugin.kf->GetCommandIDs(1); 
   gMailID = kPlugin.kf->GetCommandIDs(1);
   gConfigID = kPlugin.kf->GetCommandIDs(1);

   gImages = ImageList_LoadImage(kPlugin.hDllInstance, MAKEINTRESOURCE(IDB_TOOLBAR_BUTTONS), 12, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR );

	/* sockets */
   {
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD( 2, 2 ); 

		if (WSAStartup( wVersionRequested, &wsaData ) != 0){
			MessageBox(NULL, "Could Not Start up Winsock!", PLUGIN_NAME " Error!", 0);
			return false;
		}
	}

   // read some configs
   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.mail.server", szServerName, "");
   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.mail.username", szUsername, "");
   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.mail.password", szPassword, "");

   gEnabled = 0;
   kPlugin.kf->GetPreference(PREF_BOOL,   "kmeleon.mail.enabled", &gEnabled, &gEnabled);

   kPlugin.kf->GetPreference(PREF_STRING, "kmeleon.mail.sound", szNewMailSound, "c:\\windows\\media\\ding.wav");

   gDie = false;
   gCheckNow = true;
   gNumberOfMessages = 0;

   /* thread */
	{
		DWORD threadId;
		gMainThread = CreateThread(NULL, 0, MainThread, 0, 0, &threadId);
		SetThreadPriority(gMainThread, THREAD_PRIORITY_BELOW_NORMAL);
   }

   return true;
}

hwndVector gWindowList;

WNDPROC KMeleonWndProc;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void Create(HWND parent){
   KMeleonWndProc = (WNDPROC) GetWindowLong(parent, GWL_WNDPROC);
   SetWindowLong(parent, GWL_WNDPROC, (LONG)WndProc);

   gWindowList.push_back(parent);
}

CALLBACK OptionsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Config(HWND parent){
   DialogBoxParam(kPlugin.hDllInstance, MAKEINTRESOURCE(IDD_DIALOG1), parent, OptionsProc, 0);
}

void Quit(){
   ImageList_Destroy(gImages);

   gDie = 1;
   // wait 3 seconds
   if (WaitForSingleObject(gMainThread, 3000) == WAIT_TIMEOUT){
      TerminateThread(gMainThread, 0);
   }
   CloseHandle(gMainThread);
	WSACleanup();
}

void DoMenu(HMENU menu, char *param){
   AppendMenu(menu, MF_STRING, gCheckID, "Check For New Mail");
   AppendMenu(menu, MF_STRING, gMailID, "Open Mail");
   AppendMenu(menu, MF_STRING, gConfigID, "Config Mail");
}

void DoRebar(HWND rebarWnd){

   DWORD dwStyle = 0x40 | /*the 40 gets rid of an ugly border on top.  I have no idea what flag it corresponds to...*/
      CCS_NOPARENTALIGN | CCS_NORESIZE |
      TBSTYLE_FLAT | TBSTYLE_TRANSPARENT /* | TBSTYLE_AUTOSIZE | TBSTYLE_LIST | TBSTYLE_TOOLTIPS */;

   // Create the toolbar control to be added.
   HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, "",
      WS_CHILD | dwStyle,
      0,0,0,0,
      rebarWnd, (HMENU)/*id*/200,
      kPlugin.hDllInstance, NULL
      );

   if (!hwndTB){
      MessageBox(NULL, "Failed to create mail toolbar", NULL, 0);
      return;
   }

   SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)gImages);
   SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)gImages);

   SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(0, 100));

   TBBUTTON button;
   button.iBitmap = (gNumberOfMessages>9?10:gNumberOfMessages);
   button.idCommand = gMailID;
   button.iString = 0;

   button.dwData = 0;
   button.fsState = TBSTATE_ENABLED;
   button.fsStyle = TBSTYLE_BUTTON;
   button.bReserved[0] = 0;

   SendMessage(hwndTB, TB_INSERTBUTTON, (WPARAM)0, (LPARAM)&button);

   // Register the band name and child hwnd
   kPlugin.kf->RegisterBand(hwndTB, "Weasel Email");
   
   SetWindowText(hwndTB, "Weasel");

   // Get the height of the toolbar.
   DWORD dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);

   REBARBANDINFO rbBand;
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = /*RBBIM_TEXT |*/
      RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE |
      RBBIM_SIZE | RBBIM_IDEALSIZE;

   rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_VARIABLEHEIGHT;
   rbBand.lpText     = "Weasel";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cyIntegral = 1;
   rbBand.cyMaxChild = rbBand.cyMinChild;
   rbBand.cxIdeal    = LOWORD(dwBtnSize);
   rbBand.cx         = rbBand.cxIdeal;

   // Add the band that has the toolbar.
   SendMessage(rebarWnd, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message) {
   case WM_COMMAND:
      {
         WORD command = LOWORD(wParam);
         if (command == gCheckID){
            gCheckNow = true;
            return true;
         }
         if (command == gMailID){
            ShellExecute(hWnd, "open", "mailto:", "", "", SW_SHOWNORMAL);
            return true;
         }
         if (command == gConfigID){
            Config(NULL);
            return true;
         }
      }
      break;
   case WM_CLOSE:
      if (gWindowList.size() == 1){
         gWindowList.clear();
      }
      else{
         hwndVector::iterator window;
         for (window = gWindowList.begin(); window && window != gWindowList.end(); window++) {
            if (*window == hWnd){
               gWindowList.erase(window);
            }
         }
      }
      break;
   }
   
   return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}

void BrowseForFile(HWND hWnd, int id, char *filename, char *filter)
{
   OPENFILENAME ofn;       // common dialog box structure
   char szFile[260];
   strcpy(szFile, filename);

   // Initialize OPENFILENAME
   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hWnd;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = sizeof(szFile);
   ofn.lpstrFilter = filter;
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = NULL;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

   // Display the Open dialog box. 

   if (GetOpenFileName(&ofn)==TRUE) 
      SetDlgItemText(hWnd, id, ofn.lpstrFile);

}

CALLBACK OptionsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg){
   case WM_INITDIALOG:
      SetDlgItemText(hDlg, IDC_EDIT_SERVER, szServerName);
      SetDlgItemText(hDlg, IDC_EDIT_USERNAME, szUsername);
      SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, szPassword);
      CheckDlgButton(hDlg, IDC_CHECK_ENABLED, (gEnabled?BST_CHECKED:BST_UNCHECKED));
      SetDlgItemText(hDlg, IDC_EDIT_SOUND, szNewMailSound);
      return true;
   case WM_COMMAND:
      {
         if (HIWORD(wParam) == BN_CLICKED){
            WORD id = LOWORD(wParam);
            switch(id){
            case IDC_BUTTON_BROWSE:
               BrowseForFile(hDlg, IDC_EDIT_SOUND, szNewMailSound, "Sounds (*.wav)\0*.wav\0");
               break;

            case IDC_BUTTON_TEST:
               {
                  char szFile[256];
                  GetDlgItemText(hDlg, IDC_EDIT_SOUND, szFile, 255);
                  PlaySound(szFile, NULL, SND_FILENAME|SND_ASYNC);
               }
               break;

            case IDOK:
               GetDlgItemText(hDlg, IDC_EDIT_SERVER, szServerName, 255);
               GetDlgItemText(hDlg, IDC_EDIT_USERNAME, szUsername, 255);
               GetDlgItemText(hDlg, IDC_EDIT_PASSWORD, szPassword, 255);
               gEnabled = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLED);
               GetDlgItemText(hDlg, IDC_EDIT_SOUND, szNewMailSound, 255);

               kPlugin.kf->SetPreference(PREF_STRING, "kmeleon.mail.server",   szServerName);
               kPlugin.kf->SetPreference(PREF_STRING, "kmeleon.mail.username", szUsername);
               kPlugin.kf->SetPreference(PREF_STRING, "kmeleon.mail.password", szPassword);
               kPlugin.kf->SetPreference(PREF_BOOL,   "kmeleon.mail.enabled",  &gEnabled);
               kPlugin.kf->SetPreference(PREF_STRING, "kmeleon.mail.sound",    szNewMailSound);

               // fall through
            case IDCANCEL:
               EndDialog(hDlg, id);
               break;
            }
         }
      }
   }
   return false;
}

// so it doesn't munge the function name
extern "C" {
   KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
      return &kPlugin;
   }
}

void ChangeImage(int imageIndex)
{
   TBBUTTONINFO button;
   button.cbSize = sizeof(button);
   button.dwMask = TBIF_IMAGE;
   button.iImage = imageIndex;

   hwndVector::iterator window;
   for (window = gWindowList.begin(); window && window != gWindowList.end(); window++) {
      HWND rbWnd = FindWindowEx(*window, NULL, REBARCLASSNAME, NULL);
      HWND tbWnd = FindWindowEx(rbWnd, NULL, TOOLBARCLASSNAME, "Weasel");
      SendMessage(tbWnd, TB_SETBUTTONINFO, gMailID, (LPARAM)&button);
   }
}

DWORD WINAPI MainThread( LPVOID lpParameter /* thread data*/ ){
   int counter;

   CPOP pop;

   int previousNumOfMsgs;

   gDie = 0;
   while (!gDie){
      // wait 60 seconds (do this rather than Sleep(5000) so we can check gDie)
      if (!gEnabled || (!gCheckNow && counter < 60)){
         counter++;
         if (gNumberOfMessages) {
            if (counter % 2) {
               ChangeImage(gNumberOfMessages>9?10:gNumberOfMessages);
            }
            else {
               ChangeImage(0);
            }
         }
         Sleep(1000);
         continue;
      }
      counter = 0;
      gCheckNow = false;

      if (!pop.Setup()){
         continue;
      }
      if (!pop.Connect(szServerName)){
         continue;
      }
      if (!pop.Login(szUsername, szPassword)){
         continue;
      }

      previousNumOfMsgs = gNumberOfMessages;
      gNumberOfMessages = pop.NumberOfMessages();

      if (gNumberOfMessages != previousNumOfMsgs) {
         ChangeImage(gNumberOfMessages>9?10:gNumberOfMessages);

         if (gNumberOfMessages && szNewMailSound)
            PlaySound(szNewMailSound, NULL, SND_FILENAME|SND_ASYNC);
      }

      pop.Quit();
   }
	return 1;
}

void CPOP::SocketError(char *format, ...)
{
  char error[255];

  va_list args;
  va_start(args, format);
  vsprintf(error, format, args);
  va_end(args);

  MessageBox(NULL, error, PLUGIN_NAME " Error!", 0);
}
