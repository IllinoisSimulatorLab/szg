//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsClient.h"
#include "arWildcatUtilities.h"

arGraphicsClient* graphicsClient = NULL;
bool stereoMode = false;
bool drawFramerate = false;
bool framerateThrottle = false;
bool exitFlag = false;

// extra services
arConditionVar pauseVar;
arMutex        pauseLock;
bool           pauseFlag = false;
bool           screenshotFlag = false;
// we start with screenshot0.ppm, then do screenshot1.ppm, etc.
int            whichScreenshot = 0;
int            screenshotStartX = 0;
int            screenshotStartY = 0;
int            screenshotWidth = 640;
int            screenshotHeight = 480;
string         dataPath;  // For storing screenshots.

// the parameter variables
int xSize = -1, ySize = -1;
int xPos = -1, yPos = -1;
string textPath;

bool loadParameters(arSZGClient& cli){
  // important that we use the parameters for our particular screen
  string screenName = cli.getMode("graphics");

  int sizeBuffer[2];
  string sizeString = cli.getAttribute(screenName, "size");
  if (sizeString != "NULL" 
      && ar_parseIntString(sizeString,sizeBuffer,2)== 2){
    xSize = sizeBuffer[0];
    ySize = sizeBuffer[1];
  }
  else{
    xSize = 640;
    ySize = 480;
  }

  string posString = cli.getAttribute(screenName, "position");
  if (sizeString != "NULL"
      && ar_parseIntString(posString,sizeBuffer,2)){
    xPos = sizeBuffer[0];
    yPos = sizeBuffer[1];
  }
  else{
    xPos = 0;
    yPos = 0;
  }

  stereoMode = cli.getAttribute(screenName, "stereo",
    "|false|true|") == "true";
  graphicsClient->setStereoMode(stereoMode);
  drawFramerate = cli.getAttribute(screenName, "show_framerate",
    "|false|true|") == "true";
  graphicsClient->showFramerate(drawFramerate);
  ar_useWildcatFramelock(cli.getAttribute(
    screenName, "wildcat_framelock", "|false|true|") == "true");
  // the arScreenObject(s) are now implicitly configured in arGraphicsClient
  // via the arGraphicsWindow contained therein.
  graphicsClient->configure(&cli);
  dataPath = cli.getAttribute("SZG_DATA", "path"); // For storing screenshots.
  return true;
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    int sendID = cli->receiveMessage(&messageType,&messageBody);
    // Note how we only hit this ONCE!
    if (!sendID){
      // sendID == 0 exactly when we've been FORCED to shutdown by the 
      // szgserver.
      cout << "szgrender shutting down.\n";
      // NOTE: these shutdown tasks are cut-and-pasted from the below.
      exitFlag = true;
      // make sure we actually get to the end of the draw loop
      graphicsClient->_cliSync.skipConsumption();
      // We WILL NOT be receiving any more messages. Go ahead and exit loop.
      break;
    }
    if (messageType=="quit"){
      // note that we exit within the draw loop to avoid crashes on Win32
      exitFlag = true;
      // make sure we actually get to the end of the draw loop
      graphicsClient->_cliSync.skipConsumption();
    }
    if (messageType=="screenshot"){
      if (dataPath == "NULL"){
	cerr << "szgrender warning: empty SZG_DATA/path, screenshot failed.\n";
      }
      else{
        if (messageBody != "NULL"){
          int tempBuffer[4];
          ar_parseIntString(messageBody,tempBuffer,4);
          screenshotStartX = tempBuffer[0];
          screenshotStartY = tempBuffer[1];
          screenshotWidth  = tempBuffer[2];
          screenshotHeight = tempBuffer[3];
        }
        screenshotFlag = true;
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
        cerr << "szgrender warning: unexpected messageBody \""
	     << messageBody << "\".\n";
    }
    if (messageType=="viewmode") {
      graphicsClient->setViewMode( messageBody );
    }
    if (messageType=="color"){
      float theColors[3] = {0,0,0};
      if (messageBody == "off"){
	cout << "szgrender remark: color override off.\n";
        graphicsClient->setOverrideColor(arVector3(-1,-1,-1));
      }
      else{
	ar_parseFloatString(messageBody, theColors, 3); 
        cout << "szgrender remark: screen color ("
	     << theColors[0] << ", " << theColors[1] << ", "
	     << theColors[2] << ").\n";
        graphicsClient->setOverrideColor(arVector3(theColors));
      }
    }
    if (messageType=="camera"){
      if (messageBody == "NULL"){
        graphicsClient->setCamera(-1);
      }
      else{
	int theCamera;
        ar_parseIntString(messageBody, &theCamera, 1);
        graphicsClient->setCamera(theCamera);
      }
    }
    if (messageType=="look"){
      if (messageBody=="NULL"){
	// the default camera
        graphicsClient->setCamera(-1);
      }
      else{
	// we activate a new camera, which may be better for screenshots
        float tempBuf[15];
        ar_parseFloatString(messageBody,tempBuf,15);
        graphicsClient->setLocalCamera(tempBuf,tempBuf+6);
      }
    }
    if (messageType=="reload"){
      (void)loadParameters(*cli);
      /// \bug if stereoMode changes from true to false, one eye remains drawn.
      // Is it safe to clear both branches of initGraphics() here?
    }
  }
}

void initGraphics(){
  if (stereoMode){
    glDrawBuffer(GL_BACK_LEFT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glDrawBuffer(GL_BACK_RIGHT);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
  glutSwapBuffers();
}

void display(){
  ar_mutex_lock(&pauseLock);
  while (pauseFlag){
    pauseVar.wait(&pauseLock);
  }
  ar_mutex_unlock(&pauseLock);
  graphicsClient->_cliSync.consume();
  if (screenshotFlag){
    char numberBuffer[5];
    sprintf(numberBuffer,"%i",whichScreenshot);
    string screenshotName = string("screenshot")+numberBuffer+string(".jpg");
    char* buffer1 = new char[screenshotWidth*screenshotHeight*3];
    if (!stereoMode){
      glReadBuffer(GL_FRONT);
    }
    else{
      glReadBuffer(GL_FRONT_LEFT);
    }
    glReadPixels(screenshotStartX, screenshotStartY,
                 screenshotWidth, screenshotHeight,
                 GL_RGB,GL_UNSIGNED_BYTE,buffer1);
    arTexture* texture = new arTexture;
    texture->setPixels(buffer1,screenshotWidth,screenshotHeight);
    if (!texture->writeJPEG(screenshotName.c_str(),dataPath)){
      cerr << "szgrender remark: failed to write screenshot.\n";
    }
    delete texture;
    delete buffer1;
    // don't forget to increment the screenshot number
    whichScreenshot++;
    screenshotFlag = false;
  }
  // we want to exit here, if requested, to avoid crashes on Win32
  if (exitFlag){
    graphicsClient->_cliSync.stop();
    // this should occur in the display thread before exiting
    ar_deactivateWildcatFramelock();
    exit(0);
  }
  if (framerateThrottle){
    ar_usleep(200000);
  }
  ar_usleep(2000); // cpu throttle: we might not be the only one on this CPU
}

void keyboard(unsigned char key, int /*x*/, int /*y*/){
  switch (key) {
    case 27: /* escape key */
      // AARGH!!! This should use the stop mechanism as well!!!!
      exit(0);
    case 'f':
      glutFullScreen();
      break;
    case 'F':
      glutReshapeWindow(640,480);
      break;
  }
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

  // we expect to be able to get a lock on the computer's screen
  string screenLock = szgClient.getComputerName() + string("/")
                      + szgClient.getMode("graphics");
  int graphicsID;
  if (!szgClient.getLock(screenLock, graphicsID)){
    cout << "szgrender error: failed to get screen resource held by component "
	 << graphicsID 
         << ".\n(Kill that component to proceed.)\n";
    szgClient.sendInitResponse(false);
    return 1;
  }

  graphicsClient = new arGraphicsClient;

  // get the initial parameters
  if (!loadParameters(szgClient)){
    szgClient.sendInitResponse(false);
    return 1;
  }

  // we have successfully inited
  szgClient.sendInitResponse(true);

  ar_mutex_init(&pauseLock);
  arThread dummy(messageTask, &szgClient);

  // cout << "szgrender remark: creating window.\n";
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB |
    (stereoMode ? GLUT_STEREO : 0));
  glutInitWindowPosition(xPos,yPos);
  if (xSize>0 && ySize>0){
    glutInitWindowSize(xSize,ySize);
    glutCreateWindow("Syzygy Graphics Viewer");
  }
  else{
    glutInitWindowSize(640,480);
    glutCreateWindow("Syzygy Graphics Viewer");
    glutFullScreen();
  }
  // cout << "szgrender remark: created window.\n";
  glutSetCursor(GLUT_CURSOR_NONE);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutIdleFunc(display);

  initGraphics();
  graphicsClient->setNetworks(szgClient.getNetworks("graphics"));
  graphicsClient->start(szgClient);

  // we've successfully started
  szgClient.sendStartResponse(true);

  // so far, the init purely has to do with starting up 
  // the wildcat framelocking
  // after the window has popped up... this is CONFUSING givent the way init
  // is used elsewhere
  graphicsClient->init();

  glutMainLoop();
  return 0;
}
