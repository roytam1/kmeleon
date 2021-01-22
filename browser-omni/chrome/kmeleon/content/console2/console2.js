var gConsole = null;
var gToolbox = null;
var gFilter = null;
var gTextBoxEval = null;
var gEvaluator = null;
var gCodeToEvaluate = null;


var gModes = { // set to defaults
	"": { "Errors": true, "Warnings": true, "Messages": true },
/* for k-meleon:
	"Lang": { "JS": true, "CSS": false, "XML": false },*/
	"Lang": { "JS": true, "CSS": false, "XML": false, "KMM": false },
	"Ctx": { "Chrome": false, "Content": true, "Resource": true }
};

var Cc = Components.classes;
var Ci = Components.interfaces;


/* :::::::: Console Initialization ::::::::::::::: */

function ConsoleStartUp()
{
	gConsole = _("ConsoleBox");
	gTextBoxEval = _("TextboxEval");
	gEvaluator = _("Evaluator");
	gConsole.addEventListener("focus", function() { gConsole._focused = true; }, true);
	gConsole.addEventListener("blur", function() { gConsole._focused = false; }, true);
	// as long as the commandupdater gets it wrong the first time:
	gConsole.addEventListener("select", updateMenuItems, false);
	
	gConsole.registerListener({ listen: filterRow });
	gConsole.registerListener(gBlacklist, true);
	if (gBlacklist.loadBlocklist)
		gBlacklist.loadBlocklist();
	if (gConsole.firstChild && gConsole.firstChild.className == "console-row")
	{
		gConsole.children.forEach(function(aItem) { filterRow(aItem); gBlacklist.listen(aItem); });
	}
	
	gEvaluator.addEventListener("load", loadOrDisplayResult, true);
	
	gToolbox = _("ConsoleToolbox");
	gToolbox.customizeDone = customizeToolbarDone;
	
	updateToolbars();
	updateSortCommand();
	toggleDupes(true);
	toggleTimestamps(true);
	
	gBooleanPrefObserver.registerSelf();
	
	if (!gBlacklist.mPermissionManager)
	{
		var item = _("menu_blacklist");
		item.label = item.getAttribute("_label2");
	}
	
	if (_inSidebar())
	{
		document.documentElement.setAttribute("container", window.frameElement.id || "chrome");
	}
	
	gConsole.focus();
}

function ConsoleShutDown()
{
	if (gBlacklist.saveBlocklist)
		gBlacklist.saveBlocklist();
	gBooleanPrefObserver.unregisterSelf();
}

function versionInit()
{
	if (_isGecko("1.9.1b1pre", "min"))
	{
		/* Starting with 1.9.1b1pre search textbox binding is available */
		searchFilterbox()
	}
}

function searchFilterbox()
{
	var sss = Components.classes["@mozilla.org/content/style-sheet-service;1"]
			.getService(Components.interfaces.nsIStyleSheetService);
	var uss = Components.classes["@mozilla.org/network/io-service;1"]
			.getService(Components.interfaces.nsIIOService)
			.newURI("chrome://console2/content/textbox191.css", null, null);
	if(!sss.sheetRegistered(uss, sss.USER_SHEET))
		sss.loadAndRegisterSheet(uss, sss.USER_SHEET);
}

/* :::::::: Console UI Functions ::::::::::::::: */

function toggleMode(aMode, aType)
{
	aType = aType || "";
	gModes[aType][aMode] = !gModes[aType][aMode];
	
	setTimeout(function() { updateModeCommand(aType, aMode); }, 0);
}

function changeMode(aMode, aType)
{
	aMode = aMode || "All";
	aType = aType || "";
	var fallBack = null;
	
	for (var mode in gModes[aType])
	{
		gModes[aType][mode] = aMode == mode || aMode == "All";
	}
	if (aType == "Lang" && _("Console:enableCSS").getAttribute("disabled") == "true")
	{
		gModes[aType]["CSS"] = false;
		fallBack = "JS";
	}
	
	updateModeCommand(aType, fallBack);
}

function showAll(aEvent)
{
	changeMode("All");
	changeMode("All", "Lang");
	changeMode("All", "Ctx");
	
	if (gFilter)
	{
		gFilter.value = "";
		filterConsole();
	}
}

function clearConsole()
{
	gConsole.clear();
}

function toggleDupes(aInit)
{
	if (!aInit)
	{
		gConsole.dupes = !gConsole.dupes;
		gConsole.scrollToSelectedItem();
	}
	_("Console:modeDupes").setAttribute("checked", !gConsole.dupes);
}

function toggleTimestamps(aInit)
{
	if (!aInit)
		gConsole.timestamps = !gConsole.timestamps;
	else if (_isGecko("8.1", "max"))
	{
		// not (yet) available on the Gecko 8 branch.
		var menuitem = _("options_timestamps");
		if (menuitem)
			menuitem.hidden = true;
	}
	_("Console:modeTimestamps").setAttribute("checked", gConsole.timestamps);
}

function toggleBooleanPref(aPref, aInit)
{
	var value = gBooleanPrefObserver.mPrefBranch.getBoolPref(aPref);
	if (!aInit)
	{
		gBooleanPrefObserver.mPrefBranch.setBoolPref(aPref, (value = !value));
	}
	
	var item = _("pref_" + aPref);
	if (item)
	{
		item.setAttribute("checked", value);
	}
	
	if (aPref == "layout.css.report_errors")
	{
		if (!value && gModes["Lang"]["CSS"])
		{
			gModes["Lang"]["CSS"] = false;
		}
		goSetCommandEnabled("Console:enableCSS", value);
		goSetCommandEnabled("Console:modeLangCSS", value);
		updateModeCommand("Lang", "JS");
	}
}

/*const*/ var gBooleanPrefObserver = {
	mPrefBranch: Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2),
	_prefs: ["javascript.options.strict", "layout.css.report_errors", "dom.report_all_js_exceptions"],

	observe: function(aSubject, aTopic, aData)
	{
		if (this._prefs.indexOf(aData) > -1)
		{
			toggleBooleanPref(aData, true);
		}
	},

	registerSelf: function()
	{
		for (var i = 0; i < this._prefs.length; i++)
		{
			this.mPrefBranch.addObserver(this._prefs[i], this, false);
		}
	},

	unregisterSelf: function()
	{
		for (var i = 0; i < this._prefs.length; i++)
		{
			this.mPrefBranch.removeObserver(this._prefs[i], this);
		}
	}
};

function changeSortOrder(aOrder)
{
	var reversed = gConsole.reversed;
	gConsole.sortOrder = aOrder;
	
	if (gConsole.reversed != reversed)
	{
		gConsole.scrollToSelectedItem();
		updateSortCommand();
	}
}

function toggleOnTop(aInit)
{
/*	var xulWin = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation).QueryInterface(Ci.nsIDocShellTreeItem).treeOwner.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIXULWindow);
	
	if (!aInit)
	{
		xulWin.zLevel = (xulWin.zLevel > xulWin.normalZ)?xulWin.normalZ:xulWin.raisedZ;
	}
	if (_("menu_consoleTools"))
	{
		_("item_onTop").setAttribute("checked", xulWin.zLevel > xulWin.normalZ);
	}*/

}


function toggleNoteFocus(aInit)
{
	var enabled = gConsole.getAttribute("_noteFocus") == "true";
	if (!aInit)
	{
		enabled = !enabled;
		gConsole.setAttribute("_noteFocus", enabled);
		document.persist(gConsole.id, "_noteFocus");
	}
	gNoteFocus.enable(enabled);
	
	if (_("menu_consoleTools"))
	{
		_("item_noteFocus").setAttribute("checked", enabled);
	}
}

/*const*/ var gNoteFocus = {
	mEnabled: false,

	listen: function(aRow)
	{
		if (!aRow._hidden)
		{
			setTimeout(function(aRow) {
				if (aRow.visible)
				{
					gConsole.addItemToSelection(aRow);
					gConsole.currentItem = aRow;
					gConsole.scrollToSelectedItem();
					if (!_inSidebar())
					{
						try
						{
							gConsole.focus();
							window.focus();
						}
						catch (ex) { }
					}
				}
			}, 0, aRow);
		}
	},

	enable: function(aValue)
	{
		if (aValue && !this.mEnabled)
		{
			gConsole.registerListener(this);
		}
		else if (this.mEnabled && !aValue)
		{
			gConsole.unregisterListener(this);
		}
		this.mEnabled = aValue;
	}
};

function ignoreSelectedItems(aDelete)
{
	gConsole.selectedItems.forEach(function(aItem) {
		var label = aItem.label2;
		if (label && (!aDelete || !gConsole.dupes && aItem.hasAttribute("dupes")))
		{
			gConsole.children.forEach(function(aItem) {
				if (aItem.label2 == label)
				{
					gConsole.removeConsoleRow(aItem);
				}
			});
			if (!aDelete)
			{
				gIgnoredItems.add(label);
			}
		}
		else if (aDelete)
		{
			gConsole.removeConsoleRow(aItem);
		}
	});
	gConsole.clearSelection();
}

var gIgnoredItems = {
	mMessages: [],

	listen: function(aRow)
	{
		if (this.mMessages.indexOf(aRow.label2 || "") > -1)
		{
			gConsole.removeConsoleRow(aRow);
		}
	},

	add: function(aLabel)
	{
		if (this.mMessages.length == 0)
		{
			gConsole.registerListener(this, true);
		}
		this.mMessages.push(aLabel);
	}
};

function blacklistSelectedItems()
{
	gConsole.selectedItems.forEach(function(aItem) {
		if (!aItem.getAttribute("hideSource"))
		{
			gBlacklist.add(aItem.getAttribute("href"));
		}
	});
	gConsole.children.forEach(function(aItem) { gBlacklist.listen(aItem); });
}

var gBlacklist = ("@mozilla.org/permissionmanager;1" in Cc)?{
	mIOService: Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService),
	mPermissionManager: Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager),

	ALLOW_DOMAIN: Ci.nsIPermissionManager.ALLOW_ACTION,
	DENY_DOMAIN: Ci.nsIPermissionManager.DENY_ACTION,
	ALL_DOMAINS: Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService).newURI("http://*/", null, null),

	listen: function(aRow)
	{
		if (!aRow.getAttribute("hideSource"))
		{
			try
			{
				var uri = this.mIOService.newURI(aRow.getAttribute("href"), null, null);
				if ((this.isSupported(uri.spec) && this._test(this.ALL_DOMAINS) == this.DENY_DOMAIN)?this._test(uri) != this.ALLOW_DOMAIN:this._test(uri) == this.DENY_DOMAIN)
				{
					gConsole.removeConsoleRow(aRow);
				}
			}
			catch (ex) { }
		}
	},

	_test: function(aURI)
	{
		return this.mPermissionManager.testPermission(aURI, "console2");
	},

	add: function(aURL)
	{
		if (this.isSupported(aURL))
		{
			this.mPermissionManager.add(this.mIOService.newURI(aURL.replace(/^resource\:\/\/\//, 'resource://app/'), null, null), "console2", this.DENY_DOMAIN);
		}
	},

	isSupported: function(aURL)
	{
		return /^(https?|chrome|resource):\/+./.test(aURL);
	}
}:{ // non-persisting version for Thunderbird
	mIOService: Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService),
	mBlocking: {},
    mBlocklistPref: "extensions.console2.blocklist",

	loadBlocklist: function()
	{
		var nativeJSON = Components.classes["@mozilla.org/dom/json;1"].createInstance(Components.interfaces.nsIJSON);
		var bList = null;
		try
		{
			bList = gBooleanPrefObserver.mPrefBranch.getCharPref(this.mBlocklistPref);
		}
		catch(e) {}
		if (bList) {
			if (bList.charAt(0) == "(") // remove unneeded braces.
				bList = bList.slice(1, -1);
			this.mBlocking = nativeJSON.decode(bList);
		}
	},

	saveBlocklist: function()
	{
		var nativeJSON = Components.classes["@mozilla.org/dom/json;1"].createInstance(Components.interfaces.nsIJSON);
		var blist = "(" + nativeJSON.encode(this.mBlocking) + ")";
		gBooleanPrefObserver.mPrefBranch.setCharPref(this.mBlocklistPref, bList);
	},

	listen: function(aRow)
	{
		if (!aRow.getAttribute("hideSource"))
		{
			try
			{
				var uri = this.mIOService.newURI(aRow.getAttribute("href"), null, null);
				if (this.isSupported(uri.spec) && uri.host in this.mBlocking)
				{
					gConsole.removeConsoleRow(aRow);
				}
			}
			catch (ex) { }
		}
	},

	add: function(aURL)
	{
		if (this.isSupported(aURL))
		{
			this.mBlocking[this.mIOService.newURI(aURL.replace(/^resource\:\/\/\//, 'resource://app/'), null, null).host] = true;
		}
	},

	isSupported: function(aURL)
	{
		return /^(https?|chrome|resource):\/+./.test(aURL);
	}
};

function copySelectedItems()
{
	var content = gConsole.children.filter(function(aItem) {
		return aItem.selected && aItem.visible;
	}).map(function(aItem) {
		return aItem.label;
	}).join("\n ----------\n") + "\n";
	
	Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper).copyString(content);
}

function selectAllItems()
{
	gConsole.selectAll();
}

function openDomainlist()
{
	var dialog =
		Cc["@mozilla.org/appshell/window-mediator;1"]
		.getService(Ci.nsIWindowMediator)
		.getMostRecentWindow("global:console2:blacklist");
	if (dialog)
	{
		dialog.focus();
		return;
	}

	if (gBlacklist.mPermissionManager)
	{
		openDialog("chrome://console2/content/domainlist.xul",
			"_blank",
			"chrome,dialog,resizable,centerscreen");
	}
	else
	{
		openDialog("chrome://console2/content/blocklist.xul",
			"_blank",
			"chrome,dialog,resizable,centerscreen",
			gBlacklist);
	}
}

function toggleMenuFocus()
{
	if (gConsole._focused && _("ToolbarMode").firstChild)
	{
		_("ToolbarMode").firstChild.focus();
	}
	else
	{
		gConsole.focus();
	}
}

function filterConsole()
{
	var filter = gFilter.value.toLowerCase();
	var changed = false;
	
	gConsole.children.forEach(function(aChild) {
		var hide = filter != "" && aChild.label2.toLowerCase().indexOf(filter) == -1;
		if (aChild.hidden != hide) // aChild.visible is much slower
		{
			changed = true;
		}
		aChild.hidden = hide;
	});
	
	if (changed)
	{
		gConsole.scrollToSelectedItem();
		updateMenuItems();
	}
}

/* ........ Evaluate Toolbar Functions .............. */

function onEvalKeyPress(aEvent)
{
	if (aEvent.keyCode == aEvent.DOM_VK_RETURN)
	{
		onEvalTypein();
	}
}

function onEvalTypein()
{
	var code = gTextBoxEval.value;
	if (/\S/.test(code))
	{
		gEvalListener.registerSelf(gConsole, code, true);
		gCodeToEvaluate = code;
		// reset the iframe first; the code will be evaluated in loadOrDisplayResult
		// below, once about:blank has completed loading (see bug 385092)
		gEvaluator.contentWindow.location = "chrome://console2/content/blank.html"; // reset the iframe
		
		if ("@mozilla.org/satchel/form-history;1" in Cc)
		{ // not available for Thunderbird
			Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2 ||
			Ci.nsIFormHistory).addEntry(gTextBoxEval.getAttribute("autocompletesearchparam"), code);
		}
	}
}

function loadOrDisplayResult()
{
	if (gCodeToEvaluate)
	{
		gEvaluator.contentWindow.location = "javascript: " + gCodeToEvaluate.replace(/%/g, "%25");
		gCodeToEvaluate = "";
		return;
	}

	var resultRange = gEvaluator.contentDocument.createRange();
	resultRange.selectNode(gEvaluator.contentDocument.documentElement);
	
	var result = resultRange.toString();
	if (result)
	{
		Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(result);
		gEvalListener.registerSelf(gConsole, result);
	}
}

function showEvalPopup(aItem)
{
	var popup = _("PopupEval");
	if (popup && !(aItem.visible && gConsole._isItemVisible(aItem)))
	{
		popup.style.width = (gTextBoxEval.nextSibling.boxObject.x + gTextBoxEval.nextSibling.boxObject.width - gTextBoxEval.boxObject.x) + "px";
		popup.removeChild(popup.firstChild);
		popup.appendChild(aItem.cloneNode(true));
		popup.firstChild.hidden = false;
		popup.showPopup((document.popupNode = gTextBoxEval), gTextBoxEval.boxObject.x, gTextBoxEval.boxObject.y + gTextBoxEval.boxObject.height);
	}
	gEvalListener.unregisterSelf(gConsole);
}

/*const*/ var gEvalListener = {
	_active: false,
	_id: "",

	listen: function(aRow)
	{
		if (aRow.getAttribute((this._href)?"href":"msg") == this._id)
		{
			if (aRow._hidden)
			{
				showEvalPopup(aRow);
			}
			else
			{
				setTimeout(showEvalPopup, 0, aRow);
			}
		}
	},

	registerSelf: function(aConsole, aId, aJavaScript)
	{
		if (!aId)
		{
			this.unregisterSelf(aConsole);
		}
		else if (!this._active)
		{
			this._active = true;
			aConsole.registerListener(this);
		}
		this._id = ((aJavaScript)?"javascript:":"") + aId;
		this._href = aJavaScript;
	},

	unregisterSelf: function(aConsole)
	{
		if (this._active)
		{
			this._active = false;
			aConsole.unregisterListener(this);
		}
	}
};

/* ........ State Updating Functions .............. */

function updateModeCommands()
{
	for (var type in gModes)
	{
		var fallBack = null;
		var total = gConsole.getAttribute("mode" + type);
		if (total)
		{
			total = total.split(" ");
			for (var mode in gModes[type])
			{
				if (!fallBack)
				{
					fallBack = mode;
				}
				gModes[type][mode] = total.indexOf(mode) > -1 || total.indexOf("All") > -1;
			}
		}
		updateModeCommand(type, fallBack);
	}
}

function updateModeCommand(aType, aLastMode)
{
	var total = [], count = 0;
	
	for (var mode in gModes[aType])
	{
		_("Console:mode" + aType + mode).setAttribute("checked", gModes[aType][mode]);
		if (gModes[aType][mode])
		{
			total.push(mode);
		}
		count++;
	}
	if (total.length == 0 && aLastMode)
	{
		gModes[aType][aLastMode] = true;
		_("Console:mode" + aType + aLastMode).setAttribute("checked", true);
		total.push(aLastMode);
	}
	
	gConsole.setAttribute("mode" + aType, total.join(" "));
	gConsole.scrollToSelectedItem();
	updateMenuItems();
	
	var button = _("mode" + aType + "Button");
	if (button)
	{
		if (aType == "Lang" && _("Console:enableCSS").getAttribute("disabled") == "true")
		{
			count--;
		}
		
		var items = button.getElementsByTagName("menuitem");
		for (var i = 0; i < items.length; i++)
		{
			mode = items[i].getAttribute("_mode");
			items[i].setAttribute("checked", (mode == "All")?total.length == count:total.length == 1 && gModes[aType][mode]);
		}
		
		var label = button.getAttribute("_mixed");
		if (total.length == 1 || total.length == count)
		{
			label = button.getElementsByAttribute("checked", "true")[0].label;
		}
		button.setAttribute("label", button.getAttribute("label").match(/^([^:]+: )?/)[0] + label);
	}
}

function updateSortCommand()
{
	_("Console:sortAscend").setAttribute("checked", !gConsole.reversed);
	_("Console:sortDescend").setAttribute("checked", gConsole.reversed);
}

function updateMenuItems()
{
	if (gConsole.firstChild && !("visible" in gConsole.firstChild))
	{
		return;
	}
	
	function isVisible(aItem) { return aItem.visible; }
	function isSelectable(aItem) { return !aItem.selected && aItem.visible; }
	function isBlacklistable(aItem) { return !aItem.getAttribute("hideSource") && gBlacklist.isSupported(aItem.getAttribute("href")); }
	
	var selectedAndVisible = gConsole.selectedItems.some(isVisible);
	
	goSetCommandEnabled("cmd_ignore", selectedAndVisible);
	goSetCommandEnabled("cmd_blacklist", selectedAndVisible && gConsole.selectedItems.some(isBlacklistable));
	
	goSetCommandEnabled("cmd_select_all", gConsole.children.some(isSelectable));
	goSetCommandEnabled("cmd_copy", selectedAndVisible);
	goSetCommandEnabled("cmd_delete", selectedAndVisible);
}

function updateOptions()
{
	toggleBooleanPref("javascript.options.strict", true);
	try
	{
		toggleBooleanPref("layout.css.report_errors", true);
	}
	catch (ex) // not (yet) available on the Gecko 1.8 branches
	{
		var menuitem = _("pref_layout.css.report_errors");
		if (menuitem)
		{
			menuitem.hidden = true;
		}
	}
	var js_exceptions = "dom.report_all_js_exceptions";
	try
	{
		toggleBooleanPref(js_exceptions, true);
	}
	catch (ex)
	{
		if (_isGecko("1.9.0", "min"))
		{
			gBooleanPrefObserver.mPrefBranch.setBoolPref(js_exceptions, true);
			toggleBooleanPref(js_exceptions, true);
		}
		else
		{
			// not (yet) available on the Gecko 1.8 branches
			var menuitem2 = _("pref_dom.report_all_js_exceptions");
			if (menuitem2)
			{
				menuitem2.hidden = true;
			}
		}
	}
	
	setTimeout(function() {
		// wait for the attribute to be set on the window
		toggleOnTop(true);
		// wait for the initial messages to be loaded
		toggleNoteFocus(true);
	}, 0);
}

function initOnButtonClick()
{
	for (var type in gModes)
	{
		for (var mode in gModes[type])
		{
			var els = document.getElementsByAttribute("observes", "Console:mode" + type + mode);
			for (var i = 0; i < els.length; i++)
			{
				els[i].onmouseup = function(aEvent) {
					if (aEvent.button == 1 || (aEvent.button == 0 && aEvent.shiftKey))
					{ // middle-clicking a mode checkbox makes it behave as a radio button
						var typeMode = this.observes.match(/([A-Z][a-z]+)?([A-Z][A-Za-z]+)$/);
						changeMode(typeMode[2], typeMode[1]);
						aEvent.preventDefault();
						if (this.parentNode.parentNode.tagName == "toolbarbutton")
						{
							this.parentNode.parentNode.open = false;
						}
					}
				};
			}
		}
	}
}

function removeAccelerators()
{
	if (!_inSidebar())
	{
		var cssButton = _("item_modeLangCSS"); // special hack until a better solution for the accelerator is found
		if (cssButton && cssButton.accessKey && cssButton.label.indexOf(cssButton.accessKey) == -1)
		{
/* XXX Ask zeniko what this is all about!
			var key = document.getElementById("cssButtonHack");
			if (!key)
			{
				key = document.createElement("key");
				key.setAttribute("id", "cssButtonHack");
				key.setAttribute("key", cssButton.accessKey);
				key.setAttribute("modifiers", "alt");
				key.setAttribute("oncommand", "var b = _('item_modeLangCSS'); if (b && !b.disabled) b.doCommand();");
				_("consoleKeys").appendChild(key);
			}
			cssButton.removeAttribute("accesskey");
*/
		}
		return;
	}
	
	var buttons = document.getElementsByTagName("toolbarbutton");
	for (var i = 0; i < buttons.length; i++)
	{
		buttons[i].removeAttribute("accesskey");
	}
	
	var keys = _("consoleKeys").getElementsByTagName("key");
	for (i = 0; i < keys.length; i++)
	{
		keys[i].parentNode.removeChild(keys[i]);
	}
}

/* ........ Toolbar Customization Functions .............. */

function onToolbarPopupShowing(aPopup)
{
	var i, item;
	
	for (i = aPopup.childNodes.length - 1; i >= 0; i--)
	{
		item = aPopup.childNodes[i];
		if (item.hasAttribute("toolbarindex"))
		{
			aPopup.removeChild(item);
		}
	}
	
	var toolbars = gToolbox.getElementsByTagName("toolbar");
	for (i = toolbars.length - 1; i >= 0; i--)
	{
		if (toolbars[i].hasAttribute("toolbarname"))
		{
			item = document.createElement("menuitem");
			item.setAttribute("toolbarindex", i);
			item.setAttribute("type", "checkbox");
			item.setAttribute("label", toolbars[i].getAttribute("toolbarname"));
			item.setAttribute("accesskey", toolbars[i].getAttribute("accesskey"));
			item.setAttribute("checked", !toolbars[i].collapsed);
			
			aPopup.insertBefore(item, aPopup.firstChild);
			item.addEventListener("command", function() {
				showToolbar(this.getAttribute("toolbarindex"), this.getAttribute("checked") == "true");
			}, false);
		}
	}
}

function showToolbar(aIndex, aShow)
{
	var toolbars = gToolbox.getElementsByTagName("toolbar");
	toolbars[aIndex].collapsed = !aShow;
	document.persist(toolbars[aIndex].id, "collapsed");
}

function customizeToolbar()
{
	// Disable the toolbar context menu items
	var menubar = document.getElementById("main-menubar");
	if (menubar) // In SeaMonkey we overlay a menubar into the toolbox.
		for (var i = 0; i < menubar.childNodes.length; ++i)
			menubar.childNodes[i].setAttribute("disabled", true);

	_("Console:customizeToolbar").setAttribute("disabled", "true");
	
	openDialog("chrome://global/content/customizeToolbar.xul", "CustomizeToolbar", "chrome,all,dependent", gToolbox);
}

function customizeToolbarDone(aToolboxChanged)
{
	// Re-enable parts of the UI we disabled during the dialog
	var menubar = document.getElementById("main-menubar");
	if (menubar) // In SeaMonkey we overlay a menubar into the toolbox.
		for (var i = 0; i < menubar.childNodes.length; ++i)
			menubar.childNodes[i].setAttribute("disabled", false);

	if (aToolboxChanged)
		updateToolbars();

	ConsoleFilter.customizeToolbarDone();

	_("Console:customizeToolbar").removeAttribute("disabled");
	window.focus();
}

function updateToolbars()
{
	if (_("menu_consoleTools"))
	{
		goSetCommandEnabled("item_onTop", !_inSidebar());
		// _("item_domainlst").hidden = !gBlacklist.mPermissionManager;
		_("item_clearConsole2").hidden = _("item_clearConsole") != null;
	}
	updateModeCommands();
	updateOptions();
	initOnButtonClick();
	removeAccelerators();
	
	gFilter = _("TextboxFilter");
}


/* :::::::: Console Filter Hooks ::::::::::::::: */

function filterRow(aRow)
{
	// debug message filter //
	if (aRow.getAttribute("type") == "error" && aRow.getAttribute("msg").substr(0, 8) == "[debug] ")
	{
		aRow.setAttribute("type", "message");
		aRow.setAttribute("msg", aRow.getAttribute("msg").substr(8));
		aRow.removeAttribute("line");
		aRow.removeAttribute("typetext");
		aRow.removeAttribute("code");
	}
	
	// search filter //
	if (gFilter)
	{
		var filter = gFilter.value.toLowerCase();
		if (filter && aRow.label.toLowerCase().indexOf(filter) == -1)
		{
			aRow._hidden = true;
		}
	}
}


/* :::::::: Console Utility Functions ::::::::::::::: */

function _(aID)
{
	return document.getElementById(aID);
}

function _focus(aID)
{
	var element = _(aID);
	if (element && element.boxObject.height)
	{
		element.focus();
	}
}

function _inSidebar()
{
	return !!window.frameElement;
}

function _isGecko(aVersion, minmax)
{
	var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
			.getService(Components.interfaces.nsIXULAppInfo);
	var versionChecker = Components.classes["@mozilla.org/xpcom/version-comparator;1"]
			.getService(Components.interfaces.nsIVersionComparator);
	switch (minmax) {
	case "max":
		return (versionChecker.compare(appInfo.platformVersion, aVersion) <= 0);
	case "min":
	default:
		return (versionChecker.compare(appInfo.platformVersion, aVersion) >= 0);	
	}
}

versionInit();