//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arPhleetConfigParser.h"
#include "arXMLUtilities.h"

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
  _graphicsMode("SZG_SCREEN0"),
  _parameterFileName("szg_parameters.txt"),
  _virtualComputer("NULL"),
  _connected(false),
  _receiveBuffer(NULL),
  _receiveBufferSize(15000),
  _launchingMessageID(0),
  _dexHandshaking(false),
  _simpleHandshaking(true),
  _parseSpecialPhleetArgs(true),
  _nextMatch(0),
  _beginTimer(false),
  _dataRequested(false),
  _keepRunning(true),
  _justPrinting(false)
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
  // on the Unix side, we might need to finish a handshake with the
  // szgd, telling it we've been successfully forked
  // THIS MUST OCCUR BEFORE dialUpFallThrough

  // ANOTHER IMPORTANT NOTE: IT IS ASSUMED THAT THIS FUNCTION WILL ALWAYS,
  // REGARDLESS OF WHETHER OR NOT IT IS SUCCESSFUL FINISH THE HANSHAKE WITH
  // DEX!!! Conequently, no return statements, except at the end.
  bool success = true; // assume suceess unless there has been a 
                       // specific failure
  const string pipeIDString(ar_getenv("SZGPIPEID"));
  if (pipeIDString != "NULL"){
    _dexHandshaking = true;
    // we have, in fact, been successfully spawned on the Unix side
    const int pipeID = atoi(pipeIDString.c_str());
    // Send the success code. Note that on Win32 we set the pipe ID to -1,
    // which gets us into here (to set _dexHnadshaking to true...
    // but we don't want to try to write to the pipe on Win32 since the
    // function is unimplemented
    if (pipeID >= 0){
      char numberBuffer[8] = "\001";
      if (!ar_safePipeWrite(pipeID, numberBuffer, 1)){
        cout << _exeName << " remark: unterminated pipe-based handshake.\n";
      }
    }
  }

  // if we made it here, we managed to successfully parse the phleet
  // config file, parse the login file, and connect to the szgserver.
  // the phleet config file contains a list of networks through which
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
  // Phleet args so that the _networks and _addresses can be set.
  // This is a duplication from dialUpFallThrough...
  if (!_configParser.parseConfigFile()){
    // if this fails, just silently accept it. It will be dealt with later
    // in _dialUpFallThrough
  }
  // if the networks and addresses were NOT set via command line args or
  // environment variables, go ahead and set them now.
  // NOTE: there are different "channels" to allow different types of
  // network traffic to be routed differently (default (which is
  // represented by _networks and _addresses), graphics, sound, and input).
  // The graphics, sound, and input channels have networks/addresses set
  // down in the _parseSpecialPhleetArgs and _parseContext.
  if (_networks == "NULL"){
    _networks = _configParser.getNetworks();
  }
  if (_addresses == "NULL"){
    _addresses = _configParser.getAddresses();
  }
  
  // The special Phleet args are or are not removed in the _parsePhleetArgs
  // call.
  if (!_parsePhleetArgs(argc, argv)){
    _initResponseStream << _exeName << " error: invalid Phleet args.\n";
    // NOTE: it isn't strictly true that we are not connected.
    // However, if this happens, we want the component to quit,
    // which it will only do if _connected is false.
    _connected = false;
    success = false; // do not return yet
  }

  if (!_parseContext()){
    _initResponseStream << _exeName << " error: invalid Phleet context.\n";
    // NOTE: it isn't strictly true that we are not connected.
    // However, if this happens, we want the component to quit,
    // which it will only do if _connected is false.
    _connected = false;
    success = false; // do not return yet
  }

  // This needs to go after phleet args parsing, etc. It could be that we
  // specify where the server is, what the user name is, etc. via command
  // line args. 
  if (!_dialUpFallThrough()) {
    // Don't complain here -- dialUpFallThrough() already did.
    _connected = false;
    success = false; // do not return yet
  }

  // set the name of the component. do
  // this from the command-line args, since some of the component management
  // occurs via names, so this is a vital consistency guarantee (letting
  // the programmer set names manually is a good way for things to get out of
  // sync) 
  _exeName = string(argv[0]);
  _exeName = ar_stripExeName(_exeName);
  if (forcedName == "NULL"){
    // this is the default for the forcedName parameter. We are not trying
    // to force the name.
    _setLabel(_exeName); // tell szgserver our name
  }
  else{
    // we are indeed trying to force the name. Go ahead and print out a 
    // warning, though, if there is a difference.
    if (forcedName != _exeName){
      cout << _exeName << " warning: trying to force a component name\n"
	   << "different from the executable name (undesirable except in Windows 98).\n";
    }
    _setLabel(forcedName); // tell the szgserver our name
  }

  // we need to pack the init stream and the start stream with headers
  _initResponseStream << _generateLaunchInfoHeader();
  _startResponseStream << _generateLaunchInfoHeader();

  // now, it's time to do the handshaking with "dex", if such is required
  if (_dexHandshaking){
    // regardless of whether we are on Unix or Win32, we need to begin
    // responding to the execute message, if it was passed-through dex
    // this lets dex know that the executable did, indeed, launch.
    string tradingKey = getComputerName() + "/"
                        + ar_getenv("SZGTRADINGNUM") + "/"
                        + _exeName;
    _launchingMessageID = requestMessageOwnership(tradingKey);
    // Important that the object fails here if it can't get the message
    // ownership trade. Perhaps we were late starting and the szgd has
    // already decided that we won't actually launch.
    if (!_launchingMessageID){
      cerr << _exeName << " error: failed to get message ownership, "
	   << "despite appearing to have been launched by szgd.\n";
      return false;
    }
    else{
      // send the message response
      // NOTE: if we are doing simple handshaking, we send a complete response
      // if we are not doing simple handshaking, we send a partial response
      if (!messageResponse(_launchingMessageID, 
                           _generateLaunchInfoHeader() +
                           _exeName + string(" launched.\n"),
                           !_simpleHandshaking)){
        cerr << _exeName
	     << " warning: failed to send message response during launch.\n";
      }
    }
  }

  // HACK HACK HACK HACK HACK HACK HACK HACK HACK
  // If we have failed to connect to the szgserver, go ahead and parse
  // a local config file.
  if (!_connected){
    cout << _exeName << " remark: parsing local parameter file.\n";
    parseParameterFile(_parameterFileName);
  }
 
  return success;
}

/// Common core of sendInitResponse() and sendStartResponse().
bool arSZGClient::_sendResponse(stringstream& s, const char* sz,
                                bool state, bool state2) {
  // Append a standard success or failure message to the stream.
  s << _exeName << " component " << sz << (state ? " ok.\n" : " failed.\n");

  if (_dexHandshaking && !_simpleHandshaking){
    // Another message to dex.  state2 is false iff it's the final message.
    if (!messageResponse(_launchingMessageID, s.str(), state2)){
      cerr << _exeName
	   << " warning: failed to send message response during "
	   << sz << ".\n";
      return false;
    }
  }
  else{
    // Nowhere to send the message.
    cout << s.str();
  }
  return true;
}

/// If we have launched via szgd/dex, send the init message stream
/// back to the launching dex command. If we launched from the command line,
/// just print the stream. If the parameter state is "true", the init succeeded
/// and we'll be sending a start message later (so this will be a partial
/// response). If the parameter is "false", then the init failed and we'll
/// not be sending another response, so this should be the final one.
bool arSZGClient::sendInitResponse(bool state){
  return _sendResponse(_initResponseStream, "initialization", state, state);
}

/// If we have launched via szgd/dex, send the start message stream
/// back to the launching dex command. If we launched from the command line,
/// just print the stream. This is the final response to the launch message
/// regardless, though the parameter does alter the message sent or printed.
bool arSZGClient::sendStartResponse(bool state){
  return _sendResponse(_startResponseStream, "start", state, false);
}

void arSZGClient::closeConnection(){
  _dataClient.closeConnection();
  _connected = false;
}

bool arSZGClient::launchDiscoveryThreads(){
  if (!_keepRunning){
    // Threads would immediately terminate, if we tried to start them.
    cerr << _exeName
         << " warning: terminating, so no discovery threads launched.\n";
    return false;
  }
  if (_discoveryThreadsLaunched){
    cerr << _exeName << " warning: ignoring relaunch of discovery threads.\n";
    return true;
  }

  // initialize the server discovery socket
  _incomingAddress.setAddress(NULL,4621);
  _discoverySocket = new arUDPSocket;
  _discoverySocket->ar_create();
  _discoverySocket->setBroadcast(true);



  arThread dummy1(arSZGClientServerResponseThread,this);
  arThread dummy2(arSZGClientTimerThread,this);
  _discoveryThreadsLaunched = true;
  // just a little paranoid in a putting a delay in here
  // hope it promotes overall stability... I'm still a little
  // wary about these auto-launching threads
  ar_usleep(10000);
  return true;
}

// copypasted from getAttribute()
string arSZGClient::getAllAttributes(const string& substring){
  // NOTE: THIS DOES NOT WORK WITH THE LOCAL PARAMETER FILE!!!!
  if (!_connected){
    return string("NULL");
  }

  // We will either request ALL parameters (in dbatch-able form) or
  // we'll request only those parameters that match the given substring.
  string type;
  if (substring == "ALL"){
    type = "ALL";
  }
  else{
    type = "substring";
  }

  arStructuredData* getRequestData 
    = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  // Must use match for thread safety.
  int match = _fillMatchField(getRequestData);
  string result;
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,substring) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,type) || 
      !getRequestData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !_dataClient.sendData(getRequestData)){
    cerr << _exeName << " warning: failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(getRequestData);
  return result;
}

bool arSZGClient::parseAssignmentString(const string& text){
  stringstream parsingStream(text);
  string param1, param2, param3, param4;
  while (true){
    // Will skip whitespace(this is a default)
    parsingStream >> param1;
    if (parsingStream.fail()){
      // This can actually legitimately happen at the end of the assignment
      // block.
      if (parsingStream.eof()){
	return true;
      }
      cout << "arSZGClient error: malformed assignment string.\n";
      return false;
    }
    parsingStream >> param2;
    if (parsingStream.fail()){
      cout << "arSZGClient error: malformed assignment string.\n";
      return false;
    }
    parsingStream >> param3;
    if (parsingStream.fail()){
      cout << "arSZGClient error: malformed assignment string.\n";
      return false;
    }
    parsingStream >> param4;
    if (parsingStream.fail()){
      cout << "arSZGClient error: malformed assignment string.\n";
      return false;
    }
    else{
      setAttribute(param1, param2, param3, param4);
    }
  }
}

/// Sometimes we want to be able to read in parameters from a file, as in
/// dbatch or when starting a program in "standalone" mode (i.e. when it is
/// not connected to the Phleet).
bool arSZGClient::parseParameterFile(const string& fileName){
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
  if (!fileStream.ar_open(fileName, dataPath)){
    cerr << _exeName << " error: failed to open batch file " 
	 << fileName << "\n";
    return false;
  }
  arBuffer<char> buffer(128);
  string tagText = ar_getTagText(&fileStream, &buffer);
  if (tagText == "szg_config"){
    // Try parsing in the new way.
    while (true){
      tagText = ar_getTagText(&fileStream, &buffer);
      if (tagText == "comment"){
	// Go ahead and get the comment text (but discard it).
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
	  cout << _exeName << " error: failed to get all of comment text "
	       << "while parsing phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/comment"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found /comment)\n";
	  fileStream.ar_close();
	  return false;
	}
      }
      else if (tagText == "param"){
        // First comes the name...
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "name"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found name)\n";
	  fileStream.ar_close();
          return false;
	}
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
          cout << _exeName << " error: failed to get all of parameter "
	       << "name's text "
	       << "while parsing phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        stringstream nameText(buffer.data);
        string name;
        nameText >> name;
        if (name == ""){
          cout << _exeName << " error: empty name field found while"
	       << "parsing phleet config file.\n";
	  fileStream.ar_close();
          return false;
	}
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/name"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found /name)\n";
	  fileStream.ar_close();
          return false;
	}
	// Next comes the value...
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "value"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found value)\n";
	  fileStream.ar_close();
          return false;
	}
        if (!ar_getTextUntilEndTag(&fileStream, "value", &buffer)){
          cout << _exeName << " error: failed to get all of parameter "
	       << "value's text "
	       << "while parsing phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
	// Set the attribute.
        setGlobalAttribute(name, buffer.data);
	// NOTE: we actually have already gotten the closing tag.

	// Finally should come the closing tag for the parameter.
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/param"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found /param)\n";
	  fileStream.ar_close();
          return false;
	}
      }
      else if (tagText == "assign"){
        // Go ahead and parse the assignment info.
        if (!ar_getTextBeforeTag(&fileStream, &buffer)){
	  cout << _exeName << " error: failed to get all of assignment text "
	       << "while parsing phleet config file.\n";
	  fileStream.ar_close();
	  return false;
	}
        parseAssignmentString(buffer.data);
        tagText = ar_getTagText(&fileStream, &buffer);
        if (tagText != "/assign"){
          cout << _exeName << " error: found an illegal tag= " << tagText
	       << " when parsing phleet config file.\n"
	       << "(should have found /assign)\n";
	  fileStream.ar_close();
	  return false;
	}
      }
      else if (tagText == "/szg_config"){
	// successful closure
        break;
      }
      else{
        cout << _exeName << " error: found an illegal tag=" << tagText
	     << " in parsing the phleet config file.\n";
        fileStream.ar_close();
        return false;
      }
    }
    fileStream.ar_close();
    return true;
  }
  fileStream.ar_close();

  // Try the traditional way...
  cout << "arSZGClient remark: Did not find opening <szg_config>.\n"
	   << "Trying to parse the traditional dbatch syntax!\n";
  FILE* theFile = ar_fileOpen(fileName, dataPath, "r");
  if (!theFile){
    cerr << _exeName << " error: failed to open batch file \"" 
         << fileName << "\"\n";
    return false;
  }
  // PROBLEMS WITH FIXED-SIZED BUFFERS
  char buf[4096];
  char buf1[4096], buf2[4096], buf3[4096], buf4[4096];
  while (fgets(buf, sizeof(buf)-1, theFile)) {
    // skip comments which begin with (whitespace and) an octathorp;
    // also skip blank lines.
    char* pch = buf + strspn(buf, " \t");
    if (*pch == '#' || *pch == '\n' || *pch == '\r')
      continue;

    if (sscanf(buf, "%s %s %s %s", buf1, buf2, buf3, buf4) != 4) {
      cerr << _exeName << " warning: ignoring incomplete command: "
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
  if (!_connected){
    // In this case, we are using the local parameter file.
    return _getAttributeLocal(computerName, groupName, parameterName,
                              validValues);
  }

  // We are going to the szgserver for information.
  const string query(
    ((computerName == "NULL") ? _computerName : computerName) +
    "/" + groupName + "/" + parameterName);
  arStructuredData* getRequestData 
    = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  string result;
  int match = _fillMatchField(getRequestData);
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,query) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"value") ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !_dataClient.sendData(getRequestData)){
    cerr << _exeName << " warning: failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _changeToValidValue(groupName, parameterName, 
                                 _getAttributeResponse(match), validValues);
  }
  _dataParser->recycle(getRequestData);
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
  const string& s = getAttribute(computerName, groupName, parameterName,
                                 defaults);
  const int x = ar_stringToInt(s);
  if (parameterName.find("port") != string::npos &&
      parameterName != "com_port") {
    // It's a port.
    if (s == "NULL"){
      cerr << _exeName << " warning: undefined port " << groupName << "/"
           << parameterName << ".\n";
    }
    else{
      if (x < 1024 || x > 50000)
	cerr << _exeName << " warning: port " << groupName << "/"
	     << parameterName << " \"" << s << "\" invalid or out of range.\n";
#ifdef AR_USE_WIN_32
      else if (x >= 10000)
	cerr << _exeName << " warning: port " << groupName << "/"
	     << parameterName << " \"" << s
	     << "\" >= 10000: questionable in win32.\n";
#endif
    }
  }
  return x;
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
    cerr << _exeName << " warning: "
         << groupName << "/" << parameterName << " undefined.\n";
#endif
    return false;
  }
  int num = ar_parseFloatString(s,values,numvalues);
  if (num != numvalues) {
    cerr << _exeName << " warning: "
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
    cerr << _exeName << " warning: "
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
    cerr << _exeName << " warning: "
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
    cerr << _exeName << " warning: failed to set "
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
    cerr << _exeName << " warning: failed to set "
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
    cerr << _exeName << " warning: failed to send command.\n";
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
    cerr << _exeName << " warning: failed to set "
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
    cerr << _exeName << " warning: failed to set "
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
    cerr << _exeName << " warning: failed to send command.\n";
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
    cerr << _exeName << " warning: failed to send getProcessList command.\n";
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
  if (!_connected){
    return false;
  }
  // Must get storage for the message.
  arStructuredData* killIDData
    = _dataParser->getStorage(_l.AR_KILL);
  bool state;
  // One of the few places where we DON'T USE MATCH!
  (void)_fillMatchField(killIDData);
  if (!killIDData->dataIn(_l.AR_KILL_ID,&id,AR_INT,1) ||
      !_dataClient.sendData(killIDData)){
    cerr << _exeName << " warning: message send failed\n";
    state = false;
  }
  else{
    state = true;
  }
  // Must recycle the storage.
  _dataParser->recycle(killIDData);
  return state;
}

/// Kill the ID of the first process in the process list
/// running on the given computer with the given label
/// Return false if no process was found.
bool arSZGClient::killProcessID(const string& computer,
                                const string& processLabel){
  if (!_connected){
    return false;
  }
  string realComputer;
  if (computer == "NULL" || computer == "localhost"){
    // use the computer we are on as the default
    realComputer = _computerName;
  }
  else{
    // syzygy no longer does name resolution
    realComputer = computer;;
  }
  
  const int id = getProcessID(realComputer, processLabel);
  if (id < 0){
    return false;
  }
  return killProcessID(id);
}

/// Given the process ID, return the process label
string arSZGClient::getProcessLabel(int processID){
  if (!_connected){
    return string("NULL");
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "label");
  data->dataIn(_l.AR_PROCESS_INFO_ID, &processID, AR_INT, 1);
  if (!_dataClient.sendData(data)){
    cerr << _exeName << " warning: failed to send process label request.\n";
    return string("NULL");
  }
  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    cerr << _exeName << " warning: failed to get process ID response.\n";
    return string("NULL");
  }
  string theLabel = data->getDataString(_l.AR_PROCESS_INFO_LABEL);
  // note that this is a different data record than above, needs to be recycled
  _dataParser->recycle(data);
  return theLabel;
}

/// Return the ID of the first process in the process list
/// running on the given computer with the given label.
int arSZGClient::getProcessID(const string& computer,
                              const string& processLabel){
  
  if (!_connected){
    return -1;
  }
  string realComputer;
  if (computer == "NULL" || computer == "localhost"){
    realComputer = _computerName;
  }
  else{
    // syzygy no longer does name resolution
    realComputer = computer;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "ID");
  // zero-out the storage
  int dummy = 0;
  data->dataIn(_l.AR_PROCESS_INFO_ID, &dummy, AR_INT, 1);
  data->dataInString(_l.AR_PROCESS_INFO_LABEL, realComputer+"/"+processLabel);
  if (!_dataClient.sendData(data)){
    cerr << _exeName << " warning: failed to send process ID request.\n";
    return -1;
  }
  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    cerr << _exeName << " warning: failed to get process ID response.\n";
    return -1;
  }
  int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // note that this is a different data record than above, needs to be recycled
  _dataParser->recycle(data);
  return theID;
}

// Return the ID of the process which is "me".
int arSZGClient::getProcessID(void){
  if (!_connected)
    return -1;

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "self");
  if (!_dataClient.sendData(data)){
    cerr << _exeName << " warning: getProcessID() send failed\n";
    _dataParser->recycle(data);
    return -1;
  }
  _dataParser->recycle(data);

  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data){
    cerr << _exeName << " warning: ignoring illegal response packet\n";
    return -1;
  }
  int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // note that this is a new piece of data, so we need to recycle it again
  _dataParser->recycle(data);
  return theID;
}

// DOES THIS REALLY BELONG AS A METHOD OF arSZGClient?
// MAYBE THIS IS A HIGHER_LEVEL CONSTRUCT.
bool arSZGClient::sendReload(const string& computer,
                             const string& processLabel) {
  if (!_connected){
    return false;
  }
  if (processLabel == "NULL"){
    // Vacuously ok.
    return true;
  }
  const int pid = getProcessID(computer, processLabel);
  const int ok = pid != -1 && (sendMessage("reload", "NULL", pid) >= 0);
  if (!ok)
    cerr << _exeName << " warning: failed to reload on host \""
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
  arStructuredData* messageData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE);
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
    cerr << _exeName << " warning: message send failed.\n";
    match = -1;
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_MESSAGE_ACK);
    if (!ack){
      cerr << _exeName << " warning: got no message ack.\n";
      match = -1;
    }
    else{
      if (ack->getDataString(_l.AR_SZG_MESSAGE_ACK_STATUS) 
            != string("SZG_SUCCESS")){
        cerr << _exeName << " warning: message send failed.\n";
      }
      _dataParser->recycle(ack);
    }
  }
  // Must recycle the storage.
  _dataParser->recycle(messageData);
  return match;
}

/// Receive a message routed through the szgserver.
/// Returns the ID of the received message, or 0 on error.
/// @param userName Set to the username associated with the message
/// @param messageType Set to the type of the message
/// @param messageBody Set to the body of the message
/// @param context Set to the context of the message (the virtual computer)
int arSZGClient::receiveMessage(string* userName, string* messageType,
                                string* messageBody, string* context){
  if (!_connected){
    return 0;
  }
  // This is the one place we get a record via its type as opposed to its
  // match.
  arStructuredData* data = _getDataByID(_l.AR_SZG_MESSAGE);
  if (!data){
    cerr << _exeName << " warning: failed to receive message.\n";
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
    cerr << _exeName << " warning: no message response.\n";
    return 0;
  }
  if (ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_TYPE) != "SZG Response"){
    cerr << _exeName << " warning: no message response.\n";
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
  const string& statusField = partialResponse ? string("SZG_CONTINUE") : 
                                                string("SZG_SUCCESS");
  // Must get storage for the message.
  arStructuredData* messageAdminData
    = _dataParser->getStorage(_l.AR_SZG_MESSAGE_ADMIN);
  bool state;
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
    cerr << _exeName << " warning: failed to send message response.\n";
    state =  false;
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
  bool state;
  int match = _fillMatchField(messageAdminData);
  if (!messageAdminData->dataIn(_l.AR_SZG_MESSAGE_ADMIN_ID, &messageID,
				AR_INT, 1) ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Trade Message") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY, key) ||
      !_dataClient.sendData(messageAdminData)){
    cerr << _exeName << "warning: failed to send message ownership trade.\n";
    match = -1;
  }
  else{
    state = _getMessageAck(match, "message trade");
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
  bool state;
  int match = _fillMatchField(messageAdminData);
  if (!messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_TYPE,
				      "SZG Revoke Trade") ||
      !messageAdminData->dataInString(_l.AR_SZG_MESSAGE_ADMIN_BODY, key) ||
      !_dataClient.sendData(messageAdminData)){
    cerr << _exeName << "warning: failed to send message trade revocation.\n";
    state = false;
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
    cerr << _exeName
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
    cerr << _exeName << " warning: failed to send kill "
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
    cerr << _exeName << " remark: failed to get kill "
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
  bool state;
  int match = _fillMatchField(lockRequestData);
  if (!lockRequestData->dataInString(_l.AR_SZG_LOCK_REQUEST_NAME, 
                                     lockName) ||
      !_dataClient.sendData(lockRequestData)){
    cerr << _exeName << " warning: failed to send lock request.\n";
    state = false;
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack){
      cerr << _exeName << " warning: no ack for lock.\n";
      state = false;
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
  bool state;
  int match = _fillMatchField(lockReleaseData);
  if (!lockReleaseData->dataInString(_l.AR_SZG_LOCK_RELEASE_NAME, 
                                     lockName) ||
      !_dataClient.sendData(lockReleaseData)){
    cerr << _exeName << " warning: failed to send lock request.\n";
    state = false;
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack){
      cerr << _exeName << " warning: no ack for lock.\n";
      state = false;
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
    cerr << _exeName << " warning: failed to send lock release "
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
    cerr << _exeName << " error: failed to get lock release "
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
    cerr << _exeName << " warning: failed to send print locks request.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_LISTING);
  if (!ack){
    cerr << _exeName << " warning: did not receive response for lock listing "
	 << "request.\n";
    return;
  }
  const string locks(data->getDataString(_l.AR_SZG_LOCK_LISTING_LOCKS));
  int* IDs = (int*)data->getDataPtr(_l.AR_SZG_LOCK_LISTING_COMPONENTS, AR_INT);
  int number = data->getDataDimension(_l.AR_SZG_LOCK_LISTING_COMPONENTS);
  // print out the stuff
  int where = 0;
  for (int i=0; i<number; i++){
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
    cout << _exeName << " error: "
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
    cerr << _exeName << " warning: failed to send service registration "
	 << (fRetry ? "retry" : "") << " request.\n";
    _dataParser->recycle(data);
    return false;
  }
  _dataParser->recycle(data);

  // wait for the response
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data){
    cerr << _exeName << " warning: no response for service registration "
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
    cerr << _exeName << " error: confirmPorts(...) passed invalid "
	 << "channel.\n";
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
    cerr << _exeName << " warning: failed to send service registration "
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
    cerr << _exeName << " error: failed to get broker result in response "
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
    cerr << _exeName << " warning: failed to send discover service "
	 << "request.\n";
    result.valid = false;
    _dataParser->recycle(data);
    return result;
  }
  _dataParser->recycle(data);

  // receive the response
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data){
    cerr << _exeName << " error: failed to get broker result in response "
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
    cerr << _exeName << " warning: failed to send service listing request.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data){
    cerr << _exeName << " error: failed to get response to get services "
	 << "request.\n";
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
    cerr << _exeName
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
    cerr << _exeName << " error: failed to get service release "
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
    cerr << _exeName
         << " warning: failed to request service info.\n";
    return string("");
  }
  // Now, get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data){
    cerr << _exeName << " error: failed to get response to get service "
	 << "info.\n";
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
  const bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok){
    cerr << _exeName
         << " warning: failed to set service info.\n";
    return false;
  }
  // Now, get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data){
    cerr << _exeName << " error: failed to get response to set service "
	 << "info.\n";
    return false;
  }
  string result = data->getDataString(_l.AR_SZG_SERVICE_INFO_STATUS);
  _dataParser->recycle(data);
  return result == "SZG_SUCCESS" ? true : false;
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
    cerr << _exeName << " warning: failed to send service ID request.\n";
    _dataParser->recycle(data);
    return -1;
  }
  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data){
    cerr << _exeName << " error: failed to get response to service "
	 << "ID request.\n";
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
  cerr << _exeName << " warning: unknown channel for getNetworks().\n";
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
  cerr << _exeName << " warning: unknown channel for getAddresses().\n";
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
  cout << _exeName << " warning: unknown channel for getMode().\n";
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
    string("mode/default=")+_mode);

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
    cerr << _exeName << " warning: already connected to "
         << _IPaddress << ":" << _port << endl;
    return false;
  }

  // DO NOT LAUNCH THE DISCOVERY THREADS HERE. WE DO NOT NEED THESE FOR
  // THE STANDARD CLIENT.

  // In standalone mode, we want to be able to run, even if the computer
  // has not yet been configured. The szg.conf file only contains
  // information like the computer name and the networks to which it is 
  // attached, which doesn't really matter if we are in standalone mode.
  // In the standalone mode case, the computer name is set to NULL and,
  // consequently, a fixed parameters file can have its info read in using 
  // computer string NULL. 
  // It could be that _IPaddress, _port, and _userName have all been defined
  // from the command line... In this case, go ahead and over-ride the
  // login/ other config information. 

  // We need the following pieces of information:
  //   1. server IP/ server port
  //   2. szg user name
  // We optionally want:
  //   1. server name
  //   2. computer name
  // If the first 3 have been set already (i.e. via command line args)
  // then go ahead and don't bother to parse the config files.
  if (_IPaddress == "NULL" || _port == -1 || _userName == "NULL"){ 
    // determine the other configuration information, like the location of the
    // szgserver and the name of the host computer
    if (!_configParser.parseConfigFile()){
      cout << "arSZGClient error: failed to open config file.\n";
      cout << "  For non-standalone operation, you must run dname, etc.\n";
      return false;
    }
    // If we are not logged-in (i.e. standalone mode) the next conditional
    // will FAIL! However, some information from the config file is needed
    // for operation in standalone mode (like _computerName).
    // Consequently, set that before returning.
    _computerName = _configParser.getComputerName(); 

    // parse the login file
    if (!_configParser.parseLoginFile()){
      cout << "arSZGClient error: no login file. Please dlogin.\n";
      return false;
    }
    // currently, there is a little bug! if we connect to the szgserver
    // via an explicit IP address and port, the server name is not set.
    // consequently, we cannot use the configParser's getServerName() to
    // determine if login was valid!
    if (_configParser.getServerIP() == "NULL"){
      cout << "arSZGClient error: user not connected to szgserver."
	   << "Please dlogin.\n";
      return false;
    }
    // Find our syzygy user name.
    string userNameOverride = ar_getenv("SZGUSER");
    // NOTE: ar_getenv returns "NULL" if the environment var is not set
    if (userNameOverride != "NULL"){
      // the environment variable is set and that over-rides what
      // is in the log-in file. this allows programs like szgd to work.
      _userName = userNameOverride;
    }
    else{
      _userName = _configParser.getUserName();
    }

    // set all the other information
    _serverName   = _configParser.getServerName();
    _IPaddress    = _configParser.getServerIP();
    _port         = _configParser.getServerPort();
  }

  if (!_dataClient.dialUpFallThrough(_IPaddress.c_str(),_port)){
    // Look on the LAN for an szgserver.
    /// \todo say "dlogin first" and leave it at that, 
    /// (if dconfig would report that nobody's logged in)
    cout << _exeName
         << " remark: szgserver not found. You must dlogin. Try running "
	 << "dhunt to find running szgservers.\n";
    return false;
  }

  _connected = true;
  if (!_clientDataThread.beginThread(arSZGClientDataThread, this)) {
    cerr << _exeName << " error: failed to start client data thread.\n";
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
  arStructuredData* message;
  if (_dataParser->getNextTaggedMessage(message, tags, recordID, timeout)
      < 0){
    // Either timeout or other error has occured.
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
    cerr << _exeName << " warning: no ack for " << transaction << ".\n";
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
    cerr << _exeName << " warning: unexpected response from szgserver.\n";
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
  bool state;
  const string fullLabel = _computerName + "/" + label;
  int match = _fillMatchField(connectionAckData);
  if (!connectionAckData->dataInString(_l.AR_CONNECTION_ACK_LABEL,
                                       fullLabel) ||
      !_dataClient.sendData(connectionAckData)){
    cerr << _exeName << " warning: failed to set label.\n";
    state = false;
  }
  else{
    _exeName = label;
    _dataClient.setLabel(_exeName);
    arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
    if (!ack){
      cerr << _exeName << " warning: failed to retrieve szgserver name.\n";
      state = false;
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
    // nothing to do. NOTE: this IS NOT ar error.
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
        cout << _exeName << " error: -szg flag is last in arg list. Abort.\n";
	return false;
      }
      // we need to figure out which args these are.
      // THIS IS CUT_AND_PASTED FROM _parseContextPair below
      const string thePair(argv[i+1]);
      unsigned int location = thePair.find('=');
      if (location == string::npos){
        cout << "arSZGClient error: context pair does not contain =.\n";
        return false;
      }
      unsigned int length = thePair.length();
      if (location == length-1){
        cout << "arSZGClient error: context pair ends with =.\n";
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
    cout << _exeName << " error: context pair does not contain =.\n";
    return false;
  }
  unsigned int length = thePair.length();
  if (location == length-1){
    cout << _exeName << " error: context pair ends with =.\n";
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
      cout << _exeName << " error: in _parseContextPair(...) received mode "
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
      cout << _exeName << " error: parseContextPair() received invalid "
	   << "mode channel.\n";
      return false;
    }
  }
  else if (pair1Type == "networks"){
    // Return true iff the networks value is valid,
    // and set _networks and _addresses appropriately.
    if (pair1.size() != 2){
      cerr << _exeName << " error: "
           << "no channel in _parseContextPair()'s networks data.\n";
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
      cerr << "arSZGClient error: "
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
  else{
    cout << _exeName << " error: context pair has unknown type \""
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
    cout << _exeName << " error: _checkAndSetNetworks() got unknown channel \""
         << channel << "\".\n";
    return false;
  }
  const int numberNewNetworks = networks.size();
  const int numberCurrentNetworks = _networks.size();
  // Check to see if every one of the new networks is, in fact, one of the
  // current networks. If not, we have an error.
  int i, j;
  for (i=0; i<numberNewNetworks; i++){
    bool match = false;
    for (j=0; j<numberCurrentNetworks; j++){
      if (networks[i] == _networks[j]){
	match = true;
	break;
      }
    }
    if (!match){
      cout << _exeName << " error: invalid network specified.\n";
      return false;
    }
  }
  // OK... so each one of the new networks matched. We'll also want to use
  // addresses, so get those. Not the most efficient code ever... BUT
  // how many NICs are there likely to be in a single computer?
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
      cerr << _exeName
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
	cerr << _exeName << " warning: "
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
  char buffer[512];
  while (_keepRunning) {

    while (_discoverySocket->ar_read(buffer,299, &_incomingAddress) < 0) {
      ar_usleep(10000);
      // Win32 returns -1 if no packet was received
      // (and therefore the data below would be garbage).
    }

    // We got a packet.
    ar_usleep(10000); // avoid busy-waiting on Win32
    ar_mutex_lock(&_queueLock);
    if (_dataRequested){
      memcpy(_responseBuffer,buffer,512);
      if (_justPrinting){
	// Print out the contents of this packet.
        const string promiscuity(_responseBuffer[128] ? "true": "false");
        cout << _responseBuffer  << "/"
	     << _responseBuffer+129 << ":"
	     << _responseBuffer+161 << "\n";
      }
      else {
        // Discard subsequent packets.
        _dataRequested = false;
        _dataCondVar.signal();
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
      cout << "arSZGClient remark: the server has disconnected "
	   << "(read error).\n";
      _dataParser->clearQueues();
      cout << "arSZGClient remark: message queues cleared.\n";
      _connected = false;
      // No need to get any more data!
      break;
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
      cout << "arSZGClient remark: the server has forcibly disconnected.\n";
      _dataParser->clearQueues();
      cout << "arSZGClient remark: message queues cleared.\n";
      // HMMMM.... should the below go inside a LOCK?
      _connected = false;
      // No need to get any more data!
      break;
    }
    else{
      arStructuredData* data = _dataParser->parse(_receiveBuffer, size);
      _dataParser->pushIntoInternalTagged(data, 
                                        data->getDataInt(_l.AR_PHLEET_MATCH));
    }
  }
}

// magic numbers 128 129 161 171 4620 are duplicates from phleet/szgserver.cpp
// NOTE: this method expects the subnet parameter to have a trailing '.'
// (i.e. 192.168.0.)
void arSZGClient::_sendDiscoveryPacket(const string& name,
                                       const string& affinity,
				       const string& subnet){
  char buffer[256];
  ar_stringToBuffer(name, buffer, sizeof(buffer));
  ar_stringToBuffer(affinity, buffer+128, sizeof(buffer)-128);
  arSocketAddress broadcastAddress;
  const string IP(subnet + "255");
  broadcastAddress.setAddress(IP.c_str(),4620);
  _discoverySocket->ar_write(buffer, 160, &broadcastAddress);
}

/// Sends a broadcast packet on a specified subnet to find the szgserver
/// with the specified name. 
bool arSZGClient::discoverSZGServer(const string& name,
                                    const string& affinity,
				    const string& subnet){
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()){
    cerr << _exeName << " error: failed to launch discovery threads.\n";
    return false;
  }

  ar_mutex_lock(&_queueLock);
  _dataRequested = true;
  _beginTimer = true;
  _sendDiscoveryPacket(name,affinity,subnet);
  _timerCondVar.signal();
  while (_dataRequested && _beginTimer){
    _dataCondVar.wait(&_queueLock);
  }
  if (_dataRequested){
    // timeout
    ar_mutex_unlock(&_queueLock);
    return false;
  }

  // We actually got something
  const string theServerIP  (_responseBuffer+129);
  const string theServerPort(_responseBuffer+161);
  // note that we receive the server name back (instead of the client name
  // as before)
  const string theServerName(_responseBuffer+171);

  // to internal storage
  _IPaddress = theServerIP;
  _port = atoi(theServerPort.c_str());
  _serverName = theServerName;

  ar_mutex_unlock(&_queueLock);
  return true;
}

/// Tries to find any szgservers out there and print their names. Useful
/// for seeing who's running what.
void arSZGClient::printSZGServers(const string& subnet){
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()){
    cerr << _exeName << " error: no discovery threads.\n";
    return;
  }
  _justPrinting = true;

  ar_mutex_lock(&_queueLock);
  _dataRequested = true;
  _beginTimer = true;
  _sendDiscoveryPacket("**","*",subnet);
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
  return (_configParser.writeLoginFile());
}

/// Changes the login file to have the not-logged-in values
bool arSZGClient::logout(){
  _configParser.setUserName("NULL");
  _configParser.setServerName("NULL");
  _configParser.setServerIP("NULL");
  _configParser.setServerPort(0);
  return (_configParser.writeLoginFile());
}

/// Default message task which handles "quit" and nothing else.
void ar_messageTask(void* pClient){
  if (!pClient) {
    cerr << "ar_messageTask warning: no syzygy client, dkill disabled.\n";
    return;
  }

  arSZGClient* cli = (arSZGClient*)pClient;
  while (true){
    string messageType, messageBody;
    cli->receiveMessage(&messageType,&messageBody);
    if (messageType == "quit"){
      cli->closeConnection();
      // Do this?  But it's private.  cli->_keepRunning = false;
      exit(0);
    }
  }
}
