//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"

#include "arGlut.h"
#include "arGraphicsHeader.h"
#include "arInputSimulator.h"
#include "arLogStream.h"
#include "arSTLalgo.h"
#include "arSZGClient.h"


// Very uninteresting arInputSimulator subclass that just behaves
// exactly like the base class.
class arDefaultInputSimulator: public arInputSimulator {
 public:
  arDefaultInputSimulator();
  virtual ~arDefaultInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );

  virtual void draw();
  virtual void drawWithComposition();
  virtual void advance();

  // Mouse/keyboard input.
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

  virtual bool setMouseButtons( vector<unsigned>& mouseButtons );
};

// The plugin exposes only these two functions.
// The plugin interface (in arInputSimulatorFactory.cpp) calls baseType()
// to verify that this shared library is an input simulator plugin.
// It then calls factory() to instantiate an object.
// Further calls are made to that instance's methods.
#undef SZG_IMPORT_LIBRARY
#undef SZG_DO_NOT_EXPORT
#include "arCallingConventions.h"
extern "C" {
  SZG_CALL void baseType(char* buffer, int size)
    { ar_stringToBuffer("arInputSimulator", buffer, size); }
  SZG_CALL void* factory()
    { return (void*) new arDefaultInputSimulator(); }
}


arDefaultInputSimulator::arDefaultInputSimulator() : arInputSimulator() {}

arDefaultInputSimulator::~arDefaultInputSimulator() {
}

bool arDefaultInputSimulator::configure( arSZGClient& SZGClient ) {
  return arInputSimulator::configure( SZGClient );
}

// Possibly an overlay on a standalone app's window.
void arDefaultInputSimulator::draw() {
  arInputSimulator::draw();
}

// Overlay the display.  Not const because pre,postComposition can't be.
void arDefaultInputSimulator::drawWithComposition() {
  arInputSimulator::drawWithComposition();
}

// Called occasionally to post the current state of the simulated
// device to newly connected arInputNodes.
void arDefaultInputSimulator::advance() {
  arInputSimulator::advance();
}

// Process keyboard events.
void arDefaultInputSimulator::keyboard( unsigned char key, int state, int x, int y ) {
  arInputSimulator::keyboard( key, state, x, y );
}

// Process mouse button events.
void arDefaultInputSimulator::mouseButton(int button, int state, int x, int y) {
  arInputSimulator::mouseButton( button, state, x, y );
}

// Mouse moved.
void arDefaultInputSimulator::mousePosition(int x, int y) {
  arInputSimulator::mousePosition( x, y );
}

bool arDefaultInputSimulator::setMouseButtons( vector<unsigned>& mouseButtons ) {
  return arInputSimulator::setMouseButtons( mouseButtons );
}


