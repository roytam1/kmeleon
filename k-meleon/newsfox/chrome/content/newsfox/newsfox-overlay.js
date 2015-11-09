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
 * Portions created by the Initial Developer are Copyright (C) 2006-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
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

const NF_ADDURI = "chrome://newsfox/content/addurl.xul?%s";
const NF_AUTOSIZE = 30;
const NF_UUID = "{899DF1F8-2F43-4394-8315-37F6744E6319}";
const NF_MENUBAR = "toolbar-menubar";
const NF_NAVBAR = "nav-bar";
const NF_NAVBAR_NEW = "navigation-toolbar";
const NF_locales = [ "en-US", "ja-JP", "de-DE", "fr-FR", "ru-RU", "it-IT", "es-ES", "nl-NL", "zh-TW", "zh-CN", "tr-TR", "sk-SK", "pt-BR", "pl-PL", "hu-HU", "fi-FI", "et-EE", "cs-CZ", "bg-BG" ];
const NF_localePref = "general.useragent.locale";
const NF_STATUSBARBUTTONORDER = "ncm";
const NF_STATUSBARMENUORDER = "a";
const NF_STATUSBARLINKSMATCH = "rss|xml|atom";

function NFinitoverlay(button)
{
	NFsetCSS();
	var NFuserAgent = NFsetUserAgent();
	if (gKMeleon)
	{
		window.location.href = "chrome://newsfox/content/newsfox.xul";
		return;
	}
	NFstatusBar.register(button);
  NFstatusBar.observe();
	window.addEventListener('unload', NFstatusBar.unregister, false);
  var NFdoneAuto = NFgetPref("internal.doneAutoSubscribe", "bool", 
		NFgetPref("global.doneAutoSubscribe", "bool", false));
	if (!NFdoneAuto) NFaddAutoSubscribe();
	if (gSeaMonkey) NFdoSeaMonkeyStuff();
	try
	{
//  the following line is a syntax error in Firefox 1.5
//	Components.utils.import("resource://gre/modules/AddonManager.jsm");
		Components.utils["import"]("resource://gre/modules/AddonManager.jsm");
		AddonManager.addAddonListener(NFuninstallObserver);
		window.addEventListener('unload', 
			function() { AddonManager.removeAddonListener(NFuninstallObserver) }
			, false);
	}
	catch(e) {}
	NFuninstallObserver.register();
	window.addEventListener('unload', NFuninstallObserver.unregister, false);
}

function NFaddAutoSubscribe()
{
  try
  {
	// following 'appropriate' method doesn't handle chrome://
	// FF bug#391286
	// navigator.registerContentHandler("application/vnd.mozilla.maybe.feed", NF_ADDURI, "Newsfox");
	var feedUri = new Array();
	for (var i=0; i<=NF_AUTOSIZE; i++)
	{
		feedUri[i] = NFgetPref("browser.contentHandlers.types." + i + ".uri", "str", "",true);
		if (feedUri[i] == NF_ADDURI) return;
	}
	var i=2;
	var done = false;
	var feedPref = "chrome://browser-region/locale/region.properties";
	if (gUserAgent.indexOf("SeaMonkey") > -1)
		feedPref = "chrome://navigator-region/locale/region.properties";
	while (!done && i <= NF_AUTOSIZE)
		if (feedUri[++i] == feedPref) done = true;
	if (done)
	{
		NFsetPref("browser.contentHandlers.types." + i + ".title", "str", "NewsFox",true);
		NFsetPref("browser.contentHandlers.types." + i + ".uri", "str", NF_ADDURI,true);
		NFsetPref("browser.contentHandlers.types." + i + ".type", "str", "application/vnd.mozilla.maybe.feed",true);
	}
//	NFsetPref("browser.contentHandlers.auto.application/vnd.mozilla.maybe.feed","str",NF_ADDURI,true);
//	NFsetPref("browser.feeds.handlers.webservice","str",NF_ADDURI,true);
  }
  catch(e) {}
  NFsetPref("internal.doneAutoSubscribe", "bool", true); 
}

function openNewsfox(newTab,realNewTab)
{
	if (realNewTab != undefined)
		newTab = realNewTab;
	else
	{
		var nT = NFgetPref("z.buttonOpensNewTab", "bool", false);
		if (nT) newTab = !newTab;
	}
	var tabbrowser = window.gBrowser;
  var tabs = tabbrowser.tabContainer.childNodes;
  var tab;
  for (var i = 0; i < tabs.length; ++i) {
    tab = tabs[i];
    var browser = tabbrowser.getBrowserForTab(tab);
		if("chrome://newsfox/content/newsfox.xul" == browser.contentDocument.location)
		{
			tabbrowser.selectedTab = tab;
			return;
		}
	}
	// if we got to this point, it means that no NewsFox in current window. Try to search other windows if any.

	var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator);
	var enumerator = wm.getEnumerator("navigator:browser");
	while(enumerator.hasMoreElements())
	{
	  var win = enumerator.getNext();
  	// |win| is [Object ChromeWindow] (just like |window|), do something with it
		tabbrowser = win.gBrowser;
	  tabs = tabbrowser.tabContainer.childNodes;
	  for (var i = 0; i < tabs.length; ++i) {
	    tab = tabs[i];
	    var browser = tabbrowser.getBrowserForTab(tab);
			if("chrome://newsfox/content/newsfox.xul" == browser.contentDocument.location)
			{
				// some problems with raising, so we close it to re-open after in current window
				browser.loadURI("about:blank", null, null); // if there is only one tab it'll not be removed. So, we change the browser location to save feeds
				tabbrowser.removeTab(tab);
			}
  	}
	}
	// Now we sure that no NewsFox is opened. Starting one.
	if (newTab) {
		window.getBrowser().selectedTab = window.getBrowser().addTab("chrome://newsfox/content/newsfox.xul")
	} else {
		window.getBrowser().loadURI("chrome://newsfox/content/newsfox.xul", null, null);
	}
}

// modified from http://xulsolutions.blogspot.com/2006/07/creating-uninstall-script-for.html
var NFuninstallObserver =
{
	_uninstall : false,
  _uninstalled : false,

  onUninstalled : function(addon)
  {
    if (addon.id == NF_UUID && !this._uninstalled) this.cleanup();
  },
  
	onUninstalling : function(addon, restart)
	{
		if (addon.id == NF_UUID) this._uninstall = true;
	},

	onOperationCancelled : function(addon)
	{
		if (addon.id == NF_UUID) this._uninstall = false;
	},

	observe : function(subject, topic, data)
	{
		if (topic == "quit-application-requested" && this._uninstall && !this._uninstalled) this.cleanup();
	},

	register : function()
	{
		var observerService =
				Components.classes["@mozilla.org/observer-service;1"].
				getService(Components.interfaces.nsIObserverService);
		observerService.addObserver(this, "quit-application-requested", false);
	},

	unregister : function()
	{
  	var observerService =
				Components.classes["@mozilla.org/observer-service;1"].
				getService(Components.interfaces.nsIObserverService);
		observerService.removeObserver(this,"quit-application-requested");
	},

	cleanup : function()
	{
		const NF_SB = document.getElementById("newsfox-string-bundle");
		if (noYesConfirm(NF_SB.getString('uninstallDeleteFilesPrefs')))
		{
			try
			{
				var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
				var profURL = getProfURL(NFgetPref("global.directory", "str", ""));
				var file = nsIPH.getFileFromURLSpec(profURL);
				if (noYesConfirm(file.path))
    		if (file.exists()) file.remove(true);
			}
			catch(e) { window.alert(NF_SB.getString('uninstallUnableFiles')); }
			try
			{
 				var pref = 
						Components.classes["@mozilla.org/preferences-service;1"]
						.getService(Components.interfaces.nsIPrefBranch);
				pref.deleteBranch("newsfox");

				for (var i=0; i<=NF_AUTOSIZE; i++)
				{
					var feedUri = NFgetPref("browser.contentHandlers.types." + i + ".uri", "str", "",true);
					if (feedUri == NF_ADDURI)
						pref.deleteBranch("browser.contentHandlers.types." + i + ".");
				}
				var handlerUri = NFgetPref("browser.contentHandlers.auto.application/vnd.mozilla.maybe.feed", "str", "",true);
				if (handlerUri == NF_ADDURI)
					pref.clearUserPref("browser.contentHandlers.auto.application/vnd.mozilla.maybe.feed");
				handlerUri = NFgetPref("browser.feeds.handlers.webservice", "str", "",true);
				if (handlerUri == NF_ADDURI)
					pref.clearUserPref("browser.feeds.handlers.webservice");
			}
			catch(e) { window.alert(NF_SB.getString('uninstallUnablePrefs')); }
		}
    this._uninstalled = true;
    this.unregister();
	}
}

function NFsetCSS()
{
	var file = NFgetProfileDir(false);
	if (file == null) return;
	file.append("newsfox.css");
	if (file.exists())
	{
		var sss = Components.classes["@mozilla.org/content/style-sheet-service;1"]
						.getService(Components.interfaces.nsIStyleSheetService);
		var ios = Components.classes["@mozilla.org/network/io-service;1"]
						.getService(Components.interfaces.nsIIOService);
		var uri = ios.newURI(getFileSpec(file), null, null);
		if(!sss.sheetRegistered(uri, sss.USER_SHEET))
  		sss.loadAndRegisterSheet(uri, sss.USER_SHEET);
	}
}

function NFdoSeaMonkeyStuff()
{
	// SeaMonkey doesn't change locale properly
	var locale = null;
	var branch = Components.classes["@mozilla.org/preferences-service;1"]
										.getService(Components.interfaces.nsIPrefBranch);
	try
	{
		locale = branch.getComplexValue(NF_localePref,
							Components.interfaces.nsIPrefLocalizedString).data;
	}
	catch (e)
		{ locale = branch.getCharPref(NF_localePref); }

	if (locale)
	{
		var useL = null;
		for (var i=0; i<NF_locales.length; i++)
			if (locale == NF_locales[i])
				{ useL = NF_locales[i]; break; }
		if (useL == null)
			for (var i=0; i<NF_locales.length; i++)
				if (locale.substr(0,2) == NF_locales[i].substr(0,2))
					{ useL = NF_locales[i]; break; }
		try
		{
// seems to no longer work as of seamonkey 2.?  
// not in either CI.nsIXULChromeRegistry or CI.nsIChromeRegistry
			if (useL)
			{
				var registry = 
					Components.classes["@mozilla.org/chrome/chrome-registry;1"]
					.getService(Components.interfaces.nsIChromeRegistrySea);
				// true writes to profile chrome.rdf, won't need extra permissions
				registry.selectLocaleForPackage(useL, "newsfox", true);
			}
		}
		catch(e) {}
	}
}

var NFstatusBar = 
{
	QueryInterface: function(aIID)
	{
		if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
				aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
				aIID.equals(Components.interfaces.nsISupports))
			return this;
		throw Components.results.NS_NOINTERFACE;
	},

	onStateChange: function(aWebProgress, aRequest, aFlag, aStatus)
	{
		const S_START = Components.interfaces.nsIWebProgressListener.STATE_START;
		const S_STOP = Components.interfaces.nsIWebProgressListener.STATE_STOP;
		if(aFlag & S_START|S_STOP)
			this.doCSSfull();
		return 0;
	},

	onLocationChange: function(aProgress, aRequest, aURI)
	{ this.doCSSfull(); },

	onProgressChange: function() {return 0;},
	onStatusChange: function() {return 0;},
	onSecurityChange: function() {return 0;},
	onLinkIconAvailable: function() {return 0;},

	observe: function(subject, topic, data)
	{ 
    gOptions.load();
    if (null == document.getElementById("newsfox-status-label")) return;
    var statusLabel = document.getElementById("newsfox-status-label");
    if (gOptions.statusBarText == 2)
      statusLabel.hidden = true;
    else
      statusLabel.removeAttribute("hidden");
    statusLabel.value = "";
  },

	register: function(button)
	{
    window.gBrowser.addProgressListener(NFstatusBar);
		window.gBrowser.addEventListener('DOMLinkAdded', NFstatusBar.doCSSfull, true);
		window.gBrowser.addEventListener('TabSelect', NFstatusBar.doCSSfull, true);
		var nsIPrefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
		if (typeof nsIPrefBranch.addObserver == "function")
			nsIPrefBranch.addObserver("newsfox.internal.", NFstatusBar, false);
		else
		{
			var nsIPrefBranch2 = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch2);
			nsIPrefBranch2.addObserver("newsfox.internal.", NFstatusBar, false);
		}
    button.addEventListener('click', NFstatusBar.doButton, true);
	},

	unregister: function()
	{
    window.gBrowser.removeProgressListener(NFstatusBar);
		window.gBrowser.removeEventListener('DOMLinkAdded', NFstatusBar.doCSSfull, true);
		window.gBrowser.removeEventListener('TabSelect', NFstatusBar.doCSSfull, true);
		var nsIPrefBranch = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
		if (typeof nsIPrefBranch.removeObserver == "function")
			nsIPrefBranch.removeObserver("newsfox.internal.", NFstatusBar, false);
		else
		{
			var nsIPrefBranch2 = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch2);
			nsIPrefBranch2.removeObserver("newsfox.internal.", NFstatusBar, false);
		}
		if (document.getElementById("newsfox-status-button"))
			document.getElementById("newsfox-status-button").removeEventListener('click', NFstatusBar.doButton, true);
	},

	_real : false,

	doButton: function(event)
	{
		if (event.target.id != "newsfox-status-button") return;
		var buttonOrder = NFgetPref("internal.statusBarButtonOrder", "str", NF_STATUSBARBUTTONORDER);
		buttonOrder = buttonOrder.toLowerCase() + "xxx";
		switch (buttonOrder[event.button])
		{
			case "m":
				NFstatusBar._real = true;
				event.target.firstChild.showPopup();
				break;
			case "c":
				openNewsfox(false,false);
				break;
			case "n":
				openNewsfox(false,true);
		}
	},

  removeMenu: function(menuPopup,event)
  {
  	while (menuPopup.firstChild)
			menuPopup.removeChild(menuPopup.firstChild);
    NFstatusBar._real = false;
  },
  
	makeMenu: function(menuPopup,event)
	{
		if (!NFstatusBar._real) return false;
		// only build the main menu, this builds the submenu
		if (event.target.parentNode.id != "newsfox-status-button") { return true; }
		while (menuPopup.firstChild)
			menuPopup.removeChild(menuPopup.firstChild);
	
		var iosvc = Components.classes["@mozilla.org/network/io-service;1"]
				.getService(Components.interfaces.nsIIOService);
		var baseURI = iosvc.newURI(window.gBrowser.contentDocument.location, null, null);
	
		var menuOrder = NFgetPref("internal.statusBarMenuOrder", "str", NF_STATUSBARMENUORDER);
		var menuItem, newTab;
		for (var i=0; i<menuOrder.length; i++)
		{
			newTab = null;
			switch (menuOrder[i].toLowerCase())
			{
				case "a":
					var links = window.gBrowser.contentDocument.getElementsByTagName("link");
					for (var j=links.length-1; j>=0; j--)
						if (NFstatusBar.hasFeedType(links[j]))
							{
								var title = links[j].title;
								var href = baseURI.resolve(links[j].href);
								menuItem = document.createElement("menuitem");
								menuItem.setAttribute("label",title || href);
								menuItem.setAttribute("style", "font-weight: bold;");
								menuItem.setAttribute("tooltiptext", href);
								menuPopup.appendChild(menuItem);
							}
					break;
				case "l":
					var menu = NFstatusBar.makeBodyLinksMenu(baseURI);
					if (menu) menuPopup.appendChild(menu);
					break;
				case "n":
					newTab = "+";
				case "c":
					if (!newTab) newTab = "-";
					menuItem = document.createElement("menuitem");
					menuItem.setAttribute("label", "NewsFox");
					menuItem.setAttribute("tooltiptext",newTab);
					menuPopup.appendChild(menuItem);
					break;
			}
		}
		return true;
	},

	makeBodyLinksMenu: function(baseURI)
	{
		var menu = document.createElement("menu");
		menu.setAttribute("tooltiptext", "");
		const NF_SB = document.getElementById("newsfox-string-bundle");
		menu.setAttribute("label",NF_SB.getString('statusBarLinksMenu'));
		var menupopup2 = document.createElement("menupopup");
		var linksArray = new Array();
		var re;
		try
		{
			re = new RegExp(NFgetPref("internal.statusBarLinksMatch", "str", NF_STATUSBARLINKSMATCH),"");
		}
		catch(e)
		{
			var badRE = NF_SB.getString('filterErrorBadRegExp');
			window.alert(badRE + ": " + NFgetPref("internal.statusBarLinksMatch", "str", NF_STATUSBARLINKSMATCH));
			re = new RegExp(NF_STATUSBARLINKSMATCH,"");
		}
		links = window.gBrowser.contentDocument.links;
		for (i=0; i<links.length; i++)
			if (links[i].href.match(re)) linksArray.push(links[i]);
		for (i=0; i<linksArray.length; i++)
		{
			var link = linksArray[i];
			var title = link.title;
			var text = null;
			try { text = link.textContent; }
			catch(e) {}
			var href = baseURI.resolve(linksArray[i].href);
			menuItem = document.createElement("menuitem");
			menuItem.setAttribute("label",text || title || href);
			menuItem.setAttribute("tooltiptext", ">" + href);
			menupopup2.appendChild(menuItem);
		}
		menu.appendChild(menupopup2);

		if (linksArray.length) return menu;
		else return null;
	},
	
	subscribe: function(event)
	{
		var url = event.target.getAttribute("tooltiptext");
		var inNew;
		switch (url.substring(0,1))
		{
			case ">":
				openNewTab(url.replace(/^>/,""));
				return;
			default:
				NFsetPref("internal.addUrl", "str", url);
				inNew = true;
				break;
			case "-":
				inNew = false;
				break;
			case "+":
				inNew = true;
		}
		openNewsfox(false,inNew);
	},

	doCSSfull: function()
	{
    try
    {
		  var links = window.gBrowser.contentDocument.getElementsByTagName("link");
		  var num = 0;
		  for (var i=0; i<links.length; i++)
			 if (NFstatusBar.hasFeedType(links[i])) num++;
		  if (num > 0)
			 document.getElementById("newsfox-status-button-image")
						  .style.backgroundColor = "blue";
    	else
			 document.getElementById("newsfox-status-button-image")
						  .style.backgroundColor = "transparent";
    }
    catch(e) {}
	},

	hasFeedType: function(link)
	{
		var rel = link.getAttribute("rel");
		if (rel) rel = rel.toLowerCase();
		var ltype = link.getAttribute("type");
		if (ltype) ltype = ltype.toLowerCase();
		if (rel == "alternate" &&
					(ltype == "application/rss+xml" ||
					ltype == "application/atom+xml" ||
					ltype == "application/rdf+xml"))
			return true;
		else
			return false;
	}
}
