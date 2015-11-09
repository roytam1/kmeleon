/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["RecentWindow"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource:///modules/KMeleon.jsm");

//@line 19 "g:\mozilla-esr31b\browser\modules\RecentWindow.jsm"

this.RecentWindow = {
  /*
   * Get the most recent browser window.
   *
   * @param aOptions an object accepting the arguments for the search.
   *        * private: true to restrict the search to private windows
   *            only, false to restrict the search to non-private only.
   *            Omit the property to search in both groups.
   *        * allowPopups: true if popup windows are permissable.
   */
  getMostRecentBrowserWindow: function RW_getMostRecentBrowserWindow(aOptions) {
  
    return KMeleon.GetActiveBrowser().contentDOMWindow; // Bad
    let checkPrivacy = typeof aOptions == "object" &&
                       "private" in aOptions;

    let allowPopups = typeof aOptions == "object" && !!aOptions.allowPopups;

    function isSuitableBrowserWindow(win) {
      return (!win.closed &&
              (allowPopups || win.toolbar.visible) &&
              (!checkPrivacy ||
               PrivateBrowsingUtils.permanentPrivateBrowsing ||
               PrivateBrowsingUtils.isWindowPrivate(win) == aOptions.private));
    }

//@line 60 "g:\mozilla-esr31b\browser\modules\RecentWindow.jsm"
    let windowList = Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowList.hasMoreElements()) {
      let win = windowList.getNext();
      if (isSuitableBrowserWindow(win))
        return win;
    }
    return null;
//@line 68 "g:\mozilla-esr31b\browser\modules\RecentWindow.jsm"
  }
};

