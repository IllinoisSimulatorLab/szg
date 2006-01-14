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
visibility = None
billboard = None

#########################################
#              Classes                  #
#########################################

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
		if len(l) > 0:
			n = l[0]
			m = n.findByType("material")
			if m:
				mat = m.get()
				self.lastTouches.append( (mat, m) )
				# Must operate on a copy so that we do not modify the saved value!
				mat2 = arMaterial(mat)
				mat2.diffuse = arVector3(0,0,1)
				m.set(mat2)
		return
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
		self.oldX = 0
		self.oldY = 0
		self.headMatrix = ar_TM(0,5,0)
		self.wandMatrix = ar_TM(2,3,-1)
		# The states for the interface are:
		#    idle, rotating, moving_plane, moving_in_out, grabbing, head
		self.state = "idle"
	def onDraw(self):
		#global g
		#font = g.getTexFont()
		global font
		self.camera.loadViewMatrices()
		format = arTextBox()
		format.upperLeft = arVector3(-1,1,0)
		format.columns = 25
		format.width = 2
		format.lineSpacing = 1.2
		myText = "Interface choice:\n"
		if self.state == "idle":
			myText += "** "
		else:
			myText += "   "
		myText += "(1) Idle\n";
		if self.state == "rotating":
			myText += "** "
		else:
			myText += "   "
		myText += "(2) Rotate the wand\n";
		if self.state == "moving_plane":
			myText += "** "
		else:
			myText += "   "
		myText += "(3) Move wand in plane\n";
		if self.state == "moving_in_out":
			myText += "** "
		else:
			myText += "   "
		myText += "(4) Move wand in/out\n";
		if self.state == "grabbing":
			myText += "** "
		else:
			myText += "   "
		myText += "(5) Rotate and grab\n";
		if self.state == "head":
			myText += "** "
		else:
			myText += "   "
		myText += "(6) Move the head\n";
		
		font.renderString(myText, format)
	def onAdvance(self):
		self.getDriver().queueMatrix(0,self.headMatrix)
		self.getDriver().queueMatrix(1,self.wandMatrix)
		self.getDriver().sendQueue()
	def onKeyboard(self, key, state, x, y):
		# Note: the current implementation of standalone mode only sends key down events
		# to the simulator object. HOWEVER, to future-proof our implementation here, we
		# should make sure that we only register an event on key down (i.e. state is 1)
		if key == '1' or key == '2' or key == '3' or key == '4' or key == '6':
			self.getDriver().queueButton(0,0)
			self.getDriver().sendQueue()
		if key == '1' and state:
			self.state = 'idle'
		if key == '2' and state:
			self.state = 'rotating'
		if key == '3' and state:
			self.state = 'moving_plane'
		if key == '4' and state:
			self.state = 'moving_in_out'
		if key == '5' and state:
			self.state = 'grabbing'
			self.getDriver().queueButton(0,1)
			self.getDriver().sendQueue()
		if key == '6' and state:
			self.state = 'head'
	def onButton(self, button, state, x, y):
		global visibility
		self.oldX = x
		self.oldY = y
		if button == 0 and state == 1:
			visibility.set(1 - visibility.get())
	def onPosition(self, x, y):
		if self.state == 'head':
			self.headMatrix = ar_TM(0, 0, (y-self.oldY)*0.03)*self.headMatrix
		if self.state == "rotating" or self.state == "grabbing":
			self.wandMatrix = ar_ETM(self.wandMatrix)*ar_RM('y', (self.oldX-x)*0.01)*ar_ERM(self.wandMatrix)
		if self.state == 'moving_plane':
			self.wandMatrix = ar_TM((x-self.oldX)*0.03, (self.oldY-y)*0.03, 0)*ar_ETM(self.wandMatrix)*ar_ERM(self.wandMatrix)
		if self.state == 'moving_in_out':
			self.wandMatrix = ar_TM(0, 0, (y-self.oldY)*0.03)*ar_ETM(self.wandMatrix)*ar_ERM(self.wandMatrix)
		self.oldX = x
		self.oldY = y
		
		

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
	l = r.new("light")
	light = arLight()
	light.lightID = 2
	light.position = arVector4(0,1,0,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.set(light)
	
def addBillboard(r):
	global visibility
	global billboard
	t = r.new("transform")
	t.set(ar_TM(0,7.3,-3)*ar_SM(0.2,0.2,0.2))
	visibility = t.new("visibility")
	visibility.set(1)
	billboard = visibility.new("billboard")
	billboard.set("This app demonstrates how to construct\ncustom manipulation interfaces.\nPress button 1 to toggle this message.")

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
wand.touching = 1
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

# Want this to be drawn last so that transparency works.
# The scene graph draws things via a depth first search, with first-added children (of a particular node)
# drawn before the later-added children. Consequently, since the billboard should be drawn last
# (transparent things should be drawn last), we want to add it to the scene graph last.
addBillboard(root)
		
if f.start() != 1:
	sys.exit()
#font = g.getTexFont()
if f.getStandalone():
	textPath = f.getSZGClient().getAttribute("SZG_RENDER","text_path")
	fontFile = ar_fileFind("courier-bold.ppm", "", textPath)
	if font.load(fontFile) < 0:
		print("Error in loading font.")
	
while 1:
	f.processEventQueue()
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	time.sleep(0.02)
	f.swapBuffers()
