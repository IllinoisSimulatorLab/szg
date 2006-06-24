//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNetInputSink.h"

void ar_netInputSinkConnectionTask(void* sink){
  arNetInputSink* s = (arNetInputSink*) sink;
  // Don't retry infinitely.
  while (s->_dataServer.acceptConnection() != NULL) {
    // cerr << "arInputServer got a connection.\n";
  }
}

arNetInputSink::arNetInputSink() :
  _client(NULL),
  _dataServer(1000),
  _slot(0),
  _interface(string("NULL")),
  _port(0),
  _fValid(false),
  _info(""){
  _dataServer.smallPacketOptimize(true);
}

/// Input devices in phleet offer services based on slots.
/// Slot x corresponds to service SZG_INPUTx, for nonnegative x.
/// @param slot the slot in question
bool arNetInputSink::setSlot(int slot){
  if (slot<0){
    ar_log_warning() << "arNetInputSink ignoring negative input device slot.\n";
    return false;
  }
  _slot = slot;
  return true;
}

bool arNetInputSink::init(arSZGClient& SZGClient){
  // cache the arSZGClient for connection brokering
  _client = &SZGClient;
  _client->initResponse() << "arNetInputSink remark: initialized.\n";
  return true;
}

bool arNetInputSink::start(){
  if (!_client){
    cerr << "arNetInputSink error: can't start before init.\n";
    return false;
  }

  if (_fValid){
    // start() already succeeded.
    // Do not start() again, since e.g. stop() does nothing.
    cout << "DeviceServer remark: ignoring restart.\n";
    // Return true, lest arInputNode::restart fail.
    // Fix this once stop() does the expected thing.
    return true;
  }

  stringstream& startResponse = _client->startResponse(); 
  // Bug: handle service naming issues.
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_client->createComplexServiceName(buffer));
  int port = -1;
  if (!_client->registerService(serviceName,"input",1,&port)){
    startResponse << "arNetInputSink error: failed to register service '" <<
      serviceName << "'.\n";
    return false;
  }
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");
  //// \todo use better "< 10" code copypasted already elsewhere
  int trials = 0;
  bool success = false;
  while (trials < 10 && !success){
    if (!_dataServer.beginListening(_inp.getDictionary())){
      startResponse << "arNetInputSink error: failed to listen.\n";
      _client->requestNewPorts(serviceName,"input",1,&port);
      trials++;
    }
    else{
      success = true;
    }
  }
  if (!success)
    return false;
  
  _client->confirmPorts(serviceName,"input",1,&port);
  _fValid = true;
  arThread dummy(ar_netInputSinkConnectionTask, this);

  if (!_client->setServiceInfo(serviceName, _info)){
    cout << "arNetInputFilter remark: failed to set info type for service.\n";
  }

  startResponse << "arNetInputSink remark: started.\n";
  return true;
}

void arNetInputSink::receiveData(int,arStructuredData* data){
  if (_fValid){
    _dataServer.sendData(data);
  }
}

void arNetInputSink::setInfo(const string& info){
  _info = info;
}
