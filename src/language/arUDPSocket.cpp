//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arUDPSocket.h"
#include <stdio.h>
#include <errno.h>

void arUDPSocket::setID(int theID){
  _ID = theID;
}

int arUDPSocket::getID(){
  return _ID;
}

void arUDPSocket::setBroadcast(bool state){
  const int on = state ? 1 : 0;
  setsockopt(_socketFD, SOL_SOCKET, SO_BROADCAST,
    (const char*)&on, sizeof(int));
}

int arUDPSocket::ar_create(){
  _socketFD = socket(AF_INET, SOCK_DGRAM, 0);
  if (_socketFD < 0){
    perror("arUDPSocket socket() failed");
    return -1;
  }
  return 0;
}

void arUDPSocket::setReceiveBufferSize(int size){
  // turns out that the Win32 and UNIX code is the same here
  setsockopt(_socketFD, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(int));
}

void arUDPSocket::setSendBufferSize(int size){
  // turns out that Win32 and UNIX code is the same
  setsockopt(_socketFD,SOL_SOCKET,SO_SNDBUF,(char*)&size,sizeof(int));
}

void arUDPSocket::reuseAddress(bool flag){
  // turns out Win32 and UNIX code is the same
  const int parameter = flag ? 1 : 0;
  setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, 
             (const char*)&parameter, sizeof(int));
  // The SO_REUSEPORT socket option seems to exist and be necessary for
  // OS X and SGI only (they have more BSD heritage than linux).
#ifdef AR_USE_SGI
  setsockopt(_socketFD, SOL_SOCKET, SO_REUSEPORT,
             (const char*)&parameter, sizeof(int));
#endif
#ifdef AR_USE_DARWIN
  setsockopt(_socketFD, SOL_SOCKET, SO_REUSEPORT,
             (const char*)&parameter, sizeof(int));
#endif
}

int arUDPSocket::ar_bind(arSocketAddress* theAddress){
  int err = bind(_socketFD, (sockaddr*)theAddress->getAddress(), 
                 theAddress->getAddressLength());
  if (err < 0)
    perror("arUDPSocket bind failed");
  return err;
}

int arUDPSocket::ar_read(char* theData, int howMuch, 
                         arSocketAddress* theAddress){
  if (theAddress == NULL){
    return recvfrom(_socketFD,theData,howMuch,0,NULL,NULL);
  }
  else{
    return recvfrom(_socketFD,theData,howMuch,0,
                    (sockaddr*)theAddress->getAddress(),
                    theAddress->getAddressLengthPtr());
  }
}

int arUDPSocket::ar_write(const char* theData, int howMuch,
                          arSocketAddress* theAddress){
  return sendto(_socketFD,theData,howMuch,0,
                (sockaddr*)theAddress->getAddress(),
                theAddress->getAddressLength());
}

void arUDPSocket::ar_close(){
#ifdef AR_USE_WIN_32
   closesocket(_socketFD);
#else
   close(_socketFD);
#endif
}



