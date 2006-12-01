//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSocket.h"
#include <stdio.h>
#include <errno.h>
using namespace std;

// Only need to initialize sockets in the windows case.
#ifdef AR_USE_WIN_32
#include "arDataUtilities.h" // for ar_winSockInit()

// Helper class that implements the singleton pattern for winsock init.
class arWinSockHelper{
 public:
  arWinSockHelper(){ _initialized = false; ar_mutex_init(&_lock); }
  ~arWinSockHelper(){}

  bool init(){
    ar_mutex_lock(&_lock);
    // If we do not need to initialize, return true.
    bool state = true;
    if (!_initialized){
      state = ar_winSockInit();
      if (!state){
        cerr << "syzygy error: win sock failed to initialize.\n";
      }
      _initialized = true;
    }
    ar_mutex_unlock(&_lock);
    return state;
  }

  arMutex _lock;
  bool    _initialized;
};

bool ar_winSockHelper(){
  static arWinSockHelper helper;
  return helper.init();
}
#endif

arCommunicator::arCommunicator(){
#ifdef AR_USE_WIN_32
  (void) ar_winSockHelper();
#endif
}

///////////// Okay, now the actual arSocket code begins... ///////////

arSocket::arSocket(int type) :
  _type(type),
  _ID(-1),
  _usageCount(0){

  ar_mutex_init(&_usageLock);
#ifdef AR_USE_WIN_32
  _socketFD = INVALID_SOCKET;
#else
  _socketFD = -1;
#endif
  if (type != AR_LISTENING_SOCKET && type != AR_STANDARD_SOCKET){
    cerr << "warning: ignoring unexpected arSocket type " << type << endl;
    _type = AR_STANDARD_SOCKET;
  }
}

arSocket::~arSocket(){
  //**********************************************************************
  // This is causing problems on IRIX, specifically when we've issued
  // a blocking-read call on a socket in another thread, the close
  // call will block. This can cause hanging in the destructors in,
  // for instance, arSZGClient. A temporary work-around is to require
  // an explicit close of the socket
  //**********************************************************************
  //ar_close();
}

void arSocket::setID(int theID){
  _ID = theID;
}

int arSocket::ar_create(){
#ifdef AR_USE_WIN_32
  _socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_socketFD == INVALID_SOCKET)
#else
  _socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (_socketFD < 0)
#endif
  {
    cerr << "arSocket error: socket() failed: ";
    perror("");
    return -1;
  }
  return 0;
}

bool arSocket::setReceiveBufferSize(int size){
  if (setsockopt(_socketFD, SOL_SOCKET, SO_RCVBUF, (char*)&size,
                 sizeof(int)) < 0){
    perror("arSocket error: setReceiveBufferSize failed");
    return false;
  }
  return true;
}

bool arSocket::setSendBufferSize(int size){
  if (setsockopt(_socketFD, SOL_SOCKET, SO_SNDBUF, (char*)&size,
                 sizeof(int)) < 0){
    perror("arSocket error: setSendBufferSize failed");
    return false;
  }
  return true;
}

bool arSocket::smallPacketOptimize(bool flag){
  const int parameter = flag ? 1 : 0;
  if (setsockopt(_socketFD, IPPROTO_TCP, TCP_NODELAY, (const char*)&parameter,
                 sizeof(int)) < 0){
    perror("arSocket error: smallPacketOptimize failed");
    return false;
  }
  return true;
}

bool arSocket::reuseAddress(bool flag){
  const int parameter = flag ? 1 : 0;
  if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&parameter,
                 sizeof(int)) < 0){
    perror("arSocket error: reuseAddress failed");
    return false;
  }
  return true;
}

#ifdef AR_USE_WIN_32
// this seems to produce false alarms.
#ifdef DISABLED
static void arSocket_watchdog(void* pv)
{
  // approximately non-reentrant, in case many connect's happen simultaneously.
  static int c = 0;
  if (c > 0)
    return;
  ++c;
  ar_usleep(5000000); // how long to wait for connect(), before the impatient user thinks it's hung.
  if (*(bool*)pv)
    cout << "syzygy remark: Windows is slowly trying to open a socket.  Patience...\n";
  --c;
}
#endif
#endif

int arSocket::ar_connect(const char* IPaddress, int port){
  if (_type != AR_STANDARD_SOCKET){
    return -1;
  }
  sockaddr_in servAddr;
  memset(&servAddr,0,sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
#ifdef AR_USE_WIN_32
  servAddr.sin_addr.S_un.S_addr = inet_addr(IPaddress);

#ifdef DISABLED
  bool warning = true;
  // The watchdog will print a warning, if connect() takes too long to return.
  static int c = 0;
  if (++c < 5)
    // Warn the user only a finite number of times.
    arThread(arSocket_watchdog, &warning);
#endif

#else
  inet_pton(AF_INET,IPaddress,&(servAddr.sin_addr)); 
#endif
  const int ok = connect(_socketFD, (sockaddr*)&servAddr, sizeof(servAddr));
#ifdef AR_USE_WIN_32

#ifdef DISABLED
  warning = false;
#endif

#endif
  return ok;
}

int arSocket::ar_bind(const char* IPaddress, int port){
  if (_type != AR_LISTENING_SOCKET){
    cerr << "arSocket error: ar_bind() requires an AR_LISTENING_SOCKET.\n";
    return -1;
  }

  sockaddr_in servAddr;
  memset(&servAddr,0,sizeof(servAddr));
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
    cerr << "arSocket error: bind to "
         << (IPaddress == NULL ? "(localhost)" : IPaddress)
	 << ":" << port << " failed: ";
    perror("");
    }
  return err;
}

int arSocket::ar_listen(int queueSize){
  if (_type != AR_LISTENING_SOCKET){
    cerr << "arSocket error: ar_listen() requires an AR_LISTENING_SOCKET.\n";
    return -1;
  }
  return listen(_socketFD, queueSize);
}

// If addr != NULL, stuff it with who we accepted a connection from.
int arSocket::ar_accept(arSocket* communicationSocket, arSocketAddress* addr){
  if (!communicationSocket) {
    cerr << "arSocket warning: can't accept on null socket.\n";
    return -1;
  }
  communicationSocket->_socketFD = -1;

  // must be called by a listening socket and operate on a standard socket
  if (_type != AR_LISTENING_SOCKET ||
      communicationSocket->_type != AR_STANDARD_SOCKET){
    cerr << "arSocket error: ar_accept() requires an AR_LISTENING_SOCKET and an AR_STANDARD_SOCKET.\n";
    return -1;
  }

  if (_socketFD < 0){
    cerr << "arSocket error: ar_accept() has a bad _socketFD.\n"
         << "  (Is this a Renderer which is erroneously Accepting or Listening?)\n";
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
    if (communicationSocket->_socketFD < 0){
      cerr << "arSocket error: accept() failed: ";
      perror("");
      return -1;
    }

    if (socketAddress.checkMask(_acceptMask))
      // Accept mask allowed a connection.
      break;

    cout << "arSocket remark: refused connection from "
	 << socketAddress.getRepresentation() << "\n";
    communicationSocket->ar_close();
    // Try again.
  }

  if (addr)
    *addr = socketAddress;
  return 0;
}

int arSocket::ar_read(char* theData, int howMuch){
#ifdef AR_USE_WIN_32
  return recv(_socketFD,theData,howMuch,0);
#else
  return read(_socketFD,theData,howMuch);
#endif
}

bool arSocket::readable(const ar_timeval& timeout){
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(_socketFD, &fds);
  struct timeval tv;
  tv.tv_sec = timeout.sec;
  tv.tv_usec = timeout.usec;
  return select(_socketFD+1, &fds, NULL, NULL, &tv) == 1;
}

bool arSocket::writable(const ar_timeval& timeout){
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(_socketFD, &fds);
  struct timeval tv;
  tv.tv_sec = timeout.sec;
  tv.tv_usec = timeout.usec;
  return select(_socketFD+1, NULL, &fds, NULL, &tv) == 1;
}

bool arSocket::readable() {
  return readable(ar_timeval(0,0));
}

bool arSocket::writable() {
  return writable(ar_timeval(0,0));
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

int arSocket::ar_write(const char* theData, int howMuch){
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

  return write(_socketFD,theData,howMuch);
#else
  // Win32
  return send(_socketFD,theData,howMuch,0);
#endif
}

int arSocket::ar_safeRead(char* theData, int numBytes){
  ar_mutex_lock(&_usageLock);
  _usageCount++;
  ar_mutex_unlock(&_usageLock);
  while (numBytes>0) {
    const int n = ar_read(theData, numBytes);
    if (n<=0) { 
      // Error reading from the socket (<0),
      // or the socket closed on us (==0) causing an incomplete ar_read(),
      // possibly because a remote client went away.
      ar_mutex_lock(&_usageLock);
        _usageCount--;
      ar_mutex_unlock(&_usageLock); 
      return false; 
      }
    numBytes -= n;
    theData += n;
  }
  ar_mutex_lock(&_usageLock);
  _usageCount--;
  ar_mutex_unlock(&_usageLock); 
  return true;
}

int arSocket::ar_safeWrite(const char* theData, int numBytes){
  ar_mutex_lock(&_usageLock);
  _usageCount++;
  ar_mutex_unlock(&_usageLock);
  while (numBytes>0) {
    const int n = ar_write(theData,numBytes);
    if (n<0) {
      // an error in writing to the socket
      ar_mutex_lock(&_usageLock);
      _usageCount--;
      ar_mutex_unlock(&_usageLock);
      return false;
    }
    numBytes -= n;
    theData += n;
  }
  ar_mutex_lock(&_usageLock);
  _usageCount--;
  ar_mutex_unlock(&_usageLock);
  return true;
}

int arSocket::getUsageCount(){
  ar_mutex_lock(&_usageLock);
  const int count = _usageCount;
  ar_mutex_unlock(&_usageLock);
  return count;
}

void arSocket::ar_close(){
#ifdef AR_USE_WIN_32
  if (_socketFD != INVALID_SOCKET){
    closesocket(_socketFD);
  }
#else
  if (_socketFD != -1){
    close(_socketFD);
  }
#endif
}
