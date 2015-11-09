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
 * Ron Pruitt <wa84it@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2005-2011
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

const listNames = [ "addL", "rmL", "delL" ];

function init()
{
  var tagstr = window.arguments[0].tagstr;
	var allPct = window.arguments[0].allPct;

  var addList = document.getElementById("addList");
  var rmList = document.getElementById("rmList");
  var delList = document.getElementById("delList");
	var Lists = [ addList, rmList, delList ];

	if (tagstr == "")
	{
		document.getElementById("tagEditTopRow").hidden = true;
		document.getElementById("tagEditBottomRight").hidden = true;
	}
	else
	{
		for (i=0; i<3; i++)
		{
		  var names = new Array();
		  names = tagstr.split("\/");
		  var index = 0;
		  while (names.length >= 1)
		  {
				var name = names.shift();
				var tmp = document.createElement("listitem");
				tmp.setAttribute("type","checkbox");
		    tmp.setAttribute("label",name);
	
				var vl1 = Math.round(allPct[index]*255);
				var vl2 = 255 - vl1;
				var vlb = vl1.toString(16);
				var vlr = vl2.toString(16);
				if (vlb == "0") vlb = "00";
				if (vlr == "0") vlr = "00";
				tmp.setAttribute("style","color: #" + vlr + "00" + vlb);
	
		    if (name.length > 20) tmp.setAttribute("tooltiptext",name);
				tmp.setAttribute("id",""+listNames[i]+index);
		    Lists[i].appendChild(tmp);
				tmp.setAttribute("id",""+listNames[i]+index);
				index++;
		  }
		}
	}
}

function doAccept()
{
  var names = new Array();
  names = window.arguments[0].tagstr.split("\/");
  window.arguments[0].ok = true;
	window.arguments[0].addstr = getIt(listNames[0],names);
	window.arguments[0].rmstr = getIt(listNames[1],names);
	window.arguments[0].delstr = getIt(listNames[2],names);
	window.arguments[0].newaddstr = document.getElementById("newTagAdd").value;
  return true;
}

function getIt(baseName,names)
{
	var retArray = new Array();
  var elem = document.getElementById(baseName+"0");
  var index = 0;
  while (elem != null)
  {
    if (elem.checked) retArray.push(names[index]);
    index++;
    elem = document.getElementById(""+baseName+index);
  }
	if (retArray.length)
		return retArray.join("\/");
	else
		return null;
}
