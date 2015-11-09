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

////////////////////////////////////////////////////////////////
// Global
////////////////////////////////////////////////////////////////

var gOptions = new AppOptions();

var noObserve = false;
const MAX_SCRIPT = 20;

var nFprefObserver = 
{
	observe: function(subject, topic, data)
	{
		if (noObserve) return;
		gOptions.load();
		if (gOptions.addUrl != "") addFeedUrl();
	} 
}

function sortPair()
{
	this.colId = null;
	this.dir = null;
}

////////////////////////////////////////////////////////////////
// Model
////////////////////////////////////////////////////////////////

function AppOptions()
{
// properties assigned in this.load()

	this.startup = function()
	{
		var maxScript = NFgetPref("dom.max_script_run_time","int",10,true);
		if (maxScript < MAX_SCRIPT)
			NFsetPref("dom.max_script_run_time","int",MAX_SCRIPT,true);
		this.renamePrefs();
		this.moveAutoCheckToFeed();
		this.moveGlobalSorts();
		// need to set the directory preference, so the observer doesn't catch 
		// the setting later and process the addurl before models loaded
		var file = NFgetProfileDir();
		this.load();
		this.save(true);
		this.addObserver();
		this.loadImages();
	}

	this.addObserver = function()
	{
		var nsIPrefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
		if (typeof nsIPrefBranch.addObserver == "function")
			nsIPrefBranch.addObserver("newsfox.", nFprefObserver, false);
		else
		{
			var nsIPrefBranch2 = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch2);
			nsIPrefBranch2.addObserver("newsfox.", nFprefObserver, false);
		}
	}

	this.rmObserver = function()
	{
		var nsIPrefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
		if (typeof nsIPrefBranch.removeObserver == "function")
			nsIPrefBranch.removeObserver("newsfox.", nFprefObserver, false);
		else
		{
			var nsIPrefBranch2 = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch2);
			nsIPrefBranch2.removeObserver("newsfox.", nFprefObserver, false);
		}
	}

  this.load = function()
  {
  // 1 Text
  // 2 Web page
    this.globalStyle = NFgetPref("global.style", "int", 1);
  // 1 Delete old articles, keep unread
  // 2 Delete old articles, delete unread
	// 3 Don't delete old articles
    this.globalDeleteOldStyle = NFgetPref("global.deleteOldStyle", "int", 1);
    this.checkOnStartup = NFgetPref("global.checkOnStartup", "bool", false);
		this.notifyUponNew = NFgetPref("global.notifyUponNew", "bool", true);
		this.notifyUponNewSound = NFgetPref("global.notifyUponNewSound", "bool", false);
		this.confirmDelete = NFgetPref("global.confirmDelete", "bool", true);
		this.confirmDeleteArticle = NFgetPref("global.confirmDeleteArticle", "bool", this.confirmDelete);
	// 0 ISO date, no seconds
	// 1 system short date
	// 2 system long date
		this.dateStyle = NFgetPref("global.dateStyle", "int", 0);
    this.artTreeCols = NFgetPref("global.articleTreeColumns", "str", "-");
    
    
		this.statusBar = NFgetPref("internal.statusBar", "bool", true);
		this.statusBarText = NFgetPref("internal.statusBarText", "int", 0);
		this.addUrl = NFgetPref("internal.addUrl", "str", NEWSFOX_RSS);
		this.favicons = NFgetPref("advanced.favicons", "bool", NFgetPref("browser.chrome.favicons","bool",true,true));
		this.guessHomepage = NFgetPref("advanced.guessHomepage", "bool", true);
		this.horiz = NFgetPref("advanced.horizontalPanes", "bool", false);
		this.copyClip = NFgetPref("advanced.copyToClipboard", "int", -1);
		if (this.copyClip == -1)
		{
			this.copyToClip = NFgetPref("advanced.copyArtBodyToClipboard","bool", false);
			var prefs = Components.classes["@mozilla.org/preferences-service;1"]
				.getService(Components.interfaces.nsIPrefService)
				.getBranch("newsfox.");
			if (this.copyToClip)
				this.copyClip = 1;
			else
				this.copyClip = 0;
// need to set as load() gets called without save() when browser is opened
			NFsetPref("advanced.copyToClipboard", "int", this.copyClip);
		}
		this.selectAddedFeed = NFgetPref("advanced.selectAddedFeed", "bool", true);
		this.changedUnread = NFgetPref("advanced.markChangedArtAsUnread", "bool", false);
		this.artTooltip = NFgetPref("advanced.articleTooltip", "bool", false);
		var intTime = NFgetPref("advanced.autoRefreshTime", "int", MINAUTOTIME);
		if (!(intTime >= MINAUTOTIME)) intTime = MINAUTOTIME;
		this.autoRefreshTime = 1000 * 60 * intTime;
		this.bookmarkSync = NFgetPref("advanced.bookmarkSync", "bool", false);
	// 0 alphabetical
	// 1 cats with unread first alphabetically, then cats with no unread
	// 2 descending by #unread
		this.catSort = NFgetPref("advanced.catSort", "int", 1);
		this.spam = NFgetPref("advanced.interestFilter", "bool", true);
		this.daysKeep = NFgetPref("advanced.daysToKeepGlobal", "int", -1);
		this.flagTagged = NFgetPref("advanced.flagTagged", "bool", false);
		this.aggregate = NFgetPref("advanced.aggregate", "bool", true);
		this.backupFeedFiles = NFgetPref("advanced.backupFeedFiles", "bool", false);
		this.indent = NFgetPref("advanced.indent", "bool", true);
		this.autoRefreshInterval = NFgetPref("advanced.autoRefreshInterval", "int", -1);
		this.dontDeleteFlagged = NFgetPref("advanced.dontDeleteFlagged", "bool", false);
		this.oneGroupOpen = NFgetPref("advanced.oneGroupOpen", "bool", false);
		this.linkify = NFgetPref("advanced.linkify", "bool", false);
		this.linkifyXHTML = NFgetPref("advanced.linkifyXHTML", "bool", false);
		this.wmode = NFgetPref("advanced.wmodeOpaque", "bool", true);
		this.fixyoutube1 = NFgetPref("advanced.fixyoutube1", "bool", true);
		this.fixMailto = NFgetPref("advanced.fixMailto", "bool", true);
		this.defaultXfilterIsWeb = NFgetPref("advanced.defaultXfilterIsWeb", "bool", false);
		this.linuxNoDragDrop = NFgetPref("advanced.linuxNoDragAndDrop", "bool", true);
    this.threads = NFgetPref("advanced.checkFeedThreads", "int", 2);
    this.renewTimeout = 1000 * NFgetPref("advanced.renewTimeoutInSeconds", "int", 10);

		this.dateNoneStrict = NFgetPref("date.noneStrict", "bool", false);
		this.dateInvalidStrict = NFgetPref("date.invalidStrict", "bool", false);
		this.dateFutureStrict = NFgetPref("date.futureStrict", "bool", false);

		this.srchUnread = NFgetPref("override.showSearchUnread", "bool", true);

		this.dragCopy = NFgetPref("z.dragFeedOutOfGroupIsCopy", "bool", true);
		this.markRead = NFgetPref("z.selectMarksArticleAsRead", "bool", true);
		this.openInViewPane = NFgetPref("z.openInViewPane", "bool", false);
		this.slowDelete = NFgetPref("z.slowDelete", "bool", false);

		this.Xremove = NFgetPref("x.removeXbody", "int", 1);

		this.keyword = new Array();
		var i = 1;
		while (NFgetPref("keyword"+i, "str", null))
// https://developer.mozilla.org/En/Firefox_addons_developer_guide/Using_XPCOM%E2%80%94Implementing_advanced_processes
			this.keyword.push(decodeURIComponent(escape(NFgetPref("keyword"+i++, "str", null))));

		this.sortStr = NFgetPref("sorts", "str", "d-");
		this.sortStrG = NFgetPref("sortsG", "str", "d-");

		if (NFgetPref("z.noMainToolbar", "bool", false))
		{
			if (doToolbar('mainToolbar'))
				document.getElementById("mainMenuitem").setAttribute("checked", "false");
		}
		if (NFgetPref("z.noFeedToolbar", "bool", false))
		{
			if (doToolbar('feedToolbar'))
				document.getElementById("feedMenuitem").setAttribute("checked", "false");
		}
  }

  this.save = function(savenonUI)
  {
		noObserve = true;  // can't have observer while saving many options
    NFsetPref("global.style", "int", this.globalStyle);
    NFsetPref("global.deleteOldStyle", "int", this.globalDeleteOldStyle);
		NFsetPref("advanced.daysToKeepGlobal", "int", this.daysKeep);
    NFsetPref("global.checkOnStartup", "bool", this.checkOnStartup);
		NFsetPref("global.notifyUponNew", "bool", this.notifyUponNew);
		NFsetPref("global.confirmDelete", "bool", this.confirmDelete);
		NFsetPref("global.confirmDeleteArticle", "bool", this.confirmDeleteArticle);
		NFsetPref("global.dateStyle", "int", this.dateStyle);
    NFsetPref("global.articleTreeColumns", "str", this.artTreeCols);
		NFsetPref("internal.statusBar", "bool", this.statusBar);
		NFsetPref("internal.statusBarText", "int", this.statusBarText);
		NFsetPref("internal.addUrl", "str", this.addUrl);
		NFsetPref("date.noneStrict", "bool", this.dateNoneStrict);
		NFsetPref("date.invalidStrict", "bool", this.dateInvalidStrict);
		NFsetPref("date.futureStrict", "bool", this.dateFutureStrict);
		NFsetPref("advanced.copyToClipboard", "int", this.copyClip);
		NFsetPref("advanced.selectAddedFeed", "bool", this.selectAddedFeed);
    NFsetPref("advanced.checkFeedThreads", "int", this.threads);
    NFsetPref("advanced.renewTimeoutInSeconds", "int", this.renewTimeout/1000);
		NFsetPref("advanced.markChangedArtAsUnread", "bool", this.changedUnread);
		NFsetPref("advanced.articleTooltip", "bool", this.artTooltip);
		if (this.artTooltip)
			setArtTooltip();
		else
			rmArtTooltip();
		NFsetPref("advanced.favicons", "bool", this.favicons);
		NFsetPref("advanced.horizontalPanes", "bool", this.horiz);
		NFsetPref("advanced.bookmarkSync", "bool", this.bookmarkSync);
		NFsetPref("advanced.interestFilter", "bool", this.spam);
		NFsetPref("advanced.autoRefreshInterval", "int", this.autoRefreshInterval);
		for (var i=1; i<=this.keyword.length; i++)
			NFsetPref("keyword"+i, "str", unescape(encodeURIComponent(this.keyword[i-1])));
		var prefs = Components.classes["@mozilla.org/preferences-service;1"]
				.getService(Components.interfaces.nsIPrefService)
				.getBranch("newsfox.");
		while (NFgetPref("keyword"+i, "str", null))
			prefs.clearUserPref("keyword"+i++);
		if (savenonUI)
		{
			NFsetPref("advanced.guessHomepage", "bool", this.guessHomepage);
			NFsetPref("advanced.autoRefreshTime", "int", this.autoRefreshTime/60000);
			NFsetPref("override.showSearchUnread", "bool", this.srchUnread);
			NFsetPref("advanced.catSort", "int", this.catSort);
			NFsetPref("advanced.flagTagged", "bool", this.flagTagged);
			NFsetPref("advanced.aggregate", "bool", this.aggregate);
			NFsetPref("advanced.backupFeedFiles", "bool", this.backupFeedFiles);
			NFsetPref("advanced.indent", "bool", this.indent);
			NFsetPref("global.notifyUponNewSound", "bool", this.notifyUponNewSound);
			NFsetPref("advanced.dontDeleteFlagged", "bool", this.dontDeleteFlagged);
			NFsetPref("advanced.oneGroupOpen", "bool", this.oneGroupOpen);
			NFsetPref("advanced.linkify", "bool", this.linkify);
			NFsetPref("advanced.linkifyXHTML", "bool", this.linkifyXHTML);
			NFsetPref("advanced.wmodeOpaque", "bool", this.wmode);
			NFsetPref("advanced.fixyoutube1", "bool", this.fixyoutube1);
			NFsetPref("advanced.fixMailto", "bool", this.fixMailto);
			NFsetPref("advanced.defaultXfilterIsWeb", "bool", this.defaultXfilterIsWeb);
			NFsetPref("advanced.linuxNoDragAndDrop", "bool", this.linuxNoDragDrop);
		}
		noObserve = false;
  }

	this.renamePrefs = function()
	{
		var prefs = Components.classes["@mozilla.org/preferences-service;1"]
				.getService(Components.interfaces.nsIPrefService)
				.getBranch("newsfox.");
		var openNewPref = NFgetPref("advanced.buttonOpensNewTab", "bool", null);
		if (openNewPref != null)
		{
			if (openNewPref) NFsetPref("z.buttonOpensNewTab", "bool", true);
			prefs.clearUserPref("advanced.buttonOpensNewTab");
		}
		var done = NFgetPref("internal.rename", "bool", false);
		if (!done)
		{
			NFsetPref("internal.addUrl", "str", 
				NFgetPref("global.addUrl", "str", NEWSFOX_RSS));
			if (NFgetPref("global.addUrl","str",null))
				prefs.clearUserPref("global.addUrl");
			NFsetPref("advanced.favicons", "bool", 
				NFgetPref("global.favicons", "bool", 
					NFgetPref("browser.chrome.favicons","bool",true,true)));
			if (NFgetPref("global.favicons","bool",null))
				prefs.clearUserPref("global.favicons");
			NFsetPref("advanced.guessHomepage", "bool", 
				NFgetPref("global.guessHomepage", "bool", true));
			if (NFgetPref("global.guessHomepage","bool",null))
				prefs.clearUserPref("global.guessHomepage");
			NFsetPref("advanced.refreshTimeoutInSeconds", "int", 
				NFgetPref("global.refreshTimeoutInSeconds", "int", 60));
			if (NFgetPref("global.refreshTimeoutInSeconds","int",null))
				prefs.clearUserPref("global.refreshTimeoutInSeconds");
			NFsetPref("advanced.horizontalPanes", "bool", 
				NFgetPref("global.horizontalPanes", "bool", false));
			if (NFgetPref("global.horizontalPanes","bool",null))
				prefs.clearUserPref("global.horizontalPanes");
			if (!gKMeleon)
			{
				var dB = NFgetPref("global.doneButton", "bool", null);
				if (dB != null)
				{
					NFsetPref("internal.doneButton", "bool", dB);
					prefs.clearUserPref("global.doneButton");
				}
				var dAS = NFgetPref("global.doneAutoSubscribe", "bool", null);
				if (dAS != null)
				{
					NFsetPref("internal.doneAutoSubscribe", "bool", dAS);
					prefs.clearUserPref("global.doneAutoSubscribe");
				}
			}
			NFsetPref("internal.rename", "bool", true);
		}
	}

	this.loadImages = function()
	{
		checkLocalPng("tBcheck","check.png");
		checkLocalPng("tBcancel","cancel.png");
		checkLocalPng("tBaddFolder","addFolder.png");
		checkLocalPng("tBdelFolder","delFolder.png");
		checkLocalPng("tBadd","add.png");
		checkLocalPng("tBdel","del.png");
		checkLocalPng("tBlivemark","livemark.png");
		checkLocalPng("tBoptions","options.png");
		checkLocalPng("tBhelp","help.png");
		checkLocalPng("fBhome","home.png");
		checkLocalPng("fBcheck","check.png");
		checkLocalPng("fBtag","tag.png");
		checkLocalPng("fBmarkAllAsRead","markAllAsRead.png");
		checkLocalPng("fBmarkAllAsUnread","markAllAsUnread.png");
		checkLocalPng("fBdelete","delete.png");
		checkLocalPng("fBoptions","options.png");
	}

	this.moveAutoCheckToFeed = function()
	{
		var doAuto = NFgetPref("global.autoRefresh", "bool", false);
		this.moveAuto = 0;
		if (doAuto) this.moveAuto = NFgetPref("global.autoRefreshInterval", "int", 30)
		var prefs = Components.classes["@mozilla.org/preferences-service;1"]
				.getService(Components.interfaces.nsIPrefService)
				.getBranch("newsfox.");
		if (NFgetPref("global.autoRefresh","bool",null))
			prefs.clearUserPref("global.autoRefresh");
		if (NFgetPref("global.autoRefreshInterval", "int", null))
			prefs.clearUserPref("global.autoRefreshInterval");
	}

	this.moveGlobalSorts = function()
	{
		if (NFgetPref("sorts", "str", null)) return;

		var prefs = Components.classes["@mozilla.org/preferences-service;1"]
				.getService(Components.interfaces.nsIPrefService)
				.getBranch("newsfox.");
		var sortsStr = "";
		var i=0;
		while (NFgetPref("sorts.column"+i, "str", null))
		{
			var sPair = new sortPair();
			var s = NFgetPref("sorts.column"+i, "str", null);
			if (NFgetPref("sorts.column"+i, "str", null))
				prefs.clearUserPref("sorts.column"+i);
			switch (s.substring(0,1).toLowerCase())
			{
				case "f":
				case "t":
				case "r":
				case "d":
				case "a":
				case "s":
				case "b":
				case "p":
					sortsStr += s.substring(0,1).toLowerCase();

					s = NFgetPref("sorts.direction"+i, "str", "d");
					if (NFgetPref("sorts.direction"+i, "str", null))
						prefs.clearUserPref("sorts.direction"+i);
					switch (s.substring(0,1).toLowerCase())
					{
						case "a":
							sortsStr += "+";
							break;
						case "d":
							sortsStr += "-";
						default:
					}
				default: i++
			}
		}
		if (i == 0) sortsStr = "d-";
		if (sortsStr == "") sortsStr = "n+";
		NFsetPref("sorts", "str", sortsStr);
	}
}

function moveAutoCheck()
{
	var aRI = gOptions.moveAuto;
	for (var i=0; i<gFmodel.size(); i++)
	{
		var feed = gFmodel.get(i);
		if (feed.autoCheck) feed.autoRefreshInterval = aRI;
	}
}

function doToolbar(name)
{
	var tB = document.getElementById(name);
	if (tB == null) return false;
	if (tB.hidden)
		tB.removeAttribute("hidden");
	else
		tB.hidden = true;
	return true;
}
