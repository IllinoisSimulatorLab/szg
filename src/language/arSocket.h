//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOCKET_H
#define AR_SOCKET_H

#ifdef AR_USE_WIN_32
// For some unknown reason, it is necessary to include this here
// (for some apps). 
#include <winsock2.h>
#include <windows.h>
#include "arThread.h" // for arMutex in arCommunicator
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

#include "arThread.h"
#include <string>
using namespace std;

class ar_timeval; // forward reference into arDataUtilities.h

/// Helper class:
/// Ensures that win32 initializes before the first arSocket is constructed.

#ifdef AR_USE_WIN_32
class arCommunicatorHelper {
public:
  int _init;
  arCommunicatorHelper();
};
#endif

/// Helper class:
/// Ensures that win32 initializes before the first arSocket is constructed.

class arCommunicator {
#ifdef AR_USE_WIN_32
  static arMutex _lock;
  static bool _init;
public:
  arCommunicator();
  friend class arCommunicatorHelper;
#endif
public:
  virtual ~arCommunicator() {}
};

enum arSocketType{AR_LISTENING_SOCKET=1, AR_STANDARD_SOCKET=2};

/// TCP socket.

class arSocket: public arCommunicator {
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
  int ar_accept(arSocket* communicationSocket);
  bool readable(const ar_timeval& timeout);
  bool writable(const ar_timeval& timeout);
  bool readable(); // poll with no timeout
  bool writable();
  int ar_read(char* theData, int howMuch);
  int ar_write(const char* theData, int howMuch);
  /// the "safe" versions of read and write keep usage counts and
  /// are guaranteed to return the number of bytes requested 
  /// or an error
  int ar_safeRead(char* theData, int howMuch);
  int ar_safeWrite(const char* theData, int howMuch);
  int getUsageCount();
  void ar_close();
  /// This is a tcp-wrappers-esque feature. As explained below, the mask
  /// allows ar_accept to automatically drop attempted connections, based
  /// on IP address.
  void setAcceptMask(const string& mask){ _acceptMask = mask; }

private:
  int _type;
  int _ID;
  // since objects like arDataServer use sockets full duplex
  // with reads and writes occuring in different threads
  // clean-up requires that we can keep track of the number of
  // active operations (ar_read or ar_write) on a given socket
  // at a given time.
  int     _usageCount;
  arMutex _usageLock;
#ifdef AR_USE_WIN_32
  SOCKET _socketFD;
#else
  int _socketFD;
#endif
  // the below is either XXX.XXX.XXX or XXX.XXX.XXX.XXX. If the former,
  // then ar_accept will only return on IP addresses that match the first
  // 3 numbers of the dotted quad (i.e. something like netmask 255.255.255.0).
  // And if it's the later, then ar_accept will only return on an IP address
  // that precisely matches.
  string _acceptMask;
};

#include "arDataUtilities.h" // for ar_timeval
#endif
