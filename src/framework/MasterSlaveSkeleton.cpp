#include "arPrecompiled.h"
#include <stdio.h>
#include <stdlib.h>
#include "arMasterSlaveFramework.h"
#include "arInteractableThing.h"
#include "arInteractionUtilities.h"

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
      setInteractionSelector( arDistanceInteractionSelector(1.) );

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
    // Draw it. Note that this may require some sharing of data from master->slaves.
    // The matrix can be gotten by calling updateState() on the slaves in postExchange()
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

// Global variables - our single object and effector
ColoredSquare theSquare;
RodEffector theEffector;

// Master-slave transfer variables
// All we need to explicity transfer in this program is the square's placement matrix and
// whether or not the it is highlighted (the effector's matrix can be updated by calling 
// updateState(), see below).
int squareHighlightedTransfer = 0;
arMatrix4 squareMatrixTransfer;


// start callback (called in arMasterSlaveFramework::start()
//
bool start( arMasterSlaveFramework& framework, arSZGClient& cli ) {
  // Register shared memory.
  //  framework.addTransferField( char* name, void* address, arDataType type, int numElements ); e.g.
  framework.addTransferField("squareHighlighted", &squareHighlightedTransfer, AR_INT, 1);
  framework.addTransferField("squareMatrix", squareMatrixTransfer.v, AR_FLOAT, 16);

  // Setup navigation, so we can drive around with the joystick
  //
  // Tilting the joystick by more than 20% along axis 1 (the vertical on ours) will cause
  // translation along Z (forwards/backwards). This is actually the default behavior, so this
  // line isn't necessary.
  framework.setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );      

  // Tilting joystick left or right will rotate left/right around vertical axis (default is left/right
  // translation)
  framework.setNavRotCondition( 'y', AR_EVENT_AXIS, 0, 0.2 );      

  // Set translation & rotation speeds to 5 ft/sec & 30 deg/sec (defaults)
  framework.setNavTransSpeed( 5. );
  framework.setNavRotSpeed( 30. );
  
  // OpenGL initialization
  glClearColor(0,0,0,0);

  // set square's initial position
  theSquare.setMatrix( ar_translationMatrix(0,5,-5) );

  return true;
}

// Callback called before data is transferred from master to slaves. Only really makes
// sense to do anything here on the master. This is where anything having to do with
// processing user input or random variables should happen.
void preExchange( arMasterSlaveFramework& fw ) {
  // Do stuff on master before data is transmitted to slaves.
  if (fw.getMaster()) {

    // handle joystick-based navigation (drive around). The resulting
    // navigation matrix is automagically transferred to the slaves.
    fw.navUpdate();

    // update the input state (placement matrix & button states) of our effector.
    theEffector.updateState( fw.getInputState() );

    // Handle any interaction with the square (see interaction/arInteractionUtilities.h).
    // Any grabbing/dragging happens in here.
    ar_pollingInteraction( theEffector, (arInteractable*)&theSquare );

    // Pack data we have to transfer to slaves into appropriate variables
    // (note that we can't currently transfer bools as such, must use ints).
    squareHighlightedTransfer = (int)theSquare.getHighlight();
    squareMatrixTransfer = theSquare.getMatrix();
  }
}

// Callback called after transfer of data from master to slaves. Mostly used to
// synchronize slaves with master based on transferred data.
void postExchange( arMasterSlaveFramework& fw ) {
  // Do stuff after slaves got data and are again in sync with the master.
  if (!fw.getMaster()) {
    
    // Update effector's input state. On the slaves we only need the matrix
    // to be updated, for rendering purposes.
    theEffector.updateState( fw.getInputState() );

    // Unpack our transfer variables.
    theSquare.setHighlight( (bool)squareHighlightedTransfer );
    theSquare.setMatrix( squareMatrixTransfer.v );
  }
}

void display( arMasterSlaveFramework& fw ) {
  // Load the navigation matrix.
  fw.loadNavMatrix();

  // Draw stuff.
  theSquare.draw();
  theEffector.draw();
}

// Called if window is resized.
void reshape(int width, int height) {
  // This is the default behavior if this callback is omitted.
  // (in fact, it is omitted below. Would be framework.setReshapeCallback(reshape) ).
  glViewport(0,0,width,height);
}

int main(int argc, char** argv) {

  arMasterSlaveFramework framework;
  // Tell the framework what units we're using.
  framework.setUnitConversion(FEET_TO_LOCAL_UNITS);
  framework.setClipPlanes(nearClipDistance, farClipDistance);
  framework.setStartCallback(start);
  framework.setPreExchangeCallback(preExchange);
  framework.setPostExchangeCallback(postExchange);
  framework.setDrawCallback(display);
  // also setReshapeCallback(), setExitCallback(), setUserMessageCallback()
  // in demo/arMasterSlaveFramework.h


  if (!framework.init(argc, argv)) {
    return 1;
  }

  // Never returns unless something goes wrong
  return framework.start() ? 0 : 1;
}
