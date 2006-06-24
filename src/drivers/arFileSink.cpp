//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFileSink.h"

arFileSink::arFileSink() :
  _dataFilePath(""),
  _dataFileName("inputdump.xml"),
  _dataFile(NULL),
  _logging(false)
{
  _autoActivate = false; // parent class -- how do you do that with an initializer??
  ar_mutex_init(&_logLock);
}

// todo: add set and get functions for _dataFileName

bool arFileSink::init(arSZGClient& SZGClient){
  _dataFilePath = SZGClient.getDataPath();
  return true;
}

bool arFileSink::start(){
  if (_logging){
    ar_log_warning() << "arFileSink already logging to '" << _dataFilePath <<
      "/" << _dataFileName << "'.\n";
    return true;
  }

  if (_dataFilePath == "NULL") {
    // Only complain when it's about to get used.
    ar_log_warning() << "arFileSink has undefined SZG_DATA/path.\n";
    return false;
  }

  _dataFile = ar_fileOpen(_dataFileName,_dataFilePath,"w");
  if (!_dataFile){
    ar_log_warning() << "arFileSink failed to log to '" << _dataFilePath <<
      "/" << _dataFileName << "'.\n";
    return false;
  }

  _logging = true;
  return true;
}

bool arFileSink::stop(){
  if (!_logging){
    ar_log_remark() << "arFileSink already stopped logging.\n";
    return true;
  }

  ar_mutex_lock(&_logLock);
    _logging = false;
    if (_dataFile)
      fclose(_dataFile);
  ar_mutex_unlock(&_logLock);

  return true;
}

void arFileSink::receiveData(int /*ID*/, arStructuredData* data){
  ar_mutex_lock(&_logLock);
  if (_logging)
    data->print(_dataFile);
  ar_mutex_unlock(&_logLock);
}
