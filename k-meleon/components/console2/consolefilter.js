var ConsoleFilter = {
  mPrefBranch: Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefService)
                         .getBranch("extensions."),
  mCService: Components.classes["@mozilla.org/consoleservice;1"]
                       .getService(Components.interfaces.nsIConsoleService),
  mTimeout: null,
  mHistory: [],
  _limit: 32,

  onLoad: function()
  {
    this._textbox = _("ConsoleFilter2");
    if (this._textbox)
    {
      this._textbox.removeAttribute("disabled");
    }
    this.mCService = gConsole;

    if (this.mPrefBranch.prefHasUserValue("consolefilter"))
    {
      this.mHistory = this.mPrefBranch.getCharPref("consolefilter").split("\t").reverse();
      this.setValue(this.mHistory.pop());
      this.filter();
    }
    this.mCService.registerListener(this);
    this.focus();
  },

  onUnload: function()
  {
    this.mCService.unregisterListener(this);
    this.mHistory.push(this.getValue());

    this.mPrefBranch.setCharPref("consolefilter", this.mHistory.reverse().join("\t"));
    if (this.mHistory.length == 1 && !this.mHistory[0])
    {
      this.mPrefBranch.clearUserPref("consolefilter");
    }
  },

  customizeToolbarDone: function(aToolboxChanged)
  {
    this._textbox = _("ConsoleFilter2");
    if (this._textbox)
    {
      this._textbox.removeAttribute("disabled");
    }
    this.filter();
  },

/*** filter functionality ***/

  filter: function(aIncrement)
  {
    var filterStrings = this._parseQuery();
    var negative = filterStrings.pop();
    var changed = false;

      for (var row = gConsole.firstChild; row; row = row.nextSibling)
      {
        if (!aIncrement || !row.cf_category)
        {
          if (this.filterRow(row, filterStrings, negative))
          {
            changed = true;
          }
        }
      }

      if (changed) // assert !(this._cf2 && aIncrement)
      {
        gConsole.scrollToSelectedItem();
        updateMenuItems();
      }
  },

  filterRow: function(aRow, aStrings, aNegative)
  {
      if (!aStrings)
      {
          aStrings = this._parseQuery();
          aNegative = aStrings.pop();
      }
      if (!aRow.cf_category)
      { // cache the category names in our own format (js-*, css, xml, -)
          this._setCategory(aRow);
      }
      
      var text = aRow.getAttribute("msg") || "";
      text += "\n" + (aRow.getAttribute("url") || aRow.getAttribute("href") || "") + "\n";
      text += aRow.getAttribute("code") || "";
      text = text.replace(/[ \t]+/g, " ").toLowerCase();
      var category = "\n@" + aRow.cf_category;
      
      var visible = true;
      for (var i = 0; visible && i < aStrings.length; i++)
      {
          if (/^@\S/.test(aStrings[i]))
          { // category names always start with "@"
              visible = ((text + category).indexOf(aStrings[i]) > -1) != aNegative[i];
          }
          else
          {
              visible = (text.indexOf(aStrings[i]) > -1) != aNegative[i];
          }
      }
      
      if (arguments.length == 1)
      {
          if (!visible) // hint for Console²'s listener (prevents flickering)
          {
              aRow._hidden = true;
          }
          return false;
      }
      
      var old_hidden = aRow.hidden;
      
      aRow.hidden = !visible;
      
      return old_hidden == visible;
  },

  _parseQuery: function()
  {
      var query = this.getValue().replace(/[ \t]+/g, " ").toLowerCase();
      var filterStrings = query.replace(/(-)?(?:"([^"]+)(?:"|$)|(\S+))\s*/g, "\n$1 $2$3").split("\n").slice(1);
      var negative = filterStrings.map(function(aQuery) { return aQuery.charAt(0) == "-"; });
      filterStrings = filterStrings.map(function(aQuery, aIdx) { return aQuery.substr((negative[aIdx])?2:1); });
      
      return filterStrings.concat([negative]);
  },

  _setCategory: function(aRow)
  {
      var cf_category = aRow.getAttribute("category") || "";
      
      if (/(\S+) javascript/i.test(cf_category))
      {
          cf_category = "js-" + RegExp.$1.toLowerCase();
      }
      else if (/CSS/i.test(cf_category))
      {
          cf_category = "css";
      }
      else if (/XBL|XUL|HTML|xml/.test(cf_category))
      {
          cf_category = "xml";
      }
      else
      {
          cf_category = "-";
      }
      
      aRow.cf_category = cf_category;
  },

  observe: function(aObject)
  { // make sure that we filter after the event has been displayed
      if (window.ConsoleFilter)
      {
          setTimeout(function() { ConsoleFilter.filter(true); }, 0);
      }
  },

  listen: function(aRow)
  { // Console² offers us the row before displaying it (avoids flickering)
      this.filterRow(aRow);
  },

/*** UI element functionality ***/

  getValue: function()
  {
      return (this._textbox && this._textbox.value) ? this._textbox.value : "";
  },

  setValue: function(aValue)
  {
      if (this._textbox)
      {
          this._textbox.value = aValue;
      }
  },

  focus: function()
  {
      if (this._textbox)
      {
          this._textbox.focus();
          this._textbox.select();
      }
  },

  onUpdate: function(aEnter)
  {
      if (this.mTimeout)
      {
          clearTimeout(this.mTimeout);
      }
      if (aEnter || typeof(aEnter) == "undefined")
      {
          this.filter();
          
          if (aEnter)
          {
              this.mHistory_append(this._textbox.value);
          }
          this.mTimeout = null;
      }
      else
      {
          this.mTimeout = setTimeout(function() { ConsoleFilter.onUpdate(); }, 500);
      }
  },

  clearFilter: function()
  {
      this._textbox.value = "";
      this.onUpdate(true);
  },

/*** history functionality ***/

  onPopupHistory: function()
  {
      var popup = this._textbox.menupopup;
      
      while (popup.childNodes.length > 0)
      {
          popup.removeChild(popup.childNodes[0]);
      }
      
      for (var i = this.mHistory.length - 1; i >= 0; i--)
      {
          var item = this._textbox.appendItem(this.mHistory[i]);
          item.setAttribute("oncommand", "ConsoleFilter.onUpdate(true);");
          item.setAttribute("onclick", "ConsoleFilter.onClickHistory(this, event);");
      }
      
      item = this._textbox.appendItem(popup.getAttribute("_showall"));
      item.setAttribute("oncommand", "ConsoleFilter.clearFilter();");
  },

  onClickHistory: function(aItem, aEvent)
  {
      if (aEvent.button == 2) // right-click
      {
          this.mHistory_remove(aItem.label);          
          aItem.parentNode.removeChild(aItem);
      }
  },

  mHistory_append: function(aQuery)
  {
      this.mHistory_remove(aQuery);
      
      if (/\S/.test(aQuery))
      {
          this.mHistory.push(aQuery);
      }
      while (this.mHistory.length > this._limit)
      {
          this.mHistory.shift();
      }
  },

  mHistory_remove: function(aQuery)
  {
      for (var i = this.mHistory.length - 1; i >= 0; i--)
      {
          if (this.mHistory[i] == aQuery)
          {
              this.mHistory.splice(i, 1);
          }
      }
  },

/*** monkey patches (as long as necessary) ***/

  _addCategoriesToConsole: function()
  { // have the console add each message's category to its row
      var console = document.getElementById("ConsoleBox");
      
      if (/aObject\.category/.test(console.appendError))
      { // Bug 306223 seems to have been fixed
          return;
      }
      
      console.__preCF_appendError = console.appendError;
      console.appendError = function(aObject)
      {
          this.__preCF_appendError(aObject);
          
          var row = this.mConsoleRowBox.lastChild;
          row.setAttribute("category", aObject.category || "");
      }; // see Bug 264162 for the idea
      
      // reload all messages with our categorization method above
      var newRows = console.mConsoleRowBox.cloneNode(false);
      console.mConsoleRowBox.parentNode.replaceChild(newRows, console.mConsoleRowBox);
      console.mConsoleRowBox = newRows;
      console.mCount = 0;
      console.appendInitialItems();
  }
};

window.addEventListener("load", function() {
  if (!ConsoleFilter._loaded)
  {
      ConsoleFilter.onLoad();
      ConsoleFilter._loaded = true;
  }
}, false);

window.addEventListener("unload", function() {
  if (ConsoleFilter._loaded)
  {
      ConsoleFilter.onUnload();
      ConsoleFilter._loaded = false;
  }
}, false);
