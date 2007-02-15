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

    if (!_sigOK ||
        sig[0] != _numberButtons ||
        sig[1] != _numberAxes ||
        sig[2] != _numberMatrices) {
      _setDeviceElements(sig);
      _sigOK = _reconfig();
      if (!_sigOK)
	ar_log_warning() << "arNetInputSource failed to configure source.\n";
    }

    // Relay the data to the input sink
    _sendData();
  }

  _connected = false;
  _setDeviceElements(0,0,0);
  _sigOK = _reconfig();
  if (!_sigOK)
    ar_log_warning() << "arNetInputSource failed to deconfigure source.\n";
}

void ar_netInputSourceConnectionTask(void* inputClient){
  ((arNetInputSource*)inputClient)->_connectionTask();
}

void arNetInputSource::_connectionTask() {
  // todo: designate a particular network.
  // todo: use a virtual computer specific or user-specific service name.
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_szgClient->createComplexServiceName(buffer));
  const arSlashString networks(_szgClient->getNetworks("input"));
  ar_log_debug() << "arNetInputSource serviceName '" << serviceName <<
    "', networks '" << networks << "'\n";

  while (true){
    ar_log_debug() << "arNetInputSource discovering service...\n";
    // Ask szgserver for IP:port of service "SZG_INPUT0".
    const arPhleetAddress IPport = _szgClient->discoverService(serviceName, networks, true);
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
    _connected = true;

    _dataTask();

    ar_log_remark() << "arNetInputSource disconnected from " <<
      serviceName << " at " << IP << ":" << port << ".\n";
    _closeConnection();
  }
}

arNetInputSource::arNetInputSource() :
  _szgClient(NULL),
  _dataBuffer(new ARchar[500]),
  _dataBufferSize(500),
  _slot(0),
  _interface("NULL"),
  _port(0),
  _connected(false),
  _sigOK(false)
{
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
  _szgClient = &SZGClient;
  ar_log_remark() << "arNetInputSource initialized.\n";
  return true;
}

bool arNetInputSource::start(){
  if (!_szgClient){
    ar_log_warning() << "arNetInputSource ignoring start before init.\n";
    return false;
  }
  arThread dummy(ar_netInputSourceConnectionTask, this);
  ar_log_remark() << "arNetInputSource started.\n";
  return true;
}

void arNetInputSource::_closeConnection(){
  // should probably kill some threads here, close sockets, etc.
  _connected = false;
  _sigOK = false;
}
