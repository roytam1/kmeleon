K-Meleon
--------

K-Meleon (c) 2000 Christophe Thibault (http://www.kmeleon.org/). All rights reserved.

K-Meleon uses some bits from :
SuperPimp(tm) Installer (c) 2000 Nullsoft
Gecko and System Icon from Mozilla (http://www.mozilla.org/)
Little AVI animation borrowed from the Galeon project (http://galeon.sourceforge.net/) (c) Frédéric Toussaint

About:
------

K-Meleon is the Windows answer to Galeon. Thus, K-Meleon is a lite Web browser based on gecko 
(the mozilla rendering engine). It's fast, it has a light interface, and it is fully 
standards-compliant. To make it simple, K-Meleon could be considered as the unbloated Mozilla 
version for Windows.

K-Meleon interface tries to mimic the IE MFC interface as much as possible. For convenience, 
it also uses the IE bookmarking system.

K-Meleon is released under the GNU General Public License. 

For more information and support, visit http://kmeleon.org and the 
kmeleon forum at http://kmeleon.org/forum. The developers mailinglist can be reached at:
mailto:kmeleon-dev@lists.sourceforge.net.

The public PGP key (ID: 0x783D0587) to verify the authentity of releases can be found at: 
http://kmeleon.sourcforge.net/publicPGP.txt


ChangeLog:
----------
since then
 - Session cookies work
 - Properties->Version of k-meleon.exe contained not correct information
v0.3 (02/13/01)
 - navigating with forward/back before the site finished loading 
   leads to wrong display (2 pages on top of each other)
 - CTRL-N is now assignable as Accelerator key (Open in New Window now)
 - Plugin support for menus/toolbars
 - Scriptable menus & shortcuts
 - preferences dialog
 - replaced BCG Library by using Mozilla's new MFC-Embed code
 - closing first-window doesn't quit K-Meleon now
 - Allow basic authentication
 - View source menu entry
 - Uninstall works now, providing correct uninstall.exe now
 - fixed percentage of document loaded sometimes wrong (>100%)
 - Several Kmeleon0.21 crashes which are already fixed in the nightlies
 - Save html of a page only
 - Save complete page incl. images into a directory
 - Save images, download files with right mouse click
 - Open local files doesn't work on some systems
 - takes URL as command line option
 - mailto: links open mail app
 - wording GNU General Public License fixed
 - new bugs introduced :-)

v0.2.1 (11/27/00): Compiled with Mozilla M18 Nightly build (11/26/00)
- fixed mouse wheel crashing bug (duh).
- fixed background not erased correctly.
- Help menu will now display the correct items if you're upgrading from 0.1.

v0.2 (11/26/00): Compiled with Mozilla M18 Nightly build (11/26/00)
- Still uses BCG Library. I kinda like it. Updated to compile with 2.74.
  Panning support (3rd mouse button).
  4th and 5th mouse buttons support (back and forward).
  Right-click context menu added (Open new window, copy, paste, etc...)
  Scrollbars don't look ugly anymore.
  More toolbar buttons work.
  Lot less files to install, thanks to .jar.
  Bugfixes...

  Bugs: - There is a problem with form inputs, where it eventually doesn't accept keyboard
          input anymore. This problem is apparent too in mozilla test embed program...
        - Closing the main k-meleon browser will close all childs

v0.1 (08/21/00):
- First release by Christophe Thibault
  Lots of stuff is not implemented such as Context menu, HTTPS, History, 
  Cookies saving, Mime types handling, etc...