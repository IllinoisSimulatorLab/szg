//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arBumpMapNode.h"
#include "arGraphicsDatabase.h"

arBumpMapNode::arBumpMapNode(){
  _typeCode = AR_G_BUMP_MAP_NODE;
  _typeString = "bump map";
}

arStructuredData* arBumpMapNode::dumpData(){
  int i, currPos = 0;	// this will keep track of where we are in commandBuffer
  arStructuredData* result = _g->makeDataRecord(_g->AR_BUMPMAP);
  _dumpGenericNode(result,_g->AR_BUMPMAP_ID);

  // filename
  int len = (ARint) _commandBuffer.v[currPos++]; // characters in filename
  result->setDataDimension(_g->AR_BUMPMAP_FILE, len);
  ARchar* text = (ARchar*) result->getDataPtr(_g->AR_BUMPMAP_FILE,AR_CHAR);
  for (i=0; i<len; ++i)
    text[i] = (ARint) _commandBuffer.v[currPos++];
  
  // height
  result->dataIn(_g->AR_BUMPMAP_HEIGHT, &_commandBuffer.v[currPos++], AR_FLOAT, 1);

  return result;
}

bool arBumpMapNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_BUMPMAP){
    cerr << "arBumpMapNode error: expected "
         << _g->AR_BUMPMAP
         << " (" << _g->_stringFromID(_g->AR_BUMPMAP) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << "), for node \""
	 << getName() << "\".\n";
    return false;
  }
  int currPos = 0;	// keeps track of our position in commandBuffer

  string filename(inData->getDataString(_g->AR_BUMPMAP_FILE));
  const int flen = filename.length();
  float height = inData->getDataFloat(_g->AR_BUMPMAP_HEIGHT);

  if (!_points) {
    cerr << "arBumpMapNode error: No points for bump map...\n";
    return false;
  }
  if (!_tex2) {
    cerr << "arBumpMapNode error: No texture coords for bump map...\n";
    return false;
  }
    
  _localBumpMap = _owningDatabase->addBumpMap(filename, _points->size(),
		  			      _index?_index->size():0,
		  			      _points->v, _index?((int*)_index->v):NULL,
					      _tex2->v, height,
					      _texture ? *_texture : NULL );
  _bumpMap = &_localBumpMap;
/*
  for (int i=0; i<5; ++i)
    printf("D[%i]: P(%0.2f,%0.2f,%0.2f) I(%i) T(%0.2f,%0.2f)\n", i,
           _points->v[3*i], _points->v[3*i+1], _points->v[3*i+2],
           (int)(_index->v[i]),
           _tex2->v[2*i], _tex2->v[2*i+1]);
*/
  _commandBuffer.grow(flen + 2);

  // filename
  _commandBuffer.v[currPos++] = (float)flen;
  for (int i=0; i<flen; i++)
    _commandBuffer.v[currPos++] = (float) filename.data()[i];

  // height
  _commandBuffer.v[currPos++] = height;

  return true;
}
