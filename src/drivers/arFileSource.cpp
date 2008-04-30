//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFileSource.h"

void ar_fileSourceEventTask(void* pv){ 
  ((arFileSource*)pv)->_eventThread();
}

void arFileSource::_eventThread() {
  ar_timeval latestTime, lastCheckpoint;
  bool fNeedCheckpoint = true;
  for (;;){
    while (fNeedCheckpoint || ar_difftime(latestTime, lastCheckpoint) < 10000) {
      arStructuredData* data = _parser->parse(&_dataStream);
      if (data) {
        // Read data.

        ARint sig[3];
        data->dataOut("signature",sig,AR_INT,3);
        if (sig[0] != getNumberButtons() ||
	    sig[1] != getNumberAxes() ||
	    sig[2] != getNumberMatrices()){
	  ar_log_remark() << "arFileSource changed signature.\n";
	  _setDeviceElements(sig);
          _reconfig();
	}

        ARint timeInfo[2];
        data->dataOut("timestamp",timeInfo,AR_INT,2);
        latestTime.sec = timeInfo[0];
	latestTime.usec = timeInfo[1];

	if (fNeedCheckpoint){
          fNeedCheckpoint = false;
          lastCheckpoint.sec = timeInfo[0];
	  lastCheckpoint.usec = timeInfo[1];
	}

	// Safely send the data.
        _sendData(data);
        _parser->recycle(data);
      }
      else {
        // EOF
        _dataStream.ar_close();
	// Loop by default.
        ar_usleep(500000);
	if (!_dataStream.ar_open(_dataFileName, "", _dataFilePath)){
	  ar_log_error() << "arFileSource failed to reopen '" << _dataFilePath <<
	    "/" << _dataFileName << "'.\n";
	  return;
	}
	fNeedCheckpoint = true;
      }
    }
    fNeedCheckpoint = true;
    ar_usleep(10000);
  }
}

arFileSource::arFileSource() :
  _dataFileName("inputdump.xml"),
  _dataFilePath(""),
  _parser(new arStructuredDataParser(_lang.getDictionary()))
{}

arFileSource::~arFileSource(){
  delete _parser;
}

bool arFileSource::init(arSZGClient& SZGClient){
  _dataFilePath = SZGClient.getDataPath();
  return true;
}

bool arFileSource::start(){
  if (_dataFilePath == "NULL") {
    // Only complain when it's about to get used.
    ar_log_error() << "arFileSource: no SZG_DATA/path.\n";
    return false;
  }

  if (!_dataStream.ar_open(_dataFileName, "", _dataFilePath)){
    ar_log_error() << "arFileSource failed to open '" << _dataFilePath <<
      "/" << _dataFileName << "'.\n";
    return false;
  }

  arThread dummy(ar_fileSourceEventTask, this);
  return true;
}
