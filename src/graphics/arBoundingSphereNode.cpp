//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arBoundingSphereNode::arBoundingSphereNode(){
  // A sensible default name.
  _name = "bounding_sphere_node";
  _typeCode = AR_G_BOUNDING_SPHERE_NODE;
  _typeString = "bounding sphere";
}

void arBoundingSphereNode::draw(arGraphicsContext*){
  ar_mutex_lock(&_nodeLock);
  bool vis = _boundingSphere.visibility;
  arVector3 p = _boundingSphere.position;
  float r = _boundingSphere.radius;
  ar_mutex_unlock(&_nodeLock);
  if (vis){
    glDisable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
    glColor3f(1,1,1);
    glPushMatrix();
    glTranslatef(p[0], p[1], p[2]);
    glutWireSphere(r, 15, 15);
    glPopMatrix();
  }
}

arStructuredData* arBoundingSphereNode::dumpData(){
  // This record will be deleted by the caller.
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r = _dumpData(_boundingSphere, false);
  ar_mutex_unlock(&_nodeLock);
  return r;
}

bool arBoundingSphereNode::receiveData(arStructuredData* inData){
  if (inData->getID() == _g->AR_BOUNDING_SPHERE){
    ar_mutex_lock(&_nodeLock);
    const ARint vis = inData->getDataInt(_g->AR_BOUNDING_SPHERE_VISIBILITY);
    _boundingSphere.visibility = vis ? true : false;
    inData->dataOut(_g->AR_BOUNDING_SPHERE_RADIUS,
                    &_boundingSphere.radius, AR_FLOAT, 1);
    inData->dataOut(_g->AR_BOUNDING_SPHERE_POSITION,
                    _boundingSphere.position.v, AR_FLOAT, 3);
    ar_mutex_unlock(&_nodeLock);
    return true;
  }

  cerr << "arBoundingSphereNode error: expected "
       << _g->AR_BOUNDING_SPHERE
       << " (" << _g->_stringFromID(_g->AR_BOUNDING_SPHERE) << "), not "
       << inData->getID()
       << " (" << _g->_stringFromID(inData->getID()) << ")\n";
  return false;
}


arBoundingSphere arBoundingSphereNode::getBoundingSphere(){
  ar_mutex_lock(&_nodeLock);
  arBoundingSphere result = _boundingSphere;
  ar_mutex_unlock(&_nodeLock);
  return result;
}

void arBoundingSphereNode::setBoundingSphere(const arBoundingSphere& b){
  if (active()){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(b, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _boundingSphere = b;
    ar_mutex_unlock(&_nodeLock);
  }
}

/// Not thread-safe.
arStructuredData* arBoundingSphereNode::_dumpData(const arBoundingSphere& b,
                                                  bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result 
      = _owningDatabase->getDataParser()->getStorage(_g->AR_BOUNDING_SPHERE);
  }
  else{
    result = _g->makeDataRecord(_g->AR_BOUNDING_SPHERE);
  }
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


