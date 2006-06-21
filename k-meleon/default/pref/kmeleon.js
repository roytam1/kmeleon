/********************************************************************************************************/
// Default pref for k-meleon 
/********************************************************************************************************/

pref("general.useragent.vendor", "K-Meleon");
pref("general.useragent.vendorSub", "1.0");

/********************************************************************************************************/


/********************************************************************************************************/
// General

pref("kmeleon.MRU.maxURLs", 16);
pref("kmeleon.MRU.behavior", 2);
pref("kmeleon.general.guest_account", false);
pref("kmeleon.general.homePage", "resource://readme.html");
pref("kmeleon.general.offline", false);
pref("kmeleon.general.opengroup", "ID_OPEN_LINK|ID_OPEN_LINK_IN_BACKGROUND");
pref("kmeleon.general.openurl", "ID_OPEN_LINK");
//pref("kmeleon.general.searchEngine", "http://www.google.com/search?q=");	// I10n.js
//pref("kmeleon.general.searchEngineName", "Google");				// I10n.js
pref("kmeleon.general.skinsCurrent", "Phoenity\\");
pref("kmeleon.display.newWindowOpenAs", 0);
pref("kmeleon.display.newWindowURL", "");
pref("kmeleon.general.sourceCommand", "");
pref("kmeleon.general.sourceEnabled", false);

//pref("browser.startup.autoload_homepage",   true);

/********************************************************************************************************/
// Plugins

pref("kmeleon.plugins.bmpmenu.load", true);
pref("kmeleon.plugins.rebarmenu.load", true);
pref("kmeleon.plugins.crash.load", false);
pref("kmeleon.plugins.external.load", true);
pref("kmeleon.plugins.fullscreen.load", true);
pref("kmeleon.plugins.history.load", true);
pref("kmeleon.plugins.toolbars.load", true);

pref("kmeleon.plugins.bookmarks.load", false);
pref("kmeleon.plugins.bookmarks.chevron", true);
pref("kmeleon.plugins.bookmarks.menuAutoDetect", true);
pref("kmeleon.plugins.bookmarks.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.favorites.load", false);
pref("kmeleon.plugins.favorites.menuAutoDetect", true);
pref("kmeleon.plugins.favorites.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.gestures.down", "layers(OpenLinkBg)");
pref("kmeleon.plugins.gestures.downleft", "layers(Last)");
pref("kmeleon.plugins.gestures.downright", "layers(Close)");
pref("kmeleon.plugins.gestures.left", "ID_NAV_BACK");
pref("kmeleon.plugins.gestures.load", true);
pref("kmeleon.plugins.gestures.right", "ID_NAV_FORWARD");
pref("kmeleon.plugins.gestures.up", "macros(up_directory)");
pref("kmeleon.plugins.gestures.upleft", "layers(Prev)");
pref("kmeleon.plugins.gestures.upright", "layers(Next)");

pref("kmeleon.plugins.hotlist.load", false);
pref("kmeleon.plugins.hotlist.menuAutoDetect", true);
pref("kmeleon.plugins.hotlist.openurl", "ID_OPEN_LINK");

pref("kmeleon.plugins.layers.confirmClose", true);
pref("kmeleon.plugins.layers.load", true);
pref("kmeleon.plugins.layers.rebar", true);
pref("kmeleon.plugins.layers.style", 2);

pref("kmeleon.plugins.macros.load", true);
pref("kmeleon.plugins.macros.search.locked", true);
pref("kmeleon.plugins.macros.selected.openurl", "layers(OpenURL)");

pref("kmeleon.plugins.privacy.load", true);
pref("kmeleon.plugins.privacy.clearCache", 0);
pref("kmeleon.plugins.privacy.clearCookies", 0);
pref("kmeleon.plugins.privacy.clearHistory", 0);
pref("kmeleon.plugins.privacy.clearMRU", 0);
pref("kmeleon.plugins.privacy.clearSignOn", 0);


/********************************************************************************************************/
// Appearance

//pref("kmeleon.display.title", "K-Meleon");	// I10n.js
pref("kmeleon.display.NewWindowHasUrlFocus", false);
pref("kmeleon.display.backgroundImage", "");
pref("kmeleon.display.backgroundImageEnabled", true);
pref("kmeleon.display.maximized", false);
pref("kmeleon.display.disableResize", false);
pref("kmeleon.display.backgroundImage", "");
pref("kmeleon.display.backgroundImageEnabled", true);

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
pref("kmeleon.print.paperUnit", 0);
//pref("kmeleon.print.paperWidth", "8,5");
pref("kmeleon.print.scaling", 100);
pref("kmeleon.print.shrinkToFit", true); 


/********************************************************************************************************/
// Privacy

/* I10n.js
pref("kmeleon.privacy.useragent0.name", "K-Meleon 1.0 (Default)");
pref("kmeleon.privacy.useragent0.string", "");
pref("kmeleon.privacy.useragent1.name", "MSIE 6.0");
pref("kmeleon.privacy.useragent1.string", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)");
pref("kmeleon.privacy.useragent2.name", "Netscape 8.1");
pref("kmeleon.privacy.useragent2.string", "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.5) Gecko/20060127 Netscape/8.1");
pref("kmeleon.privacy.useragent3.name", "Opera 7.54");
pref("kmeleon.privacy.useragent3.string", "Opera/7.54 (Windows NT 5.1; U) [en]");
*/
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
pref("kmeleon.download.askOpenSave", true);
pref("kmeleon.download.SaveUnkownContent", true);
pref("kmeleon.download.showMinimizedDialog", false);
pref("kmeleon.download.closeDownloadDialog", false);
pref("kmeleon.download.dir", "");
pref("kmeleon.download.lastDir", "");
pref("kmeleon.download.flashWhenCompleted", false);


/********************************************************************************************************/
// For the tab version only

pref("browser.tabs.autoHide", true);
pref("kmeleon.plugins.tabs.OnDoubleClick", 0);
pref("kmeleon.plugins.tabs.OnMiddleClick", 0);
pref("kmeleon.plugins.tabs.OnRightClick", 2);
pref("kmeleon.plugins.tabs.catchOpen", 1);
pref("kmeleon.plugins.tabs.confirmClose", 0);
pref("kmeleon.plugins.tabs.onCloseLast", 1);
pref("kmeleon.plugins.tabs.onCloseOption", 1);
pref("kmeleon.plugins.tabs.onOpenOption", 0);


/********************************************************************************************************/

pref("browser.cache.memory.capacity", 4096);
pref("browser.cache.disk.capacity", 32768);

pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.autocomplete.enabled", true);

pref("aggreg8.stylesheet.title", "grey");
//pref("intl.charset.detector", "universal_charset_detector");		// I10n.js
//pref("keyword.URL", "http://www.google.com/search?gfns=1&q=");	// I10n.js

pref("kmeleon.find.matchCase", false);
pref("kmeleon.find.searchBackwards", false);
pref("kmeleon.find.wrapAround", false);

pref("general.smoothScroll", false);
pref("general.autoScroll", true);

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

pref("mousewheel.withcontrolkey.action",3);
