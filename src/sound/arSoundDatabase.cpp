//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundDatabase.h"
#include "arStreamNode.h"

arSoundDatabase::arSoundDatabase() :
  _path(new list<string>)
{
  _lang = (arDatabaseLanguage*)&_langSound;
  if (!_initDatabaseLanguage())
    return;
  
  // Initialize the path list.
  _path->push_back(string("")); // local directory

  ar_mutex_init(&_pathLock);

  // make the external parsing storage
  arTemplateDictionary* d = _langSound.getDictionary();
  transformData = new arStructuredData(d, "transform");
  filewavData = new arStructuredData(d, "fileWav");
  playerData = new arStructuredData(d, "player");
  speechData = new arStructuredData(d, "speech");
  streamData = new arStructuredData(d, "stream");
  if (!transformData || !*transformData ||
      !filewavData   || !*filewavData   ||
      !playerData    || !*playerData    ||
      !speechData    || !*speechData    ||
      !streamData    || !*streamData){
    cerr << "arSoundDatabase error: incorrect dictionary.\n";
  }
}

arSoundDatabase::~arSoundDatabase(){
  if (transformData){
    delete transformData;
  }
  if (filewavData){
    delete filewavData;
  }
  if (playerData){
    delete playerData;
  }
  if (speechData){
    delete speechData;
  }
  if (streamData){
    delete streamData;
  }
}

arDatabaseNode* arSoundDatabase::alter(arStructuredData* inData){
  return arDatabase::alter(inData);
}

void arSoundDatabase::reset(){
  // first, call base class to do that cleaning
  arDatabase::reset();

  // Delete wavfiles.
  for (map<string,arSoundFile*,less<string> >::iterator
        i(_filewavNameContainer.begin());
       i != _filewavNameContainer.end();
       ++i){
    delete i->second;
  }
  _filewavNameContainer.clear();
}

// We return the whole path.
string arSoundDatabase::getPath(){
  if (_path->empty()){
    return string("NULL");
  }
  string result;
  list<string>::iterator i;
  for (i=_path->begin(); i!=_path->end(); i++){
    if (*i != ""){
      result += *i; 
      result += ";"; 
    }
  }
  return result;
}
    
// Only arSoundClient, not arSoundServer, should ever call setPath().
void arSoundDatabase::setPath(const string& thePath){
  // this is probably called in a different thread from the data handling
  ar_mutex_lock(&_pathLock);

  delete _path;
  _path = new list<string>;

  // Parse the path.
  int nextChar = 0;
  int length = thePath.length();

  string result(""); // always search local directory
  _path->push_back(result);

  int cdir = 0;
  while (nextChar < length){
    result = ar_pathToken(thePath, nextChar); // updates nextChar
    if (result == "NULL")
      continue;
    ++cdir;
    _path->push_back(ar_pathAddSlash(result));
  }
  if (cdir <= 0)
    cerr << "arSoundDatabase warning: empty path (from SZG_SOUND/path).\n";

  ar_mutex_unlock(&_pathLock);
}

// Only the client, not the server, tries to
// load the soundfiles (or even verify that the files exist --
// client and server may be different machines with different disks mounted).

// Only arSoundClient, not arSoundServer, should ever call addFile().
arSoundFile* arSoundDatabase::addFile(const string& name, bool fLoop){
  // If our arDatabase is a server, not a client, then we don't do anything
  // with these files.
  if (_server)
    return NULL;

  char buffer[512];
  ar_stringToBuffer(name, buffer, sizeof(buffer));
  const map<string,arSoundFile*,less<string> >::iterator
    iFind(_filewavNameContainer.find(name));
  if (iFind != _filewavNameContainer.end()){
    return iFind->second;
  }

  // A new filewav.
  arSoundFile* theFile = new arSoundFile;

  // Try everything in the path.
  ar_mutex_lock(&_pathLock);
  char fileNameBuffer[512];
  bool fDone = false;
  for (list<string>::iterator i = _path->begin();
       !fDone && i != _path->end();
       ++i){
    const string tmp(*i + buffer);
    ar_stringToBuffer(tmp, fileNameBuffer, sizeof(fileNameBuffer));
    fDone = theFile->read(fileNameBuffer, fLoop);
  }
  static bool fComplained = false;
  if (!fDone){
    if (!theFile->dummy()) {
      cerr << "arSoundDatabase error: failed to create dummy sound.\n";
    }
    if (!fComplained){
      fComplained = true;
      cerr << "arSoundDatabase warning: soundfile \""
	   << buffer << "\" not found in ";
      if (_path->size() <= 1)
        cerr << "empty ";
      cerr << "path.\n";
    }
  }
  ar_mutex_unlock(&_pathLock);
  _filewavNameContainer.insert(
    map<string,arSoundFile*,less<string> >::value_type(name,theFile));
  return theFile; 
}

void arSoundDatabase::setPlayTransform(arSpeakerObject* s){
  arMatrix4 headMatrix(ar_identityMatrix());
  float unitConversion = 1.;
  arDatabaseNode* playerNode = getNode("szg_player", false);
  if (playerNode){
    // Get info from the database's player node.
    arStructuredData* pdata = playerNode->dumpData();
    pdata->dataOut(_langSound.AR_PLAYER_MATRIX,headMatrix.v,AR_FLOAT,16);
    pdata->dataOut(_langSound.AR_PLAYER_UNIT_CONVERSION,
                   &unitConversion,AR_FLOAT,1);
    delete pdata; // Because dumpData() allocated it.
  }
  s->setUnitConversion(unitConversion);

  // This call to loadMatrices() happens once per audio-frame,
  // 5x more common than arServerFramework::setPlayTransform 
  // calls loadMatrices.
  s->loadMatrices(headMatrix);
}

// Copy of OpenGL's matrix stack, for sound rendering.
stack<arMatrix4, deque<arMatrix4> > ar_transformStack;

void arSoundDatabase::render(){
  ar_transformStack.push(ar_identityMatrix());
  ar_mutex_lock(&_eraseLock);
  _render((arSoundNode*)&_rootNode);
  ar_mutex_unlock(&_eraseLock);
  ar_transformStack.pop();
}

void arSoundDatabase::_render(arSoundNode* node){
  arMatrix4 tempMatrix;
  if (node->getTypeCode() == AR_S_TRANSFORM_NODE){
    // Push current onto the matrix stack.
    ar_transformStack.push(ar_transformStack.top());
  }
  if (node->getID() != 0){
    // We are not the root node, so it is OK to render.
    node->render();
  }
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _render((arSoundNode*)(*i));
  }
  if (node->getTypeCode() == AR_S_TRANSFORM_NODE){
    // Pop from stack.
    ar_transformStack.pop();
  }
}

arDatabaseNode* arSoundDatabase::_makeNode(const string& type){
  arDatabaseNode* outNode = NULL;
  if (type=="transform"){
    outNode = (arDatabaseNode*) new arSoundTransformNode();
  }
  else if (type=="fileWav"){
    outNode = (arDatabaseNode*) new arSoundFileNode();
  }
  else if (type=="player"){
    outNode = (arDatabaseNode*) new arPlayerNode();
  }
  else if (type=="speech"){
    outNode = (arDatabaseNode*) new arSpeechNode();
  }
  else if (type=="stream"){
    outNode = (arDatabaseNode*) new arStreamNode();
  }
  else{
    cerr << "arSoundDatabase error: makeNode factory got unknown type="
	 << type << ".\n";
    return NULL;
  }
  return outNode;
}


 
