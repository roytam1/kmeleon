//@line 5 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"

// Services = object with smart getters for common XPCOM services
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
//Components.utils.import("resource:///modules/RecentWindow.jsm");
Components.utils.import("resource:///modules/KMeleon.jsm");

XPCOMUtils.defineLazyGetter(this, "BROWSER_NEW_TAB_URL", function () {
  const PREF = "browser.newtab.url";

  function getNewTabPageURL() {
    if (!Services.prefs.prefHasUserValue(PREF)) {
      if (PrivateBrowsingUtils.isWindowPrivate(window) &&
          !PrivateBrowsingUtils.permanentPrivateBrowsing)
        return "about:privatebrowsing";
    }
    let url = Services.prefs.getComplexValue(PREF, Ci.nsISupportsString).data;
    return url || "about:blank";
  }

  function update() {
    BROWSER_NEW_TAB_URL = getNewTabPageURL();
  }

  Services.prefs.addObserver(PREF, update, false);

  addEventListener("unload", function onUnload() {
    removeEventListener("unload", onUnload);
    Services.prefs.removeObserver(PREF, update);
  });

  return getNewTabPageURL();
});

var TAB_DROP_TYPE = "application/x-moz-tabbrowser-tab";

var gBidiUI = false;

/**
 * Determines whether the given url is considered a special URL for new tabs.
 */
function isBlankPageURL(aURL) {
  return aURL == "about:blank" || aURL == BROWSER_NEW_TAB_URL;
}

function getBrowserURL()
{
  return "chrome://browser/content/browser.xul";
}

function getTopWin(skipPopups) {
  // If this is called in a browser window, use that window regardless of
  // whether it's the frontmost window, since commands can be executed in
  // background windows (bug 626148).
  if (top.document.documentElement.getAttribute("windowtype") == "navigator:browser" &&
      (!skipPopups || top.toolbar.visible))
    return top;

  let isPrivate = PrivateBrowsingUtils.isWindowPrivate(window);
  return RecentWindow.getMostRecentBrowserWindow({private: isPrivate,
                                                  allowPopups: !skipPopups});
}

function openTopWin(url) {
  /* deprecated */
  openUILinkIn(url, "current");
}

function getBoolPref(prefname, def)
{
  try {
    return Services.prefs.getBoolPref(prefname);
  }
  catch(er) {
    return def;
  }
}

/* openUILink handles clicks on UI elements that cause URLs to load.
 *
 * As the third argument, you may pass an object with the same properties as
 * accepted by openUILinkIn, plus "ignoreButton" and "ignoreAlt".
 */
function openUILink(url, event, aIgnoreButton, aIgnoreAlt, aAllowThirdPartyFixup,
                    aPostData, aReferrerURI) {
  let params;

  if (aIgnoreButton && typeof aIgnoreButton == "object") {
    params = aIgnoreButton;

    // don't forward "ignoreButton" and "ignoreAlt" to openUILinkIn
    aIgnoreButton = params.ignoreButton;
    aIgnoreAlt = params.ignoreAlt;
    delete params.ignoreButton;
    delete params.ignoreAlt;
  } else {
    params = {
      allowThirdPartyFixup: aAllowThirdPartyFixup,
      postData: aPostData,
      referrerURI: aReferrerURI,
      referrerPolicy: Components.interfaces.nsIHttpChannel.REFERRER_POLICY_DEFAULT,
      initiatingDoc: event ? event.target.ownerDocument : null,
    };
  }

  let where = whereToOpenLink(event, aIgnoreButton, aIgnoreAlt);
  openUILinkIn(url, where, params);
}


/* whereToOpenLink() looks at an event to decide where to open a link.
 *
 * The event may be a mouse event (click, double-click, middle-click) or keypress event (enter).
 *
 * On Windows, the modifiers are:
 * Ctrl        new tab, selected
 * Shift       new window
 * Ctrl+Shift  new tab, in background
 * Alt         save
 *
 * Middle-clicking is the same as Ctrl+clicking (it opens a new tab).
 *
 * Exceptions: 
 * - Alt is ignored for menu items selected using the keyboard so you don't accidentally save stuff.  
 *    (Currently, the Alt isn't sent here at all for menu items, but that will change in bug 126189.)
 * - Alt is hard to use in context menus, because pressing Alt closes the menu.
 * - Alt can't be used on the bookmarks toolbar because Alt is used for "treat this as something draggable".
 * - The button is ignored for the middle-click-paste-URL feature, since it's always a middle-click.
 */
function whereToOpenLink( e, ignoreButton, ignoreAlt )
{
  // This method must treat a null event like a left click without modifier keys (i.e.
  // e = { shiftKey:false, ctrlKey:false, metaKey:false, altKey:false, button:0 })
  // for compatibility purposes.
  if (!e)
    return "current";

  var shift = e.shiftKey;
  var ctrl =  e.ctrlKey;
  var meta =  e.metaKey;
  var alt  =  e.altKey && !ignoreAlt;

  // ignoreButton allows "middle-click paste" to use function without always opening in a new window.
  var middle = !ignoreButton && e.button == 1;
  var middleUsesTabs = getBoolPref("browser.tabs.opentabfor.middleclick", true);

  // Don't do anything special with right-mouse clicks.  They're probably clicks on context menu items.

//@line 155 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"
  if (ctrl || (middle && middleUsesTabs))
//@line 157 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"
    return shift ? "tabshifted" : "tab";

  if (alt && getBoolPref("browser.altClickSave", false))
    return "save";

  if (shift || (middle && !middleUsesTabs))
    return "window";

  return "current";
}

/* openUILinkIn opens a URL in a place specified by the parameter |where|.
 *
 * |where| can be:
 *  "current"     current tab            (if there aren't any browser windows, then in a new window instead)
 *  "tab"         new tab                (if there aren't any browser windows, then in a new window instead)
 *  "tabshifted"  same as "tab" but in background if default is to select new tabs, and vice versa
 *  "window"      new window
 *  "save"        save to disk (with no filename hint!)
 *
 * aAllowThirdPartyFixup controls whether third party services such as Google's
 * I Feel Lucky are allowed to interpret this URL. This parameter may be
 * undefined, which is treated as false.
 *
 * Instead of aAllowThirdPartyFixup, you may also pass an object with any of
 * these properties:
 *   allowThirdPartyFixup (boolean)
 *   postData             (nsIInputStream)
 *   referrerURI          (nsIURI)
 *   relatedToCurrent     (boolean)
 */
function openUILinkIn(url, where, aAllowThirdPartyFixup, aPostData, aReferrerURI) {
  var params;

  if (arguments.length == 3 && typeof arguments[2] == "object") {
    params = aAllowThirdPartyFixup;
  } else {
    params = {
      allowThirdPartyFixup: aAllowThirdPartyFixup,
      postData: aPostData,
      referrerURI: aReferrerURI
    };
  }

  params.fromChrome = true;

  openLinkIn(url, where, params);
}

function openLinkIn(url, where, params) {
  if (!where || !url)
    return;

  var aFromChrome           = params.fromChrome;
  var aAllowThirdPartyFixup = params.allowThirdPartyFixup;
  var aPostData             = params.postData;
  var aCharset              = params.charset;
  var aReferrerURI          = params.referrerURI;
  var aRelatedToCurrent     = params.relatedToCurrent;
  var aInBackground         = params.inBackground;
  var aDisallowInheritPrincipal = params.disallowInheritPrincipal;
  // Currently, this parameter works only for where=="tab" or "current"
  var aIsUTF8               = params.isUTF8;
  var aInitiatingDoc        = params.initiatingDoc;
  var aIsPrivate            = params.private;

  if (where == "save") {
    if (!aInitiatingDoc) {
      Components.utils.reportError("openUILink/openLinkIn was called with " +
        "where == 'save' but without initiatingDoc.  See bug 814264.");
      return;
    }
    saveURL(url, null, null, true, null, aReferrerURI, aInitiatingDoc);
    return;
  }
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  
  /**   K-Meleon **********************/
  var w;
  if (where == "tab")
    w = KMeleon.Open(url, aInBackground ? KMeleon.OPEN_BACKGROUNDTAB : KMeleon.OPEN_NEWTAB);
  else if (where == "window")
    w = KMeleon.Open(url, aInBackground ? KMeleon.OPEN_BACKGROUND : KMeleon.OPEN_NEW);
  else {
    w = KMeleon.Open(url, KMeleon.OPEN_NORMAL);    
  }
  if (!aInBackground) w.contentDOMWindow.focus();
  return;
  /*************************************/
  
  

}

// Used as an onclick handler for UI elements with link-like behavior.
// e.g. onclick="checkForMiddleClick(this, event);"
function checkForMiddleClick(node, event) {
  // We should be using the disabled property here instead of the attribute,
  // but some elements that this function is used with don't support it (e.g.
  // menuitem).
  if (node.getAttribute("disabled") == "true")
    return; // Do nothing

  if (event.button == 1) {
    /* Execute the node's oncommand or command.
     *
     * XXX: we should use node.oncommand(event) once bug 246720 is fixed.
     */
    var target = node.hasAttribute("oncommand") ? node :
                 node.ownerDocument.getElementById(node.getAttribute("command"));
    var fn = new Function("event", target.getAttribute("oncommand"));
    fn.call(target, event);

    // If the middle-click was on part of a menu, close the menu.
    // (Menus close automatically with left-click but not with middle-click.)
    closeMenus(event.target);
  }
}

// Closes all popups that are ancestors of the node.
function closeMenus(node)
{
  if ("tagName" in node) {
    if (node.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
    && (node.tagName == "menupopup" || node.tagName == "popup"))
      node.hidePopup();

    closeMenus(node.parentNode);
  }
}

// Gather all descendent text under given document node.
function gatherTextUnder ( root ) 
{
  var text = "";
  var node = root.firstChild;
  var depth = 1;
  while ( node && depth > 0 ) {
    // See if this node is text.
    if ( node.nodeType == Node.TEXT_NODE ) {
      // Add this text to our collection.
      text += " " + node.data;
    } else if ( node instanceof HTMLImageElement) {
      // If it has an "alt" attribute, add that.
      var altText = node.getAttribute( "alt" );
      if ( altText && altText != "" ) {
        text += " " + altText;
      }
    }
    // Find next node to test.
    // First, see if this node has children.
    if ( node.hasChildNodes() ) {
      // Go to first child.
      node = node.firstChild;
      depth++;
    } else {
      // No children, try next sibling (or parent next sibling).
      while ( depth > 0 && !node.nextSibling ) {
        node = node.parentNode;
        depth--;
      }
      if ( node.nextSibling ) {
        node = node.nextSibling;
      }
    }
  }
  // Strip leading and tailing whitespace.
  text = text.trim();
  // Compress remaining whitespace.
  text = text.replace( /\s+/g, " " );
  return text;
}

function getShellService()
{
  var shell = null;
  try {
    shell = Components.classes["@mozilla.org/browser/shell-service;1"]
      .getService(Components.interfaces.nsIShellService);
  } catch (e) {
  }
  return shell;
}

function isBidiEnabled() {
  // first check the pref.
  if (getBoolPref("bidi.browser.ui", false))
    return true;

  // then check intl.uidirection.<locale>
  var chromeReg = Components.classes["@mozilla.org/chrome/chrome-registry;1"].
                  getService(Components.interfaces.nsIXULChromeRegistry);
  if (chromeReg.isLocaleRTL("global"))
    return true;

  // now see if the system locale is an RTL one.
  var rv = false;

  try {
    var localeService = Components.classes["@mozilla.org/intl/nslocaleservice;1"]
                                  .getService(Components.interfaces.nsILocaleService);
    var systemLocale = localeService.getSystemLocale().getCategory("NSILOCALE_CTYPE").substr(0,3);

    switch (systemLocale) {
      case "ar-":
      case "he-":
      case "fa-":
      case "ug-":
      case "ur-":
      case "syr":
        rv = true;
        Services.prefs.setBoolPref("bidi.browser.ui", true);
    }
  } catch (e) {}

  return rv;
}

function openAboutDialog() {
  var enumerator = Services.wm.getEnumerator("Browser:About");
  while (enumerator.hasMoreElements()) {
    // Only open one about window (Bug 599573)
    let win = enumerator.getNext();
    if (win.closed) {
      continue;
    }
    win.focus();
    return;
  }

//@line 465 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"
  var features = "chrome,centerscreen,dependent";
//@line 471 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"
  window.openDialog("chrome://browser/content/aboutDialog.xul", "", features);
}

function openPreferences(paneID, extraArgs)
{
  if (Services.prefs.getBoolPref("browser.preferences.inContent")) {
    openUILinkIn("about:preferences", "tab");
  } else {
    var instantApply = getBoolPref("browser.preferences.instantApply", false);
    var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

    var win = Services.wm.getMostRecentWindow("Browser:Preferences");
    if (win) {
      win.focus();
      if (paneID) {
        var pane = win.document.getElementById(paneID);
        win.document.documentElement.showPane(pane);
      }

      if (extraArgs && extraArgs["advancedTab"]) {
        var advancedPaneTabs = win.document.getElementById("advancedPrefs");
        advancedPaneTabs.selectedTab = win.document.getElementById(extraArgs["advancedTab"]);
      }

     return;
    }

    openDialog("chrome://browser/content/preferences/preferences.xul",
               "Preferences", features, paneID, extraArgs);
  }
}

function openAdvancedPreferences(tabID)
{
  openPreferences("paneAdvanced", { "advancedTab" : tabID });
}

/**
 * Opens the troubleshooting information (about:support) page for this version
 * of the application.
 */
function openTroubleshootingPage()
{
  openUILinkIn("about:support", "tab");
}

//@line 518 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"
/**
 * Opens the troubleshooting information (about:support) page for this version
 * of the application.
 */
function openHealthReport()
{
  openUILinkIn("about:healthreport", "tab");
}
//@line 527 "h:\projects\mozilla\release-src\browser\base\content\utilityOverlay.js"

/**
 * Opens the feedback page for this version of the application.
 */
function openFeedbackPage()
{
  openUILinkIn("https://input.mozilla.org/feedback", "tab");
}

function buildHelpMenu()
{
  // Enable/disable the "Report Web Forgery" menu item.
  if (typeof gSafeBrowsing != "undefined")
    gSafeBrowsing.setReportPhishingMenu();
}

function isElementVisible(aElement)
{
  if (!aElement)
    return false;

  // If aElement or a direct or indirect parent is hidden or collapsed,
  // height, width or both will be 0.
  var bo = aElement.boxObject;
  return (bo.height > 0 && bo.width > 0);
}

function makeURLAbsolute(aBase, aUrl)
{
  // Note:  makeURI() will throw if aUri is not a valid URI
  return makeURI(aUrl, null, makeURI(aBase)).spec;
}


/**
 * openNewTabWith: opens a new tab with the given URL.
 *
 * @param aURL
 *        The URL to open (as a string).
 * @param aDocument
 *        The document from which the URL came, or null. This is used to set the
 *        referrer header and to do a security check of whether the document is
 *        allowed to reference the URL. If null, there will be no referrer
 *        header and no security check.
 * @param aPostData
 *        Form POST data, or null.
 * @param aEvent
 *        The triggering event (for the purpose of determining whether to open
 *        in the background), or null.
 * @param aAllowThirdPartyFixup
 *        If true, then we allow the URL text to be sent to third party services
 *        (e.g., Google's I Feel Lucky) for interpretation. This parameter may
 *        be undefined in which case it is treated as false.
 * @param [optional] aReferrer
 *        If aDocument is null, then this will be used as the referrer.
 *        There will be no security check.
 */ 
function openNewTabWith(aURL, aDocument, aPostData, aEvent,
                        aAllowThirdPartyFixup, aReferrer) {
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  // As in openNewWindowWith(), we want to pass the charset of the
  // current document over to a new tab.
  var originCharset = aDocument && aDocument.characterSet;
  if (!originCharset &&
      document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = window.content.document.characterSet;

  openLinkIn(aURL, aEvent && aEvent.shiftKey ? "tabshifted" : "tab",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aDocument ? aDocument.documentURIObject : aReferrer });
}

function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup, aReferrer) {
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  // if and only if the current window is a browser window and it has a
  // document with a character set, then extract the current charset menu
  // setting from the current document and use it to initialize the new browser
  // window...
  var originCharset = aDocument && aDocument.characterSet;
  if (!originCharset &&
      document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = window.content.document.characterSet;

  openLinkIn(aURL, "window",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aDocument ? aDocument.documentURIObject : aReferrer });
}

/**
 * isValidFeed: checks whether the given data represents a valid feed.
 *
 * @param  aLink
 *         An object representing a feed with title, href and type.
 * @param  aPrincipal
 *         The principal of the document, used for security check.
 * @param  aIsFeed
 *         Whether this is already a known feed or not, if true only a security
 *         check will be performed.
 */ 
function isValidFeed(aLink, aPrincipal, aIsFeed)
{
  if (!aLink || !aPrincipal)
    return false;

  var type = aLink.type.toLowerCase().replace(/^\s+|\s*(?:;.*)?$/g, "");
  if (!aIsFeed) {
    aIsFeed = (type == "application/rss+xml" ||
               type == "application/atom+xml");
  }

  if (aIsFeed) {
    try {
      urlSecurityCheck(aLink.href, aPrincipal,
                       Components.interfaces.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
      return type || "application/rss+xml";
    }
    catch(ex) {
    }
  }

  return null;
}

// aCalledFromModal is optional
function openHelpLink(aHelpTopic, aCalledFromModal) {
  var url = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                      .getService(Components.interfaces.nsIURLFormatter)
                      .formatURLPref("app.support.baseURL");
  url += aHelpTopic;

  var where = aCalledFromModal ? "window" : "tab";
  openUILinkIn(url, where);
}

function openPrefsHelp() {
  // non-instant apply prefwindows are usually modal, so we can't open in the topmost window, 
  // since its probably behind the window.
  var instantApply = getBoolPref("browser.preferences.instantApply");

  var helpTopic = document.getElementsByTagName("prefwindow")[0].currentPane.helpTopic;
  openHelpLink(helpTopic, !instantApply);
}

function trimURL(aURL) {
  // This function must not modify the given URL such that calling
  // nsIURIFixup::createFixupURI with the result will produce a different URI.

  // remove single trailing slash for http/https/ftp URLs
  let url = aURL.replace(/^((?:http|https|ftp):\/\/[^/]+)\/$/, "$1");

  // remove http://
  if (!url.startsWith("http://")) {
    return url;
  }
  let urlWithoutProtocol = url.substring(7);

  let flags = Services.uriFixup.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP |
              Services.uriFixup.FIXUP_FLAG_FIX_SCHEME_TYPOS;
  let fixedUpURL = Services.uriFixup.createFixupURI(urlWithoutProtocol, flags);
  let expectedURLSpec;
  try {
    expectedURLSpec = makeURI(aURL).spec;
  } catch (ex) {
    return url;
  }
  if (fixedUpURL.spec == expectedURLSpec) {
    return urlWithoutProtocol;
  }
  return url;
}

function getShellService()
{
  var shell = null;
  try {
    shell = Components.classes["@mozilla.org/browser/shell-service;1"]
      .getService(Components.interfaces.nsIShellService);
  } catch (e) {
  }
  return shell;
}
