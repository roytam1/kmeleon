/* --- K-Meleon JSBridge Plugin --------------------------------------------------------------------------------- */

function JSBridge() {
	this.load = true;

	try {
		this._jsb = Components.classes["@kmeleon/jsbridge;1"]
				      .getService(Components.interfaces.nsIJSBridge);
	} catch(e) {
		this.load = false;
	}
}
JSBridge.prototype = {

id: function(commandID) {
	if(this.load)
		this._jsb.id(null ,commandID);
},
rebuildmenu: function(menuName) {
	if(this.load)
		this._jsb.RebuildMenu(menuName);
},
setmenu: function(menuName,itemType,itemName,command,location) {
	if(this.load) {
		// Remap the JSBridge item types to those of the setmenu() macro method.
		// In contrary to the latter, parameters cannot be omitted here.
		switch(itemType) {
			case "inline"	: itemType = this._jsb.MENU_INLINE; break;
			case "popup"	: itemType = this._jsb.MENU_POPUP; break;
			case "separator": itemType = this._jsb.MENU_SEPARATOR; break;
			case "command"	: itemType = this._jsb.MENU_COMMAND; break;
			// Item type "macro" must always be specified since items
			// are identified by command which is also modified here.
			case "macro"	: itemType = this._jsb.MENU_COMMAND; command = "macros("+command+")"; break;
			// Item type "plugin" is not supported by the setmenu() macro method.
			// Item type MENU_PLUGIN is for usage with plugin commands
			// like bookmarks() which create their own menus.
			case "plugin"	: itemType = this._jsb.MENU_PLUGIN; break;
			default		: itemType = "";
		}
		this._jsb.SetMenu(menuName,itemType,itemName,command,location);
	}
}

};
var JSB = new JSBridge();