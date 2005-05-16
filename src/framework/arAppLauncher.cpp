//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arAppLauncher.h"
#include <sstream>

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
  cout << _exeName << " remark: render program is " << _renderProgram << ".\n";
  return true;
}

bool arAppLauncher::setAppType(const string& theType){
  if (theType != "distgraphics" && theType != "distapp"){
    cerr << _exeName << " warning: unexpected type \""
	 << theType 
	 << "\".\n\tExpected \"distapp\" or \"distgraphics\".\n";
    return false;
  }
  _appType = theType;
  return true;
}

/// Sets the virtal computer and the location, based on the default information
/// held by the arSZGClient (i.e. as provided by the CONTEXT).
bool arAppLauncher::setVircomp(){
  if (!_client){
    return false;
  }
  string virtualComputer = _client->getVirtualComputer();
  return setVircomp(virtualComputer);
}

/// Allows us to set the virtaul computer explicitly (as desired for 
/// killalldemo, for instance), without starting up a whole application.
bool arAppLauncher::setVircomp(string vircomp){
  if (!_client){
    // invalid virtual computer
    return false;
  }
  if (_client->getAttribute(vircomp,"SZG_CONF","virtual","") != "true"){
    // user hasn't declared this is a valid virtual computer name.
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
  _location =
    (locationCandidate == "NULL") ? _vircomp : locationCandidate;
  return true;
}

/// Some programs (like the screensaver activator) are very short
/// wrappers around arAppLauncher methods. This is with those, to
/// encapsulate the arSZGClient connection process.
bool arAppLauncher::connectSZG(int& argc, char** argv){
  _client = new arSZGClient;
  _client->init(argc, argv);
  if (!*_client)
    return false;
  _clientIsMine = true;
  return true;
}

/// Often, the program holding the arAppLauncher object will want to have
/// its own arSZGClient. Of course, the arAppLauncher needs access to that
/// object as well, which access is provided by this method. Must be
/// called before any other method.
bool arAppLauncher::setSZGClient(arSZGClient* SZGClient){
  if (!SZGClient) {
    cerr << _exeName << " warning: ignoring setSZGClient(NULL).\n";
    return false;
  }
  _client = SZGClient;
  _clientIsMine = false;
  return true;
}

/// Queries the szgserver's database to find out the characteristics of the
/// virtual computer and parses them, performing some elementary error 
/// checking. PLEASE NOTE: pretty much everybody calls this who needs to
/// the characteristics of the virtaul computer (like killaldemo).
/// Consequently, we need to take into account the fact that the virtual
/// computer might have already been set explicitly. 
bool arAppLauncher::setParameters(){
  if (!_client){
    cerr << _exeName << " error: no arSZGClient.\n";
    return false;
  }

  if (_vircompDefined){
    // no need to go through this again.
    return true;
  }
  
  // Make sure we forward all error messages.
  stringstream& initResponse = _client->initResponse();

  // Set the virtual computer name (and also the "location").
  // Only do this if the virtaul computer was not explciitly set!
  if (_vircomp == "NULL" && !setVircomp()){
    initResponse << _exeName << " error: undefined virtual computer.\n";
    return false;
  }
  // Does this virtual computer think all services should be relaunched always?
  if (_client->getAttribute(_vircomp, "SZG_CONF", "relaunch_all", "")
      == "true"){
    _onlyIncompatibleServices = false;
  }
 
  const int numberPipes = getNumberScreens();
  if (numberPipes <= 0){
    initResponse << "arAppLauncher error: invalid number of pipes for "
		 << "virtual computer " << _vircomp << ".\n";
    return false;
  }
  _setNumberPipes(numberPipes);
 
  int i;
  for (i=0; i< numberPipes; ++i){
    if (!_setPipeName(i, initResponse)){
      // function already sent error message
      return false;
    }
    _renderLaunchInfo[i].computer = _pipeComp[i];
    _renderLaunchInfo[i].process = _renderProgram;
    _renderLaunchInfo[i].context = _getRenderContext(i);
  }

  // go ahead and deal with the input stuff
  const arSlashString inputDevs(_getAttribute("SZG_INPUT0", "map", ""));
  const int numTokens = inputDevs.size();
  if (numTokens%2){
    initResponse << _exeName
         << " error: invalid input devices format for " << _vircomp
	 << ".  Expected computer/inputDevice/.../computer/inputDevice.\n";
    return false;
  }
  _serviceList.clear();
  // step through list of computer/device token pairs 
  for (i=0; i<numTokens; i+=2){
    const string computer(inputDevs[i]);
    string device(inputDevs[i+1]);
    string info = device; // NOTE: device will be modified below.

    /// \todo hack, copypasted into demo/buttonfly/setinputfilter.cpp
    //
    // NOTE: the input device in slot 0 is the one that actually
    // connects to the application. Consequently, it must come FIRST
    // in the <virtual computer>/SZG_INPUT0/map listing.
    // The next entry in the listing is the first slave device.
    char buffer[32];
    sprintf(buffer,"%i",i/2);
    
    if (device == "inputsimulator"){
      device = device + " " + string(buffer);
    } else {
      device = "DeviceServer " + device + " " + string(buffer);
    }
    if (i < numTokens-2) {
      device = device + " -netinput";
    }
    _addService(computer, device, _getInputContext(),
                "SZG_INPUT"+string(buffer), info);
  }
  // go ahead and deal with the sound stuff, if configured
  const string soundLocation(_getAttribute("SZG_SOUND","map",""));
  if (soundLocation != "NULL"){
    _addService(soundLocation,"SoundRender",_getSoundContext(),
                "SZG_WAVEFORM", "");
  }
  initResponse << _exeName << " remark: virtual computer definition ok.\n";
  _vircompDefined = true;
  return true;
}  

/// Launches the other components of the application on the virtual
/// computer.
bool arAppLauncher::launchApp(){
  // _prepareCommand() includes the lock to match the _unlock()'s below.
  if (!_prepareCommand()){
    cerr << _exeName << " error: failed to prepare launch command.\n";
    return false;
  }

  if (_appType == "NULL"){
    cerr << _exeName 
         << " error: undefined control host or application type.\n";
    _unlock();
    return false;
  }

  // see if there is a demo currently running, if so, kill it!
  if (!_demoKill()){
    cerr << _exeName 
	 << " error: failed to kill previous application.\n";
    _unlock();
    return false;
  }
     
  // This will kill every graphics program, that is still running, and whose
  // name is different from the new graphics program. NOTE: _demoKill above
  // actually killed all the render programs for a master/slave application
  // already.
  _graphicsKill(_firstToken(_renderProgram));

  // NOTE: once the demo program and the graphics programs have all been
  // killed... no services will be provided nor will any of the locks be
  // held!!!

  // make sure szgd's are running everywhere
  int i;
  int* renderSzgdID = new int[_numberPipes]; // small memory leak
  for (i=0; i<_numberPipes; i++){
    renderSzgdID[i] = _client->getProcessID(_pipeComp[i], "szgd");
    if (renderSzgdID[i] == -1){
      cerr << _exeName << " error: no szgd on render node "
           << i+1 << " of " << _numberPipes << ".\n";
      _unlock();
      return false;
    }
  }

  // This is the list of things we will launch.
  list<arLaunchInfo> appsToLaunch;
  // Some things might need to be killed first (like incompatible services)
  list<int> serviceKillList;

  // There are, indeed, different ways in which an application can be
  // launched. For instance, the underlying "virtual computer" can require
  // that ALL services be killed and then restarted... or it can require
  // only those deemed INCOMPATIBLE be killed and then restarted.
  if (!_onlyIncompatibleServices){
    _relaunchAllServices(appsToLaunch, serviceKillList);
  }
  else{
    _relaunchIncompatibleServices(appsToLaunch, serviceKillList);
  }

  for (i=0; i<_numberPipes; i++){
    // if the is a "distapp" application, then we launch on every pipe
    // (the old notion of allowing an instance of the application to
    // both be the controller and a renderer no longer exists
    if (_appType == "distapp"
        || _client->getProcessID(_pipeComp[i],
                                 _firstToken(_renderProgram)) == -1){
      appsToLaunch.push_back(_renderLaunchInfo[i]);
    }
  }

  // Kill any incompatible services here.
  _blockingKillByID(&serviceKillList);

  const bool ok = _execList(&appsToLaunch);
  if (!ok){
    cerr << _exeName << " error: failed to execute full component list.\n";
  }
  _unlock(); 
  return ok;
}

/// Kills the various "transient" components of the application on the
/// virtual computer.
void arAppLauncher::killApp(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
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
    cerr << _exeName << " warning: no arSZGClient.\n";
    return false;
  }

  while (true) {
    string messageType, messageBody;
    if (!_client->receiveMessage(&messageType,&messageBody)){
      cout << _exeName << " remark: stopping because szgserver stopped.\n";
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
        cerr << _exeName << " error: no component running on master screen.\n";
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

  // kill the graphics
  _graphicsKill("");

  // Kill the services.
  list<int> killList;
  string namesKill;
  for (list<arLaunchInfo>::iterator iter = _serviceList.begin();
       iter != _serviceList.end();
       ++iter){
    namesKill += " " + iter->tradingTag;
    //cout << _exeName << " remark: killing service "
//	 << iter->tradingTag << ".\n";
    const int serviceID =
      _client->getServiceComponentID(iter->tradingTag);
    if (serviceID != -1){
      killList.push_back(serviceID);
    }
  }

  const int i = killList.size();
  if (i > 0) {
    cout << _exeName << " remark: killing service";
    if (i > 1)
      cout << "s";
    cout << namesKill << ".\n";
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
    
  int i;
  list<arLaunchInfo> launchList;
  // run through each pipe and start the generic render client, if necessary
  for (i=0; i<_numberPipes; i++){
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
    cerr << _exeName << " warning: uninitialized arSZGClient.\n";
    return false;
  }
  string computerName = _client->getComputerName();
  string screenName = _client->getMode("graphics");
  string masterName = getMasterName();
  if (computerName+"/"+screenName == masterName){
    return true;
  }
  return false;
}

/// Returns the number of screens in the virtual computer.
int arAppLauncher::getNumberScreens() {
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return -1;
  }
  const int num =
    _client->getAttributeInt(_vircomp, "SZG_SCREEN", "number_screens", "");
  if (num <= 0){
    cerr << _exeName << " warning: no screens defined for virtual computer "
         << _vircomp << ".\n";
  }
  return num;
}

/// Returns the pipe ID for the screen upon which this component runs.
/// Returns -1 if such does not exist.
int arAppLauncher::getPipeNumber(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
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

/// Sometimes, we might want to do something for each pipe, BUT NOT ON
/// THE MASTER (which can be any pipe). Consequently, we'll need to know
/// the master's pipe ID. Returns -1 on failure.
int arAppLauncher::getMasterPipeNumber(){
  if (!_client){
    cout << _exeName << " warning: no arSZGClient.\n";
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

/// Returns the computer/screen pair where the master instance of the
/// application is running. CAN ONLY BE CALLED AFTER ONE OF THE INITIALIZATION
/// FUNCTIONS (either setVircomp or setParameters).
string arAppLauncher::getMasterName(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return "NULL";
  }
  // note that SZG_MASTER/map gives the screen designation
  string screenName = _client->getAttribute(_vircomp, "SZG_MASTER", "map", "");
  if (screenName == "NULL"){
    return string("NULL");
  }
  return _client->getAttribute(_vircomp, screenName, "map", "");
}

/// Returns the name of the computer where the trigger instance of the
/// application is running. CAN ONLY BE CALLED AFTER ONE OF THE INITIALIZATION
/// FUNCTIONS (either setVircomp or setParameters).
string arAppLauncher::getTriggerName(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return "NULL";
  }
  return _client->getAttribute(_vircomp, "SZG_TRIGGER", "map", "");
}

/// returns the screen name of the nth screen in the current virtual computer
string arAppLauncher::getScreenName(int num){
  if ((num < 0)||(num >= getNumberScreens())) {
    cerr << _exeName << " warning: screen " << num << " out of bounds [0,"
         << getNumberScreens() << " for " << _vircomp << ".\n";
    return string("NULL");
  }
  return _pipeComp[num] + ("/") + _pipeScreen[num];
}

bool arAppLauncher::getRenderProgram(const int num, string& computer,
                                     string& renderName) {
  if ((num < 0)||(num >= getNumberScreens())) {
    cerr << _exeName << " warning: screen " << num << " out of bounds [0,"
         << getNumberScreens() << " for " << _vircomp << ".\n";
    return false;
  }
  computer = _pipeComp[num];
  int renderProgramID = -1;
  string renderProgramLock = _pipeComp[num] + ("/") + _pipeScreen[num];
  if (!_client->getLock(renderProgramLock, renderProgramID)){
    // someone is, in fact, holding the lock
    const arSlashString renderProgramLabel
      (_client->getProcessLabel(renderProgramID));
    if (renderProgramLabel != "NULL"){
      // something with this ID is still running
      renderName = renderProgramLabel[1];
      // we do not have the lock... return right away
      return true;
    }
  }
  // no program was running... we've got the lock... better release
  _client->releaseLock(renderProgramLock);
  return false;
}

void arAppLauncher::updateRenderers(const string& attribute, 
                                    const string& value) {
  // set up the virtual computer info, if necessary
  if (!setParameters()){
    cerr << _exeName << " error: invalid virtual computer definition for "
	 << "updateRenderers.\n";
    return;
  }
  const int numScreens = getNumberScreens();
  string host, program;
  for (int i=0; i<numScreens; i++) {
    // even if no render program is running, we want to update the
    // database
    (void)_client->setAttribute(_pipeComp[i], _pipeScreen[i], 
                                attribute, value);
    if (getRenderProgram(i, host, program)) {
      // Keep going if we hit any errors along the way.
      (void)_client->sendReload(host, program);
    }
  } 
}

//**************************************************************************
// The private functions follow
//**************************************************************************

/// Seperate executable names from parameter lists.
string arAppLauncher::_firstToken(const string& s){
  const int first  = s.find_first_not_of(" ");
  const int second = s.find_first_of(" ", first);
  return s.substr(first, second-first);
}

bool arAppLauncher::_setAttribute(const string& group,
  const string& param, const string& value) {
  bool ok = _client->setAttribute(_vircomp, group, param, value);
  if (!ok)
    cerr << _exeName << " warning: failed to set attribute "
         << _vircomp << "/" << group << "/" << param << " to \""
	 << value << "\".\n";
  return ok;
}

bool arAppLauncher::_prepareCommand(){
  // Make sure we have the application lock.
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

bool arAppLauncher::_setPipeName(int i, stringstream& initResponse){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return false;
  }
  const arSlashString pipe(_getAttribute(_screenName(i), "map", ""));
  if (pipe.size() != 2 || pipe[1].substr(0,10) != "SZG_SCREEN"){
    initResponse << _exeName << " warning: screen " << i << " of "
         << _vircomp << " does not map to a (computer, screen(n)) pair.\n";
    return false;
  }
  _pipeComp[i] = pipe[0];
  _pipeScreen[i] = pipe[1];
  return true;
}

string arAppLauncher::_screenName(int i) {
  char buf[20];
  sprintf(buf, "SZG_SCREEN%d", i);
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
  // VERY IMPORTANT THAT THIS USES _location and NOT _vircomp!
  // (remember... we trade against LOCATION instead of _vircomp to allow
  // multiple virtual computers in the same stop to share stuff.
  temp.tradingTag = _location + string("/") + tradingTag;
  temp.info = info;
  _serviceList.push_back(temp);
}

bool arAppLauncher::_trylock(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return false;
  }
  int ownerID;
  if (!_client->getLock(_location+string("/SZG_DEMO/lock"), ownerID)){
    const string label = _client->getProcessLabel(ownerID);
    cerr << _exeName << " warning: demo lock held for virtual computer "
	 << _vircomp
	 << ".  Application " << label << " is reorganizing the system.\n";
    return false;
  }
  return true;
}

/// \todo more cheating delays
void arAppLauncher::_unlock(){
  if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return;
  }
  if (!_client->releaseLock(_location+string("/SZG_DEMO/lock"))){
    cerr << _exeName << " warning: failed to release lock.\n";
  }
}

bool arAppLauncher::_execList(list<arLaunchInfo>* appsToLaunch){
  list<arLaunchInfo>::iterator iter;
  list<int> sentMessageMatches;
  list<int> initialMessageMatches;
  int match;
  // when we launch an app, we put the ID of the launching message into 
  // the sentMessageIDs list. It remains there until the launched component
  // has fully responded to the message
  for (iter = appsToLaunch->begin(); iter != appsToLaunch->end(); ++iter){
    const int szgdID = _client->getProcessID(iter->computer, "szgd");
    if (szgdID == -1){
      cerr << _exeName << " warning: no szgd on node "
	   << iter->computer << ".\n";
      continue;
    }
    match = _client->sendMessage("exec", iter->process, iter->context, 
                                 szgdID, true);
    if (match < 0){
      cout << _exeName << " error: failed to send exec message.\n";
    }
    else{
      sentMessageMatches.push_back(match);
      initialMessageMatches.push_back(match);
    }
  }
  // Relay the responses to the "dex" command or print them to the console.
  ar_timeval startTime = ar_time();
  while (!sentMessageMatches.empty()){
    int elapsedMicroseconds = int(ar_difftime(ar_time(), startTime));
    // We assume that, in a well-behaved Syzygy application, the initial
    // start messages will be received within 20 seconds. After that,
    // we can go ahead and allow the running application to be killed.
    if (elapsedMicroseconds/1000000 > 20){
      if (!initialMessageMatches.empty()){
        cout << _exeName << " error: have not received initial component "
	     << "messages after 20 second timeout. Application will fail.\n";
        return false;
      }
      else{
	if (_client->getLaunchingMessageID()){
          stringstream result;
	  result << _exeName << " remark: ignored launch messages "
	         << "after 20-second timeout.\n";
	  _client->messageResponse(_client->getLaunchingMessageID(),
				   result.str(), true);
	}
	else{
	  cout << _exeName << " remark: ignored launch messages "
	       << "after 20-second timeout.\n";
	}
        return true;
      }
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
    int successCode = _client->getMessageResponse(sentMessageMatches,
                                                  responseBody, 
                                                  match,
                                                  10000);
    if (successCode != 0){
      // successCode will be 0 exactly when there is a time-out.  
      // we have received a response
      if (_client->getLaunchingMessageID()){
        // We've been launched by "dex", via the message ID outlined.
        // NOTE: this is a partial response. AND this does tend to indicate
        // why partial responses are a good idea, the aggregation that occurs
        // here.
        _client->messageResponse(_client->getLaunchingMessageID(),
                                 responseBody, true);
      }
      else{
        // we haven't been launched by "dex"
        cout << responseBody << "\n";
      }
      if (successCode > 0){
        // We got the final response for this message. NOTE: remember that
        // some messages may be CONTINUATIONS, and hence not the final ones.
        sentMessageMatches.remove(match);
        initialMessageMatches.remove(match);
      }
      else if (successCode == -1){
        // This is a message continuation.
	initialMessageMatches.remove(match);
      }
      else{
	cout << _exeName << " error: got an invalid success code.\n";
      }
    }
  }
  return true;
}

/// Since the below kills via ID, it is much better suited to killing the
/// graphics programs.
void arAppLauncher::_blockingKillByID(list<int>* IDList){
if (!_client){
    cerr << _exeName << " warning: no arSZGClient.\n";
    return;
  }

  list<int> ::iterator iter;
  // copy into local storage
  list<int> kill = *IDList;

  // NOTE: the remote components DO NOT need to respond to the kill message.
  // Consequently, we ask the szgserver.
  list<int> tags;
  // Hmmm. It is annoying to have to do the following... Maybe we could
  // do some data structure with the request*Notifications?
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
    int killedID = _client->getKillNotification(tags, 8000);
    if (killedID < 0){
      cout << _exeName << " remark: timeout killing remote processes:\n";
      for (list<int>::iterator n = tags.begin(); n != tags.end(); ++n){
	// Just remove stuff from the process table, so we can move on.
        trans = tagToID.find(*n);
        if (trans == tagToID.end()){
	  // THIS SHOULD NOT HAPPEN
	  cout << _exeName << " error: failed to find kill ID.\n";
	}
	else{
	  // NOTE: THIS IS NOT THE TAG, but instead the process ID.
	  cout << trans->second << " ";
          _client->killProcessID(trans->second);
	}
      }
      cout << "\nThey have been removed from the syzygy process table.\n";
      return;
    }
    else{
      trans = tagToID.find(killedID);
      if (trans == tagToID.end()){
	// THIS SHOULD NOT HAPPEN
	cerr << _exeName << " internal error: missing kill ID.\n";
      }
      else{
	// NOTE: This IS NOT killed ID!
        cout << _exeName << " remark: killed component with ID " 
	     << trans->second << ".\n";
      }
    }
    // Only get here in case of NO timeout!
    tags.remove(killedID);
  }
}

/// kills every render program that does NOT match the specified
/// name. Blocks until this has been accomplished.
void arAppLauncher::_graphicsKill(string match){
  // Determine what graphics program is running on each display;
  // if it isn't correct, deactivate it. 
  list<int> graphicsKillList;
  for (int i=0; i<_numberPipes; i++){
    const string screenLock(_pipeComp[i]+string("/")+_pipeScreen[i]);
    int graphicsProgramID = -1;
    // this is really SUB_IDEAL! 
    if (!_client->getLock(screenLock, graphicsProgramID)){
      // someone is holding the lock for this screen already
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
      // we've now got the lock... must go ahead and release it so the
      // graphics program can start later
      // IT SURE IS OBNOXIOUS TO HAVE TO DO THIS!!!!!!! MAYBE A checkLock
      // method for arSZGClient is in order.
      _client->releaseLock(screenLock);
    }
  }
  _blockingKillByID(&graphicsKillList);
}

// kills the demo... and blocks on that to have occured. Returns false if
// something went wrong in the process.
bool arAppLauncher::_demoKill(){
  int demoID = -1;
  string demoLockName = _location+string("/SZG_DEMO/app");
  if (!_client->getLock(demoLockName,demoID)){
    // a demo is currently running
    _client->sendMessage("quit", "scratch", demoID);
    // block until the lock has been released
    // NOTE: if the lock is not currently held, this will return right away.
    int match = _client->requestLockReleaseNotification(demoLockName);
    list<int> tags;
    tags.push_back(match);
    // Assume that all syzygy apps must yield gracefully from the system
    // within 15 seconds. We assume that kill works within 8 seconds.
    if (_client->getLockReleaseNotification(tags, 15000) < 0){
      // A time-out has occured. Kill the demo via removing it from the
      // szgserver component table.
      _client->killProcessID(demoID);
      cerr << _exeName << " warning: killing slowly exiting demo.\n";
      // Go ahead and fail. The next demo launch will succeed.
      return false;
    }
    if (!_client->getLock(demoLockName, demoID)){
      cerr << _exeName << " error: failed twice to get demo lock.\n"
	   << "\t(Look for rogue processes on the cluster.)\n";
      return false;
    }
    // got the lock
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
                                              list<arLaunchInfo>& appsToLaunch,
                                              list<int>& serviceKillList){
  for (list<arLaunchInfo>::iterator iter = _serviceList.begin();
       iter != _serviceList.end();
       ++iter){
    // Here is the procedure:
    //   1. See if there is a component offering the well-known service.
    //      a. If so, check to see whether it is running on the right 
    //         computer and has the right "info" tag.
    //        i. If so, do nothing.
    //        ii. If not, go ahead and kill and start again.
    //      b. If not, go ahead and start.
    const int serviceID = _client->getServiceComponentID(iter->tradingTag);
    if (serviceID == -1){
      cout << _exeName << " remark: restarting service "
	   << iter->tradingTag << ".\n";
      appsToLaunch.push_back(*iter);
    }
    else{
      // Check whether this is running on the right computer and has the
      // right info tag.
      arSlashString processLocation(_client->getProcessLabel(serviceID));
      if (processLocation.size() == 2){
        // The process location must, in fact, be computer/name.
	// NOTE: Only check here if it is running on the right computer!
	// (do not, for instance, check the process name)
        if (processLocation[0] == iter->computer){
	  // There is something running, offering the service, with the
	  // right process name and on the right computer.
          // HOWEVER... is it the case that we have the right info?
	  // (i.e. maybe it is a DeviceServer that is running the wrong
	  //  driver)
          const string info = _client->getServiceInfo(iter->tradingTag);
	  // If info == iter->info then there is nothing to do. The service
	  // seems to be in the right place and of the right type.
          if (info != iter->info){
            // Must go ahead and kill.
	    cout << _exeName << " remark: relaunching service "
		 << iter->tradingTag
		 << ", because of invalid info " << iter->info << ".\n";
            serviceKillList.push_back(serviceID);
            appsToLaunch.push_back(*iter);
	  }
#if 0
	  else{
            cout << _exeName << " remark: compatible service "
	         << iter->tradingTag << ".\n";
	  }
#endif
	}
	else{
          // There is something running, offering the service, but not on
	  // the right computer.
          cout << _exeName << " remark: relaunching service "
	       << iter->tradingTag << " on correct host.\n";
          serviceKillList.push_back(serviceID);
          appsToLaunch.push_back(*iter);
	}
      }
      else{
        cout << _exeName << " remark: relaunching disappeared service "
	     << iter->tradingTag << ".\n";
        appsToLaunch.push_back(*iter);
      }
    }
  }
}

string arAppLauncher::_getRenderContext(int i){
  if (i<0 || i>=_numberPipes)
    return string("NULL");

  char buffer[32];
  sprintf(buffer,"%i",i);
  const string networks(_client->getAttribute(
    _vircomp,string("SZG_SCREEN")+buffer, "networks",""));
  return _client->createContext(
    _vircomp, "graphics", _pipeScreen[i], "graphics", networks);
}

string arAppLauncher::_getInputContext(){
  return _client->createContext(
    _vircomp, "default", "component", "input",
    _client->getAttribute(_vircomp,"SZG_INPUT0","networks",""));
}

string arAppLauncher::_getSoundContext(){
  return _client->createContext(
    _vircomp, "default", "component", "sound",
    _client->getAttribute(_vircomp,"SZG_SOUND","networks",""));
}
