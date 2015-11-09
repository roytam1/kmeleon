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
 * The Original Code is NewsFox.
 *
 * The Initial Developer of the Original Code is
 * Andy Frank <andy@andyfrank.com>.
 * Portions created by the Initial Developer are Copyright (C) 2005-2011
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
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

////////////////////////////////////////////////////////////////
// Global
////////////////////////////////////////////////////////////////

// common to many *.js
var gFmodel = new FdModel();
var gIdx = new Indexes();
var gFdGroup = new Array();
var gCollect = null;
var numGroupArray = new Array();

// globals just in model.js
const FOLDER_OPEN  = getPng("folderOpen.png");
const FOLDER_CLOSED  = getPng("folderClosed.png");
const FOLDER_SEARCH  = getPng("folderSearch.png");
const FOLDER_TAG  = getPng("folderTag.png");
const ICON_ERR = getPng("brokenFeed.png");
const FLAGICON = getPng("flag.png");
const READICON = getPng("read.png");
const UNREADICON = getPng("unread.png");
const GETICON = getPng("download.png");
const ERRORICON = getPng("delete.png");

// feedtree row properties (are also cell properties)
const LEVELNAME = [ "folder", "feed", "category" ];
const FOLDERTYPE = [ "feedsFolder", "tagFolder", "searchFolder" ];
const FEEDSFEED = "feedsFeed";
const ONLYINFEEDS = "onlyInFeeds";
// articletree row properties (are also cell properties)
const NOID = "noID";
const FLAG = "flagged";
// PROB + i where i=1,2,... PROBLEVELS
const PROB = "prob";
const PROBLEVELS = 10;
const HIGHPROB = "highprob";
const HIGHPROBLEVEL = 0.98;
// HOUR + i where i in HOURLEVELS else HOUR + OTHER
const OTHER = "other";
const HOUR = "hour";
const HOURLEVELS = [ 1, 3, 6, 12, 24 ];
const HOURMSEC = 1000*60*60;
// DAY + i where i in DAYLEVELS else DAY + OTHER
const DAY = "day";
const DAYLEVELS = [ 1, 7, 30 ];
const DAYMSEC = HOURMSEC * 24;
// KEYWORD + i where i=1,2,3,... from newsfox.keyword+i preferences
const KEYWORD = "keyword";
// built-in articletree column properties (are also cell properties)
// [ "flag", "title", "read", "date", "author", "source", "blog", "prob" ]
// both trees row properties (are also cell properties)
const UNREAD = "unread";
const NEWUNREAD = "newunread";
// both trees cell properties
const FAVICONCOL = "faviconcol";
// arttree cell properties
const NEEDSIMAGES = "needsimages";

const ARTICLE_TOOLTIP_WIDTH = 450;
const ARTICLE_TOOLTIP_HEIGHT = 275;
const ART_TT_REENABLE_DELAY = 1;

const colShort = [ "f", "t", "r", "x", "p", "d", "s", "a", "b", "g" ];
const colLong = [ "flag", "title", "read", "Xtend", "prob", "date", "source", "author", "blog", "tag" ];

var dragLevel = 3;
var dragGrp = -1;
var dragFeed = -1;
var dragArticle = -1;
var dragEvent;
var dragToggling = false;

////////////////////////////////////////////////////////////////
// Model
////////////////////////////////////////////////////////////////

function FdModel()
{
  this.add  = function(feed,isExcluded)
  {
    if (isExcluded) 
      feeds.push(feed);
    else
      feeds.splice(gFmodel.size(),0,feed); 
  }
  this.get  = function(index) { return feeds[index]; }
	this.set  = function(index, feed)
	{
		if(index < 0 || index > feeds.length - 1)
			return;
		feeds[index] = feed;
	}
  this.getIndexByURL  = function(url)
	{
		for(var i=0; i < feeds.length; i++)
			if(url == feeds[i].url)
				return i;
		return -1;
	}
  this.getFeedByURL  = function(url)
	{
		var nFeed = this.getIndexByURL(url);
		if (nFeed > -1) return feeds[nFeed];
		else return null;
	}
  this.getIndexByURLnoCase  = function(url)
	{
		for(var i=0; i < feeds.length; i++)
			if(url.toLowerCase() == feeds[i].url.toLowerCase())
				return i;
		return -1;
	}
  this.getFeedByURLnoCase  = function(url)
	{
		var nFeed = this.getIndexByURLnoCase(url);
		if (nFeed > -1) return feeds[nFeed];
		else return null;
	}

  this.size = function() // returns size without excluded feed(s)	
	{
		var size = 0;
		for( var i = 0; i < feeds.length; i++ )
			if( !feeds[i].exclude )
				size++;
		return size;
	}

  this.sizeTotal = function() // returns total size, i.e. with excluded from seeing feed(s)
	{
		return feeds.length;
	}

  var feeds = new Array();

  /**
   * Return a new unique uid.
   */
  this.makeUniqueUid = function(url)
  {
    var index  = url.indexOf("://");
		var body;
		if( index > -1 )
			if (url.charAt(index+3) == "/")  //  file://
				body = "local";
			else
				body = url.substring(index+3);
		else
			body = url.replace(/ /g,"");
    var domain = body;
		if( body.indexOf("/") != -1 )
			domain = body.split("/")[0];
		if( domain.indexOf(":") != -1 )
			domain = domain.split(":")[0]; // there are cases when port number follows domain name
    var count = 1;
    var name = domain;
		name = name.replace(/\?|\||<|>|"|\*|\\/g,"");  // has to be valid file name
    for (var i=0; i<feeds.length; i++)
    {
      if (name == feeds[i].uid)
      {
        name = domain + (count++);
        i = -1; // reset loop
      }
    }
    return name;
  }

  /**
   * Remove the given feed from the model.
   */
  this.remove = function(feed)
  {
    for (var i=0; i < feeds.length; i++)
      if (feed.uid == feeds[i].uid)
      {
        feeds.splice(i,1);
        return;
      }
  }
}

////////////////////////////////////////////////////////////////
// Group Model
////////////////////////////////////////////////////////////////

function FeedGroup()
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
  this.title = NF_SB.getString('newGroup');
  this.expanded = false;
  this.list = new Array();
	this.search = false;
	this.srchdat = { flagged : 2, unread : 2, text : "", textflags : 15, startTime : -1, endTime: 0 };
	this.searchTag = null;
	this.showUnread = true;
	this.unread = null;
	this.processUnread = false;
	this.pastUnread = 0;
}

FeedGroup.prototype.getUnread = function()
  {
		if (this.search && (!this.showUnread || !gOptions.srchUnread)) return 0;
		if (this.unread != null) return this.unread;
		if (this.processUnread) return 0;
		this.processUnread = true;
		var grp = this;

		this.doSearchUnread = function()
		{
			var unread = 0;
			var srchitem = new Array();
			var srchstr = null;
			if (grp.srchdat.text != "") srchstr = mkSrchstr(grp.srchdat,srchitem);
			var now = new Date().getTime();
			for (var i=0; i<grp.list.length; i++)
			{
				var feed = gFmodel.get(grp.list[i]);
				for (var j=0; j<feed.size(); j++)
					if (!feed.isRead(j) && hasPropSandbox.hasProp(j, feed, grp.srchdat, srchstr, srchitem, now))
						unread++;
			}
			if (!gAllFeedsLoaded) unread += "?";
			grp.unread = unread;
			grp.processUnread = false;
			feedTreeInvalidate();
		}

		var unread = 0;
		if (this.search && !this.searchTag)  // search group
		{
			setTimeout(this.doSearchUnread, 100);
			unread = this.pastUnread;
			if (!gAllFeedsLoaded && !isNaN(unread)) unread += "?";
		}
		else if (this.search)  // tag group
		{
		  var feed, nFeed;
		  for (var i=0; i<gFdGroup[0].list.length; i++)
			{
				nFeed = gFdGroup[0].list[i];
				feed = gFmodel.get(nFeed);
				for (var j=0; j<feed.size(); j++)
					if (!feed.isRead(j) && hasTag(j, feed, this.searchTag))
						unread++;
			}
			if (!gAllFeedsLoaded && !isNaN(unread)) unread += "?";
			this.processUnread = false;
		}
		else  // type==0, regular group
		{
			for (var i=0; i<this.list.length; i++)
	      unread += gFmodel.get(this.list[i]).getUnread(0);
			this.processUnread = false;
		}
		if (gAllFeedsLoaded) this.unread = unread;
		return unread;
  }

function Indexes()
{
	this.fdgp = new Array();
	this.feed = new Array();
	this.catg = new Array();
	this.open = new Array();
}

////////////////////////////////////////////////////////////////
// Feed Model
////////////////////////////////////////////////////////////////

function Feed()
{
  this.uid = null;
  this.url = "";
  this.homepage = null;
	this.icon = new Image;
	this.icon.src = ICON_OK;
  this.defaultName = null;
  this.customName = null;
  this.error = ERROR_OK;
  this.loaded = false;
	this.deleteOldStyle = 0;
	this.daysToKeep = -1;
	this.changedUnread = 0;
  this.autoCheck = true;
	this.exclude = false;
	this.loading = false;
	this.changed = false;
	this.prvate = false;
	this.username = null;
	this.password = null;
	this.storage = false;
	this.lastUpdate = null;
	this.autoRefreshInterval = 0;
  this.style = 0;
	this.Xfilter = null;
	this.XfilterType = -1;
	this.XfilterNew = false;
	this.XfilterMimeType = null;
	this.XfilterImages = false;
	this.sortStr = "g+";
  this.artTreeCols = "";

  var categories = new Array();
  var articles = new Array();
  var deletedarticles = new Array();
  this.flags = new Array();

  this.getStyle = function()
  {
    if (this.style == 0)
      return gOptions.globalStyle;
    return this.style;
  }
  this.getDisplayName = function()
  {
    return ((this.customName != null) && (this.customName.length > 0)) ? this.customName : this.defaultName;
  }
  this.setDefaultName = function(dname)
		{ this.defaultName = dname; }
  this.getUnread = function(nCategory)
  {
    var unread = 0;
    for (var i=0; i<this.flags.length; i++)
      if ((this.flags[i] & 0x01) == 0) // unread
      {
        if( nCategory == 0 )
          unread++;
        else
				{
					var ScatnameS = "\/" + categories[nCategory-1] + "\/";
					var ScatS = "\/" + articles[i].category + "\/";
					if (ScatS.indexOf(ScatnameS) > -1)
          	unread++;
				}
      }
    return unread;
  }
	this.hasNewUnread = function()
	{
		for (var i=0; i<this.size(); i++)
			if (this.get(i).newUnread) return true;
		return false;
	}
  this.add = function(article,flag)
  {
		articles.push(article);
		if(article.category != "")
			this.addCategory(article.category);
		this.flags.push(flag);
		return articles[articles.length-1];
  }
  this.deletedAdd = function(article)
		{ deletedarticles.push(article); }
  this.get = function(index)
		{ return articles[index]; }
  this.deletedget = function(index)
		{ return deletedarticles[index]; }
  this.set = function(index,item)
		{ articles[index] = item; }
  this.remove = function(index)
		{ articles.splice(index,1); this.flags.splice(index,1); }
	this.removeAll = function()
		{ articles = new Array(); deletedarticles = new Array(); }
  this.deletedremove = function(index)
		{ deletedarticles.splice(index,1); }
  this.size = function()
		{ return articles.length; }
  this.deletedsize = function()
		{ return deletedarticles.length; }

  this.isRead = function(index)
		{ return ((this.flags[index] & 0x01) != 0); }
  this.setRead = function(index, value)
  {
    if (value) this.flags[index] |= 0x01;
    else this.flags[index] &= 0xFE;
  }
  this.isFlagged = function(index)
		{ return ((this.flags[index] & 0x04) != 0); }
  this.setFlagged = function(index, value)
  {
    if (value) this.flags[index] |= 0x04;
    else this.flags[index] &= 0xFB;
  }
	this.isXtend = function(index)
		{ return articles[index].Xtend; }
	this.setXtend = function(index, value)
  	{ articles[index].Xtend = value; }

  // AG: added categories
  this.addCategory = function(category)
  {
		var catArray = category.split("\/");
		for (var i=0; i<catArray.length; i++)
		{
			var exists = false;
			for (var j=0; j<categories.length; j++)
				if (catArray[i] == categories[j]) exists = true;
			if (!exists) categories.push(catArray[i]);
		}
  }
  this.getCategories = function()
		{ return categories; }
  this.sortCategories = function()
	{
		var sorter = function(a,b)
		{
			var alpha = names[b-1].toLowerCase() < names[a-1].toLowerCase() ? 1 : -1;
			var posa, posb, first;
			switch (gOptions.catSort)
			{
	      default:
				case 1:
					posb = hasUnread[b];
					posa = hasUnread[a];
					if (posb && !posa) return 1;
					else if (posa && !posb) return -1;
					else return alpha;
					break;
				case 2:
					first = numUnread[b] - numUnread[a];
					if (first > 0) return 1;
					else if (first < 0) return -1;
					else return alpha;
					break;
				case 0:
					return alpha;
					break;
			}
		}

		var numUnread = new Array();
		var hasUnread = new Array();
		for (var i=0; i<categories.length; i++)
		{
			numUnread[i] = this.getUnread(i);
			hasUnread[i] = numUnread[i] > 0;
		}
		var dummy = new Array();
		var names = new Array();
		for (var i=0; i<categories.length; i++)
		{
			dummy[i] = i+1;   // categories start at 1, not zero
			names[i] = categories[i];
		}
		dummy.sort(sorter);
		for (i=0; i<names.length; i++)
			categories[i] = names[dummy[i]-1];
	}

  this.makeUniqueArtId = function(art)
  {
		var count = 1;
		var id;
		if (art.link) id = art.link;
		else id = "id";
		for (var i=0; i<articles.length; i++)
			if (id == articles[i].id)
			{
				id = art.link + (count++);
				i = -1;
			}
		return id;
	}
}

////////////////////////////////////////////////////////////////
// FeedTree Model
////////////////////////////////////////////////////////////////

function dragFd(evt) {
// Returns "WINNT" on Windows Vista, XP, 2000, and NT systems;  
// "Linux" on GNU/Linux; and "Darwin" on Mac OS X.  
	var osString = Components.classes["@mozilla.org/xre/app-info;1"]  
               .getService(Components.interfaces.nsIXULRuntime).OS;
	if (osString == "Linux" && gOptions.linuxNoDragDrop) return;
  var tree = evt.target.parentNode;
  if (tree.id != "newsfox.feedTree") return;
  var row = {}, col = {}, type = {};
  tree.treeBoxObject.getCellAt(evt.clientX, evt.clientY, row, col, type);
  dragLevel = tree.view.getLevel(row.value);
  dragGrp = gIdx.fdgp[row.value];
	dragFeed = gIdx.feed[row.value];
	dragArticle = -1;
  if (dragLevel <= 1)
	{
		var feed = gFmodel.get(dragFeed);
		evt.dataTransfer.setData("newsfox/feedtreerow", "");
		evt.dataTransfer.setData("application/rss+xml", feed.url);
		evt.dataTransfer.setData("text/x-moz-url", feed.homepage + "\n" + feed.getDisplayName() + "\n" + feed.url + "\n" + feed.getDisplayName());
		evt.dataTransfer.setData("text/uri-list", feed.homepage + "\n" + feed.url);
		evt.dataTransfer.setData("text/plain", feed.getDisplayName() + "\n" + feed.url + "\n" + feed.homepage + "\n");
		evt.dataTransfer.effectAllowed = "copyLink";
	}
	else
		evt.dataTransfer.effectAllowed = "none";
}

function FeedTreeModel() 
{
  // 3/25/05 - Temp disable flag/trash folder
  // AG: added category. Most subcode is reworked
  // 11 Jan 07 RP: remove flag/trash, add groups, code reworked

	// getter syntax needed because of Firefox bug#454088
//	this. rowCount getter = function() { return gIdx.fdgp.length; }
	this.__defineGetter__("rowCount", function() { return gIdx.fdgp.length; });
  this.isTopLevelElement = function(row){ return (this.getLevel(row) == 0); }
  this.getCellText = function(row,col)
  {
    var level = this.getLevel(row);
    var text, unread;
    if (level == 0)
    {
      var curFdGroup = gFdGroup[gIdx.fdgp[row]];
      text = curFdGroup.title;
      unread = curFdGroup.getUnread();
    }
    else
    {
      var feed = gFmodel.get(gIdx.feed[row]);
      var nCategory = gIdx.catg[row]; 
      text = "";
      unread = feed.getUnread(nCategory);
      if( nCategory )
      {
        var categories = feed.getCategories();
        text = categories[nCategory-1].replace(/&#047;/g,"/");
      }
      else
        text = feed.getDisplayName();
    }
    if (unread != 0) text += " (" + unread + ")";
    return text; 
  }
  this.setTree = function(treebox){ this.treebox = treebox; }
  this.isContainer = function(row){ return (this.getLevel(row) < 2); }
  this.isContainerEmpty = function(row)
  {
    var level = this.getLevel(row);
		if (level == 0 && gFdGroup[gIdx.fdgp[row]].search) return true;
    var feed = gFmodel.get(gIdx.feed[row]);
    if (level == 0 && gFdGroup[gIdx.fdgp[row]].list.length) return false;
    if (level == 1 && feed.loaded && feed.getCategories().length) return false;
    return true;
  }
  this.isContainerOpen = function(row){ return gIdx.open[row]; }
  this.hasNextSibling = function(row, index)
  {
    if (index+1 > gIdx.fdgp.length-1) return false;
    var level = this.getLevel(row);
    if (level == 0) return (gIdx.fdgp[gIdx.fdgp.length-1] != gIdx.fdgp[row]);
    else if (level == 1) return (gIdx.fdgp[index+1] == gIdx.fdgp[row]);
    else return ((gIdx.fdgp[index+1] == gIdx.fdgp[row]) && (gIdx.feed[index+1] == gIdx.feed[row]));
  }
  this.getParentIndex = function(row)
  {
    var index = row;
    var level = this.getLevel(row);
    if (level == 0) index = -2;
    else if (level == 1)
      while (gIdx.fdgp[index] == gIdx.fdgp[row]) index--;
    else
      while (gIdx.feed[index] == gIdx.feed[row]) index--;
    return ++index; 
  }
  this.isSeparator = function(row){ return false; }
  this.isEditable = function(row,col){ return false; }
  this.isSorted = function(row){ return false; }
  this.getLevel = function(row)
  {
    var level = 0;
    if (gIdx.catg[row] > 0) level = 2;
    else if (gIdx.feed[row] > -1) level = 1;
    return level; 
  }
  this.getImageSrc = function(row,col) 
  {
    var retval = null;
    var level = this.getLevel(row);
    if (level == 0)
		{
			if (this.isContainerOpen(row)) 
				return FOLDER_OPEN;
			else if (gFdGroup[gIdx.fdgp[row]].search && gFdGroup[gIdx.fdgp[row]].searchTag)
				return FOLDER_TAG;
			else if (gFdGroup[gIdx.fdgp[row]].search)
				return FOLDER_SEARCH;
			else
				return FOLDER_CLOSED;
		}
    else if (level == 1)
		{
			var feed = gFmodel.get(gIdx.feed[row]);
			if (feed.error == ERROR_OK)
				return feed.icon.src;
			else if (feed.error == ERROR_REFRESH)
				return GETICON;
			else
				return ICON_ERR;
		}
    return retval;
  }
  this.getRowProperties = function(row,props)
	{
		var returnProps = "";
		var level = this.getLevel(row);
		returnProps = propsAdd(LEVELNAME[level], props, returnProps);
    if (level == 0)
      {
				var foldertype = null;
				if (row == 0)
					foldertype = FOLDERTYPE[0];
				else if (gFdGroup[gIdx.fdgp[row]].search && gFdGroup[gIdx.fdgp[row]].searchTag)
					foldertype = FOLDERTYPE[1];
        else if (gFdGroup[gIdx.fdgp[row]].search)
					foldertype = FOLDERTYPE[2];
				if (foldertype)
					returnProps = propsAdd(foldertype, props, returnProps);
      }
		else if (level == 1 && gIdx.fdgp[row] == 0)  // in feeds folder
		{
			returnProps = propsAdd(FEEDSFEED, props, returnProps);
			if (numGroupArray[gIdx.feed[row]] == 1)
				returnProps = propsAdd(ONLYINFEEDS, props, returnProps);
		}

    var hasUnread = false;
    var nFeed = gIdx.feed[row];
    if (nFeed == -1 && gFdGroup[gIdx.fdgp[row]].getUnread() != 0)
      hasUnread = true;
    if (nFeed > -1 && gFmodel.get(nFeed).getUnread(gIdx.catg[row]) != 0)
      hasUnread = true;
    if (hasUnread)
      returnProps = propsAdd(UNREAD, props, returnProps);

		var hasNewUnread = false;
		var grp = gFdGroup[gIdx.fdgp[row]];
		if (nFeed == -1 && !grp.search)
		{
			for (var i=0; i<grp.list.length; i++)
				if (gFmodel.get(grp.list[i]).hasNewUnread())
				{
					hasNewUnread = true;
					break;
				}
		}
		if (nFeed > -1 && gIdx.catg[row] == 0 && gFmodel.get(nFeed).hasNewUnread())
		  hasNewUnread = true;
		if (hasNewUnread)
		  returnProps = propsAdd(NEWUNREAD, props, returnProps);
		return(returnProps);
	}
  this.getCellProperties = function(row,col,props) 
  {
		var returnProps = "";
		if (gIdx.catg[row] == 0)
			returnProps = propsAdd(FAVICONCOL, props, returnProps);
	// row props here so text can be styled
		if (props)
			this.getRowProperties(row,props);
		else
			returnProps += this.getRowProperties(row);
		return(returnProps);
  }
  this.getColumnProperties = function(colid,col,props){}
	this.toggleSpecial = function(row)
	{
		var curGrp = gIdx.fdgp[row];
		for (var i=gIdx.fdgp.length-1; i>=0; i--)
			if (this.getLevel(i) == 0 && gFdGroup[gIdx.fdgp[i]].expanded)
				this.toggleOpenState(i);
		var newrow = getGroupRow(curGrp);
		var feedtree = document.getElementById("newsfox.feedTree");
		feedtree.view.selection.select(newrow);
		feedtree.treeBoxObject.ensureRowIsVisible(newrow);
		return newrow;
	}
  this.toggleOpenState = function(row)
  {
		if (row == 0) getNumGroups();
    if (this.isContainerEmpty(row)) return;
    var level = this.getLevel(row);
		var curGrp = gIdx.fdgp[row];
		var grp = gFdGroup[curGrp];
		if (level == 0 && !grp.expanded && gOptions.oneGroupOpen)
			row = this.toggleSpecial(row);
	// need to use global variable dragToggling due to FF bug#402540
		dragToggling = true;
    var feedtree = document.getElementById("newsfox.feedTree");
    var fRow = feedtree.treeBoxObject.getFirstVisibleRow();
    if (level == 0)
    {
      if (grp.expanded)
      {
        var num = 0;
        while (gIdx.fdgp[row+num] == curGrp) num++;
        num--;
        gIdx.fdgp.splice(row+1,num);
        gIdx.feed.splice(row+1,num);
        gIdx.catg.splice(row+1,num);
        gIdx.open.splice(row+1,num);
    		feedtree.treeBoxObject.rowCountChanged(row+1,-num);
      }
      else
      {
        for (var i=1; i <= grp.list.length; i++)
        {
          gIdx.fdgp.splice(row+i,0,curGrp);
          gIdx.feed.splice(row+i,0,grp.list[i-1]);
          gIdx.catg.splice(row+i,0,0);
          gIdx.open.splice(row+i,0,false);
        }
    		feedtree.treeBoxObject.rowCountChanged(row+1,grp.list.length);
      }
      grp.expanded = !grp.expanded;
      gIdx.open[row] = !gIdx.open[row];
    }
    else if (level == 1)
    {
      var curFeed = gIdx.feed[row];
      var feed = gFmodel.get(curFeed);
			feed.sortCategories();
      if (gIdx.open[row])
      {
        var num = 0;
        while (gIdx.feed[row+num] == curFeed) num++;  // can't use categories.length since # categories may change
        num--;
        gIdx.fdgp.splice(row+1,num);
        gIdx.feed.splice(row+1,num);
        gIdx.catg.splice(row+1,num);
        gIdx.open.splice(row+1,num);
    		feedtree.treeBoxObject.rowCountChanged(row+1,-num);
      }
      else
      {
        for (var i=1; i <= feed.getCategories().length; i++)
        {
          gIdx.fdgp.splice(row+i,0,gIdx.fdgp[row]);
          gIdx.feed.splice(row+i,0,gIdx.feed[row]);
          gIdx.catg.splice(row+i,0,i);
          gIdx.open.splice(row+i,0,false);
        }
    		feedtree.treeBoxObject.rowCountChanged(row+1,feed.getCategories().length);
      }
      gIdx.open[row] = !gIdx.open[row];
    }
    else { dragToggling=false; return; }
    saveModels();
		dragToggling = false;
    feedtree.treeBoxObject.scrollToRow(fRow);
  }
	this.canDropBeforeAfter = function(row,orientation)
	{
		if (dragGrp != -1)
		{
			if (row == 0 && orientation == -1) return false;
			return (this.getLevel(row) == dragLevel && orientation != 0) ? true : false;
		}
		else if (dragArticle != -1) return false;
	}
	this.canDropOn = function(row,orientation)
	{
		if (dragGrp != -1)
		{
			if (row == 0 && orientation == -1) return false;
	    switch (this.getLevel(row))
	    {
	      case 0:
					var canDrop = false;
					if (dragLevel == 1 && gFdGroup[gIdx.fdgp[row]].searchTag && orientation == 0) canDrop = true;
					return canDrop;
	      case 1:
	      case 2:
	      default:
					return false;
	    }
		}
		else if (dragArticle != -1)
		{
			if (orientation != 0) return false;
			if (gFdGroup[gIdx.fdgp[row]].searchTag) return true;
			if (this.getLevel(row) == 1 && gFmodel.get(gIdx.feed[row]).storage)
				return true;
			return false;
		}
	}
  this.canDrop = function(row,orientation) 
  {
		if (dragGrp != -1)
		{
			if (row == 0 && orientation == -1) return false;
	    switch (this.getLevel(row))
	    {
	      case 0:
					var canDrop = false;
					if (dragLevel == 0 && orientation != 0) canDrop = true;
					if (dragLevel == 1 && gFdGroup[gIdx.fdgp[row]].searchTag && orientation == 0) canDrop = true;
					return canDrop;
	      case 1:
					return (dragLevel == 1 && orientation != 0) ? true : false;
	      case 2:
	      default:
					return false;
	    }
		}
		else if (dragArticle != -1)
		{
			if (orientation != 0) return false;
			if (gFdGroup[gIdx.fdgp[row]].searchTag) return true;
			if (this.getLevel(row) == 1 && gFmodel.get(gIdx.feed[row]).storage)
				return true;
			return false;
		}
  }
  this.drop = function(row,orientation)
  {
		var feedtree = document.getElementById("newsfox.feedTree");
		var oldFeed = dragFeed;
		var newGrp = gIdx.fdgp[row];
		var newFeed = gIdx.feed[row];
		if (dragGrp != -1)
		{
	    if (orientation == 1)
	    {
	      var i = row;
	      while (gIdx.feed[i] == newFeed) i++;
	      if (gIdx.fdgp[i] == newGrp) newFeed = gIdx.feed[i]
	      else newFeed = -2;
	    }
	    if (dragLevel == 0)
				mvGrp(dragGrp,newGrp+1*(orientation == 1));
			else if (this.getLevel(row) != dragLevel)
			{
				feedtree.view.selection.select(getFeedRow(dragGrp,oldFeed));
				selectAllArticles();
				tagSelected(gFdGroup[gIdx.fdgp[row]].searchTag,null,null);
				feedtree.view.selection.clearSelection();
				feedtree.view.selection.select(getFeedRow(dragGrp,oldFeed));
			}
			else if (dragGrp == newGrp)
				mvFeed(dragGrp,oldFeed,newFeed);
			else
			{
				var isMove = (gOptions.dragCopy || dragGrp == 0 || dragEvent.ctrlKey)
									? false : true;
				if (isMove)
				{
					var index3 = getFeedRow(dragGrp,oldFeed);
					feedtree.view.selection.select(index3);
					deleteSingleFeedRow();
				}
				var index = -1;
				for (var i=0; i<gFdGroup[newGrp].list.length; i++)
					if (gFdGroup[newGrp].list[i] == oldFeed) index = i;
				if (index != -1)
				{
					var i = getFeedRow(newGrp,oldFeed);
					feedtree.view.selection.select(i);
					feedtree.treeBoxObject.ensureRowIsVisible(i);
				}
				else
				{
					var index = getGroupRow(newGrp);
					feedtree.view.toggleOpenState(index);
					var index2 = gFdGroup[newGrp].list.length;
					for (var i=0; i<gFdGroup[newGrp].list.length; i++)
						if (gFdGroup[newGrp].list[i] == newFeed) index2 = i;
					gFdGroup[newGrp].list.splice(index2,0,oldFeed);
					feedtree.view.toggleOpenState(index);
					feedtree.view.selection.select(getFeedRow(newGrp,oldFeed));
				}
			}
	    dragLevel = 3;
	    dragGrp = -1;
		}
		else if (dragArticle != -1)
		{
			dragArticle = -1;
			if (this.getLevel(row) == 0)
				tagSelected(gFdGroup[newGrp].searchTag,null,null);
			else
			{
				var feed = gFmodel.get(newFeed);
				var arttree = document.getElementById("newsfox.articleTree");
				for (var i=0; i<gCollect.size(); i++)
					if (arttree.view.selection.isSelected(i))
					{
						var art = gCollect.get(i);
						var newArt = art.clone();
						if (!gCollect.getFeed(i).storage)
						{
							newArt.source.url = gCollect.getFeed(i).url;
							newArt.source.name = gCollect.getFeed(i).getDisplayName();
						}
						if (art.id == null) newArt.id = feed.makeUniqueArtId(art);
						if (!doesArticleExist(feed, newArt)) 
							feed.add(newArt,1*gCollect.isRead(i)+4*gCollect.isFlagged(i));
					}
				cleanUpFeed(feed,newFeed);
			}
		}
  }
}

////////////////////////////////////////////////////////////////
// Article Model
////////////////////////////////////////////////////////////////

function Article()
{
  this.link  = null;
  this.title = "";
  this.body  = "";
  this.date  = null;
  this.category  = "";
	this.tag = "";
	this.enclosures = new Array();
  this.id = null;
	this.source = { url : null, name : "" };
	this.toRemove = false;
	this.author = "";
	this.prob = 0.5;
	this.Xtend = false;
	this.XtendError = false;
	this.Xbody = "";
	this.XfilterMimeType = null;
	this.newUnread = false;
	this.imagesToGet = 0;
	this.pastStyle = null;
	this.pastXfilterType = null;
	this.pastFeed = null;
}

Article.prototype.clone	= function()
	{
		var art = new Article();
		art.link = this.link;
		art.title = this.title;
		art.body = this.body;
		art.date = this.date;
		if (NFgetPref("storage.category","bool",true))
			art.category = this.category;
		if (NFgetPref("storage.tag","bool",true))
			art.tag = this.tag;
		art.id = this.id;
		if (NFgetPref("storage.source","bool",true))
		{
			art.source.url = this.source.url;
			art.source.name = this.source.name;
		}
		art.toRemove = this.toRemove;
		art.author = this.author;
		art.prob = this.prob;
		art.Xtend = this.Xtend;
		art.Xbody = this.Xbody;
		art.XfilterMimeType = this.XfilterMimeType;
		if (NFgetPref("storage.enclosure","bool",true))
		{
			for (var i=0; i<this.enclosures.length; i++)
			{
				var enc = new Enclosure();
				enc.url = this.enclosures[i].url;
				enc.type = this.enclosures[i].type;
				enc.length = this.enclosures[i].length;
				art.enclosures[i] = enc;
			}
		}
		return art;
	}

function Enclosure()
{
	this.url = "";
	this.type = "";
	this.length = "";
}

////////////////////////////////////////////////////////////////
// ArticleTree Model
////////////////////////////////////////////////////////////////

function dragArt(evt) {
// Returns "WINNT" on Windows Vista, XP, 2000, and NT systems;  
// "Linux" on GNU/Linux; and "Darwin" on Mac OS X.  
	var osString = Components.classes["@mozilla.org/xre/app-info;1"]  
               .getService(Components.interfaces.nsIXULRuntime).OS;
	if (osString == "Linux" && gOptions.linuxNoDragDrop) return;
  var tree = evt.target.parentNode;
  if (tree.id != "newsfox.articleTree") return;
  var row = {}, col = {}, type = {};
  tree.treeBoxObject.getCellAt(evt.clientX, evt.clientY, row, col, type);
  if (row.value > -1)
	{
  	dragArticle = 1;
		dragGrp = -1;
		var art = gCollect.get(row.value);
		evt.dataTransfer.setData("newsfox/arttreerow", "");
		evt.dataTransfer.setData("text/plain", art.title + "\n" + art.link + "\n" + art.date + "\n" + art.category + "\n");
		evt.dataTransfer.effectAllowed = "copyLink";
	}
}

function setArtTooltip()
{
	document.getElementById("artTreeChildren").tooltip = "artTooltip";
}

function rmArtTooltip()
{
	document.getElementById("artTreeChildren").removeAttribute("tooltip");
}

function fillTooltip(evt)
{
	if (!gOptions.artTooltip) return false;
	else
	{
		var tree = document.getElementById("newsfox.articleTree");
  	var row = {}, col = {}, type = {};
  	tree.treeBoxObject.getCellAt(evt.clientX, evt.clientY, row, col, type);
		var hide = false;
		if (row.value == -1 || !col.value) hide = true;
 		else
		{
			var TThidden = ((evt.clientX - tree.treeBoxObject.x - col.value.x) 
									< 0.5*col.value.width) && (type.value != "image");
			if (TThidden && col.value.id == "title") hide = true;
		}
		if (hide)
		{
			// need to remove tooltip to display default tooltip (long names)
			// apparently can be reenabled immediately, ART_TT_REENABLE_DELAY=1
			rmArtTooltip();
			setTimeout(setArtTooltip,ART_TT_REENABLE_DELAY);
			return false;
		}
	}

	var artTTDeck = document.getElementById("artDeck");
	var artIF =	document.getElementById("artD0");
	var artTT1 = document.getElementById("artD1a");
	var artTT2 = document.getElementById("artD1b");
	var artTT3 = document.getElementById("artD1c");
	var artTT4 = document.getElementById("artD1d");
	var artTT5 = document.getElementById("artD1e");
	var feed = gCollect.getFeed(row.value);
	var art = gCollect.get(row.value);
	artTTDeck.width = 1;
	artTTDeck.height = 1;

	switch (col.value.id)
	{
		case "title":
			if (type.value == "image")
			{
				artTTDeck.selectedIndex = 1;
				artTT2.hidden = true;
				artTT3.hidden = true;
				artTT4.hidden = true;
				artTT5.hidden = true;
				if (isGroup())
					artTT1.value = entityDecode(feed.getDisplayName());
				else
					artTT1.value = entityDecode(art.source.name);
			}
			else
			{
				artTTDeck.selectedIndex = 0;
				artTTDeck.width = ARTICLE_TOOLTIP_WIDTH;
				artTTDeck.height = ARTICLE_TOOLTIP_HEIGHT;
				var doc = artIF.contentDocument;
				artIF.docShell.allowJavascript = false;
				while (doc.body.childNodes.length > 0)
					doc.body.removeChild(doc.body.childNodes[0]);
				var p = getXhtmlBody(art.body,"span",doc);
				p = rmImages(p);
				doc.body.appendChild(p);
			}
			break;
		case "flag":
		case "read":
		case "date":
		case "tag":
			artTTDeck.selectedIndex = 1;
			artTT2.removeAttribute("hidden");
			artTT3.removeAttribute("hidden");
			artTT1.value = "<" + art.tag + ">";
			artTT2.value = "[" + entityDecode(art.category) + "]";
			artTT3.value = displayDate(art.date,2);
			if (art.author != "")
			{
				artTT4.removeAttribute("hidden");
				artTT4.value = document.getElementById("author").getAttribute("label") + ": " + entityDecode(art.author);
			}
			if (art.source.name != "")
			{
				artTT5.removeAttribute("hidden");
				artTT5.value = document.getElementById("source").getAttribute("label") + ": " + entityDecode(art.source.name);
			}
	}
	return true;
}

function ArticleTreeModel() 
{
	removeHeaderArrows();
  setColumns();
  this.rowCount = gCollect.size();
  this.getCellText = function(row,col)
  {
    // Try to handle both Firefox 1.0 and 1.1
    var colId = (col.id) ? col.id : col;
    switch (colId)
    {
      case "title":
			{
				var title = gCollect.get(row).title;
				var enDeTitle = entityDecode(title);
				if (gOptions.indent && row > 0 && 
								noReTitle(title) == noReTitle(gCollect.get(row-1).title))
					enDeTitle = "    " + enDeTitle;
				return enDeTitle;
			}
      case "date": return displayDate(gCollect.get(row).date, gOptions.dateStyle);
			case "source": return entityDecode(gCollect.get(row).source.name);
			case "author": return entityDecode(gCollect.get(row).author);
			case "blog": return entityDecode(gCollect.getFeed(row).getDisplayName());
			case "prob": return Math.round(100*gCollect.get(row).prob)/100;
			case "tag": return gCollect.get(row).tag;
      default: return "debug-" + col;
    }
  }
  this.setTree = function(treebox){ this.treebox = treebox; }
  this.isContainer = function(row){ return false; }
  this.isSeparator = function(row){ return false; }
  this.isSorted = function(row){ return false; }
  this.getLevel = function(row){ return 0; }
  this.getImageSrc = function(row,col)
  { 
    var read = gCollect.isRead(row);
    var flag = gCollect.isFlagged(row);
		var xtend = gCollect.isXtend(row);
		var xtenderror = gCollect.get(row).XtendError;

    // Try to handle both Firefox 1.0 and 1.1
    var colId = (col.id) ? col.id : col;
    switch (colId)
    {
      case "read": return read ? READICON : UNREADICON;
      case "flag": return flag ? FLAGICON : READICON;
			case "title": return getTitleIcon(row);
			case "Xtend": 
				return xtenderror ? ERRORICON : 
		xtend ? (gCollect.get(row).Xbody != "" ? UNREADICON : GETICON) : READICON;
      default: return null;
    }
  }
  this.getRowProperties = function(row,props)
	{
		var returnProps = "";
    if (!gCollect.isRead(row))
      returnProps = propsAdd(UNREAD, props, returnProps);
		if (gCollect.get(row).newUnread)
      returnProps = propsAdd(NEWUNREAD, props, returnProps);
    if (gCollect.isFlagged(row))
      returnProps = propsAdd(FLAG, props, returnProps);
		if (gCollect.get(row).id == null)
      returnProps = propsAdd(NOID, props, returnProps);
		var timeProp = HOUR + OTHER;
		var artTime = new Date() - gCollect.get(row).date;
		for (var i=HOURLEVELS.length-1; i>=0; i--)
			if (artTime < HOURMSEC*HOURLEVELS[i]) timeProp = HOUR + HOURLEVELS[i];
		returnProps = propsAdd(timeProp, props, returnProps);
		var timeProp2 = DAY + OTHER;
		for (i=DAYLEVELS.length-1; i>=0; i--)
			if (artTime < DAYMSEC*DAYLEVELS[i]) timeProp2 = DAY + DAYLEVELS[i];
		returnProps = propsAdd(timeProp2, props, returnProps);
		if (gOptions.spam)
		{
			var probNorm = (gCollect.get(row).prob - gCollect.minProb)/(gCollect.maxProb - gCollect.minProb);
			if (probNorm > HIGHPROBLEVEL)
				returnProps = propsAdd(HIGHPROB, props, returnProps);
			var level = Math.floor(PROBLEVELS*probNorm);
			if (level == PROBLEVELS) level--;
			var probProp = PROB + (PROBLEVELS - level);
			returnProps = propsAdd(probProp, props, returnProps);
		}
		if (gSearchValue != "" && gCollect.get(row).body.toLowerCase().indexOf(gSearchValue) > -1)
		{
			returnProps = propsAdd(ONCECLASS, props, returnProps);
		}
		for (var i=1; i<=gOptions.keyword.length; i++)
			if (gCollect.get(row).body.toLowerCase().indexOf(gOptions.keyword[i-1]) > -1)
			{
				returnProps = propsAdd(KEYWORD+i, props, returnProps);
				returnProps = propsAdd(KEYWORD, props, returnProps);
			}
		return(returnProps);
	}
  this.getCellProperties = function(row,col,props) 
  {
		var returnProps = "";
    // Try to handle both Firefox 1.0 and 1.1
    var colId = (col.id) ? col.id : col;
		if (colId == "title" && getTitleIcon(row))
			returnProps = propsAdd(FAVICONCOL, props, returnProps);
		if (colId == "Xtend" && gCollect.get(row).imagesToGet > 0)
			returnProps = propsAdd(NEEDSIMAGES, props, returnProps);
	// row props here so text can be styled
		if (props)
			this.getRowProperties(row,props);
		else
			returnProps += this.getRowProperties(row);
		return(returnProps);
  }
  this.getColumnProperties = function(colid,col,props) {}
  this.cycleHeader = function(col,elem)
  {
    // Try to handle both Firefox 1.0 and 1.1
    var colId = (col.id) ? col.id : col;
if (colId == "Xtend")
{
	var arttree = document.getElementById("newsfox.articleTree");
	var art;
	for (var i=0; i<gCollect.size(); i++)
		if (arttree.view.selection.isSelected(i))
			getXbody(gCollect.get(i),gCollect.getFeed(i));
	return;
}
		var colObj = document.getElementById(colId);
		var direction = colObj.getAttribute("sortDirection");
		removeHeaderArrows();
		var newDir;
		switch (colId)
		{
			case "flag":
			case "title":
			case "source":
			case "author":
			case "blog":
				newDir = "ascending";
				if (direction == "ascending") newDir = "descending";
				break;
			case "read":
			case "date":
			case "prob":
				newDir = "descending";
				if (direction == "descending") newDir = "ascending";	
		}
		var artId = getArtId();
		gCollect.artSort(colId,newDir);
    var arttree = document.getElementById("newsfox.articleTree");
		arttree.view.selection.select(-1);
		artTreeInvalidate();
		arttree.treeBoxObject.ensureRowIsVisible(0);
		if (artId) selectArt(artId);
		colObj.setAttribute("sortDirection", newDir);
  }
  this.cycleCell = function(row,col)
  {
    // Try to handle both Firefox 1.0 and 1.1
    var colId = (col.id) ? col.id : col;

    if (colId == "read") 
    {
      var read = gCollect.isRead(row);
			if (!read) gCollect.get(row).newUnread = false;
      gCollect.setRead(row, !read);
			feedTreeInvalidate();
			resetGroupUnread();
			setTitle(false);
    }
    else if (colId == "flag")
    {
      var flag = gCollect.isFlagged(row);
      gCollect.setFlagged(row, !flag);
    }
    else if (colId == "Xtend")
    {
			var art = gCollect.get(row);
      var xtend = art.Xtend;
			if (art.XtendError)
				art.XtendError = false;
			else
			{
				art.Xtend = !art.Xtend;
				if (xtend)
				{
					var rm = null;
					switch (gOptions.Xremove)
					{
						case 1:
							rm = true;
							break;
						case 0:
							const NF_SB = document.getElementById("newsfox-string-bundle");
							var msg = NF_SB.getString("removeXbody");
							rm = yesNoConfirm(msg);
							break;
						case -1:
							rm = false;
					}
					if (rm) art.Xbody = "";
					articleSelected();
					saveFeed(gCollect.getFeed(row));
				}
				else
					if (art.Xbody == "") getXbody(art, gCollect.getFeed(row));
					else articleSelected();
			}
    }
		artTreeInvalidate();
  }
  this.canDrop = function(row,orientation) { return false; }
}

function removeHeaderArrows()
{
	var flagObj = document.getElementById("flag");
	var titleObj = document.getElementById("title");
	var readObj = document.getElementById("read");
	var dateObj = document.getElementById("date");
	var sourceObj = document.getElementById("source");
	var authorObj = document.getElementById("author");
	var blogObj = document.getElementById("blog");
	var probObj = document.getElementById("prob");
	flagObj.setAttribute("sortDirection", "natural");
	titleObj.setAttribute("sortDirection", "natural");
	readObj.setAttribute("sortDirection", "natural");
	dateObj.setAttribute("sortDirection", "natural");
	sourceObj.setAttribute("sortDirection", "natural");
	authorObj.setAttribute("sortDirection", "natural");
	blogObj.setAttribute("sortDirection", "natural");
	probObj.setAttribute("sortDirection", "natural");
}

function setColumns()
{
  var dispCols = gOptions.artTreeCols;
  if (gCollect.type == 1 || gCollect.type == 5)
    if (gCollect.getFeed(0).artTreeCols != "")
      dispCols = gCollect.getFeed(0).artTreeCols;
  for (var i=0; i<colShort.length; i++)
  {
    var artC = document.getElementById(colLong[i]);
    if (dispCols.indexOf(colShort[i]) > -1)
      artC.removeAttribute("hidden");
    else
      artC.hidden = true;
  }
}

function columnsChanged()
{
  if (gCollect.type == 1 || gCollect.type ==5)
  {
    var feed = gCollect.getFeed(0);
    var usedColumns = getColumns();
    if (gOptions.artTreeCols == usedColumns)
      feed.artTreeCols = "";
    else
      feed.artTreeCols = usedColumns;
    saveFeed(feed);
  }
  else
  {
    gOptions.artTreeCols = getColumns();
    gOptions.save();
  }
}

function getColumns()
{
  var val = "";
  for (var i=0; i<colShort.length; i++)
     if (true != document.getElementById(colLong[i]).hidden) val += colShort[i];
  return val;
}

function getTitleIcon(row)
{
	if (isGroup()) return gCollect.getFeed(row).icon.src;
	else if (gCollect.get(row).source.url)
	{
		var feed = gFmodel.getFeedByURL(gCollect.get(row).source.url);
		if (feed) return feed.icon.src;
		else return ICON_OK;
	}
	else return null;
}

////////////////////////////////////////////////////////////////
// Collections
//
// type 0: regular group
// type 1: feed
// type 2: category
// type 3: search group
// type 4: tag group
// type 5: storage feed
////////////////////////////////////////////////////////////////

// AG: categoryNo isn't an index. 0 means root (or whole feed)
function NormalCollection(index, categoryNo, isDisplayed)
{
	this.minProb = 1;
	this.maxProb = 0;
	this.type = 1;  // feed
	if (categoryNo > 0) this.type = 2;
	var feed = gFmodel.get(index);
	if (feed.storage) this.type = 5;
  this.feed = feed;
	if (isDisplayed)
	{
		var title = feed.getDisplayName();
		var hasHomepage = (feed.homepage != "");
	}
  var items = new Array();
	var itemFeed = new Array();
	var itemIndex = new Array();
  var art;
  if (categoryNo > 0)
  {
    var categories = feed.getCategories();
    var ScatnameS = "\/" + categories[categoryNo - 1] + "\/";
  }
  for (var i=0; i<feed.size(); i++)
		if (categoryNo < 1)
		{
		  items.push(feed.get(i));
			itemFeed.push(index);
			itemIndex.push(i);
			this.minProb = Math.min(this.minProb,feed.get(i).prob);
			this.maxProb = Math.max(this.maxProb,feed.get(i).prob);
		}
		else
		{
		  art = feed.get(i);
			var ScatS = "\/" + art.category + "\/";
		  if(ScatS.indexOf(ScatnameS) > -1)
			{
		    items.push(art);
				itemFeed.push(index);
				itemIndex.push(i);
				this.minProb = Math.min(this.minProb,feed.get(i).prob);
				this.maxProb = Math.max(this.maxProb,feed.get(i).prob);
			}
		}
  this.size = function() { return items.length; }
	if (isDisplayed) setFeedbarButtons(this.type,title,hasHomepage,this.size());

  this.get  = function(index) { return items[index]; }
	this.isRead = function(row) { return feed.isRead(itemIndex[row]); }
	this.setRead = function(row,value) { feed.setRead(itemIndex[row],value); }
	this.isFlagged = function(row) { return feed.isFlagged(itemIndex[row]); }
	this.setFlagged = function(row,value)
		{ feed.setFlagged(itemIndex[row],value); }
	this.isXtend = function(row) { return feed.isXtend(itemIndex[row]); }
	this.setXtend = function(row,value) { feed.setXtend(itemIndex[row],value); }
	this.getFeed = function(index) { return feed; }
	this.artSort = function(by,dir)
		{ artSort(by,dir,items,itemFeed,itemIndex); }
	this.sortStr = feed.sortStr;
	if (feed.sortStr.substring(0,1) == "g") this.sortStr = gOptions.sortStr;
	this.getTitle = function() { return title; }
}

function GroupCollection(grpindex, isSearch)
{
	this.minProb = 1;
	this.maxProb = 0;
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var srchitem = new Array();
	var srchstr = null;
	this.grpindex = grpindex;
	var grp = gFdGroup[grpindex];
	var grpHeading;
	if (isSearch)
	{
		if (grp.searchTag)
		{
			this.type = 4;  // tag
			grpHeading = NF_SB.getString('tagName');
			grp.list = gFdGroup[0].list;
		}
		else
		{
			this.type = 3;  // search
			grpHeading = NF_SB.getString('srchName');
			var now = new Date().getTime();
			if (grp.srchdat.text != "") srchstr = mkSrchstr(grp.srchdat,srchitem);
		}
	}
	else
	{
		this.type = 0;  // group
		grpHeading = NF_SB.getString('grpName');
	}
  var title = grpHeading + " " + grp.title;
  var items = new Array();
	var itemIndex = new Array();
	var itemFeed = new Array();
  var art, feed, nFeed;
  for (var i=0; i<grp.list.length; i++)
	{
		nFeed = grp.list[i];
		feed = gFmodel.get(nFeed);
		for (var j=0; j<feed.size(); j++)
			if (this.type == 0 || 
					(this.type == 4 && hasTag(j, feed, grp.searchTag)) || 
					(this.type == 3 && hasPropSandbox.hasProp(j, feed, grp.srchdat, srchstr, srchitem, now)))
			{
		  	items.push(feed.get(j));
				itemIndex.push(j);
				itemFeed.push(nFeed);
				this.minProb = Math.min(this.minProb,feed.get(j).prob);
				this.maxProb = Math.max(this.maxProb,feed.get(j).prob);
			}
	}
  this.size = function() { return items.length; }
	setFeedbarButtons(this.type,title,false,this.size());

	this.artSort = function(by,dir)
		{ artSort(by,dir,items,itemFeed,itemIndex); }
	this.sortStr = gOptions.sortStrG;

  this.get  = function(index) { return items[index]; }
	this.isRead = function(row)
		{ return gFmodel.get(itemFeed[row]).isRead(itemIndex[row]); }
	this.setRead = function(row,value)
		{ gFmodel.get(itemFeed[row]).setRead(itemIndex[row],value); }
	this.isFlagged = function(row)
		{ return gFmodel.get(itemFeed[row]).isFlagged(itemIndex[row]); }
	this.setFlagged = function(row,value)
		{ gFmodel.get(itemFeed[row]).setFlagged(itemIndex[row],value); }
	this.isXtend = function(row)
		{ return gFmodel.get(itemFeed[row]).isXtend(itemIndex[row]); }
	this.setXtend = function(row,value)
		{ gFmodel.get(itemFeed[row]).setXtend(itemIndex[row],value); }
	this.getFeed = function(index) { return gFmodel.get(itemFeed[index]); }
	this.getSrchText = function() { return srchitem; }
	this.getTitle = function() { return title; }
}

function EmptyCollection()
{
	var items = new Array();
	this.type = -1;  // empty
  this.size = function() { return items.length; }
	setFeedbarButtons(this.type,"?",false,this.size());
}

////////////////////////////////////////////////////////////
/////   Collection utilities
///////////////////////////////////////////////////////////

function isGroup()
{
	return (gCollect.type == 0 || gCollect.type == 3 || gCollect.type == 4);
}

function isFeed()
{
	return (gCollect.type == 1 || gCollect.type == 5);
}

function hasTag(j, feed, tag)
{
	var StagS = "/" + tag + "/";
	var SarttagS = "/" + feed.get(j).tag + "/";
	return (SarttagS.indexOf(StagS) > -1);
}

function hasProp(j, feed, srchdat, srchstr, st, now)
{
	if ((srchdat.flagged != 2) && (srchdat.flagged == feed.isFlagged(j)))
		return false;
	if ((srchdat.unread != 2) && (srchdat.unread != feed.isRead(j)))
		return false;

	var art = feed.get(j);
	var artTime = art.date.getTime();
	if (artTime > now - srchdat.endTime) return false;
	if (srchdat.startTime > 0 && now - srchdat.startTime > artTime) return false;

	if (!srchstr) return true;
	var caseSen = ((srchdat.textflags & 0x04) == 0);
	var matchWhat = (srchdat.textflags & 0x0B);
	var arttext = "";
	arttext += (matchWhat & 0x01) ? art.body : "";
	arttext += (matchWhat & 0x02) ? art.title : "";
	arttext += (matchWhat & 0x08) ? "/" + art.tag + "/" : "";
	if (!caseSen)
		arttext = arttext.toLowerCase();

	return eval(srchstr);
}
var hasPropSandbox = new Components.utils.Sandbox(window);
hasPropSandbox.importFunction(hasProp);

function srchText(srchtext, arttext)
{
	return (arttext.indexOf(srchtext) != -1);
}

function mkSrchstr(srchdat,st)
{
	var caseSen = ((srchdat.textflags & 0x04) == 0);
	var srchtext = srchdat.text;
	if (!caseSen)
		srchtext = srchtext.toLowerCase();

	var srchstr = srchtext;
	var i = 0;
// preprocess quoted stuff
	var postext = 0;
	var done = false;
	var malformed = false;
	var item, repl, newrepl;
	while (!done)
	{
		var q1 = srchtext.indexOf("'",postext);
		var q2 = srchtext.indexOf('"',postext);
		if (q1 == -1 && q2 == -1) done = true;
		else if ((q1 > -1 && q1 < q2) || q2 == -1)
		{
			var q3 = srchtext.indexOf("'",q1+1);
			if (q3 == -1) // unbalanced error
			{
				malformed = true;
				srchtext += "'";
				srchstr += "'";
				q3 = srchtext.indexOf("'",q1+1);
			}
			item = srchtext.substring(q1+1,q3);
			newrepl = '"st[' + i + ']"';
			st[i++] = item;
			repl = "'" + item + "'";
			srchstr = srchstr.replace(repl,newrepl);
			postext = q3+1;
		}
		else   // -1 < q2 < q1 || q1 == -1
		{
			var q3 = srchtext.indexOf('"',q2+1);
			if (q3 == -1) // unbalanced error
			{
				malformed = true;
				srchtext += '"';
				srchstr += '"';
				q3 = srchtext.indexOf('"',q2+1);
			}
			item = srchtext.substring(q2+1,q3);
			newrepl = '"st[' + i + ']"';
			st[i++] = item;
			repl = '"' + item + '"';
			srchstr = srchstr.replace(repl,newrepl);
			postext = q3+1;
		}
	}

// process string
	postext = 0;
	var pos;
	done = false;
	var inParen = 0;
	var nextIsItem = true;
	while (postext < srchstr.length)
	{
		switch (srchstr.charAt(postext))
		{
			case '"':
//alert("case\"");
				postext = srchstr.indexOf('"',postext+1)+1;
				nextIsItem = false;
				break;
			case "-":
//alert("case-");
				if (srchstr.charAt(postext+1) != ' ' && srchstr.charAt(postext+1) != '|')
				{
					srchstr = srchstr.substring(0,postext) + "!" + srchstr.substring(postext+1);
					postext++;
				}
				else
				{
					if (nextIsItem)
					{
						malformed = true;
						srchstr = srchstr.substring(0,postext+1) + srchstr.substring(postext+2);
					}
					else
					{
						malformed = true;
						srchstr = srchstr.substring(0,postext) + srchstr.substring(postext+1);
					}
				}
				break;
			case "(":
//alert("case(");
				inParen++;
				postext++;
				break;
			case ")":
//alert("case)");
				if (inParen > 0 && !nextIsItem)
				{
					inParen--;
					postext++;
				}
				else
				{
					malformed = true;
					srchstr = srchstr.substring(0,postext) + srchstr.substring(postext+1);
				}
				break;
			case "|":
//alert("case|");
				if (!nextIsItem)
				{
					srchstr = srchstr.substring(0,postext) + " || " + srchstr.substring(postext+1);
					postext += 4;
					nextIsItem = true;
				}
				else
				{
					malformed = true;
					srchstr = srchstr.substring(0,postext) + srchstr.substring(postext+1);
				}
				break;
			case " ":
//alert("case-space");
				pos = postext;
				while (srchstr.charAt(pos) == " ") pos++;
				switch (srchstr.charAt(pos))
				{
					case "|":
					case ")":
						srchstr = srchstr.substring(0,postext) + srchstr.substring(pos);
						break;
					case "o":
					case "O":
						if (srchstr.charAt(pos+1).toLowerCase() == "r" && srchstr.charAt(pos+2) == " ")
						{
							srchstr = srchstr.substring(0,postext) + "|" + srchstr.substring(pos+3);
							break;
						}
					default:
						if (pos >= srchstr.length)
						{
							postext = pos;
							break;
						}
						if (nextIsItem)
							srchstr = srchstr.substring(0,postext) + srchstr.substring(pos);
						else
						{
							srchstr = srchstr.substring(0,postext) + " && " + srchstr.substring(pos);
							postext += 4;
							nextIsItem = true;
						}
				}
				break;
			default: // search item
//alert("case");
				if (nextIsItem)
				{
					pos = postext;
					while (srchstr.charAt(pos) != '|' && srchstr.charAt(pos) != ' ' && srchstr.charAt(pos) != ')' && pos < srchstr.length) pos++;
					if (srchstr.charAt(pos) == ')' && inParen <= 0)
						while (srchstr.charAt(pos) != '|' && srchstr.charAt(pos) != ' ' && pos < srchstr.length) pos++;
					item = srchstr.substring(postext,pos);
					newrepl = '"st[' + i + ']"';
					st[i++] = item;
					srchstr = srchstr.replace(item,newrepl);
					postext = srchstr.indexOf('"',postext+1)+1;
					nextIsItem = false;
				}
				else
				{
					malformed = true;
					srchstr = srchstr.substring(0,postext) + " && " + srchstr.substring(postext+1);
					postext += 4;
					nextIsItem = true;
				}
		}
	}

	if (i == 0) return "true";
	if (nextIsItem)
	{
		malformed = true;
		var lastAnd = srchstr.lastIndexOf('&&');
		var lastOr = srchstr.lastIndexOf('||');
		var lastOp = Math.max(lastAnd, lastOr);
		while (srchstr.lastIndexOf('(') > lastOp)
		{
			srchstr = srchstr.substring(0,srchstr.lastIndexOf('('));
			inParen--;
		}
		var lastParen = srchstr.lastIndexOf('(');
		srchstr = srchstr.substring(0,lastOp);
	}

	while (inParen > 0)
	{
		srchstr += ")";
		inParen--;
	}

	if (malformed)
	{
		const NF_SB = document.getElementById("newsfox-string-bundle");
		var msg = NF_SB.getString('malformed');
		msg += "\n\n" + NF_SB.getString('searchtext') + srchtext + "\n";
		srchstrG = srchstr.replace(/!/g,"-");
		srchstrG = srchstrG.replace(/ && /g, " ");
		srchstrG = srchstrG.replace(/ \|\| /g, " \| ");
		msg += NF_SB.getString('actualsearch') + srchstrG + "\n\n";
	}
	for (var j=0; j<i; j++)
	{
		repl = '"st[' + j + ']"';
		newrepl = "srchText(st[" + j + "], arttext)";
		srchstr = srchstr.replace(repl,newrepl);
		if (malformed) msg += "st[" + j + "]= " + st[j] + "\n";
	}
	if (malformed) alert(msg);

	return srchstr;
}

function setFeedbarButtons(type,title,hasHomepage,cSize)
{
	var empty = (cSize == 0);
	document.getElementById("feedTitle").value = document.getElementById("mfeedTitle").value = title;
  var feedbarHome = document.getElementById("fBhome");
	feedbarHome.setAttribute("disabled",false);
	if (!hasHomepage || type == 5) feedbarHome.setAttribute("disabled",true);
  var feedbarRefresh = document.getElementById("fBcheck");
	feedbarRefresh.setAttribute("disabled",false);
	if (type == 2 || type == 5 || type == 4 || type == -1)
		feedbarRefresh.setAttribute("disabled",true);
  var feedbarTag = document.getElementById("fBtag");
	feedbarTag.setAttribute("disabled",false);
	if (empty) feedbarTag.setAttribute("disabled",true);
  var feedbarMarkread = document.getElementById("fBmarkAllAsRead");
	feedbarMarkread.setAttribute("disabled",false);
	if (empty) feedbarMarkread.setAttribute("disabled",true);
  var feedbarMarkunread = document.getElementById("fBmarkAllAsUnread");
	feedbarMarkunread.setAttribute("disabled",false);
	if (empty) feedbarMarkunread.setAttribute("disabled",true);
  var feedbarDelete = document.getElementById("fBdelete");
	feedbarDelete.setAttribute("disabled",false);
	if (empty) feedbarDelete.setAttribute("disabled",true);
  var feedbarOptions = document.getElementById("fBoptions");
	feedbarOptions.setAttribute("disabled",false);
	if (type == 2 || type == -1 || type == 4)
		feedbarOptions.setAttribute("disabled",true);
	var text = null;
	if (type == 0 || type == 3) text = "gtext";
	else if (type == 1 || type == 5) text = "ftext";
	if (text)
		document.getElementById("fBoptions").setAttribute("tooltiptext",
				document.getElementById("fBoptions").getAttribute(text));
	else
		document.getElementById("fBoptions").removeAttribute("tooltiptext");
}

////////////////////////////////////////////////////////////
/////   Sorting
///////////////////////////////////////////////////////////

function noReTitle(title)
{
	return title.toLowerCase().replace(/^re:\s*/,"");
}

function artSort(by,dir,items,itemFeed,itemIndex)
{
// sorter chooser
	var abc = function(by,dir)
	{
		if (dir != "ascending" && dir != "descending") return null;
		switch (by)
		{
			case "flag":
				if (dir == "descending") return FlagDown;
				else return FlagUp;
			case "title":
				if (dir == "descending") return TitleDown;
				else return TitleUp;
			case "read":
				if (dir == "descending") return ReadDown;
				else return ReadUp;
			case "date":
				if (dir == "descending") return DateDown;
				else return DateUp;
			case "source":
				if (dir == "descending") return SourceDown;
				else return SourceUp;
			case "author":
				if (dir == "descending") return AuthorDown;
				else return AuthorUp;
			case "blog":
				if (dir == "descending") return FeedDown;
				else return FeedUp;
			case "prob":
				if (dir == "descending") return ProbDown;
				else return ProbUp;
			case "orderThread":
				if (dir == "descending") return ThrDown;
				else return ThrUp;
		}
	return null;
	}

// sorters
	var FlagUp = function(a,b)
	{
		var tmp = ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x04) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x04));
		if (tmp > 0) return 1;
		else if (tmp < 0) return -1;
		else return (a < b) ? -1 : 1;
	}
// gecko sort stable is bug#224128, fixed in FF3
// when gecko sort stable, can replace FlagUp body with the following line 
//		{ return ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x04) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x04)); };
	var FlagDown = function(a,b)
	{
		var tmp = ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x04) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x04));
		if (tmp > 0) return -1;
		else if (tmp < 0) return 1;
		else return (a < b) ? -1 : 1;
	}
// when stable use next line
// 		{ return FlagUp(b,a); };

	var TitleUp = function(a,b)
	{
		var tB = noReTitle(items[b].title);
		var tA = noReTitle(items[a].title);
		if (tB == tA) return (a < b) ? -1 : 1;
		else return (tA < tB ? -1 : 1);
	}
// when stable use next line
//	{ return (items[a].title.toLowerCase() < items[b].title.toLowerCase() ? -1 : 1); };
	var TitleDown = function(a,b)
	{
		var tB = noReTitle(items[b].title);
		var tA = noReTitle(items[a].title);
		if (tB == tA) return (a < b) ? -1 : 1;
		else return (tA < tB ? 1 : -1);
	}
// when stable use next line
//	var TitleDown = function(a,b) { return TitleUp(b,a); };

	var ReadUp = function(a,b)
	{
		var tmp = ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x01) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x01));
		if (tmp > 0) return 1;
		else if (tmp < 0) return -1;
		else return (a < b) ? -1 : 1;
	}
// when stable use next line
//		{ return ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x01) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x01)); };
	var ReadDown = function(a,b)
	{
		var tmp = ((gFmodel.get(itemFeed[b]).flags[itemIndex[b]] & 0x01) - (gFmodel.get(itemFeed[a]).flags[itemIndex[a]] & 0x01));
		if (tmp > 0) return -1;
		else if (tmp < 0) return 1;
		else return (a < b) ? -1 : 1;
	}
// when stable use next line
// 		{ return ReadUp(b,a); };

	var DateUp = function(a,b)  // don't worry about ties
	{ return (items[a].date < items[b].date ? -1 : 1); };
	var DateDown = function(a,b) { return DateUp(b,a); };

	var SourceUp = function(a,b)
	{
		if (items[b].source.name.toLowerCase() == items[a].source.name.toLowerCase()) return (a < b) ? -1 : 1;
		else return (items[a].source.name.toLowerCase() < items[b].source.name.toLowerCase() ? -1 : 1);
	}
// when stable use next line
//	{ return (items[a].source.name.toLowerCase() < items[b].source.name.toLowerCase() ? -1 : 1); };
	var SourceDown = function(a,b)
	{
		if (items[b].source.name.toLowerCase() == items[a].source.name.toLowerCase()) return (a < b) ? -1 : 1;
		else return (items[a].source.name.toLowerCase() < items[b].source.name.toLowerCase() ? 1 : -1);
	}
// when stable use next line
//	{ return SourceUp(b,a); };

	var AuthorUp = function(a,b)
	{
		if (items[b].author.toLowerCase() == items[a].author.toLowerCase()) return (a < b) ? -1 : 1;
		else return (items[a].author.toLowerCase() < items[b].author.toLowerCase() ? -1 : 1);
	}
// when stable use next line
//	{ return (items[a].author.toLowerCase() < items[b].author.toLowerCase() ? -1 : 1); };
	var AuthorDown = function(a,b)
	{
		if (items[b].author.toLowerCase() == items[a].author.toLowerCase()) return (a < b) ? -1 : 1;
		else return (items[a].author.toLowerCase() < items[b].author.toLowerCase() ? 1 : -1);
	}
// when stable use next line
// { return AuthorUp(b,a); };

	var FeedUp = function(a,b)
	{
		if (gFmodel.get(itemFeed[b]).getDisplayName().toLowerCase() == gFmodel.get(itemFeed[a]).getDisplayName().toLowerCase()) return (a < b) ? -1 : 1;
		else return (gFmodel.get(itemFeed[a]).getDisplayName().toLowerCase() < gFmodel.get(itemFeed[b]).getDisplayName().toLowerCase() ? -1 : 1);
	}
// when stable use next line
//	{ return (gFmodel.get(itemFeed[a]).getDisplayName().toLowerCase() < gFmodel.get(itemFeed[b]).getDisplayName().toLowerCase() ? -1 : 1); };
	var FeedDown = function(a,b)
	{
		if (gFmodel.get(itemFeed[b]).getDisplayName().toLowerCase() == gFmodel.get(itemFeed[a]).getDisplayName().toLowerCase()) return (a < b) ? -1 : 1;
		else return (gFmodel.get(itemFeed[a]).getDisplayName().toLowerCase() < gFmodel.get(itemFeed[b]).getDisplayName().toLowerCase() ? 1 : -1);
	}
// when stable use next line
// { return FeedUp(b,a); };

	var ProbUp = function(a,b)  // don't worry about ties
	{ return (items[a].prob < items[b].prob ? -1 : 1); };
	var ProbDown = function(a,b) { return ProbUp(b,a); };

	var ThrUp = function(a,b)  // don't worry about ties
	{ return (items[a].thr < items[b].thr ? -1 : 1); };
	var ThrDown = function(a,b) { return ThrUp(b,a); };

// movers
	var tmpLoad = function(open,j,tmpA,tmpIF,tmpI)
	{
		tmpA[open] = items[j];
		tmpIF[open] = itemFeed[j];
		tmpI[open] = itemIndex[j];
	}

	var tmpUnload = function(j,open,tmpA,tmpIF,tmpI)
	{
		items[j] = tmpA[open];
		itemFeed[j] = tmpIF[open];
		itemIndex[j] = tmpI[open];
	}

// the sort function
	var sorter = abc(by,dir);
	var N = items.length;
	var dummy = new Array(N);
	var invdum = new Array(N);
	var done = new Array(N);
	var tmpA = new Array();
	var tmpIF = new Array();
	var tmpI = new Array();
	for (var i=0; i<N; i++)
	{
		dummy[i] = i;
		done[i] = false;
	}
	dummy.sort(sorter);
	for (i=0; i<N; i++)
		invdum[dummy[i]] = i;
	var j, open;
	for (i=0; i<N; i++)
		if (!done[i])
		{
			j = invdum[i];
			tmpLoad(0,i,tmpA,tmpIF,tmpI);
			open = 1;
			while (j != i)
			{
				tmpLoad(open,j,tmpA,tmpIF,tmpI);
				open = 1 - open;
				tmpUnload(j,open,tmpA,tmpIF,tmpI);
				done[dummy[j]] = true;
				j = invdum[j];
			}
			tmpUnload(j,1-open,tmpA,tmpIF,tmpI);
			done[dummy[j]] = true;
		}
}
