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
  _autoActivate = false; // override parent's constructor
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
    // Complain only when it's about to get used.
    ar_log_warning() << "arFileSink has no SZG_DATA/path.\n";
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

  arGuard dummy(_logLock);
  _logging = false;
  if (_dataFile)
    fclose(_dataFile);
  return true;
}

void arFileSink::receiveData(int /*ID*/, arStructuredData* data) const {
  arGuard dummy(_logLock);
  if (_logging && data)
    data->print(_dataFile);
}
