//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include "arLogStream.h"

arAppLauncher::arAppLauncher(const char* exeName) :
  _client(NULL),
  _clientIsMine(false),
  _vircompDefined(false),
  _numberPipes(0),
  _pipeComp(NULL),
  _pipeScreen(NULL),
  _renderLaunchInfo(NULL),
  _appType("NULL"),
  _renderProgram("NULL"),
  _vircomp("NULL"),
  _location("NULL"),
  _exeName("NULL"),
  _onlyIncompatibleServices(true)
{
  if (exeName && *exeName)
    _exeName.assign(exeName);
  else
    _exeName.assign("arAppLauncher");
}

arAppLauncher::~arAppLauncher(){
  if (_pipeComp)
    delete [] _pipeComp;
  if (_clientIsMine)
    delete _client;
}

bool arAppLauncher::setRenderProgram(const string& name){
  _renderProgram = name;
  ar_log_debug() << _exeName << ": renderer is '" << _renderProgram << "'.\n";
  return true;
}

bool arAppLauncher::setAppType(const string& theType){
  if (theType != "distgraphics" && theType != "distapp"){
    ar_log_error() << _exeName << " warning: ignoring unexpected type '"
	           << theType << "', expected 'distapp' or 'distgraphics'.\n";
    return false;
  }
  _appType = theType;
  return true;
}

// Set the virtal computer and the location, based on the default information
// held by the arSZGClient (i.e. as provided by the CONTEXT).
bool arAppLauncher::setVircomp(){
  if (!_client) {
    return false;
  }
  const string vc = _client->getVirtualComputer();
  ar_log_debug() << "arAppLauncher: virtual computer is '" << vc << "'.\n";
  return setVircomp(vc);
}

// Set the virtual computer (e.g. for killalldemo), without starting a whole app.
bool arAppLauncher::setVircomp(string vircomp){
  if (!_client){
    // invalid virtual computer
    return false;
  }
  if (_client->getAttribute(vircomp,"SZG_CONF","virtual","") != "true"){
    // Not a virtual computer name.
    return false;
  }
  _vircomp = vircomp;
  // Two locks have their names determined by the virtual computer.
  // 1. The set-up lock ensures that only one system-wide reorganziation
  //    occurs at a particular time.
  // 2. The demo lock lets us know what program is currently running so that
  //    it can be killed.
  const string locationCandidate =
    _client->getAttribute(vircomp, "SZG_CONF", "location", "");
  _location = (locationCandidate == "NULL") ? _vircomp : locationCandidate;
  return true;
}

// Encapsulate arSZGClient connection for tiny arAppLauncher-wrapper apps.
bool arAppLauncher::connectSZG(int& argc, char** argv){
  _client = new arSZGClient;
  const bool fInit = _client->init(argc, argv);
  if (!*_client)
    return _client->failStandalone(fInit);

  _clientIsMine = true;
  return true;
}

// Often, the program holding the arAppLauncher object will want to have
// its own arSZGClient. Of course, the arAppLauncher needs access to that
// object as well, which access is provided by this method. Must be
// called before any other method.
bool arAppLauncher::setSZGClient(arSZGClient* SZGClient){
  if (!SZGClient) {
    ar_log_warning() << _exeName << " warning: ignoring setSZGClient(NULL).\n";
    return false;
  }
  _client = SZGClient;
  _clientIsMine = false;
  return true;
}

// Queries the szgserver's database to find out the characteristics of the
// virtual computer and parses them, performing some elementary error 
// checking. PLEASE NOTE: pretty much everybody calls this who needs to
// the characteristics of the virtaul computer (like killaldemo).
// Consequently, we need to take into account the fact that the virtual
// computer might have already been set explicitly. 
bool arAppLauncher::setParameters(){
  if (!_client){
    ar_log_error() << _exeName << " error: no arSZGClient.\n";
    return false;
  }

  if (_vircompDefined){
    // Done already.
    return true;
  }
  
  // If the virtual computer was not explicitly set, set its name and "location."
  if (_vircomp == "NULL" && !setVircomp()){
    ar_log_warning() << _exeName << ": no virtual computer '" << _vircomp << "'.\n";
    const string s(_client->getVirtualComputers());
    if (s.empty())
      ar_log_warning() << "  (No virtual computers are defined.  Have you run dbatch?)\n";
    else
      ar_log_warning() << "  (Known virtual computers are: " << s << ".)\n";
    return false;
  }

  // Does this virtual computer think all services should be relaunched always?
  _onlyIncompatibleServices = 
    _client->getAttribute(_vircomp, "SZG_CONF", "relaunch_all", "") != "true";
 
  // todo: stick to either pipe or screen or host, don't use both terms.  Probably host.
  const int numberPipes = getNumberScreens();
  if (numberPipes <= 0){
    ar_log_error() << _exeName << ": negative number of screens for virtual computer '" << _vircomp << "'.\n";
    return false;
  }
  _setNumberPipes(numberPipes);
 
  int i = 0;
  for (i=0; i< numberPipes; ++i){
    if (!_setPipeName(i)){
      // function already sent error message
      return false;
    }
    _renderLaunchInfo[i].computer = _pipeComp[i];
    _renderLaunchInfo[i].process = _renderProgram;
    _renderLaunchInfo[i].context = _getRenderContext(i);
  }

  // Input.
  const arSlashString inputDevs(_getAttribute("SZG_INPUT0", "map", ""));
  const int numTokens = inputDevs.size();
  if (numTokens%2){
    ar_log_error() << _exeName
                   << ": invalid input devices format for virtual computer '" << _vircomp
	           << "'.  Expected computer/inputDevice/.../computer/inputDevice.\n";
    return false;
  }
  ar_log_debug() << _exeName << " found virtual computer '" << _vircomp << "'.\n";

  // Ensure no computer is duplicated, lest we launch two DeviceServers on one.
  for (i=2; i<numTokens; i+=2) {
    const string& computer_i(inputDevs[i]);
    for (int j=0; j<i; j+=2) {
      if (computer_i == inputDevs[j]) {
	ar_log_error() << _exeName << ": input devices for virtual computer '"
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

    // NOTE: the input device in slot 0 is the one that actually
    // connects to the application. Consequently, it must come FIRST
    // in the <virtual computer>/SZG_INPUT0/map listing.
    // The next entry in the listing is the first slave device.
    char buffer[32];
    sprintf(buffer,"%i",i/2);
    
    device = (device == "inputsimulator" ? "" : "DeviceServer ") +
      device + " " + string(buffer);
    if (i < numTokens-2)
      device += " -netinput";
    _addService(computer, device, _getInputContext(),
                "SZG_INPUT"+string(buffer), info);
  }

  // Sound.
  const string soundLocation(_getAttribute("SZG_SOUND","map",""));
  if (soundLocation != "NULL"){
    _addService(soundLocation,"SoundRender",_getSoundContext(), "SZG_WAVEFORM", "");
  }
  _vircompDefined = true;
  return true;
}  

// Launch the other components of the app on the virtual computer.
bool arAppLauncher::launchApp(){
  // _prepareCommand() includes the lock to match the _unlock()'s below.
  if (!_prepareCommand()){
    ar_log_error() << _exeName << " error: failed to prepare launch command.\n";
    return false;
  }

  if (_appType == "NULL"){
    ar_log_error() << _exeName << " error: undefined control host or application type.\n";
    _unlock();
    return false;
  }

  // see if there is a demo currently running, if so, kill it!
  if (!_demoKill()){
    ar_log_error() << _exeName << " error: failed to kill previous application.\n";
    _unlock();
    return false;
  }
     
  // Kill every graphics program whose
  // name differs from the new graphics program.  _demoKill above already
  // killed all the render programs for a master/slave app.
  _graphicsKill(_firstToken(_renderProgram));

  // After the demo program and the graphics programs are
  // killed, no services are provided and no locks are held.

  // Ensure szgd's are running on all render nodes.
  int* renderSzgdID = new int[_numberPipes]; // memory leak
  int i = 0;
  for (i=0; i<_numberPipes; i++){
    renderSzgdID[i] = _client->getProcessID(_pipeComp[i], "szgd");
    if (renderSzgdID[i] == -1){
      ar_log_error() << _exeName << " error: no szgd on render node "
                     << i+1 << " of " << _numberPipes << ".\n";
      _unlock();
      return false;
    }
  }

  list<arLaunchInfo> appsToLaunch;
  // Things to be killed (like incompatible services) before launching.
  list<int> serviceKillList;

  // The virtual computer sets _onlyIncompatibleServices.
  if (_onlyIncompatibleServices){
    _relaunchIncompatibleServices(appsToLaunch, serviceKillList);
  }
  else{
    _relaunchAllServices(appsToLaunch, serviceKillList);
  }

  for (i=0; i<_numberPipes; i++){
    if (_appType == "distapp" || _client->getProcessID(
	  _pipeComp[i], _firstToken(_renderProgram)) == -1){
      // Launch a "distributed app" on every host.
      appsToLaunch.push_back(_renderLaunchInfo[i]);
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

// Kills "transient" components of the application.
void arAppLauncher::killApp(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return;
  }
  
  // In the case of a master/slave application, we want to kill all the
  // copies running on the pipes. In the case of a distributed scene
  // graph application, we do nothing.
  if (_appType == "distapp"){
    _graphicsKill("");
  }
}

bool arAppLauncher::waitForKill(){ 
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return false;
  }

  while (true) {
    string messageType, messageBody;
    if (!_client->receiveMessage(&messageType,&messageBody)){
      ar_log_remark() << _exeName << " remark: stopping because szgserver stopped.\n";
      // Don't bother calling killApp(), since szgserver is gone.
      break;
    }
    if (messageType == "quit"){
      killApp();
      break;
    } else {
      const string lockName(getMasterName());
      int componentID;
      if (_client->getLock(lockName, componentID)) {
        // nobody was holding the lock
        _client->releaseLock(lockName);
        ar_log_error() << _exeName << " error: no component running on master screen.\n";
      } else {
        // something is indeed running on the master screen
        _client->sendMessage(messageType, messageBody, componentID);
      }
    }
  }
  return true;
}

bool arAppLauncher::restartServices(){
  if (!_prepareCommand())
    return false;

  // Kill the services.
  list<arLaunchInfo>::iterator iter;
  list<int> killList;
  // NOTE: we kill the services by their TRADING KEY and NOT by name!
  for (iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    const int serviceID = _client->getServiceComponentID(iter->tradingTag);
    if (serviceID != -1){
      killList.push_front(serviceID);
    }
  }
  _blockingKillByID(&killList);

  // Restart the services.
  list<arLaunchInfo> launchList;
  for (iter = _serviceList.begin(); iter != _serviceList.end(); ++iter){
    int serviceSzgdID = _client->getProcessID(iter->computer, "szgd");
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
  for (list<arLaunchInfo>::iterator iter = _serviceList.begin();
       iter != _serviceList.end();
       ++iter){
    namesKill += " " + iter->tradingTag;
    const int serviceID = _client->getServiceComponentID(iter->tradingTag);
    if (serviceID != -1){
      killList.push_back(serviceID);
    }
  }
  if (!killList.empty()) {
    ar_log_remark() << _exeName << " remark: killing service(s)" << namesKill << ".\n";
  }
  _blockingKillByID(&killList);
  _unlock();
  return true;
}

bool arAppLauncher::screenSaver(){
  if (!_prepareCommand())
    return false;

  _demoKill();

  // on every pipe, if we are not running szgrender, kill the program
  // if this is not done, the launched szgrenders might not be able to
  // get the locks they need 
  _graphicsKill("szgrender");
    
  list<arLaunchInfo> launchList;
  // run through each pipe and start the generic render client, if necessary
  for (int i=0; i<_numberPipes; i++){
    const int renderID = _client->getProcessID(_pipeComp[i], "szgrender");
    const int renderSzgdID = _client->getProcessID(_pipeComp[i], "szgd");
    if (renderID == -1 && renderSzgdID != -1){
      arLaunchInfo temp;
      temp.computer = _pipeComp[i];
      temp.process = "szgrender";
      temp.context = _getRenderContext(i);
      launchList.push_back(temp);
    }
  }
  const bool ok = _execList(&launchList);
  _unlock();
  return ok;
}

bool arAppLauncher::isMaster(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: uninitialized arSZGClient.\n";
    return false;
  }
  const string computerName = _client->getComputerName();
  const string screenName = _client->getMode("graphics");
  const string masterName = getMasterName();
  return computerName + "/" + screenName == masterName;
}

// Return the number of screens in the virtual computer.
int arAppLauncher::getNumberScreens() {
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return -1;
  }
  const int num =
    _client->getAttributeInt(_vircomp, "SZG_DISPLAY", "number_screens", "");
  if (num <= 0){
    ar_log_error() << _exeName << " error: no screens defined for virtual computer "
		   << _vircomp << ".\n";
  }
  return num;
}

// Return the pipe ID for this component's screen, or -1 if none.
int arAppLauncher::getPipeNumber(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return -1;
  }
  int numberPipes = getNumberScreens();
  for (int i=0; i<numberPipes; i++){
    if (_pipeComp[i] == _client->getComputerName()
        && _pipeScreen[i] == _client->getMode("graphics")){
      // we are indeed this pipe.
      return i;
    }
  }
  // failed to find a match
  return -1;
}

// Sometimes, we might want to do something for each pipe, BUT NOT ON
// THE MASTER (which can be any pipe). Consequently, we'll need to know
// the master's pipe ID. Returns -1 on failure.
int arAppLauncher::getMasterPipeNumber(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return -1;
  }
  string masterName = getMasterName();
  int numberPipes = getNumberScreens();
  for (int i=0; i<numberPipes; i++){
    if (_pipeComp[i]+"/"+_pipeScreen[i] == masterName){
      // we are indeed this pipe.
      return i;
    }
  }
  // failed to find a match
  return -1;
}

// Returns the computer/screen pair where the master instance of the
// application is running. CAN ONLY BE CALLED AFTER ONE OF THE INITIALIZATION
// FUNCTIONS (either setVircomp or setParameters).
string arAppLauncher::getMasterName(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return "NULL";
  }
  // SZG_MASTER/map designates the screen.
  string screenName = _client->getAttribute(_vircomp, "SZG_MASTER", "map", "");
  if (screenName == "NULL"){
    return string("NULL");
  }
  return _client->getAttribute(_vircomp, screenName, "map", "");
}

// Returns the name of the computer where the trigger instance of the
// application is running. CAN ONLY BE CALLED AFTER ONE OF THE INITIALIZATION
// FUNCTIONS (either setVircomp or setParameters).
string arAppLauncher::getTriggerName(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return "NULL";
  }
  ar_log_debug() << "arAppLauncher: getting trigger map " << _vircomp << "/"
                 << "SZG_TRIGGER/map.\n";
  return _client->getAttribute(_vircomp, "SZG_TRIGGER", "map", "");
}

// returns the screen name of the nth screen in the current virtual computer
string arAppLauncher::getScreenName(int num){
  if ((num < 0)||(num >= getNumberScreens())) {
    ar_log_warning() << _exeName << " warning: screen " << num << " out of bounds [0,"
                     << getNumberScreens() << " for " << _vircomp << ".\n";
    return string("NULL");
  }
  return _pipeComp[num] + ("/") + _pipeScreen[num];
}

bool arAppLauncher::getRenderProgram(const int num, string& computer,
                                     string& renderName) {
  if ((num < 0)||(num >= getNumberScreens())) {
    ar_log_warning() << _exeName << " warning: screen " << num << " out of bounds [0,"
                     << getNumberScreens() << " for " << _vircomp << ".\n";
    return false;
  }
  computer = _pipeComp[num];
  int renderProgramID = -1;
  const string renderProgramLock(_pipeComp[num] + "/" + _pipeScreen[num]);
  if (!_client->getLock(renderProgramLock, renderProgramID)){
    // someone is, in fact, holding the lock
    const arSlashString renderProgramLabel
      (_client->getProcessLabel(renderProgramID));
    if (renderProgramLabel != "NULL"){
      // something with this ID is still running
      renderName = renderProgramLabel[1];
      // we do not have the lock
      return true;
    }
  }
  // no program was running. we've got the lock.
  _client->releaseLock(renderProgramLock);
  return false;
}

// Allow on-the-fly modification of certain attributes in
// a XML configuration for a display. The first window specified in the
// display is the only one modified... and only top-level elements
// with attributes "value" can be modified (currently decorate, fullscreen,
// title, stereo, zorder, xdisplay).
void arAppLauncher::updateRenderers(const string& attribute, 
                                    const string& value) {
  // set up the virtual computer info, if necessary
  if (!setParameters()){
    ar_log_error() << _exeName << " error: invalid virtual computer definition for "
	           << "updateRenderers.\n";
    return;
  }
  const int numScreens = getNumberScreens();
  string host, program;
  for (int i=0; i<numScreens; i++) {
    // Even if no render program is running, update the database.
    
    // Find where the XML is located.
    // The empty string is an explicit final parameter, to 
    // disambiguate the case where "computer" (1st param) is not specified
    // but "valid values" are.
    string configName(_client->getAttribute(_pipeComp[i], _pipeScreen[i], "name", ""));
    if (configName == "NULL"){
      ar_log_warning() << "arAppLauncher warning: no XML configuration specified for "
	               << _pipeComp[i] << "/" << _pipeScreen[i] << ".\n";
    }
    else{
      // Go into the first window defined in the XML global
      // parameter given by configName, and set the specified attribute.
      configName += "/szg_window/" + attribute + "/value";
      _client->getSetGlobalXML(configName, value);
    }
    if (getRenderProgram(i, host, program)) {
      // Keep going if errors occur.
      (void)_client->sendReload(host, program);
    }
  } 
}

//**************************************************************************
// private functions
//**************************************************************************

// Seperate executable names from parameter lists.
string arAppLauncher::_firstToken(const string& s){
  const int first  = s.find_first_not_of(" ");
  const int second = s.find_first_of(" ", first);
  return s.substr(first, second-first);
}

bool arAppLauncher::_setAttribute(const string& group,
  const string& param, const string& value) {
  bool ok = _client->setAttribute(_vircomp, group, param, value);
  if (!ok)
    ar_log_warning() << _exeName << " warning: failed to set attribute "
                     << _vircomp << "/" << group << "/" << param << " to \""
	             << value << "\".\n";
  return ok;
}

bool arAppLauncher::_prepareCommand(){
  // Get the application lock.
  return _client && setParameters() && _trylock();
}

void arAppLauncher::_setNumberPipes(int numPipes){
  string* temp1 = new string[numPipes];
  string* temp2 = new string[numPipes];
  if (_numberPipes > numPipes){
    _numberPipes = numPipes;
  }
  for (int i=0; i<_numberPipes; i++){
    temp1[i] = _pipeComp[i];
    temp2[i] = _pipeScreen[i];
  }
  delete [] _pipeComp;
  delete [] _pipeScreen;
  _pipeComp = temp1;
  _pipeScreen = temp2;
  _numberPipes = numPipes;
  if (_renderLaunchInfo){
    delete [] _renderLaunchInfo;
  }
  _renderLaunchInfo = new arLaunchInfo[_numberPipes];
}

bool arAppLauncher::_setPipeName(int i){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return false;
  }
  const arSlashString pipe(_getAttribute(_screenName(i), "map", ""));
  if (pipe.size() != 2 || pipe[1].substr(0,11) != "SZG_DISPLAY"){
    ar_log_error() << _exeName << " error: screen " << i << " of "
                   << _vircomp << " does not map to a (computer, display(n)) pair.\n";
    return false;
  }
  _pipeComp[i] = pipe[0];
  _pipeScreen[i] = pipe[1];
  return true;
}

string arAppLauncher::_screenName(int i) {
  char buf[20];
  sprintf(buf, "SZG_DISPLAY%d", i);
  return string(buf);
}

void arAppLauncher::_addService(const string& computerName, 
                                const string& serviceName,
                                const string& context,
				const string& tradingTag,
				const string& info){
  arLaunchInfo temp;
  temp.computer = computerName;
  temp.process = serviceName;
  temp.context = context;

  // Trade with _location, not _vircomp, so multiple virtual computers
  // in the same location can share stuff.
  temp.tradingTag = _location + "/" + tradingTag;
  temp.info = info;
  _serviceList.push_back(temp);
  // TODO: Dump arLaunchInfo entry here.
}

bool arAppLauncher::_trylock(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return false;
  }
  int ownerID;
  if (!_client->getLock(_location + "/SZG_DEMO/lock", ownerID)){
    const string label = _client->getProcessLabel(ownerID);
    ar_log_critical() << _exeName << " warning: demo lock held for virtual computer "
	              << _vircomp
	              << ".  Application " << label << " is reorganizing the system.\n";
    return false;
  }
  return true;
}

// todo: more cheating delays
void arAppLauncher::_unlock(){
  if (!_client){
    ar_log_warning() << _exeName << " warning: no arSZGClient.\n";
    return;
  }
  if (!_client->releaseLock(_location+string("/SZG_DEMO/lock"))){
    ar_log_error() << _exeName << " warning: failed to release lock.\n";
  }
}

bool arAppLauncher::_execList(list<arLaunchInfo>* appsToLaunch){
  list<arLaunchInfo>::iterator iter;
  list<int> sentMessageMatches;
  list<int> initialMessageMatches;
  int match = -1;

  // When launching an app, add the ID of the launching message to 
  // the sentMessageIDs list. It remains there until the launched component
  // has fully responded to the message.
  for (iter = appsToLaunch->begin(); iter != appsToLaunch->end(); ++iter){
    const int szgdID = _client->getProcessID(iter->computer, "szgd");
    if (szgdID == -1){
      ar_log_warning() << _exeName << ": no szgd on host " << iter->computer << ".\n";
      continue;
    }
    match = _client->sendMessage("exec", iter->process, iter->context, szgdID, true);
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
        ar_log_error() << _exeName << " timed out before getting component messages.  Aborting.\n";
        return false;
      }
      if (_client->getLaunchingMessageID()){
	_client->messageResponse(_client->getLaunchingMessageID(),
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
    const int successCode = _client->getMessageResponse(
      sentMessageMatches, responseBody, match, 10000);
    // successCode==0 exactly when there is a time-out.  
    if (successCode != 0){
      // got a response
      if (_client->getLaunchingMessageID()){
        // We've been launched by "dex", via the message ID outlined.
        // This is a partial response. AND this pile of code indicates
        // why partial responses are a good idea.
        _client->messageResponse(_client->getLaunchingMessageID(),
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
if (!_client){
    ar_log_warning() << _exeName << ": no arSZGClient.\n";
    return;
  }

  list<int> ::iterator iter;
  // copy into local storage
  list<int> kill = *IDList;

  // Since remote components DO NOT need to respond to the kill message, ask the szgserver.
  list<int> tags;
  // This is ugly. Maybe a data structure with the request*Notifications?
  map<int, int, less<int> > tagToID;
  for (iter = kill.begin(); iter != kill.end(); ++iter){
    int tag = _client->requestKillNotification(*iter);
    tags.push_back(tag);
    tagToID.insert(map<int,int,less<int> >::value_type(tag, *iter));
  }

  // Send kill signals.
  for (iter = kill.begin(); iter != kill.end(); ++iter){
    _client->sendMessage("quit", "scratch", *iter);
  }

  // Wait for everything to die (up to a suitable 8-second time-out).
  map<int,int,less<int> >::iterator trans;
  while (!tags.empty()){
    const int killedID = _client->getKillNotification(tags, 8000);
    if (killedID < 0){
      ar_log_remark() << _exeName << " timed out while killing components with IDs:\n";
      for (list<int>::iterator n = tags.begin(); n != tags.end(); ++n){
	// Just remove stuff from the process table, so we can move on.
        trans = tagToID.find(*n);
        if (trans == tagToID.end()){
	  ar_log_critical() << _exeName << " internal error: missing kill ID.\n";
	}
	else{
	  ar_log_remark() << trans->second << " ";
          _client->killProcessID(trans->second);
	}
      }
      ar_log_remark() << "\nThese components have been removed from the process table.\n";
      return;
    }
    else{
      trans = tagToID.find(killedID);
      if (trans == tagToID.end()){
	ar_log_critical() << _exeName << " internal error: missing kill ID.\n";
      }
      else{
        ar_log_remark() << _exeName << " killed component with ID " 
	  << trans->second << ".\n";
      }
    }
    // Didn't time out.
    tags.remove(killedID);
  }
}

// Kill every render program that does NOT match the specified name. Block.
void arAppLauncher::_graphicsKill(string match){
  // Determine what graphics program is running on each display.
  // If it isn't correct, deactivate it. 
  list<int> graphicsKillList;
  for (int i=0; i<_numberPipes; i++){
    const string screenLock(_pipeComp[i]+string("/")+_pipeScreen[i]);
    int graphicsProgramID = -1;
    // Ugly.
    if (!_client->getLock(screenLock, graphicsProgramID)){
      // someone has locked this screen already
      const arSlashString graphicsProgramLabel(
        _client->getProcessLabel(graphicsProgramID));
      if (graphicsProgramLabel != "NULL"){
        // szgserver thinks there is a program with this ID
        if (graphicsProgramLabel[1] != match){
          graphicsKillList.push_front(graphicsProgramID);
	}
      }
    }
    else{
      // Unlock, so the graphics program can start.
      // Ugly.  arSZGClient::checklock() is better?
      _client->releaseLock(screenLock);
    }
  }
  _blockingKillByID(&graphicsKillList);
}

// Kill the demo. Block. Return false on error.
bool arAppLauncher::_demoKill(){
  int demoID = -1;
  const string demoLockName(_location+string("/SZG_DEMO/app"));
  if (_client->getLock(demoLockName, demoID))
    return true;

  // A demo is running.
  _client->sendMessage("quit", "scratch", demoID);
  // Block until the lock is released (not at all, if it wasn't held).
  const int match = _client->requestLockReleaseNotification(demoLockName);
  list<int> tags;
  tags.push_back(match);
  // Assume that apps yield gracefully from the system within 15 seconds.
  // Assume that kill works within 8 seconds.
  if (_client->getLockReleaseNotification(tags, 15000) < 0){
    // Timed out. Kill the demo by removing it from szgserver's component table.
    _client->killProcessID(demoID);
    ar_log_warning() << _exeName << " killing slowly exiting demo.\n";
    // Fail. The next demo launch will succeed.
    return false;
  }

  if (!_client->getLock(demoLockName, demoID)){
    ar_log_critical() << _exeName << " failed twice to get demo lock.\n"
		      << "\t(Look for rogue processes on the cluster.)\n";
    return false;
  }

  return true;
}

void arAppLauncher::_relaunchAllServices(list<arLaunchInfo>& appsToLaunch,
                                         list<int>& serviceKillList){
  for (list<arLaunchInfo>::iterator iter = _serviceList.begin();
       iter != _serviceList.end();
       ++iter){
    const int serviceID = _client->getServiceComponentID(iter->tradingTag);
    if (serviceID >= 0){
      serviceKillList.push_back(serviceID);
    }
    appsToLaunch.push_back(*iter);
  }
}

void arAppLauncher::_relaunchIncompatibleServices(
  list<arLaunchInfo>& appsToLaunch, list<int>& serviceKillList){

  for (list<arLaunchInfo>::iterator iter = _serviceList.begin();
       iter != _serviceList.end();
       ++iter){
    const int serviceID = _client->getServiceComponentID(iter->tradingTag);
    if (serviceID == -1){
      // No component offers this service.
      ar_log_remark() << _exeName << " starting service " << iter->tradingTag << ".\n";
      appsToLaunch.push_back(*iter);
      continue;
    }

    // Found the service.  Is it running on the right computer, with the right info tag?
    const arSlashString processLocation(_client->getProcessLabel(serviceID));
    if (processLocation.size() != 2){
      ar_log_remark() << _exeName << " remark: relaunching disappeared service "
	<< iter->tradingTag << ".\n";
      appsToLaunch.push_back(*iter);
      continue;
    }

    // The process location must be computer/name.
    // Check only the service's host, not its name.
    if (processLocation[0] == iter->computer){
      // Service is running with the right process name and on the right host.
      const string info(_client->getServiceInfo(iter->tradingTag));
      if (info != iter->info){
	// Perhaps a DeviceServer running the wrong driver.
	ar_log_remark() << _exeName << " restarting service "
			<< iter->tradingTag
			<< ", because of mismatched info '" << iter->info << "'.\n";
	serviceKillList.push_back(serviceID);
	appsToLaunch.push_back(*iter);
      }
      else{
	// Service is already running.
	ar_log_debug() << _exeName << " found service " << iter->tradingTag << ".\n";
      }
    }
    else{
      // Service is running, but on the wrong host.
      ar_log_remark() << _exeName << " restarting service "
	<< iter->tradingTag << " on host '" << processLocation[0]
	<< "' instead of '" << iter->computer << "'.\n";
      serviceKillList.push_back(serviceID);
      appsToLaunch.push_back(*iter);
    }
  }
}

string arAppLauncher::_getRenderContext(int i){
  if (i<0 || i>=_numberPipes)
    return string("NULL");

  char buffer[60];
  sprintf(buffer, "SZG_DISPLAY%d", i);
  return _client->createContext(
    _vircomp, "graphics", _pipeScreen[i], "graphics",
    _client->getAttribute(_vircomp, buffer, "networks", ""));
}

string arAppLauncher::_getInputContext(){
  return _client->createContext(
    _vircomp, "default", "component", "input",
    _client->getAttribute(_vircomp, "SZG_INPUT0", "networks", ""));
}

string arAppLauncher::_getSoundContext(){
  return _client->createContext(
    _vircomp, "default", "component", "sound",
    _client->getAttribute(_vircomp,"SZG_SOUND","networks",""));
}
