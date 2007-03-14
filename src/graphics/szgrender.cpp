//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arGraphicsClient.h"
#include "arFramerateGraph.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arLogStream.h"
#include "arGraphicsPluginNode.h"

arGraphicsClient graphicsClient;
arFramerateGraph  framerateGraph;
arGUIWindowManager* windowManager;
arGUIXMLParser*     guiParser;

bool framerateThrottle = false;
bool fDrawPerformance = false;
bool fExit = false;
bool fReload = false;
bool fPause = false;
arConditionVar pauseVar;
arMutex pauseLock; // around fPause and pauseVar, between message and draw threads.
string dataPath("NULL");
string textPath("NULL");

bool loadParameters(arSZGClient& cli){
  // Use our screen's parameters.
  graphicsClient.configure(&cli);

  dataPath = cli.getDataPath();
  graphicsClient.addDataBundlePathMap("SZG_DATA", dataPath);
  graphicsClient.addDataBundlePathMap("SZG_PYTHON", cli.getDataPathPython());
  arGraphicsPluginNode::setSharedLibSearchPath(cli.getAttribute("SZG_EXEC","path"));
  return true;
}

void shutdownAction(){
  if (fExit) {
    // exit already pending
    return;
  }
  fExit = true;

  ar_log_debug() << "szgrender shutdown.\n";
  // Let the draw loop _cliSync.consume() finish.
  graphicsClient._cliSync.skipConsumption();

  // exit() from the draw loop, to avoid Win32 crashes.
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    // Happens only once.
    if (!cli->receiveMessage(&messageType,&messageBody) || messageType=="quit"){
      shutdownAction();
      break;
    }

    if (messageType=="performance"){
      fDrawPerformance = messageBody=="on";
      graphicsClient.drawFrameworkObjects(fDrawPerformance);
    }

    else if (messageType=="screenshot"){
      // copypaste with framework/arMasterSlaveFramework.cpp
      if (dataPath == "NULL"){
	ar_log_warning() << "szgrender screenshot failed: no SZG_DATA/path.\n";
      }
      else{
        if (messageBody != "NULL"){
          int temp[4];
          ar_parseIntString(messageBody,temp,4);
          graphicsClient.requestScreenshot(dataPath, temp[0], temp[1], temp[2], temp[3]);
        }
      }
    }

    else if (messageType=="delay"){
      framerateThrottle = messageBody=="on";
    }

    else if (messageType=="pause"){
      if (messageBody == "on"){
        ar_mutex_lock(&pauseLock);
	  fPause = true;
	ar_mutex_unlock(&pauseLock);
      }
      else if (messageBody == "off"){
        ar_mutex_lock(&pauseLock);
	  fPause = false;
	  pauseVar.signal();
	ar_mutex_unlock(&pauseLock);
      }
      else
        ar_log_warning() << "szgrender: unexpected pause '" << messageBody << "', should be on or off.\n";
    }

    else if (messageType=="color"){
      float c[3] = {0,0,0};
      if (messageBody == "off"){
	ar_log_remark() << "szgrender: color override off.\n";
        graphicsClient.setOverrideColor(arVector3(-1,-1,-1));
      }
      else{
	ar_parseFloatString(messageBody, c, 3); 
        ar_log_remark() << "szgrender screen color (" << c[0] << ", " << c[1] << ", " << c[2] << ").\n";
        graphicsClient.setOverrideColor(arVector3(c));
      }
    }

    else if (messageType=="reload"){
      // Multiple reload messages might be ignored if this message thread
      // gets them faster than the draw thread notices them.
      // Not a problem, though, in practice:  the "last" message gets processed.
      fReload = true;
    }
  }
}

// GUI window callbacks. "Init GL" and "mouse" callbacks are not used.

void ar_guiWindowEvent(arGUIWindowInfo* wi){
  if (!wi)
    return;
  arGUIWindowManager* wm = wi->getWindowManager();
  if (!wm)
    return;
  switch (wi->getState()){
  case AR_WINDOW_RESIZE:
    wm->setWindowViewport(wi->getWindowID(), 0, 0, wi->getSizeX(), wi->getSizeY());
    break;
  case AR_WINDOW_CLOSE:
    shutdownAction();
    break;
  default:
    // avoid compiler warning
    break;
  }
}

void ar_guiWindowKeyboard(arGUIKeyInfo* ki){
  if (!ki)
    return;
  if (ki->getState() == AR_KEY_DOWN){
    switch (ki->getKey()){
    case AR_VK_ESC:
      shutdownAction();
      break;
    case AR_VK_P:
      fDrawPerformance = !fDrawPerformance;
      graphicsClient.drawFrameworkObjects(fDrawPerformance);
      break;
    default:
      // avoid compiler warning
      break;
    }
  }

}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  const string screenLock = szgClient.getComputerName() + "/" + szgClient.getMode("graphics");
  int graphicsID = -1;
  if (!szgClient.getLock(screenLock, graphicsID)){
    ar_log_error() << "szgrender screen locked by component " << graphicsID << ".\n";
    if (!szgClient.sendInitResponse(false))
      cerr << "szgrender error: maybe szgserver died.\n";
    return 1;
  }

  framerateGraph.addElement("framerate", 300, 100, arVector3(1,1,1));
  graphicsClient.addFrameworkObject(&framerateGraph);

  if (!szgClient.sendInitResponse(true)){
    cerr << "szgrender error: maybe szgserver died.\n";
  }

  ar_mutex_init(&pauseLock);
  arThread dummy(messageTask, &szgClient);

  // Default to a non-threaded window manager.
  windowManager = new arGUIWindowManager(
    ar_guiWindowEvent, ar_guiWindowKeyboard, NULL, NULL, false);

  // graphicsClient configures windows and starts window threads,
  // but szgRender's main() has the event loop.
  graphicsClient.setWindowManager(windowManager);

  if (!loadParameters(szgClient))
    ar_log_remark() << "szgrender parameter load failed.\n";

  graphicsClient.setNetworks(szgClient.getNetworks("graphics"));

  // Start the connection threads and window threads.
  graphicsClient.start(szgClient);

  // Framelock assumes the window manager is single-threaded,
  // so this is the display thread.
  // Parse window config to decide whether or not to use framelock.
  windowManager->findFramelock();

  if (!szgClient.sendStartResponse(true)){
    cerr << "szgrender error: maybe szgserver died.\n";
  }

  while (!fExit) {
    ar_mutex_lock(&pauseLock);
      while (fPause) {
	pauseVar.wait(&pauseLock);
      }
    ar_mutex_unlock(&pauseLock);

    ar_timeval time1 = ar_time();
    if (fReload){
      fReload = false;
      (void)loadParameters(szgClient);
      // Make new windows but don't start the underlying synchronization objects.
      graphicsClient.start(szgClient, false);
    }

    // Process scenegraph events, draw, and sync
    // (via callbacks to the arSyncDataClient embedded inside the arGraphicsClient).
    graphicsClient._cliSync.consume();

    if (framerateThrottle){
      ar_usleep(200000);
    }

    windowManager->processWindowEvents();
    framerateGraph.getElement("framerate")->pushNewValue(
      1000000.0 / ar_difftimeSafe(ar_time(), time1));
  }

  // Clean up.
  graphicsClient._cliSync.stop();

  // We're the display thread.  Do this before exiting.
  windowManager->deactivateFramelock();

  windowManager->deleteAllWindows();
  return 0;
}
