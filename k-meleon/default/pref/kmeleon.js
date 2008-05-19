/********************************************************************************************************/
// Default pref for k-meleon 
/********************************************************************************************************/

pref("general.useragent.vendor", "K-Meleon");
pref("general.useragent.vendorSub", "1.5");

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
pref("kmeleon.general.skinsCurrent", "Phoenity");
pref("kmeleon.display.newWindowOpenAs", 0);
pref("kmeleon.display.newWindowURL", "");
pref("kmeleon.general.sourceCommand", "");
pref("kmeleon.general.sourceEnabled", false);

pref("kmeleon.flashblock", false);
pref("kmeleon.adblocking", false);


//pref("browser.startup.autoload_homepage",   true);
pref("browser.startup.homepage", "chrome://navigator-region/locale/region.properties");

/********************************************************************************************************/
// Plugins

pref("kmeleon.plugins.bmpmenu.load", true);
pref("kmeleon.plugins.external.load", true);
pref("kmeleon.plugins.fullscreen.load", true);
pref("kmeleon.plugins.history.load", true);
pref("kmeleon.plugins.jsbridge.load", true);
pref("kmeleon.plugins.rebarmenu.load", true);
pref("kmeleon.plugins.sessions.load", true);
pref("kmeleon.plugins.toolbars.load", true);
pref("kmeleon.plugins.update.load", true);

pref("kmeleon.plugins.bookmarks.load", false);
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
pref("kmeleon.display.backgroundImageEnabled", true);
pref("kmeleon.display.maximized", false);
pref("kmeleon.display.disableResize", false);
pref("kmeleon.display.accelInMenus", true);

pref("kmeleon.favicons.show", true);
pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", false);

//pref("kmeleon.urlbar.dropdown_lines", 10); // To not set by default

/********************************************************************************************************/
// Print

pref("print.show_print_progress", false);
pref("print.use_native_print_dialog", true);
pref("kmeleon.print.BGColors", false);
pref("kmeleon.print.BGImages", false);
pref("kmeleon.print.footerLeft", "&PT");
pref("kmeleon.print.footerMiddle", "");
pref("kmeleon.print.footerRight", "&D");
pref("kmeleon.print.headerLeft", "&T");
pref("kmeleon.print.headerMiddle", "");
pref("kmeleon.print.headerRight", "&U");
//pref("kmeleon.print.marginBottom", "0,5");
//pref("kmeleon.print.marginLeft", "0,5");
//pref("kmeleon.print.marginRight", "0,5");
//pref("kmeleon.print.marginTop", "0,5");
//pref("kmeleon.print.paperHeight", "11,0");
//pref("kmeleon.print.paperUnit", 0);
//pref("kmeleon.print.paperWidth", "8,5");
pref("kmeleon.print.scaling", 100);
pref("kmeleon.print.shrinkToFit", true); 


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

pref("kmeleon.download.useDownloadDir", false);
pref("kmeleon.download.useSaveDir", false);
pref("kmeleon.download.askOpenSave", true);
pref("kmeleon.download.SaveUnkownContent", true);
pref("kmeleon.download.showMinimizedDialog", false);
pref("kmeleon.download.closeDownloadDialog", false);
pref("kmeleon.download.dir", "");
pref("kmeleon.download.lastDir", "");
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
pref("kmeleon.tabs.minWidth", 10);
pref("kmeleon.tabs.maxWidth", 35);
pref("kmeleon.tabs.style", 2);
pref("kmeleon.tabs.fixedBar", true);
pref("kmeleon.tabs.bottomBar", false);



/********************************************************************************************************/

pref("browser.cache.memory.capacity", 4096);
pref("browser.cache.disk.capacity", 32768);

pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.autocomplete.enabled", true);

//pref("intl.charset.detector", "universal_charset_detector");					// I10n.js
//pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&sourceid=navclient&gfns=1&q=");	// I10n.js

pref("kmeleon.find.matchCase", false);
pref("kmeleon.find.searchBackwards", false);
pref("kmeleon.find.wrapAround", false);

pref("general.smoothScroll", false);
pref("general.autoScroll", true);

pref("general.warnOnAboutConfig", true);

//pref("browser.chrome.image_icons.max_size", 1024);

// external link handling in tabbed browsers. values from nsIBrowserDOMWindow.
// 0=default window, 1=current window/tab, 2=new window, 3=new tab in most recent window
pref("browser.link.open_external", 2); // open externally-launched links in a new window

// handle links targeting new windows
//2 (default): In a new window 3: In a new tab 1 (or anything else): In the current window
pref("browser.link.open_newwindow", 2);

// 0: no restrictions - divert everything
// 1: don't divert window.open at all
// 2: don't divert window.open with features
pref("browser.link.open_newwindow.restriction", 2); 

// 0 opens the download manager
// 1 opens a progress dialog
// 2 and other values, no download manager, no progress dialog. 
pref("browser.downloadmanager.behavior", 0);

// Show XUL error pages instead of alerts for errors
pref("browser.xul.error_pages.enabled", true);

// Mouse wheel zoom 
pref("mousewheel.withcontrolkey.action",3);

// Prevent window to steal the focus when loading
pref("dom.disable_window_flip", true);

pref("prefs.converted-to-utf8", true);
pref("wallet.captureForms", false);