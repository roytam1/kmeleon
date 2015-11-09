Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function KLoginManager() { }

KLoginManager.prototype = {
  //classDescription: "My Hello World Javascript XPCOM Component",
  classID:          Components.ID("{121F4C54-619C-11E3-A003-FF636188709B}"),
  contractID:       "kmeleonbrowser.org/login;1",
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIKLoginManager]),
  init: function(browser) { 
	return false;
  }
};

var components = [KLoginManager];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);  