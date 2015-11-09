 ___________________________
 Privacy Plugin for K-Meleon
 ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 Version 0.0.3
 Copyright 2004 (c) Romain Vallet



 Instructions
 ¯¯¯¯¯¯¯¯¯¯¯¯
 The Privacy Plugin allows you to automatically clear informations like
 cookies, cache, history, address bar history and sign on data (logins
 and passwords) whenever K-Meleon starts up or shuts down.
 
 This plugin is in a very early stage and has not been thoroughly tested
 so far. You should backup your profile directory before using it. It
 has been tested with K-Meleon 0.8.2.
 
 To install the plugin, copy privacy.dll to <K-Meleon directory>\kplugins.
 Typically: C:\Program Files\K-Meleon\kplugins
 Then launch K-Meleon.
 
 The plugin can be configured via the Preferences dialog, in the Plugins
 section.



 Config Files
 ¯¯¯¯¯¯¯¯¯¯¯¯
 Here are the syntaxes to add Privacy functions calls in the menus,
 accelerators and toolbars. In the functions names, MRU (Most Recently Used)
 stands for Address bar history and Signon for Saved Passwords.
 
	Menus
	¯¯¯¯¯
	------Before Main{}------

		%ifplugin privacy
		&Privacy {
			privacy(ClearCookies, Clear C&ookies)
			privacy(ClearHistory, Clear &History)
			privacy(ClearCache, Clear &Disk cache)
			privacy(ClearMRU, Clear &Address bar history)
			privacy(ClearSignon, Clear Saved &passwords)
			-
			privacy(Config, &Config)
			privacy()
		}
		%endif

	------In Main{}------
	
		%ifplugin privacy
			:&Privacy
		%endif


	Toolbars & Accelerators
	¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
		privacy(clearcookies)
		privacy(clearhistory)
		privacy(clearcache)
		privacy(clearmru)
		privacy(clearsignon)

	Macros
	¯¯¯¯¯¯
		plugin(privacy, ClearCookies)
		plugin(privacy, ClearHistory)
		plugin(privacy, ClearCache)
		plugin(privacy, ClearMRU)
		plugin(privacy, ClearSignon)



 Technical issues
 ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 The history can't be fully cleared at shutdown. The reason for that is
 that K-Meleon saves the history file after all the plugins are terminated.
 As a result the history for the current session can't be deleted.
 For the same reason, the cache can be cleared only partially at shutdown.



 Contact
 ¯¯¯¯¯¯¯
 Romain Vallet <rom@jalix.org>
 This plugin and its sources can be downloaded there:
    http://rom.jalix.org/kmeleon/



 History
 ¯¯¯¯¯¯¯
 2004-04-27 - Version 0.0.3
    * Main functions can now be called via macros,
	  accelerators, toolbars and menus

 2004-04-23 - Version 0.0.2
    * Added a configuration dialog
    * Added sign on data management
 
 2004-04-22 - Version 0.0.1
    * First public release
