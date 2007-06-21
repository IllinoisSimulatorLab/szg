//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arXMLUtilities.h"
#include "arInputFactory.h"
#include "arStringTextStream.h"
#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arNetInputSource.h"
#include "arPForthFilter.h"


using std::string;


// The input node configuration looks like this:
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



bool arInputNodeConfig::_parseTokenList(arStringTextStream& tokenStream,
                                        const string& tagType,
                                        vector<string>& tokenList) {
  // Get input_sources.
  string tagText( ar_getTagText( &tokenStream ) );
  if (tagText != tagType) {
    ar_log_error() << "arInputNodeConfig parser expected " << tagType << " tag.\n";
    return false;
  }
  arBuffer<char> buffer(128);
  if (!ar_getTextBeforeTag( &tokenStream, &buffer )) {
    ar_log_error() << "arInputNodeConfig parser failed on " << tagType << " field.\n";
    return false;
  }
  stringstream tokens( buffer.data );
  string token;
  do {
    tokens >> token;
    if (!tokens.fail()) {
      tokenList.push_back( token );
      ar_log_debug() << tagType << " arInputNodeConfig token '" << token << "'\n";
    }
  } while ( !tokens.eof() );

  // Closing input_sources tag.
  tagText = ar_getTagText( &tokenStream );
  if (tagText != "/"+tagType) {
    ar_log_error() << "arInputNodeConfig parser: expected end tag </" << tagType
      << ">, not <" << tagText << ">.\n";
    return false;
  }
  return true;
}

bool arInputNodeConfig::parseXMLRecord( const string& nodeConfig ) {
  arStringTextStream configStream( nodeConfig );
  arBuffer<char> buffer(128); 
  valid = false;

  string tagText( ar_getTagText( &configStream ) );
  if (tagText != "szg_device") {
    ar_log_error() << "arInputNodeConfig parser expected starting tag <szg_device>, not <"
                     << tagText << ">.\n";
    return false;
  }
  if (!_parseTokenList( configStream, "input_sources", inputSources )) {
    return false;
  }
  if (!_parseTokenList( configStream, "input_sinks", inputSinks )) {
    return false;
  }
  if (!_parseTokenList( configStream, "input_filters", inputFilters )) {
    return false;
  }
  
  tagText = ar_getTagText( &configStream );
  if (tagText != "pforth") {
    ar_log_error() << "arInputNodeConfig parser expected <pforth> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  if (! ar_getTextBeforeTag( &configStream, &buffer )) {
    ar_log_error() << "arInputNodeConfig parser failed in <pforth> field.\n";
    return false;
  }
  pforthProgram = string( buffer.data );
  tagText = ar_getTagText( &configStream );
  if (tagText != "/pforth") {
    ar_log_error() << "arInputNodeConfig parser expected </pforth> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  tagText = ar_getTagText( &configStream );
  if (tagText != "/szg_device") {
    ar_log_error() << "arInputNodeConfig parser expected </szg_device> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  valid = true;
  return true;
}


#ifndef AR_LINKING_STATIC

#include "arSharedLib.h"

bool arInputFactory::configure( arSZGClient& szgClient ) {
  _execPath = szgClient.getAttribute( "SZG_EXEC", "path" ); // to search for dll's
  _szgClientPtr = &szgClient;
  return true;
}

arInputSource* arInputFactory::getInputSource( const string& driverName ) {
  // A dynamically loaded library.
  arSharedLib* inputSourceSharedLib = new arSharedLib();
  if (!inputSourceSharedLib) {
    ar_log_error() << "Failed to allocate inputSourceSharedLib memory.\n";
    return NULL;
  }
  string error;
  if (!inputSourceSharedLib->createFactory( driverName, _execPath, "arInputSource", error )) {
    ar_log_error() << error;
    return NULL;
  }
  arInputSource* theSource = (arInputSource*) inputSourceSharedLib->createObject();
  delete inputSourceSharedLib;
  return theSource;
}

void arInputFactory::_printDriverNames( arLogStream& os ) {
  os << "[Cannot enumerate available drivers when dynamically linked.]\n";
}

arInputSink* arInputFactory::getInputSink( const string& sinkName ) {
  arSharedLib* inputSinkSharedLib = new arSharedLib();
  if (!inputSinkSharedLib) {
    ar_log_error() << "Failed to allocate inputSinkSharedLib memory.\n";
    return NULL;
  }
  string error;
  if (!inputSinkSharedLib->createFactory( sinkName, _execPath, "arInputSink", error )) {
    ar_log_error() << error;
    return NULL;
  }
  arInputSink* theSink = (arInputSink*) inputSinkSharedLib->createObject();
  delete inputSinkSharedLib;
  return theSink;
}

arIOFilter* arInputFactory::getFilter( const string& filterName ) {
  // A dynamically loaded library
  arSharedLib* inputFilterSharedLib = new arSharedLib();
  if (!inputFilterSharedLib) {
    ar_log_error() << "Failed to allocate inputFilterSharedLib memory.\n";
    return NULL;
  }
  string error;
  if (!inputFilterSharedLib->createFactory( filterName, _execPath, "arIOFilter", error )) {
    ar_log_error() << error;
    return NULL;
  }
  arIOFilter* theFilter = (arIOFilter*) inputFilterSharedLib->createObject();
  delete inputFilterSharedLib;
  return theFilter;
}

#else

// Various device driver headers.

#ifndef AR_USE_MINGW
#include "arJoystickDriver.h"
#include "arIntelGamepadDriver.h"
#include "arSpacepadDriver.h"
#endif

#include "arMotionstarDriver.h"
#include "arFOBDriver.h"
#include "arBirdWinDriver.h"
#include "arFaroDriver.h"
#include "arEVaRTDriver.h"
#include "arIntersenseDriver.h"
#include "arVRPNDriver.h"
#include "arReactionTimerDriver.h"
#include "arFileSource.h"
#include "arLogitechDriver.h"
#include "arPPTDriver.h"
#include "arSerialSwitchDriver.h"

struct DriverTableEntry {
  const char* arName;
  const char* serviceName;
  const char* printableName;
  const char* netName;
};
const int NUM_SERVICES = 21;
// NOTE: there is a really obnoxious kludge below... namely
// arFileSource should be able to masquerade as any of the other
// devices/ services, but it is stuck as SZG_INPUT!
const struct DriverTableEntry driverTable[NUM_SERVICES] = {
  { "arJoystickDriver",     "SZG_JOYSTICK", "joystick driver", NULL},
  { "arIntelGamepadDriver", "SZG_JOYSTICK", "intel gamepad driver", NULL},
  { "arSpacepadDriver",     "SZG_TRACKER",  "Ascension Spacepad", NULL},
  { "arIdeskTracker",       "SZG_INPUT",    "IDesk tracker", NULL},
  { "arMotionstarDriver",   "SZG_TRACKER",  "MotionStar driver", NULL},
  { "arFaroDriver",         "SZG_FARO",     "FaroArm driver", NULL},
  { "arCubeTracker",        "SZG_INPUT",    "cube tracker", "USED"},
  { "arFaroFOB",            "SZG_FAROFOB",  "FaroArm/FOB combo", "USED"},
  { "arFOBDriver",          "SZG_FOB",      "flock-of-birds driver", NULL},
  { "arBirdWinDriver",      "SZG_FOB",      "WinBird flock-of-birds driver", NULL},
  { "arFaroCalib",          "SZG_FAROCAL",  "FaroArm/MotionStar combo", "USED"},
  { "arEVaRTDriver",        "SZG_EVART",    "EVaRT driver", NULL},
  { "arFileSource",         "SZG_MOCAP",    "replay of file data", NULL},
  { "arIntersenseDriver",   "SZG_INPUT",    "intersense trackers", NULL},
  { "arVRPNDriver",         "SZG_VRPN",     "vrpn bridge", NULL},
  { "arCubeTrackWand",      "SZG_INPUT",    "cube tracker with Monowand", "USED"},
  { "arReactionTimer",      "SZG_RT",       "Reaction Timer", NULL},
  { "arPassiveTracker",     "SZG_INPUT",    "Passive Display Tracker", NULL},
  { "arLogitechDriver",     "SZG_LOGITECH", "Logitech Tracker", NULL},
  { "arPPTDriver",          "SZG_PPT",      "WorldViz PPT Tracker", "USED"},
  { "arSerialSwitchDriver", "SZG_SERIALSWITCH", "Switch on serial port send/receive pins", NULL},
};

bool arInputFactory::configure( arSZGClient& szgClient ) {
  _szgClientPtr = &szgClient;
  return true;
}

arInputSource* arInputFactory::getInputSource( const string& driverName ) {
  arInputSource* theSource = NULL;
  int iService;
  for (iService = 0; iService < NUM_SERVICES; ++iService) {
    if (driverName == string(driverTable[iService].arName)) {
      // Found a match.
      //
      // This switch() could eventually become an AbstractFactory or
      // ConcreteFactory (see Design Patterns).
      switch (iService) {
      case 0:
#ifndef AR_USE_MINGW
        theSource = new arJoystickDriver;
#else
        ar_log_error() << "arJoystickDriver not supported with g++ (MinGW compiler) on Windows.\n";
#endif
        break;
      case 1:
#ifndef AR_USE_MINGW
        theSource = new arIntelGamepadDriver;
#else
        ar_log_error() << "arIntelGamepadDriver not supported with g++ (MinGW compiler) on Windows.\n";
#endif
        break;
      case 2:
#ifndef AR_USE_MINGW
        theSource = new arSpacepadDriver;
#else
        ar_log_error() << "arSpacepadDriver not supported with g++ (MinGW compiler) on Windows.\n";
#endif
        break;
      case 3:
#ifndef AR_USE_MINGW
        theSource = new arSpacepadDriver;
#else
        ar_log_error() << "arIdeskTracker not supported with g++ (MinGW compiler) on Windows.\n";
#endif
        break;
      case 4: theSource = new arMotionstarDriver;
        break;
      case 5: theSource = new arFaroDriver;
        break;
      case 6: theSource = new arMotionstarDriver;
        break;
      case 7: theSource = new arFaroDriver;
        break;
      case 8: theSource = new arFOBDriver;
        break;
      case 9: theSource = new arBirdWinDriver;
        break;
      case 10: theSource = new arMotionstarDriver;
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
      case 20: theSource = new arSerialSwitchDriver;
        break;
      }
      break;
    }
  }
  return theSource;
}

void arInputFactory::_printDriverNames( arLogStream& os ) {
  os << "Supported input device drivers:\n";
  for (int i=0; i<NUM_SERVICES; ++i) {
    os << "\t" << driverTable[i].arName << "\n";
  }
}

arInputSink* arInputFactory::getInputSink( const string& /*sinkName*/ ) {
  ar_log_error() << "arInputFactory: No input sink dlls when statically linked.\n";
  return NULL;
}

arIOFilter* arInputFactory::getFilter( const string& /*filterName*/ ) {
  ar_log_error() << "arInputFactory: No filter dlls when statically linked.\n";
  return NULL;
}

#endif

arInputFactory::~arInputFactory() {
  _deleteSources();
}

bool arInputFactory::loadInputSources( arInputNode& inputNode,
                                        int& slotNumber,
                                        bool fNetInput ) {
  ar_log_debug() << "SLOT NUMBER: " << slotNumber << ar_endl;
  vector<string>::const_iterator iter;
  vector<string>& inputSources = _inputConfig.inputSources;
  for (iter = inputSources.begin(); iter != inputSources.end(); ++iter) {
  ar_log_debug() << "SLOT NUMBER: " << slotNumber << ar_endl;
    if (*iter == "arNetInputSource") {
      arNetInputSource* netInputSource = new arNetInputSource();
      if (!netInputSource) {
        ar_log_error() << "arInputFactory::loadInputSources out of memory.\n";
        return false;
      }
      if (!netInputSource->setSlot( slotNumber )) {
        ar_log_error() << "arInputFactory: invalid slot " << slotNumber << ".\n";
        return false;
      }
      ar_log_debug() << "arInputFactory adding arNetInputSource with slot " << slotNumber << ar_endl;
      ++slotNumber;
      inputNode.addInputSource( netInputSource, true );
    } else {
      arInputSource* theSource = getInputSource( *iter );
      if (!theSource) {
        ar_log_error() << "arInputFactory failed to create source '" << *iter << "'.\n";
        _printDriverNames( ar_log_error() );
        return false;
      }
      ar_log_debug() << "arInputFactory created source '" << *iter << "'.\n";
      _sourceNameMap[*iter] = theSource;
      inputNode.addInputSource( theSource, false );
    }
  }
  ar_log_debug() << "SLOT NUMBER: " << slotNumber << ar_endl;
  if (fNetInput){
    // Add an implicit net input source.
    arNetInputSource* implicitNetInputSource = new arNetInputSource();
    if (!implicitNetInputSource) {
      ar_log_error() << "Failed to allocate implicitNetInputSource memory.\n";
      return false;
    }
    if (!implicitNetInputSource->setSlot( slotNumber )) {
      ar_log_error() << "arInputFactory: invalid slot " << slotNumber << ".\n";
      return false;
    }
    // Skip over "slotNumber+1" listening slot.
    inputNode.addInputSource( implicitNetInputSource, true );
    ar_log_debug() << "arInputFactory added implicit arNetInputSource in slot " << slotNumber << ar_endl;
    slotNumber += 2;
  }
  return true;
}

arInputSource* arInputFactory::findInputSource( const string& driverName ) {
  const map< string, arInputSource* >::iterator iter =
        _sourceNameMap.find( driverName );
  if (iter == _sourceNameMap.end()) {
    return NULL;
  }
  return iter->second;
}

void arInputFactory::_deleteSources() {
  map< string, arInputSource* >::iterator iter;
  for (iter = _sourceNameMap.begin(); iter != _sourceNameMap.end(); ++iter) {
    if (iter->second) {
      delete iter->second;
    }
  }
  _sourceNameMap.clear();
}


bool arInputFactory::loadInputSinks( arInputNode& inputNode ) {
  // Add the dynamically loaded sinks.
  vector<string>::iterator iter;
  vector<string>& inputSinks = _inputConfig.inputSinks;
  for (iter = inputSinks.begin(); iter != inputSinks.end(); ++iter) {
    arInputSink* theSink = getInputSink( *iter );
    if (!theSink) {
      ar_log_error() << "arInputFactory failed to create input sink.\n";
      return false;
    }
    ar_log_debug() << "arInputFactory created input sink '" << *iter << ".\n";
    inputNode.addInputSink( theSink, true );
  }
  return true;
}


bool arInputFactory::loadFilters( arInputNode& inputNode, const string& namedPForthProgram ) {
  // Add a PForth filter(s) to beginning of chain, for remapping sensors etc.
  // First add a filter from the config file.
  arPForthFilter* firstFilter = new arPForthFilter();
  if (!firstFilter) {
    ar_log_error() << "Failed to allocate firstFilter memory.\n";
    return false;
  }
  ar_PForthSetSZGClient( _szgClientPtr );
  if (namedPForthProgram == "") {
    if (!firstFilter->loadProgram( _inputConfig.pforthProgram )) {
      return false;
    }
  } else {
  // Use a filter from the command line, if such was specified.
    string PForthProgram = _szgClientPtr->getGlobalAttribute( namedPForthProgram );
    if (PForthProgram == "NULL") {
      ar_log_error() << "arInputFactory: no global PForth program parameter named "
                     << namedPForthProgram << ".\n";
      return false;
    }
    if (!firstFilter->loadProgram( PForthProgram )) {
      return false;
    }
  }
  inputNode.addFilter( firstFilter, true );

  // Add the various optional filters...
  vector<string>::iterator iter;
  vector<string>& inputFilters = _inputConfig.inputFilters;
  for (iter = inputFilters.begin(); iter != inputFilters.end(); ++iter) {
    arIOFilter* theFilter = getFilter( *iter );
    if (!theFilter) {
      ar_log_error() << "arInputFactory failed to create filter.\n";
      return false;
    }
    if (!theFilter->configure( _szgClientPtr )) {
      ar_log_error() << "arInputFactory failed to configure filter.\n";
      return false;
    }
    inputNode.addFilter( theFilter, true );
    ar_log_debug() << "arInputFactory created filter '" << *iter << ".\n";
  }
  return true;
}

