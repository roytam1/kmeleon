class CCmdLine {

public:
   CCmdLine();
   ~CCmdLine();
   Initialize(char *cmdLine);
   GetSwitch(const char *pSwitch, char *pArgs, BOOL bRemove);
   
   char *m_sProfilesDir;
   char *m_sCmdLine;
};