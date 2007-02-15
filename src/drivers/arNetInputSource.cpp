//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNetInputSource.h"
#include "arLogStream.h"

// Listen for events.
void arNetInputSource::_dataTask(){
  while (_dataClient.getData(_dataBuffer, _dataBufferSize)) {
    _data->unpack(_dataBuffer);
    ARint sig[3];
    _data->dataOut(_inp._SIGNATURE, sig, AR_INT, 3);
    if (!_clientInitialized) {
      _setDeviceElements(sig);
      if (!_reconfig())
        ar_log_warning() << "arNetInputSource failed to reconfigure source (#1).\n";
      _clientInitialized = true;
    }
    else{
      if (sig[0] != _numberButtons ||
          sig[1] != _numberAxes ||
          sig[2] != _numberMatrices){
	// Server was reconfigured, perhaps after connecting to a server
	// that is a composite of several smaller input devices.
        _setDeviceElements(sig);
        if (!_reconfig())
	  ar_log_warning() << "arNetInputSource failed to reconfigure source (#2).\n";
      }
    }
    // relay the data to the input sink
    _sendData();
  }
  _clientConnected = false; // lost our connection
  _setDeviceElements(0,0,0);
  if (!_reconfig())
    ar_log_warning() << "arNetInputSource failed to reconfigure source (#3).\n";
}

void ar_netInputSourceConnectionTask(void* inputClient){
  ((arNetInputSource*)inputClient)->_connectionTask();
}

void arNetInputSource::_connectionTask() {
  // todo: designate a particular network.
  // todo: use a virtual computer specific or user-specific service name.
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_client->createComplexServiceName(buffer));
  const arSlashString networks(_client->getNetworks("input"));
  ar_log_debug() << "arNetInputSource serviceName '" << serviceName <<
    "', networks '" << networks << "'\n";

  while (true){
    ar_log_debug() << "arNetInputSource discovering service...\n";
    // Ask szgserver for IP:port of service "SZG_INPUT0".
    const arPhleetAddress IPport =
      _client->discoverService(serviceName, networks, true);
    if (!IPport.valid){
      ar_log_warning() << "arNetInputSource: no service '" <<
	serviceName << "' on network '" << networks << "'.\n";
      // Throttle, since service probably won't reappear that quickly,
      // and maybe szgserver itself is stopping.
      ar_usleep(50000); // todo: increase sleep time gradually.
      continue;
    }

    // This service has exactly one port.
    const int port = IPport.portIDs[0];
    const string& IP = IPport.address;
    ar_log_debug() << "arNetInputSource connecting...\n";
    if (!_dataClient.dialUpFallThrough(IP, port)){
      ar_log_warning() << "arNetInputSource reconnecting to '"
	               << serviceName << "' at "
	               << IP << ":" << port << ".\n";
      continue;
    }

    ar_log_remark() << "arNetInputSource connected to " <<
      serviceName << " at " << IP << ":" << port << ".\n";
    _clientConnected = true;

    _dataTask();

    ar_log_remark() << "arNetInputSource disconnected from " <<
      serviceName << " at " << IP << ":" << port << ".\n";
    _closeConnection();
  }
}

// todo: initializers not assignments.
arNetInputSource::arNetInputSource(){
  _dataBufferSize = 500;
  _slot = 0;
  _interface = string("NULL");
  _port = 0;
  _clientConnected = false;
  _clientInitialized = false;
  _client = NULL;
  _dataBuffer = new ARchar[_dataBufferSize];

  _dataClient.smallPacketOptimize(true);
}

// Input devices offer services based on slots. 
// Slot 0 corresponds to service SZG_INPUT0, slot 1 to SZG_INPUT1, etc.
// @param slot the slot in question
bool arNetInputSource::setSlot(int slot){
  if (slot<0){
    ar_log_warning() << "arNetInputSource ignoring negative input device slot.\n";
    return false;
  }
  _slot = slot;
  ar_log_debug() << "arNetInputSource slot = " << _slot << ".\n";
  return true;
}

bool arNetInputSource::init(arSZGClient& SZGClient){
  _setDeviceElements(0,0,0); // Nothing's attached yet.

  // Save arSZGClient for future connection brokering.
  _client = &SZGClient;
  ar_log_remark() << "arNetInputSource initialized.\n";
  return true;
}

bool arNetInputSource::start(){
  if (!_client){
    ar_log_warning() << "arNetInputSource ignoring start before init.\n";
    return false;
  }
  arThread dummy(ar_netInputSourceConnectionTask, this);
  ar_log_remark() << "arNetInputSource started.\n";
  return true;
}

void arNetInputSource::_closeConnection(){
  // should probably kill some threads here, close sockets, etc.
  _clientConnected = false;
  _clientInitialized = false;
}

bool arNetInputSource::connected() const {
  return _clientConnected;
}
