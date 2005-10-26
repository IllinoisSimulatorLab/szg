//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arInputNode.h"
#include "arNetInputSource.h"

#include "arIOFilter.h"

void dumpState( arInputState& inp ) {
  const int cb = inp.getNumberButtons();
  const int ca = inp.getNumberAxes();
  const int cm = inp.getNumberMatrices();
  cout << "buttons: " << cb << ", "
       << "axes: " << ca << ", "
       << "matrices: " << cm << "\n";
  unsigned int i;
  if (cb > 0) {
    cout << "buttons: ";
    for (i=0; i<inp.getNumberButtons(); i++){
      cout << inp.getButton(i) << " ";
    }
  }
  if (ca > 0) {
    cout << "\naxes: ";
    for (i=0; i<inp.getNumberAxes(); i++){
      cout << inp.getAxis(i) << " ";
    }
  }
  if (cm > 0) {
    cout << "\nmatrices:\n";
    for (i=0; i<inp.getNumberMatrices(); i++){
      cout << inp.getMatrix(i) << endl;
    }
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

  if ((argc != 2)&&(argc != 3)) {
    cerr << "Usage: DeviceClient slot_number [-button]\n";
    return 1;
  }

  bool continuousDump(true);
  if (argc == 3) {
    if (std::string(argv[2]) == "-button") {
      continuousDump = false;
    }
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
    if (continuousDump) {
      dumpState( inputNode._inputState );
    }
    ar_usleep(500000);
  }
  return 0;
}
