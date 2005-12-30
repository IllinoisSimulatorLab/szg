//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arPhleetConfigParser.h"
#include "arXMLUtilities.h"
#include "arXMLParser.h"
#include "arLogStream.h"

#include <stdio.h>

void arSZGClientServerResponseThread(void* client){
  ((arSZGClient*)client)->_serverResponseThread();
}

void arSZGClientTimerThread(void* client){
  ((arSZGClient*)client)->_timerThread();
}

void arSZGClientDataThread(void* client){
  ((arSZGClient*)client)->_dataThread();
}

arSZGClient::arSZGClient():
  _IPaddress("NULL"),
  _port(-1),
  _serverName("NULL"),
  _computerName("NULL"),
  _userName("NULL"),
  _networks("NULL"),
  _addresses("NULL"),
  _graphicsNetworks("NULL"),
  _graphicsAddresses("NULL"),
  _soundNetworks("NULL"),
  _soundAddresses("NULL"),
  _inputNetworks("NULL"),
  _inputAddresses("NULL"),
  _mode("component"),
  _graphicsMode("SZG_DISPLAY0"),
  _parameterFileName("szg_parameters.txt"),
  _virtualComputer("NULL"),
  _connected(false),
  _receiveBuffer(NULL),
  _receiveBufferSize(15000),
  _launchingMessageID(0),
  _dexHandshaking(false),
  _simpleHandshaking(true),
  _parseSpecialPhleetArgs(true),
  _initialInitLength(0),
  _initialStartLength(0),
  _nextMatch(0),
  _beginTimer(false),
  _requestedName(""),
  _dataRequested(false),
  _keepRunning(true),
  _justPrinting(false),
  _logLevel(AR_LOG_ERROR)
{
  // temporary... this will be overwritten in init(...)
  _exeName.assign("Syzygy client");

  _dataClient.setLabel(_exeName);
  _dataClient.smallPacketOptimize(true);
  _dataClient.setBufferSize(_receiveBufferSize);
  _receiveBuffer = new ARchar[_receiveBufferSize];

  ar_mutex_init(&_queueLock);
  _discoveryThreadsLaunched = false;

  ar_mutex_init(&_serviceLock);
}

arSZGClient::~arSZGClient(){
  _keepRunning = false;

  /// \todo add handshaking to tell the server to quit the read and
  /// close the socket at its end, so we can close without blocking on
  /// our data thread's blocking read (e.g. IRIX).
  //_dataClient.closeConnection();

  delete [] _receiveBuffer;
}

/// Lets the programmer control the complexity of a phleet component's
/// handshaking when it has been invoked by "dex". The default is "true",
/// under which state the arSZGClient sends a final response during its
/// init method. Otherwise, if simple handshaking has been disabled (by
/// passing in "false" before init), the programmer will need to
/// send an init response using sendInitResponse and a start response using
/// sendStartResponse (which is the final response in this case). This allows
/// the programmer to pass initialization and start logs directly back to
/// the spawning "dex", which is very important for cluster manageability.
void arSZGClient::simpleHandshaking(bool state){
  _simpleHandshaking = state;
}

/// For the most part, Phleet components SHOULD NOT be able to see the
/// "special" phleet args (like -szg virtual=cube). These args are used
/// to shape the way phleet components behave (like acting as part of
/// a particular virtual computer or in a particular mode). However,
/// certain components, like dex, MUST be able to see the args, so
/// that they can be passed-on. With dex, _parseSpecialPhleetArgs must
/// be set to "false" (the default is true).
void arSZGClient::parseSpecialPhleetArgs(bool state){
  _parseSpecialPhleetArgs = state;
}

/// Sets up the client's connection to the phleet. Should be called very, very
/// soon after main(...).
/// @param argc Should be passed from main's argc
/// @param argv Should be passed from main's argv
/// @param forcedName An optional parameter. Ideally, we'd like to be able to
/// read the exe name from the command line parameters, but this fails to work
/// on Windows 98, where one gets, at best, a name in all caps. Consequently,
/// the few (support) components that might run on Win98 (like szgd,
/// DeviceServer, and SoundRender) all force their names. Note that a warning
/// is printed if the forced name and the name scraped from the command line
/// do not match.
bool arSZGClient::init(int& argc, char** argv, string forcedName){
  // Set the name of the component.
  // Do this using the command-line args, since some of the component management
  // occurs via names.
  _exeName = string(argv[0]);
  // Remove the .EXE suffix on Win32. On other OS's 
  _exeName = ar_stripExeName(_exeName);
  
  // On the Unix side, we might need to finish a handshake with the
  // szgd, telling it we've been successfully forked
  // THIS MUST OCCUR BEFORE dialUpFallThrough.

  // ANOTHER IMPORTANT NOTE: IT IS ASSUMED THAT THIS FUNCTION WILL ALWAYS,
  // REGARDLESS OF WHETHER OR NOT IT IS SUCCESSFUL, FINISH THE HANSHAKE WITH
  // DEX!!! Conequently, no return statements, except at the end.
  bool success = true; // assume suceess unless there has been a
                       // specific failure
  const string pipeIDString(ar_getenv("SZGPIPEID"));
  if (pipeIDString != "NULL"){
    _dexHandshaking = true;
    // we have, in fact, been successfully spawned on the Unix side
    const int pipeID = atoi(pipeIDString.c_str());
    // Send the success code. Note that on Win32 we set the pipe ID to -1,
    // which gets us into here (to set _dexHandshaking to true...
    // but we don't want to try to write to the pipe on Win32 since the
    // function is unimplemented
    if (pipeID >= 0){
      char numberBuffer[8] = "\001";
      if (!ar_safePipeWrite(pipeID, numberBuffer, 1)){
        ar_log_error() << _exeName << " error: unterminated pipe-based handshake.\n";
      }
    }
  }

  // The phleet config file contains a list of networks through which
  // we can communicate. various programs will wish to obtain a list
  // of networks to communicate in a uniform way. The list may need to
  // be altered from the default (all of them) to shape network traffic
  // patterns (for instance, graphics info should go exclusively through
  // a high-speed private network). The "context", as stored in SZGCONTEXT
  // can be used to do this, as can command-line args. Other properties,
  // like virtual computer and mode, can also be manipulated through these
  // means. SZGCONTEXT overwrites the command-line args. NOTE: the reason
  // argc is passed by reference is that the special Phleet args that
  // manipulate these values are stripped so as not to confuse programs.
  
  // NOTE: we need to attempt to parse the config file BEFORE parsing the
  // Phleet args (and the "context") so that the _networks and _addresses can be set.
  // If these files cannot be read, it is still possible to recover.
  (void) _configParser.parseConfigFile();
  (void) _configParser.parseLoginFile();
  // If the networks and addresses were NOT set via command line args or
  // environment variables, go ahead and set them now.
  // NOTE: there are different "channels" to allow different types of
  // network traffic to be routed differently (default (which is
  // represented by _networks and _addresses), graphics, sound, and input).
  // The graphics, sound, and input channels have networks/addresses set
  // down in the _parseSpecialPhleetArgs and _parseContext.

  // From the computer-wide config file (if such existed) and the user's login file.
  _networks = _configParser.getNetworks();           // can override
  _addresses = _configParser.getAddresses();         // can override
  _computerName = _configParser.getComputerName();   // *cannot* override
  // From the per-user login file, if such existed.
  _serverName   = _configParser.getServerName();     // *cannot* override
  _IPaddress    = _configParser.getServerIP();       // can override
  _port         = _configParser.getServerPort();     // can override
  _userName     = _configParser.getUserName();       // can override

  // Any present special Phleet args are removed in the _parsePhleetArgs
  // call. These can override some of the member variables set above.
  if (!_parsePhleetArgs(argc, argv)){
    _initResponseStream << _exeName << " error: invalid Phleet args.\n";
    // NOTE: it isn't strictly true that we are not connected.
    // However, if this happens, we want the component to quit,
    // which it will only do if _connected is false.
    _connected = false;
    success = false; // do not return yet
  }

  // These can override some of the variables set above. Note that the "context"
  // trumps command line args always.
  if (!_parseContext()){
    _initResponseStream << _exeName << " error: invalid Phleet context.\n";
    // NOTE: it isn't strictly true that we are not connected.
    // However, if this happens, we want the component to quit,
    // which it will only do if _connected is false.
    _connected = false;
    success = false; // do not return yet
  }
  
  // The special SZGUSER environment variable trumps everything else, if set!
  // This variable is used by szgd!
  const string userNameOverride = ar_getenv("SZGUSER");
  if (userNameOverride != "NULL"){
    // ar_getenv returns "NULL" if the environment var is not set.
    _userName = userNameOverride;
  }
  
  if ( _IPaddress == "NULL" || _port == -1 || _userName == "NULL" ){
    // The information to attempt a login attempt is not available.
    // We must be operating in standalone mode.
    ar_log_critical() << _exeName << " remark: RUNNING IN STANDALONE MODE. NO DISTRIBUTION.\n"
                      << "  (to run in Phleet mode, you must dlogin)\n";
    _connected = false;
    success = true;
    
    // Do not print out a warning if the file cannot be found!
    if (!parseParameterFile(_parameterFileName, false)){
      // Failed to find the specified parameter file. Go ahead and try to find
      // a file specified by environment variable SZG_PARAM.
      string possibleFileName = ar_getenv("SZG_PARAM");
      if (possibleFileName != "NULL"){
	// Do not print out a warning if the file cannot be found.
	parseParameterFile(possibleFileName, false);
      }
    }
  }
  else{
    // We are supposed to be operating in Phleet mode. Don't print out a 
    // message stating that, since this is the *normal* mode of operation.
    
    // This needs to go after phleet args parsing, etc. It could be that we
    // specify where the server is, what the user name is, etc. via command
    // line args or the "context".
    if (!_dialUpFallThrough()) {
      // Could not connect to the specified szgserver. This is a fatal error!
      // Don't complain here -- dialUpFallThrough() already did.
      _connected = false;
      success = false; // do not return yet
    }
    else{
      // Successfully connected to the szgserver. 
      // 1.Tell it our name.
      // 2. Put headers onto the init and start response streams.
      // 3. Handshake back with dex (if this program was launched by szgd).
      _connected = true;
      success = true; // do not return yet
      
      // The connection was a success. Set our process "label" with the szgserver.
      if (forcedName == "NULL"){
	// This is the default for the forcedName parameter. We are not trying
	// to force the name.
	// Tell szgserver our name.
	_setLabel(_exeName); 
      }
      else{
	// we are indeed trying to force the name. Go ahead and print out a
	// warning, though, if there is a difference.
	if (forcedName != _exeName){
	  ar_log_warning() << _exeName << " warning: forcing a component name\n"
	                   << "different from the exe name (necessary only in Win98).\n";
	}
	// This method also changes _exeName internally.
	// Tell the szgserver our name
	_setLabel(forcedName); 
      }
      
      // Pack the init stream and the start stream with headers.
      // These include important configuration information (like the "context").
      _initResponseStream << _generateLaunchInfoHeader();
      _initialInitLength = _initResponseStream.str().length();
      _startResponseStream << _generateLaunchInfoHeader();
      _initialStartLength = _startResponseStream.str().length();
      
      if (_dexHandshaking){
	// Shake hands with dex.
	// Regardless of whether we are on Unix or Win32, we need to begin
	// responding to the execute message, if it was passed-through dex
	// this lets dex know that the executable did, indeed, launch.
	string tradingKey = getComputerName() + "/"
	+ ar_getenv("SZGTRADINGNUM") + "/"
	+ _exeName;
	// Take control of the right to respond to the launching message.
	_launchingMessageID = requestMessageOwnership(tradingKey);
	// Important that the object fails here if it can't get the message
	// ownership trade. Perhaps we were late starting and the szgd has
	// already decided that we won't actually launch.
	if (!_launchingMessageID){
	  ar_log_error() << _exeName << " error: failed to own message, "
	                 << "despite appearing to have been launched by szgd.\n";
	  return false;
	}
	else{
	  // Send the message response.
	  // NOTE: if we are doing simple handshaking (the default), we send a complete response
	  // if we are not doing simple handshaking (for apps that want to send a start
	  // response, we send a partial response (i.e. there will be more responses).
	  if (!messageResponse(_launchingMessageID,
			       _generateLaunchInfoHeader() +
			       _exeName + string(" launched.\n"),
			       !_simpleHandshaking)){
	    ar_log_error() << _exeName
	                   << " warning: response failed during launch.\n";
	  }
	}
      }
    }
  }

  return success;
}

/// Common core of sendInitResponse() and sendStartResponse().
bool arSZGClient::_sendResponse(stringstream& s, 
				const char* sz,
				int initialStreamLength,
                                bool ok, 
				bool fNotFinalMessage) {
  // We might output to the terminal below. However, only do this if there has been new
  // stuff after the header.
  bool printInfo = s.str().length() > initialStreamLength ? true : false;
  // Append a standard success or failure message to the stream.
  s << _exeName << " component " << sz << (ok ? " ok.\n" : " failed.\n");
  if (_dexHandshaking && !_simpleHandshaking){
    // Another message to dex.
    if (!messageResponse(_launchingMessageID, s.str(), fNotFinalMessage)){
      ar_log_warning() << _exeName
	               << " warning: response failed during " << sz << ".\n";
      return false;
    }
  }
  else{
    // Nowhere to send the message. Might as well go to the terminal.
    // This MUST go to cout and not to the logging stream (it'll just disappear in this case).
    if (printInfo){
      cout << s.str();
    }
  }
  return true;
}

/// If we have launched via szgd/dex, send the init message stream
/// back to the launching dex command. If we launched from the command line,
/// just print the stream. If "ok" is true, the init succeeded
/// and we'll be sending a start message later (so this will be a partial
/// response). Otherwise, the init failed and we'll
/// not be sending another response, so this should be the final one.
bool arSZGClient::sendInitResponse(bool ok){
  return _sendResponse(_initResponseStream, "initialization", _initialInitLength, ok, ok);
}

/// If we have launched via szgd/dex, send the start message stream
/// back to the launching dex command. If we launched from the command line,
/// just print the stream. This is the final response to the launch message
/// regardless, though the parameter does alter the message sent or printed.
bool arSZGClient::sendStartResponse(bool ok){
  return _sendResponse(_startResponseStream, "start", _initialStartLength, ok, false);
}

void arSZGClient::closeConnection(){
  _dataClient.closeConnection();
  _connected = false;
}

bool arSZGClient::launchDiscoveryThreads(){
  if (!_keepRunning){
    // Threads would immediately terminate, if we tried to start them.
    ar_log_warning() << _exeName
                     << " warning: terminating, so no discovery threads launched.\n";
    return false;
  }
  if (_discoveryThreadsLaunched){
    ar_log_warning() << _exeName << " warning: ignoring relaunch of discovery threads.\n";
    return true;
  }

  // Initialize the server discovery socket
  _discoverySocket = new arUDPSocket;
  _discoverySocket->ar_create();
  _discoverySocket->setBroadcast(true);
  _discoverySocket->reuseAddress(true);
  arSocketAddress incomingAddress;
  incomingAddress.setAddress(NULL,4620);
  if (_discoverySocket->ar_bind(&incomingAddress) < 0){
    ar_log_error() << "arSZGClient error: failed to bind discovery response socket.\n";
    return false;
  }

  arThread dummy1(arSZGClientServerResponseThread,this);
  arThread dummy2(arSZGClientTimerThread,this);
  _discoveryThreadsLaunched = true;
  return true;
}

// copypasted from getAttribute()
string arSZGClient::getAllAttributes(const string& substring){
  // NOTE: THIS DOES NOT WORK WITH THE LOCAL PARAMETER FILE!!!!
  if (!_connected){
    return string("NULL");
  }

  // Request ALL parameters (in dbatch-able form) or
  // request only those parameters that match the given substring.
  const string type = substring=="ALL" ? "ALL" : "substring";

  arStructuredData* getRequestData
    = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  // Must use match for thread safety.
  const int match = _fillMatchField(getRequestData);
  string result;
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,substring) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,type) ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !_dataClient.sendData(getRequestData)){
    ar_log_warning() << _exeName << " warning: failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(getRequestData);
  return result;
}

// This is called only when parsing the assign blocks in the dbatch files.
bool arSZGClient::parseAssignmentString(const string& text){
  stringstream parsingStream(text);
  string param1, param2, param3, param4;
  while (true){
    // Will skip whitespace(this is a default)
    parsingStream >> param1;
    if (parsingStream.fail()){
      if (parsingStream.eof()){
	// End of assignment block, so it's okay.
	return true;
      }
      goto LFail;
    }
    parsingStream >> param2;
    if (parsingStream.fail())
      goto LFail;
    parsingStream >> param3;
    if (parsingStream.fail())
      goto LFail;
    parsingStream >> param4;
    if (parsingStream.fail()){
LFail:
      ar_log_error() << "arSZGClient error: malformed assignment string:\n";
      ar_log_error() << "----------------\n" << text << "----------------" << ar_endl;
      return false;
    }
    if (param2.substr(0,10) == "SZG_SCREEN"){
      ar_log_error() << "arSZGClient warning: are you sure you want to use SZG_SCREEN "
	             << "parameters?\n";
      ar_log_error() << "  THESE ARE OBSOLETE FOR VERSIONS PAST 0.7!\n";
    }
    setAttribute(param1, param2, param3, param4);
  }
}

/// Sometimes we want to be able to read in parameters from a file, as in
/// dbatch or when starting a program in "standalone" mode (i.e. when it is
/// not connected to the Phleet).
bool arSZGClient::parseParameterFile(const string& fileName, bool warn){
  const string dataPath(getAttribute("SZG_SCRIPT","path"));
  // There are two parameter file formats.
  // The legacy format consists of a sequence of lines as follows:
  //
  //   computer parameter_group parameter parameter_value
  //
  // While this format cannot express some of the more XML-y kinds of
  // things that the "global" attributes (like an input node description)
  // require. Consequently,
  //
  // <szg_config>
  //   ... one of more of the following ...
  //   <param>
  //      <name>
  //
  //      </name>
  //	  <value>
  //
  //      </value>
  //   </param>
  //   <include>
  //    ... a file name with more config info goes here.
  //   </include>
  //
  //   <comment>
  //     ... text of comment goes here ...
  //   </comment>
  //   <assign>
  //     a1 a2 a3 a4
  //     b1 b2 b3 b4
  //     c1 c2 c3 c4
  //      ...
  //   </assign>
  // </szg_config>
  //
  //
  // The presence of <szg_config> as the first non-whitespace block of
  // characters in the file determines the config file format that is
  // assumed.

  // First, try the new and improved XML way!
  arFileTextStream fileStream;
  // Only complain if asked via warn.
  if (!fileStream.ar_open(fileName, dataPath)){
    if (warn){
      ar_log_warning() << _exeName << " error: failed to open batch file "
	               << fileName << "\n";
    }
    return false;
  }
  ar_log_remark() << "arSZGClient remark: parsing config file " 
                  << ar_fileFind(fileName, "", dataPath) << ".\n";
  arBuffer<char> buffer(128);
  string tagText = ar_getTagText(&fileStream, &buffer);
  if (tagText == "szg_config"){
    // Try parsing in the new way.
    while (true){
      tagText = ar_getTagText(&fileStream, &buffer);
      if (tagText == "comment"){
	// Go ahead and get the comment text (but discard it).
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
	  ar_log_error() << _exeName << " error: incomplete comment text "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/comment"){
          ar_log_error() << _exeName << " error: expected /comment, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
      }
      else if (tagText == "include"){
        // Go ahead and get the comment text (but discard it).
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
	  ar_log_error() << _exeName << " error: incomplete include text "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        stringstream includeText(buffer.data);
        string include;
	includeText >> include;
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/include"){
          ar_log_error() << _exeName << " error: expected /include, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        if (!parseParameterFile(include)){
	  ar_log_error() << _exeName << " error: include directive ("
	                 << include << ") failed in parsing parameter file.\n";
	  return false;
	}
      }
      else if (tagText == "param"){
        // First comes the name...
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "name"){
          ar_log_error() << _exeName << " error: expected name, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
          ar_log_error() << _exeName << " error: incomplete text of parameter name "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        stringstream nameText(buffer.data);
        string name;
        nameText >> name;
        if (name == ""){
          ar_log_error() << _exeName << " error: empty name field "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/name"){
          ar_log_error() << _exeName << " error: expected /name, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
	// Next comes the value...
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "value"){
          ar_log_error() << _exeName << " error: expected value, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
        if (!ar_getTextUntilEndTag(&fileStream, "value", &buffer)){
          ar_log_error() << _exeName << " error: incomplete text of parameter value "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
	// Set the attribute.
        setGlobalAttribute(name, buffer.data);
	// NOTE: we actually have already gotten the closing tag.

	// Finally should come the closing tag for the parameter.
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/param"){
          ar_log_error() << _exeName << " error: expected /param, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
      }
      else if (tagText == "assign"){
        // Go ahead and parse the assignment info.
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
	  ar_log_error() << _exeName << " error: incomplete assignment text "
	                 << "in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        parseAssignmentString(buffer.data);
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/assign"){
          ar_log_error() << _exeName << " error: expected /assign, not tag= " << tagText
	                 << " in phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
      }
      else if (tagText == "/szg_config"){
	// successful closure
        break;
      }
      else{
        ar_log_error() << _exeName << " error: phleet config file has illegal XML tag \""
	               << tagText << ".\n";
        fileStream.ar_close();
        return false;
      }
    }
    fileStream.ar_close();
    return true;
  }
  fileStream.ar_close();

  // Try the traditional way...
  ar_log_warning() << "arSZGClient warning: parsing config file using pre-0.7 syntax.\n";
  FILE* theFile = ar_fileOpen(fileName, dataPath, "r");
  if (!theFile){
    ar_log_error() << _exeName << " error: failed to open config file \""
                   << fileName << "\"\n";
    return false;
  }
  // BUG: There are problems below with fixed sized buffers. NOTE: these
  // problems will go away once we've fully converted over to the XML-based
  // config files.
  char buf[4096];
  char buf1[4096], buf2[4096], buf3[4096], buf4[4096];
  while (fgets(buf, sizeof(buf)-1, theFile)) {
    // skip comments which begin with (whitespace and) an octathorp;
    // also skip blank lines.
    char* pch = buf + strspn(buf, " \t");
    if (*pch == '#' || *pch == '\n' || *pch == '\r')
      continue;

    if (sscanf(buf, "%s %s %s %s", buf1, buf2, buf3, buf4) != 4) {
      ar_log_warning() << _exeName << " warning: ignoring incomplete command: "
                       << buf; // buf already has a newline in it.
      continue;
    }

    setAttribute(buf1, buf2, buf3, buf4);
  }

  fclose(theFile);

  return true;
}

string arSZGClient::getAttribute(const string& computerName,
                                 const string& groupName,
				 const string& parameterName,
	      			 const string& validValues){
  return getAttribute(_userName,computerName,groupName,parameterName,
                      validValues);
}

string arSZGClient::getAttribute(const string& userName,
                                 const string& computerName,
                                 const string& groupName,
                                 const string& parameterName,
                                 const string& validValues){
  // Getting an attribute is somewhat complex. 
  // If not connected, check the local database (for standalone mode).
  // If connected, go to the szgserver. In either case, upon failure,
  // check the environment variable groupName_parameterName.
  string result;
  if (!_connected){
    // In this case, we are using the local parameter file.
    result = _getAttributeLocal(computerName, groupName, parameterName,
                                validValues);
  }
  else{
    // We are going to the szgserver for information.
    const string query(
      ((computerName == "NULL") ? _computerName : computerName) +
      "/" + groupName + "/" + parameterName);
    arStructuredData* getRequestData
      = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
    int match = _fillMatchField(getRequestData);
    if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,query) ||
        !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"value") ||
        !getRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
        !_dataClient.sendData(getRequestData)){
      ar_log_warning() << _exeName << " warning: failed to send command.\n";
      result = string("NULL");
    }
    else{
      result = _changeToValidValue(groupName, parameterName,
                                   _getAttributeResponse(match), validValues);
    }
    _dataParser->recycle(getRequestData);
  }
  if (result == "NULL"){
    // First try failed, go to environment variable.
    string tmp = ar_getenv(groupName+"_"+parameterName);
    result = _changeToValidValue(groupName, parameterName, tmp, validValues);
  }
  return result;
}

// Returns 0 on error.
int arSZGClient::getAttributeInt(const string& groupName,
				 const string& parameterName){
  return getAttributeInt("NULL", groupName, parameterName, "");
}

int arSZGClient::getAttributeInt(const string& computerName,
                                 const string& groupName,
		                 const string& parameterName,
				 const string& defaults){
  const string& s =
    getAttribute(computerName, groupName, parameterName, defaults);
  if (s == "NULL")
    return 0;

  int x = -1;
  if (ar_stringToIntValid(s, x))
    return x;

  ar_log_warning() << "arSZGClient warning: failed to convert '" << s << "' to an int in "
                   << groupName << "/" << parameterName << ar_endl;
  return 0;
}

bool arSZGClient::getAttributeFloats(const string& groupName,
		                     const string& parameterName,
				     float* values,
				     int numvalues) {
  const string& s = getAttribute("NULL", groupName, parameterName, "");
  if (s == "NULL") {
    // This warning is usually redundant, because caller notices that
    // we return false and then warns that it's using a default.
#ifdef UNUSED
    ar_log_warning() << _exeName << " warning: "
                     << groupName << "/" << parameterName << " undefined.\n";
#endif
    return false;
  }
  int num = ar_parseFloatString(s,values,numvalues);
  if (num != numvalues) {
    ar_log_warning() << _exeName << " warning: "
                     << groupName << "/" << parameterName << " needed "
	             << numvalues << " floats, but got only "
	             << num << " from \"" << s << "\".\n";
    return false;
  }
  return true;
}

bool arSZGClient::getAttributeInts( const string& groupName,
		                                const string& parameterName,
				                            int* values,
				                            int numvalues) {
  const string& s = getAttribute("NULL", groupName, parameterName, "");
  if (s == "NULL") {
    return false;
  }
  const int num = ar_parseIntString(s,values,numvalues);
  if (num != numvalues) {
    ar_log_warning() << _exeName << " warning: "
                     << groupName << "/" << parameterName << " needed "
	             << numvalues << " longs, but got only "
	             << num << " from \"" << s << "\".\n";
    return false;
  }
  return true;
}

bool arSZGClient::getAttributeLongs(const string& groupName,
		                                const string& parameterName,
				                            long* values,
				                            int numvalues) {
  const string& s = getAttribute("NULL", groupName, parameterName, "");
  if (s == "NULL") {
    return false;
  }
  const int num = ar_parseLongString(s,values,numvalues);
  if (num != numvalues) {
    ar_log_warning() << _exeName << " warning: "
                     << groupName << "/" << parameterName << " needed "
	             << numvalues << " longs, but got only "
	             << num << " from \"" << s << "\".\n";
    return false;
  }
  return true;
}

bool arSZGClient::getAttributeVector3( const string& groupName,
                                       const string& parameterName,
                                       arVector3& value ) {
  return getAttributeFloats( groupName, parameterName, value.v, 3 );
}

bool arSZGClient::setAttribute(const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue){
  // Default userName is _userName.
  return setAttribute(_userName,computerName,groupName,
                      parameterName,parameterValue);
}

bool arSZGClient::setAttribute(const string& userName,
                               const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue){
  if (!_connected){
    // we set attributes in the local database (parseParameterFile
    // uses this method when reading in the local config file in
    // standalone mode.
    return _setAttributeLocal(computerName, groupName, parameterName,
                              parameterValue);
  }

  const string query(
    (computerName=="NULL" ? _computerName : computerName) +
    "/"+groupName+"/"+parameterName);

  // Get storage for the message.
  arStructuredData* setRequestData
    = _dataParser->getStorage(_l.AR_ATTR_SET);
  bool status = true;
  const ARint temp = 0; // don't test-and-set.
  int match = _fillMatchField(setRequestData);
  if (!setRequestData->dataInString(_l.AR_ATTR_SET_ATTR,query) ||
      !setRequestData->dataInString(_l.AR_ATTR_SET_VAL,parameterValue) ||
      !setRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !setRequestData->dataIn(_l.AR_ATTR_SET_TYPE,&temp,AR_INT,1) ||
      !_dataClient.sendData(setRequestData)){
    ar_log_warning() << _exeName << " warning: failed to set "
                     << groupName << "/" << parameterName << " on host \""
	             << computerName << "\" to \"" << parameterValue
                     << "\" (send failed).\n";
    status = false;
  }
  // Must recycle this.
  _dataParser->recycle(setRequestData);
  if (!status){
    return false;
  }

  arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
  if (!ack){
    ar_log_warning() << _exeName << " warning: failed to set "
                     << groupName << "/" << parameterName << " on host \""
	             << computerName << "\" to \"" << parameterValue
                     << "\" (ack failed).\n";
    return false;
  }
  _dataParser->recycle(ack);
  return true;
}

/// Attributes in the database, from the arSZGClient perspective, have always
/// been organized (under a given user name) hierarchically by
/// computer/attribute group/attribute. Internally to the szgserver, this
/// hierarchy had limited meaning since the paramter database is *really*
/// given by key/value pairs. It turns out that sometimes we really want
/// to dispense with the hierarchy altogether. For instance, the
/// configuration of an input node (the filters to use, whether it should
/// get input from the network, whether there are any special input sinks,
/// etc.) really isn't tied to a particular computer.
/// NOTE: We must use a different function name since there is already a
/// getAttribute with 2 const string& parameters.
/// The idea here is that "Global" attributes are different than the
/// "local" attributes that are tied to a particular computer.
string arSZGClient::getGlobalAttribute(const string& userName,
                                       const string& attributeName){
  if (!_connected){
    // In this case, we are using the local parameter file.
    return _getGlobalAttributeLocal(attributeName);
  }

  // We are going to the szgserver for information
  arStructuredData* getRequestData
    = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  string result;
  int match = _fillMatchField(getRequestData);
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,attributeName) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"value") ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !_dataClient.sendData(getRequestData)){
    ar_log_warning() << _exeName << " warning: failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(getRequestData);
  return result;
}

/// The userName is implicit in this one (i.e. it is the name of the
/// phleet user executing the program)
string arSZGClient::getGlobalAttribute(const string& attributeName){
  return getGlobalAttribute(_userName, attributeName);
}

/// It is also necessary to set the global attributes...
bool arSZGClient::setGlobalAttribute(const string& userName,
				     const string& attributeName,
				     const string& attributeValue){
  if (!_connected){
    // we set attributes in the local database (parseParameterFile
    // uses this method when reading in the local config file in
    // standalone mode.
    return _setGlobalAttributeLocal(attributeName, attributeValue);
  }

  // Get storage for the message.
  arStructuredData* setRequestData
    = _dataParser->getStorage(_l.AR_ATTR_SET);
  bool status = true;
  const ARint temp = 0; // don't test-and-set.
  int match = _fillMatchField(setRequestData);
  if (!setRequestData->dataInString(_l.AR_ATTR_SET_ATTR,attributeName) ||
      !setRequestData->dataInString(_l.AR_ATTR_SET_VAL,attributeValue) ||
      !setRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !setRequestData->dataIn(_l.AR_ATTR_SET_TYPE,&temp,AR_INT,1) ||
      !_dataClient.sendData(setRequestData)){
    ar_log_warning() << _exeName << " warning: failed to set "
                     << attributeName << "to " << attributeValue
                     << " (send failed).\n";
    status = false;
  }
  // Must recycle this.
  _dataParser->recycle(setRequestData);
  if (!status){
    return false;
  }

  arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
  if (!ack){
    ar_log_warning() << _exeName << " warning: failed to set "
                     << attributeName << "to " << attributeValue
                     << " (send failed).\n";
    return false;
  }
  _dataParser->recycle(ack);
  return true;
}

/// The phleet user name is implicit in this one.
bool arSZGClient::setGlobalAttribute(const string& attributeName,
				     const string& attributeValue){
  return setGlobalAttribute(_userName, attributeName, attributeValue);
}

/// Using XML records in the szg parameter database has upsides and downsides.
/// Upside: easy to manipulate whole display descriptions.
/// Downside: hard to make little changes to a (possibly quite complex)
/// display description.
/// This method attempts to mitigate this downside. It accesses XML stored
/// in the Phleet parameter database, exploiting the hierarchical nature of
/// XML to access individual attributes in the doc tree.
/// The pathList variable stores the path via which we search into the XML.
/// 
/// 1. The first member of the path gives the name of the Phleet global
/// parameter where we will start.
///
/// 2. Subsequent members define child XML "elements". They are element names
/// and can be given array indices (so that the 2nd element can be specified).
///
/// global parameter name = foo
/// global parameter value =
/// <szg_display>
///  <szg_window>
///  ...
///  </szg_window>
///  <szg_window>
///  +++
///  </szg_window>
/// </szg_display>
///
/// In this case, the path foo/szg_window refers to:
///
/// <szg_window>
/// ...
/// </szg_window>
/// 
/// while foo/szg_window[1] refers to:
///
/// <szg_window>
/// +++
/// </szg_window>
/// 
/// 3. The final member of the path can refer to an XML "attribute". All of
/// the configuration data for arGUI is stored in "attributes". They look like
/// this in the XML:
///
/// <szg_viewport_list viewmode="custom" />
///
/// Here, viewmode is an attribute of the szg_viewport_list element. If a path
/// parses down to an attribute BEFORE getting to its final member, an error
/// occurs (return "NULL").
///
/// 4. Phleet allows the XML documents to be stored in multiple global
/// parameters, which facilitates reuse of individual pieces of complex configs.
/// This feat is accomplished via "pointers" embedded in the XML in a standard
/// way. For instance,
///
/// <szg_window usenamed="foo" />
///
/// In this case, if we encounter this element while parsing the path, we
/// replace the current element with that contained in the global parameter
/// "foo" and continue parsing from there. Any alteration to an attribute value
/// from this point forward will occur inside the XML document inside the
/// global parameter "foo".
///
/// NOTE: There is an exception to the aforementioned pointer parsing rule. We
/// need to be able to change the usenamed attribute instead of just following
/// the pointer. Consequently, if the *next* member of the path is "usenamed"
/// we won't actually follow the pointer, but will instead stop at the
/// attribute in question.
///
/// pathList gives the path, which is parsed and interpreted as above.
/// attributeValue is set to something besides "NULL" if we will be altering
///  an attribute value. If the path does not parse to an attribute, then this
///  is an error (returning "NULL") but otherwise returning "SZG_SUCCESS".
///  On the other hand, if attributeValue is "NULL" (the default) then the 
///  method just returns the value indicated by the parsed path (or "NULL"
///  if there is an error in the parsing).

string arSZGClient::getSetGlobalXML(const string& userName,
                                    const arSlashString& pathList,
                                    const string& attributeValue = "NULL"){
  int pathPlace = 0;
  // The first element in the path gives the name of the global attribute
  // where the XML document is stored.
  string szgDocLocation = pathList[0];
  // The XML document itself, in string format.
  string docString = getGlobalAttribute(userName, szgDocLocation);
  TiXmlDocument doc;
  doc.Clear();
  doc.Parse(docString.c_str());
  if (doc.Error()){
    ar_log_error() << "dget error: in parsing gui config xml on line: " 
                   << doc.ErrorRow() << ar_endl;
    return string("NULL");
  }
  TiXmlNode* node = doc.FirstChild();
  if (!node || !node->ToElement()){
    ar_log_error() << "dget error: malformed XML (global node).\n";
    return string("NULL");
  }
  TiXmlNode* child = node;
  pathPlace = 1;
  // Walk down the XML tree, using the path defined by pathList.
  while (pathPlace < pathList.size()){
    // As we are searching down the doc tree, we allow an array syntax for
    // picking out child elements.
    // For instance,
    //     szg_viewport[0]
    // or
    //     szg_viewport
    // mean the first szg_viewport child, while
    //     szg_viewport[1]
    // means the second child.
    unsigned int firstArrayLoc;
    // The default is to use the first child element with the appropriate name.
    int actualArrayIndex = 0;
    // This is the actual type of the element. The default is the current step
    // in the path... but this could change if there is an array index.
    string actualElementType = pathList[pathPlace];
    // Check to see if there is an array index. If so, we'll go ahead and 
    // remove it.
    if ( (firstArrayLoc = pathList[pathPlace].find('[')) 
	  != string::npos ){
      // There might be a valid array index.
      unsigned int lastArrayLoc 
        = pathList[pathPlace].find_last_of("]");
      if (lastArrayLoc == string::npos){
	// It seems like we should have a valid array index, but we do not.
	ar_log_error() << "dget error: invalid array index in "
	               << pathList[pathPlace] << ".\n";
	return string("NULL");
      }
      string potentialIndex 
        = pathList[pathPlace].substr(firstArrayLoc+1, 
                                      lastArrayLoc-firstArrayLoc-1);
      stringstream indexStream;
      indexStream << potentialIndex;
      indexStream >> actualArrayIndex;
      if (indexStream.fail()){
	ar_log_error() << "dget error: invalid array index " << potentialIndex
	               << ".\n";
	return string("NULL");
      }
      if (actualArrayIndex < 0){
	ar_log_error() << "dget error: array index cannot be negative.\n";
	return string("NULL");
      }
      // We now strip out the index. We've got the actualArrayIndex already.
      actualElementType = pathList[pathPlace].substr(0, firstArrayLoc);
    }
    // Must get the first child. If we want something further (i.e. we are
    // trying to use an array index, as tested for above), iterate from that 
    // starting place.
    TiXmlNode* newChild = child->FirstChild(actualElementType);
    int which = 1;
    if (newChild){
      // Step through siblings.
      while (newChild && which <= actualArrayIndex){
        newChild = newChild->NextSibling();
	which++;
      }
    }
    // At this point, we've either got the element itself (as determined by the
    // array index) or we've got an error (maybe there weren't enough elements
    // of this type in the doc tree to justify the array index).
    if (which < actualArrayIndex){
      ar_log_error() << "dget error: could not find elements of type "
	             << actualElementType << " up to index " 
	             << actualArrayIndex << ".\n";
      return string("NULL");
    }
     
    // Actually, there is one more possibility. Our path might have specified
    // an "attribute" instead of an "element". This is OK (as long as it is the
    // final step in the path and does not have an array index).
    if (!newChild){
      // This might still be OK. It could be an "attribute".
      if (!child->ToElement()->Attribute(pathList[pathPlace])){
	// Neither an "element" or an "attribute". This is an error.
	ar_log_error() << "dget error: " << pathList[pathPlace]
	               << " is neither an element or an attribute.\n";
	return string("NULL");
      }
      else{
	// OK, it's an attribute. This is an error if it isn't the last
	// value on the path.
        string attribute = child->ToElement()->Attribute(pathList[pathPlace]);
        if (pathPlace != pathList.size() -1){
	  ar_log_error() << "dget error: attribute name ("
	                 << pathList[pathPlace] << ") not last in path list.\n";
	  return string("NULL");
	}
        if (attributeValue != "NULL"){
          // We are altering the XML document residing in the parameter
	  // database.
          child->ToElement()->SetAttribute(pathList[pathPlace],
					   attributeValue.c_str());
          string databaseValue;
	  databaseValue << doc;
          setGlobalAttribute(szgDocLocation, databaseValue);
	  // We just wanted to alter the parameter database. Don't need
	  // to get a value back, just an indication of success.
          return string("SZG_SUCCESS");
        }
	// Actually want the value of the attribute back.
	// (i.e. attributeValue == "NULL")
	return attribute;
      }
    }
    else{
      // Check to make sure this is valid XML. 
      if (!newChild->ToElement()){
	ar_log_error() << "dget error: malformed XML in " << pathList[pathPlace] 
                       << ".\n";
        return string("NULL");
      }
      // Valid XML. Continue the walk down the tree.
      // NOTE: Syzygy supports POINTERS. So, a node might really be stored
      // inside another global parameter. Like so:
      //     <szg_screen usenamed="left_wall" />
      // If this is the case, we must get THAT document and start again.
      // NOTE: We also want to be able to SET the pointer. So, when the
      // very next step in the path is "usenamed" then do not retrieve the
      // pointed-to data.
      if (newChild->ToElement()->Attribute("usenamed")
          && (pathPlace+1 >= pathList.size() 
              || pathList[pathPlace+1] != "usenamed")){
	// Important to keep track of which sub-document we are, in fact,
	// holding as we search down the tree of pointers.
	szgDocLocation = newChild->ToElement()->Attribute("usenamed");
        string newDocString 
          = getGlobalAttribute(userName, szgDocLocation);
        doc.Clear();
        doc.Parse(newDocString.c_str());
        newChild = doc.FirstChild();
	if (!newChild || !newChild->ToElement()){
          ar_log_error() << "dget error: malformed XML in pointer ("
	                 << szgDocLocation << ").\n";
	  return string("NULL");
	}
      }
      child = newChild;
    }
    pathPlace++;
  }
  // By making it out here, we know that the final piece of the path refers
  // to an element (not an attribute).
  //
  // NOTE: This is an ERROR if we were trying to set an attribute to a new
  // value. Why? Because the parameter path didn't end in an attribute!
  // (but instead in an XML element).
  if (attributeValue != "NULL"){
    return string("NULL");
  }
  // In this case, we are just querying a value. It could be that we want to
  // get a whole XML document from the szg parameter database.
  std::string output;
  output << *child;
  return output;
}

// DOES NOT WORK IN THE LOCAL (STANDALONE) CASE!!!
// ALSO... Should this really be left in? This was an early attempt
// at providing lock-like functionality for the Phleet.
string arSZGClient::testSetAttribute(const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue){
  // use the local user name
  return testSetAttribute(
    _userName, computerName, groupName, parameterName, parameterValue);
}

// DOES NOT WORK IN THE LOCAL (STANDALONE) CASE!!!
string arSZGClient::testSetAttribute(const string& userName,
                               const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue){
  if (!_connected)
    return string("NULL");

  // for speed in dget/dset we want the szgserver to do the name resolution
  // this saves a communication round trip
  const string query(
    (computerName=="NULL" ? _computerName : computerName) +
    "/" + groupName + "/" + parameterName);
  // Get storage.
  arStructuredData* setRequestData
    = _dataParser->getStorage(_l.AR_ATTR_SET);
  string result;
  // Do test-and-set.
  ARint temp = 1;
  int match = _fillMatchField(setRequestData);
  if (!setRequestData->dataInString(_l.AR_ATTR_SET_ATTR,query) ||
      !setRequestData->dataInString(_l.AR_ATTR_SET_VAL,parameterValue) ||
      !setRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !setRequestData->dataIn(_l.AR_ATTR_SET_TYPE,&temp,AR_INT,1) ||
      !_dataClient.sendData(setRequestData)){
    ar_log_warning() << _exeName << " warning: failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(setRequestData);
  return result;
}

string arSZGClient::getProcessList(){
  if (!_connected)
    return string("NULL");

  // Get storage for the message.
  arStructuredData* getRequestData
    = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  string result;
  // "NULL" asks the server for the process table
  int match = _fillMatchField(getRequestData);
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,"NULL") ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"NULL") ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !_dataClient.sendData(getRequestData)){
    ar_log_warning() << _exeName << " warning: failed to send getProcessList command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  // Must recycle the storage
  _dataParser->recycle(getRequestData);
  return result;
}

/// Kill (or remove from the szgserver table, really) by component ID.
bool arSZGClient::killProcessID(int id){
  if (!_connected)
    return false;

  // get storage for the message.
  arStructuredData* killIDData = _dataParser->getStorage(_l.AR_KILL);
  bool ok = true;
  // One of the few places where we DON'T USE MATCH!
  (void)_fillMatchField(killIDData);
  if (!killIDData->dataIn(_l.AR_KILL_ID,&id,AR_INT,1) ||
      !_dataClient.sendData(killIDData)){
    ar_log_warning() << _exeName << " warning: message send failed\n";
    ok = false;
  }
  _dataParser->recycle(killIDData); // recycle the storage
  return ok;
}

/// Kill the ID of the first process in the process list
/// running on the given computer with the given label
/// Return false if no process was found.
bool arSZGClient::killProcessID(const string& computer,
                                const string& processLabel){
  if (!_connected)
    return false;

  const string realComputer = (computer == "NULL" || computer == "localhost") ?
    // use the computer we are on as the default
    _computerName :
    computer;

  const int id = getProcessID(realComputer, processLabel);
  return id >= 0 && killProcessID(id);
}

/// Given the process ID, return the process label
string arSZGClient::getProcessLabel(int processID){
  // massive copypaste between getProcessLabel and getProcessID
  if (!_connected)
    return string("NULL");

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "label");
  data->dataIn(_l.AR_PROCESS_INFO_ID, &processID, AR_INT, 1);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send process label request.\n";
    return string("NULL");
  }
  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    ar_log_warning() << _exeName << " warning: failed to get process label response.\n";
    return string("NULL");
  }
  const string theLabel = data->getDataString(_l.AR_PROCESS_INFO_LABEL);
  // note that this is a different data record than above, needs to be recycled
  _dataParser->recycle(data);
  return theLabel;
}

/// Return the ID of the first process in the process list
/// running on the given computer with the given label.
int arSZGClient::getProcessID(const string& computer,
                              const string& processLabel){
  // massive copypaste between getProcessLabel and getProcessID
  if (!_connected)
    return -1;

  const string realComputer = (computer == "NULL" || computer == "localhost") ?
    // use the computer we are on as the default
    _computerName :
    computer;

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "ID");
  // zero-out the storage
  int dummy = 0;
  data->dataIn(_l.AR_PROCESS_INFO_ID, &dummy, AR_INT, 1);
  data->dataInString(_l.AR_PROCESS_INFO_LABEL, realComputer+"/"+processLabel);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send process ID request.\n";
    return -1;
  }
  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    ar_log_warning() << _exeName << " warning: failed to get process ID response.\n";
    return -1;
  }
  const int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // note that this is a different data record than above, needs to be recycled
  _dataParser->recycle(data);
  return theID;
}

// Return the ID of the process which is "me".
int arSZGClient::getProcessID(void){
  if (!_connected)
    return -1;

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "self");
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: getProcessID() send failed\n";
    _dataParser->recycle(data);
    return -1;
  }
  _dataParser->recycle(data);

  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    ar_log_warning() << _exeName << " warning: ignoring illegal response packet\n";
    return -1;
  }
  const int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // note that this is a new piece of data, so we need to recycle it again
  _dataParser->recycle(data);
  return theID;
}

// DOES THIS REALLY BELONG AS A METHOD OF arSZGClient?
// MAYBE THIS IS A HIGHER_LEVEL CONSTRUCT.
bool arSZGClient::sendReload(const string& computer,
                             const string& processLabel) {
  if (!_connected)
    return false;

  if (processLabel == "NULL"){
    // Vacuously ok.
    return true;
  }
  const int pid = getProcessID(computer, processLabel);
  const int ok = pid != -1 && (sendMessage("reload", "NULL", pid) >= 0);
  if (!ok)
    ar_log_warning() << _exeName << " warning: failed to reload on host \""
                     << computer << "\".\n";
  return ok;
}

int arSZGClient::sendMessage(const string& type, const string& body,
			     int destination, bool responseRequested){
  return sendMessage(type,body,"NULL",destination, responseRequested);
}

/// Sends a message and returns the "match" for use with getMessageResponse.
/// This "match" links responses to the original messages. On an error, this
/// function returns -1.
int arSZGClient::sendMessage(const string& type, const string& body,
                             const string& context, int destination,
                             bool responseRequested){
  if (!_connected)
    return -1;

  // Must get storage for the message.
  arStructuredData* messageData =
    _dataParser->getStorage(_l.AR_SZG_MESSAGE);
  // convert from bool to int
  const int response = responseRequested ? 1 : 0;
  int match = _fillMatchField(messageData);
  if (!messageData->dataIn(_l.AR_SZG_MESSAGE_RESPONSE,
                           &response, AR_INT,1) ||
      !messageData->dataInString(_l.AR_SZG_MESSAGE_TYPE,type) ||
      !messageData->dataInString(_l.AR_SZG_MESSAGE_BODY,body) ||
      !messageData->dataIn(_l.AR_SZG_MESSAGE_DEST,&destination,AR_INT,1) ||
      !messageData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !messageData->dataInString(_l.AR_PHLEET_CONTEXT,context) ||
      !_dataClient.sendData(messageData)){
    ar_log_warning() << _exeName << " warning: message send failed.\n";
    match = -1;
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_MESSAGE_ACK);
    if (!ack){
      ar_log_warning() << _exeName << " warning: got no message ack.\n";
      match = -1;
    }
    else{
      if (ack->getDataString(_l.AR_SZG_MESSAGE_ACK_STATUS)
            != string("SZG_SUCCESS")){
        ar_log_warning() << _exeName << " warning: message send failed.\n";
      }
      _dataParser->recycle(ack);
    }
  }
  // Must recycle the storage.
  _dataParser->recycle(messageData);
  return match;
}

/// Receive a message routed through the szgserver.
/// Returns the ID of the received message, or 0 on error (client should exit).
/// @param userName Set to the username associated with the message
/// @param messageType Set to the type of the message
/// @param messageBody Set to the body of the message
/// @param context Set to the context of the message (the virtual computer)
int arSZGClient::receiveMessage(string* userName, string* messageType,
                                string* messageBody, string* context){
  if (!_connected){
    return 0;
  }
  // This is the one place we get a record via its type, not its match.
  arStructuredData* data = _getDataByID(_l.AR_SZG_MESSAGE);
  if (!data){
#if 0
    // Too low-level for a warning.  Caller should warn, if they care.
    ar_log_warning() << _exeName << " warning: failed to receive message.\n";
#endif
    return 0;
  }
  *messageType = data->getDataString(_l.AR_SZG_MESSAGE_TYPE);
  *messageBody = data->getDataString(_l.AR_SZG_MESSAGE_BODY);

  // Pack username and context structures, if requested.
  if (userName)
    *userName = data->getDataString(_l.AR_PHLEET_USER);
  if (context)
    *context = data->getDataString(_l.AR_PHLEET_CONTEXT);
  const int messageID = data->getDataInt(_l.AR_SZG_MESSAGE_ID);
  _dataParser->recycle(data);
  return messageID;
}

/// Get a response to the client's messages.  Returns 1 if a final response has
/// been received, -1 if a partial response has been received, and 0 if failure
/// (as might happen, for instance, if a component dies before having a
/// chance to respond to a message).
/// @param body is filled-in with the body of the message response
/// @param match is filled-in with the "match" of this response (which is
///   the same as the "match" of the original message that generated it.
int arSZGClient::getMessageResponse(list<int> tags,
                                    string& body,
                                    int& match,
                                    int timeout){
  if (!_connected){
    return 0;
  }
  arStructuredData* ack = NULL;
  match = _getTaggedData(ack, tags,_l.AR_SZG_MESSAGE_ADMIN, timeout);
  if (match < 0){
    ar_log_warning() << _exeName << " warning: no message response.\n";
    return 0;
  }
  if (ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_TYPE) != "SZG Response"){
    ar_log_warning() << _exeName << " warning: no message response.\n";
    _dataParser->recycle(ack);
    body = string("arSZGClient internal error: failed to get proper response");
    return 0;
  }

  body = ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_BODY);
  // Determine whether this was a partial response.
  const string status(ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_STATUS));
  _dataParser->recycle(ack);

  if (status == "SZG_SUCCESS"){
    return 1;
  }
  if (status == "SZG_CONTINUE"){
    return -1;
  }
  if (status == "SZG_FAILURE"){
    body = string("failure response (remote component died).\n");
    return 0;
  }
  body = string("arSZGClient internal error: response with unknown status ")
    + status + string(".\n");
  return 0;
}

/// Respond to a message. Return
/// true iff the response was successfully dispatched.
/// @param messageID ID of the message to which we are trying to respond
/// @param body Body we wish to send
/// @param partialResponse False iff we won't send further responses
/// (the default)
bool arSZGClient::messageResponse(int messageID, const string& body,
                                  bool partialResponse){
  if (!_connected){
    return false;
  }
  // Do not use a const string& here. That is the equivalent of a pointer
  // to a temporary.
  const string statusField = partialResponse ? string("SZG_CONTINUE") :
                                                string("SZG_SUCCESS");
  // Must get storage for the message.
  arStructuredData* messageAdminData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE_ADMIN);
  bool state = false;
  // fill in the appropriate data
  int match = _fillMatchField(messageAdminData);
  if (!messageAdminData->dataIn(_l.AR_SZG_MESSAGE_ADMIN_ID, &messageID,
                                AR_INT, 1) ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_STATUS,
				      statusField) ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Response") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY,
                                      body) ||
      !_dataClient.sendData(messageAdminData)){
    ar_log_warning() << _exeName << " warning: failed to send message response.\n";
  }
  else{
    state = _getMessageAck(match, "message response");
  }
  _dataParser->recycle(messageAdminData);
  return state;
}

/// Start a "message ownership trade".
/// Useful when the executable launched by szgd wants to respond to "dex".
/// Returns -1 on failure and otherwise the "match" associated with this
/// request.
/// @param messageID ID of the message we want to trade.  We need to
/// own the right to respond to this message for the call to succeed.
/// @param key Value of which a future ownership trade can occur
int arSZGClient::startMessageOwnershipTrade(int messageID,
                                            const string& key){
  if (!_connected){
    return -1;
  }
  // Must get storage for the message.
  arStructuredData* messageAdminData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE_ADMIN);
  int match = _fillMatchField(messageAdminData);
  if (!messageAdminData->dataIn(_l.AR_SZG_MESSAGE_ADMIN_ID, &messageID,
				AR_INT, 1) ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Trade Message") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY, key) ||
      !_dataClient.sendData(messageAdminData)){
    ar_log_warning() << _exeName << "warning: failed to send message ownership trade.\n";
    match = -1;
  }
  else{
    (void)_getMessageAck(match, "message trade");
  }
  _dataParser->recycle(messageAdminData);
  return match;
}

/// Wait until the message ownership trade completes (or until the optional
/// timeout argument has elapsed). Returns false on timeout or other error
/// and true otherwise.
bool arSZGClient::finishMessageOwnershipTrade(int match, int timeout){
  if (!_connected){
    return false;
  }
  return _getMessageAck(match, "message ownership trade", NULL, timeout);
}

/// Revoke the possibility of a previously-made message ownership
/// trade.  Return false on error: if the trade has already been
/// taken, does not exist, or if we are not authorized to revoke it.
/// @param key Fully expanded string on which the ownership trade rests
bool arSZGClient::revokeMessageOwnershipTrade(const string& key){
  if (!_connected){
    return false;
  }
  // Must get storage for the message.
  arStructuredData* messageAdminData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE_ADMIN);
  int match = _fillMatchField(messageAdminData);
  bool state = false;
  if (!messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Revoke Trade") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY, key) ||
      !_dataClient.sendData(messageAdminData)){
    ar_log_warning() << _exeName << "warning: failed to send message trade revocation.\n";
  }
  else{
    state = _getMessageAck(match, "message trade revocation");
  }
  _dataParser->recycle(messageAdminData);
  return state;
}

/// Request ownership of a message that is currently posted with a given key.
/// Returns the ID of the message if successful, and 0 otherwise
/// NOTE: messages never have ID 0!
int arSZGClient::requestMessageOwnership(const string& key){
  if (!_connected){
    return 0;
  }
  // Must get storage for the message.
  arStructuredData* messageAdminData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE_ADMIN);
  int messageID = 0;
  int match = _fillMatchField(messageAdminData);
  if (!messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Message Request") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY, key) ||
      !_dataClient.sendData(messageAdminData)){
    ar_log_warning() << _exeName
                     << " warning: failed to send message ownership request.\n";
    messageID = 0;
  }
  else{
    if (!_getMessageAck(match, "message ownership request", &messageID)){
      messageID = 0;
    }
  }
  _dataParser->recycle(messageAdminData);
  return messageID;
}

/// Requests that the client be notified when the component with the given ID
/// exits (notification will occur immedicately if that ID is not currently
/// held).
int arSZGClient::requestKillNotification(int componentID){
  if (!_connected){
    return -1;
  }
  arStructuredData* data
    = _dataParser->getStorage(_l.AR_SZG_KILL_NOTIFICATION);
  int match = _fillMatchField(data);
  data->dataIn(_l.AR_SZG_KILL_NOTIFICATION_ID, &componentID, AR_INT, 1);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send kill "
	             << "notification request.\n";
    match = -1;
  }
  _dataParser->recycle(data);
  // We do not get a message back from the szgserver, as is usual.
  // The response will come through getKillNotification.
  return match;
}

/// Receives a notification, as requested by the previous method, that a
/// component has exited. Return the match if we have succeeded. Return
/// -1 on failure.
int arSZGClient::getKillNotification(list<int> tags,
                                     int timeout){
  if (!_connected){
    return -1;
  }
  // block until the response occurs (or timeout)
  arStructuredData* data = NULL;
  int match = _getTaggedData(data, tags,
                             _l.AR_SZG_KILL_NOTIFICATION, timeout);
  if (match < 0){
    ar_log_warning() << _exeName << " remark: failed to get kill "
	             << "notification within timeout period.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

/// Acquire a lock. Check with szgserver to see
/// if the lock is available. If so, acquire the lock for this component
/// (and set ownerID to -1). Otherwise, set ownerID to the lock-holding
/// component's ID.  Return true iff the lock is acquired.
bool arSZGClient::getLock(const string& lockName, int& ownerID){
  if (!_connected){
    return false;
  }
  // Must get storage for the message.
  arStructuredData* lockRequestData
    = _dataParser->getStorage(_l.AR_SZG_LOCK_REQUEST);
  int match = _fillMatchField(lockRequestData);
  bool state = false;
  if (!lockRequestData->dataInString(_l.AR_SZG_LOCK_REQUEST_NAME,
                                     lockName) ||
      !_dataClient.sendData(lockRequestData)){
    ar_log_warning() << _exeName << " warning: failed to send lock request.\n";
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack){
      ar_log_warning() << _exeName << " warning: no ack for lock.\n";
    }
    else{
      ownerID = ack->getDataInt(_l.AR_SZG_LOCK_RESPONSE_OWNER);
      state =
        ack->getDataString(_l.AR_SZG_LOCK_RESPONSE_STATUS)
        == string("SZG_SUCCESS");
      _dataParser->recycle(ack);
    }
  }
  // Must recycle storage.
  _dataParser->recycle(lockRequestData);
  return state;
}

/// Release a lock.  Return false on error,
/// if the lock does not exist or if another component is holding it.
bool arSZGClient::releaseLock(const string& lockName){
  if (!_connected){
    return false;
  }
  // Must get storage for message.
  arStructuredData* lockReleaseData
    = _dataParser->getStorage(_l.AR_SZG_LOCK_RELEASE);
  int match = _fillMatchField(lockReleaseData);
  bool state = false;
  if (!lockReleaseData->dataInString(_l.AR_SZG_LOCK_RELEASE_NAME,
                                     lockName) ||
      !_dataClient.sendData(lockReleaseData)){
    ar_log_warning() << _exeName << " warning: failed to send lock request.\n";
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack){
      ar_log_warning() << _exeName << " warning: no ack for lock.\n";
    }
    else{
      state =
        ack->getDataString(_l.AR_SZG_LOCK_RESPONSE_STATUS)
        == string("SZG_SUCCESS");
      _dataParser->recycle(ack);
    }
  }
  // Must recycle storage.
  _dataParser->recycle(lockReleaseData);
  return state;
}

/// Requests that the client be notified when the lock in question is
/// not held by the server. NOTE: we return the function match on success
/// (which is always nonnegative) or -1 on failure.
int arSZGClient::requestLockReleaseNotification(const string& lockName){
  if (!_connected){
    return -1;
  }
  arStructuredData* data
    = _dataParser->getStorage(_l.AR_SZG_LOCK_NOTIFICATION);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_LOCK_NOTIFICATION_NAME, lockName);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send lock release "
	             << "notification request.\n";
    match = -1;
  }
  _dataParser->recycle(data);
  // We do not get a message back from the szgserver, as is usual.
  // The response will come through getLockReleaseNotification.
  return match;
}

/// Receives a notification, as requested by the previous method, that a
/// lock is no longer held. Return the match if we have succeeded. Return
/// -1 on failure.
int arSZGClient::getLockReleaseNotification(list<int> tags,
                                            int timeout){
  if (!_connected){
    return -1;
  }
  // block until the response occurs (or timeout)
  arStructuredData* data = NULL;
  int match = _getTaggedData(data, tags,
                             _l.AR_SZG_LOCK_NOTIFICATION, timeout);
  if (match < 0){
    ar_log_warning() << _exeName << " error: failed to get lock release "
	             << "notification.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

/// Prints all locks currently held inside the szgserver.
void arSZGClient::printLocks(){
  if (!_connected){
    return;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_LOCK_LISTING);
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // NOTE: we do not attempt to print out the computers on which the
  // locks are held in what follows, just the IDs.
  int match = _fillMatchField(data);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send print locks request.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_LISTING);
  if (!ack){
    ar_log_warning() << _exeName << " warning: did not receive response for lock listing "
	             << "request.\n";
    return;
  }
  const string locks(data->getDataString(_l.AR_SZG_LOCK_LISTING_LOCKS));
  int* IDs = (int*)data->getDataPtr(_l.AR_SZG_LOCK_LISTING_COMPONENTS, AR_INT);
  int number = data->getDataDimension(_l.AR_SZG_LOCK_LISTING_COMPONENTS);
  // print out the stuff
  int where = 0;
  for (int i=0; i<number; i++){
    // SHOULD NOT be ar_log_*. This is a rare case where we actually want to print to the terminal.
    cout << ar_pathToken(locks, where) << ";"
         << IDs[i] << "\n";
  }
  _dataParser->recycle(data);
}

/// Helper for registerService() and requestNewPorts().
/// @param fRetry true iff we're retrying
bool arSZGClient::_getPortsCore1(
       const string& serviceName, const string& channel,
       int numberPorts, arStructuredData*& data, int& match, bool fRetry) {
  if (channel != "default" && channel != "graphics" && channel != "sound"
      && channel != "input"){
    ar_log_error() << _exeName << " error: "
                   << (fRetry ? "requestNewPorts" : "registerService")
	           << "() passed invalid channel.\n";
    return false;
  }

  // pack the data
  data =_dataParser->getStorage(_l.AR_SZG_REGISTER_SERVICE);
  match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_STATUS,
                     fRetry ? "SZG_RETRY" : "SZG_TRY");
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_TAG, serviceName);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_NETWORKS,
                     getNetworks(channel));
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_ADDRESSES,
                     getAddresses(channel));
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_SIZE, &numberPorts, AR_INT, 1);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_COMPUTER, _computerName);
  const int temp[2] = {
    _configParser.getFirstPort(),
    _configParser.getPortBlockSize() };
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);
  return true;
}

/// Helper for registerService() and requestNewPorts().
/// @param match from AR_PHLEET_MATCH
/// @param portIDs stuffed with the good port values
/// @param fRetry true iff we're retrying
bool arSZGClient::_getPortsCore2(arStructuredData* data, int match,
                                 int* portIDs, bool fRetry) {
  if (!_connected){
    return false;
  }
  // NOTE: the match field is filled-in in _getPortsCore1
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send service registration "
	             << (fRetry ? "retry" : "") << " request.\n";
    _dataParser->recycle(data);
    return false;
  }
  _dataParser->recycle(data);

  // wait for the response
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data){
    ar_log_warning() << _exeName << " warning: no response for service registration "
	             << (fRetry ? "retry" : "") << "request.\n";
    return false;
  }
  (void)data->getDataInt(_l.AR_PHLEET_MATCH);

  if (data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) != "SZG_SUCCESS"){
    return false;
  }
  int dimension = data->getDataDimension(_l.AR_SZG_BROKER_RESULT_PORT);
  data->dataOut(_l.AR_SZG_BROKER_RESULT_PORT, portIDs, AR_INT, dimension);
  return true;
}

/// Helper function that puts the next "match" value into the given
/// piece of structured data. This value is not
/// repeated over the lifetime of the arSZGClient and is used to match
/// responses of the szgserver to sent messages. This allows arSZGClient
/// methods to be called freely from different application threads.
int arSZGClient::_fillMatchField(arStructuredData* data){
  ar_mutex_lock(&_serviceLock);
  int match = _nextMatch;
  _nextMatch++;
  ar_mutex_unlock(&_serviceLock);
  data->dataIn(_l.AR_PHLEET_MATCH,&match,AR_INT,1);
  return match;
}

/// Attempt to register a service to be offered by this component with the
/// szgserver. Returns true iff the registration was successful (i.e.
/// no other service shares the same name).
/// @param serviceName name of the offered service
/// @param channel routing channel upon which the service is offered
/// (must be one of "default", "graphics", "sound", "input")
/// @param numberPorts number of ports the service requires (up to 10)
/// @param portIDs on success, stuffed with the ports on which
/// the service should be offered
bool arSZGClient::registerService(const string& serviceName,
                                  const string& channel,
                                  int numberPorts, int* portIDs){
  if (!_connected){
    return false;
  }
  arStructuredData* data = NULL;
  int match = -1;
  return _getPortsCore1(serviceName, channel, numberPorts, data, match, false)
         && _getPortsCore2(data, match, portIDs, false);
}

/// The ports assigned to us by the szgserver can be unusable for local
/// reasons unknowable to the szgserver, so we may need to ask
/// for them to be reassigned. Otherwise, this method is similar to
/// registerService(...).
/// @param serviceName name of the service that was previously registered
/// and which needs a new set of ports.
/// @param channel the routing channel upon which we are offering the service
/// (must be one of "default", "graphics", "sound", "input")
/// @param numberPorts the number of ports required.
/// @param portIDs passes in the old, problematic ports, and will be filled
/// with new ports on success.
bool arSZGClient::requestNewPorts(const string& serviceName,
                                  const string& channel,
                                  int numberPorts, int* portIDs){
  if (!_connected){
    return false;
  }
  arStructuredData* data = NULL;
  int match = -1;
  return
    _getPortsCore1(serviceName, channel, numberPorts, data, match, true) &&
    data->dataIn(_l.AR_SZG_REGISTER_SERVICE_PORT, portIDs,
                 AR_INT, numberPorts) &&
    _getPortsCore2(data, match, portIDs, true);
}

/// The szgserver assigns us ports, but cannot know if
/// we'll be able to bind to them (see requestNewPorts() above).
/// Furthermore, it cannot fully register the service until
/// the component on this end confirms that it is accepting connections,
/// lest we get a race condition. Consequently, this function lets a component
/// confirm to the szgserver that it was able to bind.  It returns true iff
/// the send to the szgserver succeeded. The szgserver sends no response.
/// @param serviceName the name of the service
/// @param channel the routing channel upon which we are offering the service
/// (must be one of "default", "graphics", "sound", "input")
/// @param numberPorts the number of ports used by the service
/// @param portIDs an array containing the port IDs (all of which were
/// successfully bound).
bool arSZGClient::confirmPorts(const string& serviceName,
                               const string& channel,
                               int numberPorts, int* portIDs){
  if (!_connected){
    return false;
  }
  if (channel != "default" && channel != "graphics" && channel != "sound"
      && channel != "input"){
    ar_log_error() << _exeName << " error: invalid channel for confirmPorts().\n";
    return false;
  }

  // pack the data
  arStructuredData* data =_dataParser->getStorage(_l.AR_SZG_REGISTER_SERVICE);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_STATUS, "SZG_SUCCESS");
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_TAG, serviceName);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_NETWORKS,
                     getNetworks(channel));
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_ADDRESSES,
                     getAddresses(channel));
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_SIZE, &numberPorts, AR_INT, 1);
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_PORT, portIDs, AR_INT, numberPorts);
  data->dataInString(_l.AR_SZG_REGISTER_SERVICE_COMPUTER, _computerName);
  int temp[10];
  temp[0] = _configParser.getFirstPort();
  temp[1] = _configParser.getPortBlockSize();
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);

  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send service registration "
	             << "retry request.\n";
    _dataParser->recycle(data);
    return false;
  }
  _dataParser->recycle(data);

  // the best policy for system stability seems to be that every message
  // sent to the szgserver receives a reply. without this policy, a
  // communications pipe to the szgserver can go away while it is still
  // processing a message received on that pipe.
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data){
    ar_log_error() << _exeName << " error: failed to get broker result in response "
                   << "to port confirmation.\n";
    return false;
  }
  (void)data->getDataInt(_l.AR_PHLEET_MATCH);

  return data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) == "SZG_SUCCESS";
}

/// Used to find the internet address for connection to a named service
/// over one of (potentially) several specified networks. An arPhleetAddress
/// is returned. On success, the valid field is set to true, on failure
/// it is set to false. The address field contains the IP address. The
/// nuberPorts field contains the number of ports. And portIDs is an array
/// (up to 10) containing the port IDs. NOTE: this call can be made in either
/// synchronous or asynchronous mode. The difference between the two modes
/// shows up only if the requested service is not currently registered
/// in the phleet. In synchronous mode, the szgserver immediately responds
/// with a failure message. In asynchronous mode, the szgserver declines
/// responding and, instead, waits for the service to be registered, at
/// which time it responds. Note that, in asynchronous mode,
/// discoverService(...) is, as a result, a blocking call.
/// @param serviceName the name of the service to which we want to connect
/// @param networks a string containing a slash-delimited list of network
/// names we might use to connect to the service, listed in order of
/// descending preference. IS MY TERMINOLOGY BOGUS HERE?
arPhleetAddress arSZGClient::discoverService(const string& serviceName,
                                             const string& networks,
                                             bool   async){
  arPhleetAddress result;
  if (!_connected){
    result.valid = false;
    return result;
  }

  // pack the data
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_REQUEST_SERVICE);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_COMPUTER, _computerName);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_TAG, serviceName);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_NETWORKS, networks);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_ASYNC,
    async ? "SZG_TRUE" : "SZG_FALSE");
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send discover service "
	             << "request.\n";
    result.valid = false;
    _dataParser->recycle(data);
    return result;
  }
  _dataParser->recycle(data);

  // receive the response
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data){
    ar_log_error() << _exeName << " error: failed to get broker result in response "
	           << "to discover service request.\n";
    result.valid = false;
    return result;
  }
  (void)data->getDataInt(_l.AR_PHLEET_MATCH);

  // deal with the response
  if (data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) == "SZG_SUCCESS"){
    result.valid = true;
    result.address = data->getDataString(_l.AR_SZG_BROKER_RESULT_ADDRESS);
    int dimension = data->getDataDimension(_l.AR_SZG_BROKER_RESULT_PORT);
    result.numberPorts = dimension;
    data->dataOut(_l.AR_SZG_BROKER_RESULT_PORT, result.portIDs,
                  AR_INT, dimension);
  }
  else{
    result.valid = false;
  }
  _dataParser->recycle(data);
  return result;
}

/// The code to print either pending service requests or active services
void arSZGClient::_printServices(const string& type){
  if (!_connected){
    return;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_GET_SERVICES);
  int match = _fillMatchField(data);
  // we want to get the "registered" or "active" services
  data->dataInString(_l.AR_SZG_GET_SERVICES_TYPE, type);
  // As usual, "NULL" is our reserved string. In this case, it tells the
  // szgserver to return all services instead of just a particular one
  data->dataInString(_l.AR_SZG_GET_SERVICES_SERVICES, "NULL");
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send service listing request.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data){
    ar_log_error() << _exeName << " error: no response to get services request.\n";
    return;
  }
  const string services(data->getDataString(_l.AR_SZG_GET_SERVICES_SERVICES));
  const arSlashString
    computers(data->getDataString(_l.AR_SZG_GET_SERVICES_COMPUTERS));
  int* IDs = (int*) data->getDataPtr(_l.AR_SZG_GET_SERVICES_COMPONENTS,
                                     AR_INT);
  int number = data->getDataDimension(_l.AR_SZG_GET_SERVICES_COMPONENTS);
  // print out the stuff
  int where = 0;
  for (int i=0; i<number; i++){
    // SHOULD NOT be one of the ar_log_*. This is a rare case where we are actually printing out to 
    // the terminal.
    cout << computers[i] << ";"
         << ar_pathToken(services,where) << ";"
         << IDs[i] << "\n";
  }
  _dataParser->recycle(data);
}

/// Request that the szgserver send us a notification when the named
/// service is no longer actively held by a component in the system.
/// Returns the match to be used with the other side of this call on
/// success or -1 on failure.
int arSZGClient::requestServiceReleaseNotification(const string& serviceName){
  if (!_connected){
    return -1;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_RELEASE);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_RELEASE_NAME, serviceName);
  data->dataInString(_l.AR_SZG_SERVICE_RELEASE_COMPUTER, _computerName);
  const bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok){
    ar_log_warning() << _exeName
                     << " warning: failed to request service release notification.\n";
    match = -1;
  }
  // The response will occur in getServiceReleaseNotification.
  return match;
}

/// Receive the service release notification triggered in the szgserver
/// by the previous method. For thread-safety, we only try to get data from
/// a list of tags. There is also an optional timeout argument. If we
/// succeed, we simply return the match upon which we were successful.
int arSZGClient::getServiceReleaseNotification(list<int> tags,
                                               int timeout){
  if (!_connected){
    return -1;
  }
  // block until the response occurs
  arStructuredData* data;
  int match = _getTaggedData(data, tags, _l.AR_SZG_SERVICE_RELEASE, timeout);
  if (match < 0){
    ar_log_error() << _exeName << " error: failed to get service release "
	           << "notification.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

/// Tries to get the "info" associated with the given (full) service name.
/// Confusingly, at present, this returns the empty string both if the service
/// fails to exist AND if the service has the empty string as its info!
string arSZGClient::getServiceInfo(const string& serviceName){
  if (!_connected){
    return string("");
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_OP, "get");
  data->dataInString(_l.AR_SZG_SERVICE_INFO_TAG, serviceName);
  const bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok){
    ar_log_warning() << _exeName
                     << " warning: failed to request service info.\n";
    return string("");
  }
  // Now, get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data){
    ar_log_error() << _exeName << " error: no response to get service info.\n";
    return string("");
  }
  string result = data->getDataString(_l.AR_SZG_SERVICE_INFO_STATUS);
  _dataParser->recycle(data);
  return result;
}

/// Attempts to set the "info" string associated with the particular service.
bool arSZGClient::setServiceInfo(const string& serviceName,
                                 const string& info){
  if (!_connected){
    return false;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_OP, "set");
  data->dataInString(_l.AR_SZG_SERVICE_INFO_TAG, serviceName);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_STATUS, info);
  bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok){
    ar_log_warning() << _exeName
                     << " warning: failed to set service info.\n";
    return false;
  }
  // Get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data){
    ar_log_error() << _exeName << " error: no response to set service info.\n";
    return false;
  }
  ok = data->getDataString(_l.AR_SZG_SERVICE_INFO_STATUS) == "SZG_SUCCESS";
  _dataParser->recycle(data);
  return ok;
}

/// Contacts the szgserver and obtains the listing of running services,
/// including the computers on which they run, and the phleet IDs of
/// the components hosting them. This information is then printed in
/// the style of dps.
void arSZGClient::printServices(){
  _printServices("active");
}

/// Contacts the szgserver and obtains the list of pending service requests,
/// including the computers on which they are running and the phleet IDs of
/// the components making them. This information is then printed in the style
/// of dps. Note that between this command and the printServices() command,
/// one can diagnose connection brokering problems in the distributed system.
void arSZGClient::printPendingServiceRequests(){
  _printServices("pending");
}

/// Sometimes it is desirable to be able to directly retrieve the phleet ID
/// of the component running a service. For instance, open launching an
/// application on a virtual computer, we may wish to kill a previous
/// application that has been running and supplying a service that the
/// new application wishes to supply. The ID of the component is returned
/// if the service in question does, in fact, exist. Otherwise, -1 is
/// returned.
/// @param serviceName the name of the service in question
int arSZGClient::getServiceComponentID(const string& serviceName){
  if (!_connected){
    return -1;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_GET_SERVICES);
  int match = _fillMatchField(data);
  // As usual, "NULL" is our reserved string. In this case, it tells the
  // szgserver to return all services instead of just a particular one
  data->dataInString(_l.AR_SZG_GET_SERVICES_TYPE, "active");
  data->dataInString(_l.AR_SZG_GET_SERVICES_SERVICES, serviceName);
  if (!_dataClient.sendData(data)){
    ar_log_warning() << _exeName << " warning: failed to send service ID request.\n";
    _dataParser->recycle(data);
    return -1;
  }
  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data){
    ar_log_error() << _exeName << " error: no response to service ID request.\n";
    return -1;
  }
  int* IDs = (int*) data->getDataPtr(_l.AR_SZG_GET_SERVICES_COMPONENTS,
                                     AR_INT);
  int result = IDs[0];
  _dataParser->recycle(data);
  return result;
}

/// Returns the networks on which this component should operate. Used
/// as an arg for discoverService(...). Note that there are three ways
/// that the networks can be set (in order of increasing precedence):
/// the phleet config file, the phleet command line args, and the context.
/// @param channel Should be either default, graphics, sound, or input.
/// This lets network traffic be routed independently for various
/// traffic types.
arSlashString arSZGClient::getNetworks(const string& channel){
  if (channel == "default"){
    return _networks;
  }
  if (channel == "graphics"){
    return _graphicsNetworks == "NULL" ? _networks : _graphicsNetworks;
  }
  if (channel == "sound"){
    return _soundNetworks == "NULL" ? _networks : _soundNetworks;
  }
  if (channel == "input"){
    return _inputNetworks == "NULL" ? _networks : _inputNetworks;
  }
  ar_log_warning() << _exeName << " warning: unknown channel for getNetworks().\n";
  return _networks;
}

/// Returns the network addresses upon which this component will offer
/// services. Used when registering a service to determine the
/// addresses of the interfaces to which other components will try to
/// connect.
arSlashString arSZGClient::getAddresses(const string& channel){
  if (channel == "default"){
    return _addresses;
  }
  if (channel == "graphics"){
    return _graphicsAddresses == "NULL" ? _addresses : _graphicsAddresses;
  }
  if (channel == "sound"){
    return _soundAddresses == "NULL" ? _addresses : _soundAddresses;
  }
  if (channel == "input"){
    return _inputAddresses == "NULL" ? _addresses : _inputAddresses;
  }
  ar_log_warning() << _exeName << " warning: unknown channel for getAddresses().\n";
  return _addresses;
}

/// If this component is operating as part of a virtual computer, return
/// its name. Otherwise, return "NULL". As with networks, the
/// virtual computer is set via a combination of the phleet command line
/// args, and the context.
const string& arSZGClient::getVirtualComputer(){
  return _virtualComputer;
}

/// Returns the mode under which this component is operating (with respect
/// to a particular channel). For instance, the "default" channel mode is
/// one of "trigger", "master" or "component". This refers to the global
/// character of what this piece of the system does. The "graphics" channel
/// mode is one of "screen0", "screen1", "screen2", etc.
const string& arSZGClient::getMode(const string& channel){
  if (channel == "default")
    return _mode;
  if (channel == "graphics")
    return _graphicsMode;

  ar_log_warning() << _exeName << " warning: unknown channel for getMode().\n";
  return _mode;
}

/// Queries the szgserver to determine the computer designated as the
/// trigger for a given virtual computer. Returns "NULL" if the virtual
/// computer is invalid or if it does not have a trigger defined.
string arSZGClient::getTrigger(const string& virtualComputer){
  if (getAttribute(virtualComputer, "SZG_CONF", "virtual", "") != "true"){
    // not a virtual computer
    return "NULL";
  }
  return getAttribute(virtualComputer, "SZG_TRIGGER", "map", "");
}

/// Phleet components should use service names that are compartmentalized
/// so as to allow users to control sharing of services. There are 3 cases:
/// 1. Application component was NOT launched as part of a virtual computer:
///    In this case, the phleet user name provides the compartmenting:
///    (for example, SZG_BARRIER/ben)
/// 2. Application component launched as part of virtual computer FOO, which
///    has no "location" defined. In this case, the virtual computer
///    name compartments the service. (FOO/SZG_BARRIER)
/// 3. Application component launched as part of a virtual computer FOO,
///    has a "location" component BAR. In this case, the "location" name
///    compartments the service (BAR/SZG_BARRIER).
/// This described the complex service name.
string arSZGClient::createComplexServiceName(const string& serviceName){
  // At the blurry boundary between string and arSlashString.
  if (_virtualComputer == "NULL"){
    return serviceName+string("/")+_userName;
  }
  // NOTE: the trailing empty string is necessary to avoid using the WRONG
  // getAttribute(...), i.e. the one that lists default values.
  string location = getAttribute(_virtualComputer,"SZG_CONF","location","");
  if (location == "NULL"){
    return _virtualComputer+string("/")+serviceName;
  }
  else{
    return location+string("/")+serviceName;
  }
}

/// Create a context string form internal storage and returns it.
/// Used, for instance, in generating the launch info header.
string arSZGClient::createContext(){
  string result(string("virtual=")+_virtualComputer+string(";")+
    string("mode/default=")+_mode+string(";")+string("log=")+ar_logLevelToString(_logLevel));

  // Additional mode stuff.
  if (_graphicsMode != "NULL")
    result += string(";mode/graphics=")+_graphicsMode;
  result += string(";")+string("networks/default=")+_networks;

  // Additional network stuff.
  if (_graphicsNetworks != "NULL")
    result += string(";networks/graphics=")+_graphicsNetworks;
  if (_soundNetworks != "NULL")
    result += string(";networks/sound=")+_soundNetworks;
  if (_inputNetworks != "NULL")
    result += string(";networks/input=")+_inputNetworks;

  return result;
}

/// Creates a context string from the parameters. IMPORTANT NOTE: this
/// DOES NOT set the related internal variables. This is merely a way
/// to encapsulate a data format that may change over time.
/// NOTE: this is a little BOGUS as there is currently no way to specify
/// the channels upon which multiple services operate!
string arSZGClient::createContext(const string& virtualComputer,
                                  const string& modeChannel,
                                  const string& mode,
                                  const string& networksChannel,
                                  const arSlashString& networks){
  return string("virtual=")+virtualComputer+string(";")+
    string("mode/")+modeChannel+string("=")+mode+string(";")+
    string("networks/")+networksChannel+string("=")+networks;
}

/// The actual attempt to connect to the szgserver occurs here.
bool arSZGClient::_dialUpFallThrough(){
  if (_connected){
    ar_log_warning() << _exeName << " warning: already connected to "
                     << _IPaddress << ":" << _port << ar_endl;
    return false;
  }

  if (!_dataClient.dialUpFallThrough(_IPaddress.c_str(), _port)){
    // Connect to the szgserver at the designated location.
    // If we don't connect, this is an error.
    ar_log_error() << _exeName << " error: szgserver not found (" << _IPaddress 
                   << ", " << _port << ").\n"
	           << "\t(first dlogin;  type dhunt to find an szgserver.\n";
    return false;
  }

  _connected = true;
  if (!_clientDataThread.beginThread(arSZGClientDataThread, this)) {
    ar_log_error() << _exeName << " error: failed to start client data thread.\n";
    return false;
  }

  _dataParser = new arStructuredDataParser(_l.getDictionary());

  return true;
}

arStructuredData* arSZGClient::_getDataByID(int recordID){
  return _dataParser->getNextInternal(recordID);
}

arStructuredData* arSZGClient::_getTaggedData(int tag,
                                              int recordID,
                                              int timeout){
  list<int> tags;
  tags.push_back(tag);
  arStructuredData* message = NULL;
  if (_dataParser->getNextTaggedMessage(message, tags, recordID, timeout) < 0){
    // timeout or other error
    return NULL;
  }
  // We actually got the stuff.
  return message;
}

int arSZGClient::_getTaggedData(arStructuredData*& message,
                                list<int> tags,
				int recordID,
				int timeout){
  return _dataParser->getNextTaggedMessage(message, tags, recordID, timeout);
}

bool arSZGClient::_getMessageAck(int match, const char* transaction, int* id,
                                 int timeout){
  arStructuredData* ack = _getTaggedData(match,
                                         _l.AR_SZG_MESSAGE_ACK,
                                         timeout);
  if (!ack){
    ar_log_warning() << _exeName << " warning: no ack for " << transaction << ".\n";
    return false;
  }

  const bool ok =
    ack->getDataString(_l.AR_SZG_MESSAGE_ACK_STATUS) == "SZG_SUCCESS";
  // Return ID, if requested and if we succeeded
  // NOTE: in the case of SZG_FAILURE, we will not have any relevant ID data
  if (id && ok)
    *id = ack->getDataInt(_l.AR_SZG_MESSAGE_ACK_ID);
  _dataParser->recycle(ack);
  return ok;
}

string arSZGClient::_getAttributeResponse(int match) {
  arStructuredData* ack = _getTaggedData(match, _l.AR_ATTR_GET_RES);
  if (!ack) {
    ar_log_warning() << _exeName << " warning: unexpected response from szgserver.\n";
    return string("NULL");
  }

  string result(ack->getDataString(_l.AR_ATTR_GET_RES_VAL));
  _dataParser->recycle(ack);
  return result;
}

/// Set our internal label and set the communicate with the szgserver to set
/// our label there (which is a little different from the internal one, being
/// <computer>/<exe_name>). Note that this function should only be called from
/// init(...) and is consequently private. Finally, this function is
/// important in that it makes sure that the szgserver name propogates
/// to the client (in the return connection ack). Without this feature, the
/// "explicit connection" mode of dlogin will NOT get the szgserver name.
bool arSZGClient::_setLabel(const string& label){
  if (!_connected)
    return false;

  // Need to get some storage for the message.
  arStructuredData* connectionAckData
    = _dataParser->getStorage(_l.AR_CONNECTION_ACK);
  const string fullLabel = _computerName + "/" + label;
  bool state = false;
  int match = _fillMatchField(connectionAckData);
  if (!connectionAckData->dataInString(_l.AR_CONNECTION_ACK_LABEL,
                                       fullLabel) ||
      !_dataClient.sendData(connectionAckData)){
    ar_log_warning() << _exeName << " warning: failed to set label.\n";
  }
  else{
    _exeName = label;
    _dataClient.setLabel(_exeName);
    arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
    if (!ack){
      ar_log_warning() << _exeName << " warning: failed to get szgserver name.\n";
    }
    else{
      _serverName = ack->getDataString(_l.AR_CONNECTION_ACK_LABEL);
      _dataParser->recycle(ack);
      state = true;
    }
  }
  // Must recycle storage.
  _dataParser->recycle(connectionAckData);
  return state;
}

/// Parses the string in the environment variable SZGCONTEXT, to determine
/// over-ride settings for virtual computer, mode, and networks
bool arSZGClient::_parseContext(){
  const string context(ar_getenv("SZGCONTEXT"));
  if (context == "NULL"){
    // the environment variable has not been set. consequently, there's
    // nothing to do. NOTE: this IS NOT an error.
    return true;
  }
  // there are three components to the context
  int position = 0;
  while (position < int(context.length())-1){
    const string pair(ar_pathToken(context, position));
    if (!_parseContextPair(pair)){
      return false;
    }
  }
  return true;
}

/// It is important that we pass argc by reference since we are potentially
/// changing the arg list.
/// Every time a client is launched, we go ahead and examine the "phleet args",
/// i.e. those prefaced by a -szg. In most cases, we will delete these args
/// (if _parseSpecialPhleetArgs is in its default state of true) while
/// parsing them. However, if _parseSpecialPhleetArgs is false (as is the case
/// in dex), then only the args user and server (pertaining to login) are
/// parsed and removed.
bool arSZGClient::_parsePhleetArgs(int& argc, char** argv){
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i],"-szg")){
      // we have found an arg that might need to be removed.
      if (i+1 >= argc){
        ar_log_error() << _exeName << " error: expected something after -szg flag.\n";
	return false;
      }
      // we need to figure out which args these are.
      // THIS IS CUT_AND_PASTED FROM _parseContextPair below
      const string thePair(argv[i+1]);
      const unsigned location = thePair.find('=');
      if (location == string::npos){
        ar_log_error() << "arSZGClient error: missing '=' in context pair.\n";
        return false;
      }
      unsigned int length = thePair.length();
      if (location == length-1){
        ar_log_error() << "arSZGClient error: incomplete context pair.\n";
        return false;
      }
      // Everything is in the format FOO=BAR, where FOO can be
      // "virtual", "mode", "networks/default", "networks/graphics",
      // "networks/sound", "networks/input"...
      // consequently, pair1Type can be "virtual", "mode", or "networks"
      const arSlashString pair1(thePair.substr(0,location));
      const string pair1Type(pair1[0]);

      // parse the arg exactly if we are parsing all args or if it is one
      // of the special ones (user or server) that are always parsed.
      if (_parseSpecialPhleetArgs
	  || pair1Type == "user" || pair1Type == "server"){
        if (!_parseContextPair(argv[i+1])){
          return false;
        }
        // if they are parsed, then they should also be erased
        for (int j=i; j<argc-2; j++){
          argv[j] = argv[j+2];
        }
        // reset the arg count and i
        argc = argc-2;
        i--;
      }
      else{
	// we didn't parse these args... move on (without this ++ we will
	// be at i+1 at the beginning of the next loop and we should be at
	// i+2
        i++;
      }
    }
  }
  return true;
}

/// Helper function for _parseContext that takes the FOO=BAR components
/// of that string and does the right thing with them. Note that "NULL"
/// is also allowed as a do-nothing value.
bool arSZGClient::_parseContextPair(const string& thePair){
  // the context pair should be seperated by a single =
  unsigned int location = thePair.find('=');
  if (location == string::npos){
    ar_log_error() << _exeName << " error: missing '=' in context pair.\n";
    return false;
  }
  unsigned int length = thePair.length();
  if (location == length-1){
    ar_log_error() << "arSZGClient error: missing '=' in context pair.\n";
    return false;
  }
  // Everything is in the format FOO=BAR, where FOO can be
  // "virtual", "mode", "networks/default", "networks/graphics",
  // "networks/sound", "networks/input"...
  // consequently, pair1Type can be "virtual", "mode", or "networks"
  const arSlashString pair1(thePair.substr(0,location));
  const string pair1Type(pair1[0]);
  const string pair2(thePair.substr(location+1, length - location - 1));
  // if the second value is "NULL", we do nothing
  if (pair2 == "NULL")
    return true;

  if (pair1Type == "virtual"){
    _virtualComputer = pair2;
  }
  else if (pair1Type == "mode"){
    if (pair1.size() != 2){
      ar_log_error() << _exeName << " error: parseContextPair() got mode "
	             << "data without\n specified channel.\n";
      return false;
    }
    const string modeChannel(pair1[1]);
    if (modeChannel == "default"){
      _mode = pair2;
    }
    else if (modeChannel == "graphics"){
      _graphicsMode = pair2;
    }
    else{
      ar_log_error() << _exeName << " error: parseContextPair() got invalid "
	             << "mode channel.\n";
      return false;
    }
  }
  else if (pair1Type == "networks"){
    // Return true iff the networks value is valid,
    // and set _networks and _addresses appropriately.
    if (pair1.size() != 2){
      ar_log_error() << _exeName << " error: "
                     << "no channel in parseContextPair()'s networks data.\n";
      return false;
    }
    return _checkAndSetNetworks(pair1[1], pair2);
  }
  else if (pair1Type == "parameter_file"){
    _parameterFileName = pair2;
  }
  else if (pair1Type == "server"){
    arSlashString serverLocation(pair2);
    if (serverLocation.size() != 2){
      ar_log_error() << "arSZGClient error: "
	             << "server command line parameter needs a slash string "
	             << "of length 2.\n";
      return false;
    }
    _IPaddress = serverLocation[0];
    // AARGH! fixed size buffer.... very bad!
    char buffer[1024];
    ar_stringToBuffer(serverLocation[1], buffer, 1024);
    _port = atoi(buffer);
  }
  else if (pair1Type == "user"){
    _userName = pair2;
  }
  else if (pair1Type == "log"){
    _logLevel = ar_stringToLogLevel(pair2);
    ar_log().setLogLevel(_logLevel);
  }
  else{
    ar_log_error() << _exeName << " error: context pair has unknown type \""
                   << pair1Type << "\".\n";
    return false;
  }
  return true;
}

/// networks contains various network names. channel is one of "default",
/// "graphics", "input", or "sound". This allows for traffic shaping.
bool arSZGClient::_checkAndSetNetworks(const string& channel, const arSlashString& networks){
  // sanity check!
  if (channel != "default" && channel != "graphics" && channel != "input"
      && channel != "sound"){
    ar_log_error() << _exeName << " error: _checkAndSetNetworks() got unknown channel \""
                   << channel << "\".\n";
    return false;
  }
  const int numberNewNetworks = networks.size();
  const int numberCurrentNetworks = _networks.size();
  // Check to see if every one of the new networks is, in fact, one of the
  // current networks. If not, we have an error.
  int i=0, j=0;
  for (i=0; i<numberNewNetworks; i++){
    bool match = false;
    for (j=0; j<numberCurrentNetworks && !match; j++){
      if (networks[i] == _networks[j])
	match = true;
    }
    if (!match){
      ar_log_error() << _exeName << " error: virtual computer's network \""
                     << networks[i] << "\" is undefined in szg.conf.\n";
      return false;
    }
  }
  // Each one of the new networks matched.  Get addresses.
  // Inefficient, but one host doesn't have many NICs.
  arSlashString newAddresses;
  for (i=0; i<numberNewNetworks; i++){
    for (j=0; j<numberCurrentNetworks; j++){
      if (networks[i] == _networks[j])
	newAddresses /= _addresses[j];
    }
  }
  if (channel == "default"){
    _networks = networks;
    _addresses = newAddresses;
  }
  else if (channel == "graphics"){
    _graphicsNetworks = networks;
    _graphicsAddresses = newAddresses;
  }
  else if (channel == "sound"){
    _soundNetworks = networks;
    _soundAddresses = newAddresses;
  }
  else if (channel == "input"){
    _inputNetworks = networks;
    _inputAddresses = newAddresses;
  }
  return true;
}

/// Generates a standard informational header begins the text of all messages
/// sent back to dex from a launched executable
string arSZGClient::_generateLaunchInfoHeader(){
  stringstream resultStream;
  resultStream << "*user=" << _userName << ", "
	       << "context=" << createContext() << "\n"
	       << "*computer=" << _computerName << ", "
	       << "executable=" << _exeName << "\n";
  return resultStream.str();
}

/// It could be the case that we are relying on a locally parsed config
/// file. (i.e. standalone mode)
string arSZGClient::_getAttributeLocal(const string& computerName,
				       const string& groupName,
				       const string& parameterName,
                                       const string& validValues){
  const string query =
    ((computerName == "NULL") ? _computerName : computerName) + "/"
    + groupName + "/" + parameterName;
  string result;
  map<string, string, less<string> >::iterator i =
    _localParameters.find(query);
  if (i == _localParameters.end()){
    result = string("NULL");
  }
  else{
    result = i->second;
  }
  return _changeToValidValue(groupName, parameterName, result, validValues);
}

/// It could be the case that we are relying on a locally parsed config
/// file. (i.e. standalone mode)
bool arSZGClient::_setAttributeLocal(const string& computerName,
				     const string& groupName,
				     const string& parameterName,
				     const string& parameterValue){
  const string query =
    ((computerName == "NULL") ? _computerName : computerName) + "/"
    + groupName + "/" + parameterName;
  map<string, string, less<string> >::iterator i =
    _localParameters.find(query);
  if (i != _localParameters.end()){
    _localParameters.erase(i);
  }
  _localParameters.insert(map<string,string,less<string> >::value_type
                          (query,parameterValue));
  return true;
}

/// It could be the case that we are relying on a locally parsed config
/// file. (i.e. standalone mode). In this case, we are pulling a so-called
/// "global" attribute (i.e. one not tied to a computer) out of storage.
/// Since we are locally getting a "global" attribute this leads to the
/// following silly function name...
string arSZGClient::_getGlobalAttributeLocal(const string& attributeName){
  string result;
  map<string, string, less<string> >::iterator i =
    _localParameters.find(attributeName);
  if (i == _localParameters.end()){
    result = string("NULL");
  }
  else{
    result = i->second;
  }
  return result;
}

/// It could be the case that we are relying on a locally parsed config
/// file. (i.e. standalone mode). In this case, we are putting a so-called
/// "global" attribute (i.e. one not tied to a computer) into storage.
/// Since we are locally setting a "global" attribute this leads to the
/// following silly function name...
bool arSZGClient::_setGlobalAttributeLocal(const string& attributeName,
					   const string& attributeValue){
  map<string, string, less<string> >::iterator i =
    _localParameters.find(attributeName);
  if (i != _localParameters.end()){
    _localParameters.erase(i);
  }
  _localParameters.insert(map<string,string,less<string> >::value_type
                          (attributeName,attributeValue));
  return true;
}

/// For string constants in the parameter database, the arSZGClient
/// can check the value against a list of valid values. If the string
/// constant is not in the list, the first item of the list is returned
/// and a warning is printed.
string arSZGClient::_changeToValidValue(const string& groupName,
                                        const string& parameterName,
                                        const string& value,
                                        const string& validValues){
  if (validValues != "") {
    // String format is "|foo|bar|zip|baz|bletch|".
    if (validValues[0] != '|' || validValues[validValues.length()-1] != '|') {
      ar_log_warning() << _exeName
                       << " warning: getAttribute ignoring malformed validValues\n\t\""
                       << validValues << "\".\n";
      return value;
    }
    else if (validValues.find('|'+value+'|') == string::npos) {
      const int end = validValues.find('|', 1);
      const string valueNew(validValues.substr(1, end-1));
      // ONLY COMPLAIN IF WE ARE CHANGING A VALUE THAT HAS BEEN EXPLICITLY
      // SET.
      if (value != "NULL") {
	ar_log_warning() << _exeName << " warning: "
	                 << groupName+'/'+parameterName << " should be one of "
	                 << validValues << ",\n    but is " << value
	                 << ".  Using default instead (" << valueNew << ").\n";
      }
      return valueNew;
    }
  }
  // If we get here, either the valid values string is undefined or our value
  // matches one of the valid values.
  return value;
}

/// A somewhat baroque construction that lets us get configuration information
/// from the szgservers on the network.
void arSZGClient::_serverResponseThread() {
  char buffer[200];
  while (_keepRunning) {
    arSocketAddress fromAddress;
    while (_discoverySocket->ar_read(buffer, 200, &fromAddress) < 0) {
      ar_usleep(10000);
      // Win32 returns -1 if no packet was received
      // (and therefore the data below would be garbage).
    }

    // We got a packet.
    ar_usleep(10000); // avoid busy-waiting on Win32
    ar_mutex_lock(&_queueLock);
    if (_dataRequested){
      // Make sure it is the right format. Both version number (first four
      // bytes) and that it is a response (5th byte = 1).
      if (buffer[0] == 0 && buffer[1] == 0 &&
          buffer[2] == 0 && buffer[3] == SZG_VERSION_NUMBER && buffer[4] == 1){
        memcpy(_responseBuffer,buffer,200);
        if (_justPrinting){
	  // Print out the contents of this packet.
          stringstream serverInfo;
          serverInfo << _responseBuffer+5  << "/"
	             << _responseBuffer+132 << ":"
	             << _responseBuffer+164;
          // Check to see that we haven't found it already.
	  // This is possible since response packets are broadcast
	  // and someone else on the network could be generating them.
	  bool found = false;
          for (list<string>::iterator i = _foundServers.begin();
	       i != _foundServers.end(); i++){
            if (*i == serverInfo.str()){
	      // It's a duplicate to something we've already found.
	      found = true;
	      break;
	    }
	  }
	  if (!found){
	    // SHOULD NOT be one of the ar_log_*. This is a rare case where we intend to print to the 
	    // terminal.
            cout << serverInfo.str() << "\n";
            _foundServers.push_back(serverInfo.str());
	  }
        }
        else {
          // If this matches the requested name, stop, discarding
	  // subsequent packets.
	  if (_requestedName == string(_responseBuffer+5)){
            _dataRequested = false;
            _dataCondVar.signal();
	  }
        }
      }
    }
    ar_mutex_unlock(&_queueLock);
  }
}

void arSZGClient::_timerThread(){
  while (_keepRunning) {
    ar_mutex_lock(&_queueLock);
    while (!_beginTimer){
      _timerCondVar.wait(&_queueLock);
    }
    _beginTimer = false;
    ar_mutex_unlock(&_queueLock);
    // a long time-out is desirable here... what if the network
    // infrastructure is unreliable or flaky?
    ar_usleep(2000000); // 2 second time-out
    ar_mutex_lock(&_queueLock);
    _dataCondVar.signal();
    ar_mutex_unlock(&_queueLock);
  }
}

void arSZGClient::_dataThread() {
  while (_keepRunning){
    if (!_dataClient.getData(_receiveBuffer, _receiveBufferSize)){
      // Don't complain if the destructor has already been invoked.
      // Should do something here, like maybe disconnect.
      ar_log_remark() << _exeName << " remark: szgserver disconnected; queues cleared.\n";
      goto LDone;
    }
    int size = -1; // aren't actually going to use this here
    // The data distribution needs to be multi-threaded. We can assume
    // that the messages are received in a single thread (which will
    // be internal to the arSZGClient before too long). However, everything
    // else is in response to something we've sent to the szgserver and
    // hence has a "match" to enable multi-threading.
    if (ar_rawDataGetID(_receiveBuffer) == _l.AR_SZG_MESSAGE){
      _dataParser->parseIntoInternal(_receiveBuffer, size);
    }
    else if (ar_rawDataGetID(_receiveBuffer) == _l.AR_KILL){
      // Yes, we really do need to GO AWAY at this point.
      // NOTE: the clearQueues() call makes every future call for messages
      // fail! And interrupts any waiting ones as well!
      ar_log_remark() << _exeName
                      << " remark: szgserver forcibly disconnected; queues cleared.\n";
      goto LDone;
    }
    else{
      arStructuredData* data = _dataParser->parse(_receiveBuffer, size);
      _dataParser->pushIntoInternalTagged(
        data, data->getDataInt(_l.AR_PHLEET_MATCH));
    }
  }
  return;

LDone:
  _dataParser->clearQueues();
  // HMMMM.... should the below go inside a LOCK?
  _connected = false;
  // No need to get any more data!
}

// The format of the discovery and response packets is described in
// szgserver.cpp.
// 4620 is the magic port upon which the szgserver listens for discovery
// packets. 4621 is the magic port upon which the response returns.
void arSZGClient::_sendDiscoveryPacket(const string& name,
                                       const string& broadcast){
  char buf[200];
  // Set all the bytes to zero.
  memset(buf, 0, sizeof(buf));

  // The first 3 bytes are 0. This is the beginning of the magic version
  // number.
  // The fourth byte contains the magic version number.
  buf[3] = SZG_VERSION_NUMBER;
  // Since the 5th byte is 0, this is a discovery packet.

  ar_stringToBuffer(name, buf+5, 127);
  arSocketAddress broadcastAddress;
  broadcastAddress.setAddress(broadcast.c_str(),4620);
  _discoverySocket->ar_write(buf, sizeof(buf), &broadcastAddress);
}

/// Sends a broadcast packet on a specified subnet to find the szgserver
/// with the specified name.
bool arSZGClient::discoverSZGServer(const string& name,
                                    const string& broadcast){
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()){
    ar_log_error() << _exeName << " error: failed to launch discovery threads.\n";
    return false;
  }

  ar_mutex_lock(&_queueLock);
  _dataRequested = true;
  _beginTimer = true;
  _requestedName = name;
  _foundServers.clear();
  _sendDiscoveryPacket(name,broadcast);
  _timerCondVar.signal();
  while (_dataRequested && _beginTimer){
    _dataCondVar.wait(&_queueLock);
  }
  if (_dataRequested){
    // timeout
    ar_mutex_unlock(&_queueLock);
    return false;
  }

  // We actually got something.
  const string theServerName(_responseBuffer+5);
  const string theServerIP  (_responseBuffer+132);
  const string theServerPort(_responseBuffer+164);

  // to internal storage
  _IPaddress = theServerIP;
  _port = atoi(theServerPort.c_str());
  _serverName = theServerName;

  ar_mutex_unlock(&_queueLock);
  return true;
}

/// Tries to find any szgservers out there and print their names. Useful
/// for seeing who's running what.
void arSZGClient::printSZGServers(const string& broadcast){
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()){
    // Do not complain here. A complaint will have occured further
    // down in the stack.
    return;
  }
  _justPrinting = true;

  ar_mutex_lock(&_queueLock);
  _dataRequested = true;
  _beginTimer = true;
  _requestedName = "";
  _sendDiscoveryPacket("*",broadcast);
  _timerCondVar.signal();
  while (_beginTimer){
    _dataCondVar.wait(&_queueLock);
  }
  ar_mutex_unlock(&_queueLock);

  _dataRequested = false;
  _justPrinting = false;
}

/// Sets the server location manually. Necessary if we are unable
/// to reach the szgserver via broadcast packets. Note that the
/// szgserver name will be set upon actual connection via a handshake
void arSZGClient::setServerLocation(const string& IPaddress, int port){
  _IPaddress = IPaddress;
  _port = port;
}

/// Writes a login file using data gathered from, for instance,
/// discoverSZGServer(...)
bool arSZGClient::writeLoginFile(const string& userName){
  // NOTE that userName is not necessarily the same as _userName
  // the internal storage refers to the effective user name of this
  // component, which can be over-riden by the environment variable
  // SZGUSER. userName is the phleet login name in the config file
  _configParser.setUserName(userName);
  _configParser.setServerName(_serverName);
  _configParser.setServerIP(_IPaddress);
  _configParser.setServerPort(_port);
  return _configParser.writeLoginFile();
}

/// Changes the login file to have the not-logged-in values
bool arSZGClient::logout(){
  _configParser.setUserName("NULL");
  _configParser.setServerName("NULL");
  _configParser.setServerIP("NULL");
  _configParser.setServerPort(0);
  return _configParser.writeLoginFile();
}

/// Default message task which handles "quit" and nothing else.
void ar_messageTask(void* pClient){
  if (!pClient) {
    ar_log_warning() << "ar_messageTask warning: no syzygy client, dkill disabled.\n";
    return;
  }

  arSZGClient* cli = (arSZGClient*)pClient;
  while (true){
    string messageType, messageBody;
    cli->receiveMessage(&messageType, &messageBody);
    if (messageType == "quit"){
      cli->closeConnection();
      // Do this?  But it's private.  cli->_keepRunning = false;
      exit(0);
    }
  }
}
