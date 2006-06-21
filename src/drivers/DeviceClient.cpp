//********************************************************
// Syzygy is licensed under the BSD license v2
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
  const unsigned cb = inp.getNumberButtons();
  const unsigned ca = inp.getNumberAxes();
  const unsigned cm = inp.getNumberMatrices();
  if (cb == 0 && ca == 0 && cm == 0)
    return;

  cout << "buttons: " << cb << ", "
       << "axes: " << ca << ", "
       << "matrices: " << cm << "\n";
  unsigned int i;
  if (cb > 0) {
    cout << "buttons: ";
    for (i=0; i<cb; i++)
      cout << inp.getButton(i) << " ";
  }
  if (ca > 0) {
    cout << "\naxes: ";
    for (i=0; i<ca; i++)
      cout << inp.getAxis(i) << " ";
  }
  if (cm > 0) {
    cout << "\nmatrices:\n";
    for (i=0; i<cm; i++)
      cout << inp.getMatrix(i) << endl;
  }
  cout << "\n*************************************************\n";
}

class arClientEventFilter : public arIOFilter {
  public:
    arClientEventFilter() : arIOFilter() {}
    virtual ~arClientEventFilter() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    arInputState _lastInput;
};

bool arClientEventFilter::_processEvent( arInputEvent& event ) {
  bool dump = false;
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

  if (argc != 2 && argc != 3) {
    ar_log_error() << "Usage: DeviceClient slot_number [-button]\n";
    return 1;
  }

  const int slot = atoi(argv[1]);
  const bool continuousDump = argc!=3 || strcmp(argv[2], "-button");

  arInputNode inputNode;
  arNetInputSource netInputSource;
  inputNode.addInputSource(&netInputSource, false);
  if (!netInputSource.setSlot(slot)) {
    ar_log_error() << "DeviceClient: invalid slot " << slot << ".\n";
    return 1;
  }

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

  if (!netInputSource.connected())
    ar_log_warning() << "DeviceClient's input source not connected.\n";

  if (!szgClient.sendStartResponse(true))
    cerr << "DeviceClient error: maybe szgserver died.\n";

  arThread dummy(ar_messageTask, &szgClient);
  while (true){
    if (continuousDump && netInputSource.connected())
      dumpState(inputNode._inputState);
    ar_usleep(500000);
  }
  return 0;
}
