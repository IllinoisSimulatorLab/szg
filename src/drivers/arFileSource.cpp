//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFileSource.h"

void ar_fileSourceEventTask(void* fileSource){ 
  arFileSource* f = (arFileSource*) fileSource;
  ar_timeval latestTime, lastCheckpoint;
  bool establishingCheckpoint = true;
  
  while (true){
    while (establishingCheckpoint 
	   || ar_difftime(latestTime, lastCheckpoint) < 10000){
      arStructuredData* data = f->_parser->parse(&f->_dataStream);
      if (!data){
        // we must have reached the end of the file
        f->_dataStream.ar_close();
	// wait half a second and then reopen the file! we do looping
	// behavior by default
        ar_usleep(500000);
	if (!f->_dataStream.ar_open(f->_dataFileName, "", f->_dataFilePath)){
	  cerr << "arFileSource error: reopen input file.\n";
	  return;
	}
	establishingCheckpoint = true;
      }
      else{
        // we got some data
        int elements[3];
        data->dataOut("signature",elements,AR_INT,3);
        if (elements[0] != f->getNumberButtons()
	    || elements[1] != f->getNumberAxes()
	    || elements[2] != f->getNumberMatrices()){
	  cout << "arFileSource remark: the signature has changed.\n";
	  f->_setDeviceElements(elements[0], elements[1], elements[2]);
          f->_reconfig();
	}
        int timeInfo[2];
        data->dataOut("timestamp",timeInfo,AR_INT,2);
        latestTime.sec = timeInfo[0];
	latestTime.usec = timeInfo[1];
	if (establishingCheckpoint){
          lastCheckpoint.sec = timeInfo[0];
	  lastCheckpoint.usec = timeInfo[1];
          establishingCheckpoint = false;
	}
	// we can now safely send the data
        f->_sendData(data);
        f->_parser->recycle(data);
      }
    }
    establishingCheckpoint = true;
    ar_usleep(10000);
  }
}

arFileSource::arFileSource(){
  _dataFileName = "inputdump.xml";
  _dataFilePath = "";
  _parser = new arStructuredDataParser(_lang.getDictionary());
}

arFileSource::~arFileSource(){
  delete _parser;
}

bool arFileSource::init(arSZGClient& SZGClient){
  string temp = SZGClient.getAttribute("SZG_DATA","path");
  if (temp != "NULL"){
    cout << "arFileSource remark: Will write to path " << temp << ".\n";
    _dataFilePath = temp;
  }
  else{
    cout << "arFileSource warning: path not set.\n";
  }
  return true;
}

bool arFileSource::start(){
  if (!_dataStream.ar_open(_dataFileName,"",_dataFilePath)){
    cerr << "arFileSource error: Could not open data file.\n";
    return false;
  }
  _eventThread.beginThread(ar_fileSourceEventTask,this);
  return true;
}
