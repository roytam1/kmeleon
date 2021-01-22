//@line 38 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8-release/WINNT_5.2_Depend/mozilla/browser/components/preferences/changeaction.js"

var gChangeActionDialog = {
  _item             : null,
  _bundle           : null,
  _lastSelectedMode : null,
  _lastSelectedSave : null,

  init: function ()
  {
    this._item = window.arguments[0];
    this._bundle = document.getElementById("bundlePreferences");
    dump("*** ir = " + this._item.toSource() + "\n");
    
    var typeField = document.getElementById("typeField");
    typeField.value = this._item.typeName;
    
    var extensionField = document.getElementById("extensionField");
    var ext = "." + this._item.extension.toLowerCase();
    var contentType = this._item.type;
    extensionField.value = this._bundle.getFormattedString("extensionStringFormat", [ext, contentType]);
    
    var typeIcon = document.getElementById("typeIcon");
    typeIcon.src = this._item.bigIcon;

    // Custom App Handler Path - this must be set before we set the selected
    // radio button because the selection event handler for the radio group
    // requires the extapp handler field to be non-empty for the extapp radio
    // button to be selected. 
    var customApp = document.getElementById("customApp");
    if (this._item.customHandler)
      customApp.file = this._item.customHandler;
    else
      customApp.file = null;

    var defaultApp = document.getElementById("defaultApp");
    var defaultAppIcon = null;
    var fallbackIconURL = "moz-icon://goat?contentType=" + this._item.type + "&size=16";
    if (this._item.mimeInfo instanceof Components.interfaces.nsIPropertyBag) {
      try {
        defaultAppIcon = this._item.mimeInfo.getProperty("defaultApplicationIconURL");
      }
      catch (e) { }
      if (defaultAppIcon)
        defaultAppIcon += "?size=16";
    }
    defaultApp.image = defaultAppIcon || fallbackIconURL;
    defaultApp.label = this._item.mimeInfo.defaultDescription;

    var pluginName = document.getElementById("pluginName");
    var foundPlugin = false;
    for (var i = 0; i < navigator.plugins.length; ++i) {
      var plugin = navigator.plugins[i];
      for (var j = 0; j < plugin.length; ++j) {
        if (contentType == plugin[j].type) {
          pluginName.label = plugin.name;
          pluginName.image = "moz-icon://goat.goat?contentType=" + contentType + "&size=16";
          foundPlugin = true;
        }
      }
    }
    if (!foundPlugin) {
      pluginName.label = this._bundle.getString("pluginHelperNoneAvailable");
      document.getElementById("plugin").disabled = true;
    }
      
    // Selected Action Radiogroup
    var handlerGroup = document.getElementById("handlerGroup");
    if (this._item.handleMode == FILEACTION_OPEN_PLUGIN && this._item.pluginEnabled)
      handlerGroup.selectedItem = document.getElementById("plugin");
    else {
      if (this._item.handleMode == FILEACTION_OPEN_DEFAULT)
        handlerGroup.selectedItem = document.getElementById("openDefault");
      else if (this._item.handleMode == FILEACTION_SAVE_TO_DISK)
        handlerGroup.selectedItem = document.getElementById("saveToDisk");
      else
        handlerGroup.selectedItem = document.getElementById("openApplication");
    }
    this._lastSelectedMode = handlerGroup.selectedItem;
    
    // Figure out the last selected Save As mode
    var saveToOptions = document.getElementById("saveToOptions");
    this._lastSelectedSave = saveToOptions.selectedItem;

    // We don't let users open .exe files or random binary data directly 
    // from the browser at the moment because of security concerns. 
    var mimeType = this._item.mimeInfo.MIMEType;
    if (mimeType == "application/object-stream" ||
        mimeType == "application/x-msdownload") {
      document.getElementById("openApplication").disabled = true;
      document.getElementById("openDefault").disabled = true;
      handlerGroup.selectedItem = document.getElementById("saveToDisk");
    }
  },
  
  onAccept: function ()
  {
    var contentType = this._item.mimeInfo.MIMEType;
    var handlerGroup = document.getElementById("handlerGroup");
    switch (handlerGroup.selectedItem.value) {
    case "plugin":
      this._item.handleMode = FILEACTION_OPEN_PLUGIN;
      var pluginName = document.getElementById("pluginName");
      this._item.action = this._bundle.getFormattedString("openWith", [pluginName.label]);
      this._item.pluginEnabled = true;
      break;
    case "system":
      this._item.handledOnlyByPlugin = false;
      this._item.handleMode = FILEACTION_OPEN_DEFAULT;
      var defaultDescr = this._item.mimeInfo.defaultDescription;
      this._item.action = this._bundle.getFormattedString("openWith", [defaultDescr]);
      break;
    case "app":
      this._item.handledOnlyByPlugin = false;
      this._item.handleMode = FILEACTION_OPEN_CUSTOM;
      var customApp = document.getElementById("customApp");
      this._item.action = this._bundle.getFormattedString("openWith", [customApp.label]);        
      break;  
    case "save":
      this._item.handledOnlyByPlugin = false;
      this._item.handleMode = FILEACTION_SAVE_TO_DISK;
      this._item.action = this._bundle.getString("saveToDisk");
      break;  
    }
    
    // The opener uses the modifications to the FileAction item to update the
    // datasource.
    return true;
  },
  
  doEnabling: function (aSelectedItem)
  {
    var defaultApp            = document.getElementById("defaultApp");
    var saveToDefault         = document.getElementById("saveToDefault");
    var saveToCustom          = document.getElementById("saveToCustom");
    var customDownloadFolder  = document.getElementById("customDownloadFolder");
    var chooseCustomDownloadFolder = document.getElementById("chooseCustomDownloadFolder");
    var saveToAskMe           = document.getElementById("saveToAskMe");
    var pluginName            = document.getElementById("pluginName");
    var changeApp             = document.getElementById("changeApp");
    var customApp             = document.getElementById("customApp");
    
    switch (aSelectedItem.id) {
    case "openDefault":
      changeApp.disabled = customApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = pluginName.disabled = true;
      defaultApp.disabled = false;
      break;
    case "openApplication":
      defaultApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = pluginName.disabled = true;
      changeApp.disabled = customApp.disabled = false;
      if (!customApp.file && !this.changeApp()) {
        this._lastSelectedMode.click();
        return;
      }
      break;
    case "saveToDisk":
      changeApp.disabled = customApp.disabled = defaultApp.disabled = pluginName.disabled = true;
      var saveToOptions = document.getElementById("saveToOptions");
      customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = !(saveToOptions.selectedItem.id == "saveToCustom");
      saveToDefault.disabled = saveToCustom.disabled = saveToAskMe.disabled = false;
      break;
    case "plugin":
      changeApp.disabled = customApp.disabled = defaultApp.disabled = saveToDefault.disabled = saveToCustom.disabled = customDownloadFolder.disabled = chooseCustomDownloadFolder.disabled = saveToAskMe.disabled = true;
      pluginName.disabled = false;
      break;
    }
    this._lastSelectedMode = aSelectedItem;
  },
  
  doSaveToDiskEnabling: function (aSelectedItem)
  {
    var isSaveToCustom = aSelectedItem.id == "saveToCustom";
    var customDownloadFolder = document.getElementById("customDownloadFolder");
    var chooseCustomDownloadFolder = document.getElementById("chooseCustomDownloadFolder");
    chooseCustomDownloadFolder.disabled = customDownloadFolder.disabled = !isSaveToCustom;
    
    if (isSaveToCustom && 
        !customDownloadFolder.file && !this.changeCustomFolder()) {
      this._lastSelectedSave.click();
      return;
    }
    this._lastSelectedSave = aSelectedItem;
  },
  
  changeApp: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    var winTitle = this._bundle.getString("fpTitleChooseApp");
    fp.init(window, winTitle, nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterApps);
    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      var customApp = document.getElementById("customApp");
      customApp.file = fp.file;
      this._item.customHandler = fp.file;      
      return true;
    }
    return false;
  },
  
  changeCustomFolder: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);

    // extract the window title
    var winTitle = this._bundle.getString("fpTitleChooseDL");
    fp.init(window, winTitle, nsIFilePicker.modeGetFolder);
    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      var customDownloadFolder = document.getElementById("customDownloadFolder");
      customDownloadFolder.file = fp.file;
      customDownloadFolder.label = fp.file.path;
      return true;
    }
    return false;
  }
};
