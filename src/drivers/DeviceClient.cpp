//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDeviceClient.h"


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
        cout << "\nmatrices, " << cm << ":\n";
        for (i=0; i<cm; ++i)
            cout << inp.getMatrix(i) << "\n";
    }
    if (ca > 0) {
        cout << "axes, " << ca << "        : ";
        for (i=0; i<ca; ++i)
            printf("%8.2f ", inp.getAxis(i)); // more readable than cout
    }
    if (cb > 0) {
        cout << "\nbuttons, " << cb << " : ";
        for (i=0; i<cb; ++i)
            cout << inp.getButton(i); // no space after each button, to fit more per line.
    }
    if (ca > 0 || cb > 0) {
        cout << "\n";
    }
    cout << "____\n";

#if 0
    // another mode: dump prints headmatrix decomposed into xlat and euler angles
    const arMatrix4 m(inp.getMatrix(0));
    const arVector3 xlat(ar_extractTranslation(m).round());
    const arVector3 rot((180./M_PI * ar_extractEulerAngles(m, AR_YXZ)).round());
    cout << "\t\t\thead xyz " << xlat << ",    roll ele azi " << rot << "\n____\n\n";
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
            ar_log_error() << "FilterOnButton ignoring garbage event.\n";
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

    arDeviceClient devClient( &szgClient );

    if (argc != 2 && argc != 3 && argc != 4) {
        printusage();
LAbort:
        if (!szgClient.sendInitResponse(false)) {
            cerr << "DeviceClient error: maybe szgserver died.\n";
        }
        return 1;
    }

    const int slotNum = atoi(argv[1]);
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

    FilterOnButton filterOnButton;
    FilterEventStream filterEventStream;
    FilterEventStream filterButtonStream( AR_EVENT_BUTTON );
    CompactFilterEventStream compactFilterEventStream;
    PositionFilter* positionFilterPtr;

    std::vector<arIOFilter*> filtVec;

    if (mode == ON_BUTTON_DUMP) {
        filtVec.push_back( &filterOnButton );
    } else if (mode == EVENT_STREAM) {
        filtVec.push_back( &filterEventStream );
    } else if (mode == BUTTON_STREAM) {
        filtVec.push_back( &filterButtonStream );
    } else if (mode == COMPACT_STREAM) {
        filtVec.push_back( &compactFilterEventStream );
    } else if (mode == POSITION) {
        if (argc != 4) {
            printusage();
            goto LAbort;
        }
        const unsigned int matrixIndex = (unsigned int)atoi(argv[3]);
        positionFilterPtr = new PositionFilter( matrixIndex );
        filtVec.push_back( positionFilterPtr );
    }

    if (!devClient.init( slotNum, &filtVec )) {
        goto LAbort;
    }

    if (!szgClient.sendInitResponse(true)) {
        cerr << "DeviceClient error: maybe szgserver died.\n";
    }
    ar_usleep(40000); // avoid interleaving diagnostics from init and start

    // asyncMessageTask defined in phleet/arMessageHandler.h
    if (!devClient.start( asyncMessageTask )) {
        cerr << "DeviceClient failed to start message thread.\n";
        szgClient.closeConnection();
        return 1;
    }

    std::vector<arMessage> msgVec;
    std::vector<arMessage>::iterator msgIter;
    string errMsg( "Ignoring unknown message type." );

    while (devClient.continueMessageTask()) {
        if (mode == CONTINUOUS_DUMP) {
            dump(devClient.getInputNode()->_inputState);
        }

        msgVec = devClient.getMessages();
        for (msgIter = msgVec.begin(); msgIter != msgVec.end(); ++msgIter) {
            if (!msgIter->id) {
              // Shutdown "forced."
              ar_log_critical() << "arSZGClient.receiveMessage() failed.\n";
              devClient.stop();
              szgClient.closeConnection();
              ar_log_critical() << "shutdown.\n";
              return 0;
            }
            if (msgIter->messageType == "quit") {
              ar_log_critical() << "'quit' message received, exiting...\n";
              devClient.stop();
              szgClient.closeConnection();
              ar_log_critical() << "shutdown.\n";
              return 0;
            }

            devClient.respondToMessage( &(*msgIter), errMsg );
        }
        ar_usleep(10000);
    }
    return 0;
}
