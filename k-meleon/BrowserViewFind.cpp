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

#include "nsIFind.h"
#include "nsIDOMText.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMWindowCollection.h"

static const PRUnichar kSpan[] = NS_LL("span");
static const PRUnichar kStyle[] = NS_LL("style");
static const PRUnichar kClass[] = NS_LL("class");
static const PRUnichar kHighlighClassName[] = NS_LL("km_hightlight_class");
static const PRUnichar kDefaultHighlightStyle[] = NS_LL("display: inline;font-size: inherit;padding: 0;color: black;background-color: yellow;");


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
}

void CBrowserView::OnFindPrev() {
	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
   if(!finder) return;

   finder->SetFindBackwards(PR_TRUE);
   OnFindNext();
   finder->SetFindBackwards(PR_FALSE);
}

PRBool CheckNode(nsIDOMElement* elem)
{
	if (elem) 
	{
		nsEmbedString className;
		elem->GetAttribute(nsEmbedString(kClass), className);
		if (className.Equals(nsEmbedString(kHighlighClassName)))
			return PR_TRUE;
	}
	return PR_FALSE;
}

BOOL Highlight(const PRUnichar* backcolor, const PRUnichar* word, nsIDOMWindow* dom)
{
	nsresult rv;

	if (!dom)
		return FALSE;

	nsCOMPtr<nsIDOMDocument> document;
	rv = dom->GetDocument(getter_AddRefs(document));
	NS_ENSURE_SUCCESS(rv, FALSE);

	nsCOMPtr<nsIDOMWindowCollection> frames;
	dom->GetFrames(getter_AddRefs(frames));
	if (frames)
	{
		PRUint32 nbframes;
		nsCOMPtr<nsIDOMWindow> frame;
		frames->GetLength(&nbframes);
		for (PRUint32 i = 0; i<nbframes; i++)
		{
			frames->Item(i, getter_AddRefs(frame));
			Highlight(backcolor, word, frame);
		}
	}

	nsCOMPtr<nsIFind> find = do_CreateInstance("@mozilla.org/embedcomp/rangefind;1", &rv);
	if (NS_FAILED(rv)) return FALSE;

	find->SetCaseSensitive(theApp.preferences.bFindMatchCase);
    find->SetFindBackwards(PR_FALSE);

	nsCOMPtr<nsIDOMDocumentRange> docrange = do_QueryInterface(document);
	if (!docrange) return FALSE;

	nsCOMPtr<nsIDOMRange> searchRange, startPt, endPt;
    docrange->CreateRange(getter_AddRefs(searchRange));
	docrange->CreateRange(getter_AddRefs(startPt));
	docrange->CreateRange(getter_AddRefs(endPt));

	nsCOMPtr<nsIDOMHTMLElement> body;
	nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(document));
	NS_ENSURE_TRUE(htmlDoc, FALSE);
	
	htmlDoc->GetBody(getter_AddRefs(body));
	NS_ENSURE_TRUE(body, FALSE);
	
	nsCOMPtr<nsIDOMNodeList> nodelist;
	rv = body->GetChildNodes(getter_AddRefs(nodelist));
	NS_ENSURE_SUCCESS(rv, FALSE);

	PRUint32 count;
	nodelist->GetLength(&count);

	searchRange->SetStart(body, 0);
	searchRange->SetEnd(body, count);
	startPt->SetStart(body, 0);
	startPt->SetEnd(body, 0);
	endPt->SetStart(body, count);
	endPt->SetEnd(body, count);

	if(!backcolor)
	{
		// To optimize

		while (1)
		{
			nsCOMPtr<nsIDOMRange> retRange;
			rv = find->Find(word, searchRange, startPt, endPt, getter_AddRefs(retRange));
			if (NS_FAILED(rv) || !retRange)	break;
			
			nsCOMPtr<nsIDOMNode> startContainer;
			retRange->GetStartContainer(getter_AddRefs(startContainer));
			if (!startContainer) break;

			nsCOMPtr<nsIDOMNode> node;
			rv = startContainer->GetParentNode(getter_AddRefs(node));
			nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(node);
			
			PRBool bFound = CheckNode(elem);
			
			if (!bFound && elem)
			{ 
				nsCOMPtr<nsIDOMNode> node;
				retRange->GetEndContainer(getter_AddRefs(node));
				if (!node) break;
				
				while (1)
				{
					rv = elem->GetParentNode(getter_AddRefs(node));
					elem = do_QueryInterface(node);
					if (NS_FAILED(rv) || !elem) break;
					
					if (CheckNode(elem)) {
						bFound = PR_TRUE;
						break;
					}
				}
			}

			if (bFound)
			{
				nsCOMPtr<nsIDOMDocumentFragment> fragment;
				document->CreateDocumentFragment(getter_AddRefs(fragment));
				nsCOMPtr<nsIDOMNode> next, parent, child, notused;
				rv = elem->GetNextSibling(getter_AddRefs(next));
				if (NS_FAILED(rv) || !retRange)	break;
				rv = elem->GetParentNode(getter_AddRefs(parent));
				if (NS_FAILED(rv) || !retRange)	break;
				while(1)
				{
					elem->GetFirstChild(getter_AddRefs(child));
					if (NS_FAILED(rv) || !child) break;
					fragment->AppendChild(child, getter_AddRefs(notused));
				}
				docrange->CreateRange(getter_AddRefs(startPt));
				startPt->SetStartAfter(elem);
				parent->RemoveChild(elem, getter_AddRefs(notused));
				parent->InsertBefore(fragment, next, getter_AddRefs(notused));
				parent->Normalize();
			}
            else
			{
				nsCOMPtr<nsIDOMNode> ec;
				retRange->GetEndContainer(getter_AddRefs(ec));
				if (!ec) break;

				PRInt32 startOffset, endOffset;
				retRange->GetStartOffset(&startOffset);
				retRange->GetEndOffset(&endOffset);

				docrange->CreateRange(getter_AddRefs(startPt));
				startPt->SetStart(ec, endOffset);	
			}
			startPt->Collapse(PR_TRUE);
		}
		return TRUE;
	}

	nsCOMPtr<nsIDOMElement> baseElement;
	rv = document->CreateElement(nsEmbedString(kSpan), getter_AddRefs(baseElement));
	NS_ENSURE_SUCCESS(rv, FALSE);

	baseElement->SetAttribute(nsEmbedString(kStyle), nsEmbedString(kDefaultHighlightStyle));
	baseElement->SetAttribute(nsEmbedString(kClass), nsEmbedString(kHighlighClassName));

	while (1)
	{
		nsCOMPtr<nsIDOMRange> retRange;
		rv = find->Find(word, searchRange, startPt, endPt, getter_AddRefs(retRange));
		if (NS_FAILED(rv) || !retRange)	break;

		nsCOMPtr<nsIDOMNode> tNode;
		baseElement->CloneNode(PR_FALSE, getter_AddRefs(tNode));
		nsCOMPtr<nsIDOMElement> hNode = do_QueryInterface(tNode);
		if (!hNode) break;

		nsCOMPtr<nsIDOMNode> sc;
		retRange->GetStartContainer(getter_AddRefs(sc));

		nsCOMPtr<nsIDOMNode> ec;
		retRange->GetEndContainer(getter_AddRefs(ec));

        PRInt32 startOffset, endOffset;
		retRange->GetStartOffset(&startOffset);
		retRange->GetEndOffset(&endOffset);

		nsCOMPtr<nsIDOMDocumentFragment> fragment;
		rv = retRange->ExtractContents(getter_AddRefs(fragment));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNode> fchild,lchild;
		fragment->GetFirstChild(getter_AddRefs(fchild));
		fragment->GetLastChild(getter_AddRefs(lchild));
		if (!fchild || !lchild) break;

		PRUint16 ftype, ltype;
		fchild->GetNodeType(&ftype);
		lchild->GetNodeType(&ltype);
		
		nsCOMPtr<nsIDOMNode> before;
		if (ftype==nsIDOMNode::ELEMENT_NODE && ltype==nsIDOMNode::ELEMENT_NODE)
		{
			rv = ec->GetParentNode(getter_AddRefs(before));
			if (NS_FAILED(rv)||!before) break;
		}
		else 
		{
			nsCOMPtr<nsIDOMText> container = do_QueryInterface(sc);
			PRInt32 offset = startOffset;
			if (ftype==nsIDOMNode::ELEMENT_NODE)
			{
				container = do_QueryInterface(ec);
				offset = 0;
			}

			if (!container) break;

			nsCOMPtr<nsIDOMText> beforeText;
			rv = container->SplitText(offset, getter_AddRefs(beforeText));
			before = do_QueryInterface(beforeText);
			if (NS_FAILED(rv)||!before) break;
		}
			
		nsCOMPtr<nsIDOMNode> parent;
		rv = before->GetParentNode(getter_AddRefs(parent));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNode> notused;
		rv = hNode->AppendChild(fragment, getter_AddRefs(notused));
		if (NS_FAILED(rv)) break;

		rv = parent->InsertBefore(hNode, before, getter_AddRefs(notused));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMDocument> docowner;
		rv = hNode->GetOwnerDocument(getter_AddRefs(docowner));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMDocumentRange> docOwnerRange = do_QueryInterface(document);
		if (!docOwnerRange) break;
		
		rv = docOwnerRange->CreateRange(getter_AddRefs(startPt));
		if (NS_FAILED(rv)) break;

		nsCOMPtr<nsIDOMNodeList> nodelist;
		rv = hNode->GetChildNodes(getter_AddRefs(nodelist));
		if (NS_FAILED(rv) || !nodelist) break;

		PRUint32 count;
		nodelist->GetLength(&count);

		startPt->SetStart(hNode, count);
		startPt->SetEnd(hNode, count);
		
	}

	return TRUE;
}


void CBrowserView::Highlight(BOOL active)
{
	nsCOMPtr<nsIDOMWindow> dom;
	mWebBrowser->GetContentDOMWindow(getter_AddRefs(dom));
	if (!dom) return;

	CString str;
	str.LoadString(IDS_HIGHLIGHTING);
	mpBrowserFrame->m_wndStatusBar.SetPaneText(0, str);

	if (active)
	{
		nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
		if(!finder) return;

		PRUnichar *stringBuf = nsnull;
		finder->GetSearchString(&stringBuf);
		if (stringBuf[0]) 
		{
			::Highlight(L"Yellow", stringBuf, dom);
			m_lastHighlightWord = stringBuf;
		}
		nsMemory::Free(stringBuf);
	}
	else
	{
		::Highlight(NULL, m_lastHighlightWord.get(), dom);
		m_lastHighlightWord.SetLength(0);
	}

	str.LoadString(AFX_IDS_IDLEMESSAGE);
	mpBrowserFrame->m_wndStatusBar.SetPaneText(0, str);
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

	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(finder)
		finder->SetMatchCase(theApp.preferences.bFindMatchCase ? PR_TRUE : PR_FALSE);
}

void CBrowserView::OnWrapAround()
{
	if (!mpBrowserFrame->m_wndFindBar)
		theApp.preferences.bFindWrapAround = !theApp.preferences.bFindWrapAround;
	else
		theApp.preferences.bFindWrapAround = mpBrowserFrame->m_wndFindBar->WrapAround();

	nsCOMPtr<nsIWebBrowserFind> finder(do_GetInterface(mWebBrowser));
	if(finder)
		finder->SetWrapFind(theApp.preferences.bFindWrapAround ? PR_TRUE : PR_FALSE);
}

void CBrowserView::OnHighlight()
{
	theApp.preferences.bFindHighlight = mpBrowserFrame->m_wndFindBar->Highlight();
	Highlight(FALSE);
	if (theApp.preferences.bFindHighlight)
		Highlight(TRUE);
}
