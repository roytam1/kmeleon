
Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import('resource://gre/modules/Services.jsm');

function startup() {
  var KMeleon = Components.classes["@kmeleon/jsbridge;1"].getService(Components.interfaces["nsIJSBridge"]);
  KMeleon.SetMenuCallback("_Mail_News", "Read &RSS Feeds", function(mode) {
    KMeleon.Open('chrome://newsfox/content/newsfox.xul', KMeleon.OPEN_NEWTAB);
  });
}