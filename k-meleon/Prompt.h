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

#ifndef __PROMPT_H__
#define __PROMPT_H__

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsString.h"

#include "nsIPrompt.h"

class CPrompt : public nsIPrompt {
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPT

		CPrompt();
		virtual ~CPrompt();

  /**
     * Puts up an alert dialog with an OK button.
     */
  /* void alert (in wstring dialogTitle, in wstring text); */

  /**
     * Puts up an alert dialog with an OK button.
     */
  /* void alertCheck (in wstring dialogTitle, in wstring text, in wstring checkMsg, out boolean checkValue); */

  /**
     * Puts up a dialog with OK and Cancel buttons.
     * @return true for OK, false for Cancel
     */
  /* boolean confirm (in wstring dialogTitle, in wstring text); */

  /**
     * Puts up a dialog with OK and Cancel buttons, and
     * a message with a single checkbox.
     * @return true for OK, false for Cancel
     */
  /* boolean confirmCheck (in wstring dialogTitle, in wstring text, in wstring checkMsg, out boolean checkValue); */

  /**
     * Values for the savePassword parameter to prompt, promptPassword and
     * promptUsernameAndPassword.
     */

  /**
     * Puts up a text input dialog with OK and Cancel buttons.
     * @return true for OK, false for Cancel
     */
  /* boolean prompt (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in PRUint32 savePassword, in wstring defaultText, out wstring result); */

  /**
     * Puts up a username/password dialog with OK and Cancel buttons.
     * @return true for OK, false for Cancel
     */
  /* boolean promptUsernameAndPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in PRUint32 savePassword, out wstring user, out wstring pwd); */

  /**
     * Puts up a password dialog with OK and Cancel buttons.
     * @return true for OK, false for Cancel
     */
  /* boolean promptPassword (in wstring dialogTitle, in wstring text, in wstring passwordRealm, in PRUint32 savePassword, out wstring pwd); */

  /**
     * Puts up a dialog box which has a list box of strings
     */
  /* boolean select (in wstring dialogTitle, in wstring text, in PRUint32 count, [array, size_is (count)] in wstring selectList, out long outSelection); */

  /**
     * Put up a universal dialog
     */
  /* void universalDialog (in wstring titleMessage, in wstring dialogTitle, in wstring text, in wstring checkboxMsg, in wstring button0Text, in wstring button1Text, in wstring button2Text, in wstring button3Text, in wstring editfield1Msg, in wstring editfield2Msg, inout wstring editfield1Value, inout wstring editfield2Value, in wstring iconURL, inout boolean checkboxState, in PRInt32 numberButtons, in PRInt32 numberEditfields, in PRInt32 editField1Password, out PRInt32 buttonPressed); */

};

#endif //__PROMPT_H__