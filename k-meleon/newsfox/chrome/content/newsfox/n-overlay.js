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
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
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

// code based on Wladimir Palant 
// http://adblockplus.org/blog/avoiding-naming-conflicts-in-overlays
window.addEventListener('load', function()
{
	var done = false;

	function doRegister(button)
	{
		document.getElementById("newsfox-run").addEventListener("command", nfsandbox.openNewsfox, false);
    var nsbp = button.firstChild;
  	if (nsbp)
		{
			nsbp.addEventListener("command", 
				function(event){nfsandbox.NFstatusBar.subscribe(event)}, false);
  		nsbp.addEventListener("popupshowing", 
				function(event){return nfsandbox.NFstatusBar.makeMenu(this, event)}, false);
  		nsbp.addEventListener("popuphiding", 
        function(event){nfsandbox.NFstatusBar.removeMenu(this, event)}, false);
		}
	}

	function doUnregister()
	{
		window.removeEventListener('unload', doUnregister, false);

		document.getElementById("newsfox-run").removeEventListener("command", nfsandbox.openNewsfox, false);
		if (null != document.getElementById("newsfox-status-button-popup"))
		{
			var nsbp = document.getElementById("newsfox-status-button-popup");
			nsbp.removeEventListener("command", 
				function(event){nfsandbox.NFstatusBar.subscribe(event)}, false);
  		nsbp.removeEventListener("popupshowing", 
				function(event){return nfsandbox.NFstatusBar.makeMenu(this, event)}, false);
  		nsbp.removeEventListener("popuphiding", 
				function(event){nfsandbox.NFstatusBar.removeMenu(this, event)}, false);
		}
	}

  function removeOldButtonFromPalette()
  {
    var oldButton = null;
    var palette = nfsandbox.document.getElementById("navigator-toolbox").palette;
    for (var i=0; i<palette.childNodes.length; i++)
      if (palette.childNodes[i].id == "newsfox-button") { oldButton = palette.childNodes[i]; }
    if (oldButton) oldButton.parentNode.removeChild(oldButton);
  }
  
  function getButton()
  {
    var button;
    var palette = nfsandbox.document.getElementById("navigator-toolbox").palette;

    if (nfsandbox.document.getElementById("newsfox-status-button"))
      button = nfsandbox.document.getElementById("newsfox-status-button");
    else
      for (var i=0; i<palette.childNodes.length; i++)
        if (palette.childNodes[i].id == "newsfox-status-button") button = palette.childNodes[i];
        
    var doneNewButton = nfsandbox.NFgetPref("internal.doneNewButton", "bool", false);
    if (doneNewButton)
    {
      removeOldButtonFromPalette();
      return button;
    }

    var wantsButton = nfsandbox.NFgetPref("internal.statusBar", "bool", true);
    var hasOldButton = false;
    var oldButton;    
    if (document.getElementById("newsfox-button"))
    {
      oldButton = document.getElementById("newsfox-button");
      hasOldButton = true;
    }

    if (hasOldButton)
    {
      try
      {
        var toolbar = oldButton.parentNode;
        toolbar.insertItem("newsfox-status-button", oldButton, null, false);
        toolbar.currentSet = toolbar.currentSet.replace(/,newsfox-button/,"");
        toolbar.setAttribute("currentset", toolbar.currentSet);
        nfsandbox.document.persist(toolbar.id, "currentset");
      }
      catch(e) {}
    }
    else if (wantsButton)
    {
      try
      {
      var abar = document.getElementById("addon-bar");
      abar.insertItem("newsfox-status-button", null, null, false);
      abar.setAttribute("currentset", abar.currentSet);
      nfsandbox.document.persist(abar.id, "currentset");
      abar.collapsed = false;
      setToolbarVisibility(abar, true);
      }
      catch(e)
      {
// button doesn't work on first run in SeaMonkey or old Firefox versions, which the following should fix,
// but this event doesn't seem to get fired when button moved from palette to toolbar
//        button.addEventListener("DOMNodeInserted", function() { nfsandbox.NFstatusBar.register(button) }, false);
      }
    }

    nfsandbox.NFsetPref("internal.doneNewButton", "bool", true);  
    removeOldButtonFromPalette();  
    return button;
  }

  var nfsandbox = new Components.utils.Sandbox(window);

  // Define global variables "window" and "document" for the new namespace
  nfsandbox.window = window;
  nfsandbox.document = window.document;

  // Load scripts into the namespace
  var subscriptLoader = 
		Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
		.getService(Components.interfaces.mozIJSSubScriptLoader);
  subscriptLoader.loadSubScript("chrome://newsfox/content/globalfuncs.js", nfsandbox);
  subscriptLoader.loadSubScript("chrome://newsfox/content/newsfox-overlay.js", nfsandbox);
  subscriptLoader.loadSubScript("chrome://newsfox/content/options.js", nfsandbox);

  var button = getButton();
  if (button != null)
  {
    nfsandbox.NFinitoverlay(button);
    doRegister(button);
  }
	window.addEventListener('unload', doUnregister, false);

}, false);
