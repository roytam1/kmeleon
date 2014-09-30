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

#include "mozilla.h"
#include "nsIDOMEvent.h"

extern void SetFullScreen(HWND, int, bool);

NS_IMPL_ISUPPORTS1(CDomEventListener, nsIDOMEventListener)

NS_IMETHODIMP CDomEventListener::HandleEvent (nsIDOMEvent* aEvent)
{
	nsresult rv;

	nsCOMPtr<nsIDOMEventTarget> eventTarget;
	rv = aEvent->GetOriginalTarget(getter_AddRefs(eventTarget));
	NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(eventTarget);
	if (!targetNode) return NS_OK;

	mFullscreenDoc = do_QueryInterface(eventTarget);
	if (!mFullscreenDoc) return NS_OK;		

	bool fullscreen;
	mFullscreenDoc->GetMozFullScreen(&fullscreen);
	SetFullScreen(mhWnd, fullscreen ? 1 : 0, true);
	if (!fullscreen) mFullscreenDoc = nullptr;
	return NS_OK;
}

