//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arLogStream.h"

arAppLauncher::arAppLauncher(const char* exeName) :
  _szgClient(NULL),
  _ownedClient(false),
  _vircompDefined(false),
  _onlyIncompatibleServices(true),
  _appType("NULL"),
  _renderer("NULL"),
  _vircomp("NULL"),
  _location("NULL"),
  _exeName("NULL")
{
  if (exeName && *exeName)
    _exeName.assign(exeName);
  else
    _exeName.assign("arAppLauncher");
}

arAppLauncher::~arAppLauncher(){
  if (_ownedClient)
    delete _szgClient;
}

bool arAppLauncher::setRenderProgram(const string& name){
  ar_log_debug() << _exeName << ": renderer is '" << name << "'.\n";
  _renderer = name;
  return true;
}

bool arAppLauncher::setAppType(const string& theType){
  if (theType != "distgraphics" && theType != "distapp"){
    ar_log_warning() << _exeName << " ignoring unexpected type '"
	           << theType << "', expected distapp or distgraphics.\n";
    return false;
  }
  _appType = theType;
  return true;
}

bool arAppLauncher::_szgClientOK() const {
  if (_szgClient)
    return true;

  ar_log_warning() << _exeName << ": no arSZGClient.\n";
  return false;
}

// Set the virtual computer and the location, from the arSZGClient's "context."
bool arAppLauncher::setVircomp(){
  if (!_szgClientOK())
    return false;

  const string s(_szgClient->getVirtualComputer());
  ar_log_debug() << _exeName << ": virtual computer is " << s << ".\n";
  return setVircomp(s);
}

// Set the virtual computer (e.g. for dkillall), without starting a whole app.
bool arAppLauncher::setVircomp(const string& vircomp){
  if (!_szgClientOK())
    return false;

  if (_szgClient->getAttribute(vircomp,"SZG_CONF","virtual","") != "true"){
    const string s(_szgClient->getVirtualComputers());
    ar_log_warning() << _exeName << ": no virtual computer '" << vircomp << "'.\n"
      << "  (Known virtual computers are: " << s << ").\n";
    return false;
  }

  // The v.c. names two locks:
  // The setup lock lets only one cluster-wide reorg happen at once.
  // The demo lock reports what app is running, so we can kill it.

  _vircomp = vircomp;
  const string s(_getAttribute("SZG_CONF", "location", ""));
  _location = s=="NULL" ? _vircomp : s;
  return true;
}

// Encapsulate arSZGClient connection for tiny arAppLauncher-wrapper apps.
bool arAppLauncher::connectSZG(int& argc, char** argv){
  _szgClient = new arSZGClient;
  const bool fInit = _szgClient->init(argc, argv);
  if (!*_szgClient)
    return _szgClient->failStandalone(fInit);

  _ownedClient = true;
  return true;
}

// Often, the program holding the arAppLauncher has
// its own arSZGClient. This provides access to that arSZGClient.
// object as well, which access is provided by this method.
// Must be called before any other method.
bool arAppLauncher::setSZGClient(arSZGClient* SZGClient){
  if (!SZGClient) {
    ar_log_warning() << _exeName << " ignoring setSZGClient(NULL).\n";
    return false;
  }
  _szgClient = SZGClient;
  _ownedClient = false;
  return true;
}

// From szgserver, get and parse and error-check attributes of the virtual computer.
bool arAppLauncher::setParameters(){
  if (!_szgClientOK())
    return false;

  if (_vircompDefined){
    // todo: since many exes (e.g. dkillall) call this,
    // maybe handle when the v.c. was already set explicitly.
    return true;
  }
  
  // If the virtual computer was not explicitly set, set its name and "location."
  if (_vircomp == "NULL" && !setVircomp()){
    ar_log_warning() << _exeName << ": no virtual computer '" << _vircomp << "'.\n";
    const string s(_szgClient->getVirtualComputers());
    if (s.empty()) {
      ar_log_warning() << "  (No virtual computers are defined.  Have you run dbatch?)\n";
    } else {
      ar_log_warning() << "  (Known virtual computers are: " << s << ").\n";
    }
    return false;
  }

  _onlyIncompatibleServices = _getAttribute("SZG_CONF", "relaunch_all", "") != "true";

  {
    const int num = _getAttributeInt("SZG_DISPLAY", "number_screens", "");
    if (num <= 0) {
      ar_log_warning() << _exeName << ": no screens for virtual computer '" << _vircomp << "'.\n";
      return false;
    }
    if (num > 100) {
      ar_log_remark() << _exeName << ": excessive screens for virtual computer " <<
	_vircomp << ", " << num << ".\n";
    }

    _pipes.resize(num);
  } 
 
  int i;
  for (i=0; i<getNumberScreens(); ++i) {
    // A "pipe" is an ordered pair (hostname, screenname).
    const arSlashString pipe(_getAttribute(_screenName(i), "map", ""));
    if (pipe.size() != 2 || pipe[1].substr(0,11) != "SZG_DISPLAY"){
      ar_log_warning() << _exeName << ": screen " << i << " of " <<
	_vircomp << " maps to no (computer, SZG_DISPLAYn) pair.\n";
      return false;
    }
    // Two-stage assignment, because _getRenderContext(i) uses _pipes[i]
    _pipes[i] = arPipe(pipe[0], pipe[1]);
    _pipes[i].renderer = arLaunchInfo(pipe[0], _renderer, _getRenderContext(i));
  }

  // Input.
  const arSlashString inputDevs(_getAttribute("SZG_INPUT0", "map", ""));
  const int numTokens = inputDevs.size();
  if (numTokens%2){
    ar_log_warning() << _exeName
                   << ": invalid input devices format for virtual computer '" << _vircomp
	           << "'.  Expected computer/inputDevice/.../computer/inputDevice.\n";
    return false;
  }

  // Ensure no computer is duplicated, lest we launch two DeviceServers on one.
  for (i=2; i<numTokens; i+=2) {
    const string& computer_i = inputDevs[i];
    for (int j=0; j<i; j+=2) {
      if (computer_i == inputDevs[j]) {
        ar_log_warning() << _exeName << ": input devices for virtual computer '"
          << _vircomp << "' has duplicate computer '" << computer_i
          << "', in definition \n  '"
          << inputDevs
          << "'.\n  (Instead, put a compound device in your dbatch file.)\n";
        return false;
      }
    }
  }

  _serviceList.clear();
  // Parse the list of "computer/device" pairs.
  for (i=0; i<numTokens; i+=2){
    const string computer(inputDevs[i]);
    string device(inputDevs[i+1]);
    string info(device);

    // The input device in slot 0 actually connects to the app,
    // so it comes FIRST in the <virtual computer>/SZG_INPUT0/map listing.
    // After that come slave devices.
    char buffer[32];
    sprintf(buffer,"%i",i/2);
    
    device = (device == "inputsimulator" ? "" : "DeviceServer ") +
      device + " " + string(buffer);
    if (i < numTokens-2)
      device += " -netinput";
    _addService(computer, device, _getInputContext(), "SZG_INPUT"+string(buffer), info);
  }

  // Sound.
  const string soundLocation(_getAttribute("SZG_SOUND","map",""));
  if (soundLocation != "NULL"){
    _addService(soundLocation,"SoundRender", _getSoundContext(), "SZG_WAVEFORM", "");
  }

  _vircompDefined = true;
  return true;
}  

// Launch the other components of the app on the virtual computer.
bool arAppLauncher::launchApp(){
  // _prepareCommand() includes the lock to match the _unlock()'s below.
  if (!_prepareCommand()){
    ar_log_warning() << _exeName << " failed to prepare launch.\n";
    return false;
  }

  if (_appType == "NULL"){
    ar_log_warning() << _exeName << ": undefined control host or application type.\n";
    _unlock();
    return false;
  }

  // Kill any running app.
  if (!_demoKill()){
    ar_log_warning() << _exeName << " failed to kill previous application.\n";
    _unlock();
    return false;
  }
     
  // Kill every graphics program whose
  // name differs from the new graphics program.  _demoKill above already
  // killed all the render programs for a master/slave app.
  _graphicsKill(_firstToken(_renderer));

  // After the demo program and the graphics programs are
  // killed, no services are provided and no locks are held.

  int i;
  for (i=0; i<getNumberScreens(); i++){
    if (_getPID(i, "szgd") == -1){
      ar_log_warning() << _exeName << ": no szgd for rendering on host " <<
        _pipes[i].hostname << ".\n";
      _unlock();
      return false;
    }
  }

  list<arLaunchInfo> appsToLaunch;
  list<int> serviceKillList;

  if (_onlyIncompatibleServices){
    _relaunchIncompatibleServices(appsToLaunch, serviceKillList);
  }
  else{
    _relaunchAllServices(appsToLaunch, serviceKillList);
  }

  // Launch a "distributed app" on every host.
  for (i=0; i<getNumberScreens(); i++){
    if (_appType == "distapp" || _getPID(i, _firstToken(_renderer)) == -1){
      appsToLaunch.push_back(_pipes[i].renderer);
    }
  }

  // Kill incompatible services.
  _blockingKillByID(&serviceKillList);

  const bool ok = _execList(&appsToLaunch);
  if (!ok){
    ar_log_warning() << _exeName << ": some components didn't launch.\n";
  }
  _unlock(); 
  return ok;
}

// Kill "transient" copies of a master/slave app.
void arAppLauncher::killApp(){
  if (_szgClientOK() && _appType == "distapp")
    _graphicsKill("");
}

bool arAppLauncher::waitForKill(){ 
  if (!_szgClientOK())
    return false;

  while (true) {
    string messageType, messageBody;
    if (!_szgClient->receiveMessage(&messageType,&messageBody)){
      ar_log_error() << _exeName << ": no szgserver.\n";
      // Don't bother calling killApp(), since szgserver is gone.
      break;
    }
    if (messageType == "quit"){
      killApp();
      break;
    }
    const string lockName(getMasterName());
    int componentID;
    if (_szgClient->getLock(lockName, componentID)) {
      // nobody was holding the lock
      _szgClient->releaseLock(lockName);
      ar_log_warning() << _exeName << ": no component running on master screen.\n";
    } else {
      // something is indeed running on the master screen
      _szgClient->sendMessage(messageType, messageBody, componentID);
    }
  }
  return true;
}

bool arAppLauncher::restartServices(){
  if (!_prepareCommand())
    return false;

  // Kill the services.
  iLaunch iter;
  list<int> killList;
  // NOTE: we kill the services by their TRADING KEY and NOT by name!
  for (iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    const int serviceID = _szgClient->getServiceComponentID(iter->tradingTag);
    if (serviceID != -1){
      killList.push_front(serviceID);
    }
  }
  _blockingKillByID(&killList);

  // Restart the services.
  list<arLaunchInfo> launchList;
  for (iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    const int serviceSzgdID = _szgClient->getProcessID(iter->computer, "szgd");
    if (serviceSzgdID != -1){
      launchList.push_back(*iter);
    }
  }
  const bool ok = _execList(&launchList);
  _unlock();
  return ok;
}

bool arAppLauncher::killAll(){
  if (!_prepareCommand())
    return false;

  _demoKill();
  _graphicsKill("");

  // Kill the services.
  list<int> killList;
  string namesKill;
  for (iLaunch iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    namesKill += " " + iter->tradingTag;
    const int serviceID = _szgClient->getServiceComponentID(iter->tradingTag);
    ar_log_debug() << "killAll() " << iter->tradingTag << " id = " << serviceID << ar_endl;
    if (serviceID != -1){
      killList.push_back(serviceID);
    }
  }
  if (!killList.empty()) {
    ar_log_remark() << _exeName << " killing service(s) " << namesKill << ".\n";
  }
  _blockingKillByID(&killList);
  _unlock();
  return true;
}

bool arAppLauncher::screenSaver(){
  if (!_prepareCommand())
    return false;

  _demoKill();

  // Kill any apps which might hold locks which szgrenders will need.
  _graphicsKill("szgrender");
    
  // Start szgrenders.
  list<arLaunchInfo> launchList;
  for (int i=0; i<getNumberScreens(); i++){
    if (_getPID(i, "szgrender") == -1 && _getPID(i, "szgd") != -1){
      launchList.push_back(
        arLaunchInfo(_pipes[i].hostname, "szgrender", _getRenderContext(i)));
    }
  }
  const bool ok = _execList(&launchList);
  _unlock();
  return ok;
}

bool arAppLauncher::isMaster() const {
  return _szgClientOK() &&
    getMasterName() ==
    _szgClient->getComputerName() + "/" + _szgClient->getMode("graphics");
}

// Return the pipe (i.e., computer+screen) running the app's master instance.
string arAppLauncher::getMasterName() const {
  if (!_szgClientOK() || !_vircompDefined)
    return "NULL";

  const string screenName(_getAttribute("SZG_MASTER", "map", ""));
  return screenName=="NULL" ? screenName : _getAttribute(screenName, "map", "");
}

// Return the name of host running the app's trigger instance.
string arAppLauncher::getTriggerName() const {
  if (!_szgClientOK() || !_vircompDefined)
    return "NULL";

  ar_log_debug() << _exeName << " getting trigger map " << _vircomp << "/SZG_TRIGGER/map.\n";
  return _getAttribute("SZG_TRIGGER", "map", "");
}

// Return the name of a screen in the v.c., e.g. "theora/SZG_DISPLAY3".
string arAppLauncher::getScreenName(const int i) const {
  if (!_iValid(i)) {
    ar_log_warning() << _exeName << ": screen " << i << " out of bounds [0,"
                     << getNumberScreens()-1 << "] for " << _vircomp << ".\n";
    return string("NULL");
  }

  return _pipes[i].hostname + "/" + _pipes[i].screenname;
}

bool arAppLauncher::getRenderProgram(const int i, string& computer,
                                     string& renderName) {
  const string renderProgramLock(getScreenName(i));
  if (renderProgramLock == "NULL")
    return false;

  computer = _pipes[i].hostname;
  int renderProgramID = -1;
  if (!_szgClient->getLock(renderProgramLock, renderProgramID)){
    // someone is holding the lock
    const arSlashString renderProgramLabel(_szgClient->getProcessLabel(renderProgramID));
    if (renderProgramLabel != "NULL"){
      // something with this ID is still running
      renderName = renderProgramLabel[1];
      // we do not have the lock
      return true;
    }
  }
  // no program was running. we've got the lock.
  _szgClient->releaseLock(renderProgramLock);
  return false;
}

// On-the-fly modify certain attributes in a display's XML configuration.
// The first window specified in the
// display is the only one modified... and only top-level elements
// with attributes "value" can be modified (currently decorate, fullscreen,
// title, stereo, zorder, xdisplay).
void arAppLauncher::updateRenderers(const string& attribute, 
                                    const string& value) {
  // set up the virtual computer info, if necessary
  if (!setParameters()){
    ar_log_warning() << _exeName <<
      ": updateRenderers got invalid virtual computer definition.\n";
    return;
  }
  string host, program;
  for (int i=0; i<getNumberScreens(); i++) {
    // Even if no render program is running, update the database.
    
    // Find the XML.
    // The empty string is an explicit final parameter, to 
    // disambiguate the case where "computer" (1st param) is not specified
    // but "valid values" are.

    const string configName(_szgClient->getAttribute(_pipes[i].hostname, _pipes[i].screenname, "name", ""));
    // configName is the name of an <szg_display>.
    if (configName == "NULL"){
      ar_log_warning() << _exeName << ": no config for " << getScreenName(i) << ".\n";
    }
    else{
      // Go into the first window defined in the XML global
      // parameter given by configName, and set the specified attribute.
      _szgClient->getSetGlobalXML(configName + "/szg_window/" + attribute + "/value", value);
    }
    if (getRenderProgram(i, host, program)) {
      // Keep going if errors occur.
      (void)_szgClient->sendReload(host, program);
    }
  } 
}

//**************************************************************************
// private functions
//**************************************************************************

// Seperate executable names from parameter lists.
string arAppLauncher::_firstToken(const string& s) const {
  const int first  = s.find_first_not_of(" ");
  const int second = s.find_first_of(" ", first);
  return s.substr(first, second-first);
}

bool arAppLauncher::_prepareCommand(){
  // Get the application lock.
  return setParameters() && _trylock();
}

void arAppLauncher::_addService(const string& computerName, 
                                const string& serviceName,
                                const string& context,
				const string& tradingTag,
				const string& info){
  arLaunchInfo l(computerName, serviceName, context, _location + "/" + tradingTag, info);

  // Trade with _location, not _vircomp, so multiple virtual computers
  // in the same location can share stuff.
  _serviceList.push_back(l);
  // TODO: Dump arLaunchInfo entry here.
}

bool arAppLauncher::_trylock(){
  if (!_szgClientOK() || !_vircompDefined)
    return false;

  int ownerID;
  if (!_szgClient->getLock(_location + "/SZG_DEMO/lock", ownerID)){
    const string label = _szgClient->getProcessLabel(ownerID);
    ar_log_critical() << _exeName << ": demo lock held for virtual computer " <<
      _vircomp << ".  Application " << label << " is reorganizing the system.\n";
    return false;
  }
  return true;
}

// todo: more cheating delays
void arAppLauncher::_unlock(){
  if (!_szgClientOK() || !_vircompDefined)
    return;
  if (!_szgClient->releaseLock(_location + string("/SZG_DEMO/lock"))){
    ar_log_warning() << _exeName << " failed to release SZG_DEMO lock.\n";
  }
}

bool arAppLauncher::_execList(list<arLaunchInfo>* appsToLaunch){
  int match = -1;
  list<int> sentMessageMatches;
  list<int> initialMessageMatches;

  // When launching an app, add the ID of the launching message to 
  // the sentMessageIDs list. It remains there until the launched component
  // has fully responded to the message.
  for (iLaunch iter = appsToLaunch->begin(); iter != appsToLaunch->end(); ++iter){
    const int szgdID = _szgClient->getProcessID(iter->computer, "szgd");
    if (szgdID == -1){
      ar_log_warning() << _exeName << ": no szgd on host " << iter->computer << ".\n";
      continue;
    }
    match = _szgClient->sendMessage("exec", iter->process, iter->context, szgdID, true);
    if (match < 0){
      ar_log_warning() << _exeName << " failed to send exec message.\n";
      continue;
    }
    sentMessageMatches.push_back(match);
    initialMessageMatches.push_back(match);
  }

  // Return responses to the "dex" command, or print them to the console.
  const ar_timeval startTime = ar_time();
  while (!sentMessageMatches.empty()){
    const int elapsedMicroseconds = int(ar_difftime(ar_time(), startTime));
    // We assume that, in a well-behaved Syzygy app, the initial
    // start messages will be received within 20 seconds. After that,
    // let the running application be killed.
    // bug: SZG_DEX_TIMEOUT and SZG_DEX_LOCALTIMEOUT env vars should affect this hardcoded 20 seconds?
    if (elapsedMicroseconds > 20 * 1000000){
      if (!initialMessageMatches.empty()){
        ar_log_warning() << _exeName << " timed out before getting component messages.  Aborting.\n";
        return false;
      }
      if (_szgClient->getLaunchingMessageID()){
	_szgClient->messageResponse(_szgClient->getLaunchingMessageID(),
	  _exeName + string(" remark: ignored launch messages after 20-second timeout.\n",
	  true));
      }
      else{
	ar_log_remark() << _exeName << " ignored launch messages after 20-second timeout.\n";
      }
      return true;
    }
    string responseBody;
    // There are various failure modes to consider:
    //   1. Stuff cannot launch because shared libraries are missing.
    //      In this case, szgd won't get a handshake and will send back
    //      a message. This is covered below. If the Syzygy executable
    //      is properly formed, it will detect that it's launch window
    //      has timed out and will quit.
    //   2. Stuff launches, handshakes with szgd, but never responds to us.
    //      (i.e. the executable is malicious, either intentionally or
    //       unintentionally). OR it does respond, but only with 
    //      "partial responses". In this case, we probably have to cut
    //      things off somewhere, as a defense against programs that
    //      want to drag this out indefinitely.
    const int successCode = _szgClient->getMessageResponse(
      sentMessageMatches, responseBody, match, 10000);
    // successCode==0 exactly when there is a time-out.  
    if (successCode != 0){
      // got a response
      if (_szgClient->getLaunchingMessageID()){
        // We've been launched by "dex", via the message ID outlined.
        // This is a partial response. AND this pile of code indicates
        // why partial responses are a good idea.
        _szgClient->messageResponse(_szgClient->getLaunchingMessageID(),
                                 responseBody, true);
      }
      else{
        // not launched by "dex"
        ar_log_remark() << responseBody << "\n";
      }
      if (successCode > 0){
        // Got the final response for this message.
        sentMessageMatches.remove(match);
        initialMessageMatches.remove(match);
      }
      else if (successCode == -1){
        // Got a continuation of this message.
	initialMessageMatches.remove(match);
      }
      else{
	ar_log_warning() << _exeName << " got an invalid success code.\n";
      }
    }
  }
  return true;
}

// Since this kills via ID, it is better suited to killing the graphics programs.
void arAppLauncher::_blockingKillByID(list<int>* IDList){
  if (!_szgClientOK() || !_vircompDefined)
    return;

  list<int> kill = *IDList; // local copy

  // Since remote components need not respond to the kill message, ask szgserver.
  // Ugly. Maybe a data structure with the request*Notifications?
  list<int> tags;
  map<int, int, less<int> > tagToID;
  for (list<int>::iterator iter = kill.begin(); iter != kill.end(); ++iter){
    const int tag = _szgClient->requestKillNotification(*iter);
    tags.push_back(tag);
    tagToID.insert(map<int,int,less<int> >::value_type(tag, *iter));
    _szgClient->sendMessage("quit", "", *iter);
  }

  // Wait 8 seconds (per component?!) for everything to die.
  while (!tags.empty()){
    const int killedID = _szgClient->getKillNotification(tags, 8000);
    if (killedID < 0){
      ar_log_remark() << _exeName << " timed out while killing components with IDs:\n";
      for (list<int>::iterator n = tags.begin(); n != tags.end(); ++n){
	// These components are at least unhealthy, if not already dead.
	// So just remove them from szgserver's process table.
	map<int,int,less<int> >::const_iterator iter = tagToID.find(*n);
        if (iter == tagToID.end()){
	  ar_log_critical() << _exeName << " internal error: missing kill ID.\n";
	}
	else{
	  ar_log_remark() << iter->second << " ";
          _szgClient->killProcessID(iter->second);
	}
      }
      ar_log_remark() << "\nThese components have been removed from the process table.\n";
      return;
    }
    map<int,int,less<int> >::const_iterator iter = tagToID.find(killedID);
    if (iter == tagToID.end()){
      ar_log_critical() << _exeName << " internal error: missing kill ID.\n";
    }
    else{
      ar_log_remark() << _exeName << " killed component with ID " << iter->second << ".\n";
    }
    tags.remove(killedID);
  }
}

// Kill every render program that does NOT match the specified name. Block.
void arAppLauncher::_graphicsKill(const string& appKeep){
  // Determine what graphics program is running on each display.
  // If it isn't correct, deactivate it. 
  list<int> graphicsKillList;
  for (int i=0; i<getNumberScreens(); i++){
    const string screenLock(getScreenName(i));
    int pid = -1;
    if (_szgClient->getLock(screenLock, pid)){
      // Unlock, so the graphics program can start.
      _szgClient->releaseLock(screenLock); // Ugly. arSZGClient::checklock() better?
    }
    else{
      // someone has locked this screen already
      const arSlashString label(_szgClient->getProcessLabel(pid));
      if (label != "NULL" && label[1] != appKeep){
	graphicsKillList.push_front(pid);
      }
    }
  }
  _blockingKillByID(&graphicsKillList);
}

// Kill the demo. Block. Return false on error.
bool arAppLauncher::_demoKill(){
  int demoID = -1;
  const string demoLockName(_location+string("/SZG_DEMO/app"));
  if (_szgClient->getLock(demoLockName, demoID))
    return true;

  // A demo is running.
  _szgClient->sendMessage("quit", "scratch", demoID);

  // Block until the lock is released (not at all, if it wasn't held).
  const list<int> tags(1, _szgClient->requestLockReleaseNotification(demoLockName));

  // Assume that apps yield gracefully from the system within 15 seconds.
  // Assume that kill works within 8 seconds.
  if (_szgClient->getLockReleaseNotification(tags, 15000) < 0){
    // Timed out. Kill the demo by removing it from szgserver's component table.
    _szgClient->killProcessID(demoID);
    ar_log_warning() << _exeName << " killing slowly exiting demo.\n";
    // Fail. The next demo launch will succeed.
    return false;
  }

  if (!_szgClient->getLock(demoLockName, demoID)){
    ar_log_critical() << _exeName << " failed twice to get demo lock.\n"
		      << "\t(Look for rogue processes on the cluster.)\n";
    return false;
  }

  return true;
}

void arAppLauncher::_relaunchAllServices(list<arLaunchInfo>& appsToLaunch,
                                         list<int>& serviceKillList){
  for (iLaunch iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    const int serviceID = _szgClient->getServiceComponentID(iter->tradingTag);
    if (serviceID >= 0){
      serviceKillList.push_back(serviceID);
    }
    appsToLaunch.push_back(*iter);
  }
}

void arAppLauncher::_relaunchIncompatibleServices(
  list<arLaunchInfo>& appsToLaunch, list<int>& serviceKillList){

  for (iLaunch iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    const int serviceID = _szgClient->getServiceComponentID(iter->tradingTag);
    if (serviceID == -1){
      // No component offers this service.
      ar_log_remark() << _exeName << " starting service " << iter->tradingTag <<
        " on host " << iter->computer << ".\n";
      appsToLaunch.push_back(*iter);
      continue;
    }

    // Found the service.  Is it running on the right computer, with the right info tag?
    const arSlashString processLocation(_szgClient->getProcessLabel(serviceID));
    if (processLocation.size() != 2){
      ar_log_remark() << _exeName << " relaunching disappeared service "
	<< iter->tradingTag << ".\n";
      appsToLaunch.push_back(*iter);
      continue;
    }

    // The process location must be computer/name.
    // Check only the service's host, not its name.
    if (processLocation[0] == iter->computer){
      // Service is running with the right process name and on the right host.
      const string info(_szgClient->getServiceInfo(iter->tradingTag));
      if (info != iter->info){
	// Perhaps a DeviceServer running the wrong driver.
	ar_log_remark() << _exeName << " restarting service " << iter->tradingTag <<
	  " on host " << iter->computer << ", because of mismatched info '" <<
	  iter->info << "'.\n";
	serviceKillList.push_back(serviceID);
	appsToLaunch.push_back(*iter);
      }
      else{
	// Service is already running.
	ar_log_debug() << _exeName << " keeping service " << iter->tradingTag << ".\n";
      }
    }
    else{
      // Service is running, but on the wrong host.
      ar_log_remark() << _exeName << " moving service " << iter->tradingTag
	<< " from host " << iter->computer << " to " << processLocation[0] << ".\n";
      serviceKillList.push_back(serviceID);
      appsToLaunch.push_back(*iter);
    }
  }
}

string arAppLauncher::_screenName(int i) const {
  char buf[20];
  sprintf(buf, "SZG_DISPLAY%d", i);
  return string(buf);
}

string arAppLauncher::_getRenderContext(const int i) const {
  return !_iValid(i) ? "NULL" :
    _szgClient->createContext(_vircomp, "graphics", _pipes[i].screenname, "graphics",
      _getAttribute(_screenName(i), "networks", ""));
}

string arAppLauncher::_getInputContext() const {
  return _szgClient->createContext(_vircomp, "default", "component", "input",
    _getAttribute("SZG_INPUT0", "networks", ""));
}

string arAppLauncher::_getSoundContext() const {
  return _szgClient->createContext(_vircomp, "default", "component", "sound",
    _getAttribute("SZG_SOUND", "networks", ""));
}
