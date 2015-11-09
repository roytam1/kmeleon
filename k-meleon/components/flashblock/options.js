var FBlockOptions = {

  onLoad: function() {
    flashblockBundle = document.getElementById("bundle_flashblock");
    document.getElementById("fb_enableCheckbox").checked = FBlockUtils.isEnabled();
    document.getElementById("vb_enableCheckbox").checked = FBlockUtils.isVideoEnabled();
    document.getElementById("sb_enableCheckbox").checked = FBlockUtils.isSilverlightEnabled();
    document.getElementById("fb_includeTarget").checked = FBlockUtils.isTargetEnabled();
    document.getElementById("fb_enableWeave").checked = FBlockUtils.isWeaveEnabled();
    this.updateCheckboxes();
    this.popWhitelist();
    this.getVersion();
  },

  updateState: function() {
    FBlockUtils.setEnabled(document.getElementById("fb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.html5video.blocked",
                                   document.getElementById("vb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.silverlight.blocked",
                                   document.getElementById("sb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.whitelist.includeTarget",
                                   document.getElementById("fb_includeTarget").checked);
    FBlockUtils.prefs.setBoolPref("services.sync.prefs.sync.flashblock.whitelist",
                                   document.getElementById("fb_enableWeave").checked);

    this.updateCheckboxes();
  },

  updateCheckboxes: function() {
    var isJSDisabled = !FBlockUtils.isJavascriptEnabled();
    var pref = document.getElementById("flashblock_enableToggleBtn");
    if (pref) {
      pref.checked = FBlockUtils.isButtonEnabled();
      pref.disabled = isJSDisabled;
    }
    document.getElementById("fb_enableCheckbox").disabled = isJSDisabled;
    document.getElementById("sb_enableCheckbox").disabled = isJSDisabled ||
                                                            !FBlockUtils.isEnabled();
    document.getElementById("vb_enableCheckbox").disabled = isJSDisabled ||
                                                            !FBlockUtils.isEnabled();
    document.getElementById("fb_includeTarget").disabled = isJSDisabled ||
                                                            !FBlockUtils.isEnabled();
    document.getElementById("fb_siteTextbox").disabled = isJSDisabled;
    document.getElementById("fb_sitelist").disabled = isJSDisabled;
    document.getElementById("fb_btnRemoveAll").disabled = isJSDisabled;
  },

  popWhitelist: function() {
    var prefStr = FBlockUtils.getWhitelist();
    var i;
    var siteList = document.getElementById("fb_sitelist");
    while(siteList.getRowCount() > 0)
        siteList.removeItemAt(0);

    if(prefStr) {
      var array = prefStr.split(",");
      array.sort(perDomainComparison);
      for (var i = 0; i < array.length; i++) {
        siteList.ensureElementIsVisible(siteList.appendItem(array[i]));
      }
    }
  },

  getVersion: function() {
    if ("@mozilla.org/extensions/manager;1" in Components.classes) {
      var flashblockID = "{3d7eb24f-2740-49df-8937-200b1cc08f8a}";
      var em = Components.classes["@mozilla.org/extensions/manager;1"]
                         .getService(Components.interfaces.nsIExtensionManager);
      if (!("getItemForID" in em))
        return;
      var version = em.getItemForID(flashblockID).version;
      var display = document.getElementById("flashblockVersion");
      if (display && version)
        display.value = version;
    }
  }

}

function checkSiteName(siteName) {
    //var regex = /^[\w\-\.\*]+(\:\d{1,5}){0,1}(\/[^ \t\v\n\r\f\\]*)*$/; 
    var regex = /^[\w\-\.\*]+(\:\d{1,5}){0,1}\/*.*/; 
    return regex.test(siteName);
}

function listContainsSite(site) {
    var siteList = document.getElementById("fb_sitelist");
    var numRows = siteList.getRowCount();
    var i;
    for(i = 0; i < numRows; i++) {
        if(siteList.getItemAtIndex(i).label == site)
            return true;
    }
    return false;
}

function listFindIndex(site) {
    var siteList = document.getElementById("fb_sitelist");
    var numRows = siteList.getRowCount();
    var i;
    for(i = 0; i < numRows; i++) {
        if(perDomainComparison(siteList.getItemAtIndex(i).label, site) > 0)
            break;
    }
    return i;
}

// To sort the whitelist array.
function perDomainComparison(host1, host2) {
    function min(a, b) {
        return (a < b) ? a : b;
    }
    // Split on domain boundaries and reverse.
    components1 = host1.split(".").reverse();
    components2 = host2.split(".").reverse();
    // Compare element by element until a difference is found
    for (i = 0; i < min(components1.length, components2.length); i++) {
        if (components1[i] < components2[i])
            return -1;
        if (components1[i] > components2[i])
            return 1;
    }
    // If all components so far are the same, the shorter name comes first
    if (components1.length > components2.length)
        return 1;
    if (components2.length > components1.length)
        return -1;
    return 0;
}

function addOnKeypress(event) {
    if(event && event.type == "keypress" && event.keyCode != KeyEvent.DOM_VK_RETURN)
      return;
    event.preventDefault();
    addSite();
}

function addSite() {
    var textbox = document.getElementById("fb_siteTextbox");
    var siteName = textbox.value;

    if(siteName.length == 0)
        return false;

    if(! checkSiteName(siteName)) {
        var msg = flashblockBundle.getString("invalidCharsInSiteName");
        alert(msg);
        return false;
    }

    var siteList = document.getElementById("fb_sitelist");
    if(! listContainsSite(siteName)) {
        var numRows = siteList.getRowCount();
        var index = listFindIndex(siteName);

        if(index < numRows)
            newElement = siteList.insertItemAt(index, siteName, "");
        else
            newElement = siteList.appendItem(siteName, "");

        //use ensureElementIsVisible due to mozilla bug id 250123
        siteList.ensureElementIsVisible(newElement);
    }
    textbox.value = "";
    siteInput(textbox);
    textbox.focus();
    return true;
}

function removeSite() {
    var siteList = document.getElementById("fb_sitelist");
    var index = siteList.selectedIndex;
    if(index != -1) {
        siteList.removeItemAt(index);
        document.getElementById("fb_btnRemove").disabled = true;
    }
}

function removeAllSites() {
    var siteList = document.getElementById("fb_sitelist");
    var msg = flashblockBundle.getString("confirmClearWhitelist");

    if(confirm(msg)) {
        while(siteList.getRowCount() > 0)
            siteList.removeItemAt(0);
    }
}

function doOK() {
    var prefStr = sitelistToString();
    FBlockUtils.setWhitelist(prefStr);

    FBlockUtils.setEnabled(document.getElementById("fb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.html5video.blocked",
                                   document.getElementById("vb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.silverlight.blocked",
                                   document.getElementById("sb_enableCheckbox").checked);
    FBlockUtils.prefs.setBoolPref("flashblock.whitelist.includeTarget",
                                   document.getElementById("fb_includeTarget").checked);
    FBlockUtils.prefs.setBoolPref("services.sync.prefs.sync.flashblock.whitelist",
                                   document.getElementById("fb_enableWeave").checked);
    window.close();
}

function doCancel() {
    window.close();
}

function siteInput(siteField) {
    document.getElementById("btnAdd").disabled = !siteField.value;
}

function siteSelected(siteList) {
    document.getElementById("fb_btnRemove").disabled = (siteList.selectedIndex == -1);
}

function sitelistToString() {
    var siteList = document.getElementById("fb_sitelist");
    var numRows = siteList.getRowCount();
    var prefStr = "";
    var i;
    for(i = 0; i < numRows; i++) {
        if(i != 0) prefStr += ",";
        prefStr += siteList.getItemAtIndex(i).label;
    }
    return prefStr;
}

var flashblockBundle;
