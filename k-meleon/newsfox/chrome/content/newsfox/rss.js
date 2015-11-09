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

const KM_ALT_TITLE = "NEWSFOX";
const POLL_INTERVAL = 250;
const MATHML_ENTITY = " <!ENTITY % mDTD SYSTEM \"http://m/mathml.dtd\" > %mDTD; ";

var gNumToCheck;
var cancelCheck = false;
var xmlhttp = null;
var gNFPause = false;
var pauseTimeout;

var timeList = new Array();

////////////////////////////////////////////////////////////////
// Check feeds
////////////////////////////////////////////////////////////////

function setupFeedCheck(gFeedsToCheck,resetFeedNumber)
{
  var len = gFeedsToCheck.length;
  if (len == 0) postRefresh();
  if (resetFeedNumber) gNumToCheck = len;
  for (var i=0; i<gOptions.threads; i++)
    if (gNumToCheck > i) precheckFeed(gFeedsToCheck);
}

function addAnotherCheck()
{
// only add a new one if we don't have too many going
  var now = Date.now();
  var num = 0;
  for (var i=0; i<timeList.length; i++) num += 1*((now - timeList[i].time + 1) < gOptions.renewTimeout);
  if (num < gOptions.threads) precheckFeed(gFeedsToCheck);  
}

function precheckFeed(gFeedsToCheck)
{
	if (gNFPause && gFeedsToCheck.length)
	{
		updateFinishStatus();
		return;
	}
  var url = gFeedsToCheck.shift();
  if (url == null)
  {
    if (gCheckInProgress) postRefresh();
    return;
  }

  var i=gFmodel.size();
  while(gFmodel.get(--i).url != url) if (i==0) precheckFeed(gFeedsToCheck);
  var index = i;

// TODO using this causes article pane to reset, checking feed with categories
//     open is okay?, but may not be displaying all categories afterward
//     I didn't like the jumping around of the feedtree with this in either
// closes feed before checking as # of categories may change
//  var feedtree = document.getElementById("newsfox.feedTree");
//  var row = feedtree.currentIndex;
//  if (row != -1)
//  {
//    var curGrp = gIdx.fdgp[row];
//    var nFeed = gIdx.feed[row];
//    i= gIdx.feed.length;
//    while (--i >= 0)
//      if (gIdx.feed[i] == index && gIdx.open[i] == true)
//        feedtree.view.toggleOpenState(i);
//    refreshModelSelect(getFeedRow(curGrp,nFeed));
//  }

  var elem = document.getElementById("busyTextNumbers");
  elem.value = (gNumToCheck-gFeedsToCheck.length) + " / " + gNumToCheck;

  var feed = gFmodel.get(index);
	loadFeed(feed,true,false);
	if (!navigator.onLine && gCheckInProgress)
	{
		if (displayInRefresh(feed,index))
		{
			var artId = getArtId();
    	feedSelected();
			selectArt(artId);
		}
		postRefresh();
		return;
	}
  feed.error = ERROR_REFRESH;
	feedTreeInvalidate();

	var httpicon = document.getElementById("newsfox-icon");
	httpicon.src = feed.icon.src;
	httpicon.width = 16;
	httpicon.height = 16;

	url = feed.url;
	doXMLHttpRequest(url,url,feed.username,feed.password,gFeedsToCheck,0)
}

function doXMLHttpRequest(urlFeed,urlSent,username,password,gFeedsToCheck, repeat)
{
	// FF used to throw an xmlhhtp error on file not found, but now returns OK
	// from xmlhttp request
  var httpRenewTimeout = setTimeout(addAnotherCheck, gOptions.renewTimeout);
	if (urlFeed.substring(0,4) == "file")
	{
		var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
		var file = nsIPH.getFileFromURLSpec(urlFeed);
		if (!file.exists())
		{
			abortHttpRequest(urlFeed, ERROR_NOT_FOUND, httpRenewTimeout);
			return;
		}
	}
  var xmlhttp = new XMLHttpRequest();
	xmlhttp.mozBackgroundRequest = true;  // Supress Firefox authorization bug#21379
	if (repeat == 2) xmlhttp.mozBackgroundRequest = false;  // allow input of username/password for feeds where not saved
  // trick of adding the current time to bypass the cache; suggested by Konstantin Svist
	// Commented out since 0.6.4 because of problems the trick caused with different feeds
	// url += (url.match(/\?/) == null ? '?' : '&') + (new Date()).getTime();
	// Instead, Ron Pruitt proposed to put it as experienced user option to replace:
	var urlsend = urlSent.replace(/%CURRENT_DATETIME%/, (new Date()).getTime());
  
  var tmp = { url: urlFeed, time: Date.now() };
  timeList.push(tmp);
  
	try
	{
		if (username || password)
			xmlhttp.open("GET", urlsend, true, gSdr.decryptString(username), gSdr.decryptString(password));
		else
			xmlhttp.open("GET", urlsend, true);
	  xmlhttp.setRequestHeader("User-Agent", "Mozilla/5.0 NewsFox/" + VERSION);
	  xmlhttp.overrideMimeType("application/xml");
	  xmlhttp.onload = function() { checkStatus(xmlhttp,gFeedsToCheck,urlFeed,urlsend,username,password,repeat,httpRenewTimeout); }
	  // TODO: do error handling  timeout repsonds as 200-OK?
	//  xmlhttp.onerror = function() { checkStatus(xmlhttp,gFeedsToCheck,urlFeed,urlsend); }
		xmlhttp.send(null);
	}
	catch(e)
		{ abortHttpRequest(urlFeed, ERROR_INVALID_FEED_URL, httpRenewTimeout) }
}

function checkStatus(xmlhttp, gFeedsToCheck, urlFeed, urlSent, username, password, repeat, httpRenewTimeout)
{
	var feed = gFmodel.getFeedByURL(urlFeed);
	var url2 = xmlhttp.getResponseHeader("location");
	var urlLookup = urlFeed;
	switch (xmlhttp.status)
	{
		case 200:  // OK
			checkFeed(xmlhttp, gFeedsToCheck, urlFeed, urlSent, httpRenewTimeout);
			break;
		case 301:  // permanent redirect
			feed.url = url2;
			urlLookup = url2;
		case 300:  // multiple choices  ??
		case 302:  // found
		case 303:  // see other
		case 307:  // temporary redirect
			doXMLHttpRequest(urlLookup,url2,username,password,gFeedsToCheck,repeat);
			break;
		case 401:  // livejournal digest bug #21379 code from sg2002 patch
			repeat++;
			if (repeat < 3)
				doXMLHttpRequest(urlFeed,urlSent,username,password,gFeedsToCheck, repeat);
			break;
		case 404:  // not found
			abortHttpRequest(urlFeed, ERROR_NOT_FOUND, httpRenewTimeout);
			break;
		case 0: // local file returns zero???
			if (urlFeed.substring(0,4) == "file")
			{
				checkFeed(xmlhttp, gFeedsToCheck, urlFeed, urlSent, httpRenewTimeout);
				break;
			}
		default:
//		case 304:  // not modified Firefox handles?
      var err = ERROR_SERVER_ERROR + xmlhttp.status + " - " + xmlhttp.statusText;
			abortHttpRequest(urlFeed, err, httpRenewTimeout);
	}
}

function checkFeed(xmlhttp, feedsToCheck, urlFeed, urlSent, httpRenewTimeout)
{
  removeUrlFromTimeList(urlFeed);
  if (null != httpRenewTimeout) clearTimeout(httpRenewTimeout);
	var index = gFmodel.getIndexByURL(urlFeed);
  try
  {
		if (index == -1 && (feedsToCheck == gFeedsToCheck))
			precheckFeed(gFeedsToCheck);
    var feed = gFmodel.getFeedByURL(urlFeed);
		loadFeed(feed,true,false);

		var xml = xmlhttp.responseXML;
		xml = fixStupidity(xmlhttp);
		if (xml.documentElement.localName.toLowerCase() == 'parsererror')
			xml = repairIt(xmlhttp);
		var parser = new Parser2(xml,urlSent);

		var refreshingDispFeed = false;
		if ((gCollect.type == 1 || gCollect.type == 2) && feed == gCollect.getFeed(0))
			refreshingDispFeed = true;
    if (parser.title != null) 
      feed.defaultName = parser.title;
		if (refreshingDispFeed)
			document.getElementById("feedTitle").value = document.getElementById("mfeedTitle").value = feed.getDisplayName();


		if (feed.homepage == null || feed.homepage == "")
			feed.homepage = encodeUrl(parser.link);
		downloadIcon(feed);

		var now = new Date();
		var gDeleteOldStyle = gOptions.globalDeleteOldStyle;
		var deleteOldStyle = feed.deleteOldStyle;
		var deleteOld = (deleteOldStyle != 0) ?
				(deleteOldStyle == 1 || deleteOldStyle == 2) :
				(gDeleteOldStyle == 1 || gDeleteOldStyle == 2);
		var dontDeleteUnread = (deleteOldStyle != 0) ?
				(deleteOldStyle == 1) :
				(gDeleteOldStyle == 1);
		var daysKeep = (deleteOldStyle != 0) ?
				feed.daysToKeep : gOptions.daysKeep;
		for (var i=0; i<feed.size(); i++)
		{
			var art = feed.get(i);
			art.newUnread = false;
			var artAge = now - art.date;
			var isOld = artAge > daysKeep*24*60*60*1000;
			var canRemove = isOld && deleteOld && !feed.isFlagged(i) && (feed.isRead(i) || !dontDeleteUnread);
			if (canRemove)
			{
				if (daysKeep != -1)
				{
					var newArt = new Article();
					newArt.id = art.id;
					feed.deletedAdd(newArt);
					art.id = null;
				}
				art.toRemove = true;
			}
			else
				art.toRemove = false;
		}
		for (i=0; i<feed.deletedsize(); i++)
			feed.deletedget(i).toRemove = true;

		var idArray = new Array();
		var indexFrom = new Array();
		for (i=0; i<parser.items.length; i++)
		{
			var uniq = true;
			for (var j=0; j<idArray.length; j++)
				if (parser.items[i].id == idArray[j].id)
				{
					uniq = false;
					if (parser.items[i].date > idArray[j].date)
						parser.items[indexFrom[j]].id = null;
					else
						parser.items[i].id = null;
				}
			if (uniq)
			{
				idArray.push(parser.items[i]);
				indexFrom.push(i);
			}
		}

		if (gOptions.bookmarkSync)
			feed.lastUpdate = gBookmarkSync.importB(feed);

    for (i=0; i<parser.items.length; i++)
    {
      var item = parser.items[i];
      if (doesArticleExist(feed, item)) continue;
			if (gOptions.spam)
	//			if (i < S_MAX_ARTS) spamFilterAdd(item);
	//			else gArtsToAddSpam.push(item);
				gArtsToAddSpam.push(item);
      if (!item.title || item.title == "") item.title = (item.body) ? entityDecode(item.body).substr(0, 70) + "..." : "...";
			var article;
			if ((item.id && !gOptions.bookmarkSync) || item.date < TOP_NO_DATE || item.date > feed.lastUpdate)
			{
				item.newUnread = true;
				gNewItemsCount++;
      	article = feed.add(item,0);  // unread, unflagged
			}
			else
				article = feed.add(item,1);  // read, unflagged

			if (feed.XfilterNew && article) getXbody(article, feed);
    }

		if (gArtsToAddSpam.length > 0) setTimeout(doAddSpam, 50);

	// Need to turn off article pane here if it is from this feed since
	// collection and feed won't agree once we start deleting and sorting
	// adding articles above is okay since disagreement is at the end
		var refreshingDispColl = displayInRefresh(feed,index);
		if (refreshingDispColl)
		{
			var artId = getArtId();
    	var arttree = document.getElementById("newsfox.articleTree");
			arttree.view = null;  // will be replaced with new one, no need to save
		}

		for (i=feed.size()-1; i>=0; i--)
			if (feed.get(i).toRemove) feed.remove(i);
		for (i=feed.deletedsize()-1; i>=0; i--)
			if (feed.deletedget(i).toRemove) feed.deletedremove(i);

		if (gOptions.spam) spamScoreUpdate(feed);
		cleanUpFeed(feed,index);
  	feed.error = ERROR_OK;
		feedTreeInvalidate();
  }
  catch (err)
	{
		gFmodel.get(index).error = err.toString();
		feedTreeInvalidate();
	}

  // Update Title, feedTree, and articleTree if current feed
	setTitle(true);
	feedTreeInvalidate();
	resetGroupUnread();
	if (refreshingDispColl)
	{
    feedSelected();  // replaces arttree.view with new one
		selectArt(artId);
	}

	if (!gCheckInProgress || (feedsToCheck != gFeedsToCheck))
		return;
  else if (!cancelCheck)  // Check next feed
    precheckFeed(gFeedsToCheck);
  else              // We're done!
    postRefresh();
}

function doAddSpam()
{
	if (gArtsToAddSpam.length == 0) return;
	var art = gArtsToAddSpam.shift();
	spamFilterAdd(art);
	setTimeout(doAddSpam, 50);
}

function spamFilterAdd(item)
{ 
	var titleArray = getTitleArray(item);
	var j= gWordArray.length - 1;
	for (var i=titleArray.length-1; i>=0; i--)
	{
		while (gWordArray[j] > titleArray[i] && j>=0) j--;
		if (gWordArray[j] == titleArray[i])
		{
			gGoodArray[j] += S_GOOD_START;
			gTotalArray[j++] += S_TOTAL_START;
		}
		else
		{
			gWordArray.splice(++j,0,titleArray[i]);
			gGoodArray.splice(j,0,S_GOOD_START);
			gTotalArray.splice(j++,0,S_TOTAL_START);
		}
		if (j >= gWordArray.length) j = gWordArray.length - 1;
	}
}

function spamScoreUpdate(feed)
{
	// do newest articles first
	var now = new Date();
	for (var k=feed.size()-1; k>=0; k--)
	{
		var item = feed.get(k);
		var isRead = feed.isRead(k);
		var artAge = now - item.date;
		if (!isRead && artAge < S_NEW_DAYS*24*60*60*1000)
			spamScoreItem(item,isRead);
		else
			gArtsToScoreSpam.push(item);
	}
	gFeedsToSaveSpam.push(feed);
	setTimeout(doMoreScores, 50);
}

function doMoreScores()
{
	if (gArtsToScoreSpam.length == 0)
	{
		while (gFeedsToSaveSpam.length > 0)
		{
			var feed = gFeedsToSaveSpam.shift();
			saveFeed(feed);
		}
		saveFeedModel();
		return;
	}
	var art = gArtsToScoreSpam.shift();
	spamScoreItem(art,true);
	setTimeout(doMoreScores, 50);
}

function spamScoreItem(item,isRead)
{
		var titleArray = getTitleArray(item);
		var gVal = 1;
		var bVal = 1;
		var p;
		var j= gWordArray.length - 1;
		for (var i=titleArray.length-1; i>=0; i--)
		{
			while (gWordArray[j] > titleArray[i] && j>=0) j--;
			if (gWordArray[j] == titleArray[i] && gTotalArray[j] > S_MINCOUNT)
			{
				p = gGoodArray[j]/gTotalArray[j];
				gVal *= p;
				bVal *= (1-p);
			}
			j += 2;
			if (j >= gWordArray.length) j = gWordArray.length - 1;
		}
		var pp = S_UNREADPCT;
		if (isRead)
			item.prob = ((1-pp)*gVal)/(((1-pp)*gVal)+(pp*bVal));
		else
			item.prob = (pp*gVal)/((pp*gVal)+((1-pp)*bVal));
}

function getTitleArray(item)
{
	var titleArray = (item.title+" "+item.body).split(/\s+/);  // whitespace
	var i = 0;
	var done = false;
	while (!done)
	{
		titleArray[i] = titleArray[i].toLowerCase();
		// remove entire word with '=' or an entity
		titleArray[i] = titleArray[i].replace(/.*(=|&.+?;).*/,"");
		// remove html tags and common remnants
		titleArray[i] = titleArray[i].replace(/<.*?>|<a$|<p$|<img$|<span$|<div$/g,"");
		// remove junk at ends
		// other possibilities    & $ * #
		var junk = "[\\?'\"\\(\\)\\.\\!<>:;,\\[\\]\\{\\}\\/]+";
		titleArray[i] = titleArray[i].replace(new RegExp("^" + junk + "|" + junk + "$|'s$","g"),"");
		if (titleArray[i].length < S_MINWORDLENGTH)
			titleArray.splice(i,1);
		else
			i++;
		if (i >= S_MAXCOMPARE)
		{
			done = true;
			titleArray.length = S_MAXCOMPARE;
		}
		else if (i >= titleArray.length) done = true;
	}
	return titleArray.sort();
}

function displayInRefresh(feed,index)
{
	if ((gCollect.type == 1 || gCollect.type == 2) && feed == gCollect.getFeed(0))
		return true;
	else if (gCollect.type == 0 || gCollect.type == 3)
	{
		var curGrp = gFdGroup[gCollect.grpindex];
		for (var i=0; i<curGrp.list.length; i++)
			if (index == curGrp.list[i]) return true;
	}
	return false;
}

function cleanUpFeed(feed,index)
{
		var sortcollect = new NormalCollection(index,0,false);  // index of feed
		doDefaultSort(sortcollect,false);
		feed.lastUpdate = new Date();
		if (gOptions.bookmarkSync) gBookmarkSync.exportB(feed);
		feed = deleteDuplicates(feed);
    feed.sortCategories();
    saveFeed(feed);
		saveFeedModel();   // keep flags synchronized on disk
}

function getArtId()
{
	var arttree = document.getElementById("newsfox.articleTree");
	var artIndex = arttree.currentIndex;
	var artId = null;
	// need second condition due to Firefox bug#413266
	if (artIndex > -1 && arttree.view.selection.count > 0)
		artId = gCollect.get(artIndex).id;
	return artId;
}

function selectArt(artId)
{
	var arttree = document.getElementById("newsfox.articleTree");
	var index = -1;
	if (artId != null)
		for (i=0; i<gCollect.size(); i++)
			if (gCollect.get(i).id == artId) index = i;
	if (index == -1) return;
	gDisplayArticle = false;
	arttree.view.selection.select(index);
	gDisplayArticle = true;
	arttree.treeBoxObject.ensureRowIsVisible(index);
}

function removeUrlFromTimeList(urlFeed)
{
  var i=timeList.length;
  while (i>0) if (timeList[--i].url == urlFeed) timeList.splice(i,1);
}

function abortHttpRequest(urlFeed, ERROR, httpRenewTimeout)
{
  removeUrlFromTimeList(urlFeed);
  if (null != httpRenewTimeout) clearTimeout(httpRenewTimeout);
	var useError = (ERROR) ? ERROR : ERROR_OK;
  gFmodel.getFeedByURL(urlFeed).error = useError;
	stopHttpRequest();
}

function stopHttpRequest()
{
	if (null != pauseTimeout) clearTimeout(pauseTimeout);
	feedTreeInvalidate();
	if (!cancelCheck)
		precheckFeed(gFeedsToCheck);
	else
		postRefresh();
}

function setBusyText(val)
{
	var busyText = document.getElementById("busyText");
	var pauseIcon = document.getElementById("pause-icon");
	var playIcon = document.getElementById("play-icon");
	const NF_SB = document.getElementById("newsfox-string-bundle");
	busyText.value = NF_SB.getString(val);
	if (NFgetPref("advanced.showPauseButton","bool",true))
	{
		switch (val)
		{
			case "checking":
				playIcon.hidden = true;
				pauseIcon.removeAttribute("hidden");
				break;
			case "paused":
				pauseIcon.hidden = true;
				playIcon.removeAttribute("hidden");
				break;
		}
	}
}

function cancelTheCheck()
{
  cancelCheck = true;
  updateFinishStatus();
}

function changePauseStatus()
{
	gNFPause = !gNFPause;
	updateFinishStatus();
}

function updateFinishStatus()
{
	if (null != pauseTimeout) clearTimeout(pauseTimeout);
	if (cancelCheck)
	{
		stopHttpRequest();
		return;
	}
	if (gNFPause)
	{
		setBusyText("paused");
		pauseTimeout = setTimeout(updateFinishStatus, POLL_INTERVAL);
	}
	else
	{
		setBusyText("checking");
		setupFeedCheck(gFeedsToCheck,false);
	}
}

function postRefresh()
{
  timeList.length = 0;
	var httpicon = document.getElementById("newsfox-icon");
	httpicon.src = "chrome://newsfox/skin/newsfox-16.png";
  var elem = document.getElementById("busyTextNumbers");
  elem.value = "";
  var elem = document.getElementById("notBusyText");
	elem.value = NEWSFOX + " " + VERSION;
  elem.removeAttribute("hidden");
  var elem = document.getElementById("busyText");
  elem.hidden = "true";
	document.getElementById("tBcancel").setAttribute("hidden",true);
	document.getElementById("tBcheck").removeAttribute("hidden");
	document.getElementById("mfBcancel").setAttribute("hidden",true);
	document.getElementById("mfBcheck").removeAttribute("hidden");
	document.getElementById("fBcancel").setAttribute("hidden",true);
	document.getElementById("fBcheck").removeAttribute("hidden");
  cancelCheck = false;
  gFeedsToCheck = new Array();
	gNFPause = false;
	if (null != pauseTimeout) clearTimeout(pauseTimeout);
	setBusyText("checking");
	var pauseIcon = document.getElementById("pause-icon");
	pauseIcon.hidden = true;
	var playIcon = document.getElementById("play-icon");
	playIcon.hidden = true;
//	pauseIcon.src = "chrome://newsfox/skin/images/pause.png";
	if (gNewItemsCount > 0)
	{
		if(gOptions.notifyUponNew) reportRefreshResults();
		if(gOptions.notifyUponNewSound) resultsSound();
	}
  gCheckInProgress = false;
}

function reportRefreshResults()
{
	if (gKMeleon)
	{
		const DONETIME = 500;
		const TITLETIME = 500;
		const BLINKS = 5;
		for (var i=0; i<BLINKS; i++)
		{
			var offset = i*(DONETIME+TITLETIME);
			setTimeout(doneTitle,1+offset);
			setTimeout(setTitle,1+DONETIME+offset);
		}
	}
	else
	{
		var unreadTotalCount = gFdGroup[0].getUnread();
		if( gNewItemsCount > 0 )
		{
			const NF_SB = document.getElementById("newsfox-string-bundle");
			var strNew = NF_SB.getString('alert.new');
			var strUnread = NF_SB.getString('alert.unread');
			var message = gNewItemsCount + " " + strNew + ", " + unreadTotalCount + " " + strUnread;
			var alerts = Components.classes["@mozilla.org/alerts-service;1"]
	                          .getService(Components.interfaces.nsIAlertsService);
			alerts.showAlertNotification("chrome://newsfox/skin/newsfox-32.png", "NewsFox", message, false, "", null);
		}
	}
}

function resultsSound()
{
	var snd = Components.classes["@mozilla.org/sound;1"].createInstance(Components.interfaces.nsISound);
	var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
	var file = NFgetProfileDir();
	file.append(NFSOUND);
	var soundURL = adjustBase(null,nsIPH.getURLSpecFromFile(file));
	if (file.exists())
		snd.play(soundURL);
	else
		snd.beep();
}

function doneTitle()
{
	document.title = KM_ALT_TITLE;
}

////////////////////////////////////////////////////////////////
// Util
////////////////////////////////////////////////////////////////

/**
 * Encode url problem charaters.
 */
function encodeUrl(s)
{
	if (!s) return "";  // so we know it's not a new feed any more
  s = s.replace(new RegExp('&','gi'), '&amp;');
  return s;
}

/**
 * Return true if this article already exists.
 */
function doesArticleExist(feed, item)
{
	var id = item.id;
	if (id == null) return false;

  for (var i=0; i<feed.size(); i++)
		if (feed.get(i).id == id)
		{
			var art = feed.get(i);
			if (!item.title || item.title == "") item.title = (item.body) ? entityDecode(item.body).substr(0, 70) + "..." : "...";
			if (art.title != item.title || art.body != item.body)
			{
				feed.set(i,item);
				if ((feed.changedUnread == 1 || 
					(feed.changedUnread == 0 && gOptions.changedUnread)) && 
					(entityDecode(art.body) != entityDecode(item.body)))
					feed.setRead(i,false);
			}
			feed.get(i).toRemove = false;
			return true;
		}
  for (var i=0; i<feed.deletedsize(); i++)
    if (feed.deletedget(i).id == id)
		{
			feed.deletedget(i).toRemove = false;
      return true;
		}
  return false;
}

function downloadIcon(feed)
{
  if (!gOptions.favicons)
	{ 
		for (var i=0; i<gFmodel.size(); i++)
			if (gFmodel.get(i).storage)
				gFmodel.get(i).icon.src = ICON_STORAGE;
			else
				gFmodel.get(i).icon.src = ICON_OK;
	}
  else
  {
    if (feed.icon.src == null || feed.icon.src == "" || feed.icon.src == ICON_OK)
    {
      feed.icon.src = ICON_OK;
			// don't guessHomepage before feed refreshed, if feed.homepage is null
			if (gOptions.guessHomepage && feed.homepage == "")
				feed.homepage = guessHomepage(feed);
      if (feed.homepage != null && feed.homepage != "")
      {
        var favicon = feed.homepage.replace("index.html","");
        if (favicon.charAt(favicon.length-1) != "/") favicon += "/";
        favicon += "favicon.ico";
    		var file = NFgetProfileDir();
    		file.append(feed.uid + ".ico");
				getFavIcon(favicon,file);
      }
    }
  }
}

function getFavIcon(favicon,file)
{
	try
	{
	  var IOService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
	  var IOchannel = IOService.newChannel(favicon,null,null);
	  var nfListener = Components.classes["@mozilla.org/network/downloader;1"].createInstance(Components.interfaces.nsIDownloader);
	  nfListener.init(nfObserver,file);
	  IOchannel.asyncOpen(nfListener,null);
	}
	catch(e) {}
}

function isImg(file)
{
// TODO doesn't work with K-M
//  if (!file.exists() || file.fileSize == 0) return false;
  if (file.fileSize == 0) return false;
  var inputStream = Components.classes["@mozilla.org/network/file-input-stream;1"].createInstance( Components.interfaces.nsIFileInputStream );
  inputStream.init( file,-1,-1,null);
  var scInputStream = Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance( Components.interfaces.nsIScriptableInputStream );
  scInputStream.init(inputStream);
  var output = scInputStream.read(-1);
  scInputStream.close();
  inputStream.close();
  if (output.toLowerCase().indexOf('html') == -1) return true;
  return false;
}

var nfObserver =
{
  onDownloadComplete: function(adownloader, arequest, actxt , astatus, aresult)
  {
    var aleafName = aresult.leafName;
    var auid = aleafName.substr(0,aleafName.length-4);
    var i=gFmodel.size();
		if (i==0) return;
    while(gFmodel.get(--i).uid != auid) if (i==0) return;
    if (isImg(aresult)) gFmodel.get(i).icon.src = getFileSpec(aresult);
  }
}

function guessHomepage(feed)
{
	var hmpg = feed.url;
	var feedburner = hmpg.indexOf("feeds.feedburner.com");
	hmpg = hmpg.replace("feeds.feedburner.com/","www.");
	var start = hmpg.indexOf("file://");
	if (start != -1) return "";
	start = hmpg.indexOf("://");
	if (start == -1) return "";
	var end = hmpg.indexOf("/",start+3);
	if (end > -1) hmpg = hmpg.substring(0,end);
	if (feedburner > -1) hmpg += ".com";
	hmpg = hmpg.replace("/rss.","/www.");
	hmpg += "/";
	return hmpg;
}

function deleteDuplicates(feed)
{
	for (var i=feed.size()-1; i>=0; i--)
	{
		var art = feed.get(i);
		if (art.id == art.link)  // never delete ones with real ids
			for (var j=i+1; j<feed.size(); j++)
			{
				var art2 = feed.get(j);
				if (Math.abs(art.date-art2.date) > 1000) break;
				if (art2.link == art.link && art2.title == art.title && encStr(art2.body) == encStr(art.body))
					feed.remove(i);
			}
	}
	return feed;
}

function encStr(s) 
{
	if( !s ) return "";
	var i=0;
	var code, replace, hex, j, from;
	while (i < s.length)
	{
		code = s.charCodeAt(i);
		if (code < 32 || code > 126)
		{
			replace = "&#" + code + ";";
			hex = code.toString(16);
			for (j=hex.length; j<4; j++) hex = "0" + hex;
			from = "\\u" + hex;
			s = s.replace(new RegExp(from,"g"),replace);
			i += replace.length;
		}
		else
			i++;
	}
  return s;
}

// temporary function until Firefox handled external DTDs in xml parsing
// FF bug#22942
function repairIt(xmlhttp)
{
	var xml2 = xmlhttp.responseXML;
	var domParser = new DOMParser();
	// kludge: add mathml.dtd to doctype
	var httpText = xmlhttp.responseText;
	var endHeader = httpText.indexOf("?>");
	if (endHeader > -1)
	{
		var docIndex = httpText.indexOf("<!DOCTYPE");
		if (docIndex > -1)
		{
			var strtDtd = httpText.indexOf("[",docIndex);
			var nestLevel = 1;
			var index = docIndex+1;
			var nxtLt = httpText.indexOf("<",index);
			var nxtGt = httpText.indexOf(">",index);
			while (nestLevel > 0)
			{
				if (nxtLt < nxtGt)
				{
					nestLevel++;
					index = nxtLt+1;
					nxtLt = httpText.indexOf("<",index);
				}
				else
				{
					nestLevel--;
					index = nxtGt+1;
					nxtGt = httpText.indexOf(">",index);
				}
			}
			var endDoctype = httpText.lastIndexOf(">",nxtGt-1);
			if (strtDtd > -1 && strtDtd < endDoctype)
				var newText = httpText.substring(0,strtDtd+1) + MATHML_ENTITY + httpText.substring(strtDtd+1);
			else
				var newText = httpText.substring(0,endDoctype) +" [" + MATHML_ENTITY + "] " + httpText.substring(endDoctype);
		}
		else
			var newText = httpText.substring(0,endHeader+2) + "\n<!DOCTYPE mathml [" + MATHML_ENTITY + "]>\n" + httpText.substring(endHeader+2);
		xml2 = domParser.parseFromString(newText, "application/xml");
	}
	if (xml2.documentElement.localName.toLowerCase() == 'parsererror')
	{
		// remove most common non XML characters
		httpText = httpText.replace(/[\n|\r|\t]/g," ");
		httpText = httpText.replace(/[\x00-\x1F]/g,"");
		xml2 = domParser.parseFromString(httpText, "application/xml");
	}
	if (xml2.documentElement.localName.toLowerCase() == 'parsererror')
	{
		// from Nils Maier, Sage bug#15473, just replace & with &amp;
		httpText = httpText.replace(/&(?!amp;|quot;|lt;|gt;)/gm, '&amp;');
		xml2 = domParser.parseFromString(httpText, "application/xml");
	}
	if (xml2.documentElement.localName.toLowerCase() == 'parsererror')
	{
		var tmp = httpText.indexOf("<?xml");
		httpText = httpText.substring(tmp);
		xml2 = domParser.parseFromString(httpText, "application/xml");
	}
	return xml2;
}

// for feeds with xmlns="http://backend.userland.com/rss2"
function fixStupidity(xmlhttp)
{
	var xml2 = xmlhttp.responseXML;
	var re = new RegExp("xmlns=(['\"])http\:\\/\\/backend.userland.com\\/rss2\\1");
	if (xmlhttp.responseText.match(re))
	{
		var domParser = new DOMParser();
		xml2 = domParser.parseFromString(xmlhttp.responseText.replace(re,""), "application/xml");
	}
	return xml2;
}
