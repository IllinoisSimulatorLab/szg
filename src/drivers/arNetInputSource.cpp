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
    ar_log_warning() << getLabel() << " failed to deconfigure source.\n";
}

void ar_netInputSourceConnectionTask(void* p){
  ((arNetInputSource*)p)->_connectionTask();
}

void arNetInputSource::_connectionTask() {
  // todo: designate a particular network.
  // todo: use a service name specific to a virtual computer, or specific to a user.
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_szgClient->createComplexServiceName(buffer));
  const arSlashString networks(_szgClient->getNetworks("input"));

  arSleepBackoff a(50, 3000, 1.5);
  while (true){
    ar_log_debug() << getLabel() << " discovering service '" << 
      serviceName << "' on network '" << networks << "'.\n";
    // Ask szgserver for IP:port of service "SZG_INPUT0".
    // If the service doesn't exist, this call blocks until said server starts.
    const arPhleetAddress netAddress = _szgClient->discoverService(serviceName, networks, true);
    if (!netAddress.valid){
      if (netAddress.address == "standalone") {
        // arSZGClient::discoverService hardcodes "standalone"
        ar_log_error() << getLabel() << ": no szgserver.\n";
        _closeConnection();
        break;
      }
      ar_log_warning() << getLabel() << ": no service '"
        << serviceName << "' on network '" << networks << "'.\n";
      // Throttle, since service won't reappear that quickly,
      // and szgserver itself may be stopping.
      a.sleep();
      continue;
    }
    a.reset();

    // This service has exactly one port.
    const int port = netAddress.portIDs[0];
    const string& IP = netAddress.address;
    ar_log_debug() << getLabel() << " connecting to " <<
      serviceName << " on slot " << _slot << " at " << IP << ":" << port << ".\n";
    if (!_dataClient.dialUpFallThrough(IP, port)){
      ar_log_warning() << getLabel() << " reconnecting to " <<
	serviceName << " on slot " << _slot << " at " << IP << ":" << port << ".\n";
      continue;
    }

    ar_log_remark() << getLabel() << " connected to "
      << serviceName << " on slot " << _slot << " at " << IP << ":" << port << ".\n";
    _connected = true;

    _dataTask();

    ar_log_remark() << getLabel() << " disconnected from "
      << serviceName << " on slot " << _slot << " at " << IP << ":" << port << ".\n";

    _closeConnection();
  }
}

const int bufsizeStart = 500;

arNetInputSource::arNetInputSource() :
  _szgClient(NULL),
  _dataBuffer(new ARchar[bufsizeStart]),
  _dataBufferSize(bufsizeStart),
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
    ar_log_warning() << getLabel() << " ignoring negative slot.\n";
    return false;
  }
  _slot = slot;
  ar_log_debug() << getLabel() << " using slot " << _slot << ".\n";
  return true;
}

bool arNetInputSource::init(arSZGClient& SZGClient){
  _setDeviceElements(0,0,0); // Nothing's attached yet.

  _szgClient = &SZGClient;
  ar_log_remark() << getLabel() << " inited.\n";
  return true;
}

bool arNetInputSource::start(){
  if (!_szgClient){
    ar_log_warning() << getLabel() << " ignoring start before init.\n";
    return false;
  }

  arThread dummy(ar_netInputSourceConnectionTask, this);
  ar_log_remark() << getLabel() << " started.\n";
  return true;
}

void arNetInputSource::_closeConnection(){
  // should probably kill some threads here, close sockets, etc.
  _connected = false;
  _sigOK = false;
}
