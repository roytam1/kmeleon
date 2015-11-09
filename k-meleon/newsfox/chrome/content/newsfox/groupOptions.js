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
 * Ron Pruitt <wa84it@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2006-2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

const TIMEVAL = new Array();
TIMEVAL[0] = 3600000;  // msec per hour
TIMEVAL[1] = 24*TIMEVAL[0];  // msec per day
TIMEVAL[2] = 7*TIMEVAL[1];   // msec per week
TIMEVAL[4] = 365*TIMEVAL[1];  // msec per year
TIMEVAL[3] = TIMEVAL[4]/12;   // msec per month

var grp;
var feeds = new Array();
var titlelist = new Array();
var lists = new Array();
var isEmpty = false;
var addhere;
var fdGp = new Array();
var startTime;
var endTime;
var canChangeName = false;

var dragGrp;
var dragRow;

function goinit()
{
	if (window.arguments[0].isSearch && window.arguments[0].isNew)
		canChangeName = true;
	const NF_SB = document.getElementById("newsfox-string-bundle");
	fdGp = window.arguments[0].fdGp;
	if (window.arguments[0].isSearch)
	{
		document.getElementById("showUnread").checked = window.arguments[0].showUnread;
		var textflags = window.arguments[0].textflags;
		document.getElementById("radioFlagged").selectedIndex = window.arguments[0].flagged;
		document.getElementById("radioUnread").selectedIndex = window.arguments[0].unread;
		document.getElementById("Srchtxt").value = window.arguments[0].text;

		document.getElementById("radioCase").selectedIndex = (textflags & 0x04)/4;
		document.getElementById("matchBody").checked = textflags & 0x01;
		document.getElementById("matchTitle").checked = textflags & 0x02;
		document.getElementById("matchTag").checked = textflags & 0x08;
		startTime = window.arguments[0].startTime;
		endTime = window.arguments[0].endTime;

		var startEven = new Array();
		var endEven = new Array();
		for (var i=0; i<TIMEVAL.length; i++)
		{
			startEven[i] = (startTime < 0) || (startTime/TIMEVAL[i] == Math.round(startTime/TIMEVAL[i]));
			endEven[i] = (endTime/TIMEVAL[i] == Math.round(endTime/TIMEVAL[i]));
		}
		var index = 0;
		for (i=0; i<TIMEVAL.length; i++)
			if (startEven[i] && endEven[i]) index = i;

		if (startTime < 0)
		{
			document.getElementById("datemenuStart").selectedIndex = TIMEVAL.length;
			document.getElementById("startT").disabled = true;
		}
		else
		{
			document.getElementById("datemenuStart").selectedIndex = index;
			document.getElementById("startT").value = roundIt(startTime / TIMEVAL[index]);
		}
		document.getElementById("datemenuEnd").selectedIndex = index;
		document.getElementById("endT").value = roundIt(window.arguments[0].endTime / TIMEVAL[index]);
		document.getElementById("part3").hidden = true;
	}
	else
		document.getElementById("part2").hidden = true;

  addhere = NF_SB.getString('addhere');
  grp = window.arguments[0].grp;
  titlelist = window.arguments[0].titlelist;
  lists = window.arguments[0].lists;
  feeds = window.arguments[0].feeds;
  var grpmovemenu = document.getElementById("grpmove");
	grpmovemenu.removeAllItems();
  for (var i=1; i<titlelist.length; i++)
    if (i != grp)
      grpmovemenu.appendItem(titlelist[i],i);
  var LASTname = NF_SB.getString('LASTname');
  grpmovemenu.appendItem(LASTname,titlelist.length);
  grpmovemenu.selectedIndex = grp - 1;

	if (window.arguments[0].isSearch)
	{
		var sIndex = findSimilarIndex();
	  var grpMenu = document.getElementById("grpmenu");
	  while (grpMenu.firstItem != null) 
			grpMenu.removeItem(grpMenu.firstItem);
		var namelength = titlelist.length;
		if (window.arguments[0].isNew) namelength--;
		var menuItems = new Array();
	  for (var i=0; i<namelength; i++)
			menuItems[i] = grpMenu.appendItem(titlelist[i],i);
		grpMenu.appendItem("---",namelength);
		if (sIndex <  0)
			menuItems[-sIndex].setAttribute("style","color: red");
		fixMenuList2();
	}

  if (grp == 0)
  {
    grpmovemenu.value = 1;
    grpmovemenu.hidden = true;
    var grpmovespacer = document.getElementById("grpmovespacer");
    grpmovespacer.hidden = true;
    var grpmovelabel = document.getElementById("grpmovelabel");
    grpmovelabel.hidden = true;
    var grpfeeds = document.getElementById("group.feeds");
    grpfeeds.hidden = true;
    var grpfeedsspacer = document.getElementById("group.feedsspacer");
    grpfeedsspacer.hidden = true;
    var dragrtl = document.getElementById("dragrtl");
    dragrtl.hidden = true;
    var dragltr = document.getElementById("dragltr");
    dragltr.hidden = true;
  }
  document.getElementById("Name").value = titlelist[grp];
  var grpTitle = NF_SB.getString('grpName');
  var leftname = document.getElementById("feedsLeft");
  leftname.setAttribute("label",grpTitle + titlelist[grp]);
  var rightname = document.getElementById("feedsRight");
  rightname.setAttribute("label",grpTitle + titlelist[0]);
  var lefttree = document.getElementById("group.edit");
  lefttree.view = new GrpEditTreeModel(1);
  var righttree = document.getElementById("group.feeds");
  righttree.view = new GrpEditTreeModel(0);

	sizeToContent();
	if (window.arguments[0].isSearch)
	{
		var textBox = document.getElementById("Srchtxt");
		textBox.focus();
		textBox.select();
	}
}

function doAccept()
{
	if (window.arguments[0].isSearch)
	{
		window.arguments[0].showUnread = document.getElementById("showUnread").checked;
		window.arguments[0].flagged = document.getElementById("radioFlagged").selectedIndex;
		window.arguments[0].unread = document.getElementById("radioUnread").selectedIndex;
		window.arguments[0].text = document.getElementById("Srchtxt").value;
		window.arguments[0].textflags =
			1*(document.getElementById("matchBody").checked == true) + 
			2*(document.getElementById("matchTitle").checked == true) + 
			4*document.getElementById("radioCase").selectedIndex + 
			8*(document.getElementById("matchTag").checked == true);

		var index = document.getElementById("datemenuEnd").selectedIndex;
		var endBox = document.getElementById("endT");
		endTime = endBox.value * TIMEVAL[index];
		var index = document.getElementById("datemenuStart").selectedIndex;
		if (index != TIMEVAL.length)
		{
			var startBox = document.getElementById("startT");
			startTime = startBox.value * TIMEVAL[index];
			if (startTime < endTime)
			{
				var i = startTime;
				startTime = endTime;
				endTime = i;
			}
		}
		window.arguments[0].startTime = startTime;
		window.arguments[0].endTime = endTime;
	}

  window.arguments[0].ok = true;
  titlelist[grp] = document.getElementById("Name").value;
  window.arguments[0].newGrp = parseInt(document.getElementById("grpmove").value);
  if (lists[1][0] == -1) lists[1].splice(0,1);
  if (lists[0][0] == -1) lists[0].splice(0,1);
  window.arguments[0].titlelist = titlelist;
  window.arguments[0].lists = lists;
  return true;
}

function srchtxtChange()
{
	if (canChangeName)
	{
		document.getElementById("Name").value 
				= document.getElementById("Srchtxt").value;
		canChangeName = false;
	}
}

function startChg()
{
	var index = document.getElementById("datemenuStart").selectedIndex;
	var startBox = document.getElementById("startT");
	if (index == TIMEVAL.length)
	{
		startTime = -1;
		startBox.value = "";
		startBox.disabled = true;
	}
	else
	{
		if (startTime < 0) startTime = 10 * TIMEVAL[TIMEVAL.length-1];
		startBox.value = roundIt(startTime / TIMEVAL[index]);
		startBox.disabled = false;
	}
}

function endChg()
{
	var index = document.getElementById("datemenuEnd").selectedIndex;
	document.getElementById("endT").value = roundIt(endTime / TIMEVAL[index]);
}

function startBox()
{
	var index = document.getElementById("datemenuStart").selectedIndex;
	var startBox = document.getElementById("startT");
	startTime = startBox.value * TIMEVAL[index];
	startBox.value = roundIt(startBox.value);
}

function endBox()
{
	var index = document.getElementById("datemenuEnd").selectedIndex;
	var endBox = document.getElementById("endT");
	endTime = endBox.value * TIMEVAL[index];
	endBox.value = roundIt(endBox.value);
}

function roundIt(x)
{
	return Math.round(x*1000)/1000;
}

function findSimilarIndex()
{
	if (lists[1].length == 0) return 0;
	var tmpList = new Array();
	for (var i=0; i<lists[1].length; i++)
		tmpList[i] = lists[1][i];
	var toMatch = "," + tmpList.sort().join() + ",";
	var toMatchLen = tmpList.length;
	var minVal = diffVal(toMatch,toMatchLen,0);
	var minIndex = 0;
	var i = 1;
	var tmp;
	while (minVal > 0 && i < fdGp.length)
	{
		if (!fdGp[i].search && fdGp[i].list.length > 0)
		{
			tmp = diffVal(toMatch,toMatchLen,i);
			if (tmp < minVal)
			{
				minVal = tmp;
				minIndex = i;
			}
		}
		i++;
	}
	if (minVal > 0) minIndex = -minIndex;
	return minIndex;
}

function diffVal(toMatch,toMatchLen,index)
{
	var numnew = 0;
	for (var i=0; i<fdGp[index].list.length; i++)
		numnew += 1*(toMatch.indexOf(","+fdGp[index].list[i]+",") == -1)
	var numold = toMatchLen - (fdGp[index].list.length - numnew);
	return (numnew + 2*numold);
}

function fixMenuList()
{
	if (window.arguments[0].isSearch) setTimeout(fixMenuList2,500);
}

function fixMenuList2()
{
	var sIndex = findSimilarIndex();
	var grpMenu = document.getElementById("grpmenu");
	if (sIndex <  0)
		grpMenu.selectedIndex = titlelist.length;
	else
		grpMenu.selectedIndex = sIndex;
	if (sIndex != 0)
		document.getElementById("part3").removeAttribute("hidden");
	document.getElementById("group.edit").setAttribute("height","200");
	document.getElementById("group.feeds").setAttribute("height","200");
}

function fixGroupEdit()
{
	var grpMenu = document.getElementById("grpmenu");
	var grpC = grpMenu.selectedIndex;
	if (findSimilarIndex() == grpC || grpC == titlelist.length) return;
	lists[1] = new Array();
  for (var i=0; i<fdGp[grpC].list.length; i++)
    lists[1].push(fdGp[grpC].list[i]);
  var lefttree = document.getElementById("group.edit");
  lefttree.view = new GrpEditTreeModel(1);
	if (grpC == 0)
		document.getElementById("part3").hidden = true;
	else
		document.getElementById("part3").removeAttribute("hidden");
	sizeToContent();
}

function delFd(evt) { if (grp != 0) delRow(getRow(evt)); }

function delRow(row)
{
	lists[1].splice(row,1);
	lefttree = document.getElementById("group.edit");
	lefttree.view = new GrpEditTreeModel(1);
}

function addFd(evt) { addRow(1,lists[1].length,0,getRow(evt)); }

function addRow(newGrp, newrow, oldGrp, oldrow)
{
	if (isEmpty)
	{
		lists[newGrp].splice(0,1,lists[oldGrp][oldrow]);
		isEmpty = false;
	}
	else
	{
		var totdel = 0;
		var numdel = 0;  // should always be 1 or 0, but check
		var feedtoadd = lists[oldGrp][oldrow];
		var i=lists[newGrp].length;
		while (--i >= 0)
			if (feedtoadd == lists[newGrp][i])
			{
				totdel++;
				if (i < newrow) numdel++;
				lists[newGrp].splice(i,1);
			}
		lists[newGrp].splice(newrow-numdel,0,lists[oldGrp][oldrow]);
	}
//	this.treebox.rowCountChanged(newrow,1);
//	this.treebox.invalidate();
//	this.treebox.rowCountChanged(0,lists[newGrp].length);
//TODO	this.treebox.parentNode.view = new GrpEditTreeModel(newGrp);
	lefttree = document.getElementById("group.edit");
	lefttree.view = new GrpEditTreeModel(1);
}

function getRow(evt)
{
	var row = {}, col = {}, type = {};
	var tree = evt.target.parentNode;
	tree.treeBoxObject.getCellAt(evt.clientX, evt.clientY, row, col, type);
	return row.value;
}

////////////////////////////////////////////////////////////////
// EditorTreeModel
////////////////////////////////////////////////////////////////

function startDrag(evt)
{
// Returns "WINNT" on Windows Vista, XP, 2000, and NT systems;  
// "Linux" on GNU/Linux; and "Darwin" on Mac OS X.  
	var osString = Components.classes["@mozilla.org/xre/app-info;1"]  
               .getService(Components.interfaces.nsIXULRuntime).OS;
	if (osString == "Linux" && gOptions.linuxNoDragDrop) return;
	var row = {}, col = {}, type = {};
	var tree = evt.target.parentNode;
	tree.treeBoxObject.getCellAt(evt.clientX, evt.clientY, row, col, type); 
	dragGrp = 1*(tree.id == "group.edit");
	dragRow = row.value;
	evt.dataTransfer.setData("newsfox/grpoptrow", "");
	evt.dataTransfer.effectAllowed = "copyLink";
}

function GrpEditTreeModel(index)
{
  if (lists[index].length == 0)
  {
    isEmpty = true;
    lists[index].push(-1);
  }
  this.rowCount = lists[index].length,
  this.getCellText = function(row,col)
  {
    if (lists[index][row] == -1) return addhere  
    else return feeds[lists[index][row]].getDisplayName(); 
  },
  this.getCellValue = function(row,col) {},
  this.getImageSrc = function(row,col)
  { 
    if (lists[index][row] == -1) return null
    else return feeds[lists[index][row]].icon.src; 
  },
  this.setTree = function(treebox){ this.treebox = treebox; },
  this.isContainer = function(row){ return false; },
  this.isSeparator = function(row){ return false; },
  this.isSorted = function(){ return false; },
  this.getLevel = function(row){ return 0; },
  this.cycleHeader = function(col){},
  this.getRowProperties = function(row,props){},
  this.getCellProperties = function(row,col,props)
  {
		var returnProps = "";
    var aserv = Components.classes["@mozilla.org/atom-service;1"].
      getService(Components.interfaces.nsIAtomService);
		if (props)
    	props.AppendElement(aserv.getAtom("faviconcol"));
		return (returnProps + " " + "faviconcol");

  },
  this.getColumnProperties = function(colid,col,props){},
  this.canDrop = function(evt,session) { return true; },
  this.getParentIndex = function(rowindex) { return -1; },
  this.drop = function(row,orientation)
  {
		var oldGrp = dragGrp;
		var oldrow = dragRow;
    var newGrp = index;
    var newrow = row + (orientation == 1);
    if (oldGrp == newGrp)  // move position
    {
      var up = 1*(newrow > oldrow);
      tmp = lists[oldGrp].splice(oldrow,1);
      lists[oldGrp].splice(newrow-up,0,tmp);
      (up == 1) ? this.treebox.invalidateRange(oldrow,newrow)
		: this.treebox.invalidateRange(newrow,oldrow);
    }
    else if (newGrp == 0)  // delete from group
			delRow(oldrow);
    else // oldGrp == 0, add to group
			addRow(newGrp, newrow, oldGrp, oldrow);
  } 
}
