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
#include "BrowserWindow.h"

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

void CBrowserView::OnFindNext() {

	if(!mpBrowserFrame->m_wndFindBar) {
		WCHAR* searchString = m_pWindow->GetSearchString();
		if (!searchString || !*searchString) {
			mpBrowserFrame->OnShowFindBar();
			mpBrowserFrame->m_wndFindBar->OnFound();
		}
		else {
			BOOL didFind = m_pWindow->FindNext(FALSE);
			if (!didFind) {
				mpBrowserFrame->OnShowFindBar();
				mpBrowserFrame->m_wndFindBar->OnNotFound();
			}
		}
		free(searchString);
	}
	else
	{
		const WCHAR* searchString = mpBrowserFrame->m_wndFindBar->GetUFindString();
		if (searchString) {
#ifndef FINDBAR_USE_TYPEAHEAD	
			BOOL didFind = m_pWindow->Find(searchString, mpBrowserFrame->m_wndFindBar->StartSel());
#else
			BOOL didFind = m_pWindow->Find(searchString);
#endif
			if (*searchString && !didFind) {
				mpBrowserFrame->m_wndFindBar->OnNotFound();
			}
			else {
				mpBrowserFrame->m_wndFindBar->OnFound();
			}
		}
	}
/*

	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(!finder) return;
	USES_CONVERSION;

	
	if(mpBrowserFrame->m_wndFindBar)
	{
		BOOL didFind;
		WCHAR* searchString = mpBrowserFrame->m_wndFindBar->GetUFindString();

		if (searchString) {
#ifndef FINDBAR_USE_TYPEAHEAD	
			didFind = m_Finder->Find(searchString, mpBrowserFrame->m_wndFindBar->StartSel());
#else
			didFind = m_Finder->Find(searchString);
#endif
			if (didfind) {


	if(mpBrowserFrame->m_wndFindBar)
    {
		finder->SetSearchString(mpBrowserFrame->m_wndFindBar->GetUFindString());
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

   if (stringBuf[0]) 
   {
	   PRBool didFind;
       finder->FindNext(&didFind);
	   if (!didFind)
	   {
			if (!mpBrowserFrame->m_wndFindBar) 
				mpBrowserFrame->OnShowFindBar();
			mpBrowserFrame->m_wndFindBar->OnNotFound();
	   }
	   else
		   if (mpBrowserFrame->m_wndFindBar)
			   mpBrowserFrame->m_wndFindBar->OnFound();
   }
   else {
      mpBrowserFrame->OnShowFindBar();
	  mpBrowserFrame->m_wndFindBar->OnFound();
   }
   
   nsMemory::Free(stringBuf);
   */
}

void CBrowserView::OnFindPrev()
{
	BOOL didFind = m_pWindow->FindNext(TRUE);
	if (!didFind) {
		mpBrowserFrame->OnShowFindBar();
		mpBrowserFrame->m_wndFindBar->OnNotFound();
	}
	else
		mpBrowserFrame->m_wndFindBar->OnFound();
	/*
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(!finder) return;

   finder->SetFindBackwards(PR_TRUE);
   OnFindNext();
   finder->SetFindBackwards(PR_FALSE);*/
}



void CBrowserView::OnMatchCase()
{
	if (theApp.preferences.bFindHighlight) 
		Highlight(FALSE);

	if (!mpBrowserFrame->m_wndFindBar) 
		theApp.preferences.bFindMatchCase = !theApp.preferences.bFindMatchCase;
	else
		theApp.preferences.bFindMatchCase = mpBrowserFrame->m_wndFindBar->MatchCase();

	if (theApp.preferences.bFindHighlight) 
		Highlight(TRUE);

	m_pWindow->SetMatchCase(theApp.preferences.bFindMatchCase ? PR_TRUE : PR_FALSE);
}

void CBrowserView::OnWrapAround()
{
	if (!mpBrowserFrame->m_wndFindBar)
		theApp.preferences.bFindWrapAround = !theApp.preferences.bFindWrapAround;
	else
		theApp.preferences.bFindWrapAround = mpBrowserFrame->m_wndFindBar->WrapAround();

	m_pWindow->SetWrapAround(theApp.preferences.bFindWrapAround ? PR_TRUE : PR_FALSE);
}

void CBrowserView::OnHighlight()
{
	theApp.preferences.bFindHighlight = mpBrowserFrame->m_wndFindBar->Highlight();
	Highlight(FALSE);
	if (theApp.preferences.bFindHighlight)
		Highlight(TRUE);
}
