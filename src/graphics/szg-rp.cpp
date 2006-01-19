//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsPeerRPC.h"
#include "arFramerateGraph.h"
#include "arLogStream.h"

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

class PeerDescription{
public:
  PeerDescription(){}
  ~PeerDescription(){}

  arGraphicsPeer* peer;
  arMatrix4       transform;
  arMatrix4       labelTransform;
  string          comment;
};

typedef map<string,PeerDescription,less<string> > PeerContainer;
PeerContainer peers;
arMutex peerLock;
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
    ar_log_error() << "szg-rp error: init of peer failed.\n";
    return NULL;
  }
  graphicsPeer->setTexturePath(peerTexturePath);
  graphicsPeer->loadAlphabet(peerTextPath.c_str());
  return graphicsPeer;
}

bool loadParameters(arSZGClient& cli){
  // Important that we load the parameters for our particular display.
  // NOTE: szg-rp is different from the frameworks (szgrender and 
  // arMasterSlaveFramework) in that it DOES NOT try to set up multiple
  // windows, cameras, etc. Instead, we just go ahead and query the
  // config of the first window in the XML and determine the position,
  // size, and fullscreen/not-fullscreen specified.
  string screenName = cli.getMode("graphics");
  string configName = cli.getAttribute(screenName, "name");
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
  stringstream result;
  PeerContainer::iterator i = peers.find(messageBody);
  if (i != peers.end()){
    result << "szg-rp error: "
	   << "tried to create over existing reality peer.\n";
  }
  else{
    arGraphicsPeer* peer = createNewPeer(messageBody);
    if (!peer){
      result << "szg-rp error: failed to create peer.\n"
	     << "  (another peer elsewhere on the network might have "
	     << "this name)\n";
    }
    else{
      PeerDescription temp;
      temp.peer = peer;
      // THIS IS IMPORTANT. WE ARE DRAWING (AND USING THIS ITERATOR)
      // IN ANOTHER THREAD.
      ar_mutex_lock(&peerLock);
      peers.insert(PeerContainer::value_type(messageBody, temp));
      ar_mutex_unlock(&peerLock);
      result << "szg-rp success: inserted peer " << messageBody << ".\n";
    }
  }
  return result.str();
}

string handleDelete(const string& messageBody){
stringstream result;
  PeerContainer::iterator i = peers.find(messageBody);
  if (i == peers.end()){
    result << "szg-rp error: "
	   << "cannot delete a peer that does not exist.\n";
  }
  else{
    arGraphicsPeer* peer = i->second.peer;
    peer->closeAllAndReset();
    // THIS IS IMPORTANT. WE ARE DRAWING (AND USING THIS ITERATOR)
    // IN ANOTHER THREAD.
    ar_mutex_lock(&peerLock);
    peers.erase(i);
    ar_mutex_unlock(&peerLock);
    // MIGHT BE UNSAFE TO DELETE THE PEER. THIS IS A BAD MEMORY LEAK.
    // delete peer;
    result << "szg-rp success: deleted peer.\n";  
  }
  return result.str();
}

string handleTranslation(const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 4){
    result << "szg-rp error: body format must be local peer followed by "
	   << "translation.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  string tail = getTail(bodyList);
  float trans[3];
  ar_parseFloatString(tail, trans, 3);
  result << "szg-rp success: translation = " << trans[0] << " "
         << trans[1] << " " 
         << trans[2] << "\n";
  i->second.transform = ar_translationMatrix(trans[0], trans[1], trans[2]);
  return result.str();
}

string handleLabelTranslate(const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 4){
    result << "szg-rp error: body format must be local peer followed by "
	   << "translation.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  string tail = getTail(bodyList);
  float trans[3];
  ar_parseFloatString(tail, trans, 3);
  result << "szg-rp success: label translation = " << trans[0] << " "
         << trans[1] << " " 
         << trans[2] << "\n";
  i->second.labelTransform 
    = ar_translationMatrix(trans[0], trans[1], trans[2]);
  return result.str();
}

string handleCommentSet(const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need peer/comment.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  i->second.comment = bodyList[1];
  result << "szg-rp success: comment set for peer " << bodyList[0] << "\n";
  return result.str();
}

string handleCommentGet(const string& messageBody){
  stringstream result;
  PeerContainer::iterator i = peers.find(messageBody);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success: " << i->second.comment << "\n";
  return result.str();
}

string handleList(){
  stringstream result;
  result << "szg-rp success: peer list follows:\n\n";
  for (PeerContainer::iterator i = peers.begin(); i != peers.end(); i++){
    result << "peer name = " << i->first << "\n"
	   << "comment = " << i->second.comment << "\n"
           << i->second.peer->printConnections() << "\n\n";  
  }
  return result.str();
}

string handleLabel(const string& messageBody){
  stringstream result;
  if (messageBody == "on"){
    drawLabels = true;
    result << "szg-rp success: labels on.\n";
  }
  else if (messageBody == "off"){
    drawLabels = false;
    result << "szg-rp success: labels off.\n";
  }
  else{
    result << "szg-rp error: invalid body (must be on or off).\n";
  }
  return result.str();
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
  stringstream result;
  result << "szg-rp success: camera modified.\n";
  return result.str();
}

string handleMotionCull(const string& messageBody){
  stringstream result;
  result << "szg-rp success: ";
  if (messageBody == "on"){
    useMotionCull = true;
    result << "motion cull on.\n";
  }
  else{
    useMotionCull = false;
    result << "motion cull off.\n";
  }
  return result.str();
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  // all of the messages to szg-rp receive responses
  string responseBody;
  while (true) {
    int messageID = cli->receiveMessage(&messageType,&messageBody);
    if (!messageID){
      // This should only occur when the szgserver quits.
      ar_log_error() << "szg-rp error: problem in receiving message.\n";
      exit(0);
    }
    if (messageType == "quit"){
      exit(0);
    }
    else if (messageType == "performance"){
      if (messageBody == "on"){
	showPerformance = true;
      }
      if (messageBody == "off"){
	showPerformance = false;
      }
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
      ar_mutex_lock(&peerLock);
      // By convention, the messageBody will be peer_name/stuff (though /stuff
      // is optional). The following call seperates out the peer name and
      // strips messageBody in place.
      string peerName = ar_graphicsPeerStripName(messageBody);
      PeerContainer::iterator i = peers.find(peerName);
      if (i == peers.end()){
        responseBody = string("szg-rp error: failed to find specified local ")
                       + string("peer.\n  (") + peerName
                       + string(")\n");
        ar_mutex_unlock(&peerLock);
      }
      else{
        arGraphicsPeer* temp = i->second.peer;
        ar_mutex_unlock(&peerLock);
        responseBody = ar_graphicsPeerHandleMessage(temp, 
                                                    messageType, 
                                                    messageBody);
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
  ar_mutex_lock(&peerLock);
  // first pass to draw the peers
  // We keep track of the time it takes to read in the data
  // and draw that as well.
  double dataTime = 0;
  int dataAmount = 0;
  for (i = peers.begin(); i != peers.end(); i++){
    ar_timeval time1 = ar_time();
    dataAmount += i->second.peer->consume();
    dataTime += ar_difftime(ar_time(), time1)/1000.0;
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
  ar_mutex_unlock(&peerLock);
  // It seems like a good idea to throttle szg-rp artificially here.
  // After all, this workspace will be just *one* part of an overall
  // workflow. Whereas default throttling is bad in the dedicated 
  // display case, it makes sense here.
  // The command line option -p can override this setting.
  if (!highPerformance){
    ar_usleep(10000);
  }
  glutSwapBuffers();

  // It seems more advantageous to send the frame time back every
  // frame instead of more occasionally. This leads to a tighter
  // feedback loop. Still experimenting, though.
  int frametime = 50000; // a sensible default.
  if (!timeInit){
    timeInit = true;
    time1 = ar_time();
  }
  else{
    ar_timeval temp = time1;
    time1 = ar_time();
    frametime = int(ar_difftime(time1, temp)) ;
  }

  ar_mutex_lock(&peerLock);
  for (i=peers.begin(); i!=peers.end(); i++){
    i->second.peer->broadcastFrameTime(averageFrameTime(frametime));
  }
  arPerformanceElement* framerateElement 
    = framerateGraph.getElement("framerate");
  framerateElement->pushNewValue(1000000.0/frametime);
  framerateElement = framerateGraph.getElement("consume");
  framerateElement->pushNewValue(dataTime);
  framerateElement = framerateGraph.getElement("bytes");
  framerateElement->pushNewValue(dataAmount);
  ar_mutex_unlock(&peerLock);
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
    case 'P':
      showPerformance = !showPerformance;
      break;
  }
}

int main(int argc, char** argv){

  framerateGraph.addElement("framerate", 300, 100, arVector3(1,1,1));
  framerateGraph.addElement("consume", 300, 100, arVector3(1,1,0));
  framerateGraph.addElement("bytes", 300, 500000, arVector3(0,1,1));
  
  globalCamera.setSides(-0.1, 0.1, -0.1, 0.1);
  globalCamera.setNearFar(0.2, 200);
  globalCamera.setPosition(0,5,20);
  globalCamera.setTarget(0,5,0);
  globalCamera.setUp(0,1,0);

  szgClient = new arSZGClient();
  ar_mutex_init(&peerLock);
  szgClient->simpleHandshaking(false);
  szgClient->init(argc, argv);
  if (!(*szgClient)){
    return 1;
  }
  ar_log().setStream(szgClient->initResponse());
  // Check for the flag indicating that "performance" is desired.
  for (int i=0; i<argc; i++){
    if (!strcmp("-p",argv[i])){
      highPerformance = true;
      // Go ahead and strip on the arg.
      for (int j=i; j<argc-1; j++){
        argv[j] = argv[j+1];
      }
      argc--;
      i--;
    }
  }
  if (argc < 2){
    ar_log_critical() << "szg-rp usage: szg-rp <name>\n";
    if (!szgClient->sendInitResponse(false)){
      ar_log_error() << "szg-rp error: maybe szgserver died.\n";
    }
    return 1;
  }
  ar_log_remark() << "szg-rp remark: trying to run as peer=" << argv[1] << "\n";
  if (!szgClient->sendInitResponse(true)){
    ar_log_error() << "szg-rp error: maybe szgserver died.\n";
  }
  // Use locks to ensure that we have a unique workspace name.
  int componentID;
  ar_log().setStream(szgClient->startResponse());
  if (!szgClient->getLock(string("szg-rp-")+argv[1], componentID)){
    ar_log_error() << "szg-rp error: non-unique workspace name.\n"
		   << "  component with ID " << componentID 
                   << " holds this workspace.\n";
    if (!szgClient->sendStartResponse(false)){
      ar_log_error() << "szg-rp error: maybe szgserver died.\n";
    }
    return 1;
  }
  if (!loadParameters(*szgClient)){
    ar_log_error() << "szg-rp error: failed to load parameters.\n";
    if (!szgClient->sendStartResponse(false)){
      ar_log_error() << "szg-rp error: maybe szgserver died.\n";
    }
  }

  ar_log_remark() << "szg-rp remark: started.\n";
  if (!szgClient->sendStartResponse(true)){
    ar_log_error() << "szg-rp error: maybe szgserver died.\n";
  }
  
  ar_log().setStream(cout);

  arThread messageThread(messageTask, szgClient);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(xPos,yPos);
  glutInitWindowSize(xSize, ySize);
  glutCreateWindow("Syzygy Reality Peer");
  if (useFullscreen){
    glutFullScreen();
  }
  glutSetCursor(GLUT_CURSOR_NONE);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutMainLoop();
}
