//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arStreamNode.h"

arStreamNode::arStreamNode():
  _channel(-1),
  _stream(NULL),
  _fileName("NULL"),
  _oldFileName("NULL"),
  _paused(1),
  _amplitude(1.0),
  _requestedTime(0),
  _currentTime(0),
  _streamLength(0),
  _complained(false){

  // RedHat 8.0 will fail to compile this if the following statements are
  // outside the body of the constructor
  _typeCode = AR_S_STREAM_NODE;
  _typeString = "stream"; 
}

arStreamNode::~arStreamNode(){
#ifdef EnableSound
  // Is it really a good idea to shut down the fmod stuff here??
  if (!_owningDatabase->isServer()){
    if (_stream){
      FSOUND_Stream_Stop(_stream);
      FSOUND_Stream_Close(_stream);
      FSOUND_StopSound(_channel);
    }
  }
#endif
}

void arStreamNode::render(){
#ifdef EnableSound
  if (!_owningDatabase->isServer()){
    if (_fileName != _oldFileName){
      // might need to complain again
      _complained = false;
      // Shutdown the old stuff
      FSOUND_Stream_Stop(_stream);
      FSOUND_Stream_Close(_stream);
      FSOUND_StopSound(_channel);
      // It's a new file. We need to close the old stream and open the
      // new one.
      string fullName = ar_fileFind(_fileName,"",
				    _owningDatabase->getPath());
      if (fullName == "NULL"){
	// didn't find it
        _stream = NULL;
      }
      else{
        _stream = FSOUND_Stream_Open(fullName.c_str(),
                                     FSOUND_NORMAL | FSOUND_MPEGACCURATE, 
                                     0, 0);
      }
      // Store the length of the new stream.
      if (_stream){
        _streamLength = FSOUND_Stream_GetLengthMs(_stream);
	_channel = FSOUND_Stream_PlayEx(FSOUND_FREE, _stream, NULL, 1);
      }
      // This is now the current stream
      _oldFileName = _fileName;
    }
    // Only do further work if, indeed, the stream is available.
    if (_stream){
      // Set the amplitude of the stream.
      FSOUND_SetVolume(_channel, int(255*_amplitude));
      // Set the current time of the stream. For instance, we may want to be
      // able to seek the stream. NOTE: if the requested time is < 0,
      // do nothing. This is important as it allows the node to just
      // "go with the flow". Furthermore, DO NOT SEEK if the stream is paused
      // (the seeking can be an expensive operation in fmod)
      if (_requestedTime >= 0 && !_paused){
        FSOUND_Stream_SetTime(_stream, _requestedTime);
	// NOTE: THIS IS BUGGY BECAUSE....
	//   RELIES ON THE FACT THAT ALTERATIONS TO THE DATABASE DO NOT
	//   OVERLAP WITH THE ALTERATIONS.
	//
	//   IT RELIES ON THE RENDERER CHANGING NODE STATE (OTHERWISE WE'D
	//   ALWAYS SEEK BACK TO THIS SPOT)
        _requestedTime = -1;
      }
      // Set whether we are paused or not *after* setting everything else.
      // This allows us to unpause the stream at a particular spot.
      FSOUND_SetPaused(_channel, _paused);
      // Store the stream's current time.
      _currentTime = FSOUND_Stream_GetTime(_stream);
    }
    else{
      if (!_complained){
        cerr << "arStreamNode error: failed to create stream w/ file name =\n"
	     << "  " << _fileName << "\n";
	_complained = true;
      }
    }
  }
  else{
    // if the owning database is a server (i.e. a source of sounds) then
    // we should just do the following... (and maybe this isn't needed)
    _oldFileName = _fileName;
  }
#endif
}

arStructuredData* arStreamNode::dumpData(){
  arStructuredData* data = _l.makeDataRecord(_l.AR_STREAM);
  _dumpGenericNode(data,_l.AR_STREAM_ID);
  if (!data->dataInString(_l.AR_STREAM_FILE, _fileName) ||
      !data->dataIn(_l.AR_STREAM_PAUSED, &_paused, AR_INT, 1) ||
      !data->dataIn(_l.AR_STREAM_AMPLITUDE, &_amplitude, AR_FLOAT, 1) ||
      !data->dataIn(_l.AR_STREAM_TIME, &_currentTime, AR_INT, 1)){
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

  // want to be able to detect file name changes in render()
  _oldFileName = _fileName;
  _fileName = data->getDataString(_l.AR_STREAM_FILE);
  // probably a good idea to go ahead and "scrub" the file name
  // to convert it from Unix style to Win style or vice-versa
  ar_scrubPath(_fileName);
  data->dataOut(_l.AR_STREAM_PAUSED, &_paused, AR_INT, 1);
  data->dataOut(_l.AR_STREAM_AMPLITUDE, &_amplitude, AR_FLOAT, 1);
  data->dataOut(_l.AR_STREAM_TIME, &_requestedTime, AR_INT, 1);
  return true;
}
