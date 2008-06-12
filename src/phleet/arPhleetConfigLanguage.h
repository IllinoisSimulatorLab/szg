//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_PHLEET_CONFIG_LANGUAGE_H
#define AR_PHLEET_CONFIG_LANGUAGE_H

#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arDatabaseLanguage.h"
#include "arPhleetCalling.h"

// The language used in the szg.conf file.
class SZG_CALL arPhleetConfigLanguage: public arLanguage{
 public:
  arPhleetConfigLanguage();
  ~arPhleetConfigLanguage() {}

  // these integers are used for fast access to the various data fields
  int AR_COMPUTER;       // used to set the computer name
  int AR_COMPUTER_NAME;

  int AR_INTERFACE;      // used to describe a communications interface attached
                         // to the computer
  int AR_INTERFACE_TYPE;     // is this a network interface or some other sort?
  int AR_INTERFACE_NAME;     // often, networks need a human-readbale name
  int AR_INTERFACE_ADDRESS;  // the actual address, like 192.168.0.1
  int AR_INTERFACE_MASK;     // the netmask, with default = 255.255.255.0

  int AR_PORTS;          // used to describe the block of ports used on this
                         // computer for connection brokering
  int AR_PORTS_FIRST;      // the first port in the block
  int AR_PORTS_SIZE;       // the number of ports in the block

  int AR_LOGIN;          // used to describe the login data record
  int AR_LOGIN_USER;         // the user's name
  int AR_LOGIN_SERVER_NAME;  // the name of the szgserver to which the user is
                             // logged-in
  int AR_LOGIN_SERVER_IP;    // the IP address of the szgserver
  int AR_LOGIN_SERVER_PORT;  // the port of the szgserver

 private:
  arDataTemplate _computer;
  arDataTemplate _interface;
  arDataTemplate _ports;
  arDataTemplate _login;
};

#endif
