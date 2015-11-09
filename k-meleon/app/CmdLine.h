class CCmdLine {

public:
   CCmdLine();
   ~CCmdLine();
   void Initialize(char *cmdLine);
   int GetSwitch(const char *pSwitch, char *pArgs, BOOL bRemove);
   
   char *m_sProfilesDir;
   char *m_sCmdLine;
   BOOL  m_bChrome;
};