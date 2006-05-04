//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT

#include "arStringTextStream.h"
#include "arXMLUtilities.h"
#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arNetInputSource.h"
#include "arIOFilter.h"
#include "arFileSink.h"
#include "arSharedLib.h"

// Various device driver headers.
#include "arFileSource.h"
#include "arPForthFilter.h"

#include <map>

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

bool testForArgAndRemove(const string& theArg, int& argc, char** argv){
  for (int i=0; i<argc; i++){
    if (!strcmp(theArg.c_str(),argv[i])){
      // Found it
      for (int j=i; j<argc-1; j++){
        argv[j] = argv[j+1];
      } 
      argc--;
      return true;
    }
  }
  return false;
}

bool parseTokenList(arStringTextStream& tokenStream,
                    const string& tagType,
                    list<string>& tokenList){
  // Should get input_sources.
  string tagText = ar_getTagText(&tokenStream);
  arBuffer<char> buffer(128);
  if (tagText != tagType){
    ar_log_warning() << "DeviceServer parsing expected " << tagType << " tag.\n";
    return false;
  }
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
  // Look for closing input_sources tag.
  tagText = ar_getTagText(&tokenStream);
  if (tagText != "/"+tagType){
    ar_log_warning() << "DeviceServer parsing: bad end tag '" << tagType << "'.\n";
    return false;
  }
  return true;
}

class InputNodeConfig{
 public:
  InputNodeConfig(): pforthProgram(""), valid(false) {}
  ~InputNodeConfig() {}

  list<string> inputSources;
  list<string> inputSinks;
  list<string> inputFilters; 
  string       pforthProgram;
  bool         valid;
};

InputNodeConfig parseNodeConfig(const string& nodeConfig){
  InputNodeConfig result;
  arStringTextStream configStream(nodeConfig);
  arBuffer<char> buffer(128); 

  string tagText = ar_getTagText(&configStream);
  if (tagText != "szg_device"){
    ar_log_warning() << "DeviceServer parsing expected starting tag szg_device, not "
         << tagText << ".\n";
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
  
  // Should get pforth.
  tagText = ar_getTagText(&configStream);
  if (tagText != "pforth"){
    ar_log_warning() << "DeviceServer parsing expected pforth tag.\n";
    return result;
  }
  if (! ar_getTextBeforeTag(&configStream, &buffer)){
    ar_log_warning() << "DeviceServer parsing failed on pforth field.\n";
    return result;
  }
  result.pforthProgram = string(buffer.data);
  // Look for closing pforth tag.
  tagText = ar_getTagText(&configStream);
  if (tagText != "/pforth"){
    ar_log_warning() << "DeviceServer parsing expected /pforth tag.\n";
    return result;
  }
  // Look for closing /szg_device tag.
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
  arSZGClient SZGClient;
  SZGClient.simpleHandshaking(false);
  // Force the component's name because Win98 won't give the name
  // automatically.  Ascension Spacepad needs Win98.
  SZGClient.init(argc, argv, "DeviceServer");
  if (!SZGClient)
    return 1;

  // Only one instance per host.
  int ownerID = -1;
  if (!SZGClient.getLock(SZGClient.getComputerName() + "/DeviceServer", ownerID)) {
    ar_log_error() << "DeviceServer: another copy is already running (pid = " 
         << ownerID << ").\n";
    return 1;
  }

  stringstream& initResponse = SZGClient.initResponse();

  // If "simple" flag (-s),
  // just load the module and publish its output without fancy filtering.
  const bool simpleOperation = testForArgAndRemove("-s", argc, argv);
  const bool useNetInput = testForArgAndRemove("-netinput", argc, argv);

  if (argc < 3){
    initResponse << "usage: DeviceServer [-s] [-netinput] device_description "
		 << "driver_slot [pforth_filter_name]" << endl;
LAbort:
    respond(SZGClient);
    return 1;
  }

  const int slotNumber = atoi(argv[2]);
  InputNodeConfig nodeConfig;
  if (simpleOperation){
    // As command-line flats, specify only the driver and slot.
    nodeConfig.inputSources.push_back(argv[1]);
  }
  else{
    const string& config = SZGClient.getGlobalAttribute(argv[1]);
    if (config == "NULL") {
      initResponse << "DeviceServer: undefined global node (i.e., <param> in dbatch file) '" << argv[1] << "'.\n";
      goto LAbort;
    }
    nodeConfig = parseNodeConfig(config);
    if (!nodeConfig.valid){
      initResponse << "DeviceServer: misconfigured global node (i.e., <param> in dbatch file) '"
	<< argv[1] << "'\n";
      goto LAbort;
    }
  }

  std::map< std::string, arInputSource* > driverNameMap;

  // Configure the input node.
  arInputNode inputNode;

  const string execPath = SZGClient.getAttribute("SZG_EXEC","path"); // search for dll's
  // Start with the input sources.
  // Assign input "slots" to the input sources.
  int nextInputSlot = slotNumber + 1;
  // Configure the input sources.
  list<string>::iterator iter;
  for (iter = nodeConfig.inputSources.begin();
       iter != nodeConfig.inputSources.end(); iter++){
    arInputSource* theSource = NULL;
    // Check if the requested library is embedded in the library.
    if (*iter == "arNetInputSource"){
      arNetInputSource* netInputSource = new arNetInputSource();
      if (!netInputSource->setSlot(nextInputSlot)) {
	initResponse << "DeviceServer: invalid slot " << nextInputSlot << ".\n";
	goto LAbort;
      }
      nextInputSlot++;
      inputNode.addInputSource(netInputSource, true);
    } 
    else{
      // A dynamically loaded library
      arSharedLib* inputSourceObject = new arSharedLib();
      string error;
      if (!inputSourceObject->createFactory(*iter, execPath, 
                                            "arInputSource", error)){
        initResponse << error;
        goto LAbort;
      }
      // Can create our object.
      theSource = (arInputSource*) inputSourceObject->createObject();
      if (!theSource) {
        initResponse << "DeviceServer failed to create input source '" <<
	  *iter << "'.\n";
        goto LAbort;
      }
      driverNameMap[*iter] = theSource;
      inputNode.addInputSource(theSource, false);
    }
  }

  // See if, via command line arg -netinput, we want to
  // add a net input source automatically (i.e. not via the config file)
  if (useNetInput){
    arNetInputSource* commandLineNetInputSource = new arNetInputSource();
    if (!commandLineNetInputSource->setSlot(nextInputSlot)) {
      initResponse << "DeviceServer: invalid slot " << nextInputSlot << ".\n";
      goto LAbort;
    }
      nextInputSlot++;
    nextInputSlot++;
    // The node will "own" this source.
    inputNode.addInputSource(commandLineNetInputSource,true);
  }

  // Configure the input sinks. NOTE: by default, we include a net input
  // sink (for transmitting data from the DeviceServer) and a "file sink"
  // for logging.
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slotNumber)) {
    initResponse << "DeviceServer: invalid slot " << slotNumber << ".\n";
    goto LAbort;
  }
      nextInputSlot++;
  // Distinguish between different DeviceServer instances
  // (which are running different devices).  (Still needed, since multiple copies per host are forbidden?)
  netInputSink.setInfo(argv[1]);
  // And the sink to the input node.
  inputNode.addInputSink(&netInputSink,false);

  arFileSink fileSink;
  inputNode.addInputSink(&fileSink,false);

  // Add the dynamically loaded sinks.
  for (iter = nodeConfig.inputSinks.begin();
       iter != nodeConfig.inputSinks.end(); iter++){
    arInputSink* theSink = NULL;
    // A dynamically loaded library
    arSharedLib* inputSinkObject = new arSharedLib();
    string error;
    if (!inputSinkObject->createFactory(*iter, execPath, 
                                        "arInputSink", error)){
      initResponse << error;
      goto LAbort;
    }
    // Can create our object.
    theSink = (arInputSink*) inputSinkObject->createObject();
    if (!theSink) {
      initResponse << "DeviceServer failed to create input sink.\n";
      goto LAbort;
    }
    inputNode.addInputSink(theSink,true);
  }

  // Load the filters.

  // Add a PForth filter(s) to beginning of chain, for remapping sensors etc.
  // First add a filter from the config file.
  arPForthFilter firstFilter;
  ar_PForthSetSZGClient( &SZGClient );
  if (!firstFilter.loadProgram( nodeConfig.pforthProgram )){
    return 1;
  }
  // The PForth filter is owned by the program, since it is declared 
  // statically.
  inputNode.addFilter(&firstFilter, false);

  // Next, add a filter from the command line, if such was specified.
  if (argc >= 4){
    string commandLineProgram = 
      SZGClient.getGlobalAttribute(argv[3]);
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
  
  // Add the various optional filters...
  for (iter = nodeConfig.inputFilters.begin();
       iter != nodeConfig.inputFilters.end(); iter++){
    arIOFilter* theFilter = NULL;
    // A dynamically loaded library
    arSharedLib* inputFilterSharedLib = new arSharedLib();
    string error;
    if (!inputFilterSharedLib->createFactory(*iter, execPath, 
                                          "arIOFilter", error)){
      initResponse << error;
      goto LAbort;
    }
    theFilter = (arIOFilter*) inputFilterSharedLib->createObject();
    if (!theFilter) {
      initResponse << "DeviceServer failed to create input filter.\n";
      goto LAbort;
    }
    if (!theFilter->configure( &SZGClient )){
      initResponse << "DeviceServer failed to configure filter.\n";
      goto LAbort;
    }
    // We own this.
    inputNode.addFilter(theFilter,true);
  }

  const bool ok = inputNode.init(SZGClient);
  if (!respond(SZGClient, ok)){
    ar_log_warning() << "DeviceServer ignoring failed init.\n";
    // return 1;
  }
  if (!ok) {
    // Bug: in linux, this may hang.  Which other thread still runs?
    return 1; // init failed
  }

  if (!inputNode.start()){
    if (!SZGClient.sendStartResponse(false))
      cerr << "DeviceServer error: maybe szgserver died.\n";
    return 1;
  }
  if (!SZGClient.sendStartResponse(true))
    cerr << "DeviceServer error: maybe szgserver died.\n";

  // Message task.
  string messageType, messageBody;
  while (true) {
    const int sendID = SZGClient.receiveMessage(&messageType, &messageBody);
    if (!sendID){
      // sendID == 0 exactly when we are "forced" to shutdown.
      // Copypaste from below.
      inputNode.stop();
      ar_log_remark() << "DeviceServer shutdown.\n";
      return 0;
    }

    if (messageType=="quit"){
      inputNode.stop();
      std::map< std::string, arInputSource* >::iterator killIter;
      for (killIter = driverNameMap.begin(); killIter != driverNameMap.end(); ++killIter) {
        if (killIter->second)
          delete killIter->second;
      }
      driverNameMap.clear();
      ar_log_remark() << "DeviceServer shutdown.\n";
      return 0;
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

  return 0;
}
