//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOCKET_ADDRESS_H
#define AR_SOCKET_ADDRESS_H

#ifdef AR_USE_WIN_32
#include <windows.h>
#include <iostream>
#include <string>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#endif
using namespace std;

/// Abstracts the concept of a socket address.

class arSocketAddress{
 public:
  arSocketAddress();
  ~arSocketAddress() {}
  void setAddress(const char* IPaddress, int port);
  void setPort(int port);
  sockaddr_in* getAddress();
#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
  int* getAddressLengthPtr();
  int getAddressLength();
#else
  unsigned int* getAddressLengthPtr();
  unsigned int getAddressLength();
#endif
#else
  int* getAddressLengthPtr();
  int getAddressLength();
#endif
  bool checkMask(const string& mask);
 private:
  sockaddr_in _theAddress;
#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
  int _addressLength;
#else
  unsigned int _addressLength;
#endif
#else
  int _addressLength;
#endif
};

#endif
