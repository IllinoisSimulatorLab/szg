//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"

#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arNetInputSource.h"
#include "arIOFilter.h"
#include "arFileSink.h"
#include "arSharedLib.h"

// Various device driver headers.
#include "arFileSource.h"

// Filters.
#include "arConstantHeadFilter.h"
#include "arTrackCalFilter.h"
#include "arPForthFilter.h"
#include "arFaroCalFilter.h"

// These functions are embedded in the shared objects.
typedef arInputSource* (*arInputSourceFactory)();
typedef void (*arSharedObjectType)(char*, int);

int main(int argc, char** argv){
  struct widget /* what's a good name for this? */ {
    const char* arName;
    const char* serviceName;
    const char* printableName;
    const char* netName;
  };
  const int numServices = 20;
  // NOTE: there is a really obnoxious kludge below... namely
  // arFileSource should be able to masquerade as any of the other
  // devices/ services, but it is stuck as SZG_INPUT!
  const struct widget widgets[numServices] = {
    { "arJoystickDriver",     "SZG_JOYSTICK", "joystick driver", NULL},
    { "arIntelGamepadDriver", "SZG_JOYSTICK", "intel gamepad driver", NULL},
    { "arMotionstarDriver",   "SZG_TRACKER", "MotionStar driver", NULL},
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
    { "arPPTDriver",          "SZG_PPT", "WorldViz PPT Tracker", "USED"}
    };

  arSZGClient SZGClient;
  SZGClient.simpleHandshaking(false);
  // note how we force the name of the component. This is because it is
  // impossible to get the name automatically on Win98 and we want to run
  // DeviceServer on Win98
  SZGClient.init(argc, argv, "DeviceServer");
  if (!SZGClient)
    return 1;
  stringstream& initResponse = SZGClient.initResponse();

  int i;
  if (argc < 3){
    cerr << argc << ": ";
    for (i=0; i<argc; ++i) {
      cerr << argv[i] << " ";
    }
    cerr << endl;
    cerr << "usage: DeviceServer driver_name driver_slot [-netinput]\n\tLegal driver names are:\n";
    for (i=0; i<numServices; ++i)
      cerr << "\t" << widgets[i].arName << endl;
    cerr << endl;
    initResponse << "usage: DeviceServer driver_name driver_slot [-netinput]\n\tLegal driver names are:\n";
    for (i=0; i<numServices; ++i)
      initResponse << "\t" << widgets[i].arName << endl;
    initResponse << endl;
    SZGClient.sendInitResponse(false);
    return 1;
  }

  int slotNumber = atoi(argv[2]);
  bool useNetInput(false);
  if (argc > 3) {
    if (std::string(argv[3]) == "-netinput") {
      useNetInput = true;
    }
  }

  arInputSource* theSource = NULL;
  // THIS IS A BUG AND A HACK!
  int iService = 0;
  arSharedLib inputSourceObject;
  // We want to load the object from the SZG_EXEC path.
  string execPath = SZGClient.getAttribute("SZG_EXEC","path");
  if (execPath == "NULL"){
    execPath = "";
  }
  if (!inputSourceObject.open(argv[1],execPath)){
    initResponse << "DeviceServer error: could not dynamically load object "
		 << "(name=" << argv[1] << ") on path=" << execPath << ".\n";
    SZGClient.sendInitResponse(false);
    return 1;
  }
  // First, make sure that we do indeed have the right type.
  arSharedObjectType sharedObjectType
    = (arSharedObjectType) inputSourceObject.sym("baseType");
  if (!sharedObjectType){
    initResponse << "DeviceServer error: could not map type function.\n";
    SZGClient.sendInitResponse(false);
    return 1;
  }
  char typeBuffer[256];
  sharedObjectType(typeBuffer, 256);
  if (strcmp(typeBuffer, "arInputSource")){
    initResponse << "DeviceServer error: dynamically loaded object "
		 << "(name=" << argv[1] << ") declares wrong type="
		 << typeBuffer << "\n";
    SZGClient.sendInitResponse(false);
    return 1;
  }
  // Get the factory function
  arInputSourceFactory factory 
    = (arInputSourceFactory) inputSourceObject.sym("factory");
  if (!factory){
    initResponse << "DeviceServer error: could not map factory function in "
		 << "object (name=" << argv[1] << ").\n";
    SZGClient.sendInitResponse(false);
    return 1;
  }
  // Can create our object.
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // The motionstar driver has a "true" argument to its constructor in the
  // original DeviceServer
  theSource = factory();
  
  if (!theSource) {
    initResponse << "DeviceServer error: failed to create input source.\n";
    SZGClient.sendInitResponse(false);
    return 1;
  }

  arInputNode inputNode;
  inputNode.addInputSource(theSource,true);
//  if (widgets[iService].netName) {
  if (useNetInput) {
    arNetInputSource* netSource = new arNetInputSource;
    netSource->setSlot(slotNumber+1);
    inputNode.addInputSource(netSource,true);
    cerr << "DeviceServer remark: using net input, slot #" << slotNumber+1 << ".\n";
    initResponse << "DeviceServer remark: using net input, slot #" << slotNumber+1 << ".\n";
    // Memory leak.  inputNode won't free its input sources, I think.
  }

  // A HACK!!! We should be able to load multiple drivers in one
  // DeviceServer instance in a general way. But, for now, this is what I'm
  // doing to get the arIdeskTracker (and arPassiveTracker) going

  // DOH! GET RID OF THIS HACK!!!!!
  //if (widgets[iService].arName == "arIdeskTracker" 
  //    || widgets[iService].arName == "arPassiveTracker"){
  //  arInputSource* additionalSource = new arJoystickDriver;
  //  inputNode.addInputSource(additionalSource,true);
  //}
  
  arNetInputSink netInputSink;
  netInputSink.setSlot(slotNumber);
  // We need some way to distinguish between different DeviceServer instances
  // (which are running different devices). This is one way to do it.
  netInputSink.setInfo(argv[1]);
  // And the sink to the input node.
  inputNode.addInputSink(&netInputSink,false);

  arFileSink fileSink;
  inputNode.addInputSink(&fileSink,false);

  // load filters if necessary

  // Add a PForth filter to beginning of chain, for remapping sensors etc.
  arPForthFilter firstFilter;
  if (!firstFilter.configure( &SZGClient ))
    return 1;
  inputNode.addFilter(&firstFilter, true);
  
  arIOFilter* filter = NULL;
  string filterName(SZGClient.getAttribute(widgets[iService].serviceName, 
                    "filter"));

  if (filterName == "arConstantHeadFilter") {
    initResponse << "DeviceServer remark: loading "
		 << "constant-head-position filter.\n";
    filter = new arConstantHeadFilter();
  }
  else if (filterName == "arTrackCalFilter") {
    initResponse << "DeviceServer remark: loading "
		 << "tracker calibration filter.\n";
    filter = new arTrackCalFilter();
  }
  else if (filterName == "arPForthFilter") {
    initResponse << "DeviceServer remark: loading "
		 << "PForth filter.\n";
    filter = new arPForthFilter(1);
  }
  else if (filterName == "arFaroCalFilter") {
    initResponse << "DeviceServer remark: loading "
		 << "FaroCal filter.\n";
    filter = new arFaroCalFilter();
  }

  if (filter) {
    if (!filter->configure( &SZGClient )){
      initResponse << "DeviceServer remark: could no configure filter.\n";
      SZGClient.sendInitResponse(false);
      return 1;
    }
    inputNode.addFilter(filter,true);
  }

 

  if (!inputNode.init(SZGClient)){
    SZGClient.sendInitResponse(false);
  }
  else{
    SZGClient.sendInitResponse(true);
  }

  if (!inputNode.start()){
    SZGClient.sendStartResponse(false);
    return 1;
  }
  else{
    SZGClient.sendStartResponse(true);
  }

  // Message task.
  string messageType, messageBody;
  while (true) {
    int sendID = SZGClient.receiveMessage(&messageType, &messageBody);
    if (!sendID){
      // sendID == 0 exactly when we are "forced" to shutdown.
      cout << "DeviceServer is shutting down.\n";
      // Cut-and-pasted from below.
      inputNode.stop();
      exit(0);
    }
    if (messageType=="quit"){
      cout << "DeviceServer remark: received shutdown message.\n";
      inputNode.stop();
      cout << "DeviceServer remark: input node has stopped.\n";
      return 0;
    }
    if (messageType=="dumpon"){
      fileSink.start();
    }
    if (messageType=="dumpoff"){
      fileSink.stop();
    }
  }

  if (filter)
    delete filter;
  return 0;
}
