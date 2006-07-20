#!/usr/bin/python

from PySZG import *
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import random

#### More complex master/slave framework skeleton example. Demonstrates
#### a method (based on the arMasterSlaveDict class in PySZG) for
#### synchronizing a dictionary of identical objects between master and slaves.
#### Clicking & holding button #1 creates a new square that you
#### can drag around until the button is released. Existing squares can
#### be dragged with button #0. Clicking button #2 deletes the
#### selected square. Clicking button #3 toggles it between solid and
#### wireframe.

# See szg/doc/Interaction.html for a discussion of user/object interaction
# concepts & classes

# Unit conversions.  Tracker (and cube screen descriptions) use feet.
# Atlantis, for example, uses 1/2-millimeters, so the appropriate conversion
# factor is 12*2.54*10.*2.
FEET_TO_LOCAL_UNITS = 1.


#### Class definitions & implementations. ####


# We'll have just one one interactable object class, a 2-ft colored square that
# can be grabbed & dragged around. We'll also have an effector class for doing the grabbing
#

# ColoredSquare: a yellow square that changes green when 'touched'
# (i.e. when the effector tip is within 1 ft.) and can be dragged around.
# See szg/src/interaction/arInteractable.h.
class ColoredSquare(arPyInteractable):
  def __init__(self, container=None):
    # must call superclass init so that on...() methods get called
    # when appropriate.
    arPyInteractable.__init__(self)
    self._container = container

  def __del__(self):
    print 'ColoredSquare.__del__()',self
    # VERY important to call superclass destructor!!!
    arPyInteractable.__del__(self)

  # This will get called on each frame in which this object is selected for
  # interaction ('touched')
  def onProcessInteraction( self, effector ):
    # button 2 deletes this square from the dictionary
    if effector.getOnButton(2):
      # NOTE! self.clearCallbacks() is necessary here. Why? Because the arPyInteractable
      # installs some of its own methods as its callbacks when it is instantiated. This means
      # that it has several references to itself, which in turn means that unless we
      # manually clear the callbacks (clearCallbacks() sets all the callbacks to None),
      # the object's reference
      # count will never go to zero, it won't be deleted, and it won't tell the effector
      # to release it. So for the moment, whenever you want to delete an arPyInteractable
      # or instance of a sub-class thereof, you must call clearCallbacks() as well as
      # getting rid of any explicit references you may have. Eventually, I'll probably
      # re-write this stuff in Python, at which point this problem will go away.
      self.clearCallbacks()
      self._container.delValue(self)
    # button 3 toggles visibility (actually solid/wireframe)
    if effector.getOnButton(3):
      self.setVisible( not self.getVisible() )

  def draw( self, framework=None ):
    glPushMatrix()
    glMultMatrixf( self.getMatrix().toTuple() )
    glScalef( 2., 2., 1./12. )
    if self.getVisible():
      # set one of two colors depending on if this object has been selected for interaction
      if self.getHighlight():
        glColor3f( 0,1,0 )
      else:
        glColor3f( 1,1,0 )
      # draw rectangular solid 2'x2'x1'
      glutSolidCube(1.)
    # superimpose slightly larger white wireframe
    glColor3f(1,1,1)
    glutWireCube(1.03)
    glPopMatrix()

  # These two routines are required by the arMasterSlaveDict. All
  # state that must be shared between master and slaves has to be packed
  # into a tuple for each object. Note that the __init__ _MUST_ be able
  # to accept the same tuple as an argument.
  def getState(self):
    return (self.getHighlight(),self.getVisible(),self.getMatrix().toTuple())

  def setState( self, stateTuple ):
    self.setHighlight( stateTuple[0] )
    self.setVisible( stateTuple[1] )
    self.setMatrix( arMatrix4(stateTuple[2]) )


# RodEffector: an effector that uses input matrix 1 for its position/orientatin
# and has 6 buttons starting at index 0. It is visually and functionally a 5-ft.
# rod with the hot spot at the tip (see szg/src/interaction/arEffector.h)
class RodEffector( arEffector ):
  def __init__(self):
    arEffector.__init__(self,1,6,0,0,0)

    # set length to 5 ft.
    self.setTipOffset( arVector3(0,0,-5) )

    # set to interact with closest object within 1 ft. of tip
    # (see PySZG.py for alternative classes for selecting objects)
    self.setInteractionSelector( arDistanceInteractionSelector(1.) )

    # set to grab an object (that has already been selected for interaction
    # using rule specified on previous line) when button 0 
    # is pressed and held. Button 0 will allow user to drag the object with orientation
    # change. The arGrabCondition specifies that a grab will occur whenever the value
    # of the specified button event is > 0.5.
    self.setDrag( arGrabCondition( AR_EVENT_BUTTON, 0, 0.5 ), arWandRelativeDrag() )

  def draw(self):
    glPushMatrix()
    glMultMatrixf( self.getCenterMatrix().toTuple() )
    # draw grey rectangular solid 2"x2"x5'
    glScalef( 2./12, 2./12., 5. )
    glColor3fv( (.5,.5,.5) )
    glutSolidCube(1.)
    # superimpose slightly larger black wireframe (makes it easier to see shape)
    glColor3fv( (0.,0.,0.) )
    glutWireCube(1.03)
    glPopMatrix()


# arMasterSlaveDict is a dictionary of objects to be shared between master
# and slaves. Dictionary keys MUST be Ints, Floats, or Strings.
# The second constructor argument contains information about the classes the
# dictionary can contain, used to construct new objects in the slaves.
# In this particular instance, the value after the colon is a reference to
# the ColoredSquare class (remember, in Python a class is just an object that
# generates instances when called with ()).
# See arMasterSlaveDict.__doc__ for more information.
# Note that you can have multiple arMasterSlaveDicts with different names in one
# application, if desired.
class SquareDict(arMasterSlaveDict):
  def __init__( self ):
    arMasterSlaveDict.__init__( self, 'objects', [ColoredSquare] )
  def makeSquare( self, matrix ):
    b = ColoredSquare(self)
    b.setMatrix( matrix )
    self.push(b)


#####  The application framework ####
#
#  The arPyMasterSlaveFramework automatically installs certain of its methods as the
#  master/slave callbacks, e.g. the draw callback is onDraw(), etc.

class SkeletonFramework(arPyMasterSlaveFramework):
  def __init__(self):
    arPyMasterSlaveFramework.__init__(self)

    self.dictionary = SquareDict()

    # Instantiate our effector
    self.theWand = RodEffector()

    # Tell the framework what units we're using.
    self.setUnitConversion( FEET_TO_LOCAL_UNITS )

    # Near & far clipping planes.
    nearClipDistance = .1*FEET_TO_LOCAL_UNITS
    farClipDistance = 100.*FEET_TO_LOCAL_UNITS
    self.setClipPlanes( nearClipDistance, farClipDistance )


  #### Framework callbacks -- see szg/src/framework/arMasterSlaveFramework.h ####

  # start (formerly init) callback (called in arMasterSlaveFramework::start())
  #
  def onStart( self, client ):
    # Necessary to call base class method for interactive prompt
    # functionality to work (run this program with '--prompt').
    arPyMasterSlaveFramework.onStart( self, client )

    # Register the dictionary of objects to be shared between master & slaves
    self.dictionary.start( self )

    # Setup navigation, so we can drive around with the joystick
    #
    # Tilting the joystick by more than 20% along axis 1 (the vertical on ours) will cause
    # translation along Z (forwards/backwards). This is actually the default behavior, so this
    # line isn't necessary.
    self.setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 )      

    # Tilting joystick left or right (axis 0) will rotate left/right around vertical axis (default is left/right
    # translation)
    self.setNavRotCondition( 'y', AR_EVENT_AXIS, 0, 0.2 )      

    # Set translation & rotation speeds to 5 ft/sec & 30 deg/sec (defaults)
    self.setNavTransSpeed( 5. )
    self.setNavRotSpeed( 30. )
    
    if self.getMaster():
      # Create a square and add it to the dictionary
      self.dictionary.makeSquare( ar_translationMatrix(0.,5.,-6) )

    return True


  def onWindowStartGL( self, winInfo ):
    # OpenGL initialization
    glClearColor(0,0,0,0)


  # Callback called before data is transferred from master to slaves. Now
  # called _only on the master_. This is where anything having to do with
  # processing user input or random variables should happen. Normally, most
  # of the frame-by-frame work of the program should be done here.
  def onPreExchange( self ):
    # Necessary to call base class method for interactive prompt
    # functionality to work (run this program with '--prompt').
    arPyMasterSlaveFramework.onPreExchange(self)

    # handle joystick-based navigation (drive around). The resulting
    # navigation matrix is automagically transferred to the slaves.
    self.navUpdate()

    # update the input state (placement matrix & button states) of our effector.
    self.theWand.updateState( self.getInputState() )

    # create a new square and add it to the dictionary
    if self.theWand.getOnButton(1):
      self.dictionary.makeSquare( self.theWand.getMatrix() )

    # Handle any interaction with the squares (see interaction docs).
    # Any grabbing/dragging/deletion of squares happens in here.
    # Any objects in the dictionary that aren't instances of subclasses
    # of arPyInteractable will be ignored.
    self.dictionary.processInteraction( self.theWand )

    # Pack the state of the arMasterSlaveDict (and its contents) into the framework.
    # This generates a sequence of messages, starting with construction and deletion
    # messages for objects that have been inserted or delete from the dictionary,
    # and ending with state-change messages for each object currently in the
    # dictionary, based on the sequence returned by each object's getState().
    self.dictionary.packState( self )
    

  # Callback called after transfer of data from master to slaves. Mostly used to
  # synchronize slaves with master based on transferred data. Note that you normally
  # Shouldn't be doing a whole lot of computation here; the exception would be if
  # you have a complex state that can be computed from a relatively small number of
  # parameters, you might compute those parameters on the master, transfer them to
  # the slaves, & do the complex state computation in master and slaves here.
  def onPostExchange( self ):
    # Do stuff after slaves got data and are again in sync with the master.
    if not self.getMaster():
      
      # Update effector's input state. On the slaves we only need the matrix
      # to be updated, for rendering purposes.
      self.theWand.updateState( self.getInputState() )

      # Unpack the message queue and use it to update the set
      # of objects in the dictionary as well as the state of each (using
      # its setState() method).
      self.dictionary.unpackState( self )


  # Draw callback
  def onDraw( self, win, viewport ):
    # Load the [inverse of the] navigation matrix onto the OpenGL modelview matrix stack.
    self.loadNavMatrix()
    
    # Draw any objects in the dictionary with a 'draw' attribute.
    # (in this case, all of them).
    self.dictionary.draw()
    # Draw the effector.
    self.theWand.draw()



if __name__ == '__main__':
  import sys
  framework = SkeletonFramework()
  if not framework.init(sys.argv):
    raise PySZGException,'Unable to init framework.'
  print 'Framework inited.'
  # Never returns unless something goes wrong
  if not framework.start():
    raise PySZGException,'Unable to start framework.'
