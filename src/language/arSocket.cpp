//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLogStream.h"
#include "arSocket.h"

#include <stdio.h>
#include <errno.h>

using namespace std;

#ifdef AR_USE_WIN_32
#include "arDataUtilities.h" // for ar_winSockInit()

// Helper class, singleton pattern for winsock init.
class arWinSockHelper{
 public:
  arWinSockHelper(): _fInit(false) {}
  bool init() {
    _lock.lock();
    // If we do not need to initialize, return true.
    bool ok = true;
    if (!_fInit) {
      ok = ar_winSockInit();
      if (!ok) {
        ar_log_error() << "WinSock failed to initialize.\n";
      }
      _fInit = true;
    }
    _lock.unlock();
    return ok;
  }

 private:
  bool _fInit;
  arLock _lock;
};

bool ar_winSockHelper() {
  static arWinSockHelper h;
  return h.init();
}
#endif

arCommunicator::arCommunicator() {
#ifdef AR_USE_WIN_32
  (void) ar_winSockHelper();
#endif
}

///////////// Okay, now the actual arSocket code begins... ///////////

arSocket::arSocket(int type) : _type(type), _ID(-1) {
#ifdef AR_USE_WIN_32
  _socketFD = INVALID_SOCKET;
#else
  _socketFD = -1;
#endif
  if (_type != AR_LISTENING_SOCKET && _type != AR_STANDARD_SOCKET) {
    ar_log_error() << "arSocket ignoring unexpected type " << _type << ".\n";
    _type = AR_STANDARD_SOCKET;
  }
}

arSocket::~arSocket() {
  //**********************************************************************
  // This is causing problems on IRIX, specifically when we've issued
  // a blocking-read call on a socket in another thread, the close
  // call will block. This can cause hanging in the destructors in,
  // for instance, arSZGClient. A temporary work-around is to require
  // an explicit close of the socket
  //**********************************************************************
  //ar_close();
}

void arSocket::setID(int theID) {
  _ID = theID;
}

int arSocket::ar_create() {
#ifdef AR_USE_WIN_32
  _socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_socketFD == INVALID_SOCKET)
#else
  _socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (_socketFD < 0)
#endif
  {
    ar_log_error() << "arSocket: socket() failed: ";
    perror("");
    return -1;
  }
  return 0;
}

bool arSocket::setReceiveBufferSize(int size) {
  if (setsockopt(_socketFD, SOL_SOCKET, SO_RCVBUF, (char*)&size,
                 sizeof(int)) < 0) {
    perror("arSocket error: setReceiveBufferSize failed");
    return false;
  }
  return true;
}

bool arSocket::setSendBufferSize(int size) {
  if (setsockopt(_socketFD, SOL_SOCKET, SO_SNDBUF, (char*)&size,
                 sizeof(int)) < 0) {
    perror("arSocket error: setSendBufferSize failed");
    return false;
  }
  return true;
}

bool arSocket::smallPacketOptimize(bool flag) {
  const int parameter = flag ? 1 : 0;
  if (setsockopt(_socketFD, IPPROTO_TCP, TCP_NODELAY, (const char*)&parameter,
                 sizeof(int)) < 0) {
    perror("arSocket error: smallPacketOptimize failed");
    return false;
  }
  return true;
}

bool arSocket::reuseAddress(bool flag) {
  const int parameter = flag ? 1 : 0;
  if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&parameter,
                 sizeof(int)) < 0) {
    perror("arSocket error: reuseAddress failed");
    return false;
  }
  return true;
}

int arSocket::ar_connect(const char* IPaddress, int port) {
  if (_type != AR_STANDARD_SOCKET) {
    return -1;
  }
  // ar_log_debug() << "arSocket connecting to " << IPaddress << ":" << port << ".\n";
  sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
#ifdef AR_USE_WIN_32
  servAddr.sin_addr.S_un.S_addr = inet_addr(IPaddress);
  const int ok = connect(_socketFD, (sockaddr*)&servAddr, sizeof(servAddr));
#else
  inet_pton(AF_INET, IPaddress, &(servAddr.sin_addr));
  const int fOriginal = fcntl(_socketFD, F_GETFL, NULL);
  fcntl(_socketFD, F_SETFL, fOriginal | O_NONBLOCK);

  // connect() blocks forever if dwho'ing or dps'ing to a no-longer-running szgserver.
  // Even restarting that szgserver doesn't unblock connect().
  int ok = connect(_socketFD, (sockaddr*)&servAddr, sizeof(servAddr));
  if (ok == -1) {
    if (errno != EINPROGRESS) {
      perror("ar_connect failed");
    }
    else {
      /*
      man connect says:
      The socket is non-blocking and the connection cannot be completed
      immediately.  Call select(2) for completion by selecting the socket
      for writing.  After select indicates writability, use getsockopt(2)
      to read the SO_ERROR option at level SOL_SOCKET to determine whether
      connect completed successfully (SO_ERROR is zero) or unsuccessfully
      (SO_ERROR is one of the usual error codes listed here, explaining
      the reason for the failure).
      */

      fd_set s;
      struct timeval tv;
      // Timeout: 30 x 0.1 seconds = 3 seconds.
      int iTry = 30;
      while (--iTry > 0) {
	FD_ZERO(&s);
	FD_SET(_socketFD, &s);
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	const int r = select(1+_socketFD, NULL, &s, NULL, &tv);
	if (r == -1)
	  perror("select() failed");
	else if (r != 0) {
	  // Writable.
	  ok = 0;
	  break;
	}
      }
      if (ok != 0) {
	// Timed out.  Other end may be absent.
	ok = -1;
      }
      else {
	int err = 0;
#ifdef AR_USE_SGI
typedef int socklen_t;
#endif
	socklen_t errlen = sizeof(err);
	int foo = getsockopt(_socketFD, SOL_SOCKET, SO_ERROR, &err, &errlen);
	if (foo != 0)
	  goto LError;
	if (err != 0) {
	  errno = err; // hack
LError:
	  perror("arSocket connection worked only partially");
	  ok = -1;
	}
      }
    }
  }

  fcntl(_socketFD, F_SETFL, fOriginal);
#endif

  if (ok < 0) {
#ifdef AR_USE_WIN_32
    ar_log_debug() << "arSocket connection failed, winsock error " <<
      WSAGetLastError() << ".\n";
#else
    ar_log_debug() << "arSocket connection failed.\n";
#endif
  }

  return ok;
}

int arSocket::ar_bind(const char* IPaddress, int port) {
  if (_type != AR_LISTENING_SOCKET) {
    ar_log_error() << "arSocket: ar_bind() needs an AR_LISTENING_SOCKET.\n";
    return -1;
  }

  sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
#ifdef AR_USE_WIN_32
  servAddr.sin_addr.S_un.S_addr = IPaddress == NULL ?
    htonl(INADDR_ANY) : inet_addr(IPaddress);
#else
  if (IPaddress == NULL)
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    inet_pton(AF_INET, IPaddress, &servAddr.sin_addr);
#endif
  const int err = bind(_socketFD, (sockaddr*)&servAddr, sizeof(servAddr));
  if (err < 0)
    {
    ar_log_error() << "arSocket bind failed to " <<
      (IPaddress == NULL ? "(localhost)" : IPaddress) << ":" << port << ": ";
    perror("");
    }
  return err;
}

int arSocket::ar_listen(int queueSize) {
  if (_type != AR_LISTENING_SOCKET) {
    ar_log_error() << "arSocket: ar_listen() needs an AR_LISTENING_SOCKET.\n";
    return -1;
  }
  return listen(_socketFD, queueSize);
}

// If addr != NULL, stuff it with who we accepted a connection from.
int arSocket::ar_accept(arSocket* communicationSocket, arSocketAddress* addr) {
  if (!communicationSocket) {
    ar_log_error() << "arSocket can't accept on null socket.\n";
    return -1;
  }
  communicationSocket->_socketFD = -1;

  // must be called by a listening socket and operate on a standard socket
  if (_type != AR_LISTENING_SOCKET ||
      communicationSocket->_type != AR_STANDARD_SOCKET) {
    ar_log_error() << "arSocket: ar_accept() needs an AR_LISTENING_SOCKET and an AR_STANDARD_SOCKET.\n";
    return -1;
  }

  if (_socketFD < 0) {
    ar_log_error() << "arSocket: ar_accept() got a bad _socketFD.\n" <<
      "  (Is this a Renderer which is erroneously Accepting or Listening?)\n";
    return -1;
  }

  arSocketAddress socketAddress;
  for (;;) {
    communicationSocket->_socketFD =
      accept( _socketFD, (sockaddr*)socketAddress.getAddress(),
#ifdef AR_USE_DARWIN
        (socklen_t*)
#endif
            socketAddress.getAddressLengthPtr());
    if (communicationSocket->_socketFD < 0) {
      ar_log_error() << "arSocket: accept() failed: ";
      perror("");
      return -1;
    }

    if (socketAddress.checkMask(_acceptMask))
      // Accept mask allowed a connection.
      break;

    communicationSocket->ar_close();
    // Retry.
  }

  if (addr)
    *addr = socketAddress;
  return 0;
}

int arSocket::ar_read(char* theData, const int numBytes) const {
#ifdef AR_USE_WIN_32
  return recv(_socketFD, theData, numBytes, 0);
#else
  return read(_socketFD, theData, numBytes);
#endif
}

bool arSocket::readable(const ar_timeval& timeout) const {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(_socketFD, &fds);
  struct timeval tv;
  tv.tv_sec = timeout.sec;
  tv.tv_usec = timeout.usec;
  return select(_socketFD+1, &fds, NULL, NULL, &tv) == 1;
}

bool arSocket::writable(const ar_timeval& timeout) const {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(_socketFD, &fds);
  struct timeval tv;
  tv.tv_sec = timeout.sec;
  tv.tv_usec = timeout.usec;
  return select(_socketFD+1, NULL, &fds, NULL, &tv) == 1;
}

bool arSocket::readable() const {
  return readable(ar_timeval(0, 0));
}

bool arSocket::writable() const {
  return writable(ar_timeval(0, 0));
}

#ifndef AR_USE_WIN_32
#ifdef DEBUGGING_BLOCKED_SENDS
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/sockios.h> // for SIOCSIFTXQLEN
#include <linux/if.h> // for struct ifreq
#endif
#endif

int arSocket::ar_write(const char* theData, int numBytes) const {
#ifndef AR_USE_WIN_32
	// UNIX code

#ifdef DEBUGGING_BLOCKED_SENDS
{
struct stat x;
fstat(_socketFD, &x);
ino_t inode = x.st_ino;
char cmd[200];
sprintf(cmd, "/bin/grep '%ld[ ]*$' /proc/net/tcp", inode);
cerr << "system():  <" << cmd << ">\n";
system(cmd);

struct ifreq ifr;
ioctl(_socketFD, SIOCSIFTXQLEN, &ifr);

cerr << "attempt #2: queue length == " << ifr.ifr_qlen << endl;

//cat /proc/net/tcp
//sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode
//0: 2A7F7E82:271A 2A7F7E82:10B2 01 00002F08:00002000 01:00000042 00000002 12159        0 318758
//1: 2A7F7E82:10B2 2A7F7E82:271A 01 00001B00:00000000 01:00000056 00000003 12159        0 318761
}
#endif

  return write(_socketFD, theData, numBytes);
#else
  // Win32
  return send(_socketFD, theData, numBytes, 0);
#endif
}

int arSocket::ar_safeRead(char* theData, int numBytes, const double usecTimeout) {
  const bool fTimeout = usecTimeout > 0.;
  const ar_timeval tStart = ar_time();
  ++_usageCount;
  arSleepBackoff a(6, 25, 1.1);

  while (numBytes>0) {
    if (fTimeout) {
      const double usecElapsed = ar_difftime(ar_time(), tStart);
      if (usecElapsed > usecTimeout) {
	// timed out
        return false;
      }
      if (!readable()) {
        a.sleep();
	continue;
      }
      a.reset();
    }
    const int n = ar_read(theData, numBytes);
    if (n <= 0) {
      //  <0: failed to read from socket.
      // ==0: socket closed, but caller wants still more bytes.
      --_usageCount;
      return false;
      }
    numBytes -= n;
    theData += n;
  }
  --_usageCount;
  return true;
}

int arSocket::ar_safeWrite(const char* theData, int numBytes, const double usecTimeout) {
  const bool fTimeout = usecTimeout > 0.;
  const ar_timeval tStart = ar_time();
  ++_usageCount;
  arSleepBackoff a(6, 25, 1.1);

  while (numBytes>0) {
    if (fTimeout) {
      const double usecElapsed = ar_difftime(ar_time(), tStart);
      if (usecElapsed > usecTimeout) {
	// timed out
        return false;
      }
      if (!writable()) {
        a.sleep();
	continue;
      }
      a.reset();
    }
    const int n = ar_write(theData, numBytes);
    if (n<0) {
      // an error in writing to the socket
      --_usageCount;
      return false;
    }
    numBytes -= n;
    theData += n;
  }
  --_usageCount;
  return true;
}

int arSocket::getUsageCount() {
  return _usageCount;
}

void arSocket::ar_close() {
#ifdef AR_USE_WIN_32
  if (_socketFD != INVALID_SOCKET) {
    closesocket(_socketFD);
  }
#else
  if (_socketFD != -1) {
    close(_socketFD);
  }
#endif
}
