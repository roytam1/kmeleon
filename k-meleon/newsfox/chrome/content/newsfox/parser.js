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
 * The Original Code is Newsfox.
 *
 * The Initial Developer of the Original Code is
 * Ron Pruitt <wa84it@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2007-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Andy Frank <andy@andyfrank.com>
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

const YRS10 = 1000*60*60*24*365.25*10;  // close enough

const FEED_NAME = [ null, "rss", "feed", "feed" ];
var NS = [ "http://purl.org/rss/1.0/", null, "http://purl.org/atom/ns#", "http://www.w3.org/2005/Atom" ];
const XHTML = "http://www.w3.org/1999/xhtml";
const XHTML_TRANS_DOCTYPE = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">';
const XHTML_STRICT_DOCTYPE = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">';
const DC = "http://purl.org/dc/elements/1.1/";
const CONTENT = "http://purl.org/rss/1.0/modules/content/";
const MEDIA = "http://search.yahoo.com/mrss/";
const RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const CHANNEL_NAME = [ "channel", "channel", "feed", "feed" ];
const ENTRY_NAME = [ "item", "item", "entry", "entry" ];
const ID_NAME = [ "guid", "guid", "id", "id" ];
const CONTENT_NAME = [ "description", "description", "content", "content" ];
const DATE_NAME = [ "date", "pubDate", "issued", "updated" ];
const DATE_NAME2 = [ "", "", "modified", "published" ];
const CATEGORY_NAME = [ "subject", "category", "category", "category" ];
const FEED_AUTHOR = [ "", "managingEditor", "author", "author" ];
const ITEM_AUTHOR = [ "", "author", "author", "author" ];
const HREF_NAME = [ "url", "url", "href", "href" ];

const TAG_NAME = [ "a", "img", "area" ];
const ATTR_NAME = [ "href", "src", "href" ];

function Parser2(xml,baseUrl)
{
	this.title = null;
	this.link = null;
	this.items = new Array();

// type: 0=RSS1.0, 1=RSS2.0, 2=atom0.3, 3=atom1.0
	this.parse = function(xml,type,baseUrl)
	{
		var channel = xml.getElementsByTagNameNS(NS[type],CHANNEL_NAME[type]);
// BASE
		var baseuri = adjustBase(null,baseUrl);
		baseuri = adjustBase(baseuri,"/");
		baseuri = getBaseURI(channel[0],baseuri,type);
// TITLE
		var title = channel[0].getElementsByTagNameNS(NS[type],"title");
		if (title.length > 0) this.title = getText(title[0]);
// HOMEPAGE
		var uri = getLink(channel[0],baseuri,type);
		if (uri) this.link = uri.resolve("");
// FEED AUTHOR
		var feedAuthor = getAuthor(channel[0],type,true);

// ITEMS:
		var now = new Date();
		var itemContainer = (type == 0) ? xml : channel[0];
		var items = itemContainer.getElementsByTagNameNS(NS[type],ENTRY_NAME[type]);
		for (var i=0; i<items.length; i++)
		{
			var item = new Article();
// ITEM:BASE
			var itembase = null;
			itembase = getBaseURI(items[i],baseuri,type);
// ITEM:TITLE
			title = items[i].getElementsByTagNameNS(NS[type],"title");
			for (var j=0; j<title.length; j++)
				if (title[j].parentNode == items[i]) item.title = getXhtml(fixLinks(title[j],itembase,type),type);
// ITEM:LINK
			var uri = getLink(items[i],itembase,type);
			// need spec instead of resolve to pick up # anchors in link
			if (uri) item.link = uri.spec;
			if (!item.link) item.link = NO_LINK;
// ITEM:ID
			var id = items[i].getElementsByTagNameNS(NS[type],ID_NAME[type]);
			for (var j=0; j<id.length; j++)
				if (id[j].parentNode == items[i]) item.id = getText(id[j]);
			if (!item.id && item.link != NO_LINK) item.id = item.link;
			if (item.id && item.id.substring(0,5) == "http:" && item.link == NO_LINK)
				item.link = (uri) ? uri.resolve(item.id) : item.id;
// ITEM:BODY
			var body = items[i].getElementsByTagNameNS(NS[type],CONTENT_NAME[type]);
			if (body.length > 0)
				item.body = getXhtml(fixLinks(body[0],itembase,type),type);
			if (!item.body && type >= 2)  // atom
			{
				var body = items[i].getElementsByTagNameNS(NS[type],"summary");
				if (body.length > 0)
					item.body = getText(fixLinks(body[0],itembase,type));
			}
			if (type < 2)  // rss
			{
				var body = items[i].getElementsByTagNameNS(CONTENT,"encoded");
				if (body.length > 0) item.body = getText(fixLinks(body[0], itembase,type));
			}
// ITEM:DATE
			item.date = NO_DATE;
			var index = 0;
			var idate = items[i].getElementsByTagNameNS(NS[type],DATE_NAME[type]);
			if (idate.length == 0 && type >= 2)
			{
				idate = items[i].getElementsByTagNameNS(NS[type],DATE_NAME2[type]);
				index = 1;
			}
			if (idate.length == 0)
			{
				idate = items[i].getElementsByTagNameNS(DC,"date");
				index = 2;
			}
			var dateIndex = -1;
			for (var j=0; j<idate.length; j++)
				if (idate[j].parentNode == items[i]) dateIndex = j;
			if (dateIndex != -1)
				if (index == 0 && type == 1)
					item.date = setRFCDate(getText(idate[dateIndex]));
				else
					item.date = setTZDate(getText(idate[dateIndex]));
// date adjustment
			if (!gOptions.dateNoneStrict && item.date < TOP_NO_DATE 
				&& item.date > TOP_INVALID_DATE) item.date = now;
			if (!gOptions.dateInvalidStrict && item.date < TOP_INVALID_DATE 
				&& item.date > TOP_FUTURE_DATE) item.date = now;
			if (item.date - now > 10 * 60 * 1000)   // 10 minutes
			{
				if (gOptions.dateFutureStrict)
					while (item.date >= TOP_FUTURE_DATE)
						item.date = new Date(item.date - YRS10);
				else
					item.date = now;
			}
// ITEM:CATEGORIES
			var cats = items[i].getElementsByTagNameNS(NS[type],CATEGORY_NAME[type]);
			if (cats.length == 0 && type <= 1)
				cats = items[i].getElementsByTagNameNS(DC,"subject");
			var cat = "";
			var newcat;
			for (var j=0; j<cats.length; j++)
			{
				if (type < 2)
					newcat = getText(cats[j]);
				else
					newcat = cats[j].getAttribute("term");
        newcat = newcat.replace(/\//g, "&#047;");
				cat = mergeCats(cat,newcat,null);
			}
			item.category = cat;
// ITEM:ENCLOSURES
			if (type < 2)
			{
				var enc = items[i].getElementsByTagNameNS(NS[type],"enclosure");
				for (var j=0; j<enc.length; j++)
					item.enclosures.push(newEncl(enc[j],HREF_NAME[type]));
			}
			else
			{
				var enc = items[i].getElementsByTagNameNS(NS[type],"link");
				for (var j=0; j<enc.length; j++)
					if (enc[j].hasAttribute("rel") && enc[j].getAttribute("rel") == "enclosure")
						item.enclosures.push(newEncl(enc[j],HREF_NAME[type]));
			}
			var mediaContent = items[i].getElementsByTagNameNS(MEDIA,"content");
			for (var j=0; j<mediaContent.length; j++)
				if (mediaContent[j].hasAttribute("url") && mediaContent[j].getAttribute("url") != "")
					item.enclosures.push(newEncl(mediaContent[j],"url"));
// ITEM:SOURCE
			var source = items[i].getElementsByTagNameNS(NS[type],"source");
			if (source.length > 0)
				if (type < 2)  // RSS
				{
					item.source.name = getText(source[0]);
					if (source[0].hasAttribute("url"))
						item.source.url = source[0].getAttribute("url");
				}
				else
				{
					var titleArray = source[0].getElementsByTagNameNS(NS[type],"title");
					if (titleArray.length > 0) 
							item.source.name = getXhtml(titleArray[0],type);
					else
						item.source.name = "...";
					var uri = getLink(source[0],itembase,type);
					if (uri) item.source.url = uri.spec;
				}

// ITEM:AUTHOR
			item.author = getAuthor(items[i],type,false);
			if (!item.author) item.author = feedAuthor;
			if (!item.author) item.author = "";

			this.items.push(item);
		}
	}

// MAIN
	var root = xml.documentElement.localName.toLowerCase();
	var type = -1;
	var errortype = ERROR_OK;
	for (var i=1; i<4; i++)
		if (xml.getElementsByTagNameNS(NS[i],FEED_NAME[i]).length) type = i;
	if (xml.getElementsByTagNameNS(RDF_NS,"RDF").length)
	{
		type = 0;
		var ns = xml.getElementsByTagNameNS(RDF_NS,"RDF")[0].getAttribute("xmlns");
		if (ns && ns != "null" && ns.length) NS[0] = ns;
	}
	if (type == -1)
		if (root == "parsererror") errortype = ERROR_INVALID_FEED_URL;
		else errortype = ERROR_UNKNOWN_FEED_FORMAT + root;
	if (errortype != ERROR_OK) throw errortype;
	this.parse(xml,type,baseUrl);
}

function getBaseURI(xml,base,type)
{
	var baseuri = base;
	if (xml.hasAttribute("xml:base"))
		baseuri = adjustBase(baseuri,xml.getAttribute("xml:base"));
	baseuri = getAtomSelfLink(xml,baseuri);
	return baseuri;
}

function adjustBase(baseuri,url)
{
	var ioSvc = Components.classes['@mozilla.org/network/io-service;1'].getService(Components.interfaces.nsIIOService);
	try{ return ioSvc.newURI(url,null,baseuri); }
	catch(e){ return null; }
}

function getAtomSelfLink(xml,baseuri)
{
	var newuri = null;
	var url = null;
	var links = xml.getElementsByTagNameNS(NS[3],"link");
	if (links.length == 0) return baseuri;
	for (var i=0; i<links.length; i++)
		if (links[i].parentNode == xml && links[i].hasAttribute("rel") && links[i].getAttribute("rel") == "self")
		{
			url = links[i].getAttribute("href");
			break;
		}
	if (url && url.indexOf("feeds.feedburner.com") > -1) return baseuri;
	if (url) return adjustBase(baseuri,url)
	else return baseuri;
}

function getLink(xml,baseuri,type)
{
	var newuri = null;
	var url = null;
	var links = xml.getElementsByTagNameNS(NS[type],"link");
	if (links.length == 0) return url;
	if (type < 2)  // RSS
	{
		for (var i=0; i<links.length; i++)
			if (links[i].parentNode == xml)
			{
				url = getText(links[i]);
				break;
			}
	}
	else
	{
		for (var i=0; i<links.length; i++)
			if (links[i].parentNode == xml && (!links[i].hasAttribute("rel") || links[i].getAttribute("rel") == "alternate" || links[i].getAttribute("rel") == "http://www.iana.org/assignments/relation/alternate"))
			{
				url = links[i].getAttribute("href");
				break;
			}
	}
	newuri = adjustBase(baseuri,url);
	return newuri;
}

function setRFCDate(rfcDate)
{
	var ndate = new Date(Date.parse(rfcDate));
	if (ndate == "Invalid Date") ndate = rescueRFCDate(rfcDate);
	return ndate;
}

function setTZDate(isoDate)
{
	try
	{
		var dateTime = isoDate.split("T");
		var ymd = dateTime[0].split("-");
		for (var i=ymd.length; i<3; i++) ymd[i] = 1;
		var utc;
		if (dateTime.length > 1)
		{
			var timeSplitter = dateTime[1].match("[Z+-]");
			var timeOffset = dateTime[1].split(timeSplitter);
			var hms = timeOffset[0].split(":");
			for (var i=hms.length; i<3; i++) hms[i] = 0;  // hms.length<3 illegal
			utc = Date.UTC(ymd[0],ymd[1]-1,ymd[2],hms[0],hms[1],hms[2]);
			var mult = 0;
			if (timeSplitter == "+") mult = -1;
			else if (timeSplitter == "-") mult = 1;
			if (mult != 0)
			{
				var hm = timeOffset[1].split(":");
				// multiply since hm not integers
				utc = utc + mult*1000*(hm[0]*3600+hm[1]*60);
			}
		}
		else
			utc = Date.UTC(ymd[0],ymd[1]-1,ymd[2],0,0,0);
		var ndate = new Date(utc);
		if (ndate == "Invalid Date" || ymd[0] < 1970 || ymd[1] > 12 || ymd[2] > 31)
			ndate = INVALID_DATE;
		return ndate;
	}
	catch(e)
	{
		return INVALID_DATE;
	}
}

function fixLinks(node, baseuri,type)
{
  if (gOptions.fixyoutube1)
    fixYoutube1(node, baseuri, type);
  if (gOptions.wmode)
    adjustWmode(node, baseuri, type);

	var nType = node.getAttribute("type");
	if (!gOptions.openInViewPane)
	{
		if (nType == "xhtml")
		{
			var kids = node.getElementsByTagNameNS(XHTML,"a");
			for (var j=0; j<kids.length; j++)
				if (kids[j].hasAttribute("href"))
					kids[j].setAttribute("target", "_blank");
		}
		else if (type <= 1 || nType == "html" || nType == "text/html")
		{
      var hText = node.textContent;
			if (hText == "") return node;
// needed to avoid <a\nhref=...
      hText = hText.replace(/\n/g," ");
// hack, seem to need / in fixRelativeLinks, from bug#25459
			hText = hText.replace(/&#x2F;/g,"/");
      node.textContent = makeTarget_Blank(hText);
		}
	}
	baseuri = getBaseURI(node,baseuri,type);
	if (!baseuri || NFgetPref("z.dontFixRelativeLinks", "bool", false))
		return node;
	if (nType == "xhtml")
	{
		for (var i=0; i<TAG_NAME.length; i++)
		{
			var kids = node.getElementsByTagNameNS(XHTML,TAG_NAME[i]);
			for (var j=0; j<kids.length; j++)
				if (kids[j].hasAttribute(ATTR_NAME[i]))
					kids[j].setAttribute(ATTR_NAME[i],baseuri.resolve(kids[j].getAttribute(ATTR_NAME[i])));
		}
	}
	else if (type <= 1 || nType == "html" || nType == "text/html")
	{
		try { var hText = node.textContent; }
		catch(e) { return node; }  // if node is empty and openInViewPane=true 
		node.textContent = fixRelativeLinks(hText,baseuri);
	}
	return node;
}

function makeTarget_Blank(hText)
{
	var index = hText.length;
	var indTarget, indGt, indHref;
	while (index > -1)
	{
		index = hText.toLowerCase().lastIndexOf("<a ",index);
		indTarget = hText.toLowerCase().indexOf("target=",index);
		indGt = hText.indexOf(">",index);
		indHref = hText.toLowerCase().indexOf("href=",index);
		if (indHref != -1 && indHref < indGt && index > -1)
		{
			if (indTarget != -1 && indTarget < indGt)
			{
				var indTargetEnd = hText.substring(indTarget).search(/\s|>/);
				hText = hText.substring(0,indTarget) + " target=\"_blank\" " + hText.substring(indTarget+indTargetEnd);
			}
			else
				hText = hText.substring(0,index+3) + " target=\"_blank\" " + hText.substring(index+3);
		}
		index--;
	}
	return hText;
}

function fixRelativeLinks(hText,baseuri)
{
	var index, hTextLower, indGt, re, oldAttrL, indAttr, oldAttr;
	for (var i=0; i<TAG_NAME.length; i++)
	{
		index = hText.length;
		while (index > -1)
		{
			hTextLower = hText.toLowerCase();
			index = hTextLower.lastIndexOf("<"+TAG_NAME[i]+" ",index);
			indGt = hText.indexOf(">",index);
			try
			{
				switch (i)
				{
					case 0:
					case 2:
						re = /href\s*=\s*(['"])(.*?)\1/;
						break;
					case 1:
						re = /src\s*=\s*(['"])(.*?)\1/;
						break;
				}
// don't know why the following doesn't work
//				re = new RegExp(ATTR_NAME[i]+"\\s*=\\s*(['\"])(.*?)\1","");
				oldAttrL = hTextLower.substring(index).match(re)[2];
				indAttr = hTextLower.indexOf(oldAttrL,index);
				oldAttr = hText.substring(indAttr,indAttr+oldAttrL.length);
			}
			catch(e) { indAttr = null; }
			if (indAttr && indAttr < indGt)
			{
				var newAttr = baseuri.resolve(oldAttr);
				hText = hText.substring(0,indAttr-1) + "\"" + newAttr + "\"" + hText.substring(indAttr+oldAttrL.length+1);
			}
			index--;
		}
	}
	return hText;
}

function getXhtml(node,type)
{
	var nType = node.getAttribute("type");
	if (nType == "xhtml")
	{
		var serializer = new XMLSerializer();
		var xml = "";
	// have to watch out for space before the atom <div>, can only be one <div>
		for (var i=0; i<node.childNodes.length; i++)
			if (node.childNodes[i].localName == "div")
				xml = serializer.serializeToString(node.childNodes[i]);
	// div can't be part of content, need to retain namespaces
		xml = changeDivToSpan(xml);
		return "<xhtml>" + stringTrim(xml) + "</xhtml>";
	}
	else if (type >=2 && (!nType || nType == "text"))
		return encodeHTML(getText(node));
	else
		return getText(node);
}

function changeDivToSpan(xml)
{
	var ind1 = xml.indexOf("<div");
	var ind2 = xml.indexOf(":div");
	var ind3 = xml.lastIndexOf("div>");
	var goodStart = false;
	var goodEnd = false;
	if (xml.length-ind3 == 4) goodEnd = true;
	if (ind1 == 0 || (ind1 == -1 || ind2 < ind1)) goodStart = true;
	if (goodStart && goodEnd)
	{
		if (ind1 == 0) xml = xml.replace("<div","<span");
		else xml = xml.replace(":div",":span");
		xml = xml.replace(/div>$/,"span>");
	}
	return xml;
}

function getText(node)
{
	var result = "";
	var walker = node.ownerDocument.createTreeWalker(node, NodeFilter.SHOW_CDATA_SECTION | NodeFilter.SHOW_TEXT, null, false);
	while(walker.nextNode()) result += walker.currentNode.nodeValue;
	return stringTrim(result);
}

function mergeCats(cat,newcat,rmcat)
{
	var ScatS = "\/";
	if (cat) ScatS += cat + "\/";
	if (newcat)
	{
		var newcatArray = newcat.split("\/");
		for (var i=0; i<newcatArray.length; i++)
		{
			var SnewcatS = "\/" + newcatArray[i] + "\/";
			if (ScatS.indexOf(SnewcatS) == -1) ScatS += SnewcatS;
		}
	}
	if (rmcat)
	{
		var rmcatArray = rmcat.split("\/");
		for (i=0; i<rmcatArray.length; i++)
		{
			var SrmcatS = "\/" + rmcatArray[i] + "\/";
			ScatS = ScatS.replace(SrmcatS,"\/");
		}
	}
	var Back = ScatS;
	while (Back.indexOf("\/\/") > -1) Back = Back.replace(/\/\//g, "\/");
	Back = Back.replace(/^\//, "");
	Back = Back.replace(/\/$/, "");
	var backArray = Back.split("\/");
	backArray.sort();
	Back = backArray.join("\/");
	return Back;
}

function newEncl(enc,hrefname)
{
	var encl = new Enclosure();
	encl.url = enc.getAttribute(hrefname);
	if (enc.hasAttribute("type")) encl.type = enc.getAttribute("type");
	if (enc.hasAttribute("length")) encl.length = enc.getAttribute("length");
	return encl;
}

/**
 * Get a human readable summary of error. (from Andy Frank)
 */
function getErrorSummary(code)
{
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var strOK = NF_SB.getString('feed_ok');
	var strINVALID = NF_SB.getString('feed_invalid');
	var strUNKNOWN = NF_SB.getString('feed_format_unknown');
	var strSERVER = NF_SB.getString('feed_server_error');
	var strNOTFOUND = NF_SB.getString('feed_not_found');
	var strOTHER = NF_SB.getString('feed_other_error');
  switch (code.substring(0,1))
  {
    case ERROR_OK: 
      return strOK;
    case ERROR_INVALID_FEED_URL:
      return strINVALID;
    case ERROR_UNKNOWN_FEED_FORMAT:
      return strUNKNOWN + ": " + code.substring(1);
    case ERROR_SERVER_ERROR:
      return strSERVER;
		case ERROR_NOT_FOUND:
      return strNOTFOUND;
    default: return strOTHER;
  }
} 

/**
 * Get possible remedies for this error. (from Andy Frank)
 */
function getErrorRemedies(code)
{
  // TODO - break out into HTML referenced by ID
	const NF_SB = document.getElementById("newsfox-string-bundle");
	var remedyINVALID = NF_SB.getString('remedy_invalid');
	var remedyUNKNOWN = NF_SB.getString('remedy_format_unknown');
	var remedySERVER = NF_SB.getString('remedy_server_error');
  switch (code.substring(0,1))
  {
    case ERROR_OK:
		case ERROR_NOT_FOUND:
			return "";
    case ERROR_INVALID_FEED_URL:
      return remedyINVALID;
    case ERROR_UNKNOWN_FEED_FORMAT:
      return remedyUNKNOWN;
    case ERROR_SERVER_ERROR:
      return remedySERVER + ":\n\n" + code.substring(1);       
    default: return code;
  }
}

function rescueRFCDate(rfcDate)
{
	try
	{
		var dateArray = rfcDate.split(" ");
		var yr = dateArray[3];
		if (yr.length == 2) yr = yr < 70 ? "20" + yr: "19" + yr;
		dateArray[3] = yr;
	// From Bernhard Schelling bug#17681
		if (dateArray.length == 6 && isNaN(dateArray[5]))
		{
			var timeZone = String(dateArray[5]).toUpperCase();
			if      (timeZone == 'ACDT') { dateArray[5] = '+1030'; }
			else if (timeZone == 'ACST') { dateArray[5] = '+0930'; }
			else if (timeZone == 'ADT')  { dateArray[5] = '-0300'; }
			else if (timeZone == 'AEDT') { dateArray[5] = '+1100'; }
			else if (timeZone == 'AEST') { dateArray[5] = '+1000'; }
			else if (timeZone == 'AHST') { dateArray[5] = '-1000'; }
			else if (timeZone == 'AKDT') { dateArray[5] = '-0800'; }
			else if (timeZone == 'AKST') { dateArray[5] = '-0900'; }
			else if (timeZone == 'AST')  { dateArray[5] = '-0400'; }
			else if (timeZone == 'AT')   { dateArray[5] = '-0200'; }
			else if (timeZone == 'AWDT') { dateArray[5] = '+0900'; }
			else if (timeZone == 'AWST') { dateArray[5] = '+0800'; }
			else if (timeZone == 'BST')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'BT')   { dateArray[5] = '+0300'; }
			else if (timeZone == 'CAT')  { dateArray[5] = '-1000'; }
			else if (timeZone == 'CCT')  { dateArray[5] = '+0800'; }
			else if (timeZone == 'CEDT') { dateArray[5] = '+0200'; }
			else if (timeZone == 'CEST') { dateArray[5] = '+0200'; }
			else if (timeZone == 'CET')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'CXT')  { dateArray[5] = '+0700'; }
			else if (timeZone == 'EADT') { dateArray[5] = '+1100'; }
			else if (timeZone == 'EAST') { dateArray[5] = '+1000'; }
			else if (timeZone == 'EEDT') { dateArray[5] = '+0300'; }
			else if (timeZone == 'EEST') { dateArray[5] = '+0300'; }
			else if (timeZone == 'EET')  { dateArray[5] = '+0200'; }
			else if (timeZone == 'FST')  { dateArray[5] = '+0200'; }
			else if (timeZone == 'FWT')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'GST')  { dateArray[5] = '+1000'; }
			else if (timeZone == 'HAA')  { dateArray[5] = '-0300'; }
			else if (timeZone == 'HAC')  { dateArray[5] = '-0500'; }
			else if (timeZone == 'HADT') { dateArray[5] = '-0900'; }
			else if (timeZone == 'HAE')  { dateArray[5] = '-0400'; }
			else if (timeZone == 'HAP')  { dateArray[5] = '-0700'; }
			else if (timeZone == 'HAR')  { dateArray[5] = '-0600'; }
			else if (timeZone == 'HAST') { dateArray[5] = '-1000'; }
			else if (timeZone == 'HAT')  { dateArray[5] = '-0230'; }
			else if (timeZone == 'HAY')  { dateArray[5] = '-0800'; }
			else if (timeZone == 'HDT')  { dateArray[5] = '-0900'; }
			else if (timeZone == 'HNA')  { dateArray[5] = '-0400'; }
			else if (timeZone == 'HNC')  { dateArray[5] = '-0600'; }
			else if (timeZone == 'HNE')  { dateArray[5] = '-0500'; }
			else if (timeZone == 'HNP')  { dateArray[5] = '-0800'; }
			else if (timeZone == 'HNR')  { dateArray[5] = '-0700'; }
			else if (timeZone == 'HNT')  { dateArray[5] = '-0330'; }
			else if (timeZone == 'HNY')  { dateArray[5] = '-0900'; }
			else if (timeZone == 'HST')  { dateArray[5] = '-1000'; }
			else if (timeZone == 'IDLE') { dateArray[5] = '+1200'; }
			else if (timeZone == 'IDLW') { dateArray[5] = '-1200'; }
			else if (timeZone == 'IST')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'JST')  { dateArray[5] = '+0900'; }
			else if (timeZone == 'MEST') { dateArray[5] = '+0200'; }
			else if (timeZone == 'MESZ') { dateArray[5] = '+0200'; }
			else if (timeZone == 'MET')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'MEWT') { dateArray[5] = '+0100'; }
			else if (timeZone == 'MEZ')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'NDT')  { dateArray[5] = '-0230'; }
			else if (timeZone == 'NFT')  { dateArray[5] = '+1130'; }
			else if (timeZone == 'NST')  { dateArray[5] = '-0330'; }
			else if (timeZone == 'NT')   { dateArray[5] = '-1100'; }
			else if (timeZone == 'NZDT') { dateArray[5] = '+1300'; }
			else if (timeZone == 'NZST') { dateArray[5] = '+1200'; }
			else if (timeZone == 'NZT')  { dateArray[5] = '+1200'; }
			else if (timeZone == 'SST')  { dateArray[5] = '+0200'; }
			else if (timeZone == 'SWT')  { dateArray[5] = '+0100'; }
			else if (timeZone == 'UTC')  { dateArray[5] = '-0000'; }
			else if (timeZone == 'WADT') { dateArray[5] = '+0800'; }
			else if (timeZone == 'WAT')  { dateArray[5] = '-0100'; }
			else if (timeZone == 'WEDT') { dateArray[5] = '+0100'; }
			else if (timeZone == 'WEST') { dateArray[5] = '+0100'; }
			else if (timeZone == 'WET')  { dateArray[5] = '-0000'; }
			else if (timeZone == 'WST')  { dateArray[5] = '+0800'; }
			else if (timeZone == 'YDT')  { dateArray[5] = '-0800'; }
			else if (timeZone == 'YST')  { dateArray[5] = '-0900'; }
			else if (timeZone == 'ZP4')  { dateArray[5] = '+0400'; }
			else if (timeZone == 'ZP5')  { dateArray[5] = '+0500'; }
			else if (timeZone == 'ZP6')  { dateArray[5] = '+0600'; }
			//Support for single letter military time zones
			else if (dateArray[5].length==1 && dateArray[5].match(/[A-I,K-Z]/))
			{
				var i = dateArray[5].charCodeAt(0);
				i = (i==90?0:i<74?i-64:i<78?i-65:77-i);
				dateArray[5] = (i<-9?'-':i<0?'-0':i<10?'+0':'+')+String(i<0?0-i:i)+'00';
			}
	 	}
		var newString = dateArray.join(" ");
		var ndate = new Date(Date.parse(newString));
		if (ndate == "Invalid Date") return INVALID_DATE;
		else return ndate;
	}
	catch(e) { return setTZDate(rfcDate); }
}

function getAuthor(node,type,isFeed)
{
	var authorDisplay = null;
	var author;
	if (isFeed)
		author = node.getElementsByTagNameNS(NS[type],FEED_AUTHOR[type]);
	else
		author = node.getElementsByTagNameNS(NS[type],ITEM_AUTHOR[type]);
	if (author.length > 0)
	{
		if (type > 1) // atom
		{
			var name = author[0].getElementsByTagNameNS(NS[type],"name");
			try { name = name[0].textContent; }
			catch(e) { name = ""; }
			var email = author[0].getElementsByTagNameNS(NS[type],"email");
			try { email = email[0].textContent; }
			catch(e) { email = null; }
			authorDisplay = name;
			if (email) authorDisplay += " (" + email + ")";
		}
	}
	else author = node.getElementsByTagNameNS(DC,"creator");
	if (!authorDisplay && author.length > 0) authorDisplay = getText(author[0]);
	if (authorDisplay == "")
		return null;
	else
		return authorDisplay;
}

function adjustWmode(node, baseuri,type)
{
	var nType = node.getAttribute("type");

	if (nType == "xhtml")
	{
		var kids = node.getElementsByTagNameNS(XHTML,"embed");
		for (var j=0; j<kids.length; j++)
				kids[j].setAttribute("wmode", "opaque");
	}
	else if (type <= 1 || nType == "html" || nType == "text/html")
	{
    var hText = node.textContent;
		if (hText == "") return node;
		node.textContent = makeWmode_Opaque(hText);
	}

	return node;
}

function makeWmode_Opaque(hText)
{
	var index = hText.length;
	var indTarget, indGt, indHref;
	while (index > -1)
	{
		index = hText.toLowerCase().lastIndexOf("<embed",index);
		if (hText.substring(index).search(/\s/) == 6)
		{
			indTarget = hText.toLowerCase().indexOf("wmode=",index);
			indGt = hText.indexOf(">",index);
			indHref = hText.toLowerCase().indexOf("href=",index);
			if (indTarget != -1 && indTarget < indGt)
			{
					var indTargetEnd = hText.substring(indTarget).search(/\s|>/);
					hText = hText.substring(0,indTarget) + " wmode=\"opaque\" " + hText.substring(indTarget+indTargetEnd);
			}
			else if (index != -1)
				hText = hText.substring(0,index+7) + " wmode=\"opaque\" " + hText.substring(index+7);
		}
		index--;
	}
	return hText;
}

function fixYoutube1(node, baseuri, type)
{
	var nType = node.getAttribute("type");

	if (nType == "xhtml")
	{
	}
	else if (type <= 1 || nType == "html" || nType == "text/html")
	{
    var hText = node.textContent;
		var index = hText.length;
		while (index > -1)
		{
			index = hText.lastIndexOf("www.youtube.com\/embed", index);
			if (index > -1)
			{
				var index1 = hText.lastIndexOf("iframe",index);
				var index2 = hText.indexOf("iframe",index+21);
		    hText = hText.substring(0,index1) + "embed" + hText.substring(index1+6,index) + "www.youtube.com\/v" + hText.substring(index+21,index2) + "embed" + hText.substring(index2+6);
	//		hText = hText.substring(0,index) + "www.youtube.com\/v" + hText.substring(index+21);
				node.textContent = hText;
			}
			index--;
		}
	}
}
