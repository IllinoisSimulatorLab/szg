#!/usr/bin/python
from szg import *
from OpenGL.GL import *
from OpenGL.GLU import *
import sys
import os

firstFrame = True

#### Master/slave framework skeleton example. Demonstrates usage of some
#### interaction classes. Puts up a yellow square that the user can touch
#### or drag in two different ways with a custom effector using button #0
#### and button #2.

# See szg/doc/Interaction.html for a discussion of user/object interaction
# concepts & classes

# Look for textures in a 'textures' subdirectory of the directory containing
# this program
TEXTURE_PATH = os.path.join( os.path.dirname(os.path.abspath(__file__)), 'textures' )


#### Class definitions & implementations. ####

# We'll have just one one interactable object class, a 2-ft colored square that
# can be grabbed & dragged around. We'll also have an effector class for doing the grabbing
#

# TexturedSquare: a yellow square that changes green when 'touched'
# (i.e. when the effector tip is within 1 ft.) and can be dragged around.
# See szg/src/interaction/arInteractableThing.h.
class TexturedSquare(arInteractableThing):
  def __init__( self, fw ):
    arInteractableThing.__init__( self )
    self.fw = fw
    self.texture = None
    
  def loadTexture( self, filePath, alphaFilePath=None ):
    self.texture = arTexture()
    if not self.texture.readImage( filePath, '', '' ):
      print 'Failed to load TexturedSquare texture'
      self.texture = None
      return
    if alphaFilePath is not None:
      if not self.texture.readImage( alphaFilePath, '', '', 1 ):
        print 'Failed to load TexturedSquare texture'
        self.texture = None
        return
    self.texture.setTextureFunc( GL_MODULATE )
    self.texture.mipmap(True)
    self.texture.repeating(True)

  def onTouch( self, effector ):
    self.setHighlight( True )
    return True

  def onUntouch( self, effector ):
    self.setHighlight( False )
    return True

  # This will get called on each frame in which this object is selected for
  # interaction ('touched')
  def onInteraction( self, effector ):
    # button 3 toggles visibility (actually solid/wireframe)
    # ...and makes the object speak
    if effector.getOnButton(3):
      self.setVisible( not self.getVisible() )
      self.fw.speak('ouch!')
    return True

  def draw(self):
    glPushMatrix()
    glMultMatrixf( self.getMatrix().toTuple() )
    glScalef( 2., 2., .01 )
    if self.getVisible():
      glEnable( GL_TEXTURE_2D )
      glEnable( GL_BLEND )
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA )
      # set one of two colors depending on if this object has been selected for interaction
      if self.getHighlight():
        glColor3f( 0,1,0 )
      else:
        glColor3f( 1,1,0 )
      if self.texture is not None:
        self.texture.activate()
      # draw rectangle 2'x2'
      glBegin( GL_QUADS )
      glTexCoord2f( 0., 0. )
      glVertex3f( -.5, -.5, 0. )
      glTexCoord2f( 1., 0. )
      glVertex3f( .5, -.5, 0. )
      glTexCoord2f( 1., 1. )
      glVertex3f( .5, .5, 0. )
      glTexCoord2f( 0., 1. )
      glVertex3f( -.5, .5, 0. )
      glEnd()
      if self.texture is None:
        self.texture.deactivate()
      glDisable( GL_BLEND )
      glDisable( GL_TEXTURE_2D )
    # superimpose slightly larger white wireframe
    glColor3f(1,1,1)
    ar_glutWireCube(1.03)
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
    self.setDrag( arGrabCondition( AR_EVENT_BUTTON, 1, 0.5 ), arWandTranslationDrag() )

  def draw(self):
    glPushMatrix()
    glMultMatrixf( self.getCenterMatrix().toTuple() )
    # draw grey rectangular solid 2"x2"x5'
    glScalef( 2./12, 2./12., 5. )
    glColor3f( .5,.5,.5 )
    ar_glutSolidCube(1.)
    # superimpose slightly larger black wireframe (makes it easier to see shape)
    glColor3f(0,0,0)
    ar_glutWireCube(1.03)
    glPopMatrix()


#####  The application framework ####
#
#  The arPyMasterSlaveFramework automatically installs certain of its methods as the
#  master/slave callbacks, e.g. the draw callback is onDraw(), etc.

class SkeletonFramework(arPyMasterSlaveFramework):
  # Unit conversions.  Tracker (and cube screen descriptions) use feet.
  # Atlantis, for example, uses 1/2-millimeters, so the appropriate conversion
  # factor is 12*2.54*10.*2.
  unitsPerFoot = 1.   # default to feet
  # Near & far clipping planes.
  nearClipDistance = .1*unitsPerFoot
  farClipDistance = 100.*unitsPerFoot
  def __init__(self):
    arPyMasterSlaveFramework.__init__(self)

    # Our single object and effector
    self.theSquare = TexturedSquare( self )
    self.theWand = RodEffector()

    # List of objects to interact with
    self.interactionList = [self.theSquare]


  #### Framework callbacks -- see szg/src/framework/arMasterSlaveFramework.h ####

  # start (formerly init) callback (called in arMasterSlaveFramework::start())
  # NOTE: now called before window is created, so no OpenGL initialization here.
  def onStart( self, client ):
    # Necessary to call base class method for interactive prompt
    # functionality to work (run this program with '--prompt').
    arPyMasterSlaveFramework.onStart( self, client )

    # Register variables to be shared between master & slaves
    self.initSequenceTransfer('transfer')

    #Setup navigation, so we can drive around with the joystick
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

    # set square's initial position
    self.theSquare.setMatrix( ar_translationMatrix(0,5,-6) )
    
    # Load square texture
    self.theSquare.loadTexture( os.path.join( TEXTURE_PATH, 'spiral.jpg' ), \
        os.path.join( TEXTURE_PATH, 'spiral_alpha.jpg' ) )

    return True
    
  # Callback for doing window OpenGL initialization. Called from framework.start().
  def onWindowStartGL( self, winInfo ):
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

    # Handle any interaction with the squares (see interaction docs).
    # Any grabbing/dragging/deletion of squares happens in here.
    ar_pollingInteraction( self.theWand, self.interactionList )

    if self.getOnButton(2):
      self.speak( "You've pressed button 2" )

    # Pack data we have to transfer to slaves into appropriate variables
    # What we end up with here is a tuple containing two Ints and a nested tuple
    # containing 16 Floats. At this end (i.e. in the master), any sequence type
    # is allowed (e.g. lists, numarray arrays, array module arrays, I don't know what
    # all else), but the elements of any sequence must be Ints, Floats, or Strings (or
    # a nested sequence). They don't all have to have the same type within a sequence,
    # but only those types are allowed.
    transferTuple = (self.theSquare.getHighlight(), self.theSquare.getVisible(), \
        self.theSquare.getMatrix().toTuple())
    self.setSequence( 'transfer', transferTuple )


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

      # Unpack our transfer sequence. Note that no matter what sequence types
      # you used at the input end in the master, they all come out as tuples here.
      # For example, if in the master you passed in a sequence that was a 3-element
      # numarray array of Floats, when you unpacked it here you'd have a 3-element
      # tuple of Floats.
      transferTuple = self.getSequence( 'transfer' )
      self.theSquare.setHighlight( transferTuple[0] )
      self.theSquare.setVisible( transferTuple[1] )
      self.theSquare.setMatrix( arMatrix4( transferTuple[2] ) )

  # Draw callback
  def onDraw( self, win, viewport ):
    global firstFrame
    if firstFrame:
      screen = viewport.getScreen()
      print 'Screen (camera) parameters:'
      print '  center:',screen.getCenter()
      print '  normal:',screen.getNormal()
      print '      up:',screen.getUp()
      print '    size:',screen.getWidth(), 'x', screen.getHeight()
      firstFrame = False
    # Load the [inverse of the] navigation matrix onto the OpenGL modelview matrix stack.
    self.loadNavMatrix()
    
    # Draw stuff.
    self.theSquare.draw()
    self.theWand.draw()

  def onKeyDown( self, key, ctrl, alt ):
    print 'Key down:',key,' ctrl=',ctrl,' alt=',alt

  def onKeyUp( self, key, ctrl, alt ):
    print 'Key up  :',key,' ctrl=',ctrl,' alt=',alt

if __name__ == '__main__':
  szgrun( appClass=SkeletonFramework )
