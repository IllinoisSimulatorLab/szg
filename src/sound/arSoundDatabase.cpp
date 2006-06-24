//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundDatabase.h"
#include "arStreamNode.h"

arSoundDatabase::arSoundDatabase() :
  _path(new list<string>)
{
  // todo: initializers.
  _typeCode = AR_SOUND_DATABASE;
  _typeString = "sound";
  
  _lang = (arDatabaseLanguage*)&_langSound;
  if (!_initDatabaseLanguage())
    return;
  // Have to add the processing callback for the "sound admin"
  // message.
  arDataTemplate* t = _lang->find("sound_admin");
  _databaseReceive[t->getID()] 
    = (arDatabaseProcessingCallback)&arSoundDatabase::_processAdmin;
  
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

arDatabaseNode* arSoundDatabase::alter(arStructuredData* inData,
                                       bool refNode){
  return arDatabase::alter(inData, refNode);
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
    ar_log_warning() << "empty SZG_SOUND/path.\n";

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

  std::vector<std::string> triedPaths;

  const map<string,arSoundFile*,less<string> >::iterator
    iFind(_filewavNameContainer.find(name));
  if (iFind != _filewavNameContainer.end()){
    return iFind->second;
  }

  // A new sound file.
  arSoundFile* theFile = new arSoundFile;
  
  bool fDone = false;
  string potentialFileName;
  // Look at the bundle path, if it's been set.
  map<string, string, less<string> >::iterator iter 
    = _bundlePathMap.find(_bundlePathName);
  if (_bundlePathName != "NULL" && _bundleName != "NULL"
      && iter != _bundlePathMap.end()){
    arSemicolonString bundlePath(iter->second);
    for (int n=0; n<bundlePath.size() && !fDone; n++){
      potentialFileName = bundlePath[n];
      ar_pathAddSlash(potentialFileName);
      potentialFileName += _bundleName;
      ar_pathAddSlash(potentialFileName);
      potentialFileName += name;
      ar_scrubPath(potentialFileName);
      triedPaths.push_back( potentialFileName );
      fDone = theFile->read(potentialFileName.c_str(), fLoop);
    }
  }

  // Try everything in the sound path.
  ar_mutex_lock(&_pathLock);
  for (list<string>::iterator i = _path->begin();
       !fDone && i != _path->end();
       ++i){
    potentialFileName = *i + name;
    ar_scrubPath(potentialFileName);
    triedPaths.push_back( potentialFileName );
    fDone = theFile->read(potentialFileName.c_str(), fLoop);
  }
  static bool fComplained = false;
  if (!fDone){
    if (!theFile->dummy()) {
      ar_log_error() << "arSoundDatabase: failed to create dummy sound.\n";
    }
    if (!fComplained){
      fComplained = true;
      ar_log_error() << "arSoundDatabase: no soundfile '"
	   << name << "'. Tried ";
      std::vector<std::string>::iterator iter;
      for (iter = triedPaths.begin(); iter != triedPaths.end(); ++iter) {
        ar_log_error() << *iter << " ";
      }
      ar_log_error() << ".\n";
    }
  }
  ar_mutex_unlock(&_pathLock);
  triedPaths.clear();
  _filewavNameContainer.insert(
    map<string,arSoundFile*,less<string> >::value_type(name,theFile));
  return theFile; 
}

void arSoundDatabase::setPlayTransform(arSpeakerObject* s){
  arMatrix4 headMatrix;
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

  // This happens once per sound-frame, 5x more often than
  // arServerFramework::setPlayTransform calls loadMatrices.
  (void)s->loadMatrices(headMatrix);
}

// Copy of OpenGL's matrix stack, for sound rendering.
stack<arMatrix4, deque<arMatrix4> > ar_transformStack;

bool arSoundDatabase::render(){
  ar_transformStack.push(ar_identityMatrix());
    //ar_mutex_lock(&_eraseLock);
      const bool ok = _render((arSoundNode*)&_rootNode);
    //ar_mutex_unlock(&_eraseLock);
  ar_transformStack.pop();
  return ok;
}

bool arSoundDatabase::_render(arSoundNode* node){
  bool ok = true;
  if (node->getTypeCode() == AR_S_TRANSFORM_NODE){
    // Push current onto the matrix stack.
    ar_transformStack.push(ar_transformStack.top());
  }
  if (node->getID() != 0){
    // We are not the root node, so it is OK to render.
    ok &= node->render();
  }
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    ok &= _render((arSoundNode*)*i);
  }
  if (node->getTypeCode() == AR_S_TRANSFORM_NODE){
    // Pop from stack.
    ar_transformStack.pop();
  }
  return ok;
}

arDatabaseNode* arSoundDatabase::_makeNode(const string& type){
  if (type=="transform")
    return (arDatabaseNode*) new arSoundTransformNode();
  if (type=="fileWav")
    return (arDatabaseNode*) new arSoundFileNode();
  if (type=="player")
    return (arDatabaseNode*) new arPlayerNode();
  if (type=="speech")
    return (arDatabaseNode*) new arSpeechNode();
  if (type=="stream")
    return (arDatabaseNode*) new arStreamNode();
  cerr << "arSoundDatabase error: makeNode factory got unknown type="
       << type << ".\n";
  return NULL;
}

arDatabaseNode* arSoundDatabase::_processAdmin(arStructuredData* data){
  const string name = data->getDataString("name");
  cout << "arSoundDatabase remark: using sound bundle " << name << "\n";
  const arSlashString bundleInfo(name);
  if (bundleInfo.size() == 2)
    setDataBundlePath(bundleInfo[0], bundleInfo[1]);
  else
    cerr << "arSoundDatabase error: garbled sound bundle identification.\n";
  return &_rootNode;
}
