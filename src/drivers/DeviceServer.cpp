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
#include "arSharedLib.h"

// Device drivers.
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
    ar_log_error() << "DeviceServer: another copy is already running (pid = " 
         << ownerID << ").\n";
LAbort:
    respond(szgClient);
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
  unsigned nextInputSlot = slotNumber + 1;
  for (iter = nodeConfig.inputSources.begin();
       iter != nodeConfig.inputSources.end(); iter++) {
    arInputSource* theSource = NULL;
    // Is the requested library embedded in the library?
    if (*iter == "arNetInputSource") {
      arNetInputSource* netInputSource = new arNetInputSource();
      if (!netInputSource->setSlot(nextInputSlot)) {
        ar_log_warning() << "DeviceServer: invalid slot " << nextInputSlot << ".\n";
        goto LAbort;
      }
      nextInputSlot++;
      inputNode.addInputSource(netInputSource, true);
    } else {
      // A dynamically loaded library.
      arSharedLib* inputSourceSharedLib = new arSharedLib();
      string error;
      if (!inputSourceSharedLib->createFactory(*iter, execPath, "arInputSource", error)) {
        ar_log_warning() << error;
        goto LAbort;
      }

      theSource = (arInputSource*) inputSourceSharedLib->createObject();
      if (!theSource) {
        ar_log_error() << "DeviceServer failed to create input source '" <<
	  *iter << "'.\n";
        goto LAbort;
      }
      ar_log_debug() << "DeviceServer created input source '" <<
	  *iter << "' in slot " << nextInputSlot-1 << ".\n";
      driverNameMap[*iter] = theSource;
      inputNode.addInputSource(theSource, false);
    }
  }

  if (fNetInput){
    // Add a net input source automatically (i.e. not via the config file)
    arNetInputSource* commandLineNetInputSource = new arNetInputSource();
    if (!commandLineNetInputSource->setSlot(nextInputSlot)) {
      ar_log_error() << "DeviceServer: invalid slot " << nextInputSlot << ".\n";
      goto LAbort;
    }
    // Skip over "nextInputSlot+1" listening slot.
    nextInputSlot += 2;
    inputNode.addInputSource(commandLineNetInputSource,true);
  }

  // Configure the input sinks. By default, include a net input sink
  // (for transmitting data) and a "file sink" (for logging).
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slotNumber)) {
    ar_log_error() << "DeviceServer: invalid slot " << slotNumber << ".\n";
    goto LAbort;
  }
  nextInputSlot++;

  // Tell netInputSink a bit about how we were launched.
  netInputSink.setInfo(argv[1]);

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

  const bool ok = inputNode.init(szgClient);
  if (!respond(szgClient, ok)){
    cerr << "DeviceServer ignoring failed init.\n";
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
      ar_log_debug() << "DeviceServer shutdown.\n";
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
      ar_log_debug() << "DeviceServer shutdown.\n";
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
