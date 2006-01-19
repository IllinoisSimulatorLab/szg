//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSocketAddress.h"

arSocketAddress::arSocketAddress() :
  _addressLength(sizeof(_theAddress)){
}

bool arSocketAddress::setAddress(const char* IPaddress, int port){
  memset(&_theAddress,0,sizeof(_theAddress));
  _theAddress.sin_family = AF_INET;
  _theAddress.sin_port = htons(port);
#ifndef AR_USE_WIN_32
  // UNIX code
  if (IPaddress==NULL){
    _theAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else{
    int result = inet_pton(AF_INET,IPaddress,&(_theAddress.sin_addr)); 
    if (!result){
      // Must have been passed an invalid address.
      return false;
    }
  }
#else
  // Win32 code
  if (IPaddress == NULL){
    _theAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  }
  else{
    _theAddress.sin_addr.S_un.S_addr = inet_addr(IPaddress);
    if (_theAddress.sin_addr.S_un.S_addr == INADDR_NONE){
      // Must have been passed an invalid address.
      return false;
    }
  }
#endif
  // If we've got here, everything is OK.
  return true;
}

void arSocketAddress::setPort(int port){
  _theAddress.sin_port = htons(port);
}

sockaddr_in* arSocketAddress::getAddress(){
  return &_theAddress; // not const.  recvfrom() modifies it.
}

/// Sometimes we want the dotted quad string that results from
/// AND'ing the byte representation of the passed address with the
/// internally held address (for instance, what if we need to determine
/// if the internal address meets TCP-wrappers style conditions?)
/// Returns the AND'ed string on success and "NULL" otherwise.
string arSocketAddress::mask(const char* maskAddress){
  arSocketAddress masking;
  if (!masking.setAddress(maskAddress, 0)){
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

/// Given a netmask corresponding to our IP address, we return the
/// appropriate broadcast address.
string arSocketAddress::broadcastAddress(const char* maskAddress){
  arSocketAddress masking;
  if (!masking.setAddress(maskAddress, 0)){
    return string("NULL");
  }
  arSocketAddress opposite;
  // Initialize with something random just to be safe.
  // NOTE: do NOT use "255.255.255.255". I think this translates into the
  // Microsoft constant INADDR_NONE!
  if (!opposite.setAddress("192.168.0.1",0)){
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

string arSocketAddress::getRepresentation(){
#ifdef AR_USE_WIN_32
  // Win32 code
  char* sa = inet_ntoa(getAddress()->sin_addr);
  if (!sa){
    cerr << "arSocketAddress error: internal address format invalid.\n";
    return string("NULL");
  }
  return string(sa);
#else
  // Unix code
  char buffer[256];
  if (!inet_ntop(AF_INET, &(getAddress()->sin_addr), buffer, 256)){
    cerr << "arSocketAddress error: internal address format invalid.\n";
    return string("NULL");
  }
  return string(buffer);
#endif
}

#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
int arSocketAddress::getAddressLength(){
#else
unsigned int arSocketAddress::getAddressLength(){
#endif
#else
int arSocketAddress::getAddressLength(){
#endif
  return _addressLength;
}

#ifndef AR_USE_WIN_32
#ifdef AR_USE_SGI
int* arSocketAddress::getAddressLengthPtr(){
#else
unsigned int* arSocketAddress::getAddressLengthPtr(){
#endif
#else
int* arSocketAddress::getAddressLengthPtr(){
#endif
  return &_addressLength; // not const.  recvfrom() modifies it.
}

/// We want to be able to determine if this socket address meets certain
/// criteria, which are presented as a list of strings, each of the format:
///
///   XXX.XXX.XXX.XXX/YYY.YYY.YYY.YYY
/// 
/// or
/// 
///   XXX.XXX.XXX.XXX
///
/// In the first case, we go ahead and mask XXX.XXX.XXX.XXX by YYY.YYY.YYY.YYY
/// and also mask our address by YYY.YYY.YYY.YYY. If these are the same, then
/// success! In the second case, our internal address must match 
/// XXX.XXX.XXX.XXX to succeed. Only one criteria needs to succeed for the
/// call to succeed.
///
/// NOTE: 255.255.255.255 cannot be used as a mask! Use the single value
/// instead.
bool arSocketAddress::checkMask(list<string>& criteria){
  // If, in fact, the list of criteria is empty, then we pass the test!
  if (criteria.empty()){
    return true;
  }
  for (list<string>::iterator i = criteria.begin(); i != criteria.end();
       i++){
    // Is this an IP/mask pair or is this a single IP address?
    unsigned int position = (*i).find("/");
    if (position == string::npos){
      // Must be single IP address.
      if (getRepresentation() == *i){
	return true;
      }
    }
    else{
      // Check to make sure the / is not the last character.
      if (position == (*i).length() - 1){
	continue;
      }
      // Must be IP/mask.
      string IPaddr = (*i).substr(0, position);
      string maskAddr =(*i).substr(position + 1, (*i).length() - position - 1);
      arSocketAddress tmpAddress;
      if (!tmpAddress.setAddress(IPaddr.c_str(), 0)){
	cout << "arSocketAddress remark: received invalid address = "
	     << IPaddr << "\n";
	continue;
      }
      string maskedValue = tmpAddress.mask(maskAddr.c_str());
      if (maskedValue == "NULL"){
        cout << "arSocketAddress remark: received invalid mask = "
	     << maskAddr << "\n";
	continue;
      }
      if (maskedValue == mask(maskAddr.c_str())){
	return true;
      }
    }
  }
  // Didn't match one of the criteria in the list. Fail.
  return false;
}
