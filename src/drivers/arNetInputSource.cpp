//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arNetInputSource.h"
#include "arPhleetConfigParser.h"

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
        cerr << "arNetInputSource warning: failed to reconfigure source (#1).\n";
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
	  cerr << "arNetInputSource warning: failed to reconfigure source (#2).\n";
      }
    }
    // relay the data to the input sink
    _sendData();
  }
  _clientConnected = false; // we've lost our connection
  _setDeviceElements(0,0,0);
  if (!_reconfig())
    cerr << "arNetInputSource warning: failed to reconfigure source (#3).\n";
}

void ar_netInputSourceConnectionTask(void* inputClient){
  arNetInputSource* i = (arNetInputSource*) inputClient;
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // there are many problems here. how do we designate a particular network for
  // communications... how do we use a virtual computer specific or user-specific
  // service name 
  char buffer[32];
  sprintf(buffer,"SZG_INPUT%i",i->_slot);
  const string serviceName(i->_client->createComplexServiceName(buffer));
  const arSlashString networks(i->_client->getNetworks("input"));
  while (true){
    arPhleetAddress result =
      i->_client->discoverService(serviceName, networks, true);
    if (!result.valid){
      cerr << "arNetInputSource warning: no service \""
	   << serviceName << "\" on network \""
           << networks << "\".\n";
      continue;
    }
    // we know there is exactly one port for this service
    if (!i->_dataClient.dialUpFallThrough(result.address, result.portIDs[0])){
      cerr << "arNetInputSource warning: "
           << "retrying connection to service "
	   << serviceName << " at "
	   << result.address << ":" << result.portIDs[0] << ".\n";
      continue;
    }
    cout << "arNetInputSource remark: connected to service "
         << serviceName << " at "
	 << result.address << ":" << result.portIDs[0] << ".\n";
    i->_clientConnected = true;
    ar_usleep(100000);
    arThread dummy(ar_netInputSourceDataTask, i);
    while (i->_checkConnection()){
      ar_usleep(200000);
    }
    i->_closeConnection();
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

/// Input devices in phleet offer services based on slots. 
/// So... slot 0 corresponds
/// to service SZG_INPUT0, slot 1 corresponds to service SZG_INPUT1, and so on.
/// @param slot the slot in question
void arNetInputSource::setSlot(int slot){
  if (slot<0){
    cerr << "arNetInputSource warning: ignoring negative input device slot.\n";
    return;
  }
  _slot = slot;
}

bool arNetInputSource::init(arSZGClient& SZGClient){
  // no device elements, since nothing's attached yet
  _setDeviceElements(0,0,0);
  // this does not do much now that we are relying on 
  // connection brokering instead of
  // IP/port combos coded in the phleet database... 
  // annoyingly, the arSZGClient must
  // be saved for future use in connection brokering.
  _client = &SZGClient;
  _client->initResponse() << "arNetInputSource remark: initialized.\n";
  return true;
}

bool arNetInputSource::start(){
  if (!_client){
    cerr << "arNetInputSource error: start called before init.\n";
    return false;
  }
  arThread dummy(ar_netInputSourceConnectionTask, this);
  _client->startResponse() << "arNetInputSource remark: started.\n";
  return true;
}

bool arNetInputSource::stop(){
  return true;
}

bool arNetInputSource::restart(){
  return stop() && start();
}

void arNetInputSource::_closeConnection(){
  // should probably kill some threads here, close sockets, etc.
  _clientConnected = false;
  _clientInitialized = false;
}

bool arNetInputSource::_checkConnection(){
  return _clientConnected;
}
