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

// this handles plugin loading/unloading

#include "StdAfx.h"

#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

#include "BrowserView.h"
#include "BrowserFrm.h"

#include "kmeleon_plugin.h"
#include "Plugins.h"
#include "Utils.h"

int SessionSize=0;
char **pHistory;
char **pHistUrl;
kmeleonDocInfo kDocInfo;
kmeleonPointInfo gPointInfo;


CPlugins::CPlugins()
{
}

CPlugins::~CPlugins()
{

   delete gPointInfo.image;
   delete gPointInfo.link;
   delete gPointInfo.frame;
   delete gPointInfo.page;

   if (SessionSize) {
     for (int i=0; i<SessionSize; i++) {
       if (pHistory && pHistory[i])
         delete pHistory[i];
       if (pHistUrl && pHistUrl[i])
          delete pHistUrl[i];
     }
   }
   if (pHistory)
      delete pHistory;
   if (pHistUrl)
      delete pHistUrl;
   pHistory = NULL;
   pHistUrl = NULL;

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.title)
      delete kDocInfo.title;
   
   UnLoadAll();
}

// returns a pointer to the char after the last \ or /
const char *FileNoPath(const char *filepath)
{
   char *p1 = strrchr(filepath, '\\');
   char *p2 = strrchr(filepath, '/');
   if (p1 > p2) {
      return p1 + 1;
   }
   else if (p2 > p1) {
      return p2 + 1;
   }
   else {
      return filepath;
   }
}


UINT currentCmdID = PLUGIN_COMMAND_START_ID;
UINT GetCommandIDs(int num)
{
   UINT freeID = currentCmdID;
   currentCmdID += num;
   return freeID;
}

int CPlugins::OnUpdate(UINT command)
{
   if (command >= PLUGIN_COMMAND_START_ID && command <= currentCmdID)
      return true;
   return false;
}

void NavigateTo(const char *url, int windowState, HWND mainWnd)
{
   CBrowserFrame *mainFrame;
   if (mainWnd)
      mainFrame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);
   else
      mainFrame = theApp.m_pMostRecentBrowserFrame;

   if (!mainFrame) {
      return;
   }

   switch(windowState) {
   case OPEN_NORMAL:
      mainFrame->m_wndBrowserView.OpenURL(url);
      break;
   case OPEN_NEW:
      mainFrame->m_wndBrowserView.OpenURLInNewWindow(NS_ConvertASCIItoUCS2(url).get());
      break;
   case OPEN_BACKGROUND:
      mainFrame->m_wndBrowserView.OpenURLInNewWindow(NS_ConvertASCIItoUCS2(url).get(), true);
      break;
   }
}

kmeleonDocInfo * GetDocInfo(HWND mainWnd)
{
   CBrowserFrame *frame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);

   if (!frame) {
      return NULL;
   }

   char *url;
   int len = frame->m_wndBrowserView.GetCurrentURI(NULL);
   if (len) {
      url = new char[len+1];
      frame->m_wndBrowserView.GetCurrentURI(url);
   }
   else
      url = NULL;

   CString title;
   frame->m_wndBrowserView.GetPageTitle(title);
   char *doctitle = new char[title.GetLength()+1];
   strcpy(doctitle, title);

   if (kDocInfo.url)
      delete kDocInfo.url;
   
   if (kDocInfo.title)
      delete kDocInfo.title;

   kDocInfo.title = doctitle;
   kDocInfo.url = url;

   return &kDocInfo;
}

void GetPreference(enum PREFTYPE type, char *preference, void *ret, void *defVal)
{
   switch (type) {
   case PREF_BOOL:
      *(int *)ret = theApp.preferences.GetBool(preference, *(int *)defVal);
      break;
   case PREF_INT:
      *(int *)ret = theApp.preferences.GetInt(preference, *(int *)defVal);
      break;
   case PREF_STRING:
      theApp.preferences.GetString(preference, (char *)ret, (char *)defVal);
      break;
   }
}

void SetPreference(enum PREFTYPE type, char *preference, void *val, BOOL update)
{
   switch (type) {
      case PREF_BOOL:
         theApp.preferences.SetBool(preference, *(int *)val);
         break;
      case PREF_INT:
         theApp.preferences.SetInt(preference, *(int *)val);
         break;
      case PREF_STRING:
         theApp.preferences.SetString(preference, (char *)val);
         break;
   }
   if (update) {
      theApp.preferences.Save();
      theApp.preferences.Load();
   }
}

void SetStatusBarText(const char *s) {
   theApp.m_pMostRecentBrowserFrame->m_wndStatusBar.SetPaneText(0, s);
}

int GetMozillaSessionHistory (char ***titles, char ***urls, int *count, int *index)
{
   nsresult result;
   int i;

   if (!theApp.m_pMostRecentBrowserFrame) return FALSE;
   
   nsCOMPtr<nsISHistory> h;

   result = theApp.m_pMostRecentBrowserFrame->m_wndBrowserView.mWebNav->GetSessionHistory(getter_AddRefs (h));

   if (!NS_SUCCEEDED (result) || (!h)) return FALSE;
   
   h->GetCount (count);
   h->GetIndex (index);

   // Clear the previous table
   if (SessionSize) {
      for (i=0; i<SessionSize; i++) {
         if (pHistory && pHistory[i])
            delete pHistory[i];
         if (pHistUrl && pHistUrl[i])
            delete pHistUrl[i];
      }
   }
   
   SessionSize = *count;
   
   if (pHistory)
      delete pHistory;
   pHistory = new char *[SessionSize];
   
   nsCOMPtr<nsIHistoryEntry> he;
   PRUnichar *title;
   
   if (pHistUrl)
      delete pHistUrl;
   pHistUrl = new char *[SessionSize];
   
   nsCOMPtr<nsIURI> theUri;
   nsCString uri;
   
   for (i=0; i < SessionSize; i++) {
      pHistory[i] = NULL;
      pHistUrl[i] = NULL;
   }
   
   for (i=0; i < SessionSize; i++) {
      result = h->GetEntryAtIndex(i, PR_FALSE, getter_AddRefs (he));
      if (!NS_SUCCEEDED(result) || (!he)) return FALSE;
      
      result = he->GetURI(getter_AddRefs(theUri));
      if (!NS_SUCCEEDED(result) || (!theUri)) return FALSE;
      
      theUri->GetSpec(uri);
      char *t = strdup(uri.get());

      pHistUrl[i] = t;
      
      result = he->GetTitle (&title);
      if (!NS_SUCCEEDED(result) || (!title)) return FALSE;
      
      // The title is in 16-bit unicode, this converts it to 8bit (UTF)
      int len;
      len = WideCharToMultiByte(CP_OEMCP, 0, title, -1, 0, 0, NULL, NULL);
      char *s = new char[len+1];
      len = WideCharToMultiByte(CP_OEMCP, 0, title, -1, s, len, NULL, NULL);
      s[len] = 0;
      pHistory[i] = s;
   }
   
   if (titles)
      *titles = pHistory;
   if (urls)
      *urls = pHistUrl;

   return TRUE;
}

void GotoHistoryIndex(UINT index)
{
	CBrowserFrame	*mainFrame = theApp.m_pMostRecentBrowserFrame;
	if (mainFrame)
		mainFrame->m_wndBrowserView.mWebNav->GotoIndex(index);
}

void RegisterBand(HWND hWnd, char *name, int visibleOnMenu)
{
   theApp.m_pMostRecentBrowserFrame->m_wndReBar.RegisterBand(hWnd, name, visibleOnMenu);
}

// This lets a plugin create a toolbar within the current browser frame
// the advantage of having K-Meleon create the toolbar is that it will
// be handled through MFC, which will handle the button states through
// UPDATE_UI calls
HWND CreateToolbar(HWND hWnd, UINT style) {
   CBrowserFrame *browserFrm = (CBrowserFrame *)CWnd::FromHandle(hWnd);

   if (browserFrm)
      return browserFrm->CreateToolbar(style);
   else
      return NULL;
}

int GetID(char *strID) {

   int ID = theApp.GetID(strID);

   if (!ID) {
      char *plugin = strID;
      char *parameter = strchr(strID, '(');
      if (parameter) {
         *parameter++ = 0;
         char *close = strchr(parameter, ')');
         if (close)
            *close = 0;
      }
      
      theApp.plugins.SendMessage(plugin, "* GetID", "DoAccel", (long) parameter, (long)&ID);
   }

   return ID;
}

kmeleonPointInfo *GetInfoAtPoint(int x, int y) {

   delete gPointInfo.image;
   delete gPointInfo.link;
   delete gPointInfo.frame;
   delete gPointInfo.page;
   gPointInfo.image = NULL;
   gPointInfo.link  = NULL;
   gPointInfo.frame = NULL;
   gPointInfo.page  = NULL;


   if (!theApp.m_pMostRecentBrowserFrame || !theApp.m_pMostRecentBrowserFrame->m_wndBrowserView)
      return &gPointInfo;
      
   CBrowserView *pBrowserView;  
   pBrowserView = &theApp.m_pMostRecentBrowserFrame->m_wndBrowserView;

   if (!pBrowserView)
      return &gPointInfo;


   // get the page url
   int len = pBrowserView->GetCurrentURI(NULL);
   if (len) {
      gPointInfo.page = new char[len+1];
      pBrowserView->GetCurrentURI(gPointInfo.page);
   }


   // get the DOMNode at the point
   nsCOMPtr<nsIDOMNode> aNode;
   aNode = pBrowserView->GetNodeAtPoint(x, y, TRUE);
   if (!aNode) {
      // MessageBox(NULL, "no node", NULL, MB_OK);
      return &gPointInfo;
   }


   
   nsAutoString strBuf;
   nsresult rv = NS_OK;


   // check if there's a link
   // Search for an anchor element
   nsCOMPtr<nsIDOMHTMLAnchorElement> linkElement;
   nsCOMPtr<nsIDOMNode> node = aNode;
   while (node) {
      linkElement = do_QueryInterface(node);
      if (linkElement)
         break;
      
      nsCOMPtr<nsIDOMNode> parentNode;
      node->GetParentNode(getter_AddRefs(parentNode));
      node = parentNode;
   }
   if (linkElement) {
      rv = linkElement->GetHref(strBuf);
      if(NS_SUCCEEDED(rv)) {
         if (strBuf.Length()) {
            gPointInfo.link = new char[strBuf.Length() + 1];
            strBuf.ToCString(gPointInfo.link, strBuf.Length() + 1);
         }
      }
   }


   // check for an image
   nsCOMPtr<nsIDOMHTMLImageElement> imgPointInfo(do_QueryInterface(aNode, &rv));
   if(NS_SUCCEEDED(rv)) {
      rv = imgPointInfo->GetSrc(strBuf);
      if(NS_SUCCEEDED(rv)) {
         gPointInfo.image = new char[strBuf.Length() + 1];
         strBuf.ToCString(gPointInfo.image, strBuf.Length() + 1);
      }
   }


   // get the current Frame URL
   nsCOMPtr<nsIDOMDocument> domDoc;
   rv = aNode->GetOwnerDocument(getter_AddRefs(domDoc));
   
   if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(domDoc, &rv));
      if(NS_SUCCEEDED(rv)) {
         rv = htmlDoc->GetURL(strBuf);
         if(NS_SUCCEEDED(rv)) {
            gPointInfo.frame = new char[strBuf.Length() +1];
            strBuf.ToCString(gPointInfo.frame , strBuf.Length() +1);
         }
      }
   }


   return &gPointInfo;
}


// return 0 if the function did not succeed (ie, trying to open a link from
// a point that is not a link)
// return 1 if it does succeed

int CommandAtPoint(int command, WORD x, WORD y) {
   CBrowserView *pBrowserView;  
   pBrowserView = &theApp.m_pMostRecentBrowserFrame->m_wndBrowserView;


   if (!pBrowserView)
      return FALSE;

   nsCOMPtr<nsIWebBrowserFocus> focus(do_GetInterface(pBrowserView->mWebBrowser));
   if(!focus)
      return FALSE;

   focus->Activate();

   
   pBrowserView->GetNodeAtPoint(x, y, TRUE);

   switch (command) {
   case ID_OPEN_LINK:
   case ID_OPEN_LINK_IN_NEW_WINDOW:
   case ID_OPEN_LINK_IN_BACKGROUND:
   case ID_SAVE_LINK_AS:
   case ID_COPY_LINK_LOCATION:
      if (!pBrowserView->mCtxMenuLinkUrl.Length())
         return 0;
      break;

   case ID_VIEW_IMAGE:
   case ID_SAVE_IMAGE_AS:
   case ID_COPY_IMAGE_LOCATION:
   case ID_COPY_IMAGE_CONTENT:
      if (!pBrowserView->mCtxMenuImgSrc.Length())
         return 0;
      break;

   case ID_OPEN_FRAME:
   case ID_OPEN_FRAME_IN_BACKGROUND:
   case ID_OPEN_FRAME_IN_NEW_WINDOW:
   case ID_VIEW_FRAME_SOURCE:
      if (!pBrowserView->mCtxMenuCurrentFrameURL.Length())
         return 0;
      break;
   }

   pBrowserView->mpBrowserFrame->PostMessage(WM_COMMAND, command, NULL);
   return 1;
}

int GetGlobalVar(enum PREFTYPE type, char *preference, void *ret) {

   int retLen = 0;

   CBrowserView *pBrowserView;  
   pBrowserView = &theApp.m_pMostRecentBrowserFrame->m_wndBrowserView;


   if (!pBrowserView)
      return retLen;



   
   switch (type) {
   case PREF_STRING:
      if (!stricmp(preference, "URL")) {
         retLen = pBrowserView->GetCurrentURI(NULL);
         if (ret)
            pBrowserView->GetCurrentURI((char *)ret);
      }
      if (!stricmp(preference, "LinkURL")) {
         retLen = pBrowserView->mCtxMenuLinkUrl.Length();
         if (ret) pBrowserView->mCtxMenuLinkUrl.ToCString((char*)ret , retLen+1);
      }
      if (!stricmp(preference, "ImageURL")) {
         retLen = pBrowserView->mCtxMenuImgSrc.Length();
         if (ret) pBrowserView->mCtxMenuImgSrc.ToCString((char*)ret , retLen+1);
      }
      if (!stricmp(preference, "FrameURL")) {
         retLen = pBrowserView->mCtxMenuCurrentFrameURL.Length();
         if (ret) pBrowserView->mCtxMenuCurrentFrameURL.ToCString((char*)ret , retLen+1);
      }
      if (!stricmp(preference, "TITLE")) {
         CString title;
         pBrowserView->GetPageTitle(title);
         retLen = title.GetLength();
         if (ret) strcpy((char *)ret, title);
      }
      break;
   }
    
   return retLen;
}

static char *safe_strdup(const char *ptr) {
  if (ptr)
    return strdup(ptr);
  return NULL;
}

char *EncodeUTF8(const char *str) {
  USES_CONVERSION;
  nsAutoString aStr;
  aStr.Append(T2W(str));
  char *pszStr = safe_strdup(NS_ConvertUCS2toUTF8(aStr).get());
  return pszStr;
}

char *DecodeUTF8(const char *str) {
  USES_CONVERSION;
  char *pszStr = safe_strdup(W2T(NS_ConvertUTF8toUCS2(str).get()));
  return pszStr;
}

void GetBrowserviewRect(HWND mainWnd, RECT *rc) {
   CBrowserFrame *frame = (CBrowserFrame *)CWnd::FromHandle(mainWnd);
   if (frame)
      frame->m_wndBrowserView.GetWindowRect(rc);
}

HMENU GetMenu(char *menuName){
   CMenu *menu;
   menu = theApp.menus.GetMenu(menuName);

   return menu ? menu->m_hMenu : NULL;
}

void SetForceCharset(char *aCharset) {
   theApp.m_pMostRecentBrowserFrame->m_wndBrowserView.ForceCharset(aCharset);
}

void SetCheck(int id, BOOL mark) {
  theApp.menus.SetCheck(id, mark);
}

long CPlugins::SendMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   long retVal = 0;

   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc( pos, s, kPlugin);
      if (kPlugin->loaded && kPlugin->DoMessage) {
         retVal += kPlugin->DoMessage(to, from, subject, data1, data2);
      }
   }
   return retVal;
}

long SendMessage(const char *to, const char *from, const char *subject, long data1, long data2)
{
   // * is reserved for internal k-meleon messages.  plugins may not use it
   if (from[0] == '*') {
      return 0;
   }

   return theApp.plugins.SendMessage(to, from, subject, data1, data2);
}

kmeleonFunctions kmelFuncs = {
   SendMessage,
   GetCommandIDs,
   NavigateTo,
   GetDocInfo,
   GetPreference,
   SetPreference,
   SetStatusBarText,
   GetMozillaSessionHistory,
   GotoHistoryIndex,
   RegisterBand,
   CreateToolbar,
   GetID,
   GetInfoAtPoint,
   CommandAtPoint,
   GetGlobalVar,
   EncodeUTF8,
   DecodeUTF8,
   GetBrowserviewRect,
   GetMenu,
   SetForceCharset,
   SetCheck
};

BOOL CPlugins::TestLoad(const char *file, const char *description)
{
   char preference[128] = "kmeleon.plugins.";
   strcat(preference, file);
   strcat(preference, ".load");
   
   int load = theApp.preferences.GetBool(preference, -1);
   if (load == -1) {
      CString message;

      message.Format(IDS_NEW_PLUGIN_FOUND, description);

      if (MessageBox(NULL, message, "Plugin found", MB_YESNO) == IDYES)
         load = 1;
      else
         load = 0;

      theApp.preferences.SetBool(preference, load);
   }
   return load;
}

kmeleonPlugin * CPlugins::Load(char *file)
{  
   file = SkipWhiteSpace(file);
   TrimWhiteSpace(file);

   const char *noPath = FileNoPath(file);
   strlwr((char*)noPath);

   // truncate the .dll extension
   char *dot = strrchr(noPath, '.');
   if (dot && (strcmpi(dot, ".dll") == 0) )
      *dot = 0;
   else
      dot = NULL;

   // check if the plugin is already loaded
   kmeleonPlugin * kPlugin;
   if (pluginList.Lookup(noPath, kPlugin))
      return kPlugin; // it's already loaded

   // restore the '.' in the truncated string
   if (dot)
      *dot = '.';

   HINSTANCE plugin;

   char *c = file;
   while (*c && *c != ':') c++;

   // we need to append .dll because NT4 gets confused if a directory in the path
   // contains a '.' and the file to be loaded does not
   if (!*c || !dot) {        // if pattern does not contain : we need to prepend pluginsDir
                              // if it doesn't end in .dll, we need to append that
      int newlen = strlen(file);
      if (!*c) newlen += strlen(theApp.preferences.pluginsDir);
      if (!dot) newlen += 4; // ".dll"

      char *buf = new char[newlen+1];
      *buf = 0;

      if (!*c) strcpy(buf, theApp.preferences.pluginsDir);
      strcat(buf, file);
      if (!dot) strcat(buf, ".dll");

      plugin = LoadLibrary(buf);    // load the full path
      delete buf;
   }
   else {
      DWORD err = GetLastError();
      plugin = LoadLibrary(file);
      err = GetLastError();
   }

   KmeleonPluginGetter kpg = (KmeleonPluginGetter)GetProcAddress(plugin, "GetKmeleonPlugin");

   if (!kpg) {
      FreeLibrary(plugin);
      return NULL;
   }

   kPlugin = kpg();

   if (!kPlugin) {
      FreeLibrary(plugin);
      return NULL;
   }

   kPlugin->hParentInstance = AfxGetInstanceHandle();
   kPlugin->hDllInstance = plugin;
   kPlugin->kFuncs = &kmelFuncs;

   // truncate the .dll extension in noPath
   if (dot)
      *dot = 0;

   // save the filename
   char *name = new char[strlen(noPath)+1];
   strcpy(name, noPath);
   kPlugin->dllname=name;

   int loaded = kPlugin->loaded=TestLoad(noPath, kPlugin->description);

   if (kPlugin->version < KMEL_PLUGIN_VER_MAJOR) {
      CString error;
      error.Format(IDS_OLD_PLUGIN, kPlugin->description);

      AfxMessageBox(error);

      loaded = false;
   }

   // If the plugin is enabled, tell it to Init
   if ( loaded ) {
      kPlugin->DoMessage(kPlugin->dllname, "* Plugin Manager", "Load", 0, 0);
   }
   // otherwise, make a copy of the descripion, and unload it
   else {
      kmeleonPlugin *temp = new kmeleonPlugin;

      char *sBuf = new char[strlen(kPlugin->description)+1];
      temp->description = sBuf;
      strcpy(temp->description, kPlugin->description);

      temp->dllname = name;

      temp->loaded = false;

      kPlugin=temp;

      FreeLibrary(plugin);
   }

   pluginList.SetAt(noPath, kPlugin);
   
   return kPlugin;
}

int CPlugins::FindAndLoad(const char *pattern)
{
   CString filepath;
   CFileFind finder;
   BOOL bWorking;
   
   int x=strlen(pattern);
   while (x>0 && (pattern[x] != ':')) x--;
   
   if (x==0) {       // if pattern does not contain ':' we need to prepend pluginsDir
      CString search = theApp.preferences.pluginsDir + pattern;
      bWorking = finder.FindFile(search);
   }
   else bWorking = finder.FindFile(pattern);

   int i = 0;
   while (bWorking) {
      bWorking = finder.FindNextFile();

      filepath = finder.GetFilePath();
      if ( Load(filepath.LockBuffer()) )
         i++;
      filepath.UnlockBuffer();
   }
   SendMessage("*", "* Plugin Manager", "Init");
   return i;
}

void CPlugins::UnLoadAll()
{
   SendMessage("*", "* Plugin Manager", "Quit");

   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc(pos, s, kPlugin);
      if (kPlugin) {
         delete kPlugin->dllname;
         if (kPlugin->loaded) {
            FreeLibrary(kPlugin->hDllInstance);
         }
         else  { // the plugin was disabled, delete the copied description
            delete kPlugin->description;
            delete kPlugin;
         }
      }
   }
   pluginList.RemoveAll();
   currentCmdID = PLUGIN_COMMAND_START_ID;
}

int CPlugins::GetConfigFiles(configFileType *configFiles, int maxFiles)
{
   int numFiles = 0;
   POSITION pos = pluginList.GetStartPosition();
   kmeleonPlugin * kPlugin;
   CString s;
   while (pos) {
      pluginList.GetNextAssoc( pos, s, kPlugin);

      if (kPlugin->loaded && kPlugin->DoMessage) {
         configFileType *tempConfigFiles;
         int numTempConfigFiles=0;
         kPlugin->DoMessage(kPlugin->dllname, "* Plugin Manager", "GetConfigFiles", (long)&tempConfigFiles, (long)&numTempConfigFiles);
         int i = 0;
         while (numFiles < maxFiles && i < numTempConfigFiles) {
            memcpy(&configFiles[numFiles++], &tempConfigFiles[i++], sizeof(configFileType));
         }
      }
   }
   return numFiles;
}
