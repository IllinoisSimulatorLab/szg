//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arNetInputSink.h"

void ar_netInputSinkConnectionTask(void* sink){
  arNetInputSink* s = (arNetInputSink*) sink;
  // Don't keep trying infinitely.
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

/// Input devices in phleet offer services based on slots. So... slot 0 corresponds
/// to service SZG_INPUT0, slot 1 corresponds to service SZG_INPUT1, and so on.
/// @param slot the slot in question
void arNetInputSink::setSlot(int slot){
  if (slot<0){
    cerr << "arNetInputSource warning: overriding negative input device slot "
         << slot << ".\n";
    slot = 0; 
  }
  _slot = slot;
}

bool arNetInputSink::init(arSZGClient& SZGClient){
  // cache the arSZGClient for connection brokering
  _client = &SZGClient;
  _client->initResponse() << "arNetInputSink remark: initialized.\n";
  return true;
}

bool arNetInputSink::start(){
  if (!_client){
    cerr << "arNetInputSink error: init was not called before start.\n";
    return false;
  }
  // If _fValid has been set, then we have succeeded in start() already.
  // DO NOT start() again. This is important, since, for instance, stop()
  // currently does nothing.
  if (_fValid){
    cout << "DeviceServer remark: attempted to start again. Ignore.\n";
    // Return true, because otherwise arInputNode::restart might fail.
    // This is to-be-fixed once stop() actually does the expected thing.
    return true;
  }
  // Important to send output to the component's start stream
  // (which will be realyed back to the launching dex command)
  stringstream& startResponse = _client->startResponse(); 
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // this does not really deal with the many service naming issues 
  // that will exist!
  char buffer[32];
  sprintf(buffer,"SZG_INPUT%i",_slot);
  const string serviceName(_client->createComplexServiceName(buffer));
  int port = -1;
  if (!_client->registerService(serviceName,"input",1,&port)){
    startResponse << "arNetInputSink error: failed to register service.\n";
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
  if (!success){
    return false;
  }
  
  _client->confirmPorts(serviceName,"input",1,&port);
  _fValid = true;
  arThread dummy(ar_netInputSinkConnectionTask, this);

  // Must set the service's "info" attribute.
  if (!_client->setServiceInfo(serviceName, _info)){
    cout << "arNetInputFilter remark: failed to set info type for service.\n";
  }

  startResponse << "arNetInputSink remark: started.\n";
  return true;
}

bool arNetInputSink::stop(){
  return true;
}

bool arNetInputSink::restart(){
  return stop() && start();
}

void arNetInputSink::receiveData(int,arStructuredData* data){
  if (_fValid){
    _dataServer.sendData(data);
  }
}

void arNetInputSink::setInfo(const string& info){
  _info = info;
}
