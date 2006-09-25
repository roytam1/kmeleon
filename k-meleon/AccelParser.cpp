/*
*  Copyright (C) 2000 Brian Harris
*  Copyright (C) 2006 Dorian Boissonnade
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

extern BOOL ParsePluginCommand(char *pszCommand, char** plugin, char **parameter);

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
   // <modifiers> <key> = <command>
   char *e = strchr(p, '=');
   if (e){
      *e = 0;
      e++;
      e = SkipWhiteSpace(e);
      TrimWhiteSpace(e);
	  TrimWhiteSpace(p);
	  return SetAccel(p, e);
   }
   return 0;
}

int CAccelParser::SetAccel(const char* pKey, char* pCommand)
{
	int command = 0;
	char *alt, *ctrl, *shift;
	const char* p;
	BYTE virt;
    int key;

	char *plugin, *parameter;
	if (ParsePluginCommand(pCommand, &plugin, &parameter))
	{
		if (theApp.plugins.SendMessage(plugin, "* AccelParser", "DoAccel", (long)parameter, (long)&command)) {
			LOG_2("Called plugin %s with parameter %s", plugin, parameter);
		}
		else {
			LOG_ERROR_2( "Plugin %s has no accelerator %s", plugin, parameter);
			return 0;
		}
	}
	else {
		command = theApp.GetID(pCommand);
		if (!command)
			command = atoi(pCommand);
	}

	virt = 0;
	p = pKey;
	alt = strstr(pKey, "ALT");
	if (alt){
		virt |= FALT;
		p = alt + 3;
	}
	ctrl = strstr(pKey, "CTRL");
	if (ctrl){
		virt |= FCONTROL;
		if (ctrl > alt){
			p = ctrl + 4;
		}
	}
	shift = strstr(pKey, "SHIFT");
	if (shift){
		virt |= FSHIFT;
		if ((shift > alt) && (shift > ctrl)){
			p = shift + 5;
		}
	}

      // by now, p should be past the modifiers and point to " <key>"
      p = SkipWhiteSpace((char*)p);

      if (strncmp(p, "VK_", 3) == 0){
         p+=3;
         key = 0;

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
         SetMAccel(command, virt, WM_LBUTTONDOWN);
         return true;
      }
      else if (stricmp(p, "MButton") == 0){ 
         SetMAccel(command, virt, WM_MBUTTONDOWN);
         return true;
      }
      else if (stricmp(p, "RButton") == 0){ 
         SetMAccel(command, virt, WM_RBUTTONDOWN);
         return true;
      }
      else {
         // regular key, convert it to virtual-key code to 
         // get the associated key on the keyboard.
         key = VkKeyScanA(*p) & 0xff;
      }

      virt |= FVIRTKEY;
      SetAccel(command, virt, key);

   return true;
}

void CAccelParser::SetAccel(WORD command, WORD virt, WORD key)
{
   int oldAccel;
   if ((oldAccel = FindAccel(virt, key)) == -1) {
      accelerators[numAccelerators].cmd = command;
      accelerators[numAccelerators].fVirt = virt;
      accelerators[numAccelerators].key = key;
      numAccelerators++;
   }
   else {
      if (command != 0) 
         accelerators[oldAccel].cmd = command;
      else
         DeleteAccel(oldAccel);
   }
   if (accelTable) {
	   DestroyAcceleratorTable(accelTable);
	   accelTable = NULL;
   }
}

void CAccelParser::SetMAccel(WORD command, WORD virt, WORD button)
{
   if (command == 0) {
      int oldAccel = FindMAccel(virt, button);
      if (oldAccel != -1)
         DeleteMAccel(oldAccel);
   }
   mouse[numMKeys].cmd = command;
   mouse[numMKeys].fVirt = virt;
   mouse[numMKeys].key = button;
   numMKeys++;
}

void CAccelParser::DeleteMAccel(int idx) 
{
   int i;
   --numMKeys;
   for (i=idx;i<numMKeys;i++)
      mouse[i] = mouse[i+1];
}

void CAccelParser::DeleteAccel(int idx) 
{
   int i;
   --numAccelerators;
   for (i=idx;i<numAccelerators;i++)
      accelerators[i] = accelerators[i+1];
}

int CAccelParser::FindMAccel(WORD virt, DWORD key)
{
   const ACCEL* accel = NULL;

   for (int i=0;i<numAccelerators;i++)
      if (mouse[i].key == key && mouse[i].fVirt == virt)
         return i;

   return -1;
}

int CAccelParser::FindAccel(WORD virt, DWORD key)
{
   const ACCEL* accel = NULL;

   for (int i=0;i<numAccelerators;i++)
      if (accelerators[i].key == key && accelerators[i].fVirt == virt)
         return i;

   return -1;
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
				key == VK_DOWN ||
				key == VK_ADD ||
				key == VK_SUBTRACT)
				scan |= 0x100; // should remove the numpad text

			if (GetKeyNameText(scan<<16 , buf, sizeof(buf)) == 0)
				return _T("");

			// Since I can't find a way to remove the numpad indication
			// for /,* or . I'm removing it the hard way.
			TCHAR* parenthesis = _tcschr(buf, '(');
			if (parenthesis && parenthesis>buf)
				*parenthesis = 0;

			if (*buf) {
				CharLowerBuff(buf+1, _tcslen(buf)-1);
				str += buf;
			}
		}
		else {
			char c[2] = { accel->key & 0xff, '\0' };
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
