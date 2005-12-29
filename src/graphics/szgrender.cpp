//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsClient.h"
#include "arFramerateGraph.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arLogStream.h"

arGraphicsClient* graphicsClient = NULL;
arFramerateGraph  framerateGraph;
arGUIWindowManager* windowManager;
arGUIXMLParser*     guiParser;

bool framerateThrottle = false;
bool drawPerformanceDisplay = false;
// Make the szgrender use a few less resources. Set by passing in a "-n"
// flag. 
bool makeNice = false;
bool exitFlag = false;
bool requestReload = false;

// extra services
arConditionVar pauseVar;
arMutex        pauseLock;
bool           pauseFlag = false;
string         dataPath;  // For storing screenshots.

string textPath;

bool loadParameters(arSZGClient& cli){
  // important that we use the parameters for our particular screen
  graphicsClient->configure(&cli);
  dataPath = cli.getAttribute("SZG_DATA", "path"); // For storing screenshots.
  // Must remember to set up the data bundle info.
  if (dataPath != "NULL"){
    graphicsClient->addDataBundlePathMap("SZG_DATA", dataPath);
  }
  // Must also do this for the other bundle path possibility.
  string pythonPath = cli.getAttribute("SZG_PYTHON", "path");
  if (pythonPath != "NULL"){
    graphicsClient->addDataBundlePathMap("SZG_PYTHON", pythonPath);
  }
  return true;
}

void shutdownAction(){
  // Do not do this again while an exit is already pending.
  if (!exitFlag){
    ar_log_remark() << "szgrender remark: shutdown.\n";
    exitFlag = true;
    // This command guarantees we'll get to the end of _cliSync.consume().
    graphicsClient->_cliSync.skipConsumption();
  }
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    // Note how we only hit this ONCE!
    if (!cli->receiveMessage(&messageType,&messageBody)){
      shutdownAction();
      break;
    }
    if (messageType=="performance"){
      if (messageBody=="on"){
        drawPerformanceDisplay = true;
        graphicsClient->drawFrameworkObjects(drawPerformanceDisplay);
      }
      if (messageBody=="off"){
        drawPerformanceDisplay = false;
        graphicsClient->drawFrameworkObjects(drawPerformanceDisplay);
      }
    }
    if (messageType=="quit"){
      // note that we exit within the draw loop to avoid crashes on Win32
      exitFlag = true;
      // make sure we actually get to the end of the draw loop
      graphicsClient->_cliSync.skipConsumption();
    }
    if (messageType=="screenshot"){
      if (dataPath == "NULL"){
	ar_log_warning() << "szgrender warning: empty SZG_DATA/path, screenshot failed.\n";
      }
      else{
        if (messageBody != "NULL"){
          int tempBuffer[4];
          ar_parseIntString(messageBody,tempBuffer,4);
          graphicsClient->requestScreenshot(dataPath, 
                                            tempBuffer[0],
					    tempBuffer[1],
					    tempBuffer[2],
					    tempBuffer[3]);
        }
      }
    }
    if (messageType=="delay"){
      framerateThrottle = messageBody=="on";
    }
    if (messageType=="pause"){
      if (messageBody == "on"){
        ar_mutex_lock(&pauseLock);
	pauseFlag = true;
	ar_mutex_unlock(&pauseLock);
      }
      else if (messageBody == "off"){
        ar_mutex_lock(&pauseLock);
        pauseFlag = false;
        pauseVar.signal();
	ar_mutex_unlock(&pauseLock);
      }
      else
        ar_log_warning() << "szgrender warning: unexpected messageBody \""
	                 << messageBody << "\".\n";
    }
    if (messageType=="color"){
      float theColors[3] = {0,0,0};
      if (messageBody == "off"){
	ar_log_remark() << "szgrender remark: color override off.\n";
        graphicsClient->setOverrideColor(arVector3(-1,-1,-1));
      }
      else{
	ar_parseFloatString(messageBody, theColors, 3); 
        ar_log_remark() << "szgrender remark: screen color ("
	                << theColors[0] << ", " << theColors[1] << ", "
	                << theColors[2] << ").\n";
        graphicsClient->setOverrideColor(arVector3(theColors));
      }
    }
    if (messageType=="reload"){
      // This isn't quite the right way to communicate between the threads.
      // If a reload request comes too quickly on the heals of a previous
      // one, then one of the requests might be ignored. Doesn't really
      // seem too problematic, given the way the software gets used.
      requestReload = true;
    }
  }
}

// The GUI window callbacks. The "init GL" and "mouse" callbacks are not used
// here.

void ar_guiWindowEvent(arGUIWindowInfo* windowInfo){
  switch(windowInfo->getState()){
  case AR_WINDOW_RESIZE:
    windowManager->setWindowViewport(windowInfo->getWindowID(),
    				     0, 0,
   				     windowInfo->getSizeX(),
    				     windowInfo->getSizeY());
    break;
  case AR_WINDOW_CLOSE:
    shutdownAction();
    break;
  default:
    break;
  }
}

void ar_guiWindowKeyboard(arGUIKeyInfo* keyInfo){
  if (keyInfo->getState() == AR_KEY_DOWN){
    switch(keyInfo->getKey()){
    case AR_VK_ESC:
      shutdownAction();
      break;
    case AR_VK_P:
      drawPerformanceDisplay = !drawPerformanceDisplay;
      graphicsClient->drawFrameworkObjects(drawPerformanceDisplay);
      break;
    default:
      break;
    }
  }

}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  // Search for the -n arg
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i], "-n")){
      makeNice = true;
    }
  }

  ar_log().setStream(szgClient.initResponse());
  
  // we expect to be able to get a lock on the computer's screen
  string screenLock = szgClient.getComputerName() + string("/")
                      + szgClient.getMode("graphics");
  int graphicsID;
  if (!szgClient.getLock(screenLock, graphicsID)){
    ar_log_error() << "szgrender error: failed to get screen resource held by component "
	           << graphicsID 
                   << ".\n(Kill that component to proceed.)\n";
    if (!szgClient.sendInitResponse(false)){
      ar_log_error() << "szgrender error: maybe szgserver died.\n";
    }
    return 1;
  }

  graphicsClient = new arGraphicsClient;
  framerateGraph.addElement("framerate", 300, 100, arVector3(1,1,1));
  graphicsClient->addFrameworkObject(&framerateGraph);

  // we inited
  if (!szgClient.sendInitResponse(true)){
    ar_log_error() << "szgrender error: maybe szgserver died.\n";
  }
  
  ar_log().setStream(szgClient.startResponse());
  
  ar_mutex_init(&pauseLock);
  arThread dummy(messageTask, &szgClient);

  // We make the window manager object up here.
  // By default, we ask for a non-threaded window manager.
  windowManager = new arGUIWindowManager(ar_guiWindowEvent,
					 ar_guiWindowKeyboard,
					 NULL,
					 NULL,
                                         false);
  // The graphics client actually does all of the window configuration, etc.
  // internally. The window threads also get started inside it.
  // However, we control the event loop out here.
  graphicsClient->setWindowManager(windowManager);

  if (!loadParameters(szgClient)){
    ar_log_remark() << "szgrender remark: Parameter loading failed.\n";
  }
  

  graphicsClient->setNetworks(szgClient.getNetworks("graphics"));
  // The connection threads + the window threads start in here.
  graphicsClient->start(szgClient);
  // Framelock assumes we are running in single-threaded mode!
  // (consequently, this is the display thread). NOTE: whether or not
  // we are using framelock should be set by the parsing of the window config
  // itself.
  windowManager->findFramelock();

  // we've started
  if (!szgClient.sendStartResponse(true)){
    ar_log_error() << "szgrender error: maybe szgserver died.\n";
  }

  ar_log().setStream(cout);
  
  while (!exitFlag){ 
    ar_mutex_lock(&pauseLock);
    while (pauseFlag){
      pauseVar.wait(&pauseLock);
    }
    ar_mutex_unlock(&pauseLock);
    ar_timeval time1 = ar_time();
    if (requestReload){
      // We essentially get here when a "reload" message comes in.
      (void) loadParameters(szgClient);
      // Make the new windows, but do not, for instance, start the
      // underlying synchronization objects going.
      graphicsClient->start(szgClient, false);
      requestReload = false;
    }

    // Inside here, through callbacks to the arSyncDataClient embedded inside
    // the arGraphicsClient, is where all the scene graph event processing,
    // drawing, and synchronization happens.
    graphicsClient->_cliSync.consume();

    if (framerateThrottle){
      ar_usleep(200000);
    }
    if (makeNice){
      // Do not have this be a default for szgrender. It seriously throttles
      // the framerate of high framerate displays.
      ar_usleep(2000);
    }
    windowManager->processWindowEvents();
    arPerformanceElement* framerateElement 
      = framerateGraph.getElement("framerate");
    double frameTime = ar_difftime(ar_time(), time1);
    framerateElement->pushNewValue(1000000.0/frameTime);
  }
  // Clean-up.
  graphicsClient->_cliSync.stop();
  // This should occur in the display thread before exiting.
  // NOTE: we are assuming that framelock is ONLY used in the window
  // manager's single threaded mode.
  windowManager->deactivateFramelock();
  // Stops all the window threads and deletes the windows.
  // Definitely a good idea to do this here as it increases the
  // determinism of the exit.
  windowManager->deleteAllWindows();
  return 0;
}
