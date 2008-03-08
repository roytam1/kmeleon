const pageLoaderIface = Components.interfaces.nsIWebPageDescriptor;
const nsISelectionPrivate = Components.interfaces.nsISelectionPrivate;
const nsISelectionController = Components.interfaces.nsISelectionController;
var gBrowser = null;
var gViewSourceBundle = null;
var gPrefs = null;
var gLastLineFound = '';
var gGoToLine = 0;
var loaded = false;
try {
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  gPrefs = prefService.getBranch(null);
} catch (ex) {
}
var gSelectionListener = {
  timeout: 0,
  notifySelectionChanged: function(doc, sel, reason)
  {
    if (!this.timeout)
      this.timeout = setTimeout(updateStatusBar, 100);
  }
}
var query_keys = new Array();
var query_values = new Array();
function querySearch(key) {
   var value = '', n;
   for (n = 0; n < query_keys.length; n++) {
      if (query_keys[n] == key) {
         value = query_values[n];
         break;
      }
   }
   return unescape(value);
}
function queryParse() {
   var query = window.location.search;
   query = query.substring(1);
   var pairs = query.split("&"), n, pos;
   for (n = 0; n < pairs.length; n++) {
      pos = pairs[n].indexOf('=');
      if (pos >= 0) {
         query_keys[query_keys.length] = pairs[n].substring(0, pos);
         query_values[query_values.length] = pairs[n].substring(pos + 1);
      }
   }
}
function onLoadViewSource() 
{
document.title="View Source";
  if (!loaded) {
    if (!window.arguments) {
      window.arguments = new Array(4);
      queryParse();
      if (querySearch("url")) {
      	window.arguments[0] = querySearch("url");
      }
      if (querySearch("ch")) {
      	window.arguments[1] = querySearch("ch");
      }
      if (querySearch("ca")) {
      	window.arguments[2] = querySearch("ca");
      }
      if (querySearch("ln")) {
      	window.arguments[3] = querySearch("ln");
      }
    }
    viewSource(window.arguments[0]);
    document.commandDispatcher.focusedWindow = content;
    loaded = true;
  }
}
function getBrowser()
{
  if (!gBrowser)
    gBrowser = document.getElementById("content");
  return gBrowser;
}
function getSelectionController()
{
  return getBrowser().docShell
    .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    .getInterface(Components.interfaces.nsISelectionDisplay)
    .QueryInterface(nsISelectionController);
}
function getViewSourceBundle()
{
  if (!gViewSourceBundle)
    gViewSourceBundle = document.getElementById("viewSourceBundle");
  return gViewSourceBundle;
}
function viewSource(url)
{
  if (!loaded) {
    if (!url)
      return false; // throw Components.results.NS_ERROR_FAILURE;
    getBrowser().addEventListener("unload", onUnloadContent, true);
    getBrowser().addEventListener("load", onLoadContent, true);
    var loadFromURL = true;
    if ("arguments" in window) {
      var arg;
      if (window.arguments.length >= 2) {
        arg = window.arguments[1];
        try {
          if (typeof(arg) == "string" && arg.indexOf('charset=') != -1) {
            var arrayArgComponents = arg.split('=');
            if (arrayArgComponents) {
              getMarkupDocumentViewer().defaultCharacterSet = arrayArgComponents[1];
            } 
          }
        } catch (ex) {
        }
      }
      if (window.arguments.length >= 4) {
        arg = window.arguments[3];
        gGoToLine = parseInt(arg);
      }
      if (window.arguments.length >= 3) {
        arg = window.arguments[2];
        try {
          if (typeof(arg) == "object" && arg != null) {
            var PageLoader = getBrowser().webNavigation.QueryInterface(pageLoaderIface);
            PageLoader.LoadPage(arg, pageLoaderIface.DISPLAY_AS_SOURCE);
            loadFromURL = false;
          }
        } catch(ex) {
        }
      }
    }
    if (loadFromURL) {
      var loadFlags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
      var viewSrcUrl = "view-source:" + url;
      getBrowser().webNavigation.loadURI(viewSrcUrl, loadFlags, null, null, null);
    }
    if (gPrefs) {
      try {
        var wraplonglinesPrefValue = gPrefs.getBoolPref("view_source.wrap_long_lines");
        if (wraplonglinesPrefValue)
          document.getElementById('menu_wrapLongLines').setAttribute("checked", "true");
      } catch (ex) {
      }
      try {
        document.getElementById("menu_highlightSyntax").setAttribute("checked", gPrefs.getBoolPref("view_source.syntax_highlight"));
      } catch (ex) {
      }
    } else {
      document.getElementById("menu_highlightSyntax").setAttribute("hidden", "true");
    }
    window._content.focus();
  }
  return true;
}
function onLoadContent()
{
  if (gGoToLine > 0) {
    goToLine(gGoToLine);
    gGoToLine = 0;
  }
  document.getElementById('cmd_goToLine').removeAttribute('disabled');
  window._content.getSelection()
   .QueryInterface(nsISelectionPrivate)
   .addSelectionListener(gSelectionListener);
}
function onUnloadContent()
{
  document.getElementById('cmd_goToLine').setAttribute('disabled', 'true');
}
function ViewSourceClose()
{
  window.close();
}
function BrowserReload()
{
  BrowserReloadSkipCache();
}
function BrowserReloadSkipCache()
{
  const webNavigation = getBrowser().webNavigation;
  webNavigation.reload(webNavigation.LOAD_FLAGS_BYPASS_PROXY | webNavigation.LOAD_FLAGS_BYPASS_CACHE);
}
function ViewSourceEditPage()
{
  editPage(window.content.location.href.substring(12), window, false);
}
function ViewSourceSavePage()
{
  saveURL(window.content.location.href.substring(12), null, "SaveLinkTitle");
}
function ViewSourceGoToLine()
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
        .getService(Components.interfaces.nsIPromptService);
  var viewSourceBundle = getViewSourceBundle();
  var input = {value:gLastLineFound};
  for (;;) {
    var ok = promptService.prompt(
        window,
        viewSourceBundle.getString("goToLineTitle"),
        viewSourceBundle.getString("goToLineText"),
        input,
        null,
        {value:0});
    if (!ok) return;
    var line = parseInt(input.value);
    if (!(line > 0)) {
      promptService.alert(window,
          viewSourceBundle.getString("invalidInputTitle"),
          viewSourceBundle.getString("invalidInputText"));
      continue;
    }
    var found = goToLine(line);
    if (found) {
      break;
    }
    promptService.alert(window,
        viewSourceBundle.getString("outOfRangeTitle"),
        viewSourceBundle.getString("outOfRangeText"));
  }
}
function goToLine(line)
{
  var viewsource = window._content.document.body;
  var pre;
  for (var lbound = 0, ubound = viewsource.childNodes.length; ; ) {
    var middle = (lbound + ubound) >> 1;
    pre = viewsource.childNodes[middle];
    var firstLine = parseInt(pre.id.substring(4));
    if (lbound == ubound - 1) {
      break;
    }
    if (line >= firstLine) {
      lbound = middle;
    } else {
      ubound = middle;
    }
  }
  var result = {};
  var found = findLocation(pre, line, null, -1, false, result);
  if (!found) {
    return false;
  }
  var selection = window._content.getSelection();
  selection.removeAllRanges();
  selection.QueryInterface(nsISelectionPrivate)
    .interlinePosition = true;	
  selection.addRange(result.range);
  if (!selection.isCollapsed) {
    selection.collapseToEnd();
    var offset = result.range.startOffset;
    var node = result.range.startContainer;
    if (offset < node.data.length) {
      selection.extend(node, offset);
    }
    else {
      node = node.nextSibling ? node.nextSibling : node.parentNode.nextSibling;
      selection.extend(node, 0);
    }
  }
  var selCon = getSelectionController();
  selCon.setDisplaySelection(nsISelectionController.SELECTION_ON);
  selCon.setCaretEnabled(true);
  selCon.setCaretVisibilityDuringSelection(true);
  selCon.scrollSelectionIntoView(
    nsISelectionController.SELECTION_NORMAL,
    nsISelectionController.SELECTION_FOCUS_REGION,
    true);
  gLastLineFound = line;
  document.getElementById("statusbar-line-col").label = getViewSourceBundle()
      .getFormattedString("statusBarLineCol", [line, 1]);
  return true;
}
function updateStatusBar()
{
  gSelectionListener.timeout = 0;
  var statusBarField = document.getElementById("statusbar-line-col");
  var selection = window._content.getSelection();
  if (!selection.focusNode) {
    statusBarField.label = '';
    return;
  }
  if (selection.focusNode.nodeType != Node.TEXT_NODE) {
    return;
  }
  var selCon = getSelectionController();
  selCon.setDisplaySelection(nsISelectionController.SELECTION_ON);
  selCon.setCaretEnabled(true);
  selCon.setCaretVisibilityDuringSelection(true);
  var interlinePosition = selection
      .QueryInterface(nsISelectionPrivate).interlinePosition;
  var result = {};
  findLocation(null, -1, 
      selection.focusNode, selection.focusOffset, interlinePosition, result);
  statusBarField.label = getViewSourceBundle()
      .getFormattedString("statusBarLineCol", [result.line, result.col]);
}
function findLocation(pre, line, node, offset, interlinePosition, result)
{
  if (node && !pre) {
    for (pre = node;
         pre.nodeName != "PRE";
         pre = pre.parentNode);
  }
  var curLine = parseInt(pre.id.substring(4));
  var treewalker = window._content.document
      .createTreeWalker(pre, NodeFilter.SHOW_TEXT, null, false);
  var firstCol = 1;
  var found = false;
  for (var textNode = treewalker.firstChild();
       textNode && !found;
       textNode = treewalker.nextNode()) {
    var lineArray = textNode.data.split(/\n/);
    var lastLineInNode = curLine + lineArray.length - 1;
    if (node ? (textNode != node) : (lastLineInNode < line)) {
      if (lineArray.length > 1) {
        firstCol = 1;
      }
      firstCol += lineArray[lineArray.length - 1].length;
      curLine = lastLineInNode;
      continue;
    }
    for (var i = 0, curPos = 0;
         i < lineArray.length;
         curPos += lineArray[i++].length + 1) {
      if (i > 0) {
        curLine++;
      }
      if (node) {
        if (offset >= curPos && offset <= curPos + lineArray[i].length) {
          if (i > 0 && offset == curPos && !interlinePosition) {
            result.line = curLine - 1;
            var prevPos = curPos - lineArray[i - 1].length;
            result.col = (i == 1 ? firstCol : 1) + offset - prevPos;
          } else {
            result.line = curLine;
            result.col = (i == 0 ? firstCol : 1) + offset - curPos;
          }
          found = true;
          break;
        }
      } else {
        if (curLine == line && !("range" in result)) {
          result.range = document.createRange();
          result.range.setStart(textNode, curPos);
          result.range.setEndAfter(pre.lastChild);
        } else if (curLine == line + 1) {
          result.range.setEnd(textNode, curPos - 1);
          found = true;
          break;
        }
      }
    }
  }
  return found;
}
function wrapLongLines()
{
  var myWrap = window._content.document.body;
  if (myWrap.className == '')
    myWrap.className = 'wrap';
  else myWrap.className = '';
  if (gPrefs){
    try {
      if (myWrap.className == '') {
        gPrefs.setBoolPref("view_source.wrap_long_lines", false);
      }
      else {
        gPrefs.setBoolPref("view_source.wrap_long_lines", true);
      }
    } catch (ex) {
    }
  }
}
function highlightSyntax()
{
  var highlightSyntaxMenu = document.getElementById("menu_highlightSyntax");
  var highlightSyntax = (highlightSyntaxMenu.getAttribute("checked") == "true");
  gPrefs.setBoolPref("view_source.syntax_highlight", highlightSyntax);
  var PageLoader = getBrowser().webNavigation.QueryInterface(pageLoaderIface);
  PageLoader.LoadPage(PageLoader.currentDescriptor, pageLoaderIface.DISPLAY_NORMAL);
}
function BrowserSetForcedCharacterSet(aCharset)
{
  var docCharset = getBrowser().docShell.QueryInterface(
                            Components.interfaces.nsIDocCharset);
  docCharset.charset = aCharset;
  var PageLoader = getBrowser().webNavigation.QueryInterface(pageLoaderIface);
  PageLoader.LoadPage(PageLoader.currentDescriptor, pageLoaderIface.DISPLAY_NORMAL);
}
