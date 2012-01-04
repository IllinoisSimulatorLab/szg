//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOCKET_H
#define AR_SOCKET_H

#ifdef AR_USE_WIN_32
#include "arPrecompiled.h"
#endif

#include "arDataUtilities.h"
#include "arSocketAddress.h"
#include "arThread.h" // for arIntAtom

#include "arLanguageCalling.h"

#ifndef AR_USE_WIN_32
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
#include <string>
#include <list>

using namespace std;

class ar_timeval; // forward reference into arDataUtilities.h

#ifdef AR_USE_WIN_32
SZG_CALL bool ar_winSockInit();
#endif

// Helper: ensure that win32 initializes before the first arSocket is constructed.
class SZG_CALL arCommunicator {
public:
  arCommunicator();
  virtual ~arCommunicator() {}
};

SZG_CALL enum arSocketType { AR_LISTENING_SOCKET=1, AR_STANDARD_SOCKET=2 };

// TCP socket.

class SZG_CALL arSocket: public arCommunicator {
public:
  arSocket(int type);
  ~arSocket();

  void setID(int theID);
  int getID() const
    { return _ID; }

  int ar_create();
  int ar_connect(const char* IPaddress, int port);
  bool setReceiveBufferSize(int size);
  bool setSendBufferSize(int size);
  bool smallPacketOptimize(bool flag);
  bool reuseAddress(bool flag);
  int ar_bind(const char* IPaddress, int port);
  int ar_listen(int queueSize);
  int ar_accept(arSocket*, arSocketAddress* addr = NULL);
  bool readable(const ar_timeval& timeout) const;
  bool writable(const ar_timeval& timeout) const;
  bool readable() const; // poll with no timeout
  bool writable() const;
  int ar_read(char* theData, const int numBytes) const;
  int ar_write(const char* theData, int numBytes) const;

  // "Safe" versions keep usage counts, and are guaranteed to return
  // the number of bytes requested, or an error.
  int ar_safeRead(char* theData, int howMuch, const double usecTimeout = 0.);
  int ar_safeWrite(const char* theData, int howMuch, const double usecTimeout = 0.);

  int getUsageCount();
  void ar_close();
  // This is a tcp-wrappers-esque feature. As explained below, the mask
  // allows ar_accept to automatically drop attempted connections,
  // based on IP address.
  void setAcceptMask(list<string>& mask) { _acceptMask = mask; }

private:
  int _type;
  int _ID;
  // since objects like arDataServer use sockets full duplex
  // with reads and writes occuring in different threads
  // clean-up requires that we can keep track of the number of
  // active operations (ar_read or ar_write) on a given socket at a given time.
  arIntAtom _usageCount;
#ifdef AR_USE_WIN_32
  SOCKET _socketFD;
#else
  int _socketFD;
#endif
  // See arSocketAddress to understand the format of _acceptMask.
  list<string> _acceptMask;
};

#endif
