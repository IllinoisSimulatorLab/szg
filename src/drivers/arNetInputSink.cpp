//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNetInputSink.h"

void ar_netInputSinkConnectionTask(void* sink){
  arNetInputSink* s = (arNetInputSink*) sink;
  while (s->_dataServer.acceptConnection() != NULL) {
    ar_log_debug() << "arnetInputSink's arInputServer got a connection.\n";
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

// Input devices in phleet offer services based on slots.
// Slot x corresponds to service SZG_INPUTx, for nonnegative x.
bool arNetInputSink::setSlot(unsigned slot){
  _slot = slot;
  return true;
}

void arNetInputSink::setInfo(const string& info){
  _info = info;
}

bool arNetInputSink::init(arSZGClient& SZGClient){
  // cache the arSZGClient for connection brokering
  _client = &SZGClient;
  ar_log_remark() << "arNetInputSink initialized.\n";
  return true;
}

bool arNetInputSink::start(){
  if (!_client){
    // todo: unify "start before init" checks in many classes.
    ar_log_warning() << "arNetInputSink can't start before init.\n";
    return false;
  }

  if (_fValid){
    // start() already succeeded.
    // Do not start() again, since e.g. stop() does nothing.
    ar_log_remark() << "arNetInputSink ignoring restart.\n";
    // Return true, lest arInputNode::restart fail.
    // Fix this once stop() works.
    return true;
  }

  // Bug: handle service naming issues.
  char buffer[32];
  sprintf(buffer, "SZG_INPUT%i", _slot);
  const string serviceName(_client->createComplexServiceName(buffer));
  int port = -1;
  if (!_client->registerService(serviceName,"input",1,&port)){
    ar_log_warning() << "arNetInputSink failed to register service '" <<
      serviceName << "'.\n  (Does dservices already list that service?)\n";
    return false;
  }
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");

  // todo: use better "< 10" code copypasted already elsewhere
  // todo: this may be better than that other code/
  for (int trials=0;
       !_dataServer.beginListening(_inp.getDictionary()); ){
    ar_log_warning() << "arNetInputSink failed to listen on port " << port << ".\n";
    _client->requestNewPorts(serviceName,"input",1,&port);
    if (++trials >= 10)
      return false;
  }
  
  _client->confirmPorts(serviceName,"input",1,&port);
  _fValid = true;
  arThread dummy(ar_netInputSinkConnectionTask, this);

  if (!_client->setServiceInfo(serviceName, _info)){
    ar_log_remark() << "arNetInputSink failed to set info type for service.\n";
  }

  ar_log_remark() << "arNetInputSink started.\n";
  return true;
}

void arNetInputSink::receiveData(int,arStructuredData* data){
  if (!_fValid)
    return;
  if (!data) {
    ar_log_warning() << "arNetInputSink ignoring NULL data.\n";
    return;
  }
  _dataServer.sendData(data);
}
