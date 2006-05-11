//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONFIG_PARSER_H
#define AR_PHLEET_CONFIG_PARSER_H

#include "arPhleetConfigLanguage.h"
#include "arStructuredDataParser.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arPhleetCalling.h"
#include <stdio.h>
#include <list>
#include <string>
using namespace std;

class arInterfaceDescription{
 public:
  arInterfaceDescription(){}
  ~arInterfaceDescription(){}

  string address;
  string mask;
};

/// Used for parsing/storing/writing the szg.conf config file and
/// the szg_<username>.conf login files
class SZG_CALL arPhleetConfigParser{
 public:
  arPhleetConfigParser();
  ~arPhleetConfigParser() {}

  // bulk manipulation of the config file
  bool parseConfigFile();
  bool writeConfigFile();

  // bulk manipulation of user's login file
  bool parseLoginFile();
  bool writeLoginFile();

  // output configuration
  void printConfig();
  void printLogin();

  // get global config info
  /// computer name, as parsed from the config file
  string getComputerName() const
    { return _computerName; }
  /// first port in block used for connection brokering, default 4700.
  int getFirstPort() const
    { return _firstPort; }
  /// size of block of ports, default 200.
  int getPortBlockSize() const
    { return _blockSize; }
  /// number of interfaces in the config.
  int getNumberInterfaces() const
    { return _numberInterfaces; }
  arSlashString getAddresses();
  arSlashString getNetworks();
  arSlashString getMasks();

  // get login info
  string getUserName() const ///< syzygy username
    { return _userName; }
  string getServerName() const ///< hostname of szgserver
    { return _serverName; }
  string getServerIP() const ///< IP address of szgserver
    { return _serverIP; }
  int getServerPort() const ///< port of szgserver
    { return _serverPort; }
  

  // manipulating the global config. using these methods,
  // command-line wrappers can be used to build the config file
  void   setComputerName(const string& name);
  void   addInterface(const string& networkName, 
                      const string& address,
                      const string& netmask);
  bool   deleteInterface(const string& networkName, const string& address);
  bool   setPortBlock(int firstPort, int blockSize);
  // manipulating the individual login. using these methods,
  // command-line wrappers can be used to manipulate the user's
  // login file
  void   setUserName(string name);
  void   setServerName(string name);
  void   setServerIP(string IP);
  void   setServerPort(int port);

 private:
  arPhleetConfigLanguage           _l;
  // config file locations
  string                           _configFileLocation;
  string                           _loginPreamble;
  // global computer info
  string                           _computerName;
  int                              _numberInterfaces;
  list<pair<string, arInterfaceDescription> >      _networkList;
  int                              _firstPort;
  int                              _blockSize;
  // user login info
  string                           _userName;
  string                           _serverIP;
  int                              _serverPort;
  string                           _serverName;
  // actually parse the config/login files
  arStructuredDataParser*          _fileParser;

  void _processComputerRecord(arStructuredData*);
  void _processInterfaceRecord(arStructuredData*);
  void _processPortsRecord(arStructuredData*);

  bool _createIfDoesNotExist(const string& directory);
  bool _determineFileLocations();
  bool _findDir(const char*, const char*, const char*, const string&, string&);
  bool _writeName(FILE*);
  bool _writeInterfaces(FILE*);
  bool _writePorts(FILE*);
};

#endif
