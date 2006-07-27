// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

// The arSZGClient is the basic communications object. In a normal,
// framework-based application, the only method youll probably need
// is getAttribute() for reading parameters from the Syzygy database.
//
// Example: to get the position of screen 0 
//
// Example: youre running a master/slave application and you need to send a
// message to the master from a slave. You do that with the arAppLauncher
// and the arSZGClient, as shown here (fw is an arMasterSlaveFramework).
//
//  cl = fw.getSZGClient()
//  al = fw.getAppLauncher()
//  mn = al.getMasterPipeNumber()
//  rp = al.getRenderProgram( mn )
//  if rp == 'NULL':
//    return
//  rpl = rp.split('/')
//  id = cl.getProcessID(rpl[0],rpl[1])
//  m = cl.sendMessage( 'user', 'connected', id, False )


%{
#include "arPhleetConfigParser.h"
%}

/// Used for parsing/storing/writing the szg.conf config file and
/// the szg_<username>.conf login files
class arPhleetConfigParser {
 public:
  arPhleetConfigParser();
  ~arPhleetConfigParser() {}

  // bulk manipulation of the config file
  bool parseConfigFile();
  bool writeConfigFile();

  // bulk manipulation of users login file
  bool parseLoginFile();
  bool writeLoginFile();

  // output configuration
  void printConfig();
  void printLogin();

  // get global config info
  /// computer name, as parsed from the config file
  string getComputerName();
  /// first port in block used for connection brokering, default 4700.
  int getFirstPort();
  /// size of block of ports, default 200.
  int getPortBlockSize();
  /// number of interfaces in the config.
  int getNumberInterfaces();
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
  void   setUserName(string name);
  void   setServerName(string name);
  void   setServerIP(string IP);
  void   setServerPort(int port);
};


// ******************** based on arSZGClient.h ********************

class arSZGClient{
 public:
  arSZGClient();
  ~arSZGClient();

  void simpleHandshaking(bool state);
  void parseSpecialPhleetArgs(bool state);
  bool init(int&, char** argv, const string& forcedName = string("NULL"));
  stringstream& initResponse(){ return _initResponseStream; }
  bool sendInitResponse(bool state);
  stringstream& startResponse(){ return _startResponseStream; }
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
%extend{
  bool setAttributeComputer(const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue) {
      return self->setAttribute( computerName, groupName, parameterName, parameterValue );
      }
  bool setAttributeUser(const string& userName,
                    const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue)  {
      return self->setAttribute( userName, computerName, groupName, parameterName, parameterValue );
      }
}
  string testSetAttribute(const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& parameterValue);
%extend {
  string testSetAttributeUser(const string& userName,
                          const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& parameterValue) {
    return self->testSetAttribute( userName, computerName, groupName, parameterName, parameterValue );
    }
}
  // An abbreviation for the common case computerName=="NULL".
  string getAttribute(const string& groupName, 
                      const string& parameterName,
              const string& validValues = "");
%extend {
  string getAttributeUser(const string& userName,
              const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& validValues) {
    return self->getAttribute( userName, computerName, groupName, parameterName, validValues );
    }
  string getAttributeComputer(const string& computerName,
                      const string& groupName, 
                      const string& parameterName,
              const string& validValues /* no default */) {
    return self->getAttribute( computerName, groupName, parameterName, validValues );
    }
  PyObject* getAttributeVector( const string& groupName,
                                const string& parameterName ) {
    arVector3 theVec;
    if (!self->getAttributeVector3( groupName, parameterName, theVec )) {
        std::string msg("arSZGClient error: getAttributeVector() failed, probably ");
        msg += groupName+std::string("/")+parameterName+std::string(" undefined.");
        PyErr_SetString(PyExc_RuntimeError,msg.c_str());
        return NULL;
    }
    PyObject* result = PyTuple_New( 3 );
    if (!result) {
      PyErr_SetString( PyExc_MemoryError, "unable to allocate new tuple for getAttributeVector3() result." );
      return NULL;
    }
    for (int i=0; i<3; ++i) {
      PyTuple_SetItem( result, i, PyFloat_FromDouble((double)theVec.v[i]) );
    }
    return result;
  }
}
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
  int getProcessID(void); // get my own process ID
  int sendMessage(const string& type, const string& body, int destination,
                   bool responseRequested = false);

%extend {
  PyObject* receiveMessage(void) {
    std::string messageType, messageBody;
/* We need to store the current thread state and release the Python
   global interpreter lock before calling receiveMessage() here;
   otherwise, all of Python will lock up until receiveMessage()
   returns (and receiveMessage() blocks until it receives a message,
   so that would be bad). After receiveMessage() returns, we
   re-acquire the interpreter lock and restore the thread state.
*/
    PyThreadState *_save;
    _save = PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();
    int messageID = self->receiveMessage( &messageType, &messageBody );
    PyEval_AcquireLock();
    PyThreadState_Swap(_save);
    PyObject* retTuple = PyTuple_New(3);
    PyTuple_SetItem( retTuple, 0, PyInt_FromLong( (long)messageID ) );
    if (messageID != 0) {
      PyTuple_SetItem( retTuple, 1, PyString_FromString( messageType.c_str() ) );
      PyTuple_SetItem( retTuple, 2, PyString_FromString( messageBody.c_str() ) );
    } else {
      PyTuple_SetItem( retTuple, 1, Py_None );
      PyTuple_SetItem( retTuple, 2, Py_None );
    }
    return retTuple;
  }
}

  bool messageResponse(int messageID, const string& body,
                       bool partialResponse = false);


  int  startMessageOwnershipTrade(int messageID, const string& key);
  bool finishMessageOwnershipTrade(int match, int timeout = -1);
  bool revokeMessageOwnershipTrade(const string& key);
  int  requestMessageOwnership(const string& key);
  /// If a phleet component is launched via dex, it is our responsibility
  /// to respond in some fashion to the launching message. We can do so
  /// by getting the ID of the launching message like so.
  int getLaunchingMessageID(){ return _launchingMessageID; }

  // It is very convenient to be able to get a notification when a component
  // exits.
  int requestKillNotification(int componentID);
  int getKillNotification(list<int> tags, int timeout = -1);

  // Functions dealing with locks.
  bool getLock(const string& lockName, int& ownerID);
  bool releaseLock(const string& lockName);
  int requestLockReleaseNotification(const string& lockName);
  int getLockReleaseNotification(list<int> tags, int timeout = -1);
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
                                  bool async);
  int requestServiceReleaseNotification(const string& serviceName);
  int getServiceReleaseNotification(list<int> tags, int timeout = -1);
  string getServiceInfo(const string& serviceName);
  bool setServiceInfo(const string& serviceName,
                      const string& info);
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
                       const arSlashString& networks);

  // functions pertaining to connecting to an szgserver
  bool discoverSZGServer(const string& name,
                         const string& broadcast);
  void printSZGServers(const string& broadcast);
  vector<string> findSZGServers(const string& broadcast);
  void setServerLocation(const string& IPaddress, int port);
  bool writeLoginFile(const string& userName);
  bool logout();
  void closeConnection();

  bool sendReload(const string& computer, const string& processLabel);

  int getLogLevel() const { return _logLevel; }
  bool connected() const { return _connected; }

};  


