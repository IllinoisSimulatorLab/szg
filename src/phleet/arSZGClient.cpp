//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arXMLUtilities.h"
#include "arXMLParser.h"
#include "arLogStream.h"

void arSZGClientServerResponseThread(void* client) {
  ((arSZGClient*)client)->_serverResponseThread();
}

void arSZGClientTimerThread(void* client) {
  ((arSZGClient*)client)->_timerThread();
}

void arSZGClientDataThread(void* client) {
  ((arSZGClient*)client)->_dataThread();
}

arSZGClient::arSZGClient():
  _IPaddress("NULL"),
  _port(-1),
  _serverName("NULL"),
  _computerName("NULL"),
  _userName("NULL"),
  _exeName("Syzygy Client"),
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
  _parameterFileName("USE_DEFAULT"),
  _virtualComputer("NULL"),
  _connected(false),
  _receiveBuffer(new ARchar[15000]),
  _receiveBufferSize(15000),
  _launchingMessageID(0),
  _dexHandshaking(false),
  _simpleHandshaking(true),
  _ignoreMessageResponses(false),
  _parseSpecialPhleetArgs(true),
  _initialInitLength(0),
  _initialStartLength(0),
  _match(-1),
  _logLevel(AR_LOG_WARNING),
  _discoveryThreadsLaunched(false),
  _beginTimer(false),
  _requestedName(""),
  _dataRequested(false),
  _keepRunning(true),
  _bufferResponse(false),
  _justPrinting(false)
{
  _dataClient.setLabel(_exeName);
  _dataClient.smallPacketOptimize(true);
  _dataClient.setBufferSize(_receiveBufferSize);
}

arSZGClient::~arSZGClient() {
  _keepRunning = false;

  // todo: add handshaking to tell the server to quit the read and
  // close the socket at its end, so we can close without blocking on
  // our data thread's blocking read (e.g. IRIX).
  //_dataClient.closeConnection();

  delete [] _receiveBuffer;
}

// Control how to handshake with dex.
// By default, "true", send a final response during init().
// Iff simple handshaking has been disabled (by passing "false" before init),
// call sendInitResponse and sendStartResponse (the final response in this case),
// to forward init and start diagnostics to the the spawning dex for printing.
void arSZGClient::simpleHandshaking(bool fSimple) {
  if (!fSimple) {
    ar_log().setStream(initResponse());
  }
  _simpleHandshaking = fSimple;
}

// Most components should not see the
// "special" Syzygy args (like "-szg virtual=vccube"). These
// shape how Syzygy components behave (like acting as part of
// a particular virtual computer or in a particular mode).
// But components like dex must see them to pass them on: use "false" then.
void arSZGClient::parseSpecialPhleetArgs(bool state) {
  _parseSpecialPhleetArgs = state;
}

// Connect client to the cluster. Call early in main().
// @param argc main()'s argc
// @param argv main()'s argv
// @param forcedName Optional. Ideally, we'd read the exe name from argv[0],
// but Win98 gives at best a name in all caps.  (Warn if those two mismatch.)

bool arSZGClient::init(int& argc, char** const argv, string forcedName) {
  // Set the component's name from argv[0],
  // since some component management uses names.
  _exeName = ar_stripExeName(string(argv[0]));
  ar_setLogLabel( _exeName );
  
  // On Unix, we might need to finish a handshake with szgd,
  // telling it we've been successfully forked.
  // Before dialUpFallThrough.

  // Whether or not init() succeeds, finish the handshake with dex.

  const string pipeIDString = ar_getenv("SZGPIPEID");
  if (pipeIDString != "NULL") {
    _dexHandshaking = true;

    // We have been spawned on the Unix side.
    const int pipeID = atoi(pipeIDString.c_str());

    // Send the success code.  On Win32 set the pipe ID to -1,
    // which gets us into here (to set _dexHandshaking to true...
    // but don't write to the pipe on Win32 since the function is unimplemented
    if (pipeID >= 0) {
      char numberBuffer[8] = "\001";
      if (!ar_safePipeWrite(pipeID, numberBuffer, 1)) {
        ar_log_error() << "unterminated pipe-based handshake.\n";
      }
    }
  }

  // The parameter file lists networks through which
  // we can communicate. various programs will wish to obtain a list
  // of networks to communicate in a uniform way. The list may need to
  // be altered from the default (all of them) to shape network traffic
  // patterns (for instance, graphics info should go exclusively through
  // a fast private network). The "context", as stored in SZGCONTEXT
  // can be used to do this, as can command-line args. Other properties,
  // like virtual computer and mode, can also be manipulated through these
  // means. SZGCONTEXT overwrites the command-line args.
  //
  // argc is passed by reference (and modified) because the special args that
  // manipulate these values are stripped after use.
  
  // Parse the config file BEFORE parsing the Syzygy args (and the "context"),
  // to set the _networks and _addresses.
  // If these files cannot be read, it is still possible to recover.
  if (!_config.read())
    ar_log_debug() << "no explicit config file.\n";
  if (!_config.readLogin(true))
    ar_log_debug() << "not explicitly dlogin'd.\n";

  // If the networks and addresses were NOT set via command line args or
  // environment variables, set them.
  // Different "channels" let different types of
  // network traffic be routed differently (default (which is
  // represented by _networks and _addresses), graphics, sound, and input).
  // The graphics, sound, and input channels have networks/addresses set
  // down in the _parseSpecialPhleetArgs and _parseContext.

  // From the computer-wide config file (if such existed) and the user's login file.
  _networks     = _config.getNetworks();       // can override
  _addresses    = _config.getAddresses();      // can override
  _computerName = _config.getComputerName();   // *cannot* override

  // From the per-user login file, if such existed.
  _serverName   = _config.getServerName();     // *cannot* override
  _IPaddress    = _config.getServerIP();       // can override
  _port         = _config.getServerPort();     // can override
  _userName     = _config.getUserName();       // can override

  // Handle and remove any special Syzygy args.
  // These can override some _members above.
  if (!_parsePhleetArgs(argc, argv)) {
    _initResponseStream << _exeName << " error: invalid -szg args.\n";
    // Force the component to quit, even if connected.
    _connected = false;
  }

  // "Context" can override some _members above.
  if (!_parseContext()) {
    _initResponseStream << _exeName << " warning: invalid Syzygy context.\n";
    // _checkAndSetNetworks() may have failed for one network but not all networks.
    // Force the component to quit, even if connected.  (Really?)
    _connected = false;
  }

  // $SZGUSER (used by szgd) overrides all other _userName's.
  const string userNameOverride = ar_getenv("SZGUSER");
  if (userNameOverride != "NULL") {
    _userName = userNameOverride;
  }

  if ( _IPaddress == "NULL" || _port == -1 || _userName == "NULL" ) {
    // Not enough info to find an szgserver.  Fallback to standalone.
    // No dex, so redirect pending and future output to cout.
    cout << _initResponseStream.str();
    ar_log().setStream(cout);
    ar_log_critical() << "running standalone.\n";
    _connected = false;

    const bool gotParams = 
      (_parameterFileName == "USE_DEFAULT" &&
        (parseParameterFile("szg_parameters.xml", false) ||
         parseParameterFile("szg_parameters.txt", false))) ||
       parseParameterFile(_parameterFileName, false);
    if (!gotParams) {
      const string s = ar_getenv("SZG_PARAM");
      if (s != "NULL") {
        parseParameterFile(s, false);
      }
    }
    return true;
  }

  // szgserver, username, etc ("context") are all set.  Connect to that szgserver.
  if (!_dialUpFallThrough()) {
    _connected = false;
    return false;
  }
  ar_setLogLabel( _exeName + " " + ar_intToString(getProcessID()));
  ar_log_debug() << "connected to szgserver.\n";

  _connected = true;
  const string configfile_serverName(_serverName);
  
  // Tell szgserver our name.
  if (forcedName == "NULL") {
    _setLabel(_exeName); 
  }
  else{
    if (forcedName != _exeName) {
      ar_log_error() << "component name overriding exe name, Win98-style.\n";
    }
    // This also updates _exeName.
    _setLabel(forcedName); 
  }

  if (configfile_serverName != "NULL" && configfile_serverName != _serverName) {
    ar_log_critical() << "expected szgserver named '" << configfile_serverName <<
      "', not '" << _serverName << "', at " << _IPaddress << ":" << _port << ".\n";
  }

  _initResponseStream << _generateLaunchInfoHeader();
  _initialInitLength = _initResponseStream.str().length();

  _startResponseStream << _generateLaunchInfoHeader();
  _initialStartLength = _startResponseStream.str().length();

  if (_dexHandshaking) {
    // Shake hands with dex.
    // Tell dex that the exe is starting to launch.
    const string tradingKey = getComputerName() + "/" +
      ar_getenv("SZGTRADINGNUM") + "/" + _exeName;

    // Control the right to respond to the launching message.
    _launchingMessageID = requestMessageOwnership(tradingKey);

    // init should not fail here if it can't trade message ownership,
    // e.g. if szgd timed out before the exe finished launching.
    // But don't try to send responses back then.
    if (!_launchingMessageID) {
      ar_log_error() << "failed to own message, despite apparent launch by szgd.\n";
      _ignoreMessageResponses = true;
    }
    else{
      // Send the message response.
      // If simple handshaking, send a complete response.
      // Otherwise (for apps that want to send a start response),
      // send a partial response (i.e. there will be more responses).
      if (!messageResponse(_launchingMessageID,
			   _generateLaunchInfoHeader() +
			     "dex launched pid " + ar_intToString(getProcessID()) +
			     ", " + _exeName + ".\n",
			   !_simpleHandshaking)) {
	ar_log_error() << "response failed during launch.\n";
      }
    }
  }

  return true;
}

int arSZGClient::failStandalone(bool fInited) const {
  if (!*this) {
    ar_log_error() << (fInited ?
      "standalone unsupported. First dlogin.\n" :
      "failed to init arSZGClient.\n");
  }
  return 1; // for convenience
}

// Common core of sendInitResponse() and sendStartResponse().
bool arSZGClient::_sendResponse(stringstream& s, 
				const char* sz,
				unsigned initialStreamLength,
                                bool ok, 
				bool fNotFinalMessage) {
  // Does new stuff follow the header?
  const bool printInfo = s.str().length() > initialStreamLength;

  // Append a standard success or failure message.
  if (ok)
    ar_log_remark() << sz << "ed.\n";
  else
    ar_log_error()  << sz << " failed.\n";

  // Do not send the response if:
  //  - The message trade failed in init(), likely because the launcher timed out;
  //  - we were not launched by szgd; and
  //  - we ("our component") uses simple handshaking.

  if (!_ignoreMessageResponses && _dexHandshaking && !_simpleHandshaking) {
    // Return s to dex.
    if (!messageResponse(_launchingMessageID, s.str(), fNotFinalMessage)) {
      cout << _exeName << ": response failed during " << sz << ".\n";
      return false;
    }
  } else {
    // Print s.
    if (printInfo)
      cout << s.str();
  }
  return true;
}

// If dex launched us, send the init message stream
// back to the launching dex command. If we launched from the command line,
// just print the stream. If "ok" is true, the init succeeded
// and we'll be sending a start message later (so this will be a partial
// response). Otherwise, the init failed and we'll
// not be sending another response, so this should be the final one.
//
// Typically main() calls this.  Every return from main should call this.
bool arSZGClient::sendInitResponse(bool ok) {
  const bool f = _sendResponse(_initResponseStream, "init", _initialInitLength, ok, ok);
  // ?: syntax fails in Visual Studio 7.  Do it the long way.
  if (f) {
    ar_log().setStream(_startResponseStream);
  }
  else {
    ar_log().setStream(cout);
  }
  return f;
}

// If launched via szgd/dex, return the start message stream to dex.
// If launched from the command line, just print it.
// In either case this is the final response to the launch message,
// though the parameter does alter the message sent or printed.
bool arSZGClient::sendStartResponse(bool ok) {
  const bool f = _sendResponse(_startResponseStream, "start", _initialStartLength, ok, false);
  ar_log().setStream(cout);
  return f;
}

void arSZGClient::closeConnection() {
  _dataClient.closeConnection();
  _connected = false;
}

bool arSZGClient::launchDiscoveryThreads() {
  if (!_keepRunning) {
    // Threads would immediately terminate, if we tried to start them.
    ar_log_remark() << "terminating, so no discovery threads launched.\n";
    return false;
  }
  if (_discoveryThreadsLaunched) {
    ar_log_error() << "ignoring relaunch of discovery threads.\n";
    return true;
  }

  // Initialize the server discovery socket
  _discoverySocket = new arUDPSocket;
  _discoverySocket->ar_create();
  _discoverySocket->setBroadcast(true);
  _discoverySocket->reuseAddress(true);
  arSocketAddress incomingAddress;
  incomingAddress.setAddress(NULL,4620);
  if (_discoverySocket->ar_bind(&incomingAddress) < 0) {
    ar_log_error() << "failed to bind discovery response socket.\n";
    return false;
  }

  arThread dummy1(arSZGClientServerResponseThread,this);
  arThread dummy2(arSZGClientTimerThread,this);
  _discoveryThreadsLaunched = true;
  return true;
}

// copypasted from getAttribute()
string arSZGClient::getAllAttributes(const string& substring) {
  // NOTE: THIS DOES NOT WORK WITH THE LOCAL PARAMETER FILE!!!!
  if (!_connected) {
    return string("NULL");
  }

  // Request ALL parameters (in dbatch-able form),
  // or only those that match the given substring.
  const string type = substring=="ALL" ? "ALL" : "substring";

  arStructuredData* getRequestData =
    _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  const int match = _fillMatchField(getRequestData); // for thread safety
  string result;
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,substring) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,type) ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !_dataClient.sendData(getRequestData)) {
    ar_log_error() << "failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(getRequestData);
  return result;
}

// Called only when parsing assign blocks in dbatch files.
bool arSZGClient::parseAssignmentString(const string& text) {
  stringstream parsingStream(text);
  string computer, group, name, value;
  unsigned count = 0;
  while (true) {
    // Will skip whitespace(this is a default)
    parsingStream >> computer;
    if (parsingStream.fail()) {
      if (!parsingStream.eof())
	goto LFail;
      // End of assignment block.
      if (count > 0) {
	ar_log_remark() << "in assign block, replaced " << count <<
	  "x 'USER_NAME' with '" << _userName << "'.\n";
      }
      return true;
    }
    parsingStream >> group;
    if (parsingStream.fail())
      goto LFail;
    parsingStream >> name;
    if (parsingStream.fail())
      goto LFail;
    parsingStream >> value;
    if (parsingStream.fail()) {
LFail:
      ar_log_error() << "malformed assignment string '" << text << "'.\n";
      return false;
    }

    if (_userName != "NULL") {
      string::size_type index;
      while ((index = value.find("USER_NAME")) != string::npos) {
        value.replace( index, 9, _userName );
        ++count;
      }
    }
    if (group.substr(0,10) == "SZG_SCREEN") {
      ar_log_error() << "deprecated SZG_SCREEN parameters.\n";
    }
    setAttribute(computer, group, name, value);
  }
}

inline bool arSZGClient::_parseTag(arFileTextStream& fs,
    arBuffer<char>* buf, const char* tagExpect) {
  const string tag = ar_getTagText(&fs, buf);
  if (tag == tagExpect)
    return true;

  ar_log_error() << "parameter file expected '" << tagExpect << "', not '" << tag << "'.\n";
  fs.ar_close();
  return false;
}

inline bool arSZGClient::_parseBetweenTags(arFileTextStream& fs,
  arBuffer<char>* buf, const char* warning) {
  if (ar_getTextBeforeTag(&fs, buf))
    return true;

  ar_log_error() << "incomplete " << warning << " in parameter file.\n";
  fs.ar_close();
  return false;
}

// Read parameters from a file, as in
// dbatch or when starting a program standalone.
bool arSZGClient::parseParameterFile(const string& fileName, bool warn) {
  const string dataPath(getAttribute("SZG_SCRIPT","path"));
  // There are two parameter file formats.
  // (1) The legacy format is a sequence of lines of the form:
  //
  //   computer parameter_group parameter parameter_value
  //
  // (2) The XML format supports "global" attributes,
  // such as an input node description:
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
  // Assume XML format, if <szg_config> is the first non-whitespace text.

  string fileNameVerbose = "parameter file '" + fileName + "'";
  if (dataPath != "NULL")
    fileNameVerbose += " on SZG_SCRIPT/path '" + dataPath + "'";

  arFileTextStream fileStream;
  if (!fileStream.ar_open(fileName, dataPath)) {
    if (warn) {
      ar_log_error() << " failed to open " << fileNameVerbose << ".\n";
    }
    return false;
  }
  ar_log_debug() << " parsing config file " << ar_fileFind(fileName, "", dataPath) << ".\n";
  arBuffer<char> buffer(128);
  string tagText = ar_getTagText(&fileStream, &buffer);
  if (tagText == "szg_config") {
    // Parse as XML.
    while (true) {
      tagText = ar_getTagText(&fileStream, &buffer);

      if (tagText == "comment") {
        // Discard comment.
        if (!_parseBetweenTags(fileStream, &buffer, "comment"))
          return false;
        if (!_parseTag(fileStream, &buffer, "/comment"))
          return false;

      } else if (tagText == "include") {
        if (!ar_getTextBeforeTag(&fileStream, &buffer)) {
          ar_log_error() << "incomplete include in " << fileNameVerbose << ".\n";
          fileStream.ar_close();
          return false;
        }
        stringstream includeText(buffer.data);
        string include;
        includeText >> include;
        if (!_parseTag(fileStream, &buffer, "/include"))
          return false;
        if (!parseParameterFile(include)) {
          ar_log_error() << "include directive '" << include << "' failed in " <<
	    fileNameVerbose << ".\n";
          return false;
        }
      }

      else if (tagText == "param") {
        if (!_parseTag(fileStream, &buffer, "name"))
	  return false;
        if (!_parseBetweenTags(fileStream, &buffer, "parameter name"))
          return false;
        stringstream nameText(buffer.data);
        string name;
        nameText >> name;
        if (name == "") {
          ar_log_error() << fileNameVerbose << " had empty name.\n";
          fileStream.ar_close();
	  return false;
        }
        if (!_parseTag(fileStream, &buffer, "/name"))
	  return false;
        if (!_parseTag(fileStream, &buffer, "value"))
	  return false;
        if (!ar_getTextUntilEndTag(&fileStream, "value", &buffer)) {
          ar_log_error() << fileNameVerbose << " had incomplete parameter value.\n";
          fileStream.ar_close();
          return false;
        }
        setGlobalAttribute(name, buffer.data);
        if (!_parseTag(fileStream, &buffer, "/param"))
	  return false;

      } else if (tagText == "assign") {
        if (!_parseBetweenTags(fileStream, &buffer, "assignment"))
          return false;
        parseAssignmentString(buffer.data);
        if (!_parseTag(fileStream, &buffer, "/assign"))
          return false;

      } else if (tagText == "NULL") {
	// ar_getTagText() reports syntax errors as "NULL".
	// todo: report line numbers.
        ar_log_error() << fileNameVerbose << ": skipping syntax error.\n";
	// Maybe we'll recover.

      } else if (tagText == "/szg_config") {
        // Finished.
        break;

      } else {
        ar_log_error() << fileNameVerbose << " had illegal XML tag '" << tagText << "'.\n";
        fileStream.ar_close();
        return false;
      }
    }
    fileStream.ar_close();
    return true;
  }

  fileStream.ar_close();
  ar_log_warning() << "parsing pre-Syzygy-0.7 config file.\n";
  FILE* theFile = ar_fileOpen(fileName, dataPath, "r", "Syzygy config");
  if (!theFile) {
    return false;
  }

  // Bug: finite buffer lengths.  Goes away after we deprecate pre-0.7 syntax.
  char buf[4096];
  char buf1[4096], buf2[4096], buf3[4096], buf4[4096];
  while (fgets(buf, sizeof(buf)-1, theFile)) {
    // skip comments which begin with (whitespace and) an octathorp;
    // also skip blank lines.
    char* pch = buf + strspn(buf, " \t");
    if (*pch == '#' || *pch == '\n' || *pch == '\r')
      continue;

    if (sscanf(buf, "%s %s %s %s", buf1, buf2, buf3, buf4) != 4) {
      ar_log_error() << "ignoring incomplete command: " << buf; // already has \n.
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
	      			 const string& validValues) {
  return getAttribute(_userName,computerName,groupName,parameterName,
                      validValues);
}

string arSZGClient::getAttribute(const string& userName,
                                 const string& computerName,
                                 const string& groupName,
                                 const string& parameterName,
                                 const string& validValues) {
  string result;
  string tmp;
  if (!_connected) {
    // Query the local parameter file.
    result = _getAttributeLocal(computerName, groupName, parameterName,
                                validValues);
    // bug? if failed, shouldn't we ar_getenv(groupName+"_"+parameterName) too?
  } else {
    // Query the szgserver.
    const string query(
      ((computerName == "NULL") ? _computerName : computerName) +
      "/" + groupName + "/" + parameterName);
    arStructuredData* getRequestData = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
    int match = _fillMatchField(getRequestData);
    if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,query) ||
        !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"value") ||
        !getRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
        !_dataClient.sendData(getRequestData)) {
      ar_log_error() << "failed to send command.\n";
      tmp = ar_getenv(groupName+"_"+parameterName);
    } else {
      tmp = _getAttributeResponse(match);
      if (tmp == "NULL") {
        tmp = ar_getenv(groupName+"_"+parameterName);
      }
    }
    result = _changeToValidValue(groupName, parameterName, tmp, validValues);
    _dataParser->recycle(getRequestData);
  }
  return result;
}



// Returns 0 on error.
int arSZGClient::getAttributeInt(const string& groupName,
				 const string& parameterName) {
  return getAttributeInt("NULL", groupName, parameterName, "");
}

int arSZGClient::getAttributeInt(const string& computerName,
                                 const string& groupName,
		                 const string& parameterName,
				 const string& defaults) {
  const string& s = getAttribute(computerName, groupName, parameterName, defaults);
  if (s == "NULL")
    return 0;

  int x = -1;
  if (ar_stringToIntValid(s, x))
    return x;

  ar_log_error() << "failed to convert '" << s << "' to an int in " <<
    groupName << "/" << parameterName << ar_endl;
  return 0;
}

bool arSZGClient::getAttributeFloats(const string& groupName,
		                     const string& parameterName,
				     float* values,
				     int numvalues) {
  const string& s = getAttribute("NULL", groupName, parameterName, "");
  if (s == "NULL") {
    // Don't warn, since callers should fall back to a default.
    return false;
  }
  const int num = ar_parseFloatString(s,values,numvalues);
  if (num != numvalues) {
    ar_log_error() << "parameter " << groupName << "/" << parameterName <<
      " needed " << numvalues << " floats, but got only " << num <<
      " from '" << s << "'.\n";
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
    ar_log_error() << "parameter " << groupName << "/" << parameterName <<
      " needed " << numvalues << " longs, but got only " << num <<
      " from '" << s << "'.\n";
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
    ar_log_error() << "parameter " << groupName << "/" << parameterName <<
      " needed " << numvalues << " longs, but got only " << num <<
      " from '" << s << "'.\n";
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
			       const string& parameterValue) {
  // Default userName is _userName.
  return setAttribute(_userName,computerName,groupName,
                      parameterName,parameterValue);
}

bool arSZGClient::setAttribute(const string& userName,
                               const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue) {
  if (!_connected) {
    // Set attributes in the local database.  parseParameterFile
    // calls this when reading the local config file, when standalone.
    return _setAttributeLocal(computerName, groupName, parameterName, parameterValue);
  }

  const string query(
    (computerName=="NULL" ? _computerName : computerName) +
    "/" + groupName + "/" + parameterName);

  // Get storage for the message.
  arStructuredData* setRequestData = _dataParser->getStorage(_l.AR_ATTR_SET);
  bool ok = true;
  const ARint temp = 0; // don't test-and-set.
  int match = _fillMatchField(setRequestData);
  if (!setRequestData->dataInString(_l.AR_ATTR_SET_ATTR,query) ||
      !setRequestData->dataInString(_l.AR_ATTR_SET_VAL,parameterValue) ||
      !setRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !setRequestData->dataIn(_l.AR_ATTR_SET_TYPE,&temp,AR_INT,1) ||
      !_dataClient.sendData(setRequestData)) {
    ar_log_error() << "send failed while setting "
                     << groupName << "/" << parameterName << " on host '"
	             << computerName << "' to '" << parameterValue << "'.\n";
    ok = false;
  }
  _dataParser->recycle(setRequestData);
  if (!ok)
    return false;

  arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
  if (!ack) {
    ar_log_error() << "ack failed while setting "
                     << groupName << "/" << parameterName << " on host '"
	             << computerName << "' to '" << parameterValue << "'.\n";
    return false;
  }
  _dataParser->recycle(ack);
  return true;
}

// Attributes in the database, from the arSZGClient perspective, are
// organized (under a given user name) hierarchically by
// host/attribute_group/attribute. Internally to the szgserver, this
// hierarchy has limited meaning since the parameter database is *really*
// key/value pairs. Sometimes we want
// to dispense with the hierarchy. For instance, the
// configuration of an input node (the filters to use, whether it should
// get input from the network, whether there are any special input sinks,
// etc.) isn't tied to a particular host.
// NOTE: We use a different function name since there is already a
// getAttribute with 2 const string& parameters.
// "Global" attributes differ from "local" attributes for a particular host.
string arSZGClient::getGlobalAttribute(const string& userName,
                                       const string& attributeName) {
  if (!_connected) {
    // Use the local parameter file.
    return _getGlobalAttributeLocal(attributeName);
  }

  // Query the szgserver.
  string result;
  arStructuredData* getRequestData = _dataParser->getStorage(_l.AR_ATTR_GET_REQ);
  const int match = _fillMatchField(getRequestData);
  if (!getRequestData->dataInString(_l.AR_ATTR_GET_REQ_ATTR,attributeName) ||
      !getRequestData->dataInString(_l.AR_ATTR_GET_REQ_TYPE,"value") ||
      !getRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !_dataClient.sendData(getRequestData)) {
    ar_log_error() << "failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(getRequestData);
  return result;
}

// The userName is implicit in this one (i.e. it is the name of the
// Syzygy user executing the program)
string arSZGClient::getGlobalAttribute(const string& attributeName) {
  return getGlobalAttribute(_userName, attributeName);
}

// It is also necessary to set the global attributes...
bool arSZGClient::setGlobalAttribute(const string& userName,
				     const string& attributeName,
				     const string& attributeValue) {
  if (!_connected) {
    // Set attributes in the local database (parseParameterFile calls
    // this when reading the local config file when standalone).
    return _setGlobalAttributeLocal(attributeName, attributeValue);
  }

  // Get storage for the message.
  arStructuredData* setRequestData = _dataParser->getStorage(_l.AR_ATTR_SET);
  bool status = true;
  const ARint temp = 0; // don't test-and-set.
  int match = _fillMatchField(setRequestData);
  if (!setRequestData->dataInString(_l.AR_ATTR_SET_ATTR,attributeName) ||
      !setRequestData->dataInString(_l.AR_ATTR_SET_VAL,attributeValue) ||
      !setRequestData->dataInString(_l.AR_PHLEET_USER,userName) ||
      !setRequestData->dataIn(_l.AR_ATTR_SET_TYPE,&temp,AR_INT,1) ||
      !_dataClient.sendData(setRequestData)) {
    ar_log_error() << "send failed while setting " << attributeName << "to " <<
      attributeValue << ".\n";
    status = false;
  }
  // Must recycle this.
  _dataParser->recycle(setRequestData);
  if (!status) {
    return false;
  }

  arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
  if (!ack) {
    ar_log_error() << "ack failed while setting " << attributeName << "to " <<
      attributeValue << ".\n";
    return false;
  }
  _dataParser->recycle(ack);
  return true;
}

// The Syzygy user name is implicit in this one.
bool arSZGClient::setGlobalAttribute(const string& attributeName,
				     const string& attributeValue) {
  return setGlobalAttribute(_userName, attributeName, attributeValue);
}

/*

Using XML records in the szg parameter database has upsides and downsides.
Upside: easy to manipulate whole display descriptions.
Downside: hard to make little changes to a (possibly quite complex)
display description.
This method attempts to mitigate this downside. It accesses XML stored
in the Syzygy parameter database, exploiting the hierarchical nature of
XML to access individual attributes in the doc tree.
The pathList variable stores the path via which we search into the XML.

1. The first member of the path gives the name of the Syzygy global
parameter where we will start.

2. Subsequent members define child XML "elements". They are element names
and can be given array indices (so that the 2nd element can be specified).

global parameter name = foo
global parameter value =
<szg_display>
<szg_window>
...
</szg_window>
<szg_window>
+++
</szg_window>
</szg_display>

In this case, the path foo/szg_window refers to:

<szg_window>
...
</szg_window>

while foo/szg_window[1] refers to:

<szg_window>
+++
</szg_window>

3. The final member of the path can refer to an XML "attribute". All of
the configuration data for arGUI is stored in "attributes". They look like
this in the XML:

  <szg_viewport_list viewmode="custom" />

Here, viewmode is an attribute of the szg_viewport_list element. If a path
parses down to an attribute BEFORE getting to its final member, an error
occurs (return "NULL").

4. "Variable substitution." Syzygy lets XML documents be stored in
multiple global parameters, simplifying reuse of individual pieces of
complex configs.  This uses XML "pointers" such as

  <szg_window usenamed="foo" />

This element is substituted with the value of global parameter "foo."
Alteration to an attribute value from this point forward will occur
inside the XML doc inside the global parameter "foo".

NOTE: There is an exception to the aforementioned pointer parsing rule. We
need to be able to change the usenamed attribute instead of just following
the pointer. Consequently, if the *next* member of the path is "usenamed"
we won't actually follow the pointer, but will instead stop at the
attribute in question.

pathList gives the path, which is parsed and interpreted as above.
attributeValue is set to something besides "NULL" if we will be altering
an attribute value. If the path does not parse to an attribute, then this
is an error (returning "NULL") but otherwise returning "SZG_SUCCESS".
On the other hand, if attributeValue is "NULL" (the default) then the 
method just returns the value indicated by the parsed path (or "NULL"
if there is an error in the parsing).

*/

string arSZGClient::getSetGlobalXML(const string& userName,
                                    const arSlashString& pathList,
                                    const string& attributeValue = "NULL") {
  int iPath = 0;

  // The global attribute storing the XML doc.
  string szgDocLocation(pathList[iPath]);

  // The XML document itself, in string format.
  string docString = getGlobalAttribute(userName, szgDocLocation);
  TiXmlDocument doc;
  doc.Clear();
  doc.Parse(docString.c_str());
  if (doc.Error()) {
    ar_log_error() << "syntax error in gui config xml, line " << doc.ErrorRow() << ".\n";
    return string("NULL");
  }

  TiXmlNode* node = doc.FirstChild();
  if (!node || !node->ToElement()) {
    ar_log_error() << "malformed XML (global parameter).\n";
    return string("NULL");
  }

  // Traverse the XML tree, using the path defined by pathList.
  TiXmlNode* child = node;
  for (iPath = 1; iPath < pathList.size(); ++iPath) {
    // Searching down the doc tree, an array syntax picks out child elements.
    // For instance, the first szg_viewport child is
    // szg_viewport[0] or szg_viewport; the second child is szg_viewport[1].

    // Default to the first child element with the appropriate name.
    int actualArrayIndex = 0;

    // Actual type of the element. Default is the current step
    // in the path, but an array index can override that.
    string actualElementType(pathList[iPath]);

    // If there is an array index, remove it.
    const unsigned firstArrayLoc = pathList[iPath].find('[');
    if ( firstArrayLoc != string::npos ) {
      // There might be a valid array index.
      const unsigned lastArrayLoc = pathList[iPath].find_last_of("]");
      if (lastArrayLoc == string::npos) {
	ar_log_error() << "invalid array index in " << pathList[iPath] << ".\n";
	return string("NULL");
      }

      const string potentialIndex(
        pathList[iPath].substr(firstArrayLoc+1, lastArrayLoc-firstArrayLoc-1));
      if (!ar_stringToIntValid(potentialIndex, actualArrayIndex)) {
	ar_log_error() << "invalid array index " << potentialIndex << ".\n";
	return string("NULL");
      }

      if (actualArrayIndex < 0) {
	ar_log_error() << "negative array index " << actualArrayIndex << ".\n";
	return string("NULL");
      }

      // Strip out the index.  We already have actualArrayIndex.
      actualElementType = pathList[iPath].substr(0, firstArrayLoc);
    }

    // Get the first child. If we want something further (i.e. we are
    // trying to use an array index, as tested for above), iterate from there.
    TiXmlNode* newChild = child->FirstChild(actualElementType);
    int which = 1;
    if (newChild) {
      // Step through siblings.
      while (newChild && which <= actualArrayIndex) {
        newChild = newChild->NextSibling();
	which++;
      }
    }
    // At this point, we've either got the element itself (as determined by the
    // array index) or we've got an error (maybe there weren't enough elements
    // of this type in the doc tree to justify the array index).
    if (which < actualArrayIndex) {
      ar_log_error() << "no elements of type " << actualElementType << ", up to index " 
	             << actualArrayIndex << ".\n";
      return string("NULL");
    }
     
    // Actually, there is one more possibility. Our path might have specified
    // an "attribute" instead of an "element". This is OK (as long as it is the
    // final step in the path and does not have an array index).
    if (!newChild) {
      // This might still be OK. It could be an "attribute".
      if (!child->ToElement()->Attribute(pathList[iPath])) {
	ar_log_error() << "expected element or attribute, not '" <<
	  pathList[iPath] << ".\n";
	return string("NULL");
      }

      string attribute = child->ToElement()->Attribute(pathList[iPath]);
      if (iPath != pathList.size() - 1) {
	ar_log_error() << "attribute name '" << pathList[iPath] << "' not last in path list.\n";
	return string("NULL");
      }
      if (attributeValue != "NULL") {
	// Alter the XML doc in the parameter database.
	child->ToElement()->SetAttribute(pathList[iPath], attributeValue.c_str());
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
    else{
      // Check to make sure this is valid XML. 
      if (!newChild->ToElement()) {
	ar_log_error() << "malformed XML in " << pathList[iPath] << ".\n";
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
          && (iPath+1 >= pathList.size() 
              || pathList[iPath+1] != "usenamed")) {
	// Keep track of which sub-document we are in fact
	// holding, as we search down the tree of pointers.
	szgDocLocation = newChild->ToElement()->Attribute("usenamed");
        const string newDocString =
          getGlobalAttribute(userName, szgDocLocation);
        doc.Clear();
        doc.Parse(newDocString.c_str());
        newChild = doc.FirstChild();
	if (!newChild || !newChild->ToElement()) {
          ar_log_error() << "malformed XML in pointer '" << szgDocLocation << "'.\n";
	  return string("NULL");
	}
      }
      child = newChild;
    }
  }
  // By making it out here, we know that the final piece of the path refers
  // to an element (not an attribute).
  //
  // DOn't set an attribute to a new value, because the parameter path ended in
  // an XML element, not an attribute.
  if (attributeValue != "NULL") {
    return string("NULL");
  }

  // Query a value, perhaps to get a whole XML doc from the parameter database.
  std::string output;
  output << *child;
  return output;
}

// DOES NOT WORK IN THE LOCAL (STANDALONE) CASE!!!
// ALSO... Should this really be left in? This was an early attempt
// at providing lock-like functionality for a cluster.
string arSZGClient::testSetAttribute(const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue) {
  // use the local user name
  return testSetAttribute(
    _userName, computerName, groupName, parameterName, parameterValue);
}

// DOES NOT WORK IN THE LOCAL (STANDALONE) CASE!!!
string arSZGClient::testSetAttribute(const string& userName,
                               const string& computerName,
			       const string& groupName,
			       const string& parameterName,
			       const string& parameterValue) {
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
      !_dataClient.sendData(setRequestData)) {
    ar_log_error() << "failed to send command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  _dataParser->recycle(setRequestData);
  return result;
}

string arSZGClient::getProcessList() {
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
      !_dataClient.sendData(getRequestData)) {
    ar_log_error() << "failed to send getProcessList command.\n";
    result = string("NULL");
  }
  else{
    result = _getAttributeResponse(match);
  }
  // Must recycle the storage
  _dataParser->recycle(getRequestData);
  return result;
}

// Kill (or remove from the szgserver table, really) by component ID.
bool arSZGClient::killProcessID(int id) {
  if (!_connected)
    return false;

  // get storage for the message.
  arStructuredData* killIDData = _dataParser->getStorage(_l.AR_KILL);
  bool ok = true;
  // One of the few places where we DON'T USE MATCH!
  (void)_fillMatchField(killIDData);
  if (!killIDData->dataIn(_l.AR_KILL_ID,&id,AR_INT,1) ||
      !_dataClient.sendData(killIDData)) {
    ar_log_error() << "failed to send kill-process message.\n";
    ok = false;
  }
  _dataParser->recycle(killIDData); // recycle the storage
  return ok;
}

// Kill the ID of the first process in the process list
// running on the given computer with the given label
// Return false if no process was found.
bool arSZGClient::killProcessID(const string& computer,
                                const string& processLabel) {
  if (!_connected)
    return false;

  const string realComputer = (computer == "NULL" || computer == "localhost") ?
    // use the computer we are on as the default
    _computerName :
    computer;

  const int id = getProcessID(realComputer, processLabel);
  return id >= 0 && killProcessID(id);
}

// Given the process ID, return the process label
string arSZGClient::getProcessLabel(int processID) {
  // massive copypaste between getProcessLabel and getProcessID
  if (!_connected)
    return string("NULL");

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "label");
  // copypaste start with below
  data->dataIn(_l.AR_PROCESS_INFO_ID, &processID, AR_INT, 1);
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request process label.\n";
    return string("NULL");
  }

  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data) {
    ar_log_error() << "got no process label.\n";
    return string("NULL");
  }

  const string theLabel(data->getDataString(_l.AR_PROCESS_INFO_LABEL));
  // This is a different data record than above, so recycle it.
  _dataParser->recycle(data);
  // copypaste end with below
  return theLabel;
}

// Return the ID of the first process in the process list
// running on the given computer with the given label.
int arSZGClient::getProcessID(const string& computer,
                              const string& processLabel) {
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
  // copypaste start with above
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request process ID.\n";
    return -1;
  }

  _dataParser->recycle(data);
  // get the response
  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data) {
    ar_log_error() << "got no process ID.\n";
    return -1;
  }

  const int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // This is a different data record than above, so recycle it.
  _dataParser->recycle(data);
  // copypaste end with above
  return theID;
}

// Return the ID of the process which is "me".
int arSZGClient::getProcessID(void) {
  // todo: _pid caches theID, once it's gotten a valid value.

  if (!_connected)
    return -1;

  arStructuredData* data = _dataParser->getStorage(_l.AR_PROCESS_INFO);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_PROCESS_INFO_TYPE, "self");
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "getProcessID() send failed.\n";
    _dataParser->recycle(data);
    return -1;
  }
  _dataParser->recycle(data);

  data = _getTaggedData(match, _l.AR_PROCESS_INFO);
  if (!data) {
    ar_log_error() << "ignoring illegal response packet.\n";
    return -1;
  }

  const int theID = data->getDataInt(_l.AR_PROCESS_INFO_ID);
  // This is a new piece of data, so recycle it.
  _dataParser->recycle(data);
  return theID;
}

// DOES THIS REALLY BELONG AS A METHOD OF arSZGClient?
// MAYBE THIS IS A HIGHER_LEVEL CONSTRUCT.
bool arSZGClient::sendReload(const string& computer,
                             const string& processLabel) {
  if (!_connected)
    return false;

  if (processLabel == "NULL") {
    // Vacuously ok.
    return true;
  }
  const int pid = getProcessID(computer, processLabel);
  const int ok = pid != -1 && sendMessage("reload", "NULL", pid) >= 0;
  if (!ok)
    ar_log_error() << "failed to reload on host '" << computer << "'.\n";
  return ok;
}

int arSZGClient::sendMessage(const string& type, const string& body,
			     int destination, bool responseRequested) {
  return sendMessage(type,body,"NULL",destination, responseRequested);
}

// Sends a message and returns the "match" for use with getMessageResponse.
// This "match" links responses to the original messages. On an error, this
// function returns -1.
int arSZGClient::sendMessage(const string& type, const string& body,
                             const string& context, int destination,
                             bool responseRequested) {
  if (!_connected)
    return -1;

  // Storage for the message.  Recycle when done.
  arStructuredData* messageData = _dataParser->getStorage(_l.AR_SZG_MESSAGE);
  // convert from bool to int
  const int response = responseRequested ? 1 : 0;
  int match = _fillMatchField(messageData);
  if (!messageData->dataIn(_l.AR_SZG_MESSAGE_RESPONSE, &response, AR_INT,1) ||
      !messageData->dataInString(_l.AR_SZG_MESSAGE_TYPE,type) ||
      !messageData->dataInString(_l.AR_SZG_MESSAGE_BODY,body) ||
      !messageData->dataIn(_l.AR_SZG_MESSAGE_DEST,&destination,AR_INT,1) ||
      !messageData->dataInString(_l.AR_PHLEET_USER,_userName) ||
      !messageData->dataInString(_l.AR_PHLEET_CONTEXT,context) ||
      !_dataClient.sendData(messageData)) {
    ar_log_error() << "failed to send message.\n";
    match = -1;
  } else {
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_MESSAGE_ACK);
    if (!ack) {
      ar_log_error() << "got no message ack.\n";
      match = -1;
    } else {
      if (ack->getDataString(_l.AR_SZG_MESSAGE_ACK_STATUS)
            != string("SZG_SUCCESS")) {
        ar_log_error() << "failed to send message.\n";
        match = -1;
      }
      _dataParser->recycle(ack);
    }
  }
  _dataParser->recycle(messageData);
  return match;
}

// Receive a message routed through the szgserver.
// Returns the ID of the received message, or 0 on error (client should exit).
// @param userName Set to the username associated with the message
// @param messageType Set to the type of the message
// @param messageBody Set to the body of the message
// @param context Set to the context of the message (the virtual computer)
int arSZGClient::receiveMessage(string* userName, string* messageType,
                                string* messageBody, string* context) {
  if (!_connected) {
    return 0;
  }

  // This is the one place we get a record via its type, not its match.
  arStructuredData* data = _getDataByID(_l.AR_SZG_MESSAGE);
  if (!data) {
    // Too low-level for a warning.  Caller should warn, if they care.
    return 0;
  }

  *messageType = data->getDataString(_l.AR_SZG_MESSAGE_TYPE);
  *messageBody = data->getDataString(_l.AR_SZG_MESSAGE_BODY);

  // Return username and context.
  if (userName)
    *userName = data->getDataString(_l.AR_PHLEET_USER);
  if (context)
    *context = data->getDataString(_l.AR_PHLEET_CONTEXT);
  const int messageID = data->getDataInt(_l.AR_SZG_MESSAGE_ID);
  _dataParser->recycle(data);
  return messageID;
}

// Get a response to the client's messages.  Returns 1 if a final response has
// been received, -1 if a partial response has been received, and 0 if failure
// (as might happen, for instance, if a component dies before having a
// chance to respond to a message).
// Param body is filled-in with the body of the message response
// Param match is filled-in with the "match" of this response (which is
// the same as the "match" of the original message that generated it).
int arSZGClient::getMessageResponse(list<int> tags,
                                    string& body,
                                    int& match,
                                    int timeout) {
  if (!_connected) {
    return 0;
  }
  arStructuredData* ack = NULL;
  match = _getTaggedData(ack, tags,_l.AR_SZG_MESSAGE_ADMIN, timeout);
  if (match < 0) {
    ar_log_error() << "no message response.\n";
    return 0;
  }
  if (ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_TYPE) != "SZG Response") {
    ar_log_error() << "no message response.\n";
    _dataParser->recycle(ack);
    body = string("arSZGClient internal error: failed to get proper response");
    return 0;
  }

  body = ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_BODY);
  // Determine whether this was a partial response.
  const string status(ack->getDataString(_l.AR_SZG_MESSAGE_ADMIN_STATUS));
  _dataParser->recycle(ack);

  if (status == "SZG_SUCCESS") {
    return 1;
  }
  if (status == "SZG_CONTINUE") {
    return -1;
  }
  if (status == "SZG_FAILURE") {
    body = string("failure response (remote component died).\n");
    return 0;
  }
  body = string("arSZGClient internal error: response with unknown status ")
    + status + string(".\n");
  return 0;
}

// Respond to a message. Return
// true iff the response was successfully dispatched.
// @param messageID ID of the message to which we are trying to respond
// @param body Body we wish to send
// @param partialResponse False iff we won't send further responses
// (the default)
bool arSZGClient::messageResponse(int messageID, const string& body,
                                  bool partialResponse) {
  if (!_connected) {
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
      !_dataClient.sendData(messageAdminData)) {
    ar_log_error() << "failed to send message response.\n";
  }
  else{
    state = _getMessageAck(match, "message response");
  }
  _dataParser->recycle(messageAdminData);
  return state;
}

// Start a "message ownership trade".
// Useful when the executable launched by szgd wants to respond to "dex".
// Returns -1 on failure and otherwise the "match" associated with this
// request.
// @param messageID ID of the message we want to trade.  We need to
// own the right to respond to this message for the call to succeed.
// @param key Value of which a future ownership trade can occur
int arSZGClient::startMessageOwnershipTrade(int messageID,
                                            const string& key) {
  if (!_connected) {
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
      !_dataClient.sendData(messageAdminData)) {
    ar_log_error() << "failed to send message ownership trade.\n";
    match = -1;
  }
  else{
    (void)_getMessageAck(match, "message trade");
  }
  _dataParser->recycle(messageAdminData);
  return match;
}

// Wait until the message ownership trade completes (or until the optional
// timeout argument has elapsed). Returns false on timeout or other error
// and true otherwise.
bool arSZGClient::finishMessageOwnershipTrade(int match, int timeout) {
  if (!_connected) {
    return false;
  }
  return _getMessageAck(match, "message ownership trade", NULL, timeout);
}

// Revoke the possibility of a previously-made message ownership
// trade.  Return false on error: if the trade has already been
// taken, does not exist, or if we are not authorized to revoke it.
// @param key Fully expanded string on which the ownership trade rests
bool arSZGClient::revokeMessageOwnershipTrade(const string& key) {
  if (!_connected) {
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
      !_dataClient.sendData(messageAdminData)) {
    ar_log_error() << "failed to send message trade revocation.\n";
  }
  else{
    state = _getMessageAck(match, "message trade revocation");
  }
  _dataParser->recycle(messageAdminData);
  return state;
}

// Request ownership of a message that is currently posted with a given key.
// Returns the ID of the message if successful, and 0 otherwise
// NOTE: messages never have ID 0!
int arSZGClient::requestMessageOwnership(const string& key) {
  if (!_connected) {
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
      !_dataClient.sendData(messageAdminData)) {
    ar_log_error() << "failed to request message ownership.\n";
    messageID = 0;
  }
  else{
    if (!_getMessageAck(match, "message ownership request", &messageID)) {
      messageID = 0;
    }
  }
  _dataParser->recycle(messageAdminData);
  return messageID;
}

// Requests that the client be notified when the component with the given ID
// exits (notification will occur immedicately if that ID is not currently
// held).
int arSZGClient::requestKillNotification(int componentID) {
  if (!_connected) {
    return -1;
  }
  arStructuredData* data
    = _dataParser->getStorage(_l.AR_SZG_KILL_NOTIFICATION);
  int match = _fillMatchField(data);
  data->dataIn(_l.AR_SZG_KILL_NOTIFICATION_ID, &componentID, AR_INT, 1);
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request kill notification.\n";
    match = -1;
  }
  _dataParser->recycle(data);
  // We do not get a message back from the szgserver, as is usual.
  // The response will come through getKillNotification.
  return match;
}

// Receives a notification, as requested by the previous method, that a
// component has exited. Return the match if we have succeeded. Return
// -1 on failure.
int arSZGClient::getKillNotification(list<int> tags,
                                     int timeout) {
  if (!_connected) {
    return -1;
  }
  // block until the response occurs (or timeout)
  arStructuredData* data = NULL;
  const int match = _getTaggedData(data, tags, _l.AR_SZG_KILL_NOTIFICATION, timeout);
  if (match < 0) {
    ar_log_remark() << "kill notification timed out.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

// Acquire a lock. Check with szgserver to see
// if the lock is available. If so, acquire the lock for this component
// (and set ownerID to -1). Otherwise, set ownerID to the lock-holding
// component's ID.  Return true iff the lock is acquired.
bool arSZGClient::getLock(const string& lockName, int& ownerID) {
  if (!_connected) {
    ar_log_debug() << "arSZGClient::getLock(): not connected.\n";
    return false;
  }
  // Must get storage for the message.
  arStructuredData* lockRequestData
    = _dataParser->getStorage(_l.AR_SZG_LOCK_REQUEST);
  int match = _fillMatchField(lockRequestData);
  bool state = false;
  if (!lockRequestData->dataInString(_l.AR_SZG_LOCK_REQUEST_NAME,
                                     lockName) ||
      !_dataClient.sendData(lockRequestData)) {
    ar_log_error() << "failed to request lock.\n";
  } else {
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack) {
      ar_log_error() << "no response to lock request.\n";
    } else {
      ownerID = ack->getDataInt(_l.AR_SZG_LOCK_RESPONSE_OWNER);
      state = ack->getDataString(_l.AR_SZG_LOCK_RESPONSE_STATUS) == string("SZG_SUCCESS");
      _dataParser->recycle(ack);
    }
  }
  // Must recycle storage.
  _dataParser->recycle(lockRequestData);
  return state;
}

// Release a lock.  Return false on error,
// if the lock does not exist or if another component is holding it.
bool arSZGClient::releaseLock(const string& lockName) {
  if (!_connected) {
    return false;
  }
  // Must get storage for message.
  arStructuredData* lockReleaseData
    = _dataParser->getStorage(_l.AR_SZG_LOCK_RELEASE);
  int match = _fillMatchField(lockReleaseData);
  bool state = false;
  if (!lockReleaseData->dataInString(_l.AR_SZG_LOCK_RELEASE_NAME,
                                     lockName) ||
      !_dataClient.sendData(lockReleaseData)) {
    ar_log_error() << "failed to request lock.\n";
  }
  else{
    arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_RESPONSE);
    if (!ack) {
      ar_log_error() << "no response to lock request.\n";
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

// Requests that the client be notified when the lock in question is
// not held by the server. NOTE: we return the function match on success
// (which is always nonnegative) or -1 on failure.
int arSZGClient::requestLockReleaseNotification(const string& lockName) {
  if (!_connected) {
    return -1;
  }
  arStructuredData* data
    = _dataParser->getStorage(_l.AR_SZG_LOCK_NOTIFICATION);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_LOCK_NOTIFICATION_NAME, lockName);
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request lock release notification.\n";
    match = -1;
  }
  _dataParser->recycle(data);
  // We do not get a message back from the szgserver, as is usual.
  // The response will come through getLockReleaseNotification.
  return match;
}

// Receives a notification, as requested by the previous method, that a
// lock is no longer held. Return the match if we have succeeded. Return
// -1 on failure.
int arSZGClient::getLockReleaseNotification(list<int> tags,
                                            int timeout) {
  if (!_connected) {
    return -1;
  }
  // block until the response occurs (or timeout)
  arStructuredData* data = NULL;
  int match = _getTaggedData(data, tags, _l.AR_SZG_LOCK_NOTIFICATION, timeout);
  if (match < 0) {
    ar_log_error() << "failed to get lock release notification.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

// Prints all locks currently held inside the szgserver.
void arSZGClient::printLocks() {
  if (!_connected) {
    return;
  }
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_LOCK_LISTING);
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
  // NOTE: we do not attempt to print out the computers on which the
  // locks are held in what follows, just the IDs.
  const int match = _fillMatchField(data);
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "warning: failed to request list of locks.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_LOCK_LISTING);
  if (!ack) {
    ar_log_error() << "got no list of locks.\n";
    return;
  }
  const string locks(data->getDataString(_l.AR_SZG_LOCK_LISTING_LOCKS));
  int* IDs = (int*)data->getDataPtr(_l.AR_SZG_LOCK_LISTING_COMPONENTS, AR_INT);
  int number = data->getDataDimension(_l.AR_SZG_LOCK_LISTING_COMPONENTS);
  // print out the stuff
  int where = 0;
  for (int i=0; i<number; i++) {
    // SHOULD NOT be ar_log_*. This is a rare case where we actually want to print to the terminal.
    cout << ar_pathToken(locks, where) << ";"
         << IDs[i] << "\n";
  }
  _dataParser->recycle(data);
}

// Helper for registerService() and requestNewPorts().
// @param fRetry true iff we're retrying
bool arSZGClient::_getPortsCore1(
       const string& serviceName, const string& channel,
       int numberPorts, arStructuredData*& data, int& match, bool fRetry) {
  if (channel != "default" && channel != "graphics" && channel != "sound"
      && channel != "input") {
    ar_log_error() << (fRetry ? "requestNewPorts" : "registerService") <<
      "() ignoring channel '" << channel <<
      "'; expected one of: default, graphics, sound, input.\n";
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
    _config.getFirstPort(),
    _config.getPortBlockSize() };
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);
  return true;
}

// Helper for registerService() and requestNewPorts().
// @param match from AR_PHLEET_MATCH
// @param portIDs stuffed with the good port values
// @param fRetry true iff we're retrying
bool arSZGClient::_getPortsCore2(arStructuredData* data, int match,
                                 int* portIDs, bool fRetry) {
  if (!_connected)
    return false;

  // _getPortsCore1 filled in the match field.
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request service registration "
	             << (fRetry ? "retry" : "") << ".\n";
    _dataParser->recycle(data);
    return false;
  }
  _dataParser->recycle(data);

  // wait for the response
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data) {
    ar_log_error() << "got no service registration "
	             << (fRetry ? "retry" : "") << "request.\n";
    return false;
  }

  (void)data->getDataInt(_l.AR_PHLEET_MATCH);
  if (data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) != "SZG_SUCCESS") {
    return false;
  }

  const int dimension = data->getDataDimension(_l.AR_SZG_BROKER_RESULT_PORT);
  data->dataOut(_l.AR_SZG_BROKER_RESULT_PORT, portIDs, AR_INT, dimension);
  return true;
}

// Helper function that puts the next "match" value into the given
// piece of structured data. This value is not
// repeated over the lifetime of the arSZGClient and is used to match
// responses of the szgserver to sent messages. This allows arSZGClient
// methods to be called freely from different application threads.
int arSZGClient::_fillMatchField(arStructuredData* data) {
  const int match = ++_match;
  data->dataIn(_l.AR_PHLEET_MATCH,&match,AR_INT,1);
  return match;
}

// Attempt to register a service to be offered by this component with the
// szgserver. Returns true iff the registration was successful (i.e.
// no other service shares the same name).
// @param serviceName name of the offered service
// @param channel routing channel upon which the service is offered
// (must be one of "default", "graphics", "sound", "input")
// @param numberPorts number of ports the service requires (up to 10)
// @param portIDs on success, stuffed with the ports on which
// the service should be offered
bool arSZGClient::registerService(const string& serviceName,
                                  const string& channel,
                                  int numberPorts, int* portIDs) {
  if (!_connected)
    return false;

  arStructuredData* data = NULL;
  int match = -1;
  return _getPortsCore1(serviceName, channel, numberPorts, data, match, false) &&
         _getPortsCore2(data, match, portIDs, false);
}

// The ports assigned to us by the szgserver can be unusable for local
// reasons unknowable to the szgserver, so we may need to ask
// for them to be reassigned. Otherwise, this method is similar to
// registerService(...).
// @param serviceName name of the service that was previously registered
// and which needs a new set of ports.
// @param channel the routing channel upon which we are offering the service
// (must be one of "default", "graphics", "sound", "input")
// @param numberPorts the number of ports required.
// @param portIDs passes in the old, problematic ports, and will be filled
// with new ports on success.
bool arSZGClient::requestNewPorts(const string& serviceName,
                                  const string& channel,
                                  int numberPorts, int* portIDs) {
  if (!_connected) {
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

// The szgserver assigns us ports, but cannot know if
// we'll be able to bind to them (see requestNewPorts() above).
// Furthermore, it cannot fully register the service until
// the component on this end confirms that it is accepting connections,
// lest we get a race condition. Consequently, this function lets a component
// confirm to the szgserver that it was able to bind.  It returns true iff
// the send to the szgserver succeeded. The szgserver sends no response.
// @param serviceName the name of the service
// @param channel the routing channel upon which we are offering the service
// (must be one of "default", "graphics", "sound", "input")
// @param numberPorts the number of ports used by the service
// @param portIDs an array containing the port IDs (all of which were
// successfully bound).
bool arSZGClient::confirmPorts(const string& serviceName,
                               const string& channel,
                               int numberPorts, int* portIDs) {
  if (!_connected) {
    return false;
  }
  if (channel != "default" && channel != "graphics" && channel != "sound"
      && channel != "input") {
    ar_log_error() << "invalid channel for confirmPorts().\n";
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
  temp[0] = _config.getFirstPort();
  temp[1] = _config.getPortBlockSize();
  data->dataIn(_l.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);

  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request service registration retry.\n";
    _dataParser->recycle(data);
    return false;
  }
  _dataParser->recycle(data);

  // the best policy for system stability seems to be that every message
  // sent to the szgserver receives a reply. without this policy, a
  // communications pipe to the szgserver can go away while it is still
  // processing a message received on that pipe.
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data) {
    ar_log_error() << "got no broker result for port confirmation.\n";
    return false;
  }
  (void)data->getDataInt(_l.AR_PHLEET_MATCH);

  return data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) == "SZG_SUCCESS";
}

// Find the internet address for connection to a named service
// over one of (potentially) several specified networks. An arPhleetAddress
// is returned. On success, the valid field is set to true, on failure
// it is set to false. The address field contains the IP address. The
// nuberPorts field contains the number of ports. And portIDs is an array
// (up to 10) containing the port IDs. NOTE: this call can be made in either
// synchronous or asynchronous mode. The difference between the two modes
// shows up only if the requested service is not currently registered
// in the cluster. In synchronous mode, the szgserver immediately responds
// with a failure message. In asynchronous mode, the szgserver declines
// responding and, instead, waits for the service to be registered, at
// which time it responds.  In asynchronous mode, discoverService() thus blocks.
//
// @param serviceName the name of the service to which we want to connect
// @param networks a string containing a slash-delimited list of network
// names we might use to connect to the service, listed in order of
// descending preference. IS THIS TERMINOLOGY BOGUS?
arPhleetAddress arSZGClient::discoverService(const string& serviceName,
                                             const string& networks,
                                             const bool async) {
  arPhleetAddress result;
  if (!_connected) {
    // arNetInputSource::_connectionTask() hardcodes "standalone"
    result.address = "standalone";
    return result;
  }

  result.valid = false;

  // Pack data and send request.
  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_REQUEST_SERVICE);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_COMPUTER, _computerName);
  const int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_TAG, serviceName);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_NETWORKS, networks);
  data->dataInString(_l.AR_SZG_REQUEST_SERVICE_ASYNC, async ? "SZG_TRUE" : "SZG_FALSE");

  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request service.\n";
    _dataParser->recycle(data);
    return result;
  }

  _dataParser->recycle(data);

  const string svc = "service " + serviceName + " on network(s) '" + networks +
    (async ? "', async.\n" : "'.\n");
  ar_log_debug() << "awaiting " << svc;

  // Get response.
  data = _getTaggedData(match, _l.AR_SZG_BROKER_RESULT);
  if (!data) {
    ar_log_error() << "no " << svc;
    return result;
  }

  // Parse response.
  (void)data->getDataInt(_l.AR_PHLEET_MATCH);
  if (data->getDataString(_l.AR_SZG_BROKER_RESULT_STATUS) == "SZG_SUCCESS") {
    result.valid = true;
    result.address = data->getDataString(_l.AR_SZG_BROKER_RESULT_ADDRESS);
    result.numberPorts = data->getDataDimension(_l.AR_SZG_BROKER_RESULT_PORT);
    data->dataOut(_l.AR_SZG_BROKER_RESULT_PORT, result.portIDs, AR_INT, result.numberPorts);
    ar_log_debug() << "service " << serviceName << " from " << result.address << ", " <<
      result.numberPorts << " port(s).\n";
  }
  _dataParser->recycle(data);
  return result;
}

// Print either pending service requests or active services.
// Argument "type" is "registered" or "active".
void arSZGClient::_printServices(const string& type) {
  if (!_connected) {
    return;
  }

  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_GET_SERVICES);
  int match = _fillMatchField(data);

  data->dataInString(_l.AR_SZG_GET_SERVICES_TYPE, type);

  // "NULL" asks szgserver to return all services, not just a particular one.
  data->dataInString(_l.AR_SZG_GET_SERVICES_SERVICES, "NULL");

  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request service list.\n";
    _dataParser->recycle(data);
    return;
  }
  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data) {
    ar_log_error() << "got no service list.\n";
    return;
  }

  const string services(data->getDataString(_l.AR_SZG_GET_SERVICES_SERVICES));
  const arSlashString computers(data->getDataString(_l.AR_SZG_GET_SERVICES_COMPUTERS));
  const int* IDs = (int*) data->getDataPtr(_l.AR_SZG_GET_SERVICES_COMPONENTS, AR_INT);
  const int number = data->getDataDimension(_l.AR_SZG_GET_SERVICES_COMPONENTS);
  int where = 0;
  for (int i=0; i<number; i++) {
    // stdout, not ar_log_xxx().
    cout << computers[i] << ";"
         << ar_pathToken(services, where) << ";"
         << IDs[i] << "\n";
  }
  _dataParser->recycle(data);
}

// Request that the szgserver send us a notification when the named
// service is no longer actively held by a component in the system.
// Returns the match to be used with the other side of this call on
// success or -1 on failure.
int arSZGClient::requestServiceReleaseNotification(const string& serviceName) {
  if (!_connected) {
    return -1;
  }

  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_RELEASE);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_RELEASE_NAME, serviceName);
  data->dataInString(_l.AR_SZG_SERVICE_RELEASE_COMPUTER, _computerName);
  const bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok) {
    ar_log_error() << "failed to request service release notification.\n";
    match = -1;
  }
  // The response will occur in getServiceReleaseNotification.
  return match;
}

// Receive the service release notification triggered in the szgserver
// by the previous method. For thread-safety, we only try to get data from
// a list of tags. There is also an optional timeout argument. If we
// succeed, we simply return the match upon which we were successful.
int arSZGClient::getServiceReleaseNotification(list<int> tags,
                                               int timeout) {
  if (!_connected) {
    return -1;
  }
  // block until the response occurs
  arStructuredData* data;
  const int match = _getTaggedData(data, tags, _l.AR_SZG_SERVICE_RELEASE, timeout);
  if (match < 0) {
    ar_log_error() << "got no service release notification.\n";
  }
  else{
    _dataParser->recycle(data);
  }
  return match;
}

// Tries to get the "info" associated with the given (full) service name.
// Confusingly, at present, this returns the empty string both if the service
// fails to exist AND if the service has the empty string as its info!
string arSZGClient::getServiceInfo(const string& serviceName) {
  if (!_connected) {
    return string("");
  }

  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_OP, "get");
  data->dataInString(_l.AR_SZG_SERVICE_INFO_TAG, serviceName);
  const bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok) {
    ar_log_error() << "failed to request service info.\n";
    return string("");
  }

  // Now, get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data) {
    ar_log_error() << "got no service info.\n";
    return string("");
  }

  const string result = data->getDataString(_l.AR_SZG_SERVICE_INFO_STATUS);
  _dataParser->recycle(data);
  return result;
}

// Attempts to set the "info" string associated with the particular service.
bool arSZGClient::setServiceInfo(const string& serviceName,
                                 const string& info) {
  if (!_connected) {
    return false;
  }

  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_SERVICE_INFO);
  int match = _fillMatchField(data);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_OP, "set");
  data->dataInString(_l.AR_SZG_SERVICE_INFO_TAG, serviceName);
  data->dataInString(_l.AR_SZG_SERVICE_INFO_STATUS, info);
  bool ok = _dataClient.sendData(data);
  _dataParser->recycle(data);
  if (!ok) {
    ar_log_error() << "failed to set service info.\n";
    return false;
  }
  // Get the response.
  data = _getTaggedData(match, _l.AR_SZG_SERVICE_INFO);
  if (!data) {
    ar_log_error() << "no response to set service info.\n";
    return false;
  }
  ok = data->getDataString(_l.AR_SZG_SERVICE_INFO_STATUS) == "SZG_SUCCESS";
  _dataParser->recycle(data);
  return ok;
}

// From the szgserver get the list of running services,
// including the hosts on which they run, and the Syzygy IDs of
// the components hosting them. Print this list like dps does.
void arSZGClient::printServices() {
  _printServices("active");
}

// From the szgserver get the list of pending service requests,
// including the hosts on which they are running and the Syzygy IDs of
// the components making them. Print this list like dps does.
// With this command and the printServices() command,
// one can diagnose connection brokering problems in the distributed system.
void arSZGClient::printPendingServiceRequests() {
  _printServices("pending");
}

// Directly get the Syzygy ID
// of the component running a service. For instance, upon launching an
// app on a virtual computer, we may wish to kill a previously running
// app that supplied a service that the
// new application wishes to supply. Return the component's ID
// if the service exists, otherwise -1.
// @param serviceName the name of the service in question
int arSZGClient::getServiceComponentID(const string& serviceName) {
  if (!_connected) {
    return -1;
  }

  arStructuredData* data = _dataParser->getStorage(_l.AR_SZG_GET_SERVICES);
  int match = _fillMatchField(data);
  // As usual, "NULL" is our reserved string. In this case, it tells the
  // szgserver to return all services instead of just a particular one
  data->dataInString(_l.AR_SZG_GET_SERVICES_TYPE, "active");
  data->dataInString(_l.AR_SZG_GET_SERVICES_SERVICES, serviceName);
  if (!_dataClient.sendData(data)) {
    ar_log_error() << "failed to request service ID.\n";
    _dataParser->recycle(data);
    return -1;
  }

  _dataParser->recycle(data);

  // get the response
  data = _getTaggedData(match, _l.AR_SZG_GET_SERVICES);
  if (!data) {
    ar_log_error() << "got no service ID.\n";
    return -1;
  }

  const int* IDs = (int*) data->getDataPtr(_l.AR_SZG_GET_SERVICES_COMPONENTS, AR_INT);
  const int result = IDs[0];
  _dataParser->recycle(data);
  return result;
}

// Return the networks on which this component should operate.
// Used as an arg for discoverService().
// Networks can be set in 3 ways: by increasing precedence,
// the parameter file, the Syzygy command line args, and the context.
// @param channel Should be either default, graphics, sound, or input.
// This lets network traffic be routed independently for various
// traffic types.
arSlashString arSZGClient::getNetworks(const string& channel) {
  if (channel == "default")
    return _networks;
  if (channel == "graphics")
    return _graphicsNetworks == "NULL" ? _networks : _graphicsNetworks;
  if (channel == "sound")
    return _soundNetworks == "NULL" ? _networks : _soundNetworks;
  if (channel == "input")
    return _inputNetworks == "NULL" ? _networks : _inputNetworks;
  ar_log_error() << "unknown channel for getNetworks().\n";
  return _networks;
}

// Return the addresses on which this component will offer services.
// Used when registering a service, to determine the
// addresses of the interfaces to which other components will try to connect.
arSlashString arSZGClient::getAddresses(const string& channel) {
  if (channel == "default")
    return _addresses;
  if (channel == "graphics")
    return _graphicsAddresses == "NULL" ? _addresses : _graphicsAddresses;
  if (channel == "sound")
    return _soundAddresses == "NULL" ? _addresses : _soundAddresses;
  if (channel == "input")
    return _inputAddresses == "NULL" ? _addresses : _inputAddresses;
  ar_log_error() << "unknown channel for getAddresses().\n";
  return _addresses;
}

// If part of a virtual computer, return its name, otherwise "NULL".
// As with networks, the v.c. comes from both the context and
// from Syzygy command line args.
const string& arSZGClient::getVirtualComputer() {
  return _virtualComputer;
}

// List virtual computers known by the szgserver, space-delimited.
// If none, return "".
string arSZGClient::getVirtualComputers() {
  string hint = getAllAttributes("SZG_CONF/virtual");
  // Skip over "(List):\n"
  hint = hint.erase(0, hint.find('\n') + 1);
  if (hint.empty()) {
    return "";
  }

  // From each line, extract up to the first slash.
  string r;
  unsigned a=0, b=0;
  for (;;) {
    a = hint.find('/');
    b = hint.find('\n');
    if (b == string::npos)
      break;
    r += hint.substr(0, a) + " ";
    hint = hint.erase(0, b+1);
  }
  // Remove the trailing space.
  return r.erase(r.size()-1, 1);
}

// Returns the mode under which this component is operating (with respect
// to a particular channel). For instance, the "default" channel mode is
// one of "trigger", "master" or "component". This refers to the global
// character of what this piece of the system does. The "graphics" channel
// mode is one of "screen0", "screen1", "screen2", etc.
const string& arSZGClient::getMode(const string& channel) {
  if (channel == "default")
    return _mode;
  if (channel == "graphics")
    return _graphicsMode;

  ar_log_error() << "unknown channel for getMode().\n";
  return _mode;
}

// Queries the szgserver to determine the computer designated as the
// trigger for a given virtual computer. Returns "NULL" if the virtual
// computer is invalid or if it does not have a trigger defined.
string arSZGClient::getTrigger(const string& virtualComputer) {
  if (getAttribute(virtualComputer, "SZG_CONF", "virtual", "") != "true") {
    // not a virtual computer
    return "NULL";
  }
  return getAttribute(virtualComputer, "SZG_TRIGGER", "map", "");
}

// Components' service names are compartmentalized,
// to let users share services. There are 3 cases:
//
// 1. Component runs standalone:
//    The Syzygy user name provides the compartmenting, e.g. SZG_BARRIER/ben.
// 2. Component runs as part of virtual computer FOO, which
//    has no "location" defined.  The virtual computer
//    name compartments the service, e.g. FOO/SZG_BARRIER.
// 3. Component runs as part of a virtual computer FOO,
//    which has a "location" component BAR.  The "location" name
//    compartments the service, e.g. BAR/SZG_BARRIER.

string arSZGClient::createComplexServiceName(const string& serviceName) {
  // At the blurry boundary between string and arSlashString.
  if (_virtualComputer == "NULL") {
    return serviceName + string("/") + _userName;
  }

  // getAttribute(_,_,_,"") avoids getAttribute(_,_,_) which lists default values.
  const string location(getAttribute(_virtualComputer,"SZG_CONF","location",""));
  return ((location == "NULL") ? _virtualComputer : location) +
    string("/") + serviceName;
}

inline const string namevalue(const string& name, const string& value) {
  return value=="NULL" ? "" : (name + "=" + value + ";");
}

// Remove any trailing semicolons.
inline const string dropsemi(string& s) {
  const unsigned i = s.find_last_not_of(";");
  return i==string::npos ? s : s.erase(i+1);
}

// Return a context string from internal storage.
// Used to make the launch info header.
string arSZGClient::createContext() {
  string s(
    namevalue("virtual", _virtualComputer) +
    namevalue("mode/default", _mode) +
    namevalue("mode/graphics", _graphicsMode) +
    namevalue("networks/default", _networks) +
    namevalue("networks/graphics", _graphicsNetworks) +
    namevalue("networks/sound", _soundNetworks) +
    namevalue("networks/input", _inputNetworks) +
    namevalue("log", ar_logLevelToString(_logLevel)));
  return dropsemi(s);
}

// Create a context string from the parameters.
// Don't set the related internal variables;
// merely encapsulate a data format that may change over time.
//
// Todo: specify the channels upon which multiple services operate.
string arSZGClient::createContext(const string& virtualComputer,
                                  const string& modeChannel,
                                  const string& mode,
                                  const string& networksChannel,
                                  const arSlashString& networks) {
  string s(
    namevalue("virtual", virtualComputer) +
    namevalue("mode/" + modeChannel, mode) +
    namevalue("networks/" + networksChannel, networks));

  // todo: _logLevel duplicates state of ar_log().logLevel().  Unify them.
  if (ar_log().logLevel() != ar_logLevelToString(_logLevel)) {
    cerr << "arSZGClient warning: resynchronizing loglevel.\n";
    ar_log().setLogLevel(_logLevel);
  }

  if (!ar_log().logLevelDefault()) {
    // Propagate log level to all spawnees.
    s += namevalue("log", ar_logLevelToString(_logLevel));
  }
  return dropsemi(s);
}

// Connect to the szgserver.
bool arSZGClient::_dialUpFallThrough() {
  if (_connected) {
    ar_log_error() << "already connected to szgserver " <<
      _IPaddress << ":" << _port << ".\n";
    return false;
  }

  if (!_dataClient.dialUpFallThrough(_IPaddress.c_str(), _port)) {
    ar_log_error() << "dialUpFallThrough(): no szgserver at " <<
      _IPaddress << ":" << _port << ".\n  (First dlogin; dhunt finds szgservers;\n   Open firewall tcp port " << _port << ".)\n";
      // todo: don't advise dlogin or dhunt if this app IS one of those two!
    return false;
  }

  _connected = true;
  if (!_clientDataThread.beginThread(arSZGClientDataThread, this)) {
    ar_log_error() << "failed to start client data thread.\n";
    return false;
  }

  _dataParser = new arStructuredDataParser(_l.getDictionary());
  return true;
}

arStructuredData* arSZGClient::_getDataByID(int recordID) {
  return _dataParser->getNextInternal(recordID);
}

arStructuredData* arSZGClient::_getTaggedData(int tag, int recordID, int msecTimeout) {
  
  arStructuredData* message = NULL;
  return (_dataParser->getNextTaggedMessage(
	message, list<int>(1,tag), recordID, msecTimeout) < 0) ? NULL : message;
}

int arSZGClient::_getTaggedData(arStructuredData*& message,
                                list<int> tags,
				int recordID,
				int msecTimeout) {
  return _dataParser->getNextTaggedMessage(message, tags, recordID, msecTimeout);
}

bool arSZGClient::_getMessageAck(int match, const char* transaction, int* id,
                                 int msecTimeout) {
  arStructuredData* ack = _getTaggedData(match, _l.AR_SZG_MESSAGE_ACK, msecTimeout);
  if (!ack) {
    ar_log_error() << "no ack of " << transaction << ".\n";
    return false;
  }

  const bool ok = ack->getDataString(_l.AR_SZG_MESSAGE_ACK_STATUS) == "SZG_SUCCESS";
  if (ok && id) {
    // ack has something to stuff id with.
    *id = ack->getDataInt(_l.AR_SZG_MESSAGE_ACK_ID);
  }
  _dataParser->recycle(ack);
  return ok;
}

string arSZGClient::_getAttributeResponse(int match) {
  arStructuredData* ack = _getTaggedData(match, _l.AR_ATTR_GET_RES);
  if (!ack) {
    ar_log_error() << "no ack from szgserver.\n";
    return string("NULL");
  }

  string result(ack->getDataString(_l.AR_ATTR_GET_RES_VAL));
  _dataParser->recycle(ack);
  return result;
}

const string& arSZGClient::getServerName() {
  if (_serverName == "NULL") {
    // Force connection to szgserver, just to get its name.
    (void)_setLabel(_exeName);
  }
  return _serverName;
}

// Set our internal label and set the communicate with the szgserver to set
// our label there (which is a little different from the internal one, being
// <computer>/<exe_name>). Only call this from
// init().
// Make sure that the szgserver name propagates
// to the client (in the return connection ack), so that dlogin's
// "explicit connection" mode gets the szgserver name.
bool arSZGClient::_setLabel(const string& label) {
  if (!_connected)
    return false;

  // Get storage for the message.
  arStructuredData* connectionAckData =
    _dataParser->getStorage(_l.AR_CONNECTION_ACK);
  const string fullLabel = _computerName + "/" + label;
  bool ok = false;
  int match = _fillMatchField(connectionAckData);
  if (!connectionAckData->dataInString(_l.AR_CONNECTION_ACK_LABEL, fullLabel) ||
      !_dataClient.sendData(connectionAckData)) {
    ar_log_error() << "failed to set label.\n";
  }
  else {
    _exeName = label;
    _dataClient.setLabel(_exeName);
    arStructuredData* ack = _getTaggedData(match, _l.AR_CONNECTION_ACK);
    if (!ack) {
      ar_log_error() << "got no szgserver name.\n";
    }
    else{
      _serverName = ack->getDataString(_l.AR_CONNECTION_ACK_LABEL);
      _dataParser->recycle(ack);
      ok = true;
    }
  }
  _dataParser->recycle(connectionAckData);
  return ok;
}

// Parses the string in the environment variable SZGCONTEXT, to determine
// over-ride settings for virtual computer, mode, and networks
bool arSZGClient::_parseContext() {
  const string context(ar_getenv("SZGCONTEXT"));
  if (context == "NULL") {
    // No environment variable, thus nothing to do.
    return true;
  }

  // Context has 3 components.
  for (int i = 0; i < int(context.length())-1; ) {
    if (!_parseContextPair(ar_pathToken(context, i))) {
      return false;
    }
  }

  return true;
}

bool arSZGClient::_parsePair(const string& thePair,
  arSlashString& pair1, string& pair2, string& pair1Type) {
  const unsigned i = thePair.find('=');
  if (i == string::npos) {
    ar_log_error() << "no '=' in context pair '" << thePair << "'.\n";
    return false;
  }

  const unsigned length = thePair.length() - 1;
  if (i == length) {
    ar_log_error() << "nothing after '=' in context pair '" <<
      thePair << "'.\n";
    return false;
  }

  pair1 = thePair.substr(0,i);
  pair2 = thePair.substr(i+1, length-i);
  pair1Type = pair1[0];
  return true;
}

// When a client launches, parse and remove the "Syzygy args" prefaced by -szg.
// But if _parseSpecialPhleetArgs is false (e.g., in dex), then only
// the args user and server (pertaining to login) are parsed and removed.
// If args are removed, argc and argv change, to hide the removals from the caller.
bool arSZGClient::_parsePhleetArgs(int& argc, char** const argv) {
  for (int i=0; i<argc; ++i) {
    if (strcmp(argv[i],"-szg"))
      continue;

    if (i+1 >= argc) {
      ar_log_error() << "nothing after -szg flag.\n";
      return false;
    }

    // Split the pair.
    arSlashString pair1;
    string pair1Type;
    string pair2;
    if (!_parsePair(argv[i+1], pair1, pair2, pair1Type))
      return false;

    if (!_parseSpecialPhleetArgs && pair1Type != "user" && pair1Type != "server") {
      // dex parses ONLY user and server, and forwards the rest to szgd.
      // So skip this arg.
      ++i;
      continue;
    }

    if (!_parseSpecialPhleetArgs && pair1Type == "log") {
      // Parse AND forward to szgd.
      if (!_parseContextPair(argv[i+1])) {
	return false;
      }
      ++i;
      continue;
    }

    // Parse the arg.
    if (!_parseContextPair(argv[i+1])) {
      return false;
    }

    // Remove the arg.
    for (int j=i; j<argc-2; j++) {
      argv[j] = argv[j+2];
    }
    argc -= 2;
    --i;
  }
  return true;
}

// Helper for _parseContext, for FOO=BAR args
// FOO can be "virtual", "mode", "networks/default", "networks/graphics", "networks/sound",
// "networks/input"...  thus, pair1Type can be "virtual", "mode", or "networks".
bool arSZGClient::_parseContextPair(const string& thePair) {
  arSlashString pair1;
  string pair1Type;
  string pair2;
  if (!_parsePair(thePair, pair1, pair2, pair1Type))
    return false;

  if (pair2 == "NULL") {
    // do nothing
    return true;
  }

  if (pair1Type == "virtual") {
    _virtualComputer = pair2;
    return true;
  }

  if (pair1Type == "mode") {
    if (pair1.size() != 2) {
      ar_log_error() << "no channel in parseContextPair()'s mode data.\n";
      return false;
    }
    const string modeChannel(pair1[1]);
    if (modeChannel == "default") {
      _mode = pair2;
    }
    else if (modeChannel == "graphics") {
      _graphicsMode = pair2;
    } else {
      ar_log_error() << "parseContextPair() got invalid mode channel.\n";
      return false;
    }
    return true;
  }

  if (pair1Type == "networks") {
    // Return true iff the networks value is valid,
    // and set _networks and _addresses appropriately.
    if (pair1.size() != 2) {
      ar_log_error() << "no channel in parseContextPair()'s networks data.\n";
      return false;
    }
    return _checkAndSetNetworks(pair1[1], pair2);
  }

  if (pair1Type == "parameter_file") {
    _parameterFileName = pair2;
    return true;
  }

  if (pair1Type == "server") {
    arSlashString serverLocation(pair2);
    if (serverLocation.size() != 2) {
      ar_log_error() << "no ipaddress/port after 'server'.\n";
      return false;
    }
    _IPaddress = serverLocation[0];
    char buffer[1024]; // buffer overflow.  Also needs error checking.
    ar_stringToBuffer(serverLocation[1], buffer, 1024);
    _port = atoi(buffer);
    return true;
  }

  if (pair1Type == "user") {
    _userName = pair2;
    return true;
  }

  if (pair1Type == "log") {
    const int temp = ar_stringToLogLevel(pair2);
    if (!ar_log().setLogLevel(temp)) {
      ar_log_critical() << "bad log level '" << pair2 <<
        "', expected one of: SILENT, CRITICAL, ERROR, WARNING, REMARK, DEBUG.\n";
      return false;
    }

    _logLevel = temp;
    return true;
  }

  ar_log_error() << "bad type in context pair '" << pair1Type <<
    "', expected one of: virtual, mode, networks, parameter_file, server, user, log.\n";
  return false;
}

// networks contains various network names. channel is one of "default",
// "graphics", "input", or "sound". This allows for traffic shaping.
bool arSZGClient::_checkAndSetNetworks(const string& channel, const arSlashString& networks) {
  // sanity check!
  if (channel != "default" && channel != "graphics" && channel != "input"
      && channel != "sound") {
    ar_log_error() << "_checkAndSetNetworks() got unknown channel '" << channel << "'.\n";
    return false;
  }

  // Every new network must be one of the current networks.
  const int numberNewNetworks = networks.size();
  const int numberCurrentNetworks = _networks.size();
  int i=0, j=0;
  for (i=0; i<numberNewNetworks; i++) {
    bool fFound = false;
    for (j=0; j<numberCurrentNetworks && !fFound; ++j) {
      if (networks[i] == _networks[j])
	fFound = true;
    }
    if (!fFound) {
      ar_log_error() << "szg.conf has no virtual computer's network '" <<
        networks[i] << "'.\n";
      ar_log_error() << "\t(szg.conf defines these networks: ";
      for (j=0; j<numberCurrentNetworks; ++j)
	ar_log_error() << _networks[j] << " ";
      ar_log_error() << ")\n";
      return false;
    }
  }

  // Get address of each new network.
  // Inefficient, but one host doesn't have many NICs.
  arSlashString newAddresses;
  for (i=0; i<numberNewNetworks; i++) {
    for (j=0; j<numberCurrentNetworks; j++) {
      if (networks[i] == _networks[j])
	newAddresses /= _addresses[j];
    }
  }

  if (channel == "default") {
    _networks = networks;
    _addresses = newAddresses;
    return true;
  }
  if (channel == "graphics") {
    _graphicsNetworks = networks;
    _graphicsAddresses = newAddresses;
    return true;
  }
  if (channel == "sound") {
    _soundNetworks = networks;
    _soundAddresses = newAddresses;
    return true;
  }
  if (channel == "input") {
    _inputNetworks = networks;
    _inputAddresses = newAddresses;
    return true;
  }
  ar_log_error() << "ignoring unknown channel '" << channel <<
    "', expected one of: default, graphics, sound, input.\n";
  return false;
}

string arSZGClient::launchinfo(const string& u, const string& c) const {
  // Pretty indentation.  Prefix "  |" lets dex filter it out.
  return "  |user=" + u +
       "\n  |computer=" + _computerName +
       "\n  |context=\n  |  " + ar_replaceAll(c, ";", "\n  |  ") + "\n";
}

// Header of messages returned to dex from a launched exe.
string arSZGClient::_generateLaunchInfoHeader() {
  return "  |exe=" + _exeName + "\n" + launchinfo(_userName, createContext());
}

// When standalone, use a locally parsed config file.
string arSZGClient::_getAttributeLocal(const string& computerName,
				       const string& groupName,
				       const string& parameterName,
                                       const string& validValues) {
  const string query =
    ((computerName == "NULL") ? _computerName : computerName) + "/"
    + groupName + "/" + parameterName;
  map<string, string, less<string> >::const_iterator i =
    _localParameters.find(query);
  const string value = (i!=_localParameters.end()) ?
    i->second : ar_getenv(groupName+"_"+parameterName);
  return _changeToValidValue(groupName, parameterName, value, validValues);
}

// When standalone, use a locally parsed config file.
bool arSZGClient::_setAttributeLocal(const string& computerName,
				     const string& groupName,
				     const string& parameterName,
				     const string& parameterValue) {
  const string query =
    ((computerName == "NULL") ? _computerName : computerName) + "/"
    + groupName + "/" + parameterName;
  map<string, string, less<string> >::iterator i = _localParameters.find(query);
  if (i != _localParameters.end())
    _localParameters.erase(i);
  if (parameterValue != "NULL") {
    // Effectively "unset" the previous value.
    _localParameters.insert(map<string,string,less<string> >::value_type
      (query, parameterValue));
  }
  return true;
}

// When standalone ("local"), get a "global" attribute.
string arSZGClient::_getGlobalAttributeLocal(const string& attributeName) {
  map<string, string, less<string> >::iterator i =
    _localParameters.find(attributeName);
  return (i == _localParameters.end()) ? "NULL" : i->second;
}

// When standalone ("local"), set a "global" attribute.
bool arSZGClient::_setGlobalAttributeLocal(const string& attributeName,
					   const string& attributeValue) {
  map<string, string, less<string> >::iterator i =
    _localParameters.find(attributeName);
  if (i != _localParameters.end())
    _localParameters.erase(i);
  _localParameters.insert(map<string,string,less<string> >::value_type
                          (attributeName,attributeValue));
  return true;
}

// Check if a list of valid values contains
// a string constant in the parameter database.
// If not, return the list's first item and warn.
string arSZGClient::_changeToValidValue(const string& groupName,
                                        const string& parameterName,
                                        const string& value,
                                        const string& validValues) {
  if (validValues != "") {
    // String format is "|foo|bar|zip|baz|bletch|".
    if (validValues[0] != '|' || validValues[validValues.length()-1] != '|') {
      ar_log_error() << "getAttribute ignoring malformed validValues '" <<
        validValues << "'.\n";
      return value;
    }
    if (validValues.find('|'+value+'|') == string::npos) {
      const int end = validValues.find('|', 1);
      const string valueNew(validValues.substr(1, end-1));
      if (value != "NULL") {
	ar_log_error() << "expected " <<
	  groupName+'/'+parameterName << " to be one of " << validValues <<
	  ", not '" << value << "'.  Defaulting to " << valueNew << ".\n";
      }
      return valueNew;
    }
  }
  // Either validValues was undefined, or we got one of its entries.
  return value;
}

// Baroquely get configuration information from the szgservers.
void arSZGClient::_serverResponseThread() {
  char buffer[200]; // buffer overflow
  while (_keepRunning) {
    arSocketAddress fromAddress;
    arSleepBackoff a(7, 40, 1.1);
    while (_discoverySocket->ar_read(buffer, 200, &fromAddress) < 0) {
      // Win32 returns -1 if no packet was received.
      a.sleep();
    }

    // Got a packet.
    ar_usleep(10000); // avoid busy-waiting on Win32
    arGuard dummy(_lock);
    if (_dataRequested) {
      // Verify format: first 4 bytes are version, 5th indicates response.
      if (buffer[0] == 0 && buffer[1] == 0 &&
          buffer[2] == 0 && buffer[3] == SZG_VERSION_NUMBER && buffer[4] == 1) {
        memcpy(_responseBuffer,buffer,200);
        if (_bufferResponse) {
          // Print this packet's contents.
          const string serverInfo(
	    string(_responseBuffer+5)  + "/" +
	    string(_responseBuffer+132) + ":" +
	    string(_responseBuffer+164));
          bool found = false;
          for (vector<string>::const_iterator i = _foundServers.begin();
	    i != _foundServers.end(); ++i) {
            if (*i == serverInfo) {
              // Found it already (someone else broadcast a response packet?).
              found = true;
              break;
            }
          }
          // Not found.  Explicitly cout, not ar_log_xxx().
	  if (_justPrinting) {
	    cout << serverInfo << "\n";
	  }
	  _foundServers.push_back(serverInfo);
        } else if (_requestedName == string(_responseBuffer+5)) {
	  // Stop, discarding subsequent packets.
	  _dataRequested = false;
	  _dataCondVar.signal();
        }
      }
    }
  }
}

void arSZGClient::_timerThread() {
  while (_keepRunning) {
    _lock.lock();
      while (!_beginTimer) {
	_timerCondVar.wait(_lock);
      }
      _beginTimer = false;
    _lock.unlock();

    ar_usleep(1500000); // Long, for slow or flaky networks.

    arGuard dummy(_lock);
    _dataCondVar.signal();
  }
}

void arSZGClient::_dataThread() {
  while (_keepRunning) {
    // todo: pass a timeout through getData to ar_safeRead, for deterministic shutdown
    if (!_dataClient.getData(_receiveBuffer, _receiveBufferSize)) {
      // Don't complain if the destructor has already been invoked.
      // Disconnect?
      _keepRunning = false;
      ar_log_critical() << "no szgserver.\n";
      break;
    }

    // Data distribution must be multithreaded.
    // Only one thread (which should be in arSZGClient!) receives messages.
    // Everything else responds to something sent to the szgserver,
    // and hence has a "match" for multithreading.

    int size = -1; // unused
    if (ar_rawDataGetID(_receiveBuffer) == _l.AR_SZG_MESSAGE) {
      _dataParser->parseIntoInternal(_receiveBuffer, size);
    }
    else if (ar_rawDataGetID(_receiveBuffer) == _l.AR_KILL) {
      ar_log_critical() << "szgserver forcibly disconnected.\n";
      break;
    }
    arStructuredData* data = _dataParser->parse(_receiveBuffer, size);
    _dataParser->pushIntoInternalTagged(data, data->getDataInt(_l.AR_PHLEET_MATCH));
  }

  // Interrupt any waiting messages, and forbid any future ones.
  _dataParser->clearQueues();

  _connected = false; // inside a lock?
}

// szgserver.cpp describes the format of the discovery and response packets.
// szgserver listens on port 4620 for discovery packets, and responds on port 4621.
void arSZGClient::_sendDiscoveryPacket(const string& name,
                                       const string& broadcast) {
  // Magic version number: first 3 bytes are 0, 4th is version number.
  // Since the 5th byte is 0, this is a discovery packet.
  char buf[200] = {0};
  buf[3] = SZG_VERSION_NUMBER;

  ar_stringToBuffer(name, buf+5, 127);
  arSocketAddress broadcastAddress;
  broadcastAddress.setAddress(broadcast.c_str(), 4620);
  _discoverySocket->ar_write(buf, sizeof(buf), &broadcastAddress);
}

// Sends a broadcast packet on a specified subnet to find the szgserver
// with the specified name.
bool arSZGClient::discoverSZGServer(const string& name,
                                    const string& broadcast) {
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()) {
    ar_log_critical() << "failed to launch discovery threads.\n";
    return false;
  }

  arGuard dummy(_lock);
  _dataRequested = true;
  _beginTimer = true;
  _requestedName = name;
  _foundServers.clear();
  _sendDiscoveryPacket(name,broadcast);
  _timerCondVar.signal();
  while (_dataRequested && _beginTimer) {
    _dataCondVar.wait(_lock);
  }
  if (_dataRequested) {
    // timeout
    return false;
  }

  _serverName = string(_responseBuffer+5);
  _IPaddress = string(_responseBuffer+132);
  _port = atoi(_responseBuffer+164);
  return true;
}

// Find szgservers and print their names.
void arSZGClient::printSZGServers(const string& broadcast) {
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()) {
    // A diagnostic was already printed.  Don't complain here.
    return;
  }
  _bufferResponse = true;
  _justPrinting = true; // Hack. Copypaste of findSZGServers().

  _lock.lock();
    _foundServers.clear();
    _dataRequested = true;
    _beginTimer = true;
    _requestedName = "";
    _sendDiscoveryPacket("*",broadcast);
    _timerCondVar.signal();
    while (_beginTimer) {
      _dataCondVar.wait(_lock);
    }
  _lock.unlock();
  _dataRequested = false;
  _justPrinting = false;
  _bufferResponse = false;
}

// Find szgservers and return their names.
vector<string> arSZGClient::findSZGServers(const string& broadcast) {
  if (!_discoveryThreadsLaunched && !launchDiscoveryThreads()) {
    // A diagnostic was already printed.  Don't complain here.
    vector<string> tmp;
    return tmp;
  }

  _bufferResponse = true;
  _lock.lock();
  _foundServers.clear();
  _dataRequested = true;
  _beginTimer = true;
  _requestedName = "";
  _sendDiscoveryPacket("*",broadcast);
  _timerCondVar.signal();
  while (_beginTimer) {
    _dataCondVar.wait(_lock);
  }
  _lock.unlock();
  _dataRequested = false;
  _bufferResponse = false;
  return _foundServers;
}

// Set the server location manually.
// Necessary if broadcasting to the szgserver fails.
// The szgserver name is set upon connection via a handshake.
void arSZGClient::setServerLocation(const string& IPaddress, int port) {
  _IPaddress = IPaddress;
  _port = port;
}

// Write a login file using data from e.g. discoverSZGServer()
bool arSZGClient::writeLogin(const string& userName) {
  // userName may differ from _userName.
  // The internal storage refers to the effective user name of this
  // component, which the environment variable SZGUSER can override.
  // userName is the Syzygy login name in the config file
  _config.setUserName(userName);
  _config.setServerName(_serverName);
  _config.setServerIP(_IPaddress);
  _config.setServerPort(_port);
  return _config.writeLogin();
}

// Reset the login file to not-logged-in state.
bool arSZGClient::logout() {
  _config.setUserName("NULL");
  _config.setServerName("NULL");
  _config.setServerIP("NULL");
  _config.setServerPort(0);
  return _config.writeLogin();
}

// Default message task which handles "quit" and nothing else.
void ar_messageTask(void* pClient) {
  if (!pClient) {
    ar_log_error() << "ar_messageTask: no Syzygy client, dkill disabled.\n";
    return;
  }

  ((arSZGClient*)pClient)->messageTask();
}

void arSZGClient::messageTask() {
  string messageType, messageBody;
  while (_keepRunning && messageType != "quit") {
    receiveMessage(&messageType, &messageBody);
  }

  closeConnection();
  _keepRunning = false;
  ar_usleep(75000); // let other threads exit cleanly
  exit(0);
}

// Since this kills via ID, it is better suited to killing the graphics programs.
void arSZGClient::killIDs(list<int>* IDList) {
  if (IDList->empty())
    return;

  list<int> kill = *IDList; // local copy

  // Since remote components need not respond to the kill message, ask szgserver.
  // Ugly. Maybe a data structure with the request*Notifications?
  list<int> tags;
  map<int, int, less<int> > tagToID;
  for (list<int>::iterator iter = kill.begin(); iter != kill.end(); ++iter) {
    const int tag = requestKillNotification(*iter);
    tags.push_back(tag);
    tagToID.insert(map<int,int,less<int> >::value_type(tag, *iter));
    sendMessage("quit", "", *iter);
  }

  while (!tags.empty()) {
    // For each component, wait 8 seconds for it to die.
    const int killedID = getKillNotification(tags, 8000);
    if (killedID < 0) {
      ar_log_remark() << "timed out while killing components with IDs:\n";
      for (list<int>::iterator n = tags.begin(); n != tags.end(); ++n) {
	// These components are unhealthy, if not already dead.
	// So just remove them from szgserver's process table.
	map<int,int,less<int> >::const_iterator iter = tagToID.find(*n);
        if (iter == tagToID.end()) {
	  ar_log_critical() << "internal error: missing kill ID.\n";
	}
	else {
	  ar_log_remark() << iter->second << " ";
          killProcessID(iter->second);
	}
      }
      ar_log_remark() << "\nThese components have been removed from szgserver's process table.\n";
      return;
    }
    map<int,int,less<int> >::const_iterator iter = tagToID.find(killedID);
    if (iter == tagToID.end()) {
      ar_log_critical() << "internal error: missing kill ID.\n";
    }
    else {
      ar_log_remark() << "killed component with ID " << iter->second << ".\n";
    }
    tags.remove(killedID);
  }
}
