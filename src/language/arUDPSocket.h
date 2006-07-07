//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_UDP_SOCKET_H
#define AR_UDP_SOCKET_H

#ifdef AR_USE_WIN_32
#include "arPrecompiled.h"
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

// UDP socket (as opposed to TCP socket, arSocket).
// Like arSocket, derive from arCommunicator
// to get arCommunicator's implicit WinSock init.

class SZG_CALL arUDPSocket:public arCommunicator{
public:
  arUDPSocket(): _ID(-1) {}
  ~arUDPSocket() {}

  void setID(int theID) { _ID = theID; }
  int getID() const { return _ID; }

  void setBroadcast(bool) const;

  int ar_create();
  void setReceiveBufferSize(int size) const;
  void setSendBufferSize(int size) const;
  void reuseAddress(bool flag) const;
  int ar_bind(arSocketAddress*) const;
  int ar_read(char* theData, int howMuch, arSocketAddress*) const;
  int ar_write(const char* theData, int howMuch, arSocketAddress*) const;
  void ar_close() const;

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
