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
  int ar_read(char* theData, const int numBytes) const;
  int ar_write(const char* theData, int numBytes) const;

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

