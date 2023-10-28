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

/**/
// From https://stackoverflow.com/a/16472369
// Example of __DATE__ string: "Jul 27 2012"
//                              01234567890

#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])


#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')


#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])
/**/

#define KMELEON_VERSION	               0x02030050
//#define MOZILLA_BUILDID 20140105 // Not used now, use mozilla_buildid[]
#define KMELEON_BUILDID 7650
#define KMELEON_UVERSION 76.5.0
#define KMELEON_UUID {944df56f-c0ec-4d7b-94b6-f625c8cc96ea}

#define WM_DEFERSHOW                   WM_USER + 200
#define WM_DEFERSAVEAS                 WM_USER + 201
#define WM_DEFERMENU                   WM_USER + 202

#define UWM_UPDATESESSIONHISTORY       WM_APP + 110
#define UWM_REFRESHTOOLBARITEM         WM_APP + 111
#define UWM_UPDATEBUSYSTATE            WM_APP + 112
#define UWM_REFRESHMRULIST             WM_APP + 113

#define UWM_NEWWINDOW                  WM_APP + 115

#define UWM_PERSIST_SET                WM_APP + 1116
#define UWM_PERSIST_GET                WM_APP + 1117
#define UWM_PERSIST_SHOW               WM_APP + 1118

#define TB_LBUTTONDOWN                 WM_APP + 120
#define TB_MBUTTONDOWN                 WM_APP + 121
#define TB_RBUTTONDOWN                 WM_APP + 122

#define TB_LBUTTONHOLD                 WM_APP + 123
#define TB_MBUTTONHOLD                 WM_APP + 124
#define TB_RBUTTONHOLD                 WM_APP + 125

#define TB_LBUTTONUP                   WM_APP + 126
#define TB_MBUTTONUP                   WM_APP + 127
#define TB_RBUTTONUP                   WM_APP + 128

#define TB_LBUTTONDBLCLK               WM_APP + 129
#define TB_MBUTTONDBLCLK               WM_APP + 130
#define TB_RBUTTONDBLCLK               WM_APP + 131

#define SB_LBUTTONDBLCLK               WM_APP + 132
#define SB_MBUTTONDBLCLK               WM_APP + 133
#define SB_RBUTTONDBLCLK               WM_APP + 134
#define SB_LBUTTONDOWN                 WM_APP + 135
#define SB_MBUTTONDOWN                 WM_APP + 136
#define SB_RBUTTONDOWN                 WM_APP + 137
#define SB_LBUTTONUP                   WM_APP + 138
#define SB_MBUTTONUP                   WM_APP + 139
#define SB_RBUTTONUP                   WM_APP + 141

#define UWM_NEWSITEICON                WM_APP + 140

#define PLUGIN_COMMAND_START_ID        6000
#define PLUGIN_REBAR_START_ID          200

#define WINDOW_MENU_START_ID           1500 // 500 windows is more than enough :p
#define WINDOW_MENU_STOP_ID            1999
#define TOOLBAR_MENU_START_ID          2000
#define TOOLBAR_MENU_END_ID            TOOLBAR_MENU_START_ID+49  // this limits us to 50 toolbars, should be enough :) 
#define SIDEBAR_MENU_START_ID          2050
#define SIDEBAR_MENU_END_ID            SIDEBAR_MENU_START_ID+49 
#define BAND_BASE_ID                   200

#define MAX_SHMENU_NUMBER              50
#define SHISTORYB_START_ID             2400
#define SHISTORYB_END_ID			   SHISTORYB_START_ID + MAX_SHMENU_NUMBER - 1
#define SHISTORYF_START_ID             2450
#define SHISTORYF_END_ID               SHISTORYF_START_ID + MAX_SHMENU_NUMBER - 1

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
