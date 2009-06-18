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

//%Module szgphleet 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include math.sip

// math-global extra code
%ModuleHeaderCode
#include "arSZGClient.h"
#include <string>
using namespace std;
%End

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


/// Used for parsing/storing/writing the szg.conf config file and
/// the szg_<username>.conf login files
class arPhleetConfig {
  public:
  arPhleetConfig();
  ~arPhleetConfig();

  // Config file I/O.
  // Changed names because 'print' causes a name collision when you use
  // 'from PySZG import *'.
  bool readConfig();
%MethodCode
    sipRes = sipCpp->read();
%End
  bool writeConfig();
%MethodCode
    sipRes = sipCpp->write();
%End
  bool printConfig() const;
%MethodCode
    sipRes = sipCpp->print();
%End


  // Login file I/O.
  bool readLogin(bool fFromInit = false);
  bool writeLogin();
  bool printLogin() const;

  // get global config info
  /// computer name, as parsed from the config file
  string getComputerName();
  /// first port in block used for connection brokering, default 4700.
  int getFirstPort();
  /// size of block of ports, default 200.
  int getPortBlockSize();
  /// number of interfaces in the config.
  int getNumNetworks()/PyName=getNumberNetworks/;
  string getAddresses();
  string getNetworks();
  string getMasks();

  // get login info
  string getUserName();
  string getServerName();
  string getServerIP();
  int getServerPort();
  

  // manipulating the global config. using these methods,
  // command-line wrappers can be used to build the config file
  void   setComputerName(const string& name);
  void   addInterface(const string& networkName, 
                      const string& address,
                      const string& netmask);
  bool   deleteInterface(const string& networkName, const string& address);
  bool   setPortBlock(int firstPort, int blockSize);
  // manipulating the individual login. using these methods,
  // command-line wrappers can be used to manipulate the users
  // login file
  // These signatures are deliberately incorrect to match typemaps
  void   setUserName(const string& name);
  void   setServerName(const string& name);
  void   setServerIP(const string& IP);
  void   setServerPort(int port);
};


// ******************** based on arSZGClient.h ********************

class arSZGClient {
  public:
  arSZGClient();
  ~arSZGClient();

//  bool init(int&, char** argv, const string& forcedName = string("NULL"));
  // a0 and a1 in MethoCode are _pointers_ to the respective arguments!!!
  bool init( vector<string> a0, const string& a1 = string("NULL") );
%MethodCode
    int argc;
    char** argv;
    ar_vectorToArgcArgv( *a0, argc, argv );
    sipRes = sipCpp->init( argc, argv, *a1 );
    ar_cleanupArgcArgv( argc, argv );
%End

  void simpleHandshaking(bool state);
  void parseSpecialPhleetArgs(bool state);
  int failStandalone(bool fInited);

  // Ideally a stringstream should get mapped to a StringIO, but
  // I'm not sure how to do that yet.

  //stringstream& initResponse();
  bool sendInitResponse(bool state);
  //stringstream& startResponse();
  bool sendStartResponse(bool state);
  
  bool launchDiscoveryThreads();

  // functions that aid operation on a virtual computer
  string getNetworks(const string& channel);
  string getVirtualComputer();
  string getMode(const string& channel);
  string getTrigger(const string& virtualComputer);

  // Here are the functions for dealing with the parameter database
  // An abbreviation for the common case computerName=="NULL".
  bool setAttribute(const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue);
  bool setAttribute(const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue);
  bool setAttribute(const string& userName,
                    const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue);

  string testSetAttribute(const string& computerName,
                          const string& groupName,
                          const string& parameterName,
                          const string& parameterValue);
  string testSetAttribute(const string& userName,
                          const string& computerName,
                          const string& groupName,
                          const string& parameterName,
                          const string& parameterValue);

  // An abbreviation for the common case computerName=="NULL".
  string getAttribute(const string& groupName, 
                      const string& parameterName,
                      const string& validValues = "");
  string getAttribute(const string& computerName,
                      const string& groupName, 
                      const string& parameterName,
                      const string& validValues );
  string getAttribute(const string& userName,
                      const string& computerName,
                      const string& groupName,
                      const string& parameterName,
                      const string& validValues);

  bool getAttributeVector3( const string& groupName,
                            const string& parameterName,
                            arVector3& value /Out/ );

  arVector3 getAttributeVector( const string& groupName,
                                const string& parameterName );
%MethodCode
    arVector3 theVec;
    if (!sipCpp->getAttributeVector3( *a0, *a1, theVec )) {
      std::string msg("arSZGClient error: getAttributeVector() failed, probably ");
      msg += *a0+std::string("/")+*a1+std::string(" undefined.");
      PyErr_SetString(PyExc_ValueError,msg.c_str());
      sipIsErr = 1;
    } else {
      sipRes = new arVector3( theVec );
    }
%End

  // More abbreviations.
  int getAttributeInt(const string& groupName, 
                      const string& parameterName);
//  int getAttributeInt(const string&, const string&, const string&,
//              const string&);
  bool getAttributeFloats(const string& groupName,
                          const string& parameterName,
                          float* values,
                          int numvalues = 1);
  bool getAttributeInts(const string& groupName,
                        const string& parameterName,
                        int* values,
                        int numvalues = 1);
  bool getAttributeLongs(const string& groupName,
                         const string& parameterName,
                         long* values,
                         int numvalues = 1);
  string getAllAttributes(const string& substring);

  string getGlobalAttribute(const string& attributeName);
  string getGlobalAttribute( const string& userName, 
                             const string& attributeName );

  bool setGlobalAttribute(const string& attributeName,
                          const string& attributeValue);
  bool setGlobalAttribute(const string& userName,
                          const string& attributeName,
                          const string& attributeValue);

  bool parseAssignmentString(const string& text);

  // A way to get parameters in from a file (as in dbatch, for instance)
  bool parseParameterFile(const string& fileName, bool warn=true);

  string getLabel() const;

  string getComputerName() const;

  string getUserName() const;

  // general administration functions
  string getProcessList();
  bool killProcessID(const string& computer, const string& processLabel);
  string getProcessLabel(int processID);
  int getProcessID(const string& computer, const string& processLabel);
  int getProcessID(); // get my own process ID
  int sendMessage(const string& type, const string& body, int destination,
                   bool responseRequested = false);

//  int receiveMessage(string* userName /Out/, string* messageType /Out/, string* messageBody /Out/, string* context /Out/) /ReleaseGIL/ ;
 
  // There appears to be a SIP bug that prevents the above from working, so this
  // is a workaround.
  SIP_PYTUPLE receiveMessage();
%MethodCode
    string userName;
    string messageType;
    string messageBody;
    string context;
    PyThreadState *_save;
    _save = PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();
    int messageID = sipCpp->receiveMessage( &userName, &messageType, &messageBody, &context );
    PyEval_AcquireLock();
    PyThreadState_Swap(_save);
    sipRes = PyTuple_New(5);
    if (sipRes == NULL) {
      PyErr_SetString(PyExc_RuntimeError, "unable to allocate tuple for arSZGClient.receiveMessage");
      sipIsErr = 1;
    } else {
      PyTuple_SetItem( sipRes, 0, PyInt_FromLong( static_cast<long>(messageID) ) );
      PyTuple_SetItem( sipRes, 1, PyString_FromString( userName.c_str() ) );
      PyTuple_SetItem( sipRes, 2, PyString_FromString( messageType.c_str() ) );
      PyTuple_SetItem( sipRes, 3, PyString_FromString( messageBody.c_str() ) );
      PyTuple_SetItem( sipRes, 4, PyString_FromString( context.c_str() ) );
    }
%End

  bool messageResponse(int messageID, const string& body,
                       bool partialResponse = false);

  int getMessageResponse(list<int> tags,
                         string& body /Out/,
                         int& match /Out/,
                         int timeout = -1);

  int  startMessageOwnershipTrade(int messageID, const string& key);
  bool finishMessageOwnershipTrade(int match, int timeout = -1);
  bool revokeMessageOwnershipTrade(const string& key);
  int  requestMessageOwnership(const string& key);
  /// If a phleet component is launched via dex, it is our responsibility
  /// to respond in some fashion to the launching message. We can do so
  /// by getting the ID of the launching message like so.
  int getLaunchingMessageID();

  // It is very convenient to be able to get a notification when a component
  // exits.
  int requestKillNotification(int componentID);
  int getKillNotification( list<int> tags, int timeout = -1);

  // Functions dealing with locks.
  bool getLock(const string& lockName, int& ownerID /Out/);
  bool releaseLock(const string& lockName);
  int requestLockReleaseNotification(const string& lockName);
  int getLockReleaseNotification(list<int> tags, int timeout = -1);
  vector<string> findLocks();
  void printLocks();

  // connection brokering functions
  bool registerService(const string& serviceName, const string& channel,
                       int numberPorts, int* portIDs);
  bool requestNewPorts(const string& serviceName, const string& channel,
                       int numberPorts, int* portIDs);
  bool confirmPorts(const string& serviceName, const string& channel,
                    int numberPorts, int* portIDs);
  arPhleetAddress discoverService(const string& serviceName,
                                  const string& networks,
                                  bool async) /TransferBack/;
  int requestServiceReleaseNotification(const string& serviceName);
  int getServiceReleaseNotification(list<int> tags, int timeout = -1);
  string getServiceInfo(const string& serviceName);
  bool setServiceInfo(const string& serviceName,
                      const string& info);

  vector<string> findActiveServices();
  vector<string> findPendingServices();

  void printServices();
  void printPendingServiceRequests();
  int  getServiceComponentID(const string& serviceName);

  // functions that aid operation on a virtual computer
  string getAddresses(const string& channel);
  string createComplexServiceName(const string& serviceName);
  string createContext();
  string createContext(const string& virtualComputer,
                       const string& modeChannel,
                       const string& mode,
                       const string& networksChannel,
                       const string& networks);

  // functions pertaining to connecting to an szgserver
  bool discoverSZGServer(const string& name,
                         const string& broadcast);
  void printSZGServers(const string& broadcast);
  vector<string> findSZGServers(const string& broadcast);

  void setServerLocation(const string& IPaddress, int port);
  bool logout();
  void closeConnection();

  bool sendReload(const string& computer, const string& processLabel);

  int getLogLevel() const;
  bool connected() const;

private:
  // To prevent SIP from generating a copy constructor...
  arSZGClient( const arSZGClient& );
};  


