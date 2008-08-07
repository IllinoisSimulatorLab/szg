//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDeviceServerFramework.h"
#include "arInputFactory.h"

class arStandardDeviceServerFramework : public arDeviceServerFramework {
 public:
  arStandardDeviceServerFramework() :
    arDeviceServerFramework(),
    _driverFactory(),
    _fSimpleConfig(false) {}
  ~arStandardDeviceServerFramework() {}

 protected:
  bool _handleArgs( int& argc, char **argv );
  void _printUsage();
  bool _configureInputNode();
  bool _handleMessage( const string& messageType, const string& messageBody );
  void _simpleConfig( arInputNodeConfig& inputConfig );
  bool _normalConfig( arInputNodeConfig& inputConfig );
  arInputFactory _driverFactory;
  bool _fSimpleConfig;
};

bool arStandardDeviceServerFramework::_handleArgs( int& argc, char ** argv ) {
  _fSimpleConfig = checkCmdArg( argc, argv, "-s" );
  return extractCmdArg( argc, argv, 1, _deviceName );
}

void arStandardDeviceServerFramework::_printUsage() {
  ar_log_error() << "usage: DeviceServer [-s] [-netinput] device_description driver_slot\n";
}

bool arStandardDeviceServerFramework::_configureInputNode() {
  arInputNodeConfig inputConfig;
  if (_fSimpleConfig) {
    _simpleConfig( inputConfig );
  } else if (!_normalConfig( inputConfig )) {
    return false;
  }
  _driverFactory.setInputNodeConfig( inputConfig );
  if (!_driverFactory.configure( _szgClient )) {
    ar_log_critical() << "DeviceServer failed to configure arInputFactory.\n";
    return false;
  }
  if (!_driverFactory.loadInputSources( _inputNode, _nextNetInputSlot, _fNetInput )) {
    ar_log_critical() << "DeviceServer failed to load input drivers.\n";
    return false;
  }
  if (!_driverFactory.loadInputSinks( _inputNode )) {
    ar_log_critical() << "DeviceServer failed to load input sinks.\n";
    return false;
  }
  if (!_driverFactory.loadFilters( _inputNode, "" )) {
    ar_log_error() << "DeviceServer failed to load filters.\n";
    return false;
  }

  return true;
}

bool arStandardDeviceServerFramework::_handleMessage( const string& messageType, const string& messageBody ) {
  arInputSource* driver = _driverFactory.findInputSource( messageType );
  if (driver) {
    ar_log_remark() << "handling message " << messageType << "/" << messageBody << ".\n";
    driver->handleMessage( messageType, messageBody );
    return true;
  }
  return false;
}

void arStandardDeviceServerFramework::_simpleConfig( arInputNodeConfig& inputConfig ) {
  inputConfig.addInputSource( _deviceName );
}

bool arStandardDeviceServerFramework::_normalConfig( arInputNodeConfig& inputConfig ) {
  const string& config = _szgClient.getGlobalAttribute( _deviceName );
  if (config == "NULL") {
    ar_log_critical() << _name << ": no global parameter (<param> in dbatch file) '"
                   << _deviceName << "'.\n";
    return false;
  }
  if (!inputConfig.parseXMLRecord( config )) {
    ar_log_critical() << _name << ": misconfigured global parameter (<param> in dbatch file) '"
                   << _deviceName << "'\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  ar_log_critical() << "DeviceServer Syzygy version: " << ar_versionString() << ar_endl;

  string currDir;
  if (!ar_getWorkingDirectory( currDir )) {
    ar_log_critical() << "Failed to get working directory.\n";
  } else {
    ar_log_critical() << "Directory: " << currDir << ar_endl;
  }

  arStandardDeviceServerFramework fw;
  if (!fw.init( argc, argv, "DeviceServer" )) {
    ar_log_error() << "DeviceServer init failed.\n";
    if (!fw.getSZGClient()->sendInitResponse(false)) {
      cerr << "DeviceServer error: maybe szgserver died.\n";
    }
    return 1;
  }
  return fw.messageLoop();
}
