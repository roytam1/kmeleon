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
 * Andy Frank <andy@andyfrank.com>.
 * Portions created by the Initial Developer are Copyright (C) 2005-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Andrey Gromyko <andrey@gromyko.name>
 *   Ron Pruitt <wa84it@gmail.com>
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

var blurring;
var filterChange = false;
var noFilter = true;

function init()
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
  var url        = window.arguments[0].url;
  var style      = window.arguments[0].style;
  var deleteOldStyle  = window.arguments[0].deleteOldStyle;
  var autoCheck  = window.arguments[0].autoCheck;
  var iconsrc    = window.arguments[0].iconsrc;
  var groupstr   = window.arguments[0].groupstr;
  var groupmemb  = window.arguments[0].groupmemb;
	var daysToKeep = window.arguments[0].daysToKeep;

	checkLocalPng("fOlivemark","livemark.png","src");
	checkLocalPng("fOhome","home.png","src");
  document.getElementById("cbiconsrc").src = iconsrc;
  var urlbox = document.getElementById("url");
  urlbox.value = url;
  var list = document.getElementById("style");
  list.selectedIndex = style;

	document.getElementById("deleteOld").selectedIndex = deleteOldStyle;
	document.getElementById("daysFeed").selectedIndex = 1*(daysToKeep > -1);
	document.getElementById("daysToKeep").value = daysToKeep;
	doDeleteOld();
	document.getElementById("updateUnread").selectedIndex = window.arguments[0].changedUnread;
  document.getElementById("cbPrivate").checked = window.arguments[0].prvate;
  document.getElementById("username").value = window.arguments[0].username;
  document.getElementById("password").value = window.arguments[0].password;
	document.getElementById("refreshCaption").label = NF_SB.getString('lastRefresh') + " " + displayDate(window.arguments[0].lastUpdate,2);
	document.getElementById("mLtime").selectedIndex = 1*(window.arguments[0].autoRefreshInterval != 0) + 1*(window.arguments[0].autoRefreshInterval == -1);
	document.getElementById("autoRefreshInterval").value = window.arguments[0].autoRefreshInterval;
	doAutoRefresh(true);
	document.getElementById("cbAutoCheck").checked = autoCheck;
	document.getElementById("cbFilterNew").checked = window.arguments[0].XfilterNew;
	var mimeType = window.arguments[0].XfilterMimeType;
	if (mimeType) document.getElementById("cbFilterMimeType").value = mimeType;
	document.getElementById("cbFilterMime").checked = (mimeType != null);
	onFilterMimeCommand();
	document.getElementById("cbFilterImages").checked = window.arguments[0].XfilterImages;
	var xfilterType = window.arguments[0].XfilterType;
	if (window.arguments[0].Xfilter)
	{
		var xfilter = window.arguments[0].Xfilter;
		document.getElementById("Xfilter").value = xfilter;
		if (xfilterType == -1) xfilterType = guessFilterType(xfilter);
		noFilter = false;
	}
	document.getElementById("radioFilterType").selectedIndex = xfilterType;
	document.getElementById("cbGlobalSort").checked = (window.arguments[0].sortStr == "g+");
	if (document.getElementById("cbGlobalSort").checked == false)
		setSorts(window.arguments[0].sortStr, "sort", "dir");
	sortChg();

  var namebox = document.getElementById("Name");
  var homebox = document.getElementById("Homepage");
  var grplist = document.getElementById("grplist");
	if (window.arguments[0].storage)
	{
		document.getElementById("feedOptionsDlg").buttons = "accept,cancel";
		document.getElementById("storageH1").hidden = true;
		document.getElementById("cbiconsrc").src = ICON_STORAGE;
		document.getElementById("storageH2").hidden = true;
		document.getElementById("storageH3").hidden = true;
		document.getElementById("storageH4").hidden = true;
		document.getElementById("authGroupbox").hidden = true;
		document.getElementById("filterTab").hidden = true;
		if (window.arguments[0].isNew)
    	namebox.value = NF_SB.getString('newStorageFeed');
		else
    	namebox.value = window.arguments[0].name;
		namebox.selected = true;
	}
  else if (window.arguments[0].isNew)
  {
    var msg = NF_SB.getString('feedfills');
    namebox.value = msg;
    namebox.disabled = true;
    homebox.value = msg;
    homebox.disabled = true;
    urlbox.selected = true;
    var feedOptDlg = document.getElementById("feedOptionsDlg");
    feedOptDlg.defaultButton = "disclosure";
  }
  else
  {
    namebox.value = window.arguments[0].name;
    homebox.value = window.arguments[0].homepage;
  }
//	if (url.substring(0,5) != "https")
//		document.getElementById("authGroupbox").hidden = true;

  var names = new Array();
  names = groupstr.split(",");
  var membs = new Array();
  membs = groupmemb.split(",");

  var index = 0;
  while (names.length >= 1)
  {
		var colorTxt = "";
		var name = names.shift();
		var memb = membs.shift();
		var tmp = document.createElement("listitem");
		tmp.setAttribute("type","checkbox");
		if (memb & 0x02)
		{
			tmp.setAttribute("class", "listitem-iconic");
			colorTxt = " color: blue";
			tmp.setAttribute("image", getPng("folderSearch.png"));
		}
		if (memb & 0x01) tmp.setAttribute("checked",true);
		if (memb & 0x04) tmp.setAttribute("hidden",true);
		if (index == 0) tmp.setAttribute("disabled",true);
    tmp.setAttribute("label",name.replace(/&#x2C;/g,","));
    if (name.length > 20) tmp.setAttribute("tooltiptext",name);
		tmp.setAttribute("id","nwsfx"+index);
		tmp.setAttribute("style", "height: auto; border: 0px;"+ colorTxt);
    grplist.appendChild(tmp);
		index++;
  }
	blurring = false;
}

function doCheckFeed()
{
  window.arguments[0].checkFeed = true;
  var feedOptDlg = document.getElementById("feedOptionsDlg");
  feedOptDlg.acceptDialog();
}

function doAccept()
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
	if (window.arguments[0].storage)
	{
  	var namebox = document.getElementById("Name");
		if (namebox.value == "")
		{
    	namebox.value = NF_SB.getString('newStorageFeed');
			namebox.select();
			namebox.focus();
			return false;
		}
	}
	else
	{
	  var urlbox = document.getElementById("url");
		if (urlbox.value == "")
		{
			urlbox.value = "     ";
			urlbox.select();
			urlbox.focus();
			return false;
		}
	  window.arguments[0].url = urlbox.value;
		var oldFeed = window.arguments[0].model.getFeedByURL(urlbox.value);
		var uid = window.arguments[0].uid;
		if (window.arguments[0].isNew && oldFeed != null && oldFeed.uid != uid)
		{
			if (oldFeed.exclude)
				window.arguments[0].model.remove(oldFeed);
			else
			{
				alert("'" + oldFeed.getDisplayName() + "' " + NF_SB.getString('alert.feedExists'));
				window.arguments[0].checkFeed = false;
		  	var urlbox = document.getElementById("url");
				urlbox.select();
				urlbox.focus();
				return false;
			}
		}
	}

  window.arguments[0].ok = true;
	window.arguments[0].name = document.getElementById("Name").value;
	window.arguments[0].iconsrc = document.getElementById("cbiconsrc").src;
	window.arguments[0].homepage = document.getElementById("Homepage").value;

  var elem = document.getElementById("style");
  window.arguments[0].style = parseInt(elem.value);
	window.arguments[0].deleteOldStyle = document.getElementById("deleteOld").selectedIndex;
	window.arguments[0].daysToKeep = (document.getElementById("daysFeed").selectedIndex == 0) ? -1 : parseInt(document.getElementById("daysToKeep").value);
	window.arguments[0].changedUnread = document.getElementById("updateUnread").selectedIndex;

	var aRI;
	if (document.getElementById("mLtime").selectedIndex == 0)
		aRI = 0;
	else if (document.getElementById("mLtime").selectedIndex == 2)
		aRI = -1;
	else
	{
		aRI = document.getElementById("autoRefreshInterval").value;
		if (isNaN(aRI) || aRI < MINFEEDTIME) aRI = MINFEEDTIME;
	}
	window.arguments[0].autoRefreshInterval = aRI;
  elem = document.getElementById("cbAutoCheck");
  window.arguments[0].autoCheck = elem.checked;

	window.arguments[0].prvate = document.getElementById("cbPrivate").checked;
	window.arguments[0].username = document.getElementById("username").value;
	window.arguments[0].password = document.getElementById("password").value;
	window.arguments[0].XfilterNew = document.getElementById("cbFilterNew").checked;
	var mimeType = document.getElementById("cbFilterMimeType").value;
	if (document.getElementById("cbFilterMime").checked && mimeType != "")
		window.arguments[0].XfilterMimeType = mimeType;
	else
		window.arguments[0].XfilterMimeType = null;
	window.arguments[0].XfilterImages = document.getElementById("cbFilterImages").checked;
	window.arguments[0].XfilterType = document.getElementById("radioFilterType").selectedIndex;
	if (filterChange)
		window.arguments[0].Xfilter = document.getElementById("Xfilter").value;

	var sortStr = "";
	if (document.getElementById("cbGlobalSort").checked) sortStr = "g+";
	else sortStr = getSortStr("sort", "dir");
	window.arguments[0].sortStr = sortStr;

  var membs = new Array();
  var elem = document.getElementById("nwsfx0");
  var index = 0;
  while (elem != null)
  {
    membs.push(1*(elem.checked));
    index++;
    elem = document.getElementById("nwsfx"+index);
  }
  window.arguments[0].groupmemb = membs.join();
  return true;
}

function toggleIcon()
{
	var iconsrc = document.getElementById("cbiconsrc").src;
	var uid = window.arguments[0].uid;
	var file = NFgetProfileDir();
	var leafName = uid + ".ico";
	file.append(leafName);
	if (iconsrc == ICON_OK)
	{
		var nsIFilePicker = Components.interfaces.nsIFilePicker;
		var pickfile = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
		const NF_SB = document.getElementById("newsfox-string-bundle");
		pickfile.init(window, NF_SB.getString('chooseicon'), nsIFilePicker.modeOpen);
		pickfile.displayDirectory = NFgetProfileDir();
		pickfile.appendFilters(nsIFilePicker.filterImages);
		pickfile.appendFilters(nsIFilePicker.filterAll);
		var filepicked = pickfile.show();
		if (filepicked == nsIFilePicker.returnOK)
		{
			if (file.exists()) file.remove(false);
			pickfile.file.copyTo(NFgetProfileDir(),leafName);
			iconsrc = getFileSpec(file) + "?" + (new Date()).getTime();
			document.getElementById("cbiconsrc").src = iconsrc;
		}
	}
	else  // has an icon, change to ICON_OK
	{
		document.getElementById("cbiconsrc").src = ICON_OK;
		if (file.exists()) file.remove(false);
	}
}

function feedChange()
{
return true;
	if (blurring) return;
	blurring = true;
  var url = document.getElementById("url").value;
	var authGroupbox = document.getElementById("authGroupbox");
	if (url.substring(0,5) != "https")
		authGroupbox.hidden = true;
	else
		authGroupbox.removeAttribute("hidden");
// FF3 bugs, #392417, #371508: sizeToContent() broken, seems to work
// with an assignment statement intervening between resizing event
	blurring = false;
	sizeToContent();
	return true;
}

function onFilterMimeCommand()
{
	var checked = document.getElementById("cbFilterMime").checked;
	document.getElementById("cbFilterMimeType").disabled = !checked;
	if (checked && document.getElementById("cbFilterMimeType").value == "")
		document.getElementById("cbFilterMimeType").value = AUTO_MIMETYPE;
}

function onFilterImagesCommand(checkBox)
{
	if (checkBox.checked && noFilter)
	{
		var textBox = document.getElementById('Xfilter');
		var newValue = textBox.getAttribute("value2");
		var len = newValue.length;
		newValue += DEFAULTREGEXPTEXT_INSERT;
		textBox.focus();
		textBox.value = newValue;
		textBox.setSelectionRange(0,len);
		noFilter = false;
	}
}
