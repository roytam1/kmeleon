/* 2012 adodupan - set K-Meleon as default browser */
const Cc = Components.classes;
const Ci = Components.interfaces;
var dirService = Cc["@mozilla.org/file/directory_service;1"]
	.getService(Ci.nsIProperties)
	.get("GreD", Ci.nsIFile)
	.path;
var wrk = Cc["@mozilla.org/windows-registry-key;1"].createInstance(Ci.nsIWindowsRegKey);
var dBkey, dBuc = "UserChoice", dBkh = "K-MeleonHTML", dBku = "K-MeleonURL", dBpd = "Progid", dBic = "\\DefaultIcon",
dBicd = dirService + "\\k-meleon.exe,1", dBcm = "\\shell\\open\\command",
dBcmd = "\"" + dirService + "\\k-meleon.exe" + "\"" + " " + "\"" + "%1" + "\"",
dBrk = ["\\.htm", "\\.html", "\\.shtml", "\\.URL", "\\.xht", "\\.xhtml"],
dBr = ["Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",
	"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations",
	"Software\\Clients",
	"Software\\Clients\\StartMenuInternet\\K-MELEON.EXE\\Capabilities",
	"Software\\Classes",
	"Software"],
dBk = [[".htm\\" + dBuc, ".html\\" + dBuc, ".shtml\\" + dBuc, ".URL\\" + dBuc, ".xht\\" + dBuc, ".xhtml\\" + dBuc],
	["ftp\\" + dBuc, "gopher\\" + dBuc, "http\\" + dBuc, "https\\" + dBuc],
	["StartMenuInternet", "StartMenuInternet\\K-MELEON.EXE", "StartMenuInternet\\K-MELEON.EXE\\Capabilities", "StartMenuInternet\\K-MELEON.EXE\\DefaultIcon", "StartMenuInternet\\K-MELEON.EXE\\Shell\\Open\\Command"],
	["FileAssociations", "Startmenu", "UrlAssociations"],
	[".htm", ".html", ".shtml", ".URL", ".xht", ".xhtml", "ftp", "ftp" + dBic, "ftp" + dBcm, "gopher", "gopher" + dBic, "gopher" + dBcm, "http", "http" + dBic, "http" + dBcm, "https", "https" + dBic, "https" + dBcm, dBkh, dBkh + dBic, dBkh + dBcm],
	["RegisteredApplications"]],
dBv = [[[dBpd], [dBpd], [dBpd], [dBpd], [dBpd], [dBpd]],
	[[dBpd], [dBpd], [dBpd], [dBpd]],
	[[""], [""], ["ApplicationDescription", "ApplicationIcon", "ApplicationName"], [""], [""]],
	[[".htm", ".html", ".shtml", ".xht", ".xhtml"], ["StartmenuInternet"], ["ftp", "gopher", "http", "https"]],
	[[""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""], [""]],
	[["K-Meleon"]]],
dBd = [[[dBkh], [dBkh], [dBkh], [dBkh], [dBkh], [dBkh]],
	[[dBku], [dBku], [dBku], [dBku]],
	[["K-MELEON.EXE"], ["K-Meleon"], ["K-Meleon manually added.", dBicd, "K-Meleon"], [dBicd], [dirService + "\\k-meleon.exe"]],
	[[dBkh, dBkh, dBkh, dBkh, dBkh], ["K-MELEON.EXE"], [dBkh, dBkh, dBkh, dBkh]],
	[[dBkh], [dBkh], [dBkh], [dBkh], [dBkh], [dBkh], ["URL:FTP (File Transfer Protocol)"], [dBicd], [dBcmd], ["URL:GOPHER (Gopher Transfer Protocol)"], [dBicd], [dBcmd], ["URL:HTTP (HyperText Transfer Protocol)"], [dBicd], [dBcmd], ["URL:HTTPS (HyperText Transfer Protocol) with Security System"], [dBicd], [dBcmd], ["K-Meleon HTML Document"], [dBicd], [dBcmd]],
	[["Software\\Clients\\StartMenuInternet\\K-MELEON.EXE\\Capabilities"]]];

var defBrowser = {
	check : function (a, b, c) {
		var z = true;
		wrk.open(wrk.ROOT_KEY_CURRENT_USER, dBr[a], wrk.ACCESS_READ);
		dBkey = wrk.openChild(dBk[a][b], wrk.ACCESS_READ);
		if (dBkey.readStringValue(dBv[a][b][c]) != dBd[a][b][c])
			z = false;
		dBkey.close();
		wrk.close();
		return z;
	},
	get : function () {
		var dBbtn = true;
		for (var a = 0; a < dBr.length; a++) {
			for (var b = 0; b < dBk[a].length; b++) {
				try {
					for (var c = 0; c < dBv[a][b].length; c++) {
						if (this.check(a, b, c) != true) {
							dBbtn = false;
							break;
						}
					}
				} catch (err) {
					dBbtn = false;
					break;
				}
			}
		}
		document.getElementById("defbtn").disabled = dBbtn;
	},
	set : function () {
		for (var a = 0; a < dBrk.length; a++) {
			wrk.create(wrk.ROOT_KEY_CURRENT_USER, dBr[0] + dBrk[a], wrk.ACCESS_WRITE);
			try {
				if (wrk.hasChild(dBuc))
					wrk.removeChild(dBuc);
				dBkey.close();
			} catch (err) {}
			wrk.close();
		}
		for (var a = 0; a < dBr.length; a++) {
			wrk.create(wrk.ROOT_KEY_CURRENT_USER, dBr[a], wrk.ACCESS_WRITE);
			for (var b = 0; b < dBk[a].length; b++) {
				dBkey = wrk.createChild(dBk[a][b], wrk.ACCESS_WRITE);
				for (var c = 0; c < dBv[a][b].length; c++) {
					dBkey.writeStringValue(dBv[a][b][c], dBd[a][b][c]);
				}
				dBkey.close();
			}
			wrk.close();
		}
		this.get();
	},
};