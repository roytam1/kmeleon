/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is bookmarksHome.
 *
 * The Initial Developer of the Original Code is
 * Jeroen Groenenboom <jagrboom@zonnet.nl>
 * Portions created by the Initial Developer are Copyright (C) 2004-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Andrey Gromyko <andrey@gromyko.name>
 *   Ron Pruitt <wa84it@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

function lifemarkData(title, URL)
{
	this.title = title;
	this.URL = URL;
}
var liveBookmarks = 
{
	// RDF namespaces
	NC_NS : "http://home.netscape.com/NC-rdf#",
	RDF_NS : "http://www.w3.org/1999/02/22-rdf-syntax-ns#",

	getRDF: function()
	{
		return Components.classes["@mozilla.org/rdf/rdf-service;1"]
			.getService( Components.interfaces.nsIRDFService );
	},

	getRDFC: function()
	{
		return Components.classes["@mozilla.org/rdf/container;1"]
			.createInstance( Components.interfaces.nsIRDFContainer );
	},

	getRDFCU: function()
	{
		return Components.classes["@mozilla.org/rdf/container-utils;1"]
			.getService( Components.interfaces.nsIRDFContainerUtils );
	},

	getProperty: function ( aInput, aArc, DS )
	{
		var node;
		node = DS.GetTarget( aInput, aArc, true );
		if( node instanceof Components.interfaces.nsIRDFResource )
			return node.Value;
		else if( node instanceof Components.interfaces.nsIRDFLiteral )
			return node.Value;

		return "";
	},

	getAll : function()
	{
		if (gFF >= 3)
			return this.getAll3();
		else
			return this.getAll2();
	},

	Nodes : new Array(),
	getAll3 : function()
	{
		try
		{
// simple in new versions of Firefox
			Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
			var lifemarks = new Array();
			var curFeedURL, curName;
			var tmp = new Array();
			tmp = window.PlacesUtils.annotations.getItemsWithAnnotation(window.PlacesUtils.LMANNO_FEEDURI);
			for (var i=0; i<tmp.length; i++)
				{
					curFeedURL = window.PlacesUtils.annotations.getItemAnnotation(tmp[i],window.PlacesUtils.LMANNO_FEEDURI);
					curName = window.PlacesUtils.bookmarks.getItemTitle(tmp[i]);
					lifemarks.push(new lifemarkData(curName, curFeedURL));
				}
			return lifemarks;
		}
		catch(e)
		{
// for older versions of Firefox
			var rootNode;
			var lifemarks = new Array();
			var Ci = Components.interfaces;
			var bmsvc = Components.classes["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
			var bmroot = bmsvc.bookmarksMenuFolder;
			var tbroot = bmsvc.toolbarFolder;
			var histsvc = Components.classes["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
			var options = histsvc.getNewQueryOptions();
			var query1 = histsvc.getNewQuery();
			query1.setFolders([bmroot], 1);
			var result = histsvc.executeQuery(query1, options);
			this.Nodes.push(result.root);
			query1.setFolders([tbroot], 1);
			result = histsvc.executeQuery(query1, options);
			this.Nodes.push(result.root);
			while (this.Nodes.length > 0)
			{
				rootNode = this.Nodes.shift();
				rootNode.containerOpen = true;
				lifemarks = this.findlivemarks(rootNode,lifemarks);
				rootNode.containerOpen = false;
			}
			return lifemarks;
		}
	},

	findlivemarks : function(rootNode,lifemarks)
	{
		var Ci = Components.interfaces;
		var curFeedURL, curName;
		var lmsvc = Components.classes["@mozilla.org/browser/livemark-service;2"].getService(Ci.nsILivemarkService);

		for (var i=0; i<rootNode.childCount; i++)
		{
  		var node = rootNode.getChild(i);
			if (node.type == node.RESULT_TYPE_FOLDER)
			{
				if (lmsvc.isLivemark(node.itemId))
				{
					curFeedURL = lmsvc.getFeedURI(node.itemId).spec;
					curName = node.title;
					lifemarks.push(new lifemarkData(curName, curFeedURL));
				}
				else
				{
					node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
					this.Nodes.push(node);
				}
			}
		}
		return lifemarks;
	},

	getAll2 : function()
	{
		var lifemarks = new Array();
		// RDF variables
		var RDF = this.getRDF();
		var RDFC = this.getRDFC();
		var RDFCU = this.getRDFCU();
		var BMDS  = RDF.GetDataSource("rdf:bookmarks");
		var root = RDF.GetResource( "NC:BookmarksRoot" );
		var NameArc = RDF.GetResource( this.NC_NS + "Name" );
		var feedURLArc =  RDF.GetResource( this.NC_NS + "FeedURL" );
		var URLArc =  RDF.GetResource( this.NC_NS + "URL" );
		var typeArc = RDF.GetResource( this.RDF_NS + "type" );

		var nodesToProcess = new Array( root );
		var curNode, curType, curFeedURL, curName, enumerator;
		while( nodesToProcess.length > 0 )
		{
			curNode = nodesToProcess.pop();
			RDFC.Init( BMDS, curNode );
			enumerator = RDFC.GetElements();
			while( enumerator.hasMoreElements() )
			{
				curNode = enumerator.getNext();
				curType = this.getProperty( curNode, typeArc, BMDS ).split( "#" )[1];
				if( curType == "Folder" )
					nodesToProcess.push( curNode );
				else if( curType == "Livemark" )
				{
					curFeedURL = this.getProperty( curNode, feedURLArc, BMDS );
					curName = this.getProperty( curNode, NameArc, BMDS );
					lifemarks.push(new lifemarkData(curName, curFeedURL));
				}
			}
		}
		return lifemarks;
	},

  getAllUnique : function()
  {
    var livemarks = this.getAll();
    var uniqmarks = new Array();
    var isNew;
    if (livemarks.length > 0) uniqmarks.push(livemarks[0]);
    for (var i=1; i<livemarks.length; i++)
    {
      isNew = true;
      for (var j=0; j<uniqmarks.length; j++)
				if (uniqmarks[j].URL == livemarks[i].URL)
				{
					isNew = false;
					break;
				}
      if (isNew) uniqmarks.push(livemarks[i]);
    }
    return uniqmarks;
  },

	getNF_Node : function()
	{
		// RDF variables
		var RDF = this.getRDF();
		var RDFC = this.getRDFC();
		var RDFCU = this.getRDFCU();
		var BMDS  = RDF.GetDataSource("rdf:bookmarks");
		var root = RDF.GetResource( "NC:BookmarksRoot" );
		var URLArc =  RDF.GetResource( this.NC_NS + "URL" );
		var typeArc = RDF.GetResource( this.RDF_NS + "type" );

		var nodesToProcess = new Array( root );
		var curNode, curType, enumerator;
		while( nodesToProcess.length > 0 )
		{
			curNode = nodesToProcess.pop();
			RDFC.Init( BMDS, curNode );
			enumerator = RDFC.GetElements();
			while( enumerator.hasMoreElements() )
			{
				curNode = enumerator.getNext();
				curType = this.getProperty( curNode, typeArc, BMDS ).split( "#" )[1];
				if( curType == "Folder" )
					nodesToProcess.push( curNode );
				else if( this.getProperty( curNode, URLArc, BMDS ) == NF_URI )
					return curNode;
			}
		}
		return null;
	},

	uri : function(spec)
	{
		var iosvc = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
		return iosvc.newURI(spec, null, null);
	},

	getNF_Keyword : function()
	{
			var bmsvc = Components.classes["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Components.interfaces.nsINavBookmarksService);
			return bmsvc.getKeywordForURI(this.uri(NF_URI));
	},

	setNF_Keyword : function(newKeyword)
	{
		var bkmkTitle = "NewsFox sync";
		if (gFF >= 3)
		{
			var bmsvc = Components.classes["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Components.interfaces.nsINavBookmarksService);
			var nfid = bmsvc.getBookmarkIdsForURI(this.uri(NF_URI), {});
			var locIndex = -1;
			var prntFolder = bmsvc.bookmarksMenuFolder;
			if (nfid.length)
			{
				locIndex = bmsvc.getItemIndex(nfid[0]);
				prntFolder = bmsvc.getFolderIdForItem(nfid[0]);
			}
			while (nfid.length > 0)
			{
				var bkmkId = nfid[0];
				bmsvc.removeItem(bkmkId);
				nfid.shift();
			}
			if (nfid.length == 0)
			{
				var newMark = bmsvc.insertBookmark(prntFolder, this.uri(NF_URI), locIndex, bkmkTitle);
				nfid.push(newMark);
			}
			bmsvc.setKeywordForBookmark(nfid[0], newKeyword);
		}
	},

	getNF_Description : function()
	{
		if (gFF >= 3)
		{
			var annotationService = Components.classes["@mozilla.org/browser/annotation-service;1"].getService(Components.interfaces.nsIAnnotationService);
			try	{ return annotationService.getPageAnnotation(this.uri(NF_URI), "newsfox/sync"); }
			catch(e) { return null; }
		}
		else if (gFF > -1)
		{
			var RDF = this.getRDF();
			var BMDS  = RDF.GetDataSource("rdf:bookmarks");
			var KeywordArc = RDF.GetResource( this.NC_NS + "ShortcutURL" );
			var curNode = this.getNF_Node();
			if (curNode) return this.getProperty( curNode, KeywordArc, BMDS );
		}
		return null;
	},

	setNF_Description : function(newDescription)
	{
		var bkmkTitle = "NewsFox sync";
		if (gFF >= 3)
		{
			var bmsvc = Components.classes["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Components.interfaces.nsINavBookmarksService);
			var nfid = bmsvc.getBookmarkIdsForURI(this.uri(NF_URI), {});
			if (nfid.length == 0)
			{
				var newMark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, this.uri(NF_URI), -1, bkmkTitle);
				nfid.push(newMark);
			}
			var annotationService = Components.classes["@mozilla.org/browser/annotation-service;1"].getService(Components.interfaces.nsIAnnotationService);
			annotationService.setPageAnnotation(this.uri(NF_URI), "newsfox/sync", newDescription, 0, 0);
		}
		else if (gFF > -1)
		{
			var RDF = this.getRDF();
			var BMDS  = RDF.GetDataSource("rdf:bookmarks");
			var root = RDF.GetResource( "NC:BookmarksRoot" );
			var KeywordArc = RDF.GetResource( this.NC_NS + "ShortcutURL" );
			var bmsvc = Components.classes["@mozilla.org/browser/bookmarks-service;1"].getService(Components.interfaces.nsIBookmarksService);
			var curNode = this.getNF_Node();
			if (curNode)
			{
				BMDS.Change(curNode, KeywordArc, 
						RDF.GetLiteral(this.getProperty(curNode, KeywordArc, BMDS)), 							RDF.GetLiteral(newDescription));
			}
			else
				bmsvc.createBookmarkInContainer(bkmkTitle, NF_URI, newDescription, null, null, null, root, 0);
		}
	}
}

///////////////////////////////////////////////////////////
// AG: check model against livemarks. Returns an array, possibly empty.
function getNewLivemarks()
{
	var allLivemarks = liveBookmarks.getAllUnique();
	var allLivemarksLen = allLivemarks.length;
	var newLivemarks = new Array();
	var modelTotalSize = gFmodel.sizeTotal();
	for( var k=0; k < allLivemarksLen; k++ )
	{
		var livemark = allLivemarks[k];
		var isNew = true;
		for( var i=0; i < modelTotalSize; i++ )
			if( gFmodel.get(i).url == livemark.URL)
			{
				isNew = false;
				break;
			}

		if( isNew )
			newLivemarks.push(livemark);
	}
	return newLivemarks;
}

function manageLivemarks()
{
	var allLivemarks = liveBookmarks.getAllUnique();
	for (var i=gFmodel.size(); i<gFmodel.sizeTotal(); i++)
	{
		var isNew = true;
		var feed = gFmodel.get(i);
		for (var j=0; j<allLivemarks.length; j++)
			if (allLivemarks[j].URL == feed.url) isNew = false;
		if (isNew) allLivemarks.push(new lifemarkData(feed.defaultName, feed.url));
	}
	var selected = new Array();
	for( var i=0; i < gFmodel.sizeTotal(); i++)
	{
		var feed = gFmodel.get(i);
		if( !feed.exclude )
			selected.push(feed.url);
	}
	var params = { ok:false, livemarks:allLivemarks, selected:selected };
  var win = window.openDialog("chrome://newsfox/content/livemarksDlg.xul",
    "newsfox-dialog","chrome,centerscreen,modal", params);

	if (params.ok) updateLivemarks(allLivemarks, selected, false);
}

function updateLivemarks(livemarks, selected, bNew)
{
	var lastFeedURL = null;
	var feedtree = document.getElementById("newsfox.feedTree");
  for( var i=0; i < livemarks.length; i++ )
  {
    var feed = gFmodel.getFeedByURL(livemarks[i].URL);
    var isExcluded = true;
    for( var k = 0; k < selected.length; k++ )
      if( selected[k] == livemarks[i].URL )
      {
        isExcluded = false;
        break;
      }
    if( bNew || !feed)  // new feed
    {
      feed = createNewFeed(gFmodel,livemarks[i].URL,isExcluded,true,false);
      feed.setDefaultName(livemarks[i].title);
			if (!isExcluded) lastFeedURL = feed.url;
    }
    else  // current feed
    {
      if (feed.exclude != isExcluded)
      {
        if (feed.exclude)   // make included
        {
          gFmodel.remove(feed);
          var inclfeed = createNewFeed(gFmodel,feed.url,false,true,false);
          inclfeed.setDefaultName(livemarks[i].title);
					lastFeedURL = inclfeed.url;
        }
        else                // make excluded
        {
          if (!gFdGroup[0].expanded) feedtree.view.toggleOpenState(0);
          var index = -1;
          for (var j=1; j<=gFmodel.size(); j++)
            if (feed.url == gFmodel.get(gIdx.feed[j]).url) index = j;
          if (index == -1) return;
          deleteFeed(index,false);
          var exclfeed = createNewFeed(gFmodel,feed.url,true,true,false);
          exclfeed.setDefaultName(livemarks[i].title);
        }
      }
    }
	}
  saveModels();
	if (lastFeedURL)
	{
		var nFeed = gFmodel.getIndexByURL(lastFeedURL);
		var index = getFeedRow(0,nFeed);
		feedtree.view.selection.select(index);
  	feedtree.treeBoxObject.ensureRowIsVisible(index);
	}
}

function doKeywordCopy(operation)
{
	if (gFF >= 3 && gOptions.bookmarkSync)
	{
		if (operation == "read")
		{
			var nfKeyword = liveBookmarks.getNF_Keyword();
			if (nfKeyword) liveBookmarks.setNF_Description(nfKeyword);
		}
		else if (operation == "write")
		{
			var nfKeyword = liveBookmarks.getNF_Description();
			if (nfKeyword) liveBookmarks.setNF_Keyword(nfKeyword);
		}
	}
}

function doLivemarks(isLivemarks)
{
	if( gFF > -1 && gUserAgent.indexOf("Flock") == -1)
	{
		var newLivemarks;
		if (isLivemarks)
			newLivemarks = getNewLivemarks();
		else
			newLivemarks = gBookmarkSync.getNewSyncmarks();
		if( newLivemarks.length )
		{
			var selected = new Array();
			var params = { ok:false, livemarks:newLivemarks, selected:selected, onlyNew:isLivemarks, newSync:!isLivemarks };
			var win = window.openDialog("chrome://newsfox/content/livemarksDlg.xul",
				"newsfox-dialog","chrome,centerscreen,modal", params);

			if (params.ok) updateLivemarks(newLivemarks, selected, true);
		}
	}
	else
		document.getElementById("tBlivemark").setAttribute("hidden", true);
}

////////////////////////////////////////////////////////////////
// Bookmark syncing
////////////////////////////////////////////////////////////////

function BookmarkSync()
{
	const SEP1 = ":nf:";
	const SEP2 = ":at:";

	function feedUpdateItem()
	{
		this.url = "";
		this.lastUpdate = null;
	}

	var feedUpdate = new Array();

	this.updateArrays = function()
	{
		try
		{
			feedUpdate = new Array();
			var tmp = liveBookmarks.getNF_Description();
			var tmp1 = tmp.split(SEP1);
			for (var i=0; i<tmp1.length; i++)
			{
				var tmp2 = tmp1[i].split(SEP2);
				var item = new feedUpdateItem();
				item.url = tmp2[0];
				item.lastUpdate = tmp2[1];
				feedUpdate.push(item);
			}
		}
		catch(e) {}
	}

	this.getNewSyncmarks = function()
	{
    var feed;
		var newSyncmarks = new Array();
		  for (var i=0; i < feedUpdate.length; i++)
			{
		    feed = gFmodel.getFeedByURL(feedUpdate[i].url);
				if (feed == null)
					newSyncmarks.push(new lifemarkData(feedUpdate[i].url, feedUpdate[i].url));
			}
		return newSyncmarks;
	}

	this.importB = function(feed)
	{
		var lastUpdate = feed.lastUpdate;
		this.updateArrays();
		doLivemarks(false);
		for (var i=0; i<feedUpdate.length; i++)
			if (feedUpdate[i].url == feed.url)
			{
				try
				{
					lastUpdate = new Date();
					lastUpdate.setTime(Date.parse(feedUpdate[i].lastUpdate));
				}
				catch(e) {}
			}
		return lastUpdate;
	}

	this.exportB = function(feed)
	{
		this.updateArrays();
		var exists = false;
		for (var i=0; i<feedUpdate.length; i++)
			if (feedUpdate[i].url == feed.url)
			{
				if (exists) feedUpdate.splice(i--,1);
				else feedUpdate[i].lastUpdate = new Date().toUTCString();
				exists = true;
			}
		if (!exists)
		{
			var item = new feedUpdateItem();
			item.url = feed.url;
			item.lastUpdate = new Date().toUTCString();
			feedUpdate.push(item);
		}
	this.writeB();
	}

	this.writeB = function()
	{
		var combo = new Array();
		for (var i=0; i<feedUpdate.length; i++)
			combo[i] = feedUpdate[i].url + SEP2 + feedUpdate[i].lastUpdate;
		var newDescription = combo.join(SEP1);
		liveBookmarks.setNF_Description(newDescription);
	}

	this.deleteB = function(feed)
	{
		this.updateArrays();
		for (var i=0; i<feedUpdate.length; i++)
			if (feedUpdate[i].url == feed.url)
				feedUpdate.splice(i,1);
		this.writeB();
	}
}
