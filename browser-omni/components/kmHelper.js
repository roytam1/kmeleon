"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/AddonManager.jsm');
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Log.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "LoginManagerContent", "resource://gre/modules/LoginManagerContent.jsm");

XPCOMUtils.defineLazyServiceGetter(this, 
  "KMeleon", "@kmeleon/jsbridge;1","nsIJSBridge");   
  
XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
  "resource://gre/modules/FormHistory.jsm");

const LOGGER_ID = "kmHelper";
let formatter = new Log.BasicFormatter();
let logger = Log.repository.getLogger(LOGGER_ID);
logger.level = Log.Level.Warn;
logger.addAppender(new Log.ConsoleAppender(formatter));
logger.addAppender(new Log.DumpAppender(formatter));

let bundleServ =
  Components.classes["@mozilla.org/intl/stringbundle;1"].
  getService(Components.interfaces.nsIStringBundleService);
var gHelperBundle =
  bundleServ.createBundle("chrome://kmeleon/locale/kmhelper.properties");   

function kmHelper() {
}

kmHelper.prototype = {
  classID: Components.ID('{6e4995c3-deef-46a2-be73-154647710190}'),
  contractID: "kmeleon/helper;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.kmIHelper, Ci.nsIFormSubmitObserver, Ci.nsIObserver, Ci.nsIDOMEventListener, Ci.nsISupportsWeakReference]),
  _xpcom_categories: [{category: 'app-startup'}],
  addonStarted: false,
  browser: null,
  domWindow: null,
  debug: true,
  enabled: true,
  saveHttpsForms: true,
  formEntries: {},

  /** nsIDOMEventListener **/
  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "DOMContentLoaded": {
        Services.console.logStringMessage('kmAddon: DOMContentLoaded');      
      }
    }
  },

  /** kmIHelper **/
  initLogon: function(browser, win) {
    
    //let chrome = Services.ww.getChromeForWindow(win);
    //var browser = chrome.getWebBrowser();
    this.browser = browser;
    this.domWindow = win;
    Services.obs.addObserver(this, "earlyformsubmit", false);
    Services.prefs.addObserver("browser.formfill.", this, false);    
    
    if (LoginManagerContent.onFormPassword)
      browser.addEventListener("DOMFormHasPassword", function(event) {
        LoginManagerContent.onFormPassword(event);
      });
    if (LoginManagerContent.onContentLoaded)
      browser.addEventListener("DOMContentLoaded", function(event) {
        LoginManagerContent.onContentLoaded(event);
      });
    browser.addEventListener("DOMAutoComplete", function(event) {
      LoginManagerContent.onUsernameInput(event);
    });
    browser.addEventListener("blur", function(event) {
      LoginManagerContent.onUsernameInput(event);
    });
    browser.addEventListener("flashblockCheckLoad", Flashblock.checkLoadFlash, true, true);
    
    return true;
  },

  /** nsIObserver **/
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'nsPref:changed' :
        this.updatePrefs();
        break;
      case 'app-startup' :	
        Services.obs.addObserver(this, 'kmeleon-init', false);
        Services.obs.addObserver(this, "addon-install-blocked", false);    
        Services.obs.addObserver(this, "addon-install-failed", false);  
        Services.obs.addObserver(this, "addon-install-complete", false);
        break;
      case 'kmeleon-init':	
        this.updatePrefs();
        this.initAddon();  
        Services.obs.removeObserver(this, 'kmeleon-init');
        break;
      /*case 'content-document-global-created':	
        //this.initLogon(aSubject);
        if (!this.addonStarted) {
          this.addonStarted = true;
          this.initAddon();
        }
        Services.obs.removeObserver(this, 'content-document-global-created');
        break;*/			
      case 'xpcom-shutdown' :	
        //Services.obs.removeObserver(this, 'content-document-global-created');
        Services.obs.removeObserver(this, "addon-install-blocked");    
        Services.obs.removeObserver(this, "addon-install-failed");  
        Services.obs.removeObserver(this, "addon-install-complete"); 
        break;
      case "addon-install-blocked":
        logger.debug("install blocked");
        var installInfo = aSubject.QueryInterface(Components.interfaces.amIWebInstallInfo);             
        var win = installInfo.originatingWindow;

        var messageString = gHelperBundle.formatStringFromName("xpinstallPromptWarning",
                         ["K-Meleon", installInfo.originatingURI.host], 2);  
        
        /*
        var shell = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIWebNavigation)
                          .QueryInterface(Components.interfaces.nsIDocShell);*/
        if (Services.prompt.confirm(win, "", messageString)) {
          installInfo.install();
        }
        break;
      case "addon-install-failed":
        logger.debug("install failed");
        var installInfo = aSubject.QueryInterface(Components.interfaces.amIWebInstallInfo); 
        for (let install of installInfo.installs) {
          let host = (installInfo.originatingURI instanceof Ci.nsIStandardURL) &&
                    installInfo.originatingURI.host;
          if (!host)
            host = (install.sourceURI instanceof Ci.nsIStandardURL) &&
                  install.sourceURI.host;

          let error = (host || install.error == 0) ? "addonError" : "addonLocalError";
          if (install.error != 0)
            error += install.error;
          else if (install.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
            error += "Blocklisted";
          else
            error += "Incompatible";
         

          messageString = gHelperBundle.GetStringFromName(error);
          messageString = messageString.replace("#1", install.name);
          if (host)
            messageString = messageString.replace("#2", host);
          messageString = messageString.replace("#3", "K-Meleon");
          messageString = messageString.replace("#4", Services.appinfo.version);
          
          Services.prompt.alert(win, "", messageString);
       }
            
        break;
      case "addon-install-complete":
        logger.debug("install complete");
        KMeleon.Open("about:addons", KMeleon.OPEN_NEWTAB);
        break;
    }
  },

  
    /* ---- nsIFormSubmitObserver interfaces ---- */

  notify : function(form, domWin, actionURI, cancelSubmit) {
    try {
        // Even though the global context is for a specific browser, we
        // can receive observer events from other tabs! Ensure this event
        // is about our content.
        
        //logger.debug("Form submit observer notified." + domWin.top + " " + this.domWindow);
        //if (domWin.top != this.domWindow)
        //  return; // Look like useless and don't work on the first tab
                
        let target = domWin.QueryInterface(Ci.nsIDOMEventTarget);
        if (!target)
            return;
        if (!this.enabled)
            return;
            
        logger.debug("Form submit observer notified filtered.");

        //if (PrivateBrowsingUtils.isWindowPrivate(domWin))
        //    return;

        

        if (!this.saveHttpsForms) {
            if (actionURI.schemeIs("https"))
                return;
            if (form.ownerDocument.documentURIObject.schemeIs("https"))
                return;
        }

        if (form.hasAttribute("autocomplete") &&
            form.getAttribute("autocomplete").toLowerCase() == "off")
            return;

        let entries = [];
        for (let i = 0; i < form.elements.length; i++) {
            let input = form.elements[i];
            if (!(input instanceof Ci.nsIDOMHTMLInputElement))
                continue;

            // Only use inputs that hold text values (not including type="password")
            if (!input.mozIsTextField(true))
                continue;

            // Bug 394612: If Login Manager marked this input, don't save it.
            // The login manager will deal with remembering it.

            // Don't save values when autocomplete=off is present.
            if (input.hasAttribute("autocomplete") &&
                input.getAttribute("autocomplete").toLowerCase() == "off")
                continue;

            let value = input.value.trim();

            // Don't save empty or unchanged values.
            if (!value || value == input.defaultValue.trim())
                continue;

            // Don't save credit card numbers.
            if (this.isValidCCNumber(value)) {
                this.log("skipping saving a credit card number");
                continue;
            }

            let name = input.name || input.id;
            if (!name)
                continue;

            if (name == 'searchbar-history') {
                this.log('addEntry for input name "' + name + '" is denied')
                continue;
            }

            // Limit stored data to 200 characters.
            if (name.length > 200 || value.length > 200) {
                this.log("skipping input that has a name/value too large");
                continue;
            }

            // Limit number of fields stored per form.
            if (entries.length >= 100) {
                this.log("not saving any more entries for this form.");
                break;
            }

            entries.push({ name: name, value: value });
        }

        if (entries.length) {
        
            // Ok gecko send this event for each tab open and doesn't even check for
            // duplicate when adding an entrie soooo ........ Trying to limit the problem
            if (this.formEntries.name == entries[0].name && this.formEntries.value == entries[0].value ) return;
            this.formEntries = entries[0];
        
            logger.debug("sending entries to parent process for form " + form.id);
            
            let changes = entries.map(function(entry) {
              return {
                op : "bump",
                fieldname : entry.name,
                value : entry.value,
              }
            });

            FormHistory.update(changes);
        
           /* let messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                         getService(Ci.nsIMessageListenerManager);
                         
            messageManager.sendAsyncMessage("FormHistory:FormSubmitEntries", entries);*/
        }
    }
    catch (e) {
        logger.error("notify failed: " + e);
    }
  },

  /** Private **/
  
  updatePrefs : function () {
      this.debug          = Services.prefs.getBoolPref("browser.formfill.debug");
      this.enabled        = Services.prefs.getBoolPref("browser.formfill.enable");
      this.saveHttpsForms = Services.prefs.getBoolPref("browser.formfill.saveHttpsForms");
      
      let debugLogEnabled = false;
      try {
        debugLogEnabled = Services.prefs.getBoolPref("kmeleon.helper.debug");
      } catch (e) {
      }
      if (debugLogEnabled) {
        logger.level = Log.Level.Debug;
      }
      else {
        logger.level = Log.Level.Warn;
      }
  },
  
  // Implements the Luhn checksum algorithm as described at
  // http://wikipedia.org/wiki/Luhn_algorithm
  isValidCCNumber : function (ccNumber) {
      // Remove dashes and whitespace
      ccNumber = ccNumber.replace(/[\-\s]/g, '');

      let len = ccNumber.length;
      if (len != 9 && len != 15 && len != 16)
          return false;

      if (!/^\d+$/.test(ccNumber))
          return false;

      let total = 0;
      for (let i = 0; i < len; i++) {
          let ch = parseInt(ccNumber[len - i - 1]);
          if (i % 2 == 1) {
              // Double it, add digits together if > 10
              ch *= 2;
              if (ch > 9)
                  ch -= 9;
          }
          total += ch;
      }
      return total % 10 == 0;
  },

  initAddon: function() {
    logger.debug('initAddon');
    
    AddonManager.getAddonsByTypes(["extension"], function(addons) {
      logger.debug('Got addons');
      addons.forEach(function(a) {
        if (a.isActive)
        {
          logger.debug('Loading ' + a.name);
          
          var uri = a.getResourceURI('load.kmm');
          try {
            NetUtil.asyncFetch(uri, function(aInputStream, aResult) {
              // Check that we had success.
              if (!Components.isSuccessCode(aResult)) {
                logger.debug('Not found ' + uri.spec);
              } else {
                var macro = NetUtil.readInputStreamToString(aInputStream, aInputStream.available());
                KMeleon.SendMessage("macros", "kmHelper", "RunMacro", macro);
              }
            });
          } catch (e) {
            logger.debug('Not found ' + uri.spec, e);
          }           
            
          let principal = Cc["@mozilla.org/systemprincipal;1"].
                  createInstance(Ci.nsIPrincipal);        
                  
          uri = a.getResourceURI("kmeleon.js").spec;          
          if (uri) {
            let scope = new Components.utils.Sandbox(principal, {sandboxName: uri});  
            let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].createInstance(Ci.mozIJSSubScriptLoader);
            
            logger.debug('Eval ' + uri);
            
            try {
              scope.__SCRIPT_URI_SPEC__ = uri;
              Components.utils.evalInSandbox(
                  "Components.classes['@mozilla.org/moz/jssubscript-loader;1'] \
                  .createInstance(Components.interfaces.mozIJSSubScriptLoader) \
                  .loadSubScript(__SCRIPT_URI_SPEC__);", scope, "ECMAv5");
            
              scope['startup']();
            } catch (e) {
              logger.debug("Script "+uri, e);
            }                     
            
          }           
        }
      });
    });
  }
};
const NSGetFactory = XPCOMUtils.generateNSGetFactory([kmHelper]);

var Flashblock = {

	whiteList: null,
	
	getFlashblockWhitelist : function() {
		//if (!Flashblock.whiteList)
			Flashblock.loadWhitelist();
		return Flashblock.whiteList;
	},
	
	isTargetEnabled : function() {
	        var prefs = Components.classes["@mozilla.org/preferences-service;1"]
	                              .getService(Components.interfaces.nsIPrefBranch);

	        if(prefs.getPrefType("flashblock.silverlight.blocked") == prefs.PREF_BOOL)
	            return prefs.getBoolPref("flashblock.whitelist.includeTarget");
	        return true;
	    },
	
	checkLoadFlash : function (e) {
		if(
			(e.target &&
				(e.target.ownerDocument && e.target.ownerDocument.location && (Flashblock.checkWhitelist(e.target.ownerDocument.location)) ||
				(Flashblock.isTargetEnabled() && Flashblock.checkWhitelist(Flashblock.getTargetURI(e.target))) ||
				Flashblock.blockedByContentPolicy(e.target)))
			) {
			e.preventDefault();
		}
		e.stopPropagation();
	},
	
	checkWhitelist : function (url) {
		if(!FBlockUtils.isLocalBlocked()) {
			if(url.protocol == "file:")
				return true;
		}

		return this.checkHostInWhitelist(url.host) ||  this.checkHostInWhitelist(url.spec);
	},
	
	checkHostInWhitelist : function (host) {
		if (!host)
			return false;
		var fl = this.getFlashblockWhitelist()
		for (var i = 0; i < fl.length; i++) {
			// Handle *
			var expr = fl[i];
			expr = expr.replace(/\./g, "\\.");
			expr = expr.replace(/\-/g, "\\-");
			expr = expr.replace(/\?/g, "\\?");
			expr = expr.replace(/\+/g, "\\+");
			//expr = expr.replace(/\*/g, "[A-Za-z0-9_\\-\\.]*")
			//expr = expr.replace(/\*/g, "[^ \t\v\n\r\f]*")
			expr = expr.replace(/\*/g, ".*")
			if (expr.slice(-2) != ".*")
				expr = expr + ".*"
			expr = expr + "$"; // "^" + 

			var re = new RegExp(expr);
			if(re.test(host))
				return true;
		}
		return false;
	},
	
	loadWhitelist : function () {
		var flashblockPref = FBlockUtils.getWhitelist();
		Flashblock.whiteList = new Array();
		if (flashblockPref)
		    Flashblock.whiteList = flashblockPref.split(",");
	},
	
	blockedByContentPolicy : function(node) {
		try {
			var uri = this.getTargetURI(node);
			// Ask content policy whether this object is already blocked
			var ios = Components.classes["@mozilla.org/network/io-service;1"]
				.getService(Components.interfaces.nsIIOService);
			var requestOrigin = ios.newURI(node.ownerDocument.location, null, null);
			var contentPolicy = Components.classes["@mozilla.org/layout/content-policy;1"]
				.getService(Components.interfaces.nsIContentPolicy);
			var blockType = contentPolicy.shouldLoad(Components.interfaces.nsIContentPolicy.TYPE_OBJECT,
						uri, requestOrigin, node,
						node.getAttribute("type"), null);
			return blockType != Components.interfaces.nsIContentPolicy.ACCEPT;
		}
		catch (e) {
			Components.utils.reportError(e);
			return false;
		}
	},
	
	getTargetURI : function(node) {
		var targetURI;
		try {
			// Get object URI in the same way as nsObjectLoadingContent::LoadObject()
			var relativeURI;
			switch (node.localName.toLowerCase()) {
				case "object":
					relativeURI = node.getAttribute("data") || node.getAttribute("src") || "";
					if (!relativeURI) {
						var params = node.getElementsByTagName("param");

						for (var ii = 0; ii < params.length; ii++) {
							var name = params[ii].getAttribute("name");
							switch (name) {
								case "movie":
								case "src":
									relativeURI = params[ii].getAttribute("value");
									break;
							}
						}
					}
					break;
				case "embed":
					relativeURI = node.getAttribute("src") || "";
					break;
			}

			var ios = Components.classes["@mozilla.org/network/io-service;1"]
				.getService(Components.interfaces.nsIIOService);
			var baseURI = ios.newURI(node.baseURI, null, null);
			var codeBase = node.getAttribute("codebase");
			if (codeBase) {
				try {
					baseURI = ios.newURI(codeBase, node.ownerDocument.characterSet, baseURI);
				} catch (e) {}  // Ignore invalid codebase attribute
			}
			if (node.ownerDocument)
				targetURI = ios.newURI(relativeURI, node.ownerDocument.characterSet, baseURI);
			else
				targetURI = ios.newURI(relativeURI, null, baseURI);
		}
		catch (e) {
			Components.utils.reportError(e);
		}
		return targetURI;
	}
}

var gFlashblockWhitelist;

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
