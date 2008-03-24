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
  BUTTON_STREAM,
  COMPACT_STREAM,
  POSITION
};

void dump( arInputState& inp ) {
  const unsigned cm = inp.getNumberMatrices();
  const unsigned ca = inp.getNumberAxes();
  const unsigned cb = inp.getNumberButtons();
  if (cm == 0 && ca == 0 && cb == 0)
    return;

  unsigned i;
  if (cm > 0) {
    cout << "\nmatrices (" << cm << ") :\n";
    for (i=0; i<cm; i++)
      cout << inp.getMatrix(i) << "\n";
  }
  if (ca > 0) {
    cout << "axes (" << ca << ")     : ";
    for (i=0; i<ca; i++)
      printf(" %.3g ", inp.getAxis(i)); // more readable than cout
  }
  if (cb > 0) {
    cout << "\nbuttons (" << cb << ")  : ";
    for (i=0; i<cb; i++)
      cout << inp.getButton(i) << " ";
  }
  if (cm == 0) {
    cout << "\n";
  }
  cout << "____\n";

#if 0
  // another mode: dump prints headmatrix decomposed into xlat and euler angles
  const arMatrix4 m(inp.getMatrix(0));
  const arVector3 xlat(ar_extractTranslation(m).round());
  const arVector3 rot((180./M_PI * ar_extractEulerAngles(m, AR_YXZ)).round());
  cout << "\t\t\thead xyz " << xlat << ",  roll ele azi " << rot << "\n____\n\n";
#endif
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
    int _printEventType;
};
bool FilterEventStream::_processEvent( arInputEvent& event ) {
  if (_printEventType==AR_EVENT_GARBAGE || _printEventType==event.getType()) {
    cout << event << "\n";
  }
  return true;
}

class CompactFilterEventStream : public FilterEventStream {
  public:
    CompactFilterEventStream() : FilterEventStream( AR_EVENT_GARBAGE ) {}
    virtual ~CompactFilterEventStream() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
};
bool CompactFilterEventStream::_processEvent( arInputEvent& event ) {
  if (_printEventType!=AR_EVENT_GARBAGE && _printEventType!=event.getType())
    return true;

  switch (event.getType()) {
    case AR_EVENT_GARBAGE:
      break;
    case AR_EVENT_BUTTON:
      cout << "button|" << event.getIndex() << "|" << event.getButton() << endl;
      break;
    case AR_EVENT_AXIS:
      cout << "axis|" << event.getIndex() << "|" << event.getAxis() << endl;
      break;
    case AR_EVENT_MATRIX:
      const float *p = event.getMatrix().v;
      cout << "matrix|" << event.getIndex() << "|" << *p++;
      for (int i=1; i<16; ++i) {
	cout << "," << *p++;
      }
      cout << endl;
  }
  return true;
}

class PositionFilter : public arIOFilter {
  public:
    PositionFilter( unsigned int matrixIndex=0 ) :
      arIOFilter(),
      _matrixIndex(matrixIndex) {
      }
    virtual ~PositionFilter() {}
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
    unsigned int _matrixIndex;
};
bool PositionFilter::_processEvent( arInputEvent& event ) {
  if (event.getType()==AR_EVENT_MATRIX && event.getIndex()==_matrixIndex) {
    cout << ar_extractTranslation(event.getMatrix()) << "\n";
  }
  return true;
}


void printusage() {
  ar_log_error() <<
    "usage: DeviceClient slot_number [-onbutton | -stream | -buttonstream | -compactstream | -position <#> ]\n";
}

int main(int argc, char** argv) {
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc != 2 && argc != 3 && argc != 4) {
    printusage();
LAbort:
    if (!szgClient.sendInitResponse(false)) {
      cerr << "DeviceClient error: maybe szgserver died.\n";
    }
    return 1;
  }

  const int slot = atoi(argv[1]);
  int mode = CONTINUOUS_DUMP;
  if (argc > 2) {
    if (!strcmp(argv[2], "-onbutton")) {
      mode = ON_BUTTON_DUMP;
    } else if (!strcmp(argv[2], "-stream")) {
      mode = EVENT_STREAM;
    } else if (!strcmp(argv[2], "-buttonstream")) {
      mode = BUTTON_STREAM;
    } else if (!strcmp(argv[2], "-compactstream")) {
      mode = COMPACT_STREAM;
    } else if (!strcmp(argv[2], "-position")) {
      mode = POSITION;
    }
  }

  arNetInputSource src;
  if (!src.setSlot(slot)) {
    ar_log_error() << "DeviceClient: invalid slot " << slot << ".\n";
    goto LAbort;
  }
  ar_log_remark() << "DeviceClient listening on slot " << slot << ".\n";
  arInputNode inputNode;
  inputNode.addInputSource(&src, false);

  FilterOnButton filterOnButton;
  FilterEventStream filterEventStream;
  FilterEventStream filterButtonStream( AR_EVENT_BUTTON );
  CompactFilterEventStream compactFilterEventStream;
  PositionFilter* postionFilterPtr;

  if (mode == ON_BUTTON_DUMP) {
    inputNode.addFilter( &filterOnButton, false );
  } else if (mode == EVENT_STREAM) {
    inputNode.addFilter( &filterEventStream, false );
  } else if (mode == BUTTON_STREAM) {
    inputNode.addFilter( &filterButtonStream, false );
  } else if (mode == COMPACT_STREAM) {
    inputNode.addFilter( &compactFilterEventStream, false );
  } else if (mode == POSITION) {
    if (argc != 4) {
      printusage();
      goto LAbort;
    }
    const unsigned int matrixIndex = (unsigned int)atoi(argv[3]);
    postionFilterPtr = new PositionFilter( matrixIndex );
    inputNode.addFilter( postionFilterPtr, false );
  }

  if (!inputNode.init(szgClient)) {
    goto LAbort;
  }

  if (!szgClient.sendInitResponse(true)) {
    cerr << "DeviceClient error: maybe szgserver died.\n";
  }
  ar_usleep(40000); // avoid interleaving diagnostics from init and start

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
    ar_usleep(150000);
  }
  return 0;
}
