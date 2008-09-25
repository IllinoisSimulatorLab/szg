# A GLUT-based joystick simulator, to demonstrate how to write a GLUT GUI-based input
# device in Python. Drag the mouse around in the GLUT window to send joystick events.
# You would use it by:
# 1) Make sure you have defined SZG_PYTHON/executable for the computer it's on.
# 2) Place it in a directory on your SZG_PYTHON/path (or one level down).
# 3) Add it to a virtual computer input map, e.g.:
#        vcpyin  SZG_INPUT0  map  this_computer/event_console.py
#

import szg
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import sys
import thread


class JoystickSimulator( szg.arPyDeviceServerFramework ):
  def __init__(self):
    szg.arPyDeviceServerFramework.__init__(self)

    self.axes = [0.,0.]
    self.width = 0
    self.height = 0
    self.running = True

    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH)
    window = glutCreateWindow('glutjoystick.py')
    glutDisplayFunc( self.onDisplay )
    glutIdleFunc( self.onIdle )
    glutReshapeFunc( self.onReshape )
    glutKeyboardFunc( self.onKey )
    glutMotionFunc( self.onMotion )

  def configureInputNode(self):
    self.driver = szg.arGenericDriver()
    # signature = # buttons, # axes, # matrices
    self.driver.setSignature( 0,2,2 )
    # add...Mine() means you still own the installed object and nobody
    # else should try to delete it.
    self.getInputNode().addInputSourceMine( self.driver )
    # Look for another daisy-chained (via the network) input device, if
    # specified by the virtual computer definition.
    return self.addNetInput()

  def start(self):
    thread.start_new_thread( self.messageLoop, () )
    glutMainLoop()

  def messageLoop( self ):
    while self.running:
      messID, user, messType, messBody, context = self.getSZGClient().receiveMessage()
      if not messID:
        szg.ar_log_debug().write( "shutdown.\n" )
        # Calling sys.exit here causes the program to hang, so set a flag and let
        # the GLUT thread call sys.exit.
        self.running = False
      elif messType == "quit":
        szg.ar_log_debug().write( "exiting...\n" )
        self.running = False
      else:
        szg.ar_log_warning().write( 'ignoring ('+messType+','+messBody+') message.\n' )

  def onDisplay(self):
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT )

    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    glOrtho( -1, 1, -1, 1, -1, 1 )
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()
    glLineWidth(1.)

    # Marker for the joystick.
    glPushMatrix()
    glTranslatef( self.axes[0], self.axes[1], 0. )
    glColor3f(0.3, 1, 0)
    glutSolidSphere(0.1, 12, 12)
    glPopMatrix()

    glutSwapBuffers()


  def onKey( self, key, x, y ):
    if ord(key) == 27: # ESC
      sys.exit(0)

  def onIdle( self ):
    if not self.running:
      sys.exit(0)
    # send static head and wand matrices.
    self.driver.queueMatrix( 0, szg.ar_translationMatrix(0,5.5,0) )
    self.driver.queueMatrix( 1, szg.ar_translationMatrix(1,3.5,-1) )
    self.driver.sendQueue()
    glutPostRedisplay()

  def onReshape(self, width, height ):
    self.width = width
    self.height = height
    glViewport(0, 0, width, height)

  def onMotion( self, x, y ):
    def _clipem( coord ):
      if coord < -1.:
        return -1.
      if coord > 1.:
        return 1.
      return coord
    self.axes = map( _clipem, [2.*x/float(self.width)-1. , 1.-2.*y/float(self.height)] )
    glutPostRedisplay()
    self.driver.queueAxis( 0, self.axes[0] )
    self.driver.queueAxis( 1, self.axes[1] )
    self.driver.sendQueue()


def main():
  app = JoystickSimulator()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.start()


if __name__ == '__main__':
  main()
