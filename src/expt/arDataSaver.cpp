#include "arPrecompiled.h"
#include "arDataSaver.h"

bool arDataSaver::setFilePath( std::string& dataPath, arSZGClient& szgClient ) {
  ar_pathAddSlash( dataPath );
  stringstream& errStream = szgClient.startResponse();
  string fileRoot = szgClient.getAttribute("SZG_EXPT","file_name");
  _dataFilePath = dataPath + fileRoot + "_dat." + _fileSuffix;
  cerr << "Set _dataFilePath to '" << _dataFilePath << "'.\n";
  bool isFile, itExists;
  if (!ar_fileExists( _dataFilePath, itExists, isFile )) {
    cerr << "arDataSaver error: file existence check failed for " << _dataFilePath << endl;
    errStream << "arDataSaver error: file existence check failed for " << _dataFilePath << endl;
    return false;
  }
  if (itExists) {
    cerr << "arDataSaver error: an item " << _dataFilePath << " already exists.\n";
    errStream << "arDataSaver error: an item " << _dataFilePath << " already exists.\n";
    return false;
  }
  return true;
}


