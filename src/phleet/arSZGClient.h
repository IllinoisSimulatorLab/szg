//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SZG_CLIENT_H
#define AR_SZG_CLIENT_H

#include "arDataClient.h"
#include "arThread.h"
#include "arUDPSocket.h"
#include "arStructuredDataParser.h"
#include "arPhleetConfigParser.h"
#include "arPhleetConnectionBroker.h"
#include "arPhleetOSLanguage.h"
#include "arMath.h"
#include "arPhleetCalling.h"

#include <list>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

// How an application connects to the rest of the syzygy cluster.

class SZG_CALL arSZGClient{
  // Needs assignment operator and copy constructor, for pointer members.
  friend void arSZGClientServerResponseThread(void*);
  friend void arSZGClientTimerThread(void*);
  friend void arSZGClientDataThread(void*);
  friend class arPhleetConfigParser; // its parseLoginFile() calls _dialUpFallThrough()
 public:
  arSZGClient();
  ~arSZGClient();

  void simpleHandshaking(bool state);
  void parseSpecialPhleetArgs(bool state);
  bool init(int&, char** const argv, string forcedName = string("NULL"));
       // Call init() before parsing argv, so "-szg foo=..." works.
  int failStandalone(bool fInited) const;
  stringstream& initResponse(){ return _initResponseStream; }
  bool sendInitResponse(bool ok);
  stringstream& startResponse(){ return _startResponseStream; }
  bool sendStartResponse(bool ok);

  bool launchDiscoveryThreads();

  // Parameter database.
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
			  const string& paramterName,
			  const string& parameterValue);
  string testSetAttribute(const string& userName,
                          const string& computerName,
			  const string& groupName,
			  const string& parameterName,
			  const string& parameterValue);
  string getAttribute(const string& userName,
		      const string& computerName,
		      const string& groupName,
		      const string& parameterName,
		      const string& validValues);
  string getAttribute(const string& computerName,
                      const string& groupName,
                      const string& parameterName,
		      const string& validValues /* no default */);

  // Parameter database, abbreviations for common cases thereof.
  bool setAttribute(const string& groupName,
                    const string& parameterName,
                    const string& parameterValue)
    { return setAttribute("NULL", groupName, parameterName, parameterValue); }
  string getAttribute(const string& groupName,
                      const string& parameterName,
		      const string& validValues = "")
    { return getAttribute("NULL", groupName, parameterName, validValues); }
  int getAttributeInt(const string& groupName,
                      const string& parameterName);
  int getAttributeInt(const string&, const string&, const string&,
		      const string&);
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
  bool getAttributeVector3( const string& groupName,
                            const string& parameterName,
                            arVector3& value );
  string getAllAttributes(const string& substring);
  const string getDataPath()
    { return getAttribute("SZG_DATA", "path"); }
  const string getDataPathPython()
    { return getAttribute("SZG_PYTHON", "path"); }

  // "Global" attributes.
  bool setGlobalAttribute(const string& attributeName,
			  const string& attributeValue);
  bool setGlobalAttribute(const string& userName,
			  const string& attributeName,
			  const string& attributeValue);
  string getGlobalAttribute(const string& attributeName);
  string getGlobalAttribute(const string& userName,
			    const string& attributeName);
  string getSetGlobalXML(const string& userName,
			 const arSlashString& pathList,
			 const string& attributeValue);
  string getSetGlobalXML(const arSlashString& pathList,
                         const string& attributeValue = "NULL"){
    return getSetGlobalXML(_userName, pathList, attributeValue);
  }

  bool parseAssignmentString(const string& text);
  // A way to get parameters in from a file (as in dbatch, for instance)
  bool parseParameterFile(const string& fileName, bool warn = true);

  const string& getLabel() const
    { return _exeName; }

  const string& getComputerName() const
    { return _computerName; }
  const string& getUserName() const
    { return _userName; }

  // general administration functions
  string getProcessList();
  bool killProcessID(int id);
  bool killProcessID(const string& computer, const string& processLabel);
  string getProcessLabel(int processID);
  int getProcessID(const string& computer, const string& processLabel);
  int getProcessID(void); // get my own process ID

  // Functions dealing with messaging.
  int sendMessage(const string& type, const string& body, int destination,
                   bool responseRequested = false);
  int sendMessage(const string& type, const string& body,
		   const string& context, int destination,
                   bool responseRequested = false);
  int receiveMessage(string* messageType, string* messageBody)
    { return receiveMessage(NULL, messageType, messageBody, NULL); }
  int receiveMessage(string* userName, string* messageType,
                      string* messageBody)
    { return receiveMessage(userName, messageType, messageBody, NULL); }
  int receiveMessage(string* userName, string* messageType,
                      string* messageBody, string* context);
  int getMessageResponse(list<int> tags,
                         string& body,
                         int& match,
                         int timeout = -1);
  bool messageResponse(int messageID, const string& body,
                       bool partialResponse = false);
  int  startMessageOwnershipTrade(int messageID, const string& key);
  bool finishMessageOwnershipTrade(int match, int timeout = -1);
  bool revokeMessageOwnershipTrade(const string& key);
  int  requestMessageOwnership(const string& key);
  // If a phleet component is launched via dex, it is our responsibility
  // to respond in some fashion to the launching message. We can do so
  // by getting the ID of the launching message like so.
  int getLaunchingMessageID(){ return _launchingMessageID; }

  // Get a notification when a component exits.
  int requestKillNotification(int componentID);
  int getKillNotification(list<int> tags, int timeout = -1);

  // Locks.
  bool getLock(const string& lockName, int& ownerID);
  bool releaseLock(const string& lockName);
  int requestLockReleaseNotification(const string& lockName);
  int getLockReleaseNotification(list<int> tags, int timeout = -1);
  void printLocks();

  // Connection brokering.
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
  void _printServices(const string& type);
  void printServices();
  void printPendingServiceRequests();
  int  getServiceComponentID(const string& serviceName);

  // functions that aid operation on a virtual computer
  arSlashString getNetworks(const string& channel);
  arSlashString getAddresses(const string& channel);
  const string& getVirtualComputer();
  string getVirtualComputers();
  const string& getMode(const string& channel);
  string getTrigger(const string& virtualComputer);
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
  void setServerLocation(const string& IPaddress, int port);
  bool writeLoginFile(const string& userName);
  bool logout();
  void closeConnection();

  bool sendReload(const string& computer, const string& processLabel);

  int getLogLevel() const { return _logLevel; }
  bool connected() const { return _connected; }
  operator bool() const { return _connected; }
  const string& getServerName();

 private:
  arPhleetConfigParser _configParser;
  arPhleetOSLanguage   _l;
  arDataClient         _dataClient;
  string               _IPaddress;
  int                  _port;
  string               _serverName;
  string               _computerName;
  string               _userName;
  string               _exeName;
  arSlashString       _networks;  // the default networks
  arSlashString       _addresses; // the default addresses
  // A HACK to enable the arSZGClient to route network traffic differently
  // for different types of services. IT SEEMS LAME TO PUT THIS HERE.
  arSlashString       _graphicsNetworks;
  arSlashString       _graphicsAddresses;
  arSlashString       _soundNetworks;
  arSlashString       _soundAddresses;
  arSlashString       _inputNetworks;
  arSlashString       _inputAddresses;
  // the overall mode: master, trigger, component
  string               _mode;
  // SZG_DISPLAY0, etc.
  string               _graphicsMode;
  // file from which parameters are read in standalone mode
  string               _parameterFileName;
  string               _virtualComputer;

  // Don't init sockets in the global name space,
  // because of the "global" construction that inits winsock automatically.
  // todo: could we eliminate that?
  arUDPSocket*  _discoverySocket;

  bool          _connected;

  ARchar*       _receiveBuffer;
  int           _receiveBufferSize;

  // Members related to the handshaking and information-passing on start-up
  int           _launchingMessageID;
  bool          _dexHandshaking;
  bool          _simpleHandshaking;
  bool          _ignoreMessageResponses;
  bool          _parseSpecialPhleetArgs;
  stringstream  _initResponseStream;
  int           _initialInitLength;
  stringstream  _startResponseStream;
  int           _initialStartLength;
  // Member related to service registration/discovery. NOTE: we want to be
  // able to use the service/registration/discovery functions in multiple
  // threads.
  arMutex       _serviceLock;
  int           _nextMatch;   // to match up async rpc's

  arStructuredDataParser* _dataParser;
  arThread                _clientDataThread;
  
  // Verbosity of printed remarks.
  int _logLevel;

  // Internal functions.
  bool _dialUpFallThrough();
  arStructuredData* _getDataByID(int recordID);
  arStructuredData* _getTaggedData(int tag,
                                   int recordID = -1,
                                   int timeout = -1);
  int _getTaggedData(arStructuredData*& message,
                     list<int> tags,
                     int recordID = -1,
                     int timeout = -1);
  string _getAttributeResponse(int match);
  bool _getMessageAck(int match,
                      const char* transaction,
                      int* id = NULL,
                      int timeout = -1);
  bool _send(const char* diagnostic);
  bool _setLabel(const string& label);
  string _generateLaunchInfoHeader();
  bool _sendResponse(stringstream&, const char*, unsigned, bool, bool);

  // functions dealing with the local parameter database (as will only
  // be used in standalone mode)
  string _getAttributeLocal(const string&, const string&, const string&,
			    const string&);
  bool _setAttributeLocal(const string&, const string&, const string&,
			  const string&);
  string _getGlobalAttributeLocal(const string&);
  bool _setGlobalAttributeLocal(const string&, const string&);
  string _changeToValidValue(const string&, const string&,
                             const string&, const string&);
  map<string, string, less<string> > _localParameters;

  // functions relating to parsing phleet-specific args or the context
  bool _parseContext();
  bool _parsePhleetArgs(int& argc, char** const argv);
  bool _parseContextPair(const string& pair);
  bool _checkAndSetNetworks(const string& channel,
                            const arSlashString& networks);
  bool _getPortsCore1(const string& serviceName, const string& channel,
                      int numberPorts, arStructuredData*& data, int& match,
		      bool fRetry);
  bool _getPortsCore2(arStructuredData* data, int match, int* portIDs,
		      bool fRetry);
  int  _fillMatchField(arStructuredData*);

  // server discovery functions and variables
  void    _sendDiscoveryPacket(const string&, const string&);
  bool    _discoveryThreadsLaunched;
  bool    _beginTimer;
  string  _requestedName;
  bool    _dataRequested;
  arMutex _queueLock; // for server discovery functions
  bool    _keepRunning;
  arConditionVar _dataCondVar;
  arConditionVar _timerCondVar;
  bool    _justPrinting;
  char    _responseBuffer[512];
  list<string> _foundServers; // prevents repeats being printed.

  void _serverResponseThread();
  void _timerThread();
  void _dataThread();
};

SZG_CALL void ar_messageTask(void* pClient);

#endif
