//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDrawableNode.h"
#include "arMath.h"
#include "arGraphicsDatabase.h"

arDrawableNode::arDrawableNode():
  _firstMessageReceived(false),
  _type(DG_POINTS),
  _number(0) {
  _name = "drawable_node";
  // does not compile on RedHat 8 if these statements appear outside the
  // constructor body.
  _typeCode = AR_G_DRAWABLE_NODE;
  _typeString = "drawable";
}

void arDrawableNode::draw(arGraphicsContext* context) {
  // One message to the database creates the database node.
  // the database. A later message initializes the node.
  // If both messages haven't arrived, there's nothing to draw.
  if (!_firstMessageReceived)
    return;

  _nodeLock.lock("arDrawableNode::draw");
    const ARint whatKind = _type;
    ARint howMany = _number; // May change as we check array bounds.
  _nodeLock.unlock();

  int numberPos = 0;
  // Each vertex sent down the geometry pipeline has info associated with it.
  // Compute maxNumber, the max # of drawn vertices as determined
  // by the sizes of arrays like normals and tex coords.
  int maxNumber = -1;
  // Get the context.
  arGraphicsNode* pNode = NULL;
  arGraphicsNode* iNode = NULL;
  arGraphicsNode* nNode = NULL;
  arGraphicsNode* cNode = NULL;
  arGraphicsNode* t2Node = NULL;
  arGraphicsNode* tNode = NULL;
  if (context) {
    // In the draw functions, ensure there are an appropriate number of points.
    // To do this, pass in the SIZE of the points array.
    pNode = (arGraphicsNode*) context->getNode(AR_G_POINTS_NODE);
    if (pNode)
      numberPos = pNode->getBufferSize()/3;
    iNode = (arGraphicsNode*) context->getNode(AR_G_INDEX_NODE);
    if (iNode)
      // An index node will be used in drawing; determine amount of data.
      maxNumber = iNode->getBufferSize();
    else
      // No index node exists. We will be using the points one after another.
      maxNumber = numberPos;
    nNode = (arGraphicsNode*) context->getNode(AR_G_NORMAL3_NODE);
    // If a normals node will be used in drawing, determine amount of data.
    if (nNode && (maxNumber < 0 || nNode->getBufferSize()/3 < maxNumber)) {
      maxNumber = nNode->getBufferSize()/3;
    }
    cNode = (arGraphicsNode*) context->getNode(AR_G_COLOR4_NODE);
    // If a color node will be used in drawing, determine amount of data.
    if (cNode && (maxNumber < 0 || cNode->getBufferSize()/4 < howMany)) {
      maxNumber = cNode->getBufferSize()/4;
    }
    t2Node = (arGraphicsNode*) context->getNode(AR_G_TEX2_NODE);
    // If a tex2 coordinate node will be used in drawing, determine amount of data.
    if (t2Node && (maxNumber < 0 || t2Node->getBufferSize()/2 < howMany)) {
      maxNumber = t2Node->getBufferSize()/2;
    }
    // The texture node doesn't affect bounds checking.
    tNode = (arGraphicsNode*) context->getNode(AR_G_TEXTURE_NODE);
  }

  float blendFactor = 1.0;
  switch (whatKind) {
  case DG_POINTS:
    if (_0DPreDraw(pNode, context, blendFactor)) {
      // Truncate based on array sizes. A point set draws howMany vertices.
      howMany = howMany <= maxNumber ? howMany : maxNumber;
      ar_drawPoints(howMany,
                    (const int*) (iNode ? iNode->getBuffer() : NULL),
                    numberPos,
                    (const float*) (pNode ? pNode->getBuffer() : NULL),
                    (const float*) (cNode ? cNode->getBuffer(): NULL), blendFactor);
    }
    break;
  case DG_LINES:
    if (_1DPreDraw(pNode, context, blendFactor)) {
      // Truncate based on array sizes. A line set draws 2*howMany vertices.
      // howMany = number of lines.
      howMany = howMany <= maxNumber/2 ? howMany : maxNumber/2;
      ar_drawLines(howMany,
                   (const int*) (iNode ? iNode->getBuffer() : NULL),
                   numberPos,
                   (const float*) (pNode ? pNode->getBuffer() : NULL),
                   (const float*) (cNode ? cNode->getBuffer() : NULL), blendFactor);
    }
    break;
  case DG_LINE_STRIP:
    if (_1DPreDraw(pNode, context, blendFactor)) {
      // Truncate based on array sizes. A line strip draws 1+howMany vertices.
      // howMany = number of lines.
      howMany = howMany <= maxNumber-1 ? howMany : maxNumber-1;
      ar_drawLineStrip(howMany,
                       (const int*) (iNode ? iNode->getBuffer() : NULL),
                       numberPos,
                       (const float*) (pNode ? pNode->getBuffer() : NULL),
                       (const float*) (cNode ? cNode->getBuffer() : NULL),
                       blendFactor);
    }
    break;
  case DG_TRIANGLES:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)) {
      // Truncate based on array sizes. A triangle set draws 3*howMany
      // vertices. howMany = number of triangles.
      howMany = howMany <= maxNumber/3 ? howMany : maxNumber/3;
      // A failed experiment with CG code once was here.
      ar_drawTriangles(howMany,
                       (const int*) (iNode ? iNode->getBuffer() : NULL),
                       numberPos,
                       (const float*) (pNode ? pNode->getBuffer() : NULL),
                       (const float*) (nNode ? nNode->getBuffer() : NULL),
                       (const float*) (cNode ? cNode->getBuffer() : NULL),
                       (const float*) (tNode && t2Node ? t2Node->getBuffer() : NULL),
                       blendFactor);
    }
    break;
  case DG_TRIANGLE_STRIP:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)) {
      // Truncate based on array sizes. A triangle strip draws 2+howMany
      // vertices. howMany = number of triangles.
      howMany = howMany <= maxNumber-2 ? howMany : maxNumber-2;
      ar_drawTriangleStrip(howMany,
                           (const int*) (iNode ? iNode->getBuffer() : NULL),
                           numberPos,
                           (const float*) (pNode ? pNode->getBuffer(): NULL),
                           (const float*) (nNode ? nNode->getBuffer() : NULL),
                           (const float*) (cNode ? cNode->getBuffer() : NULL),
                           (const float*) (t2Node ? t2Node->getBuffer() : NULL),
                           blendFactor);
    }
    break;
  case DG_QUADS:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)) {
      // Truncate based on array sizes. A quad set draws 4*howMany vertices.
      // howMany = number of quads.
      howMany = howMany <= maxNumber/4 ? howMany : maxNumber/4;
      ar_drawQuads(howMany,
                   (const int*) (iNode ? iNode->getBuffer() : NULL),
                   numberPos,
                   (const float*) (pNode ? pNode->getBuffer() : NULL),
                   (const float*) (nNode ? nNode->getBuffer() : NULL),
                   (const float*) (cNode ? cNode->getBuffer() : NULL),
                   (const float*) (t2Node ? t2Node->getBuffer() : NULL),
                   blendFactor);
    }
    break;
  case DG_QUAD_STRIP:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)) {
      // Truncate based on array sizes. A quad set draws 2*howMany+2 vertices.
      // howMany = number of quads.
      howMany = howMany <= maxNumber/2 - 1 ? howMany : maxNumber/2 - 1;
      ar_drawQuadStrip(howMany,
                       (const int*) (iNode ? iNode->getBuffer() : NULL),
                       numberPos,
                       (const float*) (pNode ? pNode->getBuffer() : NULL),
                       (const float*) (nNode ? nNode->getBuffer() : NULL),
                       (const float*) (cNode ? cNode->getBuffer() : NULL),
                       (const float*) (t2Node ? t2Node->getBuffer() : NULL),
                       blendFactor);
    }
    break;
  case DG_POLYGON:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)) {
      // Truncate based on array sizes. A polygon draws howMany vertices.
      howMany = howMany <= maxNumber ? howMany : maxNumber;
      ar_drawPolygon(howMany,
                     (const int*) (iNode ? iNode->getBuffer() : NULL),
                     numberPos,
                     (const float*) (pNode ? pNode->getBuffer() : NULL),
                     (const float*) (nNode ? nNode->getBuffer() : NULL),
                     (const float*) (cNode ? cNode->getBuffer() : NULL),
                     (const float*) (t2Node ? t2Node->getBuffer() : NULL),
                     blendFactor);
    }
    break;
  default:
    cerr << "arDrawableNode warning: ignoring unexpected arDrawableType "
         << whatKind << ".\n";
    break;
  }
}

bool arDrawableNode::receiveData(arStructuredData* inData) {
  // Got second node-init message (after node-create message).
  _firstMessageReceived = true;

  if (!_g->checkNodeID(_g->AR_DRAWABLE, inData->getID(), "arDrawableNode"))
    return false;

  arGuard _(_nodeLock, "arDrawableNode::receiveData");
  _type = inData->getDataInt(_g->AR_DRAWABLE_TYPE);
  _number = inData->getDataInt(_g->AR_DRAWABLE_NUMBER);
  return true;
}

int arDrawableNode::getType() {
  arGuard _(_nodeLock, "arDrawableNode::getType");
  return _type;
}

string arDrawableNode::getTypeAsString() {
  return _convertTypeToString(getType());
}

int arDrawableNode::getNumber() {
  arGuard _(_nodeLock, "arDrawableNode::getNumber");
  return _number;
}

void arDrawableNode::setDrawable(arDrawableType type, int number) {
  if (active()) {
    _nodeLock.lock("arDrawableNode::setDrawable active");
      arStructuredData* r = _dumpData(type, number, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arDrawableNode::setDrawable inactive");
    _type = type;
    _number = number;
  }
}

void arDrawableNode::setDrawableViaString(const string& type, int number) {
  setDrawable(_convertStringToType(type), number);
}

arStructuredData* arDrawableNode::dumpData() {
  arGuard _(_nodeLock, "arDrawableNode::dumpData");
  return _dumpData(_type, _number, false);
}

arStructuredData* arDrawableNode::_dumpData(int type, int number, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_DRAWABLE);
  _dumpGenericNode(r, _g->AR_DRAWABLE_ID);
  if (!r->dataIn(_g->AR_DRAWABLE_TYPE, &type, AR_INT, 1) ||
      !r->dataIn(_g->AR_DRAWABLE_NUMBER, &number, AR_INT, 1)) {
    delete r;
    return NULL;
  }
  return r;
}

bool arDrawableNode::_0DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsContext* context,
                                float& blendFactor) {
  if (!pointsNode) {
    return false;
  }
  if (context) {
    context->setPointState(blendFactor);
  }
  return true;
}

bool arDrawableNode::_1DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsContext* context,
                                float& blendFactor) {
  if (!pointsNode) {
    return false;
  }
  if (context) {
    context->setLineState(blendFactor);
  }
  return true;
}

bool arDrawableNode::_2DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsNode* normal3Node,
                                arGraphicsContext* context,
                                float& blendFactor) {
  // Some data is necessary to draw triangles.
  if (!pointsNode) {
    cerr << "arDrawableNode error: missing points set for 2D geometry.\n";
    return false;
  }
  if (!normal3Node) {
    cerr << "arDrawableNode error: missing normals set for 2D geometry.\n";
    return false;
  }
  if (context) {
    context->setTriangleState(blendFactor);
  }
  return true;
}

string arDrawableNode::_convertTypeToString(int type) {
  switch(type) {
  case DG_POINTS:
    return "points";
  case DG_LINES:
    return "lines";
  case DG_LINE_STRIP:
    return "line_strip";
  case DG_TRIANGLES:
    return "triangles";
  case DG_TRIANGLE_STRIP:
    return "triangle_strip";
  case DG_QUADS:
    return "quads";
  case DG_QUAD_STRIP:
    return "quad_strip";
  case DG_POLYGON:
    return "polygon";
  default:
    return "points";
  }
}

arDrawableType arDrawableNode::_convertStringToType(const string& type) {
  if (type == "points") {
    return DG_POINTS;
  }
  if (type == "lines") {
    return DG_LINES;
  }
  if (type == "line_strip") {
    return DG_LINE_STRIP;
  }
  if (type == "triangles") {
    return DG_TRIANGLES;
  }
  if (type == "triangle_strip") {
    return DG_TRIANGLE_STRIP;
  }
  if (type == "quads") {
    return DG_QUADS;
  }
  if (type == "quad_strip") {
    return DG_QUAD_STRIP;
  }
  if (type == "polygon") {
    return DG_POLYGON;
  }
  return DG_POINTS;
}
