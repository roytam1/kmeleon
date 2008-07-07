/* --- K-Meleon Home Page --------------------------------------------------------------------------------------- */

// pref.js and pref.properties (pref_bundle) required!

var Homepage = {

pref : "browser.startup.homepage",
init : function() {
	initTextbox(this.pref,false);
	document.getElementById(this.pref).nextSibling.disabled = this.isDefault();
},
selectFile: function(prefID) {
	var fp = Components.classes["@mozilla.org/filepicker;1"]
			   .createInstance(nsIFilePicker);

	fp.init(window,document.getElementById("pref_bundle").getString("select_"+prefID),nsIFilePicker.modeOpen);
	fp.appendFilters(	nsIFilePicker.filterHTML |
				nsIFilePicker.filterXML |
				nsIFilePicker.filterText |
				nsIFilePicker.filterImages);
	fp.displayDirectory = getFile(getFolder("Pers"),"");

	var ret = fp.show();
	if (ret == nsIFilePicker.returnOK) {
		setCharPref(prefID,fp.fileURL.spec);
		this.init();
	}
},
restoreDefault: function() {
//	restoreDefault(this.pref);
// Avoid Bug 1008
	var lang = pref.getCharPref("general.useragent.locale");

	setCharPref(this.pref,"resource:///"+(lang=="en-US"?"":("locales/"+lang+"/"))+"readme.html");
},
isDefault: function() {
//	return !pref.prefHasUserValue(this.pref);
// Avoid Bug 1008
	return (/^resource:\/\/\/(locales\/[a-z]{2}-[A-Z]{2}\/)?readme.html$/.exec(pref.getCharPref(this.pref)) != null);
}

};