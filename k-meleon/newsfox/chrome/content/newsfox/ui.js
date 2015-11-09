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

// shared with rss.js
var gAutoRefreshTimer = null;
var gCheckInProgress = false;
var gFeedsToCheck = new Array();
var gDisplayArticle = true;
var gLoadingTimeout;
var gBookmarkSync;

// ui.js only
var defaultURL = "";
var firstLoad = true;
var loadIndex;
var initNewsfox = false;
var clean = false;
var feedSelectTimeout;

////////////////////////////////////////////////////////////////
// Lifecycle
////////////////////////////////////////////////////////////////

function startup()
{
  // This method was getting called twice, not sure why
  // so short circuit if method already invoked.
  if (initNewsfox) return;
	initNewsfox = true;
	gSdr = Components.classes["@mozilla.org/security/sdr;1"]
		.createInstance(Components.interfaces.nsISecretDecoderRing);
	NFsetUserAgent();
	document.getElementById("notBusyText").value = NEWSFOX + " " + VERSION;
	gOptions.startup();  
	doHorizontal(gOptions.horiz);
	getAccel();
	gCollect = new EmptyCollection();
	gBookmarkSync = new BookmarkSync();
  loadModels(true);
	if (gOptions.moveAuto != 0) moveAutoCheck();
	backupOpml();
	setTitle(false);
  setTimeout(loadAllFeeds,50);
	doKeywordCopy("read");
	getNumGroups();
  checkArtTreeColumnChange();

	showHref("chrome://newsfox/content/help/start.xhtml");
	var newFeedUrl = gOptions.addUrl;
	if (newFeedUrl != "") addFeedUrl();
  if(gOptions.checkOnStartup)
	{
		if (newFeedUrl != "" && !gCheckInProgress)
			gFeedsToCheck.push(newFeedUrl);   // check new feed first
    checkFeeds();
	}
	if( gOptions.autoRefreshTime != -1 ) checkAutoFeeds();
  doLivemarks(true);
  getImgsForTextview();
}

function saveModels()
{
  saveFeedModel();
  saveGroupModel();
  saveIndices();
}

function refreshModel()
{
  loadFeedModel();
  loadGroupModel();
  loadIndices();
  var tree = document.getElementById("newsfox.feedTree");
	tree.view = new FeedTreeModel();
}

function refreshModelSelect(index)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var fRow = feedtree.treeBoxObject.getFirstVisibleRow();
	feedtree.view = new FeedTreeModel();
  feedtree.treeBoxObject.scrollToRow(fRow);
  feedtree.view.selection.select(index);
  feedtree.treeBoxObject.ensureRowIsVisible(index);
}

function cleanup()
{
  if (!initNewsfox) return;
  if (clean) return;
	doKeywordCopy("write");
	if( null != gAutoRefreshTimer )
		clearTimeout(gAutoRefreshTimer);
	gOptions.rmObserver();
	window.status = "";
	if (!modelCorrupt())
	{
		saveModels();
		if (gOptions.spam)
		{
			spamCleanup();
			saveFilterData();
		}
	}
  clean = true;
}

function spamCleanup()
{
	var sorter = function(a,b)
	{
		var tmp =  gTotalArray[b] - gTotalArray[a];
		if (tmp > 0) return 1;
		else if (tmp < 0) return -1;
		else return ((gGoodArray[b] > gGoodArray[a]) ? 1 : -1);
	}

	var dummy = new Array();
	var tWordArray = new Array();
	var tGoodArray = new Array();
	var tTotalArray = new Array();
	for (var i=0; i<gWordArray.length; i++)
	{
		dummy[i] = i;
		tWordArray[i] = gWordArray[i];
		if (gTotalArray[i] > S_BIGNUM)
		{
			tGoodArray[i] = gGoodArray[i]/S_BIGNUMDIV;
			tTotalArray[i] = gTotalArray[i]/S_BIGNUMDIV;
		}
		else
		{
			tGoodArray[i] = gGoodArray[i];
			tTotalArray[i] = gTotalArray[i];
		}
	}
	dummy.sort(sorter);
	for (var i=0; i<gWordArray.length; i++)
	{
		gTotalArray[i] = Math.round(S_MULT*tTotalArray[dummy[i]]);
		if (gTotalArray[i] < S_MINWORDTOTAL && i >= S_MINWORDS) break;
		gWordArray[i] = tWordArray[dummy[i]];
		gGoodArray[i] = Math.round(S_MULT*tGoodArray[dummy[i]]);
	}
	var maxlen = Math.min(S_MAXWORDS, i);
	if (gWordArray.length > maxlen)
	{
		gWordArray.length = maxlen;
		gGoodArray.length = maxlen;
		gTotalArray.length = maxlen;
	}
}

function modelCorrupt()
{
	return (gLoadFlags > 0 || gIdx.fdgp.length == 0 || gFdGroup.length == 0);
}

function loadModels(keepTrying)
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
  var file = NFgetProfileDir();
  file.append(MASTER+".xml");
  var hasFeeds = file.exists();
  var file = NFgetProfileDir();
  file.append(MASTER_INDEX+".xml");
  var hasGroups = file.exists();
  if (!hasGroups)
  {
    gFdGroup[0] = new FeedGroup();
    gFdGroup.length = 1;
    gFdGroup[0].title = NF_SB.getString('FEEDS');
    gFdGroup[0].expanded = false;
    gIdx.fdgp[0] = 0;
    gIdx.feed[0] = -1;
    gIdx.catg[0] = 0;
    gIdx.open[0] = false;
    if (hasFeeds)
    {
      loadFeedModel();
      for (var i=0; i<gFmodel.size(); i++)
        gFdGroup[0].list[i] = i;
      gFdGroup[0].list.length = gFmodel.size();
    }
    else
    {
      gFdGroup[0].list.length = 0;
    }
    saveGroupModel();
    saveIndices();
  }
  if (hasFeeds) loadFeedModel();
  loadGroupModel();
  loadIndices();

	// handle errors
	if (keepTrying && modelCorrupt())
	{
    alert(NF_SB.getString('filecorrupt'));
		doReset(MASTER_INDEX);
		doReset(MASTER_GROUP);
		doReset(MASTER);
		loadModels(false);
		return;
	}

  var feedtree = document.getElementById("newsfox.feedTree");
	feedtree.view = new FeedTreeModel();
  if (!hasGroups && hasFeeds) feedtree.view.toggleOpenState(0);
	loadFilterData();
}

function doReset(leafName)
{
	var bakExists = false;
	var xml = NFgetProfileDir();
	xml.append(leafName+".xml");
	var bak = NFgetProfileDir();
	bak.append(leafName+".bak");
	if (xml.exists()) xml.remove(false);
	if (bak.exists())
	{
		bakExists = true;
		bak.moveTo(NFgetProfileDir(),leafName+".xml");
	}
	return bakExists;
}

function loadAllFeeds()
{
  if (!firstLoad) return;
  firstLoad = false;
	loadIndex = gFmodel.size() - 1;
	if (loadIndex < 0) { gAllFeedsLoaded = true; return; }
	gLoadingTimeout = setTimeout(loadAFeed,50);
}

function loadAFeed()
{
	if (loadIndex < 0)
	{
		gAllFeedsLoaded = true;
		resetGroupUnread();
		feedTreeInvalidate();
		return;
	}
	loadFeed(gFmodel.get(loadIndex),false,false);
	loadIndex--;
	gLoadingTimeout = setTimeout(loadAFeed,50);
}

////////////////////////////////////////////////////////////////
// Commands
////////////////////////////////////////////////////////////////

function doCheckCancel()
{
	if (gCheckInProgress) cancelTheCheck();
	else feedCheck(true);
}

function doCancelCheckFeeds() { if (gCheckInProgress) cancelTheCheck(); }

/**
 * Check feeds for new items.
 */
function feedCheck(button)
{
// need to pass array of URLs in case feeds are deleted during check
  if (gCheckInProgress) return;
	setUpCheck();
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  var level = feedtree.view.getLevel(index);
  if (index == -1 || button) level = -1;
  switch(level) 
  {
    case 0:  // check in group
      var curGrp = gIdx.fdgp[index];
      for (var i=0; i<gFdGroup[curGrp].list.length; i++)
				gFeedsToCheck.push(gFmodel.get(gFdGroup[curGrp].list[i]).url);
      break;
		case 2:  // in category, check its feed
			while (feedtree.view.getLevel(index) != 1) index--;
			feedtree.view.toggleOpenState(index);
  		feedtree.view.selection.select(index);
    case 1:  // check feed
      gFeedsToCheck.push(gFmodel.get(gIdx.feed[index]).url);
      break;
    case -1:  // none selected or button, do auto
			var autoFeed = false;
			if (gFeedsToCheck.length > 0) autoFeed = true; 
      for (var i=0; i<gFdGroup[0].list.length; i++)
        if (gFmodel.get(gFdGroup[0].list[i]).autoCheck)
          gFeedsToCheck.push(gFmodel.get(gFdGroup[0].list[i]).url);
			var rootList = gFdGroup[0].list;
			var checkLast = gFmodel.get(rootList[rootList.length-1]).autoCheck;
			if (autoFeed)
				if (checkLast) gFeedsToCheck.length--;  // don't do new feed twice
				else gFeedsToCheck.shift();  // don't do new feed at all
  }
	for (var i=gFeedsToCheck.length-1; i>=0; i--)
		if (gFeedsToCheck[i] == "") gFeedsToCheck.splice(i,1);
	gNewItemsCount = 0;
  setupFeedCheck(gFeedsToCheck,true);
}

function setUpCheck()
{
	gCheckInProgress = true;
	document.getElementById("tBcheck").setAttribute("hidden",true);
	document.getElementById("tBcancel").removeAttribute("hidden");
	document.getElementById("mfBcheck").setAttribute("hidden",true);
	document.getElementById("mfBcancel").removeAttribute("hidden");
	document.getElementById("fBcheck").setAttribute("hidden",true);
	document.getElementById("fBcancel").removeAttribute("hidden");
	document.getElementById("busyText").removeAttribute("hidden");
	document.getElementById("notBusyText").hidden = "true";
	if (NFgetPref("advanced.showPauseButton","bool",true))
		document.getElementById("pause-icon").removeAttribute("hidden");
	setBusyText("checking");
}

/**
 * Auto check feeds.
 */
function checkFeeds()
{
	if (gCheckInProgress)
	{
		setTimeout(checkFeeds,500);
		return;
	}
  feedCheck(true);
}

function checkAutoFeeds()
{
	if (gCheckInProgress)
	{
		setTimeout(checkAutoFeeds,500);
		return;
	}
	setUpCheck();
	for (var i=0; i<gFdGroup[0].list.length; i++)
	{
		var feed = gFmodel.get(gFdGroup[0].list[i]);
		var beenTooLong = false;
		var now = new Date();
		var aRI = feed.autoRefreshInterval;
		if (aRI == 0) aRI = gOptions.autoRefreshInterval;
		if (aRI >= 0)
			beenTooLong = (now - feed.lastUpdate) > (60000 * aRI);
		if (beenTooLong) gFeedsToCheck.push(feed.url);
	}
	gNewItemsCount = 0;
  setupFeedCheck(gFeedsToCheck,true);
	gAutoRefreshTimer = setTimeout(checkAutoFeeds, gOptions.autoRefreshTime);
}

/**
 * Add a new group.
 */
function addGroup(type)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var num = gFdGroup.length;
  var grp = new FeedGroup();
	if (type != "regular") grp.search = true;
  gFdGroup.push(grp);
  var index = gIdx.fdgp.length;
  gIdx.fdgp.push(num);
  gIdx.feed.push(-1);
  gIdx.catg.push(0);
  gIdx.open.push(false);
  saveIndices();
  saveGroupModel();
  feedtree.treeBoxObject.rowCountChanged(index,1);
  if (type != "tag") showGroupOptions(index,true,(type == "search"));
	else
	{
		mvGrp(num,1);
		feedtree.view.selection.select(getGroupRow(1));
	}
}

function addTagGroup(index)
{
	addGroup("tag");
	updateTagGroup(index);
}

function updateTagGroup(index)
{
	var tags;
	if (gTag == "")
		tags = new Array();
	else
		tags = gTag.split("\/");
	if (index >= tags.length)
	{
		const NF_SB = document.getElementById("newsfox-string-bundle");
  	var newTagPrmpt = NF_SB.getString('newtagprompt');
		var newTag = window.prompt(newTagPrmpt,"",newTagPrmpt);
		var SgTagS = "\/" + gTag + "\/";
		if (newTag && SgTagS.indexOf("\/"+newTag+"\/") == -1)
		{
			newTag = newTag.replace(/\//g,"");
			if (gTag != "") gTag += "\/" + newTag;
			else gTag = newTag;
			tags.push(newTag);
		}
		else if (newTag)
		{
			for (index=0; index<tags.length; index++)
				if (tags[index] == newTag) break;
		}
		else
		{
			deleteGroup(false);
			return;
		}
	}
	var feedtree = document.getElementById("newsfox.feedTree");
	var fTindex = feedtree.currentIndex;
	var curGrp = gIdx.fdgp[fTindex];
	gFdGroup[curGrp].title = tags[index];
	gFdGroup[curGrp].searchTag = tags[index];
	gFdGroup[curGrp].unread = null;
	feedTreeInvalidate();
	feedSelected();
}

/**
 * Delete a group.
 */
function deleteGroup(confirm)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  if (feedtree.view.getLevel(index) != 0) return;
  var curGrp = gIdx.fdgp[index];
  if (curGrp == 0) return;

	const NF_SB = document.getElementById("newsfox-string-bundle");
  var confirmationMessage = NF_SB.getString('confirmation.deleteGroup');
  confirmationMessage += "\n\n" + gFdGroup[curGrp].title;
  if (!confirm || !gOptions.confirmDelete || yesNoConfirm(confirmationMessage))
  {
    if (gFdGroup[curGrp].expanded) feedtree.view.toggleOpenState(index);
    gFdGroup.splice(curGrp,1);
    for (var i=index+1;i < gIdx.fdgp.length;i++)
      gIdx.fdgp[i]--;
    gIdx.fdgp.splice(index,1);
    gIdx.feed.splice(index,1);
    gIdx.catg.splice(index,1);
    gIdx.open.splice(index,1);
    feedtree.treeBoxObject.rowCountChanged(index,-1);
    saveGroupModel();
    saveIndices();
		if (index >= gIdx.fdgp.length) index = gIdx.fdgp.length - 1;
		feedtree.view.selection.select(index);
  }
}

/**
 * Add a new feed.
 */
function addFeed(type)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  var pRow = feedtree.treeBoxObject.getFirstVisibleRow();
	var pGrp = gIdx.fdgp[index];
	var pFeed = gIdx.feed[index];
	var exp0 = gFdGroup[0].expanded;
  var url = defaultURL;
  createNewFeed(gFmodel, url, false, true, false);
  var nFeed = gFmodel.size() - 1;
	if (type == "storage") gFmodel.get(nFeed).storage = true;
  index = 0;
  while (nFeed != gIdx.feed[index]) index++;
	if (type != "startup")
	{
  	index = showFeedOptions(index,true);
		if (gOptions.selectAddedFeed)
		{
			if (!exp0 && gIdx.fdgp[index] != 0)
			{
				feedtree.view.toggleOpenState(0);
  			feedtree.treeBoxObject.scrollToRow(pRow);
  			feedtree.treeBoxObject.ensureRowIsVisible(feedtree.currentIndex);
			}
		}
		else
		{
			if (!exp0) feedtree.view.toggleOpenState(0);
			var i = gIdx.fdgp.length-1;
			while ((gIdx.fdgp[i] != pGrp || gIdx.feed[i] != pFeed) && i > 0) i--;
  		feedtree.treeBoxObject.scrollToRow(pRow);
  		feedtree.view.selection.select(i);
  		feedtree.treeBoxObject.ensureRowIsVisible(i);
		}
	}
	else
	{
		feedtree.view.selection.select(index);
		feedCheck(false);
	}
}

function addFeedUrl()
{
	var isStartup = false;
	if (gOptions.addUrl == NEWSFOX_RSS) isStartup = true;
	defaultURL = gOptions.addUrl;
	gOptions.addUrl = "";
	gOptions.save();
	if (isStartup || gFmodel.size() == 0)
		addFeed("startup");
	else
		addFeed("regular");
	defaultURL = "";
}

function deleteRow()
{
  var feedtree = document.getElementById("newsfox.feedTree");
	var index = feedtree.currentIndex;
	var level = feedtree.view.getLevel(index);
  if (level == 1) deleteFeed(index,true);
	else if (level == 0) deleteGroup(true);
}

function deleteSingleFeedRow()
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  if (feedtree.view.getLevel(index) != 1) return;
	var nGrp = gIdx.fdgp[index];
	if (nGrp == 0) return;
  var nFeed = gIdx.feed[index];
	if (!(index + 1 < gIdx.feed.length && gIdx.feed[index+1] >= 0)) // next is not feed
		index--;

	for (var j=gFdGroup[nGrp].list.length-1; j>=0; j--)
		if (gFdGroup[nGrp].list[j] == nFeed)
			gFdGroup[nGrp].list.splice(j,1);
	if (gFdGroup[nGrp].list.length == 0) gIdx.open[getGroupRow(nGrp)] = gFdGroup[nGrp].expanded = false;

	for (var i=gIdx.feed.length-1; i>=0; i--)
		if (gIdx.fdgp[i] == nGrp && gIdx.feed[i] == nFeed)
		{
			gIdx.fdgp.splice(i,1);
			gIdx.feed.splice(i,1);
			gIdx.catg.splice(i,1);
			gIdx.open.splice(i,1);
			feedtree.treeBoxObject.rowCountChanged(i,-1);
		}
	saveModels();
	feedtree.view.selection.select(index);
}

/**
 * Permanently delete a feed.
 */
function deleteFeed(index,confirm)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var fRow = feedtree.treeBoxObject.getFirstVisibleRow();
  if (index == -1) index = feedtree.currentIndex;
  if (feedtree.view.getLevel(index) != 1) return;
	var nGrp = gIdx.fdgp[index];
  var nFeed = gIdx.feed[index];
	var newFeed;
	if (index + 1 < gIdx.feed.length && gIdx.feed[index+1] >= 0)
		newFeed = gIdx.feed[index+1];
	else
		newFeed = gIdx.feed[index-1];
	if (newFeed > nFeed) newFeed--;
  var feed = gFmodel.get(nFeed);
	const NF_SB = document.getElementById("newsfox-string-bundle");
  var confirmationMessage = NF_SB.getString('confirmation.deleteFeed');
  confirmationMessage += "\n\n" + feed.getDisplayName();
  if (!confirm || !gOptions.confirmDelete || yesNoConfirm(confirmationMessage))
  {
    deleteFeedFromDisk(feed);
		if (gOptions.bookmarkSync) gBookmarkSync.deleteB(feed);
    for (var i=0; i<gFdGroup.length; i++)
      for (var j=0; j<gFdGroup[i].list.length; j++)
        if (gFdGroup[i].list[j] >= nFeed)
            (gFdGroup[i].list[j] == nFeed) ? gFdGroup[i].list[j]=-2 : gFdGroup[i].list[j]--;
    for (i=0; i<gIdx.feed.length; i++)
      if (gIdx.feed[i] >= nFeed)
        (gIdx.feed[i] == nFeed) ? gIdx.feed[i]=-2 : gIdx.feed[i]--;
    for (var i=0; i<gFdGroup.length; i++)
    {
      for (var j=gFdGroup[i].list.length-1; j>=0; j--)
        if (gFdGroup[i].list[j] == -2)
          gFdGroup[i].list.splice(j,1);
      if (gFdGroup[i].list.length == 0) gIdx.open[getGroupRow(i)] = gFdGroup[i].expanded = false;
    }
    for (i=gIdx.feed.length-1; i>=0; i--)
      if (gIdx.feed[i] == -2)
      {
        gIdx.fdgp.splice(i,1);
        gIdx.feed.splice(i,1);
        gIdx.catg.splice(i,1);
        gIdx.open.splice(i,1);
     		feedtree.treeBoxObject.rowCountChanged(i,-1);
      }
    gFmodel.remove(feed);
    saveModels();
    feedtree.treeBoxObject.scrollToRow(fRow);
		if (newFeed < 0)
			feedtree.view.selection.select(getGroupRow(nGrp));
		else
			feedtree.view.selection.select(getFeedRow(nGrp,newFeed));
		resetGroupUnread();
		setTitle(false);
  }
}

/**
 * Delete articles.
 */
function deleteArticle()
{
	var arttree = document.getElementById("newsfox.articleTree");
	if (arttree.view.selection.count == 0) return;
  try
  {
		const NF_SB = document.getElementById("newsfox-string-bundle");
    var confirmationMessage = NF_SB.getString('confirmation.deleteArticles');
    if (gOptions.confirmDeleteArticle && !yesNoConfirm(confirmationMessage))
      return;

    // Regular delete
		for (var i=0; i<gCollect.size(); i++)
			if (arttree.view.selection.isSelected(i))
			if (!(gOptions.dontDeleteFlagged && gCollect.isFlagged(i)))
			{
				var feed = gCollect.getFeed(i);
				var art = gCollect.get(i);
				if (art.id)
				{
					if (!feed.storage)
					{
						var newArt = new Article();
						newArt.id = art.id;
						feed.deletedAdd(newArt);
					}
					art.id = null;
					gCollect.setRead(i,true);
					gCollect.setFlagged(i,false);
					feed.changed = true;
				}
				else
				{
// should do a better job for undelete
					art.id = art.link;
				}
			}

		updateFeeds(false);
		var index = arttree.currentIndex;
		while (index >=0 && index < gCollect.size()-1 && 
				(arttree.view.selection.isSelected(index) || !gCollect.get(index).id))
			index++;
		if (index == gCollect.size()-1 && !gCollect.get(index).id)
			while (index >= 0 && !gCollect.get(index).id) index--;
		arttree.view.selection.select(index);
		if (index >= 0) arttree.treeBoxObject.ensureRowIsVisible(index);
		if (index == -1) buildBlank();

    var feedtree = document.getElementById("newsfox.feedTree");
		if (gOptions.slowDelete)
		{
			var artId = getArtId();
			var index = feedtree.currentIndex;
			feedtree.view.selection.select(-1);
			feedtree.view.selection.select(index);
			selectArt(artId);
		}

    // Update unread count
    feedTreeInvalidate();
		artTreeInvalidate();
		resetGroupUnread();
		setTitle(false);
  }
  catch (err)
  {
    var msg = "deleteArticle(): " + err;
    alert(msg);
  }
}

function updateFeeds(doAll)
{
	if (doAll || isGroup())
	{
		var curGrp = gFdGroup[gCollect.grpindex];
		if (doAll || gCollect.type == 4) curGrp = gFdGroup[0];
		for (i=0; i<curGrp.list.length; i++)
		{
			var feed = gFmodel.get(curGrp.list[i]);
			if (feed.changed) saveFeedUnload(feed);
		}
	}
	else                    // feed or category
	{
		var tree = document.getElementById("newsfox.feedTree");
		var index = tree.currentIndex;
		var feed = gFmodel.get(gIdx.feed[index]);
		saveFeedUnload(feed);
	}
	saveFeedModel();
}

function saveFeedUnload(feed)
{
	saveFeed(feed);
	feed.loaded = false;
	feed.changed = false;
}

function openArticle()
{
    var arttree = document.getElementById("newsfox.articleTree");
		for (var i=0; i<gCollect.size(); i++)
			if (arttree.view.selection.isSelected(i))
				openNewTab(gCollect.get(i).link);
}

function selectAllArticles()
{
	var arttree = document.getElementById("newsfox.articleTree");
	arttree.view.selection.selectAll();
}

function tagEdit()
{
	var arttree = document.getElementById("newsfox.articleTree");
	if (arttree.currentIndex == -1) return;

	var allTag = "";
	var allNum = 0;
	for (var i=0; i<gCollect.size(); i++)
		if (arttree.view.selection.isSelected(i))
		{
			allTag += "\/" + gCollect.get(i).tag;
			allNum++;
		}
	allTag += "\/";
	var allArray = gTag.split("\/");
	var allPct = new Array();
	for (var i=0; i<allArray.length; i++)
	{
		var tag = "\/" + allArray[i] + "\/";
		var tagLn = tag.length;
		var allLn = allTag.length;
		allTag = allTag.replace(new RegExp(tag,'g'),"\/\/");
		var newLn = allTag.length;
		allPct[i] = (allLn-newLn)/(allNum*(tagLn-2));
	}

  var params = { ok:false, tagstr:gTag, allPct:allPct, addstr:null, rmstr:null, delstr:null, newaddstr:null };
  var win = window.openDialog("chrome://newsfox/content/tagEdit.xul",
    "newsfox-dialog","chrome,centerscreen,modal", params);

  if (params.ok)
	{
		var addTags = mergeCats(params.addstr,params.newaddstr,null);
		var rmTags = params.rmstr;
		var delTags = params.delstr;
		tagSelected(addTags, rmTags, delTags);
	}
}

function tagSelected(addTags, rmTags, delTags)
{
	var toFlag = addTags && gOptions.flagTagged;
	var arttree = document.getElementById("newsfox.articleTree");
	for (var i=0; i<gCollect.size(); i++)
		if (arttree.view.selection.isSelected(i))
		{
			gCollect.get(i).tag = mergeCats(gCollect.get(i).tag,addTags,rmTags);
			if (toFlag) gCollect.setFlagged(i,true);
		}
	gTag = mergeCats(gTag,addTags,delTags);
	if (delTags)
	{
		var curGrp = gFdGroup[0];
		for (i=0; i<curGrp.list.length; i++)
		{
			var feed = gFmodel.get(curGrp.list[i]);
			for (j=0; j<feed.size(); j++)
				feed.get(j).tag = mergeCats(feed.get(j).tag,null,delTags)
		}
		updateFeeds(true);
	}
	else
		updateFeeds(false);
	for (var i=0; i<gFdGroup.length; i++)
		if (gFdGroup[i].searchTag) gFdGroup[i].unread = null;
	var feedtree = document.getElementById("newsfox.feedTree");
	if (delTags)
	{
		var index = feedtree.currentIndex;
		var didDelete = false;
		var delT = "\/" + delTags + "\/";
		for (var i=gIdx.fdgp.length-1; i>=0; i--)
		{
			var tag = gFdGroup[gIdx.fdgp[i]].searchTag;
			if (tag && delT.indexOf("\/"+tag+"\/") > -1)
			{
				didDelete = true;
				feedtree.currentIndex = i;
				deleteGroup(false);
				if (i < index) index--;
			}
		}
		if (didDelete) feedtree.view.selection.select(index);
	}
	feedTreeInvalidate();
	articleSelected();  // need new article
	if (gCollect.type == 4)
	{
		var index = feedtree.currentIndex;
		feedtree.view.selection.clearSelection();
		feedtree.view.selection.select(index);
	}
}

function markSelected(read)
{
	var arttree = document.getElementById("newsfox.articleTree");
  for (var i=0; i<gCollect.size(); i++)
	if (arttree.view.selection.isSelected(i))
		gCollect.setRead(i,read);
    
	feedTreeInvalidate();
	artTreeInvalidate();
	resetGroupUnread();
	setTitle(false);    
}

function markFlaggedUnread(doflag,read)
{
  for(var i=0; i < gCollect.size(); i++)
  {
		var article = gCollect.get(i);
    var artread = gCollect.isRead(i);
		if (!artread && read && gOptions.spam)
			gArtsToUpdateSpam.push(article);
    var flagged = gCollect.isFlagged(i);
    gCollect.setRead(i, doflag ? !flagged : (read ? true : false));
		if (read) gCollect.get(i).newUnread = false;
    if (doflag) gCollect.setFlagged(i,0);
	}

	feedTreeInvalidate();
	artTreeInvalidate();
	resetGroupUnread();
	setTitle(false);
	if (gArtsToUpdateSpam.length > 0) setTimeout(doUpdateSpam, 50);
}

function openUnread()
{
  var feedTree = document.getElementById("newsfox.feedTree");
  if( feedTree.currentIndex == -1) return;
  var article, read;
  for(var i=0; i < gCollect.size(); i++)
  {
    article = gCollect.get(i);
    read = gCollect.isRead(i);
    gCollect.setRead(i,true);
    if (!read)
		{
			openNewTab(article.link);
			if (gOptions.spam) spamFilterUpdate(article,read,true);
		}
  }
  feedTreeInvalidate();
	artTreeInvalidate();
}

function doUpdateSpam()
{
	if (gArtsToUpdateSpam.length == 0) return;
	var art = gArtsToUpdateSpam.shift();
	spamFilterUpdate(art,false,false);
	setTimeout(doUpdateSpam, 50);
}

function spamFilterUpdate(art, read, isGood)
{
	var titleArray = getTitleArray(art);
	var j= gWordArray.length - 1;
	for (var i=titleArray.length-1; i>=0; i--)
	{
		while (gWordArray[j] > titleArray[i] && j>=0) j--;
		if (gWordArray[j] == titleArray[i])
		{
			if (isGood)
			{
				gGoodArray[j] += S_TOTAL_START - S_GOOD_START;
				if (read) gTotalArray[j] += S_TOTAL_START - S_GOOD_START;
				if (gGoodArray[j] > gTotalArray[j]) gTotalArray[j] = gGoodArray[j]+1;
			}
			else
			{
				gGoodArray[j] -= (S_GOOD_START/2);
				if (gGoodArray[j] < 1) gGoodArray[j] = 1;
			}
		}
		j += 2;
		if (j >= gWordArray.length) j = gWordArray.length - 1;
	}
}

function help()
{
	showHref("chrome://newsfox/content/help/overview.xhtml");
}

function showAbout()
{
	showHref("chrome://newsfox/content/help/about.xhtml");
}

function showNF()
{
	openNewTab(NFINFO);
}

function showHref(url)
{
	var hrefPane = document.getElementById("hrefContent");
	hrefPane.setAttribute("src", url);
	var contentDeck = document.getElementById("contentDeck");
	contentDeck.selectedIndex = 0;
}

function showShortcuts()
{
  window.openDialog("chrome://newsfox/content/help/shortcuts.xul",
    "newsfox-dialog","centerscreen,resizable", null);
}

function home()
{
  var tree = document.getElementById("newsfox.feedTree");
  var feed = gFmodel.get( gIdx.feed[tree.currentIndex] );
  openNewTab(feed.homepage);
}

////////////////////////////////////////////////////////////////
// Options
////////////////////////////////////////////////////////////////

/**
 * Show global options.
 */
function showOptions()
{
	var keyType = new Array();
	var index = 0;
	var keySet = document.getElementById("shortcut-keys");
	for (var i=keySet.firstChild; i != null; i=i.nextSibling)
	{
		keyType[index] = 3;  // 3 is custom
		for (var j=0; j<3; j++)
		{
			var modMatch = (i.getAttribute("modifiers") == i.getAttribute("mod"+j));
			var keyMatch = (i.getAttribute("key") == i.getAttribute("key"+j));
			if (modMatch && keyMatch)
				keyType[index] = j;
		}
		if (keyType[index] != keyType[0])
			keyType[0] = 3;  // it is custom
		index++;
	}
	var keyIndex = keyType[0];
	if (keySet.firstChild == null) keyIndex = 0;
	var newKeyIndex, mvContents;
	var curDir = NFgetPref("global.directory", "str", "");
  var params = { ok:false, keyIndex:keyIndex, newKeyIndex:newKeyIndex, nfDir:curDir, mvContents:mvContents, gOpt:gOptions, keyList:scKeyList };
  var win = window.openDialog("chrome://newsfox/content/programOptions.xul",
    "newsfox-dialog","chrome,centerscreen,modal", params);

  if (params.ok)
  {
		gOptions = params.gOpt;
		scKeyList = params.keyList;
		newKeyIndex = params.newKeyIndex;
		if (keyIndex != newKeyIndex || newKeyIndex == 3)
			updateShortcuts(newKeyIndex);
    gOptions.save(false);
		setTimeout(setTitle, 500);  // time for overlay to reset statusbar

		if (curDir != params.nfDir)
		{
			var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
			var feedtree = document.getElementById("newsfox.feedTree");
			cleanup();
			feedtree.view = null;
			gMsgDone = true;
			if (params.mvContents < 2)  // copy or move
			{
				var oldDirFile = nsIPH.getFileFromURLSpec(curDir);
				var newDirFile = nsIPH.getFileFromURLSpec(params.nfDir);
				var newDirName = newDirFile.leafName;
				var newDirParent = newDirFile.parent;
// TODO need to put up a progress meter if long copy
// doesn't work: 
//var hithere = setTimeout(lttOn,500);
				try
				{
					newDirFile.remove(true);
					oldDirFile.copyTo(newDirParent,newDirName);
				}
				catch(err)
				{
					const NF_SB = document.getElementById("newsfox-string-bundle");
					var dirNotChanged = NF_SB.getString('dirNotChanged');
					alert(dirNotChanged + "\n\n" + err);
					return;
				}
//if (hithere != null) clearTimeout(hithere);
//loadingTooltip(false);
				NFsetPref("global.directory", "str", params.nfDir);
				gNewsfoxDirURL = params.nfDir;
				refreshModel();
				if (params.mvContents == 1) oldDirFile.remove(true);
			}
			else
			{
				NFsetPref("global.directory", "str", params.nfDir);
				location.href = "chrome://newsfox/content/newsfox.xul";
			}
		}
  }
	return;
}

//function lttOn()
//{
//	loadingTooltip(true);
//}
/**
 * Show options dialog for current feed/group.
 */
function chooseOptions()
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  if (index == -1) return;
  var level = feedtree.view.getLevel(index);
	// need following line because of FF bug not disabling option for 
	// categories in context menu
	if (level > 1) return;
	var isSearch = gFdGroup[gIdx.fdgp[index]].search;
  if (level == 1) showFeedOptions(index,false);
	else showGroupOptions(index,false,isSearch);
}

/**
 * Show options dialog for current group/search.
 */
function showGroupOptions(index,isNew,isSearch)
{
	// following two lines due to FF bug that sometimes doesn't
	// hide context menu properly
  var feedMenu = document.getElementById("feedMenu");
	feedMenu.hidePopup();
  var feedtree = document.getElementById("newsfox.feedTree");
  var grp = gIdx.fdgp[index];

  var expgrp = false;
  var exp0 = false;

  var feeds = new Array();
  for (var i=0; i<gFmodel.size(); i++)
    feeds.push(gFmodel.get(i));

  var titlelist = new Array();
  for (i=0; i<gFdGroup.length; i++)
    titlelist.push(gFdGroup[i].title);

  var lists = new Array();
  lists[0] = new Array();
  for (i=0; i<gFdGroup[0].list.length; i++)
    lists[0].push(gFdGroup[0].list[i]);
  lists[1] = new Array();
  for (i=0; i<gFdGroup[grp].list.length; i++)
    lists[1].push(gFdGroup[grp].list[i]);
	if (isNew && isSearch)
		for (i=0; i<gFdGroup[0].list.length; i++)
			lists[1].push(gFdGroup[0].list[i]);

	var showUnread = null;
	var flagged = null;
	var unread = null;
	var text = null;
	var textflags = null;
	var startTime = null;
	var endTime = null;
	if (isSearch)
	{
		showUnread = gFdGroup[grp].showUnread;
		var srchdat = gFdGroup[grp].srchdat;
		flagged = srchdat.flagged;
		unread = srchdat.unread;
		text = srchdat.text;
		textflags = srchdat.textflags;
		startTime = srchdat.startTime;
		endTime = srchdat.endTime;
	}

  var params = { ok:false, grp:grp, newGrp:grp, feeds:feeds, titlelist:titlelist, lists:lists, flagged:flagged, unread:unread, text:text, textflags:textflags, startTime:startTime, endTime:endTime, isSearch:isSearch, fdGp:gFdGroup, isNew:isNew, showUnread:showUnread };
  var win = window.openDialog("chrome://newsfox/content/groupOptions.xul",
    "newsfox-dialog","resizable=yes,chrome,centerscreen,modal", params);

  if (params.ok)
  {
	  if (grp != 0 && gFdGroup[grp].expanded)
	  {
	    feedtree.view.toggleOpenState(index);
	    expgrp = true;
	  }
	  if (gFdGroup[0].expanded)
	  {
	    feedtree.view.toggleOpenState(0);
	    exp0 = true;
	  }
		if (isSearch)
		{
			gFdGroup[grp].showUnread = params.showUnread;
			gFdGroup[grp].srchdat.flagged = params.flagged;
			gFdGroup[grp].srchdat.unread = params.unread;
			gFdGroup[grp].srchdat.text = params.text;
			gFdGroup[grp].srchdat.textflags = params.textflags;
			gFdGroup[grp].srchdat.startTime = params.startTime;
			gFdGroup[grp].srchdat.endTime = params.endTime;
		}
    gFdGroup[grp].title = params.titlelist[grp];
    var feedslist = params.lists[0];
    for (i=0; i<feedslist.length; i++)
      gFdGroup[0].list[i] = feedslist[i];
    gFdGroup[0].list.length = feedslist.length;
    var grplist = params.lists[1];
    for (i=0; i<grplist.length; i++)
      gFdGroup[grp].list[i] = grplist[i];
    gFdGroup[grp].list.length = grplist.length;
    mvGrp(grp,params.newGrp);
		grp = (params.newGrp >= grp) ? params.newGrp-1 : params.newGrp;
    if (gFdGroup[grp].list.length == 0) expgrp = false;
    saveIndices();
    saveGroupModel();
    loadGroupModel();
  }
  else    // !params.ok
  {
    if (isNew)
    {
			index = getGroupRow(grp);
      refreshModelSelect(index);
      deleteGroup(false);
    }
  }
  if (exp0) feedtree.view.toggleOpenState(0);
  index = getGroupRow(grp);
  if (expgrp) feedtree.view.toggleOpenState(index);
  refreshModelSelect(index);
}

/**
 * Show options dialog for current feed.
 */
function showFeedOptions(index,isNew)
{
  var checkFeed = false;
  var curGrp = gIdx.fdgp[index];
  var nFeed = gIdx.feed[index];
  var feed = gFmodel.get(nFeed);
  var namearray = new Array();
  var membarray = new Array();
  var membarray2 = new Array();
  var tmp, j;
  for (var i=0; i<gFdGroup.length; i++)
  {
    namearray.push(gFdGroup[i].title.replace(/,/g,"&#x2C;"));
    tmp = 0;
    for (j=0; j<gFdGroup[i].list.length; j++)
      if (gFdGroup[i].list[j] == nFeed) tmp = 1;
		membarray2.push(tmp);
		if (gFdGroup[i].search)
			if (isNew)
				tmp = 3;
			else
				tmp += 2;
		if (gFdGroup[i].search && gFdGroup[i].searchTag) tmp += 4;
    membarray.push(tmp);
  }
  var groupstr = namearray.join();
  var groupmemb = membarray.join();

	var un = "";
	if (feed.username) un = gSdr.decryptString(feed.username);
	var pw = "";
	if (feed.password) pw = gSdr.decryptString(feed.password);

  var params = { ok:false, name:feed.getDisplayName(), iconsrc:feed.icon.src, homepage:feed.homepage, url:feed.url, style:feed.style, deleteOldStyle:feed.deleteOldStyle, autoCheck:feed.autoCheck, groupstr:groupstr, groupmemb:groupmemb, isNew:isNew, checkFeed:checkFeed, uid:feed.uid, model:gFmodel, daysToKeep:feed.daysToKeep, prvate:feed.prvate, username:un, password:pw, storage:feed.storage, lastUpdate:feed.lastUpdate, autoRefreshInterval:feed.autoRefreshInterval, Xfilter:feed.Xfilter, XfilterType: feed.XfilterType, XfilterNew: feed.XfilterNew, XfilterMimeType:feed.XfilterMimeType, XfilterImages: feed.XfilterImages, sortStr: feed.sortStr, changedUnread:feed.changedUnread };
  var win = window.openDialog("chrome://newsfox/content/feedOptions.xul",
    "newsfox-dialog","chrome,centerscreen,modal", params);

  if (params.ok)
  {
    var newmembarray = params.groupmemb.split(",");
    for (i=0; i<membarray2.length; i++)
    {
      var diff = newmembarray[i] - membarray2[i];
      if (diff == 1)  // adding
      {
        gFdGroup[i].list.push(nFeed);
        if (gFdGroup[i].expanded)
        {
          var top = getGroupRow(i+1);
          gIdx.fdgp.splice(top,0,i);
          gIdx.feed.splice(top,0,nFeed);
          gIdx.catg.splice(top,0,0);
          gIdx.open.splice(top,0,false);
        }
      }
      else if (diff == -1)  // deleting
      {
        for (var j=gFdGroup[i].list.length-1; j>=0; j--)
          if (gFdGroup[i].list[j] == nFeed)
            gFdGroup[i].list.splice(j,1);
        if (gFdGroup[i].list.length == 0) gIdx.open[getGroupRow(i)] = gFdGroup[i].expanded = false;
        var grprow = getGroupRow(i);
        var top = getGroupRow(i+1);
        for (j=top-1; j>=grprow; j--)
          if (gIdx.feed[j] == nFeed)
          {
            gIdx.fdgp.splice(j,1);
            gIdx.feed.splice(j,1);
            gIdx.catg.splice(j,1);
            gIdx.open.splice(j,1);
          }
      }
			if (diff != 0) resetGUnread(i);
    }

    feed.style = params.style;
		if (feed.storage)
		{
			feed.icon.src = ICON_STORAGE;
			var val = stringTrim(params.name);
			if (isNew)
			{
				var uid = gFmodel.makeUniqueUid(val);
				feed.uid = uid;
				deleteFeedFromDisk(feed);
			}
			feed.defaultName = val;
		}
		else
		{
	    feed.url = stringTrim(params.url);
	    feed.deleteOldStyle = params.deleteOldStyle;
			feed.daysToKeep = params.daysToKeep;
			feed.changedUnread = params.changedUnread;
	    feed.autoCheck = params.autoCheck;
			feed.autoRefreshInterval = params.autoRefreshInterval;
			feed.XfilterNew = params.XfilterNew;
			feed.XfilterMimeType = params.XfilterMimeType;
			feed.XfilterImages = params.XfilterImages;
			feed.Xfilter = params.Xfilter;
			feed.XfilterType = params.XfilterType;
			feed.sortStr = params.sortStr;
	    if (isNew)
	    {
	      var uid = gFmodel.makeUniqueUid(feed.url);
	      feed.uid = uid;
				deleteFeedFromDisk(feed);
	      feed.defaultName = uid;
				feed.homepage = null;   // TODO feed.homepage gets set via downloadicon?
	    }
	    else
	    {
	      var val = stringTrim(params.name);
	      if (val == feed.defaultName) feed.customName = "";
	      else feed.customName = val;
	      if (params.iconsrc == "")
	        feed.icon.src = ICON_OK;
	      else
	        feed.icon.src = stringTrim(params.iconsrc);
	      feed.homepage = stringTrim(params.homepage);
	      downloadIcon(feed);
	    }
		}
		if (feed.prvate != params.prvate || un != params.username || pw != params.password)
		{
    	feed.prvate = params.prvate;
			feed.username = gSdr.encryptString(params.username);
			feed.password = gSdr.encryptString(params.password);
			if (params.username == "" && params.password == "")
			{
				feed.username = null;
				feed.password = null;
			}
			saveFeed(feed);
		}
    saveModels();
		if (isNew) curGrp++;   // prefer not FEEDS group
   	index = getGroupRow(curGrp);    // index may have changed, recompute
    while (gIdx.feed[index] != nFeed && index < gIdx.fdgp.length) index++;
    if (gIdx.feed[index] != nFeed)
    {
      index=0;
      while (gIdx.feed[index] != nFeed  && index < gIdx.fdgp.length) index++;
      if (gIdx.feed[index] != nFeed)    // can't happen, feed still exists
      { 
        refreshModelSelect(0);
        return 0;
      }
    }
    refreshModelSelect(index); 
    if (params.checkFeed) feedCheck(false);
  }
  else   //  !params.ok
  {
    if (isNew)
    {
//      refreshModelSelect(index);
      deleteFeed(index,false);
    }
  }
	return index;
}

function buildBlank()
{
	resetIframe("buildContent");
	var contentDeck = document.getElementById("contentDeck");
	contentDeck.selectedIndex = 1;
	var hrefPane = document.getElementById("hrefContent");
	hrefPane.setAttribute("src", "about:blank");
}

/**
 * Troubleshoot feed errors.
 */
function troubleshoot()
{
  var tree = document.getElementById("newsfox.feedTree");
  var nFeed = gIdx.feed[tree.currentIndex];
  if (nFeed < 0) return;
  var feed = gFmodel.get(nFeed);

	resetIframe("buildContent");
  var iframe = document.getElementById("buildContent");
  var doc = iframe.contentDocument;

  var summary = getErrorSummary(feed.error);
  var remedies = getErrorRemedies(feed.error);

		var b = doc.createElement("b");
		b.appendChild(doc.createTextNode(summary));
  doc.body.appendChild(b);
  doc.body.appendChild(doc.createElement("hr"));

  	var p = doc.createElement("p");
  	p.innerHTML = remedies.replace(/\n/g,"<br/>");
  doc.body.appendChild(p);

		if (feed.error.substring(0,1) == ERROR_INVALID_FEED_URL)
		{
			var p1 = doc.createElement("p");
			var a1 = doc.createElement("a");
			a1.setAttribute("href",FEED_VALIDATOR + escape(feed.url));
			a1.setAttribute("target","_blank");
			const NF_SB = document.getElementById("newsfox-string-bundle");
			var feedValidator = NF_SB.getString('remedy_checkFeedValidator');
			a1.innerHTML = feedValidator;
			p1.appendChild(a1);
	doc.body.appendChild(p1);
		}

	  var p2 = doc.createElement("p");
		var a2 = doc.createElement("a");
		a2.setAttribute("href",feed.url);
		a2.setAttribute("target","_blank");
		a2.innerHTML = feed.url;
	  p2.appendChild(a2);
  doc.body.appendChild(p2);

	var contentDeck = document.getElementById("contentDeck");
	contentDeck.selectedIndex = 1;
	var hrefPane = document.getElementById("hrefContent");
	hrefPane.setAttribute("src", "about:blank");
}

////////////////////////////////////////////////////////////////
// Selection
////////////////////////////////////////////////////////////////

/**
 * Feed selected.
 */
function feedSelected()
{
	if (feedSelectTimeout != null) clearTimeout(feedSelectTimeout);
	// when dragging articles don't want new collection
	// toggleopenstate fires the select event (rowCountChanged does)
	if (dragToggling) return;
	if ((gCollect.type == 1 || gCollect.type == 2) && NFgetPref("u.markReadOnExit", "bool", false))
		markFlaggedUnread(false,true);
  var tree = document.getElementById("newsfox.feedTree");
  var index = tree.currentIndex;
  var arttree = document.getElementById("newsfox.articleTree");
  if (index == -1)
	{
		gCollect = new EmptyCollection();
		arttree.view = null;
		return;
	}
  var nFeed = gIdx.feed[index];
  if (nFeed < 0)  // group
	{
		var grpindex = gIdx.fdgp[index];
		var grp = gFdGroup[grpindex];
		if (grp.search || gOptions.aggregate)
		{
			loadingTooltip(true);
			document.getElementById("mfeedTitle").value = "?";
			for (var i=0; i<grp.list.length; i++)
			{
				loadFeed(gFmodel.get(grp.list[i]),true,false);
				setPmeter((100*i)/grp.list.length);
			}
			gCollect = new GroupCollection(grpindex, grp.search);
			setPmeter(0);
			loadingTooltip(false);
		}
		else
		{
			gCollect = new EmptyCollection();
			arttree.view = null;
			return;
		}
	}
	else if (nFeed >= 0)  // feed
	{
  	var feed = gFmodel.get(nFeed);
		downloadIcon(feed);
		loadFeed(feed,true,false);
		feed.flags.length = feed.size();
		gCollect = new NormalCollection(nFeed, gIdx.catg[index], true);
	}

	if (gCollect.type != -1) doDefaultSort(gCollect,true);
//  arttree.view = null;  
  arttree.view = new ArticleTreeModel();
	artTreeInvalidate();
	gExtraInvalidateTimer = setTimeout(extraInvalidate, 500);
	if (nFeed >= 0 && feed.error != ERROR_OK && feed.error != ERROR_REFRESH)
		feedSelectTimeout = setTimeout(troubleshoot, 250);
}

var gExtraInvalidateTimer;
function extraInvalidate()
{
	clearTimeout(gExtraInvalidateTimer);
	artTreeInvalidate();
}

function doDefaultSort(collect, doArrow)
{
	var sorts = collect.sortStr;
	for (var i=0; i<sorts.length/2; i++)
	{
		var colId = COL_NAME[sorts[2*i]];
		if (colId == "orderThread") makeThreadIndex(collect);
		var dir = "descending";
		if (sorts[2*i+1] == "+") dir = "ascending";
		if (colId != "none")
			collect.artSort(colId, dir);
	}
	if (doArrow  && sorts.length && sorts[0] != "n" && sorts[0] != "o")
	{
		i = sorts.length - 1;
		var colId = COL_NAME[sorts[i-1]];
		var dir = "descending";
		if (sorts[i] == "+") dir = "ascending";
		document.getElementById(colId).setAttribute("sortDirection",dir);
	}
}

function makeThreadIndex(collect)
{
	collect.artSort("date", "descending");
	var titles = "";
	var next = 1;
	const MULT = 100000;
	var nextArray = new Array();
	for (var i=0; i<collect.size(); i++)
	{
		var rTitle = noReTitle(collect.get(i).title);
		rTitle = rTitle.replace(/\//g,"");
		var index = titles.indexOf("/"+rTitle+"/")-1;
		if (index > -1)
		{
			var which = titles.substring(titles.lastIndexOf("/",index-1)+1,index+1);
			collect.get(i).thr = --nextArray[which];
		}
		else
		{
			nextArray[next] = MULT*next;
			collect.get(i).thr = nextArray[next];
			titles += next++ + "/" + rTitle + "/";
		}
	}
}

var pastArticles = new Array();
function onHistoryMenuShowing(menu)
{
	while (menu.firstChild != null) menu.removeChild(menu.firstChild);
	for (var i=pastArticles.length-1; i>-1; i--)
	{
		var mI = document.createElement("menuitem");
//		mI.addEventListener("command", to do it this way need to find the index from the Event? );
		mI.setAttribute("oncommand", "pastArticleSelected(" + i + ");");
		mI.setAttribute("label", pastArticles[i].title);
		menu.appendChild(mI);
	}
  return true;
}

function pastArticleSelected(index)
{
	var art = pastArticles[index];
  var style = art.pastStyle;
  var XfilterType = art.pastXfilterType;
  var feed = art.pastFeed;
	doDisplay(art, style, XfilterType, feed);
}
/**
 * Article selected.
 */
function articleSelected()
{
	if (!gDisplayArticle) return;
  var tree = document.getElementById("newsfox.articleTree");
	// don't display article if selecting multiple articles
	if (tree.view.selection.count > 1) return;
  var index = tree.currentIndex;
	if (index == -1) return;
  var art = gCollect.get(index);
  var read = gCollect.isRead(index);
	var feed = gCollect.getFeed(index);

	if (gOptions.spam) spamFilterUpdate(art,read,true);

	if (!read && gOptions.markRead) markArtRead(false);
  var style = gCollect.getFeed(index).getStyle();
	var XfilterType = feed.XfilterType;
  art.pastStyle = style;
  art.pastXfilterType = XfilterType;
  art.pastFeed = feed;
  pastArticles.push(art);
	doDisplay(art, style, XfilterType, feed);
}

function doDisplay(art, style, XfilterType, feed)
{
  // Stop any previous page loading
	var nsIWebNav = Components.interfaces.nsIWebNavigation;
	var iframe = document.getElementById("hrefContent");
	var webnav = iframe.docShell.QueryInterface(nsIWebNav);
	webnav.stop(nsIWebNav.STOP_NETWORK);
	var iframe = document.getElementById("buildContent");
	var webnav = iframe.docShell.QueryInterface(nsIWebNav);
	webnav.stop(nsIWebNav.STOP_NETWORK);

  // Display body in content
	var contentDeck = document.getElementById("contentDeck");

	if (gOptions.copyClip != 0)
	{
		var trans = Components.classes["@mozilla.org/widget/transferable;1"]
			.createInstance(Components.interfaces.nsITransferable);
		try { trans.init(null); }
		catch(e) {}
  	trans.addDataFlavor("text/html");
	  trans.addDataFlavor("text/unicode");

		var htmlStr = Components.classes["@mozilla.org/supports-string;1"]
			.createInstance(Components.interfaces.nsISupportsString);
		switch (gOptions.copyClip)
		{
			default:
			case 1:
				htmlStr.data = art.body;
				break;
			case 2:
				htmlStr.data = art.link;
				break;
			case 3:
				htmlStr.data = art.title;
				break;
			case 4:
				htmlStr.data = art.date;
				break;
			case 5:
				htmlStr.data = art.category;
				break;
			case 6:
				htmlStr.data = art.tag;
				break;
			case 7:
				htmlStr.data = art.id;
				break;
			case 8:
				htmlStr.data = art.author;
				break;
			case 9:
				htmlStr.data = art.prob;
				break;
			case 10:
				htmlStr.data = art.Xbody;
				break;
		}

		trans.setTransferData("text/html",htmlStr, htmlStr.toString().length);
    trans.setTransferData("text/unicode",htmlStr, htmlStr.toString().length*2);

		var clipId = Components.interfaces.nsIClipboard;
		var clipB = Components.classes["@mozilla.org/widget/clipboard;1"]
			.getService(clipId);
  	clipB.setData(trans,null,clipId.kGlobalClipboard);
	}

  if (((style == 2 || style == 3) 
				&& art.link && (art.Xtend == false || art.Xbody == "")) 
				|| (art.Xtend && (XfilterType == 3))
				|| (art.Xtend && gOptions.defaultXfilterIsWeb && XfilterType == -1))
  {
    // Display as webpage
		var iframe = document.getElementById("hrefContent");
		iframe.docShell.allowJavascript = NFgetPref("javascript.enabled","bool",true,true);
		if (style == 3)
			document.getElementById("hrefContent").docShell.allowJavascript = false;
		var hrefPane = document.getElementById("hrefContent");
		hrefPane.setAttribute("src", art.link);
		contentDeck.selectedIndex = 0;
  }
  else
  {
		// display using innerHTML to resolve security issues pointed out by Wladimir Palant
		resetIframe("buildContent");
		getTextView(art, feed);
		var buildPane = document.getElementById("buildContent");
		buildPane.contentWindow.scrollTo(0,0);
		contentDeck.selectedIndex = 1;
  }
}

function markArtRead(blurring)
{
	// don't mark read if another tab opened
	if (blurring && document.commandDispatcher.focusedElement)
	{
		var focusId = document.commandDispatcher.focusedElement.id;
		if (focusId == "newsfox.articleTree") return;
	}

  var tree = document.getElementById("newsfox.articleTree");
  var index = tree.currentIndex;
	if (index == -1) return;
  if (!gCollect.isRead(index))
  {
    gCollect.setRead(index, true);
		gCollect.get(index).newUnread = false;
    tree.treeBoxObject.invalidateRow(index);
		feedTreeInvalidate();
		resetGroupUnread();
		setTitle(false);
  }
}

function articleTreeMClicked()
{
	var tree = document.getElementById("newsfox.articleTree");
	var index = tree.currentIndex;
	if (index == -1) return;  // in header
	var article = gCollect.get(index);
	openNewTab(article.link);
}

function articleTreeDblClicked()
{
	this.articleTreeMClicked();
}

/**
 * Select the next unread article.
 */
function selectNextUnreadArticle()
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var arttree = document.getElementById("newsfox.articleTree");
  var row = feedtree.currentIndex;
  if (row < 0) return;
  var curGrp = gIdx.fdgp[row];
	var artIndex = arttree.currentIndex;
	if (artIndex < 0) artIndex = 0;
	if (goToNext(artIndex)) return;
	while (++row < gIdx.fdgp.length)
	{
    feedtree.view.selection.select(row);
	  feedtree.treeBoxObject.ensureRowIsVisible(row);
// skip opened groups and categories
    if (!((isGroup() && gFdGroup[gIdx.fdgp[row]].expanded) || gCollect.type == 2))
      if (goToNext(0)) return;
	}
	if (NFgetPref("advanced.warnNextPrev","bool",true))
	{
		const NF_SB = document.getElementById("newsfox-string-bundle");
  	alert(NF_SB.getString('noNextUnread'));
	}
	else
		flashTitle();
}

function flashTitle()
{
	setTimeout(doneTitle,1);
	setTimeout(setTitle,501);
}

function goToNext(index)
{
  var arttree = document.getElementById("newsfox.articleTree");
	for (var j = index; j < gCollect.size(); j++)
		if (!gCollect.isRead(j))
		{
			arttree.view.selection.select(j);
      arttree.treeBoxObject.ensureRowIsVisible(j);
      return true;
		}
	return false;
}

/**
 * Select the previous unread article.
 */
function selectPrevUnreadArticle()
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var arttree = document.getElementById("newsfox.articleTree");
  var row = feedtree.currentIndex;
  if (row < 0) return;
  var curGrp = gIdx.fdgp[row];
  var artIndex = arttree.currentIndex;
  if (artIndex < 0)
  {
    artIndex = gCollect.size()-1;
    if (artIndex < 0) artIndex = 0;
  }
	if (goToPrev(artIndex)) return;
	while (--row >= 0)
	{
	  feedtree.view.selection.select(row);
	  feedtree.treeBoxObject.ensureRowIsVisible(row);
// skip opened groups and categories
    if (!((isGroup() && gFdGroup[gIdx.fdgp[row]].expanded) || gCollect.type ==2))
      if (goToPrev(gCollect.size()-1)) return;
	}
	if (NFgetPref("advanced.warnNextPrev","bool",true))
	{
		const NF_SB = document.getElementById("newsfox-string-bundle");
		alert(NF_SB.getString('noPrevUnread'));
	}
	else
		flashTitle();
}

function goToPrev(index)
{
  var arttree = document.getElementById("newsfox.articleTree");
	for (var j = index; j >= 0; j--)
		if (!gCollect.isRead(j))
		{
			arttree.view.selection.select(j);
      arttree.treeBoxObject.ensureRowIsVisible(j);
      return true;
		}
	return false;
}

////////////////////////////////////////////////////////////////
// Event Handlers
////////////////////////////////////////////////////////////////

/**
 * Route events.
 */
function handleEvent(e) 
{
  if (e.keyCode == 0) handleMouseEvent(e);
  handleKeyEvent(e);
}

/**
 * Handle mouse events.
 */
function handleMouseEvent(e) 
{
}

/**
 * Handle keyboard events.
 */
function handleKeyEvent(e)
{
	var focus = 0;
	try { var focusId = document.commandDispatcher.focusedElement.id; }
	catch(e) { var focusId = null; }
	if (focusId == "newsfox.feedTree") focus = 1;
	else if (focusId == "newsfox.articleTree") focus = 2;
	else focus = 3;
  switch(e.keyCode) 
  {
		case 37: // left-arrow
			if (focus >= 2 && !e.ctrlKey && !e.altKey && !e.shiftKey)
				mvFocus("newsfox.articleTree","up","select");
			else if (e.ctrlKey && !e.altKey)
			{
				if (e.shiftKey)
				{
        	e.stopPropagation();
        	e.preventDefault();
					var arttree = document.getElementById("newsfox.articleTree");
					var row = arttree.currentIndex;
					var flag = gCollect.isFlagged(row);
					gCollect.setFlagged(row, !flag);
					artTreeInvalidate();
				}
				else
					markFlaggedUnread(false,true);
			}
			else if (e.ctrlKey && e.altKey && !e.shiftKey)
				mvFocus("newsfox.feedTree","up","select");
			break;

    case 39:  // right-arrow
			if (focus >= 2 && !e.ctrlKey && !e.altKey && !e.shiftKey)
				mvFocus("newsfox.articleTree","down","select");
			else if (focus >= 2 && e.ctrlKey && !e.altKey)
			{
				if (e.shiftKey)
				{
        	e.stopPropagation();
        	e.preventDefault();
					openArticle();
				}
				else
				{
  				var arttree = document.getElementById("newsfox.articleTree");
					arttree.view.selection.select(arttree.currentIndex);
				}
			}
			else if (e.ctrlKey && e.altKey && !e.shiftKey)
				mvFocus("newsfox.feedTree","down","select");
			else if (e.ctrlKey && e.altKey && e.shiftKey)
				toggleFilter();
			break;

    case 38:   // up-arrow
      if (!e.ctrlKey && e.altKey && !e.shiftKey) 
      {
        e.stopPropagation();
        e.preventDefault();
        selectPrevUnreadArticle(); 
      }
      else if (e.ctrlKey && !e.altKey && !e.shiftKey)
			{
				if (focus == 1) moveIt(true);
			// focus == 2 has following built-in
				else if (focus == 3)
					mvFocus("newsfox.articleTree","up","focus");
			}
			else if (e.ctrlKey && e.altKey && !e.shiftKey)
				printArticle();
			else if (e.shiftKey && !e.altKey && !e.ctrlKey && focus == 2)
			{
  			var arttree = document.getElementById("newsfox.articleTree");
				arttree.treeBoxObject.scrollByLines(-1);
			}
			break;

    case 40:  // down-arrow
      if (!e.ctrlKey && e.altKey && !e.shiftKey) 
      {
        e.stopPropagation();
        e.preventDefault();
        selectNextUnreadArticle(); 
      }
      else if (e.ctrlKey && !e.altKey && !e.shiftKey)
			{
				if (focus == 1) moveIt(false);
			// focus == 2 has following built-in
				else if (focus == 3)
					mvFocus("newsfox.articleTree","down","focus");
			}
			else if (e.ctrlKey && e.altKey && !e.shiftKey)
			{
  			var feedTree = document.getElementById("newsfox.feedTree");
				feedTree.view.toggleOpenState(feedTree.currentIndex);
				feedTreeInvalidate();
			}
			else if (e.shiftKey && !e.altKey && !e.ctrlKey && focus == 2)
			{
  			var arttree = document.getElementById("newsfox.articleTree");
				arttree.treeBoxObject.scrollByLines(1);
			}
      break;

    case 46: // delete
			if (!e.ctrlKey && !e.altKey)
			{
				if (e.shiftKey) selectAllArticles();
      	else if (focus >= 2) deleteArticle();
      	else if (focus == 1) deleteRow();
			}
      break;

		case 13: // enter key
			if (focus == 2 && !e.altKey && !e.ctrlKey && !e.shiftKey)
				openArticle();
			break;

		case 0:  // character pressed
			switch (e.charCode)
			{
				case 65:  // A, because a doesn't get passed to tree
					if (focus >= 2 && e.ctrlKey && !e.altKey) selectAllArticles();
					break;
				case 97:  // a
 					if (e.ctrlKey && !e.altKey) selectAllArticles();
					break;
				case 83: // S
					if (e.ctrlKey && !e.altKey) doSearch();
					if (!e.ctrlKey && e.altKey) doSearch();
					break;
				case 115:  // s
					if (focus >= 2 && e.ctrlKey && e.altKey) selectAllArticles();
					break;
			}
			break;
  }
}

////////////////////////////////////////////////////////////////
// Util
////////////////////////////////////////////////////////////////

function onOptMenuShowing(menu)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  var level = feedtree.view.getLevel(index);
  if (index == -1) level = -1;

  var children = menu.childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var id = children[i].getAttribute("id");
    switch (id)
    {
      case "tool.group":
				if (level == 0 && gCollect.type != 4)
					children[i].removeAttribute("hidden")
				else
					children[i].setAttribute("hidden",true);
				break;
      case "tool.feed":
				if (level == 1) children[i].removeAttribute("hidden")
				else children[i].setAttribute("hidden",true);
				break;
      case "tool.passwd":
				if (hasSecure()) children[i].removeAttribute("hidden")
				else children[i].setAttribute("hidden",true);
				break;
    }
  }
  return true;
}

function hasSecure()
{
	for (var i=0; i<gFmodel.size(); i++)
		if (gFmodel.get(i).url.substring(0,5) == "https") return true;
	return false;
}

function onFeedMenuShowing(menu)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
	if (index == -1) return false;
  var level = feedtree.view.getLevel(index);

	var nGrp = gIdx.fdgp[index];
	var curgGroup = nGrp + "," + gFdGroup[nGrp].list.length + "," + gFdGroup[nGrp].title;
	var canUnsort = (gGroup == curgGroup);

  var children = menu.childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var id = children[i].getAttribute("id");
    switch (id)
    {
      case "home":
				if (level == 0 || gFmodel.get(gIdx.feed[index]).homepage == "" || 										gCollect.type == 5)
					children[i].setAttribute("disabled",true)
				else
					children[i].setAttribute("disabled",false);
				if (gCollect.type == 0) children[i].setAttribute("hidden",true);
				else children[i].removeAttribute("hidden");
				break;
      case "home2":
				if (gCollect.type != 0) children[i].setAttribute("hidden",true);
				else children[i].removeAttribute("hidden");
				break;
      case "fMprops":
				if (gCollect.type == 4) children[i].setAttribute("hidden",true);
				else
				{
					children[i].removeAttribute("hidden");
					if (level <= 1) children[i].setAttribute("disabled",false);
					else children[i].setAttribute("disabled",true);
					if (level == 0)
						document.getElementById("fMprops").setAttribute("label", 
								document.getElementById("fBoptions").getAttribute("gtext"));
					else
						document.getElementById("fMprops").setAttribute("label", 
								document.getElementById("fBoptions").getAttribute("ftext"));
				}
				break;
      case "checkFeed":	
				if (gCollect.type == 5 || gCollect.type == 4) 
					children[i].setAttribute("hidden",true);
				else children[i].removeAttribute("hidden");
				if (level <= 1 && gCollect.type != 5)
					children[i].setAttribute("disabled",false)
				else
					children[i].setAttribute("disabled",true);
				if (gCheckInProgress)
					children[i].disabled = true;
				break;
      case "checkFeed2":
				if (gCollect.type == 5 || gCollect.type == 4) 
					children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
				break;
			case "fMprops2":
				if (gCollect.type != 4) children[i].setAttribute("hidden",true);
				else
				{
					children[i].removeAttribute("hidden");
					makeTagMenu("fMprops3", false);
				}
				break;
      case "openSort":	
				if (gCollect.type == 0) children[i].setAttribute("hidden",true);
				else children[i].removeAttribute("hidden");
				break;
      case "openSort2":
				if (gCollect.type == 0  && !canUnsort) children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
				break;
      case "openSort3":
				if (gCollect.type == 0 && canUnsort) children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
				break;
    }
  }
  return true;
}

function onArtMenuShowing(menu)
{
	var arttree = document.getElementById("newsfox.articleTree");
  var cnt = arttree.view.selection.count;
  var cnt2 = 0;
  for (var i=0; i<gCollect.size(); i++)
		if (arttree.view.selection.isSelected(i) && gCollect.isRead(i)) cnt2++;
  
  var children = menu.childNodes;
  for (var i = 0; i < children.length; i++)
  {
    var id = children[i].getAttribute("id");
    switch (id)
    {
      case "marksel1":  // mark all read
      	if (cnt <= 1) children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
        break;
      case "marksel2":  // mark selected read
        if (cnt > 0 && cnt2 != cnt) children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
        break;
      case "marksel3":  // mark selected unread
        if (cnt > 0 && cnt2 != 0) children[i].removeAttribute("hidden");
				else children[i].setAttribute("hidden",true);
        break;
    }
  }
  return (gCollect.type != -1);
}

function onAddGroupMenuShowing(menu)
{
	makeTagMenu("aGtag", true);
	return true;
}

function makeTagMenu(id,toAdd)
{
	var tagSorter = function(a,b)
	{
		return (tags[a] > tags[b]) ? 1 : -1;
	}

	var menu = document.getElementById(id);
	var tags;
	if (gTag == "")
		tags = new Array();
	else
		tags = gTag.split("\/");
	var orderArray = new Array();
	for (var i=0; i<tags.length; i++)
		orderArray.push(i);
	orderArray.sort(tagSorter);
	tags.sort();
	orderArray.push(tags.length);
	const NF_SB = document.getElementById("newsfox-string-bundle");
  tags.push(NF_SB.getString('newtag'));
	while (menu.firstChild != null) menu.removeChild(menu.firstChild);

	for (var i=0; i<tags.length; i++)
	{
		var menuitem = document.createElement("menuitem");
		menuitem.setAttribute("label",tags[i]);
		if (toAdd)
//			menuitem.addEventListener("command", to implement this way need to find i from Event?
			menuitem.setAttribute("oncommand",
					"addTagGroup("+orderArray[i]+"); event.stopPropagation();");
		else
//			menuitem.addEventListener("command", to implement this way need to find i from Event?
			menuitem.setAttribute("oncommand","updateTagGroup("+orderArray[i]+")");
		menu.appendChild(menuitem);
	}
}

function mvGrp(oldgrp, newgrp)
{
  if (oldgrp == newgrp || oldgrp+1 == newgrp || oldgrp == 0 || newgrp == 0) return;
  var up = 1*(newgrp > oldgrp);
  var down = 1 - up;
  var newrow = getGroupRow(newgrp);
  var oldrow = getGroupRow(oldgrp);
  var i = oldrow;
  while (i < gIdx.fdgp.length && gIdx.fdgp[i] == oldgrp) i++;
  var num = i - oldrow;
  for (i=0; i<num; i++)
  {
    gIdx.fdgp.splice(newrow+i*down,0,gIdx.fdgp[oldrow+i*down]);
    gIdx.fdgp.splice(oldrow+(i+1)*down,1);
    gIdx.feed.splice(newrow+i*down,0,gIdx.feed[oldrow+i*down]);
    gIdx.feed.splice(oldrow+(i+1)*down,1);
    gIdx.catg.splice(newrow+i*down,0,gIdx.catg[oldrow+i*down]);
    gIdx.catg.splice(oldrow+(i+1)*down,1);
    gIdx.open.splice(newrow+i*down,0,gIdx.open[oldrow+i*down]);
    gIdx.open.splice(oldrow+(i+1)*down,1);
  }
  var tmpGrp = gFdGroup[oldgrp];
  gFdGroup.splice(oldgrp,1);
  gFdGroup.splice(newgrp-up,0,tmpGrp);
  grpChg(oldgrp,-2);
  if (newgrp > oldgrp)
  {
    newgrp--;
    for (var i=0; i<(newgrp-oldgrp); i++)
      grpChg(oldgrp+i+1,oldgrp+i);
  }
  else
    for (var i=0; i<(oldgrp-newgrp); i++)
      grpChg(oldgrp-i-1,oldgrp-i);
  grpChg(-2,newgrp);
  saveGroupModel();
  saveIndices();
  refreshModelSelect(newrow - up*num);
}

function grpChg(oldgrp,newgrp)
{
  for (var i=0; i<gIdx.fdgp.length; i++)
    if (gIdx.fdgp[i] == oldgrp)
      gIdx.fdgp[i] = newgrp;
}

function getGroupRow(grp)
{
  var i = gIdx.fdgp.length - 1;
  while (i >= 0 && gIdx.fdgp[i] >= grp) i--;
  return ++i;
}

function getFeedRow(grp,nFeed)
{
  var i = getGroupRow(grp);
  while (i < gIdx.fdgp.length && gIdx.fdgp[i] == grp && gIdx.feed[i] != nFeed) i++;
  return i;  // returns row after last feed if not in group
}

function moveIt(movingUp)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var index = feedtree.currentIndex;
  var curGrp = gIdx.fdgp[index];
  var curFeed = gIdx.feed[index];
  if (gIdx.catg[index] != 0) return;  // on category
  if (curFeed == -1)                // on group
  {
    if (movingUp && curGrp <= 1) return;
    if (!movingUp && curGrp == gFdGroup.length-1) return;
    var newGrp = curGrp + 2;
    if (movingUp) newGrp -= 3;
    mvGrp(curGrp,newGrp);
  }
  else                             // on feed
  {
    var curPos = -1;
    for (var i=0; i<gFdGroup[curGrp].list.length; i++)
      if (gFdGroup[curGrp].list[i] == curFeed) curPos = i;
    if (movingUp && curPos == 0) return;
    if (!movingUp && curPos == gFdGroup[curGrp].list.length-1) return;
    var newFeed;
    if (movingUp) newFeed = gFdGroup[curGrp].list[curPos-1]
    else if (curPos+2 == gFdGroup[curGrp].list.length) newFeed = -2
    else newFeed = gFdGroup[curGrp].list[curPos+2];
    mvFeed(curGrp,curFeed,newFeed);
  }
}

function mvFocus(treeName,direction,select)
{
  var tree = document.getElementById(treeName);
	var j = tree.currentIndex;
	if (direction == "up")
	{
		j--;
		if (j < 0) j = 0;
	}
	else
	{
		j++;
		if (j >= tree.view.rowCount) j = tree.view.rowCount - 1;
	}
	if (select == "select")
		tree.view.selection.select(j);
	else
		tree.currentIndex = j;
	tree.treeBoxObject.ensureRowIsVisible(j);
}

function mvFeed(curGrp,curFeed,newFeed)
{
  var feedtree = document.getElementById("newsfox.feedTree");
  var curRow = getFeedRow(curGrp,curFeed);
  var newRow = getFeedRow(curGrp,newFeed);
  var curPos;
  var newPos = gFdGroup[curGrp].list.length;
  for (var i=0; i<gFdGroup[curGrp].list.length; i++)
  {
    if (gFdGroup[curGrp].list[i] == curFeed) curPos = i
    else if (gFdGroup[curGrp].list[i] == newFeed) newPos = i;
  }
  var up = (newRow > curRow);
  if (curPos == newPos || curPos+1 == newPos) return;
  var curExpand = gIdx.open[curRow];
  if (curExpand) feedtree.view.toggleOpenState(curRow);
  gFdGroup[curGrp].list.splice(curPos,1);
  gFdGroup[curGrp].list.splice(newPos-up,0,curFeed);
  newRow = getFeedRow(curGrp,newFeed);
  gIdx.feed.splice(curRow,1);
  gIdx.feed.splice(newRow-up,0,curFeed);
  gIdx.fdgp.splice(curRow,1);
  gIdx.fdgp.splice(newRow-up,0,curGrp);
  gIdx.catg.splice(curRow,1);
  gIdx.catg.splice(newRow-up,0,0);
  gIdx.open.splice(curRow,1);
  gIdx.open.splice(newRow-up,0,false);
  if (curExpand) feedtree.view.toggleOpenState(newRow-up);

  saveGroupModel();
  saveIndices();
  refreshModelSelect(newRow-up);
}

function loadingTooltip(show)
{
	var tooltip = document.getElementById("loadingTooltip");
	if (show)
		if (gFF >= 3)
			tooltip.openPopup();
		else
			tooltip.showPopup();
	else
		tooltip.hidePopup();
}

function setPmeter(value)
{
	var pmeter = document.getElementById("pmeter");
	if (value == 0)
		pmeter.hidden = true;
	else
	{
		pmeter.hidden = false;
		pmeter.setAttribute("value", value + "%");
	}
}

function doHorizontal(horiz)
{
	var hartBox = document.getElementById("hbox3pane");
	var rightPane = document.getElementById("rightpane");
	if(horiz)
	{
		var vartBox = document.getElementById("vbox3pane");
		vartBox.setAttribute("orient","horizontal");
		rightPane.appendChild(vartBox);
		var vartsplitter = document.getElementById("vartsplitter");
		var hartsplitter = document.getElementById("hartsplitter");
		vartBox.replaceChild(hartsplitter,vartsplitter);
	}
	rightPane.removeChild(hartBox);
}

function resetGroupUnread()
{
  for (var i=0; i<gFdGroup.length; i++)
//  {
//		gFdGroup[i].pastUnread = gFdGroup[i].unread;
//		gFdGroup[i].unread = null;
//		gFdGroup[i].processUnread = false;
//	}
		resetGUnread(i);
}

function resetGUnread(i)
{
	gFdGroup[i].pastUnread = gFdGroup[i].unread;
	gFdGroup[i].unread = null;
	gFdGroup[i].processUnread = false;
}

function abc(a,b)
{
		var feedSorter = NFgetPref("advanced.feedSorter", "str", null);
		if (feedSorter)
		{
			var mult = 1;
			if (feedSorter.charAt(0) == "-")
			{
				mult = -1;
				feedSorter = feedSorter.substring(1);
			}
			var bval = eval("gFmodel.get(b)."+feedSorter);
			var aval = eval("gFmodel.get(a)."+feedSorter);
			if (bval || aval) return (bval < aval ? mult : -1*mult);
		}
		return gFmodel.get(b).getDisplayName().toLowerCase() < gFmodel.get(a).getDisplayName().toLowerCase() ? 1 : -1;
}

function AtoZ()
{
	var abcSandbox = new Components.utils.Sandbox(window);
	abcSandbox.importFunction(abc);

  var feedtree = document.getElementById("newsfox.feedTree");
	var row = feedtree.currentIndex;
	var nGrp = gIdx.fdgp[row];
	gGroup = nGrp + "," + gFdGroup[nGrp].list.length + "," + gFdGroup[nGrp].title;
	gJoin = gFdGroup[nGrp].list.join();
  var expand = gIdx.open[row];
  if (expand) feedtree.view.toggleOpenState(row);
	gFdGroup[nGrp].list.sort(abcSandbox.abc);
  if (expand) feedtree.view.toggleOpenState(row);
}

var gGroup = null;
var gJoin = null;

function unsortFeeds()
{
  var feedtree = document.getElementById("newsfox.feedTree");
	var row = feedtree.currentIndex;
	var nGrp = gIdx.fdgp[row];
  var expand = gIdx.open[row];
  if (expand) feedtree.view.toggleOpenState(row);
	var newOrder = new Array();
	newOrder = gJoin.split(",");
	gGroup = null;
	gJoin = null;
	for (var i=0; i<newOrder.length; i++)
		gFdGroup[nGrp].list[i] = newOrder[i];
  if (expand) feedtree.view.toggleOpenState(row);
}

function printArticle()
{
	if (document.getElementById("contentDeck").selectedIndex == 0)
		document.getElementById("hrefContent").contentWindow.print();
	else
//var win = document.getElementById("buildContent").contentWindow;
//win.focus();
//win.print();
		document.getElementById("buildContent").contentWindow.print();
}

function toggleFilter()
{
	var col = { id : "Xtend" }
	var arttree = document.getElementById("newsfox.articleTree");
	var row = arttree.currentIndex;
	if (row > -1) arttree.view.cycleCell(row,col);
}

function feedTreeInvalidate()
{
  var feedtree = document.getElementById("newsfox.feedTree");
  feedtree.treeBoxObject.invalidate();
}

function artTreeInvalidate()
{
	var arttree = document.getElementById("newsfox.articleTree");
	arttree.treeBoxObject.invalidate();
	return arttree;
}

function getNumGroups()
{
	for (var i=0; i<gFmodel.size(); i++) numGroupArray[i] = 0;
	for (var i=0; i<gFdGroup.length; i++)
	{
		if (!gFdGroup[i].search)
			for (var j=0; j<gFdGroup[i].list.length; j++)
				numGroupArray[gFdGroup[i].list[j]]++;
	}
}

function checkArtTreeColumnChange()
{
// could use mouseout(FF2+) or mouseleave(newer) but they are not quite right   
  document.getElementById("artTreeCols").addEventListener("command", columnsChanged, false);
  window.addEventListener('unload', 
    function() { document.getElementById("artTreeCols").removeEventListener("command", columnsChanged, false); }
    , false);
    
  if (gOptions.artTreeCols != "-") return;
// first run 
  var existingCols = getColumns();
  if (existingCols.indexOf("t") == -1) existingCols = "ftrpd";
  gOptions.artTreeCols = existingCols;
  gOptions.save();
}

var gSearchValue ="";
function doSearch()
{
	var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
		.getService(Components.interfaces.nsIPromptService);
	var check = {value: false};
	var input = {value: gSearchValue};
	var result = prompts.prompt(null, "", "SEARCH:", input, null, check);
	if (result) gSearchValue = input.value;
	else gSearchValue = "";
}
