const PREF_NONE   = 0x00;
const PREF_STRING = 0x20;
const PREF_INT    = 0x40;
const PREF_BOOL   = 0x80;

const nsISupportsString      = Components.interfaces.nsISupportsString;
const nsIPrefLocalizedString = Components.interfaces.nsIPrefLocalizedString;

var pref = Components.classes["@mozilla.org/preferences-service;1"]
		     .getService(Components.interfaces.nsIPrefBranch);
var sstr = Components.classes["@mozilla.org/supports-string;1"]
                     .createInstance(nsISupportsString);


function getPrefValue(prefID) {
	try {
		switch(pref.getPrefType(prefID)) {
			case PREF_NONE	: return null;
			case PREF_BOOL	: return (pref.getBoolPref(prefID))?"true":"false";
			case PREF_INT	: return pref.getIntPref(prefID).toString(10);
//			case PREF_STRING: return pref.getComplexValue(prefID,nsISupportsString).data;
			case PREF_STRING: try {
						var v = pref.getComplexValue(prefID,nsIPrefLocalizedString).data;
					  } catch(e) {
						    v = pref.getComplexValue(prefID,nsISupportsString).data;
					  }
					  return v;
//			case PREF_STRING: return pref.getCharPref(prefID);
			default         : return "[unknown PrefType: " + pref.getPrefType(prefID) + "]";
		}
	} catch(e) {
		return null;
	}
}
function restoreDefault(prefID) {
	try {
		pref.clearUserPref(prefID);
	} catch(e) {}
}
function setIntPref(prefID,prefVal) {
	if(prefVal) {
		var re = new RegExp("^[0-9]+$");
		if(re.test(prefVal))
			pref.setIntPref(prefID,parseInt(prefVal));
		else {
			alert(document.getElementById("pref_bundle").getString("positive_integer_expected"));
			document.getElementById(prefID).value = getPrefValue(prefID);
			document.getElementById(prefID).focus();
		}
	} else
		restoreDefault(prefID);
}
function setCharPref(prefID,prefVal) {
	sstr.data = prefVal;
	pref.setComplexValue(prefID,nsISupportsString,sstr);
//	pref.setCharPref(prefID,prefVal);
}
function initCheckbox(prefID) {
	var box = document.getElementById(prefID);

	try {
		box.checked = (box.getAttribute("inverse")=="true") ? !pref.getBoolPref(prefID) : pref.getBoolPref(prefID);
	} catch(e) {
		box.disabled = true;
	}
}
function toggleCheckbox(prefID) {
	var val;

	try {
		val = !pref.getBoolPref(prefID);
	} catch(e) {
		var box = document.getElementById(prefID);
		val = (box.getAttribute("inverse")=="true") ? !box.checked : box.checked;
	}
	pref.setBoolPref(prefID,val);
}
function initRadiogroup(prefID) {
	var grp = document.getElementById(prefID),
	    val = getPrefValue(prefID);

	if(val==null)
		try {
			for(grp.selectedIndex=0;grp.selectedItem.value;grp.selectedIndex++)
				grp.selectedItem.disabled = true;
		} catch(e) {}
	else
		try {
			for(grp.selectedIndex=0;grp.selectedItem.value!=val;grp.selectedIndex++);
		} catch(e) {}
}
function toggleRadiogroup(prefID) {
	var grp = document.getElementById(prefID),
	    val = grp.value;

	try {
		switch(pref.getPrefType(prefID)) {
			case PREF_BOOL	: pref.setBoolPref(prefID,(val=="true")?true:false); break;
			case PREF_INT	: val = parseInt(val); if(!isNaN(val)) pref.setIntPref(prefID,val); break;
			case PREF_STRING: pref.setCharPref(prefID,val); break;
		}
	} catch(e) {}
}
function initTextbox(prefID,emptyStr) {
	var box = document.getElementById(prefID),
	    val = getPrefValue(prefID);

	box.removeAttribute("onfocus");
	box.removeAttribute("onblur");
	if(val==null) {
		val = document.getElementById("pref_bundle").getString("pref_not_set");
		if(!box.readonly) {
			box.setAttribute("onfocus","this.value=''");
			box.setAttribute("onblur","initTextbox('"+prefID+"',"+((emptyStr)?("'"+emptyStr+"'"):"false")+")");
		}
		box.value = (emptyStr) ? emptyStr : val;
		return;
	}
	if(!val && emptyStr) {
		if(!box.readonly) {
			box.setAttribute("onfocus","this.value=''");
			box.setAttribute("onblur","initTextbox('"+prefID+"','"+emptyStr+"')");
		}
		box.value = emptyStr;
	} else
		box.value = val;
}
function initMenulist(prefName,prefRoot,invalidNote) {
	var lst = document.getElementById(prefName),
	    val = getPrefValue(prefRoot+prefName),
	    sel = null;

	lst.selectedIndex = 0;
	lst.value = val;
	try {
		sel = lst.selectedItem.value;
	} catch(e) {}
	if(lst.value != sel) {
		lst.firstChild.appendChild(document.createElement("menuseparator"));
		lst.firstChild.appendChild(document.createElement("menuitem"));
		lst.firstChild.lastChild.setAttribute("value",val);
		lst.firstChild.lastChild.setAttribute("label",(val)?((invalidNote)?val+" "+invalidNote:val):document.getElementById("pref_bundle").getString("pref_not_set"));
		lst.firstChild.lastChild.setAttribute("disabled",!val);
		lst.selectedItem = lst.firstChild.lastChild;
	}
}
/* --- String Functions ----------------------------------------------------------------------------------------- */
function alphabetical(x,y) {
	return x.toLowerCase().localeCompare(y.toLowerCase());
}
function makeUTF8(string) {
	setCharPref(kmPrefs.temp,string);
	var ret = pref.getCharPref(kmPrefs.temp);
	restoreDefault(kmPrefs.temp);
	return ret;
}
/* --- Windows Registry Functions (Mozilla 1.8) ----------------------------------------------------------------- */
const HKCR = 0x80000000;
const HKCU = 0x80000001;
const HKLM = 0x80000002;
const HKU  = 0x80000003;
const HKCC = 0x80000005;

function getKey(keyRoot,keyPath,keyName) {
	var keyValue = null;

	try{
		var reg = Components.classes["@mozilla.org/windows-registry-key;1"].createInstance(Components.interfaces.nsIWindowsRegKey);
		reg.open(keyRoot,keyPath,reg.ACCESS_READ);
/*		switch(reg.getValueType(keyName)) {
// getValueType() is not available
			case reg.TYPE_STRING: keyValue = reg.readStringValue(keyName); break;
			case reg.TYPE_BINARY: keyValue = reg.readBinaryValue(keyName); break;
			case reg.TYPE_INT   : keyValue = reg.readIntValue(keyName); break;
			case reg.TYPE_INT64 : keyValue = reg.readInt64Value(keyName); break;
		}
*/		keyValue = reg.readStringValue(keyName);
		reg.close();
	} catch(e) {
	}
	return keyValue;
}
/* --- File & Folder Functions ---------------------------------------------------------------------------------- */
const nsIFilePicker = Components.interfaces.nsIFilePicker;
const nsILocalFile  = Components.interfaces.nsILocalFile;
const nsIFile       = Components.interfaces.nsIFile;
const nsIProperties = Components.interfaces.nsIProperties;

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_RDWR     = 0x04;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;
const MODE_SYNC     = 0x40;
const MODE_EXCL     = 0x80;

function getFolder(prefID) {
	var path;
	try {
			path = pref.getComplexValue(prefID,nsILocalFile);
	} catch(e) {
		try {
			path = Components.classes["@mozilla.org/file/directory_service;1"]
					 .getService(nsIProperties)
					 .get(prefID,nsIFile);
		} catch(e) {
			path = false;
		}
	}
	return ((path) ? path.path : false);
}
function getFile(filePath,fileName) {
	var file = Components.classes["@mozilla.org/file/local;1"]
			     .createInstance(nsILocalFile);

	file.initWithPath(filePath);
	if(fileName) file.append(fileName);
	return file;
}
function getFileURL(file) {
	var ios = Components.classes["@mozilla.org/network/io-service;1"]
			    .getService(Components.interfaces.nsIIOService);
	var fph = ios.getProtocolHandler("file")
		     .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
        return fph.getURLSpecFromFile(file);

}
function getFileContents(file) {
	var fileContents = "";

	if(file.exists()) {
		var fis = Components.classes["@mozilla.org/network/file-input-stream;1"]
				    .createInstance(Components.interfaces.nsIFileInputStream);
		var sis = Components.classes["@mozilla.org/scriptableinputstream;1"]
				    .createInstance(Components.interfaces.nsIScriptableInputStream);

		fis.init(file,MODE_RDONLY,0,fis.CLOSE_ON_EOF);
		sis.init(fis);
		while(sis.available() > 0) fileContents += sis.read(sis.available());
		sis.close();
		fis.close();
	}
	return fileContents;
}
function getFileContentsUTF8(file) {
	var fileContents = "";

	if(file.exists()) {
		var fis = Components.classes["@mozilla.org/network/file-input-stream;1"]
				    .createInstance(Components.interfaces.nsIFileInputStream);
		var sis = Components.classes["@mozilla.org/scriptableinputstream;1"]
				    .createInstance(Components.interfaces.nsIScriptableInputStream);

		fis.init(file,MODE_RDONLY,0,fis.CLOSE_ON_EOF);
		sis.init(fis);
		while(sis.available() > 0) fileContents += makeUTF8(sis.read(sis.available()));
		sis.close();
		fis.close();
	}
	return fileContents;
}
function setFileContents(file,fileContents) {
	var fos = Components.classes["@mozilla.org/network/file-output-stream;1"]
			    .createInstance(Components.interfaces.nsIFileOutputStream);

	fos.init(file,MODE_WRONLY|MODE_CREATE|MODE_TRUNCATE,0644,0);
	var ret = fos.write(fileContents,fileContents.length);
	fos.close();
	return ret;
}
/* --- User StyleSheet Functions -------------------------------------------------------------------------------- */
var UserPrefs = getFile(getFolder("ProfD"),"user.js");
var UserStyle = getFile(getFolder("UChrm"),"userContent.css");
var testInverse = false;
var testResult  = true;

function getIntUserPref(prefID,emptyStr) {
	var re = new RegExp("user_pref[\\s]*\\([\\s]*."+prefID+".[\\s]*,[\\s]*([\\d]+)[\\s]*\\)[\\s]*;","m");
	try {
		var ret = getFileContents(UserPrefs).match(re)[1];
	} catch(e) {
		var ret = emptyStr;
	}
	return ret;
}
function testMultiline(aString,aRegExpString) {
	var re = new RegExp(aRegExpString,"m");

	return re.test(aString);
}
function addToUserStyle(aString) {
	var fileContents = getFileContents(UserStyle);

	if(aString.indexOf("@import") > -1)
		// insert @import rules on top of others
		fileContents = aString + "\r\n" + fileContents;
	else
		fileContents += "\r\n" + aString;

	return (setFileContents(UserStyle,fileContents) == fileContents.length);
}
function addToUserPrefs(aString) {
	var fileContents = getFileContents(UserPrefs);

	fileContents = aString + "\r\n" + fileContents;

	return (setFileContents(UserPrefs,fileContents) == fileContents.length);
}
function removeFromFile(file,aRegExpString) {
	var fileContents = getFileContents(file);
	if(testInverse)
		var re = new RegExp("\r\n"+aRegExpString);
	else {
		var re = new RegExp(aRegExpString+"\r\n");
		testResult = true;
	}

	if(re.test(fileContents)) {
		fileContents = fileContents.replace(re,"");
		testResult &= (setFileContents(file,fileContents) == fileContents.length);
	} else
		testResult &= true;
	if(testInverse) {
		testInverse = false;
		return testResult;
	} else {
		testInverse = true;
		return removeFromFile(file,aRegExpString);
	}
}
/* --- K-Meleon-specific Functions ------------------------------------------------------------------------------ */
var kPlugin = {

absent	: function(pluginName) {
	return !(getFile(getFolder("KAPlugins"),pluginName+".dll").exists()||getFile(getFolder("KUPlugins"),pluginName+".dll").exists());
},
load	: function(pluginName) {
	if(this.absent(pluginName)) return false;
	try {
		var val = pref.getBoolPref("kmeleon.plugins."+pluginName+".load");
	} catch(e) {
		return false;
	}
	return val;
},
fullName: function(pluginName) {
	try {
		var val = document.getElementById("kplugin_bundle").getString(pluginName);
	} catch(e) {
		return document.getElementById("kplugin_bundle").getString("unknown_kplugin");
	}
	return val;
}

};

function kTabs() {
	try {
		var val = pref.getBoolPref("kmeleon.notab");
	} catch(e) {
		return true;
	}
	return !val;
}

var kMacrosModule = {

load: function(moduleName) {
	if( getFile(getFolder("KUserSettings"),      "macros.cfg").exists()) return false;
	if(!getFile(getFolder("CurProcD")+"\\macros\\","main.kmm").exists()) return false;
	if(moduleName == "main") return true;
	return ((getFile(getFolder("CurProcD")+"\\macros\\",moduleName+".kmm").exists()||getFile(getFolder("KUserSettings")+"\\macros\\",moduleName+".kmm").exists())&&(getPrefValue("kmeleon.plugins.macros.modules."+moduleName+".load") != "false"));
}

};
/* --- Helper Functions ----------------------------------------------------------------------------------------- */
var interactiveElements = new Array("button","checkbox","menulist","radio","textbox");

function disableAll(disable) {
	for(var j=0;j<interactiveElements.length;j++)
		for(var k=0,e=document.getElementsByTagName(interactiveElements[j]);k<e.length;k++)
			e[k].disabled=disable;
	for(var j=0,t=document.getElementsByTagName("tab");j<t.length;j++)
		t[j].setAttribute("disabled",disable);
}

var kmPrefs = {

last: "kmprefs.last",
temp: "kmprefs.temp",
save: function() {
	try {
		Components.classes["@mozilla.org/preferences-service;1"]
			  .getService(Components.interfaces.nsIPrefService)
			  .savePrefFile(null);
	} catch(e) {}
}

};