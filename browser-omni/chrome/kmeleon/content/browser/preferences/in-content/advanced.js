/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Load DownloadUtils module for convertByteUnits
Components.utils.import("resource://gre/modules/DownloadUtils.jsm");
Components.utils.import("resource://gre/modules/LoadContextInfo.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var gAdvancedPane = {
  _inited: false,

  /**
   * Brings the appropriate tab to the front and initializes various bits of UI.
   */
  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gAdvancedPane));
    }

    this._inited = true;
    var advancedPrefs = document.getElementById("advancedPrefs");

    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    if (preference.value !== null)
        advancedPrefs.selectedIndex = preference.value;

//@line 40 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    this.updateOfflineApps();
//@line 44 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    this.initTelemetry();
//@line 48 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    this.updateActualCacheSize();
    this.updateActualAppCacheSize();

    setEventListener("layers.acceleration.disabled", "change",
                     gAdvancedPane.updateHardwareAcceleration);
    setEventListener("advancedPrefs", "select",
                     gAdvancedPane.tabSelectionChanged);
//@line 63 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    setEventListener("connectionSettings", "command",
                     gAdvancedPane.showConnections);
    setEventListener("clearCacheButton", "command",
                     gAdvancedPane.clearCache);
    setEventListener("clearOfflineAppCacheButton", "command",
                     gAdvancedPane.clearOfflineAppCache);
    setEventListener("offlineNotifyExceptions", "command",
                     gAdvancedPane.showOfflineExceptions);
    setEventListener("offlineAppsList", "select",
                     gAdvancedPane.offlineAppSelected);
    let bundlePrefs = document.getElementById("bundlePreferences");
    document.getElementById("offlineAppsList")
            .style.height = bundlePrefs.getString("offlineAppsList.height");
    setEventListener("offlineAppsListRemove", "command",
                     gAdvancedPane.removeOfflineApp);
//@line 84 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    setEventListener("viewCertificatesButton", "command",
                     gAdvancedPane.showCertificates);
    setEventListener("viewSecurityDevicesButton", "command",
                     gAdvancedPane.showSecurityDevices);

//@line 97 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
  },

  /**
   * Stores the identity of the current tab in preferences so that the selected
   * tab can be persisted between openings of the preferences window.
   */
  tabSelectionChanged: function ()
  {
    if (!this._inited)
      return;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    preference.valueFromPreferences = advancedPrefs.selectedIndex;
  },

  // GENERAL TAB

  /*
   * Preferences:
   *
   * accessibility.browsewithcaret
   * - true enables keyboard navigation and selection within web pages using a
   *   visible caret, false uses normal keyboard navigation with no caret
   * accessibility.typeaheadfind
   * - when set to true, typing outside text areas and input boxes will
   *   automatically start searching for what's typed within the current
   *   document; when set to false, no search action happens
   * general.autoScroll
   * - when set to true, clicking the scroll wheel on the mouse activates a
   *   mouse mode where moving the mouse down scrolls the document downward with
   *   speed correlated with the distance of the cursor from the original
   *   position at which the click occurred (and likewise with movement upward);
   *   if false, this behavior is disabled
   * general.smoothScroll
   * - set to true to enable finer page scrolling than line-by-line on page-up,
   *   page-down, and other such page movements
   * layout.spellcheckDefault
   * - an integer:
   *     0  disables spellchecking
   *     1  enables spellchecking, but only for multiline text fields
   *     2  enables spellchecking for all text fields
   */

  /**
   * Stores the original value of the spellchecking preference to enable proper
   * restoration if unchanged (since we're mapping a tristate onto a checkbox).
   */
  _storedSpellCheck: 0,

  /**
   * Returns true if any spellchecking is enabled and false otherwise, caching
   * the current value to enable proper pref restoration if the checkbox is
   * never changed.
   */
  readCheckSpelling: function ()
  {
    var pref = document.getElementById("layout.spellcheckDefault");
    this._storedSpellCheck = pref.value;

    return (pref.value != 0);
  },

  /**
   * Returns the value of the spellchecking preference represented by UI,
   * preserving the preference's "hidden" value if the preference is
   * unchanged and represents a value not strictly allowed in UI.
   */
  writeCheckSpelling: function ()
  {
    var checkbox = document.getElementById("checkSpelling");
    return checkbox.checked ? (this._storedSpellCheck == 2 ? 2 : 1) : 0;
  },

  /**
   * security.OCSP.enabled is an integer value for legacy reasons.
   * A value of 1 means OCSP is enabled. Any other value means it is disabled.
   */
  readEnableOCSP: function ()
  {
    var preference = document.getElementById("security.OCSP.enabled");
    // This is the case if the preference is the default value.
    if (preference.value === undefined) {
      return true;
    }
    return preference.value == 1;
  },

  /**
   * See documentation for readEnableOCSP.
   */
  writeEnableOCSP: function ()
  {
    var checkbox = document.getElementById("enableOCSP");
    return checkbox.checked ? 1 : 0;
  },

  /**
   * When the user toggles the layers.acceleration.disabled pref,
   * sync its new value to the gfx.direct2d.disabled pref too.
   */
  updateHardwareAcceleration: function()
  {
//@line 200 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
    var fromPref = document.getElementById("layers.acceleration.disabled");
    var toPref = document.getElementById("gfx.direct2d.disabled");
    toPref.value = fromPref.value;
//@line 204 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
  },

  // DATA CHOICES TAB

  /**
   * Set up or hide the Learn More links for various data collection options
   */
  _setupLearnMoreLink: function (pref, element) {
    // set up the Learn More link with the correct URL
    let url = Services.prefs.getCharPref(pref);
    let el = document.getElementById(element);

    if (url) {
      el.setAttribute("href", url);
    } else {
      el.setAttribute("hidden", "true");
    }
  },

  /**
   *
   */
  initSubmitCrashes: function ()
  {
    this._setupLearnMoreLink("toolkit.crashreporter.infoURL",
                             "crashReporterLearnMore");

    var checkbox = document.getElementById("submitCrashesBox");
    try {
      var cr = Components.classes["@mozilla.org/toolkit/crash-reporter;1"].
               getService(Components.interfaces.nsICrashReporter);
      checkbox.checked = cr.submitReports;
    } catch (e) {
      checkbox.style.display = "none";
    }
  },

  /**
   *
   */
  updateSubmitCrashes: function ()
  {
    var checkbox = document.getElementById("submitCrashesBox");
    try {
      var cr = Components.classes["@mozilla.org/toolkit/crash-reporter;1"].
               getService(Components.interfaces.nsICrashReporter);
      cr.submitReports = checkbox.checked;
    } catch (e) { }
  },

  /**
   * The preference/checkbox is configured in XUL.
   *
   * In all cases, set up the Learn More link sanely.
   */
  initTelemetry: function ()
  {
//@line 264 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
  },

  /**
   * Set the status of the telemetry controls based on the input argument.
   * @param {Boolean} aEnabled False disables the controls, true enables them.
   */
  setTelemetrySectionEnabled: function (aEnabled)
  {
//@line 282 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
  },

//@line 326 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"

  // NETWORK TAB

  /*
   * Preferences:
   *
   * browser.cache.disk.capacity
   * - the size of the browser cache in KB
   * - Only used if browser.cache.disk.smart_size.enabled is disabled
   */

  /**
   * Displays a dialog in which proxy settings may be changed.
   */
  showConnections: function ()
  {
    openDialog("chrome://browser/content/preferences/connection.xul",
               "mozilla:connectionmanager",
               "modal=yes",
               null);
  },

  // Retrieves the amount of space currently used by disk cache
  updateActualCacheSize: function ()
  {
    var actualSizeLabel = document.getElementById("actualDiskCacheSize");
    var prefStrBundle = document.getElementById("bundlePreferences");

    // Needs to root the observer since cache service keeps only a weak reference.
    this.observer = {
      onNetworkCacheDiskConsumption: function(consumption) {
        var size = DownloadUtils.convertByteUnits(consumption);
        actualSizeLabel.value = prefStrBundle.getFormattedString("actualDiskCacheSize", size);
      },

      QueryInterface: XPCOMUtils.generateQI([
        Components.interfaces.nsICacheStorageConsumptionObserver,
        Components.interfaces.nsISupportsWeakReference
      ])
    };

    actualSizeLabel.textContent = prefStrBundle.getString("actualDiskCacheSizeCalculated");

    try {
      var cacheService =
        Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                  .getService(Components.interfaces.nsICacheStorageService);
      cacheService.asyncGetDiskConsumption(this.observer);
    } catch (e) {}
  },

  // Retrieves the amount of space currently used by offline cache
  updateActualAppCacheSize: function ()
  {
    var visitor = {
      onCacheStorageInfo: function (aEntryCount, aConsumption, aCapacity, aDiskDirectory)
      {
        var actualSizeLabel = document.getElementById("actualAppCacheSize");
        var sizeStrings = DownloadUtils.convertByteUnits(aConsumption);
        var prefStrBundle = document.getElementById("bundlePreferences");
        var sizeStr = prefStrBundle.getFormattedString("actualAppCacheSize", sizeStrings);
        actualSizeLabel.value = sizeStr;
      }
    };

    try {
      var cacheService =
        Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                  .getService(Components.interfaces.nsICacheStorageService);
      var storage = cacheService.appCacheStorage(LoadContextInfo.default, null);
      storage.asyncVisitStorage(visitor, false);
    } catch (e) {}
  },

  updateCacheSizeUI: function (smartSizeEnabled)
  {
    document.getElementById("useCacheBefore").disabled = smartSizeEnabled;
    document.getElementById("cacheSize").disabled = smartSizeEnabled;
    document.getElementById("useCacheAfter").disabled = smartSizeEnabled;
  },

  readSmartSizeEnabled: function ()
  {
    // The smart_size.enabled preference element is inverted="true", so its
    // value is the opposite of the actual pref value
    var disabled = document.getElementById("browser.cache.disk.smart_size.enabled").value;
    this.updateCacheSizeUI(!disabled);
  },

  /**
   * Converts the cache size from units of KB to units of MB and returns that
   * value.
   */
  readCacheSize: function ()
  {
    var preference = document.getElementById("browser.cache.disk.capacity");
    return preference.value / 1024;
  },
  
  displayMemCache: function()
  {
    var pref = document.getElementById("browser.cache.memory.capacity");
    var choice = document.getElementById("memCacheSizeChoice");
    var text = document.getElementById("memCacheSize");
    if (choice.value == 0) pref.value = -1;
    text.disabled = (choice.value == 0);
  },
 
  readMemCache: function()
  {
    var choice = document.getElementById("memCacheSizeChoice");
    var preference = document.getElementById("browser.cache.memory.capacity");
    var text = document.getElementById("memCacheSize");
    text.disabled = (preference.value == -1);
    if (preference.value == -1) {
        choice.value = 0;
        return "";
    }
    choice.value = 1;
    return preference.value / 1024;
  },
  
  writeMemCache: function()
  {
    var cacheSize = document.getElementById("memCacheSize");
    var intValue = parseInt(cacheSize.value, 10);
    return isNaN(intValue) ? 0 : intValue * 1024;
  },

  /**
   * Converts the cache size as specified in UI (in MB) to KB and returns that
   * value.
   */
  writeCacheSize: function ()
  {
    var cacheSize = document.getElementById("cacheSize");
    var intValue = parseInt(cacheSize.value, 10);
    return isNaN(intValue) ? 0 : intValue * 1024;
  },

  /**
   * Clears the cache.
   */
  clearCache: function ()
  {
    try {
      var cache = Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
                            .getService(Components.interfaces.nsICacheStorageService);
      cache.clear();
    } catch(ex) {}
    this.updateActualCacheSize();
  },

  /**
   * Clears the application cache.
   */
  clearOfflineAppCache: function ()
  {
    Components.utils.import("resource:///modules/offlineAppCache.jsm");
    OfflineAppCacheHelper.clear();

    this.updateActualAppCacheSize();
    this.updateOfflineApps();
  },

  readOfflineNotify: function()
  {
    var pref = document.getElementById("browser.offline-apps.notify");
    var button = document.getElementById("offlineNotifyExceptions");
    button.disabled = !pref.value;
    return pref.value;
  },

  showOfflineExceptions: function()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible     : false,
                   sessionVisible   : false,
                   allowVisible     : false,
                   prefilledHost    : "",
                   permissionType   : "offline-app",
                   manageCapability : Components.interfaces.nsIPermissionManager.DENY_ACTION,
                   windowTitle      : bundlePreferences.getString("offlinepermissionstitle"),
                   introText        : bundlePreferences.getString("offlinepermissionstext") };
    openDialog("chrome://browser/content/preferences/permissions.xul",
               "Browser:Permissions",
               "modal=yes",
               params);
  },

  // XXX: duplicated in browser.js
  _getOfflineAppUsage: function (host, groups)
  {
    var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                       getService(Components.interfaces.nsIApplicationCacheService);
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);

    var usage = 0;
    for (var i = 0; i < groups.length; i++) {
      var uri = ios.newURI(groups[i], null, null);
      if (uri.asciiHost == host) {
        var cache = cacheService.getActiveCache(groups[i]);
        usage += cache.usage;
      }
    }

    return usage;
  },

  /**
   * Updates the list of offline applications
   */
  updateOfflineApps: function ()
  {
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(Components.interfaces.nsIPermissionManager);

    var list = document.getElementById("offlineAppsList");
    while (list.firstChild) {
      list.removeChild(list.firstChild);
    }

    var groups;
    try {
      var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                         getService(Components.interfaces.nsIApplicationCacheService);
      groups = cacheService.getGroups();
    } catch (e) {
      return;
    }

    var bundle = document.getElementById("bundlePreferences");

    var enumerator = pm.enumerator;
    while (enumerator.hasMoreElements()) {
      var perm = enumerator.getNext().QueryInterface(Components.interfaces.nsIPermission);
      if (perm.type == "offline-app" &&
          perm.capability != Components.interfaces.nsIPermissionManager.DEFAULT_ACTION &&
          perm.capability != Components.interfaces.nsIPermissionManager.DENY_ACTION) {
        var row = document.createElement("listitem");
        row.id = "";
        row.className = "offlineapp";
        row.setAttribute("host", perm.host);
        var converted = DownloadUtils.
                        convertByteUnits(this._getOfflineAppUsage(perm.host, groups));
        row.setAttribute("usage",
                         bundle.getFormattedString("offlineAppUsage",
                                                   converted));
        list.appendChild(row);
      }
    }
  },

  offlineAppSelected: function()
  {
    var removeButton = document.getElementById("offlineAppsListRemove");
    var list = document.getElementById("offlineAppsList");
    if (list.selectedItem) {
      removeButton.setAttribute("disabled", "false");
    } else {
      removeButton.setAttribute("disabled", "true");
    }
  },

  removeOfflineApp: function()
  {
    var list = document.getElementById("offlineAppsList");
    var item = list.selectedItem;
    var host = item.getAttribute("host");

    var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Components.interfaces.nsIPromptService);
    var flags = prompts.BUTTON_TITLE_IS_STRING * prompts.BUTTON_POS_0 +
                prompts.BUTTON_TITLE_CANCEL * prompts.BUTTON_POS_1;

    var bundle = document.getElementById("bundlePreferences");
    var title = bundle.getString("offlineAppRemoveTitle");
    var prompt = bundle.getFormattedString("offlineAppRemovePrompt", [host]);
    var confirm = bundle.getString("offlineAppRemoveConfirm");
    var result = prompts.confirmEx(window, title, prompt, flags, confirm,
                                   null, null, null, {});
    if (result != 0)
      return;

    // clear offline cache entries
    try {
      var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                         getService(Components.interfaces.nsIApplicationCacheService);
      var ios = Components.classes["@mozilla.org/network/io-service;1"].
                getService(Components.interfaces.nsIIOService);
      var groups = cacheService.getGroups();
      for (var i = 0; i < groups.length; i++) {
          var uri = ios.newURI(groups[i], null, null);
          if (uri.asciiHost == host) {
              var cache = cacheService.getActiveCache(groups[i]);
              cache.discard();
          }
      }
    } catch (e) {}

    // remove the permission
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(Components.interfaces.nsIPermissionManager);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIPermissionManager.ALLOW_ACTION);
    pm.remove(host, "offline-app",
              Components.interfaces.nsIOfflineCacheUpdateService.ALLOW_NO_WARN);

    list.removeChild(item);
    gAdvancedPane.offlineAppSelected();
    this.updateActualAppCacheSize();
  },

  // UPDATE TAB

  /*
   * Preferences:
   *
   * app.update.enabled
   * - true if updates to the application are enabled, false otherwise
   * extensions.update.enabled
   * - true if updates to extensions and themes are enabled, false otherwise
   * browser.search.update
   * - true if updates to search engines are enabled, false otherwise
   * app.update.auto
   * - true if updates should be automatically downloaded and installed,
   *   possibly with a warning if incompatible extensions are installed (see
   *   app.update.mode); false if the user should be asked what he wants to do
   *   when an update is available
   * app.update.mode
   * - an integer:
   *     0    do not warn if an update will disable extensions or themes
   *     1    warn if an update will disable extensions or themes
   *     2    warn if an update will disable extensions or themes *or* if the
   *          update is a major update
   */

//@line 837 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"

  // ENCRYPTION TAB

  /*
   * Preferences:
   *
   * security.default_personal_cert
   * - a string:
   *     "Select Automatically"   select a certificate automatically when a site
   *                              requests one
   *     "Ask Every Time"         present a dialog to the user so he can select
   *                              the certificate to use on a site which
   *                              requests one
   */

  /**
   * Displays the user's certificates and associated options.
   */
  showCertificates: function ()
  {
    openDialog("chrome://pippki/content/certManager.xul",
               "mozilla:certmanager",
               "modal=yes", null);
  },

  /**
   * Displays a dialog from which the user can manage his security devices.
   */
  showSecurityDevices: function ()
  {
    openDialog("chrome://pippki/content/device_manager.xul",
               "mozilla:devicemanager",
               "modal=yes", null);
  },

//@line 881 "g:\mozilla-esr38\browser\components\preferences\in-content\advanced.js"
};
