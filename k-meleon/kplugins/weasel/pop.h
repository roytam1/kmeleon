#include <winsock2.h>

class CPOP
{
protected:
   SOCKET sock;

   char *GetSockData();

   void SocketError(char *format, ...);

public:
   CPOP(){ sock = NULL; }

   int Setup();
   int Connect(const char *szServerName);
   int Login(const char *szUsername, const char *szPassword);
   int NumberOfMessages();
   int Quit();

   ~CPOP(){
      if (sock) { 
         Quit();
      }
   }
};
