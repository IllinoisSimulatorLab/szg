//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arStringTextStream.h"
#include "arXMLUtilities.h"
#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arNetInputSource.h"
#include "arIOFilter.h"
#include "arFileSink.h"

#ifndef AR_LINKING_STATIC
#include "arSharedLib.h"
#else
// Various device driver headers.
#include "arJoystickDriver.h"
#include "arIntelGamepadDriver.h"
#include "arMotionstarDriver.h"
#include "arFOBDriver.h"
#include "arBirdWinDriver.h"
#include "arFaroDriver.h"
#include "arSpacepadDriver.h"
#include "arEVaRTDriver.h"
#include "arIntersenseDriver.h"
#include "arVRPNDriver.h"
#include "arReactionTimerDriver.h"
#include "arFileSource.h"
#include "arLogitechDriver.h"
#include "arPPTDriver.h"
#endif

// Device drivers.
#include "arFileSource.h"
#include "arPForthFilter.h"

#include <map>

#ifdef AR_LINKING_STATIC
struct DriverTableEntry {
  const char* arName;
  const char* serviceName;
  const char* printableName;
  const char* netName;
};
const int NUM_SERVICES = 20;
// NOTE: there is a really obnoxious kludge below... namely
// arFileSource should be able to masquerade as any of the other
// devices/ services, but it is stuck as SZG_INPUT!
const struct DriverTableEntry driverTable[NUM_SERVICES] = {
  { "arJoystickDriver",     "SZG_JOYSTICK", "joystick driver", NULL},
  { "arIntelGamepadDriver", "SZG_JOYSTICK", "intel gamepad driver", NULL},
  { "arMotionstarDriver",   "SZG_TRACKER",  "MotionStar driver", NULL},
  { "arFaroDriver",         "SZG_FARO",     "FaroArm driver", NULL},
  { "arCubeTracker",        "SZG_INPUT",    "cube tracker", "USED"},
  { "arFaroFOB",            "SZG_FAROFOB",  "FaroArm/FOB combo", "USED"},
  { "arFOBDriver",          "SZG_FOB",      "flock-of-birds driver", NULL},
  { "arBirdWinDriver",      "SZG_FOB",      "WinBird flock-of-birds driver", NULL},
  { "arFaroCalib",          "SZG_FAROCAL",  "FaroArm/MotionStar combo", "USED"},
  { "arSpacepadDriver",     "SZG_TRACKER",  "Ascension Spacepad", NULL},
  { "arIdeskTracker",       "SZG_INPUT",    "IDesk tracker", NULL},
  { "arEVaRTDriver",        "SZG_EVART",    "EVaRT driver", NULL},
  { "arFileSource",         "SZG_MOCAP",    "replay of file data", NULL},
  { "arIntersenseDriver",   "SZG_INPUT",    "intersense trackers", NULL},
  { "arVRPNDriver",         "SZG_VRPN",     "vrpn bridge", NULL},
  { "arCubeTrackWand",      "SZG_INPUT",    "cube tracker with Monowand", "USED"},
  { "arReactionTimer",      "SZG_RT",       "Reaction Timer", NULL},
  { "arPassiveTracker",     "SZG_INPUT",    "Passive Display Tracker", NULL},
  { "arLogitechDriver",     "SZG_LOGITECH", "Logitech Tracker", NULL},
  { "arPPTDriver",          "SZG_PPT",      "WorldViz PPT Tracker", "USED"}
};

arInputSource* inputSourceFactory( const string& driverName ) {
  arInputSource* theSource = NULL;
  int iService;
  for (iService = 0; iService < NUM_SERVICES; ++iService) {
    if (driverName == string(driverTable[iService].arName)) {
      // Found a match.
      //
      // This switch() could eventually become an AbstractFactory or
      // ConcreteFactory (see Design Patterns).
      switch (iService) {
      case 0: theSource = new arJoystickDriver;
        break;
      case 1: theSource = new arIntelGamepadDriver;
        break;
      case 2: theSource = new arMotionstarDriver;
        break;
      case 3: theSource = new arFaroDriver;
        break;
      case 4: theSource = new arMotionstarDriver;
        break;
      case 5: theSource = new arFaroDriver;
        break;
      case 6: theSource = new arFOBDriver;
        break;
      case 7: theSource = new arBirdWinDriver;
        break;
      case 8: theSource = new arMotionstarDriver;
        break;
      case 9: theSource = new arSpacepadDriver;
        break;
      case 10: theSource = new arSpacepadDriver;
        break;
      case 11: theSource = new arEVaRTDriver;
        break;
      case 12: theSource = new arFileSource;
        break;
      case 13: theSource = new arIntersenseDriver;
        break;
      case 14: theSource = new arVRPNDriver;
        break;
      case 15: theSource = new arMotionstarDriver();
        break;
      case 16: theSource = new arReactionTimerDriver;
        break;
      case 17: theSource = new arFOBDriver;
        break;
      case 18: theSource = new arLogitechDriver;
        break;
      case 19: theSource = new arPPTDriver;
        break;
      }
      break;
    }
  }
  return theSource;
}

void printDriverList( arLogStream& os ) {
  os << "Supported input device drivers:\n";
  for (int i=0; i<NUM_SERVICES; ++i) {
    os << "\t" << driverTable[i].arName << "\n";
  }
}
#endif

// The input node configuration (at this early stage) looks like this:
// <szg_device>
//   <input_sources>
//     ... list (possibly empty) of input source object names ...
//   </input_sources>
//   <input_sinks>
//     ... list (possibly empty) of input sink object names ...
//   </input_sinks>
//   <input_filters>
//     ... list (possibly empty) of input filters
//   </input_filters>
//   <pforth>
//     ... either empty (all whitespace) or a pforth program ...
//   </pforth>
// </szg_device>

bool parseArg(const char* const sz, int& argc, char** argv){
  // Start at 1 not 0: exe's own name isn't an arg.
  for (int i=1; i<argc; ++i){
    if (!strcmp(sz, argv[i])){
      // Shift later args over found one (from i+1 to argc-1 inclusive).
      memmove(argv+i, argv+i+1, sizeof(char**) * (argc-- -i-1));
      return true;
    }
  }
  return false;
}

bool parseTokenList(arStringTextStream& tokenStream,
                    const string& tagType,
                    list<string>& tokenList){
  // Get input_sources.
  string tagText(ar_getTagText(&tokenStream));
  if (tagText != tagType){
    ar_log_warning() << "DeviceServer parsing expected " << tagType << " tag.\n";
    return false;
  }
  arBuffer<char> buffer(128);
  if (!ar_getTextBeforeTag(&tokenStream, &buffer)){
    ar_log_warning() << "DeviceServer parsing failed on " << tagType << " field.\n";
    return false;
  }
  stringstream tokens(buffer.data);
  string token;
  do {
    tokens >> token;
    if (!tokens.fail()){
      tokenList.push_back(token);
      ar_log_remark() << tagType << " DeviceServer token '" << token << "'\n";
    }
  } while (!tokens.eof());

  // Closing input_sources tag.
  tagText = ar_getTagText(&tokenStream);
  if (tagText != "/"+tagType){
    ar_log_warning() << "DeviceServer parsing: expected end tag '/" << tagType << "', not '"
      << tagText << "'.\n";
    return false;
  }
  return true;
}

class InputNodeConfig{
 public:
  InputNodeConfig(): valid(false), pforthProgram("") {}
  ~InputNodeConfig() {}

  bool         valid;
  list<string> inputSources;
  list<string> inputSinks;
  list<string> inputFilters; 
  string       pforthProgram;
};

InputNodeConfig parseNodeConfig(const string& nodeConfig){
  InputNodeConfig result;
  arStringTextStream configStream(nodeConfig);
  arBuffer<char> buffer(128); 

  string tagText(ar_getTagText(&configStream));
  if (tagText != "szg_device"){
    ar_log_warning() << "DeviceServer parsing expected starting tag szg_device, not '"
         << tagText << "'.\n";
    return result;
  }
  if (!parseTokenList(configStream, "input_sources", result.inputSources)){
    return result;
  }
  if (!parseTokenList(configStream, "input_sinks", result.inputSinks)){
    return result;
  }
  if (!parseTokenList(configStream, "input_filters", result.inputFilters)){
    return result;
  }
  
  tagText = ar_getTagText(&configStream);
  if (tagText != "pforth"){
    ar_log_warning() << "DeviceServer parsing expected pforth tag.\n";
    return result;
  }
  if (! ar_getTextBeforeTag(&configStream, &buffer)){
    ar_log_warning() << "DeviceServer parsing failed in pforth field.\n";
    return result;
  }
  result.pforthProgram = string(buffer.data);
  tagText = ar_getTagText(&configStream);
  if (tagText != "/pforth"){
    ar_log_warning() << "DeviceServer parsing expected /pforth tag.\n";
    return result;
  }
  tagText = ar_getTagText(&configStream);
  if (tagText != "/szg_device"){
    ar_log_warning() << "DeviceServer parsing expected /szg_device tag.\n";
    return result;
  }
  result.valid = true;
  return result;
}

static bool respond(arSZGClient& cli, bool f = false) {
  if (!cli.sendInitResponse(f)) {
    cerr << "DeviceServer error: maybe szgserver died.\n";
    return false;
  }
  return true;
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  // Force the component's name because Win98 won't give the name
  // automatically.  Ascension Spacepad needs Win98.
  const bool fInit = szgClient.init(argc, argv, "DeviceServer");
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  // At most one instance per host.
  int ownerID = -1;
  if (!szgClient.getLock(szgClient.getComputerName() + "/DeviceServer", ownerID)) {
    ar_log_error() << "DeviceServer: already running (pid = " 
         << ownerID << ").\n";
LAbort:
    (void)respond(szgClient);
    return 1;
  }
  const bool fSimple = parseArg("-s", argc, argv);
  const bool fNetInput = parseArg("-netinput", argc, argv);

  if (argc < 3){
    ar_log_error() << "usage: DeviceServer [-s] [-netinput] device_description driver_slot [pforth_filter_name]\n";
    goto LAbort;
  }

  const unsigned slotNumber = atoi(argv[2]);
  InputNodeConfig nodeConfig;
  if (fSimple){
    // As command-line flags, specify only the driver and slot.
    nodeConfig.inputSources.push_back(argv[1]);
  }
  else{
    const string& config = szgClient.getGlobalAttribute(argv[1]);
    if (config == "NULL") {
      ar_log_error() << "DeviceServer: undefined global parameter (<param> in dbatch file) '" << argv[1] << "'.\n";
      goto LAbort;
    }
    nodeConfig = parseNodeConfig(config);
    if (!nodeConfig.valid){
      ar_log_error() << "DeviceServer: misconfigured global parameter (<param> in dbatch file) '"
	<< argv[1] << "'\n";
      goto LAbort;
    }
  }

  list<string>::iterator iter;

  // Configure the input sources.
  arInputNode inputNode;
  const string execPath = szgClient.getAttribute("SZG_EXEC","path"); // search for dll's
  std::map< std::string, arInputSource* > driverNameMap;
  unsigned slotNext = slotNumber + 1;
  for (iter = nodeConfig.inputSources.begin();
       iter != nodeConfig.inputSources.end(); iter++) {
    arInputSource* theSource = NULL;
    // Is the requested library embedded in the library?
    if (*iter == "arNetInputSource") {
      arNetInputSource* netInputSource = new arNetInputSource();
      if (!netInputSource->setSlot(slotNext)) {
        ar_log_warning() << "DeviceServer: invalid slot " << slotNext << ".\n";
        goto LAbort;
      }
      slotNext++;
      inputNode.addInputSource(netInputSource, true);
    } else {
#ifndef AR_LINKING_STATIC
      // A dynamically loaded library.
      arSharedLib* inputSourceSharedLib = new arSharedLib();
      string error;
      if (!inputSourceSharedLib->createFactory(*iter, execPath, "arInputSource", error)) {
        ar_log_error() << error;
        goto LAbort;
      }

      theSource = (arInputSource*) inputSourceSharedLib->createObject();
#else
      theSource = inputSourceFactory( *iter );
      if (!theSource) {
        printDriverList( ar_log_error() );
      }
#endif
      if (!theSource) {
        ar_log_error() << "DeviceServer failed to create input source '" << *iter << "'.\n";
        goto LAbort;
      }
      ar_log_debug() << "DeviceServer created input source '"
                     << *iter << "' in slot " << slotNext-1 << ".\n";
      driverNameMap[*iter] = theSource;
      inputNode.addInputSource(theSource, false);
    }
  }

  if (fNetInput){
    // Add an implicit net input source.
    arNetInputSource* commandLineNetInputSource = new arNetInputSource();
    if (!commandLineNetInputSource->setSlot(slotNext)) {
      ar_log_error() << "DeviceServer: invalid slot " << slotNext << ".\n";
      goto LAbort;
    }
    // Skip over "slotNext+1" listening slot.
    slotNext += 2;
    inputNode.addInputSource(commandLineNetInputSource,true);
  }

  // Configure the input sinks. By default, include a net input sink
  // (for transmitting data) and a "file sink" (for logging).
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slotNumber)) {
    ar_log_error() << "DeviceServer: invalid slot " << slotNumber << ".\n";
    goto LAbort;
  }
  slotNext++;

  // Tell netInputSink how we were launched.
  netInputSink.setInfo(argv[1]);

  inputNode.addInputSink(&netInputSink,false);

  arFileSink fileSink;
  inputNode.addInputSink(&fileSink,false);

#ifndef AR_LINKING_STATIC
  // Add the dynamically loaded sinks.
  for (iter = nodeConfig.inputSinks.begin();
       iter != nodeConfig.inputSinks.end(); iter++){
    arInputSink* theSink = NULL;
    // A dynamically loaded library
    arSharedLib* inputSinkObject = new arSharedLib();
    string error;
    if (!inputSinkObject->createFactory(*iter, execPath, "arInputSink", error)){
      ar_log_error() << error;
      goto LAbort;
    }
    // Can create our object.
    theSink = (arInputSink*) inputSinkObject->createObject();
    if (!theSink) {
      ar_log_error() << "DeviceServer failed to create input sink.\n";
      goto LAbort;
    }
    ar_log_debug() << "DeviceServer created input sink '" << *iter << ".\n";
    inputNode.addInputSink(theSink, true);
  }
#endif

  // Load the filters.

  // Add a PForth filter(s) to beginning of chain, for remapping sensors etc.
  // First add a filter from the config file.
  arPForthFilter firstFilter;
  ar_PForthSetSZGClient( &szgClient );
  if (!firstFilter.loadProgram( nodeConfig.pforthProgram )){
    return 1;
  }
  // The PForth filter is owned by the program, since it is declared 
  // statically.
  inputNode.addFilter(&firstFilter, false);

  // Next, add a filter from the command line, if such was specified.
  if (argc >= 4){
    string commandLineProgram = 
      szgClient.getGlobalAttribute(argv[3]);
    if (commandLineProgram == "NULL"){
      ar_log_remark() << "DeviceServer: no program named " << argv[3] << ".\n";
    }
    else{
      arPForthFilter* commandLineFilter = new arPForthFilter();
      if (!commandLineFilter->loadProgram( commandLineProgram )){
	return 1;
      }
      // Node owns the filter.
      inputNode.addFilter(commandLineFilter, false);
    }
  }
  
#ifndef AR_LINKING_STATIC
  // Add the various optional filters...
  for (iter = nodeConfig.inputFilters.begin();
       iter != nodeConfig.inputFilters.end(); iter++){
    arIOFilter* theFilter = NULL;
    // A dynamically loaded library
    arSharedLib* inputFilterSharedLib = new arSharedLib();
    string error;
    if (!inputFilterSharedLib->createFactory(*iter, execPath, "arIOFilter", error)){
      ar_log_error() << error;
      goto LAbort;
    }
    theFilter = (arIOFilter*) inputFilterSharedLib->createObject();
    if (!theFilter) {
      ar_log_error() << "DeviceServer failed to create input filter.\n";
      goto LAbort;
    }
    if (!theFilter->configure( &szgClient )){
      ar_log_error() << "DeviceServer failed to configure filter.\n";
      goto LAbort;
    }
    // We own this.
    inputNode.addFilter(theFilter,true);
  }
#endif

  const bool ok = inputNode.init(szgClient);
  if (!ok)
    ar_log_error() << "DeviceServer has no input.\n";
  if (!respond(szgClient, ok)){
    ar_log_warning() << "DeviceServer ignoring failed init.\n";
    // return 1;
  }
  if (!ok) {
    // Bug: in linux, this may hang.  Which other thread still runs?
    return 1; // init failed
  }

  if (!inputNode.start()){
    if (!szgClient.sendStartResponse(false))
      cerr << "DeviceServer error: maybe szgserver died.\n";
    return 1;
  }
  if (!szgClient.sendStartResponse(true))
    cerr << "DeviceServer error: maybe szgserver died.\n";

  // Message task.
  string messageType, messageBody;
  while (true) {
    const int sendID = szgClient.receiveMessage(&messageType, &messageBody);
    if (!sendID){
      // Shutdown "forced."
      // Copypaste from below.
      inputNode.stop();
LDie:
      ar_log_debug() << "DeviceServer shutdown.\n";
      return 0;
    }

    if (messageType=="quit"){
      ar_log_debug() << "DeviceServer shutting down...\n";
      inputNode.stop();
      std::map< std::string, arInputSource* >::iterator killIter;
      for (killIter = driverNameMap.begin(); killIter != driverNameMap.end(); ++killIter) {
        if (killIter->second)
          delete killIter->second;
      }
      driverNameMap.clear();
      goto LDie;
    }

    else if (messageType=="restart"){
      ar_log_remark() << "DeviceServer restarting.\n";
      inputNode.restart();
    }
    else if (messageType=="dumpon"){
      fileSink.start();
    }
    else if (messageType=="dumpoff"){
      fileSink.stop();
    } else {
      const std::map< std::string, arInputSource* >::iterator iter =
        driverNameMap.find( messageType );
      if (iter == driverNameMap.end()) {
        ar_log_warning() << "DeviceServer ignoring unrecognized messageType '"
	     << messageType << "'.\n";
      }
      else {
        ar_log_remark() << "DeviceServer handling message " << messageType 
	  << "/" << messageBody << ".\n";
        arInputSource* driver = iter->second;
        if (driver)
          driver->handleMessage( messageType, messageBody );
	else
          ar_log_warning() << "DeviceServer ignoring NULL from driverNameMap.\n";
      }
    }
  }

  // unreachable
  ar_log_error() << "DeviceServer fell out of message loop.\n";
  return -1;
}
