//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNetInputSource.h"
#include "arLogStream.h"

// Listen for events.
void arNetInputSource::_dataTask() {
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
        ar_log_error() << "netinput failed to configure source.\n";
    }

    // Relay the data to the input sink
    _sendData();
  }

  _connected = false;
  _setDeviceElements(0, 0, 0);
  _sigOK = _reconfig();
  if (!_sigOK)
    ar_log_error() << "failed to deconfigure source.\n";
}

void ar_netInputSourceConnectionTask(void* p) {
  ((arNetInputSource*)p)->_connectionTask();
}

void arNetInputSource::_connectionTask() {
  // todo: designate a particular network.
  // todo: use a service name specific to a virtual computer, or specific to a user.
  string serviceName( _serviceName );
  if (serviceName == "NULL") {
    serviceName = _szgClient->createComplexServiceName("SZG_INPUT" + ar_intToString(_slot));
  }
  const arSlashString networks(_szgClient->getNetworks("input"));

  arSleepBackoff a(50, 3000, 1.5);
  while (true) {
    string svc = "service '" + serviceName +  "' on network '" + networks + "'.\n";
    ar_log_debug() << "discovering " << svc;
    // Ask szgserver for IP:port of service "SZG_INPUT0".
    // If the service doesn't exist, this call blocks until said server starts.
    const arPhleetAddress netAddress = _szgClient->discoverService(serviceName, networks, true);
    if (!netAddress.valid) {
      if (netAddress.address == "standalone") {
        // arSZGClient::discoverService hardcodes "standalone"
        ar_log_error() << "netinput: no szgserver.\n";
        _closeConnection();
        break;
      }
      ar_log_error() << "no " << svc;
      // Throttle, since service won't reappear that quickly,
      // and szgserver itself may be stopping.
      a.sleep();
      continue;
    }
    a.reset();

    // This service has exactly one port.
    const int port = netAddress.portIDs[0];
    const string& IP = netAddress.address;
    svc = serviceName + ", " + IP + ":" + ar_intToString(port) + ".\n";
    ar_log_debug() << "connecting to " << svc;
    if (!_dataClient.dialUpFallThrough(IP, port)) {
      ar_log_warning() << "reconnecting to " << svc;
      continue;
    }

    if (_IP != "NULL" && IP != _IP)
      ar_log_warning() << "different host providing service, was " << _IP << ", now " << svc;
    _IP = IP;

    ar_log_remark() << "connected to " << svc;
    _connected = true;
    _dataTask();
    ar_log_remark() << "disconnected from " << svc;
    _closeConnection();
  }
}

const int bufsizeStart = 500;

arNetInputSource::arNetInputSource() :
  _szgClient(NULL),
  _dataBuffer(new ARchar[bufsizeStart]),
  _dataBufferSize(bufsizeStart),
  _slot(0),
  _serviceName("NULL"),
  _IP("NULL"),
  _port(0),
  _connected(false),
  _sigOK(false)
{
  _dataClient.smallPacketOptimize(true);
}

// Input devices offer services based on slots.
// Slot 0 corresponds to service SZG_INPUT0, slot 1 to SZG_INPUT1, etc.

bool arNetInputSource::setSlot(unsigned slot) {
  if (_serviceName != "NULL") {
    ar_log_error() << "arNetInputSource service name has already been set, setSlot() is illegal.\n";
    return false;
  }
  _slot = slot;
  ar_log_debug() << "using slot " << _slot << ".\n";
  return true;
}

bool arNetInputSource::setServiceName( const string& name ) {
  if (_slot != 0) {
    ar_log_error() << "arNetInputSource service slot has already been set, setServiceName() is illegal.\n";
    return false;
  }
  _serviceName = name;
  ar_log_debug() << "using service " << _serviceName << ".\n";
  return true;
}

bool arNetInputSource::init(arSZGClient& SZGClient) {
  _setDeviceElements(0, 0, 0); // Nothing's attached yet.
  _szgClient = &SZGClient;
  ar_log_debug() << "netinput inited.\n";
  return true;
}

bool arNetInputSource::start() {
  if (!_szgClient) {
    ar_log_error() << "netinput ignoring start before init.\n";
    return false;
  }

  arThread dummy(ar_netInputSourceConnectionTask, this);
  ar_log_remark() << "netinput started.\n";
  return true;
}

void arNetInputSource::_closeConnection() {
  // todo: kill threads, close sockets, etc.
  _connected = false;
  _sigOK = false;
}
