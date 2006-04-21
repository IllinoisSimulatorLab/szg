//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arStreamNode.h"

arStreamNode::arStreamNode():
  _channel(NULL),
  _stream(NULL),
  _fileName("NULL"),
  _fileNamePrev("NULL"),
  _paused(true),
  _amplitude(1.0),
  _msecRequested(0),
  _msecNow(0),
  _msecDuration(0),
  _complained(false){

  // RedHat 8.0's gcc requires these to be *in* the constructor body.
  _typeCode = AR_S_STREAM_NODE;
  _typeString = "stream"; 
}

arStreamNode::~arStreamNode(){
  if (_owningDatabase->isServer())
    return;
#ifdef EnableSound
  if (_stream){
    (void)ar_fmodcheck(_stream->release());
  }
#endif
}

bool arStreamNode::render(){
  if (_owningDatabase->isServer()){
    _fileNamePrev = _fileName; // This has no effect?
    return true;
  }

#ifdef EnableSound
  if (_fileName != _fileNamePrev){
    _complained = false;

    // Close the old stream
    if (_stream){
      ar_fmodcheck(_stream->release());
      _stream = NULL;
    }

    // Open the new stream.
    const string fullName = ar_fileFind(_fileName,"", _owningDatabase->getPath());
    if (fullName != "NULL"){
      if (!ar_fmodcheck(ar_fmod()->createStream(fullName.c_str(), FMOD_3D, 0, &_stream))){
        return false;
      }
    }
    // Store the length of the new stream.
    if (_stream){
      if (!ar_fmodcheck(_stream->getLength(
             &_msecDuration, FMOD_TIMEUNIT_MS)) ||
          !ar_fmodcheck(ar_fmod()->playSound(
	     FMOD_CHANNEL_FREE, _stream, false, &_channel))) {
	return false;
      }
    }
    _fileNamePrev = _fileName;
  }

  if (_stream){
    if (!ar_fmodcheck(_channel->setVolume(_amplitude)))
      return false;

    // Set the current time of the stream, so it's seekable.
    // Ignore negative _msecRequested, to "go with the flow."
    // Don't seek if paused -- this can be expensive.
    if (_msecRequested >= 0 && !_paused){
      if (!ar_fmodcheck(_channel->setPosition(_msecRequested, FMOD_TIMEUNIT_MS)))
        return false;

      // NOTE: THIS IS BUGGY BECAUSE....
      //   RELIES ON THE FACT THAT ALTERATIONS TO THE DATABASE DO NOT
      //   OVERLAP WITH THE ALTERATIONS.
      //
      //   IT RELIES ON THE RENDERER CHANGING NODE STATE (OTHERWISE WE'D
      //   ALWAYS SEEK BACK TO THIS SPOT)
      _msecRequested = -1;
    }
    if (!ar_fmodcheck(_channel->setPaused(_paused)) ||
        !ar_fmodcheck(_channel->getPosition(&_msecNow, FMOD_TIMEUNIT_MS))) {
      return false;
    }
  }
  else{
    if (!_complained){
      cerr << "arStreamNode error: failed to create stream '"
	   << "  " << _fileName << "'\n";
      _complained = true;
    }
  }
#endif

  return true;
}

arStructuredData* arStreamNode::dumpData(){
  arStructuredData* data = _l.makeDataRecord(_l.AR_STREAM);
  _dumpGenericNode(data,_l.AR_STREAM_ID);
  if (!data->dataInString(_l.AR_STREAM_FILE, _fileName) ||
      !data->dataIn(_l.AR_STREAM_PAUSED, &_paused, AR_INT, 1) ||
      !data->dataIn(_l.AR_STREAM_AMPLITUDE, &_amplitude, AR_FLOAT, 1) ||
      !data->dataIn(_l.AR_STREAM_TIME, &_msecNow, AR_INT, 1)){
    delete data;
    return NULL;
  }
  return data;
}

bool arStreamNode::receiveData(arStructuredData* data){
  if (data->getID() != _l.AR_STREAM){
    cerr << "arStreamNode error: expected "
	 << _l.AR_STREAM
	 << " (" << _l._stringFromID(_l.AR_STREAM) << "), not "
	 << data->getID()
	 << " (" << _l._stringFromID(data->getID()) << ")\n";
    return false;
  }

  // for detecting changed filename in render()
  _fileNamePrev = _fileName;
  _fileName = data->getDataString(_l.AR_STREAM_FILE);
  ar_scrubPath(_fileName);
  data->dataOut(_l.AR_STREAM_PAUSED, &_paused, AR_INT, 1);
  data->dataOut(_l.AR_STREAM_AMPLITUDE, &_amplitude, AR_FLOAT, 1);
  data->dataOut(_l.AR_STREAM_TIME, &_msecRequested, AR_INT, 1);
  return true;
}
