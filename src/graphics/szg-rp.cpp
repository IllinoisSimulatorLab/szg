//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arGraphicsPeerRPC.h"
#include "arFramerateGraph.h"
#include "arLogStream.h"
#include "arGlut.h"

// NOTE: arSZGClient CANNOT BE GLOBAL ON WINDOWS. THIS IS BECAUSE 
// OF OUR METHOD OF INITIALIZING WINSOCK (USING GLOBALS).
arSZGClient* szgClient = NULL;

arFramerateGraph framerateGraph;
// By default, the drawing function has a 1/100 second delay added to
// every frame. This improves system responsiveness (the idea is that
// the szg-rp will be only one of several things running on the computer).
bool highPerformance = false;
bool showPerformance = false;

bool drawLabels = false;

struct PeerDescription{
public:
  arGraphicsPeer* peer;
  arMatrix4       transform;
  arMatrix4       labelTransform;
  string          comment;
};

typedef map<string,PeerDescription,less<string> > PeerContainer;
PeerContainer peers;
arLock peerLock;
int xPos = 0;
int yPos = 0;
int xSize = 600;
int ySize = 600;
bool useFullscreen = false;
arMatrix4 translationMatrix;
string peerTexturePath, peerTextPath;

// The camera can be moved.
arPerspectiveCamera globalCamera;
// A placeholder that helps with "motion culling", if such has been enabled.
arGraphicsPeerCullObject cullObject;
bool useMotionCull = false;

arGraphicsPeer* createNewPeer(const string& name){
  arGraphicsPeer* graphicsPeer = new arGraphicsPeer();
  graphicsPeer->setName(name);
  graphicsPeer->useLocalDatabase(true);
  graphicsPeer->queueData(true);
  graphicsPeer->init(*szgClient);
  if (!graphicsPeer->start()){
    ar_log_error() << "szg-rp error: peer failed to init.\n";
    return NULL;
  }
  graphicsPeer->setTexturePath(peerTexturePath);
  graphicsPeer->loadAlphabet(peerTextPath.c_str());
  return graphicsPeer;
}

bool loadParameters(arSZGClient& cli){
  // Load the parameters for our particular display.
  // szg-rp differs from the frameworks (szgrender and 
  // arMasterSlaveFramework): it does not try to set up multiple
  // windows, cameras, etc. Instead, it justs query the
  // config of the first window in the XML to determine the position,
  // size, and fullscreen/not-fullscreen.
  const string screenName(cli.getMode("graphics"));
  const string configName(cli.getAttribute(screenName, "name"));
  if (configName != "NULL"){
    // A configuration record has actually been specified.
    string queryName = configName + "/szg_window/size/width";
    string queryResult = cli.getSetGlobalXML(queryName);
    if (queryResult != "NULL"){
      // Something was actually defined. A default is already set globally.
      xSize = atoi(queryResult.c_str());
    }
    queryName = configName + "/szg_window/size/height";
    queryResult = cli.getSetGlobalXML(queryName);
    if (queryResult != "NULL"){
      // Something was actually defined. A default is already set globally.
      ySize = atoi(queryResult.c_str());
    }
    queryName = configName + "/szg_window/position/x";
    queryResult = cli.getSetGlobalXML(queryName);
    if (queryResult != "NULL"){
      // Something was actually defined. A default is already set globally.
      xPos = atoi(queryResult.c_str());
    }
    queryName = configName + "/szg_window/position/y";
    queryResult = cli.getSetGlobalXML(queryName);
    if (queryResult != "NULL"){
      // Something was actually defined. A default is already sey globally.
      yPos = atoi(queryResult.c_str());
    }
    queryName = configName + "/szg_window/fullscreen/value";
    queryResult = cli.getSetGlobalXML(queryName);
    if (queryResult == "true"){
      // The default is false and is set in the globals.
      useFullscreen = true;
    }
  }

  peerTexturePath = cli.getAttribute("SZG_RENDER", "texture_path");
  peerTextPath = cli.getAttribute("SZG_RENDER","text_path");
  ar_pathAddSlash(peerTextPath);
  return true;
}

string getTail(const string& text){
  unsigned int location = text.find('/');
  return text.substr(location+1, text.length()-location -1);
}

// The following message handlers are for messages directly to the
// szg-rp program.

string handleCreate(const string& messageBody){
  PeerContainer::iterator i = peers.find(messageBody);
  if (i != peers.end()){
    return "szg-rp error: tried to create over existing reality peer.\n";
  }

  arGraphicsPeer* peer = createNewPeer(messageBody);
  if (!peer){
    return "szg-rp error: failed to create peer (name already used?).\n";
  }

  PeerDescription temp;
  temp.peer = peer;
  peerLock.lock();
    // Lock, since another thread draws and uses this iterator.
    peers.insert(PeerContainer::value_type(messageBody, temp));
  peerLock.unlock();
  return "szg-rp success: inserted peer " + messageBody + ".\n";
}

string handleDelete(const string& messageBody){
  PeerContainer::iterator i = peers.find(messageBody);
  if (i == peers.end()){
    return "szg-rp error: cannot delete nonexistent peer.\n";
  }

  arGraphicsPeer* peer = i->second.peer;
  peer->closeAllAndReset();
  // Lock, since another thread draws and uses this iterator.
  peerLock.lock();
    peers.erase(i);
  peerLock.unlock();
  // delete peer; // Memory leak: delete may be unsafe.
  return "szg-rp success: deleted peer.\n";  
}

string handleTranslation(const string& messageBody){
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 4){
    return "szg-rp error: body format must be local peer followed by translation.\n";
  }

  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    return "szg-rp error: no local peer named '" + bodyList[0] + "'.\n";
  }

  arVector3 trans;
  ar_parseFloatString(getTail(bodyList), trans.v, 3);
  i->second.transform = ar_translationMatrix(trans);
  stringstream s;
  s << "szg-rp success: translation = " << trans << "\n";
  return s.str();
}

string handleLabelTranslate(const string& messageBody){
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 4){
    return "szg-rp error: body format must be local peer followed by translation.\n";
  }

  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    return "szg-rp error: no local peer named '" + bodyList[0] + "'.\n";
  }

  arVector3 trans;
  ar_parseFloatString(getTail(bodyList), trans.v, 3);
  i->second.labelTransform = ar_translationMatrix(trans);
  stringstream s;
  s << "szg-rp success: label translation = " << trans << "\n";
  return s.str();
}

string handleCommentSet(const string& messageBody){
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    return "szg-rp error: need peer/comment.\n";
  }

  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    return "szg-rp error: no local peer named '" + bodyList[0] + "'.\n";
  }

  i->second.comment = bodyList[1];
  return "szg-rp success: comment set for peer " + bodyList[0] + "\n";
}

string handleCommentGet(const string& messageBody){
  PeerContainer::iterator i = peers.find(messageBody);
  if (i == peers.end()){
    return "szg-rp error: no local peer named '" + messageBody + "'.\n";
  }

  return "szg-rp success: " + i->second.comment + "\n";
}

string handleList(){
  string s("szg-rp success: peer list:\n");
  for (PeerContainer::const_iterator i = peers.begin(); i != peers.end(); ++i){
    s += "peer name = " + i->first +
	 "\ncomment = " + i->second.comment + "\n" +
         i->second.peer->printConnections() + "\n\n";  
  }
  return s;
}

string handleLabel(const string& messageBody){
  if (messageBody == "on"){
    drawLabels = true;
    return "szg-rp success: labels on.\n";
  }

  if (messageBody == "off"){
    drawLabels = false;
    return "szg-rp success: labels off.\n";
  }

  return "szg-rp error: invalid body (must be on or off).\n";
}

string handleCamera(const string& messageBody){
  float temp[15] = {0};
  ar_parseFloatString(messageBody, temp, 15);
  int i;
  for (i=0; i<6; i++){
    globalCamera.frustum[i] = temp[i];
  }
  for (i=0; i<9; i++){
    globalCamera.lookat[i] = temp[i+6];
  }
  return "szg-rp success: camera modified.\n";
}

string handleMotionCull(const string& messageBody){
  useMotionCull = messageBody == "on";
  return "szg-rp success: motion cull " + string(useMotionCull ? "on" : "off") + ".\n";
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  // all of the messages to szg-rp receive responses
  string responseBody;
  while (true) {
    const int messageID = cli->receiveMessage(&messageType,&messageBody);
    if (!messageID){
      // szgserver has quit.
      cerr << "szg-rp error: problem receiving message.\n";
      exit(0);
    }
    if (messageType == "quit"){
      exit(0);
    }
    else if (messageType == "performance"){
      showPerformance = messageBody == "on";
    }
    else if (messageType == "create"){
      responseBody = handleCreate(messageBody);
    }
    else if (messageType == "delete"){
      responseBody = handleDelete(messageBody);
    }
    else if (messageType == "translate"){
      responseBody = handleTranslation(messageBody);
    }
    else if (messageType == "comment-get"){
      responseBody = handleCommentGet(messageBody);
    }
    else if (messageType == "comment-set"){
      responseBody = handleCommentSet(messageBody);
    }
    else if (messageType == "list"){
      responseBody = handleList();
    }
    else if (messageType == "label-translate"){
      responseBody = handleLabelTranslate(messageBody);
    }
    else if (messageType == "label"){
      responseBody = handleLabel(messageBody);
    }
    else if (messageType == "camera"){
      responseBody = handleCamera(messageBody);
    }
    else if (messageType == "motion_cull"){
      responseBody = handleMotionCull(messageBody);
    }
    // This ends the messages specifically sent to the szg-rp.
    else{
      peerLock.lock();
      // By convention, the messageBody will be peer_name/stuff (though /stuff
      // is optional). The following call seperates out the peer name and
      // strips messageBody in place.
      const string peerName(ar_graphicsPeerStripName(messageBody));
      const PeerContainer::iterator i = peers.find(peerName);
      if (i == peers.end()){
        responseBody = "szg-rp error: no local peer '" + peerName + "'\n";
        peerLock.unlock();
      }
      else{
        arGraphicsPeer* temp = i->second.peer;
        peerLock.unlock();
        responseBody = ar_graphicsPeerHandleMessage(temp, messageType, messageBody);
      }
    }
    // return the message response.
    if (!cli->messageResponse(messageID, responseBody)){
      ar_log_error() << "szg-rp error: message response failed.\n";
    }
  }
}

void renderLabel(const string& name){
  glColor3f(1,0.2,0.2);
  glBegin(GL_QUADS);
  glVertex3f(-0.5,-0.3,0);
  glVertex3f(-0.5,0.7,0);
  glVertex3f(name.length()/2.5,0.7,0);
  glVertex3f(name.length()/2.5,-0.3,0);
  glEnd();
  glColor3f(1,1,1);
  glScalef(0.005, 0.005, 0.005);
  for (unsigned int i=0; i<name.length(); i++){
    glutStrokeCharacter(GLUT_STROKE_ROMAN, name[i]);
  }
}

list<int> lastFrameTimes;

int averageFrameTime(int thisTime){
  // Low-pass filter: average last 20 frame times.
  if (lastFrameTimes.size() >= 20){
    lastFrameTimes.pop_front();
  }
  lastFrameTimes.push_back(thisTime);
  int total = 0;
  for (list<int>::iterator i = lastFrameTimes.begin();
       i != lastFrameTimes.end(); i++){
    total += *i;
  }
  return int(total/20.0);
}

void display(){
  static ar_timeval time1;
  static bool timeInit = false;
  glClearColor(0,0,0,0);
  glEnable(GL_DEPTH_TEST);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /*glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.1,0.1,-0.1,0.1,0.2,100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,5,20, 0,5,0, 0,1,0);*/
  globalCamera.loadViewMatrices();
  arMatrix4 projectionCullMatrix(globalCamera.getProjectionMatrix());
  PeerContainer::iterator i;
  peerLock.lock();
  // first pass to draw the peers
  // Track how long it takes to read in the data, and draw that as well.
  double dataTime = 0;
  int dataAmount = 0;
  for (i = peers.begin(); i != peers.end(); i++){
    const ar_timeval time9 = ar_time();
    dataAmount += i->second.peer->consume();
    dataTime += ar_difftime(ar_time(), time9)/1000.0;
    glPushMatrix();
      glMultMatrixf(i->second.transform.v);
      i->second.peer->activateLights();
      i->second.peer->draw(&projectionCullMatrix);
    glPopMatrix();
  }
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // "motion culling" does not work when the peers are translated using
  // the translation function of the workspace. Maybe that function is 
  // outmoded anyway...
  if (useMotionCull){
    for (i = peers.begin(); i!= peers.end(); i++){
      i->second.peer->motionCull(&cullObject,&globalCamera);
      if (!cullObject.cullChangeOn.empty()){
        for (list<int>::iterator onIter = cullObject.cullChangeOn.begin();
	     onIter != cullObject.cullChangeOn.end();
	     onIter++){
	  // Every node's update is requested.
          i->second.peer->mappedFilterDataBelow(*onIter, AR_TRANSIENT_NODE);
        }
      }
      if (!cullObject.cullChangeOff.empty()){
        for (list<int>::iterator offIter = cullObject.cullChangeOff.begin();
	     offIter != cullObject.cullChangeOff.end();
	     offIter++){
	  // Only the updates to transient nodes are discarded.
          i->second.peer->mappedFilterDataBelow(*offIter, AR_OPTIONAL_NODE);
        }
      }
      // Clear the lists.
      cullObject.frame();
    }
  }
  // second pass to draw the labels
  if (drawLabels){
    // AARGH! The label code will not work with different camera angles.
    glDisable(GL_DEPTH_TEST);
    for (i = peers.begin(); i != peers.end(); i++){
      glPushMatrix();
      glMultMatrixf(i->second.transform.v);
      glMultMatrixf(i->second.labelTransform.v);
      glColor3f(1,1,1);
      glDisable(GL_LIGHTING);
      renderLabel(i->first);
      glPopMatrix();
    }
    glEnable(GL_DEPTH_TEST);
  }
  if (showPerformance){
    framerateGraph.drawWithComposition();
  }
  peerLock.unlock();
  // It seems like a good idea to throttle szg-rp artificially here.
  // After all, this workspace will be just *one* part of an overall
  // workflow. Whereas default throttling is bad in the dedicated 
  // display case, it makes sense here.
  // The command line option -p can override this setting.
  if (!highPerformance){
    ar_usleep(10000);
  }
  glutSwapBuffers();

  // Return the frame time every frame instead of more occasionally,
  // for a tighter feedback loop? Still experimenting.
  int frametime = 50000; // default
  if (!timeInit){
    timeInit = true;
    time1 = ar_time();
  }
  else{
    ar_timeval temp = time1;
    time1 = ar_time();
    frametime = int(ar_difftimeSafe(time1, temp));
  }

  peerLock.lock();
  for (i=peers.begin(); i!=peers.end(); i++){
    i->second.peer->broadcastFrameTime(averageFrameTime(frametime));
  }
  framerateGraph.getElement("           fps")->pushNewValue(1000000.0/frametime);
  framerateGraph.getElement("consume() msec")->pushNewValue(dataTime);
  framerateGraph.getElement("consume()   kB")->pushNewValue(dataAmount/1000.);
  peerLock.unlock();
}

void keyboard(unsigned char key, int /*x*/, int /*y*/){
  switch (key) {
    case 27: /* escape key */
      // AARGH!!! This should use the stop mechanism as well!!!!
      szgClient->messageTaskStop();
      exit(0);
    case 'f':
      glutFullScreen();
      break;
    case 'F':
      glutReshapeWindow(640,480);
      break;
    case 'P':
      showPerformance = !showPerformance;
      break;
  }
}

int main(int argc, char** argv){
  framerateGraph.addElement("           fps", 300, 100, arVector3(1,1,1));
  framerateGraph.addElement("consume() msec", 300, 100, arVector3(1,1,0));
  framerateGraph.addElement("consume()   kB", 300, 500, arVector3(0,1,1));
  
  globalCamera.setSides(-0.1, 0.1, -0.1, 0.1);
  globalCamera.setNearFar(0.2, 200);
  globalCamera.setPosition(0,5,20);
  globalCamera.setTarget(0,5,0);
  globalCamera.setUp(0,1,0);

  szgClient = new arSZGClient();
  szgClient->simpleHandshaking(false);
  const bool fInit = szgClient->init(argc, argv);
  if (!*szgClient){
    return szgClient->failStandalone(fInit);
  }

  for (int i=0; i<argc; i++){
    if (!strcmp("-p",argv[i])){
      highPerformance = true;
      // Strip the arg.
      for (int j=i; j<argc-1; j++){
        argv[j] = argv[j+1];
      }
      --argc;
      --i;
    }
  }
  if (argc < 2){
    ar_log_critical() << "usage: szg-rp <name>\n";
    if (!szgClient->sendInitResponse(false)){
      cerr << "szg-rp error: maybe szgserver died.\n";
    }
    return 1;
  }

  ar_log_remark() << "running as peer '" << argv[1] << "'.\n";
  if (!szgClient->sendInitResponse(true)){
    cerr << "szg-rp error: maybe szgserver died.\n";
  }

  // Lock to ensure a unique workspace name.
  int componentID = -1;
  if (!szgClient->getLock(string("szg-rp-")+argv[1], componentID)){
    ar_log_critical() << "non-unique workspace name, already held by component with ID " <<
      componentID << ".\n";
    if (!szgClient->sendStartResponse(false)){
      cerr << "szg-rp error: maybe szgserver died.\n";
    }
    return 1;
  }

  if (!loadParameters(*szgClient)){
    ar_log_error() << "failed to load parameters.\n";
    if (!szgClient->sendStartResponse(false)){
      cerr << "szg-rp error: maybe szgserver died.\n";
    }
  }

  ar_log_remark() << "szg-rp started.\n";
  if (!szgClient->sendStartResponse(true)){
    cerr << "szg-rp error: maybe szgserver died.\n";
  }
  arThread dummy(messageTask, szgClient);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(xPos,yPos);
  glutInitWindowSize(xSize, ySize);
  glutCreateWindow("Syzygy Reality Peer");
  if (useFullscreen)
    glutFullScreen();
  glutSetCursor(GLUT_CURSOR_NONE);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutMainLoop();
  return 0;
}
