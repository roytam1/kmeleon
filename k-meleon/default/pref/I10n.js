/***********************************************************************/
// Locale default pref for k-meleon 
/***********************************************************************/


/***********************************************************************/
// Appearance

pref("kmeleon.display.title", "K-Meleon");
pref("kmeleon.display.URLbarTitle", "URL:");

pref("kmeleon.plugins.favorites.title", "Links:");
pref("kmeleon.plugins.favorites.toolbarFolder", "Links");

pref("kmeleon.plugins.layers.title", "Layers:");


/***********************************************************************/
// Privacy

pref("kmeleon.privacy.useragent0.name", "K-Meleon 1.0 (Default)");
pref("kmeleon.privacy.useragent0.string", "");
pref("kmeleon.privacy.useragent1.name", "MSIE 6.0");
pref("kmeleon.privacy.useragent1.string", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)");
pref("kmeleon.privacy.useragent2.name", "Netscape 8.1");
pref("kmeleon.privacy.useragent2.string", "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.5) Gecko/20060127 Netscape/8.1");
pref("kmeleon.privacy.useragent3.name", "Opera 7.54");
pref("kmeleon.privacy.useragent3.string", "Opera/7.54 (Windows NT 5.1; U) [en]");


/***********************************************************************/

pref("intl.charset.detector", "universal_charset_detector");
pref("keyword.URL", "http://www.google.com/search?gfns=1&q=");



/***********************************************************************/
// Locale default pref for k-meleon macros
/***********************************************************************/

// Custom Proxies
pref("kmeleon.plugins.macros.proxy1.name", "Custom Proxy 1");
pref("kmeleon.plugins.macros.proxy2.name", "Custom Proxy 2");
pref("kmeleon.plugins.macros.proxy3.name", "Custom Proxy 3");

// Domain completion
pref("kmeleon.plugins.macros.domComplete0.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete0.suffix", ".com");
pref("kmeleon.plugins.macros.domComplete1.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete1.suffix", ".net");
pref("kmeleon.plugins.macros.domComplete2.prefix", "www.");
pref("kmeleon.plugins.macros.domComplete2.suffix", ".org");


/***********************************************************************/
// Web Search

// Default Search Engine URL  (MUST be one out of kmeleon.plugins.macros.search.engine[0..9].url)
pref("kmeleon.general.searchEngine", "http://www.google.com/search?q=");
// Default Search Engine Name (MUST be one out of kmeleon.plugins.macros.search.engine[0..9].name)
pref("kmeleon.general.searchEngineName", "Google");

// K-Meleon Forums Search
pref("kmeleon.plugins.macros.search.kmforums", "http://kmeleon.sourceforge.net/forum/search.php?f=3&globalsearch=1&match=1&date=0&fldsubject=1&fldbody=1&search=");

// Web Search (engine[0..9] REQUIRED as far as macros are present in menus)
pref("kmeleon.plugins.macros.search.engine0.name", "Google");
pref("kmeleon.plugins.macros.search.engine0.url", "http://www.google.com/search?q=");
pref("kmeleon.plugins.macros.search.engine1.name", "Google Images");
pref("kmeleon.plugins.macros.search.engine1.url", "http://images.google.com/images?btnG=Google+Search&safe=off&filter=0&q=");
pref("kmeleon.plugins.macros.search.engine2.name", "Google Groups");
pref("kmeleon.plugins.macros.search.engine2.url", "http://groups.google.co.uk/groups?hl=en&btnG=Google+Search&q=");
pref("kmeleon.plugins.macros.search.engine3.name", "LEO English-German Dictionary");
pref("kmeleon.plugins.macros.search.engine3.url", "http://dict.leo.org/ende?lp=ende&lang=en&searchLoc=0&cmpType=relaxed&sectHdr=on&spellToler=on&relink=on&search=");
pref("kmeleon.plugins.macros.search.engine4.name", "MSN Search");
pref("kmeleon.plugins.macros.search.engine4.url", "http://search.msn.com/results.aspx?q=");
pref("kmeleon.plugins.macros.search.engine5.name", "OneLook Dictionary Search");
pref("kmeleon.plugins.macros.search.engine5.url", "http://www.onelook.com/?w=");
pref("kmeleon.plugins.macros.search.engine6.name", "Wikipedia, The Free Encyclopedia");
pref("kmeleon.plugins.macros.search.engine6.url", "http://en.wikipedia.org/wiki/Special:Search/");
pref("kmeleon.plugins.macros.search.engine7.name", "Yahoo! Search");
pref("kmeleon.plugins.macros.search.engine7.url", "http://search.yahoo.com/bin/search?p=");
pref("kmeleon.plugins.macros.search.engine8.name", "Google Scholar");
pref("kmeleon.plugins.macros.search.engine8.url", "http://www.scholar.google.com/scholar?q=");
pref("kmeleon.plugins.macros.search.engine9.name", "Internet Movie Database (English)");
pref("kmeleon.plugins.macros.search.engine9.url", "http://www.imdb.com/find?q=");

// Metasearch (meta[0..9] possible)
pref("kmeleon.plugins.macros.search.meta0.url", "http://www.google.com/search?num=100&q=");
pref("kmeleon.plugins.macros.search.meta1.url", "http://search.msn.com/results.aspx?q=");
pref("kmeleon.plugins.macros.search.meta2.url", "http://search.yahoo.com/bin/search?p=");
pref("kmeleon.plugins.macros.search.meta3.url", "http://images.google.com/images?btnG=Google+Search&safe=off&filter=0&q=");
pref("kmeleon.plugins.macros.search.meta4.url", "http://s.teoma.com/search?q=");
pref("kmeleon.plugins.macros.search.meta5.url", "http://www.gigablast.com/search?q=");
