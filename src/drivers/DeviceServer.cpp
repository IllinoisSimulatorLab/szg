//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arFileSink.h"
#include "arInputFactory.h"


bool parseArg(const char* const sz, int& argc, char** argv) {
  // Start at 1 not 0: exe's own name isn't an arg.
  for (int i=1; i<argc; ++i) {
    if (!strcmp(sz, argv[i])) {
      // Shift later args over found one (from i+1 to argc-1 inclusive).
      memmove(argv+i, argv+i+1, sizeof(char**) * (argc-- -i-1));
      return true;
    }
  }
  return false;
}

static bool respond(arSZGClient& cli, bool f = false) {
  if (!cli.sendInitResponse(f)) {
    cerr << "DeviceServer error: maybe szgserver died.\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  // Force the component's name because Win98 won't give the name
  // automatically.  Ascension Spacepad needs Win98.
  const bool fInit = szgClient.init(argc, argv, "DeviceServer");
  if (!szgClient) {
    ar_log_critical() << "DeviceServer failed to init arSZGClient.\n";
    return szgClient.failStandalone(fInit);
  }
  ar_log_debug() << "DeviceServer inited arSZGClient.\n";

  // At most one instance per host.
  int ownerID = -1;
  if (!szgClient.getLock(szgClient.getComputerName() + "/DeviceServer", ownerID)) {
    ar_log_critical() << "DeviceServer: already running (pid = " << ownerID << ").\n";
LAbort:
    (void)respond(szgClient);
    return 1;
  }
  const bool fSimple = parseArg("-s", argc, argv);
  const bool fNetInput = parseArg("-netinput", argc, argv);

  if (argc < 3) {
    ar_log_error() << "usage: DeviceServer [-s] [-netinput] device_description driver_slot [pforth_filter_name]\n";
    goto LAbort;
  }

  arInputNodeConfig inputConfig;
  const int outputSlot = atoi(argv[2]);

  ar_log_debug() << "DeviceServer -netinput = " << fNetInput << ", outputSlot = " << outputSlot << ar_endl;

  int slotNumber = outputSlot+1;
  if (fSimple) {
    // As command-line flags, specify only the driver and slot.
    inputConfig.addInputSource( string(argv[1]) );
  } else {
    const string& config = szgClient.getGlobalAttribute(argv[1]);
    if (config == "NULL") {
      ar_log_critical() << "DeviceServer: no global parameter (<param> in dbatch file) '"
                     << argv[1] << "'.\n";
      goto LAbort;
    }
    if (!inputConfig.parseXMLRecord( config )) {
      ar_log_critical() << "DeviceServer: misconfigured global parameter (<param> in dbatch file) '"
	                   << argv[1] << "'\n";
      goto LAbort;
    }
  }

  // Configure the input sources.
  arInputNode inputNode;
  arInputFactory driverFactory( inputConfig );
  if (!driverFactory.configure( szgClient )) {
    ar_log_critical() << "DeviceServer failed to configure arInputFactory.\n";
    goto LAbort;
  }

  if (!driverFactory.loadInputSources( inputNode, slotNumber, fNetInput )) {
    ar_log_critical() << "DeviceServer failed to load input drivers.\n";
    goto LAbort;
  }

  // Configure the input sinks. By default, include a net input sink
  // (for transmitting data) and a "file sink" (for logging).
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot( outputSlot )) {
    ar_log_critical() << "DeviceServer: invalid slot " << outputSlot << ".\n";
    goto LAbort;
  }
  // Tell netInputSink how we were launched.
  netInputSink.setInfo( argv[1] );
  inputNode.addInputSink( &netInputSink, false );

  arFileSink fileSink;
  inputNode.addInputSink( &fileSink, false );

  if (!driverFactory.loadInputSinks( inputNode )) {
    ar_log_critical() << "DeviceServer failed to load input sinks.\n";
    goto LAbort;
  }

  string namedPForthProgram;
  if (argc >= 4) {
    namedPForthProgram = string(argv[3]);
  }
  if (!driverFactory.loadFilters( inputNode, namedPForthProgram )) {
    ar_log_critical() << "DeviceServer failed to load filters.\n";
    goto LAbort;
  }

  const bool ok = inputNode.init(szgClient);
  if (!ok) {
    ar_log_critical() << "DeviceServer has no input.\n";
  }
  if (!respond(szgClient, ok)) {
    ar_log_error() << "DeviceServer ignoring failed init.\n";
  }
  if (!ok) {
    // Bug: in linux, this may hang.  Which other thread still runs?
    return 1; // init failed
  }

  if (!inputNode.start()) {
    if (!szgClient.sendStartResponse(false)) {
      cerr << "DeviceServer error: maybe szgserver died.\n";
    }
    return 1;
  }
  if (!szgClient.sendStartResponse(true)) {
    cerr << "DeviceServer error: maybe szgserver died.\n";
  }

  // Message task.
  string messageType, messageBody;
  while (true) {
    const int sendID = szgClient.receiveMessage(&messageType, &messageBody);
    if (!sendID) {
      // Shutdown "forced."
LDie:
      inputNode.stop();
      ar_log_debug() << "shutdown.\n";
      return 0;
    }

    if (messageType=="quit") {
      ar_log_debug() << "shutting down...\n";
      goto LDie;
    }

    else if (messageType=="restart") {
      ar_log_remark() << "DeviceServer restarting.\n";
      inputNode.restart();
    }
    else if (messageType=="dumpon") {
      fileSink.start();
    }
    else if (messageType=="dumpoff") {
      fileSink.stop();
    }
    else if (messageType=="log") {
      (void)ar_setLogLevel( messageBody );
    } else {
      arInputSource* driver = driverFactory.findInputSource( messageType );
      if (driver) {
        ar_log_remark() << "handling message " <<
	  messageType << "/" << messageBody << ".\n";
        driver->handleMessage( messageType, messageBody );
      } else {
        ar_log_error() << "ignoring unrecognized messageType '" << messageType << "'.\n";
      }
    }
  }

  // unreachable
  ar_log_critical() << "fell out of message loop.\n";
  return 1;
}
