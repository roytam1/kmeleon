const Cu = Components.utils;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function setEngine() {
    let searchEngineName;
    try {
      searchEngineName = Services.prefs.getComplexValue("kmeleon.general.searchEngineName", Ci.nsIPrefLocalizedString);
    } catch (ex) {
      searchEngineName = Services.prefs.getCharPref("kmeleon.general.searchEngineName", '');
    }
    
	  //var searchEngineName = Services.search.defaultEngine.name;
	  let searchText = document.getElementById("searchText");	  
    searchText.addEventListener("blur", function searchText_onBlur() {
        searchText.removeEventListener("blur", searchText_onBlur);
        searchText.removeAttribute("autofocus");
    });
    
    searchText.placeholder = searchEngineName;
    /*var submission = engine.getSubmission("_searchTerms_", null, "homepage");
    var searchURL = submission.uri.spec;//Services.prefs.getComplexValue("kmeleon.general.searchEngine", Ci.nsIPrefLocalizedString);
    searchText.setAttribute("data-url", searchURL);
    */
}

window.addEventListener('load', function () {
    Services.prefs.addObserver("kmeleon.general.searchEngineName", setEngine, false);
    setEngine();
});

window.addEventListener('beforeunload', function () {
    Services.prefs.removeObserver("kmeleon.general.searchEngineName", setEngine);
});
	
function onSearchSubmit(aEvent)
{
    var engine = Services.search.defaultEngine;
    var submission = engine.getSubmission("_searchTerms_", null, "homepage");
    var searchURL = submission && submission.uri ? submission.uri.spec : "";//Services.prefs.getComplexValue("kmeleon.general.searchEngine", Ci.nsIPrefLocalizedString);
    var searchTerms = document.getElementById("searchText").value;
    //var searchURL = document.getElementById("searchText").getAttribute("data-url");
   
    if (searchURL && searchTerms.length > 0) {

        const SEARCH_TOKEN = "_searchTerms_";
        let searchPostData = document.documentElement.getAttribute("searchEnginePostData");
        if (searchPostData) {
            // Check if a post form already exists. If so, remove it.
            const POST_FORM_NAME = "searchFormPost";
            let form = document.forms[POST_FORM_NAME];
            if (form) {
                form.parentNode.removeChild(form);
            }

            // Create a new post form.
            form = document.body.appendChild(document.createElement("form"));
            form.setAttribute("name", POST_FORM_NAME);
            // Set the URL to submit the form to.
            form.setAttribute("action", searchURL.replace(SEARCH_TOKEN, searchTerms));
            form.setAttribute("method", "post");

            // Create new <input type=hidden> elements for search param.
            searchPostData = searchPostData.split("&");
            for (let postVar of searchPostData) {
                let [name, value] = postVar.split("=");
                if (value == SEARCH_TOKEN) {
                    value = searchTerms;
                }
                let input = document.createElement("input");
                input.setAttribute("type", "hidden");
                input.setAttribute("name", name);
                input.setAttribute("value", value);
                form.appendChild(input);
            }
            // Submit the form.
            form.submit();
        } else {
            searchURL = searchURL.replace(SEARCH_TOKEN, encodeURIComponent(searchTerms));
            window.location.href = searchURL;
        }
    }

    aEvent.preventDefault();
}