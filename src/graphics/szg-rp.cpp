//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsPeer.h"

// NOTE: arSZGClient CANNOT BE GLOBAL ON WINDOWS. THIS IS BECAUSE 
// OF OUR METHOD OF INITIALIZING WINSOCK (USING GLOBALS).
arSZGClient* szgClient;

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
  if (sizeString != "NULL"
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

string handleReset(const string& messageBody){
  stringstream result;
  // MUST BE ATOMIC WITH RESPECT TO DRAWING
  ar_mutex_lock(&peerLock);
  arSlashString bodyList(messageBody);
  // THERE IS A BUG IN arSlashString WHEREBY A STRING WITHOUT / IS
  // NOT GIVEN THE LENGTH OF 1! 
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    ar_mutex_unlock(&peerLock);
    return result.str();
  }
  arGraphicsPeer* temp = i->second.peer;
  ar_mutex_unlock(&peerLock);
  temp->closeAllAndReset();
  result << "szg-rp success: reset peer " << bodyList[0] << ".\n";
  return result.str();
}

string handleConnect(const string& messageBody){
  stringstream result;
  // connect to a running database
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer .\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  int connectionID = i->second.peer->connectToPeer(bodyList[1]);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" << bodyList[1] << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers connected (" << connectionID << ").\n";
  return result.str();
}

string handleConnectDump(const string& messageBody, bool relayOn){
  stringstream result;
  // COPY-PASTE FROM handleConnect(...)
  // connect to a running database
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  int connectionID = i->second.peer->connectToPeer(bodyList[1]);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" << bodyList[1] << ")\n";
    return result.str();
  }
  i->second.peer->pullSerial(bodyList[1], relayOn);
  result << "szg-rp success: connect-dump succeeded (" << connectionID 
         << ").\n";
  return result.str();
}

string handlePushSerial(const string& messageBody, bool sendOn){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  if (!i->second.peer->pushSerial(bodyList[1], sendOn)){
    result << "szg-rp error: failed to send serialization to named peer.\n";
  }
  else{
    result << "szg-rp success: push serial succeeded.\n";
  }
  return result.str();
}

string handleDisconnect(const string& messageBody){
  stringstream result;
  // connect to a running database
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer .\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  int socketID = i->second.peer->closeConnection(bodyList[1]);
  if (socketID < 0){
    cerr << "szg-rp error: failed to close connection to remote peer.\n"
	 << "  (" << bodyList[1] << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers disconnected.\n";
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

string handleRelay(const string& messageBody, bool state){
  stringstream result;
  // connect to a running database
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  if (!i->second.peer->receiving(bodyList[1],state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << bodyList[1] << ")\n";
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

string handleSend(const string& messageBody, bool state){
  stringstream result;
  // connect to a running database
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: need local/remote.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  if (!i->second.peer->sending(bodyList[1],state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << bodyList[1] << ")\n";
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

string handlePrint(const string& messageBody){
  stringstream result;
  PeerContainer::iterator i = peers.find(messageBody);
  if (i == peers.end()){
    result << "szg-rp error: failed to find specified local peer.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success:\n";
  result << i->second.peer->print();
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

string handleLoad(const string& messageBody, bool useXML){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be local peer followed by "
	   << "file name.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  bool state = false;
  if (useXML){
    state = i->second.peer->readDatabaseXML(bodyList[1]);
  }
  else{
    state = i->second.peer->readDatabase(bodyList[1]);
  }
  if (!state){
    result << "szg-rp error: failed to load specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database loaded.\n";
  return result.str();
}

string handleSave(const string& messageBody, bool useXML){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be local peer followed by "
	   << "file name.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  bool state = false;
  if (useXML){
    state = i->second.peer->writeDatabaseXML(bodyList[1]);
  }
  else{
    state = i->second.peer->writeDatabase(bodyList[1]);
  }
  if (!state){
    result << "szg-rp error: failed to save specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database saved.\n";
  return result.str();
}

string handleLock(const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be local peer followed by "
	   << "remote peer followed by node ID.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = i->second.peer->lockRemoteNode(bodyList[1], ID);
  }
  else{
    state = i->second.peer->unlockRemoteNode(bodyList[1], ID);
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

string handleLockLocal(const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be local peer followed by "
	   << "remote peer followed by node ID.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = i->second.peer->lockLocalNode(bodyList[1], ID);
  }
  else{
    // NOTE: the second parameter is meaningless in this case.
    state = i->second.peer->unlockLocalNode(ID);
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

string handleDataFilter(const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be local peer, remote peer,"
	   << "local node ID, filter code.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
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
  i->second.peer->filterDataBelowRemote(bodyList[1], ID, value);
  result << "szg-rp success: filter applied.\n";
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

string handleNodeID(const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be local peer followed by "
	   << "remote peer followed by node name.\n";
    return result.str();
  }
  PeerContainer::iterator i = peers.find(bodyList[0]);
  if (i == peers.end()){
    result << "szg-rp error: no such named local peer.\n"
	   << "  (" << bodyList[0] << ")\n";
    return result.str();
  }
  result << "szg-rp success: node ID =\n  "
	 << i->second.peer->getNodeIDRemote(bodyList[1], bodyList[2]);
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
      cerr << "szg-rp error: problem in receiving message.\n";
      continue;
    }
    if (messageType == "quit"){
      exit(0);
    }
    else if (messageType == "create"){
      responseBody = handleCreate(messageBody);
    }
    else if (messageType == "delete"){
      responseBody = handleDelete(messageBody);
    }
    else if (messageType == "connect"){
      responseBody = handleConnect(messageBody);
    }
    else if (messageType == "connect-dump"){
      responseBody = handleConnectDump(messageBody, false);
    }
    else if (messageType == "connect-dump-relay"){
      responseBody = handleConnectDump(messageBody, true);
    }
    else if (messageType == "push-serial"){
      responseBody = handlePushSerial(messageBody, false);
    }
    else if (messageType == "push-serial-send"){
      responseBody = handlePushSerial(messageBody, true);
    }
    else if (messageType == "disconnect"){
      responseBody = handleDisconnect(messageBody);
    }
    else if (messageType == "translate"){
      responseBody = handleTranslation(messageBody);
    }
    else if (messageType == "relay-on"){
      responseBody = handleRelay(messageBody, true);
    }
    else if (messageType == "relay-off"){
      responseBody = handleRelay(messageBody, false);
    }
    else if (messageType == "send-on"){
      responseBody = handleSend(messageBody, true);
    }
    else if (messageType == "send-off"){
      responseBody = handleSend(messageBody, false);
    }
    else if (messageType == "reset"){
      responseBody = handleReset(messageBody);
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
    else if (messageType == "print"){
      responseBody = handlePrint(messageBody);
    }
    else if (messageType == "label-translate"){
      responseBody = handleLabelTranslate(messageBody);
    }
    else if (messageType == "label"){
      responseBody = handleLabel(messageBody);
    }
    else if (messageType == "load"){
      responseBody = handleLoad(messageBody, false);
    }
    else if (messageType == "load-xml"){
      responseBody = handleLoad(messageBody, true);
    }
    else if (messageType == "save"){
      responseBody = handleSave(messageBody, false);
    }
    else if (messageType == "save-xml"){
      responseBody = handleSave(messageBody, true);
    }
    else if (messageType =="lock"){
      responseBody = handleLock(messageBody, true);
    }
    else if (messageType == "unlock"){
      responseBody = handleLock(messageBody, false);
    } 
    else if (messageType == "lock-local"){
      responseBody = handleLockLocal(messageBody, true);
    }
    else if (messageType == "unlock-local"){
      responseBody = handleLockLocal(messageBody, false);
    }
    else if (messageType == "filter_data"){
      responseBody = handleDataFilter(messageBody);
    }
    else if (messageType == "camera"){
      responseBody = handleCamera(messageBody);
    }
    // Hmmm... this one doesn't seem quite so necessary since we are
    // downloading copies (plus node names) anyway. Also, probably 
    // want to get away from using node names...
    else if (messageType == "node-ID"){
      responseBody = handleNodeID(messageBody);
    }
    else{
      responseBody = string("szg-rp error: unknown message type.\n");
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
  for (i = peers.begin(); i != peers.end(); i++){
    //ar_timeval time1 = ar_time();
    i->second.peer->consume();
    //ar_timeval time2 = ar_time();
    glPushMatrix();
    glMultMatrixf(i->second.transform.v);
    i->second.peer->activateLights();
    i->second.peer->draw();
    glPopMatrix();
    //ar_timeval time3 = ar_time();
    //cout << "process time = " << ar_difftime(time2, time1)
    //	 << "draw time = " << ar_difftime(time3, time2) << "\n";
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
  ar_mutex_unlock(&peerLock);
  // It seems like a good idea to throttle szg-rp artificially here.
  // After all, this workspace will be just *one* part of an overall
  // workflow. Whereas default throttling is bad in the dedicated 
  // display case, it makes sense here.
  ar_usleep(10000);
  glutSwapBuffers();
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
