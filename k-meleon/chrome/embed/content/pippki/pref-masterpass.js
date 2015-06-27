const nsIPK11Token = Components.interfaces.nsIPK11Token;
const nsPK11TokenDB = "@mozilla.org/security/pk11tokendb;1";
const nsIPK11TokenDB = Components.interfaces.nsIPK11TokenDB;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsDialogParamBlock = "@mozilla.org/embedcomp/dialogparam;1";
var internal_token;
function onMasterPasswordLoad()
{
  var tokendb = Components.classes[nsPK11TokenDB].getService(nsIPK11TokenDB);
  internal_token = tokendb.getInternalKeyToken();
  var askTimes = internal_token.getAskPasswordTimes();
  switch (askTimes) {
  case nsIPK11Token.ASK_FIRST_TIME:  askTimes = 0; break;
  case nsIPK11Token.ASK_EVERY_TIME:  askTimes = 1; break;
  case nsIPK11Token.ASK_EXPIRE_TIME: askTimes = 2; break;
  }
  var radiogroup = document.getElementById("passwordAskTimes");
  var radioitem;
  switch (askTimes) {
  case 0: radioitem = document.getElementById("askFirstTime"); break;
  case 1: radioitem = document.getElementById("askEveryTime"); break;
  case 2: radioitem = document.getElementById("askTimeout"); break;
  }
  radiogroup.selectedItem = radioitem;
  var timeout = internal_token.getAskPasswordTimeout();
  var timeoutField = document.getElementById("passwordTimeout");
  timeoutField.setAttribute("value", timeout);
  changePasswordSettings(false);
}
function changePasswordSettings(setFocus)
{
  var askTimes = 0;
  var timeout = internal_token.getAskPasswordTimeout();
  var timeoutField = document.getElementById("passwordTimeout");
  var radiogroup = document.getElementById("passwordAskTimes");
  switch ( radiogroup.value ) {
  case "0": 
    timeoutField.setAttribute("disabled", true);
    askTimes = nsIPK11Token.ASK_FIRST_TIME;
    break;
  case "1": 
    timeoutField.setAttribute("disabled", true);
    askTimes = nsIPK11Token.ASK_EVERY_TIME;
    break;
  case "2":
    timeoutField.removeAttribute("disabled");
    if ( setFocus ) {
      timeoutField.focus();
    }
    timeout = timeoutField.value;
    var re = new RegExp("^[0-9]+$");
    if (!re.test(timeout)) {
      timeout = "1";
    }
    askTimes = nsIPK11Token.ASK_EXPIRE_TIME;
    break;
  }
  internal_token.setAskPasswordDefaults(askTimes, timeout);
  var askEveryTimeHidden = document.getElementById("askEveryTimeHidden");
  askEveryTimeHidden.checked = (radiogroup.value == 1) ? true : false;
}
function ChangePW()
{
  var params = Components.classes[nsDialogParamBlock].createInstance(nsIDialogParamBlock);
  params.SetString(1,"");
  window.openDialog("chrome://pippki/content/changepassword.xul","",
                    "chrome,centerscreen,modal",params);
}
function ResetPW()
{
var params = Components.classes[nsDialogParamBlock].createInstance(nsIDialogParamBlock);
  params.SetString(1,internal_token.tokenName);
  window.openDialog("chrome://pippki/content/resetpassword.xul", "",
                    "chrome,centerscreen,modal", params);
}
