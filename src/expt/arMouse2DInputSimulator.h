//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MOUSE2D_INPUT_SIMULATOR_H
#define AR_MOUSE2D_INPUT_SIMULATOR_H

#include "arInputSimulator.h"
#include "arGraphicsScreen.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

// Changes from arInputSimulator:
// (1) invisible.
// (2) mouse scaling maps from position on real screen in pixels to position on
// virtual screen (arGraphicsScreen) in feet, i.e. moving the mouse all the way
// across the screen will cause the position of event #1 to move across the
// rectangle specified by the arGraphicsScreen (and in its plane).
// (3) Each key gets a button event (0-255), any mouse buttons start with button event
// index 256 and go up from there. Each physical mouse button may map to only one
// button event.
// (4) no axis events.

class SZG_CALL arMouse2DInputSimulator : public arInputSimulator {
 public:
  arMouse2DInputSimulator();
  virtual ~arMouse2DInputSimulator();

  virtual bool configure( arSZGClient& SZGClient );

  virtual bool setMouseButtons( std::vector<unsigned int>& mouseButtons );
  
  virtual void draw();
  virtual void drawWithComposition();

  // used to capture and process mouse/keyboard data
  virtual void keyboard(unsigned char key, int state, int x, int y);
  virtual void mouseButton(int button, int state, int x, int y);
  virtual void mousePosition(int x, int y);

 private:
  void _computeWandMatrix( int xMouse, int yMouse );
  int _windowDims[2];
  float _screenDims[2];
  arVector3 _center;
  arVector3 _up;
  arVector3 _right;
  arVector3 _headPosition;
};

#endif
