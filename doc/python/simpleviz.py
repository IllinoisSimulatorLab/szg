########################################################################################
# Demonstrates how to use the Myriad scene graph for grabber-based user interaction.
# A collection of objects is randomly dispersed throughout space.
# Here, we have mixed default user navigation with custom object manipulation code.
########################################################################################

from PySZG import *
import sys
import time
import random

########################################################################################
# Exercises:
#  1. Can you use a bounding sphere hierarchy to have faster drawing and intersection
#     testing?
########################################################################################

#########################################
#          Global variables             #
#########################################

font = arTexFont()
widgetList = []

class tool:
	# Constructor.
	def __init__(self):
		self.grabNode = None
		self.drawNode = None
		self.grabButton = 0
		# The matrix held by the manipulation tool at the grab.
		self.wandMatrixAtGrab = None
		# The matrix held by the manipulated transform node at the grab.
		# This member is used as a flag indicating whether the object is currently grabbed.
		self.objMatrixAtGrab = None
		
	def attach(self, transformNode):
		self.drawNode = transformNode
				
	# Start grabbing the object.
	def attemptGrab(self, matrix):
		if not self.grabNode:
			return
		self.objMatrixAtGrab = self.grabNode.get()
		self.wandMatrixAtGrab = matrix

	# Release a grabbed object (if any is currently grabbed).
	def attemptRelease(self):
		# This data member is used as a flag indicating if the object is currently grabbed.
		self.objMatrixAtGrab = None

	# If an object is grabbed, change its transform in response to user interface events.
	def attemptDrag(self, matrix):
		# Check to see if the object is currently grabbed.
		if self.objMatrixAtGrab:
			# Recall that ar_ETM is shorthand for ar_extractTranslationMatrix and ar_ERM is shorthand
			# for ar_extractRotationMatrix. They assume the matrix can be written like T*R*S, where
			# S is a uniform scaling.
			tmp = ar_ETM(self.wandMatrixAtGrab)
			# Note how translation and rotation and treated seperately for our interaction technique.
			trans = ar_ETM(matrix)*tmp.inverse()*ar_ETM(self.objMatrixAtGrab)
			rot = ar_ERM(matrix)*ar_ERM(self.wandMatrixAtGrab.inverse())*ar_ERM(self.objMatrixAtGrab)
			# Don't forget to update the scene graph's transform node!
			if self.grabNode:
				self.grabNode.set(trans*rot)

	def update(self, framework, event):
		# Other valid event types are AR_EVENT_AXIS and AR_EVENT_MATRIX, with corresponding getAxis and getMatrix methods.
		if event.getType() == AR_EVENT_BUTTON and event.getIndex() == self.grabButton:
			# Button 0 (see getIndex above) has been pushed. Grab object.
			if event.getButton() == 1:
				self.attemptGrab(framework.getMatrix(1))
			# Button 0 has been released. Release object.
			else:
				self.attemptRelease()
		# Drag a grabbed object.
		self.attemptDrag(framework.getMatrix(1))
			
class wandTool(tool):
	# Constructor
	def __init__(self):
		tool.__init__(self)
		self.length = 5
		self.touching = 0
		self.touchButton = 1
		self.lastTouches = []
	def attach(self, transformNode):
		tool.attach(self, transformNode)
		c = arCubeMesh()	
		c.setTransform(ar_TM(0,0,-self.length/2.0)*ar_SM(0.5, 0.5, self.length))
		c.attachMesh(self.drawNode)
	def showTouched(self, l):
		for n in self.lastTouches:
			n[1].set(n[0])
		self.lastTouches = []
		for n in l:
			m = n.findByType("material")
			if m:
				mat = m.get()
				self.lastTouches.append( (mat, m) )
				# Must operate on a copy so that we do not modify the saved value!
				mat2 = arMaterial(mat)
				mat2.diffuse = arVector3(0,0,1)
				m.set(mat2)
	def update(self, framework, event):
		tool.update(self, framework, event)
		if event.getType() == AR_EVENT_MATRIX and event.getIndex() == 1:
			self.drawNode.set(event.getMatrix())
			if self.touching:
				b = arBoundingSphere()
				b.radius = 0.5
				b.position = self.drawNode.get()*arVector3(0,0,-self.length)
				l = framework.getDatabase().intersectSphere(b)
				self.showTouched(l)
		if event.getType() == AR_EVENT_BUTTON and event.getIndex() == self.touchButton:
			self.touching = event.getButton()
			
class simulator(arPyInputSimulator):
	def __init__(self):
		arPyInputSimulator.__init__(self)
		# SHOULD MAKE A DEFAULT CAMERA OBJECT!!!!!
		self.camera = arOrthoCamera()
		self.camera.setSides(-1,1,-1,1)
		self.camera.setNearFar(0,100)
		self.camera.setPosition(0,0,5)
		self.camera.setTarget(0,0,0)
		self.camera.setUp(0,1,0)
	def onDraw(self):
		global font
		self.camera.loadViewMatrices()
		format = arTextBox()
		format.columns = 20
		format.rows = 20
		format.width = 2
		format.lineSpacing = 2
		font.renderString("WWWWWW\nfoo\nBen Schaeffer rocks!", format)
		
		

def addLights(r):
	l = r.new("light")
	light = arLight()
	light.lightID = 0
	light.position = arVector4(0,0,-1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.set(light)
	l = r.new("light")
	light = arLight()
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.set(light)

# The event processing callback. All it does is grab the events that have queued
# since last call and send them to the items on the widget list. A very generic function.
# NOTE: callbacks of this type must return 0/1 (i.e. bool on the C++ side).
def eventProcessing(framework, eventQueue):
	while not eventQueue.empty():
		e = eventQueue.popNextEvent()
		for w in widgetList:
			w.update(framework, e)
	return 1

f = arDistSceneGraphFramework()
sim = simulator()
f.setInputSimulator(sim)
# We want to do event-based interaction for the tool. See eventProcessing(...) function above.
f.setEventQueueCallback(eventProcessing)
f.setAutoBufferSwap(0)
if f.init(sys.argv) != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
root = g.getRoot()
addLights(root)
toolTransform = root.new("transform")
wand = wandTool()
wand.attach(toolTransform)
widgetList.append(wand)
navNode = f.getNavNode()
world = navNode.new("transform")
rotator = tool()
rotator.grabNode = world
widgetList.append(rotator)
tlist = []
for i in range(8):
	for j in range(8):
		t = world.new("transform")
		t.set(ar_TM(-3.5+i, -3.5+j + 5, -4))
		tlist.append(t)
		scl = t.new("transform")
		scl.set(ar_SM(random.random()))
		bound = scl.new("bounding sphere")
		b = arBoundingSphere()
		b.position = arVector3(0,0,0)
		b.radius = 0.5
		bound.set(b)
		m = bound.new("material")
		mat = arMaterial()
		mat.diffuse = arVector3(1,0,0)
		mat.specular = arVector3(0.2, 0.2, 0.2)
		mat.emissive = arVector3(0,0,0.1)
		mat.exponent = 100
		m.set(mat)
		s = arSphereMesh()
		s.setAttributes(15)
		s.attachMesh(m)
		
if f.start() != 1:
	sys.exit()

# Must occur after the window gets created in start().
# BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG. THIS IS ONLY VALID IN STANDALONE MODE!
if font.loadFont("c:\\cygwin\\home\\schaeffr\\Texture\\Text\\courier.txf") < 0:
	print("Error in loading font.")
	
while 1:
	f.processEventQueue()
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	time.sleep(0.02)
	f.swapBuffers()
