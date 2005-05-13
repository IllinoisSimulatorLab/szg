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
       << "matrices: " << inp.getNumberMatrices() << "\n"
       << "buttons: ";
  unsigned int i;
  for (i=0; i<inp.getNumberButtons(); i++){
    cout << inp.getButton(i) << " ";
  }
  cout << "\naxes: ";
  for (i=0; i<inp.getNumberAxes(); i++){
    cout << inp.getAxis(i) << " ";
  }
  cout << "\nmatrices:\n";
  for (i=0; i<inp.getNumberMatrices(); i++){
    cout << inp.getMatrix(i) << endl;
  }
  cout << "\n*************************************************\n";
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
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  if (argc != 2){
    cerr << "Usage: DeviceClient slot_number\n";
    return 1;
  }

  const int slot = atoi(argv[1]);
  arInputNode inputNode;
  arNetInputSource netInputSource;
  inputNode.addInputSource(&netInputSource,false);
  netInputSource.setSlot(slot);
  arClientEventFilter filter;
  inputNode.addFilter( &filter, false );
  if (!inputNode.init(szgClient)){
    if (!szgClient.sendInitResponse(false))
      cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  if (!szgClient.sendInitResponse(true))
    cerr << "DeviceClient error: maybe szgserver died.\n";
  if (!inputNode.start()){
    if (!szgClient.sendStartResponse(false))
      cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  if (!szgClient.sendStartResponse(true))
    cerr << "DeviceClient error: maybe szgserver died.\n";

  arThread dummy(ar_messageTask, &szgClient);
  while (true){
    dumpState( inputNode._inputState );
    ar_usleep(500000);
  }
  return 0;
}
