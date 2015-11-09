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

const BACKUPS = "backups";
const NUMBACKUP = 5;

var numNewFeed = 0;
var fromBackup = true;
var opmlStack = new Array();
var expGrp0;
var fRow;
var opmlFeedStack = new Array();
var reqOpml;

////////////////////////////////////////////////////////////////
// Export OPML
////////////////////////////////////////////////////////////////

function exportOpml(doAll)
{
  var picker = Components.classes["@mozilla.org/filepicker;1"].
    createInstance(Components.interfaces.nsIFilePicker);
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var wintitle = document.getElementById("exportopml").getAttribute("label");
  picker.init(window, wintitle, picker.modeSave);
	var opmlFiles = NF_SB.getString('opmlfilesfilter');
  picker.appendFilter(opmlFiles, "*.xml;*.opml");
  picker.appendFilters(picker.filterAll);
  picker.defaultString = "newsfox.opml";
  
  var result = picker.show();
  if(result == picker.returnOK || result == picker.returnReplace)
	{
		var file = picker.file;
		if(file.exists()) file.remove(true);
		file.create(file.NORMAL_FILE_TYPE, 0666);
		expOpml(file,true,doAll);
	}
}

function expOpml(file,report,doAll)
{
	try 
	{
		var lowIndex, highIndex;
		if (doAll)
		{
			lowIndex = 0;
			highIndex = gFdGroup.length-1;
		}
		else
		{
  		var feedtree = document.getElementById("newsfox.feedTree");
  		var index = feedtree.currentIndex;
			lowIndex = gIdx.fdgp[index];
			highIndex = lowIndex;
		}

		var srcTemplate =  '<?xml version="1.0" encoding="UTF-8"?>';
		srcTemplate += "<opml version=\"1.0\">\n";
		srcTemplate += "<head>\n\t<title>NewsFox OPML Export</title>\n";
		var currentDate = new Date();
		srcTemplate += "\t<dateModified>" + currentDate.toString() + "</dateModified>\n";
		srcTemplate += "</head>\n";
		srcTemplate += "<body/>\n</opml>";

		var opmlDoc = new DOMParser().parseFromString(srcTemplate, "text/xml");
		var opmlBody = opmlDoc.getElementsByTagName("body")[0];

		var groupNode, feedNode, feed;
		for (var i=lowIndex; i<=highIndex; i++)
		{
			groupNode = opmlDoc.createElement("outline");
			groupNode.setAttribute("text",gFdGroup[i].title);
			if (gFdGroup[i].search && gFdGroup[i].searchTag)
			{
				groupNode.setAttribute("type","NFtag");
				groupNode.setAttribute("NFsearchTag",gFdGroup[i].searchTag);
			}
			else if (gFdGroup[i].search)
			{
				var srchdat = gFdGroup[i].srchdat;
				groupNode.setAttribute("type","NFsearch");
				groupNode.setAttribute("NFflagged",srchdat.flagged);
				groupNode.setAttribute("NFunread",srchdat.unread);
				groupNode.setAttribute("NFtext",srchdat.text);
				groupNode.setAttribute("NFtextflags",srchdat.textflags);
				groupNode.setAttribute("NFstartTime",srchdat.startTime);
				groupNode.setAttribute("NFendTime",srchdat.endTime);
			}
			else
				groupNode.setAttribute("type","NFgroup");
			opmlBody.appendChild(groupNode);
			for (var j=0; j<gFdGroup[i].list.length; j++)
			{
				feed = gFmodel.get(gFdGroup[i].list[j]);
				feedNode = opmlDoc.createElement("outline");
				feedNode.setAttribute("type","rss");
				feedNode.setAttribute("text",feed.getDisplayName());
				feedNode.setAttribute("xmlUrl",feed.url);
				feedNode.setAttribute("htmlUrl",feed.homepage);
				feedNode.setAttribute("NFuid",feed.uid);
				feedNode.setAttribute("NFdeleteOldStyle",feed.deleteOldStyle);
				feedNode.setAttribute("NFautoCheck",feed.autoCheck);
				feedNode.setAttribute("NFstyle",feed.style);
				feedNode.setAttribute("NFstorage",feed.storage);
				feedNode.setAttribute("NFlastUpdate",feed.lastUpdate);
				feedNode.setAttribute("NFautoRefreshInterval",feed.autoRefreshInterval);
				groupNode.appendChild(feedNode);
			}
		}  

		var opmlSource = new XMLSerializer().serializeToString(opmlDoc);
		var unicodeConverter = Components.classes['@mozilla.org/intl/scriptableunicodeconverter'].getService(Components.interfaces.nsIScriptableUnicodeConverter);
		unicodeConverter.charset = "UTF-8";
		opmlSource = unicodeConverter.ConvertFromUnicode(opmlSource);
		opmlSource += "\n";

	  var out = Components.classes['@mozilla.org/network/file-output-stream;1'].
	    createInstance(Components.interfaces.nsIFileOutputStream);
	  out.init(file, 2, 0x200, false); // open as "write only"
	  out.write(opmlSource, opmlSource.length);
	  out.close();

		const NF_SB = document.getElementById("newsfox-string-bundle");
		var rpt = NF_SB.getString('opmlexport');  
	  if (report) alert(rpt);
	} 
	catch (err) 
	{
	  if (report) alert("Export failed: " + err);
	}
}

function backupOpml()
{
	var now = new Date();
	var nowFilename = getBackupFilename(now);
	var file = NFgetProfileDir();
	file.append(BACKUPS);
	if (!file.exists()) file.create(file.DIRECTORY_TYPE, 0750);
	file.append(nowFilename);
	if(file.exists())
		return;
	else
	{
		var backupDir = NFgetProfileDir();
		backupDir.append(BACKUPS);
		var backupEnum = backupDir.directoryEntries;
		var dirFiles = new Array();
		var i = 0;
		while (backupEnum.hasMoreElements())
			dirFiles[i++] = backupEnum.getNext().QueryInterface(Components.interfaces.nsILocalFile);
		for (i=0; i<dirFiles.length-(NUMBACKUP-1); i++)
			dirFiles[i].remove(false);
		file.create(file.NORMAL_FILE_TYPE, 0666);
		expOpml(file,false,true);
	}
}

function getBackupFilename(date)
{
	var year = date.getFullYear();
	var month = date.getMonth() + 1;
	month = (month < 10 ? "0" : "") + month;
	var date1 = date.getDate();
	date1 = (date1 < 10 ? "0" : "") + date1;
	var filename = "newsfox-"+year+"-"+month+"-"+date1+".opml";
	return filename;
}

////////////////////////////////////////////////////////////////
// Import OPML
////////////////////////////////////////////////////////////////

function restoreBackup()
{
	importOpml(true,true,false);
}

function importOpml(frBackup,fromFile,removeDir)
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var wintitle = document.getElementById("importopml").getAttribute("label");
	fromBackup = frBackup;

	if (!fromFile) // from URL
	{
		var prmpt = NF_SB.getString('opmlhttpimportprompt');
		var url = window.prompt(prmpt,"http://",wintitle);
		if (url) impOpml(url,fromBackup,removeDir);
		return;
	}
  var picker = Components.classes["@mozilla.org/filepicker;1"].
    createInstance(Components.interfaces.nsIFilePicker);
	if (fromBackup)
	{
		var backupDir = NFgetProfileDir();
		backupDir.append(BACKUPS);
		picker.displayDirectory = backupDir;
	}
  picker.init(window, wintitle, picker.modeOpen);
	var opmlFiles = NF_SB.getString('opmlfilesfilter');
  picker.appendFilter(opmlFiles, "*.xml;*.opml");
  picker.appendFilters(picker.filterAll);

  if(picker.show() == picker.returnOK)
	{
		var nsIPH = Components.classes["@mozilla.org/network/protocol;1?name=file"].createInstance(Components.interfaces.nsIFileProtocolHandler);
		var url = nsIPH.getURLSpecFromFile(picker.file);
		impOpml(url,fromBackup,removeDir);
	}
}

function makeNewModels()
{
  var arttree = document.getElementById("newsfox.articleTree");
	gCollect = new EmptyCollection();
	arttree.view = null;  
	arttree.view = new ArticleTreeModel();
	gCollect = new EmptyCollection();
  var file = NFgetProfileDir();
  file.append(MASTER+".xml");
	if (file.exists()) file.remove(false);
  file = NFgetProfileDir();
  file.append(MASTER_INDEX+".xml");
	if (file.exists()) file.remove(false);
  file = NFgetProfileDir();
  file.append(MASTER_GROUP+".xml");
	if (file.exists()) file.remove(false);
	gFmodel = new FdModel();
	gFdGroup = new Array();
	gIdx = new Indexes();
	loadModels(false);
}

function impOpml(url,fromBackup,removeDir)
{ 
	try 
	{
		var feedtree = document.getElementById("newsfox.feedTree");
		fRow = feedtree.treeBoxObject.getFirstVisibleRow();
		feedtree.view.selection.select(-1);
		if (removeDir)
		{
			if (null != gLoadingTimeout) clearTimeout(gLoadingTimeout);
			showAbout();
			for (var i=gFmodel.size()-1; i>=0; i--)
				deleteFeedFromDisk(gFmodel.get(i));
			fRow = 0;
			feedtree.view = null;
			makeNewModels();
			feedtree.view = new FeedTreeModel();
		}

		if (fromBackup) makeNewModels();

		var fix = Components.classes['@mozilla.org/docshell/urifixup;1'].getService(Components.interfaces.nsIURIFixup);
		url = fix.createFixupURI(url, fix.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP);

    reqOpml = new XMLHttpRequest();
    reqOpml.open("GET", url.spec, true);
    reqOpml.overrideMimeType("application/xml");
    reqOpml.onload = function() { finishImpOpml(reqOpml); }
    reqOpml.send(null);
	}
	catch (err)
	{
		alert("Import failed: " + err);
	}
}

function finishImpOpml(reqOpml)
{
    opmlStack = reqOpml.responseXML.getElementsByTagName("outline");
		var feedtree = document.getElementById("newsfox.feedTree");
		expGrp0 = false;
		if (gFdGroup[0].expanded == true)
		{
			expGrp0 = true;
			feedtree.view.toggleOpenState(0);
		}
		feedtree.view = null;

		setPmeter(10);
		loadingTooltip(true);
		// to allow time for pmeter and tooltip to show
		setTimeout(makeFeedStack,100);
}

function stackElement(url,index)
{
	this.url = url;
	this.index = index;
	this.nfIndex = null;
}

function makeFeedStack()
{
	// construct an array of unique feeds - opmlFeedStack
	// add feednumber as newsfox property to opmlStack
	var urlABC = function(a,b)
	{
		if (b.url < a.url) return 1;
		else if (b.url > a.url) return -1;
		else return (b.index < a.index) ? 1 : -1;
	}

	var newsfox123 = function(a,b)
	{
		return a.getAttribute("newsfox") - b.getAttribute("newsfox");
	}

	opmlFeedStack = new Array();  // global
	var newStack = new Array();
	var opmlFeedStackIndex = new Array();
	var newModelStack = new Array();

	for (var i=0; i<opmlStack.length; i++)
	{
		var url = opmlStack[i].getAttribute("xmlUrl");
		if (url != null) newStack.push(new stackElement(url,i));
		setPmeter(10 + (40*i)/opmlStack.length);
	}
	newStack.sort(urlABC);

	var maxModel = gFmodel.size();
	if (maxModel > 0)
	{
		for (i=0; i<maxModel; i++)
			newModelStack.push(new stackElement(gFmodel.get(i).url,i));
		newModelStack.sort(urlABC);
	}

	var modelIndex = 0;
	var newIndex = maxModel;
	var nfIndex;
	for (i=0; i<newStack.length; i++)
	{
		while (modelIndex < maxModel 
				&& newStack[i].url > newModelStack[modelIndex].url)
			modelIndex++;
		if (modelIndex < maxModel 
				&& newStack[i].url == newModelStack[modelIndex].url)
			nfIndex = newModelStack[modelIndex].index;
		else
		{
			opmlFeedStackIndex.push(i);
			nfIndex = newIndex++;
		}

		var j = i;
		while (j<newStack.length && newStack[j].url == newStack[i].url)
			newStack[j++].nfIndex = nfIndex;
		i = j-1;
	}

	for (i=0; i<newStack.length; i++)
//?		if (newStack[i].index < opmlStack.length)
			opmlStack[newStack[i].index].setAttribute("newsfox", newStack[i].nfIndex);

	for (i=0; i<opmlFeedStackIndex.length; i++)
		opmlFeedStack.push(opmlStack[newStack[opmlFeedStackIndex[i]].index]);

	opmlFeedStack.sort(newsfox123);
	setTimeout(addOpmlFeeds,100);
}

function addOpmlFeeds()
{
	numNewFeed = opmlFeedStack.length;
	for (var i=0; i<numNewFeed; i++)
	{
		setPmeter(50 + (40*i)/numNewFeed);
		var kid = opmlFeedStack[i];
		var url = kid.getAttribute("xmlUrl");
		var feed = createNewFeed(gFmodel, url, false, false, true);
		var uid = kid.getAttribute("NFuid");
		var ktext = kid.getAttribute("text");
		if (ktext != null) feed.customName = ktext;
		var khome = kid.getAttribute("htmlUrl");
		if (khome != null) feed.homepage = khome;
		if (uid != null)
		{
			feed.deleteOldStyle = kid.getAttribute("NFdeleteOldStyle");;
			feed.autoCheck = false;
			if (kid.getAttribute("NFautoCheck") == "true")
				feed.autoCheck = true;
			feed.style = kid.getAttribute("NFstyle");
			feed.storage = false;
			if (kid.getAttribute("NFstorage") == "true")
			{
				feed.storage = true;
				feed.icon.src = ICON_STORAGE;
			}
			feed.lastUpdate = new Date();
			var lastUpdate = kid.getAttribute("NFlastUpdate");
			if (lastUpdate) feed.lastUpdate.setTime(Date.parse(lastUpdate));
			else feed.lastUpdate = null;
			feed.autoRefreshInterval = kid.getAttribute("NFautoRefreshInterval");
			if (!feed.autoRefreshInterval) feed.autoRefreshInterval = 0;
		}
		if (fromBackup && uid != null)
		{
		// has to be unique if from an *unmodified* Newsfox file
			var uniq = true;
			for (var j=0; j<gFmodel.size(); j++)
				if (uid == gFmodel.get(j).uid) uniq = false;
			if (uniq) feed.uid = feed.defaultName = uid;
		}
	}
	setTimeout(addOpmlGroups,100);
}

function addOpmlGroups()
{
	try
	{
		var grpKids, feedNumber, grp, num;
		var numNewGrp = 0;
		for (var i=0; i<opmlStack.length; i++)
		{
			var grptype = opmlStack[i].getAttribute("type");
			if (grptype == "rss") continue;
			grp = new FeedGroup();
			if (grptype == "NFgroup")
				grp.search = false;
			else if (grptype == "NFsearch")
			{
				grp.search = true;
				grp.srchdat.flagged = opmlStack[i].getAttribute("NFflagged");
				grp.srchdat.unread = opmlStack[i].getAttribute("NFunread");
				grp.srchdat.text = opmlStack[i].getAttribute("NFtext");
				grp.srchdat.textflags = opmlStack[i].getAttribute("NFtextflags");
				grp.srchdat.startTime = opmlStack[i].getAttribute("NFstartTime");
				grp.srchdat.endTime = opmlStack[i].getAttribute("NFendTime");
			}
			else if (grptype == "NFtag")
			{
				grp.search = true;
				grp.searchTag = opmlStack[i].getAttribute("NFsearchTag");
			}
			grp.title = opmlStack[i].getAttribute("text");
			grpKids = opmlStack[i].getElementsByTagName("outline");
			for (var j=0; j<grpKids.length; j++)
			{
				feedNumber = grpKids[j].getAttribute("newsfox");
        if (feedNumber == null) continue;
				grp.list.push(feedNumber);
			}
			var isNew = true;
			if (!grp.search)
			{
				if (grp.title == gFdGroup[0].title)
					if (grp.list.length == gFdGroup[0].list.length)
					{
						var g0List = new Array();
						var grpList = new Array();
						for (var k=0; k<grp.list.length; k++)
						{
							grpList[k] = grp.list[k];
							g0List[k] = gFdGroup[0].list[k];
						}
						g0List.sort();
						grpList.sort();
						isNew = false;
						for (var k=0; k<grp.list.length; k++)
							if (grpList[k] != g0List[k]) isNew = true;
					// order group 0 as it was
						if (!isNew)
							for (k=0; k<grp.list.length; k++)
								gFdGroup[0].list[k] = grp.list[k];
					}
				for (j=1; j<gFdGroup.length; j++)
				{
					if (!isNew) break;
					if (grp.title == gFdGroup[j].title)
						if (grp.list.length == gFdGroup[j].list.length)
						{
							isNew = false;
							for (var k=0; k<grp.list.length; k++)
								if (grp.list[k] != gFdGroup[j].list[k]) isNew = true;
						}
				}
			}

			if (isNew)
			{
				num = gFdGroup.length;
				gFdGroup.push(grp);
				gIdx.fdgp.push(num);
				gIdx.feed.push(-1);
				gIdx.catg.push(0);
				gIdx.open.push(false);
				numNewGrp++;
			}
		}
		var feedtree = document.getElementById("newsfox.feedTree");
		feedtree.view = new FeedTreeModel();
		if (expGrp0) feedtree.view.toggleOpenState(0);
		if (fromBackup)
		{
			feedtree.view.selection.select(0);
			markFlaggedUnread(false,false);
		}
		else
		{
			feedtree.view.selection.select(-1);
    	if (fRow) feedtree.treeBoxObject.scrollToRow(fRow);
		}
		saveModels();
		setPmeter(0);
		loadingTooltip(false);

		const NF_SB = document.getElementById("newsfox-string-bundle");
		if (opmlStack.length == 0)
		{
			var rptfail = NF_SB.getString('opmlfail');
			alert(rptfail);
		}
		else
		{
			var rpt = NF_SB.getString('opmlimport');
			var rptnfeed = NF_SB.getString('opmlnewfeeds');
			var rptngroup = NF_SB.getString('opmlnewgroups');
      alert(rpt + "\n\n" + numNewFeed + " " + rptnfeed + "\n" + numNewGrp + " " + rptngroup);
		}
	}
	catch (err)
	{
		alert("Import failed: " + err);
	}
}

////////////////////////////////////////////////////////////////
// Export RSS
////////////////////////////////////////////////////////////////

function exportRSS()
{
  var picker = Components.classes["@mozilla.org/filepicker;1"].
    createInstance(Components.interfaces.nsIFilePicker);
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var wintitle = document.getElementById("exportrss").getAttribute("label");
  picker.init(window, wintitle, picker.modeSave);
	var xmlFiles = NF_SB.getString('xmlfilesfilter');
  picker.appendFilter(xmlFiles, "*.xml");
  picker.appendFilters(picker.filterAll);
  picker.defaultString = "rss.xml";
  
  var result = picker.show();
  if(result == picker.returnOK || result == picker.returnReplace)
	{
		var file = picker.file;
		if(file.exists()) file.remove(true);
		file.create(file.NORMAL_FILE_TYPE, 0666);

		var ostream = openOutputStream(file);
    var data = "";
		data += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		data += "<rss version=\"2.0\">\n\t<channel>\n";
		var title = "<![CDATA[Newsfox: " + toUTF8(gCollect.getTitle()) + "]]>";
		data += "\t\t<title>" + title + "</title>\n";
		data += "\t\t<link>" + getFileSpec(file) + "</link>\n";
		data += "\t\t<description>" + title + "</description>\n";
		var now = new Date().toUTCString();
		data += "\t\t<lastBuildDate>" + now + "</lastBuildDate>\n";
		data += "\t\t<generator>Newsfox " + VERSION + "</generator>\n";
		for (var i=0; i<gCollect.size(); i++)
		{
			var art = gCollect.get(i);
			data += "\t\t<item>\n";
			data += "\t\t\t<title><![CDATA[" + toUTF8(art.title) + "]]></title>\n";
			data += "\t\t\t<link><![CDATA[" + art.link + "]]></link>\n";
			if (art.id)
				data += "\t\t\t<guid isPermaLink=\"false\"><![CDATA[" + art.id + "]]></guid>\n";
			data += "\t\t\t<pubDate>" + art.date.toUTCString() + "</pubDate>\n";
			if (art.category != "" && NFgetPref("exportRSS.category","bool",true))
				data += "\t\t\t<category><![CDATA[" + toUTF8(art.category) + "]]></category>\n";
			if (art.tag != "" && NFgetPref("exportRSS.tag","bool",true))
				data += "\t\t\t<category><![CDATA[" + toUTF8(art.tag) + "]]></category>\n";
			var feed = gCollect.getFeed(i);
			if (NFgetPref("exportRSS.source","bool",true))
			{
				if (feed.url.substring(0,5) == "http:")
					data += "\t\t\t<source url=\"" + encodeHTML(feed.url) + "\"><![CDATA[" + toUTF8(feed.getDisplayName()) + "]]></source>\n";
				else if (feed.storage)
					data += "\t\t\t<source url=\"" + encodeHTML(art.source.url) + "\"><![CDATA[" + toUTF8(art.source.name) + "]]></source>\n";
			}
			data += "\t\t\t<description><![CDATA[" + toUTF8(art.body) + "]]></description>\n";
			if (NFgetPref("exportRSS.enclosure","bool",true))
			{
				for (var j=0; j<art.enclosures.length; j++)
				{
					var enc = art.enclosures[j];
					data += "\t\t\t<enclosure url=\"" + encodeHTML(enc.url) + "\" type=\"" + encodeHTML(enc.type) + "\" length=\"" + encodeHTML(enc.length) + "\"/>\n";
				}
			}
			data += "\t\t</item>\n";
		}
		data += "\t</channel>\n</rss>\n";

		try
		{
			if (hasNetUtil)
			{
				istream = setInputStream(data);
				NetUtil.asyncCopy(istream, ostream, function(err) {
			  	if (!Components.isSuccessCode(err)) {
						alert("exportRSS(): " + err);
			  	}
				})
			}
			else
			{
				ostream.write(data, data.length);
				ostream.close();
			}
		}
  	catch (err) { alert("exportRSS(): " + err); }
	}
}
