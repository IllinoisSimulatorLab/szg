//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPhleetConfig.h"
#include "arDataUtilities.h" // for ar_getUser()
#include "arSZGClient.h"
#include "arLogStream.h"

#include <sys/types.h>     // for chmod and stat
#include <sys/stat.h>      // for chmod and stat
#ifdef AR_USE_WIN_32
#include <io.h>            // for chmod
#include <direct.h>        // for mkdir
#endif

// todo: use initializers
arPhleetConfig::arPhleetConfig(){
  _computerName = string("NULL");
  _numberInterfaces = 0;
  _numNetworks = 0;
  // _firstPort and _blockSize are reasonable for Linux and Win32.
  _firstPort = 4700;
  _blockSize = 200;
  _userName = string("NULL");
  _serverIP = string("NULL");
  _serverPort = 0;
  _serverName = string("NULL");
  _fileParser = new arStructuredDataParser(_l.getDictionary());
}

// \todo Unify the callers' error messages, they differ wildly at the moment.
// Parse the config file into internal storage, returning 
// true iff successful. Note the logic about which config file to parse.
bool arPhleetConfig::read(){
  if (!_determineFileLocations())
    return false;

  arFileTextStream configStream;
  if (!configStream.ar_open(_configFileLocation.c_str())){
    return false;
  }

  // clear internal storage
  _networkList.clear();

  // sadly, this is actually normal... 
  // Bug: arStructuredDataParser should distinguish between end-of-file and error.
  arStructuredData* data = NULL;
  while ((data = _fileParser->parse(&configStream)) != NULL){
    const int ID = data->getID();
    if (ID == _l.AR_COMPUTER)
      _processComputerRecord(data);
    else if (ID == _l.AR_INTERFACE)
      _processInterfaceRecord(data);
    else if (ID == _l.AR_PORTS)
      _processPortsRecord(data);
    else
      ar_log_error() << "arPhleetConfig ignoring unexpected ID " << ID <<
        " in config file '" << _configFileLocation << "'.\n";
  }
  configStream.ar_close();

  return true;
}

// Write info to the appropriate config
// file location (depending on the alternative config vs. actual config)
bool arPhleetConfig::write(){
  if (!_determineFileLocations()) {
    ar_log_error() << "failed to write config file.\n";
    return false;
  }

  FILE* config = fopen(_configFileLocation.c_str(), "w");
  if (!config){
    ar_log_error() << "failed to write config file '" << _configFileLocation << "'.\n";
    return false;
  }
  
  _writeName(config);
  _writeInterfaces(config);
  _writePorts(config);

  // Unlike writeLogin(), make the config file world-readwriteable.
#ifdef AR_USE_WIN_32
  _chmod(_configFileLocation.c_str(), 00666);
#else
  chmod(_configFileLocation.c_str(), 00666);
#endif
  fclose(config);
  return true;
}

// Try to read in the login-file szg_<user name>.conf.
// It may not be an error if this file does not exist on a
// particular computer (the user might not have logged in yet).
bool arPhleetConfig::readLogin(bool fFromInit){
  if (!_determineFileLocations())
    return false;

  string fileName(_loginPreamble + ar_getUser() + string(".conf"));
  arFileTextStream loginStream;
  if (!loginStream.ar_open(fileName)){
    // Caller's responsible for printing warnings.
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

  if (fFromInit)
    return true;

  if (_serverName != "NULL" && _serverIP != "NULL" && _serverPort != 0) {
    // We're not called from arSZGClient::init, not already connecting to the szgserver.
    // Temporarily connect to _serverIP:_serverPort, to verify the szgserver's name.
    arSZGClient szgClient;
    szgClient.setServerLocation(_serverIP, _serverPort);
    if (szgClient._dialUpFallThrough()) {
      const string n = szgClient.getServerName();
      if (n != _serverName) {
        ar_log_critical() << "expected szgserver named '" << _serverName <<
    "', not '" << n << "', at " << _serverIP << ":" << _serverPort << ".\n";
      }
    }
    // szgClient disconnects (ungracefully) in its destructor.
  }
  return true;
}

// Write the login file, szg_<user name>.conf.
bool arPhleetConfig::writeLogin(){
  if (!_determineFileLocations())
    return false;

  const string fileName = _loginPreamble + ar_getUser() + string(".conf");
  FILE* login = fopen(fileName.c_str(),"w");
  if (!login){
    ar_log_error() << "failed to write login file '" << fileName << "'.\n";
    return false;
  }

  arStructuredData* data = _fileParser->getStorage(_l.AR_LOGIN);
  data->dataInString(_l.AR_LOGIN_USER, _userName);
  data->dataInString(_l.AR_LOGIN_SERVER_NAME, _serverName);
  data->dataInString(_l.AR_LOGIN_SERVER_IP, _serverIP);
  data->dataIn(_l.AR_LOGIN_SERVER_PORT, &_serverPort, AR_INT, 1);
  data->print(login);
  fclose(login);

  // Unlike write(), don't make the login file world-readable.
  return true;
}

// Print human-readable config file information
bool arPhleetConfig::print() const {
  cout << "computer = " << _computerName << "\n";
  for (iNetConst i = _networkList.begin(); i != _networkList.end(); ++i){
    cout << "network  = " << i->first << ", " << i->second.address 
	 << "/" << i->second.mask  << "\n";
  }
  cout << "ports    = " << _firstPort << "-" << _firstPort+_blockSize-1 << "\n";
  return true; // convenience for caller
}

// Print human-readable login information
bool arPhleetConfig::printLogin() const {
  cout << "OS user     = " << ar_getUser() << "\n"
       << "syzygy user = " << _userName << "\n"
       << "szgserver   = " << _serverName << ", " << _serverIP << ":" << _serverPort << "\n";
  return true; // convenience for caller
}

// Returns slash-delimited addresses (as defined in the config file) of
// interfaces used by this computer, or empty string if there are none.
arSlashString arPhleetConfig::getAddresses() const {
  arSlashString result;
  for (iNetConst i =_networkList.begin(); i != _networkList.end(); ++i){
    result /= i->second.address;
  }
  return result;
}

// Return the networks to which the host is connected, or ""
// if the config files says the host is not networked.
// Connection brokering uses this.  Non-uniqueness of network names is
// ok, since, networks, addresses, and masks
// form slices of a given "interface" structure.
arSlashString arPhleetConfig::getNetworks() const {
  arSlashString result;
  for (iNetConst i =_networkList.begin(); i != _networkList.end(); ++i){
    result /= i->first;
  }
  return result;
}

// Each interface has a netmask associated with it. This need not be
// specified in the configuration (it is given a default value of
// 255.255.255.0). This function returns a slash string, with the
// masks given in order of the network names.
//
// This is IP protocol specific and, in some ways, goes against the
// idea that some of these interfaces might be of a different sort entirely.
arSlashString arPhleetConfig::getMasks() const {
  arSlashString result;
  for (iNetConst i =_networkList.begin(); i != _networkList.end(); ++i){
    result /= i->second.mask;
  }
  return result;
}

string arPhleetConfig::getBroadcast(const string& mask, const string& address) const {
  arSocketAddress tmp;
  if (!tmp.setAddress(address.c_str(), 0)) {
    ar_log_error() << "config file has illegal address '" << address << "'.\n";
    return "NULL";
  }
  const string broadcast(tmp.broadcastAddress(mask.c_str()));
  if (broadcast == "NULL") {
    ar_log_error() << "config file has illegal mask '" <<
      mask << "' for address '" << address << "'.\n";
  }
  return broadcast;
}

string arPhleetConfig::getBroadcast(const int index) const {
  
  if (index >= _numNetworks) {
    ar_log_error() << "config file specifies only " << _numNetworks <<
      " networks, not " << index << ".\n";
    return "NULL";
  }

  iNetConst i=_networkList.begin();
  for (int k=0; k<index; ++k, ++i)
    ;
  return getBroadcast(i->second.mask, i->second.address);
}


// Set the computer name internally (you have to invoke write
// explicitly to write this change out to disk).
void arPhleetConfig::setComputerName(const string& name){
  _computerName = name;
}

// Add a new interface internally (call write()
// explicitly to save this change to disk). If the interface does
// not exist, add a pair to the list;  otherwise, alter the address.
void arPhleetConfig::addInterface(const string& networkName,
					const string& address,
                                        const string& netmask){
  bool result = true; // don't know if it is a duplicate yet
  for (list<pair<string, arInterfaceDescription> >::iterator i = _networkList.begin();
       i != _networkList.end(); i++){
    if (networkName == i->first){
      result = false;
      i->second.address = address;
      i->second.mask = netmask;
      break; // no need to search further
    }
  }
  if (result){
    // Found no match.
    _networkList.push_back( pair<string,arInterfaceDescription>(networkName,
      arInterfaceDescription(address, netmask)));
    // This is the only place _networkList grows.
    ++_numNetworks;
  }
}

// Removes an interface internally (you have to invoke write
// explicitly to write this change out to disk). Returns false if the
// network name/ address does not describe an existing interface and
// returns true otherwise.
bool arPhleetConfig::deleteInterface(const string& networkName,
					   const string& address){
  for (iNet i = _networkList.begin(); i != _networkList.end(); ++i){
    if (i->first == networkName && i->second.address == address){
      _networkList.erase(i);
      return true; // no need to search further
    }
  }
  ar_log_error() << "failed to delete missing interface "
                 << networkName  << "/" << address << ".\n";
  return false;
}

// Sets the internal port block (you have to invoke write
// explicitly to write this change out to disk). Returns false if
// the port block description is nonsensical somehow (for instance
// the block size is less than 1) and returns true otherwise.
bool arPhleetConfig::setPortBlock(int firstPort, int blockSize){
  if (firstPort < 1024) {
    ar_log_error() << "arPhleetConfig: port-block starts below 1024.\n";
    return false;
  }
  if (blockSize < 1){
    ar_log_error() << "arPhleetConfig: less than 1 port in block, " << blockSize << ".\n";
    return false;
  }
  _firstPort = firstPort;
  _blockSize = blockSize;
  return true;
}

// Sets the internal user name
void arPhleetConfig::setUserName(string name){
  _userName = name;
}

// Sets the internal szgserver name
void arPhleetConfig::setServerName(string name){
  _serverName = name;
}

// Sets the internal szgserver IP
void arPhleetConfig::setServerIP(string IP){
  _serverIP = IP;
}

// Sets the internal szgserver port
void arPhleetConfig::setServerPort(int port){
  _serverPort = port;
}

// If the directory exists, return true. If it does not exist, try to
// create it, returning true on success and false on failure.
bool arPhleetConfig::_createIfDoesNotExist(const string& directory){
#ifdef AR_USE_WIN_32
  struct _stat statbuf;
  if (_stat(directory.c_str(), &statbuf) < 0){
    // Not found.
    if (_mkdir(directory.c_str()) < 0){
      return false;
    }
    // AARGH! This does't seem to work as expected! 
    // Specifically, the hope is that any user will be able to change the 
    // szg.conf file. Unfortunately, Windows file permissions are not really 
    // accessed this way (i.e. group and all don't seem to work). The upshot 
    // is that only an administrator or the original creator of the
    // szg.conf file can change it after it has been created. Maybe that's a 
    // good thing...
    _chmod(directory.c_str(), 00777);
  }
  return true;
#else
  struct stat statbuf;
  if (stat(directory.c_str(), &statbuf) < 0){
    // Not found.
    if (mkdir(directory.c_str(), 00777) < 0){
      return false;
    }
    // Anybody should be able to modify a file in this directory.
    chmod(directory.c_str(), 00777);
  }
  return true;
#endif
}

// Find the config file and login preamble.
bool arPhleetConfig::_determineFileLocations(){
  return _findDir("SZG_CONF",  "/etc", "config file", "szg.conf", _configFileLocation)
      && _findDir("SZG_LOGIN", "/tmp", "login",       "szg_", _loginPreamble);
}

bool arPhleetConfig::_findDir(const char* envvar, const char* fallback, const char* name, const string& suffix, string& result) {
  string sz(ar_getenv(envvar));
  if (sz == "NULL"){
    // Fall back to the default.
#ifdef AR_USE_WIN_32
    // No trailing slash.
    fallback = "c:\\szg";
#endif
    sz = fallback;
  }
  if (!_createIfDoesNotExist(sz)){
    ar_log_error() << "failed to create " << name << " directory '" << sz << "'.\n";
    return false;
  }

  ar_pathAddSlash(sz);
  ar_scrubPath(sz);
  result = sz + suffix;
  return true;
}

void arPhleetConfig::_processComputerRecord(arStructuredData* data){
  setComputerName(data->getDataString(_l.AR_COMPUTER_NAME));
}

void arPhleetConfig::_processInterfaceRecord(arStructuredData* data){
  string netmask = data->getDataString(_l.AR_INTERFACE_MASK);
  if (netmask == ""){
    // Default, if not in config file.
    netmask = "255.255.255.0";
  }
  addInterface(data->getDataString(_l.AR_INTERFACE_NAME),
	       data->getDataString(_l.AR_INTERFACE_ADDRESS),
               netmask);
}

void arPhleetConfig::_processPortsRecord(arStructuredData* data){
  setPortBlock(data->getDataInt(_l.AR_PORTS_FIRST),
	       data->getDataInt(_l.AR_PORTS_SIZE));
}

bool arPhleetConfig::_writeName(FILE* output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_COMPUTER);
  data->dataInString(_l.AR_COMPUTER_NAME, _computerName);
  data->print(output);
  _fileParser->recycle(data);
  return true;
}

bool arPhleetConfig::_writeInterfaces(FILE* output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_INTERFACE);
  for (iNetConst i = _networkList.begin(); i != _networkList.end(); ++i){
    // so far, the software only supports socket communications
    data->dataInString(_l.AR_INTERFACE_TYPE, "IP");
    data->dataInString(_l.AR_INTERFACE_NAME, i->first);
    data->dataInString(_l.AR_INTERFACE_ADDRESS, i->second.address);
    data->dataInString(_l.AR_INTERFACE_MASK, i->second.mask);
    data->print(output);
  }
  _fileParser->recycle(data);
  return true;
}

bool arPhleetConfig::_writePorts(FILE*output){
  arStructuredData* data = _fileParser->getStorage(_l.AR_PORTS);
  data->dataIn(_l.AR_PORTS_FIRST, &_firstPort, AR_INT, 1);
  data->dataIn(_l.AR_PORTS_SIZE, &_blockSize, AR_INT, 1);
  data->print(output);
  _fileParser->recycle(data);
  return true;
}
