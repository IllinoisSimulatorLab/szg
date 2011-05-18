//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONNECTION_BROKER_H
#define AR_PHLEET_CONNECTION_BROKER_H

#include "arDataUtilities.h"
#include "arPhleetCalling.h"

#include <string>
#include <list>
#include <map>
#include <iostream>
using namespace std;

// All the information required to notify a component that a specified
// event has occured: the ID of the component along with the "match"
// tag that the arSZGClient will use to determine which notifivation
// requests this answers.
class SZG_CALL arPhleetNotification {
 public:
  arPhleetNotification() : componentID(-1), match(-1) {}
  arPhleetNotification(int a, int b) : componentID(a), match(b) {}

  int componentID; // Component requesting the notification.
  int match;       // Tag of the original request.
};

// A description of a service, including where it exists, what networks
// it can communicate upon, the ports it uses, the components that must
// be notified when it is deleted, etc.
class SZG_CALL arPhleetService {
 public:
  arPhleetService() : valid(false), componentID(-1), numberPorts(0) {}

  bool valid;
  string info;      // Services can publish information here about their
                    //  characteristics. important for flexible brokering.
  string computer;  // host running the service
  int componentID;  // phleet ID of component running the service
  arSlashString networks;  // networks offering the service
  arSlashString addresses; // addresses offering the servic3, in same order
  int numberPorts;
  int portIDs[10];

  list<arPhleetNotification> notifications;
};

// A specific way a client can connect to a service. The result of when
// a client asks the broker about a service
class SZG_CALL arPhleetAddress {
 public:
  arPhleetAddress() : valid(false), address("NULL"), numberPorts(0) {}

  bool valid;
  string address; // IP address
  int numberPorts; // how many ports
  int portIDs[10]; // the actual port IDs
};

// A request for connection to a phleet service that has yet to be fulfilled
class SZG_CALL arPhleetServiceRequest {
 public:
  arPhleetServiceRequest() : componentID(-1), match(-1) {}

  int    componentID;  // phleet ID of component requesting the service
  string computer;     // host running requested component
                       // (nice to list locations of pending service requests)
  int    match;        // "async rpc" message matcher
  string serviceName;  // requested service's name
  string networks;     // networks on which the requester
                       // can communicate, slash-delimited list
};

// the connection broker must keep track of which ports have what status on
// each computer being managed by the szgserver
class SZG_CALL arBrokerComputerData {
 public:
  arBrokerComputerData() : firstPort(-1), blockSize(-1) {}

  string    networks;
  string    addresses;
  list<int> availablePorts;
  list<int> temporaryPorts;
  list<int> usedPorts;
  int       firstPort; // the first port in the current reserved block
  int       blockSize; // the size of the current reserved block
};

// Store info for each active component which has come to the attention of the connection
// broker.  Especially when a component exits and global lists need to be adjusted.
class SZG_CALL arBrokerComponentData {
 public:
  arBrokerComponentData() : computer("NULL") {}

  string       computer;       // host running this component
  list<string> temporaryTags;  // names of services assigned to this component
                               // but not yet started
  list<string> usedTags;       // names of services assigned to this component
                               // and started
  list<string> releaseTags;    // when these services are released, this
                               // component should be notified.
  list<int>    temporaryPorts; // ports this component has been assigned but
                               // has yet to bind to
  list<int>    usedPorts;      // ports to which this component has bound
};

// Abbreviations.
typedef map<string, arBrokerComputerData, less<string> > SZGComputerData;
typedef map<int, arBrokerComponentData, less<int> >      SZGComponentData;
typedef map<string, arPhleetService, less<string> >      SZGServiceData;
typedef list<arPhleetServiceRequest>                   SZGRequestList;

// Manages connection brokering services for the szgserver. This code
// could actually go in szgserver.cpp, but that file is getting too
// complicated. An important long term strategy would be to create more
// managers like this one (for instance for messages, locks, and the
// database)
class SZG_CALL arPhleetConnectionBroker {
 public:
  arPhleetConnectionBroker();
  virtual ~arPhleetConnectionBroker() {}

  virtual void onServiceRelease( int componentID, int match, const string& serviceName );

  void setReleaseNotificationCallback(void(*callback)(int, int, const string&))
    { _releaseNotificationCallback = callback; }
  arPhleetService requestPorts(int componentID, const string& serviceName,
                               const string& computer, const string& networks,
                               const string& addresses, int size,
                               int firstPort, int blockSize);
  arPhleetService retryPorts(int componentID, const string& serviceName);
  bool confirmPorts(int componentID, const string& serviceName);
  string getServiceInfo(const string& serviceName) const;
  bool setServiceInfo(const int componentID,
                      const string& serviceName,
                      const string& info);
  bool checkService(const string& serviceName) const;
  arPhleetAddress requestService(int componentID, const string& computer,
                                 int match,
                                 const string& serviceName,
                                 const arSlashString& networks, bool async);
  SZGRequestList  getPendingRequests( const string& serviceName="" );
  string getServiceNames() const;
  arSlashString getServiceComputers() const;
  int    getServiceComponents(int*& IDs) const;
  int    getServiceComponentID(const string& serviceName) const;
  void   registerReleaseNotification(int componentID,
                                     int match,
                                     const string& computer,
                                     const string& serviceName);
  void _removeService(const string& serviceName);
  void removeComponent(int componentID);
  void print() const;

 private:
  SZGComputerData  _computerData;
  SZGComponentData _componentData;
  SZGServiceData   _temporaryServices;
  SZGServiceData   _usedServices;
  SZGRequestList   _requestedServices;
  mutable arLock _l;
  void (*_releaseNotificationCallback)(int, int, const string&);

  void _resizeComputerPorts(arBrokerComputerData& computer,
                            const int first, const int size);
  bool _portValid(const int port, const arBrokerComputerData&) const;
};

#endif
