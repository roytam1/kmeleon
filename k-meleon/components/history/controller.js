/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource:///modules/KMeleon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil", "resource://gre/modules/NetUtil.jsm");
// When removing a bunch of pages we split them in chunks to give some breath
// to the main-thread.
const REMOVE_PAGES_CHUNKLEN = 300;
/**
 * History Controller
 */
function HistoryController(aView) {
	this._view = aView;
	XPCOMUtils.defineLazyServiceGetter(this, "clipboard", "@mozilla.org/widget/clipboard;1", "nsIClipboard");
	XPCOMUtils.defineLazyServiceGetter(this, "hsvc", "@mozilla.org/browser/nav-history-service;1", "nsINavHistoryService");
	XPCOMUtils.defineLazyGetter(this, "profileName", function () {
		return Services.dirsvc.get("ProfD", Ci.nsIFile).leafName;
	});
	this._cachedLivemarkInfoObjects = new Map();
}
HistoryController.prototype = {
	/**
	 * The History view.
	 */
	_view : null,
	QueryInterface : XPCOMUtils.generateQI([Ci.nsIClipboardOwner]),
	terminate: function PC_terminate() {
		this._releaseClipboardOwnership();
	},
	
	isCommandEnabled : function PC_isCommandEnabled(aCommand) {
		switch (aCommand) {
		case "historyCmd_open":
		case "historyCmd_opentab":
		case "historyCmd_opentabback":
		case "historyCmd_copylink":
		case "historyCmd_openwindow":
		case "historyCmd_openbackwindow":
			var selectedNode = this._view.selectedNode;
			return selectedNode && PlacesUtils.nodeIsURI(selectedNode);
		case "historyCmd_delete":

		case "historyCmd_deleteall":
			return this.hsvc.hasHistoryEntries;
		default:
			return false;
		}
	},
	doCommand : function PC_doCommand(aCommand) {
		switch (aCommand) {
		case "historyCmd_open":
                        KMeleon.Open(this._view.selectedNode.uri, KMeleon.OPEN_NORMAL);
			break;
		case "historyCmd_opentab":
                        KMeleon.Open(this._view.selectedNode.uri, KMeleon.OPEN_NEWTAB);
			break;
		case "historyCmd_opentabback":
			KMeleon.Open(this._view.selectedNode.uri, KMeleon.OPEN_BACKGROUNDTAB);
			break;
		case "historyCmd_openwindow":
                        KMeleon.Open(this._view.selectedNode.uri, KMeleon.OPEN_NEW);
			break;
		case "historyCmd_openbackwindow":
			KMeleon.Open(this._view.selectedNode.uri, KMeleon.OPEN_BACKGROUND);
			break;	
		case "historyCmd_delete":
			this.remove("Remove Selection");
			break;
		case "historyCmd_deleteall":
			this.hsvc.removeAllPages();
			break;
		case "historyCmd_copylink":
                        var clip = Cc['@mozilla.org/widget/clipboardhelper;1'].
                                getService(Ci.nsIClipboardHelper);
                        clip.copyString(this._view.selectedNode.uri, document);                        
                        break;
		}
	},	
	/**
	 * Determine whether or not the selection can be removed, either by the
	 * delete or cut operations based on whether or not any of its contents
	 * are non-removable. We don't need to worry about recursion here since it
	 * is a policy decision that a removable item not be placed inside a non-
	 * removable item.
	 * @param aIsMoveCommand
	 *        True if the command for which this method is called only moves the
	 *        selected items to another container, false otherwise.
	 * @returns true if all nodes in the selection can be removed,
	 *          false otherwise.
	 */
	_hasRemovableSelection : function PC__hasRemovableSelection(aIsMoveCommand) {
		var ranges = this._view.removableSelectionRanges;
		if (!ranges.length)
			return false;
		var root = this._view.result.root;
		for (var j = 0; j < ranges.length; j++) {
			var nodes = ranges[j];
			for (var i = 0; i < nodes.length; ++i) {
				// Disallow removing the view's root node
				if (nodes[i] == root)
					return false;
				if (PlacesUtils.nodeIsFolder(nodes[i]) &&
					!PlacesControllerDragHelper.canMoveNode(nodes[i]))
					return false;
				// We don't call nodeIsReadOnly here, because nodeIsReadOnly means that
				// a node has children that cannot be edited, reordered or removed. Here,
				// we don't care if a node's children can't be reordered or edited, just
				// that they're removable. All history results have removable children
				// (based on the principle that any URL in the history table should be
				// removable), but some special bookmark folders may have non-removable
				// children, e.g. live bookmark folder children. It doesn't make sense
				// to delete a child of a live bookmark folder, since when the folder
				// refreshes, the child will return.
				var parent = nodes[i].parent || root;
				//if (PlacesUtils.isReadonlyFolder(parent))
				//	return false;
			}
		}
		return true;
	},
	/**
	 * Walk the list of folders we're removing in this delete operation, and
	 * see if the selected node specified is already implicitly being removed
	 * because it is a child of that folder.
	 * @param   node
	 *          Node to check for containment.
	 * @param   pastFolders
	 *          List of folders the calling function has already traversed
	 * @returns true if the node should be skipped, false otherwise.
	 */
	_shouldSkipNode : function PC_shouldSkipNode(node, pastFolders) {
		/**
		 * Determines if a node is contained by another node within a resultset.
		 * @param   node
		 *          The node to check for containment for
		 * @param   parent
		 *          The parent container to check for containment in
		 * @returns true if node is a member of parent's children, false otherwise.
		 */
		function isContainedBy(node, parent) {
			var cursor = node.parent;
			while (cursor) {
				if (cursor == parent)
					return true;
				cursor = cursor.parent;
			}
			return false;
		}
		for (var j = 0; j < pastFolders.length; ++j) {
			if (isContainedBy(node, pastFolders[j]))
				return true;
		}
		return false;
	},
	/**
	 * Creates a set of transactions for the removal of a range of items.
	 * A range is an array of adjacent nodes in a view.
	 * @param   [in] range
	 *          An array of nodes to remove. Should all be adjacent.
	 * @param   [out] transactions
	 *          An array of transactions.
	 * @param   [optional] removedFolders
	 *          An array of folder nodes that have already been removed.
	 */
	_removeRange : function PC__removeRange(range, transactions, removedFolders) {
		NS_ASSERT(transactions instanceof Array, "Must pass a transactions array");
		if (!removedFolders)
			removedFolders = [];
		for (var i = 0; i < range.length; ++i) {
			var node = range[i];
			if (this._shouldSkipNode(node, removedFolders))
				continue;
			if (PlacesUtils.nodeIsTagQuery(node.parent)) {
				// This is a uri node inside a tag container.  It needs a special
				// untag transaction.
				var tagItemId = PlacesUtils.getConcreteItemId(node.parent);
				var uri = NetUtil.newURI(node.uri);
				let txn = new PlacesUntagURITransaction(uri, [tagItemId]);
				transactions.push(txn);
			} else if (PlacesUtils.nodeIsTagQuery(node) && node.parent &&
				PlacesUtils.nodeIsQuery(node.parent) &&
				PlacesUtils.asQuery(node.parent).queryOptions.resultType ==
				Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY) {
				// This is a tag container.
				// Untag all URIs tagged with this tag only if the tag container is
				// child of the "Tags" query in the library, in all other places we
				// must only remove the query node.
				var tag = node.title;
				var URIs = PlacesUtils.tagging.getURIsForTag(tag);
				for (var j = 0; j < URIs.length; j++) {
					let txn = new PlacesUntagURITransaction(URIs[j], [tag]);
					transactions.push(txn);
				}
			} else if (PlacesUtils.nodeIsURI(node) &&
				PlacesUtils.nodeIsQuery(node.parent) &&
				PlacesUtils.asQuery(node.parent).queryOptions.queryType ==
				Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
				// This is a uri node inside an history query.
				PlacesUtils.bhistory.removePage(NetUtil.newURI(node.uri));
				// History deletes are not undoable, so we don't have a transaction.
			} else if (node.itemId == -1 &&
				PlacesUtils.nodeIsQuery(node) &&
				PlacesUtils.asQuery(node).queryOptions.queryType ==
				Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
				// This is a dynamically generated history query, like queries
				// grouped by site, time or both.  Dynamically generated queries don't
				// have an itemId even if they are descendants of a bookmark.
				this._removeHistoryContainer(node);
				// History deletes are not undoable, so we don't have a transaction.
			} else {
				// This is a common bookmark item.
				if (PlacesUtils.nodeIsFolder(node)) {
					// If this is a folder we add it to our array of folders, used
					// to skip nodes that are children of an already removed folder.
					removedFolders.push(node);
				}
				let txn = new PlacesRemoveItemTransaction(node.itemId);
				transactions.push(txn);
			}
		}
	},
	/**
	 * Removes the set of selected ranges from bookmarks.
	 * @param   txnName
	 *          See |remove|.
	 */
	_removeRowsFromBookmarks : function PC__removeRowsFromBookmarks(txnName) {
		var ranges = this._view.removableSelectionRanges;
		var transactions = [];
		var removedFolders = [];
		for (var i = 0; i < ranges.length; i++)
			this._removeRange(ranges[i], transactions, removedFolders);
		if (transactions.length > 0) {
			var txn = new PlacesAggregatedTransaction(txnName, transactions);
			PlacesUtils.transactionManager.doTransaction(txn);
		}
	},
	/**
	 * Removes the set of selected ranges from history.
	 *
	 * @note history deletes are not undoable.
	 */
	_removeRowsFromHistory : function PC__removeRowsFromHistory() {
		let nodes = this._view.selectedNodes;
		let URIs = [];
		for (let i = 0; i < nodes.length; ++i) {
			let node = nodes[i];
			if (PlacesUtils.nodeIsURI(node)) {
				let uri = NetUtil.newURI(node.uri);
				// Avoid duplicates.
				if (URIs.indexOf(uri) < 0) {
					URIs.push(uri);
				}
			} else if (PlacesUtils.nodeIsQuery(node) &&
				PlacesUtils.asQuery(node).queryOptions.queryType ==
				Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
				this._removeHistoryContainer(node);
			}
		}
		// Do removal in chunks to give some breath to main-thread.
		function pagesChunkGenerator(aURIs) {
			while (aURIs.length) {
				let URIslice = aURIs.splice(0, REMOVE_PAGES_CHUNKLEN);
				PlacesUtils.bhistory.removePages(URIslice, URIslice.length);
				Services.tm.mainThread.dispatch(function () {
					try {
						gen.next();
					} catch (ex if ex instanceof StopIteration) {}
				}, Ci.nsIThread.DISPATCH_NORMAL);
				yield undefined;
			}
		}
		let gen = pagesChunkGenerator(URIs);
		gen.next();
	},
	/**
	 * Removes history visits for an history container node.
	 * @param   [in] aContainerNode
	 *          The container node to remove.
	 *
	 * @note history deletes are not undoable.
	 */
	_removeHistoryContainer : function PC__removeHistoryContainer(aContainerNode) {
		if (PlacesUtils.nodeIsHost(aContainerNode)) {
			// Site container.
			PlacesUtils.bhistory.removePagesFromHost(aContainerNode.title, true);
		} else if (PlacesUtils.nodeIsDay(aContainerNode)) {
			// Day container.
			let query = aContainerNode.getQueries()[0];
			let beginTime = query.beginTime;
			let endTime = query.endTime;
			NS_ASSERT(query && beginTime && endTime,
				"A valid date container query should exist!");
			// We want to exclude beginTime from the removal because
			// removePagesByTimeframe includes both extremes, while date containers
			// exclude the lower extreme.  So, if we would not exclude it, we would
			// end up removing more history than requested.
			PlacesUtils.bhistory.removePagesByTimeframe(beginTime + 1, endTime);
		}
	},
	/**
	 * Removes the selection
	 * @param   aTxnName
	 *          A name for the transaction if this is being performed
	 *          as part of another operation.
	 */
	remove : function PC_remove(aTxnName) {
		if (!this._hasRemovableSelection(false))
			return;
		NS_ASSERT(aTxnName !== undefined, "Must supply Transaction Name");
		var root = this._view.result.root;
		if (PlacesUtils.nodeIsFolder(root))
			this._removeRowsFromBookmarks(aTxnName);
		else if (PlacesUtils.nodeIsQuery(root)) {
			var queryType = PlacesUtils.asQuery(root).queryOptions.queryType;
			if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS)
				this._removeRowsFromBookmarks(aTxnName);
			else if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY)
				this._removeRowsFromHistory();
			else
				NS_ASSERT(false, "implement support for QUERY_TYPE_UNIFIED");
		} else
			NS_ASSERT(false, "unexpected root");
	},
	
	_releaseClipboardOwnership: function PC__releaseClipboardOwnership() {
          /*if (this.cutNodes.length > 0) {
            // This clears the logical clipboard, doesn't remove data.
            this.clipboard.emptyClipboard(Ci.nsIClipboard.kGlobalClipboard);
          }*/
        },
	
	/**
	 * Returns whether or not there's cached mozILivemarkInfo object for a node.
	 * @param aNode
	 *        a places result node.
	 * @return true if there's a cached mozILivemarkInfo object for
	 * aNode, false otherwise.
	 */
	hasCachedLivemarkInfo : function PC_hasCachedLivemarkInfo(aNode)
	this._cachedLivemarkInfoObjects.has(aNode)
};
function fillInBHTooltip(aDocument, aEvent) {
    var node;
    var cropped = false;
    var targetURI;

    if (aDocument.tooltipNode.localName == "treechildren") {
      var tree = aDocument.tooltipNode.parentNode;
      var row = {}, column = {};
      var tbo = tree.treeBoxObject;
      tbo.getCellAt(aEvent.clientX, aEvent.clientY, row, column, {});
      if (row.value == -1)
        return false;
      node = tree.view.nodeForTreeIndex(row.value);
      cropped = tbo.isCellCropped(row.value, column.value);
    }
    else {
      // Check whether the tooltipNode is a Places node.
      // In such a case use it, otherwise check for targetURI attribute.
      var tooltipNode = aDocument.tooltipNode;
      if (tooltipNode._placesNode)
        node = tooltipNode._placesNode;
      else {
        // This is a static non-Places node.
        targetURI = tooltipNode.getAttribute("targetURI");
      }
    }

    if (!node && !targetURI)
      return false;

    // Show node.label as tooltip's title for non-Places nodes.
    var title = node ? node.title : tooltipNode.label;

    // Show URL only for Places URI-nodes or nodes with a targetURI attribute.
    var url;
    if (targetURI || PlacesUtils.nodeIsURI(node))
      url = targetURI || node.uri;

    // Show tooltip for containers only if their title is cropped.
    if (!cropped && !url)
      return false;

    var tooltipTitle = aDocument.getElementById("bhtTitleText");
    tooltipTitle.hidden = (!title || (title == url));
    if (!tooltipTitle.hidden)
      tooltipTitle.textContent = title;

    var tooltipUrl = aDocument.getElementById("bhtUrlText");
    tooltipUrl.hidden = !url;
    if (!tooltipUrl.hidden)
      tooltipUrl.value = url;

    // Show tooltip.
    return true;
  }
function goUpdateHistoryCommands() {
	// Get the controller for one of the history commands.
	var historyController = doGetHistoryControllerForCommand("historyCmd_open");
	function updateHistoryCommand(aCommand) {
		goSetCommandEnabled(aCommand, historyController && historyController.isCommandEnabled(aCommand));
	}
	updateHistoryCommand("historyCmd_open");
	updateHistoryCommand("historyCmd_opentab");
	updateHistoryCommand("historyCmd_opentabback");
	updateHistoryCommand("historyCmd_delete");
	updateHistoryCommand("historyCmd_deleteall");
}
function doGetHistoryControllerForCommand(aCommand) {
	// A context menu may be built for non-focusable views.  Thus, we first try
	// to look for a view associated with document.popupNode
	let popupNode;
	try {
		popupNode = document.popupNode;
	} catch (e) {
		// The document went away (bug 797307).
		return null;
	}
	if (popupNode) {
		let view = PlacesUIUtils.getViewForNode(popupNode);
		if (view && view._contextMenuShown)
			return view.controllers.getControllerForCommand(aCommand);
	}
	// When we're not building a context menu, only focusable views
	// are possible.  Thus, we can safely use the command dispatcher.
	let controller = top.document.commandDispatcher.getControllerForCommand(aCommand);
	if (controller)
		return controller;
	return null;
}
function goDoHistoryCommand(aCommand) {
	let controller = doGetHistoryControllerForCommand(aCommand);
	if (controller && controller.isCommandEnabled(aCommand))
		controller.doCommand(aCommand);
}