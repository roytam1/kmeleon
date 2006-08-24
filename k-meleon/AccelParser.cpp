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

#include "StdAfx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "AccelParser.h"
#include "Utils.h"
#include "resource.h"

#include "Plugins.h"
#include "kmeleon_plugin.h"

#define BEGIN_VK_TEST if (0){}
#define VK_TEST(KEY)  else if (stricmp(p, #KEY) == 0){ key = VK_##KEY; }

// a few that winuser.h doesn't have for some reason (snagged from msdn)
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
// and some more friendly definitions...
#define VK_PLUS VK_OEM_PLUS
#define VK_EQUALS VK_OEM_PLUS
#define VK_MINUS VK_OEM_MINUS
#define VK_COMMA VK_OEM_COMMA
#define VK_PERIOD VK_OEM_PERIOD
#define VK_PERIOD VK_OEM_PERIOD
#define VK_PAGE_UP VK_PRIOR
#define VK_PAGE_DOWN VK_NEXT 

//Standard accel to show in menu 

static const ACCEL stdAccels[] = {
	{FCONTROL, 'C', ID_EDIT_COPY},
	{FCONTROL, 'V', ID_EDIT_PASTE},
	{FCONTROL, 'X', ID_EDIT_CUT},
	{FVIRTKEY|FALT, VK_F4, ID_FILE_CLOSE}
};


CAccelParser::CAccelParser()
{
   accelTable = NULL;
   numAccelerators = 0;
   memset(accelerators, 0, sizeof(ACCEL) * MAX_ACCEL);
   numMKeys = 0;
   memset(mouse, 0, sizeof(ACCEL) * MAX_MOUSE);
}

CAccelParser::CAccelParser(LPCTSTR filename)
{
   accelTable = NULL;
   numAccelerators = 0;
   memset(accelerators, 0, sizeof(ACCEL) * MAX_ACCEL);

   Load(filename);
}

CAccelParser::~CAccelParser()
{
   if (accelTable){
      DestroyAcceleratorTable(accelTable);
   }
}

int CAccelParser::Load(LPCTSTR filename)
{
   SETUP_LOG("Accel");

   if (accelTable){
      DestroyAcceleratorTable(accelTable);
      accelTable = NULL;
      numAccelerators = 0;
   }

   int retVal = CParser::Load(filename);

   END_LOG();

   return retVal;
}

int CAccelParser::Parse(char *p)
{
   char *e, *s;
   char *alt, *ctrl, *shift;
   BYTE virt;
   int command;
   int key;
   BOOL vkey;

   // <modifiers> <key> = <command>
   e = strchr(p, '=');
   if (e){
      *e = 0;
      e++;
      e = SkipWhiteSpace(e);
      TrimWhiteSpace(e);

      char *op = strchr(e, '(');
      if (op) {                        // if there's an open parenthesis, we'll assume it's a plugin
         char *parameter = op + 1;
         char *cp = strrchr(parameter, ')');
         if (cp) *cp = 0;
         *op = 0;

         TrimWhiteSpace(e);

         if (theApp.plugins.SendMessage(e, "* AccelParser", "DoAccel", (long)parameter, (long)&command)) {
            LOG_2("Called plugin %s with parameter %s", e, parameter);
         }
         else {
            LOG_ERROR_2( "Plugin %s has no accelerator %s", e, parameter);
         }
      }
      
      else {
         command = theApp.GetID(e);
         if (!command)
            command = atoi(e);
      }

      TrimWhiteSpace(p);
      s = p;

      virt = 0;
      alt = strstr(s, "ALT");
      if (alt){
         virt |= FALT;
         p = alt + 3;
      }
      ctrl = strstr(s, "CTRL");
      if (ctrl){
         virt |= FCONTROL;
         if (ctrl > alt){
            p = ctrl + 4;
         }
      }
      shift = strstr(s, "SHIFT");
      if (shift){
         virt |= FSHIFT;
         if ((shift > alt) && (shift > ctrl)){
            p = shift + 5;
         }
      }
      virt |= FVIRTKEY;
      // by now, p should be past the modifiers and point to " <key>"
      p = SkipWhiteSpace(p);

      if (strncmp(p, "VK_", 3) == 0){
         p+=3;
         key = 0;
         vkey = TRUE;

         // these should be in order of frequency of use to speed up parsing
         BEGIN_VK_TEST

            VK_TEST(ESCAPE)
            VK_TEST(LEFT)
            VK_TEST(RIGHT)

            VK_TEST(F1)
            VK_TEST(F2)
            VK_TEST(F3)
            VK_TEST(F4)
            VK_TEST(F5)
            VK_TEST(F6)
            VK_TEST(F7)
            VK_TEST(F8)
            VK_TEST(F9)
            VK_TEST(F10)
            VK_TEST(F11)
            VK_TEST(F12)

            VK_TEST(HOME)
            VK_TEST(END)

            VK_TEST(PRIOR)  // page up
            VK_TEST(NEXT)   // page down
            VK_TEST(PAGE_UP)
            VK_TEST(PAGE_DOWN) 
            VK_TEST(UP)
            VK_TEST(DOWN)
            VK_TEST(INSERT)
            VK_TEST(DELETE)
            VK_TEST(SPACE)
            VK_TEST(HELP)
            VK_TEST(EXECUTE)
            VK_TEST(SELECT)
            VK_TEST(PRINT)
            VK_TEST(SNAPSHOT) // print screen?

            VK_TEST(PLUS)
            VK_TEST(MINUS)
            VK_TEST(COMMA)
            VK_TEST(PERIOD)
            VK_TEST(EQUALS)

            VK_TEST(BACK)
            VK_TEST(TAB)
            VK_TEST(CLEAR)

            VK_TEST(RETURN)

            VK_TEST(MULTIPLY)
            VK_TEST(ADD)
            VK_TEST(SUBTRACT)
            VK_TEST(DECIMAL)
            VK_TEST(DIVIDE)
            VK_TEST(SEPARATOR)

            VK_TEST(PAUSE)
            VK_TEST(CAPITAL)
            VK_TEST(MENU)

	   /*
            VK_TEST(KANA)
            VK_TEST(JUNJA)
            VK_TEST(FINAL)
            VK_TEST(HANJA)
            VK_TEST(KANJI)

            VK_TEST(CONVERT)
            VK_TEST(NONCONVERT)
            VK_TEST(ACCEPT)
            VK_TEST(MODECHANGE)
	   */

            VK_TEST(LWIN)
            VK_TEST(RWIN)
            VK_TEST(APPS)

            VK_TEST(NUMPAD0)
            VK_TEST(NUMPAD1)
            VK_TEST(NUMPAD2)
            VK_TEST(NUMPAD3)
            VK_TEST(NUMPAD4)
            VK_TEST(NUMPAD5)
            VK_TEST(NUMPAD6)
            VK_TEST(NUMPAD7)
            VK_TEST(NUMPAD8)
            VK_TEST(NUMPAD9)

	   /*
            VK_TEST(F13)
            VK_TEST(F14)
            VK_TEST(F15)
            VK_TEST(F16)
            VK_TEST(F17)
            VK_TEST(F18)
            VK_TEST(F19)
            VK_TEST(F20)
            VK_TEST(F21)
            VK_TEST(F22)
            VK_TEST(F23)
            VK_TEST(F24)
	   */

            VK_TEST(NUMLOCK)
            VK_TEST(SCROLL)
      }
      else if (stricmp(p, "LButton") == 0){ 
	mouse[numMKeys].cmd = command;
	mouse[numMKeys].fVirt = virt & (~FVIRTKEY);
	mouse[numMKeys].key = WM_LBUTTONDOWN;
	numMKeys++;
	return true;
      }
      else if (stricmp(p, "MButton") == 0){ 
	mouse[numMKeys].cmd = command;
	mouse[numMKeys].fVirt = virt & (~FVIRTKEY);
	mouse[numMKeys].key = WM_MBUTTONDOWN;
	numMKeys++;
	return true;
      }
      else if (stricmp(p, "RButton") == 0){ 
	mouse[numMKeys].cmd = command;
	mouse[numMKeys].fVirt = virt & (~FVIRTKEY);
	mouse[numMKeys].key = WM_RBUTTONDOWN;
	numMKeys++;
	return true;
      }
      else {
         // regular key...
         key = (WORD)*p;
         vkey = FALSE;
      }

      accelerators[numAccelerators].cmd = command;
      accelerators[numAccelerators].fVirt = virt;
      if ( vkey || ((key >= 'A') && (key <= 'Z')) || ((key >= 'a') && (key <= 'z')) )
         accelerators[numAccelerators].key = key;
      else
         accelerators[numAccelerators].key = VkKeyScan(key);
      numAccelerators++;
   } // if e

   return true;
}

HACCEL CAccelParser::GetTable(){
   if (!accelTable){
      accelTable = CreateAcceleratorTable(accelerators, numAccelerators);
   }
   return accelTable;
}

CString CAccelParser::GetStrAccel(UINT id)
{
	const ACCEL* accel = NULL;

	for (int i=0;i<numAccelerators;i++) {
		if (accelerators[i].cmd == id) {
			accel = &accelerators[i];
			break;
		}
	}

	if (!accel) {
		for (int i=0;i<sizeof(stdAccels)/sizeof(ACCEL);i++) {
			if (stdAccels[i].cmd == id) {
				accel = &stdAccels[i];
				break;
			}
		}
	}

	if (accel) {
		CString str;
		TCHAR buf[25];
		if(accel->fVirt & FCONTROL) {
			GetKeyNameText(MapVirtualKey(VK_CONTROL, 0)<<16, buf, sizeof(buf));
			if (*buf) {
				CharLowerBuff(buf+1, _tcslen(buf)-1);
				str = str + buf + _T("+");
			}
		}
		if(accel->fVirt & FSHIFT) {
			GetKeyNameText(MapVirtualKey(VK_SHIFT, 0)<<16, buf, sizeof(buf));
			if (*buf) {
				CharLowerBuff(buf+1, _tcslen(buf)-1);
				str = str + buf + _T("+");
			}
		}
		if(accel->fVirt & FALT) {
			GetKeyNameText(MapVirtualKey(VK_MENU, 0)<<16, buf, sizeof(buf));
			if (*buf) {
				CharLowerBuff(buf+1, _tcslen(buf)-1);
				str = str + buf + _T("+");
			}
		}
		if(accel->fVirt & FVIRTKEY) {
			WORD key = accel->key;
			UINT scan = MapVirtualKey(key, 0);
			if (key == VK_INSERT || 
				key == VK_DELETE ||
				key == VK_HOME ||
				key == VK_END ||
				key == VK_NEXT ||
				key == VK_PRIOR ||
				key == VK_LEFT ||
				key == VK_RIGHT ||
				key == VK_UP ||
				key == VK_DOWN)
				scan |= 0x100;

			if (GetKeyNameText(scan<<16 , buf, sizeof(buf)) == 0)
				return _T("");
			if (*buf) {
				CharLowerBuff(buf+1, _tcslen(buf)-1);
				str += buf;
			}
		}
		else {
			char c[2] = { accel->key, '\0' };
			USES_CONVERSION;
			str += A2CT(c);
		}
		return str;
	}

	return _T("");
}

int CAccelParser::CheckMouse(UINT message){
   int i, virt = 0;
   if (GetKeyState(VK_SHIFT) < 0)
      virt |= FSHIFT;
   if (GetKeyState(VK_CONTROL) < 0)
      virt |= FCONTROL;
   if (GetKeyState(VK_MENU) < 0)
      virt |= FALT;
   for (i=0; i<numMKeys; i++) {
      if (mouse[i].key == message) {
	 if (mouse[i].fVirt == virt)
	    return mouse[i].cmd;
      }
   }
   return 0;
}
