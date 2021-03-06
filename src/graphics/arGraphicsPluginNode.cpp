//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsPluginNode.h"
#include "arGraphicsDatabase.h"
#include "arSTLalgo.h"

#include <map>

arGraphicsPluginNode::arGraphicsPluginNode( bool isGraphicsServer )
{
  _setName ( "graphics_plugin" );
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
  arGuard _(_nodeLock, "arGraphicsPluginNode::draw");
  if (!_object) {
    ar_log_debug() << "arGraphicsPluginNode draw(): invalid plugin object.\n";
    return;
  }
  arGraphicsWindow* win = context->getWindow();
  if (!win) {
    ar_log_error() << "arGraphicsPluginNode draw(): invalid arGraphicsWindow.\n";
    return;
  }
  arViewport* vp = context->getViewport();
  if (!vp) {
    ar_log_error() << "arGraphicsPluginNode draw(): invalid arViewport.\n";
    return;
  }
  _object->draw( *win, *vp );
}

bool arGraphicsPluginNode::receiveData(arStructuredData* data) {
  // Get the name change record, for instance, if sent.
  if (arDatabaseNode::receiveData(data)) {
    return true;
  }
  if (!_g->checkNodeID(_g->AR_GRAPHICS_PLUGIN, data->getID(), "arGraphicsPluginNode")) {
    return false;
  }

  arGuard _(_nodeLock, "arGraphicsPluginNode::receiveData");
  std::string newFileName = data->getDataString( _g->AR_GRAPHICS_PLUGIN_NAME );
  if (!_object && (newFileName == "")) {
    ar_log_error() << "arGraphicsPluginNode ignoring empty file name.\n";
    return false;
  }

  if (_object && (newFileName != _fileName)) {
    ar_log_remark() << "arGraphicsPluginNode converting " << _fileName << " to " <<
      newFileName << "\n";
    delete[] _object;
    _object = NULL;
    _triedToLoad = false;
  }
  _fileName = newFileName;
  if (!_isGraphicsServer) {
    if (!_object) {
      if (_triedToLoad) {
        return true;
      }
      ar_log_debug() << "arGraphicsPluginNode creating " << _fileName << " object.\n";
      _object = _makeObject();
      if (!_object) {
        ar_log_error() << "arGraphicsPluginNode failed to create " << _fileName << " object.\n";
        return false;
      }
      ar_log_debug() << "arGraphicsPluginNode created " << _fileName << "object.\n";
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

  return _isGraphicsServer || !_object || _object->setState( _intData, _longData, _floatData, _doubleData, _stringData );
}

arStructuredData* arGraphicsPluginNode::dumpData() {
  arGuard _(_nodeLock, "arGraphicsPluginNode::dumpData");
  return _dumpData(
    _fileName, _intData, _longData, _floatData, _doubleData, _stringData, false );
}

arStructuredData* arGraphicsPluginNode::_dumpData( const string& fileName,
                                                     std::vector<int>& intData,
                                                     std::vector<long>& longData,
                                                     std::vector<float>& floatData,
                                                     std::vector<double>& doubleData,
                                                     std::vector< std::string >& stringData,
                                                     bool owned ) {
  arStructuredData* r = _getRecord(owned, _g->AR_GRAPHICS_PLUGIN);
  _dumpGenericNode( r, _g->AR_GRAPHICS_PLUGIN_ID );

  // Use the function arg, not the member variable.
  r->dataInString( _g->AR_GRAPHICS_PLUGIN_NAME, fileName );

  int* intPtr = new int[intData.size()];
  long* longPtr = new long[longData.size()];
  float* floatPtr = new float[floatData.size()];
  double* doublePtr = new double[doubleData.size()];
  unsigned stringSize;
  char* stringPtr = ar_packStringVector( stringData, stringSize );

  if (!intPtr || !longPtr || !floatPtr || !doublePtr || !stringPtr) {
    ar_log_error() << "arGraphicsPluginNode _dumpData() out of memory.\n";
    return NULL;
  }

  std::copy( intData.begin(), intData.end(), intPtr );
  std::copy( longData.begin(), longData.end(), longPtr );
  std::copy( floatData.begin(), floatData.end(), floatPtr );
  std::copy( doubleData.begin(), doubleData.end(), doublePtr );
  const int numStrings = (int)stringData.size();

  const bool ok = r->dataIn( _g->AR_GRAPHICS_PLUGIN_INT, (const void*)intPtr, AR_INT, intData.size() )
    && r->dataIn( _g->AR_GRAPHICS_PLUGIN_LONG, (const void*)longPtr, AR_LONG, longData.size() )
    && r->dataIn( _g->AR_GRAPHICS_PLUGIN_FLOAT, (const void*)floatPtr, AR_FLOAT, floatData.size() )
    && r->dataIn( _g->AR_GRAPHICS_PLUGIN_DOUBLE, (const void*)doublePtr, AR_DOUBLE, doubleData.size() )
    && r->dataIn( _g->AR_GRAPHICS_PLUGIN_STRING, (const void*)stringPtr, AR_CHAR, stringSize )
    && r->dataIn( _g->AR_GRAPHICS_PLUGIN_NUMSTRINGS, (const void*)&numStrings, AR_INT, 1 );

  delete[] intPtr;
  delete[] longPtr;
  delete[] floatPtr;
  delete[] doublePtr;
  delete[] stringPtr;

  if (!ok) {
    ar_log_error() << "arGraphicsPluginNode _dumpData() failed.\n";
    return NULL;
  }

  return r;
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
  ar_log_debug() << "arGraphicsPluginNode loading " << fileName << ar_endl;
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
    ar_log_error() << "arGraphicsPlugin createFactory: " << error << ar_endl;
    return NULL;
  }
  return lib;
}



