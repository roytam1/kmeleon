const Cc = Components.classes;
const Ci = Components.interfaces;

var gTree = null;
var gEntries = [];
var gBlacklist = null;

/* :::::::: Blocklist Initialization ::::::::::::::: */

function BlocklistStartUp()
{
  gTree = _("domains");
  var sorting = gTree.getAttribute("sorting").match(/^(~?)(.*)$/);
  gTreeView.mSortColumn = sorting[2];
  gTreeView.mSortReversed = !sorting[1];
  sortByColumn(sorting[2] || "domain");

  if ("arguments" in window && window.arguments[0]) {
    gBlacklist = window.arguments[0];
    for (var i in gBlacklist.mBlocking) {
      gEntries.push({domain: i});
    }
  }

  gTree.treeBoxObject.view = gTreeView;
  gTree.onkeydown = onTreeKeyDown;
  gTree.onselect = onTreeSelect;
  gTree.firstChild.nextSibling.ondblclick = onTreeDblClick;

  sortTreeInternal();

  _("uri").focus();
}

function BlocklistShutDown()
{
//  Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService).removeObserver(gChangeObserver, "perm-changed");
  for (var i in gBlacklist.mBlocking)
    delete gBlacklist.mBlocking[i];

  for (var j in gEntries) {
    gBlacklist.mBlocking[ gEntries[j].domain ] = true;
  }
  gBlacklist.saveBlocklist();
}


/* :::::::: Domainlist UI Functions ::::::::::::::: */

function onURIInput()
{
  var host = _("uri").value.replace(/^\./, "");
  //_("block").disabled = ! (host && gEntries.indexOf({domain: host}) == -1);
  _("block").disabled = !host;
}

function onTreeKeyDown(aEvent)
{
  if (aEvent.keyCode == aEvent.DOM_VK_DELETE)
  {
    removeDomain();
  }
  else if (aEvent.keyCode == aEvent.DOM_VK_A && aEvent.ctrlKey)
  {
    gTree.view.selection.selectAll();
  }
}

function onTreeSelect()
{
  document.documentElement.getButton("extra2").disabled = gTreeView.selection.count == 0 || gTreeView.rowCount == 0;
}

function onTreeDblClick()
{
  _("uri").value = gEntries[gTree.view.selection.currentIndex].domain;
  onURIInput();
}

function sortByColumn(aColumn)
{
  gTreeView.mSortReversed = (aColumn == gTreeView.mSortColumn) && !gTreeView.mSortReversed;
  gTreeView.mSortColumn = aColumn;
  gTree.setAttribute("sorting", ((gTreeView.mSortReversed)?"~":"") + aColumn);
  
  var cols = gTree.getElementsByTagName("treecol");
  for (var i = 0; i < cols.length; i++)
  {
    cols[i].removeAttribute("sortActive");
    cols[i].removeAttribute("sortDirection");
  }
  _(aColumn).setAttribute("sortActive", true);
  _(aColumn).setAttribute("sortDirection", (gTreeView.mSortReversed)?"descending":"ascending");
  
  sortTreeInternal();
}

function sortTreeInternal()
{
  var column = gTreeView.mSortColumn;
  gEntries.sort(function (a, b) {
    return a[column].toLowerCase().localeCompare(b[column].toLowerCase());
  });
  if (gTreeView.mSortReversed)
  {
    gEntries.reverse();
  }
  
  gTree.view.selection.select(-1);
  gTree.view.selection.select(0);
  gTree.treeBoxObject.invalidate();
  gTree.treeBoxObject.ensureRowIsVisible(0);
}

function addDomain()
{
  var uri = _("uri");
  
  var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var host = uri.value.replace(/^\s*([\w-]*:)?/, "");
  try
  {
    host = ioService.newURI("http://" + host, null, null).host;
    if (!host)
    {
      throw new Error();
    }
    host = host.replace(/^\./, "");
    if (!gEntries.some(function(element, index, array) {
        return element.domain == host;}))
    {
      gEntries.push({domain: host});
      gTree.treeBoxObject.rowCountChanged(gTreeView.rowCount - 1, 1);
      sortTreeInternal();

      uri.value = "";
      onURIInput();
    }
  }
  catch (ex)
  {
    _("block").disabled = true;
  }
  
  uri.focus();
}

function removeDomain()
{
  if (gEntries.length == 0)
  {
    return;
  }
  
  var selection = gTree.view.selection;
  var currentIndex = selection.currentIndex
  selection.selectedEventsSuppressed = true;

  var removed = [];
  for (var i = selection.getRangeCount() - 1; i >= 0; i--)
  {
    var min = {}, max = {};
    selection.getRangeAt(i, min, max);
    max = max.value - min.value + 1;
    removed = removed.concat(gEntries.splice(min.value, max));
    gTree.treeBoxObject.rowCountChanged(min.value, -max);
  }
  selection.selectEventsSuppressed = false;
  //removed.forEach(function(aEntry) { gPermissionManager.remove(aEntry.host, aEntry.type); });
  
  if (gEntries.length > 0)
  {
    currentIndex = Math.min(currentIndex, gEntries.length - 1);
    selection.select(currentIndex);
    gTree.treeBoxObject.ensureRowIsVisible(currentIndex);
    gTree.focus();
  }
  else
  {
    document.documentElement.getButton("extra2").disabled = true;
    _("uri").focus();
  }
}

/* :::::::: Domainlist State Objects ::::::::::::::: */

var gTreeView = {
  mSortColumn: "",
  mSortReversed: false,

  get rowCount() { return gEntries.length; },
  getCellText: function(aRow, aColumn) { return gEntries[aRow][aColumn.id] || ""; },
  isSorted: function() { return true; },

  isSeparator: function(aIndex) { return false; },
  isContainer: function(aIndex) { return false; },
  setTree: function(treebox){ this.treebox = treebox; },
  getImageSrc: function(aRow, aColumn) { },
  getProgressMode: function(aRow, aColumn) { },
  getCellValue: function(aRow, aColumn) { },
  cycleHeader: function(column) { },
  getRowProperties: function(row, prop) { },
  getColumnProperties: function(column, prop) { },
  getCellProperties: function(row, column, prop) { }
};

/* :::::::: Domainlist Utility Functions ::::::::::::::: */

function _(aID)
{
  return document.getElementById(aID);
}
