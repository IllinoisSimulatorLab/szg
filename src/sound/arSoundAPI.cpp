//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundAPI.h"

static arSoundDatabase* __currentDatabase = NULL;
static arSoundLanguage  __lang;

void dsSetSoundDatabase(arSoundDatabase* database){
  __currentDatabase = database;
}

arDatabaseNode* dsMakeNode(const string& name, 
                           const string& parent, 
                           const string& type){
  if (!__currentDatabase) {
    cerr << "syzygy error: dsSetSoundDatabase not yet called.\n";
    return NULL;
  }

  int ID = -1;
  arStructuredData* data = __currentDatabase->makeNodeData;
  arDatabaseNode* parentNode = __currentDatabase->getNode(parent);
  if (!parentNode){
    // error messge already printed in getNode(...)
    return NULL;
  }
  int parentID = parentNode->getID();
  if (!data->dataIn(__lang.AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1) ||
      !data->dataIn(__lang.AR_MAKE_NODE_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__lang.AR_MAKE_NODE_NAME, name) ||
      !data->dataInString(__lang.AR_MAKE_NODE_TYPE, type)) {
    cerr << "syzygy arSoundAPI error: dsMakeNode dataIn failed.\n";
    return NULL;
  }
  // Use alter not arSoundDatabase::alter, to ensure that
  // the remote node will be created.
  return __currentDatabase->alter(data);
}
  
int dsTransform(const string& name,
                const string& parent, 
                const arMatrix4& matrix){
  arDatabaseNode* node = dsMakeNode(name, parent, "transform");
  return node && dsTransform(node->getID(), matrix) ?
    node->getID() : -1;
}

bool dsTransform(int ID, const arMatrix4& matrix){
  if (ID<0)
    return false;
  if (!__currentDatabase) {
    cerr << "syzygy error: dsSetSoundDatabase not yet called.\n";
    return false;
  }
  arStructuredData* data = __currentDatabase->transformData;
  if (!data->dataIn(__lang.AR_TRANSFORM_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__lang.AR_TRANSFORM_MATRIX, matrix.v, AR_FLOAT, 16)) {
    cerr << "syzygy arSoundAPI error: dsTransform dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
}

int dsPlayer(const arMatrix4& headMatrix, const arVector3& midEyeOffset,
             float unitConversion){
  if (!__currentDatabase) {
    cerr << "syzygy error: dsSetSoundDatabase not yet called.\n";
    return -1;
  }
  // AARGH! THIS IS NOT GOOD! getNode(nodeName) has become a very, very slow
  // call!
  arDatabaseNode* node = __currentDatabase->getNode("szg_player", false);
  if (!node){
    node = dsMakeNode("szg_player","root","player");
  }
  const ARint ID = node->getID();
  arStructuredData* data = __currentDatabase->playerData;
  if (!data->dataIn(__lang.AR_PLAYER_ID,&ID,AR_INT,1) ||
      !data->dataIn(__lang.AR_PLAYER_MATRIX,headMatrix.v,AR_FLOAT,16) ||
      !data->dataIn(__lang.AR_PLAYER_MID_EYE_OFFSET,midEyeOffset.v,AR_FLOAT,3) || //;; offset from tracker to point midway between eyes?
  // Other data could go in here: rolloff factor, doppler factor, clipping distances, reverb, ...
      !data->dataIn(__lang.AR_PLAYER_UNIT_CONVERSION,&unitConversion,AR_FLOAT,1)) { //;; for FSOUND_3D_Listener_SetDistanceFactor
    cerr << "syzygy arSoundAPI error: dsPlayer dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
}

// fLoop: 1 means loop, 0 means don't loop because it will be triggered,
//        -1 means trigger now.
int dsLoop(const string& name, const string& parent, 
           const string& filename, int fLoop, float ampl, 
           const arVector3& xyz){
  return dsLoop(name, parent, filename, fLoop, ampl, &xyz.v[0]);
}

int dsLoop(const string& name, const string& parent, 
           const string& filename, int fLoop, float ampl, const float* xyz){
  arDatabaseNode* node = dsMakeNode(name, parent, "fileWav");
  return node && dsLoop(node->getID(), filename, fLoop, ampl, xyz) ?
    node->getID() : -1;
} 

bool dsLoop(int ID, const string& name, int fLoop, float ampl, 
            const arVector3& xyz){
  return dsLoop(ID, name, fLoop, ampl, &xyz.v[0]);
}
bool dsLoop(int ID, const string& name, int fLoop, float ampl, 
            const float* xyz){
  if (ID<0)
    return false;
  arStructuredData* data = __currentDatabase->filewavData;
  if (!data->dataInString(__lang.AR_FILEWAV_FILE, name) ||
      !data->dataIn(__lang.AR_FILEWAV_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__lang.AR_FILEWAV_LOOP, &fLoop, AR_INT, 1) ||
      !data->dataIn(__lang.AR_FILEWAV_AMPL, &ampl, AR_FLOAT, 1) ||
      !data->dataIn(__lang.AR_FILEWAV_XYZ, xyz, AR_FLOAT, 3)) {
    cerr << "syzygy arSoundAPI error: dsLoop dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
} 

int dsSpeak( const string& name, const string& parent, const string& text ){
  arDatabaseNode* node = dsMakeNode(name, parent, "speech");
  return node && dsSpeak( node->getID(), text ) ? node->getID() : -1;
} 

bool dsSpeak( int ID, const string& text ) {
  if (ID<0)
    return false;
  arStructuredData* data = __currentDatabase->speechData;
  if (!data->dataInString(__lang.AR_SPEECH_TEXT, text) ||
      !data->dataIn(__lang.AR_SPEECH_ID, &ID, AR_INT, 1)) {
    cerr << "syzygy arSoundAPI error: dsSpeak dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
} 

/// This function is used for long streams which must be seekable by the
/// the calling application. 
/// name: the name of the node
/// parent: the name of the node's parent
/// fileName: the name of the sound file (.mp3 or .wav)
/// paused: 1 if the stream is to be paused
///         0 if the stream is not to be paused
/// amplitude: 0 is silent, 1 is normal.
/// time: If >=0, the time (in ms) that the stream should be playing at.
///       Otherwise, a signal that the parameter should be ignored.
/// NOTE: to avoid sonic glitches, always start new streams paused and
///       with time = -1.
int dsStream( const string& name, const string& parent, const string& fileName,
              int paused, float amplitude, int time){
  arDatabaseNode* node = dsMakeNode(name, parent, "stream");
  return node && dsStream(node->getID(), fileName, paused, amplitude, time) 
    ? node->getID() : -1;
}

bool dsStream(int ID, const string& fileName, int paused, float amplitude,
              int time){
  if (ID<0){
    return false;
  }
  arStructuredData* data = __currentDatabase->streamData;
  if (!data->dataIn(__lang.AR_STREAM_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__lang.AR_STREAM_FILE, fileName) || 
      !data->dataIn(__lang.AR_STREAM_PAUSED, &paused, AR_INT, 1) ||
      !data->dataIn(__lang.AR_STREAM_AMPLITUDE, &amplitude, AR_FLOAT, 1) ||
      !data->dataIn(__lang.AR_STREAM_TIME, &time, AR_INT, 1)){
    cerr << "syzygy arSoundAPI error: dsStream dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
}

bool dsErase(const string& name){
  if (!__currentDatabase) {
    cerr << "syzygy error: dsSetSoundDatabase not yet called.\n";
    return -1;
  }
  arStructuredData* data = __currentDatabase->eraseData;
  arDatabaseNode* node = __currentDatabase->getNode(name);
  if (!node){
    // error message was already printed in the above.
    return false;
  }
  int ID = node->getID();
  if (!data->dataIn(__lang.AR_ERASE_ID, &ID, AR_INT, 1)) {
    cerr << "syzygy arSoundAPI error: dsErase dataIn failed.\n";
    return false;
  }
  return __currentDatabase->alter(data) ? true : false;
}
