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

#include "StdAfx.h"
#include "version.h"
#include "About.h"
#include "BrowserFrm.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#define _QUOTE(blah) #blah

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
   m_credits = _QUOTE(
      \r\n
      K-Meleon - Copyright 2000-2002\r\n
      \r\n
      Uses the Gecko rendering engine by the Mozilla Group\r\n
      Based on MfcEmbed\r\n
      \r\n
      Core Developers:\r\n
      Jeff Doozan <jeff@tcbnetworks.com>\r\n
      Brian Harris <binaryc@teamreaction.com>\r\n
      Mark Liffiton <liffiton@simons-rock.edu>\r\n
      \r\n
      Plugin Developers:\r\n
      Ulf Erikson <ulferikson@fastmail.fm>\r\n
      Rob Johnson <rob@rob-johnson.net>\r\n
      \r\n
      Project Documentation:\r\n
      Andrew Mutch <amutch@tln.lib.mi.us>\r\n
      \r\n
      Past Contributors:\r\n
      Sebastian Spaeth <Sebastian@SSpaeth.de>\r\n    
      Christophe Thibault <christophe@nullsoft.com>\r\n
      Chak Nanga <chak@netscape.com>\r\n
  );
   m_version.Format("Version %s Build %d Compiled %s", VERSION, BUILD_NUMBER, BUILD_TIME);
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX){
	DDX_Text(pDX, IDC_CREDITS, m_credits);
	DDX_Text(pDX, IDC_VERSION, m_version);
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CMfcEmbedApp)
	ON_COMMAND(IDC_KMELEON_HOME, OnHome)
	ON_COMMAND(IDC_KMELEON_FORUM, OnForum)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CAboutDlg::OnHome() {
   if (theApp.m_pMostRecentBrowserFrame)
      theApp.m_pMostRecentBrowserFrame->PostMessage(WM_COMMAND, ID_LINK_KMELEON_HOME);
}


void CAboutDlg::OnForum() {
   if (theApp.m_pMostRecentBrowserFrame)
      theApp.m_pMostRecentBrowserFrame->PostMessage(WM_COMMAND, ID_LINK_KMELEON_FORUM);
}
