//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundDatabase.h"
#include "arStreamNode.h"

arSoundDatabase::arSoundDatabase() :
  _renderMode(mode_fmod),
  _path(new list<string>(1, "") /* local dir */ )
{
  // todo: initializers.  (unless these two fail in red hat 8, like other nodes)
  _typeCode = AR_SOUND_DATABASE;
  _typeString = "sound";

  _lang = (arDatabaseLanguage*)&_langSound;
  if (!_initDatabaseLanguage())
    return;

  // Add the processing callback for the "sound admin" message.
  arDataTemplate* t = _lang->find("sound_admin");
  _databaseReceive[t->getID()] =
    (arDatabaseProcessingCallback)&arSoundDatabase::_processAdmin;

  // External storage for parsing.
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
      !streamData    || !*streamData) {
    ar_log_error() << "arSoundDatabase: incorrect dictionary.\n";
  }
}

arSoundDatabase::~arSoundDatabase() {
  if (transformData)
    delete transformData;
  if (filewavData)
    delete filewavData;
  if (playerData)
    delete playerData;
  if (speechData)
    delete speechData;
  if (streamData)
    delete streamData;
}

arDatabaseNode* arSoundDatabase::alter(arStructuredData* inData, bool refNode) {
  return arDatabase::alter(inData, refNode);
}

void arSoundDatabase::reset() {
  arDatabase::reset();

  // Delete wavfiles.
  for (map<string, arSoundFile*, less<string> >::iterator
        i(_filewavNameContainer.begin());
       i != _filewavNameContainer.end();
       ++i) {
    delete i->second;
  }
  _filewavNameContainer.clear();
}

string arSoundDatabase::getPath() const {
  arGuard _(_pathLock, "arSoundDatabase::getPath");
  if (_path->empty()) {
    return string("NULL");
  }

  string s;
  for (list<string>::const_iterator i = _path->begin(); i!=_path->end(); i++) {
    if (*i != "") {
      s += *i + ";";
    }
  }
  return s;
}

// Only arSoundClient, not arSoundServer, should ever call setPath().
void arSoundDatabase::setPath(const string& thePath) {
  // Parse the path.
  const int length = thePath.length();
  string dir; // always search local directory
  int cdir = 0;

  // probably called in a thread other than the data handling
  arGuard _(_pathLock, "arSoundDatabase::setPath");

  delete _path;
  _path = new list<string>(1, dir);
  for (int nextChar=0; nextChar < length; ) {
    dir = ar_pathToken(thePath, nextChar); // updates nextChar
    if (dir == "NULL")
      continue;
    ++cdir;
    _path->push_back(ar_pathAddSlash(dir));
  }
  if (cdir <= 0)
    ar_log_warning() << "empty SZG_SOUND/path.\n";
}

// Only clients, not the server, load soundfiles or even check that they exist.
// Client and server may be different machines, mounting different disks).

// Only arSoundClient, not arSoundServer, should ever call addFile().
arSoundFile* arSoundDatabase::addFile(const string& name, bool fLoop) {
  if (_server) {
    // arDatabase is a server.  Nothing to do.
    return NULL;
  }

  const map<string, arSoundFile*, less<string> >::const_iterator
    iFind(_filewavNameContainer.find(name));
  if (iFind != _filewavNameContainer.end()) {
    return iFind->second;
  }

  arSoundFile* theFile = new arSoundFile;
  bool fDone = false;
  string s; // potential filename
  vector<string> triedPaths;
  map<string, string, less<string> >::const_iterator iter(_bundlePathMap.find(_bundlePathName));
  if (_bundlePathName != "NULL" && _bundleName != "NULL" && iter != _bundlePathMap.end()) {
    // Bundle path.
    arSemicolonString bundlePath(iter->second);
    for (int n=0; n<bundlePath.size() && !fDone; n++) {
      s = bundlePath[n];
      ar_pathAddSlash(s);
      s += _bundleName;
      ar_pathAddSlash(s);
      s += name;
      ar_fixPathDelimiter(s);
      triedPaths.push_back( s );
      fDone = theFile->read(s.c_str(), fLoop, _renderMode);
    }
  }

  // Sound path.
  _pathLock.lock("arSoundDatabase::addFile");
  for (list<string>::const_iterator i = _path->begin(); i != _path->end() && !fDone; ++i) {
    s = *i + name;
    ar_fixPathDelimiter(s);
    triedPaths.push_back( s );
    fDone = theFile->read(s.c_str(), fLoop, _renderMode);
  }
  _pathLock.unlock();
  static bool fComplained = false;
  if (!fDone) {
    if (!theFile->dummy()) {
      ar_log_error() << "arSoundDatabase failed to create dummy sound.\n";
    }
    if (!fComplained) {
      fComplained = true;
      ar_log_error() << "arSoundDatabase: no soundfile '" << name << "'. Tried ";
      std::vector<std::string>::iterator iter;
      for (iter = triedPaths.begin(); iter != triedPaths.end(); ++iter) {
        ar_log_error() << *iter << " ";
      }
      ar_log_error() << ".\n";
    }
  }
  triedPaths.clear();
  _filewavNameContainer.insert(
    map<string, arSoundFile*, less<string> >::value_type(name, theFile));
  return theFile;
}

void arSoundDatabase::setPlayTransform(arSpeakerObject* s) {
  arDatabaseNode* n = getNode("szg_player", false);
  float unitConversion = 1.;
  arMatrix4 mHead;

  if (n) {
    arStructuredData* pdata = n->dumpData();
    const bool ok =
      pdata->dataOut(_langSound.AR_PLAYER_MATRIX, mHead.v, AR_FLOAT, 16) &&
      pdata->dataOut(_langSound.AR_PLAYER_UNIT_CONVERSION, &unitConversion, AR_FLOAT, 1);
    if (!ok)
      ar_log_error() << "arSoundDatabase: bogus head or unitConversion.\n";
    delete pdata; // dumpData() allocated it.
    if (!ok)
      return;
  }

  s->setUnitConversion(unitConversion);

  // Once per sound-frame, 5x more often than
  // arServerFramework::setPlayTransform calls loadMatrices.
  (void)s->loadMatrices(mHead, _renderMode);
}

// Copy of OpenGL's stack of transformation matrices, for sound rendering.
stack<arMatrix4, deque<arMatrix4> > ar_transformStack;

bool arSoundDatabase::render() {
  ar_transformStack.push(arMatrix4());
  //_eraseLock.lock("arSoundDatabase::render");
  const bool ok = _render((arSoundNode*)&_rootNode);
  //_eraseLock.unlock();
  ar_transformStack.pop();
  return ok;
}

bool arSoundDatabase::_render(arSoundNode* node) {
  const bool fTransform = node->getTypeCode() == AR_S_TRANSFORM_NODE;
  if (fTransform)
    ar_transformStack.push(ar_transformStack.top());

  // Don't render the root node.
  bool ok = node->isroot() || node->render();

  // Use _children, not getChildren(), to avoid copying the whole list.
  const list<arDatabaseNode*>& children = node->_children;
  for (list<arDatabaseNode*>::const_iterator i = children.begin(); i != children.end(); ++i) {
    ok &= _render((arSoundNode*)*i);
  }
  if (fTransform)
    ar_transformStack.pop();
  return ok;
}

arDatabaseNode* arSoundDatabase::_makeNode(const string& type) {
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
  ar_log_error() << "arSoundDatabase: makeNode got unknown type '" << type << "'.\n";
  return NULL;
}

arDatabaseNode* arSoundDatabase::_processAdmin(arStructuredData* data) {
  const string action = data->getDataString("action");
  const string name(data->getDataString("name"));
  if ((action == "remote_path")||(action == "bundle_path")) {
    const arSlashString bundleInfo(name);
    if (bundleInfo.size() == 2) {
      setDataBundlePath(bundleInfo[0], bundleInfo[1]);
      ar_log_remark() << "arSoundDatabase using sound bundle '" << name << "'\n";
    } else {
      ar_log_error() << "arSoundDatabase ignoring garbled sound bundle id for '" << name << "'.\n";
    }
  }
  return &_rootNode;
}
