//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsPeer.h"
#include "arFramerateGraph.h"

// NOTE: arSZGClient CANNOT BE GLOBAL ON WINDOWS. THIS IS BECAUSE 
// OF OUR METHOD OF INITIALIZING WINSOCK (USING GLOBALS).
arSZGClient* szgClient;

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
arMatrix4 translationMatrix;
string peerTexturePath, peerTextPath;

// The camera can be moved.
arPerspectiveCamera globalCamera;

arGraphicsPeer* createNewPeer(const string& name){
  arGraphicsPeer* graphicsPeer = new arGraphicsPeer();
  graphicsPeer->setName(name);
  graphicsPeer->useLocalDatabase(true);
  graphicsPeer->queueData(true);
  graphicsPeer->init(*szgClient);
  if (!graphicsPeer->start()){
    cerr << "szg-rp error: init of peer failed.\n";
    return NULL;
  }
  graphicsPeer->setTexturePath(peerTexturePath);
  graphicsPeer->loadAlphabet(peerTextPath.c_str());
  return graphicsPeer;
}

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
  if (posString != "NULL"
      && ar_parseIntString(posString,sizeBuffer,2)){
    xPos = sizeBuffer[0];
    yPos = sizeBuffer[1];
  }
  else{
    xPos = 0;
    yPos = 0;
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
  PeerContainer::iterator i;
  result << "szg-rp success: peer list follows:\n\n";
  for (i = peers.begin(); i != peers.end(); i++){
    result << "peer name = " << i->first << "\n"
	   << "comment = " << i->second.comment << "\n";
    result << i->second.peer->printConnections() << "\n\n";  
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
  float temp[15];
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

// The next block of message handlers really just relay messages
// to the appropriate peers.

// messageBody is ignored.
string ar_graphicsPeerHandleReset(arGraphicsPeer* peer, 
                                  const string&){
  stringstream result;
  peer->closeAllAndReset();
  result << "szg-rp success: reset peer " << peer->getName() << ".\n";
  return result.str();
}

// messageBody = connect_to_peer_name
string ar_graphicsPeerHandleConnect(arGraphicsPeer* peer, 
                                    const string& messageBody){
  stringstream result;
  int connectionID = peer->connectToPeer(messageBody);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers connected (" << connectionID << ").\n";
  return result.str();
}

// BUG BUG BUG BUG BUG BUG BUG BUG! This is really dealing with *two* messages,
// serialize and afterwards suck new stuff in... and serialize and afterwards
// do not.
// Format = connect_to_peer_name 
string ar_graphicsPeerHandleConnectDump(arGraphicsPeer* peer, 
                                        const string& messageBody, 
                                        bool relayOn){
  stringstream result;
  int connectionID = peer->connectToPeer(messageBody);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" <<  messageBody << ")\n";
    return result.str();
  }
  peer->pullSerial(messageBody, relayOn);
  result << "szg-rp success: connect-dump succeeded (" << connectionID 
         << ").\n";
  return result.str();
}

// Again, really two messages.
// Format = push_to_peer_name
string ar_graphicsPeerHandlePushSerial(arGraphicsPeer* peer, 
                                       const string& messageBody, bool sendOn){
  stringstream result;
  if (!peer->pushSerial(messageBody, 0, sendOn)){
    result << "szg-rp error: failed to send serialization to named peer.\n";
  }
  else{
    result << "szg-rp success: push serial succeeded.\n";
  }
  return result.str();
}

// Format = disconnect_from_peer_name
string ar_graphicsPeerHandleDisconnect(arGraphicsPeer* peer, 
                                       const string& messageBody){
  stringstream result;
  int socketID = peer->closeConnection(messageBody);
  if (socketID < 0){
    cerr << "szg-rp error: failed to close connection to remote peer.\n"
	 << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers disconnected.\n";
  return result.str();
}

// Again, really two messages.
// Format = receive_from_peer_name
string ar_graphicsPeerHandleRelay(arGraphicsPeer* peer, 
                                  const string& messageBody, bool state){
  stringstream result;
  if (!peer->receiving(messageBody, state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  if (state){
    result << "szg-rp success: relay is on.\n";
  }
  else{
    result << "szg-rp success: relay is off.\n";
  }
  return result.str();
}

// Again, two messages.
// Format = send_to_peer_name
string ar_graphicsPeerHandleSend(arGraphicsPeer* peer, 
                                 const string& messageBody, bool state){
  stringstream result;
  if (!peer->sending(messageBody, state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  if (state){
    result << "szg-rp success: send is on.\n";
  }
  else{
    result << "szg-rp success: send is off.\n";
  }
  return result.str();
}

// messageBody is unused.
string ar_graphicsPeerHandlePrint(arGraphicsPeer* peer, 
                                  const string&){
  stringstream result;
  result << "szg-rp success:\n";
  result << peer->print();
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleLoad(arGraphicsPeer* peer, 
                                 const string& messageBody, bool useXML){
  stringstream result;
  bool state = false;
  if (useXML){
    state = peer->readDatabaseXML(messageBody);
  }
  else{
    state = peer->readDatabase(messageBody);
  }
  if (!state){
    result << "szg-rp error: failed to load specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database loaded.\n";
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleSave(arGraphicsPeer* peer, 
                                 const string& messageBody, bool useXML){
  stringstream result;
  bool state = false;
  if (useXML){
    state = peer->writeDatabaseXML(messageBody);
  }
  else{
    state = peer->writeDatabase(messageBody);
  }
  if (!state){
    result << "szg-rp error: failed to save specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database saved.\n";
  return result.str();
}

// Two messages!
// Lets us lock a node in a remote peer to us!
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleLock(arGraphicsPeer* peer, 
                                 const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->lockRemoteNode(bodyList[1], ID);
  }
  else{
    state = peer->unlockRemoteNode(bodyList[1], ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[1] << "\n";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node locked.\n";
  }
  else{
    result << "szg-rp success: node unlocked.\n";
  }
  return result.str();
}

// Two messages!
// It lets us lock a local node to a particular peer. Dual of handleLock.
// Format = remote_peer_name
string ar_graphicsPeerHandleLockLocal(arGraphicsPeer* peer, 
                                      const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->lockLocalNode(bodyList[1], ID);
  }
  else{
    // NOTE: the second parameter is meaningless in this case.
    state = peer->unlockLocalNode(ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[1] << "\n";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node locked.\n";
  }
  else{
    result << "szg-rp success: node unlocked.\n";
  }
  return result.str();
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleDataFilter(arGraphicsPeer* peer, 
                                       const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be remote peer,"
	   << "local node ID, filter code.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  stringstream valueStream;
  int value;
  valueStream << bodyList[3];
  valueStream >> value;
  peer->filterDataBelowRemote(bodyList[1], ID, value);
  result << "szg-rp success: filter applied.\n";
  return result.str();
}

// Format = remote_peer_name/remote_node_name
string ar_graphicsPeerHandleNodeID(arGraphicsPeer* peer, 
                                   const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node name.\n";
    return result.str();
  }
  result << "szg-rp success: node ID =\n  "
	 << peer->getNodeIDRemote(bodyList[1], bodyList[2]);
  return result.str();
}

string ar_graphicsPeerHandleMessage(arGraphicsPeer* peer,
                                    const string& messageType,
				    const string& messageBody){
  string responseBody;

  if (messageType == "connect"){
    responseBody = ar_graphicsPeerHandleConnect(peer,messageBody);
  }
  else if (messageType == "connect-dump"){
    responseBody = ar_graphicsPeerHandleConnectDump(peer, messageBody, false);
  }
  else if (messageType == "connect-dump-relay"){
    responseBody = ar_graphicsPeerHandleConnectDump(peer, messageBody, true);
  }
  else if (messageType == "push-serial"){
    responseBody = ar_graphicsPeerHandlePushSerial(peer, messageBody, false);
  }
  else if (messageType == "push-serial-send"){
    responseBody = ar_graphicsPeerHandlePushSerial(peer, messageBody, true);
  }
  else if (messageType == "disconnect"){
    responseBody = ar_graphicsPeerHandleDisconnect(peer, messageBody);
  }
  else if (messageType == "relay-on"){
    responseBody = ar_graphicsPeerHandleRelay(peer, messageBody, true);
  }
  else if (messageType == "relay-off"){
    responseBody = ar_graphicsPeerHandleRelay(peer, messageBody, false);
  }
  else if (messageType == "send-on"){
    responseBody = ar_graphicsPeerHandleSend(peer, messageBody, true);
  }
  else if (messageType == "send-off"){
    responseBody = ar_graphicsPeerHandleSend(peer, messageBody, false);
  }
  else if (messageType == "reset"){
    responseBody = ar_graphicsPeerHandleReset(peer, messageBody);
  }
  else if (messageType == "print"){
    responseBody = ar_graphicsPeerHandlePrint(peer, messageBody);
  }
  else if (messageType == "load"){
    responseBody = ar_graphicsPeerHandleLoad(peer, messageBody, false);
  }
  else if (messageType == "load-xml"){
    responseBody = ar_graphicsPeerHandleLoad(peer, messageBody, true);
  }
  else if (messageType == "save"){
    responseBody = ar_graphicsPeerHandleSave(peer, messageBody, false);
  }
  else if (messageType == "save-xml"){
    responseBody = ar_graphicsPeerHandleSave(peer, messageBody, true);
  }
  else if (messageType =="lock"){
    responseBody = ar_graphicsPeerHandleLock(peer, messageBody, true);
  }
  else if (messageType == "unlock"){
    responseBody = ar_graphicsPeerHandleLock(peer, messageBody, false);
  } 
  else if (messageType == "lock-local"){
    responseBody = ar_graphicsPeerHandleLockLocal(peer, messageBody, true);
  }
  else if (messageType == "unlock-local"){
    responseBody = ar_graphicsPeerHandleLockLocal(peer, messageBody, false);
  }
  else if (messageType == "filter_data"){
    responseBody = ar_graphicsPeerHandleDataFilter(peer, messageBody);
  }
  else if (messageType == "node-ID"){
    responseBody = ar_graphicsPeerHandleNodeID(peer, messageBody);
  }
  else{
    responseBody = string("szg-rp error: unknown message type.\n");
  }
  return responseBody;
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  // all of the messages to szg-rp receive responses
  string responseBody;
  while (true) {
    int messageID = cli->receiveMessage(&messageType,&messageBody);
    if (!messageID){
      cerr << "szg-rp error: problem in receiving message.\n";
      continue;
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
    // This ends the messages specifically sent to the szg-rp.
    else{
      ar_mutex_lock(&peerLock);
      // By convention, the messageBody will be peer_name/stuff (though /stuff
      // is optional).
      string peerName;
      string actualMessageBody;
      unsigned int position = messageBody.find('/');
      if (position == string::npos){
        // Just the peer name.
        peerName = messageBody;
        actualMessageBody = "";
      }
      else{
        peerName = messageBody.substr(0, position);
        if (messageBody.length() > position+1){
          actualMessageBody 
            = messageBody.substr(position+1, messageBody.length()-position-1);
        }
        else{
          actualMessageBody = "";
        }
      }
      PeerContainer::iterator i = peers.find(peerName);
      if (i == peers.end()){
        responseBody = string("szg-rp error: failed to find specified local ")
                       + string("peer.\n  (") + peerName
                       + string(")\n");
        ar_mutex_unlock(&peerLock);
      }
      arGraphicsPeer* temp = i->second.peer;
      ar_mutex_unlock(&peerLock);
      responseBody = ar_graphicsPeerHandleMessage(temp, 
                                                  messageType, 
                                                  actualMessageBody);
    }
    // return the message response.
    if (!cli->messageResponse(messageID, responseBody)){
      cerr << "szg-rp error: message response failed.\n";
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

void display(){
  static int frameSkip = 0;
  ar_timeval time1 = ar_time();
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
    i->second.peer->draw();
    glPopMatrix();
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
  if (true || frameSkip == 20){
    int frametime = int(ar_difftime(ar_time(), time1));
    ar_mutex_lock(&peerLock);
    for (i=peers.begin(); i!=peers.end(); i++){
      i->second.peer->broadcastFrameTime(frametime);
    }
    arPerformanceElement* framerateElement 
      = framerateGraph.getElement("framerate");
    framerateElement->pushNewValue(1000000.0/frametime);
    framerateElement = framerateGraph.getElement("consume");
    framerateElement->pushNewValue(dataTime);
    framerateElement = framerateGraph.getElement("bytes");
    framerateElement->pushNewValue(dataAmount);
    ar_mutex_unlock(&peerLock);
    frameSkip = 0;
  }
  frameSkip++;
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
  stringstream& initResponse = szgClient->initResponse();
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
    initResponse << "szg-rp usage: szg-rp <name>\n";
    szgClient->sendInitResponse(false);
    return 1;
  }
  initResponse << "szg-rp remark: trying to run as peer=" << argv[1] << "\n";
  szgClient->sendInitResponse(true);
  // Use locks to ensure that we have a unique workspace name.
  int componentID;
  stringstream& startResponse = szgClient->startResponse();
  if (!szgClient->getLock(string("szg-rp-")+argv[1], componentID)){
    startResponse << "szg-rp error: workspace name must be unique.\n"
                  << "  component with ID " << componentID 
                  << " holds this workspace.\n";
    szgClient->sendStartResponse(false);
    return 1;
  }
  if (!loadParameters(*szgClient)){
    startResponse << "szg-rp error: failed to load parameters.\n";
    szgClient->sendStartResponse(false);
  }

  startResponse << "szg-rp remark: start succeeded.\n";
  szgClient->sendStartResponse(true);

  arThread messageThread(messageTask, szgClient);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(xPos,yPos);
  if (xSize>0 && ySize>0){
    glutInitWindowSize(xSize,ySize);
    glutCreateWindow("Syzygy Reality Peer");
  }
  else{
    glutInitWindowSize(640,480);
    glutCreateWindow("Syzygy Reality Peer");
    glutFullScreen();
  }
  glutSetCursor(GLUT_CURSOR_NONE);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutMainLoop();
}
