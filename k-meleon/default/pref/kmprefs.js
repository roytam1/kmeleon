/********************************************************************************************************/
// Default pref for k-meleon - only necessary for XUL pref panel
/********************************************************************************************************/

//pref("kmeleon.display.title", "K-Meleon");			// kmeleon.js
pref("kmeleon.display.URLbarTitle", "URL:");

pref("kmeleon.toolband.Throbber.visibility", true);


/********************************************************************************************************/
// kPlugins

//pref("kmeleon.plugins.bookmarks.load", false);		// kmeleon.js
pref("kmeleon.plugins.bookmarks.askforFolder", true);
//pref("kmeleon.plugins.bookmarks.chevron", false);		// kmeleon.js
pref("kmeleon.plugins.bookmarks.editdialog.width", 500);
pref("kmeleon.plugins.bookmarks.editdialog.height", 500);
pref("kmeleon.plugins.bookmarks.editdialog.left", 50);
pref("kmeleon.plugins.bookmarks.editdialog.top", 50);
pref("kmeleon.plugins.bookmarks.editdialog.maximized", false);
pref("kmeleon.plugins.bookmarks.editdialog.zoom", false);
pref("kmeleon.plugins.bookmarks.maxMenuLength", 20); // plugin default: 20
// variable button width (according to the length of the name),
// button names with more than 99 characters are shortened...
pref("kmeleon.plugins.bookmarks.maxToolbarSize", 100); // plugin default: 20
//pref("kmeleon.plugins.bookmarks.menuAutoDetect", true);	// kmeleon.js
//pref("kmeleon.plugins.bookmarks.openurl", "ID_OPEN_LINK");	// kmeleon.js
pref("kmeleon.plugins.bookmarks.toolbarEnabled", false);

//pref("kmeleon.plugins.favorites.load", false);		// kmeleon.js
// variable button width (according to the length of the name),
// button names with more than 99 characters are shortened...
pref("kmeleon.plugins.favorites.buttonMaxWidth", -100);
pref("kmeleon.plugins.favorites.buttonIcons", true);
pref("kmeleon.plugins.favorites.chevron", false);
//pref("kmeleon.plugins.favorites.menuAutoDetect", true);	// kmeleon.js
//pref("kmeleon.plugins.favorites.openurl", "ID_OPEN_LINK");	// kmeleon.js
pref("kmeleon.plugins.favorites.rebar", false);
pref("kmeleon.plugins.favorites.sortOrder", 17);
pref("kmeleon.plugins.favorites.title", "Links:");
pref("kmeleon.plugins.favorites.toolbarFolder", "Links");

pref("kmeleon.plugins.fullscreen.auto", false);
pref("kmeleon.plugins.fullscreen.hide_rebar", true);
pref("kmeleon.plugins.fullscreen.hide_statusbar", true);
pref("kmeleon.plugins.fullscreen.hide_taskbar", true);

//pref("kmeleon.plugins.hotlist.load", false);			// kmeleon.js
pref("kmeleon.plugins.hotlist.buttonIcons", true);
pref("kmeleon.plugins.hotlist.buttonMaxWidth", 35);
pref("kmeleon.plugins.hotlist.buttonMinWidth", 10);
pref("kmeleon.plugins.hotlist.chevron", false);
pref("kmeleon.plugins.hotlist.editdialog.width", 500);
pref("kmeleon.plugins.hotlist.editdialog.height", 500);
pref("kmeleon.plugins.hotlist.editdialog.left", 50);
pref("kmeleon.plugins.hotlist.editdialog.top", 50);
pref("kmeleon.plugins.hotlist.editdialog.maximized", false);
pref("kmeleon.plugins.hotlist.editdialog.zoom", false);
pref("kmeleon.plugins.hotlist.maxMenuLength", 20); // plugin default: 20
//pref("kmeleon.plugins.hotlist.menuAutoDetect", true);		// kmeleon.js
//pref("kmeleon.plugins.hotlist.openurl", "ID_OPEN_LINK");	// kmeleon.js
pref("kmeleon.plugins.hotlist.rebar", false);
pref("kmeleon.plugins.hotlist.sortOrder", 209);

//pref("kmeleon.plugins.layers.load", true);			// kmeleon.js
pref("kmeleon.plugins.layers.catchOpen", false);
//pref("kmeleon.plugins.layers.confirmClose", true);		// kmeleon.js
pref("kmeleon.plugins.layers.maxWidth", 35);
pref("kmeleon.plugins.layers.minWidth", 10);
pref("kmeleon.plugins.layers.numbers", false);
pref("kmeleon.plugins.layers.onCloseOption", 0);
pref("kmeleon.plugins.layers.onOpenOption", 0);
//pref("kmeleon.plugins.layers.rebar", true);			// kmeleon.js
//pref("kmeleon.plugins.layers.style", 2);			// kmeleon.js
pref("kmeleon.plugins.layers.title", "Layers:");


/********************************************************************************************************/
// Macros

// Domain completion
pref("kmeleon.plugins.macros.domComplete0.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete0.suffix", ".com");
pref("kmeleon.plugins.macros.domComplete1.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete1.suffix", ".net");
pref("kmeleon.plugins.macros.domComplete2.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete2.suffix", ".org");

// Hotlinks
pref("kmeleon.plugins.macros.hotlink0.new", true);
pref("kmeleon.plugins.macros.hotlink1.new", true);
pref("kmeleon.plugins.macros.hotlink2.new", true);
pref("kmeleon.plugins.macros.hotlink3.new", true);
pref("kmeleon.plugins.macros.hotlink4.new", true);
pref("kmeleon.plugins.macros.hotlink5.new", true);
pref("kmeleon.plugins.macros.hotlink6.new", true);
pref("kmeleon.plugins.macros.hotlink7.new", true);
pref("kmeleon.plugins.macros.hotlink8.new", true);
pref("kmeleon.plugins.macros.hotlink9.new", true);
