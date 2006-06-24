//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONNECTION_BROKER_H
#define AR_PHLEET_CONNECTION_BROKER_H

#include "arThread.h"
#include "arDataUtilities.h"
#include "arPhleetCalling.h"

#include <string>
#include <list>
#include <map>
#include <iostream>
using namespace std;

/// All the information required to notify a component that a specified
/// event has occured: the ID of the component along with the "match"
/// tag that the arSZGClient will use to determine which notifivation
/// requests this answers.
class SZG_CALL arPhleetNotification{
 public:
  arPhleetNotification(){}
  ~arPhleetNotification(){}

  int componentID; // This component requests the notification.
  int match;       // This is the tag of the original request.
};

/// A description of a service, including where it exists, what networks
/// it can communicate upon, the ports it uses, the components that must
/// be notified when it is deleted, etc.
class SZG_CALL arPhleetService{
 public:
  arPhleetService(){}
  ~arPhleetService(){}

  bool valid;       // is this valid or not... means we can use
                    // arPhleetService as a success flag
  string info;      // services can publish information here about their
                    //  characteristics. important for flexible brokering.
  string computer;  // the computer on which the service is running
  int componentID;  // the phleet ID of the component running this service
  arSlashString networks;  // networks on which the service is offered
  arSlashString addresses; // addresses on which it's offered, in same order
  int numberPorts;
  int portIDs[10];

  list<arPhleetNotification> notifications;
};

/// A specific way a client can connect to a service. The result of when
/// a client asks the broker about a service
class SZG_CALL arPhleetAddress{
 public:
  arPhleetAddress(){}
  ~arPhleetAddress(){}

  bool   valid;   // is this valid or not, means that we can use
                  // arPhleetAddress as a success flag
  string address; // an IP address
  int    numberPorts; // there can be multiple ports associated with the 
                      // address
  int    portIDs[10]; // the actual port IDs
};

/// A request for connection to a phleet service that has yet to be fulfilled
class SZG_CALL arPhleetServiceRequest{
 public:
  arPhleetServiceRequest(){}
  ~arPhleetServiceRequest(){}

  int    componentID;  // the phleet ID of the component making the service
                       // request
  string computer;     // the computer on which the requesting component 
                       // runs... it is helpful to be able to list the
                       // locations of pending service requests
  int    match;        // the "async rpc" message matcher
  string serviceName;  // the name of the requested service
  string networks;     // the networks on which the requesting component
                       // can communicate, slash-delimited list 
};

/// the connection broker must keep track of which ports have what status on
/// each computer being managed by the szgserver
class SZG_CALL arBrokerComputerData{
 public:
  arBrokerComputerData(){}
  ~arBrokerComputerData(){}

  string    networks;
  string    addresses;
  list<int> availablePorts;
  list<int> temporaryPorts;
  list<int> usedPorts;
  int       firstPort; // the first port in the current reserved block
  int       blockSize; // the size of the current reserved block
};

/// for each active component which has come to the attention of the connection
/// broker, we need to store some info. This is especially important when
/// a component exits somehow and we need to adjust the various global
/// lists.
class SZG_CALL arBrokerComponentData{
 public:
  arBrokerComponentData(){}
  ~arBrokerComponentData(){}

  string       computer;       // the computer on which this component runs
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

// without typedefs, the STL code can get very,very verbose
typedef map<string,arBrokerComputerData,less<string> > SZGComputerData;
typedef map<int,arBrokerComponentData,less<int> >      SZGComponentData;
typedef map<string,arPhleetService,less<string> >      SZGServiceData;
typedef list<arPhleetServiceRequest>                   SZGRequestList;

/// Manages connection brokering services for the szgserver. This code
/// could actually go in szgserver.cpp, but that file is getting too 
/// complicated. An important long term strategy would be to create more
/// managers like this one (for instance for messages, locks, and the
/// database)
class SZG_CALL arPhleetConnectionBroker{
 public:
  arPhleetConnectionBroker();
  ~arPhleetConnectionBroker() {}

  void setReleaseNotificationCallback(void(*callback)(int,int,const string&))
    { _releaseNotificationCallback = callback; }
  arPhleetService requestPorts(int componentID, const string& serviceName, 
                               const string& computer, const string& networks,
                               const string& addresses, int size, 
                               int firstPort, int blockSize);
  arPhleetService retryPorts(int componentID, const string& serviceName);
  bool confirmPorts(int componentID, const string& serviceName);
  string getServiceInfo(const string& serviceName);
  bool setServiceInfo(int componentID, 
                      const string& serviceName,
                      const string& info);
  bool checkService(const string& serviceName);
  arPhleetAddress requestService(int componentID, const string& computer, 
                                 int match,
                                 const string& serviceName, 
                                 const arSlashString& networks, bool async);
  SZGRequestList  getPendingRequests(const string& serviceName);
  SZGRequestList  getPendingRequests();
  string getServiceNames();
  arSlashString getServiceComputers();
  int    getServiceComponents(int*& IDs);
  int    getServiceComponentID(const string& serviceName);
  void   registerReleaseNotification(int componentID, 
                                     int match,
                                     const string& computer,
				     const string& serviceName);
  void _removeService(const string& serviceName);
  void removeComponent(int componentID);
  void print();
  
 private:
  arMutex               _brokerLock;
  SZGComputerData       _computerData;
  SZGComponentData      _componentData;
  SZGServiceData        _temporaryServices;
  SZGServiceData        _usedServices;
  SZGRequestList        _requestedServices;
  void (*_releaseNotificationCallback)(int,int,const string&);

  void _resizeComputerPorts(arBrokerComputerData& computer,
                            int first, int size);
  bool _portValid(int port, arBrokerComputerData& computer);
};

#endif
