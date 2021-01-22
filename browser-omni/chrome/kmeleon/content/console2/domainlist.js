const Cc = Components.classes;
const Ci = Components.interfaces;

const ALLOW_DOMAIN = Ci.nsIPermissionManager.ALLOW_ACTION;
const BLOCK_DOMAIN = Ci.nsIPermissionManager.DENY_ACTION;
const gPermissionManager = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);

var gTree = null;
var gEntries = [];
var gCapabilityStrings = {};


/* :::::::: Domainlist Initialization ::::::::::::::: */

function DomainlistStartUp()
{
	Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService).addObserver(gChangeObserver, "perm-changed", false);
	
	gCapabilityStrings[ALLOW_DOMAIN] = _("allow").label;
	gCapabilityStrings[BLOCK_DOMAIN] = _("block").label;
	
	gTree = _("domains");
	var sorting = gTree.getAttribute("sorting").match(/^(~?)(.*)$/);
	gTreeView.mSortColumn = sorting[2];
	gTreeView.mSortReversed = !sorting[1];
	sortByColumn(sorting[2] || "domain");
	
	var enu = gPermissionManager.enumerator;
	while (enu.hasMoreElements())
	{
		addDomainInternal(enu.getNext().QueryInterface(Ci.nsIPermission));
	}
	
	gTree.treeBoxObject.view = gTreeView;
	gTree.onkeydown = onTreeKeyDown;
	gTree.onselect = onTreeSelect;
	gTree.firstChild.nextSibling.ondblclick = onTreeDblClick;
	
	sortTreeInternal();
	
	_("uri").focus();
}

function DomainlistShutDown()
{
	Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService).removeObserver(gChangeObserver, "perm-changed");
}


/* :::::::: Domainlist UI Functions ::::::::::::::: */

function onURIInput()
{
	_("block").disabled = _("allow").disabled = !_("uri").value;
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
	_("uri").value = gEntries[gTree.view.selection.currentIndex].host;
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

function addDomain(aCapability)
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
		gPermissionManager.add(ioService.newURI("http://" + host.replace(/^\./, ""), null, null), "console2", aCapability);
		
		uri.value = "";
		onURIInput();
	}
	catch (ex)
	{
		_("block").disabled = _("allow").disabled = true;
	}
	
	uri.focus();
}

function addDomainInternal(aPermission)
{
	if (aPermission.type == "console2")
	{
		gEntries.push({
			host: aPermission.host,
			domain: aPermission.host.replace(/^\./, ""),
			type: aPermission.type,
			status: gCapabilityStrings[aPermission.capability] || null,
			perm: aPermission.capability
		});
	}
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
	removed.forEach(function(aEntry) { gPermissionManager.remove(aEntry.host, aEntry.type); });
	
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


/* :::::::: Domainlist State Objects ::::::::::::::: */

const gTreeView = {
	mSortColumn: "",
	mSortReversed: false,

	get rowCount() { return gEntries.length; },
	getCellText: function(aRow, aColumn) { return gEntries[aRow][aColumn.id] || ""; },
	isSorted: function() { return true; },

	isSeparator: function(aIndex) { return false; },
	isContainer: function(aIndex) { return false; },
	setTree: function(aTree) { },
	getImageSrc: function(aRow, aColumn) { },
	getProgressMode: function(aRow, aColumn) { },
	getCellValue: function(aRow, aColumn) { },
	cycleHeader: function(column) { },
	getRowProperties: function(row, prop) { },
	getColumnProperties: function(column, prop) { },
	getCellProperties: function(row, column, prop) { }
};

const gChangeObserver = {
	observe: function(aSubject, aTopic, aData)
	{
		if (aTopic == "perm-changed")
		{
			var permission = aSubject.QueryInterface(Ci.nsIPermission);
			if (permission.type != "console2")
			{
				return;
			}
			
			switch (aData)
			{
			case "added":
				addDomainInternal(permission);
				gTree.treeBoxObject.rowCountChanged(gTreeView.rowCount - 1, 1);
				sortTreeInternal();
				break;
			case "changed":
			case "deleted":
				for (var i = 0; i < gEntries.length; i++)
				{
					if (gEntries[i].host == permission.host)
					{
						if (aData == "changed")
						{
							gEntries[i].status = gCapabilityStrings[permission.capability] || null;
							gEntries[i].perm = permission.capability;
						}
						else
						{
							gEntries.splice(i, 1);
							gTree.treeBoxObject.rowCountChanged(i, -1);
						}
						break;
					}
				}
				if (gTreeView.mSortColumn == "status")
				{
					sortTreeInternal();
				}
				else
				{
					gTree.treeBoxObject.invalidate();
				}
				break;
			}
		}
	}
};


/* :::::::: Domainlist Utility Functions ::::::::::::::: */

function _(aID)
{
	return document.getElementById(aID);
}
