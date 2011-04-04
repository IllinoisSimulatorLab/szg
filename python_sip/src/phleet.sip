//
// Important note: argument labels in methods with %MethodCode blocks, e.g. in:
//
//   void fromSequence( SIP_PYOBJECT a0 );
//
// The 'a0' is there for clarity only and is ignored by the SIP parser.
// In particular, you can't change these labels because SIP automatically maps the first
// argument to 'a0', second to 'a1', and so on.
//
// The 'this' pointer is mapped by SIP onto the variable 'sipCpp', but there's a gotcha:
// Confusingly, if what is implemented as an object method is implemented in a
// global operator C++, as in the following case:
//
//    arVector3 __add__( arVector3 a1 );
//
// Which is implemented in C++ as:
//
//    arVector3 operator+( const arVector3&, const arVector3& );
//
// ...then the 'self-as-C++-object' pointer ends up in a0.

//%Module szgmath 0

// Defines mappings from python to C++ types.
%Include typemaps.sip


class arPhleetOSLanguage : arLanguage {
%TypeHeaderCode
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arLanguage.h"
#include "arPhleetTemplate.h"
#include "arPhleetCalling.h"
%End
 public:
  arPhleetOSLanguage();
  ~arPhleetOSLanguage();

  // The following field and record IDs are public in order to allow
  // programmers to use

  // Constants corresponding to the common fields for each Syzygy record.
  // The Syzygy user name.
  int AR_PHLEET_USER;
  // The context in which the communication has been issued. In general,
  // this field gives a list of key/value pairs.
  int AR_PHLEET_CONTEXT;
  // Included to provide password or other authentication material.
  // Information in the context field will determine how this is utilized,
  // if at all.
  int AR_PHLEET_AUTH;
  // Responses are routed to their appropriate destinations on the client side
  // via this field. This is necessary in order to allow multiple threads on
  // the client to communicate with the szgserver simultaneously.
  int AR_PHLEET_MATCH;

  // A description of the various records in the Syzygy protocol follows
  // CONNECTION_ACK: After an arSZGClient connects to the szgserver,
  // it uses this record to communicate it's component's name. Many
  // management operations (like dkill) depend on component name.
  // Also, a set attribute request receives a connection ack response,
  // which ensures that the szgserver has processed a request before the
  // dset (for instance) has finished executing.
  int AR_CONNECTION_ACK;
  int AR_CONNECTION_ACK_LABEL;

  // KILL: Ideally, when components expire, their entries will be purged from
  // the database of Syzygy components. This currently relies on the szgserver
  // realizing, via the TCP connection, that the component has exited.
  // Unfortunately, sometimes this doesn't work (if the component's host
  // crashes or if they are communicating over a wireless link). Consequently,
  // a method to manually remove components from the database is needed.
  // This method is "dkill -9" and uses the KILL record, while "dkill" uses
  // the MESSAGE record. NOTE: THIS MESSAGE DOES NOT SEEM TO AUTOMATICALLY
  // GET A RESPONSE. IS THAT REALLY SAFE?

  // NOTE: This message is ALSO used to send to the component in question,
  // indicating that it is to be externally shut down.
  int AR_KILL;
  int AR_KILL_ID;

  // PROCESS_INFO: This one is used to get ID and/or label information from
  // one of the Syzygy components. By varying the "type" field, we can
  // get the label of a process with a given ID, the ID of a process with a
  // given label, or the ID of ourselves.
  int AR_PROCESS_INFO;
  int AR_PROCESS_INFO_TYPE;
  int AR_PROCESS_INFO_ID;
  int AR_PROCESS_INFO_LABEL;

  // ATTR_GET_REQ: The user can request several different kinds of
  // information from the szgserver, depending upon the value of
  // AR_ATTR_GET_REQ_TYPE:
  //
  // NULL: Get the process table.
  // ALL:  Get all parameters in a format suitable for dbatch-ing
  // substring: Return only those parameters whose names match the substring.
  // value: Return the value corresponding to the attribute name in
  //   AR_ATTR_GET_REQ_ATTR
  int AR_ATTR_GET_REQ;
  int AR_ATTR_GET_REQ_TYPE;
  int AR_ATTR_GET_REQ_ATTR;

  // ATTR_GET_RES: The response to the above record, with the value stored in
  // the ATTR_GET_RES_VAL field.
  int AR_ATTR_GET_RES;
  int AR_ATTR_GET_RES_ATTR;
  int AR_ATTR_GET_RES_VAL;

  // ATTR_SET: Set an attribute's value.
  // The arSZGClient function formats the ATTR field as
  // computer/group/attribute. This is equivalent to storing
  // named records with key/value pairs on a per-host basis
  // (the group corresponds to the record name). The type field lets us
  // do either a set or an atomic test-and-set.
  int AR_ATTR_SET;
  int AR_ATTR_SET_TYPE;
  int AR_ATTR_SET_ATTR;
  int AR_ATTR_SET_VAL;

  // AR_SZG_MESSAGE: The user can send messages to other components using
  // this record. RESPONSE is 0 or 1, depending upon whether a resonse is
  // desired. USER is the Syzygy user. CONTEXT is the context in which the
  // message is sent (i.e. the virtual computer). DEST is the Syzygy ID of
  // the component to which the message is to be sent. TYPE and BODY give
  // the content of the message.
  // NOTE: the szgserver gets the message, fills in various fields (like
  // the ID) and forwards it to the destination.
  int AR_SZG_MESSAGE;
  int AR_SZG_MESSAGE_ID;
  int AR_SZG_MESSAGE_RESPONSE;
  int AR_SZG_MESSAGE_TYPE;
  int AR_SZG_MESSAGE_BODY;
  int AR_SZG_MESSAGE_DEST;

  // AR_SZG_MESSAGE_ADMIN: A catch-all record that can be used to respond
  // to a message, initiate a message right-of-response trade, or
  // trade the right-to-respond to a message
  int AR_SZG_MESSAGE_ADMIN;
  int AR_SZG_MESSAGE_ADMIN_ID;
  int AR_SZG_MESSAGE_ADMIN_STATUS;
  int AR_SZG_MESSAGE_ADMIN_TYPE;
  int AR_SZG_MESSAGE_ADMIN_BODY;

  // AR_SZG_MESSAGE_ACK: A catch-all for acknowledgements from the szgserver
  // of communications regarding messaging. For instance, this is sent
  // to the client in response to a message send (though it is not a message
  // response).
  int AR_SZG_MESSAGE_ACK;
  int AR_SZG_MESSAGE_ACK_ID;
  int AR_SZG_MESSAGE_ACK_STATUS;

  // AR_SZG_KILL_NOTIFICATION
  int AR_SZG_KILL_NOTIFICATION;
  int AR_SZG_KILL_NOTIFICATION_ID;

  // AR_SZG_LOCK_REQUEST: Allows a component to request a lock with a given
  // name.

  int AR_SZG_LOCK_REQUEST;
  int AR_SZG_LOCK_REQUEST_NAME;

  // AR_SZG_LOCK_RELEASE: Allows a component to release a lock (that it owns)
  // with a given name.

  int AR_SZG_LOCK_RELEASE;
  int AR_SZG_LOCK_RELEASE_NAME;

  // AR_SZG_LOCK_RESPONSE: Allows the szgserver to respond to lock request
  // and lock release messages.

  int AR_SZG_LOCK_RESPONSE;
  int AR_SZG_LOCK_RESPONSE_NAME;
  int AR_SZG_LOCK_RESPONSE_STATUS;
  int AR_SZG_LOCK_RESPONSE_OWNER;

  // AR_SZG_LOCK_NOTIFICATION: Allows the szgserver to notify a component
  // when a particular lock has been released.
  int AR_SZG_LOCK_NOTIFICATION;
  int AR_SZG_LOCK_NOTIFICATION_NAME;

  // AR_SZG_LOCK_LISTING: Allows the szgserver to respond to a request to
  // enumerate all locks
  int AR_SZG_LOCK_LISTING;
  int AR_SZG_LOCK_LISTING_LOCKS;
  int AR_SZG_LOCK_LISTING_COMPUTERS;
  int AR_SZG_LOCK_LISTING_COMPONENTS;

  // AR_SZG_REGISTER_SERVICE: Allows a client to register a service with the
  // szgserver for later discovery by other clients
  int AR_SZG_REGISTER_SERVICE;
  int AR_SZG_REGISTER_SERVICE_STATUS;
  int AR_SZG_REGISTER_SERVICE_TAG;
  int AR_SZG_REGISTER_SERVICE_NETWORKS;
  int AR_SZG_REGISTER_SERVICE_ADDRESSES;
  int AR_SZG_REGISTER_SERVICE_SIZE;
  int AR_SZG_REGISTER_SERVICE_COMPUTER;
  int AR_SZG_REGISTER_SERVICE_BLOCK;
  int AR_SZG_REGISTER_SERVICE_PORT;

  // AR_SZG_REQUEST_SERVICE: Allows a client to find the address of a service
  // that has previously registered with the szgserver.
  int AR_SZG_REQUEST_SERVICE;
  int AR_SZG_REQUEST_SERVICE_COMPUTER;
  int AR_SZG_REQUEST_SERVICE_TAG;
  int AR_SZG_REQUEST_SERVICE_NETWORKS;
  int AR_SZG_REQUEST_SERVICE_ASYNC;

  // AR_SZG_BROKER_RESULT: Used by the szgserver to communicate to tell the
  // client the address of the requested service
  int AR_SZG_BROKER_RESULT;
  int AR_SZG_BROKER_RESULT_STATUS;
  int AR_SZG_BROKER_RESULT_ADDRESS;
  int AR_SZG_BROKER_RESULT_PORT;

  // AR_SZG_GET_SERVICES: Used by the client to request a list of services
  // currently registered with the szgserver OR for the list of services
  // for which there are pending requests.
  int AR_SZG_GET_SERVICES;
  int AR_SZG_GET_SERVICES_TYPE;
  int AR_SZG_GET_SERVICES_SERVICES;
  int AR_SZG_GET_SERVICES_COMPUTERS;
  int AR_SZG_GET_SERVICES_COMPONENTS;

  // AR_SZG_SERVICE_RELEASE: A client uses this to request notification when
  // a service is released.
  int AR_SZG_SERVICE_RELEASE;
  int AR_SZG_SERVICE_RELEASE_NAME;
  int AR_SZG_SERVICE_RELEASE_COMPUTER;

  // AR_SZG_SERVICE_INFO: A client uses this to request a service's info.
  int AR_SZG_SERVICE_INFO;
  int AR_SZG_SERVICE_INFO_OP;
  int AR_SZG_SERVICE_INFO_STATUS;
  int AR_SZG_SERVICE_INFO_TAG;
};



// A description of a service, including where it exists, what networks
// it can communicate upon, the ports it uses, the components that must
// be notified when it is deleted, etc.
class arPhleetService {
 public:
  arPhleetService();

  bool valid;
  string info;      // Services can publish information here about their
                    //  characteristics. important for flexible brokering.
  string computer;  // host running the service
  int componentID;  // phleet ID of component running the service
  string networks;  // networks offering the service
  string addresses; // addresses offering the servic3, in same order
  int numberPorts;
  int* portIDs; // the actual port IDs
%GetCode
  if ((sipPy = PyList_New(sipCpp->numberPorts)) == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "unable to allocate list for arPhleetService.portIDs");
  } else {
    for (int i=0; i<sipCpp->numberPorts; ++i) {
      PyList_SET_ITEM(sipPy, i, PyInt_FromLong( static_cast<long>(sipCpp->portIDs[i]) ) );
    }
  }
%End
%SetCode
  PyErr_SetString(PyExc_RuntimeError, "Can't set arPhleetService.portIDs");
%End

//  list<arPhleetNotification> notifications;
};


// A specific way a client can connect to a service. The result of when
// a client asks the broker about a service
class arPhleetAddress {
  public:
  arPhleetAddress();
  bool valid;
  string address; // IP address
  int numberPorts; // how many ports
  int* portIDs; // the actual port IDs
%GetCode
  if ((sipPy = PyList_New(sipCpp->numberPorts)) == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "unable to allocate list for arPhleetAddress.portIDs");
  } else {
    for (int i=0; i<sipCpp->numberPorts; ++i) {
      PyList_SET_ITEM(sipPy, i, PyInt_FromLong( static_cast<long>(sipCpp->portIDs[i]) ) );
    }
  }
%End
%SetCode
  PyErr_SetString(PyExc_RuntimeError, "Can't set arPhleetAddress.portIDs");
%End
};


// Manages connection brokering services for the szgserver. This code
// could actually go in szgserver.cpp, but that file is getting too
// complicated. An important long term strategy would be to create more
// managers like this one (for instance for messages, locks, and the
// database)
class arPhleetConnectionBroker {
 public:
  arPhleetConnectionBroker();

//  void setReleaseNotificationCallback(void(*callback)(int, int, const string&));
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
                                 const string& networks, bool async);
//  SZGRequestList  getPendingRequests(const string& serviceName);
//  SZGRequestList  getPendingRequests() const;
  string getServiceNames() const;
  string getServiceComputers() const;
//  int    getServiceComponents(int*& IDs) const;
  int    getServiceComponentID(const string& serviceName) const;
  void   registerReleaseNotification(int componentID,
                                     int match,
                                     const string& computer,
                                     const string& serviceName);
  void _removeService(const string& serviceName);
  void removeComponent(int componentID);
  void print() const;
};

