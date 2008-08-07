%Module szgopengl 0

%Import szg.sip

%ModuleHeaderCode
#include "arGraphicsHeader.h"
#include "arMath.h"
%End

void ar_glLoadMatrixf( arMatrix4& );
%MethodCode
  glLoadMatrixf( a0->v );
%End

void ar_glMultMatrixf( arMatrix4& );
%MethodCode
  glMultMatrixf( a0->v );
%End

void ar_glScalef( arVector3& );
%MethodCode
  float* v = a0->v;
  glScalef( v[0], v[1], v[2] );
%End

void ar_glTranslatef( arVector3& );
%MethodCode
  float* v = a0->v;
  glTranslatef( v[0], v[1], v[2] );
%End

void ar_glVertex2f( arVector2& );
%MethodCode
  glVertex2fv( a0->v );
%End

void ar_glVertex3f( arVector3& );
%MethodCode
  glVertex3fv( a0->v );
%End

void ar_glVertex4f( arVector4& );
%MethodCode
  glVertex4fv( a0->v );
%End

void ar_glNormal3f( arVector3& );
%MethodCode
  glNormal3fv( a0->v );
%End

void ar_glColor3f( arVector3& );
%MethodCode
  glColor3fv( a0->v );
%End

void ar_glColor4f( arVector4& );
%MethodCode
  glColor4fv( a0->v );
%End

void ar_glTexCoord2f( arVector2& );
%MethodCode
  glTexCoord2fv( a0->v );
%End

void ar_glRasterPos2f( arVector2& );
%MethodCode
  glRasterPos2fv( a0->v );
%End

void ar_glRasterPos3f( arVector3& );
%MethodCode
  glRasterPos3fv( a0->v );
%End

void ar_glRasterPos4f( arVector4& );
%MethodCode
  glRasterPos4fv( a0->v );
%End

void ar_glRectf( arVector2&, arVector2& );
%MethodCode
  glRectfv( a0->v, a1->v );
%End

