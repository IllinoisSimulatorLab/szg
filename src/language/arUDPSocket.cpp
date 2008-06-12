//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arUDPSocket.h"

#include <stdio.h>
#include <errno.h>

void arUDPSocket::setBroadcast(bool fOn) const {
  const int on = fOn ? 1 : 0;
  setsockopt(_socketFD, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(int));
}

int arUDPSocket::ar_create() {
  _socketFD = socket(AF_INET, SOCK_DGRAM, 0);
  if (_socketFD < 0) {
    perror("arUDPSocket socket() failed");
    return -1;
  }
  return 0;
}

void arUDPSocket::setReceiveBufferSize(int size) const {
  setsockopt(_socketFD, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(int));
}

void arUDPSocket::setSendBufferSize(int size) const {
  setsockopt(_socketFD, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int));
}

void arUDPSocket::reuseAddress(bool fReuse) const {
  const int parameter = fReuse ? 1 : 0;
  setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, (const char*)&parameter, sizeof(int));
#if defined(AR_USE_SGI) || defined(AR_USE_DARWIN)
  setsockopt(_socketFD, SOL_SOCKET, SO_REUSEPORT, (const char*)&parameter, sizeof(int));
#endif
}

int arUDPSocket::ar_bind(arSocketAddress* addr) const {
  const int err = bind(_socketFD, (sockaddr*)addr->getAddress(),
                 addr->getAddressLength());
  if (err < 0)
    perror("arUDPSocket bind failed");
  return err;
}

int arUDPSocket::ar_read(char* theData, int howMuch, arSocketAddress* addr) const {
  if (!addr)
    return recvfrom(_socketFD, theData, howMuch, 0, NULL, NULL);

  return recvfrom(_socketFD, theData, howMuch, 0,
		  (sockaddr*)addr->getAddress(),
#if !defined(AR_USE_SGI) && !defined(AR_USE_WIN_32)
		  (socklen_t*)
#endif
		              addr->getAddressLengthPtr());
}

int arUDPSocket::ar_write(const char* theData, int howMuch, arSocketAddress* addr) const {
  return sendto(_socketFD, theData, howMuch, 0,
                (sockaddr*)addr->getAddress(),
                addr->getAddressLength());
}

void arUDPSocket::ar_close() const {
#ifdef AR_USE_WIN_32
   closesocket(_socketFD);
#else
   close(_socketFD);
#endif
}
