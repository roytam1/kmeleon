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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include "loader.h"
#include "config.h"
#include "tray.h"
#include "resource.h"
#include "resrc1.h"


UINT currentTrackPos;
HFONT hFontTitle=NULL;

void ShowDialog(HINSTANCE hInstance, HWND parent) {
   DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG), parent, ConfigDlgProc, NULL);   
}

void UpdateDialog(HWND hWnd) {
   switch (currentTrackPos) {
   case 1: // start page
      SetDlgItemText(hWnd, IDC_STATIC_TITLE, TEXT_START_TITLE);
      SetDlgItemText(hWnd, IDC_STATIC_INFO, TEXT_START_INFO);
      break;
   case 2: // window
      SetDlgItemText(hWnd, IDC_STATIC_TITLE, TEXT_WINDOW_TITLE);
      SetDlgItemText(hWnd, IDC_STATIC_INFO, TEXT_WINDOW_INFO);
      break;
   case 3: // browser
      SetDlgItemText(hWnd, IDC_STATIC_TITLE, TEXT_BROWSER_TITLE);
      SetDlgItemText(hWnd, IDC_STATIC_INFO, TEXT_BROWSER_INFO);
      break;
   case 4: // none
      SetDlgItemText(hWnd, IDC_STATIC_TITLE, TEXT_NONE_TITLE);
      SetDlgItemText(hWnd, IDC_STATIC_INFO, TEXT_NONE_INFO);
      break;
   }
}

BOOL CALLBACK ConfigDlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {

   case WM_INITDIALOG:

      SendDlgItemMessage(hWnd, IDC_SLIDER_PRELOAD, TBM_SETRANGE, FALSE, MAKELPARAM(1, 4));
      SendDlgItemMessage(hWnd, IDC_SLIDER_PRELOAD, TBM_SETTICFREQ, 1, 0);

      if (GetPersistStartPage())
         currentTrackPos = 1;
      else if (GetPersistWindow())
         currentTrackPos = 2;
      else if (GetPersistBrowser())
         currentTrackPos = 3;
      else
         currentTrackPos = 4;

      SendDlgItemMessage(hWnd, IDC_SLIDER_PRELOAD, TBM_SETPOS, TRUE, currentTrackPos);


      // make the title text use a bold font
      HFONT hFontCurrent;
      hFontCurrent = (HFONT) SendDlgItemMessage(hWnd, IDC_STATIC_TITLE, WM_GETFONT, 0, 0 ) ;
      if ( hFontCurrent ) {
         LOGFONT lf;
         if (GetObject(hFontCurrent, sizeof(LOGFONT), (LPSTR) &lf ) != NULL) {
            lf.lfWeight = FW_BOLD;
            hFontTitle = CreateFontIndirect (&lf);
            if (hFontTitle)
               SendDlgItemMessage(hWnd, IDC_STATIC_TITLE, WM_SETFONT, (WPARAM) hFontTitle, MAKELPARAM(FALSE, 0)) ;
         } 
      }

      UpdateDialog(hWnd);

      // return false unless we set explicitly set the focus
      return FALSE;

   case WM_VSCROLL:
      UINT newTrackPos;
      newTrackPos = SendDlgItemMessage(hWnd, IDC_SLIDER_PRELOAD, TBM_GETPOS, NULL, NULL);
      if (newTrackPos != currentTrackPos) {
         currentTrackPos = newTrackPos;
         UpdateDialog(hWnd);
      }
      break;

   case WM_CLOSE:
      if (hFontTitle)
         DeleteObject(hFontTitle);
      EndDialog(hWnd, NULL);
      break;

   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDOK:

         SetPersistStartPage(FALSE);
         SetPersistWindow (FALSE);
         SetPersistBrowser (FALSE);

         // the lack of break statements is intentional,
         // these settings are cumulative
         switch (currentTrackPos) {
         case 1:
            SetPersistStartPage (TRUE);
         case 2:
            SetPersistWindow (TRUE);
         case 3:
            SetPersistBrowser (TRUE);
         }

         SaveSettings();
         UpdateBrowser();

         EndDialog(hWnd, NULL);
         break;
      case IDCANCEL:
         EndDialog(hWnd, NULL);
         break;
      }
      break;

   default:
      return FALSE;

   }
   return TRUE;
}