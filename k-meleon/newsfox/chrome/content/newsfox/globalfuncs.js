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

const VERSION = "1.0.9.1";
const NEWSFOX = "NewsFox";
const NEWSFOX_RSS = "http://newsfox.mozdev.org/rss/rss.xml?startup";
const NEWSFOX_DATE = "April 15, 2003 8:01 PM";
const NF_URI = "chrome://newsfox/content/newsfox.xul?bkmk";
const NFINFO = "http://newsfox.mozdev.org/";
const NFSOUND = "NFsound.wav";
const LONG_DATE_STYLE = 2;
const ERROR_OK = "0";
const ERROR_INVALID_FEED_URL = "1";
const ERROR_UNKNOWN_FEED_FORMAT = "2";
const ERROR_SERVER_ERROR = "3";
const ERROR_NOT_FOUND = "4";
const ERROR_REFRESH = "5";
const FEED_VALIDATOR = "http://validator.w3.org/feed/check.cgi?url=";
const NO_LINK = "";
const ICON_OK = getPng("feed.png");
const ICON_STORAGE = getPng("storage.png");
const MASTER = "master";
const MASTER_GROUP = "master_group";
const MASTER_INDEX = "master_index";
const MASTER_FILTER = "master_filter";
const AUTO_MIMETYPE = "+";
const TEST_MIMETYPE = ">";

const HRS12 = 1000*60*60*12;
const DATEBASE = -HRS12;
const TOP_NO_DATE = new Date(DATEBASE + HRS12);
const NO_DATE = new Date(DATEBASE);
const TOP_INVALID_DATE = new Date(DATEBASE - HRS12);
const INVALID_DATE = new Date(DATEBASE - 2*HRS12);
const TOP_FUTURE_DATE = new Date(DATEBASE - 3*HRS12);

const MINAUTOTIME = 5;
const MINFEEDTIME = 10;

const COL_NAME = { n: "none", f: "flag", t: "title", r: "read", d: "date", a: "author", s: "source", b: "blog", p: "prob", o: "orderThread" };
const COL_LETTER = [ "n", "f", "t", "r", "d", "a", "s", "b", "p", "o" ];
const DIR_LETTER = [ "+", "-" ];

var gUserAgent;
var gKMeleon = false;
var gEMusic = false;
var gSeaMonkey = false;
var gFlock = false;
var gFF = -1;
var gNewsfoxDirURL = null;
var gMsgDone = false;
var gSdr;
var gTag = "";
var gAllFeedsLoaded = false;
var gLoadFlags = 0;
var gNewItemsCount = 0;

// spam filter
var gGoodArray = new Array();
var gTotalArray = new Array();
var gWordArray = new Array();
const S_TOTAL_START = 1000;
const S_GOOD_START = 500;
const S_MULT = 0.99;
const S_MINCOUNT = 2500;
const S_MINWORDLENGTH = 3;
const S_MAXCOMPARE = 30;
const S_UNREADPCT = 0.75;
const S_MAXWORDS = 2500;
const S_MINWORDTOTAL = 1000;
const S_MINWORDS = 1000;
var gArtsToUpdateSpam = new Array();
var gArtsToAddSpam = new Array();
var gArtsToScoreSpam = new Array();
var gFeedsToSaveSpam = new Array();
const S_BIGNUM = 1000111000111;
const S_BIGNUMDIV = 1000000;
const S_MAX_ARTS = 200;
const S_NEW_DAYS = 1;

const DEFAULTREGEXPTEXT = "<p.*?<\/p>";
const DEFAULTREGEXPTEXT_INSERT = "<p.*?<\\\/p>";
const DEFAULTREGEXP = new RegExp(DEFAULTREGEXPTEXT, "g");

var stringTrim = function(v)
{
	return v.replace(/^\s+|\s+$/g, '');
}

/**
 * Get the newsfox directory 
 */

function getProfURL(profURL)
{
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	var ioSvc = Components.classes['@mozilla.org/network/io-service;1'].getService(Components.interfaces.nsIIOService);
  var dFile = Components.classes["@mozilla.org/file/directory_service;1"].
    getService(Components.interfaces.nsIProperties).
    get("ProfD", Components.interfaces.nsIFile);
  dFile.append("newsfox");
  if (!dFile.exists()) dFile.create(dFile.DIRECTORY_TYPE, 0750);
// doesn't work in FF2
//	if (!dFile.exists()) dFile.create(dFile.DIRECTORY_TYPE, 0o0750);
	var dURI = ioSvc.newURI(nsIPH.getURLSpecFromFile(dFile),null,null);
	return dURI.resolve(profURL);
}

function NFgetProfileDir(force)
{
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var profURL = getProfURL(NFgetPref("global.directory", "str", ""));

	if (gNewsfoxDirURL != null)  // use it, but warn if preference changed
	{
		if (gNewsfoxDirURL != profURL && !gMsgDone)
		{
			window.alert(NF_SB.getString('inuse'));
			gMsgDone = true;
		}
		return nsIPH.getFileFromURLSpec(gNewsfoxDirURL);
	}
	if (profURL != "")  // use it if exists, otherwise choose new
	{
		var file = nsIPH.getFileFromURLSpec(profURL);
		if (file.exists())
		{
			gNewsfoxDirURL = profURL;
			return nsIPH.getFileFromURLSpec(gNewsfoxDirURL);
		}

		if (force == false) return null;
		var msg = NF_SB.getString('confirm.newNewsfoxDir');
		if (yesNoConfirm(msg))  // pick new directory, else use default
		{
			var picker = NFdirPicked(file);
			if (picker)
			{
				gNewsfoxDirURL = nsIPH.getURLSpecFromFile(picker.file);
				NFsetPref("global.directory", "str", gNewsfoxDirURL);
				return picker.file;
			}
		}
	}

	// default to standard location
  var file = Components.classes["@mozilla.org/file/directory_service;1"].
    getService(Components.interfaces.nsIProperties).
    get("ProfD", Components.interfaces.nsIFile);
  file.append("newsfox");
  if (!file.exists()) file.create(file.DIRECTORY_TYPE, 0750);
// doesn't work in FF2
//  if (!file.exists()) file.create(file.DIRECTORY_TYPE, 0o0750);
	gNewsfoxDirURL = nsIPH.getURLSpecFromFile(file);
	NFsetPref("global.directory", "str", ".");
	return file;
}

function NFdirPicked(startFile)
{
	var picker = Components.classes["@mozilla.org/filepicker;1"].
		createInstance(Components.interfaces.nsIFilePicker);
	var file = startFile;
	try
	{
		while (!file.exists() || file.isFile()) file = file.parent;
		picker.displayDirectory = file;
	}
	catch(e){}
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var wintitle = NF_SB.getString('chooseNewsfoxFolder');
	picker.init(window, wintitle, picker.modeGetFolder);
	if(picker.show() == picker.returnOK) return picker;
	else return null;
}

function NFgetPref(name, type, dfault, notNewsfox)
{
	var base = "newsfox.";
	if (notNewsfox) base = "";
	var prefs = Components.classes["@mozilla.org/preferences-service;1"]
		.getService(Components.interfaces.nsIPrefService)
		.getBranch(base);

	if (prefs.getPrefType(name) == prefs.PREF_INVALID) return dfault;
	try
	{
		switch (type)
		{
			case "str":
				return prefs.getCharPref(name);
			case "int":
				return prefs.getIntPref(name);
			case "bool":
				return prefs.getBoolPref(name);
		}
		return null;
	}
	catch(e) { return dfault; }
}

function NFsetPref(name, type, value, notNewsfox)
{
	var base = "newsfox.";
	if (notNewsfox) base = "";
	var prefs = Components.classes["@mozilla.org/preferences-service;1"]
		.getService(Components.interfaces.nsIPrefService)
		.getBranch(base);

	switch (type)
	{
		case "str":
			prefs.setCharPref(name,value);
			break;
		case "int":
			prefs.setIntPref(name,value);
			break;
		case "bool":
			prefs.setBoolPref(name,value);
			break;
	}
}

function setTitle(doNew)
{
	var updateText = null;
	var unread = gFdGroup[0].getUnread();
	newTitle(unread);
	switch (gOptions.statusBarText)
	{ 
		case 0:
			if (doNew) updateText = gNewItemsCount;
			break;
		case 1:
			updateText = unread;
			if (updateText == 0) updateText = "";
	}
	if (updateText != null && !gKMeleon) updateStatusText(updateText);
}

function newTitle(undone)
{
	var nF = NEWSFOX;
	if (undone > 0) nF += " (" + undone + ")";
	// prevent flicker, only redo if needed
	if (document.title != nF) document.title = nF;
}

function updateStatusText(text)
{
	var mainWindow = 
			window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
			.getInterface(Components.interfaces.nsIWebNavigation)
			.QueryInterface(Components.interfaces.nsIDocShellTreeItem)
			.rootTreeItem
			.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
			.getInterface(Components.interfaces.nsIDOMWindow);
	var doc = mainWindow.document;
	if (null != doc.getElementById("newsfox-status-label"))
	{
		doc.getElementById("newsfox-status-label").removeAttribute("hidden");
		doc.getElementById("newsfox-status-label").setAttribute("value", text);
	}
}

function getPng(Pngfile)
{
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	var profURL = getProfURL(NFgetPref("global.directory", "str", ""));
	if (profURL != "")
	{
		var file = nsIPH.getFileFromURLSpec(profURL);
		if (file.exists())
		{
			file.append("images");
			file.append(Pngfile);
			if (file.exists()) return getFileSpec(file);
		}
	}
	return "chrome://newsfox/skin/images/"+Pngfile;
}

function checkLocalPng(id,png,prop)
{
	if (!prop) prop = "image";
	var localPng = getPng(png);
	if (localPng.charAt(0) == "f")  // local "f"ile
		document.getElementById(id).setAttribute(prop, localPng);
}

function displayDate(date, style)
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
	if (date > TOP_NO_DATE)
	{
		if (style == 2)
			return date.toLocaleString();
		else if (style == 0)
		{
			var hour = date.getHours();
			if (hour < 10) hour = "0" + hour;
			var min = date.getMinutes();
			if (min < 10) min = "0" + min;
			var time = hour + ":" + min;
			var dateM = date.getMonth()+1;
			if (dateM < 10) dateM = "0" + dateM;
			var dateD = date.getDate();
			if (dateD < 10) dateD = "0" + dateD;
			var dat = date.getFullYear() + "-" + dateM + "-" + dateD + " ";
			var now = new Date();
			var nowM = now.getMonth()+1;
			if (nowM < 10) nowM = "0" + nowM;
			var nowD = now.getDate();
			if (nowD < 10) nowD = "0" + nowD;
			var nowdat = now.getFullYear() + "-" + nowM + "-" + nowD + " ";
			if (dat == nowdat) dat = "";
			return (dat + time);
		}
		else  // style == 1
		{
			var sdf =  Components.classes["@mozilla.org/intl/scriptabledateformat;1"]
				.createInstance(Components.interfaces.nsIScriptableDateFormat);
			return sdf.FormatDateTime("", sdf.dateFormatShort,
				sdf.timeFormatNoSeconds, date.getFullYear(), date.getMonth()+1,
				date.getDate(), date.getHours(), date.getMinutes(), date.getSeconds());
		}
	}
	else if (date <= TOP_FUTURE_DATE) return NF_SB.getString('FUTURE_DATE');
	else if (date <= TOP_INVALID_DATE) return NF_SB.getString('INVALID_DATE');
	else return NF_SB.getString('NO_DATE');
}

function NFsetUserAgent()
{
	gUserAgent = NFgetPref("general.useragent.extra.firefox", "str", null, true);
	if (gUserAgent)  // Flock too  'Firefox/x.x.x Flock/y.y.y'
	{
		gFF = 3;
		if (gUserAgent.indexOf("fox/2") > -1) gFF = 2;
		else if (gUserAgent.indexOf("fox/1") > -1) gFF = 1;
		gFlock = (NFgetPref("general.useragent.extra.flock", "str", false, true) || gUserAgent.indexOf("Flock") > -1);
		return gUserAgent;
	}
	gUserAgent = NFgetPref("general.useragent.extra.seamonkey", "str", null, true);
	if (gUserAgent)
		{ gSeaMonkey = true; return gUserAgent; }
	gUserAgent = window.navigator.vendor + "/" + window.navigator.vendorSub;
	if (gUserAgent.indexOf("K-Meleon") > -1)
		{ gKMeleon = true; return gUserAgent; }
	if (gUserAgent.indexOf("eMusic") > -1)
		{ gEMusic = true; return gUserAgent; }
	gUserAgent = window.navigator.userAgent;
	if (gUserAgent.indexOf('Firefox') > -1) gFF = 4;
	if (gUserAgent.indexOf('GranParadiso') > -1 || gUserAgent.indexOf('Minefield') > -1) gFF = 4;
	return gUserAgent;
}

function getFileSpec(file)
{
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	return nsIPH.getURLSpecFromFile(file);
}

function yesNoConfirm(message)
{
	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
	var flags = prompts.STD_YES_NO_BUTTONS + prompts.BUTTON_POS_0_DEFAULT;
	return pConfirm(message, flags);
}

function noYesConfirm(message)
{
	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
	var flags = prompts.STD_YES_NO_BUTTONS + prompts.BUTTON_POS_1_DEFAULT;
	return pConfirm(message, flags);
}

function noOKConfirm(message)
{
	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
	var flags = prompts.STD_OK_CANCEL_BUTTONS + prompts.BUTTON_POS_1_DEFAULT;
	return pConfirm(message, flags);
}

function pConfirm(message, flags)
{
	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
	var check = {value: false};
	var button = prompts.confirmEx(null, "", message, flags, "", "", "", null, check);
	if (button == 0) return true;
	else return false;
}

function sortChgG()
{
	doSortChg("sortG", "dirG");
}

function sortChg()
{
	var ckbox = document.getElementById("cbGlobalSort");
	if (ckbox && ckbox.checked == true)
	{
		for (var i=1; i<=4; i++)
		{
			document.getElementById("sort"+i).disabled = true;
			document.getElementById("dir"+i).disabled = true;
		}
	}
	else
		doSortChg("sort", "dir");
}

function doSortChg(sortname, dirname)
{
	for (var i=1; i<=4; i++)
	{
		document.getElementById(sortname+""+i).removeAttribute("disabled");
		if (document.getElementById(sortname+""+i).selectedIndex == 0)
		{
			document.getElementById(dirname+""+i).disabled = true;
			for (var j=i+1; j<=4; j++)
				document.getElementById(sortname+""+j).selectedIndex = 0;
		}
		else
			document.getElementById(dirname+""+i).removeAttribute("disabled");
	}
}

function setSorts(sortStr, sortname, dirname)
{
	for (var i=1; i<=sortStr.length/2; i++)
	{
		document.getElementById(sortname+""+i).selectedIndex = COL_LETTER.indexOf(sortStr[2*(i-1)]);
		document.getElementById(dirname+""+i).selectedIndex = (sortStr[2*i-1] == "+") ? 0 : 1;
	}
	for (var i=sortStr.length/2+1; i<=4; i++)
	{
		document.getElementById(sortname+""+i).selectedIndex=0;
		document.getElementById(dirname+""+i).disabled = true;
	}
}

function getSortStr(sortname, dirname)
{
	var sortStr = "";
	for (var i=1; i<=4; i++)
	{
		var colIdIndex = document.getElementById(sortname+""+i).selectedIndex;
		if (colIdIndex == 0) break;
		var dirIndex = document.getElementById(dirname+""+i).selectedIndex; 
		sortStr += COL_LETTER[colIdIndex] + DIR_LETTER[dirIndex];
	}
	if (i == 1) sortStr = "n+";
	return sortStr;
}

function openNewTab(url)
{
	if (url == NO_LINK) return;
	if(gKMeleon || gEMusic)
	{
		window.open(url);
		window.focus();
	}
	else
	{
		const kWindowMediatorContractID = "@mozilla.org/appshell/window-mediator;1";
		const kWindowMediatorIID = Components.interfaces.nsIWindowMediator;
		const kWindowMediator = Components.classes[kWindowMediatorContractID].getService(kWindowMediatorIID);
		var browserWindow = kWindowMediator.getMostRecentWindow("navigator:browser");
		var browser = browserWindow.getBrowser();
		var tab = browser.addTab(url);
	}
}

function guessFilterType(xfilter)
{
	if (xfilter.indexOf("linkDOM") > -1 || xfilter.indexOf("linkHTML") > -1)
		return 1;   // JavaScript
	if (xfilter.indexOf("[@") > -1 || xfilter.indexOf("//") > -1)
		return 2;   // XPath
	return 0;   // RegExp
}

function doDeleteOld()
{
	var dOsI = document.getElementById("deleteOld").value;
	var dFsI = document.getElementById("daysFeed").value;
	var disable = false;
	if (dOsI == 0 || dOsI == 3) disable = true;
	document.getElementById("daysFeed").disabled = disable;
	document.getElementById("daysToKeep").disabled = disable;
	if (dFsI == 0)
	{
		document.getElementById("daysToKeep").hidden = true;
		document.getElementById("daysToKeep").value = "2";
		document.getElementById("daysToKeepLabel").hidden = true;
	}
	else
	{
		document.getElementById("daysToKeep").removeAttribute("hidden");
		document.getElementById("daysToKeepLabel").removeAttribute("hidden");
	}
}

function doAutoRefresh(fromFeed)
{
	var aRI = document.getElementById("autoRefreshInterval");
	var needNum;
	if (fromFeed)
		needNum = (document.getElementById("mLtime").selectedIndex == 1);
	else
		needNum = document.getElementById("cbAutoRefresh").checked;
	if (needNum)
		aRI.disabled = false;
	else
	{
		aRI.value = "";
		aRI.disabled = true;
	}
}

function autoInterval()
{
	var aRI = document.getElementById("autoRefreshInterval");
	if (isNaN(aRI.value) || aRI.value < MINFEEDTIME)
	{
		aRI.value = MINFEEDTIME;
		const NF_SB = document.getElementById("newsfox-string-bundle");
		window.alert(NF_SB.getString('autoCheckIntervalWarning'));
	}
}

function propsAdd(value, props, returnProps)
{
	var aserv = Components.classes["@mozilla.org/atom-service;1"].
			getService(Components.interfaces.nsIAtomService);
	if (props)
		props.AppendElement(aserv.getAtom(value));
	return (returnProps + " " + value);
}
