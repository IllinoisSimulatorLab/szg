from PySZG import *
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from numarray import *
import cPickle

#### Master/slave framework skeleton example. Demonstrates usage of some
#### interaction classes. Puts up a yellow square that the user can touch
#### or drag in two different ways with a custom effector using button #0
#### and button #2.

# See szg/doc/Interaction.html for a discussion of user/object interaction
# concepts & classes

# Unit conversions.  Tracker (and cube screen descriptions) use feet.
# Atlantis, for example, uses 1/2-millimeters, so the appropriate conversion
# factor is 12*2.54*10.*2.
FEET_TO_LOCAL_UNITS = 1.


# Utility routine to convert an arMatrix4 to a numarray array
# (for glMultMatrixf)
def arMatrix4ToNumarray( mat ):
  result = array( [[mat[0],mat[1],mat[2],mat[3]],
                  [mat[4],mat[5],mat[6],mat[7]],
                  [mat[8],mat[9],mat[10],mat[11]],
                  [mat[12],mat[13],mat[14],mat[15]]] )
  return result


#### Class definitions & implementations. ####

# We'll have just one one interactable object class, a 2-ft colored square that
# can be grabbed & dragged around. We'll also have an effector class for doing the grabbing
#

# ColoredSquare: a yellow square that changes green when 'touched'
# (i.e. when the effector tip is within 1 ft.) and can be dragged around.
# See szg/src/interaction/arInteractable.h.
class ColoredSquare(arPyInteractable):

  def __init__(self):
    # must call superclass init so that on...() methods get called
    # when appropriate.
    arPyInteractable.__init__(self)

  # This will get called on each frame in which this object is selected for
  # interation ('touched')
  def onProcessInteraction( self, effector ):
    if effector.getOnButton(3):
      self.setVisible( not self.getVisible() )

  def draw(self):
    glPushMatrix()
    glMultMatrixf( arMatrix4ToNumarray(self.getMatrix()) )
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
    # using rule specified on previous line) when button 0 or button 2
    # is pressed and held. Button 0 will allow user to drag the object with orientation
    # change, button 2 will allow dragging but square will maintain fixed orientation.
    # The arGrabCondition specifies that a grab will occur whenever the value
    # of the specified button event # is > 0.5.
    self.setDrag( arGrabCondition( AR_EVENT_BUTTON, 0, 0.5 ), arWandRelativeDrag() )
    self.setDrag( arGrabCondition( AR_EVENT_BUTTON, 2, 0.5 ), arWandTranslationDrag() )

  def draw(self):
    glPushMatrix()
    glMultMatrixf( arMatrix4ToNumarray(self.getCenterMatrix()) )
    # draw grey rectangular solid 2"x2"x5'
    glScalef( 2./12, 2./12., 5. )
    glColor3f( .5,.5,.5 )
    glutSolidCube(1.)
    # superimpose slightly larger black wireframe (makes it easier to see shape)
    glColor3f(0,0,0)
    glutWireCube(1.03)
    glPopMatrix()


# An (optional) event filter. You would define & install one of these if
# for some reason you needed to be absolutely sure of never missing an
# input event (calling e.g. framework or effector getAxis() routines 
# only gives you the most recent value of each event). Installing an event
# filter gives you access to each individual event as it comes in.
# See PyInputEvents.i and PyEventFilter.i.
class SkeletonEventFilter(arPyEventFilter):
  def __init__(self,framework):
    arPyEventFilter.__init__(self,framework)
    print 'SkeletonEventFilter.__init__(), getFramework() = ', self.getFramework()
  def onEvent( self, event ):
    print event
    return True


#####  The application framework ####

# The only thing that's a little weird about this is that the 'self'
# and 'framework' arguments of several of the methods actually
# refer to the same object; that's just the way the master/slave
# callbacks are set up. I've started working on incorporating a
# framework subclass into the SWIG bindings to get rid of this,
# but it isn't working yet. So just ignore 'framework' for now.

class SkeletonFramework(arMasterSlaveFramework):
  def __init__(self):
    arMasterSlaveFramework.__init__(self)

    # Our single object and effector
    self.theSquare = ColoredSquare()
    self.theWand = RodEffector()

    # List of objects to interact with
    self.interactionList = [self.theSquare]

    # Master-slave transfer variables
    # All we need to explicity transfer in this program is the square's placement matrix and
    # whether or not the it is highlighted (the effector's matrix can be updated by calling 
    # updateState(), see below).
    self.transferDict = {}

    # Tell the framework what units we're using.
    self.setUnitConversion( FEET_TO_LOCAL_UNITS )

    # Near & far clipping planes.
    nearClipDistance = .1*FEET_TO_LOCAL_UNITS
    farClipDistance = 100.*FEET_TO_LOCAL_UNITS
    self.setClipPlanes( nearClipDistance, farClipDistance )

    # install framework callbacks
    self.setStartCallback( self.onStart )
    self.setPreExchangeCallback( self.onPreExchange )
    self.setPostExchangeCallback( self.onPostExchange )
    self.setDrawCallback( self.onDraw )

    # Optional event filter. Can't be installed until after framework.init(),
    # see __main__
    #self.eventFilter = SkeletonEventFilter(self)


  #### Framework callbacks -- see szg/src/framework/arMasterSlaveFramework.h ####

  # start (formerly init) callback (called in arMasterSlaveFramework::start())
  #
  def onStart( self, framework, client ):

    # Register variables to be shared between master & slaves
    self.initObjectTransfer('transfer')

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
    
    # OpenGL initialization
    glClearColor(0,0,0,0)

    # set square's initial position
    self.theSquare.setMatrix( ar_translationMatrix(0,5,-5) )

    return True


  # Callback called before data is transferred from master to slaves. Now
  # called _only on the master_. This is where anything having to do with
  # processing user input or random variables should happen.
  def onPreExchange( self, framework ):

    # handle joystick-based navigation (drive around). The resulting
    # navigation matrix is automagically transferred to the slaves.
    self.navUpdate()

    # update the input state (placement matrix & button states) of our effector.
    self.theWand.updateState( self.getInputState() )

    # Handle any interaction with the square (see PySZG.i).
    # Any grabbing/dragging happens in here.
    ar_processInteractionList( self.theWand, self.interactionList )

    # Pack data we have to transfer to slaves into appropriate variables
    # (note that we can't currently transfer bools as such, must use ints).
    self.transferDict['highlight'] = self.theSquare.getHighlight()
    self.transferDict['visible'] = self.theSquare.getVisible()
    self.transferDict['matrix'] = self.theSquare.getMatrix()
    self.setObject( 'transfer', self.transferDict ) 


  # Callback called after transfer of data from master to slaves. Mostly used to
  # synchronize slaves with master based on transferred data.
  def onPostExchange( self, framework ):
    # Do stuff after slaves got data and are again in sync with the master.
    if not self.getMaster():
      
      # Update effector's input state. On the slaves we only need the matrix
      # to be updated, for rendering purposes.
      self.theWand.updateState( self.getInputState() )

      # Unpack our transfer variables.
      # NOTE: The post-exchange callback is no longer called
      # on disconnected slaves (which used to cause an exception
      # here).
      self.transferDict = self.getObject( 'transfer' )
      self.theSquare.setHighlight( self.transferDict['highlight'] )
      self.theSquare.setVisible( self.transferDict['visible'] )
      self.theSquare.setMatrix( self.transferDict['matrix'] )

  # Draw callback
  def onDraw( self, framework ):
    # Load the navigation matrix.
    self.loadNavMatrix()
    
    # Draw stuff.
    self.theSquare.draw()
    self.theWand.draw()



if __name__ == '__main__':
  framework = SkeletonFramework()
  if not framework.init(sys.argv):
    raise PySZGException,'Unable to init framework.'

  # install event filter now.
  #framework.setEventFilter( framework.eventFilter )

  # Never returns unless something goes wrong
  if not framework.start():
    raise PySZGExcreption,'Unable to start framework.'