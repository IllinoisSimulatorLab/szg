//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arBoundingSphereNode::arBoundingSphereNode(){
  // A sensible default name.
  _name = "bounding_sphere_node";
  _typeCode = AR_G_BOUNDING_SPHERE_NODE;
  _typeString = "bounding sphere";
}

void arBoundingSphereNode::draw(arGraphicsContext*){
  if (_boundingSphere.visibility){
    glDisable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
    glColor3f(1,1,1);
    glPushMatrix();
    glTranslatef(_boundingSphere.position[0], 
                 _boundingSphere.position[1], 
                 _boundingSphere.position[2]);
    glutWireSphere(_boundingSphere.radius, 15, 15);
    glPopMatrix();
  }
}

arStructuredData* arBoundingSphereNode::dumpData(){
  return _dumpData(_boundingSphere);
}

bool arBoundingSphereNode::receiveData(arStructuredData* inData){
  if (inData->getID() == _g->AR_BOUNDING_SPHERE){
    const ARint vis = inData->getDataInt(_g->AR_BOUNDING_SPHERE_VISIBILITY);
    _boundingSphere.visibility = vis ? true : false;
    inData->dataOut(_g->AR_BOUNDING_SPHERE_RADIUS,
                    &_boundingSphere.radius, AR_FLOAT, 1);
    inData->dataOut(_g->AR_BOUNDING_SPHERE_POSITION,
                    _boundingSphere.position.v, AR_FLOAT, 3);
    return true;
  }

  cerr << "arBoundingSphereNode error: expected "
       << _g->AR_BOUNDING_SPHERE
       << " (" << _g->_stringFromID(_g->AR_BOUNDING_SPHERE) << "), not "
       << inData->getID()
       << " (" << _g->_stringFromID(inData->getID()) << ")\n";
  return false;
}

void arBoundingSphereNode::setBoundingSphere(const arBoundingSphere& b){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(b);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _boundingSphere = b;
  }
}

arStructuredData* arBoundingSphereNode::_dumpData(const arBoundingSphere& b){
  arStructuredData* result = _g->makeDataRecord(_g->AR_BOUNDING_SPHERE);
  _dumpGenericNode(result,_g->AR_BOUNDING_SPHERE_ID);
  const ARint visible = b.visibility ? 1 : 0;
  if (!result->dataIn(_g->AR_BOUNDING_SPHERE_VISIBILITY, &visible, AR_INT,1) ||
      !result->dataIn(_g->AR_BOUNDING_SPHERE_RADIUS,
        &b.radius, AR_FLOAT, 1) ||
      !result->dataIn(_g->AR_BOUNDING_SPHERE_POSITION,
        b.position.v, AR_FLOAT, 3)) {
    delete result;
    return NULL;
  }
  return result;
}


