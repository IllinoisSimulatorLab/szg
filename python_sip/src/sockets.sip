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
//  bool setAddress(const char* IPaddress, int port);
  bool setAddress( const string* IPaddress, int port );
%MethodCode
    if (*a0 == "") {
      sipRes = sipCpp->setAddress( NULL, a1 );
    } else {
      sipRes = sipCpp->setAddress( a0->c_str(), a1 );
    }
%End

  void setPort(int port);
//  sockaddr_in* getAddress();
  string mask(const char* maskAddress) /TransferBack/;
  string broadcastAddress(const char* maskAddress) /TransferBack/;
  string getRepresentation() /TransferBack/;
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
//  int ar_accept(arSocket*, arSocketAddress* addr = NULL);
  int ar_accept(arSocket*);
  bool readable(const ar_timeval& timeout) const;
  bool writable(const ar_timeval& timeout) const;
  bool readable() const; // poll with no timeout
  bool writable() const;
//  int ar_read(char* theData, const int numBytes) const;
  SIP_PYOBJECT ar_read( const int maxLen ) /TransferBack/;
%MethodCode
    char* tmp = new char[a0+1];
    for (int i=0; i<(a0+1); ++i) tmp[i] = '\0';
    if (!tmp) {
      PyErr_SetString( PyExc_RuntimeError, "arSocket::ar_read() failed to allocate buffer" );
      sipIsErr = 1;
    } else {
      int numReceived = sipCpp->ar_read( tmp, a0 );
      sipRes = PyString_FromStringAndSize( tmp, numReceived );
      delete[] tmp;
      if (sipRes == NULL) {
        PyErr_SetString( PyExc_RuntimeError, "arSocket::ar_read() failed to convert buffer" );
        sipIsErr = 1;
      }
    }
%End
//  int ar_write(const char* theData, int numBytes) const;
  int ar_write( SIP_PYOBJECT theData /TransferBack/ );
%MethodCode
    if (!PyString_Check( a0 )) {
      PyErr_SetString( PyExc_RuntimeError, "arSocket::ar_write() argument must be a string" );
      sipIsErr = 1;
    } else {
      // may be unnecessary.
      int len = PyString_Size( a0 );
      char* bufPtr;
      if (PyString_AsStringAndSize( a0, &bufPtr, &len )==-1) {
        sipIsErr = 1;
      } else {
        sipRes = sipCpp->ar_write( bufPtr, len );
      }
    }
%End

  // "Safe" versions keep usage counts, and are guaranteed to return
  // the number of bytes requested, or an error.
//  int ar_safeRead(char* theData, int howMuch, const double usecTimeout = 0.);
//  int ar_safeWrite(const char* theData, int howMuch, const double usecTimeout = 0.);

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
  SIP_PYTUPLE ar_read( int maxLen ) /TransferBack/;
%MethodCode
    cerr << "beginning ar_read().\n";
    char* tmp = new char[a0+1];
    for (int i=0; i<(a0+1); ++i) tmp[i] = '\0';
    if (!tmp) {
      PyErr_SetString( PyExc_RuntimeError, "arUDPSocket::ar_read() failed to allocate buffer" );
      sipIsErr = 1;
    } else {
      arSocketAddress* addr = new arSocketAddress();
      cerr << "C++ before ar_read().\n" << a0 << endl;
      int numReceived = sipCpp->ar_read( tmp, a0, addr );
      cerr << "C++ after ar_read().\n";
      PyObject* bufString = PyString_FromStringAndSize( tmp, numReceived );
      if (bufString == NULL) {
        cerr << "arUDPSocket::ar_read() failed to convert buffer.\n";
        PyErr_SetString( PyExc_RuntimeError, "arUDPSocket::ar_read() failed to convert buffer" );
        sipIsErr = 1;
        delete[] tmp;
      } else {
        delete[] tmp;
        sipRes = PyTuple_New(2);
        if (sipRes == NULL) {
          cerr << "arUDPSocket::ar_read() failed to allocate tuple.\n";
          PyErr_SetString( PyExc_RuntimeError, "arUDPSocket::ar_read() failed to allocate tuple" );
          Py_DECREF( bufString );
          sipIsErr = 1;
        } else {
          PyObject* o = sipConvertFromInstance( addr, sipClass_arSocketAddress, Py_None );
          if (o == NULL) {
            cerr << "arUDPSocket::ar_read() failed to convert address.\n";
            PyErr_SetString( PyExc_RuntimeError, "arUDPSocket::ar_read() failed to convert address" );
            Py_DECREF( bufString );
            Py_DECREF( sipRes );
            sipIsErr = 1;
          } else {
            PyTuple_SET_ITEM( sipRes, 0, bufString );
            PyTuple_SET_ITEM( sipRes, 1, o );
          }
        }
      }
    }
%End

//  int ar_write(const char* theData, int howMuch, arSocketAddress*) const;
  int ar_write( arSocketAddress* sock /TransferBack/, SIP_PYOBJECT theData /TransferBack/ );
%MethodCode
    if (!PyString_Check( a1 )) {
      PyErr_SetString( PyExc_RuntimeError, "Usage: arUDPSocket.ar_write( arSocketAddress, string )" );
      sipIsErr = 1;
    } else {
      // may be unnecessary.
      int len = PyString_Size( a1 );
      char* bufPtr;
      if (PyString_AsStringAndSize( a1, &bufPtr, &len )==-1) {
        sipIsErr = 1;
      } else {
        char* buffer = new char[len+1];
        memcpy( buffer, bufPtr, len );
        sipRes = sipCpp->ar_write( buffer, len, a0 );
        delete[] buffer;
      }
    }
%End

  void ar_close() const;
};

