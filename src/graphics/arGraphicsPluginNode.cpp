//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsPluginNode.h"
#include "arGraphicsDatabase.h"
#include "arSTLalgo.h"

#include <map>


arGraphicsPluginNode::arGraphicsPluginNode( bool isGraphicsServer ) {
  _name = "graphics_plugin";
  _typeString = "graphics plugin";
  _typeCode = AR_G_GRAPHICS_PLUGIN_NODE;
  _isGraphicsServer = isGraphicsServer;
  _object = NULL;
  _triedToLoad = false;
}

arGraphicsPluginNode::~arGraphicsPluginNode() {
  if (_object) {
    delete _object;
  }
  _intData.clear();
  _longData.clear();
  _floatData.clear();
  _doubleData.clear();
  _stringData.clear();
}


void arGraphicsPluginNode::draw(arGraphicsContext* context) {
  _nodeLock.lock();
  if (!_object) {
    ar_log_debug() << "arGraphicsPluginNode draw() without valid plugin object." << ar_endl;
    _nodeLock.unlock();
    return;
  }
  arGraphicsWindow* win = context->getWindow();
  if (!win) {
    ar_log_error() << "arGraphicsPluginNode draw() not passed a valid arGraphicsWindow." << ar_endl;
    _nodeLock.unlock();
    return;
  }
  arViewport* vp = context->getViewport();
  if (!vp) {
    ar_log_error() << "arGraphicsPluginNode draw() not passed a valid arViewport." << ar_endl;
    _nodeLock.unlock();
    return;
  }
  _object->draw( *win, *vp );
  _nodeLock.unlock();
}


arStructuredData* arGraphicsPluginNode::dumpData() {
  // Caller is responsible for deleting.
  _nodeLock.lock();
  arStructuredData* r = _dumpData( _fileName, _intData, _longData,
                                   _floatData, _doubleData, _stringData,
                                   false );
  _nodeLock.unlock();
  return r;
}

bool arGraphicsPluginNode::receiveData(arStructuredData* data) {
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(data)) {
    return true;
  }
  if (data->getID() != _g->AR_GRAPHICS_PLUGIN) {
    cerr << "arGraphicsPluginNode error: expected " << _g->AR_GRAPHICS_PLUGIN
         << " (" << _g->_stringFromID(_g->AR_GRAPHICS_PLUGIN) << "), not "
         << data->getID() << " (" << _g->_stringFromID(data->getID()) << ")\n";
    return false;
  }
  _nodeLock.lock();
  std::string newFileName = data->getDataString( _g->AR_GRAPHICS_PLUGIN_NAME );
  if (!_object && (newFileName == "")) {
    ar_log_error() << "arGraphicsPluginNode got empty file name.\n";
    _nodeLock.unlock();
    return false;
  }
  if (_object && (newFileName != _fileName)) {
    ar_log_remark() << "arGraphicsPluginNode about to convert " << _fileName
                   << " to " << newFileName << ar_endl;
    delete[] _object;
    _object = NULL;
    _triedToLoad = false;
  }
  _fileName = newFileName;
  if (!_isGraphicsServer) {
    if (!_object) {
      if (_triedToLoad) {
        _nodeLock.unlock();
        return true;
      }
      ar_log_debug() << "arGraphicsPluginNode attempting to create " << _fileName
                     << " object." << ar_endl;
      _object = _makeObject();
      if (!_object) {
        ar_log_error() << "arGraphicsPluginNode failed to create " << _fileName
                       << " object." << ar_endl;
        _nodeLock.unlock();
        return true;
      }
      ar_log_debug() << "arGraphicsPluginNode successfully created "
                     << _fileName << "object.\n";
    }
  }

  int*    intPtr     = (int*)   data->getDataPtr( _g->AR_GRAPHICS_PLUGIN_INT, AR_INT );
  long*   longPtr    = (long*)  data->getDataPtr( _g->AR_GRAPHICS_PLUGIN_LONG, AR_LONG );
  float*  floatPtr   = (float*) data->getDataPtr( _g->AR_GRAPHICS_PLUGIN_FLOAT, AR_FLOAT );
  double* doublePtr  = (double*)data->getDataPtr( _g->AR_GRAPHICS_PLUGIN_DOUBLE, AR_DOUBLE );
  char*   stringPtr  = (char*)  data->getDataPtr( _g->AR_GRAPHICS_PLUGIN_STRING, AR_CHAR );
  int     numStrings = data->getDataInt( _g->AR_GRAPHICS_PLUGIN_NUMSTRINGS );

  if (numStrings == -1) {
    ar_log_error() << "arGraphicsPluginNode got numStrings==-1 in receiveData().\n";
    _nodeLock.unlock();
    return false;
  }

  if (intPtr) {
    int intSize           = data->getDataDimension( _g->AR_GRAPHICS_PLUGIN_INT );
    _intData.clear();
    _intData.insert(    _intData.begin(), intPtr, intPtr+intSize );
  }
  if (longPtr) {
    int longSize          = data->getDataDimension( _g->AR_GRAPHICS_PLUGIN_LONG );
    _longData.clear();
    _longData.insert(   _longData.begin(), longPtr, longPtr+longSize );
  }
  if (floatPtr) {
    int floatSize         = data->getDataDimension( _g->AR_GRAPHICS_PLUGIN_FLOAT );
    _floatData.clear();
    _floatData.insert(  _floatData.begin(), floatPtr, floatPtr+floatSize );
  }
  if (doublePtr) {
    int doubleSize        = data->getDataDimension( _g->AR_GRAPHICS_PLUGIN_DOUBLE );
    _doubleData.clear();
    _doubleData.insert( _doubleData.begin(), doublePtr, doublePtr+doubleSize );
  }
  if (stringPtr && (numStrings>0)) {
    ar_unpackStringVector( stringPtr, numStrings, _stringData );
  }

  bool stat = true;
  if (!_isGraphicsServer && _object) {
    stat = _object->setState( _intData, _longData, _floatData, _doubleData, _stringData );
  }

  _nodeLock.unlock();
  return stat;
}

// NOT thread-safe.
arStructuredData* arGraphicsPluginNode::_dumpData( const string& fileName,
                                                     std::vector<int>& intData,
                                                     std::vector<long>& longData,
                                                     std::vector<float>& floatData,
                                                     std::vector<double>& doubleData,
                                                     std::vector< std::string >& stringData,
                                                     bool owned ) {
  arStructuredData* result;
  if (owned) {
    result =getOwner()->getDataParser()->getStorage( _g->AR_GRAPHICS_PLUGIN );
  } else {
    result = _g->makeDataRecord( _g->AR_GRAPHICS_PLUGIN );
  }
  _dumpGenericNode( result, _g->AR_GRAPHICS_PLUGIN_ID );

  // Don't use the member variable. Instead, use the function parameter.
  result->dataInString( _g->AR_GRAPHICS_PLUGIN_NAME, fileName );
  
  int* intPtr = new int[intData.size()];
  long* longPtr = new long[longData.size()];
  float* floatPtr = new float[floatData.size()];
  double* doublePtr = new double[doubleData.size()];

  int numStrings = (int)stringData.size();
  unsigned int stringSize;
  char* stringPtr = ar_packStringVector( stringData, stringSize );

  if (!intPtr || !longPtr || !floatPtr || !doublePtr || !stringPtr) {
    ar_log_error() << "arGraphicsPluginNode failed to allocate buffers in _dumpData().\n";
    return NULL;
  }

  std::copy( intData.begin(), intData.end(), intPtr );
  std::copy( longData.begin(), longData.end(), longPtr );
  std::copy( floatData.begin(), floatData.end(), floatPtr );
  std::copy( doubleData.begin(), doubleData.end(), doublePtr );
  
  bool stat = result->dataIn( _g->AR_GRAPHICS_PLUGIN_INT, (const void*)intPtr, AR_INT, intData.size() )
    && result->dataIn( _g->AR_GRAPHICS_PLUGIN_LONG, (const void*)longPtr, AR_LONG, longData.size() )
    && result->dataIn( _g->AR_GRAPHICS_PLUGIN_FLOAT, (const void*)floatPtr, AR_FLOAT, floatData.size() )
    && result->dataIn( _g->AR_GRAPHICS_PLUGIN_DOUBLE, (const void*)doublePtr, AR_DOUBLE, doubleData.size() )
    && result->dataIn( _g->AR_GRAPHICS_PLUGIN_STRING, (const void*)stringPtr, AR_CHAR, stringSize )
    && result->dataIn( _g->AR_GRAPHICS_PLUGIN_NUMSTRINGS, (const void*)&numStrings, AR_INT, 1 );

  delete[] intPtr;
  delete[] longPtr;
  delete[] floatPtr;
  delete[] doublePtr;
  delete[] stringPtr;

  if (!stat) {
    ar_log_error() << "arGraphicsPluginNode failed to dump data in _dumpData()." << ar_endl;
    return NULL;
  }
  
  return result;
}


arGraphicsPlugin* arGraphicsPluginNode::_makeObject() {
  _triedToLoad = true;
  arSharedLib* lib = arGraphicsPluginNode::getSharedLib( _fileName );
  if (!lib) {
    ar_log_error() << "arGraphicsPluginNode failed to load shared library "
                   << _fileName << ar_endl;
    return NULL;
  } 
  arGraphicsPlugin* obj = (arGraphicsPlugin*)lib->createObject();
  if (!obj) {
    ar_log_error() << "arGraphicsPluginNode failed to create object from shared library "
                   << _fileName << ar_endl;
    return NULL;
  }
  return obj;
}


std::string arGraphicsPluginNode::__sharedLibSearchPath;
std::map< std::string, arSharedLib* > arGraphicsPluginNode::__sharedLibMap;

void arGraphicsPluginNode::setSharedLibSearchPath( const std::string& searchPath ) {
  arGraphicsPluginNode::__sharedLibSearchPath = searchPath;
}

arSharedLib* arGraphicsPluginNode::getSharedLib( const std::string& fileName ) {
  ar_log_debug() << "arGraphicsPluginNode attempting to load " << fileName << ar_endl;
  std::map< std::string, arSharedLib* >::iterator iter = arGraphicsPluginNode::__sharedLibMap.find( fileName );
  if (iter != arGraphicsPluginNode::__sharedLibMap.end()) {
    ar_log_debug() << "arGraphicsPluginNode found loaded plugin " << fileName << ar_endl;
    return &*(iter->second);
  }
  arSharedLib* lib = new arSharedLib;
  arGraphicsPluginNode::__sharedLibMap[fileName] = lib;
  string error;
  if (!lib->createFactory( fileName, arGraphicsPluginNode::__sharedLibSearchPath,
        "arGraphicsPlugin", error )) {
    ar_log_error() << "arGraphicsPlugin got the following error in createFactory:"
                   << ar_endl << "     " << error << ar_endl;
    return NULL;
  }
  return lib;
}



