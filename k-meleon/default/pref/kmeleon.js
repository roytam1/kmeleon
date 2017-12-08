/********************************************************************************************************/
// Default pref for k-meleon 
/********************************************************************************************************/



/********************************************************************************************************/
// General

pref("kmeleon.MRU.maxURLs", 16);
pref("kmeleon.MRU.behavior", 2);
pref("kmeleon.general.guest_account", false);
pref("kmeleon.general.offline", false);
pref("kmeleon.general.opengroup", "ID_OPEN_LINK_IN_NEW_TAB|ID_OPEN_LINK_IN_BACKGROUNDTAB");	// Tabs ON
pref("kmeleon.general.openurl", "ID_OPEN_LINK");
//pref("kmeleon.general.searchEngine", "http://www.google.com/search?q=");	// I10n.js
//pref("kmeleon.general.searchEngineName", "Google");				// I10n.js
pref("kmeleon.general.skinsCurrent", "Default");
pref("kmeleon.display.newWindowOpenAs", 0);
pref("kmeleon.display.newWindowURL", "");
pref("kmeleon.general.sourceCommand", "");
pref("kmeleon.general.sourceEnabled", false);

pref("kmeleon.flashblock", false);
pref("kmeleon.adblocking", false);


//pref("browser.startup.autoload_homepage",   true);
pref("browser.startup.homepage", "about:home");
pref("browser.sessionstore.restore_on_demand", true);

/********************************************************************************************************/
// Plugins

pref("kmeleon.plugins.bmpmenu.load", true);
pref("kmeleon.plugins.fullscreen.load", true);
pref("kmeleon.plugins.history.load", false);
pref("kmeleon.plugins.jsbridge.load", true);
pref("kmeleon.plugins.rebarmenu.load", true);
pref("kmeleon.plugins.sessions.load", true);
pref("kmeleon.plugins.toolbars.load", true);
pref("kmeleon.plugins.update.load", true);
pref("kmeleon.plugins.crashrpt.load", true);
pref("kmeleon.plugins.login.load", true);
pref("kmeleon.plugins.spellcheck.load", true);

pref("kmeleon.plugins.bookmarks.load", true);
pref("kmeleon.plugins.bookmarks.chevron", true);
pref("kmeleon.plugins.bookmarks.menuAutoDetect", true);
pref("kmeleon.plugins.bookmarks.openurl", "ID_OPEN_LINK");
pref("kmeleon.plugins.bookmarks.openurlm", "ID_OPEN_LINK_IN_NEW_TAB");
pref("kmeleon.plugins.bookmarks.openurlr", "ID_OPEN_LINK_IN_BACKGROUNDTAB");

pref("kmeleon.plugins.favorites.load", false);
pref("kmeleon.plugins.favorites.menuAutoDetect", true);
pref("kmeleon.plugins.favorites.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.hotlist.load", false);
pref("kmeleon.plugins.hotlist.menuAutoDetect", true);
pref("kmeleon.plugins.hotlist.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.gestures.load", false);
pref("kmeleon.plugins.gestures.down", "ID_OPEN_LINK_IN_BACKGROUNDTAB");
pref("kmeleon.plugins.gestures.downleft", "ID_TAB_LAST");
pref("kmeleon.plugins.gestures.downright", "ID_CLOSE_TAB");
pref("kmeleon.plugins.gestures.left", "ID_NAV_BACK");
pref("kmeleon.plugins.gestures.right", "ID_NAV_FORWARD");
pref("kmeleon.plugins.gestures.up", "ID_OPEN_LINK_IN_NEW_TAB");
pref("kmeleon.plugins.gestures.upleft", "ID_TAB_PREV");
pref("kmeleon.plugins.gestures.upright", "ID_TAB_NEXT");
pref("kmeleon.plugins.gestures.SDLink", "ID_OPEN_LINK_IN_BACKGROUNDTAB");
pref("kmeleon.plugins.gestures.SDImage", "viewImage");
pref("kmeleon.plugins.gestures.SDText", "ID_NAV_SEARCH");

pref("kmeleon.plugins.layers.load", false);

pref("kmeleon.plugins.macros.load", true);
pref("kmeleon.plugins.macros.search.locked", true);
pref("kmeleon.plugins.macros.selected.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.privacy.load", true);
pref("kmeleon.plugins.privacy.clearCache", 0);
pref("kmeleon.plugins.privacy.clearCookies", 0);
pref("kmeleon.plugins.privacy.clearHistory", 0);
pref("kmeleon.plugins.privacy.clearMRU", 0);
pref("kmeleon.plugins.privacy.clearSignOn", 0);

/********************************************************************************************************/
// Appearance

pref("kmeleon.display.title", "K-Meleon");	
pref("kmeleon.display.NewWindowHasUrlFocus", false);
pref("kmeleon.display.backgroundImage", "");
pref("kmeleon.display.backgroundImageEnabled", false);
pref("kmeleon.display.maximized", false);
pref("kmeleon.display.disableResize", false);
pref("kmeleon.display.accelInMenus", true);
pref("kmeleon.display.hideTitleBar", false);
pref("kmeleon.display.toolbars_line", true);
pref("kmeleon.favicons.show", true);
pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", true);


//pref("kmeleon.urlbar.dropdown_lines", 10); // To not set by default

/********************************************************************************************************/
// Print

pref("print.show_print_progress", false);
pref("print.use_native_print_dialog", true);


/********************************************************************************************************/
// Privacy

pref("signon.rememberSignons", true);
pref("capability.policy.restrictedpopups.Window.open", "allAccess");
pref("capability.policy.restrictedpopups.sites", "");
pref("dom.disable_open_during_load", true);
pref("kmeleon.favicons.cached", true);

 //Determines how the browser should handle  cookies.
//0: Enable all cookies 1: Allow cookies from originating server only 2: Disable all cookies 3: Use P3P policy to decide
pref("network.cookie.cookieBehavior", 1);


/********************************************************************************************************/
// Download

pref("kmeleon.download.saveType", 0);
pref("kmeleon.download.saveDir", "");
pref("kmeleon.download.saveUseTitle", true);

pref("browser.download.useDownloadDir", false);
pref("kmeleon.download.useSaveDir", false);
pref("kmeleon.download.askOpenSave", true);
pref("kmeleon.download.SaveUnkownContent", true);
pref("kmeleon.download.showMinimizedDialog", false);
pref("kmeleon.download.closeDownloadDialog", false);
pref("browser.download.dir", "");
pref("browser.download.lastDir", "");
pref("kmeleon.download.flashWhenCompleted", false);


/********************************************************************************************************/
// For the tab version only

pref("browser.tabs.autoHide", false);		// classic behavior
pref("browser.tabs.warnOnClose", true);
pref("kmeleon.tabs.OnDoubleClick", 3);		// classic behavior
pref("kmeleon.tabs.OnMiddleClick", 3);		// classic behavior
pref("kmeleon.tabs.OnRightClick", 2);
pref("kmeleon.tabs.onCloseLast", 1);
pref("kmeleon.tabs.onCloseOption", 1);
pref("kmeleon.tabs.onOpenOption", 0);
pref("kmeleon.tabs.useLoadingTitle", false);
pref("kmeleon.notab", false);
pref("kmeleon.tabs.title", "");
pref("kmeleon.tabs.minWidth", 6);
pref("kmeleon.tabs.maxWidth", 60);
pref("kmeleon.tabs.style", 2);
pref("kmeleon.tabs.fixedBar", false); 
pref("kmeleon.tabs.position", "band");




/********************************************************************************************************/

pref("browser.cache.memory.capacity", -1);
pref("browser.cache.disk.capacity", 131072);
pref("browser.cache.disk.max_entry_size", 4096);
pref("browser.cache.disk.smart_size.enabled", false);
pref("browser.cache.disk.smart_size.first_run", false);

pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.autocomplete.enabled", true);

pref("keyword.enabled", true);
pref("kmeleon.find.matchCase", false);
pref("kmeleon.find.searchBackwards", false);
pref("kmeleon.find.wrapAround", true);

pref("general.smoothScroll", false);
pref("general.autoScroll", true);

//pref("browser.chrome.image_icons.max_size", 1024);

// external link handling in tabbed browsers. values from nsIBrowserDOMWindow.
// 0=default window, 1=current window/tab, 2=new window, 3=new tab in most recent window
pref("browser.link.open_external", 3); 

// handle links targeting new windows
//2 (default): In a new window 3: In a new tab 1 (or anything else): In the current window
pref("browser.link.open_newwindow", 2);

// 0: no restrictions - divert everything
// 1: don't divert window.open at all
// 2: don't divert window.open with features
pref("browser.link.open_newwindow.restriction", 2); 

// Show XUL error pages instead of alerts for errors
pref("browser.xul.error_pages.enabled", true);





// Flashblock
pref("flashblock.html5video.blocked", false);
pref("flashblock.silverlight.blocked", false);
pref("flashblock.whitelist.includeTarget", false);
pref("flashblock.blockLocal", false);
pref("services.sync.prefs.sync.flashblock.whitelist", false);



// History
pref("browser.tabs.loadBookmarksInBackground", false);
pref("browser.tabs.warnOnOpen", true);
pref("browser.tabs.maxOpenBeforeWarn", 15);
pref('places.history.enabled', true);

// Others
pref('app.update.enabled', false); 
pref('extensions.console2.max-errors', 1000);
pref("browser.zoom.full", true);
pref("full-screen-api.enabled", true);

// Prevent window to steal the focus when loading
pref("dom.disable_window_flip", true);

pref("dom.ipc.plugins.enabled", false);
pref("dom.ipc.plugins.enabled.npswf32.dll", true);
pref("plugins.load_appdir_plugins", true);
pref("accessibility.typeaheadfind.flashBar", 1);

pref("general.useragent.compatMode.firefox", true);
pref("layout.spellcheckDefault", 0);

// for performance !
pref("consoleservice.enabled", false);

// extensions
pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
pref("extensions.getAddons.cache.enabled", true);
pref("xpinstall.enabled", true);
pref("xpinstall.whitelist.add", "addons.mozilla.org");
pref("extensions.blocklist.enabled", true);
pref("extensions.logging.enabled", false);
pref("extensions.strictCompatibility", false);
pref("extensions.update.autoUpdateDefault", false);
pref("general.skins.selectedSkin", "classic/1.0");
//pref("plugins.click_to_play", true);

// preferences
pref("browser.preferences.instantApply", false);
pref("browser.translation.ui.show", false);
pref("app.support.baseURL", "https://support.mozilla.org/1/firefox/%VERSION%/%OS%/%LOCALE%/");
pref("browser.search.showOneOffButtons", true);
pref("pdfjs.disabled", true);
pref("browser.eme.ui.enabled", true);

// Others
pref("security.csp.speccompliant", true);
pref("places.favicons.optimizeToDimension", 32);