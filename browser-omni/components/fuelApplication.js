/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const Ci = Components.interfaces;
const Cc = Components.classes;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const APPLICATION_CID = Components.ID("fe74cf80-aa2d-11db-abbd-0800200c9a66");
const APPLICATION_CONTRACTID = "@mozilla.org/fuel/application;1";

//=================================================
// Singleton that holds services and utilities
var Utilities = {
  get bookmarks() {
    let bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                    getService(Ci.nsINavBookmarksService);
    this.__defineGetter__("bookmarks", function() bookmarks);
    return this.bookmarks;
  },

  get bookmarksObserver() {
    let bookmarksObserver = new BookmarksObserver();
    this.__defineGetter__("bookmarksObserver", function() bookmarksObserver);
    return this.bookmarksObserver;
  },

  get annotations() {
    let annotations = Cc["@mozilla.org/browser/annotation-service;1"].
                      getService(Ci.nsIAnnotationService);
    this.__defineGetter__("annotations", function() annotations);
    return this.annotations;
  },

  get history() {
    let history = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);
    this.__defineGetter__("history", function() history);
    return this.history;
  },

  get windowMediator() {
    let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    this.__defineGetter__("windowMediator", function() windowMediator);
    return this.windowMediator;
  },

  makeURI: function fuelutil_makeURI(aSpec) {
    if (!aSpec)
      return null;
    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    return ios.newURI(aSpec, null, null);
  },

  free: function fuelutil_free() {
    delete this.bookmarks;
    delete this.bookmarksObserver;
    delete this.annotations;
    delete this.history;
    delete this.windowMediator;
  }
};


//=================================================
// Window implementation

var fuelWindowMap = new WeakMap();
function getWindow(aWindow) {
  let fuelWindow = fuelWindowMap.get(aWindow);
  if (!fuelWindow) {
    fuelWindow = new Window(aWindow);
    fuelWindowMap.set(aWindow, fuelWindow);
  }
  return fuelWindow;
}

// Don't call new Window() directly; use getWindow instead.
function Window(aWindow) {
  this._window = aWindow;
  this._events = new Events();

  this._watch("TabOpen");
  this._watch("TabMove");
  this._watch("TabClose");
  this._watch("TabSelect");
}

Window.prototype = {
  get events() {
    return this._events;
  },

  get _tabbrowser() {
    return this._window.gBrowser;
  },

  /*
   * Helper used to setup event handlers on the XBL element. Note that the events
   * are actually dispatched to tabs, so we capture them.
   */
  _watch: function win_watch(aType) {
    this._tabbrowser.tabContainer.addEventListener(aType, this,
                                                   /* useCapture = */ true);
  },

  handleEvent: function win_handleEvent(aEvent) {
    this._events.dispatch(aEvent.type, getBrowserTab(this, aEvent.originalTarget.linkedBrowser));
  },

  get tabs() {
    var tabs = [];
    var browsers = this._tabbrowser.browsers;
    for (var i=0; i<browsers.length; i++)
      tabs.push(getBrowserTab(this, browsers[i]));
    return tabs;
  },

  get activeTab() {
    return getBrowserTab(this, this._tabbrowser.selectedBrowser);
  },

  open: function win_open(aURI) {
    return getBrowserTab(this, this._tabbrowser.addTab(aURI.spec).linkedBrowser);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIWindow])
};

//=================================================
// BrowserTab implementation

var fuelBrowserTabMap = new WeakMap();
function getBrowserTab(aFUELWindow, aBrowser) {
  let fuelBrowserTab = fuelBrowserTabMap.get(aBrowser);
  if (!fuelBrowserTab) {
    fuelBrowserTab = new BrowserTab(aFUELWindow, aBrowser);
    fuelBrowserTabMap.set(aBrowser, fuelBrowserTab);
  }
  else {
    // This tab may have moved to another window, so make sure its cached
    // window is up-to-date.
    fuelBrowserTab._window = aFUELWindow;
  }

  return fuelBrowserTab;
}

// Don't call new BrowserTab() directly; call getBrowserTab instead.
function BrowserTab(aFUELWindow, aBrowser) {
  this._window = aFUELWindow;
  this._browser = aBrowser;
  this._events = new Events();

  this._watch("load");
}

BrowserTab.prototype = {
  get _tabbrowser() {
    return this._window._tabbrowser;
  },

  get uri() {
    return this._browser.currentURI;
  },

  get index() {
    var tabs = this._tabbrowser.tabs;
    for (var i=0; i<tabs.length; i++) {
      if (tabs[i].linkedBrowser == this._browser)
        return i;
    }
    return -1;
  },

  get events() {
    return this._events;
  },

  get window() {
    return this._window;
  },

  get document() {
    return this._browser.contentDocument;
  },

  /*
   * Helper used to setup event handlers on the XBL element
   */
  _watch: function bt_watch(aType) {
    this._browser.addEventListener(aType, this,
                                   /* useCapture = */ true);
  },

  handleEvent: function bt_handleEvent(aEvent) {
    if (aEvent.type == "load") {
      if (!(aEvent.originalTarget instanceof Ci.nsIDOMDocument))
        return;

      if (aEvent.originalTarget.defaultView instanceof Ci.nsIDOMWindow &&
          aEvent.originalTarget.defaultView.frameElement)
        return;
    }
    this._events.dispatch(aEvent.type, this);
  },
  /*
   * Helper used to determine the index offset of the browsertab
   */
  _getTab: function bt_gettab() {
    var tabs = this._tabbrowser.tabs;
    return tabs[this.index] || null;
  },

  load: function bt_load(aURI) {
    this._browser.loadURI(aURI.spec, null, null);
  },

  focus: function bt_focus() {
    this._tabbrowser.selectedTab = this._getTab();
    this._tabbrowser.focus();
  },

  close: function bt_close() {
    this._tabbrowser.removeTab(this._getTab());
  },

  moveBefore: function bt_movebefore(aBefore) {
    this._tabbrowser.moveTabTo(this._getTab(), aBefore.index);
  },

  moveToEnd: function bt_moveend() {
    this._tabbrowser.moveTabTo(this._getTab(), this._tabbrowser.browsers.length);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIBrowserTab])
};


//=================================================
// Annotations implementation
function Annotations(aId) {
  this._id = aId;
}

Annotations.prototype = {
  get names() {
    return Utilities.annotations.getItemAnnotationNames(this._id);
  },

  has: function ann_has(aName) {
    return Utilities.annotations.itemHasAnnotation(this._id, aName);
  },

  get: function ann_get(aName) {
    if (this.has(aName))
      return Utilities.annotations.getItemAnnotation(this._id, aName);
    return null;
  },

  set: function ann_set(aName, aValue, aExpiration) {
    Utilities.annotations.setItemAnnotation(this._id, aName, aValue, 0, aExpiration);
  },

  remove: function ann_remove(aName) {
    if (aName)
      Utilities.annotations.removeItemAnnotation(this._id, aName);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIAnnotations])
};


//=================================================
// BookmarksObserver implementation (internal class)
//
// BookmarksObserver is a global singleton which watches the browser's
// bookmarks and sends you events when things change.
//
// You can register three different kinds of event listeners on
// BookmarksObserver, using addListener, addFolderListener, and
// addRootlistener.
//
//  - addListener(aId, aEvent, aListener) lets you listen to a specific
//    bookmark.  You can listen to the "change", "move", and "remove" events.
//
//  - addFolderListener(aId, aEvent, aListener) lets you listen to a specific
//    bookmark folder.  You can listen to "addchild" and "removechild".
//
//  - addRootListener(aEvent, aListener) lets you listen to the root bookmark
//    node.  This lets you hear "add", "remove", and "change" events on all
//    bookmarks.
//

function BookmarksObserver() {
  this._eventsDict = {};
  this._folderEventsDict = {};
  this._rootEvents = new Events();
  Utilities.bookmarks.addObserver(this, /* ownsWeak = */ true);
}

BookmarksObserver.prototype = {
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onItemVisited: function () {},

  onItemAdded: function bo_onItemAdded(aId, aFolder, aIndex, aItemType, aURI) {
    this._rootEvents.dispatch("add", aId);
    this._dispatchToEvents("addchild", aId, this._folderEventsDict[aFolder]);
  },

  onItemRemoved: function bo_onItemRemoved(aId, aFolder, aIndex) {
    this._rootEvents.dispatch("remove", aId);
    this._dispatchToEvents("remove", aId, this._eventsDict[aId]);
    this._dispatchToEvents("removechild", aId, this._folderEventsDict[aFolder]);
  },

  onItemChanged: function bo_onItemChanged(aId, aProperty, aIsAnnotationProperty, aValue) {
    this._rootEvents.dispatch("change", aProperty);
    this._dispatchToEvents("change", aProperty, this._eventsDict[aId]);
  },

  onItemMoved: function bo_onItemMoved(aId, aOldParent, aOldIndex, aNewParent, aNewIndex) {
    this._dispatchToEvents("move", aId, this._eventsDict[aId]);
  },

  _dispatchToEvents: function bo_dispatchToEvents(aEvent, aData, aEvents) {
    if (aEvents) {
      aEvents.dispatch(aEvent, aData);
    }
  },

  _addListenerToDict: function bo_addListenerToDict(aId, aEvent, aListener, aDict) {
    var events = aDict[aId];
    if (!events) {
      events = new Events();
      aDict[aId] = events;
    }
    events.addListener(aEvent, aListener);
  },

  _removeListenerFromDict: function bo_removeListenerFromDict(aId, aEvent, aListener, aDict) {
    var events = aDict[aId];
    if (!events) {
      return;
    }
    events.removeListener(aEvent, aListener);
    if (events._listeners.length == 0) {
      delete aDict[aId];
    }
  },

  addListener: function bo_addListener(aId, aEvent, aListener) {
    this._addListenerToDict(aId, aEvent, aListener, this._eventsDict);
  },

  removeListener: function bo_removeListener(aId, aEvent, aListener) {
    this._removeListenerFromDict(aId, aEvent, aListener, this._eventsDict);
  },

  addFolderListener: function addFolderListener(aId, aEvent, aListener) {
    this._addListenerToDict(aId, aEvent, aListener, this._folderEventsDict);
  },

  removeFolderListener: function removeFolderListener(aId, aEvent, aListener) {
    this._removeListenerFromDict(aId, aEvent, aListener, this._folderEventsDict);
  },

  addRootListener: function addRootListener(aEvent, aListener) {
    this._rootEvents.addListener(aEvent, aListener);
  },

  removeRootListener: function removeRootListener(aEvent, aListener) {
    this._rootEvents.removeListener(aEvent, aListener);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver,
                                         Ci.nsISupportsWeakReference])
};

//=================================================
// Bookmark implementation
//
// Bookmark event listeners are stored in BookmarksObserver, not in the
// Bookmark objects themselves.  Thus, you don't have to hold on to a Bookmark
// object in order for your event listener to stay valid, and Bookmark objects
// not kept alive by the extension can be GC'ed.
//
// A consequence of this is that if you have two different Bookmark objects x
// and y for the same bookmark (i.e., x != y but x.id == y.id), and you do
//
//   x.addListener("foo", fun);
//   y.removeListener("foo", fun);
//
// the second line will in fact remove the listener added in the first line.
//

function Bookmark(aId, aParent, aType) {
  this._id = aId;
  this._parent = aParent;
  this._type = aType || "bookmark";
  this._annotations = new Annotations(this._id);

  // Our _events object forwards to bookmarksObserver.
  var self = this;
  this._events = {
    addListener: function bookmarkevents_al(aEvent, aListener) {
      Utilities.bookmarksObserver.addListener(self._id, aEvent, aListener);
    },
    removeListener: function bookmarkevents_rl(aEvent, aListener) {
      Utilities.bookmarksObserver.removeListener(self._id, aEvent, aListener);
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
  };

  // For our onItemMoved listener, which updates this._parent.
  Utilities.bookmarks.addObserver(this, /* ownsWeak = */ true);
}

Bookmark.prototype = {
  get id() {
    return this._id;
  },

  get title() {
    return Utilities.bookmarks.getItemTitle(this._id);
  },

  set title(aTitle) {
    Utilities.bookmarks.setItemTitle(this._id, aTitle);
  },

  get uri() {
    return Utilities.bookmarks.getBookmarkURI(this._id);
  },

  set uri(aURI) {
    return Utilities.bookmarks.changeBookmarkURI(this._id, aURI);
  },

  get description() {
    return this._annotations.get("bookmarkProperties/description");
  },

  set description(aDesc) {
    this._annotations.set("bookmarkProperties/description", aDesc, Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  get keyword() {
    return Utilities.bookmarks.getKeywordForBookmark(this._id);
  },

  set keyword(aKeyword) {
    Utilities.bookmarks.setKeywordForBookmark(this._id, aKeyword);
  },

  get type() {
    return this._type;
  },

  get parent() {
    return this._parent;
  },

  set parent(aFolder) {
    Utilities.bookmarks.moveItem(this._id, aFolder.id, Utilities.bookmarks.DEFAULT_INDEX);
    // this._parent is updated in onItemMoved
  },

  get annotations() {
    return this._annotations;
  },

  get events() {
    return this._events;
  },

  remove : function bm_remove() {
    Utilities.bookmarks.removeItem(this._id);
  },

  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onItemAdded: function () {},
  onItemVisited: function () {},
  onItemRemoved: function () {},
  onItemChanged: function () {},

  onItemMoved: function(aId, aOldParent, aOldIndex, aNewParent, aNewIndex) {
    if (aId == this._id) {
      this._parent = new BookmarkFolder(aNewParent, Utilities.bookmarks.getFolderIdForItem(aNewParent));
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIBookmark,
                                         Ci.nsINavBookmarkObserver,
                                         Ci.nsISupportsWeakReference])
};


//=================================================
// BookmarkFolder implementation
//
// As with Bookmark, events on BookmarkFolder are handled by the
// BookmarksObserver singleton.
//

function BookmarkFolder(aId, aParent) {
  this._id = aId;
  this._parent = aParent;
  this._annotations = new Annotations(this._id);

  // Our event listeners are handled by the BookmarksObserver singleton.  This
  // is a bit complicated because there are three different kinds of events we
  // might want to listen to here:
  //
  //  - If this._parent is null, we're the root bookmark folder, and all our
  //    listeners should be root listeners.
  //
  //  - Otherwise, events ending with "child" (addchild, removechild) are
  //    handled by a folder listener.
  //
  //  - Other events are handled by a vanilla bookmark listener.

  var self = this;
  this._events = {
    addListener: function bmfevents_al(aEvent, aListener) {
      if (self._parent) {
        if (/child$/.test(aEvent)) {
          Utilities.bookmarksObserver.addFolderListener(self._id, aEvent, aListener);
        }
        else {
          Utilities.bookmarksObserver.addListener(self._id, aEvent, aListener);
        }
      }
      else {
        Utilities.bookmarksObserver.addRootListener(aEvent, aListener);
      }
    },
    removeListener: function bmfevents_rl(aEvent, aListener) {
      if (self._parent) {
        if (/child$/.test(aEvent)) {
          Utilities.bookmarksObserver.removeFolderListener(self._id, aEvent, aListener);
        }
        else {
          Utilities.bookmarksObserver.removeListener(self._id, aEvent, aListener);
        }
      }
      else {
        Utilities.bookmarksObserver.removeRootListener(aEvent, aListener);
      }
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
  };

  // For our onItemMoved listener, which updates this._parent.
  Utilities.bookmarks.addObserver(this, /* ownsWeak = */ true);
}

BookmarkFolder.prototype = {
  get id() {
    return this._id;
  },

  get title() {
    return Utilities.bookmarks.getItemTitle(this._id);
  },

  set title(aTitle) {
    Utilities.bookmarks.setItemTitle(this._id, aTitle);
  },

  get description() {
    return this._annotations.get("bookmarkProperties/description");
  },

  set description(aDesc) {
    this._annotations.set("bookmarkProperties/description", aDesc, Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  get type() {
    return "folder";
  },

  get parent() {
    return this._parent;
  },

  set parent(aFolder) {
    Utilities.bookmarks.moveItem(this._id, aFolder.id, Utilities.bookmarks.DEFAULT_INDEX);
    // this._parent is updated in onItemMoved
  },

  get annotations() {
    return this._annotations;
  },

  get events() {
    return this._events;
  },

  get children() {
    var items = [];

    var options = Utilities.history.getNewQueryOptions();
    var query = Utilities.history.getNewQuery();
    query.setFolders([this._id], 1);
    var result = Utilities.history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      if (node.type == node.RESULT_TYPE_FOLDER) {
        var folder = new BookmarkFolder(node.itemId, this._id);
        items.push(folder);
      }
      else if (node.type == node.RESULT_TYPE_SEPARATOR) {
        var separator = new Bookmark(node.itemId, this._id, "separator");
        items.push(separator);
      }
      else {
        var bookmark = new Bookmark(node.itemId, this._id, "bookmark");
        items.push(bookmark);
      }
    }
    rootNode.containerOpen = false;

    return items;
  },

  addBookmark: function bmf_addbm(aTitle, aUri) {
    var newBookmarkID = Utilities.bookmarks.insertBookmark(this._id, aUri, Utilities.bookmarks.DEFAULT_INDEX, aTitle);
    var newBookmark = new Bookmark(newBookmarkID, this, "bookmark");
    return newBookmark;
  },

  addSeparator: function bmf_addsep() {
    var newBookmarkID = Utilities.bookmarks.insertSeparator(this._id, Utilities.bookmarks.DEFAULT_INDEX);
    var newBookmark = new Bookmark(newBookmarkID, this, "separator");
    return newBookmark;
  },

  addFolder: function bmf_addfolder(aTitle) {
    var newFolderID = Utilities.bookmarks.createFolder(this._id, aTitle, Utilities.bookmarks.DEFAULT_INDEX);
    var newFolder = new BookmarkFolder(newFolderID, this);
    return newFolder;
  },

  remove: function bmf_remove() {
    Utilities.bookmarks.removeItem(this._id);
  },

  // observer
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch : function () {},
  onItemAdded : function () {},
  onItemRemoved : function () {},
  onItemChanged : function () {},

  onItemMoved: function bf_onItemMoved(aId, aOldParent, aOldIndex, aNewParent, aNewIndex) {
    if (this._id == aId) {
      this._parent = new BookmarkFolder(aNewParent, Utilities.bookmarks.getFolderIdForItem(aNewParent));
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIBookmarkFolder,
                                         Ci.nsINavBookmarkObserver,
                                         Ci.nsISupportsWeakReference])
};

//=================================================
// BookmarkRoots implementation
function BookmarkRoots() {
}

BookmarkRoots.prototype = {
  get menu() {
    if (!this._menu)
      this._menu = new BookmarkFolder(Utilities.bookmarks.bookmarksMenuFolder, null);

    return this._menu;
  },

  get toolbar() {
    if (!this._toolbar)
      this._toolbar = new BookmarkFolder(Utilities.bookmarks.toolbarFolder, null);

    return this._toolbar;
  },

  get tags() {
    if (!this._tags)
      this._tags = new BookmarkFolder(Utilities.bookmarks.tagsFolder, null);

    return this._tags;
  },

  get unfiled() {
    if (!this._unfiled)
      this._unfiled = new BookmarkFolder(Utilities.bookmarks.unfiledBookmarksFolder, null);

    return this._unfiled;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.fuelIBookmarkRoots])
};


//=================================================
// Factory - Treat Application as a singleton
// XXX This is required, because we're registered for the 'JavaScript global
// privileged property' category, whose handler always calls createInstance.
// See bug 386535.
var gSingleton = null;
var ApplicationFactory = {
  createInstance: function af_ci(aOuter, aIID) {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (gSingleton == null) {
      gSingleton = new Application();
    }

    return gSingleton.QueryInterface(aIID);
  }
};


/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

//=================================================
// Console constructor
function Console() {
  this._console = Components.classes["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
}

//=================================================
// Console implementation
Console.prototype = {
  log: function cs_log(aMsg) {
    this._console.logStringMessage(aMsg);
  },

  open: function cs_open() {
    var wMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                              .getService(Ci.nsIWindowMediator);
    var console = wMediator.getMostRecentWindow("global:console");
    if (!console) {
      var wWatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                             .getService(Ci.nsIWindowWatcher);
      wWatch.openWindow(null, "chrome://global/content/console.xul", "_blank",
                        "chrome,dialog=no,all", null);
    } else {
      // console was already open
      console.focus();
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIConsole])
};


//=================================================
// EventItem constructor
function EventItem(aType, aData) {
  this._type = aType;
  this._data = aData;
}

//=================================================
// EventItem implementation
EventItem.prototype = {
  _cancel: false,

  get type() {
    return this._type;
  },

  get data() {
    return this._data;
  },

  preventDefault: function ei_pd() {
    this._cancel = true;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIEventItem])
};


//=================================================
// Events constructor
function Events(notifier) {
  this._listeners = [];
  this._notifier = notifier;
}

//=================================================
// Events implementation
Events.prototype = {
  addListener: function evts_al(aEvent, aListener) {
    function hasFilter(element) {
      return element.event == aEvent && element.listener == aListener;
    }

    if (this._listeners.some(hasFilter))
      return;

    this._listeners.push({
      event: aEvent,
      listener: aListener
    });

    if (this._notifier) {
      this._notifier(aEvent, aListener);
    }
  },

  removeListener: function evts_rl(aEvent, aListener) {
    function hasFilter(element) {
      return (element.event != aEvent) || (element.listener != aListener);
    }

    this._listeners = this._listeners.filter(hasFilter);
  },

  dispatch: function evts_dispatch(aEvent, aEventItem) {
    var eventItem = new EventItem(aEvent, aEventItem);

    this._listeners.forEach(function(key){
      if (key.event == aEvent) {
        key.listener.handleEvent ?
          key.listener.handleEvent(eventItem) :
          key.listener(eventItem);
      }
    });

    return !eventItem._cancel;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
};

//=================================================
// PreferenceObserver (internal class)
//
// PreferenceObserver is a global singleton which watches the browser's
// preferences and sends you events when things change.

function PreferenceObserver() {
  this._observersDict = {};
}

PreferenceObserver.prototype = {
  /**
   * Add a preference observer.
   *
   * @param aPrefs the nsIPrefBranch onto which we'll install our listener.
   * @param aDomain the domain our listener will watch (a string).
   * @param aEvent the event to listen to (you probably want "change").
   * @param aListener the function to call back when the event fires.  This
   *                  function will receive an EventData argument.
   */
  addListener: function po_al(aPrefs, aDomain, aEvent, aListener) {
    var root = aPrefs.root;
    if (!this._observersDict[root]) {
      this._observersDict[root] = {};
    }
    var observer = this._observersDict[root][aDomain];

    if (!observer) {
      observer = {
        events: new Events(),
        observe: function po_observer_obs(aSubject, aTopic, aData) {
          this.events.dispatch("change", aData);
        },
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference])
      };
      observer.prefBranch = aPrefs;
      observer.prefBranch.addObserver(aDomain, observer, /* ownsWeak = */ true);

      // Notice that the prefBranch keeps a weak reference to the observer;
      // it's this._observersDict which keeps the observer alive.
      this._observersDict[root][aDomain] = observer;
    }
    observer.events.addListener(aEvent, aListener);
  },

  /**
   * Remove a preference observer.
   *
   * This function's parameters are identical to addListener's.
   */
  removeListener: function po_rl(aPrefs, aDomain, aEvent, aListener) {
    var root = aPrefs.root;
    if (!this._observersDict[root] ||
        !this._observersDict[root][aDomain]) {
      return;
    }
    var observer = this._observersDict[root][aDomain];
    observer.events.removeListener(aEvent, aListener);

    if (observer.events._listeners.length == 0) {
      // nsIPrefBranch objects are not singletons -- we can have two
      // nsIPrefBranch'es for the same branch.  There's no guarantee that
      // aPrefs is the same object as observer.prefBranch, so we have to call
      // removeObserver on observer.prefBranch.
      observer.prefBranch.removeObserver(aDomain, observer);
      delete this._observersDict[root][aDomain];
      if (Object.keys(this._observersDict[root]).length == 0) {
        delete this._observersDict[root];
      }
    }
  }
};

//=================================================
// PreferenceBranch constructor
function PreferenceBranch(aBranch) {
  if (!aBranch)
    aBranch = "";

  this._root = aBranch;
  this._prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefService)
                          .QueryInterface(Ci.nsIPrefBranch);

  if (aBranch)
    this._prefs = this._prefs.getBranch(aBranch);

  let prefs = this._prefs;
  this._events = {
    addListener: function pb_al(aEvent, aListener) {
      gPreferenceObserver.addListener(prefs, "", aEvent, aListener);
    },
    removeListener: function pb_rl(aEvent, aListener) {
      gPreferenceObserver.removeListener(prefs, "", aEvent, aListener);
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
  };
}

//=================================================
// PreferenceBranch implementation
PreferenceBranch.prototype = {
  get root() {
    return this._root;
  },

  get all() {
    return this.find({});
  },

  get events() {
    return this._events;
  },

  // XXX: Disabled until we can figure out the wrapped object issues
  // name: "name" or /name/
  // path: "foo.bar." or "" or /fo+\.bar/
  // type: Boolean, Number, String (getPrefType)
  // locked: true, false (prefIsLocked)
  // modified: true, false (prefHasUserValue)
  find: function prefs_find(aOptions) {
    var retVal = [];
    var items = this._prefs.getChildList("");

    for (var i = 0; i < items.length; i++) {
      retVal.push(new Preference(items[i], this));
    }

    return retVal;
  },

  has: function prefs_has(aName) {
    return (this._prefs.getPrefType(aName) != Ci.nsIPrefBranch.PREF_INVALID);
  },

  get: function prefs_get(aName) {
    return this.has(aName) ? new Preference(aName, this) : null;
  },

  getValue: function prefs_gv(aName, aValue) {
    var type = this._prefs.getPrefType(aName);

    switch (type) {
      case Ci.nsIPrefBranch.PREF_STRING:
        aValue = this._prefs.getComplexValue(aName, Ci.nsISupportsString).data;
        break;
      case Ci.nsIPrefBranch.PREF_BOOL:
        aValue = this._prefs.getBoolPref(aName);
        break;
      case Ci.nsIPrefBranch.PREF_INT:
        aValue = this._prefs.getIntPref(aName);
        break;
    }

    return aValue;
  },

  setValue: function prefs_sv(aName, aValue) {
    var type = aValue != null ? aValue.constructor.name : "";

    switch (type) {
      case "String":
        var str = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(Ci.nsISupportsString);
        str.data = aValue;
        this._prefs.setComplexValue(aName, Ci.nsISupportsString, str);
        break;
      case "Boolean":
        this._prefs.setBoolPref(aName, aValue);
        break;
      case "Number":
        this._prefs.setIntPref(aName, aValue);
        break;
      default:
        throw("Unknown preference value specified.");
    }
  },

  reset: function prefs_reset() {
    this._prefs.resetBranch("");
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIPreferenceBranch])
};


//=================================================
// Preference constructor
function Preference(aName, aBranch) {
  this._name = aName;
  this._branch = aBranch;

  var self = this;
  this._events = {
    addListener: function pref_al(aEvent, aListener) {
      gPreferenceObserver.addListener(self._branch._prefs, self._name, aEvent, aListener);
    },
    removeListener: function pref_rl(aEvent, aListener) {
      gPreferenceObserver.removeListener(self._branch._prefs, self._name, aEvent, aListener);
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
  };
}

//=================================================
// Preference implementation
Preference.prototype = {
  get name() {
    return this._name;
  },

  get type() {
    var value = "";
    var type = this.branch._prefs.getPrefType(this._name);

    switch (type) {
      case Ci.nsIPrefBranch.PREF_STRING:
        value = "String";
        break;
      case Ci.nsIPrefBranch.PREF_BOOL:
        value = "Boolean";
        break;
      case Ci.nsIPrefBranch.PREF_INT:
        value = "Number";
        break;
    }

    return value;
  },

  get value() {
    return this.branch.getValue(this._name, null);
  },

  set value(aValue) {
    return this.branch.setValue(this._name, aValue);
  },

  get locked() {
    return this.branch._prefs.prefIsLocked(this.name);
  },

  set locked(aValue) {
    this.branch._prefs[ aValue ? "lockPref" : "unlockPref" ](this.name);
  },

  get modified() {
    return this.branch._prefs.prefHasUserValue(this.name);
  },

  get branch() {
    return this._branch;
  },

  get events() {
    return this._events;
  },

  reset: function pref_reset() {
    this.branch._prefs.clearUserPref(this.name);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIPreference])
};


//=================================================
// SessionStorage constructor
function SessionStorage() {
  this._storage = {};
  this._events = new Events();
}

//=================================================
// SessionStorage implementation
SessionStorage.prototype = {
  get events() {
    return this._events;
  },

  has: function ss_has(aName) {
    return this._storage.hasOwnProperty(aName);
  },

  set: function ss_set(aName, aValue) {
    this._storage[aName] = aValue;
    this._events.dispatch("change", aName);
  },

  get: function ss_get(aName, aDefaultValue) {
    return this.has(aName) ? this._storage[aName] : aDefaultValue;
  },

  QueryInterface : XPCOMUtils.generateQI([Ci.extISessionStorage])
};

//=================================================
// ExtensionObserver constructor (internal class)
//
// ExtensionObserver is a global singleton which watches the browser's
// extensions and sends you events when things change.

function ExtensionObserver() {
  this._eventsDict = {};

  AddonManager.addAddonListener(this);
  AddonManager.addInstallListener(this);
}

//=================================================
// ExtensionObserver implementation (internal class)
ExtensionObserver.prototype = {
  onDisabling: function eo_onDisabling(addon, needsRestart) {
    this._dispatchEvent(addon.id, "disable");
  },

  onEnabling: function eo_onEnabling(addon, needsRestart) {
    this._dispatchEvent(addon.id, "enable");
  },

  onUninstalling: function eo_onUninstalling(addon, needsRestart) {
    this._dispatchEvent(addon.id, "uninstall");
  },

  onOperationCancelled: function eo_onOperationCancelled(addon) {
    this._dispatchEvent(addon.id, "cancel");
  },

  onInstallEnded: function eo_onInstallEnded(install, addon) {
    this._dispatchEvent(addon.id, "upgrade");
  },

  addListener: function eo_al(aId, aEvent, aListener) {
    var events = this._eventsDict[aId];
    if (!events) {
      events = new Events();
      this._eventsDict[aId] = events;
    }
    events.addListener(aEvent, aListener);
  },

  removeListener: function eo_rl(aId, aEvent, aListener) {
    var events = this._eventsDict[aId];
    if (!events) {
      return;
    }
    events.removeListener(aEvent, aListener);
    if (events._listeners.length == 0) {
      delete this._eventsDict[aId];
    }
  },

  _dispatchEvent: function eo_dispatchEvent(aId, aEvent) {
    var events = this._eventsDict[aId];
    if (events) {
      events.dispatch(aEvent, aId);
    }
  }
};

//=================================================
// Extension constructor
function Extension(aItem) {
  this._item = aItem;
  this._firstRun = false;
  this._prefs = new PreferenceBranch("extensions." + this.id + ".");
  this._storage = new SessionStorage();

  let id = this.id;
  this._events = {
    addListener: function ext_events_al(aEvent, aListener) {
      gExtensionObserver.addListener(id, aEvent, aListener);
    },
    removeListener: function ext_events_rl(aEvent, aListener) {
      gExtensionObserver.addListener(id, aEvent, aListener);
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.extIEvents])
  };

  var installPref = "install-event-fired";
  if (!this._prefs.has(installPref)) {
    this._prefs.setValue(installPref, true);
    this._firstRun = true;
  }
}

//=================================================
// Extension implementation
Extension.prototype = {
  get id() {
    return this._item.id;
  },

  get name() {
    return this._item.name;
  },

  get enabled() {
    return this._item.isActive;
  },

  get version() {
    return this._item.version;
  },

  get firstRun() {
    return this._firstRun;
  },

  get storage() {
    return this._storage;
  },

  get prefs() {
    return this._prefs;
  },

  get events() {
    return this._events;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIExtension])
};


//=================================================
// Extensions constructor
function Extensions(addons) {
  this._cache = {};

  addons.forEach(function (addon) {
    this._cache[addon.id] = new Extension(addon);
  }, this);
}

//=================================================
// Extensions implementation
Extensions.prototype = {
  get all() {
    return this.find({});
  },

  // XXX: Disabled until we can figure out the wrapped object issues
  // id: "some@id" or /id/
  // name: "name" or /name/
  // version: "1.0.1"
  // minVersion: "1.0"
  // maxVersion: "2.0"
  find: function exts_find(aOptions) {
    return [e for each (e in this._cache)];
  },

  has: function exts_has(aId) {
    return aId in this._cache;
  },

  get: function exts_get(aId) {
    return this.has(aId) ? this._cache[aId] : null;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIExtensions])
};

//=================================================
// Application globals

var gExtensionObserver = new ExtensionObserver();
var gPreferenceObserver = new PreferenceObserver();

//=================================================
// extApplication constructor
function extApplication() {
}

//=================================================
// extApplication implementation
extApplication.prototype = {
  initToolkitHelpers: function extApp_initToolkitHelpers() {
    XPCOMUtils.defineLazyServiceGetter(this, "_info",
                                       "@mozilla.org/xre/app-info;1",
                                       "nsIXULAppInfo");

    this._obs = Cc["@mozilla.org/observer-service;1"].
                getService(Ci.nsIObserverService);
    this._obs.addObserver(this, "xpcom-shutdown", /* ownsWeak = */ true);
    this._registered = {"unload": true};
  },

  classInfo: XPCOMUtils.generateCI({interfaces: [Ci.extIApplication,
                                                 Ci.nsIObserver],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  // extIApplication
  get id() {
    return this._info.ID;
  },

  get name() {
    return this._info.name;
  },

  get version() {
    return this._info.version;
  },

  // for nsIObserver
  observe: function app_observe(aSubject, aTopic, aData) {
    if (aTopic == "app-startup") {
      this.events.dispatch("load", "application");
    }
    else if (aTopic == "final-ui-startup") {
      this.events.dispatch("ready", "application");
    }
    else if (aTopic == "quit-application-requested") {
      // we can stop the quit by checking the return value
      if (this.events.dispatch("quit", "application") == false)
        aSubject.data = true;
    }
    else if (aTopic == "xpcom-shutdown") {
      this.events.dispatch("unload", "application");
      gExtensionObserver = null;
      gPreferenceObserver = null;
    }
  },

  get console() {
    let console = new Console();
    this.__defineGetter__("console", function () console);
    return this.console;
  },

  get storage() {
    let storage = new SessionStorage();
    this.__defineGetter__("storage", function () storage);
    return this.storage;
  },

  get prefs() {
    let prefs = new PreferenceBranch("");
    this.__defineGetter__("prefs", function () prefs);
    return this.prefs;
  },

  getExtensions: function(callback) {
    AddonManager.getAddonsByTypes(["extension"], function (addons) {
      callback.callback(new Extensions(addons));
    });
  },

  get events() {

    // This ensures that FUEL only registers for notifications as needed
    // by callers. Note that the unload (xpcom-shutdown) event is listened
    // for by default, as it's needed for cleanup purposes.
    var self = this;
    function registerCheck(aEvent) {
      var rmap = { "load": "app-startup",
                   "ready": "final-ui-startup",
                   "quit": "quit-application-requested"};
      if (!(aEvent in rmap) || aEvent in self._registered)
        return;

      self._obs.addObserver(self, rmap[aEvent], /* ownsWeak = */ true);
      self._registered[aEvent] = true;
    }

    let events = new Events(registerCheck);
    this.__defineGetter__("events", function () events);
    return this.events;
  },

  // helper method for correct quitting/restarting
  _quitWithFlags: function app__quitWithFlags(aFlags) {
    let cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"]
                               .createInstance(Components.interfaces.nsISupportsPRBool);
    let quitType = aFlags & Components.interfaces.nsIAppStartup.eRestart ? "restart" : null;
    this._obs.notifyObservers(cancelQuit, "quit-application-requested", quitType);
    if (cancelQuit.data)
      return false; // somebody canceled our quit request

    let appStartup = Components.classes['@mozilla.org/toolkit/app-startup;1']
                               .getService(Components.interfaces.nsIAppStartup);
    appStartup.quit(aFlags);
    return true;
  },

  quit: function app_quit() {
    return this._quitWithFlags(Components.interfaces.nsIAppStartup.eAttemptQuit);
  },

  restart: function app_restart() {
    return this._quitWithFlags(Components.interfaces.nsIAppStartup.eAttemptQuit |
                               Components.interfaces.nsIAppStartup.eRestart);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.extIApplication, Ci.nsISupportsWeakReference])
};
//@line 733 "g:\mozilla-esr38\browser\fuel\fuelApplication.js"

//=================================================
// Application constructor
function Application() {
  this.initToolkitHelpers();
}

//=================================================
// Application implementation
function ApplicationPrototype() {
  // for nsIClassInfo + XPCOMUtils
  this.classID = APPLICATION_CID;

  // redefine the default factory for XPCOMUtils
  this._xpcom_factory = ApplicationFactory;

  // for nsISupports
  this.QueryInterface = XPCOMUtils.generateQI([
    Ci.fuelIApplication,
    Ci.extIApplication,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ]);

  // for nsIClassInfo
  this.classInfo = XPCOMUtils.generateCI({
    classID: APPLICATION_CID,
    contractID: APPLICATION_CONTRACTID,
    interfaces: [
      Ci.fuelIApplication,
      Ci.extIApplication,
      Ci.nsIObserver
    ],
    flags: Ci.nsIClassInfo.SINGLETON
  });

  // for nsIObserver
  this.observe = function (aSubject, aTopic, aData) {
    // Call the extApplication version of this function first
    var superPrototype = Object.getPrototypeOf(Object.getPrototypeOf(this));
    superPrototype.observe.call(this, aSubject, aTopic, aData);
    if (aTopic == "xpcom-shutdown") {
      this._obs.removeObserver(this, "xpcom-shutdown");
      Utilities.free();
    }
  };

  Object.defineProperty(this, "bookmarks", {
    get: function bookmarks () {
      let bookmarks = new BookmarkRoots();
      Object.defineProperty(this, "bookmarks", { value: bookmarks });
      return this.bookmarks;
    },
    enumerable: true,
    configurable: true
  });

  Object.defineProperty(this, "windows", {
    get: function windows() {
      var win = [];
      var browserEnum = Utilities.windowMediator.getEnumerator("navigator:browser");

      while (browserEnum.hasMoreElements())
        win.push(getWindow(browserEnum.getNext()));

      return win;
    },
    enumerable: true,
    configurable: true
  });

  Object.defineProperty(this, "activeWindow", {
    get: () => getWindow(Utilities.windowMediator.getMostRecentWindow("navigator:browser")),
    enumerable: true,
    configurable: true
  });

};

// set the proto, defined in extApplication.js
ApplicationPrototype.prototype = extApplication.prototype;

Application.prototype = new ApplicationPrototype();

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Application]);

