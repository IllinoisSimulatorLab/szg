//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFileSource.h"

void ar_fileSourceEventTask(void* fileSource){ 
  arFileSource* f = (arFileSource*) fileSource;
  ar_timeval latestTime, lastCheckpoint;
  bool establishingCheckpoint = true;
  
  for (;;){
    while (establishingCheckpoint ||
	   ar_difftime(latestTime, lastCheckpoint) < 10000) {
      arStructuredData* data = f->_parser->parse(&f->_dataStream);
      if (data) {
        // Read data.

        ARint sig[3];
        data->dataOut("signature",sig,AR_INT,3);
        if (sig[0] != f->getNumberButtons() ||
	    sig[1] != f->getNumberAxes() ||
	    sig[2] != f->getNumberMatrices()){
	  ar_log_remark() << "arFileSource signature changed.\n";
	  f->_setDeviceElements(sig);
          f->_reconfig();
	}

        ARint timeInfo[2];
        data->dataOut("timestamp",timeInfo,AR_INT,2);
        latestTime.sec = timeInfo[0];
	latestTime.usec = timeInfo[1];

	if (establishingCheckpoint){
          establishingCheckpoint = false;
          lastCheckpoint.sec = timeInfo[0];
	  lastCheckpoint.usec = timeInfo[1];
	}

	// Safely send the data.
        f->_sendData(data);
        f->_parser->recycle(data);
      }
      else {
        // Reached eof.
        f->_dataStream.ar_close();
	// Wait half a second and then reopen. Loop by default.
        ar_usleep(500000);
	if (!f->_dataStream.ar_open(f->_dataFileName, "", f->_dataFilePath)){
	  cerr << "arFileSource warning: reopen input file.\n";
	  return;
	}
	establishingCheckpoint = true;
      }
    }
    establishingCheckpoint = true;
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
    ar_log_warning() << "arFileSink has undefined SZG_DATA/path.\n";
    return false;
  }

  if (!_dataStream.ar_open(_dataFileName, "", _dataFilePath)){
    ar_log_error() << "arFileSource failed to open '" << _dataFilePath <<
      "/" << _dataFileName << "'.\n";
    return false;
  }

  _eventThread.beginThread(ar_fileSourceEventTask,this);
  return true;
}
