//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arNetInputSource.h"
#include "arPhleetConfigParser.h"
#include "arLogStream.h"

void ar_netInputSourceDataTask(void* parameter){
  ((arNetInputSource*)parameter)->_dataTask();
}

void arNetInputSource::_dataTask(){
  while (_dataClient.getData(_dataBuffer, _dataBufferSize)) {
    _data->unpack(_dataBuffer);
    ARint sig[3];
    _data->dataOut(_inp._SIGNATURE, sig, AR_INT, 3);
    if (!_clientInitialized){
      _setDeviceElements(sig[0], sig[1], sig[2]);
      if (!_reconfig())
        ar_log_error() << "arNetInputSource warning: failed to reconfigure source (#1).\n";
      _clientInitialized = true;
    }
    else{
      // has the server has been reconfigured?
      // this can happen after the connection has occured, for instance
      // with an server that is the composite of several devices
      if (sig[0] != _numberButtons ||
          sig[1] != _numberAxes ||
          sig[2] != _numberMatrices){
        _setDeviceElements(sig[0], sig[1], sig[2]);
        if (!_reconfig())
	  ar_log_warning() << "arNetInputSource warning: failed to reconfigure source (#2).\n";
      }
    }
    // relay the data to the input sink
    _sendData();
  }
  _clientConnected = false; // we've lost our connection
  _setDeviceElements(0,0,0);
  if (!_reconfig())
    ar_log_warning() << "arNetInputSource warning: failed to reconfigure source (#3).\n";
}

void ar_netInputSourceConnectionTask(void* inputClient){
  ((arNetInputSource*)inputClient)->_connectionTask();
}

void arNetInputSource::_connectionTask() {
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // there are many problems here. how do we designate a particular network for
  // communications... how do we use a virtual computer specific or user-specific
  // service name 
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_client->createComplexServiceName(buffer));
  const arSlashString networks(_client->getNetworks("input"));
  ar_log_debug() << "arNetInputSource serviceName '" << serviceName <<
    "', networks '" << networks << "'\n";

  while (true){
    ar_log_debug() << "arNetInputSource discovering service...\n";
    arPhleetAddress result =
      _client->discoverService(serviceName, networks, true);
    if (!result.valid){
      ar_log_warning() << "arNetInputSource warning: no service '" <<
	serviceName << "' on network '" << networks << "'.\n";
      continue;
    }
    // This service has exactly one port.
    ar_log_debug() << "arNetInputSource connecting...\n";
    if (!_dataClient.dialUpFallThrough(result.address, result.portIDs[0])){
      ar_log_warning() << "arNetInputSource reconnecting to service '"
	               << serviceName << "' at "
	               << result.address << ":" << result.portIDs[0] << ".\n";
      continue;
    }
    ar_log_remark() << "arNetInputSource connected to service " <<
      serviceName << " at " << result.address << ":" << result.portIDs[0] << ".\n";
    _clientConnected = true;
    ar_usleep(100000);
    arThread dummy(ar_netInputSourceDataTask, this);
    while (connected())
      ar_usleep(200000);
    ar_log_remark() << "arNetInputSource disconnected from service " <<
      serviceName << " at " << result.address << ":" << result.portIDs[0] << ".\n";
    _closeConnection();
  }
}

/// \todo initializers not assignments.
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

/// Input devices offer services based on slots. 
/// Slot 0 corresponds to service SZG_INPUT0, slot 1 to SZG_INPUT1, etc.
/// @param slot the slot in question
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

  // this does not do much now that we are relying on 
  // connection brokering instead of
  // IP/port combos coded in the phleet database... 
  // annoyingly, the arSZGClient must
  // be saved for future use in connection brokering.
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
