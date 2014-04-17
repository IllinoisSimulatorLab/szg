//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first non-comment line!
#include "arPrecompiled.h"
#include "arMouse2DInputSimulator.h"
#include "arGraphicsHeader.h"
#include "arSTLalgo.h"
#include "arSZGClient.h"

arMouse2DInputSimulator::arMouse2DInputSimulator() : arInputSimulator() {
  // set the values of the internal simulator state appropriately
  std::vector<unsigned int> buttons;
  buttons.push_back(0);
  buttons.push_back(2);
  if (!setMouseButtons( buttons )) {
    cerr << "arMouse2DInputSimulator warning: setMouseButtons() failed in constructor.\n";
  }

  // set the initial values of the simulated device appropriately
  _head.setPosition( arVector3(0,5.5,0) );
  _wand.setPosition( arVector3(2,3,-1) );
}

arMouse2DInputSimulator::~arMouse2DInputSimulator() {
  _mouseButtons.clear();
  _lastButtonEvents.clear();
  _newButtonEvents.clear();
}

bool arMouse2DInputSimulator::configure( arSZGClient& SZGClient ) {
  arSlashString mouseButtonString = SZGClient.getAttribute( "SZG_INPUTSIM", "mouse_buttons" );
  int numItems = mouseButtonString.size();
  int* mouseButtons = new int[numItems];
  if (!mouseButtons) {
    cerr << "arMouse2DInputSimulator error: new[] failed.\n";
    return false;
  }
  int numValues = ar_parseIntString( mouseButtonString, mouseButtons, numItems );
  if (numValues != numItems) {
    cerr << "arMouse2DInputSimulator warning: can't configure mouse buttons from SZG_INPUTSIM/mouse_buttons.\n"
         << "   Using default: 0 (left) and 2 (right).\n";
    delete[] mouseButtons;
  } else {
    std::vector<unsigned int> mouseButtonsVector;
    for (int i=0; i<numValues; ++i) {
      mouseButtonsVector.push_back( mouseButtons[i] );
    }
    delete[] mouseButtons;
    if (!setMouseButtons( mouseButtonsVector )) {
      cerr << "arMouse2DInputSimulator error: setMouseButtons() failed in configure().\n";
    }
  }

  if (!SZGClient.getAttributeInts( "SZG_INPUTSIM", "window_dimensions",
                                   _windowDims, 2)) {
    cerr << "arMouse2DInputSimulator error: failed to get SZG_INPUTSIM/window_dimensions.\n";
    return false;
  }
  
  arGraphicsScreen screen;
  if (!screen.configure( SZGClient )) {
    cerr << "arMouse2DInputSimulator error: arGraphicsScreen configuration failed.\n";
    return false;
  }
  if (!screen.getAlwaysFixedHeadMode()) {
    cerr << "arMouse2DInputSimulator error: use_fixed_head_mode must be set to always.\n";
    return false;
  }
  _center = screen.getCenter();
  arVector3 normal = screen.getNormal();
  _up = screen.getUp();
  _right = normal*_up;
  _headPosition = screen.getFixedHeadHeadPosition();
  _screenDims[0] = screen.getWidth();
  _screenDims[1] = screen.getHeight();
  _head.setPosition( _headPosition );
  cout << "arMouse2DInputSimulator remark: setting head matrix to\n" << _head << endl;
  return true;
}

bool arMouse2DInputSimulator::setMouseButtons( std::vector<unsigned int>& mouseButtons ) {
  if (!arInputSimulator::setMouseButtons( mouseButtons )) {
    return false;
  }
  setNumberButtonEvents( mouseButtons.size() );
  _driver.setSignature(_numButtonEvents,0,2);
}

/// The simulator can draw its current state. This can be used as a
/// stand-alone display, like in inputsimulator, or it can be used as an
/// overlay.
void arMouse2DInputSimulator::draw(){
  // nothing to draw.
}

/// Draws the display... but with a nice mode of composition, so that it can
/// be used as an overlay.
void arMouse2DInputSimulator::drawWithComposition() {
  // nothing to draw.
}

/// Process keyboard events to drive the simulated interface.
void arMouse2DInputSimulator::keyboard(unsigned char key, int, int x, int y) {
  // no key response
}

/// Process mouse button events to drive the simulated interface.
void arMouse2DInputSimulator::mouseButton(int button, int state, int x, int y){
  if (button < 0) {
    cerr << "arMouse2DInputSimulator error: button index < 0 in mouseButton().\n";
    return;
  }
  unsigned int buttonIndex = (unsigned int)button;
  std::map<unsigned int,int>::iterator findIter = _mouseButtons.find( buttonIndex );
  bool haveIndex = findIter != _mouseButtons.end();
  if (!haveIndex) {
    cerr << "arMouse2DInputSimulator warning: failed to find button index " << buttonIndex
         << " in index list.\n";
  }
  if (haveIndex) {
    _mouseButtons[buttonIndex] = state;
    unsigned int eventIndex(0);
    std::map<unsigned int,int>::iterator iter;
    for (iter = _mouseButtons.begin(); (iter != findIter) && (iter != _mouseButtons.end()); ++iter) {
      ++eventIndex;
    }
    if (eventIndex >= _newButtonEvents.size()) {
      cerr << "arMouse2DInputSimulator error: button event index out of range in mouseButton().\n";
      return;
    }
    _newButtonEvents[eventIndex] = state;
    _driver.queueButton(eventIndex, state);
  }
}

/// Process mouse movement events to drive the simulated interface.
void arMouse2DInputSimulator::mousePosition(int x, int y) {
  _computeWandMatrix( x, y );
}

void arMouse2DInputSimulator::_computeWandMatrix( int xMouse, int yMouse ) {
  float xFrac( (xMouse/(float)_windowDims[0])-.5 );
  float yFrac( -(yMouse/(float)_windowDims[1])+.5 );
  float xfMouse( _screenDims[0]*xFrac );
  float yfMouse( _screenDims[1]*yFrac );
  arVector3 tipPos = _center + xfMouse*_right + yfMouse*_up;
  arVector3 tipDirection = (tipPos-_headPosition).normalize();
  arMatrix4 rotMatrix = ar_rotateVectorToVector( arVector3(0,0,-1), tipDirection );
  _wand.setPosition( _headPosition );
  _wand.extractRotation( rotMatrix );
}

