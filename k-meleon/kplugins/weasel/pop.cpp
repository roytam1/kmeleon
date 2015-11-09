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

#include "stdafx.h"
#include "pop.h"

class CAutoDeleteBuffer {
protected:
   char *buffer;

public:
   CAutoDeleteBuffer() { buffer = NULL; }
   ~CAutoDeleteBuffer() { delete buffer; }

   inline char * operator =(char * newBuffer) {
      if (buffer)
         delete buffer;

      buffer = newBuffer;

      return buffer;
   }
   inline operator char *() {
      return buffer;
   }
   inline char operator[](int index) {
      return buffer[index];
   }
};

// will malloc, must free the buffer!
char *CPOP::GetSockData()
{
  // this little peek is here so we'll sit until we have data
  char peek[2];
  recv(sock, peek, 1, MSG_PEEK);

  unsigned long size;
  ioctlsocket(sock, FIONREAD, &size);

  char *buffer = new char[size+1];

  size = recv(sock, buffer, size, 0);
  buffer[size] = 0;

  return buffer;
}

int CPOP::Setup()
{
   struct sockaddr_in cli_addr;

   sock = socket(PF_INET, SOCK_STREAM, 0);

   if (sock == INVALID_SOCKET){
	   SocketError("Invalid Socket");
	   return 0;
   }

   cli_addr.sin_family = AF_INET;
   cli_addr.sin_addr.s_addr = INADDR_ANY;
   cli_addr.sin_port = 0;

   if ( bind(sock, (LPSOCKADDR)&cli_addr, sizeof(cli_addr)) == SOCKET_ERROR ) {
	   SocketError("Could not Bind Socket to address");
     closesocket(sock);
	   return 0;
   }

  return 1;
}

int CPOP::Connect(const char *szServerName)
{
   if (!sock) {
      SocketError("Called Connect without a valid socket");
   }
   
   struct sockaddr_in srv_addr;
   LPHOSTENT host_info;

   host_info = gethostbyname(szServerName);
   if (host_info == NULL){
      SocketError("Could not lookup hostname %s!", szServerName);
      closesocket(sock);
	   return 0;
   }
   srv_addr.sin_family = AF_INET;	/* casts are fun :)	*/
   srv_addr.sin_addr.S_un.S_addr = *(unsigned long *)*(unsigned long *)host_info->h_addr_list;
   srv_addr.sin_port = htons(110); // 110 = POP

   if ( connect(sock, (LPSOCKADDR)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR ){
	   SocketError("Could Not Connect to %s!", szServerName);
     closesocket(sock);
	   return 0;
   }

   return 1;
}

int CPOP::Login(const char *szUsername, const char *szPassword)
{
  if (!sock) {
     SocketError("Called Login without a valid socket");
  }

  CAutoDeleteBuffer buffer;

  /* get initial server message */
  buffer = GetSockData();

  if (*buffer != '+') {
     SocketError("Server did not say hello!");
     closesocket(sock);
     return 0;
  }

  /* send username */
  char userline[256];
  sprintf(userline, "USER %s\n", szUsername);
  send(sock, userline, strlen(userline), 0);

  buffer = GetSockData();

  if (*buffer != '+') {
     SocketError("Server did not like your username!");
     SocketError("Server Said %s", (char *)buffer);
     closesocket(sock);
     return 0;
  }

  /* send password */
  char passline[256];
  sprintf(passline, "PASS %s\n", szPassword);
  send (sock, passline, strlen(passline), 0);

  buffer = GetSockData();

  if (*buffer != '+') {
     SocketError("Server did not like your password!");
     closesocket(sock);
     return 0;
  }

  return 1;
}

int CPOP::NumberOfMessages()
{
  if (!sock) {
     SocketError("Called NumberOfMessages without a valid socket");
  }

  CAutoDeleteBuffer buffer;

  send (sock, "STAT\n", 5, 0);

  buffer = GetSockData();

  if (*buffer != '+') {
     SocketError("Server did send the proper stat reply!");
     closesocket(sock);
     return 0;
  }
  else {
     // stat returns something like: +OK 1 819
     // the +4 jumps over the "+OK " and atoi will stop parsing at the second space
     // so we'd return 1
     return atoi(buffer+4);
  }
}

int CPOP::Quit()
{
  if (!sock) {
     SocketError("Called Quit without a valid socket");
  }

  send(sock, "QUIT\n", 5, 0);

  closesocket(sock);

  sock = NULL;

  return 1;
}
