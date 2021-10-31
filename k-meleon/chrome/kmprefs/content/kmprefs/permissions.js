Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import("resource://gre/modules/NetUtil.jsm");

const nsIPermissionManager = Components.interfaces.nsIPermissionManager;
const nsICookiePermission = Components.interfaces.nsICookiePermission;

function Permission(host, rawHost, type, capability, perm) {
	this.host = host;
	this.rawHost = rawHost;
	this.type = type;
	this.capability = capability;
	this.perm = perm;
}
var gPermissionManager = {
	_type: '',
	_permissions: [],
	_bundle: null,
	_tree: null,
	_view: {
		_rowCount: 0,
		get rowCount() {
			return this._rowCount;
		},
		getCellText: function (aRow, aColumn) {
			if (aColumn.id == 'siteCol')
				return gPermissionManager._permissions[aRow].rawHost;
			else if (aColumn.id == 'statusCol')
				return gPermissionManager._permissions[aRow].capability;
			return '';
		},
		isSeparator: function (aIndex) {
			return false;
		},
		isSorted: function () {
			return false;
		},
		isContainer: function (aIndex) {
			return false;
		},
		setTree: function (aTree) {},
		getImageSrc: function (aRow, aColumn) {},
		getProgressMode: function (aRow, aColumn) {},
		getCellValue: function (aRow, aColumn) {},
		cycleHeader: function (column) {},
		getRowProperties: function (row) {
			return '';
		},
		getColumnProperties: function (column) {
			return '';
		},
		getCellProperties: function (row, column) {
			if (column.element.getAttribute('id') == 'siteCol')
				return 'ltr';
			return '';
		}
	},
	_getCapabilityString: function (aCapability) {
		var stringKey = null;
		switch (aCapability) {
		case nsIPermissionManager.ALLOW_ACTION:
			stringKey = 'can';
			break;
		case nsIPermissionManager.DENY_ACTION:
			stringKey = 'cannot';
			break;
		case nsICookiePermission.ACCESS_ALLOW_FIRST_PARTY_ONLY:
			stringKey = 'canAccessFirstParty';
			break;
		case nsICookiePermission.ACCESS_SESSION:
			stringKey = 'canSession';
			break;
		}
		return this._bundle.getString(stringKey);
	},
	addPermission: function (aCapability) {
		var textbox = document.getElementById('url');
		var host = textbox.value.replace(/^\s*([-\w]*:\/+)?/, ''); // trim any leading space and scheme
		try {
			var uri = NetUtil.newURI("http://" + host);
			host = uri.host;
		} catch (ex) {
			var message = this._bundle.getString('invalidURI');
			var title = this._bundle.getString('invalidURITitle');
			Services.prompt.alert(window, title, message);
			return;
		}
		var capabilityString = this._getCapabilityString(aCapability);
		// check whether the permission already exists, if not, add it
		var exists = false;
		for (var i = 0; i < this._permissions.length; ++i) {
			if (this._permissions[i].rawHost == host) {
				// Avoid calling the permission manager if the capability settings are
				// the same. Otherwise allow the call to the permissions manager to
				// update the listbox for us.
				exists = this._permissions[i].perm == aCapability;
				break;
			}
		}
		if (!exists) {
			host = (host.charAt(0) == '.') ? host.substring(1, host.length) : host;
			var uri = NetUtil.newURI("http://" + host);
			Services.perms.add(uri, this._type, aCapability);
		}
		textbox.value = '';
		textbox.focus();
		// covers a case where the site exists already, so the buttons don't disable
		this.onHostInput(textbox);
		// enable 'remove all' button as needed
		document.getElementById('removeAllPermissions').disabled = this._permissions.length == 0;
	},
	onHostInput: function (aSiteField) {
		document.getElementById('btnSession').disabled = !aSiteField.value;
		document.getElementById('btnBlock').disabled = !aSiteField.value;
		document.getElementById('btnAllow').disabled = !aSiteField.value;
	},
	onWindowKeyPress: function (aEvent) {
		if (aEvent.keyCode == KeyEvent.DOM_VK_ESCAPE)
			window.close();
	},
	onHostKeyPress: function (aEvent) {
		if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN)
			document.getElementById('btnAllow').click();
	},
	check: function (val) {
		if (val == 'false')
			return false;
		else if (val == 'true')
			return true;
		else if (val == 'empty')
			return '';
		else if (val == 'null')
			return null;
		else
			return val;
	},
	onLoad: function () {
		this._bundle = document.getElementById('bundlePermissions');
		var params = {}, args;
		if (document.location.search) {
			args = document.location.search.substring(1, document.location.search.length).replace(/\s+/g, '').split(',');
		}
		for (var c in args) {
			var aKey = args[c].split(':')[0],
			aValue = this.check(args[c].split(':')[1]);
			if (aValue == 'about')
				aValue = '';
			Object.defineProperty(params, aKey, {
				value: aValue
			});
		}
		this.init(params);
	},
	init: function (aParams) {
		if (this._type) {
			// reusing an open dialog, clear the old observer
			this.uninit();
		}
		this._type = aParams.permissionType;
		this._manageCapability = aParams.manageCapability;
		var permissionsText = document.getElementById('permissionsText');
		while (permissionsText.hasChildNodes())
			permissionsText.removeChild(permissionsText.firstChild);
		permissionsText.appendChild(document.createTextNode(this._bundle.getString(aParams.introText)));
		document.title = this._bundle.getString(aParams.windowTitle);
		document.getElementById('btnBlock').hidden = !aParams.blockVisible;
		document.getElementById('btnSession').hidden = !aParams.sessionVisible;
		document.getElementById('btnAllow').hidden = !aParams.allowVisible;
		var urlFieldVisible = (aParams.blockVisible || aParams.sessionVisible || aParams.allowVisible);
		var urlField = document.getElementById('url');
		urlField.value = aParams.prefilledHost;
		urlField.hidden = !urlFieldVisible;
		this.onHostInput(urlField);
		var urlLabel = document.getElementById('urlLabel');
		urlLabel.hidden = !urlFieldVisible;
		Services.obs.addObserver(this, 'perm-changed', false);
		this._loadPermissions();
		urlField.focus();
	},
	uninit: function () {
		Services.obs.removeObserver(this, 'perm-changed');
	},
	observe: function (aSubject, aTopic, aData) {
		if (aTopic == 'perm-changed') {
			var permission = aSubject.QueryInterface(Components.interfaces.nsIPermission);
			if (aData == 'added') {
				this._addPermissionToList(permission);
				++this._view._rowCount;
				this._tree.treeBoxObject.rowCountChanged(this._view.rowCount - 1, 1);
				// Re-do the sort, since we inserted this new item at the end.
				this._loadPermissions();
			} else if (aData == 'changed') {
				for (var i = 0; i < this._permissions.length; ++i) {
					if (this._permissions[i].host == permission.host) {
						this._permissions[i].capability = this._getCapabilityString(permission.capability);
						break;
					}
				}
				this._tree.treeBoxObject.invalidate();
			}
			// No UI other than this window causes this method to be sent a 'deleted'
			// notification, so we don't need to implement it since Delete is handled
			// directly by the Permission Removal handlers. If that ever changes, those
			// implementations will have to move into here.
		}
	},
	onPermissionChosen: function (index) {
		if (this._tree.view.selection.count > 0 && this._tree.view.rowCount > 0) {
			var textbox = document.getElementById('url');
			this.setHost(this._permissions[index].rawHost);
			this._loadPermissions();
			textbox.focus();
			this.onHostInput(textbox);
		}
	},
	onPermissionSelected: function () {
		var hasSelection = this._tree.view.selection.count > 0;
		var hasRows = this._tree.view.rowCount > 0;
		document.getElementById('removePermission').disabled = !hasRows || !hasSelection;
		document.getElementById('removeAllPermissions').disabled = !hasRows;
	},
	onPermissionDeleted: function () {
		if (!this._view.rowCount)
			return;
		var removedPermissions = [];
		gTreeUtils.deleteSelectedItems(this._tree, this._view, this._permissions, removedPermissions);
		for (var i = 0; i < removedPermissions.length; ++i) {
			var p = removedPermissions[i];
			let uri = NetUtil.newURI("http://" + p.host);
			Services.perms.remove(uri, p.type);
		}
		document.getElementById('removePermission').disabled = !this._permissions.length;
		document.getElementById('removeAllPermissions').disabled = !this._permissions.length;
	},
	onAllPermissionsDeleted: function () {
		if (!this._view.rowCount)
			return;
		var removedPermissions = [];
		gTreeUtils.deleteAll(this._tree, this._view, this._permissions, removedPermissions);
		for (var i = 0; i < removedPermissions.length; ++i) {
			var p = removedPermissions[i];
			Services.perms.remove(p.host, p.type);
		}
		document.getElementById('removePermission').disabled = true;
		document.getElementById('removeAllPermissions').disabled = true;
	},
	onPermissionKeyPress: function (aEvent) {
		if (aEvent.keyCode == 46)
			this.onPermissionDeleted();
	},
	_lastSortCol: 'rawHost',
	_lastSortAsc: false,
	_Comparator: function (a, b) {
		return a.toLowerCase().localeCompare(b.toLowerCase());
	},
	onPermissionSort: function (aColumn,aSort) {
		this._lastSortAsc = gTreeUtils.sort(this._tree, this._view, this._permissions, aColumn, this._Comparator, this._lastSortCol, aSort);
		this._lastSortCol = aColumn;
	},
	_loadPermissions: function () {
		this._tree = document.getElementById('permissionsTree');
		this._permissions = [];
		// load permissions into a table
		var count = 0;
		var enumerator = Services.perms.enumerator;
		while (enumerator.hasMoreElements()) {
			var nextPermission = enumerator.getNext().QueryInterface(Components.interfaces.nsIPermission);
			this._addPermissionToList(nextPermission);
		}
		this._view._rowCount = this._permissions.length;
		// sort and display the table
		this._tree.treeBoxObject.view = this._view;
		this.onPermissionSort(this._lastSortCol, false);
		// disable 'remove all' button if there are none
		document.getElementById('removeAllPermissions').disabled = this._permissions.length == 0;
	},
	_addPermissionToList: function (aPermission) {
		if (aPermission.type == this._type &&
			(!this._manageCapability ||
				(aPermission.capability == this._manageCapability))) {
			var host = aPermission.host;
			var capabilityString = this._getCapabilityString(aPermission.capability);
			var p = new Permission(host,
					(host.charAt(0) == '.') ? host.substring(1, host.length) : host,
					aPermission.type,
					capabilityString,
					aPermission.capability);
			this._permissions.push(p);
		}
	},
	setHost: function (aHost) {
		document.getElementById('url').value = aHost;
	}
};