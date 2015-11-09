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

var curDir = "";
var processedDir = "";
var canAccept = true;
const COL_ID = [ "flag", "title", "read", "date", "author", "source", "blog", "prob" ];
const DIR = [ "ascending", "descending" ];
//var keyList = new Array();

function init()
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
	checkLocalPng("pOhelp","help.png","src");
	checkLocalPng("pOfolderClosed","folderClosed.png","image");

	var args = window.arguments[0];
	var gOpt = args.gOpt;
	var keyList = args.keyList;

	var style = gOpt.globalStyle;
	var list = document.getElementById("style");
	list.selectedIndex = style-1;
	var dOstyle = gOpt.globalDeleteOldStyle;
	document.getElementById("deleteOld").selectedIndex = dOstyle - 1;
	document.getElementById("daysFeed").selectedIndex = 1*(gOpt.daysKeep > -1);
	document.getElementById("daysToKeep").value = gOpt.daysKeep;
	doDeleteOld();
	document.getElementById("pOchangedUnread").checked = gOpt.changedUnread;
	var chk = (gOpt.autoRefreshInterval != -1);
	document.getElementById("cbAutoRefresh").checked = chk;
	if (chk) document.getElementById("autoRefreshInterval").value = gOpt.autoRefreshInterval;
	doAutoRefresh(false);

	document.getElementById("cbCheckOnStartup").checked = gOpt.checkOnStartup;
	document.getElementById("cbNotifyUponNew").checked = gOpt.notifyUponNew;
	document.getElementById("cbConfirmDelete").checked = gOpt.confirmDelete;
	document.getElementById("cbConfirmDeleteArt").checked = gOpt.confirmDeleteArticle;
	var keyIndex = args.keyIndex;
	document.getElementById("shortcut").selectedIndex = keyIndex;
	if (keyIndex != 3) document.getElementById("custshortcut").hidden = true;
	curDir = args.nfDir;
	document.getElementById("nfdir").value = curDir;
	var dat = new Date(NEWSFOX_DATE);
	document.getElementById("date0").label = displayDate(dat,0);
	document.getElementById("date1").label = displayDate(dat,1);
	document.getElementById("date2").label = displayDate(dat,2);
	document.getElementById("dateStyle").selectedIndex = -1;
	document.getElementById("dateStyle").selectedIndex = gOpt.dateStyle;
	document.getElementById("pOnone").checked = !gOpt.dateNoneStrict;
	document.getElementById("pOnone").label = NF_SB.getString('NO_DATE');
	document.getElementById("pOinvalid").checked = !gOpt.dateInvalidStrict;
	document.getElementById("pOinvalid").label = NF_SB.getString('INVALID_DATE');
	document.getElementById("pOfuture").checked = !gOpt.dateFutureStrict;
	document.getElementById("pOfuture").label = NF_SB.getString('FUTURE_DATE');
	document.getElementById("pOtooltip").checked = gOpt.artTooltip;
	document.getElementById("pOcopyClip").checked = gOpt.copyToClip;
	document.getElementById("pOselectAddedFeed").checked = gOpt.selectAddedFeed;
	document.getElementById("pOthreads").value = gOpt.threads;
	document.getElementById("pOtimeout").value = gOpt.renewTimeout/1000;
	document.getElementById("pOhoriz").checked = gOpt.horiz;
	document.getElementById("pOfavicons").checked = gOpt.favicons;
	document.getElementById("pOspam").checked = gOpt.spam;
	var pOsync = document.getElementById("pOsync");
	pOsync.checked = gOpt.bookmarkSync;
	NFsetUserAgent();
	if (gFF == -1 || gUserAgent.indexOf("Flock") > -1) pOsync.hidden = true;
	if (gKMeleon) document.getElementById("pOallStatus").hidden = true;

	for (var i=0; i<Math.min(gOpt.keyword.length,5); i++)
		if (gOpt.keyword[i])
			document.getElementById("keyword"+i).value = gOpt.keyword[i];

	document.getElementById("radioStatusBarText").selectedIndex = gOpt.statusBarText;
	setSorts(gOpt.sortStr, "sort", "dir");
	setSorts(gOpt.sortStrG, "sortG", "dirG");
	updateKeys(keyIndex);
}

function doAccept()
{
	if (!canAccept)
	{
		canAccept = true;
		return false;
	}

	const NF_SB = document.getElementById("newsfox-string-bundle");
	var args = window.arguments[0];
	var gOpt = args.gOpt;
	var keyList = args.keyList;

	var elem = document.getElementById("style");
	gOpt.globalStyle = parseInt(elem.value);
	elem = document.getElementById("deleteOld");
	gOpt.globalDeleteOldStyle = parseInt(elem.value);
	gOpt.daysKeep = (document.getElementById("daysFeed").selectedIndex == 0) ? -1 : parseInt(document.getElementById("daysToKeep").value);
	gOpt.changedUnread = document.getElementById("pOchangedUnread").checked;
	var chk = document.getElementById("cbAutoRefresh").checked;
	var val = document.getElementById("autoRefreshInterval").value;
	if (val == "") chk = false;
	if (chk)
		gOpt.autoRefreshInterval = document.getElementById("autoRefreshInterval").value;
	else
		gOpt.autoRefreshInterval = -1;

	gOpt.checkOnStartup = document.getElementById("cbCheckOnStartup").checked;

	gOpt.notifyUponNew = document.getElementById("cbNotifyUponNew").checked;
	gOpt.confirmDelete = document.getElementById("cbConfirmDelete").checked;
	gOpt.confirmDeleteArticle = document.getElementById("cbConfirmDeleteArt").checked;
	args.newKeyIndex = document.getElementById("shortcut").selectedIndex;
	gOpt.dateStyle = document.getElementById("dateStyle").selectedIndex;
	gOpt.dateNoneStrict = !document.getElementById("pOnone").checked;
	gOpt.dateInvalidStrict = !document.getElementById("pOinvalid").checked;
	gOpt.dateFutureStrict = !document.getElementById("pOfuture").checked;
	gOpt.artTooltip = document.getElementById("pOtooltip").checked;
	gOpt.copyToClip = document.getElementById("pOcopyClip").checked;
	gOpt.selectAddedFeed = document.getElementById("pOselectAddedFeed").checked;
  var val = parseInt(document.getElementById("pOthreads").value);
  if (val < 1 || isNaN(val)) val = 1;
  gOpt.threads = val;
  val = parseInt(1000 * document.getElementById("pOtimeout").value);
  if ( val < 0 || isNaN(val)) val = 10000;
	gOpt.renewTimeout = val;
	gOpt.horiz = document.getElementById("pOhoriz").checked;
	gOpt.favicons = document.getElementById("pOfavicons").checked;
	gOpt.spam = document.getElementById("pOspam").checked;
	gOpt.bookmarkSync = document.getElementById("pOsync").checked;

	var lenToCheck = Math.min(gOpt.keyword.length,5)-1;
	for (var i=gOpt.keyword.length; i<5; i++)
	{
		var newValue = document.getElementById("keyword"+i).value.toLowerCase();
		if (stringTrim(newValue) == "") continue;
		else gOpt.keyword.push(newValue);
	}
	for (var i=lenToCheck; i>= 0; i--)
	{
		var newValue = document.getElementById("keyword"+i).value.toLowerCase();
		if (gOpt.keyword[i] != newValue)
		{
			if (stringTrim(newValue) == "") gOpt.keyword.splice(i,1);
			else gOpt.keyword[i] = newValue;
		}
	}

	gOpt.statusBarText = document.getElementById("radioStatusBarText").selectedIndex;
	gOpt.sortStr = getSortStr("sort", "dir");
	gOpt.sortStrG = getSortStr("sortG", "dirG");
	NFsetPref("sorts", "str", gOpt.sortStr);
	NFsetPref("sortsG", "str", gOpt.sortStrG);

	var nfDir = document.getElementById("nfdir").value;
	args.nfDir = nfDir;
	var mvContents = document.getElementById("mvNfDir").selectedIndex;
	args.mvContents = mvContents;
	if (mvContents < 2 && curDir != nfDir)
	{
		var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
		var nfDirFile = nsIPH.getFileFromURLSpec(nfDir);
  	var dirNotEmpty = NF_SB.getString('dirNotEmpty');
		if (nfDirFile.directoryEntries.hasMoreElements())  // not empty
			if (!noOKConfirm(dirNotEmpty))
			{
				document.getElementById("nfdir").value = curDir;
				checkDir();
				return false;
			}
	}
	args.ok = true;
	return true;
}

function chooseDir()
{
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	var url = document.getElementById("nfdir").value;
	url = getProfURL(url);
	var file = nsIPH.getFileFromURLSpec(url);
	var picker = NFdirPicked(file);
	if (picker)
	{
		var nfdir = nsIPH.getURLSpecFromFile(picker.file);
		document.getElementById("nfdir").value = nfdir;
		processDir(nfdir);
	}
}

function checkDir()
{
	canAccept = true;
	var nfdir = document.getElementById("nfdir").value;
	if (nfdir != "" && nfdir.charAt(nfdir.length-1) != "/") nfdir += "/";
	if (nfdir == "./") nfdir = "";
	document.getElementById("nfdir").value = nfdir;
	nfdir = getProfURL(nfdir);

	if (nfdir == processedDir) return true;
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	const NF_SB = document.getElementById("newsfox-string-bundle");
	try 
	{ var nffile = nsIPH.getFileFromURLSpec(nfdir); }
	catch(e)
	{
		alert(NF_SB.getString('badDir'));
		document.getElementById("nfdir").value = curDir;
		canAccept = false;
		return false;
	}
	if (!nffile.exists())
	{
		if (yesNoConfirm(NF_SB.getString('createDir')))
      nffile.create(nffile.DIRECTORY_TYPE, 0750);
// doesn't work in FF2
//			nffile.create(nffile.DIRECTORY_TYPE, 0o0750);
		else
			nfdir = curDir;
	}
	processDir(nfdir);
	return false;
}

function processDir(nfdir)
{
	canAccept = false;
	processedDir = nfdir;
	var mvContents = document.getElementById("mvNfDirHBox");
	var tmpCurDir = getProfURL(curDir);
	if (tmpCurDir == nfdir)
		mvContents.hidden = true;
	else
	{
		document.getElementById("mvNFDirCopy").hidden = false;
		document.getElementById("mvNFDirMove").hidden = false;
		document.getElementById("mvChild").hidden = true;
		document.getElementById("mvParent").hidden = true;
		mvContents.hidden = false;
		if (nfdir.indexOf(tmpCurDir) > -1 || tmpCurDir.indexOf(nfdir) > -1)
		{
			document.getElementById("mvNFDirCopy").hidden = true;
			document.getElementById("mvNFDirMove").hidden = true;
			document.getElementById("mvNfDir").selectedIndex = 2;
			if (nfdir.indexOf(tmpCurDir) > -1)
				document.getElementById("mvChild").hidden = false;
			else
				document.getElementById("mvParent").hidden = false;
		}
	}
	window.sizeToContent();
}

function shortcutChanged()
{
	var keyType = document.getElementById("shortcut").selectedIndex;
	updateKeys(keyType);
}

function updateKeys(keyIndex)
{
	var args = window.arguments[0];
	var keyList = args.keyList;
	var treeKids = document.getElementById("treeKids");
	while (treeKids.firstChild != null)
		treeKids.removeChild(treeKids.firstChild);
	for (var i=0; i<keyList.length; i++)
	{
		var tmp = document.createElement("treeitem");
		var tmp1 = document.createElement("treerow");
		var cell1 = document.createElement("treecell");
		var cell2 = document.createElement("treecell");
		var mod = keyList[i].getAttribute("mod"+keyIndex);
		if (!mod) mod = "";
		var key = keyList[i].getAttribute("key"+keyIndex);
		if (!key) key = "";
		cell1.setAttribute("label",mod + " " + key);
		cell1.setAttribute("style","text-align: center;");
		tmp1.appendChild(cell1);
		cell2.setAttribute("label",keyList[i].getAttribute("label"));
		cell2.setAttribute("editable", "false");
		tmp1.appendChild(cell2);
		tmp.appendChild(tmp1);
		treeKids.appendChild(tmp); 
	}
}

function treeKeypress(event)
{
	if (event.charCode == 0) return;
	var tree = document.getElementById("keyTree");
	var row = tree.currentIndex;
	var mod = "";
	if (event.altKey) mod += "alt";
	if (event.ctrlKey) mod += " ctrl";
	if (event.shiftKey) mod += " shift";
	if (event.metaKey) mod += " meta";
	mod = stringTrim(mod);
	var key = String.fromCharCode(event.charCode);
	try { tree.stopEditing(false) } catch(e) {}
	var newRow = existingRow(mod,key);
	if (newRow != null)
	{
		tree.view.selection.select(newRow);
		tree.treeBoxObject.ensureRowIsVisible(newRow);
	}
	else
	{
		tree.view.setCellText(row,tree.treeBoxObject.columns["keystroke"], mod+" " + key);
		var args = window.arguments[0];
		var keyList = args.keyList;
		var keyType = document.getElementById("shortcut").selectedIndex;
		if (keyType != 3)
		{
			for (var i=0; i<keyList.length; i++)
			{
				keyList[i].setAttribute("mod3", keyList[i].getAttribute("mod"+keyType));
				keyList[i].setAttribute("key3", keyList[i].getAttribute("key"+keyType));
			}
		}
		keyList[row].setAttribute("mod3", mod);
		keyList[row].setAttribute("key3", key);
		document.getElementById("shortcut").selectedIndex = 3;
	}
}

function existingRow(mod, key)
{
	var modCount = modCnt(mod);
	var treeKids = document.getElementById("treeKids");
	var existRow = null;
	var matchKey, matchMod;
	for (var i=0; i<treeKids.childNodes.length; i++)
	{
		var treeRow = treeKids.childNodes[i].firstChild;
		var treeCellText = treeRow.firstChild.getAttribute("label");
		var lastSpace = treeCellText.lastIndexOf(" ");
		var matchKey = treeCellText.substring(lastSpace+1);
		var matchMod = treeCellText.substr(0,lastSpace);
		if (key.toLowerCase() != matchKey.toLowerCase()) continue;
		if (modCnt(matchMod) != modCount) continue;
		existRow = i;
		break;
	}
	return existRow;
}

function modCnt(mod)
{
	var retval = 0;
	retval += (mod.indexOf("ctrl") > -1);
	retval += 2*(mod.indexOf("alt") > -1);
	retval += 4*(mod.indexOf("shift") > -1);
	retval += 8*(mod.indexOf("meta") > -1);
	return retval;
}

