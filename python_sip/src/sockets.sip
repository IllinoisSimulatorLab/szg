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

