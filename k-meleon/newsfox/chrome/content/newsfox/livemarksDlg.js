/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is NewsFox.
 *
 * The Initial Developer of the Original Code is
 * Andrey Gromyko <andrey@gromyko.name>.
 * Portions created by the Initial Developer are Copyright (C) 2006-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////////////////////////////
function initLivemarksDlg()
{
	var listbox = document.getElementById('LivemarksListbox');
	var item;
	var livemarks = window.arguments[0].livemarks;
	var selected = window.arguments[0].selected;
	var showOnlyNew = window.arguments[0].onlyNew;
	var newSync = window.arguments[0].newSync;

	if (newSync)
		document.getElementById("newSyncmarksMsg").removeAttribute("hidden");
	if (showOnlyNew)
		document.getElementById("newLivemarksMsg").removeAttribute("hidden");

	var selectedLen = selected.length;

	if (livemarks.length > 0)
		document.getElementById("NFselectAllItem").removeAttribute("hidden");
	else
		document.getElementById("NFselectAllItem").hidden = true;

	for(var i=0; i < livemarks.length; i++ )
	{
		var livemark = livemarks[i];
		item = document.createElement('listitem');
		item.setAttribute('type',"checkbox");
		item.setAttribute('label',livemark.title);
		item.setAttribute('tooltiptext', livemark.URL);

		var isSelected = false;
		for( var j=0; j < selectedLen; j++ )
			if( livemark.URL == selected[j] )
			{
				isSelected = true;
				break;
			}
		if( isSelected )
			item.setAttribute('checked',"true");

		listbox.appendChild(item);
	}
}

function doAcceptLivemarksDlg()
{
	window.arguments[0].ok = true;
	var livemarks = window.arguments[0].livemarks;
	var selected = window.arguments[0].selected;
	selected.splice(0, 1000000); // emptying selected
	var showOnlyNew = window.arguments[0].onlyNew;

	var listbox = document.getElementById('LivemarksListbox');
	for(var i=1; i < listbox.getRowCount(); i++)
		if( listbox.getItemAtIndex(i).checked )
			selected.push(livemarks[i-1].URL);
	return true;
}

function onSelectAll()
{
	var chk = document.getElementById("NFselectAllItem").checked;
	var listbox = document.getElementById('LivemarksListbox');
	for (var i=0; i<listbox.childNodes.length; i++)
		listbox.childNodes[i].checked = chk;
}
