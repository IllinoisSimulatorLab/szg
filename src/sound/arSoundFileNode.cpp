//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundFileNode.h"
#include "arSoundDatabase.h"

arSoundFileNode::arSoundFileNode() :
  _fInit(false),
  _psamp(NULL),
  _chan(-1),
  _amplitudePrev(-1),
  _pointPrev(arVector3(-1e9,-1e9,-1e9)),
  _fLoop(0)
{
  _fComplained[0] = _fComplained[1] = false;

  // redhat 8.0 won't compile this if the below are outside of the constructor
  // body
  _typeCode = AR_S_FILE_NODE;
  _typeString = "fileWav";
}

arSoundFileNode::~arSoundFileNode(){
  if (_chan >= 0){
    _loopStop();
    _freeChan(_chan);
  }
}

void arSoundFileNode::_adjust(bool fForce){
  if (_chan < 0){
    // _getChan() failed, earlier.  Don't bother retrying now, just give up.
    return;
  }

  float velocity[3] = {0,0,0}; // doppler is NYI
  if (_amplitude != _amplitudePrev || fForce){
    //cout << "\t\t\t\tampl " << _amplitude << endl;
    if (isClient())
      FSOUND_SetVolume(_chan, int(_amplitude * 255));
    _amplitudePrev = _amplitude;
  }

  arVector3 point = ar_transformStack.empty() ? _point :
    ar_transformStack.top() * _point; // apply "opengl matrix" to _point

  // negate z, to swap handedness of coord systems between Syzygy and FMOD
  point[2] = -point[2];
  if (point != _pointPrev || fForce) {
    if (isClient()) {
      //cout << "\t\t\t\txyz " << point << endl;;
      FSOUND_3D_SetAttributes(_chan, &point[0], velocity);

      //cout << "arSoundFileNode::_adjust calls FSOUND_3D_Update\n";;
      // NOTE: it is a bad idea to put FSOUND_3D_Update() here,
      // since this should occur really occur just ONCE PER FRAME
      // The FSOUND_3D_Update() call has been MOVED to
      // ar_soundClientPostSyncCallback()
      // FSOUND_3D_Update();
      // Should call FSOUND_3D_Update() only once per something-or-other, 
      // via a dirty flag.
      // Set the dirty flag here (and possibly elsewhere), and test it after
      // in arSoundClient::_render() after the line: _soundDatabase.render();
    }

    _pointPrev = point;
  }
}

void arSoundFileNode::_loopStart(){
  if (isClient())
    FSOUND_PlaySoundEx(_chan, _psamp, NULL, 1); // begin paused
  _adjust();
  if (isClient())
    FSOUND_SetPaused(_chan, 0);
}

void arSoundFileNode::_loopStop(){
  if (isClient())
    FSOUND_StopSound(_chan);
}

void arSoundFileNode::_triggerStart() {
  // Trigger now.
  // If the sound is already playing on this channel,
  // just stop and restart it.
  if (isClient()) {
    FSOUND_StopSound(_chan);
    FSOUND_PlaySoundEx(_chan, _psamp, NULL, 1); // begin paused
  }
  _adjust(true);
  if (isClient())
    FSOUND_SetPaused(_chan, 0);
}

void arSoundFileNode::render(){
  // Don't warn if _amplitude>1:  this could make sense for a very quiet sound.
  if (_amplitude < 0.) {
    if (!_fComplained[0]) {
      _fComplained[0] = true;
      cerr << "arSoundFileNode warning: \""
	   << _name << "\" has negative amplitude "
	   << _amplitude << ".\n";
    }
  }
  else
    _fComplained[0] = false;

  if (_amplitude > 100.) {
    if (!_fComplained[1]) {
      _fComplained[1] = true;
      cerr << "arSoundFileNode warning: \""
	   << _name << "\" has excessively large amplitude "
	   << _amplitude << ".\n";
    }
  }
  else
    _fComplained[1] = false;

  if (!_fInit){
    _fInit = true;
    _psamp = _localSoundFile->psamp();
    // const char* name = _localSoundFile->name().c_str();

    if (_getChan() < 0)
      return;

    if (_fLoop == 1)
      _loopStart();
    return;
  }

  if (_fLoop == 1){
    _adjust();
    // We assume, dangerously, that _localSoundFile doesn't ever change.
    // ...So we shouldn't keep sending that string over every time!
    //cout << "YOYO\t" << _point << " / " << _amplitude << endl;;
  }
}

arStructuredData* arSoundFileNode::dumpData(){
  arStructuredData* pdata = _l.makeDataRecord(_l.AR_FILEWAV);
  _dumpGenericNode(pdata,_l.AR_FILEWAV_ID);

  // Stuff pdata with member variables.
  if (!pdata->dataIn(_l.AR_FILEWAV_LOOP, &_fLoop, AR_INT, 1) ||
      !pdata->dataIn(_l.AR_FILEWAV_AMPL, &_amplitude, AR_FLOAT, 1) ||
      !pdata->dataIn(_l.AR_FILEWAV_XYZ, &_point, AR_FLOAT, 3)) {
    delete pdata;
    return NULL;
  }

  // Stuff pdata's AR_FILEWAV_FILE with _commandBuffer[].
  const int len = _commandBuffer.size();
  pdata->setDataDimension(_l.AR_FILEWAV_FILE,len);
  ARchar* text = (ARchar*) pdata->getDataPtr(_l.AR_FILEWAV_FILE,AR_CHAR);
  for (int i=0; i<len; i++)
    text[i] = (ARint) _commandBuffer.v[i];
  return pdata;
}

/** @bug Can't update ampl or xyz of a triggered sound, once it's started.
 *  Workaround:  keep triggered sounds short.
 */

bool arSoundFileNode::receiveData(arStructuredData* pdata){
  if (pdata->getID() != _l.AR_FILEWAV){
    cout << "arSoundFileNode error: expected "
         << _l.AR_FILEWAV
         << " (" << _l._stringFromID(_l.AR_FILEWAV) << "), not "
         << pdata->getID()
         << " (" << _l._stringFromID(pdata->getID()) << ")\n";
    return false;
  }

  // Stuff member variables.
  const int fLoopPrev = _fLoop;
  _fLoop = pdata->getDataInt(_l.AR_FILEWAV_LOOP);
  _amplitude = pdata->getDataFloat(_l.AR_FILEWAV_AMPL);
  pdata->dataOut(_l.AR_FILEWAV_XYZ, &_point, AR_FLOAT, 3);

  // Stuff _commandBuffer[] with AR_FILEWAV_FILE.
  //;;if (_fSilent)
    //;;return false;
  const string filename(pdata->getDataString(_l.AR_FILEWAV_FILE));
  _localSoundFile = _owningDatabase->addFile(filename, _fLoop);
  _arSoundFiles = &_localSoundFile;
  const int len = filename.length();
  _commandBuffer.grow(len+1);
  for (int i=0; i<len; i++)
    _commandBuffer.v[i] = float(filename.data()[i]);

  if (_chan < 0){
    // _getChan() failed, earlier.  Don't bother retrying now, just give up.
    return true;
  }

  // Special processing for momentary events:
  // not propagating state from server to clients, but propagating a single event.

  if (_fLoop == 1 && fLoopPrev == 0)
    _loopStart();
  if (_fLoop == 0 && fLoopPrev == 1)
    _loopStop();
  if (_fLoop == -1 && fLoopPrev != -1)
    _triggerStart();

  return true;
}

bool arSoundFileNode::_chanUsed[_numchans] = {0};

int arSoundFileNode::_getChan(){
  // Find a free channel.  Use it if found.
    for (int i=0; i<_numchans; i++)
      if (!_chanUsed[i]){
	_chan = i;
        _useChan(_chan);
	return _chan;
      }
  cerr << "arSoundFileNode error: out of channels for \"" << _name << "\".\n";
  _chan = -1;
  return _chan;
}
