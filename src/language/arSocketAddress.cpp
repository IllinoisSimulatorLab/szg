//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSocketAddress.h"
#include "arLogStream.h"

arSocketAddress::arSocketAddress() :
  _addressLength(sizeof(_theAddress)) {
}

bool arSocketAddress::setAddress(const char* IPaddress, int port) {
  memset(&_theAddress, 0, sizeof(_theAddress));
  _theAddress.sin_family = AF_INET;
  _theAddress.sin_port = htons(port);
#ifndef AR_USE_WIN_32
  // UNIX code
  if (IPaddress==NULL) {
    _theAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else{
    int result = inet_pton(AF_INET, IPaddress, &(_theAddress.sin_addr));
    if (!result) {
      // Must have been passed an invalid address.
      return false;
    }
  }
#else
  // Win32 code
  if (IPaddress == NULL) {
    _theAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  }
  else{
    _theAddress.sin_addr.S_un.S_addr = inet_addr(IPaddress);
    if (_theAddress.sin_addr.S_un.S_addr == INADDR_NONE) {
      // Must have been passed an invalid address.
      return false;
    }
  }
#endif
  // If we've got here, everything is OK.
  return true;
}

void arSocketAddress::setPort(int port) {
  _theAddress.sin_port = htons(port);
}

sockaddr_in* arSocketAddress::getAddress() {
  return &_theAddress; // not const.  recvfrom() modifies it.
}

// Sometimes we want the dotted quad string that results from
// AND'ing the byte representation of the passed address with the
// internally held address (for instance, what if we need to determine
// if the internal address meets TCP-wrappers style conditions?)
// Returns the AND'ed string on success and "NULL" otherwise.
string arSocketAddress::mask(const char* maskAddress) {
  arSocketAddress masking;
  if (!masking.setAddress(maskAddress, 0)) {
    return string("NULL");
  }
#ifndef AR_USE_WIN_32
  // Unix code
  masking._theAddress.sin_addr.s_addr
    = masking._theAddress.sin_addr.s_addr
      & _theAddress.sin_addr.s_addr;
#else
  // Win32 code
  masking._theAddress.sin_addr.S_un.S_addr
    = masking._theAddress.sin_addr.S_un.S_addr
      & _theAddress.sin_addr.S_un.S_addr;
#endif
  return masking.getRepresentation();
}

// Given a netmask corresponding to our IP address, we return the
// appropriate broadcast address.
string arSocketAddress::broadcastAddress(const char* maskAddress) {
  arSocketAddress masking;
  if (!masking.setAddress(maskAddress, 0)) {
    return string("NULL");
  }
  arSocketAddress opposite;
  // Initialize with something random just to be safe.
  // NOTE: do NOT use "255.255.255.255". I think this translates into the
  // Microsoft constant INADDR_NONE!
  if (!opposite.setAddress("192.168.0.1", 0)) {
    return string("NULL");
  }
#ifndef AR_USE_WIN_32
  // Unix code
  opposite._theAddress.sin_addr.s_addr = ~masking._theAddress.sin_addr.s_addr;
  masking._theAddress.sin_addr.s_addr
    = (masking._theAddress.sin_addr.s_addr
      & _theAddress.sin_addr.s_addr) | opposite._theAddress.sin_addr.s_addr;
#else
  // Win32 code
  opposite._theAddress.sin_addr.S_un.S_addr
    = ~masking._theAddress.sin_addr.S_un.S_addr;
  masking._theAddress.sin_addr.S_un.S_addr
    = (masking._theAddress.sin_addr.S_un.S_addr
      & _theAddress.sin_addr.S_un.S_addr)
      | opposite._theAddress.sin_addr.S_un.S_addr  ;
#endif
  return masking.getRepresentation();
}

string arSocketAddress::getRepresentation() {
#ifdef AR_USE_WIN_32
  char* sa = inet_ntoa(getAddress()->sin_addr);
  if (!sa) {
    cerr << "arSocketAddress error: internal address format invalid.\n";
    return string("NULL");
  }
  return string(sa);
#else
  char buffer[256];
  if (!inet_ntop(AF_INET, &(getAddress()->sin_addr), buffer, 256)) {
    cerr << "arSocketAddress error: internal address format invalid.\n";
    return string("NULL");
  }
  return string(buffer);
#endif
}

#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
int arSocketAddress::getAddressLength() {
#else
unsigned int arSocketAddress::getAddressLength() {
#endif
#else
int arSocketAddress::getAddressLength() {
#endif
  return _addressLength;
}

#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
int* arSocketAddress::getAddressLengthPtr() {
#else
unsigned int* arSocketAddress::getAddressLengthPtr() {
#endif
#else
int* arSocketAddress::getAddressLengthPtr() {
#endif
  return &_addressLength; // not const.  recvfrom() modifies it.
}

// Determine if this socket address meets certain criteria,
// presented as a list of strings of the form:
//     XXX.XXX.XXX.XXX/YYY.YYY.YYY.YYY
// or
//     XXX.XXX.XXX.XXX
// In the first case, mask XXX.XXX.XXX.XXX by YYY.YYY.YYY.YYY
// and also mask our address by YYY.YYY.YYY.YYY. If these match, success.
// In the second case, our internal address must match
// XXX.XXX.XXX.XXX. Only one criterion needs to succeed for the call to succeed.
//
// Don't use 255.255.255.255 as a mask! Use the single value instead.
bool arSocketAddress::checkMask(list<string>& criteria) {
  if (criteria.empty()) {
    return true;
  }

  for (list<string>::iterator i = criteria.begin(); i != criteria.end(); ++i) {
    string::size_type position = i->find("/");
    if (position == string::npos) {
      // Single IP address.
      if (getRepresentation() == *i) {
        return true;
      }
      continue;
    }

    if (position == i->length() - 1) {
      // IP/ without a following mask.  Weird.
      ar_log_error() << "checkMask() skipping weird address/netmask string '" << *i << ";.\n";
      continue;
    }

    // IP/mask.
    const string IPaddr(i->substr(0, position++));
    const string maskAddr(i->substr(position, i->length() - position));
    arSocketAddress tmp;
    if (!tmp.setAddress(IPaddr.c_str(), 0)) {
      ar_log_error() << "arSocketAddress: invalid address '" << IPaddr << "'.\n";
      continue;
    }

    const string maskedValue(tmp.mask(maskAddr.c_str()));
    if (maskedValue == "NULL") {
      ar_log_error() << "arSocketAddress: invalid mask '" << maskAddr << "'.\n";
      continue;
    }

    if (maskedValue == mask(maskAddr.c_str())) {
      return true;
    }
  }

  // Matched no criteria in the list.
  return false;
}
