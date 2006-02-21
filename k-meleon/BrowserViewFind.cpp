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
*/

/*
  This code handles the Findbar
*/

#include "stdafx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "Dialogs.h"
#include "BrowserFrm.h"
#include "BrowserView.h"
#include "nsISelection.h"
#include "nsIDOMWindowCollection.h"


// A new handler for WM_FINDMSG, if a plugin want to use it.
// At first I wanted to put the findbar in a plugin but...
// wParam (char*) Text to search for in UTF8
// lParam flags

#define FM_WRAPAROUND		0x1
#define FM_MATCHCASE		0x2
#define FM_SEARCHBACKWARD	0x4

/*LRESULT CBrowserView::OnFindMsg(WPARAM wParam, LPARAM lParam)
{
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
    if(!finder)
		return 0;
	
	nsEmbedString searchStrUTF16;
    NS_CStringToUTF16(nsEmbedCString((char*)wParam), NS_CSTRING_ENCODING_UTF8, searchStrUTF16);

	finder->SetFindBackwards( lParam & FM_SEARCHBACKWARD ? PR_TRUE : PR_FALSE);
	finder->SetMatchCase( lParam & FM_MATCHCASE ? PR_TRUE : PR_FALSE);
	finder->SetWrapFind( lParam & FM_WRAPAROUND ? PR_TRUE : PR_FALSE);
	finder->SetSearchString(searchStrUTF16.get());

	PRBool didFind;
    finder->FindNext(&didFind);

	return (didFind == PR_TRUE ? 1 : 0);
}*/
#include "nsITypeAheadFind.h"

#ifndef FINDBAR_USE_TYPEAHEAD
// This function collapse the current selection in
// the window and in frames. See below for its purpose.
void CollapseSelToStartInFrame(nsIDOMWindow* dom)
{
	if (!dom) return;

	// Because IFrame exists
	nsCOMPtr<nsISelection> sel;
	dom->GetSelection(getter_AddRefs(sel));
	if (sel) sel->CollapseToStart();

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		frames->GetLength(&nbframes);
		if (nbframes>0)
		{
			nsCOMPtr<nsIDOMWindow> frame;
			for (PRUint32 i = 0; i<nbframes; i++)
			{
				frames->Item(i, getter_AddRefs(frame));
				CollapseSelToStartInFrame(frame);
			}
		}
	}
}
#endif

void CBrowserView::OnFindNext() {

	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(!finder) return;
	USES_CONVERSION;
#ifndef FINDBAR_USE_TYPEAHEAD	
	if(mpBrowserFrame->m_wndFindBar)
    {
		finder->SetSearchString(T2CW(mpBrowserFrame->m_wndFindBar->GetFindString()));
		//finder->SetMatchCase(mpBrowserFrame->m_wndFindBar->MatchCase() ? PR_TRUE : PR_FALSE);
		//finder->SetWrapFind(mpBrowserFrame->m_wndFindBar->WrapAround() ? PR_TRUE : PR_FALSE);
		
		// HACK because not use typeahead
		// The problem with the autosearch feature is that 
		// webbrowserfind start to search at the end of the 
		// current selection. But with autosearch it should
		// start at the beginning. So I collapse the selection.
        if (mpBrowserFrame->m_wndFindBar->StartSel())
		{
			nsCOMPtr<nsIDOMWindow> dom(do_GetInterface(mWebBrowser));
			if (dom)
			{
				// Have to look if we have frames. It's a little violent
				// currently. The observer is also passing the root and 
				// not the frame so it's useless.
				
				CollapseSelToStartInFrame(dom);
			}
		}
    }
#endif
   PRUnichar *stringBuf = nsnull;
   finder->GetSearchString(&stringBuf);
   if (stringBuf[0]) {
	   PRBool didFind;
       finder->FindNext(&didFind);
	   if (!didFind)
	   {
			if (!mpBrowserFrame->m_wndFindBar) mpBrowserFrame->OnShowFindBar();
			mpBrowserFrame->m_wndFindBar->OnNotFound();
	   }
	   else if (mpBrowserFrame->m_wndFindBar) mpBrowserFrame->m_wndFindBar->OnFound();
   }
   else {
      mpBrowserFrame->OnShowFindBar();
	  mpBrowserFrame->m_wndFindBar->OnFound();
   }
   
   nsMemory::Free(stringBuf);
}

void CBrowserView::OnFindPrev() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(!finder) return;

   finder->SetFindBackwards(PR_TRUE);
   OnFindNext();
   finder->SetFindBackwards(PR_FALSE);
}

void CBrowserView::OnWrapAround()
{
   theApp.preferences.bFindWrapAround = mpBrowserFrame->m_wndFindBar->WrapAround();
   nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(finder)
   finder->SetWrapFind(theApp.preferences.bFindWrapAround ? PR_TRUE : PR_FALSE);
}

void CBrowserView::OnMatchCase()
{
	theApp.preferences.bFindMatchCase = mpBrowserFrame->m_wndFindBar->MatchCase();
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(finder)
	finder->SetMatchCase(theApp.preferences.bFindMatchCase ? PR_TRUE : PR_FALSE);
}
