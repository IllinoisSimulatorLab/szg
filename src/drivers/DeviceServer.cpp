//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"

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

class InputNodeConfig{
 public:
  InputNodeConfig(){ pforthProgram = ""; valid = false; }
  ~InputNodeConfig(){}

  list<string> inputSources;
  list<string> inputSinks;
  list<string> inputFilters; 
  string       pforthProgram;
  bool         valid;
};

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
    cout << "DeviceServer parsing error: expected " << tagType << " tag.\n";
    return false;
  }
  if (! ar_getTextBeforeTag(&tokenStream, &buffer)){
    cout << "DeviceServer parsing error: failed on " << tagType << " field.\n";
    return false;
  }
  stringstream tokens(buffer.data);
  string token;
  while (true){
    tokens >> token;
    if (!tokens.fail()){
      tokenList.push_back(token);
      cout << tagType << " token = " << token << "\n";
    }
    if (tokens.eof()){
      break;
    }
  }
  // Look for closing input_sources tag.
  tagText = ar_getTagText(&tokenStream);
  if (tagText != "/"+tagType){
    cout << "DeviceServer parsing error: failed on " << tagType 
         << ") end tag.\n";
    return false;
  }
  return true;
}

InputNodeConfig parseNodeConfig(const string& nodeConfig){
  InputNodeConfig result;
  arStringTextStream configStream(nodeConfig);
  arBuffer<char> buffer(128); 
  // Look for starting szg_device tag.
  string tagText = ar_getTagText(&configStream);
  if (tagText != "szg_device"){
    cout << "DeviceServer parsing error: got incorrect opening tag.\n";
    result.valid = false;
    return result;
  }
  
  if (!parseTokenList(configStream, "input_sources", result.inputSources)){
    result.valid = false;
    return result;
  }
  if (!parseTokenList(configStream, "input_sinks", result.inputSinks)){
    result.valid = false;
    return result;
  }
  if (!parseTokenList(configStream, "input_filters", result.inputFilters)){
    result.valid = false;
    return result;
  }
  
  // Should get pforth.
  tagText = ar_getTagText(&configStream);
  if (tagText != "pforth"){
    cout << "DeviceServer parsing error: expected pforth tag.\n";
    result.valid = false;
    return result;
  }
  if (! ar_getTextBeforeTag(&configStream, &buffer)){
    cout << "DeviceServer parsing error: failed on pforth field.\n";
    result.valid = false;
    return result;
  }
  result.pforthProgram = string(buffer.data);
  cout << "PForth program = " << result.pforthProgram << "\n";
  // Look for closing pforth tag.
  tagText = ar_getTagText(&configStream);
  if (tagText != "/pforth"){
    cout << "DeviceServer parsing error: failed on pforth end tag.\n";
    result.valid = false;
    return result;
  }
  // Look for closing /szg_device tag.
  tagText = ar_getTagText(&configStream);
  if (tagText != "/szg_device"){
    cout << "DeviceServer parsing error: failed on szg_device end tag.\n";
    result.valid = false;
    return result;
  }
  result.valid = true;
  return result;
}

int main(int argc, char** argv){
  arSZGClient SZGClient;
  SZGClient.simpleHandshaking(false);
  // note how we force the name of the component. This is because it is
  // impossible to get the name automatically on Win98 and we want to run
  // DeviceServer on Win98 (for instance, to support legacy spacepads...)
  SZGClient.init(argc, argv, "DeviceServer");
  if (!SZGClient)
    return 1;
  stringstream& initResponse = SZGClient.initResponse();

  int i;

  // First, check to see if we've got the "simple" flag set (-s)
  // If so, we aren't planning on doing any filtering or other such,
  // just loading in the module and putting its output on the network.
  bool simpleOperation = testForArgAndRemove("-s", argc, argv);
  bool useNetInput = testForArgAndRemove("-netinput", argc, argv);

  if (argc < 3){
    initResponse << "usage: DeviceServer [-s] [-netinput] device_description "
		 << "driver_slot [pforth_filter_name]" << endl;
    SZGClient.sendInitResponse(false);
    return 1;
  }

  int slotNumber = atoi(argv[2]);
  InputNodeConfig nodeConfig;
  if (!simpleOperation){
    nodeConfig = parseNodeConfig(SZGClient.getGlobalAttribute(argv[1]));
    if (!nodeConfig.valid){
      initResponse << "DeviceServer error: got invalid node config from "
		   << "attribute " << argv[1] << "\n";
      SZGClient.sendInitResponse(false);
    }
  }
  else{
    // NOTE: In simple operation, we only specify the driver on the command 
    // line.
    nodeConfig.inputSources.push_back(argv[1]);
  }

  // Configure the input node.
  arInputNode inputNode;
  // Need to know from where the shared lilbraries will be loaded.
  // (The SZG_EXEC path)
  string execPath = SZGClient.getAttribute("SZG_EXEC","path");
  // Start with the input sources.
  // Necessary to assign input "slots" to the input sources.
  int nextInputSlot = slotNumber + 1;
  // First, see if, via command line arg -netinput, we want to automatically
  // add a net input source (i.e. not via the config file)
  if (useNetInput){
    arNetInputSource* commandLineNetInputSource = new arNetInputSource();
    commandLineNetInputSource->setSlot(nextInputSlot);
    nextInputSlot++;
    // The node will "own" this source.
    inputNode.addInputSource(commandLineNetInputSource,true);
  }
  // Configure the input sources.
  list<string>::iterator iter;
  for (iter = nodeConfig.inputSources.begin();
       iter != nodeConfig.inputSources.end(); iter++){
    arInputSource* theSource = NULL;
    // Must check to see whether the requested library is embedded in the
    // library or not.
    if (*iter == "arNetInputSource"){
      arNetInputSource* netInputSource = new arNetInputSource();
      netInputSource->setSlot(nextInputSlot);
      nextInputSlot++;
	  inputNode.addInputSource(netInputSource,true);
    }
    else{
      // A dynamically loaded library
      arSharedLib* inputSourceObject = new arSharedLib();
      string error;
      if (!inputSourceObject->createFactory(*iter, execPath, 
                                            "arInputSource", error)){
        initResponse << error;
        SZGClient.sendInitResponse(false);
        return 1;
      }
      // Can create our object.
      theSource = (arInputSource*) inputSourceObject->createObject();
      if (!theSource) {
        initResponse << "DeviceServer error: failed to create input source.\n";
        SZGClient.sendInitResponse(false);
        return 1;
      }
      inputNode.addInputSource(theSource,true);
    }
  }

  // Configure the input sinks. NOTE: by default, we include a net input
  // sink (for transmitting data from the DeviceServer) and a "file sink"
  // for logging.
  arNetInputSink netInputSink;
  netInputSink.setSlot(slotNumber);
  // We need some way to distinguish between different DeviceServer instances
  // (which are running different devices). This is one way to do it.
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
      SZGClient.sendInitResponse(false);
      return 1;
    }
    // Can create our object.
    theSink = (arInputSink*) inputSinkObject->createObject();
    if (!theSink) {
      initResponse << "DeviceServer error: failed to create input sink.\n";
      SZGClient.sendInitResponse(false);
      return 1;
    }
    inputNode.addInputSink(theSink,true);
  }

  // Go ahead and load the filters.

  // Add a PForth filter(s) to beginning of chain, for remapping sensors etc.
  // We first add a filter from the config file.
  arPForthFilter firstFilter;
  ar_PForthSetSZGClient( &SZGClient );
  if (!firstFilter.configure( nodeConfig.pforthProgram )){
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
      cout << "DeviceServer remark: no program of name " << argv[3]
	   << " exists.\n";
    }
    else{
      arPForthFilter* commandLineFilter = new arPForthFilter();
      if (!commandLineFilter->configure( commandLineProgram )){
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
    arSharedLib* inputFilterObject = new arSharedLib();
    string error;
    if (!inputFilterObject->createFactory(*iter, execPath, 
                                          "arIOFilter", error)){
      initResponse << error;
      SZGClient.sendInitResponse(false);
      return 1;
    }
    // Can create our object.
    theFilter = (arIOFilter*) inputFilterObject->createObject();
    if (!theFilter) {
      initResponse << "DeviceServer error: failed to create input filter.\n";
      SZGClient.sendInitResponse(false);
      return 1;
    }
    if (!theFilter->configure( &SZGClient )){
      initResponse << "DeviceServer remark: could not configure filter.\n";
      SZGClient.sendInitResponse(false);
      return 1;
    }
    // We do, indeed, own this.
    inputNode.addFilter(theFilter,true);
  }

  SZGClient.sendInitResponse(inputNode.init(SZGClient));
  if (!inputNode.start()){
    SZGClient.sendStartResponse(false);
    return 1;
  }
  SZGClient.sendStartResponse(true);

  // Message task.
  string messageType, messageBody;
  while (true) {
    const int sendID = SZGClient.receiveMessage(&messageType, &messageBody);
    if (!sendID){
      // sendID == 0 exactly when we are "forced" to shutdown.
      cout << "DeviceServer remark: shutdown.\n";
      // Cut-and-pasted from below.
      inputNode.stop();
      exit(0);
    }
    if (messageType=="quit"){
      cout << "DeviceServer remark: got shutdown message.\n";
      inputNode.stop();
      cout << "DeviceServer remark: input node stopped.\n";
      return 0;
    }
    else if (messageType=="dumpon"){
      fileSink.start();
    }
    else if (messageType=="dumpoff"){
      fileSink.stop();
    }
  }

  return 0;
}
