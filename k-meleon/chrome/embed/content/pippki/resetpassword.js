const nsPK11TokenDB = "@mozilla.org/security/pk11tokendb;1";
const nsIPK11TokenDB = Components.interfaces.nsIPK11TokenDB;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
var tokenName;
function onLoad()
{
  if ("arguments" in window) {
    var params = window.arguments[0].QueryInterface(nsIDialogParamBlock);
    tokenName = params.GetString(1);
  } else {
    tokenName = self.name;
  }
}
function resetPassword()
{
  var pk11db = Components.classes[nsPK11TokenDB].getService(nsIPK11TokenDB);
  var token = pk11db.findTokenByName(tokenName);
  token.reset();
  var pref = Components.classes['@mozilla.org/preferences-service;1'].getService(Components.interfaces.nsIPrefService);
  if (pref) {
    pref = pref.getBranch(null);
    try {
      if (pref.getBoolPref("wallet.crypto")) {
        var wallet = Components.classes['@mozilla.org/wallet/wallet-service;1'];
        if (wallet) {
          wallet = wallet.getService(Components.interfaces.nsIWalletService);
          wallet.WALLET_DeleteAll();
        }
      }
    }
    catch(e) {
    }
  }
  var bundle = document.getElementById("pippki_bundle");
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
  promptService = promptService.QueryInterface(Components.interfaces.nsIPromptService);
  if (promptService && bundle) {
    promptService.alert(window,
      bundle.getString("resetPasswordConfirmationTitle"), 
      bundle.getString("resetPasswordConfirmationMessage"));
  }
  return true;
}
