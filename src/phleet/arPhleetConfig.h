//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONFIG_PARSER_H
#define AR_PHLEET_CONFIG_PARSER_H

#include "arPhleetConfigLanguage.h"
#include "arStructuredDataParser.h"
#include "arPhleetCalling.h"

#include <stdio.h>
#include <list>
#include <string>
using namespace std;

class arInterfaceDescription {
 public:
  arInterfaceDescription() {}
  arInterfaceDescription(const string& a, const string& m) :
    address(a), mask(m) {}
  ~arInterfaceDescription() {}

  string address;
  string mask;
};

// Used for parsing/storing/writing the szg.conf config file and
// the szg_<username>.conf login files
class SZG_CALL arPhleetConfig {
 public:
  arPhleetConfig();
  ~arPhleetConfig() {}

  bool determineFileLocations( string& configLocation, string& loginFileLocation );

  // Config file I/O.
  bool read();
  bool write();
  bool print() const;

  // Login file I/O.
  bool readLogin(bool fFromInit = false);
  bool writeLogin();
  bool printLogin() const;

  // Get global config info.
  // computer name, as parsed from the config file
  string getComputerName() const
    { return _computerName; }
  // first port in block used for connection brokering, default 4700.
  int getFirstPort() const
    { return _firstPort; }
  // size of block of ports, default 200.
  int getPortBlockSize() const
    { return _blockSize; }
  // number of networks in the config.
  int getNumNetworks() const
    { return _numNetworks; }
  arSlashString getAddresses() const;
  arSlashString getNetworks() const;
  arSlashString getMasks() const;
  string getBroadcast(const string& mask, const string& address) const;
  string getBroadcast(const int i) const;

  // get login info
  string getUserName() const // syzygy username
    { return _userName; }
  string getServerName() const // hostname of szgserver
    { return _serverName; }
  string getServerIP() const // IP address of szgserver
    { return _serverIP; }
  int getServerPort() const // port of szgserver
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
  string                           _loginFileLocation;
  // global computer info
  string                           _computerName;
  list<pair<string, arInterfaceDescription> >      _networkList;
  int                              _numNetworks; // == size of _networkList
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
  bool _findDir(const char*, const char*, const char*, const string&, string&);
  bool _writeName(FILE*);
  bool _writeInterfaces(FILE*);
  bool _writePorts(FILE*);

  typedef std::list<pair<string, arInterfaceDescription> >::iterator iNet;
  typedef std::list<pair<string, arInterfaceDescription> >::const_iterator iNetConst;
};

#endif
