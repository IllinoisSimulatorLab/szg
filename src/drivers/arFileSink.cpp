//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFileSink.h"

arFileSink::arFileSink(){
  // note that we do not want this sink to be automatically started
  // by the server
  _autoActivate = false;
  _dataFileName = "inputdump.xml";
  _dataFilePath = "";
  _dataFile = NULL;
  _logging = false;
  ar_mutex_init(&_logLock);
}

arFileSink::~arFileSink(){
}

bool arFileSink::init(arSZGClient& SZGClient){
  const string& temp = SZGClient.getAttribute("SZG_DATA","path");
  if (temp != "NULL"){
    cout << "arFileSink remark: will write to path " << temp << ".\n";
    _dataFilePath = temp;
  }
  else{
    cerr << "arFileSink warning: no path.\n";
  }
  return true;
}

bool arFileSink::start(){
  if (_logging){
    cerr << "arFileSink warning: already logging.\n";
    return true;
  }
  _dataFile = ar_fileOpen(_dataFileName,_dataFilePath,"w");
  if (!_dataFile){
    cerr << "arFileSink error: failed to log to data file.\n";
    return false;
  }

  _logging = true;
  cout << "arFileSink remark: logging.\n";
  return true;
}

bool arFileSink::stop(){
  if (!_logging){
    cerr << "arFileSink warning: logging already stopped.\n";
    return true;
  }
  ar_mutex_lock(&_logLock);
  _logging = false;
  if (_dataFile)
    fclose(_dataFile);
  ar_mutex_unlock(&_logLock);
  cout << "arFileSink remark: stopped logging.\n";
  return true;
}

void arFileSink::receiveData(int /*ID*/, arStructuredData* data){
  ar_mutex_lock(&_logLock);
  if (_logging)
    data->print(_dataFile);
  ar_mutex_unlock(&_logLock);
}
