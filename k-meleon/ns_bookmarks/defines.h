/*
*  Copyright (C) 2000 Brian Harris
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define BOOKMARKS_TITLE_LEN 64

#define _T(x) TEXT(x)
#define _Q(x) #x

#define PLUGIN_NAME "Netscape Bookmark Plugin"

#define PREFERENCE_BOOKMARK_FILE   "kmeleon.plugins.bookmarks.bookmarkFile"
#define PREFERENCE_TOOLBAR_FOLDER  "kmeleon.plugins.bookmarks.toolbarFolder"
#define PREFERENCE_TOOLBAR_ENABLED "kmeleon.plugins.bookmarks.toolbarEnabled"
#define PREFERENCE_MAX_MENU_LENGTH "kmeleon.plugins.bookmarks.maxMenuLength"
#define PREFERENCE_MENU_AUTODETECT "kmeleon.plugins.bookmarks.menuAutoDetect"
#define PREFERENCE_MAX_TB_SIZE     "kmeleon.plugins.bookmarks.maxToolbarSize"
#define PREFERENCE_SETTINGS_DIR    "kmeleon.general.settingsDir"

#define BOOKMARK_TAG "<!DOCTYPE NETSCAPE-Bookmark-file-1>"
#define KMELEON_TAG "<!-- Generated By KMeleon -->"
#define COMMENT_TAG "<!-- This is an automatically generated file.\nIt will be read and overwritten.\nDo Not Edit! -->"
#define CONTENT_TYPE_TAG "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">"

#define BOOKMARKS_DEFAULT_FILENAME "bookmarks.html"
#define BOOKMARKS_FILTER "Bookmark Files\0bookmark.htm;bookmarks.html;bookmark.htm\0HTML Files\0*.htm;*.html\0"
#define BOOKMARKS_DEFAULT_TITLE "Bookmarks"
#define BOOKMARKS_NOT_FOUND "Your existing bookmarks file could not be found.\n\n" \
                            "Would you like to locate this file now?\n\n" \
                            "(Press No to create a new bookmarks file)"
#define BOOKMARKS_CREATING_NEW "K-Meleon will create a new, empty bookmark file for you"
#define BOOKMARKS_NOT_BY_US "The Bookmarks file was not created by this plugin.  Would you like to save your changes?"
#define BOOKMARKS_SAVE_CHANGES "Would you like to save your changes?"

#define TOOLBAND_NAME "Bookmarks"
#define TOOLBAND_FAILED_TO_CREATE "Failed to create bookmark toolbar"

#define IMAGE_BLANK         -1
#define IMAGE_FOLDER_CLOSED 0
#define IMAGE_FOLDER_OPEN   1
#define IMAGE_BOOKMARK      2
#define IMAGE_CHEVRON       3
#define IMAGE_FOLDER_SPECIAL_CLOSED 4
#define IMAGE_FOLDER_SPECIAL_OPEN   5

