//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOCKET_ADDRESS_H
#define AR_SOCKET_ADDRESS_H

#ifdef AR_USE_WIN_32
// DO NOT INCLUDE windows.h here. Instead, do as below.
#include "arPrecompiled.h"
#include <iostream>
#include <string>
#include <list>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <list>
#include <iostream>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#endif
#include "arLanguageCalling.h"
using namespace std;

// Abstract socket address.

class SZG_CALL arSocketAddress{
 public:
  arSocketAddress();
  ~arSocketAddress() {}
  bool setAddress(const char* IPaddress, int port);
  void setPort(int port);
  sockaddr_in* getAddress();
  string mask(const char* maskAddress);
  string broadcastAddress(const char* maskAddress);
  string getRepresentation();
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
  bool checkMask(list<string>& criteria);
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
