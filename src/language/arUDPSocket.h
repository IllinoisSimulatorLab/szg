//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_UDP_SOCKET_H
#define AR_UDP_SOCKET_H

#ifdef AR_USE_WIN_32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#endif

#include "arSocket.h"
#include "arSocketAddress.h"
#include "arLanguageCalling.h"

/// UDP socket (as opposed to TCP socket, arSocket).
/// Note that this class needs to descend from arCommunicator, just like
/// arSocket, since we need arCommunicator's implicit WinSock init on the
/// Win32 side

class SZG_CALL arUDPSocket:public arCommunicator{
public:
  arUDPSocket(): _ID(-1) {}
  ~arUDPSocket() {}

  void setID(int theID);
  int getID();

  void setBroadcast(bool);

  int ar_create();
  void setReceiveBufferSize(int size);
  void setSendBufferSize(int size);
  void reuseAddress(bool flag);
  int ar_bind(arSocketAddress*);
  int ar_read(char* theData, int howMuch, arSocketAddress*);
  int ar_write(const char* theData, int howMuch, arSocketAddress*);
  void ar_close();

private:
  int _type;
  int _ID;
#ifdef AR_USE_WIN_32
  SOCKET _socketFD;
#else
  int _socketFD;
#endif
};

#endif
