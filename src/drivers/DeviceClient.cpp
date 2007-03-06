//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arInputNode.h"
#include "arNetInputSource.h"

enum {
  CONTINUOUS_DUMP = 0,
  ON_BUTTON_DUMP,
  EVENT_STREAM,
  BUTTON_STREAM
};

void dump( arInputState& inp ) {
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
  cout << "____\n\n";

  const arMatrix4 m(inp.getMatrix(0));
  const arVector3 xlat(ar_extractTranslation(m));
  cout << "xyz: " << xlat << "\n";


  cout << "____\n";
}

class FilterOnButton : public arIOFilter {
  public:
    FilterOnButton() : arIOFilter() {}
    virtual ~FilterOnButton() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    arInputState _lastInput;
};

bool FilterOnButton::_processEvent( arInputEvent& event ) {
  bool fDump = false;
  switch (event.getType()) {
    case AR_EVENT_BUTTON:
      fDump |= event.getButton() && !_lastInput.getButton( event.getIndex() );
      break;
    case AR_EVENT_GARBAGE:
      ar_log_warning() << "FilterOnButton ignoring garbage.\n";
      break;
    default: // avoid compiler warning
      break;
  }
  _lastInput = *getInputState();
  if (fDump) {
    _lastInput.update( event );
    dump( _lastInput );
  }
  return true;
}

class FilterEventStream : public arIOFilter {
  public:
    FilterEventStream( arInputEventType eventType=AR_EVENT_GARBAGE ) :
      arIOFilter(),
      _printEventType(eventType) {
      }
    virtual ~FilterEventStream() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    int _printEventType;
};
bool FilterEventStream::_processEvent( arInputEvent& event ) {
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
    } else if (!strcmp(argv[2], "-stream")) {
      mode = EVENT_STREAM;
    } else if (!strcmp(argv[2], "-buttonstream")) {
      mode = BUTTON_STREAM;
    }
  }

  arInputNode inputNode;
  arNetInputSource src;
  if (!src.setSlot(slot)) {
    ar_log_error() << "DeviceClient: invalid slot " << slot << ".\n";
    goto LAbort;
  }
  ar_log_remark() << "DeviceClient listening on slot " << slot << ".\n";
  inputNode.addInputSource(&src, false);

  FilterOnButton filterOnButton;
  FilterEventStream filterEventStream;
  FilterEventStream filterButtonStream( AR_EVENT_BUTTON );
  if (mode == ON_BUTTON_DUMP) {
    inputNode.addFilter( &filterOnButton, false );
  } else if (mode == EVENT_STREAM) {
    inputNode.addFilter( &filterEventStream, false );
  } else if (mode == BUTTON_STREAM) {
    inputNode.addFilter( &filterButtonStream, false );
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
  if (!src.connected()) {
    ar_log_warning() << "DeviceClient not yet connected on slot " << slot << ".\n";
  }

  if (!szgClient.sendStartResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }

  arThread dummy(ar_messageTask, &szgClient);

  while (szgClient.connected()){
    if (mode == CONTINUOUS_DUMP && src.connected())
      dump(inputNode._inputState);
    ar_usleep(500000);
  }
  return 0;
}

/*
another mode:
dump prints matrix decomposed into xlat and euler angles
head only, special.
*/
