//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arIOFilter.h"

int CONTINUOUS_DUMP = 0;
int ON_BUTTON_DUMP = 1;
int EVENT_STREAM = 2;
int BUTTON_STREAM = 3;


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
  cout << "****\n";
}

class onButtonEventFilter : public arIOFilter {
  public:
    onButtonEventFilter() : arIOFilter() {}
    virtual ~onButtonEventFilter() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    arInputState _lastInput;
};

bool onButtonEventFilter::_processEvent( arInputEvent& event ) {
  bool dump = false;
  switch (event.getType()) {
    case AR_EVENT_BUTTON:
      dump |= event.getButton() && !_lastInput.getButton( event.getIndex() );
      break;
    case AR_EVENT_GARBAGE:
      ar_log_warning() << "onButtonEventFilter ignoring garbage.\n";
      break;
    default:
      // avoid compiler warning
      break;
  }
  _lastInput = *getInputState();
  if (dump) {
    _lastInput.update( event );
    dumpState( _lastInput );
  }
  return true;
}

class EventStreamEventFilter : public arIOFilter {
  public:
    EventStreamEventFilter( arInputEventType eventType=AR_EVENT_GARBAGE ) :
      arIOFilter(),
      _printEventType(eventType) {
      }
    virtual ~EventStreamEventFilter() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    int _printEventType;
};
bool EventStreamEventFilter::_processEvent( arInputEvent& event ) {
  if ((_printEventType==AR_EVENT_GARBAGE) || (_printEventType==event.getType())) {
    cout << event << endl;
  }
  return true;
}


int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  // Without this, ar_log_xxx() are mute.
  ar_log().setStream(cout);

  if (argc != 2 && argc != 3) {
    ar_log_error() << "usage: DeviceClient slot_number [-onbutton | -stream | -buttonstream]\n";
    return 1;
  }

  const int slot = atoi(argv[1]);
  ar_log_remark() << "DeviceClient slot = " << slot << ".\n";

  int operationMode = CONTINUOUS_DUMP;
  if (argc == 3) {
    if (!strcmp(argv[2], "-onbutton")) {
      operationMode = ON_BUTTON_DUMP;
      ar_log_remark() << "DeviceClient Operation mode = ON_BUTTON_DUMP.\n";
    } else if (!strcmp(argv[2], "-stream")) {
      operationMode = EVENT_STREAM;
      ar_log_remark() << "DeviceClient Operation mode = EVENT_STREAM.\n";
    } else if (!strcmp(argv[2], "-buttonstream")) {
      operationMode = BUTTON_STREAM;
      ar_log_remark() << "DeviceClient Operation mode = BUTTON_STREAM.\n";
    }
  }

  arInputNode inputNode;
  arNetInputSource netInputSource;
  inputNode.addInputSource(&netInputSource, false);
  if (!netInputSource.setSlot(slot)) {
    ar_log_error() << "DeviceClient: invalid slot " << slot << ".\n";
    return 1;
  }

  onButtonEventFilter onButtonFilter;
  EventStreamEventFilter eventStreamFilter;
  EventStreamEventFilter buttonStreamFilter( AR_EVENT_BUTTON );
  if (operationMode == ON_BUTTON_DUMP) {
    inputNode.addFilter( &onButtonFilter, false );
  } else if (operationMode == EVENT_STREAM) {
    inputNode.addFilter( &eventStreamFilter, false );
  } else if (operationMode == BUTTON_STREAM) {
    inputNode.addFilter( &buttonStreamFilter, false );
  }

  if (!inputNode.init(szgClient)) {
    if (!szgClient.sendInitResponse(false))
     cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  if (!szgClient.sendInitResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }
  if (!inputNode.start()) {
    if (!szgClient.sendStartResponse(false))
      cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  if (!netInputSource.connected()) {
    ar_log_error() << "DeviceClient: not connected on slot " << slot << ".\n";
    if (!szgClient.sendStartResponse(false))
      cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  if (!szgClient.sendStartResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }

  arThread dummy(ar_messageTask, &szgClient);
  while (true){
    if ((operationMode==CONTINUOUS_DUMP) && netInputSource.connected())
      dumpState(inputNode._inputState);
    ar_usleep(500000);
  }
  return 0;
}
