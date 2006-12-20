//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include <stdio.h>
#include <stdlib.h>
#include "arMasterSlaveFramework.h"
#include "arInteractableThing.h"
#include "arInteractionUtilities.h"
#include "arGlut.h"

// OOPified skeleton.cpp. Subclasses arMasterSlaveFramework and overrides its
// on...() methods (as opposed to installing callback functions).

// Unit conversions.  Tracker (and cube screen descriptions) use feet.
// Atlantis, for example, uses 1/2-millimeters, so the appropriate conversion
// factor is 12*2.54*20.
const float FEET_TO_LOCAL_UNITS = 1.;

// Near & far clipping planes.
const float nearClipDistance = .1*FEET_TO_LOCAL_UNITS;
const float farClipDistance = 100.*FEET_TO_LOCAL_UNITS;
  
// Class definitions & imlpementations. We'll have just one one class, a 2-ft colored square that
// can be grabbed & dragged around. We'll also have an effector class for doing the grabbing
//
class ColoredSquare : public arInteractableThing {
  public:
    // set the initial position of any ColoredSquare to 5 ft up & 2 ft. back
    ColoredSquare() : arInteractableThing() {}
    ~ColoredSquare() {}
    void draw( arMasterSlaveFramework* fw=0 );
};
void ColoredSquare::draw( arMasterSlaveFramework* /*fw*/ ) {
  glPushMatrix();
    glMultMatrixf( getMatrix().v );
    // set one of two colors depending on if this object has been selected for interaction
    if (getHighlight()) {
      glColor3f( 0,1,0 );
    } else {
     glColor3f( 1,1,0 );
    }
    // draw a 2-ft upright square
    glBegin( GL_QUADS );	
      glVertex3f( -1,-1,0 );
      glVertex3f( -1,1,0 );
      glVertex3f( 1,1,0 );
      glVertex3f( 1,-1,0 );
    glEnd();
  glPopMatrix();
}

class RodEffector : public arEffector {
  public:
    // set it to use matrix #1 (#0 is normally the user's head) and 3 buttons starting at 0 
    // (no axes, i.e. joystick-style controls; those are generally better-suited for navigation
    // than interaction)
    RodEffector() : arEffector( 1, 3, 0, 0, 0, 0, 0 ) {

      // set length to 5 ft.
      setTipOffset( arVector3(0,0,-5) );

      // set to interact with closest object within 1 ft. of tip
      // (see interaction/arInteractionSelector.h for alternative ways to select objects)
      setInteractionSelector( arDistanceInteractionSelector(2.) );

      // set to grab an object (that has already been selected for interaction
      // using rule specified on previous line) when button 0 or button 2
      // is pressed. Button 0 will allow user to drag the object with orientation
      // change, button 2 will allow dragging but square will maintain orientation.
      // (see interaction/arDragBehavior.h for alternative behaviors)
      // The arGrabCondition specifies that a grab will occur whenever the value
      // of the specified button event # is > 0.5.
      setDrag( arGrabCondition( AR_EVENT_BUTTON, 0, 0.5 ), arWandRelativeDrag() );
      setDrag( arGrabCondition( AR_EVENT_BUTTON, 2, 0.5 ), arWandTranslationDrag() );
    }
    // Draw it. Maybe share some data from master to slaves.
    // The matrix can be gotten from updateState() on the slaves' postExchange().
    void draw() const;
  private:
};
void RodEffector::draw() const {
  glPushMatrix();
    glMultMatrixf( getCenterMatrix().v );
    // draw grey rectangular solid 2"x2"x5'
    glScalef( 2./12, 2./12., 5. );
    glColor3f( .5,.5,.5 );
    glutSolidCube(1.);
    // superimpose slightly larger black wireframe (makes it easier to see shape)
    glColor3f(0,0,0);
    glutWireCube(1.03);
  glPopMatrix();
}

// End of classes



class SkeletonFramework: public arMasterSlaveFramework {
  public:
    SkeletonFramework();
    virtual bool onStart( arSZGClient& SZGClient );
    virtual void onWindowStartGL( arGUIWindowInfo* );
    virtual void onPreExchange( void );
    virtual void onPostExchange( void );
//    virtual void onWindowInit( void );
    virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
//    virtual void onDisconnectDraw( void );
//    virtual void onPlay( void );
    virtual void onWindowEvent( arGUIWindowInfo* );
//    virtual void onCleanup( void );
//    virtual void onUserMessage( const string& messageBody );
//    virtual void onOverlay( void );
//    virtual void onKey( unsigned char key, int x, int y );
//    virtual void onKey( arGUIKeyInfo* );
//    virtual void onMouse( arGUIMouseInfo* );
  private:
    // Our single object and effector
    ColoredSquare _square;
    RodEffector _effector;

    // Master-slave transfer variables
    // All we need to explicity transfer in this program is the square's placement matrix and
    // whether or not the it is highlighted (the effector's matrix can be updated by calling 
    // updateState(), see below).
    int _squareHighlightedTransfer;
    arMatrix4 _squareMatrixTransfer;
};


SkeletonFramework::SkeletonFramework() :
  arMasterSlaveFramework(),
  _squareHighlightedTransfer(0) {
}


// onStart callback method (called in arMasterSlaveFramework::start()
//
// Note: DO NOT do OpenGL initialization here; this is now called
// __before__ window creation. Do it in the onWindowStartGL()
//
bool SkeletonFramework::onStart( arSZGClient& /*cli*/ ) {
  // Register shared memory.
  //  framework.addTransferField( char* name, void* address, arDataType type, int numElements ); e.g.
  addTransferField("squareHighlighted", &_squareHighlightedTransfer, AR_INT, 1);
  addTransferField("squareMatrix", _squareMatrixTransfer.v, AR_FLOAT, 16);

  // Setup navigation, so we can drive around with the joystick
  //
  // Tilting the joystick by more than 20% along axis 1 (the vertical on ours) will cause
  // translation along Z (forwards/backwards). This is actually the default behavior, so this
  // line isn't necessary.
  setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );      

  // Tilting joystick left or right will rotate left/right around vertical axis (default is left/right
  // translation)
  setNavRotCondition( 'y', AR_EVENT_AXIS, 0, 0.2 );      

  // Set translation & rotation speeds to 5 ft/sec & 30 deg/sec (defaults)
  setNavTransSpeed( 5. );
  setNavRotSpeed( 30. );
  
  // set square's initial position
  _square.setMatrix( ar_translationMatrix(0,5,-6) );

  return true;
}

// Method to initialize each window (because now a Syzygy app can
// have more than one).
void SkeletonFramework::onWindowStartGL( arGUIWindowInfo* ) {
  // OpenGL initialization
  glClearColor(0,0,0,0);
}


// Method called before data is transferred from master to slaves. Only really makes
// sense to do anything here on the master. This is where anything having to do with
// processing user input or random variables should happen.
void SkeletonFramework::onPreExchange() {
  // Do stuff on master before data is transmitted to slaves.
  if (getMaster()) {

    // handle joystick-based navigation (drive around). The resulting
    // navigation matrix is automagically transferred to the slaves.
    navUpdate();

    // update the input state (placement matrix & button states) of our effector.
    _effector.updateState( getInputState() );

    // Handle any interaction with the square (see interaction/arInteractionUtilities.h).
    // Any grabbing/dragging happens in here.
    ar_pollingInteraction( _effector, (arInteractable*)&_square );

    // Pack data destined for slaves into appropriate variables
    // (bools transfer as ints).
    _squareHighlightedTransfer = (int)_square.getHighlight();
    _squareMatrixTransfer = _square.getMatrix();
  }
}

// Method called after transfer of data from master to slaves. Mostly used to
// synchronize slaves with master based on transferred data.
void SkeletonFramework::onPostExchange() {
  // Do stuff after slaves got data and are again in sync with the master.
  if (!getMaster()) {
    
    // Update effector's input state. On the slaves we only need the matrix
    // to be updated, for rendering purposes.
    _effector.updateState( getInputState() );

    // Unpack our transfer variables.
    _square.setHighlight( (bool)_squareHighlightedTransfer );
    _square.setMatrix( _squareMatrixTransfer.v );
  }
}

void SkeletonFramework::onDraw( arGraphicsWindow& /*win*/, arViewport& /*vp*/ ) {
  // Load the navigation matrix.
  loadNavMatrix();
  // Draw stuff.
  _square.draw();
  _effector.draw();
}

// This is how we have to catch reshape events now, still
// dealing with the fallout from the GLUT->arGUI conversion.
// Note that the behavior implemented below is the default.
void SkeletonFramework::onWindowEvent( arGUIWindowInfo* winInfo ) {
  // The values are defined in src/graphics/arGUIDefines.h.
  // arGUIWindowInfo is in arGUIInfo.h
  // The window manager is in arGUIWindowManager.h
  if (winInfo->getState() == AR_WINDOW_RESIZE) {
    const int windowID = winInfo->getWindowID();
#ifdef UNUSED
    const int x = winInfo->getPosX();
    const int y = winInfo->getPosY();
#endif
    const int width = winInfo->getSizeX();
    const int height = winInfo->getSizeY();
    getWindowManager()->setWindowViewport( windowID, 0, 0, width, height );
  }
}


int main(int argc, char** argv) {

  SkeletonFramework framework;
  // Tell the framework what units we're using.
  framework.setUnitConversion(FEET_TO_LOCAL_UNITS);
  framework.setClipPlanes(nearClipDistance, farClipDistance);

  if (!framework.init(argc, argv)) {
    return 1;
  }

  // Never returns unless something goes wrong
  return framework.start() ? 0 : 1;
}
