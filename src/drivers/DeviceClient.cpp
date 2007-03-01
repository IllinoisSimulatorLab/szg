//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arIOFilter.h"

enum {
  CONTINUOUS_DUMP = 0,
  ON_BUTTON_DUMP,
  EVENT_STREAM,
  BUTTON_STREAM };

void dumpState( arInputState& inp ) {
  const unsigned cb = inp.getNumberButtons();
  const unsigned ca = inp.getNumberAxes();
  const unsigned cm = inp.getNumberMatrices();
  if (cb == 0 && ca == 0 && cm == 0)
    return;

  unsigned int i;
  if (cb > 0) {
    cout << "buttons (" << cb << ")  : ";
    for (i=0; i<cb; i++)
      cout << inp.getButton(i) << " ";
  }
  if (ca > 0) {
    cout << "\naxes (" << ca << ")     : ";
    for (i=0; i<ca; i++)
      cout << inp.getAxis(i) << " ";
  }
  if (cm > 0) {
    cout << "\nmatrices (" << cm << ") :\n\n";
    for (i=0; i<cm; i++)
      cout << inp.getMatrix(i) << "\n";
  }
  cout << "____\n";
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
  if (_printEventType==AR_EVENT_GARBAGE || _printEventType==event.getType()) {
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

  if (argc != 2 && argc != 3) {
    ar_log_error() << "usage: DeviceClient slot_number [-onbutton | -stream | -buttonstream]\n";
LAbort:
    if (!szgClient.sendInitResponse(false))
     cerr << "DeviceClient error: maybe szgserver died.\n";
    return 1;
  }

  const int slot = atoi(argv[1]);
  int mode = CONTINUOUS_DUMP;
  if (argc == 3) {
    if (!strcmp(argv[2], "-onbutton")) {
      mode = ON_BUTTON_DUMP;
      ar_log_remark() << "DeviceClient using mode ON_BUTTON_DUMP.\n";
    } else if (!strcmp(argv[2], "-stream")) {
      mode = EVENT_STREAM;
      ar_log_remark() << "DeviceClient using mode EVENT_STREAM.\n";
    } else if (!strcmp(argv[2], "-buttonstream")) {
      mode = BUTTON_STREAM;
      ar_log_remark() << "DeviceClient using mode BUTTON_STREAM.\n";
    }
  }

  arInputNode inputNode;
  arNetInputSource netInputSource;
  inputNode.addInputSource(&netInputSource, false);
  if (!netInputSource.setSlot(slot)) {
    ar_log_error() << "DeviceClient: invalid slot " << slot << ".\n";
    goto LAbort;
  }
  ar_log_remark() << "DeviceClient listening on slot " << slot << ".\n";

  onButtonEventFilter onButtonFilter;
  EventStreamEventFilter eventStreamFilter;
  EventStreamEventFilter buttonStreamFilter( AR_EVENT_BUTTON );
  if (mode == ON_BUTTON_DUMP) {
    inputNode.addFilter( &onButtonFilter, false );
  } else if (mode == EVENT_STREAM) {
    inputNode.addFilter( &eventStreamFilter, false );
  } else if (mode == BUTTON_STREAM) {
    inputNode.addFilter( &buttonStreamFilter, false );
  }

  if (!inputNode.init(szgClient)) {
    goto LAbort;
  }

  if (!szgClient.sendInitResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }
  ar_usleep(50000); // avoid interleaving diagnostics from init and start

  if (!inputNode.start()) {
    if (!szgClient.sendStartResponse(false)) {
      cerr << "DeviceClient error: maybe szgserver died.\n";
    }
    return 1;
  }

  // todo: warn only after 2 seconds have elapsed (separate thread).
  if (!netInputSource.connected()) {
    ar_log_warning() << "DeviceClient not yet connected on slot " << slot << ".\n";
  }

  if (!szgClient.sendStartResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }

  arThread dummy(ar_messageTask, &szgClient);

  while (szgClient.connected()){
    if (mode == CONTINUOUS_DUMP && netInputSource.connected())
      dumpState(inputNode._inputState);
    ar_usleep(500000);
  }
  return 0;
}
