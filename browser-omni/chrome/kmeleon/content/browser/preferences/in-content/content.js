/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

var gContentPane = {
  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gContentPane));
    }

    // Initializes the fonts dropdowns displayed in this pane.
    this._rebuildFonts();
    var menulist = document.getElementById("defaultFont");
    if (menulist.selectedIndex == -1) {
      menulist.insertItemAt(0, "", "", "");
      menulist.selectedIndex = 0;
    }

    // Show translation preferences if we may:
    const prefName = "browser.translation.ui.show";
    if (Services.prefs.getBoolPref(prefName)) {
      let row = document.getElementById("translationBox");
      row.removeAttribute("hidden");
    }

    setEventListener("font.language.group", "change",
      gContentPane._rebuildFonts);
    setEventListener("popupPolicyButton", "command",
      gContentPane.showPopupExceptions);
    setEventListener("advancedFonts", "command",
      gContentPane.configureFonts);
    setEventListener("colors", "command",
      gContentPane.configureColors);
    setEventListener("chooseLanguage", "command",
      gContentPane.showLanguages);
    setEventListener("translationAttributionImage", "click",
      gContentPane.openTranslationProviderAttribution);
    setEventListener("translateButton", "command",
      gContentPane.showTranslationExceptions);

    let drmInfoURL =
      Services.urlFormatter.formatURLPref("app.support.baseURL") + "drm-content";
    document.getElementById("playDRMContentLink").setAttribute("href", drmInfoURL);
    let emeUIEnabled = Services.prefs.getBoolPref("browser.eme.ui.enabled");
    // Force-disable/hide on WinXP:
    if (navigator.platform.toLowerCase().startsWith("win")) {
      emeUIEnabled = emeUIEnabled && parseFloat(Services.sysinfo.get("version")) >= 6;
    }
    if (!emeUIEnabled) {
      // Don't want to rely on .hidden for the toplevel groupbox because
      // of the pane hiding/showing code potentially interfering:
      document.getElementById("drmGroup").setAttribute("style", "display: none !important");
    }
  },

  // UTILITY FUNCTIONS

  /**
   * Utility function to enable/disable the button specified by aButtonID based
   * on the value of the Boolean preference specified by aPreferenceID.
   */
  updateButtons: function (aButtonID, aPreferenceID)
  {
    var button = document.getElementById(aButtonID);
    var preference = document.getElementById(aPreferenceID);
    button.disabled = preference.value != true;
    return undefined;
  },

  // BEGIN UI CODE

  /*
   * Preferences:
   *
   * dom.disable_open_during_load
   * - true if popups are blocked by default, false otherwise
   */

  // POP-UPS

  /**
   * Displays the popup exceptions dialog where specific site popup preferences
   * can be set.
   */
  showPopupExceptions: function ()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible: false, sessionVisible: false, allowVisible: true,
                   prefilledHost: "", permissionType: "popup" }
    params.windowTitle = bundlePreferences.getString("popuppermissionstitle");
    params.introText = bundlePreferences.getString("popuppermissionstext");

    gSubDialog.open("chrome://browser/content/preferences/permissions.xul",
                    "Browser:Permissions", "resizable=yes", params);
  },

  showFlashblockExceptions: function ()
  {
    openDialog("chrome://flashblock/content/options.xul", "", "chrome,centerscreen,modal");
  },
  
  // FONTS

  /**
   * Populates the default font list in UI.
   */
  _rebuildFonts: function ()
  {
    var preferences = document.getElementById("contentPreferences");
    // Ensure preferences are "visible" to ensure bindings work.
    preferences.hidden = false;
    // Force flush:
    preferences.clientHeight;
    var langGroupPref = document.getElementById("font.language.group");
    this._selectDefaultLanguageGroup(langGroupPref.value,
                                     this._readDefaultFontTypeForLanguage(langGroupPref.value) == "serif");
  },

  /**
   * 
   */
  _selectDefaultLanguageGroup: function (aLanguageGroup, aIsSerif)
  {
    const kFontNameFmtSerif         = "font.name.serif.%LANG%";
    const kFontNameFmtSansSerif     = "font.name.sans-serif.%LANG%";
    const kFontNameListFmtSerif     = "font.name-list.serif.%LANG%";
    const kFontNameListFmtSansSerif = "font.name-list.sans-serif.%LANG%";
    const kFontSizeFmtVariable      = "font.size.variable.%LANG%";

    var preferences = document.getElementById("contentPreferences");
    var prefs = [{ format   : aIsSerif ? kFontNameFmtSerif : kFontNameFmtSansSerif,
                   type     : "fontname",
                   element  : "defaultFont",
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : aIsSerif ? kFontNameListFmtSerif : kFontNameListFmtSansSerif,
                   type     : "unichar",
                   element  : null,
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : kFontSizeFmtVariable,
                   type     : "int",
                   element  : "defaultFontSize",
                   fonttype : null }];
    for (var i = 0; i < prefs.length; ++i) {
      var preference = document.getElementById(prefs[i].format.replace(/%LANG%/, aLanguageGroup));
      if (!preference) {
        preference = document.createElement("preference");
        var name = prefs[i].format.replace(/%LANG%/, aLanguageGroup);
        preference.id = name;
        preference.setAttribute("name", name);
        preference.setAttribute("type", prefs[i].type);
        preferences.appendChild(preference);
      }

      if (!prefs[i].element)
        continue;

      var element = document.getElementById(prefs[i].element);
      if (element) {
        element.setAttribute("preference", preference.id);

        if (prefs[i].fonttype)
          FontBuilder.buildFontList(aLanguageGroup, prefs[i].fonttype, element);

        preference.setElementValue(element);
      }
    }
  },

  /**
   * Returns the type of the current default font for the language denoted by
   * aLanguageGroup.
   */
  _readDefaultFontTypeForLanguage: function (aLanguageGroup)
  {
    const kDefaultFontType = "font.default.%LANG%";
    var defaultFontTypePref = kDefaultFontType.replace(/%LANG%/, aLanguageGroup);
    var preference = document.getElementById(defaultFontTypePref);
    if (!preference) {
      preference = document.createElement("preference");
      preference.id = defaultFontTypePref;
      preference.setAttribute("name", defaultFontTypePref);
      preference.setAttribute("type", "string");
      preference.setAttribute("onchange", "gContentPane._rebuildFonts();");
      document.getElementById("contentPreferences").appendChild(preference);
    }
    return preference.value;
  },

  /**
   * Displays the fonts dialog, where web page font names and sizes can be
   * configured.
   */  
  configureFonts: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/fonts.xul", "Browser:FontPreferences", "resizable=no");
  },

  /**
   * Displays the colors dialog, where default web page/link/etc. colors can be
   * configured.
   */
  configureColors: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/colors.xul", "Browser:ColorPreferences", "resizable=no");
  },

  // LANGUAGES

  /**
   * Shows a dialog in which the preferred language for web content may be set.
   */
  showLanguages: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/languages.xul", "Browser:LanguagePreferences");
  },

  /**
   * Displays the translation exceptions dialog where specific site and language
   * translation preferences can be set.
   */
  showTranslationExceptions: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/translation.xul", "");
  },

  openTranslationProviderAttribution: function ()
  {
    Components.utils.import("resource:///modules/translation/Translation.jsm");
    Translation.openProviderAttribution();
  },
  
  configureCSSAdblock: function()
  {
    var css_doctype = "";
    var css_cmd = "";
    var wrk = Components.classes["@mozilla.org/windows-registry-key;1"]
      .createInstance(Components.interfaces.nsIWindowsRegKey);
    try {
      wrk.open(wrk.ROOT_KEY_CLASSES_ROOT,
        ".css\\",
        wrk.ACCESS_READ);
      css_doctype = wrk.readStringValue("");
    } catch (e) {
      css_doctype = "";
    }
    if (css_doctype != "") {
      try {
        wrk.open(wrk.ROOT_KEY_CLASSES_ROOT,
          css_doctype + "\\shell\\edit\\command\\",
          wrk.ACCESS_READ);
        css_cmd = wrk.readStringValue("");
      } catch (e) {
        css_cmd = "";
      }
    }
    if (css_cmd == "") {
      try {
        wrk.open(wrk.ROOT_KEY_CLASSES_ROOT,
          css_doctype + "\\shell\\",
          wrk.ACCESS_READ);
        css_cmd = wrk.readStringValue("");
      } catch (e) {
        css_cmd = "";
      }
    }
    wrk.close();
    var adb_file = Cc["@mozilla.org/file/directory_service;1"]
        .getService(Ci.nsIProperties)
        .get("UChrm", Ci.nsIFile);
    adb_file.append("adblock.css");
    var proc = Components.classes["@mozilla.org/process/util;1"]
      .createInstance(Components.interfaces.nsIProcess);

    if (css_cmd != "") {
      css_cmd = css_cmd.replace(' "%1"', '');
      css_cmd = css_cmd.replace('"', '', "g");
      var lFile = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
      var lPath = css_cmd;
      lFile.initWithPath(lPath);
      proc.init(lFile);
    } else {
      var file = Cc["@mozilla.org/file/directory_service;1"]
        .getService(Ci.nsIProperties)
        .get("WinD", Ci.nsIFile);
        file.append("notepad.exe");
      proc.init(file);
      //proc.run(true,[getFile(getFolder("UChrm"),"adblock.css")],1);
    }
    proc.run(false, [adb_file.path], 1);
  }
  
  
};
