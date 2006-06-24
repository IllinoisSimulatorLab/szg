//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPhleetConfigLanguage.h"

arPhleetConfigLanguage::arPhleetConfigLanguage():
  _computer("computer"),
  _interface("interface"),
  _ports("ports"),
  _login("login"){
  
  AR_COMPUTER_NAME = _computer.add("name", AR_CHAR);
  AR_COMPUTER = _dictionary.add(&_computer);

  AR_INTERFACE_TYPE = _interface.add("type", AR_CHAR);
  AR_INTERFACE_NAME = _interface.add("name", AR_CHAR);
  AR_INTERFACE_ADDRESS = _interface.add("address", AR_CHAR);
  AR_INTERFACE_MASK = _interface.add("mask", AR_CHAR);
  AR_INTERFACE = _dictionary.add(&_interface);

  AR_PORTS_FIRST = _ports.add("first", AR_INT);
  AR_PORTS_SIZE = _ports.add("size", AR_INT);
  AR_PORTS = _dictionary.add(&_ports);

  AR_LOGIN_USER = _login.add("user", AR_CHAR);
  AR_LOGIN_SERVER_NAME = _login.add("server_name", AR_CHAR);
  AR_LOGIN_SERVER_IP = _login.add("server_IP", AR_CHAR);
  AR_LOGIN_SERVER_PORT = _login.add("server_port", AR_INT);
  AR_LOGIN = _dictionary.add(&_login);
}
