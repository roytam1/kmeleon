/*
*  Copyright (C) 2001 Jeff Doozan
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

#define UWM_UPDATESESSIONHISTORY       WM_APP + 110
#define UWM_REFRESHTOOLBARITEM         WM_APP + 111
#define UWM_UPDATEBUSYSTATE            WM_APP + 112
#define UWM_REFRESHMRULIST             WM_APP + 113

#define UWM_NEWWINDOW                  WM_APP + 115

#define UWM_PERSIST_SET                WM_APP + 1116
#define UWM_PERSIST_GET                WM_APP + 1117
#define UWM_PERSIST_SHOW               WM_APP + 1118

#define TB_LBUTTONDOWN	               WM_APP + 120
#define TB_MBUTTONDOWN	               WM_APP + 121
#define TB_RBUTTONDOWN	               WM_APP + 122

#define TB_LBUTTONHOLD                 WM_APP + 123
#define TB_MBUTTONHOLD	               WM_APP + 124
#define TB_RBUTTONHOLD	               WM_APP + 125

#define TB_LBUTTONUP	                  WM_APP + 126
#define TB_MBUTTONUP	                  WM_APP + 127
#define TB_RBUTTONUP	                  WM_APP + 128

#define TB_LBUTTONDBLCLK                  WM_APP + 129
#define TB_MBUTTONDBLCLK                  WM_APP + 130
#define TB_RBUTTONDBLCLK                  WM_APP + 131


#define PLUGIN_COMMAND_START_ID        5000
#define PLUGIN_REBAR_START_ID          200

#define TOOLBAR_MENU_START_ID          2000
#define TOOLBAR_MENU_END_ID            TOOLBAR_MENU_START_ID+50  // this limits us to 50 toolbars, should be enough :)
#define BAND_BASE_ID                   200

#define PREF_NEW_WINDOW_BLANK          0
#define PREF_NEW_WINDOW_HOME           1
#define PREF_NEW_WINDOW_CURRENT        2
#define PREF_NEW_WINDOW_URL            3


// the SHOWNOW flag is set when the loader launches kmeleon and needs a browser window immediately
#define PERSIST_BROWSER                0x0001
#define PERSIST_WINDOW                 0x0002
#define PERSIST_STARTPAGE              0x0004
#define PERSIST_SHOWNOW                0x0008

#define PERSIST_STATE_DISABLED         0
#define PERSIST_STATE_ENABLED          1
#define PERSIST_STATE_PERSISTING       2
