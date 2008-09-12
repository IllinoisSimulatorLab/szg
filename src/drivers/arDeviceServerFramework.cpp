//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDeviceServerFramework.h"

arDeviceServerFramework::arDeviceServerFramework() :
  _name("DeviceServer"),
  _deviceName("NULL"),
  _netOutputSlot(0),
  _nextNetInputSlot(1),
  _szgClient(),
  _inputNode(),
  _netSink(),
  _fileSink()
{
}

arDeviceServerFramework::~arDeviceServerFramework() {
}

bool arDeviceServerFramework::checkCmdArg( int& argc, char** argv, const char* const sz ) {
  // Start at 1 not 0: exe's own name isn't an arg.
  for (int i=1; i<argc; ++i) {
    if (!strcmp(sz, argv[i])) {
      // Shift later args over found one (from i+1 to argc-1 inclusive).
      memmove(argv+i, argv+i+1, sizeof(char**) * (argc-- -i-1));
      return true;
    }
  }
  return false;
}


bool arDeviceServerFramework::extractCmdArg( int& argc, char** argv, int i, string& val ) {
  if ((i < 0) || (i >= argc)) {
    ar_log_error() << "extractCmdArg() index out of range.\n";
    return false;
  }
  val = string( argv[i] );
  // Shift later args over found one (from i+1 to argc-1 inclusive).
  memmove(argv+i, argv+i+1, sizeof(char**) * (argc-- -i-1));
  return true;
}


bool arDeviceServerFramework::init( int& argc, char** argv, string forcedName ) {
  if (forcedName == "NULL") {
    _name = ar_stripExeName(string(argv[0]));
  } else {
    _name = forcedName;
  }
  setDeviceName( _name );
  _szgClient.simpleHandshaking(false);
  // Force the component's name because Win98 won't give the name
  // automatically.  Ascension Spacepad needs Win98.
  const bool fInit = _szgClient.init(argc, argv, forcedName);
  ar_log_debug() << _name << " Syzygy version: " << ar_versionString() << ar_endl;

  string currDir;
  if (!ar_getWorkingDirectory( currDir )) {
    ar_log_critical() << "Failed to get working directory.\n";
  } else {
    ar_log_critical() << "Directory: " << currDir << ar_endl;
  }

  if (!_szgClient.connected()) {
    ar_log_critical() << _name << " failed to init arSZGClient.\n";
    return _szgClient.failStandalone(fInit);
  }
  ar_log_debug() << _name << " inited arSZGClient.\n";

  _fNetInput = checkCmdArg( argc, argv, "-netinput" );

  if (!_handleArgs( argc, argv )) {
    ar_log_error() << _name << " failed to parse special arguments.\n";
    return false;
  }

  // by this point all args except for the app name and the output slot
  // _must_ have been removed from (argc, argv).
  if (argc != 2) {
    _printUsage();
    return false;
  }
  _netOutputSlot = atoi(argv[1]);
  _nextNetInputSlot = _netOutputSlot + 1;

  if (!_getLock()) {
    return false;
  }

  if (!_netSink.setSlot( _netOutputSlot )) {
    ar_log_critical() << "DeviceServerFramework: invalid network slot " << _netOutputSlot << ".\n";
    return false;
  }

  // Tell netInputSink how we were launched.
  _netSink.setInfo( _deviceName );
  _inputNode.addInputSink( &_netSink, false );
  _inputNode.addInputSink( &_fileSink, false );

  if (!_configureInputNode()) {
    ar_log_error() << _name << " failed to configure input node.\n";
    return false;
  }

  const bool ok = _inputNode.init( _szgClient );
  if (!ok) {
    ar_log_critical() << _name << " has no input.\n";
  }
  if (!_szgClient.sendInitResponse(ok)) {
    ar_log_error() << _name << " ignoring failed init.\n";
    cerr << _name << " error: maybe szgserver died.\n";
  }
  if (!ok) {
    // Bug: in linux, this may hang.  Which other thread still runs?
    return false; // init failed
  }
  if (!_inputNode.start()) {
    if (!_szgClient.sendStartResponse(false)) {
      cerr << _name << " error: maybe szgserver died.\n";
    }
    return false;
  }
  if (!_szgClient.sendStartResponse(true)) {
    cerr << _name << " error: maybe szgserver died.\n";
  }
  return true;
}


bool arDeviceServerFramework::_getLock() {
  // At most one DeviceServer per host.
  int ownerID = -1;
  // NOTE: I'm not positive that each one should have a unique lock. Perhaps
  // the lock should always be 'DeviceServer'?
  if (!_szgClient.getLock(_szgClient.getComputerName() + "/" + _name, ownerID)) {
    ar_log_critical() << _name << ": already running (pid = " << ownerID << ").\n";
    return false;
  }
  return true;
}

bool arDeviceServerFramework::_configureInputNode() {
  return true;
}

bool arDeviceServerFramework::_addNetInput() {
  if (_fNetInput) {
    arNetInputSource* implicitNetInputSource = new arNetInputSource();
    if (!implicitNetInputSource) {
      ar_log_error() << "arDeviceServerFramework::_addNetInput() out of memory.\n";
      return false;
    }
    if (!implicitNetInputSource->setSlot( _nextNetInputSlot )) {
      ar_log_error() << "arInputFactory: invalid slot " << _nextNetInputSlot << ".\n";
      return false;
    }
    _inputNode.addInputSource( implicitNetInputSource, true );
    ar_log_debug() << "arInputFactory added implicit arNetInputSource in slot "
                   << _nextNetInputSlot << ar_endl;
    ++_nextNetInputSlot;
  }
  return true;
}


int arDeviceServerFramework::messageLoop() {
  // Message task.
  string messageType, messageBody;
  while (true) {
    const int sendID = _szgClient.receiveMessage(&messageType, &messageBody);
    if (!sendID) {
      // Shutdown "forced."
      _inputNode.stop();
      ar_log_debug() << "shutdown.\n";
      return 0;
    }

    if (messageType=="quit") {
      ar_log_debug() << "shutting down...\n";
      _inputNode.stop();
      ar_log_debug() << "shutdown.\n";
      return 0;
    }

    else if (messageType=="restart") {
      ar_log_remark() << "DeviceServerFramework restarting.\n";
      _inputNode.restart();
    }
    else if (messageType=="dumpon") {
      _fileSink.start();
    }
    else if (messageType=="dumpoff") {
      _fileSink.stop();
    }
    else if (messageType=="log") {
      (void)ar_setLogLevel( messageBody );
    } else {
      if (!_handleMessage( messageType, messageBody )) {
        ar_log_error() << "ignoring unrecognized messageType '" << messageType << "'.\n";
      }
    }
  }

  // unreachable
  ar_log_critical() << "fell out of message loop.\n";
  return 1;
}
