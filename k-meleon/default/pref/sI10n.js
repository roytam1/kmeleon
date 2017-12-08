/********************************************************************************************************/
// Locale default pref for k-meleon 
/********************************************************************************************************/
//pref("general.useragent.locale", "chrome://global/locale/intl.properties");
//pref("intl.accept_languages", "chrome://global/locale/intl.properties");
//pref("intl.charset.detector", "universal_charset_detector");


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

// K-Meleon Forums Search
pref("kmeleon.plugins.macros.search.kmforums", "http://kmeleon.sourceforge.net/forum/search.php?forum_id=0&match_forum=ALL&match_dates=0&match_type=ALL&search=");


/********************************************************************************************************/
// Web Search

// Default Search Engine URL  (MUST be one out of kmeleon.plugins.macros.search.engine[0..?].url)
pref("kmeleon.general.searchEngine", "http://duckduckgo.com/?q=");
// Default Search Engine Name (MUST be one out of kmeleon.plugins.macros.search.engine[0..?].name)
pref("kmeleon.general.searchEngineName", "DuckDuckGo");


// Web Search (engine[0..POSITIVE_INFINITY] possible)
// Make sure these engines are defined in search.xml!
pref("kmeleon.plugins.macros.search.engine0.name", "DuckDuckGo");
pref("kmeleon.plugins.macros.search.engine0.url", "http://duckduckgo.com/?q=");
pref("kmeleon.plugins.macros.search.engine1.name", "Google");
pref("kmeleon.plugins.macros.search.engine1.url", "http://www.google.com/search?q=");
pref("kmeleon.plugins.macros.search.engine2.name", "Bing - Web");
pref("kmeleon.plugins.macros.search.engine2.url", "http://www.bing.com/search?q=");

// Metasearch (meta[0..POSITIVE_INFINITY] possible)
// Make sure these engines are defined in search.xml!
pref("kmeleon.plugins.macros.search.meta0.url", "http://www.google.com/search?num=100&q=");
pref("kmeleon.plugins.macros.search.meta1.url", "http://duckduckgo.com/?q=");
