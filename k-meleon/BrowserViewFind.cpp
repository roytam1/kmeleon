/*
*  Copyright (C) 2000 Christophe Thibault, Brian Harris, Jeff Doozan
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

/*
  This code handles the Find... dialog box stuff
*/

#include "stdafx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Dialogs.h"
#include "BrowserFrm.h"
#include "BrowserView.h"

void CBrowserView::OnShowFindDlg() 
{
    // When the the user chooses the Find menu item
    // and if a Find dlg. is already being shown
    // just set focus to the existing dlg instead of
    // creating a new one
    if(m_pFindDlg)
    {
	m_pFindDlg->SetFocus();
	return;
    }

    CString csSearchStr;
    PRBool bMatchCase = PR_FALSE;
    PRBool bMatchWholeWord = PR_FALSE;
    PRBool bWrapAround = PR_FALSE;
    PRBool bSearchBackwards = PR_FALSE;

    // See if we can get and initialize the dlg box with
    // the values/settings the user specified in the previous search
    nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
    if(finder)
    {
	nsXPIDLString stringBuf;
	finder->GetSearchString(getter_Copies(stringBuf));
	csSearchStr = stringBuf.get();

	finder->GetMatchCase(&bMatchCase);
	finder->GetEntireWord(&bMatchWholeWord);
	finder->GetWrapFind(&bWrapAround);
	finder->GetFindBackwards(&bSearchBackwards);		
    }

    m_pFindDlg = new CFindDialog(csSearchStr, bMatchCase, bMatchWholeWord,
				 bWrapAround, bSearchBackwards, this);
    m_pFindDlg->Create(TRUE, NULL, NULL, 0, this);
}

// This will be called whenever the user pushes the Find
// button in the Find dialog box
// This method gets bound to the WM_FINDMSG windows msg via the 
//
//	   ON_REGISTERED_MESSAGE(WM_FINDMSG, OnFindMsg) 
//
//	message map entry.
//
// WM_FINDMSG (which is registered towards the beginning of this file)
// is the message via which the FindDialog communicates with this view
//
LRESULT CBrowserView::OnFindMsg(WPARAM wParam, LPARAM lParam)
{
    nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
    if(!finder)
	return NULL;

    // Get the pointer to the current Find dialog box
    CFindDialog* dlg = (CFindDialog *) CFindReplaceDialog::GetNotifier(lParam);
    if(!dlg) 
	return NULL;

    // Has the user decided to terminate the dialog box?
    if(dlg->IsTerminating())
	return NULL;

    if(dlg->FindNext())
    {
        USES_CONVERSION;
        finder->SetSearchString(T2W(dlg->GetFindString().GetBuffer(0)));
        finder->SetMatchCase(dlg->MatchCase() ? PR_TRUE : PR_FALSE);
        finder->SetEntireWord(dlg->MatchWholeWord() ? PR_TRUE : PR_FALSE);
        finder->SetWrapFind(dlg->WrapAround() ? PR_TRUE : PR_FALSE);
        finder->SetFindBackwards(dlg->SearchBackwards() ? PR_TRUE : PR_FALSE);
 
	PRBool didFind;
	nsresult rv = finder->FindNext(&didFind);
	
        if(!didFind)
	{
            MessageBox("Not found.");
            dlg->SetFocus();
        }

        return (NS_SUCCEEDED(rv) && didFind);
    }

    return 0;
}

void CBrowserView::OnFindNext() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));

   if(!finder) return;

	nsXPIDLString stringBuf;
   finder->GetSearchString(getter_Copies(stringBuf));
   if (stringBuf.get()[0]) {

      PRBool didFind;
	   finder->FindNext(&didFind);
   }
   else {
      OnShowFindDlg();
   }
}

void CBrowserView::OnFindPrev() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));

   if(!finder) return;

   PRBool rv;

   finder->GetFindBackwards(&rv);
   finder->SetFindBackwards(rv^2);  // reverse the find direction

	nsXPIDLString stringBuf;
   finder->GetSearchString(getter_Copies(stringBuf));
   if (stringBuf.get()[0]) {
      PRBool didFind;
	   finder->FindNext(&didFind);
   }
   else {
      OnShowFindDlg();
   }
      
   finder->GetFindBackwards(&rv);
   finder->SetFindBackwards(rv^2);  // reset the initial find direction
}
