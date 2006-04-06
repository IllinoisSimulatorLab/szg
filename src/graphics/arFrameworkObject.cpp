//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFrameworkObject.h"
#include "arGraphicsHeader.h"

arFrameworkObject::arFrameworkObject(){
  // NOTE: _changed should definitely start as false (so that a subsequent
  // call to internalDumpState on an owning arMasterSlaveDataRouter
  // (before an update call) will NOT dump this framework object's 
  // initial state. There's no reason to do so after all.
  _changed = false;
}

/// We set up a viewport upon which to compose our object. This is useful
/// for little widgets that will appear on top of the main visualization,
/// sort of like in sub windows. The coordinates are window-relative (i.e.
/// positive numbers between 0 and 1).
void arFrameworkObject::preComposition(float lowerX, float lowerY,
	  			       float widthX, float widthY){
  // In order to be able to both see the application and the widget,
  // the application's render is faded (after depth buffer is cleared).
  // So... the visual effect should be something like a faded square of the
  // application with a 3D overlay.
  glPushAttrib( GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
  glDisable( GL_TEXTURE_1D );
  glDisable( GL_TEXTURE_2D );
  // At least one OpenGL implementation 
  // barfs on pushing the lighting bit with glPushAttrib.
  // So do that by hand.
  _lightingOn = glIsEnabled(GL_LIGHTING) == GL_TRUE;
  glDisable( GL_LIGHTING );
  glGetIntegerv( GL_VIEWPORT, (GLint*)_viewport);
  const int left = int(_viewport[2]*lowerX);
  const int bottom = int(_viewport[3]*lowerY);
  const int width = int(_viewport[2]*widthX);
  const int height = int(_viewport[3]*widthY);
  glViewport( (GLint)left, (GLint)bottom, (GLsizei)width, (GLsizei)height );
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
  glColor4f(0,0,0,0.8);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1,1,-1,1,0,1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,0,1, 0,0,0, 0,1,0);
  glBegin(GL_QUADS);
  glVertex3f(-1,-1,-998);
  glVertex3f(-1,1,-998);
  glVertex3f(1,1,-998);
  glVertex3f(1,-1,-998);
  glEnd();
  glDisable(GL_BLEND);
}

void arFrameworkObject::postComposition(){
  // better restore the viewport and other pieces of the state
  glViewport( (GLint)_viewport[0], (GLint)_viewport[1], (GLint)_viewport[2],
              (GLint)_viewport[3]);
  if (_lightingOn)
    glEnable(GL_LIGHTING);
  glPopAttrib();
}

/// The arFrameworkObject method should be invoked by all classes that over-ride it.
/// It keeps track of whether or not the object needs to transfer its state.
void arFrameworkObject::update(){
  _changed = true;
}

/// If we've dumped data, don't need to do so again until update has been called again.
arStructuredData* arFrameworkObject::dumpData(){
  _changed = false;
  return NULL;
}
