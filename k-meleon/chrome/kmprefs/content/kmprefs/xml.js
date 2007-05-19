/* --- XML Helpers ---------------------------------------------------------------------------------------------- */

// prefs.js and xml.properties (xml_bundle) required!

// Helper for the kmPrefs 1.1 mouse gestures and search engines XML databases
function XML(url) {
	// Conversion has to take place before an object of this type is created.
	CFGtoXML = null;

	this.data = document.implementation.createDocument("","",null);
	this.data.addEventListener("load",init,true); // when ready, call the document's init() function
//	this.data.async = false;
	this.data.load(url);
}
XML.prototype = {

_mod: false,
save: function(file,attribs) {
	if(!this._mod) return;

	var data = "<?xml version=\"1.0\"?>\r\n";

	data += "<" + this.data.firstChild.tagName;
	data += " version=\"1.1\" caption=\"" + this.data.firstChild.getAttribute("caption") + "\">\r\n";
	for(var i=0,nodes=this.data.firstChild.childNodes;i<nodes.length;i++) if(nodes[i].nodeType==1) {
		data += "<" + nodes[i].tagName;
		for(var j=0;j<attribs.length;j++)
			data += ((j)?"\r\n\t":" ") + attribs[j] + "=\"" + makeUTF8(nodes[i].getAttribute(attribs[j]).replace(/&(?!amp;)/g,"&amp;")) + "\"";
		data += "/>\r\n";
	}
	data += "</" + this.data.firstChild.tagName + ">";
	setFileContents(file,data);
},
del: function(name,index) {
	var tags = this.data.getElementsByTagName(name);

	if((tags.length) && (index>-1) && (index<tags.length)) {
		this.data.firstChild.removeChild(tags[index]);
		this._mod = true;
		return true;
	}
	return false;
},
set: function(name,index,attribs,values,attrib) {
	var tags = this.data.getElementsByTagName(name),
	    tag  = this.data.createElement(name);

	for(var j=0;j<attribs.length;j++)
		tag.setAttribute(attribs[j],values[j]);
	if((index>-1) && (index<tags.length)) {
		if((attrib!=null) && (tags[index].getAttribute(attribs[attrib])==values[attrib])) {
			this.data.firstChild.replaceChild(tag,tags[index]);
			this._mod = true;
			return index;
		} else	this.del(name,index);
	}
	if(attrib!=null) {
		j=0;
		var a = attribs[attrib],
		    v = values[attrib].toLowerCase();
		while((j<tags.length) && (tags[j].getAttribute(a).toLowerCase().localeCompare(v)<0)) j++;
	} else	j=index;
	if((j>-1) && (j<tags.length)) {
		this.data.firstChild.insertBefore(tag,tags[j]);
		this._mod = true;
		return j;
	} else {
		this.data.firstChild.appendChild(tag);
		this._mod = true;
		return tags.length;
	}
}

};

// Convert kmPrefs 1.0 *.cfg files to kmPrefs 1.1 *.xml files, at least create an empty XML file.
var CFGtoXML = {

_cfg: null,
_xml: null,
_tag: null,

data: null,
init: function(variant) {
	this._xml = getFile(getFolder("KUserSettings"),variant+".xml");
	if(this._xml.exists()) return;
	this._cfg = getFile(getFolder("KUserSettings"),variant+".cfg");
	this.data = getFileContentsUTF8(this._cfg);
	this.data = this.data.replace(/\r/g,"\n").replace(/\n(?:\n)+/g,"\n").replace(/#.*\n/g,"");
	this.data = this.data.replace(/&(?!amp;)/g,"&amp;");
	switch(variant) {
		case "gestures"	: this._tag = "actions"; this.data = this.data.replace(/([^\n\t#=]+)\s*=\s*(.+)\n/g,"<action caption=\"$2\"\r\n\tcommand=\"$1\"/>\r\n"); break;
		case "search"	: this._tag = "engines"; this.data = this.data.replace(/([^\n\t#=]+)\s*=\s*(.+)\n/g,"<engine caption=\"$1\"\r\n\turl=\"$2\"/>\r\n"); break;
	}
	this.data = "<?xml version=\"1.0\"?>\r\n<" + this._tag + " version=\"1.1\" caption=\"" + makeUTF8(document.getElementById("xml_bundle").getString(variant)) + "\">\r\n" + this.data + "</" + this._tag + ">";
	setFileContents(this._xml,this.data);
}

};