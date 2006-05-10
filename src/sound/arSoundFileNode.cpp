//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundFileNode.h"
#include "arSoundDatabase.h"

arSoundFileNode::arSoundFileNode() :
  _fInit(false),
#ifdef EnableSound
  _psamp(NULL),
  _channel(NULL),
#endif
  _fLoop(0),
  _fileName("NULL"),
  _oldFileName("NULL"),
  _action("none"),
  _triggerAmplitude(0),
  _triggerPoint(arVector3(0,0,0)){

  _fComplained[0] = _fComplained[1] = false;

  // redhat 8.0 won't compile if the below are outside the constructor body
  _typeCode = AR_S_FILE_NODE;
  _typeString = "fileWav";
}

arSoundFileNode::~arSoundFileNode(){
#ifdef EnableSound
  if (_channel && isClient()) {
    const FMOD_RESULT ok = _channel->stop();
    if (ok == FMOD_ERR_INVALID_HANDLE) {
      // _channel is invalid, probably because it already stopped playing.
      // Don't complain.
      _channel = NULL;
    }
  }
#endif
}

bool arSoundFileNode::_adjust(bool useTrigger){
#ifndef EnableSound
  return true;
#else
  if (!isClient())
    return true;
  if (!_channel)
    return false;

  FMOD_MODE m;
  if (!ar_fmodcheck(_channel->getMode(&m))) {
    // _channel was invalid.
    (void)_channel->stop();
    _channel = NULL;
    return false;
  }

  const float a = useTrigger ? _triggerAmplitude : _amplitude;
  if (!ar_fmodcheck(_channel->setVolume(a)))
    return false;

  if (m & FMOD_2D) {
    // Nothing to do.  This sound is 2D not 3D, perhaps a dummy sound.
    ar_log_debug() << "2D sound won't be 3D-updated.\n";
    return true;
  }

  // Paranoia.  Exactly one of FMOD_2D and FMOD_3D should be true.
  if (!(m & FMOD_3D)) {
    // Nothing to do.  This sound is 2D not 3D, perhaps a dummy sound.
    ar_log_remark() << "2D sound won't be 3D-updated, and I mean it this time.\n";
    return true;
  }

  const arVector3& p = useTrigger ? _triggerPoint : _point;
  const arVector3 point(ar_transformStack.empty() ? p : ar_transformStack.top() * p);
  const FMOD_VECTOR tmp(FmodvectorFromArvector(point)); // doppler velocity nyi
  const FMOD_VECTOR velocity(FmodvectorFromArvector(arVector3(0,0,0)));
  return ar_fmodcheck(_channel->set3DAttributes(&tmp, &velocity));
#endif
}

bool arSoundFileNode::render(){
#ifdef EnableSound
  if (_amplitude < 0.) {
    if (!_fComplained[0]) {
      _fComplained[0] = true;
      cerr << "arSoundFileNode warning: \""
	   << _name << "\" has negative amplitude "
	   << _amplitude << ".\n";
    }
  }
  else{
    _fComplained[0] = false;
  }

  if (_amplitude > 100.) {
    if (!_fComplained[1]) {
      _fComplained[1] = true;
      cerr << "arSoundFileNode warning: \""
	   << _name << "\" has excessive amplitude "
	   << _amplitude << ".\n";
    }
  }
  else{
    _fComplained[1] = false;
  }

  // If sample needs to be inited, do so.
  // As with graphics where all OpenGL calls go into render(),
  // all fmod calls should go in render.

  // Bug: we only change the sample once.
  if (_oldFileName != _fileName && !_fInit){
    // Do this only once in the lifetime of the node.
    _fInit = true; 

    // This file is either our file OR a dummy.
    // If _fLoop is 0 or -1, we likely want a NON-LOOPING sound.
    _localSoundFile = _owningDatabase->addFile(_fileName, _fLoop<=0 ? 0 : 1);
    _arSoundFiles = &_localSoundFile;
    _oldFileName = _fileName;

    _psamp = _localSoundFile->psamp();
    if (!_psamp) {
      cerr << "arSoundFileNode warning: no sound to play.\n";
      return false;
    }

    // Start the sound paused.
    if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
      return false;
  }

  if (_fInit){
    // Use the sound.

    // Only adjust if the loop is playing (fLoop == 1) or if triggered:
    // Since triggering has previously depended upon detecting
    // an fLoop diff, there needed to be a fLoop of 0 passed in after the
    // triggering fLoop of -1. NO ADJUSTMENT SHOULD OCCUR on that data!
    // (i.e. the volume might have been 0). This is a backwards compatibility hack.
    if (_fLoop == 1){
      if (!_adjust(false))
        return false;
    }



    if (_action == "play"){
      if (!_channel){
        if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, _psamp, false, &_channel)))
	  return false;
      }
      if (!ar_fmodcheck(_channel->setPaused(false))) // redundant, from playSound(_,_,false,_) ?
        return false;
      _action = "none";
    }
    else if (_action == "pause"){
      if (!_channel){
        if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      if (!ar_fmodcheck(_channel->setPaused(true))) // redundant, from playSound(_,_,true,_) ?
	return false;
      _action = "none";
    }
    else if (_action == "trigger"){
      if (!_channel){

#if 0
;;;;
When sound finishes, call _channel->stop();
So keep my own timer, and consult it whenever a method is called.
Is this still a memory leak?
#endif

        if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      else{

//	bool fPlaying;
//	if (!ar_fmodcheck(_channel->isPlaying(&fPlaying))) {
//	  _channel = NULL;
//	  return false;
//	}

	const FMOD_RESULT ok = _channel->stop();
	if (ok == FMOD_ERR_INVALID_HANDLE) {
	  // _channel is invalid, probably because it already stopped playing.
	  _channel = NULL;
	}
	else if (!ar_fmodcheck(ok)) {
	  _channel = NULL;
	  return false;
	}
        if (!ar_fmodcheck(ar_fmod()->playSound(FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      if (!_adjust(true))
        return false;
      if (!ar_fmodcheck(_channel->setPaused(false)))
        return false;
      _action = "none";
    }
    else if (_action == "none"){
    }
    else
      ar_log_warning() << "ignoring unexpected sound action " << _action << ar_endl;
  }
#endif
  return true;
}

arStructuredData* arSoundFileNode::dumpData(){
  arStructuredData* pdata = _l.makeDataRecord(_l.AR_FILEWAV);
  _dumpGenericNode(pdata,_l.AR_FILEWAV_ID);

  // Stuff pdata with member variables.
  if (!pdata->dataIn(_l.AR_FILEWAV_LOOP, &_fLoop, AR_INT, 1) ||
      !pdata->dataIn(_l.AR_FILEWAV_AMPL, &_amplitude, AR_FLOAT, 1) ||
      !pdata->dataIn(_l.AR_FILEWAV_XYZ, &_point, AR_FLOAT, 3) ||
      !pdata->dataInString(_l.AR_FILEWAV_FILE, _fileName)) {
    delete pdata;
    return NULL;
  }

#if 0
  // The commandBuffer is really going to become obsolete....
  const int len = _commandBuffer.size();
  pdata->setDataDimension(_l.AR_FILEWAV_FILE,len);
  ARchar* text = (ARchar*) pdata->getDataPtr(_l.AR_FILEWAV_FILE,AR_CHAR);
  for (int i=0; i<len; i++)
    text[i] = (ARint) _commandBuffer.v[i];
#endif

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
  _fileName = pdata->getDataString(_l.AR_FILEWAV_FILE);

  // This is really just in need of fixing. So, we'll let it keep it's
  // broken behavior until it gets modified in the great increasing
  // of object orientedness. The problem is that... the infrastructure for 
  // graphics maps awkwardly onto an infrastructure for sound. Sound, more 
  // than graphics, is naturally conceived as a stream of events.

  // The action to be taken at the next render frame is determined.
  // Changing things too quickly in the API will discard actions.

  // Here, we RELY on the fact that sound rendering is distinct from the
  // actual modifying of the nodes.

  if (_fLoop == 1 && fLoopPrev == 0){
    _action = "play";
  }
  else if (_fLoop == 0 && fLoopPrev == 1){
    _action = "pause";
  }
  else if (_fLoop == -1 && fLoopPrev != -1){
    // NOTE: we need to be able to get around the fact that only the initial
    // volume and point location can matter in the old (messed-up)
    // implementation.
    _action = "trigger";
    _triggerAmplitude = _amplitude;
    _triggerPoint = _point;
  }

  // Wait for render to reset _action to "none".  Don't do it here.
  // Bug: _action ought to be an enum, not a string.
  return true;
}
