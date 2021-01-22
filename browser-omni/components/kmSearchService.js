const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
  "resource://gre/modules/DeferredTask.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");

// A text encoder to UTF8, used whenever we commit the
// engine metadata to disk.
XPCOMUtils.defineLazyGetter(this, "gEncoder",
                            function() {
                              return new TextEncoder();
                            });


const BROWSER_SEARCH_PREF = "kmeleon.search.";
const USER_DEFINED = "{searchTerms}";
const SEARCH_SERVICE_TOPIC       = "browser-search-service";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const SEARCH_ENGINE_TOPIC        = "browser-search-engine-modified";
const QUIT_APPLICATION_TOPIC     = "quit-application";

const SEARCH_ENGINE_REMOVED      = "engine-removed";
const SEARCH_ENGINE_ADDED        = "engine-added";
const SEARCH_ENGINE_CHANGED      = "engine-changed";
const SEARCH_ENGINE_LOADED       = "engine-loaded";
const SEARCH_ENGINE_CURRENT      = "engine-current";
const SEARCH_ENGINE_DEFAULT      = "engine-default";

const DEFAULT_QUERY_CHARSET = "ISO-8859-1";

const URLTYPE_SUGGEST_JSON = "application/x-suggestions+json";
const URLTYPE_SEARCH_HTML  = "text/html";
const URLTYPE_OPENSEARCH   = "application/opensearchdescription+xml";

// Returns false for whitespace-only or commented out lines in a
// Sherlock file, true otherwise.
function isUsefulLine(aLine) {
  return !(/^\s*($|#)/i.test(aLine));
}

this.__defineGetter__("FileUtils", function() {
  delete this.FileUtils;
  Components.utils.import("resource://gre/modules/FileUtils.jsm");
  return FileUtils;
});

this.__defineGetter__("NetUtil", function() {
  delete this.NetUtil;
  Components.utils.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

this.__defineGetter__("gChromeReg", function() {
  delete this.gChromeReg;
  return this.gChromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                           getService(Ci.nsIChromeRegistry);
});

/**
 * Prefixed to all search debug output.
 */
const SEARCH_LOG_PREFIX = "*** Search: ";

/**
 * Outputs aText to the JavaScript console as well as to stdout.
 */
function DO_LOG(aText) {
  dump(SEARCH_LOG_PREFIX + aText + "\n");
  Services.console.logStringMessage(aText);
}

/**
 * In debug builds, use a live, pref-based (browser.search.log) LOG function
 * to allow enabling/disabling without a restart.
 */
function PREF_LOG(aText) {
  if (getBoolPref(BROWSER_SEARCH_PREF + "log", false))
    DO_LOG(aText);
}
var LOG = PREF_LOG;
//var LOG = function(){};

/**
 * Presents an assertion dialog in non-release builds and throws.
 * @param  message
 *         A message to display
 * @param  resultCode
 *         The NS_ERROR_* value to throw.
 * @throws resultCode
 */
function ERROR(message, resultCode) {
  NS_ASSERT(false, SEARCH_LOG_PREFIX + message);
  throw Components.Exception(message, resultCode);
}

/**
 * Logs the failure message (if browser.search.log is enabled) and throws.
 * @param  message
 *         A message to display
 * @param  resultCode
 *         The NS_ERROR_* value to throw.
 * @throws resultCode or NS_ERROR_INVALID_ARG if resultCode isn't specified.
 */
function FAIL(message, resultCode) {
  LOG(message);
  throw Components.Exception(message, resultCode || Cr.NS_ERROR_INVALID_ARG);
}

/**
 * Truncates big blobs of (data-)URIs to console-friendly sizes
 * @param str
 *        String to tone down
 * @param len
 *        Maximum length of the string to return. Defaults to the length of a tweet.
 */
function limitURILength(str, len) {
  len = len || 140;
  if (str.length > len)
    return str.slice(0, len) + "...";
  return str;
}

/**
 * Ensures an assertion is met before continuing. Should be used to indicate
 * fatal errors.
 * @param  assertion
 *         An assertion that must be met
 * @param  message
 *         A message to display if the assertion is not met
 * @param  resultCode
 *         The NS_ERROR_* value to throw if the assertion is not met
 * @throws resultCode
 */
function ENSURE_WARN(assertion, message, resultCode) {
  NS_ASSERT(assertion, SEARCH_LOG_PREFIX + message);
  if (!assertion)
    throw Components.Exception(message, resultCode);
}



/**
 * Used to verify a given DOM node's localName and namespaceURI.
 * @param aElement
 *        The element to verify.
 * @param aLocalNameArray
 *        An array of strings to compare against aElement's localName.
 * @param aNameSpaceArray
 *        An array of strings to compare against aElement's namespaceURI.
 *
 * @returns false if aElement is null, or if its localName or namespaceURI
 *          does not match one of the elements in the aLocalNameArray or
 *          aNameSpaceArray arrays, respectively.
 * @throws NS_ERROR_INVALID_ARG if aLocalNameArray or aNameSpaceArray are null.
 */
function checkNameSpace(aElement, aLocalNameArray, aNameSpaceArray) {
  if (!aLocalNameArray || !aNameSpaceArray)
    FAIL("missing aLocalNameArray or aNameSpaceArray for checkNameSpace");
  return (aElement                                                &&
          (aLocalNameArray.indexOf(aElement.localName)    != -1)  &&
          (aNameSpaceArray.indexOf(aElement.namespaceURI) != -1));
}

/**
 * Safely close a nsISafeOutputStream.
 * @param aFOS
 *        The file output stream to close.
 */
function closeSafeOutputStream(aFOS) {
  if (aFOS instanceof Ci.nsISafeOutputStream) {
    try {
      aFOS.finish();
      return;
    } catch (e) { }
  }
  aFOS.close();
}

/**
 * Wrapper function for nsIIOService::newURI.
 * @param aURLSpec
 *        The URL string from which to create an nsIURI.
 * @returns an nsIURI object, or null if the creation of the URI failed.
 */
function makeURI(aURLSpec, aCharset) {
  try {
    return NetUtil.newURI(aURLSpec, aCharset);
  } catch (ex) { }

  return null;
}

/**
 * Gets a directory from the directory service.
 * @param aKey
 *        The directory service key indicating the directory to get.
 */
function getDir(aKey, aIFace) {
  if (!aKey)
    FAIL("getDir requires a directory key!");

  return Services.dirsvc.get(aKey, aIFace || Ci.nsIFile);
}

/**
 * Returns a string interpretation of aBytes using aCharset, or null on
 * failure.
 */
function bytesToString(aBytes, aCharset) {
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  LOG("bytesToString: converting using charset: " + aCharset);

  try {
    converter.charset = aCharset;
    return converter.convertFromByteArray(aBytes, aBytes.length);
  } catch (ex) {}

  return null;
}

/**
 * Gets the current value of the locale.  It's possible for this preference to
 * be localized, so we have to do a little extra work here.  Similar code
 * exists in nsHttpHandler.cpp when building the UA string.
 */
function getLocale() {
  const localePref = "general.useragent.locale";
  var locale = getLocalizedPref(localePref);
  if (locale)
    return locale;

  // Not localized
  return Services.prefs.getCharPref(localePref);
}

/**
 * Wrapper for nsIPrefBranch::getComplexValue.
 * @param aPrefName
 *        The name of the pref to get.
 * @returns aDefault if the requested pref doesn't exist.
 */
function getLocalizedPref(aPrefName, aDefault) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    return Services.prefs.getComplexValue(aPrefName, nsIPLS).data;
  } catch (ex) {}
	
  return aDefault;
}

/**
 * Wrapper for nsIPrefBranch::setComplexValue.
 * @param aPrefName
 *        The name of the pref to set.
 */
function setLocalizedPref(aPrefName, aValue) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    var pls = Components.classes["@mozilla.org/pref-localizedstring;1"]
                        .createInstance(Ci.nsIPrefLocalizedString);
    pls.data = aValue;
    Services.prefs.setComplexValue(aPrefName, nsIPLS, pls);
  } catch (ex) {}
}

/**
 * Wrapper for nsIPrefBranch::getBoolPref.
 * @param aPrefName
 *        The name of the pref to get.
 * @returns aDefault if the requested pref doesn't exist.
 */
function getBoolPref(aName, aDefault) {
  try {
    return Services.prefs.getBoolPref(aName);
  } catch (ex) {
    return aDefault;
  }
}

/**
 * Get a unique nsIFile object with a sanitized name, based on the engine name.
 * @param aName
 *        A name to "sanitize". Can be an empty string, in which case a random
 *        8 character filename will be produced.
 * @returns A nsIFile object in the user's search engines directory with a
 *          unique sanitized name.
 */
function getSanitizedFile(aName) {
  var fileName = sanitizeName(aName) + ".xml";
  var file = getDir(NS_APP_USER_SEARCH_DIR);
  file.append(fileName);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  return file;
}

/**
 * @return a sanitized name to be used as a filename, or a random name
 *         if a sanitized name cannot be obtained (if aName contains
 *         no valid characters).
 */
function sanitizeName(aName) {
  const maxLength = 60;
  const minLength = 1;
  var name = aName.toLowerCase();
  name = name.replace(/\s+/g, "-");
  name = name.replace(/[^-a-z0-9]/g, "");

  // Use a random name if our input had no valid characters.
  if (name.length < minLength)
    name = Math.random().toString(36).replace(/^.*\./, '');

  // Force max length.
  return name.substring(0, maxLength);
}

/**
 * Retrieve a pref from the search param branch.
 *
 * @param prefName
 *        The name of the pref.
 **/
function getMozParamPref(prefName)
  Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "param." + prefName);

/**
 * Notifies watchers of SEARCH_ENGINE_TOPIC about changes to an engine or to
 * the state of the search service.
 *
 * @param aEngine
 *        The nsISearchEngine object to which the change applies.
 * @param aVerb
 *        A verb describing the change.
 *
 * @see nsIBrowserSearchService.idl
 */
let gInitialized = false;
function notifyAction(aEngine, aVerb) {
  if (gInitialized) {
    LOG("NOTIFY: Engine: \"" + aEngine.name + "\"; Verb: \"" + aVerb + "\"");
    Services.obs.notifyObservers(aEngine, SEARCH_ENGINE_TOPIC, aVerb);
  }
}

/**
 * Simple object representing a name/value pair.
 */
function QueryParameter(aName, aValue, aPurpose) {
  if (!aName || (aValue == null))
    FAIL("missing name or value for QueryParameter!");

  this.name = aName;
  this.value = aValue;
  this.purpose = aPurpose;
}

function Engine(aName, aUrl, aIsReadOnly = false) {
  this._name = aName;
  this._url = aUrl;
  this._readOnly = aIsReadOnly;
}

Engine.prototype = {

  _url:null,
  _name:null,
 // The engine's alias (can be null). Initialized to |undefined| to indicate
  // not-initialized-from-engineMetadataService.
  _alias: undefined,
  // A distribution-unique identifier for the engine. Either null or set
  // when loaded. See getter.
  _identifier: undefined,
  // The data describing the engine. Is either an array of bytes, for Sherlock
  // files, or an XML document element, for XML plugins.
  _data: null,
  // The engine's data type. See data types (DATA_) defined above.
  _dataType: null,
  // Whether or not the engine is readonly.
  _readOnly: true,
  // The engine's description
  _description: "",
  // Used to store the engine to replace, if we're an update to an existing
  // engine.
  _engineToUpdate: null,
  _readOnly: true,
  _hidden:false,

  
  // nsISearchEngine
  get alias() {
	return this._name;
    if (this._alias === undefined)
      this._alias = engineMetadataService.getAttr(this, "alias");

    return this._alias;
  },
  set alias(val) {
    this._alias = val;
    //engineMetadataService.setAttr(this, "alias", val);
    notifyAction(this, SEARCH_ENGINE_CHANGED);
  },

  /**
   * Return the built-in identifier of app-provided engines.
   *
   * Note that this identifier is substantially similar to _id, with the
   * following exceptions:
   *
   * * There is no trailing file extension.
   * * There is no [app] prefix.
   *
   * @return a string identifier, or null.
   */
  get identifier() {
    if (this._identifier !== undefined) {
      return this._identifier;
    }

    // No identifier if If the engine isn't app-provided
    if (!this._isInAppDir && !this._isInJAR) {
      return this._identifier = null;
    }

    let leaf = this._getLeafName();
    ENSURE_WARN(leaf, "identifier: app-provided engine has no leafName");

    // Strip file extension.
    let ext = leaf.lastIndexOf(".");
    if (ext == -1) {
      return this._identifier = leaf;
    }
    return this._identifier = leaf.substring(0, ext);
  },

  get description() {
    return this._description;
  },

  get hidden() {
    //if (this._hidden === null)
    //  this._hidden = engineMetadataService.getAttr(this, "hidden") || false;
    return this._hidden;
  },
  set hidden(val) {
    var value = !!val;
    if (value != this._hidden) {
      this._hidden = value;
      //engineMetadataService.setAttr(this, "hidden", value);
      notifyAction(this, SEARCH_ENGINE_CHANGED);
    }
  },

  get iconURI() {
    if (this._iconURI)
      return this._iconURI;
    return null;
  },

  get _iconURL() {
    if (!this._iconURI)
      return "";
    return this._iconURI.spec;
  },

  get name() {
    return this._name;
  },

  get type() {
    return this._type;
  },

  get searchForm() {
	return this._url;
  },

  get queryCharset() {
    return 'utf-8';
    if (this._queryCharset)
      return this._queryCharset;
    return this._queryCharset = getLocalizedPref("intl.charset.default", DEFAULT_QUERY_CHARSET);//queryCharsetFromCode(/* get the default */);
  },

  // from nsISearchEngine
  addParam: function SRCH_ENG_addParam(aName, aValue, aResponseType) {
    if (!aName || (aValue == null))
      FAIL("missing name or value for nsISearchEngine::addParam!");
    ENSURE_WARN(!this._readOnly,
                "called nsISearchEngine::addParam on a read-only engine!",
                Cr.NS_ERROR_FAILURE);
    if (!aResponseType)
      aResponseType = URLTYPE_SEARCH_HTML;

    var url = this._getURLOfType(aResponseType);
    if (!url)
      FAIL("Engine object has no URL for response type " + aResponseType,
           Cr.NS_ERROR_FAILURE);

    url.addParam(aName, aValue);

    // Serialize the changes to file lazily
    this._lazySerializeToFile();
  },


  // from nsISearchEngine
  getSubmission: function SRCH_ENG_getSubmission(aData, aResponseType, aPurpose) {
    if (!aResponseType) {
      aResponseType = URLTYPE_SEARCH_HTML;
    }

    var url = this._url;//this._getURLOfType(aResponseType);

    if (!url)
      return null;

    if (!aData) {
      // Return a dummy submission object with our searchForm attribute
      return new Submission(makeURI(this.searchForm));
    }

    LOG("getSubmission: In data: \"" + aData + "\"; Purpose: \"" + aPurpose + "\"");
    var textToSubURI = Cc["@mozilla.org/intl/texttosuburi;1"].
                       getService(Ci.nsITextToSubURI);
    var data = "";
    try {
      data = textToSubURI.ConvertAndEscape(this.queryCharset, aData);
    } catch (ex) {
      LOG("getSubmission: Falling back to default queryCharset!");
      data = textToSubURI.ConvertAndEscape(DEFAULT_QUERY_CHARSET, aData);
    }
    LOG("getSubmission: Out data: \"" + data + "\"");
    return this._getSubmission(this._url, data, aPurpose);
  },
  
  _getSubmission: function SRCH_EURL_getSubmission(url, aSearchTerms, aPurpose) {
    url += aSearchTerms;
    //var url = ParamSubstitution(this.template, aSearchTerms, aEngine);
    // Default to an empty string if the purpose is not provided so that default purpose params
    // (purpose="") work consistently rather than having to define "null" and "" purposes.
    var purpose = aPurpose || "";

    // Create an application/x-www-form-urlencoded representation of our params
    // (name=value&name=value&name=value)
    var dataString = "";
   /* 
    for (var i = 0; i < this.params.length; ++i) {
      var param = this.params[i];

      // If this parameter has a purpose, only add it if the purpose matches
      if (param.purpose !== undefined && param.purpose != purpose)
        continue;

      //var value = ParamSubstitution(param.value, aSearchTerms, aEngine);

      dataString += (i > 0 ? "&" : "") + param.name + "=" + value;
    }*/

    var postData = null;
    let postDataString = null;
    if (this.method == "GET") {
      // GET method requests have no post data, and append the encoded
      // query string to the url...
      if (url.indexOf("?") == -1 && dataString)
        url += "?";
      url += dataString;
    } else if (this.method == "POST") {
      // For POST requests, specify the data as a MIME stream as well as a string.
      postDataString = dataString;
      var stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                         createInstance(Ci.nsIStringInputStream);
      stringStream.data = dataString;

      postData = Cc["@mozilla.org/network/mime-input-stream;1"].
                 createInstance(Ci.nsIMIMEInputStream);
      postData.addHeader("Content-Type", "application/x-www-form-urlencoded");
      postData.addContentLength = true;
      postData.setData(stringStream);
    }

    return new Submission(makeURI(url), postData, postDataString);
  },

  // from nsISearchEngine
  supportsResponseType: function SRCH_ENG_supportsResponseType(type) {
	return URLTYPE_SEARCH_HTML == type;
  },

  // nsISupports
  QueryInterface: function SRCH_ENG_QI(aIID) {
    if (aIID.equals(Ci.nsISearchEngine) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  get wrappedJSObject() {
    return this;
  }

};

// nsISearchSubmission
function Submission(aURI, aPostData = null, aPostDataString = null) {
  this._uri = aURI;
  this._postData = aPostData;
  this._postDataString = aPostDataString;
}
Submission.prototype = {
  get uri() {
    return this._uri;
  },
  get postData() {
    return this._postData;
  },
  get postDataString() {
    return this._postDataString;
  },
  QueryInterface: function SRCH_SUBM_QI(aIID) {
    if (aIID.equals(Ci.nsISearchSubmission) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}

function executeSoon(func) {
  Services.tm.mainThread.dispatch(func, Ci.nsIThread.DISPATCH_NORMAL);
}


// nsIBrowserSearchService
function SearchService() {
  // Replace empty LOG function with the useful one if the log pref is set.
  if (getBoolPref(BROWSER_SEARCH_PREF + "log", false))
    LOG = DO_LOG;

  this._initObservers = Promise.defer();
}

SearchService.prototype = {
  classID: Components.ID("{6319788a-fe93-4db3-9f39-818cf08f4256}"),

  // The current status of initialization. Note that it does not determine if
  // initialization is complete, only if an error has been encountered so far.
  _initRV: Cr.NS_OK,

  // If initialization has not been completed yet, perform synchronous
  // initialization.
  // Throws in case of initialization error.
  _ensureInitialized: function  SRCH_SVC__ensureInitialized() {
    if (gInitialized) {
      if (!Components.isSuccessCode(this._initRV)) {
        LOG("_ensureInitialized: failure");
        throw this._initRV;
      }
      return;
    }

    let warning =
      "Search service falling back to synchronous initialization. " +
      "This is generally the consequence of an add-on using a deprecated " +
      "search service API.";
    // Bug 785487 - Disable warning until our own callers are fixed.
    //Deprecated.warning(warning, "https://developer.mozilla.org/en-US/docs/XPCOM_Interface_Reference/nsIBrowserSearchService#async_warning");
    LOG(warning);

   // engineMetadataService.syncInit();
    this._syncInit();
    if (!Components.isSuccessCode(this._initRV)) {
      throw this._initRV;
    }
  },
  
  _addEngineToStore: function SRCH_SVC_addEngineToStore(aEngine) {
    this._engines[aEngine.name] = aEngine;
    notifyAction(aEngine, SEARCH_ENGINE_ADDED);
    LOG('Added engine: ' + aEngine.name + (aEngine._readOnly?' readonly':''));
  },
  
  _loadSearchXML: function SRCH_SVC__loadSearchXML(aFile, aDefault) {
    if (!aFile || !aFile.exists()) {
      LOG("File "+aFile.path+" not found!");
      return;
    }
    var domParser = Cc["@mozilla.org/xmlextras/domparser;1"].
                        createInstance(Ci.nsIDOMParser);
	
    var fileInStream = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);

    fileInStream.init(aFile, MODE_RDONLY, FileUtils.PERMS_FILE, false);
	
    var doc = domParser.parseFromStream(fileInStream, "UTF-8",
                 aFile.fileSize,
                 "text/xml");
    var data = doc.documentElement;
	
    for(var j=0,engines=data.getElementsByTagName("engine"),titem,tcell;j<engines.length;j++) {
	    var engine = new Engine(engines[j].getAttribute('caption'), engines[j].getAttribute('url'), aDefault);
	    this._addEngineToStore(engine);
	  }    
  },
  
  _syncLoadEngines: function SRCH_SVC__syncLoadEngines() {
  
    var file = getDir("DefRt");
    file.append("search.xml");
    this._loadSearchXML(file, true);
      
    var file = getDir("GreD");
    file.append('locales');
    file.append(getLocalizedPref('general.useragent.locale'))
    file.append("search.xml");
    if (file.exists())
      this._loadSearchXML(file, false);
        
    file = getDir("KUserSettings");
    file.append("search.xml");
    this._loadSearchXML(file, false);      
  },
  
  _saveEngines: function SRCH_SVC__syncLoadEngines() {
   
    var doc = Components.classes["@mozilla.org/xml/xml-document;1"] 
      .createInstance(Components.interfaces.nsIDOMDocument);
    var elEngines = doc.createElement("engines");
    doc.appendChild( elEngines );  
    for each (engine in this._engines) {
      
      if (!engine.hidden && engine._readOnly) continue;
      LOG(engine.name + ' ' + engine.hidden +' ' + engine._readOnly + ' ' + (!engine.hidden && engine._readOnly));
      var el = doc.createElement("engine");
      el.setAttribute("caption" , engine._name );
      el.setAttribute("url" , engine.hidden ? "" : engine._url );
      elEngines.appendChild( el );
    }
    
    var file = getDir("KUserSettings", false);
    file.append("search.xml");
    
    var domSerializer = Components.classes["@mozilla.org/xmlextras/xmlserializer;1"]
                    .createInstance(Components.interfaces.nsIDOMSerializer);
                    
    var stream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                  .createInstance(Components.interfaces.nsIFileOutputStream);
    stream.init(file, MODE_TRUNCATE|MODE_CREATE|MODE_WRONLY, FileUtils.PERMS_FILE, false); 
    domSerializer.serializeToStream(doc, stream, "utf-8"); 
    stream.close();
  },
  

  // Synchronous implementation of the initializer.
  // Used by |_ensureInitialized| as a fallback if initialization is not
  // complete. In this implementation, it is also used by |init|.
  _syncInit: function SRCH_SVC__syncInit() {
    LOG("_syncInit start");
    try {
      this._syncLoadEngines();
    } catch (ex) {
      this._initRV = Cr.NS_ERROR_FAILURE;
      LOG("_syncInit: failure loading engines: " + ex);
    }
    this._addObservers();

    gInitialized = true;

    this._initObservers.resolve(this._initRV);

    Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "init-complete");

    LOG("_syncInit end");
  },

  _engines: { },
  __sortedEngines: null,
  get _sortedEngines() {
    if (!this.__sortedEngines)
      return this._buildSortedEngineList();
    return this.__sortedEngines;
  },
  
  _buildSortedEngineList: function SRCH_SVC_buildSortedEngineList() {
    var alphaEngines = [];
    for each (engine in this._engines) {
        alphaEngines.push(this._engines[engine.name]);
    }
    this.__sortedEngines = alphaEngines.sort(function (a, b) {
                                       return a.name.localeCompare(b.name);
                                     });
    return this.__sortedEngines;
  },
	
  _saveSortedEngineList: function SRCH_SVC_saveSortedEngineList() {
  },
  
  /**
   * Get a sorted array of engines.
   * @param aWithHidden
   *        True if hidden plugins should be included in the result.
   */
  _getSortedEngines: function SRCH_SVC_getSorted(aWithHidden) {
     if (aWithHidden)
       return this._sortedEngines;

    return this._sortedEngines.filter(function (engine) {
                                        return !engine.hidden;
                                      });
  },

  // nsIBrowserSearchService
  init: function SRCH_SVC_init(observer) {
    LOG("SearchService.init");
    let self = this;
    if (!this._initStarted) {
      TelemetryStopwatch.start("SEARCH_SERVICE_INIT_MS");
      this._initStarted = true;
      TaskUtils.spawn(function task() {
        try {
          //yield engineMetadataService.init();
          if (gInitialized) {
            // No need to pursue asynchronous initialization,
            // synchronous fallback had to be called and has finished.
            return;
          }
          // Complete initialization. In the current implementation,
          // this is done by calling the synchronous initializer.
          // Future versions might introduce an actually synchronous
          // implementation.
          self._syncInit();
          TelemetryStopwatch.finish("SEARCH_SERVICE_INIT_MS");
        } catch (ex) {
          self._initObservers.reject(ex);
          TelemetryStopwatch.cancel("SEARCH_SERVICE_INIT_MS");
        }
      });
    }
    if (observer) {
      TaskUtils.captureErrors(this._initObservers.promise.then(
        function onSuccess() {
          observer.onInitComplete(self._initRV);
        },
        function onError(aReason) {
          Components.utils.reportError("Internal error while initializing SearchService: " + aReason);
          observer.onInitComplete(Components.results.NS_ERROR_UNEXPECTED);
        }
      ));
    }
  },

  get isInitialized() {
    return gInitialized;
  },

  getEngines: function SRCH_SVC_getEngines(aCount) {
    LOG("getEngines: getting all engines");
    this._ensureInitialized();    
    var engines = this._getSortedEngines(true);
    aCount.value = engines.length;
    return engines;
  },

  getVisibleEngines: function SRCH_SVC_getVisible(aCount) {
    this._ensureInitialized();
    LOG("getVisibleEngines: getting all visible engines");
    var engines = this._getSortedEngines(false);
    aCount.value = engines.length;
    return engines;
  },

  getDefaultEngines: function SRCH_SVC_getDefault(aCount) {
    this._ensureInitialized();
    function isDefault(engine) {
      return engine._isDefault;
    };
    var engines = this._sortedEngines.filter(isDefault);
    var engineOrder = {};
    var engineName;
    var i = 1;

    // Build a list of engines which we have ordering information for.
    // We're rebuilding the list here because _sortedEngines contain the
    // current order, but we want the original order.

    // First, look at the "browser.search.order.extra" branch.
    try {
      var extras = Services.prefs.getChildList(BROWSER_SEARCH_PREF + "order.extra.");

      for each (var prefName in extras) {
        engineName = Services.prefs.getCharPref(prefName);

        if (!(engineName in engineOrder))
          engineOrder[engineName] = i++;
      }
    } catch (e) {
      LOG("Getting extra order prefs failed: " + e);
    }

    // Now look through the "browser.search.order" branch.
    for (var j = 1; ; j++) {
      engineName = getLocalizedPref(BROWSER_SEARCH_PREF + "order." + j);
      if (!engineName)
        break;

      if (!(engineName in engineOrder))
        engineOrder[engineName] = i++;
    }

    LOG("getDefaultEngines: engineOrder: " + engineOrder.toSource());

    function compareEngines (a, b) {
      var aIdx = engineOrder[a.name];
      var bIdx = engineOrder[b.name];

      if (aIdx && bIdx)
        return aIdx - bIdx;
      if (aIdx)
        return -1;
      if (bIdx)
        return 1;

      return a.name.localeCompare(b.name);
    }
    engines.sort(compareEngines);

    aCount.value = engines.length;
    return engines;
  },

  getEngineByName: function SRCH_SVC_getEngineByName(aEngineName) {
    this._ensureInitialized();
    LOG('Get engine ' + aEngineName);
    return this._engines[aEngineName] || null;
  },

  getEngineByAlias: function SRCH_SVC_getEngineByAlias(aAlias) {
    this._ensureInitialized();
    for (var engineName in this._engines) {
      var engine = this._engines[engineName];
      if (engine && engine.alias == aAlias)
        return engine;
    }
    return null;
  },

  addEngineWithDetails: function SRCH_SVC_addEWD(aName, aIconURL, aAlias,
                                                 aDescription, aMethod,
                                                 aTemplate) {
    this._ensureInitialized();
    if (!aName)
      FAIL("Invalid name passed to addEngineWithDetails!");
    if (!aMethod)
      FAIL("Invalid method passed to addEngineWithDetails!");
    if (!aTemplate)
      FAIL("Invalid template passed to addEngineWithDetails!");
    //if (this._engines[aName])
    // FAIL("An engine with that name already exists!", Cr.NS_ERROR_FILE_ALREADY_EXISTS);

    var engine = new Engine(aName, aTemplate, false);
    this._addEngineToStore(engine);
    this._saveEngines();
    this.__sortedEngines = null;
  },

  addEngine: function SRCH_SVC_addEngine(aEngineURL, aDataType, aIconURL,
                                         aConfirm, aCallback) {
    LOG("addEngine: Adding \"" + aEngineURL + "\".");
    this._ensureInitialized();
    //FAIL("addEngine: Error adding engine:\n" + ex, Cr.NS_ERROR_FAILURE);
    
  },

  removeEngine: function SRCH_SVC_removeEngine(aEngine) {
    this._ensureInitialized();
    if (!aEngine)
      FAIL("no engine passed to removeEngine!");

    var engineToRemove = null;
    for (var e in this._engines) {
      if (aEngine.wrappedJSObject == this._engines[e])
        engineToRemove = this._engines[e];
    }

    if (!engineToRemove)
      FAIL("removeEngine: Can't find engine to remove!", Cr.NS_ERROR_FILE_NOT_FOUND);

    if (engineToRemove == this.currentEngine) {
      this._currentEngine = null;
    }

    if (engineToRemove == this.defaultEngine) {
      this._defaultEngine = null;
    }

    if (engineToRemove._readOnly) {
      // Just hide it (the "hidden" setter will notify) and remove its alias to
      // avoid future conflicts with other engines.
      engineToRemove.hidden = true;
      engineToRemove.alias = null;
    } else {

      // Remove the engine from _sortedEngines
      var index = this._sortedEngines.indexOf(engineToRemove);
      if (index == -1)
        FAIL("Can't find engine to remove in _sortedEngines!", Cr.NS_ERROR_FAILURE);
      this.__sortedEngines.splice(index, 1);

      // Remove the engine from the internal store
      delete this._engines[engineToRemove.name];

      notifyAction(engineToRemove, SEARCH_ENGINE_REMOVED);

      // Since we removed an engine, we need to update the preferences.
      this._saveSortedEngineList();
    }
    
    this._saveEngines();
    this.__sortedEngines = null;
  },

  moveEngine: function SRCH_SVC_moveEngine(aEngine, aNewIndex) {
    this._ensureInitialized();
    if ((aNewIndex > this._sortedEngines.length) || (aNewIndex < 0))
      FAIL("SRCH_SVC_moveEngine: Index out of bounds!");
    if (!(aEngine instanceof Ci.nsISearchEngine))
      FAIL("SRCH_SVC_moveEngine: Invalid engine passed to moveEngine!");
    if (aEngine.hidden)
      FAIL("moveEngine: Can't move a hidden engine!", Cr.NS_ERROR_FAILURE);

    var engine = aEngine.wrappedJSObject;

    var currentIndex = this._sortedEngines.indexOf(engine);
    if (currentIndex == -1)
      FAIL("moveEngine: Can't find engine to move!", Cr.NS_ERROR_UNEXPECTED);

    // Our callers only take into account non-hidden engines when calculating
    // aNewIndex, but we need to move it in the array of all engines, so we
    // need to adjust aNewIndex accordingly. To do this, we count the number
    // of hidden engines in the list before the engine that we're taking the
    // place of. We do this by first finding newIndexEngine (the engine that
    // we were supposed to replace) and then iterating through the complete 
    // engine list until we reach it, increasing aNewIndex for each hidden
    // engine we find on our way there.
    //
    // This could be further simplified by having our caller pass in
    // newIndexEngine directly instead of aNewIndex.
    var newIndexEngine = this._getSortedEngines(false)[aNewIndex];
    if (!newIndexEngine)
      FAIL("moveEngine: Can't find engine to replace!", Cr.NS_ERROR_UNEXPECTED);

    for (var i = 0; i < this._sortedEngines.length; ++i) {
      if (newIndexEngine == this._sortedEngines[i])
        break;
      if (this._sortedEngines[i].hidden)
        aNewIndex++;
    }

    if (currentIndex == aNewIndex)
      return; // nothing to do!

    // Move the engine
    var movedEngine = this.__sortedEngines.splice(currentIndex, 1)[0];
    this.__sortedEngines.splice(aNewIndex, 0, movedEngine);

    notifyAction(engine, SEARCH_ENGINE_CHANGED);

    // Since we moved an engine, we need to update the preferences.
    this._saveSortedEngineList();
  },

  restoreDefaultEngines: function SRCH_SVC_resetDefaultEngines() {
    this._ensureInitialized();
    for each (var e in this._engines) {
      // Unhide all default engines
      if (e.hidden && e._isDefault)
        e.hidden = false;
    }
  },

  get defaultEngine() {
    this._ensureInitialized();
    if (!this._defaultEngine) {
      let defPref = "kmeleon.general.searchEngineName";
      let defaultEngine = this.getEngineByName(getLocalizedPref(defPref, ""))
	    if (!defaultEngine)
        defaultEngine = this.getEngineByName(Services.prefs.getCharPref(defPref));	
      if (!defaultEngine)
        defaultEngine = this._getSortedEngines(false)[0] || null;
      this._defaultEngine = defaultEngine;
    }
    if (this._defaultEngine && this._defaultEngine.hidden)
      return this._getSortedEngines(false)[0];
    return this._defaultEngine;
  },

  set defaultEngine(val) {
    this._ensureInitialized();
    // Sometimes we get wrapped nsISearchEngine objects (external XPCOM callers),
    // and sometimes we get raw Engine JS objects (callers in this file), so
    // handle both.
    if (!(val instanceof Ci.nsISearchEngine) && !(val instanceof Engine))
      FAIL("Invalid argument passed to defaultEngine setter");

    let newDefaultEngine = this.getEngineByName(val.name);
    if (!newDefaultEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    if (newDefaultEngine == this._defaultEngine)
      return;

    this._defaultEngine = newDefaultEngine;
    var submission = this._defaultEngine.getSubmission("", null, "homepage");
    var searchURL = submission && submission.uri? submission.uri.spec : '';

    // Set a flag to keep track that this setter was called properly, not by
    // setting the pref alone.
    this._changingDefaultEngine = true;
    let defPref = "kmeleon.general.searchEngineName";
    // If we change the default engine in the future, that change should impact
    // users who have switched away from and then back to the build's "default"
    // engine. So clear the user pref when the defaultEngine is set to the
    // build's default engine, so that the defaultEngine getter falls back to
    // whatever the default is.
    if (this._defaultEngine == this._originalDefaultEngine) {
      Services.prefs.clearUserPref(defPref);
      Services.prefs.clearUserPref("kmeleon.general.searchEngine");
    }
    else {
      Services.prefs.setCharPref(defPref, this._defaultEngine.name);
      Services.prefs.setCharPref("kmeleon.general.searchEngine", searchURL);
    }
    this._changingDefaultEngine = false;

    notifyAction(this._defaultEngine, SEARCH_ENGINE_DEFAULT);
  },

  get currentEngine() {
    this._ensureInitialized();
    if (!this._currentEngine) {
      let selectedEngine = getLocalizedPref(BROWSER_SEARCH_PREF +
                                            "selectedEngine");
      this._currentEngine = this.getEngineByName(selectedEngine);
    }

    if (!this._currentEngine || this._currentEngine.hidden)
      this._currentEngine = this.defaultEngine;
    return this._currentEngine;
  },

  set currentEngine(val) {
    this._ensureInitialized();
    // Sometimes we get wrapped nsISearchEngine objects (external XPCOM callers),
    // and sometimes we get raw Engine JS objects (callers in this file), so
    // handle both.
    if (!(val instanceof Ci.nsISearchEngine) && !(val instanceof Engine))
      FAIL("Invalid argument passed to currentEngine setter");

    var newCurrentEngine = this.getEngineByName(val.name);
    if (!newCurrentEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    if (newCurrentEngine == this._currentEngine)
      return;

    this._currentEngine = newCurrentEngine;

    var currentEnginePref = BROWSER_SEARCH_PREF + "selectedEngine";

    // Set a flag to keep track that this setter was called properly, not by
    // setting the pref alone.
    this._changingCurrentEngine = true;
    // If we change the default engine in the future, that change should impact
    // users who have switched away from and then back to the build's "default"
    // engine. So clear the user pref when the currentEngine is set to the
    // build's default engine, so that the currentEngine getter falls back to
    // whatever the default is.
    if (this._currentEngine == this._originalDefaultEngine) {
      Services.prefs.clearUserPref(currentEnginePref);
    }
    else {
      setLocalizedPref(currentEnginePref, this._currentEngine.name);     
    }
    this._changingCurrentEngine = false;

    notifyAction(this._currentEngine, SEARCH_ENGINE_CURRENT);
  },
  
   _setEngineByPref: function SRCH_SVC_setEngineByPref(aEngineType, aPref) {
    this._ensureInitialized();
    let newEngine = this.getEngineByName(getLocalizedPref(aPref, ""));
    if (!newEngine)
      newEngine = this.getEngineByName(Services.prefs.getCharPref(aPref, ""));
    if (!newEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    this[aEngineType] = newEngine;
  },

  // nsIObserver
  observe: function SRCH_SVC_observe(aEngine, aTopic, aVerb) {
    switch (aTopic) {
      case SEARCH_ENGINE_TOPIC:
        switch (aVerb) {
          case SEARCH_ENGINE_LOADED:
            var engine = aEngine.QueryInterface(Ci.nsISearchEngine);
            LOG("nsSearchService::observe: Done installation of " + engine.name
                + ".");
            this._addEngineToStore(engine.wrappedJSObject);
            if (engine.wrappedJSObject._useNow) {
              LOG("nsSearchService::observe: setting current");
              this.currentEngine = aEngine;
            }
            break;
          case SEARCH_ENGINE_CHANGED:
          case SEARCH_ENGINE_REMOVED:
            break;
        }
        break;

      case QUIT_APPLICATION_TOPIC:
        this._removeObservers();
        if (this._batchTimer) {
          // Flush to disk immediately
          this._batchTimer.cancel();
          this._buildCache();
        }
        //engineMetadataService.flush();
        break;

      case "nsPref:changed":
        let currPref = BROWSER_SEARCH_PREF + "selectedEngine";
        let defPref = "kmeleon.general.searchEngineName";
        if (aVerb == currPref && !this._changingCurrentEngine) {
          this._setEngineByPref("currentEngine", currPref);
        } else if (aVerb == defPref && !this._changingDefaultEngine) {
          this._setEngineByPref("defaultEngine", defPref);
        }
        break;
    }
  },

  // nsITimerCallback
  notify: function SRCH_SVC_notify(aTimer) {
    LOG("_notify: checking for updates");

    if (!getBoolPref(BROWSER_SEARCH_PREF + "update", false))
      return;

    // Our timer has expired, but unfortunately, we can't get any data from it.
    // Therefore, we need to walk our engine-list, looking for expired engines
    var currentTime = Date.now();
    LOG("currentTime: " + currentTime);
    for each (engine in this._engines) {
      engine = engine.wrappedJSObject;
      if (!engine._hasUpdates)
        continue;

      LOG("checking " + engine.name);

     // var expirTime = engineMetadataService.getAttr(engine, "updateexpir");
      LOG("expirTime: " + expirTime + "\nupdateURL: " + engine._updateURL +
          "\niconUpdateURL: " + engine._iconUpdateURL);

      var engineExpired = expirTime <= currentTime;

      if (!expirTime || !engineExpired) {
        LOG("skipping engine");
        continue;
      }

      LOG(engine.name + " has expired");

      engineUpdateService.update(engine);

      // Schedule the next update
      engineUpdateService.scheduleNextUpdate(engine);

    } // end engine iteration
  },

  _addObservers: function SRCH_SVC_addObservers() {
    //Services.obs.addObserver(this, SEARCH_ENGINE_TOPIC, false);
    //Services.obs.addObserver(this, QUIT_APPLICATION_TOPIC, false);
    Services.prefs.addObserver("kmeleon.general.searchEngineName", this, false);
    //Services.prefs.addObserver(BROWSER_SEARCH_PREF + "selectedEngine", this, false);
  },

  _removeObservers: function SRCH_SVC_removeObservers() {
    //Services.obs.removeObserver(this, SEARCH_ENGINE_TOPIC);
    //Services.obs.removeObserver(this, QUIT_APPLICATION_TOPIC);
    Services.prefs.removeObserver("kmeleon.general.searchEngineName", this);
    //Services.prefs.removeObserver(BROWSER_SEARCH_PREF + "selectedEngine", this);
  },

  QueryInterface: function SRCH_SVC_QI(aIID) {
    if (aIID.equals(Ci.nsIBrowserSearchService) ||
        aIID.equals(Ci.nsIObserver)             ||
        aIID.equals(Ci.nsITimerCallback)        ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};





this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SearchService]);

/* vim:set ts=2 sw=2 sts=2 ci et: */
/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file contains functions that are useful for debugging purposes from
// within JavaScript code.

this.EXPORTED_SYMBOLS = ["NS_ASSERT"];

var gTraceOnAssert = true;

/**
 * This function provides a simple assertion function for JavaScript.
 * If the condition is true, this function will do nothing.  If the
 * condition is false, then the message will be printed to the console
 * and an alert will appear showing a stack trace, so that the (alpha
 * or nightly) user can file a bug containing it.  For future enhancements, 
 * see bugs 330077 and 330078.
 *
 * To suppress the dialogs, you can run with the environment variable
 * XUL_ASSERT_PROMPT set to 0 (if unset, this defaults to 1).
 *
 * @param condition represents the condition that we're asserting to be
 *                  true when we call this function--should be
 *                  something that can be evaluated as a boolean.
 * @param message   a string to be displayed upon failure of the assertion
 */

this.NS_ASSERT = function NS_ASSERT(condition, message) {
  if (condition)
    return;

  var releaseBuild = true;
  var defB = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefService)
                       .getDefaultBranch(null);
  try {
    switch (defB.getCharPref("app.update.channel")) {
      case "nightly":
      case "beta":
      case "default":
        releaseBuild = false;
    }
  } catch(ex) {}

  var caller = arguments.callee.caller;
  var assertionText = "ASSERT: " + message + "\n";

  if (releaseBuild) {
    // Just report the error to the console
    Components.utils.reportError(assertionText);
    return;
  }

  // Otherwise, dump to stdout and launch an assertion failure dialog
  dump(assertionText);

  var stackText = "";
  if (gTraceOnAssert) {
    stackText = "Stack Trace: \n";
    var count = 0;
    while (caller) {
      stackText += count++ + ":" + caller.name + "(";
      for (var i = 0; i < caller.arguments.length; ++i) {
        var arg = caller.arguments[i];
        stackText += arg;
        if (i < caller.arguments.length - 1)
          stackText += ",";
      }
      stackText += ")\n";
      caller = caller.arguments.callee.caller;
    }
  }

  var environment = Components.classes["@mozilla.org/process/environment;1"].
                    getService(Components.interfaces.nsIEnvironment);
  if (environment.exists("XUL_ASSERT_PROMPT") &&
      !parseInt(environment.get("XUL_ASSERT_PROMPT")))
    return;

  var source = null;
  if (this.window)
    source = this.window;
  var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].
           getService(Components.interfaces.nsIPromptService);
  ps.alert(source, "Assertion Failed", assertionText + stackText);
}


