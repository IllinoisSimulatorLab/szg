//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"
#include "arGlut.h"

arBoundingSphereNode::arBoundingSphereNode(){
  _name = "bounding_sphere_node";
  _typeCode = AR_G_BOUNDING_SPHERE_NODE;
  _typeString = "bounding sphere";
}

void arBoundingSphereNode::draw(arGraphicsContext*){
  _nodeLock.lock();
    const bool vis = _boundingSphere.visibility;
    arVector3 p(_boundingSphere.position);
    const float r = _boundingSphere.radius;
  _nodeLock.unlock();
  if (!vis)
    return;

  glDisable(GL_LIGHTING);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
  glColor3f(1,1,1);
  glPushMatrix();
  glTranslatef(p[0], p[1], p[2]);
  glutWireSphere(r, 15, 15);
  glPopMatrix();
}

arStructuredData* arBoundingSphereNode::dumpData(){
  // This record will be deleted by the caller.
  _nodeLock.lock();
    arStructuredData* r = _dumpData(_boundingSphere, false);
  _nodeLock.unlock();
  return r;
}

bool arBoundingSphereNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_BOUNDING_SPHERE, inData->getID(), "arBoundingSphereNode"))
    return false;

  _nodeLock.lock();
  const ARint vis = inData->getDataInt(_g->AR_BOUNDING_SPHERE_VISIBILITY);
  _boundingSphere.visibility = vis ? true : false;
  inData->dataOut(_g->AR_BOUNDING_SPHERE_RADIUS,
		  &_boundingSphere.radius, AR_FLOAT, 1);
  inData->dataOut(_g->AR_BOUNDING_SPHERE_POSITION,
		  _boundingSphere.position.v, AR_FLOAT, 3);
  _nodeLock.unlock();
  return true;
}

arBoundingSphere arBoundingSphereNode::getBoundingSphere(){
  _nodeLock.lock();
  arBoundingSphere result = _boundingSphere;
  _nodeLock.unlock();
  return result;
}

void arBoundingSphereNode::setBoundingSphere(const arBoundingSphere& b){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(b, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _boundingSphere = b;
    _nodeLock.unlock();
  }
}

// Not thread-safe.
arStructuredData* arBoundingSphereNode::_dumpData(const arBoundingSphere& b,
                                                  bool owned){
  arStructuredData* result = owned ?
    _owningDatabase->getDataParser()->getStorage(_g->AR_BOUNDING_SPHERE) :
    _g->makeDataRecord(_g->AR_BOUNDING_SPHERE);
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
