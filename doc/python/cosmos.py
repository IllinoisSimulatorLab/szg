####################################################################################################
# This program is inspired by the szg c++ demo cosmos.cpp, though some modifications have occured.
# It demonstrates how to construct a simple user interface from scratch, with the ability to
# manipulate (translate and rotate) the spinning torii, in addition to navigating around
# the scene.
# This is DISTINCT from using the Syzygy interaction framework, which is demo'ed in lorenz.py, for
# instance.
####################################################################################################

#############################################################################################
# Exercises:
# 1. Substitute a new navigator for the current one. It could operate via events 
#    (much like the manipulator) instead of polling. How about adding rotations to the 
#    navigator?
# 2. Write a new manipulator. Can you write one that uses polling instead of events?
#    How about a manipulator that does not translate the object? Or that uses
#    scaling to make the wand translation distance different from the resulting
#    object translation distance? How about adding the ability to scale the object?
# 3. Change the objects in the scene, eliminate light, or otherwise mess with the graphics.
#############################################################################################

# Import the Syzygy stub Python module.
from PySZG import *
# Need sys so we can parse command line arguments.
import sys
# Must measure wall clock time to get framerate independent animation.
import time
# If we use the "rays" they should be random colors.
import random

########################################
#    Initialize the global variables.  #
########################################

# trans1-trans4 are the scene graph nodes that hold the torii's transformations.
trans1 = None
trans2 = None
trans3 = None
trans4 = None
# Are we drawing rays? The default is no (but the command line arguments can change this).
useRays = 0
# The scene graph node that holds the points defining the radiating line segments (if such are used).
# NOTE: On slower computers (like a 600 MHz iBook), the framerate is actually limited by processing
# the ray points (but not, of course, in drawing them).
points = None
# If we are drawing rays, they start out at full length.
percentage = 1.0
# The visibility scene graph node associated with the billboard (so it can blink on and off).
viz = None
# The transfom node associated with the billboard.
billboardTrans = None
# A collection of objects that will receive input events and act on the scene graph. Only one
# is active in this sample code (a manipulator).
widgetList = []

# This class implements simple navigation. It is based on polling (though you could also come up
# with a navigator class based on input events, like the manipulator class below).
# The user travels through the scene following the wand direction by pushing the joystick forward
# and orthogonally to the wand direction by pushing the joystick from side to side.
# The class makes assumptions about the input device. Comments in the code explain what they are.
class navigator:
	# Constructor.
	def __init__(self):
		# The scene graph "navigation node". All geometry that will be effected by navigation
		# must be attached below this node. Note that it is possible to have some geometry
		# that does not move during navigation (think of a head's-up display or a tool the
		# user has).
		self.node = None
		# Must keep track of the time between updates so we can move at a fixed speed.
		self.lastTime = -1
		# The speed in units per second.
		self.speed = 1.0
		# The code assumes joystick axis values range between -1 and 1. Do not navigate
		# unless the absolute value of an axis is greater than threshold.
		self.threshold = 0.4

	# This function is called once per event loop. Thus, the navigator class works by "polling".
	# Must pass in the framework object (which gives us access to current input values) and the
	# current time.
	def update(self, framework, time):
		# If this is the first time calling the method, just set the time called and return.
		if self.lastTime < 0:
			self.lastTime = time
			return
		# Assumes that the "neutral" position of the wand (which is matrix 1) is pointed straight at 
		# the front CAVE wall (and that we are using the CAVE coordinate system).
		# ar_ERM is shorthand for ar_extractRotationMatrix. This is related to some other 
		wandDir = ar_ERM(framework.getMatrix(1))*arVector3(0,0,-1)
		wandTransDir = ar_ERM(framework.getMatrix(1))*arVector3(1,0,0)
		# Figure out the navigation deltas, based on the joystick values. This is another assumption 
		# about our input device. It should have two axes, each reporting values between -1 and 1. The
		# first axis should be horizontal and report 1 when pushed to the right. The second axis
		# should be vertical and report 1 when pushed forward.
		# To get around problems with the joystick not reporting exactly (0,0) in its neutral position,
		# there must be an action threshold.
		deltaX = framework.getAxis(0)
		deltaY = framework.getAxis(1)
		if abs(deltaX) < self.threshold:
			deltaX = 0
		else:
			deltaX = (deltaX - self.threshold) / (1.0 - self.threshold)
		if abs(deltaY) < self.threshold:
			deltaY = 0
		else:
			deltaY = (deltaY - self.threshold) / (1.0 - self.threshold)
		# t is the egocentric translation vector.
		t = (wandDir * deltaY + wandTransDir * deltaX) * (time - self.lastTime) * self.speed 
		# Since we are (in the scene graph context) moving the world instead, we must apply the inverse
		# transformation (egocentric motion to exocentric motion)
		self.node.set(ar_TM(-t)*self.node.get())
		self.lastTime = time
		
class manipulator:
	def __init__(self):
		self.node = None
		self.wandMatrixAtGrab = None
		self.objMatrixAtGrab = None
	def attemptGrab(self, matrix):
		if not self.node:
			return
		self.objMatrixAtGrab = self.node.get()
		self.wandMatrixAtGrab = matrix
	def attemptRelease(self):
		self.objMatrixAtGrab = None
	def attemptDrag(self, matrix):
		if self.objMatrixAtGrab:
			tmp = ar_ETM(self.wandMatrixAtGrab)
			trans = ar_ETM(matrix)*tmp.inverse()*ar_ETM(self.objMatrixAtGrab)
			rot = ar_ERM(matrix)*ar_ERM(self.wandMatrixAtGrab.inverse())*ar_ERM(self.objMatrixAtGrab)
			self.node.set(trans*rot)
	def update(self, framework, event):
		if event.getType() == AR_EVENT_BUTTON and event.getIndex() == 0:
			if event.getButton() == 1:
				self.attemptGrab(framework.getMatrix(1))
			else:
				self.attemptRelease()
		self.attemptDrag(framework.getMatrix(1))
		

# By default, the scene graph uses lighting. Consequently, we must add lights, otherwise everything will
# be dark!
def addLights(g):
	# Get the scene graph's root node. Our light nodes will be its children.
	r = g.getRoot()
	# Create a new light node as a child of the root.
	l = r.new("light")
	# Create a light object (which will be stored in the light node).
	light = arLight()
	# All lights must have an ID (0-7). This should be unique (if not, the last defined light w/ that ID is used).
	light.lightID = 0
	# Since the last entry is 0, this is a directional light (as in OpenGL). If the last position were 1,
	# it would be a positional light.
	light.position = arVector4(0,0,-1,0)
	# No ambient component to the light.
        light.ambient = arVector3(0,0,0)
	# Dim white diffuse lighting.
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	# Store the light in the scene graph node.
	l.setLight(light)
	# Create a new light node.
	l = r.new("light")
	# Create a new light.
	light = arLight()
	# Note that this lightID is different than 0 (the lightID of the previous light).
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	# Store the light in the scene graph node.
	l.setLight(light)

def attachLineSet(t):
	global points
	plist = []
	for i in range(150):
		p = arVector3(-5 + 10 * random.random(),
                              -5 + 10 * random.random(),
                              -5 + 10 * random.random())
		if p.magnitude() == 0:
			p = arVector3(0,0,1)
		p.normalize()
		plist.append(p)
		plist.append(arVector3(0,0,0))
	points = t.new("points")
	points.set(plist)
	clist = []
	for i in range(150):
		c = arVector4(random.random(),
                              random.random(),
                              random.random(),
                              1)
		clist.append(c)
		clist.append(arVector4(0,0,0,0))
	colors = points.new("color4")
	colors.set(clist)
	draw = colors.new("drawable")
	draw.set(("lines",150))

def lineChange(percentage):
	plist = points.get()
	for i in range(150):
		plist[2*i+1] = plist[2*i]*percentage
	points.set(plist)

def worldInit(t):
	global trans1
	global trans2
	global trans3
	global trans4
	global viz
	global billboardTrans
 	trans1 = t.new("transform")
 	tex = trans1.new("texture")
 	tex.set("WallTexture1.ppm")
 	m = tex.new("material")
 	mat = arMaterial()
 	mat.diffuse = arVector3(1,0.6,0.6)
 	m.set(mat)
  	torus = arTorusMesh(60,30,4,0.5)
	torus.attachMesh(m)

	trans2 = t.new("transform")
 	tex = trans2.new("texture")
 	tex.set("WallTexture2.ppm")
  	torus.reset(60,30,2,0.5)
	torus.attachMesh(tex)

	trans3 = t.new("transform")
 	tex = trans3.new("texture")
 	tex.set("WallTexture3.ppm")
  	torus.reset(60,30,1,0.5)
	torus.attachMesh(tex)

	trans4 = t.new("transform")
 	tex = trans4.new("texture")
 	tex.set("WallTexture4.ppm")
  	torus.reset(60,30,3,0.5)
	torus.attachMesh(tex)

	viz = trans1.new("visibility")
	viz.set(1)
	billboardTrans = viz.new("transform")
	billboardTrans.set(ar_TM(4.55,0,0)*ar_RM('x', -1.571)*ar_RM('y', -1.571)*ar_SM(0.1))
	billboard = billboardTrans.new("billboard")
	billboard.set(" myriad scene graph ")

def worldAlter(elapsedTime):
	global trans1
	global trans2
	global trans3
	global trans4
	global percentage
        trans1.set(ar_RM('x', elapsedTime *  1.6))
	trans2.set(ar_RM('y', elapsedTime *  3.1))
	trans3.set(ar_RM('z', elapsedTime *  2.1))
	trans4.set(ar_RM('z', elapsedTime * -5.5))
	billboardTrans.set(ar_RM('z', elapsedTime*0.02)*billboardTrans.get())
	if useRays:
		percentage -= 0.05
		if percentage < 0:
			percentage = 1
		lineChange(percentage)

# The event processing callback. All it does is grab the events that have queued
# since last call and send them to the items on the widget list. A very generic function.
def eventProcessing(framework, eventQueue):
	while not eventQueue.empty():
		e = eventQueue.popNextEvent()
		for w in widgetList:
			w.update(framework, e)
	return 1
	
##########################################################
#                     Main Program                       #
##########################################################

# This is a distributed scene graph program. We need a framework object to do the management.
f = arDistSceneGraphFramework()

# We want to do event-based interaction for the manipulator. See eventProcessing(...) function above.
f.setEventQueueCallback(eventProcessing)

# Distributed scene graph programs can run in one of two modes:
#  Automatic graphics buffer swap: In this case, the library
#   itself decides when the buffer swap should occur.
#   This is the default.
#  Manual graphics buffer swap: The programmer is responsible
#   for requesting a buffer swap. This is better for times
#   when precise control over the contents of successive frames
#   is desired.
# We choose manual swap (by setting auto swap to false). 
# This must occur BEFORE f.init(...).
f.setAutoBufferSwap(0)

# This initializes the framework, setting mode of operation and configuration.
# Configuration information comes from the Phleet (the Syzygy distributed OS).
# PLEASE NOTE: All Syzygy programs accept a collection of "special" Phleet args
# (of the form -szg foo=bar). These are stripped from the command line by init.
# Consequently, the framework init must come BEFORE argument parsing!
if f.init(sys.argv) != 1:
	sys.exit()

# Parse command line arguments. Valid ways to invoke the program:
#   python cosmos.py
#     Spinning torii with no rays. Better for slow machines. Computing the new ray sizes is actually inefficient in Python!
#   python cosmos.py 1
#     Spinning torii with rays. Similar to the original cosmos demo. Better for a modern (2005) machine.
if len(sys.argv) > 1:
	useRays = int(sys.argv[1])

# Get the scene graph database held by the framework object. All of our geometry, etc. goes in here.
g = f.getDatabase()
# Add lights to the scene. By default, lighting is turned on in the Myriad scene graph. So, without this
# statement, the scene would be dark!
addLights(g)
# Attach a transform node to the scene graph root node. This will be the "navigation node". All other scene
# graph geometry will attach to it and our "navigator" object will change its embedded transform in response
# to user action.
navNode = g.getRoot().new("transform")
# Attach a new transform node to the navigation node. This node places the scene in a convenient spot.
# NOTE: a path of transform nodes down the scene graph tree operates much like a sequence of matrices in the OpenGL stack.
worldTrans = r.new("transform")
# ar_TM is an abbreviation for ar_translationMatrix, which returns an arMatrix4 expresses the expected translation.
worldTrans.set(ar_TM(0,5,-5))
# Attach a transform node to worldTrans. This new node will be used to drag the objects around.
worldManip = worldTrans.new("transform")
# The torii are attached to the worldManip node. We want to be able to drag them around after all.
worldInit(worldManip)
# If rays are requested, attach them to the worldManip node as well (they should drag around with the torii).
if useRays:
	attachLineSet(worldManip)

# Create a navigator object and tell it the scene graph's navigation node.
nav = navigator()
nav.node = navNode
# Create a manipulator object and tell it the scene graph node to be manipulated.
manip = manipulator()
manip.node = worldManip
# The manipulator must be added to the widgetList (which contains all objects that will receive input events).
# See eventProcessing(...) above.
widgetList.append(manip)

# Starts all the services, windows, etc. NOTE: it makes the most sense to do
# this AFTER lengthy initializations of the scene graph (though you can issue
# this statement any time after initializing the framework).
if f.start() != 1:
	sys.exit()

# Just using the wall clock time for torus animation does not yield very smooth results. Hence this hack.
counter = 0
while 1:
	# The event processing callback is called from within this method
	f.processEventQueue()
	# Wall clock time seems OK for navigation.
	nav.update(f, time.clock())
	f.setViewer()
	worldAlter(counter)
	counter += 0.02
	f.swapBuffers()
