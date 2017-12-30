this.EXPORTED_SYMBOLS = ["KMeleon"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Log.jsm");
//this.KMeleon = Components.classes["@kmeleon/jsbridge;1"].getService(Components.interfaces["nsIJSBridge"]);
XPCOMUtils.defineLazyServiceGetter(this, "JsBridge", "@kmeleon/jsbridge;1", "nsIJSBridge");

const LOGGER_ID = "kmKMeleon";
let formatter = new Log.BasicFormatter();
let logger = Log.repository.getLogger(LOGGER_ID);
logger.level = Log.Level.Debug;
logger.addAppender(new Log.ConsoleAppender(formatter));
logger.addAppender(new Log.DumpAppender(formatter));

this.KMeleon = {

  MENU_COMMAND: JsBridge.MENU_COMMAND,
	MENU_POPUP: JsBridge.MENU_POPUP,
	MENU_INLINE: JsBridge.MENU_INLINE,
	MENU_PLUGIN: JsBridge.MENU_PLUGIN,
	MENU_SEPARATOR: JsBridge.MENU_SEPARATOR,

	OPEN_NORMAL: JsBridge.OPEN_NORMAL,
	OPEN_NEW: JsBridge.OPEN_NEW,
	OPEN_BACKGROUND: JsBridge.OPEN_BACKGROUND,
	OPEN_NEWTAB: JsBridge.OPEN_NEWTAB,
	OPEN_BACKGROUNDTAB: JsBridge.OPEN_BACKGROUNDTAB,
	OPEN_CLONE: JsBridge.OPEN_CLONE,


  SetMenu: function(menu, type, label, command, before) {
    JsBridge.SetMenu(menu, type, label, command, before);
  },

  SetMenuCallback: function(menu, label, callback, before) {
    JsBridge.SetMenuCallback(menu, label, callback, before);
  },
  
  SendMessage: function(plugin, to, from, data) {
    return JsBridge.SendMessage(plugin, to, from, data);
  },
  
  /**
  * @return nsIWebBrowser
  */    
  Open: function(url, state) {
    logger.debug("Open "+url);
    return JsBridge.Open(url, state);
  },
  
  /**
  * @return nsIWebBrowser
  */  
  GetActiveBrowser: function() {
    return JsBridge.GetActiveBrowser();
  },
  
  GetLocales: function() {
    let lcls = ["en-US"];
    let dir = this.GetFolder("CurProcD");
    if (!dir) return false;
    
    dir.append("locales");
    if (dir.exists() && dir.isDirectory()) {
      var file,	files = dir.directoryEntries;
      while (files.hasMoreElements()) {
        file = files.getNext();
        if (file instanceof Ci.nsILocalFile && file.isDirectory())
          lcls.push(file.leafName);
      }
    }
    return lcls;
  },
  
  GetSkins: function() {
    let skins = [];
    let dir = this.GetFolder("KUSkins");
		if (dir && dir.exists() && dir.isDirectory()) {
			var file,	files = dir.directoryEntries;
			while (files.hasMoreElements()) {
				file = files.getNext();
				if (file instanceof Ci.nsILocalFile && file.isDirectory()) {
          skins.push(file.leafName);
			  }
	 	  }
	 	}
	 	
	 	dir = this.GetFolder("KASkins");
	 	if (dir && dir.exists() && dir.isDirectory()) {
			var file,	files = dir.directoryEntries;
			while (files.hasMoreElements()) {
				file = files.getNext();
				if (file instanceof Ci.nsILocalFile && file.isDirectory() && file.leafName != 'shared') {
          skins.push(file.leafName);
			  }
	 	  }
	 	}
	 	
	 	return skins.filter(function(item, pos, a) { 
	 	  return a.indexOf(item) == pos; 
	 	}).sort(function alphabetical(x, y) { 
      return x.toLowerCase().localeCompare(y.toLowerCase());
    });
  },
  
  GetFolder: function(prefID) {
    let dir = false;
    try {
      dir = Cc["@mozilla.org/file/directory_service;1"]
        .getService(Ci.nsIProperties)
        .get(prefID, Ci.nsIFile);
    } catch (e) {
      logger.error("GetFolder error: "+prefID+" "+e);
    }

    if (dir && prefID == "CurProcD")
        dir = dir.parent;

    return dir;
  },
  
  
  // Testing
  // Example
  // KMeleon.RegisterCmd("test", "My awesome test", function() {
  //    logger.debug('Yeah');
  // }, "chrome://browser/skin/preferences/Options-sync.png");
  // KMeleon.AddButton("Browser Con&figuration", "test", "&Tools");
  //
  // icon can be an object : {path:'myimage.png', top:16,left:16,right:32,bottom:32}
  
  RegisterCmd: function(name, desc, callback, icon, enabled, checked) {
    return JsBridge.RegisterCmd(name, desc, callback, icon, enabled, checked);
  },
  
  UnregisterCmd: function(name) {
    return JsBridge.UnregisterCmd(name);
  },
  
  SetCmdIcon: function(name, icon) {
    return JsBridge.SetCmdIcon(name, icon);
  },
  
  AddToolbar: function(tbName) {
    return JsBridge.AddToolbar(tbName);
  },
  
  AddButton: function(tbName, command, menu) {
    return JsBridge.AddButton(tbName, command, menu);
  },
  
  RemoveButton: function(tbName, command) {
    return JsBridge.RemoveButton(tbName, command);
  },
  
  GetCmdList: function() {
    return JsBridge.GetCmdList();
  },
  
  AddMenuItem: function(menu, label, command) {
    return JsBridge.SetMenu(menu, KMeleon.MENU_COMMAND, label, command);
  },
  
  RemoveMenuItem: function(menu, command) {
    return JsBridge.SetMenu(menu, KMeleon.MENU_COMMAND, "", command);
  },
  
  GetPlugins: function() {
  },
  
  GetMacros: function() {
  },
  
  GetWindows: function() {
    return JsBridge.GetWindows();
  },
  
  GetCurrentWindow: function() {
    return JsBridge.GetCurrentWindow();
  },
  
  SetAccel: function(key, command) {
    return JsBridge.SetAccel(key, command);
  }
}