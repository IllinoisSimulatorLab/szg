//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPhleetConfigParser.h"
#include "arDataUtilities.h" // for ar_getUser()
#include <sys/types.h>     // for chmod and stat
#include <sys/stat.h>      // for chmod and stat

#ifdef AR_USE_WIN_32
#include <io.h>            // for chmod
#include <direct.h>        // for mkdir
#endif

#ifdef AR_USE_WIN_32
// NOTE: the following cannot be const for Win32 since we check to see
// if there is a c: drive on the computer, and, if not, use the d: drive.
// NOTE: on windows all files are now in a subdirectory of the C drive.
// Previously, they were at the top level. However, with Windows XP, only 
// the administrator user can put a file there, but anybody can create a
// directory there... and anybody can write into that directory.
char* windowsPhleetDirectory = "C:\\szg";
char* phleetConfigFile  = "C:\\szg\\szg.conf";
char* phleetAlternativeConfigFile = "C:\\szg\\szg.conf";
char* phleetLoginPreamble = "C:\\szg\\szg_";
#else
const char* phleetConfigFile  = "/etc/szg.conf";
const char* phleetAlternativeConfigFile = "/tmp/szg.conf";
const char* phleetLoginPreamble = "/tmp/szg_";
#endif

/// \todo use initializers
arPhleetConfigParser::arPhleetConfigParser(){
  _alternativeConfig = false;
  _computerName = string("NULL");
  _numberInterfaces = 0;
  // note the defaults for the port block. These are pretty reasonable
  // for both Linux and Win32
  _firstPort = 4700;
  _blockSize = 200;
  _userName = string("NULL");
  _serverIP = string("NULL");
  _serverPort = 0;
  _serverName = string("NULL");
  _fileParser = new arStructuredDataParser(_l.getDictionary());
}

/// To enable people to experiment w/ Phleet on Unix systems, the location of
/// the config file can be altered from the default /etc/szg.conf to
/// /tmp/szg.conf. This does nothing on Win32 systems.
void arPhleetConfigParser::useAlternativeConfigFile(bool flag){
  _alternativeConfig = flag;
}

/// \todo Unify the callers' error messages, they differ wildly at the moment.
/// Parse the config file into internal storage, returning 
/// true iff successful. Note the logic about which config file to parse.
bool arPhleetConfigParser::parseConfigFile(){
  // perform the sanity check that, for instance, on Win32 changes the
  // file names to use the d: drive if the c: drive does not exist
  if (!_sanityCheck()){
    return false;
  }
  // if _alternativeConfig has been set, we are forced to use the 
  // "alternative config file". Otherwise, first try the
  // standard file... and only if that does not exist, try the
  // alternative.
  string fileName;
  arFileTextStream configStream;
  if (!_alternativeConfig){
    fileName = string(phleetConfigFile);
    if (!configStream.ar_open(fileName))
      goto LAlternative;
  }
  else{
LAlternative:
    fileName = string(phleetAlternativeConfigFile);
    if (!configStream.ar_open(fileName)){
      // no error message here... messages are output in arSZGClient
      return false;
    }
  }

  // clear the internal storage first
  _networkList.clear();

  arStructuredData* data;
  // sadly, this is actually normal... 
  // BUG MUST FIX in arStructuredDataParser
  while ((data = _fileParser->parse(&configStream)) != NULL){
    const int ID = data->getID();
    if (ID == _l.AR_COMPUTER)
      _processComputerRecord(data);
    else if (ID == _l.AR_INTERFACE)
      _processInterfaceRecord(data);
    else if (ID == _l.AR_PORTS)
      _processPortsRecord(data);
    else
      cerr << "phleet warning: config file contains unexpected ID "
           << ID << ".\n";
  }
  configStream.ar_close();
  return true;
}

/// Write the config file information we are holding to the appropriate config
/// file location (depending on the alternative config vs. actual config)
bool arPhleetConfigParser::writeConfigFile(){
  const char* filename = _alternativeConfig ?
    phleetAlternativeConfigFile : phleetConfigFile;
  if (!_sanityCheck()) {
    cerr << "phleet error: failed to write config file.\n";
    return false;
  }

  // NOTE the difference between writing the config file and reading it.
  // Here we only try to write the specified file (while there, we go
  // through a fault tree).
  FILE* config = fopen(filename, "w");
  if (!config){
    cerr << "phleet error: failed to write config file " << filename << ".\n";
    return false;
  }
  
  _writeName(config);
  _writeInterfaces(config);
  _writePorts(config);

#ifdef AR_USE_WIN_32
  // there is only one config file on Win32
  _chmod(phleetConfigFile, 00666);
#else
  if (_alternativeConfig){
    chmod(phleetAlternativeConfigFile, 006666);
  }
#endif
  fclose(config);
  return true;
}

/// Read in the login-file, szg_<user name>.conf, if it exists. Note that
/// it is not, necessarily, an error if this file does not exist on a
/// particular computer(the user might not have logged-in yet).
bool arPhleetConfigParser::parseLoginFile(){
  if (!_sanityCheck()){
    return false;
  }
  string fileName = phleetLoginPreamble + ar_getUser() + string(".conf");
  arFileTextStream loginStream;
  if (!loginStream.ar_open(fileName)){
    // no error message here, the caller will have to do the work
    return false;
  }
  arStructuredData* data = _fileParser->parse(&loginStream);
  if (!data || data->getID() != _l.AR_LOGIN){
    // there must be exactly one login record in the file
    return false;
  }
  _userName = data->getDataString(_l.AR_LOGIN_USER);
  _serverName = data->getDataString(_l.AR_LOGIN_SERVER_NAME);
  _serverIP = data->getDataString(_l.AR_LOGIN_SERVER_IP);
  _serverPort = data->getDataInt(_l.AR_LOGIN_SERVER_PORT);
  _fileParser->recycle(data);
  loginStream.ar_close();
  return true;
}

/// Write the login file, szg_<user name>.conf.
bool arPhleetConfigParser::writeLoginFile(){
  if (!_sanityCheck()){
    return false;
  }
  string fileName = phleetLoginPreamble + ar_getUser() + string(".conf");
  FILE* login = fopen(fileName.c_str(),"w");
  if (!login){
    return false;
  }
  arStructuredData* data = _fileParser->getStorage(_l.AR_LOGIN);
  data->dataInString(_l.AR_LOGIN_USER, _userName);
  data->dataInString(_l.AR_LOGIN_SERVER_NAME, _serverName);
  data->dataInString(_l.AR_LOGIN_SERVER_IP, _serverIP);
  data->dataIn(_l.AR_LOGIN_SERVER_PORT, &_serverPort, AR_INT, 1);
  data->print(login);
  fclose(login);
  // NOTE that we DO NOT change permissions on the login file. Other
  // users should not be able to read it (at least on Unix systems)
  return true;
}

/// Print human-readable config file information
void arPhleetConfigParser::printConfig(){
  cout << "Phleet configuration:\n"
       << "    computer = " << _computerName << "\n";
  list<pair<string, string> >::iterator i = _networkList.begin();
  if (_networkList.size() == 1) {
    // special case
    cout << "    network  = " << i->first
	 << ", " << i->second << ":";
  }
  else {
    for (; i != _networkList.end(); i++){
      cout << "    network  = " << i->first
	   << ", address = " << i->second << "\n";
    }
    cout << "    ports    = ";
  }
  cout << _firstPort << "-" << _firstPort+_blockSize-1 << "\n";
}

/// Print human-readable login information
void arPhleetConfigParser::printLogin(){
  cout << "Phleet login:\n"
       << "    system user name = " << ar_getUser() << "\n"
       << "    phleet user name = " << _userName << "\n"
       << "    szgserver        = \""
       << _serverName << "\", " << _serverIP << ":" << _serverPort << "\n";
}

/// Returns slash-delimited addresses (as defined in the config file) of
/// interfaces used by this computer, or empty string if there are none.
/// If networkName is not "", restrict addresses to that kind:
/// Thus, if this computer has 2 dotted-quad addresses
/// XXX.XXX.XXX.XXX and YYY.YYY.YYY.YYY, and networkName is "internet",
/// then return XXX.XXX.XXX.XXX/YYY.YYY.YYY.YYY.
arSlashString arPhleetConfigParser::getAddresses(const string& networkName){
  arSlashString result;
  for (list<pair<string, string> >::iterator i=_networkList.begin();
       i != _networkList.end(); i++){
    if (networkName == "" || i->first == networkName)
      result /= i->second;
  }
  return result;
}

/// Returns the networks to which the computer is connected. Note that
/// the empty string is returned if the computer (according to the
/// config file) is not attached to any networks. This is used
/// in connection brokering.
arSlashString arPhleetConfigParser::getNetworks(){
  list<string> uniqueNetworks;
  for (list<pair<string, string> >::iterator i=_networkList.begin();
       i != _networkList.end(); i++){
    uniqueNetworks.push_back(i->first);
  }
  // remove duplicates
  uniqueNetworks.unique();

  arSlashString result;
  for (list<string>::iterator j = uniqueNetworks.begin();
       j != uniqueNetworks.end(); j++){
    result /= *j;
  }
  return result;
}

/// Sets the computer name internally (you have to invoke writeConfigFile
/// explicitly to write this change out to disk).
void arPhleetConfigParser::setComputerName(const string& name){
  _computerName = name;
}

/// Adds a new interface internally (you have to invoke writeConfigFile
/// explicitly to write this change out to disk). If the interface does
/// not exist, adds a pair to the list. If the interface does exist,
/// alters the address
void arPhleetConfigParser::addInterface(const string& networkName,
					const string& address){
  bool result = true; // don't know if it is a duplicate yet
  for (list<pair<string, string> >::iterator i = _networkList.begin();
       i != _networkList.end(); i++){
    if (networkName == i->first){
      result = false;
      i->second = address;
      break; // no need to search further
    }
  }
  if (result){
    // no match was found
    _networkList.push_back(pair<string,string>(networkName,address));
  }
}

/// Removes an interface internally (you have to invoke writeConfigFile
/// explicitly to write this change out to disk). Returns false if the
/// network name/ address does not describe an existing interface and
/// returns true otherwise.
bool arPhleetConfigParser::deleteInterface(const string& networkName,
					   const string& address){
  const pair<string, string> candidate(networkName, address);
  for (list<pair<string, string> >::iterator i = _networkList.begin();
       i != _networkList.end(); i++){
    if (*i == candidate){
      _networkList.erase(i);
      return true; // no need to search further
    }
  }
  cerr << "phleet error: failed to delete missing interface "
       << networkName  << "/" << address << ".\n";
  return false;
}

/// Sets the internal port block (you have to invoke writeConfigFile
/// explicitly to write this change out to disk). Returns false if
/// the port block description is nonsensical somehow (for instance
/// the block size is less than 1) and returns true otherwise.
bool arPhleetConfigParser::setPortBlock(int firstPort, int blockSize){
  // a little sanity check
  if (firstPort < 1024 || blockSize < 1){
    cout << "arPhleetConfigParser error: block parameters are invalid.\n";
    return false;
  }
  _firstPort = firstPort;
  _blockSize = blockSize;
  return true;
}

/// Sets the internal user name
void arPhleetConfigParser::setUserName(string name){
  _userName = name;
}

/// Sets the internal szgserver name
void arPhleetConfigParser::setServerName(string name){
  _serverName = name;
}

/// Sets the internal szgserver IP
void arPhleetConfigParser::setServerIP(string IP){
  _serverIP = IP;
}

/// Sets the internal szgserver port
void arPhleetConfigParser::setServerPort(int port){
  _serverPort = port;
}

/// On Win32, makes sure that the C: drive exists, and, if not, changes
/// the file name to use the D: drive. On Unix, makes sure that either
/// /tmp or /etc exist, depending on whether we are using the alternative
/// config file.

bool arPhleetConfigParser::_sanityCheck(){
#ifdef AR_USE_WIN_32
  struct _stat statbuf;
  // check for the existence of the C drive
  if (_stat ("C:\\", &statbuf) < 0) {
    // drive C: not found
    if (_stat ("D:\\", &statbuf) < 0) {
      printf("syzygy error: neither drive C: nor drive D: found.\n");
      return false;
    }
    // Use drive D instead of C.
    windowsPhleetDirectory[0] = 'D';
    phleetConfigFile[0] = 'D';
    phleetAlternativeConfigFile[0] = 'D';
    phleetLoginPreamble[0] = 'D';
  }
  // check for the existence of C:\szg (or D:\szg if the C drive does not
  // exist). If it does not exist, create it.
  if (_stat(windowsPhleetDirectory, &statbuf) < 0){
    _mkdir(windowsPhleetDirectory);
    // AARGH! This does't seem to work as expected! Specifically, the hope is that any
    // user will be able to change the szg.conf file. Unfortunately, Windows file
    // permissions are not really accessed this way (i.e. group and all don't seem to
    // work). The upshot is that only an administrator or the original creator of the
    // szg.conf file can change it after it has been created. Maybe that's a good thing...
    _chmod(windowsPhleetDirectory, 00666);
  }
#else
  struct stat statbuf;
  if (_alternativeConfig){
    if (stat ("/tmp", &statbuf) < 0) {
      printf("syzygy error: /tmp not found.  Please create /tmp!\n");
      return false;
    }
  }
  else{
    if (stat ("/etc", &statbuf) < 0) {
      printf("syzygy error: /etc not found.  Please create /etc!\n");
      return false;
    }
  }
#endif
  return true;
}

void arPhleetConfigParser::_processComputerRecord(arStructuredData* data){
  setComputerName(data->getDataString(_l.AR_COMPUTER_NAME));
}

void arPhleetConfigParser::_processInterfaceRecord(arStructuredData* data){
  addInterface(data->getDataString(_l.AR_INTERFACE_NAME),
	       data->getDataString(_l.AR_INTERFACE_ADDRESS));
}

void arPhleetConfigParser::_processPortsRecord(arStructuredData* data){
  setPortBlock(data->getDataInt(_l.AR_PORTS_FIRST),
	       data->getDataInt(_l.AR_PORTS_SIZE));
}

bool arPhleetConfigParser::_writeName(FILE* output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_COMPUTER);
  data->dataInString(_l.AR_COMPUTER_NAME, _computerName);
  data->print(output);
  _fileParser->recycle(data);
  return true;
}

bool arPhleetConfigParser::_writeInterfaces(FILE* output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_INTERFACE);
  for (list<pair<string, string> >::iterator i = _networkList.begin();
       i != _networkList.end(); i++){
    // so far, the software only supports socket communications
    data->dataInString(_l.AR_INTERFACE_TYPE, "IP");
    data->dataInString(_l.AR_INTERFACE_NAME, i->first);
    data->dataInString(_l.AR_INTERFACE_ADDRESS, i->second);
    data->print(output);
  }
  _fileParser->recycle(data);
  return true;
}

bool arPhleetConfigParser::_writePorts(FILE*output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_PORTS);
  data->dataIn(_l.AR_PORTS_FIRST, &_firstPort, AR_INT, 1);
  data->dataIn(_l.AR_PORTS_SIZE, &_blockSize, AR_INT, 1);
  data->print(output);
  _fileParser->recycle(data);
  return true;
}
