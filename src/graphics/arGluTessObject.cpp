#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGluTessObject.h"
#include "arLogStream.h"
#include "stdlib.h" // for malloc

//****************************************************
// gluTess callbacks
//****************************************************

#ifndef CALLBACK
#define CALLBACK
#endif

void CALLBACK beginCallback(GLenum which) {
  glBegin(which);
}

void CALLBACK errorCallback(GLenum errorCode) {
   const GLubyte *estring;

   estring = gluErrorString(errorCode);
   ar_log_error() << "Tessellation Error: " << static_cast<const unsigned char*>(estring) << ar_endl;
}

void CALLBACK endCallback(void) {
   glEnd();
}

void CALLBACK vertexCallback(GLvoid *vertex,void* userData) {
  GLdouble* ptr = (GLdouble *)vertex;
  float* texInfo = (float*)userData;
  float sTexOffset = texInfo[0];
  float tTexOffset = texInfo[1];
  float sTexScale = texInfo[2];
  float tTexScale = texInfo[3];
  glTexCoord2d( sTexOffset + sTexScale*ptr[0], tTexOffset + tTexScale*ptr[1] );
  glVertex3dv( ptr );
}

void CALLBACK combineCallback(GLdouble coords[3], GLdouble *data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
   GLdouble *vertex;
   vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));

   vertex[0] = coords[0];
   vertex[1] = coords[1];
   vertex[2] = coords[2];
   
   *dataOut = vertex;
}


arGluTessObject::arGluTessObject( bool useDisplayList ) :
  arDrawable(),
  _displayListID(0),
  _useDisplayList(useDisplayList),
  _displayListDirty(true),
  _thickness( 0.1 ),
  _scaleFactors(1,1,1),
  _windingRule(GLU_TESS_WINDING_ODD) {
    setTextureScales(1.,1.);
    setTextureOffsets(0.,0.);
}

arGluTessObject::arGluTessObject( const arGluTessObject& x ) :
  arDrawable(x),
  _displayListID(0),
  _useDisplayList(x._useDisplayList),
  _displayListDirty(true),
  _thickness(x._thickness),
  _scaleFactors(x._scaleFactors),
  _windingRule(x._windingRule),
  _contours(x._contours) {
    setTextureOffsets(x._textureScaleInfo[0],x._textureScaleInfo[1]);
    setTextureScales(x._textureScaleInfo[2],x._textureScaleInfo[3]);
}

arGluTessObject& arGluTessObject::operator=( const arGluTessObject& x ) {
  if (&x == this)
    return *this;  
  _useDisplayList = x._useDisplayList;
  _displayListDirty = true;
  _thickness = x._thickness;
  _scaleFactors = x._scaleFactors;
  _windingRule = x._windingRule;
  _contours = x._contours;  
  setTextureOffsets(x._textureScaleInfo[0],x._textureScaleInfo[1]);
  setTextureScales(x._textureScaleInfo[2],x._textureScaleInfo[3]);
  return *this;
}

arGluTessObject::~arGluTessObject() {
  if (_displayListID != 0)
    glDeleteLists( _displayListID, 1 );
}

void arGluTessObject::useDisplayList( bool use ) {
  _useDisplayList = use;
}

void arGluTessObject::setTextureScales( float sScale, float tScale ) {
  _textureScaleInfo[2] = sScale;
  _textureScaleInfo[3] = tScale;
  _displayListDirty = true;
}

void arGluTessObject::setTextureOffsets( float sOffset, float tOffset ) {
  _textureScaleInfo[0] = sOffset;
  _textureScaleInfo[1] = tOffset;
  _displayListDirty = true;
}

void arGluTessObject::draw() {
  if (!_visible) {
      return;
  }
  glPushMatrix();
  glMultMatrixf( ar_scaleMatrix(_scaleFactors).v );
  glPushMatrix();
  glTranslatef( 0, 0, _thickness );
  if (_useDisplayList && (_displayListDirty || _displayListID==0))
    buildDisplayList();
  if (_useDisplayList && (_displayListID!=0)) {
    glCallList( _displayListID );
  } else {
    _drawTess();
  }
  glPopMatrix();
  glPushMatrix();
  glTranslatef( 0, 0, -_thickness );
  if (_useDisplayList && (_displayListID!=0)) {
    glCallList( _displayListID );
  } else {
    _drawTess();
  }
  glPopMatrix();
  arTessVertices::iterator contourIter;
  for (contourIter =_contours.begin(); contourIter != _contours.end(); contourIter++) {
    std::vector<arVector3>& vertices = *contourIter;
    if (vertices.empty()) {
      continue;
    }
    for (unsigned int i = 1; i< vertices.size(); i++) {
      float* pos = vertices[i].v;
      float* lastPos = vertices[i-1].v;
      glBegin( GL_QUADS );
        glVertex3f( pos[0], pos[1], pos[2]+_thickness );
        glVertex3f( pos[0], pos[1], pos[2]-_thickness );
        glVertex3f( lastPos[0], lastPos[1], lastPos[2]-_thickness );
        glVertex3f( lastPos[0], lastPos[1], lastPos[2]+_thickness );
      glEnd();
    }
    arVector3& first = vertices.front();
    arVector3& last = vertices.back();    
    glBegin( GL_QUADS );
      glVertex3f( first[0], first[1], first[2]+_thickness );
      glVertex3f( first[0], first[1], first[2]-_thickness );
      glVertex3f( last[0], last[1], last[2]-_thickness );
      glVertex3f( last[0], last[1], last[2]+_thickness );
    glEnd();
  }
  glPopMatrix();
}

bool arGluTessObject::buildDisplayList() {
  if (_displayListID == 0) {
    _displayListID = glGenLists(1);
  }
  if (_displayListID == 0) {
    ar_log_error() << "arGluTessObject error: failed to allocate display list.\n";
    return false;
  }
  bool stat = _drawTess();
  _displayListDirty = false;
  return stat;
}

bool arGluTessObject::_drawTess() {
  if (_contours.empty()) {
    ar_log_error() << "arGluTessObject error: _drawTess() called with no contour data.\n"
         << "    Calling setVisible(false) to suppress further error messages.\n";
    return false;
  }
  arTessVertices::iterator contourIter;
  unsigned int totalVertices = 0;
  for (contourIter =_contours.begin(); contourIter != _contours.end(); contourIter++) {
    totalVertices += contourIter->size();
  }
  
  // Copy vertices into an array of doubles
  double* vertexStorage = new double[3*totalVertices];
  double* tempPtr = vertexStorage;
  unsigned int i;
  if (!vertexStorage) {
    ar_log_error() << "arGluTessObject error: memory panic.\n";
    return false;
  }
  for (contourIter =_contours.begin(); contourIter != _contours.end(); contourIter++) {
    std::vector<arVector3>& vertices = *contourIter;
    if (vertices.empty()) {
      ar_log_error() << "arGluTessObject warning: in _drawTess(), a contour is empty.\n";
      continue;
    }
    std::vector<arVector3>::iterator vertexIter;
    for (vertexIter = vertices.begin(); vertexIter != vertices.end(); vertexIter++) {
      for (i=0; i<3; i++)
        tempPtr[i] = (double)(vertexIter->v[i]);
      tempPtr += 3;
    }
  }
  tempPtr = vertexStorage;
  
  // Tesselate object into display list
  GLUtesselator* tess = gluNewTess();
  gluTessProperty( tess, GLU_TESS_WINDING_RULE, _windingRule );
  gluTessCallback( tess, GLU_TESS_BEGIN, (void (CALLBACK*)())beginCallback );
  gluTessCallback( tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK*)())vertexCallback );
  gluTessCallback( tess, GLU_TESS_COMBINE, (void (CALLBACK*)())combineCallback );
  gluTessCallback( tess, GLU_TESS_END, (void (CALLBACK*)())endCallback );
  gluTessCallback( tess, GLU_TESS_ERROR, (void (CALLBACK*)())errorCallback );
  if (_displayListID != 0) {
    glNewList( _displayListID, GL_COMPILE );
  }
  glShadeModel( GL_FLAT );    
  gluTessBeginPolygon( tess, (void*)_textureScaleInfo );
  for (contourIter =_contours.begin(); contourIter != _contours.end(); contourIter++) {
    unsigned int numVertices = contourIter->size();
    if (numVertices == 0)
      continue;
    gluTessBeginContour( tess );
    for (i=0; i<numVertices; i++) {
      gluTessVertex( tess, tempPtr, tempPtr );
      tempPtr += 3;
    }
    gluTessEndContour( tess );
  }
  gluTessEndPolygon(tess);
  if (_displayListID != 0) {
    glEndList();
  }
  gluDeleteTess( tess );
  delete[] vertexStorage;
  return true;
}

void arGluTessObject::addContour( std::vector< arVector3 >& newContour ) {
  _contours.push_back( newContour );
}

