//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arInputNode.h"
#include "arNetInputSource.h"

#include "arIOFilter.h"

void dumpState( arInputState& inp ) {
  cout << "buttons: " << inp.getNumberButtons() << ", "
       << "axes: " << inp.getNumberAxes() << ", "
       << "matrices: " << inp.getNumberMatrices() << "\n";
  cout << "buttons: ";
  unsigned int i;
  for (i=0; i<inp.getNumberButtons(); i++){
    cout << inp.getButton(i) << " ";
  }
  cout << "\n";
  cout << "axes: ";
  for (i=0; i<inp.getNumberAxes(); i++){
    cout << inp.getAxis(i) << " ";
  }
  cout << "\n";
  cout << "matrices:\n";
  for (i=0; i<inp.getNumberMatrices(); i++){
    cout << inp.getMatrix(i) << endl;
  }
  cout << "\n";
  cout << "*************************************************\n";
}

class arClientEventFilter : public arIOFilter {
  public:
    arClientEventFilter() : arIOFilter(), _first(true) {}
    virtual ~arClientEventFilter() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    bool _first;
    arInputState _lastInput;
};
bool arClientEventFilter::_processEvent( arInputEvent& event ) {
  bool dump(false);
  switch (event.getType()) {
    case AR_EVENT_BUTTON:
      if (event.getButton() && !_lastInput.getButton( event.getIndex() )) {
        dump = true;
      }
      break;
    default:
      ;
  }
  _lastInput = *getInputState();
  if (dump) {
    _lastInput.update( event );
    dumpState( _lastInput );
  }
  return true;
}

int main(int argc, char** argv){

  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  cerr << "initing...\n";
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  if (argc != 2){
    cerr << "Usage: DeviceClient slot_number\n";;
    return 1;
  }
  int slot = atoi(argv[1]);

  arInputNode inputNode;
  arNetInputSource netInputSource;
  arClientEventFilter filter;
  inputNode.addInputSource(&netInputSource,false);
  netInputSource.setSlot(slot);
  inputNode.addFilter( &filter, false );
  if (!inputNode.init(szgClient)){
    szgClient.sendInitResponse(false);
    return 1;
  }
  else{
    szgClient.sendInitResponse(true);
  }
  if (!inputNode.start()){
    szgClient.sendStartResponse(false);
    return 1;
  }
  else{
    szgClient.sendStartResponse(true);
  }

  arThread dummy(ar_messageTask, &szgClient);
  while (true){
//    dumpState( inputNode._inputState );
    ar_usleep(500000);
  }
  return 0;
}
