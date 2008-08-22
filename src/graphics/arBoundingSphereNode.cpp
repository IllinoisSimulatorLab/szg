//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"
#include "arGlut.h"

arBoundingSphereNode::arBoundingSphereNode() {
  _name = "bounding_sphere_node";
  _typeCode = AR_G_BOUNDING_SPHERE_NODE;
  _typeString = "bounding sphere";
}

void arBoundingSphereNode::draw(arGraphicsContext*) {
  _nodeLock.lock("arBoundingSphereNode::draw");
    const bool vis = _boundingSphere.visibility;
    arVector3 p(_boundingSphere.position);
    const float r = _boundingSphere.radius;
  _nodeLock.unlock();
  if (!vis)
    return;

  glDisable(GL_LIGHTING);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glColor3f(1, 1, 1);
  glPushMatrix();
  glTranslatef(p[0], p[1], p[2]);
  glutWireSphere(r, 15, 15);
  glPopMatrix();
}

bool arBoundingSphereNode::receiveData(arStructuredData* inData) {
  if (!_g->checkNodeID(_g->AR_BOUNDING_SPHERE, inData->getID(), "arBoundingSphereNode"))
    return false;

  const bool vis = inData->getDataInt(_g->AR_BOUNDING_SPHERE_VISIBILITY) ? true : false;
  arGuard _(_nodeLock, "arBoundingSphereNode::receiveData");
  _boundingSphere.visibility = vis;
  inData->dataOut(_g->AR_BOUNDING_SPHERE_RADIUS,
		  &_boundingSphere.radius, AR_FLOAT, 1);
  inData->dataOut(_g->AR_BOUNDING_SPHERE_POSITION,
		  _boundingSphere.position.v, AR_FLOAT, 3);
  return true;
}

arBoundingSphere arBoundingSphereNode::getBoundingSphere() {
  arGuard _(_nodeLock, "arBoundingSphereNode::getBoundingSphere");
  return _boundingSphere;
}

void arBoundingSphereNode::setBoundingSphere(const arBoundingSphere& b) {
  if (active()) {
    _nodeLock.lock("arBoundingSphereNode::setBoundingSphere active");
      arStructuredData* r = _dumpData(b, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arBoundingSphereNode::setBoundingSphere inactive");
    _boundingSphere = b;
  }
}

arStructuredData* arBoundingSphereNode::dumpData() {
  arGuard _(_nodeLock, "arBoundingSphereNode::dumpData");
  return _dumpData(_boundingSphere, false);
}

arStructuredData* arBoundingSphereNode::_dumpData(
  const arBoundingSphere& b, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_BOUNDING_SPHERE);
  _dumpGenericNode(r, _g->AR_BOUNDING_SPHERE_ID);
  const ARint visible = b.visibility ? 1 : 0;
  if (!r->dataIn(_g->AR_BOUNDING_SPHERE_VISIBILITY, &visible, AR_INT,1) ||
      !r->dataIn(_g->AR_BOUNDING_SPHERE_RADIUS, &b.radius, AR_FLOAT, 1) ||
      !r->dataIn(_g->AR_BOUNDING_SPHERE_POSITION, b.position.v, AR_FLOAT, 3)) {
    delete r;
    return NULL;
  }
  return r;
}
