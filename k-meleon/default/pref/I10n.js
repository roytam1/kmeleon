/********************************************************************************************************/
// Locale default pref for k-meleon 
/********************************************************************************************************/
pref("general.useragent.locale", "en-US");
pref("general.useragent.contentlocale", "US");

// Avoid Bug 1008
pref("kmeleon.general.homePage", "resource:///readme.html");

/********************************************************************************************************/
// Privacy

pref("kmeleon.privacy.useragent1.name", "Firefox 2.0");
pref("kmeleon.privacy.useragent1.string", "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.20) Gecko/20081217 Firefox/2.0.0.20");
pref("kmeleon.privacy.useragent2.name", "SeaMonkey 1.1");
pref("kmeleon.privacy.useragent2.string", "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.23) Gecko/20090825 SeaMonkey/1.1.18");


/********************************************************************************************************/

pref("intl.charset.detector", "universal_charset_detector");

// Make sure this engine is defined in search.xml!
pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&sourceid=navclient&gfns=1&q=");


/********************************************************************************************************/
// Locale default pref for k-meleon macros
/********************************************************************************************************/

// Domain completion
pref("kmeleon.plugins.macros.domComplete0.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete0.suffix", ".com");
pref("kmeleon.plugins.macros.domComplete1.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete1.suffix", ".net");
pref("kmeleon.plugins.macros.domComplete2.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete2.suffix", ".org");

// URL Bar access key: Alt+? (set to the key used in IE)
pref("kmeleon.plugins.macros.accel.urlbar.access", "D");


/********************************************************************************************************/
// Web Search

// Default Search Engine URL  (MUST be one out of kmeleon.plugins.macros.search.engine[0..?].url)
pref("kmeleon.general.searchEngine", "http://www.google.com/search?q=");
// Default Search Engine Name (MUST be one out of kmeleon.plugins.macros.search.engine[0..?].name)
pref("kmeleon.general.searchEngineName", "Google");

// K-Meleon Forums Search
pref("kmeleon.plugins.macros.search.kmforums", "http://kmeleon.sourceforge.net/forum/search.php?forum_id=0&match_forum=ALL&match_dates=0&match_type=ALL&search=");

// Web Search (engine[0..POSITIVE_INFINITY] possible)
// Make sure these engines are defined in search.xml!
pref("kmeleon.plugins.macros.search.engine0.name", "Google");
pref("kmeleon.plugins.macros.search.engine0.url", "http://www.google.com/search?q=");
pref("kmeleon.plugins.macros.search.engine1.name", "MSN Search");
pref("kmeleon.plugins.macros.search.engine1.url", "http://search.msn.com/results.aspx?q=");
pref("kmeleon.plugins.macros.search.engine2.name", "Yahoo! Search");
pref("kmeleon.plugins.macros.search.engine2.url", "http://search.yahoo.com/bin/search?p=");
pref("kmeleon.plugins.macros.search.engine2.name", "Wikipedia, The Free Encyclopedia");
pref("kmeleon.plugins.macros.search.engine2.url", "http://en.wikipedia.org/w/index.php?title=Special%3ASearch&fulltext=Search&search=");

// Metasearch (meta[0..POSITIVE_INFINITY] possible)
// Make sure these engines are defined in search.xml!
pref("kmeleon.plugins.macros.search.meta0.url", "http://www.google.com/search?num=100&q=");
pref("kmeleon.plugins.macros.search.meta1.url", "http://search.msn.com/results.aspx?q=");
pref("kmeleon.plugins.macros.search.meta2.url", "http://search.yahoo.com/bin/search?p=");
