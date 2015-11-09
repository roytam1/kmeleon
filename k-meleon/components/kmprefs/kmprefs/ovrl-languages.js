/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adrian Havill <havill@redhat.com>
 *   Ian Neal <iann_bugzilla@blueyonder.co.uk>
 *   Stefan Hermes <stefanh@inbox.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

var gActiveLanguages;
var gLanguages;
var gLanguageNames = [];
var gLanguageTitles = {};

function initLangWeb() {
	
	var observerService = Components.classes["@mozilla.org/observer-service;1"]
		.getService(Components.interfaces.nsIObserverService);
	observerService.notifyObservers(null, "charsetmenu-selected", "other");
	
	var defaultCharsetList = document.getElementById("intl.charset.default");
	defaultCharsetList.setAttribute("ref", "NC:DecodersRoot");
	//<menulist id="defaultCharsetList" ref="" datasources="rdf:charset-menu" preference="intl.charset.default">
	initMenulist(defaultCharsetList.id, "", "");
	
	gActiveLanguages = document.getElementById("activeLanguages");
	// gLanguages stores the ordered list of languages, due to the nature
	// of childNodes it is live and updates automatically.
	gLanguages = gActiveLanguages.childNodes;
	
	ReadAvailableLanguages();
	ReadActiveLanguages();
}

function AddLanguage() {
	//document.documentElement.openSubDialog("chrome://kmprefs/content/kmprefs/dlg-languages-add.xul", "addlangwindow", gLanguageNames);
	// Save how many items in list
	var nlangs = gLanguages.length;
	// Send gLanguages nodes object list to Add dialog
	var params1 = {
		enter : gLanguages,
		out : null
	};
	window.openDialog("chrome://kmprefs/content/kmprefs/dlg-languages-add.xul", "addlangwindow", "modal", gLanguageNames, params1);
	// Check if dialog not was closed/canceled
	if (params1.out != null) {
		var nlangs2 = params1.out.length;
		// Check if languages were added
		if (nlangs < nlangs2) {
			var LangsToAdd = params1.out;
			// Convert the childNodes object array to a string array
			var langlist = "";
			for (var j = 0; j < nlangs; j++) {
				langlist = langlist + gLanguages[j].value + ",";
			}
			// Convert LangsToAdd to string
			LangsToAdd = LangsToAdd.join(",");
			// Cut the string to obtain only new languages added to list
			LangsToAdd = LangsToAdd.replace(langlist, "");
			// Convert LangsToAdd to string array
			LangsToAdd = LangsToAdd.split(/\s*,\s*/);
			
			LangsToAdd.forEach(
				function (aKey) {
				if (aKey) {
					let langTitle = gLanguageTitles.hasOwnProperty(aKey) ?
						gLanguageTitles[aKey] : "[" + aKey + "]";
					gActiveLanguages.appendItem(langTitle, aKey);
				}
			});
			// Languages added to list, enable Apply button
			EnableElementById("langApply", true, false);
		}
	}
}

function ReadAvailableLanguages() {
	var i = 0;
	var languagesBundle = document.getElementById("languagesBundle");
	var prefLangBundle = document.getElementById("prefLangBundle");
	var regionsBundle = document.getElementById("regionsBundle");
	var langStrings = document.getElementById("acceptedBundle").strings;
	/*
	const Cc = Components.classes;
	const Ci = Components.interfaces;
	var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
	var langbundle = stringBundleService.createBundle("resource://gre/res/language.properties");
	//  var str = bundle.GetStringFromName("propertyName");
	var langStrings = langbundle.strings;
	 */
	while (langStrings.hasMoreElements()) {
		// Progress through the bundle.
		var curItem = langStrings.getNext();
		
		if (!(curItem instanceof Components.interfaces.nsIPropertyElement))
			break;
		
		var stringNameProperty = curItem.key.split('.');
		
		var str = stringNameProperty[0];
		if (str && stringNameProperty[1] == 'accept') {
			var stringLangRegion = str.split('-');
			
			if (stringLangRegion[0]) {
				var language = "";
				var region = null;
				
				try {
					language = languagesBundle.getString(stringLangRegion[0]);
				} catch (ex) {}
				
				if (stringLangRegion.length > 1) {
					try {
						region = regionsBundle.getString(stringLangRegion[1]);
					} catch (ex) {}
				}
				
				var title;
				if (region)
					title = prefLangBundle.getFormattedString("languageRegionCodeFormat",
							[language, region, str]);
				else
					title = prefLangBundle.getFormattedString("languageCodeFormat",
							[language, str]);
				gLanguageTitles[str] = title;
				if (curItem.value == "true")
					gLanguageNames.push([title, str]);
			}
		}
	}
	
	// Sort on first element.
	gLanguageNames.sort(
		function compareFn(a, b) {
		return a[0].localeCompare(b[0]);
	});
}

function ReadActiveLanguages() {
	//  var arrayOfPrefs = document.getElementById("intl.accept_languages").value.split(/\s*,\s*/);
	var arrayOfPrefs = getPrefValue("intl.accept_languages").split(/\s*,\s*/);
	
	// No need to rebuild listitems if languages in prefs and listitems match.
	if (InSync(arrayOfPrefs))
		return;
	while (gActiveLanguages.hasChildNodes())
		gActiveLanguages.removeChild(gActiveLanguages.lastChild);
	
	arrayOfPrefs.forEach(
		function (aKey) {
		if (aKey) {
			let langTitle = gLanguageTitles.hasOwnProperty(aKey) ?
				gLanguageTitles[aKey] : "[" + aKey + "]";
			gActiveLanguages.appendItem(langTitle, aKey);
		}
	});
	
	SelectLanguage();
	
	return;
}

// Checks whether listitems and pref values matches, returns false if not.
function InSync(aPrefArray) {
	// Can't match if they don't have the same length.
	if (aPrefArray.length != gLanguages.length)
		return false;
	
	return aPrefArray.every(
		function (aElement, aIndex) {
		return aElement == gLanguages[aIndex].value;
	});
}

// Called on onsynctopreference.
function WriteActiveLanguages() {
	return Array.map(gLanguages, function (e) {
		return e.value;
	}).join(",");
}

function MoveUp() {
	var selected = gActiveLanguages.selectedItem;
	var before = selected.previousSibling;
	if (before) {
		before.parentNode.insertBefore(selected, before);
		gActiveLanguages.selectItem(selected);
		gActiveLanguages.ensureElementIsVisible(selected);
	}
	
	SelectLanguage();
	gActiveLanguages.doCommand();
	
	EnableElementById("langApply", true, false);
}

function MoveDown() {
	var selected = gActiveLanguages.selectedItem;
	if (selected.nextSibling) {
		var before = selected.nextSibling.nextSibling;
		gActiveLanguages.insertBefore(selected, before);
		gActiveLanguages.selectItem(selected);
	}
	
	SelectLanguage();
	gActiveLanguages.doCommand();
	
	EnableElementById("langApply", true, false);
}

function RemoveActiveLanguage(aEvent) {
	if (aEvent && aEvent.keyCode != aEvent.DOM_VK_DELETE &&
		aEvent.keyCode != aEvent.DOM_VK_BACK_SPACE)
		return;
	
	var nextNode = null;
	while (gActiveLanguages.selectedItem) {
		var selectedNode = gActiveLanguages.selectedItem;
		nextNode = selectedNode.nextSibling || selectedNode.previousSibling;
		gActiveLanguages.removeChild(selectedNode);
	}
	
	if (nextNode)
		gActiveLanguages.selectItem(nextNode);
	
	SelectLanguage();
	gActiveLanguages.doCommand();
	
	EnableElementById("langApply", true, false);
}

function SelectLanguage() {
	var len = gActiveLanguages.selectedItems.length;
	EnableElementById("langRemove", len, false);
	var selected = gActiveLanguages.selectedItem;
	EnableElementById("langDown", (len == 1) && selected.nextSibling, false);
	EnableElementById("langUp", (len == 1) && selected.previousSibling, false);
}

function ApplyLangWeb() {
	setCharPref("intl.accept_languages", WriteActiveLanguages());
	ReadActiveLanguages();
	EnableElementById("langApply", false, false);
}
