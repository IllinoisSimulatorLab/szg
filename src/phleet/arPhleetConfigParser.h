//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONFIG_PARSER_H
#define AR_PHLEET_CONFIG_PARSER_H

#include "arPhleetConfigLanguage.h"
#include "arStructuredDataParser.h"
#include <stdio.h>
#include <list>
#include <string>
using namespace std;

/// Used for parsing/storing/writing the szg.conf config file and
/// the szg_<username>.conf login files
class arPhleetConfigParser{
 public:
  arPhleetConfigParser();
  ~arPhleetConfigParser() {}

  void useAlternativeConfigFile(bool flag);

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
  arSlashString getAddresses(const string& networkName = "");
  arSlashString getNetworks();

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
  void   addInterface(const string& networkName, const string& address);
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
  bool                             _alternativeConfig;
  // global computer info
  string                           _computerName;
  int                              _numberInterfaces;
  list<pair<string, string> >      _networkList;
  int                              _firstPort;
  int                              _blockSize;
  // user login info
  string                           _userName;
  string                           _serverIP;
  int                              _serverPort;
  string                           _serverName;
  // this is what actually parses the config/login files
  arStructuredDataParser*          _fileParser;

  void _processComputerRecord(arStructuredData*);
  void _processInterfaceRecord(arStructuredData*);
  void _processPortsRecord(arStructuredData*);

  bool _sanityCheck();
  bool _writeName(FILE*);
  bool _writeInterfaces(FILE*);
  bool _writePorts(FILE*);
};

/// Scan and remove -t flag from argc/argv.
inline bool arTFlag(int& argc, char** argv) {
  bool flag = false;
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i], "-t")){
      flag = true;
      for (int j=i+1; j<argc; j++){
        argv[j-i] = argv[j];
      }
      --argc;
    }
  }
  return flag;
}

#endif
