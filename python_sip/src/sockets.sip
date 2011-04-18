// Defines mapping from python to C++ strings.
%Include typemaps.sip

// math-global extra code
%ModuleHeaderCode
#include <string>
using namespace std;
%End

class arSocketAddress {
%TypeHeaderCode
#include "arSocketAddress.h"
%End
 public:
  arSocketAddress();
  ~arSocketAddress();
  bool setAddress(const char* IPaddress, int port);
//  bool setAddress( const string& IPaddress, int port );
//%MethodCode
//    return setAddress( IPaddress.c_str(), port );
//%End

  void setPort(int port);
//  sockaddr_in* getAddress();
  string mask(const char* maskAddress);
  string broadcastAddress(const char* maskAddress);
  string getRepresentation();
  bool checkMask(list<string>& criteria);
 private:
  arSocketAddress( const arSocketAddress& );
};


enum arSocketType {
  AR_LISTENING_SOCKET=1,
  AR_STANDARD_SOCKET=2
};

// TCP socket.

class arSocket {
%TypeHeaderCode
#include "arSocket.h"
%End
private:
  arSocket();
public:
  arSocket(int type);
  ~arSocket();

  void setID(int theID);
  int getID() const;

  int ar_create();
  int ar_connect(const char* IPaddress, int port);
  bool setReceiveBufferSize(int size);
  bool setSendBufferSize(int size);
  bool smallPacketOptimize(bool flag);
  bool reuseAddress(bool flag);
  int ar_bind(const char* IPaddress, int port);
  int ar_listen(int queueSize);
  int ar_accept(arSocket*, arSocketAddress* addr = NULL);
  bool readable(const ar_timeval& timeout) const;
  bool writable(const ar_timeval& timeout) const;
  bool readable() const; // poll with no timeout
  bool writable() const;
//  int ar_read(char* theData, const int numBytes) const;
  string* ar_read( const int maxLen );
%MethodCode
    char* tmp = new char[a0+1];
    if (!tmp) {
      PyErr_SetString( PyExc_RuntimeError, "arSocket::ar_read() failed to allocate buffer" );
      sipIsErr = 1;
    } else {
      int numReceived = sipCpp->ar_read( tmp, a0 );
      tmp[numReceived] = '\0';
      sipRes = new string( tmp );
      delete[] tmp;
    }
%End
//  int ar_write(const char* theData, int numBytes) const;
  int ar_write( string* theData );
%MethodCode
    sipRes = sipCpp->ar_write( a0->c_str(), a0->size() );
%End

  // "Safe" versions keep usage counts, and are guaranteed to return
  // the number of bytes requested, or an error.
  int ar_safeRead(char* theData, int howMuch, const double usecTimeout = 0.);
  int ar_safeWrite(const char* theData, int howMuch, const double usecTimeout = 0.);

  int getUsageCount();
  void ar_close();
  // This is a tcp-wrappers-esque feature. As explained below, the mask
  // allows ar_accept to automatically drop attempted connections,
  // based on IP address.
  void setAcceptMask(list<string>& mask);
};


// UDP socket (as opposed to TCP socket, arSocket).
// Like arSocket, derive from arCommunicator
// to get arCommunicator's implicit WinSock init.

class arUDPSocket {
%TypeHeaderCode
#include "arUDPSocket.h"
%End
public:
  arUDPSocket();
  ~arUDPSocket();

  void setID(int theID);
  int getID() const;

  void setBroadcast(bool) const;

  int ar_create();
  void setReceiveBufferSize(int size) const;
  void setSendBufferSize(int size) const;
  void reuseAddress(bool flag) const;
  int ar_bind(arSocketAddress*) const;
//  int ar_read(char* theData, int howMuch, arSocketAddress*) const;
  string* ar_read( arSocketAddress* sock, int maxLen );
%MethodCode
    char* tmp = new char[a1+1];
    if (!tmp) {
      PyErr_SetString( PyExc_RuntimeError, "arUDPSocket::ar_read() failed to allocate buffer" );
      sipIsErr = 1;
    } else {
      int numReceived = sipCpp->ar_read( tmp, a1, a0 );
      tmp[numReceived] = '\0';
      sipRes = new string( tmp );
      delete[] tmp;
    }
%End

//  int ar_write(const char* theData, int howMuch, arSocketAddress*) const;
  int ar_write( arSocketAddress* sock, string* theData );
%MethodCode
    sipRes = sipCpp->ar_write( a1->c_str(), a1->size(), a0 );
%End
  void ar_close() const;
};

