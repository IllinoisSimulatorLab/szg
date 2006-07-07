//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundFileNode.h"
#include "arSoundDatabase.h"

arSoundFileNode::arSoundFileNode() :
  _fInit(false),
  _psamp(NULL),
  _channel(-1),
  _fLoop(0),
  _fileName("NULL"),
  _oldFileName("NULL"),
  _action("none"),
  _triggerAmplitude(0),
  _triggerPoint(arVector3(0,0,0)){

  _fComplained[0] = _fComplained[1] = false;

  // redhat 8.0 won't compile this if the below are outside of the constructor
  // body
  _typeCode = AR_S_FILE_NODE;
  _typeString = "fileWav";
}

arSoundFileNode::~arSoundFileNode(){
  if (_channel >= 0 && isClient()){
    FSOUND_StopSound(_channel);
  }
}

void arSoundFileNode::_adjust(bool useTrigger){
  if (isClient()){
    // If fmod failed to get a channel, do nothing.
    if (_channel < 0){
      return;
    }

    // NOTE: doppler has yet to be added!
    float velocity[3] = {0,0,0};
    arVector3 point;
    if (!useTrigger){
      FSOUND_SetVolume(_channel, int(_amplitude * 255)); 
      point = ar_transformStack.empty() 
              ? _point :ar_transformStack.top() * _point;
    }
    else{
      FSOUND_SetVolume(_channel, int(_triggerAmplitude * 255));
      point = ar_transformStack.empty() 
              ? _triggerPoint :ar_transformStack.top() * _triggerPoint;
    }
    // negate z, to swap handedness of coord systems between Syzygy and FMOD
    // THIS IS NOT THE CORRECT WAY TO CHANGE COORDINATE SYSTEMS
    point[2] = -point[2];
    FSOUND_3D_SetAttributes(_channel, &point[0], velocity);
  }
}

void arSoundFileNode::render(){
#ifdef EnableSound
  // Output some user warnings.=, just in case bogus values are being inputed.
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
	   << _name << "\" has excessively large amplitude "
	   << _amplitude << ".\n";
    }
  }
  else{
    _fComplained[1] = false;
  }

  // If sample needs to be inited, do so. As with graphics
  // (where all OpenGL related calls go into render()), all sound related 
  // calls should go in render.

  // We only change the sample once. This is a BUG in the implementation.
  if (_oldFileName != _fileName && !_fInit){
    // We only do this once in the lifetime of the node.
    _fInit = true; 
    // NOTE: this file either be our file OR a dummy sound.
    // If _fLoop is 0 or -1, then we want a NON-LOOPING sound, most-likely.
    // (most-likely said here instead of certainly since, well, we could
    // be making a non-looping sound.
    _localSoundFile = _owningDatabase->addFile(_fileName, _fLoop<=0 ? 0 : 1);
    _arSoundFiles = &_localSoundFile;
    _oldFileName = _fileName;
    // Queue up the sound.
    _psamp = _localSoundFile->psamp();
    // Start the sound paused.
    _channel = FSOUND_PlaySoundEx(FSOUND_FREE, _psamp, NULL, 1);
  }

  if (_fInit){
    // The sound is ready to go. Use it.

    // ONLY ADJUST IF THE LOOP IS PLAYING (fLoop == 1) or if triggered!
    // Why? since triggering has previously depended upon detecting
    // an fLoop diff, there needed to be a fLoop of 0 passed in after the
    // triggering fLoop of -1. NO ADJUSTMENT SHOULD OCCUR on that data!
    // (i.e. the volume might have been 0). This is a backwards compatibility
    // HACK!
    if (_fLoop == 1){
      _adjust(false);
    }

    if (_action == "play"){
      if (_channel == -1){
        _channel = FSOUND_PlaySoundEx(FSOUND_FREE, _psamp, NULL, 1);
      }
      FSOUND_SetPaused(_channel, 0);
      _action = "none";
    }
    else if (_action == "pause"){
      if (_channel == -1){
        _channel = FSOUND_PlaySoundEx(FSOUND_FREE, _psamp, NULL, 1);
      }
      FSOUND_SetPaused(_channel, 1);
      _action = "none";
    }
    else if (_action == "trigger"){
      if (_channel == -1){
	//;;;; when sound finishes, call FSOUND_StopSound(_channel).
	//;;;; So keep my own timer, and consult it whenever a method's called.
	//;;;; try this, test it, check it in.
        _channel = FSOUND_PlaySoundEx(FSOUND_FREE, _psamp, NULL, 1);
      }
      else{
        FSOUND_StopSound(_channel);
        _channel = FSOUND_PlaySoundEx(FSOUND_FREE, _psamp, NULL, 1);
      }
      _adjust(true);
      FSOUND_SetPaused(_channel, 0);
      _action = "none";
    }
  }
#endif
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

  // The commandBuffer is really going to become obsolete....
  /*const int len = _commandBuffer.size();
  pdata->setDataDimension(_l.AR_FILEWAV_FILE,len);
  ARchar* text = (ARchar*) pdata->getDataPtr(_l.AR_FILEWAV_FILE,AR_CHAR);
  for (int i=0; i<len; i++)
    text[i] = (ARint) _commandBuffer.v[i];*/

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
  // NOTE: by changing things too quickly in the API, actions will be
  // discarded.

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
  // There needs to be some stickiness. Specifically, let the render reset
  // things. i.e. DO NOT set _action == "none" here

  return true;
}
