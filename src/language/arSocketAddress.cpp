//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSocketAddress.h"

arSocketAddress::arSocketAddress() :
  _addressLength(sizeof(_theAddress)){
}

void arSocketAddress::setAddress(const char* IPaddress, int port){
  memset(&_theAddress,0,sizeof(_theAddress));
  _theAddress.sin_family = AF_INET;
  _theAddress.sin_port = htons(port);
#ifndef AR_USE_WIN_32
  // UNIX code
  if (IPaddress==NULL){
    _theAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else{
    inet_pton(AF_INET,IPaddress,&(_theAddress.sin_addr)); 
  }
#else
  // Win32 code
  if (IPaddress == NULL){
    _theAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  }
  else{
    _theAddress.sin_addr.S_un.S_addr = inet_addr(IPaddress);
  }
#endif
}

void arSocketAddress::setPort(int port){
  _theAddress.sin_port = htons(port);
}

sockaddr_in* arSocketAddress::getAddress(){
  return &_theAddress; // not const.  recvfrom() modifies it.
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
/// criteria. This can be use to filter incoming connections based on IP
/// address, for instance. This is a pretty lame hack so far, but...
/// If the mask has the format XXX.XXX.XXX, then we just make sure the first
/// 3 numbers in the dotted quad match. If the mask has the format
/// XXX.XXX.XXX.XXX, then we make sure they all match.
bool arSocketAddress::checkMask(const string& mask){

  // mask == "NULL" is a default saying "do no masking"
  if (mask == "NULL"){
    return true;
  }

  // Count the number of '.' in the string. 
  int count = 0;
  for (unsigned int i=0; i<mask.length(); i++){
    if (mask[i] == '.'){
      count++;
    }
  }
  if (count != 2 && count != 3){
    cerr << "arSocketAddress error: incorrectly formed mask.\n";
    return false;
  }

  // Want to get the socket address is dotted quad format
  string socketAddress;
#ifdef AR_USE_WIN_32
  // Win32 code
  char* sa = inet_ntoa(getAddress()->sin_addr);
  if (!sa){
    cerr << "arSocketAddress error: internal address format invalid.\n";
    return false;
  }
  socketAddress = string(sa);
#else
  // Unix code
  char buffer[256];
  // NO ERROR CHECKING... THIS IS VERY BAD!
  inet_ntop(AF_INET, &(getAddress()->sin_addr), buffer, 256);
  socketAddress = string(buffer);
#endif

  if (count == 3){
    // must match exactly
    if (socketAddress != mask){
      cout << "arSocketAddress remark: failed to match \n  mask = (" 
           << mask << ") " << "and IP = (" << socketAddress << ").\n";
      return false;
    }
    return true;
  }  
  else{
    // just match the first 2
    unsigned int position = socketAddress.find_last_of(".");
    if (position == string::npos){
      // this should never happen
      cerr << "arSocketAddress error: invalid dotted quad.\n";
      return false;
    }
    string match = socketAddress.substr(0,position);
    if (match != mask){
      cout << "arSocketAddress remark: failed to match \n  mask = (" 
           << mask << ") " << "and IP = (" << socketAddress << ").\n";
      return false;
    }
    return true;
  }
}
