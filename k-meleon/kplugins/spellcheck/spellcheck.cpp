/*
	this file is under GPL
*/


#include "mozilla-config.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#define KMELEON_PLUGIN_EXPORTS
#include "kmeleon_plugin.h"
#include "strconv.h"

#define XPCOM_GLUE
#include "xpcom-config.h"
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsEmbedString.h>
#include <nsServiceManagerUtils.h>
#include <nsIWebBrowser.h>
#include <nsIWebBrowserFocus.h>
#include <nsIDOMWindow.h>
#include <nsPIDOMWindow.h>
#include <nsPIWindowRoot.h>
#include <nsIDOMElement.h>
#include <nsIDOMNSEditableElement.h>
#include <nsIDOMRange.h>
#include <nsIEditor.h>
#include <nsISelection.h>
#include <nsIInlineSpellChecker.h>
#include <nsIEditorSpellCheck.h>

#include <nsMemory.h>
#include "nsIDOMDocument.h"
#include "nsIDocShell.h"

#include <nsXPCOMCID.h>
#include <nsIProperties.h>
#include <nsILocalFile.h>

#include <nsIServiceManager.h>
#include <nsIAccessible.h>
#include <nsIAccessibleRetrieval.h>
#include <ISimpleDOMText.h>
#include <oleacc.h>
#include <servprov.h>
#include <mozISpellCheckingEngine.h>

#include "mozilla/fallible.h"
#include "mozilla/a11y/Accessible.h"

#include "mozilla/ChaosMode.h" // ChaosMode hack

#include <algorithm>


#define _Tr(x) kPlugin.kFuncs->Translate(x)

#define PLUGIN_NAME "Spellcheck"
#define LABEL_TITLE "K-Meleon spellchecker"
#define LABEL_ADDWORD "Add this word"
#define LABEL_IGNOREWORD "Ignore this word"
#define LABEL_SELECTDIC "Dictionaries"
#define LABEL_SETDIC "Set Default"
#define LABEL_ENABLE "Spellchecker is disabled.\r\nDo you want to enable it?"
#define LABEL_DISABLE "Disable spellchecker"

#define L_ID_DISABLE 1
#define L_ID_ADDWORD 2
#define L_ID_IGNOREWORD 3
#define L_ID_SETDIC 4

#define PREFKEY_DICTIONARY     "spellchecker.dictionary"
#define PREFKEY_ENABLED        "layout.spellcheckDefault"
#define PREFKEY_DICTIONARY_DIR "kmeleon.plugins.spelltest.dictDir"
#define PREFKEY_MENU_POSITION  "kmeleon.plugins.spelltest.position"
#define PREFKEY_SUGGEST_ORDER  "kmeleon.plugins.spelltest.order"
#define PREFKEY_MAX_SUGGESTS   "kmeleon.plugins.spelltest.max_suggests"

long DoMessage(const char *, const char *, const char *, long, long);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Load();
void Create(HWND);
int DoAccel(char *);
BOOL DoCommand(HWND, BOOL);
BOOL SetDictionariesDir();

WNDPROC KMeleonWndProc;
UINT g_id;

kmeleonPlugin kPlugin = {
	KMEL_PLUGIN_VER_UTF8,
	PLUGIN_NAME,
	DoMessage
};

#define MAX_DICT_COMMAND 10
char gDictCommand[MAX_DICT_COMMAND][20] = {0};


#include <nsIDOMEventListener.h>
#include <nsIDOMMouseEvent.h>
class CDomEventListener : public nsIDOMEventListener
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	CDomEventListener() : mRange(nullptr), mOffset(0) {}
	~CDomEventListener(){}
	BOOL Init(HWND hwnd)
	{		
		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;
		
		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return FALSE;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, FALSE);
	
		rv = mEventTarget->AddEventListener(NS_LITERAL_STRING("contextmenu"),
							this, false);

		NS_ENSURE_SUCCESS(rv, FALSE);		
		return TRUE;
	}

	void Done(HWND hwnd)
	{	
		mRange = nullptr;
		nsCOMPtr<nsIDOMEventTarget> mEventTarget;
		nsCOMPtr<nsIWebBrowser> mWebBrowser;

		if (!kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(mWebBrowser)))
			return;

		nsresult rv = GetDOMEventTarget(mWebBrowser, (getter_AddRefs(mEventTarget)));
		NS_ENSURE_SUCCESS(rv, );

		if (mEventTarget)
			mEventTarget->RemoveEventListener(NS_LITERAL_STRING("contextmenu"),
				  this, false);
	}

	bool GetRange(nsIDOMNode** node)
	{
		if (!mRange) return false;
		*node = mRange;
		NS_IF_ADDREF(*node);
		return true;
	}

	bool GetOffset(int32_t* offset) 
	{
		if (!mRange) return false;
		*offset = mOffset;
		return true;
	}
	
protected:

	nsresult GetDOMEventTarget (nsIWebBrowser* aWebBrowser, nsIDOMEventTarget** aTarget)
	{
		NS_ENSURE_ARG(aWebBrowser);
		nsCOMPtr<nsIDOMWindow> domWin;
		aWebBrowser->GetContentDOMWindow (getter_AddRefs(domWin));
		NS_ENSURE_TRUE (domWin, NS_ERROR_FAILURE);
	
		return domWin->GetWindowRoot (aTarget);
	}

	nsCOMPtr<nsIDOMNode> mRange;
	int32_t mOffset;
};

NS_IMPL_ISUPPORTS(CDomEventListener, nsIDOMEventListener)

NS_IMETHODIMP CDomEventListener::HandleEvent (nsIDOMEvent* aEvent)
{
	nsresult rv;
	nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
	NS_ENSURE_TRUE(mouseEvent, NS_ERROR_UNEXPECTED);

	mouseEvent->GetRangeParent(getter_AddRefs(mRange));
	mouseEvent->GetRangeOffset(&mOffset);
	return NS_OK;
}

CDomEventListener* gListener = nullptr;

void CreateTab(HWND hWndParent, HWND hTab) {
	gListener->Init(hTab);
}

void DestroyTab(HWND hWndParent, HWND hTab) {
	gListener->Done(hTab);
}


void Init()
{	
	kmeleonMenuItem menuitem;
	menuitem.before = 0;
	menuitem.command = g_id;
	menuitem.label = "Spell Checker";
	menuitem.type = MENU_COMMAND;
	kPlugin.kFuncs->SetMenu("TextPopup", &menuitem);
	gListener = new CDomEventListener();
	NS_IF_ADDREF(gListener);
}

void Quit()
{
	NS_IF_RELEASE(gListener);
	gListener = nullptr;
}


void DoMenu(HMENU menu, char* opt) {

}

long DoMessage(const char *to, const char *from, const char *subject,
               long data1, long data2)
{
	if (to[0] == '*' || stricmp(to, kPlugin.dllname) == 0) {
		if (stricmp(subject, "Load") == 0) {
			Load();
		}
		if (stricmp(subject, "Init") == 0) {
			Init();
		}
		if (stricmp(subject, "Quit") == 0) {
			Quit();
		}
		else if (stricmp(subject, "Create") == 0) {
			Create((HWND) data1);
		}
		if (strcmp(subject, "CreateTab") == 0) {
			CreateTab((HWND)data1, (HWND)data2);
		}
		else if (strcmp(subject, "DestroyTab") == 0) {
			DestroyTab((HWND)data1, (HWND)data2);
		}
		else if (strcmp(subject, "DoMenu") == 0) {
			DoMenu((HMENU)data1, (char *)data2);
			return 1;
        }
		else if (stricmp(subject, "DoAccel") == 0) {
			int *id = (int *) data2;
			*id = DoAccel((char *) data1);
		}
		else return 0;
		return 1;
	}
	return 0;
}


void Load()
{
	SetDictionariesDir();
	g_id = kPlugin.kFuncs->GetCommandIDs(2+MAX_DICT_COMMAND);
}




void Create(HWND hWndParent)
{	
	KMeleonWndProc = (WNDPROC) GetWindowLong(hWndParent, GWL_WNDPROC);
	SetWindowLong(hWndParent, GWL_WNDPROC, (LONG)WndProc);
}




int DoAccel(char *param)
{
	if (stricmp(param, "caret") == 0 || stricmp(param, "here") == 0 || stricmp(param, "word") == 0) {
		return g_id + 1;
	}
	else if (stricmp(param, "mouse") == 0) {
		return g_id;
	}
	else if (*param) {
		int free = -1;
		for (int i=0;i<MAX_DICT_COMMAND;i++) {			
			if (strcmp(gDictCommand[i], param) == 0)
				return g_id+i+2;
			if (!*gDictCommand[i] && free == -1) free = i;
		}
		if (free == -1) return 0;
		strcpy_s(gDictCommand[free], param);
		return free+2+g_id;
	}
	int pos = 0;
	kPlugin.kFuncs->GetPreference(PREF_INT, PREFKEY_MENU_POSITION, &pos, &pos);
	if (pos) {
		return g_id + 1;
	}
	return g_id;
}


/*
  Get nsIEditor on the focus from nsIWebBrowser.
 */

BOOL get_editor2(HWND hwnd, nsCOMPtr<nsIEditor>& editor) 
{
	nsCOMPtr<nsIDOMNode> target;
	if (gListener) gListener->GetRange(getter_AddRefs(target));
	NS_ENSURE_TRUE(target, FALSE);

	nsCOMPtr<nsIDOMDocument> doc;
	target->GetOwnerDocument(getter_AddRefs(doc));
	NS_ENSURE_TRUE(doc, FALSE);

	nsCOMPtr<nsIDOMWindow> win;
	doc->GetDefaultView(getter_AddRefs(win));
	NS_ENSURE_TRUE(win, FALSE);

	nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(win));
	if (!piWin) return FALSE;
	nsIDocShell* docShell = piWin->GetDocShell();
	if (!docShell) return FALSE;

	docShell->GetEditor(getter_AddRefs(editor));
	NS_ENSURE_TRUE(editor, FALSE);
	return TRUE;
}

BOOL get_editor(HWND hwnd, nsCOMPtr<nsIEditor>& editor)
{
	nsresult rv;
	
	nsCOMPtr<nsIWebBrowser> browser;
	if (! kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(browser))) {
		return FALSE;
	}

	nsCOMPtr<nsIWebBrowserFocus> focus(do_QueryInterface(browser));
	NS_ENSURE_TRUE(focus, FALSE);
	
	nsCOMPtr<nsIDOMElement> elem;
	rv = focus->GetFocusedElement(getter_AddRefs(elem));
	NS_ENSURE_TRUE(elem, FALSE);
	
	nsCOMPtr<nsIDOMNSEditableElement> ee(do_QueryInterface(elem));
	NS_ENSURE_TRUE(ee, FALSE);
	
	rv = ee->GetEditor(getter_AddRefs(editor));
	NS_ENSURE_TRUE(editor, FALSE);
	
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
		if (LOWORD(wParam) == g_id) {
			DoCommand(hWnd, FALSE);
		}
		else if (LOWORD(wParam) == g_id + 1) {
			DoCommand(hWnd, TRUE);
		} else if (LOWORD(wParam) >= g_id + 2 &&  (LOWORD(wParam) < g_id + 2 + MAX_DICT_COMMAND)) {
			char* lang = gDictCommand[LOWORD(wParam) - g_id - 2];			
			if (*lang) {
				nsString nsLang;
				NS_CStringToUTF16(nsDependentCString(lang), NS_CSTRING_ENCODING_UTF8, nsLang);
				nsCOMPtr<nsIEditor> editor;
				if (get_editor(hWnd, editor) || get_editor2(hWnd, editor)) {
					nsCOMPtr<nsIInlineSpellChecker> ispell;
					editor->GetInlineSpellChecker(true, getter_AddRefs(ispell));
					if (ispell) {
						nsCOMPtr<nsIEditorSpellCheck> spell;
						ispell->GetSpellChecker(getter_AddRefs(spell));
						if (spell) {							
							spell->SetCurrentDictionary(nsLang);
							ispell->SpellCheckRange(0);
							return 0;
						}
					}
				}
				nsCOMPtr<mozISpellCheckingEngine> hunspell(do_GetService("@mozilla.org/spellchecker/engine;1"));
				if (hunspell) {
					hunspell->SetDictionary(nsLang.get());
					return 0;
				}
			}
		}
		break;
	}
	return CallWindowProc(KMeleonWndProc, hWnd, message, wParam, lParam);
}


extern "C" {
	BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
	{
		switch (dwReason) {
		case DLL_PROCESS_DETACH:
			break;
		}
		return TRUE;
	}
	
	KMELEON_PLUGIN kmeleonPlugin *GetKmeleonPlugin() {
		return &kPlugin;
	}
	
	KMELEON_PLUGIN int DrawBitmap(DRAWITEMSTRUCT *dis) {
		return 14; // 14 = icon width
	}
}

nsresult GetServiceByContractID(const char* cid, const nsIID& iid, void** result)
{
	nsresult rv;
	nsCOMPtr<nsIServiceManager> servicemanager;
	rv = NS_GetServiceManager(getter_AddRefs(servicemanager));
	NS_ENSURE_SUCCESS(rv, rv);
	NS_ENSURE_TRUE(servicemanager, NS_ERROR_FAILURE);
	return servicemanager->GetServiceByContractID(cid, iid, result);
}



/*
  Set dictionaries path.
  
  seamonkey/extensions/spellcheck/idl/mozISpellCheckingEngine.idl
  seamonkey/extensions/spellcheck/hunspell/src/mozHunspell.cpp
 */

BOOL SetDictionariesDir()
{
	nsresult rv;
	wchar_t dirname[MAX_PATH] = {0};
	long len;

	nsCOMPtr<mozISpellCheckingEngine> hunspell(do_GetService("@mozilla.org/spellchecker/engine;1"));
	if (!hunspell) return FALSE;

	char _localeDir[MAX_PATH];
	kPlugin.kFuncs->GetFolder(LocaleFolder, _localeDir, MAX_PATH);
	if (!_localeDir[0]) return TRUE;

	wchar_t localeDir[MAX_PATH];
	utf8_to_utf16(_localeDir, localeDir, MAX_PATH);
	wcscat_s(localeDir, L"\\*");
	
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(localeDir, &ffd);
	localeDir[wcslen(localeDir)-1] = 0;

	if (hFind != INVALID_HANDLE_VALUE) {            
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
				ffd.cFileName[0]!='.') {					
					nsCOMPtr<nsIFile> mozDir;
					wchar_t dir[MAX_PATH] = {0};
					wcscpy(dir, localeDir);
					wcscat_s(dir, ffd.cFileName);
					NS_NewLocalFile(nsDependentString(dir), false, getter_AddRefs(mozDir));
					hunspell->AddDirectory(mozDir);
			}
		} while ( FindNextFile(hFind, &ffd) );
		FindClose(hFind);
	}   

	return TRUE;
}


/*
  Minimal string list class
 */
struct miniWords
{
	/* construct with alloc */
	miniWords(int size);
	/* construct by pointer */
	miniWords(PRUnichar** p, int size)
		: m_length(size), m_size(size), m_words(p) {}
	~miniWords() { clear(); }
	int length() { return m_length; }
	int size() { return m_size; }
	void push(PRUnichar* p) {
		if (m_length < m_size) 
			m_words[m_length ++] = p;
	}
	void clear();
	PRUnichar* operator[] (const int i) {
		return (i >= 0 && i < m_length) ? m_words[i] : 0;
	}
	void sort();
private:
	static bool comp(PRUnichar *a, PRUnichar *b) {
		return wcscmp(a, b) < 0;
	}
	int m_length, m_size;
	PRUnichar** m_words;
};

miniWords::miniWords(int size) : m_length(0), m_size(0), m_words(0) {
	m_words = (PRUnichar**) nsMemory::Alloc(sizeof(PRUnichar*) * size);
	if (m_words) {
		m_size = size;
	}
}

void miniWords::clear()
{
	if (m_words) {
		for (int i = 0; i < m_length; ++ i) {
			nsMemory::Free(m_words[i]);
		}
		nsMemory::Free(m_words);
		m_words = 0;
		m_length = m_size = 0;
	}
}

void miniWords::sort()
{
	if (m_words) {
		std::sort(m_words, m_words + m_length, miniWords::comp);
	}
}

template <typename T>
struct miniPtr
{
	/* construct by pointer */
	miniPtr(T* p = 0) : m_ptr(p) {}
	~miniPtr() { if (m_ptr) nsMemory::Free(m_ptr); }
	operator T* () { return m_ptr; }
	T** operator&() { return &m_ptr; }
private:
	T* m_ptr;
};

BOOL append_menuA(HMENU hMenu, UINT flags, UINT item, const wchar_t* label)
{
	return AppendMenuA(hMenu, flags, item, CUTF16_to_ANSI(label));
}
BOOL append_menuA_tr(HMENU hMenu, UINT flags, UINT item, const char* label)
{
	return AppendMenuA(hMenu, flags, item, kPlugin.kFuncs->Translate(label));
}
BOOL append_menuW(HMENU hMenu, UINT flags, UINT item, const wchar_t* label)
{
	return AppendMenuW(hMenu, flags, item, label);
}
BOOL append_menuW_tr(HMENU hMenu, UINT flags, UINT item, const char* label)
{
	return AppendMenuW(hMenu, flags, item, CUTF8_to_UTF16(kPlugin.kFuncs->Translate(label)));
}

struct miniHMenu
{
	miniHMenu() : m_hmenu(0) {};
	~miniHMenu() { if (m_hmenu) DestroyMenu(m_hmenu); }
	HMENU operator= (HMENU hmenu) { return m_hmenu = hmenu; }
	operator HMENU () { return m_hmenu; }
	HMENU release() { HMENU m = m_hmenu; m_hmenu = 0; return m; }
private:
	HMENU m_hmenu;
};


/*
  show popup menu
  
  returns
  -1 : error
   0 : no select
   1 : disable spellcheck
   2 : add word to personal dictionary
   3 : ignore word
   4 : set default dictionary
   0x1000 - 0x1fff : dictionaries
   0x2000 - 0x2fff : suggested words
 */
INT select_menu(HWND hwnd, miniWords& suggest, miniWords& dics, const wchar_t* current, int x, int y)
{
	miniHMenu hDics, hMenu;
	BOOL (*append_menu)(HMENU, UINT, UINT, const wchar_t*);
	BOOL (*append_menu_tr)(HMENU, UINT, UINT, const char*);
	
	if (IsWindowUnicode(hwnd)) {
		append_menu = append_menuW;
		append_menu_tr = append_menuW_tr;
	} else {
		append_menu = append_menuA;
		append_menu_tr = append_menuA_tr;
	}
	// dictionaries is on sub menu.
	hDics = CreatePopupMenu();
	if (hDics == NULL) return -1;
	// append dictionaries
	for (int i = 0, id = 0x1000; i < dics.length(); ++ i, ++ id) {
		UINT flags = MF_STRING;
		// lstrcmpW can't compare correctly.
		flags |= wcscmp(current, dics[i]) ? MF_ENABLED : MF_CHECKED | MF_GRAYED;
		if (! append_menu(hDics, flags, id, dics[i])) {
			return -1;
		}
	}
	if (! append_menu(hDics, MF_SEPARATOR, 0, 0) ||
	    ! append_menu_tr(hDics, MF_ENABLED | MF_STRING, L_ID_SETDIC, LABEL_SETDIC)) {
		return -1;
	}
	// main popup menu
	hMenu = CreatePopupMenu();
	if (hMenu == NULL) return -1;
	// append suggested word
	for (int i = 0, id = 0x2000; i < suggest.length(); ++ i, ++ id) {
		if (! append_menu(hMenu, MF_ENABLED | MF_STRING, id, suggest[i])) {
			return -1;
		}
	}
	// append "add this word" and "ignore this word"
	if (suggest.length() > 0) {
		if (! append_menu(hMenu, MF_SEPARATOR, 0, 0) ||
		    ! append_menu_tr(hMenu, MF_ENABLED | MF_STRING, L_ID_IGNOREWORD, LABEL_IGNOREWORD) ||
		    ! append_menu(hMenu, MF_SEPARATOR, 0, 0) ||
		    ! append_menu_tr(hMenu, MF_ENABLED | MF_STRING, L_ID_ADDWORD, LABEL_ADDWORD)) {
			return -1;
		}
	}
	// append dictionary selecting popup
	if (! append_menu_tr(hMenu, MF_ENABLED | MF_POPUP | MF_STRING, (UINT) (HMENU) hDics, LABEL_SELECTDIC)) {
		return -1;
	}
	hDics.release();
	// append "disable spellchecker"
	if (! append_menu(hMenu, MF_SEPARATOR, 0, 0) ||
	    ! append_menu_tr(hMenu, MF_ENABLED | MF_STRING, L_ID_DISABLE, LABEL_DISABLE)) {
		return -1;
	}
	INT result = TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, hwnd, NULL);
	return result;
}


int message_box(HWND hwnd, LPCSTR lpText, LPCSTR lpTitle, UINT style)
{
	if (IsWindowUnicode(hwnd)) {
		return MessageBoxW(hwnd, CUTF8_to_UTF16(kPlugin.kFuncs->Translate(lpText)), CUTF8_to_UTF16(kPlugin.kFuncs->Translate(lpTitle)), style);
	}
	return MessageBoxA(hwnd, kPlugin.kFuncs->Translate(lpText), kPlugin.kFuncs->Translate(lpTitle), style);
}

/*
  Get screen position of a word by accesibility API
  provided mozilla and windows .
 */
BOOL get_word_pos(HWND hwnd, nsCOMPtr<nsIDOMNode>& node, PRInt32 offset, int& rx, int& ry)
{
	nsresult rv;
	
	nsCOMPtr<nsIAccessibleRetrieval> accessibleretrieval = do_GetService("@mozilla.org/accessibleRetrieval;1");
	NS_ENSURE_TRUE(accessibleretrieval, FALSE);
	
	nsCOMPtr<nsIAccessible> accessible;
	rv = accessibleretrieval->GetAccessibleFor(node, getter_AddRefs(accessible));
	NS_ENSURE_SUCCESS(rv, FALSE);
	NS_ENSURE_TRUE(accessible, FALSE);
	
	// below code is related with Microsoft Active Accessibility
	
	mozilla::a11y::Accessible* acc = accessible->ToInternalAccessible();
	
	// This is MS COM component, isn't XPCOM.
	IAccessible* pAccessible = NULL;
	acc->GetNativeInterface((void**) &pAccessible);
	NS_ENSURE_SUCCESS(rv, FALSE);
	NS_ENSURE_TRUE(pAccessible, FALSE);
	
	HRESULT hresult;
	IServiceProvider *pServProv = NULL;
	hresult = pAccessible->QueryInterface(IID_IServiceProvider, (void**) &pServProv);
	pAccessible->Release();
	if (FAILED(hresult) || pServProv == NULL) return FALSE;
	
	const GUID refguid = {0x0c539790, 0x12e4, 0x11cf, 0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8};
	ISimpleDOMText *pSimpleDOMText = NULL;
	hresult = pServProv->QueryService(refguid, IID_ISimpleDOMText, (void**) &pSimpleDOMText);
	pServProv->Release();
	if (FAILED(hresult) || pSimpleDOMText == NULL) return FALSE;
	
	int x, y, w, h;
	hresult = pSimpleDOMText->get_clippedSubstringBounds(offset, offset + 1, &x, &y, &w, &h);
	pSimpleDOMText->Release();
	if (FAILED(hresult)) return FALSE;
	
	rx = x; ry = y + h;
	return TRUE;
}


/*
  Sort Suggestion List.
  
 */
VOID SortSuggestionList(miniWords& suggests)
{
	int order = 0;
	
	kPlugin.kFuncs->GetPreference(PREF_INT, PREFKEY_SUGGEST_ORDER, &order, &order);
	
	switch (order) {
	case 1:
		suggests.sort();
		break;
	default:
		break;
	}
}


int GetMaxSuggests()
{
	int n = 20;
	
	kPlugin.kFuncs->GetPreference(PREF_INT, PREFKEY_MAX_SUGGESTS, &n, &n);
	if (n > 30) {
		n = 30;
	}
	else if (n < 5) {
		n = 5;
	}
	return n;
}


/*
  Almost steps are borrowed from seamonkey.
  
  mozilla/editor/ui/composer/content/editorInlineSpellCheck.js
  mozilla/toolkit/content/inlineSpellCheckUI.js
 */

BOOL DoCommand(HWND hwnd, BOOL bHere)
{
	nsresult rv;
	nsEmbedString word;
	
	nsCOMPtr<nsIEditor> editor;
	if (!get_editor(hwnd, editor) && !get_editor2(hwnd, editor))
		return FALSE;
	
	nsCOMPtr<nsIInlineSpellChecker> ispell;
	rv = editor->GetInlineSpellChecker(true, getter_AddRefs(ispell));
	NS_ENSURE_SUCCESS(rv, FALSE);
	NS_ENSURE_TRUE(ispell, FALSE);
	
	// Is spellchecker enabled?
	bool is_enabled = 0;
	rv = ispell->GetEnableRealTimeSpell(&is_enabled);
	NS_ENSURE_SUCCESS(rv, FALSE);
	if (! is_enabled) {
		int result = message_box(hwnd, LABEL_ENABLE, LABEL_TITLE, MB_YESNOCANCEL | MB_ICONQUESTION);
		if (result == IDYES) {
			// enable spellchecher
			rv = ispell->SetEnableRealTimeSpell(true);
			NS_ENSURE_SUCCESS(rv, FALSE);
			int i = 0;
			kPlugin.kFuncs->GetPreference(PREF_INT, PREFKEY_ENABLED, &i, &i);
			if (i == 0) {
				i = 1;
				kPlugin.kFuncs->SetPreference(PREF_INT, PREFKEY_ENABLED, &i, TRUE);
			}			
		}
		// Can't continue. Must return.
		return TRUE;
	}

/*	nsCOMPtr<nsIWebBrowser> browser;
	if (! kPlugin.kFuncs->GetMozillaWebBrowser(hwnd, getter_AddRefs(browser))) {
		return FALSE;
	}

	nsCOMPtr<nsIDOMWindow> win;
	rv = browser->GetContentDOMWindow(getter_AddRefs(win));
	NS_ENSURE_SUCCESS(rv, rv);
	NS_ENSURE_TRUE(win, FALSE);

	nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(win));
	NS_ENSURE_TRUE(window, FALSE);
	nsCOMPtr<nsPIWindowRoot> root = window->GetTopWindowRoot();
	NS_ENSURE_TRUE(root, FALSE);*/

	int32_t offset = 0;
	nsCOMPtr<nsIDOMNode> node;
	if (!bHere && gListener) {
		gListener->GetRange(getter_AddRefs(node));
		gListener->GetOffset(&offset);
	}
	
	if (!node) {
		nsCOMPtr<nsISelection> selection;
		rv = editor->GetSelection(getter_AddRefs(selection));
		NS_ENSURE_SUCCESS(rv, FALSE);
		NS_ENSURE_TRUE(selection, FALSE);
	
		rv = selection->GetAnchorNode(getter_AddRefs(node));
		NS_ENSURE_SUCCESS(rv, FALSE);
		NS_ENSURE_TRUE(node, FALSE);	
		
		rv = selection->GetAnchorOffset(&offset);
		NS_ENSURE_SUCCESS(rv, FALSE);
	}
	
	nsCOMPtr<nsIEditorSpellCheck> spell;
	rv = ispell->GetSpellChecker(getter_AddRefs(spell));
	NS_ENSURE_SUCCESS(rv, FALSE);
	NS_ENSURE_TRUE(spell, FALSE);
	
	bool is_mis = 0;
	nsCOMPtr<nsIDOMRange> range;
	rv = ispell->GetMisspelledWord(node, offset, getter_AddRefs(range));
	if (NS_SUCCEEDED(rv) && range) {
		rv = range->ToString(word);
		NS_ENSURE_SUCCESS(rv, FALSE);
		NS_ENSURE_TRUE(range, FALSE);
		
		rv = spell->CheckCurrentWord(word.get(), &is_mis);
		NS_ENSURE_SUCCESS(rv, FALSE);
	}
	
	// get suggestions list
	const int max_suggests = GetMaxSuggests();
	miniWords suggests(max_suggests);
	if (is_mis) {
		for (int i = 0; i < max_suggests; ++ i) {
			PRUnichar *w;
			rv = spell->GetSuggestedWord(&w);
			NS_ENSURE_SUCCESS(rv, FALSE);
			if (! w) break;
			if (! *w) {
				nsMemory::Free(w);
				break;
			}
			suggests.push(w);
		}
	}
	SortSuggestionList(suggests);
	
	// get dictionaries list
	PRUnichar** d;
	PRUint32 c;
	rv = spell->GetDictionaryList(&d, &c);
	NS_ENSURE_SUCCESS(rv, FALSE);
	miniWords dics(d, c);
	dics.sort();
	
	// get current dictionary
	nsString dict;
	rv = spell->GetCurrentDictionary(dict);
	NS_ENSURE_SUCCESS(rv, FALSE);
	
	// get menu position
	int x, y;
	if (! bHere || ! get_word_pos(hwnd, node, offset, x, y)) {
		POINT point = {0};
		GetCursorPos(&point);
		x = point.x; y = point.y;
	}
	
	// show menu
	INT result = select_menu(hwnd, suggests, dics, dict.get(), x, y);
	if (result < 1) {
		// canceled
	} else if (result == L_ID_DISABLE) {
		rv = ispell->SetEnableRealTimeSpell(false);
		NS_ENSURE_SUCCESS(rv, FALSE);
	} else if (result == L_ID_ADDWORD) {
		rv = ispell->AddWordToDictionary(word);
		NS_ENSURE_SUCCESS(rv, FALSE);
	} else if (result == L_ID_IGNOREWORD) {
		rv = ispell->IgnoreWord(word);
		NS_ENSURE_SUCCESS(rv, FALSE);
	} else if (result == L_ID_SETDIC) {
		// mozilla/editor/composer/src/nsEditorSpellCheck.cpp
		// SaveDefaultDictionary
		kPlugin.kFuncs->SetPreference(PREF_UNISTRING, PREFKEY_DICTIONARY, (void*)dict.get(), TRUE);
	} else if (result & 0x1000) {
		// change dictionary
		result &= 0xfff;
		if (result < dics.length()) {
			rv = spell->SetCurrentDictionary(nsDependentString(dics[result]));
			NS_ENSURE_SUCCESS(rv, FALSE);
			// re-check all
			rv = ispell->SpellCheckRange(0);
			NS_ENSURE_SUCCESS(rv, FALSE);
		}
	} else if (result & 0x2000) {
		// correct word
		result &= 0xfff;
		if (result < suggests.length()) {
			word = suggests[result];
			rv = ispell->ReplaceWord(node, offset, word);
			NS_ENSURE_SUCCESS(rv, FALSE);
		}
	}
	
	return TRUE;
}

#if 1 //ChaosMode hack
namespace mozilla {
namespace detail {

Atomic<uint32_t> gChaosModeCounter(0);

} /* namespace detail */
} /* namespace mozilla */
#endif
