//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

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

  // Red Hat 8.0 won't compile if these two are initializers.
  // Like graphics/ar*Node.cpp.
  _typeCode = AR_S_FILE_NODE;
  _typeString = "fileWav";
}

arSoundFileNode::~arSoundFileNode(){
#ifdef EnableSound
  if (_channel && isClient()) {
    const FMOD_RESULT ok = FMOD_Channel_Stop( _channel );
    if (ok == FMOD_ERR_INVALID_HANDLE) {
      // _channel is invalid, probably because it already stopped playing.
      // Don't complain.
      _channel = NULL;
    }
  }
#endif
}

extern arMatrix4 __globalSoundListener;

bool arSoundFileNode::_adjust(bool useTrigger){
#ifndef EnableSound
  (void)useTrigger; // avoid compiler warning
  return true;
#else
  if (!isClient())
    return true;
  if (!_channel)
    return false;

  FMOD_MODE m;
  if (!ar_fmodcheck( FMOD_Channel_GetMode( _channel, &m ))) {
    // _channel was invalid.
    (void) FMOD_Channel_Stop( _channel );
    _channel = NULL;
    return false;
  }

  const float aRaw = useTrigger ? _triggerAmplitude : _amplitude;
  if (!ar_fmodcheck( FMOD_Channel_SetVolume( _channel, aRaw )))
    return false;

  const arMatrix4 mat = ar_transformStack.empty() ? ar_identityMatrix() : ar_transformStack.top();
  const arVector3& p = useTrigger ? _triggerPoint : _point;
  const arVector3 point((__globalSoundListener * mat) * p);

  if (m & FMOD_2D) {
    const float r = 1.5;
    const float& x = point.v[0];
    const float pan = x<-r ? -1. : x>r ? 1. : x/r;
    const float dist = point.magnitude() / r;
    const float aDist = dist<1. ? 1. : 1. / dist; // hack: inverse not inverse square
    ar_log_debug() << "2D sound from " << point << ", pan " << pan <<
      ", ampl " << aDist << ".\n";
    return ar_fmodcheck( FMOD_Channel_SetVolume( _channel, aRaw * aDist)) &&
           ar_fmodcheck( FMOD_Channel_SetPan( _channel, pan));
  }

  if (m & FMOD_3D) {
    static const FMOD_VECTOR velocity_unused(
      FmodvectorFromArvector(arVector3(0,0,0))); // doppler nyi
    const FMOD_VECTOR tmp(FmodvectorFromArvector(point));
    return ar_fmodcheck( FMOD_Channel_Set3DAttributes( _channel, &tmp, &velocity_unused ));
  }

  // Paranoia.  Exactly one of FMOD_2D and FMOD_3D should be true.
  ar_log_warning() << "internal FMOD mode error.\n";
  return true;

#endif
}

bool arSoundFileNode::render(){
#ifdef EnableSound
  if (_amplitude < 0.) {
    if (!_fComplained[0]) {
      _fComplained[0] = true;
      ar_log_warning() << "arSoundFileNode '"
	   << _name << "' has negative amplitude " << _amplitude << ".\n";
    }
  }
  else{
    _fComplained[0] = false;
  }

  if (_amplitude > 100.) {
    if (!_fComplained[1]) {
      _fComplained[1] = true;
      ar_log_warning() << "arSoundFileNode '"
	   << _name << "' has excessive amplitude " << _amplitude << ".\n";
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
      ar_log_warning() << "arSoundFileNode: no sound to play.\n";
      return false;
    }

    // Start the sound paused.
    if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
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
        if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, _psamp, false, &_channel)))
	  return false;
      }
      if (!ar_fmodcheck( FMOD_Channel_SetPaused( _channel, false ))) // redundant, from playSound(_,_,false,_) ?
        return false;
      _action = "none";
    }
    else if (_action == "pause"){
      if (!_channel){
        if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      if (!ar_fmodcheck( FMOD_Channel_SetPaused( _channel, true ))) // redundant, from playSound(_,_,true,_) ?
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

        if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      else{

//	bool fPlaying;
//	if (!ar_fmodcheck(_channel->isPlaying(&fPlaying))) {
//	  _channel = NULL;
//	  return false;
//	}

	const FMOD_RESULT ok = FMOD_Channel_Stop( _channel );
	if (ok == FMOD_ERR_INVALID_HANDLE) {
	  // _channel is invalid, probably because it already stopped playing.
	  _channel = NULL;
	}
	else if (!ar_fmodcheck(ok)) {
	  _channel = NULL;
	  return false;
	}
        if (!ar_fmodcheck( FMOD_System_PlaySound( ar_fmod(), FMOD_CHANNEL_FREE, _psamp, true, &_channel)))
	  return false;
      }
      if (!_adjust(true))
        return false;
      if (!ar_fmodcheck( FMOD_Channel_SetPaused( _channel, false )))
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

// Bug: Can't update ampl or xyz of a triggered sound, once it's started.
// Workaround:  keep triggered sounds short.

bool arSoundFileNode::receiveData(arStructuredData* pdata){
  if (pdata->getID() != _l.AR_FILEWAV){
    ar_log_warning() << "arSoundFileNode expected "
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
  (void)pdata->dataOut(_l.AR_FILEWAV_XYZ, &_point, AR_FLOAT, 3);
  _fileName = pdata->getDataString(_l.AR_FILEWAV_FILE);

  // Design problem: graphics maps poorly onto sound, which
  // is better conceived as a stream of events.

  // Determine what to do at the next render frame.
  // Changing things too quickly in the API will discard actions.
  // Rely on the fact that sound rendering is distinct from the
  // actual modifying of the nodes.

  if (_fLoop == 1 && fLoopPrev == 0){
    _action = "play";
  }
  else if (_fLoop == 0 && fLoopPrev == 1){
    _action = "pause";
  }
  else if (_fLoop == -1 && fLoopPrev != -1){
    // Workaround that only the initial volume and location matter in
    // the old (messed-up) implementation.
    _action = "trigger";
    _triggerAmplitude = _amplitude;
    _triggerPoint = _point;
  }

  // Wait for render to reset _action to "none".  Don't do it here.
  // todo: make _action an enum, not a string.
  return true;
}
