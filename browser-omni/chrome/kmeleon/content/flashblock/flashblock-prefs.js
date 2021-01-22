var FBlockUtils = {

  /// PREFS FUNCTIONS

  prefs: Components.classes["@mozilla.org/preferences-service;1"]
         .getService(Components.interfaces.nsIPrefBranch)
         .QueryInterface(Components.interfaces.nsIPrefBranchInternal),

  // Returns the value of the flashblock.enabled pref
  isEnabled: function() {
    if(this.prefs.getPrefType("flashblock.enabled") == this.prefs.PREF_BOOL)
      return this.prefs.getBoolPref("flashblock.enabled");
    else {
      this.prefs.setBoolPref("flashblock.enabled", true);
      return true;
    }
  },

  isVideoEnabled : function() {
    return this.prefs.getBoolPref("flashblock.html5video.blocked");
  },

  isSilverlightEnabled : function() {
    return this.prefs.getBoolPref("flashblock.silverlight.blocked");
  },

  isTargetEnabled : function() {
    return this.prefs.getBoolPref("flashblock.whitelist.includeTarget");
  },

  isWeaveEnabled : function() {
    return this.prefs.getBoolPref("services.sync.prefs.sync.flashblock.whitelist");
  },

  // Returns the value of the flashblock.blockLocal pref
  isLocalBlocked: function() {
    if(this.prefs.getPrefType("flashblock.blockLocal") == this.prefs.PREF_BOOL)
      return this.prefs.getBoolPref("flashblock.blockLocal");
    else {
      return false;
    }
  },

  // Returns the value of the javascript.enabled pref
  isJavascriptEnabled:function() {
    return this.prefs.getBoolPref("javascript.enabled");
  },

  // Returns the value of the browser.toolbars.showbutton.flashblockMozToggle pref
  isButtonEnabled: function() {
  	var buttonpref = "browser.toolbars.showbutton.flashblockMozToggle";
    if(this.prefs.getPrefType(buttonpref) == this.prefs.PREF_BOOL)
        return this.prefs.getBoolPref(buttonpref);
    else {
        this.prefs.setBoolPref(buttonpref, true);
        return true;
    }
  },

  // Sets the flashblock.enabled pref to the given boolean value
  setEnabled: function(enabled) {
    return this.prefs.setBoolPref("flashblock.enabled", enabled);
  },

  // Returns the value of the flashblock.whitelist pref
  getWhitelist: function() {
    if(this.prefs.getPrefType("flashblock.whitelist") == this.prefs.PREF_STRING)
	    return this.prefs.getCharPref("flashblock.whitelist");
	else
		return "";
  },

  // Set the flashblock.whitelist pref to the given string
  setWhitelist: function(prefStr) {
    this.prefs.setCharPref("flashblock.whitelist", prefStr);
  }
}
